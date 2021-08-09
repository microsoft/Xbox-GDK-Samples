#pragma once

#if defined(ENABLE_PLAYFABSERVER_API)

#include <playfab/PlayFabMatchmakerDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Matchmaker APIs
    /// </summary>
    class PlayFabMatchmakerAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Matchmaker.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void AuthUser(MatchmakerModels::AuthUserRequest& request, const ProcessApiCallback<MatchmakerModels::AuthUserResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void PlayerJoined(MatchmakerModels::PlayerJoinedRequest& request, const ProcessApiCallback<MatchmakerModels::PlayerJoinedResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void PlayerLeft(MatchmakerModels::PlayerLeftRequest& request, const ProcessApiCallback<MatchmakerModels::PlayerLeftResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void StartGame(MatchmakerModels::StartGameRequest& request, const ProcessApiCallback<MatchmakerModels::StartGameResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UserInfo(MatchmakerModels::UserInfoRequest& request, const ProcessApiCallback<MatchmakerModels::UserInfoResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabMatchmakerAPI(); // Private constructor, static class should never have an instance
        PlayFabMatchmakerAPI(const PlayFabMatchmakerAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAuthUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnPlayerJoinedResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnPlayerLeftResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnStartGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUserInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if defined(ENABLE_PLAYFABSERVER_API)
