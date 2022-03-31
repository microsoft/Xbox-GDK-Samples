#pragma once

#if !defined(DISABLE_PLAYFABCLIENT_API)

#include <playfab/PlayFabClientDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Client APIs
    /// </summary>
    class PlayFabClientInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabClientInstanceAPI();
        PlayFabClientInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings);
        PlayFabClientInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabClientInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabClientInstanceAPI() = default;
        PlayFabClientInstanceAPI(const PlayFabClientInstanceAPI& source) = delete; // disable copy
        PlayFabClientInstanceAPI(PlayFabClientInstanceAPI&&) = delete; // disable move
        PlayFabClientInstanceAPI& operator=(const PlayFabClientInstanceAPI& source) = delete; // disable assignment
        PlayFabClientInstanceAPI& operator=(PlayFabClientInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Client.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // Public, Client-Specific
        bool IsClientLoggedIn();

        // ------------ Generated API calls
        void AcceptTrade(ClientModels::AcceptTradeRequest& request, const ProcessApiCallback<ClientModels::AcceptTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddFriend(ClientModels::AddFriendRequest& request, const ProcessApiCallback<ClientModels::AddFriendResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddGenericID(ClientModels::AddGenericIDRequest& request, const ProcessApiCallback<ClientModels::AddGenericIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddOrUpdateContactEmail(ClientModels::AddOrUpdateContactEmailRequest& request, const ProcessApiCallback<ClientModels::AddOrUpdateContactEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddSharedGroupMembers(ClientModels::AddSharedGroupMembersRequest& request, const ProcessApiCallback<ClientModels::AddSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddUsernamePassword(ClientModels::AddUsernamePasswordRequest& request, const ProcessApiCallback<ClientModels::AddUsernamePasswordResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddUserVirtualCurrency(ClientModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<ClientModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AndroidDevicePushNotificationRegistration(ClientModels::AndroidDevicePushNotificationRegistrationRequest& request, const ProcessApiCallback<ClientModels::AndroidDevicePushNotificationRegistrationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AttributeInstall(ClientModels::AttributeInstallRequest& request, const ProcessApiCallback<ClientModels::AttributeInstallResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CancelTrade(ClientModels::CancelTradeRequest& request, const ProcessApiCallback<ClientModels::CancelTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConfirmPurchase(ClientModels::ConfirmPurchaseRequest& request, const ProcessApiCallback<ClientModels::ConfirmPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumeItem(ClientModels::ConsumeItemRequest& request, const ProcessApiCallback<ClientModels::ConsumeItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumeMicrosoftStoreEntitlements(ClientModels::ConsumeMicrosoftStoreEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumeMicrosoftStoreEntitlementsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumePS5Entitlements(ClientModels::ConsumePS5EntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumePS5EntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumePSNEntitlements(ClientModels::ConsumePSNEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumePSNEntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumeXboxEntitlements(ClientModels::ConsumeXboxEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumeXboxEntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateSharedGroup(ClientModels::CreateSharedGroupRequest& request, const ProcessApiCallback<ClientModels::CreateSharedGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ExecuteCloudScript(ClientModels::ExecuteCloudScriptRequest& request, const ProcessApiCallback<ClientModels::ExecuteCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAccountInfo(ClientModels::GetAccountInfoRequest& request, const ProcessApiCallback<ClientModels::GetAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAdPlacements(ClientModels::GetAdPlacementsRequest& request, const ProcessApiCallback<ClientModels::GetAdPlacementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAllUsersCharacters(ClientModels::ListUsersCharactersRequest& request, const ProcessApiCallback<ClientModels::ListUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCatalogItems(ClientModels::GetCatalogItemsRequest& request, const ProcessApiCallback<ClientModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterData(ClientModels::GetCharacterDataRequest& request, const ProcessApiCallback<ClientModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterInventory(ClientModels::GetCharacterInventoryRequest& request, const ProcessApiCallback<ClientModels::GetCharacterInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterLeaderboard(ClientModels::GetCharacterLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetCharacterLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterReadOnlyData(ClientModels::GetCharacterDataRequest& request, const ProcessApiCallback<ClientModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterStatistics(ClientModels::GetCharacterStatisticsRequest& request, const ProcessApiCallback<ClientModels::GetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetContentDownloadUrl(ClientModels::GetContentDownloadUrlRequest& request, const ProcessApiCallback<ClientModels::GetContentDownloadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCurrentGames(ClientModels::CurrentGamesRequest& request, const ProcessApiCallback<ClientModels::CurrentGamesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFriendLeaderboard(ClientModels::GetFriendLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFriendLeaderboardAroundPlayer(ClientModels::GetFriendLeaderboardAroundPlayerRequest& request, const ProcessApiCallback<ClientModels::GetFriendLeaderboardAroundPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFriendsList(ClientModels::GetFriendsListRequest& request, const ProcessApiCallback<ClientModels::GetFriendsListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetGameServerRegions(ClientModels::GameServerRegionsRequest& request, const ProcessApiCallback<ClientModels::GameServerRegionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboard(ClientModels::GetLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardAroundCharacter(ClientModels::GetLeaderboardAroundCharacterRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardAroundCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardAroundPlayer(ClientModels::GetLeaderboardAroundPlayerRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardAroundPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardForUserCharacters(ClientModels::GetLeaderboardForUsersCharactersRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardForUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPaymentToken(ClientModels::GetPaymentTokenRequest& request, const ProcessApiCallback<ClientModels::GetPaymentTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPhotonAuthenticationToken(ClientModels::GetPhotonAuthenticationTokenRequest& request, const ProcessApiCallback<ClientModels::GetPhotonAuthenticationTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerCombinedInfo(ClientModels::GetPlayerCombinedInfoRequest& request, const ProcessApiCallback<ClientModels::GetPlayerCombinedInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerProfile(ClientModels::GetPlayerProfileRequest& request, const ProcessApiCallback<ClientModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerSegments(ClientModels::GetPlayerSegmentsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatistics(ClientModels::GetPlayerStatisticsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatisticVersions(ClientModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerTags(ClientModels::GetPlayerTagsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerTrades(ClientModels::GetPlayerTradesRequest& request, const ProcessApiCallback<ClientModels::GetPlayerTradesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromFacebookIDs(ClientModels::GetPlayFabIDsFromFacebookIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromFacebookIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromFacebookInstantGamesIds(ClientModels::GetPlayFabIDsFromFacebookInstantGamesIdsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromFacebookInstantGamesIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromGameCenterIDs(ClientModels::GetPlayFabIDsFromGameCenterIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGameCenterIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromGenericIDs(ClientModels::GetPlayFabIDsFromGenericIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGenericIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromGoogleIDs(ClientModels::GetPlayFabIDsFromGoogleIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGoogleIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromKongregateIDs(ClientModels::GetPlayFabIDsFromKongregateIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromKongregateIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromNintendoSwitchDeviceIds(ClientModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromPSNAccountIDs(ClientModels::GetPlayFabIDsFromPSNAccountIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromPSNAccountIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromSteamIDs(ClientModels::GetPlayFabIDsFromSteamIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromSteamIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromTwitchIDs(ClientModels::GetPlayFabIDsFromTwitchIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromTwitchIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromXboxLiveIDs(ClientModels::GetPlayFabIDsFromXboxLiveIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromXboxLiveIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPublisherData(ClientModels::GetPublisherDataRequest& request, const ProcessApiCallback<ClientModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPurchase(ClientModels::GetPurchaseRequest& request, const ProcessApiCallback<ClientModels::GetPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetSharedGroupData(ClientModels::GetSharedGroupDataRequest& request, const ProcessApiCallback<ClientModels::GetSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetStoreItems(ClientModels::GetStoreItemsRequest& request, const ProcessApiCallback<ClientModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTime(ClientModels::GetTimeRequest& request, const ProcessApiCallback<ClientModels::GetTimeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleData(ClientModels::GetTitleDataRequest& request, const ProcessApiCallback<ClientModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleNews(ClientModels::GetTitleNewsRequest& request, const ProcessApiCallback<ClientModels::GetTitleNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitlePublicKey(ClientModels::GetTitlePublicKeyRequest& request, const ProcessApiCallback<ClientModels::GetTitlePublicKeyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTradeStatus(ClientModels::GetTradeStatusRequest& request, const ProcessApiCallback<ClientModels::GetTradeStatusResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserInventory(ClientModels::GetUserInventoryRequest& request, const ProcessApiCallback<ClientModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherReadOnlyData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserReadOnlyData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantCharacterToUser(ClientModels::GrantCharacterToUserRequest& request, const ProcessApiCallback<ClientModels::GrantCharacterToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkAndroidDeviceID(ClientModels::LinkAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LinkAndroidDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkApple(ClientModels::LinkAppleRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkCustomID(ClientModels::LinkCustomIDRequest& request, const ProcessApiCallback<ClientModels::LinkCustomIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkFacebookAccount(ClientModels::LinkFacebookAccountRequest& request, const ProcessApiCallback<ClientModels::LinkFacebookAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkFacebookInstantGamesId(ClientModels::LinkFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::LinkFacebookInstantGamesIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkGameCenterAccount(ClientModels::LinkGameCenterAccountRequest& request, const ProcessApiCallback<ClientModels::LinkGameCenterAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkGoogleAccount(ClientModels::LinkGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::LinkGoogleAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkIOSDeviceID(ClientModels::LinkIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LinkIOSDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkKongregate(ClientModels::LinkKongregateAccountRequest& request, const ProcessApiCallback<ClientModels::LinkKongregateAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkNintendoServiceAccount(ClientModels::LinkNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkNintendoSwitchDeviceId(ClientModels::LinkNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::LinkNintendoSwitchDeviceIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkOpenIdConnect(ClientModels::LinkOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkPSNAccount(ClientModels::LinkPSNAccountRequest& request, const ProcessApiCallback<ClientModels::LinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkSteamAccount(ClientModels::LinkSteamAccountRequest& request, const ProcessApiCallback<ClientModels::LinkSteamAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkTwitch(ClientModels::LinkTwitchAccountRequest& request, const ProcessApiCallback<ClientModels::LinkTwitchAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkXboxAccount(ClientModels::LinkXboxAccountRequest& request, const ProcessApiCallback<ClientModels::LinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithAndroidDeviceID(ClientModels::LoginWithAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithApple(ClientModels::LoginWithAppleRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithCustomID(ClientModels::LoginWithCustomIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithEmailAddress(ClientModels::LoginWithEmailAddressRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithFacebook(ClientModels::LoginWithFacebookRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithFacebookInstantGamesId(ClientModels::LoginWithFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithGameCenter(ClientModels::LoginWithGameCenterRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithGoogleAccount(ClientModels::LoginWithGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithIOSDeviceID(ClientModels::LoginWithIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithKongregate(ClientModels::LoginWithKongregateRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithNintendoServiceAccount(ClientModels::LoginWithNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithNintendoSwitchDeviceId(ClientModels::LoginWithNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithOpenIdConnect(ClientModels::LoginWithOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithPlayFab(ClientModels::LoginWithPlayFabRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithPSN(ClientModels::LoginWithPSNRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithSteam(ClientModels::LoginWithSteamRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithTwitch(ClientModels::LoginWithTwitchRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithXbox(ClientModels::LoginWithXboxRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void Matchmake(ClientModels::MatchmakeRequest& request, const ProcessApiCallback<ClientModels::MatchmakeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void OpenTrade(ClientModels::OpenTradeRequest& request, const ProcessApiCallback<ClientModels::OpenTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PayForPurchase(ClientModels::PayForPurchaseRequest& request, const ProcessApiCallback<ClientModels::PayForPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PurchaseItem(ClientModels::PurchaseItemRequest& request, const ProcessApiCallback<ClientModels::PurchaseItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RedeemCoupon(ClientModels::RedeemCouponRequest& request, const ProcessApiCallback<ClientModels::RedeemCouponResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RefreshPSNAuthToken(ClientModels::RefreshPSNAuthTokenRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RegisterForIOSPushNotification(ClientModels::RegisterForIOSPushNotificationRequest& request, const ProcessApiCallback<ClientModels::RegisterForIOSPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RegisterPlayFabUser(ClientModels::RegisterPlayFabUserRequest& request, const ProcessApiCallback<ClientModels::RegisterPlayFabUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveContactEmail(ClientModels::RemoveContactEmailRequest& request, const ProcessApiCallback<ClientModels::RemoveContactEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveFriend(ClientModels::RemoveFriendRequest& request, const ProcessApiCallback<ClientModels::RemoveFriendResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveGenericID(ClientModels::RemoveGenericIDRequest& request, const ProcessApiCallback<ClientModels::RemoveGenericIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveSharedGroupMembers(ClientModels::RemoveSharedGroupMembersRequest& request, const ProcessApiCallback<ClientModels::RemoveSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ReportAdActivity(ClientModels::ReportAdActivityRequest& request, const ProcessApiCallback<ClientModels::ReportAdActivityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ReportDeviceInfo(ClientModels::DeviceInfoRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ReportPlayer(ClientModels::ReportPlayerClientRequest& request, const ProcessApiCallback<ClientModels::ReportPlayerClientResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RestoreIOSPurchases(ClientModels::RestoreIOSPurchasesRequest& request, const ProcessApiCallback<ClientModels::RestoreIOSPurchasesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RewardAdActivity(ClientModels::RewardAdActivityRequest& request, const ProcessApiCallback<ClientModels::RewardAdActivityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendAccountRecoveryEmail(ClientModels::SendAccountRecoveryEmailRequest& request, const ProcessApiCallback<ClientModels::SendAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetFriendTags(ClientModels::SetFriendTagsRequest& request, const ProcessApiCallback<ClientModels::SetFriendTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPlayerSecret(ClientModels::SetPlayerSecretRequest& request, const ProcessApiCallback<ClientModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void StartGame(ClientModels::StartGameRequest& request, const ProcessApiCallback<ClientModels::StartGameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void StartPurchase(ClientModels::StartPurchaseRequest& request, const ProcessApiCallback<ClientModels::StartPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SubtractUserVirtualCurrency(ClientModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<ClientModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkAndroidDeviceID(ClientModels::UnlinkAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkAndroidDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkApple(ClientModels::UnlinkAppleRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkCustomID(ClientModels::UnlinkCustomIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkCustomIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkFacebookAccount(ClientModels::UnlinkFacebookAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkFacebookAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkFacebookInstantGamesId(ClientModels::UnlinkFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::UnlinkFacebookInstantGamesIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkGameCenterAccount(ClientModels::UnlinkGameCenterAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkGameCenterAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkGoogleAccount(ClientModels::UnlinkGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkGoogleAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkIOSDeviceID(ClientModels::UnlinkIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkIOSDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkKongregate(ClientModels::UnlinkKongregateAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkKongregateAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkNintendoServiceAccount(ClientModels::UnlinkNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkNintendoSwitchDeviceId(ClientModels::UnlinkNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::UnlinkNintendoSwitchDeviceIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkOpenIdConnect(ClientModels::UnlinkOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkPSNAccount(ClientModels::UnlinkPSNAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkSteamAccount(ClientModels::UnlinkSteamAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkSteamAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkTwitch(ClientModels::UnlinkTwitchAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkTwitchAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkXboxAccount(ClientModels::UnlinkXboxAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlockContainerInstance(ClientModels::UnlockContainerInstanceRequest& request, const ProcessApiCallback<ClientModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlockContainerItem(ClientModels::UnlockContainerItemRequest& request, const ProcessApiCallback<ClientModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateAvatarUrl(ClientModels::UpdateAvatarUrlRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterData(ClientModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ClientModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterStatistics(ClientModels::UpdateCharacterStatisticsRequest& request, const ProcessApiCallback<ClientModels::UpdateCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdatePlayerStatistics(ClientModels::UpdatePlayerStatisticsRequest& request, const ProcessApiCallback<ClientModels::UpdatePlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateSharedGroupData(ClientModels::UpdateSharedGroupDataRequest& request, const ProcessApiCallback<ClientModels::UpdateSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserData(ClientModels::UpdateUserDataRequest& request, const ProcessApiCallback<ClientModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherData(ClientModels::UpdateUserDataRequest& request, const ProcessApiCallback<ClientModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserTitleDisplayName(ClientModels::UpdateUserTitleDisplayNameRequest& request, const ProcessApiCallback<ClientModels::UpdateUserTitleDisplayNameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ValidateAmazonIAPReceipt(ClientModels::ValidateAmazonReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateAmazonReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ValidateGooglePlayPurchase(ClientModels::ValidateGooglePlayPurchaseRequest& request, const ProcessApiCallback<ClientModels::ValidateGooglePlayPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ValidateIOSReceipt(ClientModels::ValidateIOSReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateIOSReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ValidateWindowsStoreReceipt(ClientModels::ValidateWindowsReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateWindowsReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WriteCharacterEvent(ClientModels::WriteClientCharacterEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WritePlayerEvent(ClientModels::WriteClientPlayerEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WriteTitleEvent(ClientModels::WriteTitleEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnAcceptTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddOrUpdateContactEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddUsernamePasswordResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAndroidDevicePushNotificationRegistrationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAttributeInstallResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCancelTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConfirmPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumeItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumeMicrosoftStoreEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumePS5EntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumePSNEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumeXboxEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnExecuteCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAdPlacementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAllUsersCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetContentDownloadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCurrentGamesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFriendLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFriendLeaderboardAroundPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFriendsListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetGameServerRegionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardAroundCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardAroundPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardForUserCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPaymentTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPhotonAuthenticationTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerCombinedInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerTradesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromFacebookIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromFacebookInstantGamesIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromGameCenterIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromGenericIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromGoogleIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromKongregateIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromNintendoSwitchDeviceIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromPSNAccountIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromSteamIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromTwitchIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromXboxLiveIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTimeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitlePublicKeyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTradeStatusResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantCharacterToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkFacebookAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkGameCenterAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkSteamAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithEmailAddressResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithFacebookResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithGameCenterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithPlayFabResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithPSNResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithSteamResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithXboxResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnMatchmakeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnOpenTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPayForPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPurchaseItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRedeemCouponResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRefreshPSNAuthTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRegisterForIOSPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRegisterPlayFabUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveContactEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnReportAdActivityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnReportDeviceInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnReportPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRestoreIOSPurchasesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRewardAdActivityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetFriendTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnStartGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnStartPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkFacebookAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkGameCenterAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkSteamAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlockContainerInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlockContainerItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateAvatarUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdatePlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserTitleDisplayNameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnValidateAmazonIAPReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnValidateGooglePlayPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnValidateIOSReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnValidateWindowsStoreReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWriteCharacterEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWritePlayerEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWriteTitleEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
