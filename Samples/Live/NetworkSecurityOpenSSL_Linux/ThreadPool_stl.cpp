#include "pch.h"
#include "ThreadPool.h"

namespace OS
{
    class ThreadPoolImpl
    {
    public:

        ~ThreadPoolImpl() noexcept
        {
            Terminate();
        }

        void AddRef()
        {
            m_refs++;
        }

        void Release()
        {
            if (--m_refs == 0)
            {
                delete this;
            }
        }

        HRESULT Initialize(
            _In_opt_ void* context,
            _In_ ThreadPoolCallback* callback) noexcept
        {
            m_context = context;
            m_callback = callback;

            uint32_t numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0)
            {
                numThreads = 1;
            }

            try
            {
                while (numThreads != 0)
                {
                    numThreads--;
                    m_pool.emplace_back(std::thread([this]
                    {
#if defined(HC_PLATFORM) && HC_PLATFORM == HC_PLATFORM_ANDROID
                        JNIEnv* jniEnv = nullptr;
                        JavaVM* jvm = nullptr;
#endif

                        std::unique_lock<std::mutex> lock(m_wakeLock);
                        while (true)
                        {
                            if (m_calls == 0)
                            {
                                m_wake.wait(lock);
                            }

                            if (m_terminate)
                            {
                                break;
                            }

#if defined(HC_PLATFORM) && HC_PLATFORM == HC_PLATFORM_ANDROID
                            // lazy check for the JavaVM, we do it here so that we
                            // will attach even if the thread pool is initialized
                            // before we're given the jvm
                            if (!jniEnv)
                            {
                                jvm = s_javaVm;
                                if (jvm)
                                {
                                    jvm->AttachCurrentThread(&jniEnv, nullptr);
                                }
                            }
#endif

                            if (m_calls != 0)
                            {
                                m_calls--;

                                // ActionComplete is an optional call
                                // the callback can make to indicate 
                                // all portions of the call have finished
                                // and it is safe to release the
                                // thread pool, even if the callback has
                                // not totally unwound.  This is neccessary
                                // to allow users to close a task queue from
                                // within a callback.  Task queue guards with an 
                                // extra ref to ensure a safe point where 
                                // member state is no longer accessed, but the
                                // final release does need to wait on outstanding
                                // calls.

                                {
                                    std::unique_lock<std::mutex> lock(m_activeLock);
                                    m_activeCalls++;
                                }

                                ActionCompleteImpl ac(this);

                                lock.unlock();
                                AddRef();
                                m_callback(m_context, ac);
                                lock.lock();

                                if (!ac.Invoked)
                                {
                                    ac();
                                }

                                if (m_terminate)
                                {
                                    lock.unlock();
                                    Release(); // This could destroy us
                                    break;
                                }
                                else
                                {
                                    Release();
                                }
                            }
                        }

#if defined(HC_PLATFORM) && HC_PLATFORM == HC_PLATFORM_ANDROID
                        if (jniEnv && jvm)
                        {
                            jvm->DetachCurrentThread();
                        }
#endif
                    }));
                }
            }
            catch (const std::bad_alloc&)
            {
                return E_OUTOFMEMORY;
            }

            return S_OK;
        }

        void Terminate() noexcept
        {
            {
                std::unique_lock<std::mutex> wakeLock(m_wakeLock); // Must lock before m_activeLock
                m_terminate = true;
            }
            m_wake.notify_all();

            std::unique_lock<std::mutex> activeLock(m_activeLock);

            // Wait for the active call count
            // to go to zero.
            while (m_activeCalls != 0)
            {
                m_active.wait(activeLock);
            }
            activeLock.unlock();

            for (auto& t : m_pool)
            {
                if (t.get_id() == std::this_thread::get_id())
                {
                    t.detach();
                }
                else
                {
                    t.join();
                }
            }

            m_pool.clear();
        }

        void Submit() noexcept
        {
            {
                std::unique_lock<std::mutex> lock(m_wakeLock);
                m_calls++;
            }

            // Release lock before notify_all to optimize immediate awakes
            m_wake.notify_all();
        }

    private:

        struct ActionCompleteImpl : ThreadPoolActionComplete
        {
            ActionCompleteImpl(ThreadPoolImpl* owner) :
                m_owner(owner)
            {
            }

            bool Invoked = false;

            void operator()() override
            {
                Invoked = true;

                {
                    std::unique_lock<std::mutex> lock(m_owner->m_activeLock);
                    m_owner->m_activeCalls--;
                }

                // Release lock before notify_all to optimize immediate awakes
                m_owner->m_active.notify_all();
            }

        private:
            ThreadPoolImpl* m_owner = nullptr;
        };

        std::atomic<uint32_t> m_refs{ 1 };

        std::mutex m_wakeLock;
        std::condition_variable m_wake;
        uint32_t m_calls{ 0 };
        bool m_terminate{ false };

        std::mutex m_activeLock;
        std::condition_variable m_active;
        uint32_t m_activeCalls{ 0 };

        std::vector<std::thread> m_pool;
        void* m_context = nullptr;
        ThreadPoolCallback* m_callback = nullptr;

#if defined(HC_PLATFORM) && HC_PLATFORM == HC_PLATFORM_ANDROID
    public:
        static std::atomic<JavaVM*> s_javaVm;
#endif
    };

    ThreadPool::ThreadPool() noexcept :
        m_impl(nullptr)
    {
    }

    ThreadPool::~ThreadPool() noexcept
    {
        Terminate();
    }

    HRESULT ThreadPool::Initialize(_In_opt_ void* context, _In_ ThreadPoolCallback* callback) noexcept
    {
        RETURN_HR_IF(E_UNEXPECTED, m_impl != nullptr);

        std::unique_ptr<ThreadPoolImpl> impl(new (std::nothrow) ThreadPoolImpl);
        RETURN_IF_NULL_ALLOC(impl);

        RETURN_IF_FAILED(impl->Initialize(context, callback));

        m_impl = impl.release();
        return S_OK;
    }

    void ThreadPool::Terminate() noexcept
    {
        if (m_impl != nullptr)
        {
            m_impl->Terminate();
            m_impl->Release();
            m_impl = nullptr;
        }
    }

    void ThreadPool::Submit()
    {
        m_impl->Submit();
    }

#if defined(HC_PLATFORM) && HC_PLATFORM == HC_PLATFORM_ANDROID
    STDAPI XTaskQueueSetJvm(_In_ JavaVM* jvm) noexcept
    {
        assert(ThreadPoolImpl::s_javaVm == nullptr || ThreadPoolImpl::s_javaVm == jvm);
        ThreadPoolImpl::s_javaVm = jvm;
        return S_OK;
    }

    std::atomic<JavaVM*> ThreadPoolImpl::s_javaVm;
#endif
} // Namespace
