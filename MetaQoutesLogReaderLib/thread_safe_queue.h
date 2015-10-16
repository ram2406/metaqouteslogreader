#include <new>
#include <Windows.h>

namespace containers {

	template<class T>
	class CAutoPtr {
		T* ptr;

		CAutoPtr(const CAutoPtr&) {}
		CAutoPtr& operator= (const CAutoPtr&) { return *this; }
	public:
		CAutoPtr(T* ptr)
			: ptr(ptr) {
		}
		~CAutoPtr() {
			delete ptr;
		}

		T* get() {
			return ptr;
		}

		T* operator-> () {
			return ptr;
		}
		T& operator* () {
			return *ptr;
		}
		operator bool() {
			return ptr;
		}
		T* release() {
			auto p = ptr;
			ptr = nullptr;
			return p;
		}
		void reset(T* p) {
			delete ptr;
			ptr = p;
		}
	};

	template <class Element>
	class CQueue {
		struct CNode {
			Element data;
			CAutoPtr<CNode> next;
			CNode(const Element& data)
				: data(data)
				, next(nullptr)
			{
			}
			CNode() : next(nullptr) { }
		};
		CAutoPtr<CNode> head;
		CNode* tail;

		CQueue(const CQueue&) {}
		CQueue& operator= (const CQueue&) { return *this; }
	public:
		CQueue()
			: head(new (std::nothrow) CNode)
			, tail(head.get()) {
		}
		void Push(const Element& data) {
			if (!head) { return; }
			CNode* new_tail = new (std::nothrow) CNode;
			tail->data = data;
			tail->next.reset(new_tail);
			tail = new_tail;
		}
		bool Pop(Element& data) {
			if (!head) { return false; }
			if (!head->next) {
				return false;
			}
			data = head->data;
			head.reset(head->next.release());
			return true;
		}
	};

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
			const auto &res = ::WaitForSingleObject(h, 0);
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

	template < class Mx >
	class CLockGuard {
		Mx& mutex;
	public:
		CLockGuard(Mx& mx) : mutex(mx) { mutex.lock(); }
		~CLockGuard() { mutex.unlock(); }
	};

	template<class Element, class Mx = CMutex>
	class CThreadSafeQueue
		: private CQueue<Element>
	{
		Mx mutex;
		typedef CQueue<Element> base;
	public:
		CThreadSafeQueue() {
			
		}
		~CThreadSafeQueue() {
			
		}
		bool Pop(Element& data) {
			CLockGuard<Mx> lk(mutex);
			return base::Pop(data);
		}
		void Push(const Element& data) {
			CLockGuard<Mx> lk(mutex);
			base::Push(data);
		}
	};
}