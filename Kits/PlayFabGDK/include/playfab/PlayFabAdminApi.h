#pragma once

#if defined(ENABLE_PLAYFABADMIN_API)

#include <playfab/PlayFabAdminDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Admin APIs
    /// </summary>
    class PlayFabAdminAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Admin.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void AbortTaskInstance(AdminModels::AbortTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddLocalizedNews(AdminModels::AddLocalizedNewsRequest& request, const ProcessApiCallback<AdminModels::AddLocalizedNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddNews(AdminModels::AddNewsRequest& request, const ProcessApiCallback<AdminModels::AddNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddPlayerTag(AdminModels::AddPlayerTagRequest& request, const ProcessApiCallback<AdminModels::AddPlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddServerBuild(AdminModels::AddServerBuildRequest& request, const ProcessApiCallback<AdminModels::AddServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddUserVirtualCurrency(AdminModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<AdminModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddVirtualCurrencyTypes(AdminModels::AddVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void BanUsers(AdminModels::BanUsersRequest& request, const ProcessApiCallback<AdminModels::BanUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CheckLimitedEditionItemAvailability(AdminModels::CheckLimitedEditionItemAvailabilityRequest& request, const ProcessApiCallback<AdminModels::CheckLimitedEditionItemAvailabilityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateActionsOnPlayersInSegmentTask(AdminModels::CreateActionsOnPlayerSegmentTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateCloudScriptTask(AdminModels::CreateCloudScriptTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateInsightsScheduledScalingTask(AdminModels::CreateInsightsScheduledScalingTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateOpenIdConnection(AdminModels::CreateOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreatePlayerSharedSecret(AdminModels::CreatePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::CreatePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreatePlayerStatisticDefinition(AdminModels::CreatePlayerStatisticDefinitionRequest& request, const ProcessApiCallback<AdminModels::CreatePlayerStatisticDefinitionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateSegment(AdminModels::CreateSegmentRequest& request, const ProcessApiCallback<AdminModels::CreateSegmentResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteContent(AdminModels::DeleteContentRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteMasterPlayerAccount(AdminModels::DeleteMasterPlayerAccountRequest& request, const ProcessApiCallback<AdminModels::DeleteMasterPlayerAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteMembershipSubscription(AdminModels::DeleteMembershipSubscriptionRequest& request, const ProcessApiCallback<AdminModels::DeleteMembershipSubscriptionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteOpenIdConnection(AdminModels::DeleteOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeletePlayer(AdminModels::DeletePlayerRequest& request, const ProcessApiCallback<AdminModels::DeletePlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeletePlayerSharedSecret(AdminModels::DeletePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::DeletePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteSegment(AdminModels::DeleteSegmentRequest& request, const ProcessApiCallback<AdminModels::DeleteSegmentsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteStore(AdminModels::DeleteStoreRequest& request, const ProcessApiCallback<AdminModels::DeleteStoreResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteTask(AdminModels::DeleteTaskRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteTitle(AdminModels::DeleteTitleRequest& request, const ProcessApiCallback<AdminModels::DeleteTitleResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteTitleDataOverride(AdminModels::DeleteTitleDataOverrideRequest& request, const ProcessApiCallback<AdminModels::DeleteTitleDataOverrideResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ExportMasterPlayerData(AdminModels::ExportMasterPlayerDataRequest& request, const ProcessApiCallback<AdminModels::ExportMasterPlayerDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetActionsOnPlayersInSegmentTaskInstance(AdminModels::GetTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::GetActionsOnPlayersInSegmentTaskInstanceResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetAllSegments(AdminModels::GetAllSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetAllSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCatalogItems(AdminModels::GetCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCloudScriptRevision(AdminModels::GetCloudScriptRevisionRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptRevisionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCloudScriptTaskInstance(AdminModels::GetTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptTaskInstanceResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetCloudScriptVersions(AdminModels::GetCloudScriptVersionsRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetContentList(AdminModels::GetContentListRequest& request, const ProcessApiCallback<AdminModels::GetContentListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetContentUploadUrl(AdminModels::GetContentUploadUrlRequest& request, const ProcessApiCallback<AdminModels::GetContentUploadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetDataReport(AdminModels::GetDataReportRequest& request, const ProcessApiCallback<AdminModels::GetDataReportResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetMatchmakerGameInfo(AdminModels::GetMatchmakerGameInfoRequest& request, const ProcessApiCallback<AdminModels::GetMatchmakerGameInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetMatchmakerGameModes(AdminModels::GetMatchmakerGameModesRequest& request, const ProcessApiCallback<AdminModels::GetMatchmakerGameModesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayedTitleList(AdminModels::GetPlayedTitleListRequest& request, const ProcessApiCallback<AdminModels::GetPlayedTitleListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerIdFromAuthToken(AdminModels::GetPlayerIdFromAuthTokenRequest& request, const ProcessApiCallback<AdminModels::GetPlayerIdFromAuthTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerProfile(AdminModels::GetPlayerProfileRequest& request, const ProcessApiCallback<AdminModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerSegments(AdminModels::GetPlayersSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerSharedSecrets(AdminModels::GetPlayerSharedSecretsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerSharedSecretsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayersInSegment(AdminModels::GetPlayersInSegmentRequest& request, const ProcessApiCallback<AdminModels::GetPlayersInSegmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatisticDefinitions(AdminModels::GetPlayerStatisticDefinitionsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerStatisticDefinitionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerStatisticVersions(AdminModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPlayerTags(AdminModels::GetPlayerTagsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPolicy(AdminModels::GetPolicyRequest& request, const ProcessApiCallback<AdminModels::GetPolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetPublisherData(AdminModels::GetPublisherDataRequest& request, const ProcessApiCallback<AdminModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetRandomResultTables(AdminModels::GetRandomResultTablesRequest& request, const ProcessApiCallback<AdminModels::GetRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetSegments(AdminModels::GetSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetSegmentsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetServerBuildInfo(AdminModels::GetServerBuildInfoRequest& request, const ProcessApiCallback<AdminModels::GetServerBuildInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetServerBuildUploadUrl(AdminModels::GetServerBuildUploadURLRequest& request, const ProcessApiCallback<AdminModels::GetServerBuildUploadURLResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetStoreItems(AdminModels::GetStoreItemsRequest& request, const ProcessApiCallback<AdminModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTaskInstances(AdminModels::GetTaskInstancesRequest& request, const ProcessApiCallback<AdminModels::GetTaskInstancesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTasks(AdminModels::GetTasksRequest& request, const ProcessApiCallback<AdminModels::GetTasksResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleData(AdminModels::GetTitleDataRequest& request, const ProcessApiCallback<AdminModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetTitleInternalData(AdminModels::GetTitleDataRequest& request, const ProcessApiCallback<AdminModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserAccountInfo(AdminModels::LookupUserAccountInfoRequest& request, const ProcessApiCallback<AdminModels::LookupUserAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserBans(AdminModels::GetUserBansRequest& request, const ProcessApiCallback<AdminModels::GetUserBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserInternalData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserInventory(AdminModels::GetUserInventoryRequest& request, const ProcessApiCallback<AdminModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherInternalData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserPublisherReadOnlyData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetUserReadOnlyData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GrantItemsToUsers(AdminModels::GrantItemsToUsersRequest& request, const ProcessApiCallback<AdminModels::GrantItemsToUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void IncrementLimitedEditionItemAvailability(AdminModels::IncrementLimitedEditionItemAvailabilityRequest& request, const ProcessApiCallback<AdminModels::IncrementLimitedEditionItemAvailabilityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void IncrementPlayerStatisticVersion(AdminModels::IncrementPlayerStatisticVersionRequest& request, const ProcessApiCallback<AdminModels::IncrementPlayerStatisticVersionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListOpenIdConnection(AdminModels::ListOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::ListOpenIdConnectionResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListServerBuilds(AdminModels::ListBuildsRequest& request, const ProcessApiCallback<AdminModels::ListBuildsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListVirtualCurrencyTypes(AdminModels::ListVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::ListVirtualCurrencyTypesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ModifyMatchmakerGameModes(AdminModels::ModifyMatchmakerGameModesRequest& request, const ProcessApiCallback<AdminModels::ModifyMatchmakerGameModesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ModifyServerBuild(AdminModels::ModifyServerBuildRequest& request, const ProcessApiCallback<AdminModels::ModifyServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RefundPurchase(AdminModels::RefundPurchaseRequest& request, const ProcessApiCallback<AdminModels::RefundPurchaseResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemovePlayerTag(AdminModels::RemovePlayerTagRequest& request, const ProcessApiCallback<AdminModels::RemovePlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveServerBuild(AdminModels::RemoveServerBuildRequest& request, const ProcessApiCallback<AdminModels::RemoveServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveVirtualCurrencyTypes(AdminModels::RemoveVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ResetCharacterStatistics(AdminModels::ResetCharacterStatisticsRequest& request, const ProcessApiCallback<AdminModels::ResetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ResetPassword(AdminModels::ResetPasswordRequest& request, const ProcessApiCallback<AdminModels::ResetPasswordResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ResetUserStatistics(AdminModels::ResetUserStatisticsRequest& request, const ProcessApiCallback<AdminModels::ResetUserStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ResolvePurchaseDispute(AdminModels::ResolvePurchaseDisputeRequest& request, const ProcessApiCallback<AdminModels::ResolvePurchaseDisputeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeAllBansForUser(AdminModels::RevokeAllBansForUserRequest& request, const ProcessApiCallback<AdminModels::RevokeAllBansForUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeBans(AdminModels::RevokeBansRequest& request, const ProcessApiCallback<AdminModels::RevokeBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeInventoryItem(AdminModels::RevokeInventoryItemRequest& request, const ProcessApiCallback<AdminModels::RevokeInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RevokeInventoryItems(AdminModels::RevokeInventoryItemsRequest& request, const ProcessApiCallback<AdminModels::RevokeInventoryItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RunTask(AdminModels::RunTaskRequest& request, const ProcessApiCallback<AdminModels::RunTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SendAccountRecoveryEmail(AdminModels::SendAccountRecoveryEmailRequest& request, const ProcessApiCallback<AdminModels::SendAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetCatalogItems(AdminModels::UpdateCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetMembershipOverride(AdminModels::SetMembershipOverrideRequest& request, const ProcessApiCallback<AdminModels::SetMembershipOverrideResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPlayerSecret(AdminModels::SetPlayerSecretRequest& request, const ProcessApiCallback<AdminModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPublishedRevision(AdminModels::SetPublishedRevisionRequest& request, const ProcessApiCallback<AdminModels::SetPublishedRevisionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetPublisherData(AdminModels::SetPublisherDataRequest& request, const ProcessApiCallback<AdminModels::SetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetStoreItems(AdminModels::UpdateStoreItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetTitleData(AdminModels::SetTitleDataRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetTitleDataAndOverrides(AdminModels::SetTitleDataAndOverridesRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataAndOverridesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetTitleInternalData(AdminModels::SetTitleDataRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SetupPushNotification(AdminModels::SetupPushNotificationRequest& request, const ProcessApiCallback<AdminModels::SetupPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void SubtractUserVirtualCurrency(AdminModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<AdminModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateBans(AdminModels::UpdateBansRequest& request, const ProcessApiCallback<AdminModels::UpdateBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCatalogItems(AdminModels::UpdateCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateCloudScript(AdminModels::UpdateCloudScriptRequest& request, const ProcessApiCallback<AdminModels::UpdateCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateOpenIdConnection(AdminModels::UpdateOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdatePlayerSharedSecret(AdminModels::UpdatePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::UpdatePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdatePlayerStatisticDefinition(AdminModels::UpdatePlayerStatisticDefinitionRequest& request, const ProcessApiCallback<AdminModels::UpdatePlayerStatisticDefinitionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdatePolicy(AdminModels::UpdatePolicyRequest& request, const ProcessApiCallback<AdminModels::UpdatePolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateRandomResultTables(AdminModels::UpdateRandomResultTablesRequest& request, const ProcessApiCallback<AdminModels::UpdateRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateSegment(AdminModels::UpdateSegmentRequest& request, const ProcessApiCallback<AdminModels::UpdateSegmentResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateStoreItems(AdminModels::UpdateStoreItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateTask(AdminModels::UpdateTaskRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserInternalData(AdminModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherInternalData(AdminModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserPublisherReadOnlyData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserReadOnlyData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateUserTitleDisplayName(AdminModels::UpdateUserTitleDisplayNameRequest& request, const ProcessApiCallback<AdminModels::UpdateUserTitleDisplayNameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabAdminAPI(); // Private constructor, static class should never have an instance
        PlayFabAdminAPI(const PlayFabAdminAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAbortTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddLocalizedNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddPlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnBanUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCheckLimitedEditionItemAvailabilityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateActionsOnPlayersInSegmentTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateCloudScriptTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateInsightsScheduledScalingTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreatePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreatePlayerStatisticDefinitionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteContentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteMasterPlayerAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteMembershipSubscriptionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeletePlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeletePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteStoreResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteTitleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteTitleDataOverrideResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnExportMasterPlayerDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetActionsOnPlayersInSegmentTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetAllSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCloudScriptRevisionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCloudScriptTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetCloudScriptVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetContentListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetContentUploadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetDataReportResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetMatchmakerGameInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetMatchmakerGameModesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayedTitleListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerIdFromAuthTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerSharedSecretsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayersInSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticDefinitionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetServerBuildInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetServerBuildUploadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTaskInstancesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTasksResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGrantItemsToUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnIncrementLimitedEditionItemAvailabilityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnIncrementPlayerStatisticVersionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListServerBuildsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnModifyMatchmakerGameModesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnModifyServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRefundPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemovePlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnResetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnResetPasswordResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnResetUserStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnResolvePurchaseDisputeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeAllBansForUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeInventoryItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRevokeInventoryItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRunTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSendAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetMembershipOverrideResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPublishedRevisionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetTitleDataAndOverridesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSetupPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdatePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdatePlayerStatisticDefinitionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdatePolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateUserTitleDisplayNameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if defined(ENABLE_PLAYFABADMIN_API)
