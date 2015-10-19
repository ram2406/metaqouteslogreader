#include "log_reader.h"
#include "regex.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <string.h>

typedef lr::CLogReader clr;

int log_reader::test(const char* filename, const char* regex) {
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
	}
	
	return	sz ? 0 : lr::NotMatching;
}

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
		if (::fseek(data.file, res.position_in_file, SEEK_SET)) {
			this->data.lastError = lr::InnerError;
			return false;
		}

		if (::fgets(buf, sizeof(bufsize), data.file) == 0) {
			return false;
		}
		return true;
	}

	if (!threads.size()) {
		init_threads();
		if (!threads.size()) {
			this->data.lastError = lr::InnerError;
			return false;
		}
	}

	Result res;
	while (!results.Pop(res)) {
		if (shared_data.GetLastPos() == EOF && threads.size()) {
			threads.clear();	//threads finished
			if (!results.Pop(res)) {
				//not have new results
				return false;
			}
			break;
		}
		spec::Sleep(10);	//wait of threads
	}
	

	//move result into user
	if (::fseek(data.file, res.position_in_file, SEEK_SET)) {
		this->data.lastError = lr::InnerError;
		return false;
	}

	if (::fgets(buf, sizeof(bufsize), data.file) == 0) {
		return false;
	}

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

bool clr::match(const char* string) {
	return rx::match(this->data.filter, string) != 0;
}

void clr::lastErrorAssert() {
	if (data.lastError) {
		printf_s("last error: %d \n", data.lastError);
	}
	assert(!data.lastError && "ClogReader: has error");
}

namespace log_reader {
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
		} fg(param->data.fileName);

		char string_buf[1024];
		
		while (true) {
			int offset = param->shared_data.Shift();
			if (offset < 0) {
				return 0;
			}
			::fseek(fg.file, offset, SEEK_SET);
			for (unsigned si = 0; si < clr::SharedData::OffsetOfShift; ++si) {
				int pos = ::ftell(fg.file);
				if (::fgets(string_buf, sizeof(string_buf), fg.file) != 0) {
					if (rx::match(param->data.filter, string_buf)) {
						clr::Result res = { pos };
						param->results.Push(res);
					}
				}
			}
		}
		
		//fake
		return 0;
	}
}

void clr::init_threads() {
	const auto& thread_limit = spec::Hardware_concurrency();
	threads.resize(thread_limit);
	for (unsigned ti = 0; ti < thread_limit; ++ti) {
		threads[ti].Set(&lr::RegexThreadProc, &this->thread_param);
	}
}