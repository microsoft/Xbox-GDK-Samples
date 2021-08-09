#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabProfilesDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Profiles APIs
    /// </summary>
    class PlayFabProfilesAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Profiles.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void GetGlobalPolicy(ProfilesModels::GetGlobalPolicyRequest& request, const ProcessApiCallback<ProfilesModels::GetGlobalPolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetProfile(ProfilesModels::GetEntityProfileRequest& request, const ProcessApiCallback<ProfilesModels::GetEntityProfileResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetProfiles(ProfilesModels::GetEntityProfilesRequest& request, const ProcessApiCallback<ProfilesModels::GetEntityProfilesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitlePlayersFromMasterPlayerAccountIds(ProfilesModels::GetTitlePlayersFromMasterPlayerAccountIdsRequest& request, const ProcessApiCallback<ProfilesModels::GetTitlePlayersFromMasterPlayerAccountIdsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetGlobalPolicy(ProfilesModels::SetGlobalPolicyRequest& request, const ProcessApiCallback<ProfilesModels::SetGlobalPolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetProfileLanguage(ProfilesModels::SetProfileLanguageRequest& request, const ProcessApiCallback<ProfilesModels::SetProfileLanguageResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetProfilePolicy(ProfilesModels::SetEntityProfilePolicyRequest& request, const ProcessApiCallback<ProfilesModels::SetEntityProfilePolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabProfilesAPI(); // Private constructor, static class should never have an instance
        PlayFabProfilesAPI(const PlayFabProfilesAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnGetGlobalPolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetProfilesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitlePlayersFromMasterPlayerAccountIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetGlobalPolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetProfileLanguageResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetProfilePolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABENTITY_API)
