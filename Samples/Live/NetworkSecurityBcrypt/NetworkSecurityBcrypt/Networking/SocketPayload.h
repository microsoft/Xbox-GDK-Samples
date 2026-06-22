//--------------------------------------------------------------------------------------
// File: SocketPayload.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "pch.h"

namespace ATG
{
    constexpr size_t c_MaxPayloadSize = 1384llu;

    struct SocketPayload
    {
        SocketPayload()
        {
            payload.reserve(c_MaxPayloadSize);
        }

        ~SocketPayload() = default;

        template<typename T>
        void Write(T x)
        {
            size_t originalSize = payload.size();
            size_t sizeOfT = sizeof(T);
            payload.resize(originalSize + sizeOfT);

            uint8_t* writeLocation = payload.data() + originalSize;

            if (readLocation >= payload.size())
            {
                assert(false);
            }

            memcpy(writeLocation, &x, sizeOfT);
        }

        template<typename T>
        void Write(std::vector<T>& x)
        {
            size_t sizeOfT = x.size() * sizeof(T);
            uint8_t* data = (uint8_t*)x.data();

            payload.insert(std::end(payload), data, data + sizeOfT);
        }

        template<typename T>
        void Read(T& x)
        {
            uint8_t* pos = payload.data() + readLocation;

            if (readLocation >= payload.size())
            {
                assert(false);
            }

            memcpy(&x, pos, sizeof(T));

            readLocation += sizeof(T);
        }

        template<typename T>
        T Read(void)
        {
            uint8_t* pos = payload.data() + readLocation;
            T x;

            if (readLocation >= payload.size())
            {
                assert(false);
            }

            readLocation += sizeof(T);

            memcpy(&x, pos, sizeof(T));

            return x;
        }

        template<typename T>
        SocketPayload& operator>>(T& x)
        {
            Read(x);
            return *this;
        }

        uint8_t* Data()
        {
            return payload.data();
        }

        DWORD Size() const
        {
            return static_cast<DWORD>(payload.size());
        }

        void Clear()
        {
            payload.clear();
            readLocation = 0;
        }

        void Fill(const void* data, size_t size)
        {
            size_t originalSize = payload.size();
            payload.resize(originalSize + size);

            uint8_t* writeLocation = payload.data() + originalSize;

            memcpy(writeLocation, data, size);
        }

        void Append(const std::vector<uint8_t>& data)
        {
            payload.insert(payload.end(), data.begin(), data.end());
        }

        template <size_t size>
        void Read(std::array<uint8_t, size>& buffer)
        {
            uint8_t* pos = payload.data() + readLocation;

            if (GetRemainingReadSize() < size)
            {
                assert(false);
            }

            memcpy(&buffer[0], pos, size);

            readLocation += size;
        }

        void Read(std::vector<uint8_t>& buffer, size_t length)
        {
            uint8_t* pos = payload.data() + readLocation;

            if (GetRemainingReadSize() < length || buffer.size() < length)
            {
                assert(false);
            }

            memcpy(buffer.data(), pos, length);

            readLocation += length;
        }

        void Read(std::vector<uint8_t>& buffer)
        {
            uint8_t* pos = payload.data() + readLocation;

            if (GetRemainingReadSize() < buffer.size())
            {
                assert(false);
            }

            memcpy(buffer.data(), pos, buffer.size());

            readLocation += buffer.size();
        }

        size_t GetRemainingReadSize() const
        {
            return payload.size() - readLocation;
        }

        size_t GetReadLocation() const
        {
            return readLocation;
        }

        void SetReadLocation(size_t newLoc)
        {
            assert(newLoc >= 0);
            assert(newLoc < payload.size());

            readLocation = newLoc;
        }

        void IncrementReadLocation(size_t val)
        {
            assert(val > 0);
            assert(readLocation + val < payload.size());

            readLocation += val;
        }

        std::string ToString()
        {
            return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
        }

        std::vector<uint8_t> payload;
        size_t readLocation = 0;
    };
}
