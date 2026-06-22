//--------------------------------------------------------------------------------------
// File: DelayedOperation.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <chrono>
#include <functional>

#include <XAsync.h>
#include <XAsyncProvider.h>

#include "NetworkDebugHelpers.h"

#undef max

namespace ATG
{
    class DelayedOperation
    {
    public:
        ~DelayedOperation()
        {
            Cancel();
        }

        void Init(std::function<void()> callback, XTaskQueueHandle queue) noexcept
        {
            async.context = this;
            async.queue = queue;
            async.callback = [](XAsyncBlock* async)
            {
                auto op = reinterpret_cast<DelayedOperation*>(async->context);

                if (XAsyncGetStatus(async, false) != E_ABORT)
                {
                    // If the lock is acquired elsewhere that means this is either being canceled or reset
                    // so we can skip over the rest of this.
                    if(op->m_lock.try_lock())
                    {
                        if(std::chrono::high_resolution_clock::now() >= op->m_endTime)
                        {
                            DebugLog("Timed op executing");
                            if (op->m_callback)
                            {
                                op->m_callback();
                            }
                        }

                        op->m_completed = true;

                        op->m_lock.unlock();
                    }
                    else
                    {
                        DebugLog("Timer being reset");
                    }
                }
                else
                {
                    DebugLog("Timer Aborted");
                }
                
            };

            m_callback = callback;
        }

        void Cancel() noexcept
        {
            std::scoped_lock lock(m_lock);

            if(!m_completed)
            {
                XAsyncCancel(&async);
            }

            m_endTime = time_point::max();
        }

        HRESULT Reset(uint32_t newDelayMS) noexcept
        {
            std::scoped_lock lock(m_lock);

            if (!m_completed || XAsyncGetStatus(&async, false) == E_PENDING)
            {
                XAsyncCancel(&async);
            }

            m_completed = false;

            m_delay = newDelayMS;
            m_endTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(newDelayMS);

            return XAsyncBegin(&async, this, nullptr, nullptr, DelayedOperation::Provider);
        }

        static HRESULT Provider(
            _In_ XAsyncOp op,
            _In_ const XAsyncProviderData* data
        ) noexcept
        {
            auto context = static_cast<DelayedOperation*>(data->context);

            if (op == XAsyncOp::Begin)
            {
                return XAsyncSchedule(data->async, context->m_delay);
            }
            else if (op == XAsyncOp::DoWork)
            {
                XAsyncComplete(data->async, S_OK, 0);
            }
            else if (op == XAsyncOp::Cancel)
            {
                XAsyncComplete(data->async, E_ABORT, 0);
            }

            return S_OK;
        }
    private:
        XAsyncBlock async{};

        std::mutex m_lock;
        bool m_completed{ false };

        std::function<void()> m_callback;

        using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

        uint32_t m_delay;
        time_point m_startTime;
        time_point m_endTime;
    };
}
