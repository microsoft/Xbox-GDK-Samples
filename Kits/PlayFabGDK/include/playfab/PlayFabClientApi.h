#pragma once

#if !defined(DISABLE_PLAYFABCLIENT_API)

#include <playfab/PlayFabClientDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Client APIs
    /// </summary>
    class PlayFabClientAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Client.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // Public, Client-Specific
        static bool IsClientLoggedIn();

        // ------------ Generated API calls
        static void AcceptTrade(ClientModels::AcceptTradeRequest& request, const ProcessApiCallback<ClientModels::AcceptTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddFriend(ClientModels::AddFriendRequest& request, const ProcessApiCallback<ClientModels::AddFriendResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddGenericID(ClientModels::AddGenericIDRequest& request, const ProcessApiCallback<ClientModels::AddGenericIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddOrUpdateContactEmail(ClientModels::AddOrUpdateContactEmailRequest& request, const ProcessApiCallback<ClientModels::AddOrUpdateContactEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddSharedGroupMembers(ClientModels::AddSharedGroupMembersRequest& request, const ProcessApiCallback<ClientModels::AddSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddUsernamePassword(ClientModels::AddUsernamePasswordRequest& request, const ProcessApiCallback<ClientModels::AddUsernamePasswordResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddUserVirtualCurrency(ClientModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<ClientModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AndroidDevicePushNotificationRegistration(ClientModels::AndroidDevicePushNotificationRegistrationRequest& request, const ProcessApiCallback<ClientModels::AndroidDevicePushNotificationRegistrationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AttributeInstall(ClientModels::AttributeInstallRequest& request, const ProcessApiCallback<ClientModels::AttributeInstallResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CancelTrade(ClientModels::CancelTradeRequest& request, const ProcessApiCallback<ClientModels::CancelTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConfirmPurchase(ClientModels::ConfirmPurchaseRequest& request, const ProcessApiCallback<ClientModels::ConfirmPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumeItem(ClientModels::ConsumeItemRequest& request, const ProcessApiCallback<ClientModels::ConsumeItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumeMicrosoftStoreEntitlements(ClientModels::ConsumeMicrosoftStoreEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumeMicrosoftStoreEntitlementsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumePS5Entitlements(ClientModels::ConsumePS5EntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumePS5EntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumePSNEntitlements(ClientModels::ConsumePSNEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumePSNEntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumeXboxEntitlements(ClientModels::ConsumeXboxEntitlementsRequest& request, const ProcessApiCallback<ClientModels::ConsumeXboxEntitlementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateSharedGroup(ClientModels::CreateSharedGroupRequest& request, const ProcessApiCallback<ClientModels::CreateSharedGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ExecuteCloudScript(ClientModels::ExecuteCloudScriptRequest& request, const ProcessApiCallback<ClientModels::ExecuteCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAccountInfo(ClientModels::GetAccountInfoRequest& request, const ProcessApiCallback<ClientModels::GetAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAdPlacements(ClientModels::GetAdPlacementsRequest& request, const ProcessApiCallback<ClientModels::GetAdPlacementsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAllUsersCharacters(ClientModels::ListUsersCharactersRequest& request, const ProcessApiCallback<ClientModels::ListUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCatalogItems(ClientModels::GetCatalogItemsRequest& request, const ProcessApiCallback<ClientModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterData(ClientModels::GetCharacterDataRequest& request, const ProcessApiCallback<ClientModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterInventory(ClientModels::GetCharacterInventoryRequest& request, const ProcessApiCallback<ClientModels::GetCharacterInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterLeaderboard(ClientModels::GetCharacterLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetCharacterLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterReadOnlyData(ClientModels::GetCharacterDataRequest& request, const ProcessApiCallback<ClientModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterStatistics(ClientModels::GetCharacterStatisticsRequest& request, const ProcessApiCallback<ClientModels::GetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetContentDownloadUrl(ClientModels::GetContentDownloadUrlRequest& request, const ProcessApiCallback<ClientModels::GetContentDownloadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCurrentGames(ClientModels::CurrentGamesRequest& request, const ProcessApiCallback<ClientModels::CurrentGamesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFriendLeaderboard(ClientModels::GetFriendLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFriendLeaderboardAroundPlayer(ClientModels::GetFriendLeaderboardAroundPlayerRequest& request, const ProcessApiCallback<ClientModels::GetFriendLeaderboardAroundPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFriendsList(ClientModels::GetFriendsListRequest& request, const ProcessApiCallback<ClientModels::GetFriendsListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetGameServerRegions(ClientModels::GameServerRegionsRequest& request, const ProcessApiCallback<ClientModels::GameServerRegionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboard(ClientModels::GetLeaderboardRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardAroundCharacter(ClientModels::GetLeaderboardAroundCharacterRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardAroundCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardAroundPlayer(ClientModels::GetLeaderboardAroundPlayerRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardAroundPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardForUserCharacters(ClientModels::GetLeaderboardForUsersCharactersRequest& request, const ProcessApiCallback<ClientModels::GetLeaderboardForUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPaymentToken(ClientModels::GetPaymentTokenRequest& request, const ProcessApiCallback<ClientModels::GetPaymentTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPhotonAuthenticationToken(ClientModels::GetPhotonAuthenticationTokenRequest& request, const ProcessApiCallback<ClientModels::GetPhotonAuthenticationTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerCombinedInfo(ClientModels::GetPlayerCombinedInfoRequest& request, const ProcessApiCallback<ClientModels::GetPlayerCombinedInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerProfile(ClientModels::GetPlayerProfileRequest& request, const ProcessApiCallback<ClientModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerSegments(ClientModels::GetPlayerSegmentsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatistics(ClientModels::GetPlayerStatisticsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatisticVersions(ClientModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerTags(ClientModels::GetPlayerTagsRequest& request, const ProcessApiCallback<ClientModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerTrades(ClientModels::GetPlayerTradesRequest& request, const ProcessApiCallback<ClientModels::GetPlayerTradesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromFacebookIDs(ClientModels::GetPlayFabIDsFromFacebookIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromFacebookIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromFacebookInstantGamesIds(ClientModels::GetPlayFabIDsFromFacebookInstantGamesIdsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromFacebookInstantGamesIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromGameCenterIDs(ClientModels::GetPlayFabIDsFromGameCenterIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGameCenterIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromGenericIDs(ClientModels::GetPlayFabIDsFromGenericIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGenericIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromGoogleIDs(ClientModels::GetPlayFabIDsFromGoogleIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromGoogleIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromKongregateIDs(ClientModels::GetPlayFabIDsFromKongregateIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromKongregateIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromNintendoSwitchDeviceIds(ClientModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromPSNAccountIDs(ClientModels::GetPlayFabIDsFromPSNAccountIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromPSNAccountIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromSteamIDs(ClientModels::GetPlayFabIDsFromSteamIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromSteamIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromTwitchIDs(ClientModels::GetPlayFabIDsFromTwitchIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromTwitchIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromXboxLiveIDs(ClientModels::GetPlayFabIDsFromXboxLiveIDsRequest& request, const ProcessApiCallback<ClientModels::GetPlayFabIDsFromXboxLiveIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPublisherData(ClientModels::GetPublisherDataRequest& request, const ProcessApiCallback<ClientModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPurchase(ClientModels::GetPurchaseRequest& request, const ProcessApiCallback<ClientModels::GetPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetSharedGroupData(ClientModels::GetSharedGroupDataRequest& request, const ProcessApiCallback<ClientModels::GetSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetStoreItems(ClientModels::GetStoreItemsRequest& request, const ProcessApiCallback<ClientModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTime(ClientModels::GetTimeRequest& request, const ProcessApiCallback<ClientModels::GetTimeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleData(ClientModels::GetTitleDataRequest& request, const ProcessApiCallback<ClientModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleNews(ClientModels::GetTitleNewsRequest& request, const ProcessApiCallback<ClientModels::GetTitleNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitlePublicKey(ClientModels::GetTitlePublicKeyRequest& request, const ProcessApiCallback<ClientModels::GetTitlePublicKeyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTradeStatus(ClientModels::GetTradeStatusRequest& request, const ProcessApiCallback<ClientModels::GetTradeStatusResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserInventory(ClientModels::GetUserInventoryRequest& request, const ProcessApiCallback<ClientModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherReadOnlyData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserReadOnlyData(ClientModels::GetUserDataRequest& request, const ProcessApiCallback<ClientModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantCharacterToUser(ClientModels::GrantCharacterToUserRequest& request, const ProcessApiCallback<ClientModels::GrantCharacterToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkAndroidDeviceID(ClientModels::LinkAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LinkAndroidDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkApple(ClientModels::LinkAppleRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkCustomID(ClientModels::LinkCustomIDRequest& request, const ProcessApiCallback<ClientModels::LinkCustomIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkFacebookAccount(ClientModels::LinkFacebookAccountRequest& request, const ProcessApiCallback<ClientModels::LinkFacebookAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkFacebookInstantGamesId(ClientModels::LinkFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::LinkFacebookInstantGamesIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkGameCenterAccount(ClientModels::LinkGameCenterAccountRequest& request, const ProcessApiCallback<ClientModels::LinkGameCenterAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkGoogleAccount(ClientModels::LinkGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::LinkGoogleAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkIOSDeviceID(ClientModels::LinkIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LinkIOSDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkKongregate(ClientModels::LinkKongregateAccountRequest& request, const ProcessApiCallback<ClientModels::LinkKongregateAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkNintendoServiceAccount(ClientModels::LinkNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkNintendoSwitchDeviceId(ClientModels::LinkNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::LinkNintendoSwitchDeviceIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkOpenIdConnect(ClientModels::LinkOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkPSNAccount(ClientModels::LinkPSNAccountRequest& request, const ProcessApiCallback<ClientModels::LinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkSteamAccount(ClientModels::LinkSteamAccountRequest& request, const ProcessApiCallback<ClientModels::LinkSteamAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkTwitch(ClientModels::LinkTwitchAccountRequest& request, const ProcessApiCallback<ClientModels::LinkTwitchAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkXboxAccount(ClientModels::LinkXboxAccountRequest& request, const ProcessApiCallback<ClientModels::LinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithAndroidDeviceID(ClientModels::LoginWithAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithApple(ClientModels::LoginWithAppleRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithCustomID(ClientModels::LoginWithCustomIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithEmailAddress(ClientModels::LoginWithEmailAddressRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithFacebook(ClientModels::LoginWithFacebookRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithFacebookInstantGamesId(ClientModels::LoginWithFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithGameCenter(ClientModels::LoginWithGameCenterRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithGoogleAccount(ClientModels::LoginWithGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithIOSDeviceID(ClientModels::LoginWithIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithKongregate(ClientModels::LoginWithKongregateRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithNintendoServiceAccount(ClientModels::LoginWithNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithNintendoSwitchDeviceId(ClientModels::LoginWithNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithOpenIdConnect(ClientModels::LoginWithOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithPlayFab(ClientModels::LoginWithPlayFabRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithPSN(ClientModels::LoginWithPSNRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithSteam(ClientModels::LoginWithSteamRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithTwitch(ClientModels::LoginWithTwitchRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithXbox(ClientModels::LoginWithXboxRequest& request, const ProcessApiCallback<ClientModels::LoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void Matchmake(ClientModels::MatchmakeRequest& request, const ProcessApiCallback<ClientModels::MatchmakeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void OpenTrade(ClientModels::OpenTradeRequest& request, const ProcessApiCallback<ClientModels::OpenTradeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void PayForPurchase(ClientModels::PayForPurchaseRequest& request, const ProcessApiCallback<ClientModels::PayForPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void PurchaseItem(ClientModels::PurchaseItemRequest& request, const ProcessApiCallback<ClientModels::PurchaseItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RedeemCoupon(ClientModels::RedeemCouponRequest& request, const ProcessApiCallback<ClientModels::RedeemCouponResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RefreshPSNAuthToken(ClientModels::RefreshPSNAuthTokenRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RegisterForIOSPushNotification(ClientModels::RegisterForIOSPushNotificationRequest& request, const ProcessApiCallback<ClientModels::RegisterForIOSPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RegisterPlayFabUser(ClientModels::RegisterPlayFabUserRequest& request, const ProcessApiCallback<ClientModels::RegisterPlayFabUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveContactEmail(ClientModels::RemoveContactEmailRequest& request, const ProcessApiCallback<ClientModels::RemoveContactEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveFriend(ClientModels::RemoveFriendRequest& request, const ProcessApiCallback<ClientModels::RemoveFriendResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveGenericID(ClientModels::RemoveGenericIDRequest& request, const ProcessApiCallback<ClientModels::RemoveGenericIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveSharedGroupMembers(ClientModels::RemoveSharedGroupMembersRequest& request, const ProcessApiCallback<ClientModels::RemoveSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ReportAdActivity(ClientModels::ReportAdActivityRequest& request, const ProcessApiCallback<ClientModels::ReportAdActivityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ReportDeviceInfo(ClientModels::DeviceInfoRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ReportPlayer(ClientModels::ReportPlayerClientRequest& request, const ProcessApiCallback<ClientModels::ReportPlayerClientResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RestoreIOSPurchases(ClientModels::RestoreIOSPurchasesRequest& request, const ProcessApiCallback<ClientModels::RestoreIOSPurchasesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RewardAdActivity(ClientModels::RewardAdActivityRequest& request, const ProcessApiCallback<ClientModels::RewardAdActivityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendAccountRecoveryEmail(ClientModels::SendAccountRecoveryEmailRequest& request, const ProcessApiCallback<ClientModels::SendAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetFriendTags(ClientModels::SetFriendTagsRequest& request, const ProcessApiCallback<ClientModels::SetFriendTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPlayerSecret(ClientModels::SetPlayerSecretRequest& request, const ProcessApiCallback<ClientModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void StartGame(ClientModels::StartGameRequest& request, const ProcessApiCallback<ClientModels::StartGameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void StartPurchase(ClientModels::StartPurchaseRequest& request, const ProcessApiCallback<ClientModels::StartPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SubtractUserVirtualCurrency(ClientModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<ClientModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkAndroidDeviceID(ClientModels::UnlinkAndroidDeviceIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkAndroidDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkApple(ClientModels::UnlinkAppleRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkCustomID(ClientModels::UnlinkCustomIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkCustomIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkFacebookAccount(ClientModels::UnlinkFacebookAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkFacebookAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkFacebookInstantGamesId(ClientModels::UnlinkFacebookInstantGamesIdRequest& request, const ProcessApiCallback<ClientModels::UnlinkFacebookInstantGamesIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkGameCenterAccount(ClientModels::UnlinkGameCenterAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkGameCenterAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkGoogleAccount(ClientModels::UnlinkGoogleAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkGoogleAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkIOSDeviceID(ClientModels::UnlinkIOSDeviceIDRequest& request, const ProcessApiCallback<ClientModels::UnlinkIOSDeviceIDResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkKongregate(ClientModels::UnlinkKongregateAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkKongregateAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkNintendoServiceAccount(ClientModels::UnlinkNintendoServiceAccountRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkNintendoSwitchDeviceId(ClientModels::UnlinkNintendoSwitchDeviceIdRequest& request, const ProcessApiCallback<ClientModels::UnlinkNintendoSwitchDeviceIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkOpenIdConnect(ClientModels::UnlinkOpenIdConnectRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkPSNAccount(ClientModels::UnlinkPSNAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkSteamAccount(ClientModels::UnlinkSteamAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkSteamAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkTwitch(ClientModels::UnlinkTwitchAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkTwitchAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkXboxAccount(ClientModels::UnlinkXboxAccountRequest& request, const ProcessApiCallback<ClientModels::UnlinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlockContainerInstance(ClientModels::UnlockContainerInstanceRequest& request, const ProcessApiCallback<ClientModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlockContainerItem(ClientModels::UnlockContainerItemRequest& request, const ProcessApiCallback<ClientModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateAvatarUrl(ClientModels::UpdateAvatarUrlRequest& request, const ProcessApiCallback<ClientModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterData(ClientModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ClientModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterStatistics(ClientModels::UpdateCharacterStatisticsRequest& request, const ProcessApiCallback<ClientModels::UpdateCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdatePlayerStatistics(ClientModels::UpdatePlayerStatisticsRequest& request, const ProcessApiCallback<ClientModels::UpdatePlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateSharedGroupData(ClientModels::UpdateSharedGroupDataRequest& request, const ProcessApiCallback<ClientModels::UpdateSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserData(ClientModels::UpdateUserDataRequest& request, const ProcessApiCallback<ClientModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherData(ClientModels::UpdateUserDataRequest& request, const ProcessApiCallback<ClientModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserTitleDisplayName(ClientModels::UpdateUserTitleDisplayNameRequest& request, const ProcessApiCallback<ClientModels::UpdateUserTitleDisplayNameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ValidateAmazonIAPReceipt(ClientModels::ValidateAmazonReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateAmazonReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ValidateGooglePlayPurchase(ClientModels::ValidateGooglePlayPurchaseRequest& request, const ProcessApiCallback<ClientModels::ValidateGooglePlayPurchaseResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ValidateIOSReceipt(ClientModels::ValidateIOSReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateIOSReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ValidateWindowsStoreReceipt(ClientModels::ValidateWindowsReceiptRequest& request, const ProcessApiCallback<ClientModels::ValidateWindowsReceiptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WriteCharacterEvent(ClientModels::WriteClientCharacterEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WritePlayerEvent(ClientModels::WriteClientPlayerEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WriteTitleEvent(ClientModels::WriteTitleEventRequest& request, const ProcessApiCallback<ClientModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabClientAPI(); // Private constructor, static class should never have an instance
        PlayFabClientAPI(const PlayFabClientAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAcceptTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddOrUpdateContactEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddUsernamePasswordResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAndroidDevicePushNotificationRegistrationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAttributeInstallResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCancelTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConfirmPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumeItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumeMicrosoftStoreEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumePS5EntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumePSNEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumeXboxEntitlementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnExecuteCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAdPlacementsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAllUsersCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetContentDownloadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCurrentGamesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFriendLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFriendLeaderboardAroundPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFriendsListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetGameServerRegionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardAroundCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardAroundPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardForUserCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPaymentTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPhotonAuthenticationTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerCombinedInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerTradesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromFacebookIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromFacebookInstantGamesIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromGameCenterIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromGenericIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromGoogleIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromKongregateIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromNintendoSwitchDeviceIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromPSNAccountIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromSteamIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromTwitchIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromXboxLiveIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTimeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitlePublicKeyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTradeStatusResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantCharacterToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkFacebookAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkGameCenterAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkSteamAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithEmailAddressResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithFacebookResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithGameCenterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithPlayFabResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithPSNResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithSteamResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithXboxResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnMatchmakeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnOpenTradeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnPayForPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnPurchaseItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRedeemCouponResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRefreshPSNAuthTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRegisterForIOSPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRegisterPlayFabUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveContactEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnReportAdActivityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnReportDeviceInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnReportPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRestoreIOSPurchasesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRewardAdActivityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetFriendTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnStartGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnStartPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkAndroidDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkAppleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkCustomIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkFacebookAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkFacebookInstantGamesIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkGameCenterAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkGoogleAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkIOSDeviceIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkKongregateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkNintendoServiceAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkNintendoSwitchDeviceIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkOpenIdConnectResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkSteamAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkTwitchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlockContainerInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlockContainerItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateAvatarUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdatePlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserTitleDisplayNameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnValidateAmazonIAPReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnValidateGooglePlayPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnValidateIOSReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnValidateWindowsStoreReceiptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWriteCharacterEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWritePlayerEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWriteTitleEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABCLIENT_API)
