#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabGroupsDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all Groups APIs
    /// </summary>
    class PlayFabGroupsAPI
    {
    public:
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     Groups.Update();
        /// </summary>
        static size_t Update();
        static void ForgetAllCredentials();


        // ------------ Generated API calls
        static void AcceptGroupApplication(GroupsModels::AcceptGroupApplicationRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AcceptGroupInvitation(GroupsModels::AcceptGroupInvitationRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void AddMembers(GroupsModels::AddMembersRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ApplyToGroup(GroupsModels::ApplyToGroupRequest& request, const ProcessApiCallback<GroupsModels::ApplyToGroupResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void BlockEntity(GroupsModels::BlockEntityRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ChangeMemberRole(GroupsModels::ChangeMemberRoleRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateGroup(GroupsModels::CreateGroupRequest& request, const ProcessApiCallback<GroupsModels::CreateGroupResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void CreateRole(GroupsModels::CreateGroupRoleRequest& request, const ProcessApiCallback<GroupsModels::CreateGroupRoleResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteGroup(GroupsModels::DeleteGroupRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void DeleteRole(GroupsModels::DeleteRoleRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void GetGroup(GroupsModels::GetGroupRequest& request, const ProcessApiCallback<GroupsModels::GetGroupResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void InviteToGroup(GroupsModels::InviteToGroupRequest& request, const ProcessApiCallback<GroupsModels::InviteToGroupResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void IsMember(GroupsModels::IsMemberRequest& request, const ProcessApiCallback<GroupsModels::IsMemberResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListGroupApplications(GroupsModels::ListGroupApplicationsRequest& request, const ProcessApiCallback<GroupsModels::ListGroupApplicationsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListGroupBlocks(GroupsModels::ListGroupBlocksRequest& request, const ProcessApiCallback<GroupsModels::ListGroupBlocksResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListGroupInvitations(GroupsModels::ListGroupInvitationsRequest& request, const ProcessApiCallback<GroupsModels::ListGroupInvitationsResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListGroupMembers(GroupsModels::ListGroupMembersRequest& request, const ProcessApiCallback<GroupsModels::ListGroupMembersResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListMembership(GroupsModels::ListMembershipRequest& request, const ProcessApiCallback<GroupsModels::ListMembershipResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void ListMembershipOpportunities(GroupsModels::ListMembershipOpportunitiesRequest& request, const ProcessApiCallback<GroupsModels::ListMembershipOpportunitiesResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveGroupApplication(GroupsModels::RemoveGroupApplicationRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveGroupInvitation(GroupsModels::RemoveGroupInvitationRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void RemoveMembers(GroupsModels::RemoveMembersRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UnblockEntity(GroupsModels::UnblockEntityRequest& request, const ProcessApiCallback<GroupsModels::EmptyResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateGroup(GroupsModels::UpdateGroupRequest& request, const ProcessApiCallback<GroupsModels::UpdateGroupResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        static void UpdateRole(GroupsModels::UpdateGroupRoleRequest& request, const ProcessApiCallback<GroupsModels::UpdateGroupRoleResponse> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

    private:
        PlayFabGroupsAPI(); // Private constructor, static class should never have an instance
        PlayFabGroupsAPI(const PlayFabGroupsAPI& other); // Private copy-constructor, static class should never have an instance

        // ------------ Generated result handlers
        static void OnAcceptGroupApplicationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAcceptGroupInvitationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnAddMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnApplyToGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnBlockEntityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnChangeMemberRoleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnCreateRoleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnDeleteRoleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnGetGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnInviteToGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnIsMemberResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListGroupApplicationsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListGroupBlocksResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListGroupInvitationsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListGroupMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListMembershipResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnListMembershipOpportunitiesResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveGroupApplicationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveGroupInvitationResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnRemoveMembersResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUnblockEntityResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateGroupResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        static void OnUpdateRoleResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        static bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif // #if !defined(DISABLE_PLAYFABENTITY_API)
