#pragma once

namespace OS
{
    struct ThreadPoolActionComplete
    {
        virtual void operator()() = 0;
    };

    using ThreadPoolCallback = void(_In_opt_ void*, _In_ ThreadPoolActionComplete & complete);

    class ThreadPoolImpl;

    // A thread pool will invoke its callback on a pool of threads.
    class ThreadPool
    {
    public:
        ThreadPool() noexcept;
        ~ThreadPool() noexcept;

        // Initializes the thread pool.
        HRESULT Initialize(_In_opt_ void* context, _In_ ThreadPoolCallback* callback) noexcept;

        // Terminates the thread pool, waiting for any outstanding calls to drain
        // and and canceling any pending calls.
        void Terminate() noexcept;

        // Submits a new callback to the thread pool.  The callback passed to Initialize will
        // be invoked on a thread pool thread. May throw / crash if called after termination
        // or before init.
        void Submit();

    private:
        ThreadPoolImpl* m_impl;
    };
}
