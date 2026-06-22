#pragma once

#include <XAsync.h>
#include <XTaskQueue.h>
#include "XboxHandle.h"
#include "IManager.h"

namespace PlayFabMultiplayerRumble
{
    class AsyncTaskManager : public IManager
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
