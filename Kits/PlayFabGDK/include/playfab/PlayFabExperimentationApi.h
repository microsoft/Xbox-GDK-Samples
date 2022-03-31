#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabExperimentationDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Experimentation APIs
    /// </summary>
    class PlayFabExperimentationAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Experimentation.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void CreateExclusionGroup(ExperimentationModels::CreateExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::CreateExclusionGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateExperiment(ExperimentationModels::CreateExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::CreateExperimentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteExclusionGroup(ExperimentationModels::DeleteExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteExperiment(ExperimentationModels::DeleteExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetExclusionGroups(ExperimentationModels::GetExclusionGroupsRequest& request, const ProcessApiCallback<ExperimentationModels::GetExclusionGroupsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetExclusionGroupTraffic(ExperimentationModels::GetExclusionGroupTrafficRequest& request, const ProcessApiCallback<ExperimentationModels::GetExclusionGroupTrafficResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetExperiments(ExperimentationModels::GetExperimentsRequest& request, const ProcessApiCallback<ExperimentationModels::GetExperimentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLatestScorecard(ExperimentationModels::GetLatestScorecardRequest& request, const ProcessApiCallback<ExperimentationModels::GetLatestScorecardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTreatmentAssignment(ExperimentationModels::GetTreatmentAssignmentRequest& request, const ProcessApiCallback<ExperimentationModels::GetTreatmentAssignmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void StartExperiment(ExperimentationModels::StartExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void StopExperiment(ExperimentationModels::StopExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateExclusionGroup(ExperimentationModels::UpdateExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateExperiment(ExperimentationModels::UpdateExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabExperimentationAPI(); // Private constructor, static class should never have an instance
        PlayFabExperimentationAPI(const PlayFabExperimentationAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnCreateExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetExclusionGroupsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetExclusionGroupTrafficResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetExperimentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLatestScorecardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTreatmentAssignmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnStartExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnStopExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABENTITY_API)
