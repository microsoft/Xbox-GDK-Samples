#pragma once

#include <XAsync.h>
#include <XTaskQueue.h>
#include "XboxHandle.h"
#include "Manager.h"

namespace NetRumble
{
    class AsyncTaskManager : public Manager
    {
    public:
        AsyncTaskManager() noexcept;

        void Tick();

        ATG::XboxHandle<XTaskQueueHandle> GetDefaultQueue() const;
        ATG::XboxHandle<XTaskQueueHandle> GetAsyncCompletionQueue() const;

    private:

        ATG::XboxHandle<XTaskQueueHandle> m_defaultQueue;
        ATG::XboxHandle<XTaskQueueHandle> m_asyncCompletionQueue;
    };
}
