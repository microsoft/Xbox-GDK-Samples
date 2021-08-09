#pragma once

#include <playfab/PlayFabCallRequestContainer.h>
#include <playfab/PlayFabPluginManager.h>
#include <playfab/PlayFabError.h>
#ifdef PLAYFAB_PLATFORM_GDK
#include <xcurl.h>
#else
#include <curl/curl.h>
#endif
#include <functional>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    /// <summary>
    /// PlayFabCurlHttpPlugin is an https implementation to interact with PlayFab services using curl.
    /// </summary>
    class PlayFabCurlHttpPlugin : public IPlayFabHttpPlugin
    {
    public:
        PlayFabCurlHttpPlugin();
        PlayFabCurlHttpPlugin(const PlayFabCurlHttpPlugin& other) = delete;
        PlayFabCurlHttpPlugin(PlayFabCurlHttpPlugin&& other) = delete;
        PlayFabCurlHttpPlugin& operator=(PlayFabCurlHttpPlugin&& other) = delete;
        virtual ~PlayFabCurlHttpPlugin();

        virtual void MakePostRequest(std::unique_ptr<CallRequestContainerBase> requestContainer) override;
        virtual size_t Update() override;

    protected:
        static size_t CurlReceiveData(char* buffer, size_t blockSize, size_t blockCount, void* userData);
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

    private:
        void CurlHeaderFailed(CallRequestContainer& requestContainer, const char* failedHeader);
        curl_slist* SetPredefinedHeaders(CallRequestContainer& requestContainer);
        curl_slist* TryCurlAddHeader(CallRequestContainer& requestContainer, curl_slist* list, const char* headerToAppend);
    };
}