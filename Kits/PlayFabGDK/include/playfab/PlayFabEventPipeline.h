#pragma once

#ifndef DISABLE_PLAYFABENTITY_API

// define body for logging or debug output
#define LOG_PIPELINE(S) /*std::cout << S*/

#include <playfab/PlayFabEvent.h>
#include <playfab/PlayFabEventBuffer.h>

#include <mutex>
#include <unordered_map>

namespace PlayFab
{
    class PlayFabEventsInstanceAPI;

    enum class PlayFabEventPipelineType
    {
        PlayFabPlayStream,
        PlayFabTelemetry,
    };

    /// <summary>
    /// Settings for any event pipeline
    /// NOTE: settings are expected to be set prior to calling PlayFabEventPipeline::Start()
    /// changing them after PlayFabEventPipeline::Start() may cause threading issues
    /// users should not expect changes made to settings to take effect after ::Start is called unless the pipeline is destroyed and re-created
    /// </summary>
    class PlayFabEventPipelineSettings
    {
    public:
        PlayFabEventPipelineSettings();
        PlayFabEventPipelineSettings(PlayFabEventPipelineType emitType);
        PlayFabEventPipelineSettings(PlayFabEventPipelineType emitType, bool useBackgroundThread);
        virtual ~PlayFabEventPipelineSettings() {};

        size_t bufferSize; // The minimal size of buffer, in bytes. The actually allocated size will be a power of 2 that is equal or greater than this value.
        size_t maximalNumberOfItemsInBatch; // The maximal number of items (events) a batch can hold before it is sent out.
        size_t maximalBatchWaitTime; // The maximal wait time before a batch must be sent out even if it's still incomplete, in seconds.
        size_t maximalNumberOfRetries; // The maximal number of retries for transient transport errors, before a batch is discarded.
        size_t maximalNumberOfBatchesInFlight; // The maximal number of batches currently "in flight" (sent to a transport plugin).
        int64_t readBufferWaitTime; // The wait time between attempts to read events from buffer when it is empty, in milliseconds.
        std::shared_ptr<PlayFabAuthenticationContext> authenticationContext; // The optional PlayFab authentication context that can be used with static PlayFab events API
        PlayFabEventPipelineType emitType; // whether we call WriteEvent or WriteTelemetryEvent through PlayFab
        bool useBackgroundThread;
    };

    /// <summary>
    /// Interface for any event pipeline
    /// </summary>
    class IPlayFabEventPipeline
    {
    public:
        virtual ~IPlayFabEventPipeline() {}
        virtual void Start() {} // Start pipeline's worker thread
        virtual void Stop() = 0;
        virtual void Update() = 0;
        virtual void IntakeEvent(std::shared_ptr<const IPlayFabEmitEventRequest> request) = 0; // Intake an event. This method must be thread-safe.
    };

    /// <summary>
    /// Implementation of PlayFab-specific event pipeline
    /// </summary>
    class PlayFabEventPipeline : public IPlayFabEventPipeline
    {
    public:
        explicit PlayFabEventPipeline(const std::shared_ptr<PlayFabEventPipelineSettings>& settings);
        virtual ~PlayFabEventPipeline() override;

        PlayFabEventPipeline(const PlayFabEventPipeline& source) = delete; // disable copy
        PlayFabEventPipeline(PlayFabEventPipeline&&) = delete; // disable move
        PlayFabEventPipeline& operator=(const PlayFabEventPipeline& source) = delete; // disable assignment
        PlayFabEventPipeline& operator=(PlayFabEventPipeline&& other) = delete; // disable move assignment

        // NOTE: settings are expected to be set prior to calling PlayFabEventPipeline::Start()
        // changing them after PlayFabEventPipeline::Start() may cause threading issues unless you have set useBackgroundThread flag to true
        // If this flag is not set, users should not expect changes made to settings to take effect after ::Start is called
        //   unless the pipeline is A.) destroyed and re-created or B.) restart it by running Stop() and then Start() again 
        std::shared_ptr<PlayFabEventPipelineSettings> GetSettings() const;
        virtual void Start() override;
        virtual void Stop() override;
        virtual void Update() override;
        virtual void IntakeEvent(std::shared_ptr<const IPlayFabEmitEventRequest> request) override;

        void SetExceptionCallback(ExceptionCallback callback);

    protected:
        virtual void SendBatch(std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>>& batch);

    private:
        void WorkerThread();
        bool DoWork();
        void WriteEventsApiCallback(const EventsModels::WriteEventsResponse& result, void* customData);
        void WriteEventsApiErrorCallback(const PlayFabError& error, void* customData);
        void CallbackRequest(std::shared_ptr<const IPlayFabEmitEventRequest> request, std::shared_ptr<const IPlayFabEmitEventResponse> response);

    protected:
        // PlayFab's public Events API (e.g. WriteEvents method) allows to pass only a pointer to some custom object (void* customData) that will be relayed back to its callbacks.
        // This is the only reliable way to relate a particular Events API call with its particular callbacks since it is an asynchronous operation.
        // We are using that feature (custom pointer relay) because we need to know which batch it was when we receive a callback from the Events API.
        // To keep track of all batches currently in flight (i.e. those for which we called Events API) we need to have a container with controllable size
        // that would allow to quickly map a pointer (like void* customData) to a batch (like a std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>>).
        std::mutex inFlightMutex;
        std::unordered_map<void*, std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>>> batchesInFlight;

    private:
        std::shared_ptr<PlayFabEventsInstanceAPI> eventsApi;

        std::atomic_uintptr_t batchCounter;
        std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>> batch;
        std::chrono::steady_clock::time_point momentBatchStarted;
        std::shared_ptr<PlayFabEventPipelineSettings> settings;
        PlayFabEventBuffer buffer;
        std::thread workerThread;
        std::atomic<bool> isWorkerThreadRunning;
        std::mutex userExceptionCallbackMutex;
        ExceptionCallback userExceptionCallback;

        bool TryGetBatchOutOfFlight(void* customData, std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>>* batchReturn);
    };
}

#endif
