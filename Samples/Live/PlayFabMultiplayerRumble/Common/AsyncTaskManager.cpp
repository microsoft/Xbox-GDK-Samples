#include "pch.h"
#include "AsyncTaskManager.h"

using namespace PlayFabMultiplayerRumble;

AsyncTaskManager::AsyncTaskManager() noexcept
{
    // To keep threading sync issues in this sample simpler, the background work can
    // happen on any thread, but the completion handlers only get called on the thread
    // of our choice.
    XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::Manual,
        m_defaultQueue.release_and_get_address_of());

    // this is much more tricky to handle for this simple sample, so we use this queue
    // for only very specific and controlled cases
    XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        m_asyncCompletionQueue.release_and_get_address_of());
}

ATG::XboxHandle<XTaskQueueHandle> AsyncTaskManager::GetDefaultQueue() const
{
    return m_defaultQueue;
}

ATG::XboxHandle<XTaskQueueHandle> AsyncTaskManager::GetAsyncCompletionQueue() const
{
    return m_asyncCompletionQueue;
}

void AsyncTaskManager::Tick()
{
    while (XTaskQueueDispatch(m_defaultQueue.get(), XTaskQueuePort::Completion, 0))
    {
    }
}
