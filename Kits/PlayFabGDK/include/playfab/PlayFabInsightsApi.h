#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabInsightsDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Insights APIs
    /// </summary>
    class PlayFabInsightsAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Insights.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void GetDetails(InsightsModels::InsightsEmptyRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLimits(InsightsModels::InsightsEmptyRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetLimitsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetOperationStatus(InsightsModels::InsightsGetOperationStatusRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetOperationStatusResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPendingOperations(InsightsModels::InsightsGetPendingOperationsRequest& request, const ProcessApiCallback<InsightsModels::InsightsGetPendingOperationsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPerformance(InsightsModels::InsightsSetPerformanceRequest& request, const ProcessApiCallback<InsightsModels::InsightsOperationResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetStorageRetention(InsightsModels::InsightsSetStorageRetentionRequest& request, const ProcessApiCallback<InsightsModels::InsightsOperationResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabInsightsAPI(); // Private constructor, static class should never have an instance
        PlayFabInsightsAPI(const PlayFabInsightsAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnGetDetailsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLimitsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetOperationStatusResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPendingOperationsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPerformanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetStorageRetentionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABENTITY_API)
