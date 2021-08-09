#pragma once
#if defined(PLAYFAB_PLATFORM_XBOX)

#include <playfab/PlayFabCallRequestContainer.h>
#include <playfab/PlayFabPluginManager.h>
#include <playfab/PlayFabError.h>
#include <functional>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include <json/value.h>
#include <playfab/PlayFabIXHR2HttpRequest.h>

namespace PlayFab
{
    /// <summary>
    /// PlayFabIXHR2HttpPlugin is the default https implementation for xbox to interact with PlayFab services using IXHR2 API.
    /// </summary>
    class PlayFabIXHR2HttpPlugin : public IPlayFabHttpPlugin
    {
    public:
        PlayFabIXHR2HttpPlugin();
        PlayFabIXHR2HttpPlugin(const PlayFabIXHR2HttpPlugin& other) = delete;
        PlayFabIXHR2HttpPlugin(PlayFabIXHR2HttpPlugin&& other) = delete;
        PlayFabIXHR2HttpPlugin& operator=(PlayFabIXHR2HttpPlugin&& other) = delete;
        virtual ~PlayFabIXHR2HttpPlugin();

        virtual void MakePostRequest(std::unique_ptr<CallRequestContainerBase> requestContainer) override;
        virtual size_t Update() override;

    protected:
        void SetupRequestHeaders(const CallRequestContainer& reqContainer, std::vector<HttpHeaderInfo>& headers);
        virtual void ExecuteRequest(std::unique_ptr<CallRequestContainer> requestContainer);
        void WorkerThread();
        void HandleCallback(std::unique_ptr<CallRequestContainer> requestContainer);
        void HandleResults(std::unique_ptr<CallRequestContainer> requestContainer);

        std::thread workerThread;
        std::mutex httpRequestMutex;
        std::atomic<bool> threadRunning;
        int activeRequestCount;
        std::deque<std::unique_ptr<CallRequestContainerBase>> pendingRequests;
        std::deque<std::unique_ptr<CallRequestContainerBase>> pendingResults;
    };
}
#endif // defined(PLAYFAB_PLATFORM_XBOX)