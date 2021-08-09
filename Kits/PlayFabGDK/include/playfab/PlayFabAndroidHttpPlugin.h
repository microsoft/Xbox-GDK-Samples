#pragma once

#include <playfab/PlayFabCallRequestContainer.h>
#include <playfab/PlayFabPluginManager.h>
#include <thread>
#include <mutex>
#include <atomic>

namespace PlayFab
{
    /// <summary>
    /// PlayFabAndroidHttpPlugin is an https implementation to interact with PlayFab services using WinHTTP.
    /// </summary>
    class PlayFabAndroidHttpPlugin : public IPlayFabHttpPlugin
    {
    public:
        PlayFabAndroidHttpPlugin();
        PlayFabAndroidHttpPlugin(const PlayFabAndroidHttpPlugin& other) = delete;
        PlayFabAndroidHttpPlugin(PlayFabAndroidHttpPlugin&& other) = delete;
        PlayFabAndroidHttpPlugin& operator=(PlayFabAndroidHttpPlugin&& other) = delete;
        virtual ~PlayFabAndroidHttpPlugin();

        virtual void MakePostRequest(std::unique_ptr<CallRequestContainerBase> requestContainer) override;
        virtual size_t Update() override;

    protected:
        struct RequestTask;

        void WorkerThreadEntry();
        void WorkerThread(void* jniEnv);
        bool ExecuteRequest(RequestTask& requestTask);
        bool CheckResponse(RequestTask& requestTask);
        void SetResponseAsBadRequest(RequestTask& requestTask);

        virtual void SetPredefinedHeaders(const RequestTask& requestTask);
        virtual void SetHeader(const RequestTask& requestTask, const char* name, const char* value);
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
            std::string GetRequestContainerUrl() const
            {
                return requestContainer->GetUrl();
            }
            std::atomic<State> state;
            std::unique_ptr<CallRequestContainerBase> requestContainer;
            std::unique_ptr<PlayFabAndroidHttpPlugin::RequestImpl> impl;
        };
        std::unique_ptr<std::thread> workerThread;
        std::mutex httpRequestMutex;
        std::atomic<bool> threadRunning;
        std::deque<std::shared_ptr<RequestTask>> pendingRequests;
        std::shared_ptr<RequestTask> requestingTask;
        std::deque<std::shared_ptr<RequestTask>> pendingResults;
    };
}
