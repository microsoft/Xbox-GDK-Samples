#pragma once

#include <playfab/PlayFabCallRequestContainer.h>
#include <playfab/PlayFabPluginManager.h>
#include <thread>
#include <mutex>
#include <atomic>

namespace PlayFab
{
    /// <summary>
    /// PlayFabIOSHttpPlugin is an https implementation to interact with PlayFab services using IOSHTTP.
    /// </summary>
    class PlayFabIOSHttpPlugin : public IPlayFabHttpPlugin
    {
    public:
        PlayFabIOSHttpPlugin();
        PlayFabIOSHttpPlugin(const PlayFabIOSHttpPlugin& other) = delete;
        PlayFabIOSHttpPlugin(PlayFabIOSHttpPlugin&& other) = delete;
        PlayFabIOSHttpPlugin& operator=(PlayFabIOSHttpPlugin&& other) = delete;
        virtual ~PlayFabIOSHttpPlugin();

        virtual void MakePostRequest(std::unique_ptr<CallRequestContainerBase> requestContainer) override;
        virtual size_t Update() override;

    protected:
        struct RequestTask;

        void WorkerThread();
        void ExecuteRequest(RequestTask& requestTask);

        virtual std::string GetUrl(const RequestTask& requestTask) const;
        virtual void SetPredefinedHeaders(const RequestTask& requestTask, void* urlRequest);
        virtual bool GetBinaryPayload(RequestTask& requestTask, void*& payload, size_t& payloadSize) const;
        virtual void ProcessResponse(RequestTask& requestTask, const int httpCode);
        virtual void HandleResults(RequestTask& requestTask);

        struct RequestImpl;
        struct RequestTask
        {
            RequestTask();
            virtual ~RequestTask();

            bool Initialize(std::unique_ptr<CallRequestContainerBase>& requestContainer);
            
            enum class State:int
            {
                None = 0,
                Pending = (int)RequestTask::State::None,
                Requesting,
                Finished
            };

            CallRequestContainer& RequestContainer()
            {
                return *dynamic_cast<CallRequestContainer*>(requestContainer.get());
            }

            const CallRequestContainer& ConstRequestContainer() const
            {
                return *dynamic_cast<CallRequestContainer*>(requestContainer.get());
            }

            std::string GetRequestContainerUrl() const
            {
                return requestContainer->GetUrl();
            }

            std::string GetRequestContainerFullUrl() const
            {
                return ConstRequestContainer().GetFullUrl();
            }

            void Cancel();
            std::atomic<State> state;
            std::unique_ptr<CallRequestContainerBase> requestContainer;
            RequestImpl* impl;
        };
        std::unique_ptr<std::thread> workerThread;
        std::mutex httpRequestMutex;
        std::atomic<bool> threadRunning;
        std::deque<std::shared_ptr<RequestTask>> pendingRequests;
        std::shared_ptr<RequestTask> requestingTask;
        std::deque<std::shared_ptr<RequestTask>> pendingResults;
    };
}
