#pragma once

#if defined(ENABLE_PLAYFABSERVER_API)

#include <playfab/PlayFabServerDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Server APIs
    /// </summary>
    class PlayFabServerInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabServerInstanceAPI();
        PlayFabServerInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings);
        PlayFabServerInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabServerInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabServerInstanceAPI() = default;
        PlayFabServerInstanceAPI(const PlayFabServerInstanceAPI& source) = delete; // disable copy
        PlayFabServerInstanceAPI(PlayFabServerInstanceAPI&&) = delete; // disable move
        PlayFabServerInstanceAPI& operator=(const PlayFabServerInstanceAPI& source) = delete; // disable assignment
        PlayFabServerInstanceAPI& operator=(PlayFabServerInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Server.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void AddCharacterVirtualCurrency(ServerModels::AddCharacterVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyCharacterVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddFriend(ServerModels::AddFriendRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddGenericID(ServerModels::AddGenericIDRequest& request, const ProcessApiCallback<ServerModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddPlayerTag(ServerModels::AddPlayerTagRequest& request, const ProcessApiCallback<ServerModels::AddPlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddSharedGroupMembers(ServerModels::AddSharedGroupMembersRequest& request, const ProcessApiCallback<ServerModels::AddSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddUserVirtualCurrency(ServerModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AuthenticateSessionTicket(ServerModels::AuthenticateSessionTicketRequest& request, const ProcessApiCallback<ServerModels::AuthenticateSessionTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AwardSteamAchievement(ServerModels::AwardSteamAchievementRequest& request, const ProcessApiCallback<ServerModels::AwardSteamAchievementResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void BanUsers(ServerModels::BanUsersRequest& request, const ProcessApiCallback<ServerModels::BanUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ConsumeItem(ServerModels::ConsumeItemRequest& request, const ProcessApiCallback<ServerModels::ConsumeItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateSharedGroup(ServerModels::CreateSharedGroupRequest& request, const ProcessApiCallback<ServerModels::CreateSharedGroupResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteCharacterFromUser(ServerModels::DeleteCharacterFromUserRequest& request, const ProcessApiCallback<ServerModels::DeleteCharacterFromUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeletePlayer(ServerModels::DeletePlayerRequest& request, const ProcessApiCallback<ServerModels::DeletePlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeletePushNotificationTemplate(ServerModels::DeletePushNotificationTemplateRequest& request, const ProcessApiCallback<ServerModels::DeletePushNotificationTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteSharedGroup(ServerModels::DeleteSharedGroupRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeregisterGame(ServerModels::DeregisterGameRequest& request, const ProcessApiCallback<ServerModels::DeregisterGameResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void EvaluateRandomResultTable(ServerModels::EvaluateRandomResultTableRequest& request, const ProcessApiCallback<ServerModels::EvaluateRandomResultTableResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ExecuteCloudScript(ServerModels::ExecuteCloudScriptServerRequest& request, const ProcessApiCallback<ServerModels::ExecuteCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAllSegments(ServerModels::GetAllSegmentsRequest& request, const ProcessApiCallback<ServerModels::GetAllSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAllUsersCharacters(ServerModels::ListUsersCharactersRequest& request, const ProcessApiCallback<ServerModels::ListUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCatalogItems(ServerModels::GetCatalogItemsRequest& request, const ProcessApiCallback<ServerModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterInternalData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterInventory(ServerModels::GetCharacterInventoryRequest& request, const ProcessApiCallback<ServerModels::GetCharacterInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterLeaderboard(ServerModels::GetCharacterLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetCharacterLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterReadOnlyData(ServerModels::GetCharacterDataRequest& request, const ProcessApiCallback<ServerModels::GetCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCharacterStatistics(ServerModels::GetCharacterStatisticsRequest& request, const ProcessApiCallback<ServerModels::GetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetContentDownloadUrl(ServerModels::GetContentDownloadUrlRequest& request, const ProcessApiCallback<ServerModels::GetContentDownloadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFriendLeaderboard(ServerModels::GetFriendLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFriendsList(ServerModels::GetFriendsListRequest& request, const ProcessApiCallback<ServerModels::GetFriendsListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboard(ServerModels::GetLeaderboardRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardAroundCharacter(ServerModels::GetLeaderboardAroundCharacterRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardAroundCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardAroundUser(ServerModels::GetLeaderboardAroundUserRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardAroundUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetLeaderboardForUserCharacters(ServerModels::GetLeaderboardForUsersCharactersRequest& request, const ProcessApiCallback<ServerModels::GetLeaderboardForUsersCharactersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerCombinedInfo(ServerModels::GetPlayerCombinedInfoRequest& request, const ProcessApiCallback<ServerModels::GetPlayerCombinedInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerProfile(ServerModels::GetPlayerProfileRequest& request, const ProcessApiCallback<ServerModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerSegments(ServerModels::GetPlayersSegmentsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayersInSegment(ServerModels::GetPlayersInSegmentRequest& request, const ProcessApiCallback<ServerModels::GetPlayersInSegmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatistics(ServerModels::GetPlayerStatisticsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatisticVersions(ServerModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerTags(ServerModels::GetPlayerTagsRequest& request, const ProcessApiCallback<ServerModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromFacebookIDs(ServerModels::GetPlayFabIDsFromFacebookIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromFacebookIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromFacebookInstantGamesIds(ServerModels::GetPlayFabIDsFromFacebookInstantGamesIdsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromFacebookInstantGamesIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromGenericIDs(ServerModels::GetPlayFabIDsFromGenericIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromGenericIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromNintendoSwitchDeviceIds(ServerModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromNintendoSwitchDeviceIdsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromPSNAccountIDs(ServerModels::GetPlayFabIDsFromPSNAccountIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromPSNAccountIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromSteamIDs(ServerModels::GetPlayFabIDsFromSteamIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromSteamIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayFabIDsFromXboxLiveIDs(ServerModels::GetPlayFabIDsFromXboxLiveIDsRequest& request, const ProcessApiCallback<ServerModels::GetPlayFabIDsFromXboxLiveIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPublisherData(ServerModels::GetPublisherDataRequest& request, const ProcessApiCallback<ServerModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetRandomResultTables(ServerModels::GetRandomResultTablesRequest& request, const ProcessApiCallback<ServerModels::GetRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetServerCustomIDsFromPlayFabIDs(ServerModels::GetServerCustomIDsFromPlayFabIDsRequest& request, const ProcessApiCallback<ServerModels::GetServerCustomIDsFromPlayFabIDsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetSharedGroupData(ServerModels::GetSharedGroupDataRequest& request, const ProcessApiCallback<ServerModels::GetSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetStoreItems(ServerModels::GetStoreItemsServerRequest& request, const ProcessApiCallback<ServerModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTime(ServerModels::GetTimeRequest& request, const ProcessApiCallback<ServerModels::GetTimeResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleData(ServerModels::GetTitleDataRequest& request, const ProcessApiCallback<ServerModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleInternalData(ServerModels::GetTitleDataRequest& request, const ProcessApiCallback<ServerModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleNews(ServerModels::GetTitleNewsRequest& request, const ProcessApiCallback<ServerModels::GetTitleNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserAccountInfo(ServerModels::GetUserAccountInfoRequest& request, const ProcessApiCallback<ServerModels::GetUserAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserBans(ServerModels::GetUserBansRequest& request, const ProcessApiCallback<ServerModels::GetUserBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserInternalData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserInventory(ServerModels::GetUserInventoryRequest& request, const ProcessApiCallback<ServerModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherInternalData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherReadOnlyData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserReadOnlyData(ServerModels::GetUserDataRequest& request, const ProcessApiCallback<ServerModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantCharacterToUser(ServerModels::GrantCharacterToUserRequest& request, const ProcessApiCallback<ServerModels::GrantCharacterToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantItemsToCharacter(ServerModels::GrantItemsToCharacterRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantItemsToUser(ServerModels::GrantItemsToUserRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantItemsToUsers(ServerModels::GrantItemsToUsersRequest& request, const ProcessApiCallback<ServerModels::GrantItemsToUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkPSNAccount(ServerModels::LinkPSNAccountRequest& request, const ProcessApiCallback<ServerModels::LinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkServerCustomId(ServerModels::LinkServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::LinkServerCustomIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LinkXboxAccount(ServerModels::LinkXboxAccountRequest& request, const ProcessApiCallback<ServerModels::LinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithServerCustomId(ServerModels::LoginWithServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithSteamId(ServerModels::LoginWithSteamIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithXbox(ServerModels::LoginWithXboxRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void LoginWithXboxId(ServerModels::LoginWithXboxIdRequest& request, const ProcessApiCallback<ServerModels::ServerLoginResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ModifyItemUses(ServerModels::ModifyItemUsesRequest& request, const ProcessApiCallback<ServerModels::ModifyItemUsesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void MoveItemToCharacterFromCharacter(ServerModels::MoveItemToCharacterFromCharacterRequest& request, const ProcessApiCallback<ServerModels::MoveItemToCharacterFromCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void MoveItemToCharacterFromUser(ServerModels::MoveItemToCharacterFromUserRequest& request, const ProcessApiCallback<ServerModels::MoveItemToCharacterFromUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void MoveItemToUserFromCharacter(ServerModels::MoveItemToUserFromCharacterRequest& request, const ProcessApiCallback<ServerModels::MoveItemToUserFromCharacterResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void NotifyMatchmakerPlayerLeft(ServerModels::NotifyMatchmakerPlayerLeftRequest& request, const ProcessApiCallback<ServerModels::NotifyMatchmakerPlayerLeftResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RedeemCoupon(ServerModels::RedeemCouponRequest& request, const ProcessApiCallback<ServerModels::RedeemCouponResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RedeemMatchmakerTicket(ServerModels::RedeemMatchmakerTicketRequest& request, const ProcessApiCallback<ServerModels::RedeemMatchmakerTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RefreshGameServerInstanceHeartbeat(ServerModels::RefreshGameServerInstanceHeartbeatRequest& request, const ProcessApiCallback<ServerModels::RefreshGameServerInstanceHeartbeatResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RegisterGame(ServerModels::RegisterGameRequest& request, const ProcessApiCallback<ServerModels::RegisterGameResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveFriend(ServerModels::RemoveFriendRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveGenericID(ServerModels::RemoveGenericIDRequest& request, const ProcessApiCallback<ServerModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemovePlayerTag(ServerModels::RemovePlayerTagRequest& request, const ProcessApiCallback<ServerModels::RemovePlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveSharedGroupMembers(ServerModels::RemoveSharedGroupMembersRequest& request, const ProcessApiCallback<ServerModels::RemoveSharedGroupMembersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ReportPlayer(ServerModels::ReportPlayerServerRequest& request, const ProcessApiCallback<ServerModels::ReportPlayerServerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeAllBansForUser(ServerModels::RevokeAllBansForUserRequest& request, const ProcessApiCallback<ServerModels::RevokeAllBansForUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeBans(ServerModels::RevokeBansRequest& request, const ProcessApiCallback<ServerModels::RevokeBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeInventoryItem(ServerModels::RevokeInventoryItemRequest& request, const ProcessApiCallback<ServerModels::RevokeInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeInventoryItems(ServerModels::RevokeInventoryItemsRequest& request, const ProcessApiCallback<ServerModels::RevokeInventoryItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SavePushNotificationTemplate(ServerModels::SavePushNotificationTemplateRequest& request, const ProcessApiCallback<ServerModels::SavePushNotificationTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendCustomAccountRecoveryEmail(ServerModels::SendCustomAccountRecoveryEmailRequest& request, const ProcessApiCallback<ServerModels::SendCustomAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendEmailFromTemplate(ServerModels::SendEmailFromTemplateRequest& request, const ProcessApiCallback<ServerModels::SendEmailFromTemplateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendPushNotification(ServerModels::SendPushNotificationRequest& request, const ProcessApiCallback<ServerModels::SendPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendPushNotificationFromTemplate(ServerModels::SendPushNotificationFromTemplateRequest& request, const ProcessApiCallback<ServerModels::SendPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetFriendTags(ServerModels::SetFriendTagsRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetGameServerInstanceData(ServerModels::SetGameServerInstanceDataRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetGameServerInstanceState(ServerModels::SetGameServerInstanceStateRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceStateResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetGameServerInstanceTags(ServerModels::SetGameServerInstanceTagsRequest& request, const ProcessApiCallback<ServerModels::SetGameServerInstanceTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPlayerSecret(ServerModels::SetPlayerSecretRequest& request, const ProcessApiCallback<ServerModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPublisherData(ServerModels::SetPublisherDataRequest& request, const ProcessApiCallback<ServerModels::SetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetTitleData(ServerModels::SetTitleDataRequest& request, const ProcessApiCallback<ServerModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetTitleInternalData(ServerModels::SetTitleDataRequest& request, const ProcessApiCallback<ServerModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SubtractCharacterVirtualCurrency(ServerModels::SubtractCharacterVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyCharacterVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SubtractUserVirtualCurrency(ServerModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<ServerModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkPSNAccount(ServerModels::UnlinkPSNAccountRequest& request, const ProcessApiCallback<ServerModels::UnlinkPSNAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkServerCustomId(ServerModels::UnlinkServerCustomIdRequest& request, const ProcessApiCallback<ServerModels::UnlinkServerCustomIdResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlinkXboxAccount(ServerModels::UnlinkXboxAccountRequest& request, const ProcessApiCallback<ServerModels::UnlinkXboxAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlockContainerInstance(ServerModels::UnlockContainerInstanceRequest& request, const ProcessApiCallback<ServerModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnlockContainerItem(ServerModels::UnlockContainerItemRequest& request, const ProcessApiCallback<ServerModels::UnlockContainerItemResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateAvatarUrl(ServerModels::UpdateAvatarUrlRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBans(ServerModels::UpdateBansRequest& request, const ProcessApiCallback<ServerModels::UpdateBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterInternalData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterReadOnlyData(ServerModels::UpdateCharacterDataRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCharacterStatistics(ServerModels::UpdateCharacterStatisticsRequest& request, const ProcessApiCallback<ServerModels::UpdateCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdatePlayerStatistics(ServerModels::UpdatePlayerStatisticsRequest& request, const ProcessApiCallback<ServerModels::UpdatePlayerStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateSharedGroupData(ServerModels::UpdateSharedGroupDataRequest& request, const ProcessApiCallback<ServerModels::UpdateSharedGroupDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserInternalData(ServerModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserInventoryItemCustomData(ServerModels::UpdateUserInventoryItemDataRequest& request, const ProcessApiCallback<ServerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherInternalData(ServerModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherReadOnlyData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserReadOnlyData(ServerModels::UpdateUserDataRequest& request, const ProcessApiCallback<ServerModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WriteCharacterEvent(ServerModels::WriteServerCharacterEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WritePlayerEvent(ServerModels::WriteServerPlayerEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void WriteTitleEvent(ServerModels::WriteTitleEventRequest& request, const ProcessApiCallback<ServerModels::WriteEventResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnAddCharacterVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddPlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAuthenticateSessionTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAwardSteamAchievementResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnBanUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnConsumeItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteCharacterFromUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeletePlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeletePushNotificationTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteSharedGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeregisterGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnEvaluateRandomResultTableResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnExecuteCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAllSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAllUsersCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetContentDownloadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFriendLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFriendsListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardAroundCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardAroundUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetLeaderboardForUserCharactersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerCombinedInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayersInSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromFacebookIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromFacebookInstantGamesIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromGenericIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromNintendoSwitchDeviceIdsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromPSNAccountIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromSteamIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayFabIDsFromXboxLiveIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetServerCustomIDsFromPlayFabIDsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTimeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantCharacterToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantItemsToCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantItemsToUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantItemsToUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithSteamIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithXboxResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnLoginWithXboxIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnModifyItemUsesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnMoveItemToCharacterFromCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnMoveItemToCharacterFromUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnMoveItemToUserFromCharacterResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnNotifyMatchmakerPlayerLeftResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRedeemCouponResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRedeemMatchmakerTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRefreshGameServerInstanceHeartbeatResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRegisterGameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveFriendResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveGenericIDResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemovePlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveSharedGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnReportPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeAllBansForUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeInventoryItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeInventoryItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSavePushNotificationTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendCustomAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendEmailFromTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendPushNotificationFromTemplateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetFriendTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetGameServerInstanceDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetGameServerInstanceStateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetGameServerInstanceTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSubtractCharacterVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkPSNAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkServerCustomIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlinkXboxAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlockContainerInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnlockContainerItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateAvatarUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdatePlayerStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateSharedGroupDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserInventoryItemCustomDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWriteCharacterEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWritePlayerEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnWriteTitleEventResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
