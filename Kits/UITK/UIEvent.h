//--------------------------------------------------------------------------------------
// File: UIEvent.h
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <mutex>
#include <map>

namespace ATG
{
    namespace UITK
    {
        /// the UIEvent class is a template for allowing code to "register" for class-specific events
        /// where the event handler being registered is given a pointer to the sender of those
        /// events which is class-specific.
        template <class ElementT, typename StateValT>
        class UIStateEvent
        {
        public:
            using ListenerHandle = uint32_t;
            using NewStateValT = StateValT;
            using PrevStateValT = StateValT;
            using Listener = std::function<void(ElementT*)>;
            using ListenerPredicate = std::function<bool(const NewStateValT&, const PrevStateValT&)>;

        public:
            struct RegisteredListener
            {
                ListenerHandle handle;
                ListenerPredicate predicate;
                Listener listener;
                bool active;

                RegisteredListener(ListenerHandle lh, ListenerPredicate pred, Listener l) :
                    handle(lh), predicate(pred), listener(l), active(true)
                {}
            };

        public:
            UIStateEvent() :
                m_stateValue(),
                m_prevStateValue(),
                m_availableHandle(1),
                m_listeners(),
                m_sync(),
                m_pendingRemovals(false)
            {}

            UIStateEvent(StateValT initialState) :
                m_stateValue(initialState),
                m_prevStateValue(initialState),
                m_availableHandle(1),
                m_listeners(),
                m_sync(),
                m_pendingRemovals(false)
            {}

        
        public:
            StateValT Get() const { return m_stateValue; }
            StateValT GetPrev() const { return m_prevStateValue; }

            inline ListenerHandle NewHandle() { return m_availableHandle++; }

            ListenerHandle AddListener(Listener listener)
            {
                std::scoped_lock<std::mutex> lock(m_sync);
                auto handle = NewHandle();
                auto pred = [](const NewStateValT&, const PrevStateValT&) -> bool
                {
                    return true;
                };
                auto rl = std::make_unique<RegisteredListener>(RegisteredListener(handle, pred, listener));
                m_listeners.insert(std::make_pair(handle, std::move(rl)));
                return handle;
            }

            ListenerHandle AddListenerWhen(StateValT value, Listener listener)
            {
                std::scoped_lock<std::mutex> lock(m_sync);
                auto handle = NewHandle();
                auto pred = [value](const NewStateValT& newVal, const PrevStateValT&) -> bool
                {
                    return newVal == value;
                };
                auto rl = std::make_unique<RegisteredListener>(RegisteredListener(handle, pred, listener));
                m_listeners.insert(std::make_pair(handle, std::move(rl)));
                return handle;
            }

            ListenerHandle AddListenerWhen(ListenerPredicate predicate, Listener listener)
            {
                std::scoped_lock<std::mutex> lock(m_sync);
                auto handle = NewHandle();
                auto rl = std::make_unique<RegisteredListener>(RegisteredListener(handle, predicate, listener));
                m_listeners.insert(std::make_pair(handle, std::move(rl)));
                return handle;
            }

            void RemoveListener(ListenerHandle handle)
            {
                auto iter = m_listeners.find(handle);

                if (iter != m_listeners.end())
                {
                    iter->second->active = false;
                    m_pendingRemovals = true;
                }
            }

        protected:
            void Set(StateValT value, ElementT* setter, bool notify = true)
            {
                if (m_stateValue != value)
                {
                    m_prevStateValue = m_stateValue;
                    m_stateValue = value;
                    if (notify) { Dispatch(setter); }
                }
            }

            void ClearTo(StateValT value)
            {
                Set(value, nullptr, false);
            }

            void Dispatch(ElementT* sender)
            {
                std::scoped_lock<std::mutex> lock(m_sync);

                if (m_pendingRemovals)
                {
                    for (auto iter = m_listeners.begin(), last = m_listeners.end(); iter != last; ) {
                        if (iter->second->active == false) {
                            iter = m_listeners.erase(iter);
                        }
                        else {
                            ++iter;
                        }
                    }
                    m_pendingRemovals = false;
                }

                // NOTE: we iterate in reverse just in case the recipient of the event chooses
                // to remove their handler during their callback
                for (auto& [handle, listener] : m_listeners)
                {
                    if (listener->predicate(m_stateValue, m_prevStateValue))
                    {
                        listener->listener(sender);
                    }
                }
            }

            static bool DefaultPredicate(const NewStateValT&, const PrevStateValT&) { return true; }

            StateValT m_stateValue;
            StateValT m_prevStateValue;
            ListenerHandle m_availableHandle;
            std::map<ListenerHandle, std::unique_ptr<RegisteredListener>> m_listeners;
            std::mutex m_sync;
            bool m_pendingRemovals;

            friend ElementT;
        };
    }
}
