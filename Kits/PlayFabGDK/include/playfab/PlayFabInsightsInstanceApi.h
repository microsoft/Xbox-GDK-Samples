#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabInsightsDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Insights APIs
    /// </summary>
    class PlayFabInsightsInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabInsightsInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabInsightsInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabInsightsInstanceAPI() = default;
        PlayFabInsightsInstanceAPI(const PlayFabInsightsInstanceAPI& source) = delete; // disable copy
        PlayFabInsightsInstanceAPI(PlayFabInsightsInstanceAPI&&) = delete; // disable move
        PlayFabInsightsInstanceAPI& operator=(const PlayFabInsightsInstanceAPI& source) = delete; // disable assignment
        PlayFabInsightsInstanceAPI& operator=(PlayFabInsightsInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Insights.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void GetDetails(InsightsModels::InsightsEmptyRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLimits(InsightsModels::InsightsEmptyRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetLimitsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetOperationStatus(InsightsModels::InsightsGetOperationStatusRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetOperationStatusResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPendingOperations(InsightsModels::InsightsGetPendingOperationsRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetPendingOperationsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPerformance(InsightsModels::InsightsSetPerformanceRequest& request, const ProcessApiCallback<InsightsModels::InsightsOperationResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetStorageRetention(InsightsModels::InsightsSetStorageRetentionRequest& request, const ProcessApiCallback<InsightsModels::InsightsOperationResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnGetDetailsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLimitsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetOperationStatusResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPendingOperationsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPerformanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetStorageRetentionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
