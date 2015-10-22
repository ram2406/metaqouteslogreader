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
			spec::CFile file_for_read;
			spec::CFile file_for_seach;
		};

		///////////// Result ///////////////
		struct Result {
			unsigned long position_in_file;
		};
		//////////// ThreadParam ////////////
		enum ThreadState : unsigned char {
			StartThread = 0,
			FinishThread,
			NeedProcess,
			AlreadyProcessed,
			
		};
		struct ThreadData {
			static const unsigned StringsBufferLength = 4096U;
			ThreadState state;
			char stringBuffer[StringsBufferLength];
			unsigned long pos_in_file;

			ThreadData()
				: state(StartThread)
				, pos_in_file(0) {
			}
		};
		
		//////////// ThreadParam ////////////
		struct ThreadParam {
			ThreadData data;
			cs::CThreadSafeQueue<Result>& results;
			const char* const filter;

			ThreadParam	( cs::CThreadSafeQueue<Result>& results
						, const char* filter)
						: results(results)
						, filter(filter) {
			}
			
		};
		struct ThraedWithData {
			spec::CThread thread;
			cs::CAutoPtr<ThreadParam> param;
			ThraedWithData() : param(nullptr) {
			}
		};
		struct ReadFileThreadParam {
			cs::CVector<ThraedWithData>& dataPerThread;
			spec::CFile& file;
			ThreadState state;
			ReadFileThreadParam(cs::CVector<ThraedWithData>& dataPerThread,
								spec::CFile& file)
				: dataPerThread(dataPerThread)
				, file(file)
				, state(StartThread) {
			}
		};
		////////////////////////////////////
	private:

		////////////// Members /////////////
		Data data;		//read async, write only in class
		
		cs::CVector<ThraedWithData> threads;		//count == logic_core_count
		cs::CThreadSafeQueue<Result> results;	//thread-safe linked list

		ReadFileThreadParam read_thread_param;
		spec::CThread read_thread;
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