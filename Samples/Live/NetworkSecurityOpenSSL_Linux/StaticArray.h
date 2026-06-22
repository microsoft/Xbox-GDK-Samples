// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

template <typename TData, uint32_t size>
class StaticArray
{
public:

    StaticArray()
    {
    }

    StaticArray(const StaticArray& other)
    {
        m_count = other.m_count;
        for(uint32_t idx = 0; idx < m_count; idx++)
        {
            m_array[idx] = other.m_array[idx];
        }
    }

    StaticArray& operator=(const StaticArray& other)
    {
        m_count = other.m_count;
        for(uint32_t idx = 0; idx < m_count; idx++)
        {
            m_array[idx] = other.m_array[idx];
        }
        return *this;
    }

    uint32_t count() { return m_count; }
    uint32_t capacity() { return ARRAYSIZE(m_array) - m_count; }
    void clear() { m_count = 0; }
    TData* data() { return m_array; }
    TData& operator[](size_t idx) { return m_array[idx]; }

    void append(const TData& data)
    {
        ASSERT(m_count != ARRAYSIZE(m_array));
        m_array[m_count++] = data;
    }

    void removeAt(uint32_t idx)
    {
        if (idx == m_count - 1)
        {
            m_count --;
        }
        else
        {
            for (uint32_t i = idx + 1; i < m_count; i++)
            {
                m_array[i - 1] = m_array[i];
            }
            m_count--;
        }
    }

private:

    uint32_t m_count = 0;
    TData m_array[size];

};
