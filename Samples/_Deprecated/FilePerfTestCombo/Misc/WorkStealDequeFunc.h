//--------------------------------------------------------------------------------------
// WorkStealDequeFunc.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdio>
#include <mutex>
#include <functional>
#include <condition_variable>

// Implements a deque that is suitable to use within a work stealing algorithm.
// One and only one thread would normally call push_front/pop_front. Any other thread can call push_back/pop_back
// The deque is implemented as a linked list, the nodes within the list can either be supplied or an underlying allocator can be given
// The deque stores std::function objects that return a uintptr_t. The major use would be a function object built up through std::bind

namespace ATG
{
	namespace WorkSteal
	{
		//////////////////////////////////////////////////////////////////////////
		/// \brief DequeFuncEntry
		/// \details ATG::WorkSteal::DequeFuncEntry<nodeDataType>
		/// \details An entry within the DequeFunc class exposed as an external structure
		///          Used by owners of a DequeFunc instance to provide their own memory allocation
		///          When the DequeFunc instance owner controls memory allocation then the raw version of push/pop should be used
		//////////////////////////////////////////////////////////////////////////
		struct DequeFuncEntry
		{
			typedef std::function<uintptr_t()> DataType;

			DequeFuncEntry *next;
			DequeFuncEntry *prev;
			std::function<uintptr_t()> func;
			DequeFuncEntry() : next(nullptr), prev(nullptr) {}
			DequeFuncEntry(const DataType& p1) : next(nullptr), prev(nullptr), func(p1) {}
		};

		//////////////////////////////////////////////////////////////////////////
		/// \brief DequeFunc
		/// \details ATG::WorkSteal::DequeFunc<lockClass, allocator>
		/// \details Implements a Deque storing std::function objects with an interface suitable for work stealing
		///          One and only one thread would normally call push_front/pop_front. Any other thread can call push_back/pop_back
		///          Implemented as a linked list, the nodes within the list can either be supplied or an underlying allocator can be given
		///          Stores std::function objects that return a uintptr_t. The major use would be a function object built up through std::bind
		///          Provides thread safety, multiple threads can simultaneously call push/pop front/back
		///          Interface matches std::deque, Ability to iterate through the deque is not provided since that would violate it's thread safety
		//////////////////////////////////////////////////////////////////////////
		template<class lockClass = std::mutex, class allocator = std::allocator<DequeFuncEntry>>
		class DequeFunc
		{
		public:
			typedef DequeFuncEntry NodeType;
			typedef lockClass LockType;
			typedef allocator AllocatorType;
			typedef std::function<uintptr_t()> DataType;

		private:
			NodeType *m_head;
			NodeType *m_tail;
			std::condition_variable_any m_waitCondition;
			lockClass m_stealLock;
			std::shared_ptr<allocator> m_allocator;

		public:
			DequeFunc(AllocatorType *alloc = nullptr) :
				m_head(nullptr)
				, m_tail(nullptr)
				, m_allocator(alloc ? alloc : new AllocatorType())
			{}
			~DequeFunc() {}

			void push_front(const DataType& func);
			void push_back(const DataType& func);

			bool pop_front(DataType& data, bool wait = true);
			bool pop_back(DataType& data, bool wait = true);

			bool invoke_front(uintptr_t& output, bool wait = true);
			bool invoke_back(uintptr_t& output, bool wait = true);

			void wake_all() { m_waitCondition.notify_all(); }
		};

		template<class lockClass, class allocator>
		inline void DequeFunc<lockClass, allocator>::push_back(const DataType& func)
		{
			std::lock_guard<lockClass> lock(m_stealLock);

			NodeType *newNode = m_allocator->allocate(1);
			std::allocator_traits<allocator>::construct(*(m_allocator.get()), newNode, func);

			newNode->prev = m_tail;
			if (m_tail)
				m_tail->next = newNode;
			else
				m_head = newNode;
			m_tail = newNode;
			m_waitCondition.notify_one();
		}

		template<class lockClass, class allocator>
		inline void DequeFunc<lockClass, allocator>::push_front(const DataType& func)
		{
			std::lock_guard<lockClass> lock(m_stealLock);

			NodeType *newNode = m_allocator->allocate(1);
			std::allocator_traits<allocator>::construct(*(m_allocator.get()), newNode, func);
			newNode->next = m_head;
			if (m_head)
				m_head->prev = newNode;
			else
				m_tail = newNode;
			m_head = newNode;
			m_waitCondition.notify_one();
		}

		template<class lockClass, class allocator>
		inline bool DequeFunc<lockClass, allocator>::pop_front(DataType& data, bool wait)
		{
			std::unique_lock<lockClass> lock(m_stealLock);
			if (!m_head && wait)
			{
				m_waitCondition.wait(lock);
			}
			if (!m_head)
				return false;

			data = m_head->func;

			NodeType *oldHead = m_head;

			m_head = m_head->next;
			if (m_head)
				m_head->prev = nullptr;
			else
				m_tail = nullptr;
			m_allocator->deallocate(oldHead, 1);

			return true;
		}

		template<class lockClass, class allocator>
		inline bool DequeFunc<lockClass, allocator>::pop_back(DataType& data, bool wait)
		{
			std::unique_lock<lockClass> lock(m_stealLock);
			if (!m_tail && wait)
			{
				m_waitCondition.wait(lock);
			}
			if (!m_tail)
				return false;

			data = m_tail->func;

			NodeType *oldTail = m_tail;

			m_tail = m_tail->prev;
			if (m_tail)
				m_tail->next = nullptr;
			else
				m_head = nullptr;
			m_allocator->deallocate(oldTail, 1);

			return true;
		}

		template<class lockClass, class allocator>
		inline bool DequeFunc<lockClass, allocator>::invoke_front(uintptr_t& output, bool wait)
		{
			m_stealLock.lock();
			if (!m_head && wait)
			{
				m_waitCondition.wait(m_stealLock);
			}
			if (m_head)
			{
				NodeType *oldHead = m_head;

				m_head = m_head->next;
				if (m_head)
					m_head->prev = nullptr;
				else
					m_tail = nullptr;
				m_stealLock.unlock();
				output = oldHead->func();
				m_allocator->deallocate(oldHead, 1);
				return true;
			}
			m_stealLock.unlock();
			return false;
		}

		template<class lockClass, class allocator>
		inline bool DequeFunc<lockClass, allocator>::invoke_back(uintptr_t& output, bool wait)
		{
			m_stealLock.lock();
			if (!m_tail && wait)
			{
				m_waitCondition.wait(m_stealLock);
			}

			if (m_tail)
			{
				NodeType *oldTail = m_tail;

				m_tail = m_tail->prev;
				if (m_tail)
					m_tail->next = nullptr;
				else
					m_head = nullptr;

				m_stealLock.unlock();
				output = oldTail->func();
				m_allocator->deallocate(oldTail, 1);
				return true;
			}
			m_stealLock.unlock();
			return false;
		}
	}
}
