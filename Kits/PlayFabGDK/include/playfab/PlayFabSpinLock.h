#pragma once

#include <atomic>
#include <thread>

namespace PlayFab
{
    /// <summary>
    /// The implementation of a lightweight atomic spin that allows to control access to critical sections of code
    /// from multiple threads. The implementation is based on the implementation of atomic spin in Microsoft Gaming Cloud CELL library.
    /// </summary>
    struct AtomicSpin final
    {
        AtomicSpin();
        ~AtomicSpin();
        void Acquire();
        bool TryAcquire();
        void Release();

    private:
        AtomicSpin(const AtomicSpin& source) = delete; // disable copy
        const AtomicSpin& operator=(const AtomicSpin& source) = delete; // disable assignment

        std::atomic<std::thread::id> threadId;
        int32_t count;
    };

    /// <summary>
    /// A lightweight spin lock based on atomic spin.
    /// The implementation is based on the implementation of spin lock in Microsoft Gaming Cloud CELL library.
    /// </summary>
    struct SpinLock final
    {
        explicit SpinLock(AtomicSpin& atomicSpin);
        ~SpinLock();

        SpinLock(const SpinLock& source) = delete; // disable copy
        SpinLock(SpinLock&&) = delete; // disable move
        const SpinLock& operator=(const SpinLock& source) = delete; // disable assignment
        SpinLock& operator=(SpinLock&& other) = delete; // disable move assignment

    private:
        AtomicSpin& spin;
    };
}