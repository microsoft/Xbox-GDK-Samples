
// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

// A wrapper around a vector that allows lock-free enumeration of the
// contents.  Add/Remove requires a lock.
template <class TElement>
class AtomicVector
{
public:

    template<typename TArg>
    HRESULT Add(_In_ TArg&& value)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
        uint32_t bufferWriteIdx = 1 - bufferReadIdx;

        m_buffers[bufferWriteIdx] = m_buffers[bufferReadIdx];
        m_buffers[bufferWriteIdx].push_back(std::forward<TArg>(value));

        // Now spin wait to swap the active buffer.
        uint32_t expected = bufferReadIdx << 31;
        uint32_t desired = bufferWriteIdx << 31;

        for (;;)
        {
            uint32_t expectedSwap = expected;
            if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
            {
                break;
            }
        }

        // Now that the read buffer is not being used we can free
        // up its contents.
        m_buffers[bufferReadIdx].clear();
        m_buffers[bufferReadIdx].shrink_to_fit();

        return S_OK;
    }

    template <typename Func>
    void Remove(_In_ Func predicate)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
        uint32_t bufferWriteIdx = 1 - bufferReadIdx;

        m_buffers[bufferWriteIdx] = m_buffers[bufferReadIdx];

        auto &buffer = m_buffers[bufferWriteIdx];
        auto it = std::find_if(buffer.begin(), buffer.end(), predicate);

        if (it != buffer.end())
        {
            buffer.erase(it);
        }

        // Now spin wait to swap the active buffer.
        uint32_t expected = bufferReadIdx << 31;
        uint32_t desired = bufferWriteIdx << 31;

        for (;;)
        {
            uint32_t expectedSwap = expected;
            if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
            {
                break;
            }
        }

        // Now that the read buffer is not being used we can free
        // up its contents.
        m_buffers[bufferReadIdx].clear();
        m_buffers[bufferReadIdx].shrink_to_fit();
    }

    template <typename Func>
    void Visit(_In_ Func callback)
    {
        uint32_t indexAndRef = ++m_indexAndRef;
        uint32_t bufferIdx = (indexAndRef & 0x80000000 ? 1 : 0);

        for (auto const &it : m_buffers[bufferIdx])
        {
            callback(it);
        }

        m_indexAndRef--;
    }

private:

    std::mutex m_lock;
    std::vector<TElement> m_buffers[2];
    std::atomic<uint32_t> m_indexAndRef { 0 };
};
