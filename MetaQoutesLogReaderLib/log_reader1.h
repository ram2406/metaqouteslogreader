#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

namespace log_reader {

	int test(const char* filename, const char* regex, bool printResult);
	const char FileOpenMode[] = "rb";
	enum ErrorCode {
		NoneError = 0,

		ArgumentError = 1,
		FileOpenError = 2,
		FileCloseError = 3,
		InnerError = 4,

		NotMatching = 5
	};

	class CLogReader
	{
		char filter[255];
		char fileName[255];
		HANDLE file;

		ErrorCode lastError;
		inline void lastErrorAssert();

		bool match(const char* string);
	public:
		CLogReader(const char* filename);
		~CLogReader();
		bool    Open();							 // открытие файла, false - ошибка
		void    Close();                         // закрытие файла

		bool    SetFilter(const char *filter);   // установка фильтра строк, false - ошибка

		// false - конец файла или ошибка
		bool    GetNextLine(char *buf,           // запрос очередной найденной строки, 
							const int bufsize);  // buf - буфер, bufsize - максимальная длина
		

		ErrorCode GetLastError() const { return lastError; }

	};

}

namespace lr = log_reader;