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
		fpos_t offset;
		fgetpos(f, &offset);
		::fread(buf, 1, bufsize, f);
		
		for (unsigned bi = 0; bi < bufsize; bi += 4) {
			if (buf[bi + 0] == '\n') { buf[bi + 0] = '\0'; break; }
			++offset;
			if (buf[bi + 1] == '\n') { buf[bi + 1] = '\0'; break; }
			++offset;
			if (buf[bi + 2] == '\n') { buf[bi + 2] = '\0'; break; }
			++offset;
			if (buf[bi + 3] == '\n') { buf[bi + 3] = '\0'; break; }
			++offset;
		}
		++offset;
		::fsetpos(f, &offset);
	}


	//inline
	//void read_string(char *buf, unsigned bufsize, FILE* f) {
	//	for (unsigned bi = 0; bi < bufsize; bi += 4) {
	//		{
	//			const auto& c = fgetc(f);
	//			if (c == '\n' || c == EOF) {
	//				buf[bi + 0] = '\0';
	//				break;
	//			}
	//			buf[bi + 0] = c;
	//		}
	//		////////////////
	//		{
	//			const auto& c = fgetc(f);
	//			if (bufsize == bi) { break; }
	//			if (c == '\n' || c == EOF) {
	//				buf[bi + 1] = '\0';
	//				break;
	//			}
	//			buf[bi + 1] = c;
	//		}
	//		////////////////
	//		{
	//			const auto& c = fgetc(f);
	//			if (bufsize == bi) { break; }
	//			if (c == '\n' || c == EOF) {
	//				buf[bi + 2] = '\0';
	//				break;
	//			}
	//			buf[bi + 2] = c;
	//		}
	//		////////////////
	//		{
	//			const auto& c = fgetc(f);
	//			if (bufsize == bi) { break; }
	//			if (c == '\n' || c == EOF) {
	//				buf[bi + 3] = '\0';
	//				break;
	//			}
	//			buf[bi + 3] = c;
	//			////////////////
	//		}
	//	}
	//}


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
		spec::CFile file;
		file.OpenForRead(param->data.fileName);		//one FILE* per thread

		//for next string
		char string_buf[1024];

		while (true) {
			//get current offset and shift on fixed constant value (portion for a one thread)
			long long offset = param->shared_data.Shift();
			if (offset < 0) {
				return 0;
			}
			//shifting in file
			file.SetPosition((unsigned long)offset);
			//for each iteration in portion
			for (unsigned si = 0; si < clr::SharedData::OffsetOfShift; ++si) {
				fpos_t pos;
				pos = file.GetPosition();	//save current position
				file.ReadString(string_buf, sizeof(string_buf));
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
		
		data.file.SetPosition((unsigned long)res.position_in_file);
		data.file.ReadString(buf, bufsize);
		
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
	data.file.SetPosition((unsigned long)res.position_in_file);
	data.file.ReadString(buf, bufsize);

	return true;
}

bool clr::Open() {
	lastErrorAssert();
	this->Close();
	
	bool res = data.file.OpenForRead(data.fileName) 
		&& shared_data.file.OpenForRead(data.fileName);
	if (!res) {
		data.lastError = lr::FileOpenError;
		return false;
	}
	return true;
}

void clr::Close() {
	lastErrorAssert();
	bool res = data.file.Close() && shared_data.file.Close();
	if (!res) {
		data.lastError = lr::FileCloseError;
	}
	
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
	fpos_t old_pos = last_pos_in_file;
	
	unsigned string_count = 0;
	while (true) {
		unsigned long act_bufsize;
		if (!file.ReadBuffer(strbuf, sizeof(strbuf), act_bufsize)) {
			last_pos_in_file = EOF;
			return old_pos;
		}
		unsigned bi = 0;
		for (; bi < act_bufsize; ++bi) {
			if (strbuf[bi] == '\n') {
				++string_count;
				
				if (string_count == OffsetOfShift) {
					last_pos_in_file += bi;
					return old_pos;
				}
			}
		}
		last_pos_in_file += bi;
	}
	
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