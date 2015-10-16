#include "log_reader.h"
#include "regex.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <string.h>

typedef lr::CLogReader clr;

int log_reader::test(const char* filename, const char* regex) {
	char matching_text[255] = {0};
	clr reader(filename);
	if (!reader.SetFilter(regex)) {
		return reader.GetLastError();
	}
	if (!reader.Open()) {
		return reader.GetLastError();
	}
	const auto& hasMatching = reader.GetNextLine(matching_text, sizeof(matching_text));
	//if (hasMatching) {
	//	printf_s("match: %s", matching_text);
	//}
	return	hasMatching 
			? 0 
			: lr::NotMatching;
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
	while (::fgets(buf, bufsize, this->file) != NULL) {
		if (this->match(buf)) {
			return true;
		}
	}
	return false;
}

bool clr::Open() {
	lastErrorAssert();
	this->Close();
	
	if (fopen_s(&this->file, this->fileName, lr::FileOpenMode)) {
		this->lastError = lr::FileOpenError;
		return false;
	}
	assert(file && "ClogReader: logic uncorrect");
	return true;
}
void clr::Close() {
	lastErrorAssert();
	if (!this->file) {
		return;
	}
	if (fclose(this->file)) {
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
	const auto& filter_len = ::strlen(filter) +1;
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