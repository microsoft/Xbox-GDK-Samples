#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabDataDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Data APIs
    /// </summary>
    class PlayFabDataAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Data.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void AbortFileUploads(DataModels::AbortFileUploadsRequest& request, const ProcessApiCallback<DataModels::AbortFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteFiles(DataModels::DeleteFilesRequest& request, const ProcessApiCallback<DataModels::DeleteFilesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void FinalizeFileUploads(DataModels::FinalizeFileUploadsRequest& request, const ProcessApiCallback<DataModels::FinalizeFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFiles(DataModels::GetFilesRequest& request, const ProcessApiCallback<DataModels::GetFilesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetObjects(DataModels::GetObjectsRequest& request, const ProcessApiCallback<DataModels::GetObjectsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void InitiateFileUploads(DataModels::InitiateFileUploadsRequest& request, const ProcessApiCallback<DataModels::InitiateFileUploadsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetObjects(DataModels::SetObjectsRequest& request, const ProcessApiCallback<DataModels::SetObjectsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabDataAPI(); // Private constructor, static class should never have an instance
        PlayFabDataAPI(const PlayFabDataAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAbortFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteFilesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnFinalizeFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFilesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetObjectsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnInitiateFileUploadsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetObjectsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABENTITY_API)
