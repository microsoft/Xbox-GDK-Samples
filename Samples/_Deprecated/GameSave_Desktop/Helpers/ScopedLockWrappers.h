//--------------------------------------------------------------------------------------
// ScopedLockWrappers.h
//
// Wrappers around SRW locks for scoped exclusive (write) and scoped shared (read) locks,
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    // Implements a scoped exclusive lock around an SRWLock

    class ScopedExclusiveLock
    {
        PSRWLOCK srwLock = nullptr;

    public:
        ScopedExclusiveLock(SRWLOCK& lock) : srwLock(&lock)
        {
            ::AcquireSRWLockExclusive(srwLock);
        }

        ~ScopedExclusiveLock()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockExclusive(srwLock);
            }
        }

        ScopedExclusiveLock(ScopedExclusiveLock&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
        }

        ScopedExclusiveLock& operator=(ScopedExclusiveLock&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockExclusive(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
            }

            return *this;
        }

        ScopedExclusiveLock(const ScopedExclusiveLock& copyFrom) = delete;
        ScopedExclusiveLock& operator=(const ScopedExclusiveLock& copyFrom) = delete;
    };

    // Implements try-enter functionality for a scoped exclusive lock.

    class TryEnterScopedExclusiveLock
    {
        PSRWLOCK srwLock;
    public:
        TryEnterScopedExclusiveLock(SRWLOCK& lock) : srwLock(&lock)
        {
            if (!::TryAcquireSRWLockExclusive(srwLock))
            {
                srwLock = nullptr;
            }
        }
        ~TryEnterScopedExclusiveLock()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockExclusive(srwLock);
            }
        }

        TryEnterScopedExclusiveLock(TryEnterScopedExclusiveLock&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
        }

        operator bool() const noexcept
        {
            return srwLock != nullptr;
        }

        TryEnterScopedExclusiveLock& operator=(TryEnterScopedExclusiveLock&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockExclusive(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
            }

            return *this;
        }

        TryEnterScopedExclusiveLock(const TryEnterScopedExclusiveLock& copyFrom) = delete;
        TryEnterScopedExclusiveLock& operator=(const TryEnterScopedExclusiveLock& copyFrom) = delete;
    };

    // Implements a scoped shared lock around an SRWLock

    class ScopedSharedLock
    {
        PSRWLOCK srwLock;
    public:
        ScopedSharedLock(SRWLOCK& lock) : srwLock(&lock)
        {
            ::AcquireSRWLockShared(srwLock);
        }
        ~ScopedSharedLock()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockShared(srwLock);
            }
        }

        ScopedSharedLock(ScopedSharedLock&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
        }

        ScopedSharedLock& operator=(ScopedSharedLock&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockShared(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
            }

            return *this;
        }

        ScopedSharedLock(const ScopedSharedLock& copyFrom) = delete;
        ScopedSharedLock& operator=(const ScopedSharedLock& copyFrom) = delete;
    };

    // Implements try-enter functionality for a scoped shared lock around an SRWLock

    class TryEnterScopedSharedLock
    {
        PSRWLOCK srwLock;
    public:
        TryEnterScopedSharedLock(SRWLOCK& lock) : srwLock(&lock)
        {
            if (!::TryAcquireSRWLockShared(srwLock))
            {
                srwLock = nullptr;
            }
        }
        ~TryEnterScopedSharedLock()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockShared(srwLock);
            }
        }

        TryEnterScopedSharedLock(TryEnterScopedSharedLock&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
        }

        operator bool() const noexcept
        {
            return srwLock != nullptr;
        }

        TryEnterScopedSharedLock& operator=(TryEnterScopedSharedLock&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockShared(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
            }

            return *this;
        }

        TryEnterScopedSharedLock(const TryEnterScopedSharedLock& copyFrom) = delete;
        TryEnterScopedSharedLock& operator=(const TryEnterScopedSharedLock& copyFrom) = delete;
    };

    // Versions of scoped locks with associated payloads

    // Implements a scoped exclusive lock around an SRWLock, with a payload the lock is protecting access to.
    //
    // Example:
    // ```cpp
    //
    // struct MyInternalData {
    //    Data* a;
    //    Data* b;
    // };
    //
    // struct MyLockableObject {
    //    SRWLOCK lock;
    //    Data a;
    //    Data b;
    //
    //    ScopedSharedLockWithPayload< MyInternalData > GetSharedViewOfA() noexcept
    //    {
    //      return ScopedSharedLockWithPayload< MyInternallData >( lock, a );
    //    }
    // };
    //
    // ```
    //
    // Note: The payload is carried around as a pointer, so the shared lock cannot contain references to local
    // data - when it goes out of scope the pointers will be invalidated.
    //
    // The ScopedSharedLockWithPayload returns const pointers and const references to its data.

    template< typename TPayload >
    class ScopedExclusiveLockWithPayload
    {
        PSRWLOCK srwLock = nullptr;
        TPayload* payload;
    public:

        ScopedExclusiveLockWithPayload(SRWLOCK& lock) : srwLock(&lock)
        {
            ::AcquireSRWLockExclusive(srwLock);
        }

        ScopedExclusiveLockWithPayload(SRWLOCK& lock, TPayload& payloadData) : srwLock(&lock)
        {
            payload = &payloadData;
            ::AcquireSRWLockExclusive(srwLock);
        }

        ~ScopedExclusiveLockWithPayload()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockExclusive(srwLock);
            }
        }

        ScopedExclusiveLockWithPayload(ScopedExclusiveLockWithPayload&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
            payload = moveFrom.payload;
            moveFrom.payload = nullptr;
        }

        ScopedExclusiveLockWithPayload& operator=(ScopedExclusiveLockWithPayload&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockExclusive(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
                payload = moveFrom.payload;
                moveFrom.payload = nullptr;
            }

            return *this;
        }

        ScopedExclusiveLockWithPayload(const ScopedExclusiveLockWithPayload& copyFrom) = delete;
        ScopedExclusiveLockWithPayload& operator=(const ScopedExclusiveLockWithPayload& copyFrom) = delete;

        TPayload& operator*()
        {
            return *payload;
        }

        TPayload* operator->()
        {
            return payload;
        }
    };

    // Implements a scoped shared lock around an SRWLock

    template< typename TPayload >
    class ScopedSharedLockWithPayload
    {
        PSRWLOCK srwLock;
        const TPayload* payload;
    public:
        ScopedSharedLockWithPayload(SRWLOCK& lock) : srwLock(&lock)
        {
            ::AcquireSRWLockShared(srwLock);
        }

        ScopedSharedLockWithPayload(SRWLOCK& lock, const TPayload& payloadData) : srwLock(&lock)
        {
            payload = &payloadData;
            ::AcquireSRWLockShared(srwLock);
        }

        ~ScopedSharedLockWithPayload()
        {
            if (srwLock != nullptr)
            {
                ::ReleaseSRWLockShared(srwLock);
            }
        }

        ScopedSharedLockWithPayload(ScopedSharedLockWithPayload<TPayload>&& moveFrom)
        {
            srwLock = moveFrom.srwLock;
            moveFrom.srwLock = nullptr;
            payload = moveFrom.payload;
            moveFrom.payload = nullptr;
        }

        ScopedSharedLockWithPayload<TPayload>& operator=(ScopedSharedLockWithPayload<TPayload>&& moveFrom)
        {
            if (&moveFrom != this)
            {
                if (this->srwLock != nullptr)
                {
                    ::ReleaseSRWLockShared(this->srwLock);
                }
                srwLock = moveFrom.srwLock;
                moveFrom.srwLock = nullptr;
                payload = moveFrom.payload;
                moveFrom.payload = nullptr;
            }

            return *this;
        }

        ScopedSharedLockWithPayload(const ScopedSharedLockWithPayload<TPayload>& copyFrom) = delete;
        ScopedSharedLockWithPayload<TPayload>& operator=(const ScopedSharedLockWithPayload<TPayload>& copyFrom) = delete;

        const TPayload& operator*()
        {
            return *payload;
        }

        const TPayload* operator->()
        {
            return payload;
        }
    };
}
