#pragma once
#include <new>
#include "platform_spec.h"

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
			return ptr != nullptr;
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
	protected:
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
		~CQueue() {
			Element data;
			while (this->Pop(data));
		}
		void Push(const Element& data) {
			if (!head) { return; }
			CNode* new_tail = new (std::nothrow) CNode;
			if (!new_tail) { return; }
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

	

	template < class Mx >
	class CLockGuard {
		Mx& mutex;
	public:
		CLockGuard(Mx& mx) : mutex(mx) { mutex.lock(); }
		~CLockGuard() { mutex.unlock(); }
	};

	template<class Element, class Mx = spec::CMutex>
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

	template<class Element>
	class CVector {
		Element* first;
		unsigned sz;
	public:
		CVector(unsigned size)
			: sz(size) {
			this->first = new (std::nothrow) Element[sz];
		}
		CVector() : first(nullptr), sz(0U) {
		}
		~CVector() {
			delete [] first;
		}

		Element& operator[] (unsigned index) {
			return first[index];
		}
		unsigned size() { return sz; }
		bool resize(unsigned new_size) {
			delete[] first;
			this->first = new (std::nothrow) Element[new_size];
			if (!this->first) { this->sz = 0;  return false; }
			this->sz = new_size;
			return true;
		}

		void clear() {
			resize(0);
		}
	};
}
namespace cs = containers;