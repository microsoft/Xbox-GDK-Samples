#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabDataDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Data APIs
    /// </summary>
    class PlayFabDataInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabDataInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabDataInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabDataInstanceAPI() = default;
        PlayFabDataInstanceAPI(const PlayFabDataInstanceAPI& source) = delete; // disable copy
        PlayFabDataInstanceAPI(PlayFabDataInstanceAPI&&) = delete; // disable move
        PlayFabDataInstanceAPI& operator=(const PlayFabDataInstanceAPI& source) = delete; // disable assignment
        PlayFabDataInstanceAPI& operator=(PlayFabDataInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Data.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void AbortFileUploads(DataModels::AbortFileUploadsRequest& request, const ProcessApiCallback<DataModels::AbortFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteFiles(DataModels::DeleteFilesRequest& request, const ProcessApiCallback<DataModels::DeleteFilesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void FinalizeFileUploads(DataModels::FinalizeFileUploadsRequest& request, const ProcessApiCallback<DataModels::FinalizeFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFiles(DataModels::GetFilesRequest& request, const ProcessApiCallback<DataModels::GetFilesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetObjects(DataModels::GetObjectsRequest& request, const ProcessApiCallback<DataModels::GetObjectsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void InitiateFileUploads(DataModels::InitiateFileUploadsRequest& request, const ProcessApiCallback<DataModels::InitiateFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetObjects(DataModels::SetObjectsRequest& request, const ProcessApiCallback<DataModels::SetObjectsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnAbortFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteFilesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnFinalizeFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFilesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetObjectsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnInitiateFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetObjectsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
