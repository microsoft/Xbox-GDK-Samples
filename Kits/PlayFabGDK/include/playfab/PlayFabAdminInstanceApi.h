#pragma once

#if defined(ENABLE_PLAYFABADMIN_API)

#include <playfab/PlayFabAdminDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Admin APIs
    /// </summary>
    class PlayFabAdminInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabAdminInstanceAPI();
        PlayFabAdminInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings);
        PlayFabAdminInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabAdminInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabAdminInstanceAPI() = default;
        PlayFabAdminInstanceAPI(const PlayFabAdminInstanceAPI& source) = delete; // disable copy
        PlayFabAdminInstanceAPI(PlayFabAdminInstanceAPI&&) = delete; // disable move
        PlayFabAdminInstanceAPI& operator=(const PlayFabAdminInstanceAPI& source) = delete; // disable assignment
        PlayFabAdminInstanceAPI& operator=(PlayFabAdminInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Admin.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void AbortTaskInstance(AdminModels::AbortTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddLocalizedNews(AdminModels::AddLocalizedNewsRequest& request, const ProcessApiCallback<AdminModels::AddLocalizedNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddNews(AdminModels::AddNewsRequest& request, const ProcessApiCallback<AdminModels::AddNewsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddPlayerTag(AdminModels::AddPlayerTagRequest& request, const ProcessApiCallback<AdminModels::AddPlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddServerBuild(AdminModels::AddServerBuildRequest& request, const ProcessApiCallback<AdminModels::AddServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddUserVirtualCurrency(AdminModels::AddUserVirtualCurrencyRequest& request, const ProcessApiCallback<AdminModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void AddVirtualCurrencyTypes(AdminModels::AddVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void BanUsers(AdminModels::BanUsersRequest& request, const ProcessApiCallback<AdminModels::BanUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CheckLimitedEditionItemAvailability(AdminModels::CheckLimitedEditionItemAvailabilityRequest& request, const ProcessApiCallback<AdminModels::CheckLimitedEditionItemAvailabilityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateActionsOnPlayersInSegmentTask(AdminModels::CreateActionsOnPlayerSegmentTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateCloudScriptTask(AdminModels::CreateCloudScriptTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateInsightsScheduledScalingTask(AdminModels::CreateInsightsScheduledScalingTaskRequest& request, const ProcessApiCallback<AdminModels::CreateTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateOpenIdConnection(AdminModels::CreateOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreatePlayerSharedSecret(AdminModels::CreatePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::CreatePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreatePlayerStatisticDefinition(AdminModels::CreatePlayerStatisticDefinitionRequest& request, const ProcessApiCallback<AdminModels::CreatePlayerStatisticDefinitionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateSegment(AdminModels::CreateSegmentRequest& request, const ProcessApiCallback<AdminModels::CreateSegmentResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteContent(AdminModels::DeleteContentRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteMasterPlayerAccount(AdminModels::DeleteMasterPlayerAccountRequest& request, const ProcessApiCallback<AdminModels::DeleteMasterPlayerAccountResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteMembershipSubscription(AdminModels::DeleteMembershipSubscriptionRequest& request, const ProcessApiCallback<AdminModels::DeleteMembershipSubscriptionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteOpenIdConnection(AdminModels::DeleteOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeletePlayer(AdminModels::DeletePlayerRequest& request, const ProcessApiCallback<AdminModels::DeletePlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeletePlayerSharedSecret(AdminModels::DeletePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::DeletePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteSegment(AdminModels::DeleteSegmentRequest& request, const ProcessApiCallback<AdminModels::DeleteSegmentsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteStore(AdminModels::DeleteStoreRequest& request, const ProcessApiCallback<AdminModels::DeleteStoreResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteTask(AdminModels::DeleteTaskRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteTitle(AdminModels::DeleteTitleRequest& request, const ProcessApiCallback<AdminModels::DeleteTitleResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteTitleDataOverride(AdminModels::DeleteTitleDataOverrideRequest& request, const ProcessApiCallback<AdminModels::DeleteTitleDataOverrideResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ExportMasterPlayerData(AdminModels::ExportMasterPlayerDataRequest& request, const ProcessApiCallback<AdminModels::ExportMasterPlayerDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetActionsOnPlayersInSegmentTaskInstance(AdminModels::GetTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::GetActionsOnPlayersInSegmentTaskInstanceResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAllSegments(AdminModels::GetAllSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetAllSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCatalogItems(AdminModels::GetCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::GetCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCloudScriptRevision(AdminModels::GetCloudScriptRevisionRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptRevisionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCloudScriptTaskInstance(AdminModels::GetTaskInstanceRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptTaskInstanceResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetCloudScriptVersions(AdminModels::GetCloudScriptVersionsRequest& request, const ProcessApiCallback<AdminModels::GetCloudScriptVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetContentList(AdminModels::GetContentListRequest& request, const ProcessApiCallback<AdminModels::GetContentListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetContentUploadUrl(AdminModels::GetContentUploadUrlRequest& request, const ProcessApiCallback<AdminModels::GetContentUploadUrlResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetDataReport(AdminModels::GetDataReportRequest& request, const ProcessApiCallback<AdminModels::GetDataReportResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMatchmakerGameInfo(AdminModels::GetMatchmakerGameInfoRequest& request, const ProcessApiCallback<AdminModels::GetMatchmakerGameInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMatchmakerGameModes(AdminModels::GetMatchmakerGameModesRequest& request, const ProcessApiCallback<AdminModels::GetMatchmakerGameModesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayedTitleList(AdminModels::GetPlayedTitleListRequest& request, const ProcessApiCallback<AdminModels::GetPlayedTitleListResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerIdFromAuthToken(AdminModels::GetPlayerIdFromAuthTokenRequest& request, const ProcessApiCallback<AdminModels::GetPlayerIdFromAuthTokenResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerProfile(AdminModels::GetPlayerProfileRequest& request, const ProcessApiCallback<AdminModels::GetPlayerProfileResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerSegments(AdminModels::GetPlayersSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerSegmentsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerSharedSecrets(AdminModels::GetPlayerSharedSecretsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerSharedSecretsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayersInSegment(AdminModels::GetPlayersInSegmentRequest& request, const ProcessApiCallback<AdminModels::GetPlayersInSegmentResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatisticDefinitions(AdminModels::GetPlayerStatisticDefinitionsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerStatisticDefinitionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerStatisticVersions(AdminModels::GetPlayerStatisticVersionsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerStatisticVersionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPlayerTags(AdminModels::GetPlayerTagsRequest& request, const ProcessApiCallback<AdminModels::GetPlayerTagsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPolicy(AdminModels::GetPolicyRequest& request, const ProcessApiCallback<AdminModels::GetPolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetPublisherData(AdminModels::GetPublisherDataRequest& request, const ProcessApiCallback<AdminModels::GetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetRandomResultTables(AdminModels::GetRandomResultTablesRequest& request, const ProcessApiCallback<AdminModels::GetRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetSegments(AdminModels::GetSegmentsRequest& request, const ProcessApiCallback<AdminModels::GetSegmentsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetServerBuildInfo(AdminModels::GetServerBuildInfoRequest& request, const ProcessApiCallback<AdminModels::GetServerBuildInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetServerBuildUploadUrl(AdminModels::GetServerBuildUploadURLRequest& request, const ProcessApiCallback<AdminModels::GetServerBuildUploadURLResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetStoreItems(AdminModels::GetStoreItemsRequest& request, const ProcessApiCallback<AdminModels::GetStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTaskInstances(AdminModels::GetTaskInstancesRequest& request, const ProcessApiCallback<AdminModels::GetTaskInstancesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTasks(AdminModels::GetTasksRequest& request, const ProcessApiCallback<AdminModels::GetTasksResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleData(AdminModels::GetTitleDataRequest& request, const ProcessApiCallback<AdminModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleInternalData(AdminModels::GetTitleDataRequest& request, const ProcessApiCallback<AdminModels::GetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserAccountInfo(AdminModels::LookupUserAccountInfoRequest& request, const ProcessApiCallback<AdminModels::LookupUserAccountInfoResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserBans(AdminModels::GetUserBansRequest& request, const ProcessApiCallback<AdminModels::GetUserBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserInternalData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserInventory(AdminModels::GetUserInventoryRequest& request, const ProcessApiCallback<AdminModels::GetUserInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherInternalData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserPublisherReadOnlyData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetUserReadOnlyData(AdminModels::GetUserDataRequest& request, const ProcessApiCallback<AdminModels::GetUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GrantItemsToUsers(AdminModels::GrantItemsToUsersRequest& request, const ProcessApiCallback<AdminModels::GrantItemsToUsersResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void IncrementLimitedEditionItemAvailability(AdminModels::IncrementLimitedEditionItemAvailabilityRequest& request, const ProcessApiCallback<AdminModels::IncrementLimitedEditionItemAvailabilityResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void IncrementPlayerStatisticVersion(AdminModels::IncrementPlayerStatisticVersionRequest& request, const ProcessApiCallback<AdminModels::IncrementPlayerStatisticVersionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListOpenIdConnection(AdminModels::ListOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::ListOpenIdConnectionResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListServerBuilds(AdminModels::ListBuildsRequest& request, const ProcessApiCallback<AdminModels::ListBuildsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListVirtualCurrencyTypes(AdminModels::ListVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::ListVirtualCurrencyTypesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ModifyMatchmakerGameModes(AdminModels::ModifyMatchmakerGameModesRequest& request, const ProcessApiCallback<AdminModels::ModifyMatchmakerGameModesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ModifyServerBuild(AdminModels::ModifyServerBuildRequest& request, const ProcessApiCallback<AdminModels::ModifyServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RefundPurchase(AdminModels::RefundPurchaseRequest& request, const ProcessApiCallback<AdminModels::RefundPurchaseResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemovePlayerTag(AdminModels::RemovePlayerTagRequest& request, const ProcessApiCallback<AdminModels::RemovePlayerTagResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveServerBuild(AdminModels::RemoveServerBuildRequest& request, const ProcessApiCallback<AdminModels::RemoveServerBuildResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RemoveVirtualCurrencyTypes(AdminModels::RemoveVirtualCurrencyTypesRequest& request, const ProcessApiCallback<AdminModels::BlankResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ResetCharacterStatistics(AdminModels::ResetCharacterStatisticsRequest& request, const ProcessApiCallback<AdminModels::ResetCharacterStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ResetPassword(AdminModels::ResetPasswordRequest& request, const ProcessApiCallback<AdminModels::ResetPasswordResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ResetUserStatistics(AdminModels::ResetUserStatisticsRequest& request, const ProcessApiCallback<AdminModels::ResetUserStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ResolvePurchaseDispute(AdminModels::ResolvePurchaseDisputeRequest& request, const ProcessApiCallback<AdminModels::ResolvePurchaseDisputeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeAllBansForUser(AdminModels::RevokeAllBansForUserRequest& request, const ProcessApiCallback<AdminModels::RevokeAllBansForUserResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeBans(AdminModels::RevokeBansRequest& request, const ProcessApiCallback<AdminModels::RevokeBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeInventoryItem(AdminModels::RevokeInventoryItemRequest& request, const ProcessApiCallback<AdminModels::RevokeInventoryResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RevokeInventoryItems(AdminModels::RevokeInventoryItemsRequest& request, const ProcessApiCallback<AdminModels::RevokeInventoryItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RunTask(AdminModels::RunTaskRequest& request, const ProcessApiCallback<AdminModels::RunTaskResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SendAccountRecoveryEmail(AdminModels::SendAccountRecoveryEmailRequest& request, const ProcessApiCallback<AdminModels::SendAccountRecoveryEmailResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetCatalogItems(AdminModels::UpdateCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetMembershipOverride(AdminModels::SetMembershipOverrideRequest& request, const ProcessApiCallback<AdminModels::SetMembershipOverrideResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPlayerSecret(AdminModels::SetPlayerSecretRequest& request, const ProcessApiCallback<AdminModels::SetPlayerSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPublishedRevision(AdminModels::SetPublishedRevisionRequest& request, const ProcessApiCallback<AdminModels::SetPublishedRevisionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetPublisherData(AdminModels::SetPublisherDataRequest& request, const ProcessApiCallback<AdminModels::SetPublisherDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetStoreItems(AdminModels::UpdateStoreItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetTitleData(AdminModels::SetTitleDataRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetTitleDataAndOverrides(AdminModels::SetTitleDataAndOverridesRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataAndOverridesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetTitleInternalData(AdminModels::SetTitleDataRequest& request, const ProcessApiCallback<AdminModels::SetTitleDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SetupPushNotification(AdminModels::SetupPushNotificationRequest& request, const ProcessApiCallback<AdminModels::SetupPushNotificationResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void SubtractUserVirtualCurrency(AdminModels::SubtractUserVirtualCurrencyRequest& request, const ProcessApiCallback<AdminModels::ModifyUserVirtualCurrencyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBans(AdminModels::UpdateBansRequest& request, const ProcessApiCallback<AdminModels::UpdateBansResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCatalogItems(AdminModels::UpdateCatalogItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateCatalogItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateCloudScript(AdminModels::UpdateCloudScriptRequest& request, const ProcessApiCallback<AdminModels::UpdateCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateOpenIdConnection(AdminModels::UpdateOpenIdConnectionRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdatePlayerSharedSecret(AdminModels::UpdatePlayerSharedSecretRequest& request, const ProcessApiCallback<AdminModels::UpdatePlayerSharedSecretResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdatePlayerStatisticDefinition(AdminModels::UpdatePlayerStatisticDefinitionRequest& request, const ProcessApiCallback<AdminModels::UpdatePlayerStatisticDefinitionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdatePolicy(AdminModels::UpdatePolicyRequest& request, const ProcessApiCallback<AdminModels::UpdatePolicyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateRandomResultTables(AdminModels::UpdateRandomResultTablesRequest& request, const ProcessApiCallback<AdminModels::UpdateRandomResultTablesResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateSegment(AdminModels::UpdateSegmentRequest& request, const ProcessApiCallback<AdminModels::UpdateSegmentResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateStoreItems(AdminModels::UpdateStoreItemsRequest& request, const ProcessApiCallback<AdminModels::UpdateStoreItemsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateTask(AdminModels::UpdateTaskRequest& request, const ProcessApiCallback<AdminModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserInternalData(AdminModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherInternalData(AdminModels::UpdateUserInternalDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserPublisherReadOnlyData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserReadOnlyData(AdminModels::UpdateUserDataRequest& request, const ProcessApiCallback<AdminModels::UpdateUserDataResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateUserTitleDisplayName(AdminModels::UpdateUserTitleDisplayNameRequest& request, const ProcessApiCallback<AdminModels::UpdateUserTitleDisplayNameResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnAbortTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddLocalizedNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddNewsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddPlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnAddVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnBanUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCheckLimitedEditionItemAvailabilityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateActionsOnPlayersInSegmentTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateCloudScriptTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateInsightsScheduledScalingTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreatePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreatePlayerStatisticDefinitionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteContentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteMasterPlayerAccountResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteMembershipSubscriptionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeletePlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeletePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteStoreResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteTitleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteTitleDataOverrideResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnExportMasterPlayerDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetActionsOnPlayersInSegmentTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAllSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCloudScriptRevisionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCloudScriptTaskInstanceResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetCloudScriptVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetContentListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetContentUploadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetDataReportResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMatchmakerGameInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMatchmakerGameModesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayedTitleListResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerIdFromAuthTokenResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerProfileResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerSharedSecretsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayersInSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticDefinitionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerStatisticVersionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPlayerTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetSegmentsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetServerBuildInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetServerBuildUploadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTaskInstancesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTasksResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserAccountInfoResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserInventoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGrantItemsToUsersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnIncrementLimitedEditionItemAvailabilityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnIncrementPlayerStatisticVersionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListServerBuildsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnModifyMatchmakerGameModesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnModifyServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRefundPurchaseResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemovePlayerTagResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveServerBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRemoveVirtualCurrencyTypesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnResetCharacterStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnResetPasswordResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnResetUserStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnResolvePurchaseDisputeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeAllBansForUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeInventoryItemResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRevokeInventoryItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRunTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSendAccountRecoveryEmailResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetMembershipOverrideResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPlayerSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPublishedRevisionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetTitleDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetTitleDataAndOverridesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetTitleInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSetupPushNotificationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnSubtractUserVirtualCurrencyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBansResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCatalogItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateOpenIdConnectionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdatePlayerSharedSecretResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdatePlayerStatisticDefinitionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdatePolicyResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateRandomResultTablesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateSegmentResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateStoreItemsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherInternalDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserPublisherReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserReadOnlyDataResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateUserTitleDisplayNameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
