#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabMultiplayerDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Multiplayer APIs
    /// </summary>
    class PlayFabMultiplayerInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabMultiplayerInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabMultiplayerInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabMultiplayerInstanceAPI() = default;
        PlayFabMultiplayerInstanceAPI(const PlayFabMultiplayerInstanceAPI& source) = delete; // disable copy
        PlayFabMultiplayerInstanceAPI(PlayFabMultiplayerInstanceAPI&&) = delete; // disable move
        PlayFabMultiplayerInstanceAPI& operator=(const PlayFabMultiplayerInstanceAPI& source) = delete; // disable assignment
        PlayFabMultiplayerInstanceAPI& operator=(PlayFabMultiplayerInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Multiplayer.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void CancelAllMatchmakingTicketsForPlayer(MultiplayerModels::CancelAllMatchmakingTicketsForPlayerRequest& request, const ProcessApiCallback<MultiplayerModels::CancelAllMatchmakingTicketsForPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CancelAllServerBackfillTicketsForPlayer(MultiplayerModels::CancelAllServerBackfillTicketsForPlayerRequest& request, const ProcessApiCallback<MultiplayerModels::CancelAllServerBackfillTicketsForPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CancelMatchmakingTicket(MultiplayerModels::CancelMatchmakingTicketRequest& request, const ProcessApiCallback<MultiplayerModels::CancelMatchmakingTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CancelServerBackfillTicket(MultiplayerModels::CancelServerBackfillTicketRequest& request, const ProcessApiCallback<MultiplayerModels::CancelServerBackfillTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateBuildAlias(MultiplayerModels::CreateBuildAliasRequest& request, const ProcessApiCallback<MultiplayerModels::BuildAliasDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateBuildWithCustomContainer(MultiplayerModels::CreateBuildWithCustomContainerRequest& request, const ProcessApiCallback<MultiplayerModels::CreateBuildWithCustomContainerResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateBuildWithManagedContainer(MultiplayerModels::CreateBuildWithManagedContainerRequest& request, const ProcessApiCallback<MultiplayerModels::CreateBuildWithManagedContainerResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateBuildWithProcessBasedServer(MultiplayerModels::CreateBuildWithProcessBasedServerRequest& request, const ProcessApiCallback<MultiplayerModels::CreateBuildWithProcessBasedServerResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateMatchmakingTicket(MultiplayerModels::CreateMatchmakingTicketRequest& request, const ProcessApiCallback<MultiplayerModels::CreateMatchmakingTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateRemoteUser(MultiplayerModels::CreateRemoteUserRequest& request, const ProcessApiCallback<MultiplayerModels::CreateRemoteUserResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateServerBackfillTicket(MultiplayerModels::CreateServerBackfillTicketRequest& request, const ProcessApiCallback<MultiplayerModels::CreateServerBackfillTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateServerMatchmakingTicket(MultiplayerModels::CreateServerMatchmakingTicketRequest& request, const ProcessApiCallback<MultiplayerModels::CreateMatchmakingTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void CreateTitleMultiplayerServersQuotaChange(MultiplayerModels::CreateTitleMultiplayerServersQuotaChangeRequest& request, const ProcessApiCallback<MultiplayerModels::CreateTitleMultiplayerServersQuotaChangeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteAsset(MultiplayerModels::DeleteAssetRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteBuild(MultiplayerModels::DeleteBuildRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteBuildAlias(MultiplayerModels::DeleteBuildAliasRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteBuildRegion(MultiplayerModels::DeleteBuildRegionRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteCertificate(MultiplayerModels::DeleteCertificateRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteContainerImageRepository(MultiplayerModels::DeleteContainerImageRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void DeleteRemoteUser(MultiplayerModels::DeleteRemoteUserRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void EnableMultiplayerServersForTitle(MultiplayerModels::EnableMultiplayerServersForTitleRequest& request, const ProcessApiCallback<MultiplayerModels::EnableMultiplayerServersForTitleResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAssetDownloadUrl(MultiplayerModels::GetAssetDownloadUrlRequest& request, const ProcessApiCallback<MultiplayerModels::GetAssetDownloadUrlResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetAssetUploadUrl(MultiplayerModels::GetAssetUploadUrlRequest& request, const ProcessApiCallback<MultiplayerModels::GetAssetUploadUrlResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetBuild(MultiplayerModels::GetBuildRequest& request, const ProcessApiCallback<MultiplayerModels::GetBuildResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetBuildAlias(MultiplayerModels::GetBuildAliasRequest& request, const ProcessApiCallback<MultiplayerModels::BuildAliasDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetContainerRegistryCredentials(MultiplayerModels::GetContainerRegistryCredentialsRequest& request, const ProcessApiCallback<MultiplayerModels::GetContainerRegistryCredentialsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMatch(MultiplayerModels::GetMatchRequest& request, const ProcessApiCallback<MultiplayerModels::GetMatchResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMatchmakingTicket(MultiplayerModels::GetMatchmakingTicketRequest& request, const ProcessApiCallback<MultiplayerModels::GetMatchmakingTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMultiplayerServerDetails(MultiplayerModels::GetMultiplayerServerDetailsRequest& request, const ProcessApiCallback<MultiplayerModels::GetMultiplayerServerDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMultiplayerServerLogs(MultiplayerModels::GetMultiplayerServerLogsRequest& request, const ProcessApiCallback<MultiplayerModels::GetMultiplayerServerLogsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetMultiplayerSessionLogsBySessionId(MultiplayerModels::GetMultiplayerSessionLogsBySessionIdRequest& request, const ProcessApiCallback<MultiplayerModels::GetMultiplayerServerLogsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetQueueStatistics(MultiplayerModels::GetQueueStatisticsRequest& request, const ProcessApiCallback<MultiplayerModels::GetQueueStatisticsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetRemoteLoginEndpoint(MultiplayerModels::GetRemoteLoginEndpointRequest& request, const ProcessApiCallback<MultiplayerModels::GetRemoteLoginEndpointResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetServerBackfillTicket(MultiplayerModels::GetServerBackfillTicketRequest& request, const ProcessApiCallback<MultiplayerModels::GetServerBackfillTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleEnabledForMultiplayerServersStatus(MultiplayerModels::GetTitleEnabledForMultiplayerServersStatusRequest& request, const ProcessApiCallback<MultiplayerModels::GetTitleEnabledForMultiplayerServersStatusResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleMultiplayerServersQuotaChange(MultiplayerModels::GetTitleMultiplayerServersQuotaChangeRequest& request, const ProcessApiCallback<MultiplayerModels::GetTitleMultiplayerServersQuotaChangeResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetTitleMultiplayerServersQuotas(MultiplayerModels::GetTitleMultiplayerServersQuotasRequest& request, const ProcessApiCallback<MultiplayerModels::GetTitleMultiplayerServersQuotasResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void JoinMatchmakingTicket(MultiplayerModels::JoinMatchmakingTicketRequest& request, const ProcessApiCallback<MultiplayerModels::JoinMatchmakingTicketResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListArchivedMultiplayerServers(MultiplayerModels::ListMultiplayerServersRequest& request, const ProcessApiCallback<MultiplayerModels::ListMultiplayerServersResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListAssetSummaries(MultiplayerModels::ListAssetSummariesRequest& request, const ProcessApiCallback<MultiplayerModels::ListAssetSummariesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListBuildAliases(MultiplayerModels::ListBuildAliasesRequest& request, const ProcessApiCallback<MultiplayerModels::ListBuildAliasesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListBuildSummariesV2(MultiplayerModels::ListBuildSummariesRequest& request, const ProcessApiCallback<MultiplayerModels::ListBuildSummariesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListCertificateSummaries(MultiplayerModels::ListCertificateSummariesRequest& request, const ProcessApiCallback<MultiplayerModels::ListCertificateSummariesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListContainerImages(MultiplayerModels::ListContainerImagesRequest& request, const ProcessApiCallback<MultiplayerModels::ListContainerImagesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListContainerImageTags(MultiplayerModels::ListContainerImageTagsRequest& request, const ProcessApiCallback<MultiplayerModels::ListContainerImageTagsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListMatchmakingTicketsForPlayer(MultiplayerModels::ListMatchmakingTicketsForPlayerRequest& request, const ProcessApiCallback<MultiplayerModels::ListMatchmakingTicketsForPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListMultiplayerServers(MultiplayerModels::ListMultiplayerServersRequest& request, const ProcessApiCallback<MultiplayerModels::ListMultiplayerServersResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListPartyQosServers(MultiplayerModels::ListPartyQosServersRequest& request, const ProcessApiCallback<MultiplayerModels::ListPartyQosServersResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListQosServersForTitle(MultiplayerModels::ListQosServersForTitleRequest& request, const ProcessApiCallback<MultiplayerModels::ListQosServersForTitleResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListServerBackfillTicketsForPlayer(MultiplayerModels::ListServerBackfillTicketsForPlayerRequest& request, const ProcessApiCallback<MultiplayerModels::ListServerBackfillTicketsForPlayerResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListTitleMultiplayerServersQuotaChanges(MultiplayerModels::ListTitleMultiplayerServersQuotaChangesRequest& request, const ProcessApiCallback<MultiplayerModels::ListTitleMultiplayerServersQuotaChangesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListVirtualMachineSummaries(MultiplayerModels::ListVirtualMachineSummariesRequest& request, const ProcessApiCallback<MultiplayerModels::ListVirtualMachineSummariesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RequestMultiplayerServer(MultiplayerModels::RequestMultiplayerServerRequest& request, const ProcessApiCallback<MultiplayerModels::RequestMultiplayerServerResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RolloverContainerRegistryCredentials(MultiplayerModels::RolloverContainerRegistryCredentialsRequest& request, const ProcessApiCallback<MultiplayerModels::RolloverContainerRegistryCredentialsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ShutdownMultiplayerServer(MultiplayerModels::ShutdownMultiplayerServerRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UntagContainerImage(MultiplayerModels::UntagContainerImageRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBuildAlias(MultiplayerModels::UpdateBuildAliasRequest& request, const ProcessApiCallback<MultiplayerModels::BuildAliasDetailsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBuildName(MultiplayerModels::UpdateBuildNameRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBuildRegion(MultiplayerModels::UpdateBuildRegionRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UpdateBuildRegions(MultiplayerModels::UpdateBuildRegionsRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UploadCertificate(MultiplayerModels::UploadCertificateRequest& request, const ProcessApiCallback<MultiplayerModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnCancelAllMatchmakingTicketsForPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCancelAllServerBackfillTicketsForPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCancelMatchmakingTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCancelServerBackfillTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateBuildAliasResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateBuildWithCustomContainerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateBuildWithManagedContainerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateBuildWithProcessBasedServerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateMatchmakingTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateRemoteUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateServerBackfillTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateServerMatchmakingTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnCreateTitleMultiplayerServersQuotaChangeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteAssetResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteBuildAliasResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteBuildRegionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteCertificateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteContainerImageRepositoryResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnDeleteRemoteUserResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnEnableMultiplayerServersForTitleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAssetDownloadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetAssetUploadUrlResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetBuildResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetBuildAliasResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetContainerRegistryCredentialsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMatchResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMatchmakingTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMultiplayerServerDetailsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMultiplayerServerLogsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetMultiplayerSessionLogsBySessionIdResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetQueueStatisticsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetRemoteLoginEndpointResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetServerBackfillTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleEnabledForMultiplayerServersStatusResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleMultiplayerServersQuotaChangeResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetTitleMultiplayerServersQuotasResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnJoinMatchmakingTicketResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListArchivedMultiplayerServersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListAssetSummariesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListBuildAliasesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListBuildSummariesV2Result(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListCertificateSummariesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListContainerImagesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListContainerImageTagsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListMatchmakingTicketsForPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListMultiplayerServersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListPartyQosServersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListQosServersForTitleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListServerBackfillTicketsForPlayerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListTitleMultiplayerServersQuotaChangesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListVirtualMachineSummariesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRequestMultiplayerServerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRolloverContainerRegistryCredentialsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnShutdownMultiplayerServerResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUntagContainerImageResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBuildAliasResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBuildNameResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBuildRegionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUpdateBuildRegionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUploadCertificateResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
