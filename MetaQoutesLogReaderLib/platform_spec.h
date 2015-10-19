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

	inline
	void Sleep(unsigned milliseconds) {
		::Sleep(milliseconds);
	}
};