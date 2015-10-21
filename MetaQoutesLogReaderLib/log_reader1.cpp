#include "log_reader1.h"
#include "regex.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <string.h>

typedef lr::CLogReader clr;

inline
bool read_string(char *buf, unsigned bufsize, HANDLE f) {

	auto dwCurrentFilePosition = SetFilePointer(
		f, // must have GENERIC_READ and/or GENERIC_WRITE
		0, // do not move pointer
		NULL, // hFile is not large enough in needing this pointer
		FILE_CURRENT); // provides offset from current position

	DWORD buf_size_act = 0;
	if (!ReadFile(f, buf, bufsize, &buf_size_act, 0) || !buf_size_act) {
		auto err = GetLastError();
		
		return false;
	}
	
	unsigned long& offset = dwCurrentFilePosition;
	for (unsigned bi = 0; bi < buf_size_act; bi += 4) {
		if (buf[bi + 0] == '\n') { buf[bi + 0] = '\0'; break; }
		++offset;
		if (buf[bi + 1] == '\n') { buf[bi + 1] = '\0'; break; }
		++offset;
		if (buf[bi + 2] == '\n') { buf[bi + 2] = '\0'; break; }
		++offset;
		if (buf[bi + 3] == '\n') { buf[bi + 3] = '\0'; break; }
		++offset;
	}
	if (bufsize > buf_size_act) {
		buf[buf_size_act] = '\0';
	}
	++offset;
	SetFilePointer(
		f, // must have GENERIC_READ and/or GENERIC_WRITE
		offset, // do not move pointer
		NULL, // hFile is not large enough in needing this pointer
		FILE_BEGIN); // provides offset from current position
	return true;
}

int log_reader::test(const char* filename, const char* regex, bool printResult) {
	printf("\nCLogReader started\n regex = \"%s\"\n filename = \"%s\"\n\n", regex, filename);
	char matching_text[10000] = { 0 };
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
		if (printResult) {
			printf("%d: %s\n", sz, matching_text);
		}
	}
	printf("CLogReader finished, matched %d\n", sz);
	return	sz ? 0 : lr::NotMatching;
}

clr::CLogReader(const char* filename)
	: file(nullptr)
	, lastError(NoneError) {
	lastErrorAssert();
	::memset(this->filter, 0, sizeof(this->filter));
	::memset(this->fileName, 0, sizeof(this->fileName));
	if (!filename || !filename[0]) {
		this->lastError = ArgumentError;
		return;
	}
	const auto& filename_len = ::strlen(filename) +1;
	if (filename_len > sizeof(this->fileName)) {
		this->lastError = ArgumentError;
		return;
	}
	::memcpy_s(this->fileName, sizeof(fileName), filename, filename_len);
}
clr::~CLogReader() {
	lastError = lr::NoneError;
	this->Close();
}

bool clr::GetNextLine(char *buf, const int bufsize) {
	lastErrorAssert();
	if (!buf || !bufsize) {
		this->lastError = lr::ArgumentError;
		return false;
	}
	while (read_string(buf, bufsize, file)) {
		if (this->match(buf)) {
			return true;
		}
	}
	return false;
}

bool clr::Open() {
	lastErrorAssert();
	this->Close();
	this->file = CreateFile(this->fileName, GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);
	if (this->file == INVALID_HANDLE_VALUE) {
		this->lastError = lr::FileOpenError;
		return false;
	}
	assert(file && "ClogReader: logic uncorrect");
	return true;
}
void clr::Close() {
	lastErrorAssert();
	if (!this->file || this->file == INVALID_HANDLE_VALUE) {
		return;
	}
	if (!CloseHandle(this->file)) {
		this->lastError = lr::FileCloseError;
	}
	this->file = nullptr;
	assert(!file && "ClogReader: logic uncorrect");
}

bool clr::SetFilter(const char* filter) {
	this->lastErrorAssert();
	if (!filter || !filter[0]) {
		this->lastError = lr::ArgumentError;
		return false;
	}
	const auto& filter_len = ::strlen(filter) + 1;
	if (filter_len > sizeof(this->filter)) {
		this->lastError = lr::ArgumentError;
		return false;
	}
	::memcpy_s(this->filter, sizeof(this->filter), filter, filter_len);
	assert((filter_len == strlen(this->filter) +1) && "ClogReader: logic uncorrect");
	return true;
}

bool clr::match(const char* string) {
	return rx::match(this->filter, string) != 0;
}

void clr::lastErrorAssert() {
	if (lastError) {
		printf_s("last error: %d \n", lastError);
	}
	assert(!lastError && "ClogReader: has error");
}