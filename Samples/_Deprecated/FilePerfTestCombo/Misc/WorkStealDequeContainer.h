//--------------------------------------------------------------------------------------
// WorkStealDequeContainer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <atomic>
#include <cassert>

// Provides a container that holds multiple deques with interfaces to support work stealing
// The standard deque functions are supported with an additional parameter for which deque to route the call
// If that particular deque is empty then it will attempt to steal data from another deque.

namespace ATG
{
    namespace WorkSteal
    {
        //////////////////////////////////////////////////////////////////////////
        /// \brief WorkStealDequeContainer
        /// \details ATG::WorkSteal::WorkStealDequeContainer<DequeBase, m_numDeques, DequeBase, allocator>
        /// \details A container that holds multiple deques and provides push/pop front/back on each based on index
        ///          Pop operations can steal from another deque and this stealing always happens from the back
        ///          Thread safety is provided by the underlying deque classes, if they do not provide safety then this class is not thread safe
        //////////////////////////////////////////////////////////////////////////
        template<class DequeBase, class allocator = std::allocator<typename DequeBase::NodeType>>
        class WorkStealDequeContainer
        {
        public:
            typedef allocator AllocatorType;
            enum class PopDataType
            {
                NO_ELEMENT,
                OWNED_ELEMENT,
                STOLEN_ELEMENT
            };

        private:
            uint64_t m_numDeques;
            DequeBase **m_deques;
            std::atomic<uint64_t> *m_stealQueue;

            uint64_t IncrementStealIndex(uint64_t queueIndex)
            {
                uint64_t oldValue = m_stealQueue[queueIndex].load(std::memory_order_relaxed);
                uint64_t newValue = oldValue + 1;
                newValue %= m_numDeques;
                // we don't really care if this fails, that just means somebody else already incremented
                m_stealQueue[queueIndex].compare_exchange_weak(oldValue, newValue);
                return m_stealQueue[queueIndex].load();
            }

        public:

            WorkStealDequeContainer(uint64_t numDeques, AllocatorType *alloc = nullptr)
            {
                m_numDeques = numDeques;
                m_deques = reinterpret_cast<DequeBase **> (new uintptr_t[m_numDeques]);
                m_stealQueue = new std::atomic<uint64_t>[m_numDeques];
                for (uint64_t i = 0; i < m_numDeques; i++)
                {
                    m_deques[i] = new DequeBase(alloc);
                    m_stealQueue[i] = i;
                }
            }

            ~WorkStealDequeContainer()
            {
                for (uint64_t i = 0; i < m_numDeques; i++)
                    delete m_deques[i];
                delete[] m_deques;
            }

            void wake_all()
            {
                for (uint64_t i = 0; i < m_numDeques; i++)
                    m_deques[i]->wake_all();
            }

            void push_back(uint64_t queueIndex, const typename DequeBase::DataType& node)
            {
                assert(queueIndex < m_numDeques);
                m_deques[queueIndex]->push_back(node);
            }

            void push_front(uint64_t queueIndex, const typename DequeBase::DataType& node)
            {
                assert(queueIndex < m_numDeques);
                m_deques[queueIndex]->push_front(node);
            }

            PopDataType pop_back(uint64_t queueIndex, typename DequeBase::DataType& data, bool wait = true)
            {
                assert(queueIndex < m_numDeques);
                if (m_deques[queueIndex]->pop_back(data, false))
                    return PopDataType::OWNED_ELEMENT;
                if (m_deques[IncrementStealIndex(queueIndex)]->pop_back(data, false))
                    return PopDataType::STOLEN_ELEMENT;
                if (wait)
                {
                    m_deques[queueIndex]->pop_back(data, wait);
                    return PopDataType::OWNED_ELEMENT;
                }
                return PopDataType::NO_ELEMENT;
            }

            PopDataType pop_front(uint64_t queueIndex, typename DequeBase::DataType& data, bool wait = true)
            {
                assert(queueIndex < m_numDeques);
                if (m_deques[queueIndex]->pop_front(data, false))
                    return PopDataType::OWNED_ELEMENT;
                // stealing is always done from the back
                if (m_deques[IncrementStealIndex(queueIndex)]->pop_back(data, false))
                    return PopDataType::STOLEN_ELEMENT;
                if (wait)
                {
                    m_deques[queueIndex]->pop_front(data, wait);
                    return PopDataType::OWNED_ELEMENT;
                }
                return PopDataType::NO_ELEMENT;
            }

            PopDataType invoke_front(uint64_t queueIndex, uintptr_t& output, bool wait = true)
            {
                assert(queueIndex < m_numDeques);
                if (m_deques[queueIndex]->invoke_front(output, false))
                    return PopDataType::OWNED_ELEMENT;
                // stealing always happens from the back
                if (m_deques[IncrementStealIndex(queueIndex)]->invoke_back(output, false))
                    return PopDataType::STOLEN_ELEMENT;
                if (wait)
                {
                    if (m_deques[queueIndex]->invoke_front(output, wait))
                        return PopDataType::OWNED_ELEMENT;
                }
                return PopDataType::NO_ELEMENT;
            }

            PopDataType invoke_back(uint64_t queueIndex, uintptr_t& output, bool wait = true)
            {
                assert(queueIndex < m_numDeques);
                if (m_deques[queueIndex]->invoke_back(output, false))
                    return PopDataType::OWNED_ELEMENT;
                // stealing always happens from the back
                if (m_deques[IncrementStealIndex(queueIndex)]->invoke_back(output, false))
                    return PopDataType::STOLEN_ELEMENT;
                if (wait)
                {
                    m_deques[queueIndex]->invoke_back(output, wait);
                    return PopDataType::OWNED_ELEMENT;
                }
                return PopDataType::NO_ELEMENT;
            }
        };
    }
}
