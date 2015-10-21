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
		clr::ThreadParam* param = (clr::ThreadParam*)lpParam;
		clr::ThreadData& data = param->data;
		while (data.state != clr::FinishThread) {
			if( data.state == clr::NeedProcess) {
				unsigned string_pos = 0;
				for (unsigned bi = 0; bi < sizeof(data.stringBuffer); ++bi) {
				   char& c = data.stringBuffer[bi];
				   if (c == '\n') {
					   c = '\0';
					   char* str = data.stringBuffer + string_pos;
					   string_pos = bi + 1;
					   bool matched = rx::match(param->filter, str) > 0;
					   if (!matched) { continue; }
					   clr::Result res = { data.pos_in_file + (str - data.stringBuffer) };
					   param->results.Push(res);
				   }
				}
				data.state = clr::AlreadyProcessed;
			}
			
			spec::Sleep(10);
		}
		return 0;
	}

	DWORD WINAPI FileReadThreadProc(LPVOID lpParam) {
		clr::ReadFileThreadParam* rfThreadParam = (clr::ReadFileThreadParam*)lpParam;
		while (true) {
			if (rfThreadParam->state == clr::StartThread) {
				spec::Sleep(10);
				continue;
			}

			for (unsigned ti = 0
				, n = rfThreadParam->dataPerThread.size();
				ti < n; ++ti) {
				clr::ThraedWithData& td = rfThreadParam->dataPerThread[ti];
				const bool& isRestThread = td.param->data.state == clr::AlreadyProcessed
					|| td.param->data.state == clr::StartThread;
				if (!isRestThread) {
					continue;
				}
				unsigned long pos ;
				if (!rfThreadParam->file.ReadStrings(td.param->data.stringBuffer, sizeof(td.param->data.stringBuffer), pos)) {
					//file in the end
					for (unsigned tii = 0
						, nn = rfThreadParam->dataPerThread.size();
						tii < nn; ++tii) {
						clr::ThraedWithData& tdd = rfThreadParam->dataPerThread[ti];
						tdd.param->data.state = clr::FinishThread;
					}
					rfThreadParam->state = clr::FinishThread;
					return 0;
				}
				td.param->data.pos_in_file = pos;
				td.param->data.state = clr::NeedProcess;
			}
			//spec::Sleep(10);
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
	: read_thread_param(this->threads, this->data.file_for_seach) {
	data.lastError = NoneError;

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

	//init threads once
	if (!threads.size()) {
		init_threads();
		if (!threads.size()) {
			this->data.lastError = lr::InnerError;
			return false;
		}
		read_thread_param.state = clr::AlreadyProcessed;
		read_thread.Set(lr::FileReadThreadProc, &read_thread_param);
	}

	//try pop result from queue, while file reading was finished
	Result res;
	while (!results.Pop(res)) {
		if (read_thread_param.state == clr::FinishThread && threads.size()) {
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
	data.file_for_read.SetPosition(res.position_in_file);
	data.file_for_read.ReadString(buf, bufsize);

	return true;
}

bool clr::Open() {
	lastErrorAssert();
	this->Close();
	
	bool res = data.file_for_read.OpenForRead(data.fileName);
	if (!res) {
		data.lastError = lr::FileOpenError;
	}
	res = data.file_for_seach.OpenForRead(data.fileName);
	if (!res) {
		data.lastError = lr::FileOpenError;
	}
	assert(res && "ClogReader: logic uncorrect");
	return true;
}

void clr::Close() {
	lastErrorAssert();
	bool res = data.file_for_read.Close();
	if (!res) {
		data.lastError = lr::FileCloseError;
	}
	res = data.file_for_seach.Close();
	if (!res) {
		data.lastError = lr::FileCloseError;
	}
	assert(res && "ClogReader: logic uncorrect");
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
		ThraedWithData& tdata = threads[ti];
		auto* param = new clr::ThreadParam(this->results, this->data.filter);
		tdata.param.reset(param);
		tdata.thread.Set(lr::RegexThreadProc, param);
	}
}
