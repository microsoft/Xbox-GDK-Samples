#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabExperimentationDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Experimentation APIs
    /// </summary>
    class PlayFabExperimentationInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabExperimentationInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabExperimentationInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabExperimentationInstanceAPI() = default;
        PlayFabExperimentationInstanceAPI(const PlayFabExperimentationInstanceAPI& source) = delete; // disable copy
        PlayFabExperimentationInstanceAPI(PlayFabExperimentationInstanceAPI&&) = delete; // disable move
        PlayFabExperimentationInstanceAPI& operator=(const PlayFabExperimentationInstanceAPI& source) = delete; // disable assignment
        PlayFabExperimentationInstanceAPI& operator=(PlayFabExperimentationInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Experimentation.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void CreateExclusionGroup(ExperimentationModels::CreateExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::CreateExclusionGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateExperiment(ExperimentationModels::CreateExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::CreateExperimentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteExclusionGroup(ExperimentationModels::DeleteExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteExperiment(ExperimentationModels::DeleteExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetExclusionGroups(ExperimentationModels::GetExclusionGroupsRequest& request, const ProcessApiCallback<ExperimentationModels::GetExclusionGroupsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetExclusionGroupTraffic(ExperimentationModels::GetExclusionGroupTrafficRequest& request, const ProcessApiCallback<ExperimentationModels::GetExclusionGroupTrafficResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetExperiments(ExperimentationModels::GetExperimentsRequest& request, const ProcessApiCallback<ExperimentationModels::GetExperimentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLatestScorecard(ExperimentationModels::GetLatestScorecardRequest& request, const ProcessApiCallback<ExperimentationModels::GetLatestScorecardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTreatmentAssignment(ExperimentationModels::GetTreatmentAssignmentRequest& request, const ProcessApiCallback<ExperimentationModels::GetTreatmentAssignmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void StartExperiment(ExperimentationModels::StartExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void StopExperiment(ExperimentationModels::StopExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateExclusionGroup(ExperimentationModels::UpdateExclusionGroupRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateExperiment(ExperimentationModels::UpdateExperimentRequest& request, const ProcessApiCallback<ExperimentationModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnCreateExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetExclusionGroupsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetExclusionGroupTrafficResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetExperimentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLatestScorecardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTreatmentAssignmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnStartExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnStopExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateExclusionGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateExperimentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
