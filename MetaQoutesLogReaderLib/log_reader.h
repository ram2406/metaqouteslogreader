#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "thread_safe_queue.h"

namespace log_reader {

	int test(const char* filename, const char* regex);
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
		struct Data {
			char filter[255];
			char fileName[255];
			FILE *file;
			ErrorCode lastError;			
		};
		
		struct SharedData {
			friend class CLogReader;
			static const unsigned OffsetOfShift = 1000U;
		private:
			
			fpos_t last_pos_in_file;
			spec::CMutex mutex;
			FILE *file;
		public:
			fpos_t GetLastPos() {
				cs::CLockGuard<spec::CMutex> lk(mutex);
				return last_pos_in_file;
			}
			fpos_t Shift() {
				cs::CLockGuard<spec::CMutex> lk(mutex);
				if (last_pos_in_file == EOF) {
					return last_pos_in_file;
				}
				for (unsigned i = 0; i < OffsetOfShift; ++i) {
					while (true) {
						char c = ::fgetc(file);
						if (c == '\n') {
							break;
						}
						if (c == EOF) {
							fpos_t old_pos = last_pos_in_file;
							last_pos_in_file = EOF;
							return old_pos;
						}
					}
				}
				fpos_t old_pos = last_pos_in_file;
				::fgetpos(file, &last_pos_in_file);
				return old_pos;
			}
		};

		struct Result {
			fpos_t position_in_file;
		};

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

	private:
		Data data;
		SharedData shared_data;
		

		inline void lastErrorAssert();

		bool match(const char* string);

		cs::CVector<spec::CThread> threads;
		cs::CThreadSafeQueue<Result> results;

		ThreadParam thread_param;

		void init_threads();
	public:
		CLogReader(const char* filename);
		~CLogReader();

		bool    Open();							 // �������� �����, false - ������
		void    Close();                         // �������� �����

		bool    SetFilter(const char *filter);   // ��������� ������� �����, false - ������

		// false - ����� ����� ��� ������
		bool    GetNextLine(char *buf,           // ������ ��������� ��������� ������, 
							const int bufsize);  // buf - �����, bufsize - ������������ �����
		

		ErrorCode GetLastError() const { return data.lastError; }
	};

}

namespace lr = log_reader;