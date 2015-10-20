#include "log_reader.h"
#include "regex.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <string.h>

typedef lr::CLogReader clr;

namespace log_reader {

	//reading string from file
	inline
	void read_string(char *buf, unsigned bufsize, FILE* f) {
		for (unsigned bi = 0; bi < bufsize; ++bi) {
			const auto& c = fgetc(f);
			if (c == '\n' || c == EOF) {
				buf[bi] = '\0';
				break;
			}
			buf[bi] = c;
		}
	}


	inline 
	void log(int i) {
	#ifdef _DEBUG
		FILE* f;
		fopen_s(&f, "log.txt", "a");
		char s[128];
		_itoa_s(i, s, 10);
		fputs(s, f);
		fputc('\n', f);
		fclose(f);
	#endif
	}

	//async function
	//open file, reading while not EOF
	DWORD WINAPI RegexThreadProc(LPVOID lpParam) {
		clr::ThreadParam* param = (clr::ThreadParam*) lpParam;
		struct FileGuard {
			FILE* file;
			FileGuard(const char* filename) {
				::fopen_s(&file, filename, lr::FileOpenMode);
			}
			~FileGuard() {
				::fclose(file);
			}
		} fg(param->data.fileName);		//one FILE* per thread

		//for next string
		char string_buf[1024];

		while (true) {
			//get current offset and shift on fixed constant value (portion for a one thread)
			fpos_t offset = param->shared_data.Shift();
			if (offset < 0) {
				return 0;
			}
			//shifting in file
			if (::fsetpos(fg.file, &offset)) {
				return 0;
			}
			//for each iteration in portion
			for (unsigned si = 0; si < clr::SharedData::OffsetOfShift; ++si) {
				fpos_t pos;
				::fgetpos(fg.file, &pos);	//save current position
				lr::read_string(string_buf, sizeof(string_buf), fg.file);
				if (rx::match(param->data.filter, string_buf)) {
					clr::Result res = { pos };
					param->results.Push(res);	//insert saving position
				}
			}
		}

		//fake
		return 0;
	}
}

int log_reader::test(const char* filename, const char* regex, bool withPrintResult) {
	printf("\nCLogReader started\n regex = \"%s\"\n filename = \"%s\"\n\n", regex, filename);
	char matching_text[10000] = {0};
	clr reader(filename);
	if (!reader.SetFilter(regex)) {
		return reader.GetLastError();
	}
	if (!reader.Open()) {
		return reader.GetLastError();
	}
	unsigned sz = 0;
	
	
	while (reader.GetNextLine(matching_text, sizeof(matching_text))) {
		++sz;
		if (withPrintResult) {
			printf("%d: %s\n", sz, matching_text);
		}
	}
	printf("CLogReader finished, matched %d\n", sz);
	return	sz ? 0 : lr::NotMatching;
}

///////////// Public ///////////////
clr::CLogReader(const char* filename)
	: thread_param(data, shared_data, results) {
	data.lastError = NoneError;
	data.file = nullptr;
	shared_data.file = nullptr;
	shared_data.last_pos_in_file = 0;

	lastErrorAssert();
	::memset(this->data.filter, 0, sizeof(this->data.filter));
	::memset(this->data.fileName, 0, sizeof(this->data.fileName));
	if (!filename || !filename[0]) {
		this->data.lastError = ArgumentError;
		return;
	}
	const auto& filename_len = ::strlen(filename) +1;
	if (filename_len > sizeof(this->data.fileName)) {
		this->data.lastError = ArgumentError;
		return;
	}
	::memcpy_s(this->data.fileName, sizeof(data.fileName), filename, filename_len);
}

clr::~CLogReader() {
	data.lastError = lr::NoneError;
	threads.clear();
	this->Close();
}

bool clr::GetNextLine(char *buf, const int bufsize) {
	lastErrorAssert();
	if (!buf || !bufsize) {
		this->data.lastError = lr::ArgumentError;
		return false;
	}

	//if threads already finished, but has results
	if (shared_data.GetLastPos() == EOF && !threads.size()) {
		Result res;
		if (!results.Pop(res)) {
			//not have new results
			return false;
		}
		if (::fsetpos(data.file, &res.position_in_file)) {
			this->data.lastError = lr::InnerError;
			return false;
		}

		lr::read_string(buf, bufsize, data.file);
		return true;
	}

	//init threads once
	if (!threads.size()) {
		init_threads();
		if (!threads.size()) {
			this->data.lastError = lr::InnerError;
			return false;
		}
	}

	//try pop result from queue, while file reading was finished
	Result res;
	while (!results.Pop(res)) {
		if (shared_data.GetLastPos() == EOF && threads.size()) {
			//means file finished
			threads.clear();	//threads finished
			if (!results.Pop(res)) {
				//not have new results
				return false;
			}
			break;	//has result(s)
		}
		spec::Sleep(10);	//wait of threads
	}
	
	//for debug
	//lr::log(res.position_in_file);

	//move result into user
	if (::fsetpos(data.file, &res.position_in_file)) {
		this->data.lastError = lr::InnerError;
		return false;
	}
	
	lr::read_string(buf, bufsize, data.file);

	return true;
}

bool clr::Open() {
	lastErrorAssert();
	this->Close();
	
	if (fopen_s(&this->shared_data.file, this->data.fileName, lr::FileOpenMode)) {
		this->data.lastError = lr::FileOpenError;
		return false;
	}

	if (fopen_s(&this->data.file, this->data.fileName, lr::FileOpenMode)) {
		this->data.lastError = lr::FileOpenError;
		return false;
	}

	assert(this->shared_data.file && "ClogReader: logic uncorrect");
	return true;
}

void clr::Close() {
	lastErrorAssert();
	if (!this->shared_data.file) {
		return;
	}
	if (fclose(this->shared_data.file)) {
		this->data.lastError = lr::FileCloseError;
	}
	this->shared_data.file = nullptr;

	if (!this->data.file) {
		return;
	}
	if (fclose(this->data.file)) {
		this->data.lastError = lr::FileCloseError;
	}
	this->data.file = nullptr;

	assert(!this->shared_data.file && "ClogReader: logic uncorrect");
}

bool clr::SetFilter(const char* filter) {
	this->lastErrorAssert();
	if (!filter || !filter[0]) {
		this->data.lastError = lr::ArgumentError;
		return false;
	}
	const auto& filter_len = ::strlen(filter) +1;
	if (filter_len > sizeof(this->data.filter)) {
		this->data.lastError = lr::ArgumentError;
		return false;
	}
	::memcpy_s(this->data.filter, sizeof(this->data.filter), filter, filter_len);
	assert((filter_len == strlen(this->data.filter) +1) && "ClogReader: logic uncorrect");
	return true;
}

/////////// Private ////////////////
bool clr::match(const char* string) {
	return rx::match(this->data.filter, string) != 0;
}

void clr::lastErrorAssert() {
	if (data.lastError) {
		printf_s("last error: %d \n", data.lastError);
	}
	assert(!data.lastError && "ClogReader: has error");
}

void clr::init_threads() {
	const auto& thread_limit = spec::Hardware_concurrency();
	threads.resize(thread_limit);
	for (unsigned ti = 0; ti < thread_limit; ++ti) {
		threads[ti].Set(&lr::RegexThreadProc, &this->thread_param);
	}
}

//////////// SharedData ///////////
fpos_t clr::SharedData::GetLastPos() {
	cs::CLockGuard<spec::CMutex> lk(mutex);
	return last_pos_in_file;
}

fpos_t clr::SharedData::Shift() {
	cs::CLockGuard<spec::CMutex> lk(mutex);
	if (last_pos_in_file == EOF) {
		return last_pos_in_file;
	}
	for (unsigned i = 0; i < OffsetOfShift; ++i) {
		while (true) {
			char c = ::fgetc(file);
			if (c == '\n') {
				//move to next line
				break;
			}
			if (c == EOF) {
				//file is finished
				fpos_t old_pos = last_pos_in_file;
				last_pos_in_file = EOF;
				return old_pos;
			}
		}
	}
	fpos_t old_pos = last_pos_in_file;
	::fgetpos(file, &last_pos_in_file);	//set next position after OffsetOfShift lines
	return old_pos;	//return current position
}

/* unfortunate 
void* operator new  (std::size_t count, const std::nothrow_t& tag) {
	return ::GlobalAlloc(GMEM_FIXED, count);
}

void* operator new[](std::size_t count, const std::nothrow_t& tag) {
	return ::GlobalAlloc(GMEM_FIXED, count);
}

void* operator new  (std::size_t count){
	return ::GlobalAlloc(GMEM_FIXED, count);
}

void* operator new[](std::size_t count) {
	return ::GlobalAlloc(GMEM_FIXED, count);
}


void operator delete(void* ptr) {
	::GlobalFree(ptr);
}

void operator delete[](void* ptr) {
	::GlobalFree(ptr);
}
//*/