#pragma once
#include <Windows.h>
#include <assert.h>

namespace spec {
	inline 
	unsigned Hardware_concurrency() {

		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		return sysinfo.dwNumberOfProcessors;
	}

	class CMutex {
		HANDLE h;
	public:
		CMutex() {
			h = ::CreateMutex(0, FALSE, 0);
		}
		~CMutex() {
			::CloseHandle(h);
		}
		void lock() {
			const auto &res = ::WaitForSingleObject(h, INFINITE);
			assert(res == WAIT_OBJECT_0 && "Mutex: lock failed");
		}
		bool try_lock() {
			return ::WaitForSingleObject(h, 1) == WAIT_OBJECT_0;
		}
		void unlock() {
			const auto &res = ::ReleaseMutex(h);
			assert(res == TRUE && "Mutex: unlock failed");
		}
	};

	class CThread {
		HANDLE hThread;
	public:
		typedef DWORD (WINAPI *ThreadProc)(LPVOID lpParam);
		CThread(ThreadProc proc, LPVOID lpParam) {
			Set(proc, lpParam);
		}
		CThread() : hThread(0) {
		}
		~CThread() {
			if (!hThread) { return; }
			::WaitForSingleObject(hThread, INFINITE);
			::CloseHandle(hThread);
		}
		void Set(ThreadProc proc, LPVOID lpParam) {
			hThread = ::CreateThread(0, 0, proc, lpParam, 0, 0);
		}
		bool IsRun() const { return hThread != 0; }

	};
	
	class CFile {
		HANDLE file;
	public:
		CFile() : file(nullptr) {
		}
		~CFile() {
			this->Close();
		}
		bool OpenForRead(const char* filename) {
			this->file = CreateFile(filename, FILE_READ_DATA,          // open for reading
				FILE_SHARE_READ,       // share for reading
				NULL,                  // default security
				OPEN_EXISTING,         // existing file only
				FILE_ATTRIBUTE_NORMAL, // normal file
				NULL);
			return file != INVALID_HANDLE_VALUE;
		}
		bool Close() {
			if (!file || file == INVALID_HANDLE_VALUE) {
				return true;
			}
			auto res = CloseHandle(file);
			file = nullptr;
			return res > 0;
		}

		//return false when file already ended
		bool ReadString(char *buf, unsigned bufsize) {
			auto dwCurrentFilePosition = GetPosition();

			DWORD buf_size_act = 0;
			if (!ReadFile(file, buf, bufsize, &buf_size_act, 0) || !buf_size_act) {
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
			SetPosition(offset);
			return true;
		}

		bool ReadStrings(char *buf, unsigned bufsize, unsigned long& pos, unsigned long& newpos) {
			DWORD buf_size_act = 0;

			pos = GetPosition();

			if (!ReadFile(file, buf, bufsize, &buf_size_act, 0) || !buf_size_act) {
				auto err = GetLastError();

				return false;
			}
			
			unsigned long offset = buf_size_act;
			for (; offset >= 0; --offset) {
				if (buf[offset] == '\n') { 
					buf[offset] = '\0'; 
					break; 
				}
			}
			
			if (bufsize > buf_size_act) {
				buf[buf_size_act] = '\0';
			}
			if (offset == 0) {
				offset = buf_size_act;
				buf[bufsize - 1] = '\0';
			}
			++offset;
			newpos = pos + offset;
			SetPosition(newpos);
			return true;
		}

		void SetPosition(unsigned long pos) {
			SetFilePointer(
				file, // must have GENERIC_READ and/or GENERIC_WRITE
				pos, // do not move pointer
				NULL, // hFile is not large enough in needing this pointer
				FILE_BEGIN); // provides offset from current position
		}
		unsigned long GetPosition() {
			return SetFilePointer(
				file, // must have GENERIC_READ and/or GENERIC_WRITE
				0, // do not move pointer
				NULL, // hFile is not large enough in needing this pointer
				FILE_CURRENT); // provides offset from current position
		}
	};

	class CFileInC {
		FILE* file;
	public:
		CFileInC() : file(nullptr) {
		}
		~CFileInC() {
			this->Close();
		}
		bool OpenForRead(const char* filename) {
			fopen_s(&file, filename, "rb");
			return file != nullptr;
		}
		bool Close() {
			if (!file || file == INVALID_HANDLE_VALUE) {
				return true;
			}
			return fclose(file) == 0;
		}

		//return false when file already ended
		bool ReadString(char *buf, unsigned bufsize) {
			auto dwCurrentFilePosition = GetPosition();
			auto buf_size_act = fread_s(buf, bufsize, 1, bufsize, file);
			if (!buf_size_act) {
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
			SetPosition(offset);
			return true;
		}

		bool ReadStrings(char *buf, unsigned bufsize, unsigned long& pos) {
		
			pos = GetPosition();
		
			auto buf_size_act = fread_s(buf, bufsize, 1, bufsize, file);
			if (!buf_size_act) {
				return false;
			}

			unsigned long offset = buf_size_act;
			for (; offset >= 0; --offset) {
				if (buf[offset] == '\n') {
					buf[offset] = '\0';
					break;
				}
			}

			if (bufsize > buf_size_act) {
				buf[buf_size_act] = '\0';
			}
			if (offset == 0) {
				offset = buf_size_act;
				buf[bufsize - 1] = '\0';
			}
			++offset;
			SetPosition(pos + offset);
			return true;
		}

		void SetPosition(unsigned long pos) {
			fpos_t p = pos;
			::fsetpos(file, &p);
		}
		unsigned long GetPosition() {
			fpos_t p;
			::fgetpos(file, &p);
			return (unsigned long)p;
		}
	};




	inline
	void Sleep(unsigned milliseconds) {
		::Sleep(milliseconds);
	}
};