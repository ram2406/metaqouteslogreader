#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "thread_safe_queue.h"

namespace log_reader {

	//for behavior testing of class
	int test(const char* filename, const char* regex, bool withPrintResult);

	//mode for each FILE*:
	//CLogReader::data.file
	//SharedData::file
	//local in RegexThreadProc (for each threads)
	const char FileOpenMode[] = "rb";

	enum ErrorCode : unsigned char {
		NoneError = 0,

		ArgumentError = 1,
		FileOpenError = 2,
		FileCloseError = 3,
		InnerError = 4,

		NotMatching = 5
	};

	class CLogReader
	{
	public:
		///////////// Data /////////////////
		struct Data {
			char filter[255];
			char fileName[255];
			
			ErrorCode lastError;			
			spec::CFile file;
		};
		//////////// SharedData ////////////
		struct SharedData {
			friend class CLogReader;
			static const unsigned OffsetOfShift = 10000U;
		private:
			char strbuf[2048];
			long long last_pos_in_file;
			spec::CMutex mutex;
			spec::CFile file;
		public:
			inline	long long GetLastPos();
			inline	long long Shift();
		};
		///////////// Result ///////////////
		struct Result {
			long long position_in_file;
		};
		//////////// ThreadParam ////////////
		struct ThreadParam {

			const Data& data;
			SharedData& shared_data;
			cs::CThreadSafeQueue<Result>& results;

			ThreadParam	( const Data& data
						, SharedData& shared_data
						, cs::CThreadSafeQueue<Result>& results)
						: data(data)
						, shared_data(shared_data)
						, results(results) {
			}
			
		};
		////////////////////////////////////
	private:

		////////////// Members /////////////
		Data data;		//read async, write only in class
		SharedData shared_data;		// r/w in different threads
		cs::CVector<spec::CThread> threads;		//count == logic_core_count
		cs::CThreadSafeQueue<Result> results;	//thread-safe linked list

		ThreadParam thread_param;	//one for all threads

		////////////////////////////////////
		void init_threads();		//once
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
		

		ErrorCode GetLastError() const { return data.lastError; }
	};

}

namespace lr = log_reader;