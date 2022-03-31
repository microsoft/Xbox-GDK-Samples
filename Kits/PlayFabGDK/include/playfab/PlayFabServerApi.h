#pragma once

#if defined(ENABLE_PLAYFABSERVER_API)

#include <playfab/PlayFabServerDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Server APIs
    /// </summary>
    class PlayFabServerAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Server.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void AddCharacterVirtualCurrency(ServerModels::AddCharacterVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyCharacterVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddFriend(ServerModels::AddFriendRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddGenericID(ServerModels::AddGenericIDRequest& request, const ProcessApiCallback<ServerModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddPlayerTag(ServerModels::AddPlayerTagRequest& request, const ProcessApiCallback<ServerModels::AddPlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddSharedGroupMembers(ServerModels::AddSharedGroupMembersRequest& request, const ProcessApiCallback<ServerModels::AddSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddUserVirtualCurrency(ServerModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AuthenticateSessionTicket(ServerModels::AuthenticateSessionTicketRequest& request, const ProcessApiCallback<ServerModels::AuthenticateSessionTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AwardSteamAchievement(ServerModels::AwardSteamAchievementRequest& request, const ProcessApiCallback<ServerModels::AwardSteamAchievementResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void BanUsers(ServerModels::BanUsersRequest& request, const ProcessApiCallback<ServerModels::BanUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ConsumeItem(ServerModels::ConsumeItemRequest& request, const ProcessApiCallback<ServerModels::ConsumeItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateSharedGroup(ServerModels::CreateSharedGroupRequest& request, const ProcessApiCallback<ServerModels::CreateSharedGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteCharacterFromUser(ServerModels::DeleteCharacterFromUserRequest& request, const ProcessApiCallback<ServerModels::DeleteCharacterFromUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeletePlayer(ServerModels::DeletePlayerRequest& request, const ProcessApiCallback<ServerModels::DeletePlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeletePushNotificationTemplate(ServerModels::DeletePushNotificationTemplateRequest& request, const ProcessApiCallback<ServerModels::DeletePushNotificationTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteSharedGroup(ServerModels::DeleteSharedGroupRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeregisterGame(ServerModels::DeregisterGameRequest& request, const ProcessApiCallback<ServerModels::DeregisterGameResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void EvaluateRandomResultTable(ServerModels::EvaluateRandomResultTableRequest& request, const ProcessApiCallback<ServerModels::EvaluateRandomResultTableResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ExecuteCloudScript(ServerModels::ExecuteCloudScriptServerRequest& request, const ProcessApiCallback<ServerModels::ExecuteCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAllSegments(ServerModels::GetAllSegmentsRequest& request, const ProcessApiCallback<ServerModels::GetAllSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAllUsersCharacters(ServerModels::ListUsersCharactersRequest& request, const ProcessApiCallback<ServerModels::ListUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCatalogItems(ServerModels::GetCatalogItemsRequest& request, const ProcessApiCallback<ServerModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterInternalData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterInventory(ServerModels::GetCharacterInventoryRequest& request, const ProcessApiCallback<ServerModels::GetCharacterInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterLeaderboard(ServerModels::GetCharacterLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetCharacterLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterReadOnlyData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCharacterStatistics(ServerModels::GetCharacterStatisticsRequest& request, const ProcessApiCallback<ServerModels::GetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetContentDownloadUrl(ServerModels::GetContentDownloadUrlRequest& request, const ProcessApiCallback<ServerModels::GetContentDownloadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFriendLeaderboard(ServerModels::GetFriendLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetFriendsList(ServerModels::GetFriendsListRequest& request, const ProcessApiCallback<ServerModels::GetFriendsListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboard(ServerModels::GetLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardAroundCharacter(ServerModels::GetLeaderboardAroundCharacterRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardAroundCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardAroundUser(ServerModels::GetLeaderboardAroundUserRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardAroundUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetLeaderboardForUserCharacters(ServerModels::GetLeaderboardForUsersCharactersRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardForUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerCombinedInfo(ServerModels::GetPlayerCombinedInfoRequest& request, const ProcessApiCallback<ServerModels::GetPlayerCombinedInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerProfile(ServerModels::GetPlayerProfileRequest& request, const ProcessApiCallback<ServerModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerSegments(ServerModels::GetPlayersSegmentsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayersInSegment(ServerModels::GetPlayersInSegmentRequest& request, const ProcessApiCallback<ServerModels::GetPlayersInSegmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatistics(ServerModels::GetPlayerStatisticsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatisticVersions(ServerModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerTags(ServerModels::GetPlayerTagsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromFacebookIDs(ServerModels::GetPlayFabIDsFromFacebookIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromFacebookIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromFacebookInstantGamesIds(ServerModels::GetPlayFabIDsFromFacebookInstantGamesIdsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromFacebookInstantGamesIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromGenericIDs(ServerModels::GetPlayFabIDsFromGenericIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromGenericIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromNintendoSwitchDeviceIds(ServerModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromPSNAccountIDs(ServerModels::GetPlayFabIDsFromPSNAccountIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromPSNAccountIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromSteamIDs(ServerModels::GetPlayFabIDsFromSteamIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromSteamIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayFabIDsFromXboxLiveIDs(ServerModels::GetPlayFabIDsFromXboxLiveIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromXboxLiveIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPublisherData(ServerModels::GetPublisherDataRequest& request, const ProcessApiCallback<ServerModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetRandomResultTables(ServerModels::GetRandomResultTablesRequest& request, const ProcessApiCallback<ServerModels::GetRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetServerCustomIDsFromPlayFabIDs(ServerModels::GetServerCustomIDsFromPlayFabIDsRequest& request, const ProcessApiCallback<ServerModels::GetServerCustomIDsFromPlayFabIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetSharedGroupData(ServerModels::GetSharedGroupDataRequest& request, const ProcessApiCallback<ServerModels::GetSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetStoreItems(ServerModels::GetStoreItemsServerRequest& request, const ProcessApiCallback<ServerModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTime(ServerModels::GetTimeRequest& request, const ProcessApiCallback<ServerModels::GetTimeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleData(ServerModels::GetTitleDataRequest& request, const ProcessApiCallback<ServerModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleInternalData(ServerModels::GetTitleDataRequest& request, const ProcessApiCallback<ServerModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleNews(ServerModels::GetTitleNewsRequest& request, const ProcessApiCallback<ServerModels::GetTitleNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserAccountInfo(ServerModels::GetUserAccountInfoRequest& request, const ProcessApiCallback<ServerModels::GetUserAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserBans(ServerModels::GetUserBansRequest& request, const ProcessApiCallback<ServerModels::GetUserBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserInternalData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserInventory(ServerModels::GetUserInventoryRequest& request, const ProcessApiCallback<ServerModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherInternalData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherReadOnlyData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserReadOnlyData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantCharacterToUser(ServerModels::GrantCharacterToUserRequest& request, const ProcessApiCallback<ServerModels::GrantCharacterToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantItemsToCharacter(ServerModels::GrantItemsToCharacterRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantItemsToUser(ServerModels::GrantItemsToUserRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantItemsToUsers(ServerModels::GrantItemsToUsersRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkPSNAccount(ServerModels::LinkPSNAccountRequest& request, const ProcessApiCallback<ServerModels::LinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkServerCustomId(ServerModels::LinkServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::LinkServerCustomIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LinkXboxAccount(ServerModels::LinkXboxAccountRequest& request, const ProcessApiCallback<ServerModels::LinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithServerCustomId(ServerModels::LoginWithServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithSteamId(ServerModels::LoginWithSteamIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithXbox(ServerModels::LoginWithXboxRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void LoginWithXboxId(ServerModels::LoginWithXboxIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ModifyItemUses(ServerModels::ModifyItemUsesRequest& request, const ProcessApiCallback<ServerModels::ModifyItemUsesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void MoveItemToCharacterFromCharacter(ServerModels::MoveItemToCharacterFromCharacterRequest& request, const ProcessApiCallback<ServerModels::MoveItemToCharacterFromCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void MoveItemToCharacterFromUser(ServerModels::MoveItemToCharacterFromUserRequest& request, const ProcessApiCallback<ServerModels::MoveItemToCharacterFromUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void MoveItemToUserFromCharacter(ServerModels::MoveItemToUserFromCharacterRequest& request, const ProcessApiCallback<ServerModels::MoveItemToUserFromCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void NotifyMatchmakerPlayerLeft(ServerModels::NotifyMatchmakerPlayerLeftRequest& request, const ProcessApiCallback<ServerModels::NotifyMatchmakerPlayerLeftResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RedeemCoupon(ServerModels::RedeemCouponRequest& request, const ProcessApiCallback<ServerModels::RedeemCouponResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RedeemMatchmakerTicket(ServerModels::RedeemMatchmakerTicketRequest& request, const ProcessApiCallback<ServerModels::RedeemMatchmakerTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RefreshGameServerInstanceHeartbeat(ServerModels::RefreshGameServerInstanceHeartbeatRequest& request, const ProcessApiCallback<ServerModels::RefreshGameServerInstanceHeartbeatResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RegisterGame(ServerModels::RegisterGameRequest& request, const ProcessApiCallback<ServerModels::RegisterGameResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveFriend(ServerModels::RemoveFriendRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveGenericID(ServerModels::RemoveGenericIDRequest& request, const ProcessApiCallback<ServerModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemovePlayerTag(ServerModels::RemovePlayerTagRequest& request, const ProcessApiCallback<ServerModels::RemovePlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveSharedGroupMembers(ServerModels::RemoveSharedGroupMembersRequest& request, const ProcessApiCallback<ServerModels::RemoveSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ReportPlayer(ServerModels::ReportPlayerServerRequest& request, const ProcessApiCallback<ServerModels::ReportPlayerServerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeAllBansForUser(ServerModels::RevokeAllBansForUserRequest& request, const ProcessApiCallback<ServerModels::RevokeAllBansForUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeBans(ServerModels::RevokeBansRequest& request, const ProcessApiCallback<ServerModels::RevokeBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeInventoryItem(ServerModels::RevokeInventoryItemRequest& request, const ProcessApiCallback<ServerModels::RevokeInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeInventoryItems(ServerModels::RevokeInventoryItemsRequest& request, const ProcessApiCallback<ServerModels::RevokeInventoryItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SavePushNotificationTemplate(ServerModels::SavePushNotificationTemplateRequest& request, const ProcessApiCallback<ServerModels::SavePushNotificationTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendCustomAccountRecoveryEmail(ServerModels::SendCustomAccountRecoveryEmailRequest& request, const ProcessApiCallback<ServerModels::SendCustomAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendEmailFromTemplate(ServerModels::SendEmailFromTemplateRequest& request, const ProcessApiCallback<ServerModels::SendEmailFromTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendPushNotification(ServerModels::SendPushNotificationRequest& request, const ProcessApiCallback<ServerModels::SendPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendPushNotificationFromTemplate(ServerModels::SendPushNotificationFromTemplateRequest& request, const ProcessApiCallback<ServerModels::SendPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetFriendTags(ServerModels::SetFriendTagsRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetGameServerInstanceData(ServerModels::SetGameServerInstanceDataRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetGameServerInstanceState(ServerModels::SetGameServerInstanceStateRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceStateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetGameServerInstanceTags(ServerModels::SetGameServerInstanceTagsRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPlayerSecret(ServerModels::SetPlayerSecretRequest& request, const ProcessApiCallback<ServerModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPublisherData(ServerModels::SetPublisherDataRequest& request, const ProcessApiCallback<ServerModels::SetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetTitleData(ServerModels::SetTitleDataRequest& request, const ProcessApiCallback<ServerModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetTitleInternalData(ServerModels::SetTitleDataRequest& request, const ProcessApiCallback<ServerModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SubtractCharacterVirtualCurrency(ServerModels::SubtractCharacterVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyCharacterVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SubtractUserVirtualCurrency(ServerModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkPSNAccount(ServerModels::UnlinkPSNAccountRequest& request, const ProcessApiCallback<ServerModels::UnlinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkServerCustomId(ServerModels::UnlinkServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::UnlinkServerCustomIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlinkXboxAccount(ServerModels::UnlinkXboxAccountRequest& request, const ProcessApiCallback<ServerModels::UnlinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlockContainerInstance(ServerModels::UnlockContainerInstanceRequest& request, const ProcessApiCallback<ServerModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnlockContainerItem(ServerModels::UnlockContainerItemRequest& request, const ProcessApiCallback<ServerModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateAvatarUrl(ServerModels::UpdateAvatarUrlRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateBans(ServerModels::UpdateBansRequest& request, const ProcessApiCallback<ServerModels::UpdateBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterInternalData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterReadOnlyData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCharacterStatistics(ServerModels::UpdateCharacterStatisticsRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdatePlayerStatistics(ServerModels::UpdatePlayerStatisticsRequest& request, const ProcessApiCallback<ServerModels::UpdatePlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateSharedGroupData(ServerModels::UpdateSharedGroupDataRequest& request, const ProcessApiCallback<ServerModels::UpdateSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserInternalData(ServerModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserInventoryItemCustomData(ServerModels::UpdateUserInventoryItemDataRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherInternalData(ServerModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherReadOnlyData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserReadOnlyData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WriteCharacterEvent(ServerModels::WriteServerCharacterEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WritePlayerEvent(ServerModels::WriteServerPlayerEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void WriteTitleEvent(ServerModels::WriteTitleEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabServerAPI(); // Private constructor, static class should never have an instance
        PlayFabServerAPI(const PlayFabServerAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAddCharacterVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddPlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAuthenticateSessionTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAwardSteamAchievementResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnBanUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnConsumeItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteCharacterFromUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeletePlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeletePushNotificationTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeregisterGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnEvaluateRandomResultTableResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnExecuteCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAllSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAllUsersCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetContentDownloadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFriendLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetFriendsListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardAroundCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardAroundUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetLeaderboardForUserCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerCombinedInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayersInSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromFacebookIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromFacebookInstantGamesIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromGenericIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromNintendoSwitchDeviceIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromPSNAccountIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromSteamIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayFabIDsFromXboxLiveIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetServerCustomIDsFromPlayFabIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTimeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantCharacterToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantItemsToCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantItemsToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantItemsToUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithSteamIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithXboxResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnLoginWithXboxIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnModifyItemUsesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnMoveItemToCharacterFromCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnMoveItemToCharacterFromUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnMoveItemToUserFromCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnNotifyMatchmakerPlayerLeftResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRedeemCouponResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRedeemMatchmakerTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRefreshGameServerInstanceHeartbeatResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRegisterGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemovePlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnReportPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeAllBansForUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeInventoryItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeInventoryItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSavePushNotificationTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendCustomAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendEmailFromTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendPushNotificationFromTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetFriendTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetGameServerInstanceDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetGameServerInstanceStateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetGameServerInstanceTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSubtractCharacterVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlockContainerInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnlockContainerItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateAvatarUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdatePlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserInventoryItemCustomDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWriteCharacterEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWritePlayerEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnWriteTitleEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if defined(ENABLE_PLAYFABSERVER_API)
