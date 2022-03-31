#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabCloudScriptDataModels.h>
#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class CallRequestContainerBase;
    class CallRequestContainer;
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// Main interface for PlayFab Sdk, specifically all CloudScript APIs
    /// </summary>
    class PlayFabCloudScriptInstanceAPI
    {
    private:
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;

    public:
        PlayFabCloudScriptInstanceAPI(const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);
        PlayFabCloudScriptInstanceAPI(const std::shared_ptr<PlayFabApiSettings>& apiSettings, const std::shared_ptr<PlayFabAuthenticationContext>& authenticationContext);

        ~PlayFabCloudScriptInstanceAPI() = default;
        PlayFabCloudScriptInstanceAPI(const PlayFabCloudScriptInstanceAPI& source) = delete; // disable copy
        PlayFabCloudScriptInstanceAPI(PlayFabCloudScriptInstanceAPI&&) = delete; // disable move
        PlayFabCloudScriptInstanceAPI& operator=(const PlayFabCloudScriptInstanceAPI& source) = delete; // disable assignment
        PlayFabCloudScriptInstanceAPI& operator=(PlayFabCloudScriptInstanceAPI&& other) = delete; // disable move assignment

        std::shared_ptr<PlayFabApiSettings> GetSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetAuthenticationContext() const;
        /// <summary>
        /// Calls the Update function on your implementation of the IHttpPlugin to check for responses to HTTP requests.
        /// All api's (Client, Server, Admin etc.) share the same IHttpPlugin. 
        /// This means that you only need to call Update() on one API to retrieve the responses for all APIs.
        /// Additional calls to Update (on any API) during the same tick are unlikely to retrieve additional responses.
        /// Call Update when your game ticks as follows:
        ///     CloudScript.Update();
        /// </summary>
        size_t Update();
        void ForgetAllCredentials();

        // ------------ Generated API calls
        void ExecuteEntityCloudScript(CloudScriptModels::ExecuteEntityCloudScriptRequest& request, const ProcessApiCallback<CloudScriptModels::ExecuteCloudScriptResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ExecuteFunction(CloudScriptModels::ExecuteFunctionRequest& request, const ProcessApiCallback<CloudScriptModels::ExecuteFunctionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void GetFunction(CloudScriptModels::GetFunctionRequest& request, const ProcessApiCallback<CloudScriptModels::GetFunctionResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListFunctions(CloudScriptModels::ListFunctionsRequest& request, const ProcessApiCallback<CloudScriptModels::ListFunctionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListHttpFunctions(CloudScriptModels::ListFunctionsRequest& request, const ProcessApiCallback<CloudScriptModels::ListHttpFunctionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void ListQueuedFunctions(CloudScriptModels::ListFunctionsRequest& request, const ProcessApiCallback<CloudScriptModels::ListQueuedFunctionsResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PostFunctionResultForEntityTriggeredAction(CloudScriptModels::PostFunctionResultForEntityTriggeredActionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PostFunctionResultForFunctionExecution(CloudScriptModels::PostFunctionResultForFunctionExecutionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PostFunctionResultForPlayerTriggeredAction(CloudScriptModels::PostFunctionResultForPlayerTriggeredActionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void PostFunctionResultForScheduledTask(CloudScriptModels::PostFunctionResultForScheduledTaskRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RegisterHttpFunction(CloudScriptModels::RegisterHttpFunctionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void RegisterQueuedFunction(CloudScriptModels::RegisterQueuedFunctionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);
        void UnregisterFunction(CloudScriptModels::UnregisterFunctionRequest& request, const ProcessApiCallback<CloudScriptModels::EmptyResult> callback, const ErrorCallback errorCallback = nullptr, void* customData = nullptr);

        // ------------ Generated result handlers
        void OnExecuteEntityCloudScriptResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnExecuteFunctionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnGetFunctionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListFunctionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListHttpFunctionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnListQueuedFunctionsResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPostFunctionResultForEntityTriggeredActionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPostFunctionResultForFunctionExecutionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPostFunctionResultForPlayerTriggeredActionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnPostFunctionResultForScheduledTaskResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRegisterHttpFunctionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnRegisterQueuedFunctionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);
        void OnUnregisterFunctionResult(int httpCode, const std::string& result, const std::shared_ptr<CallRequestContainerBase>& reqContainer);

        bool ValidateResult(PlayFabResultCommon& resultCommon, const CallRequestContainer& container);
    };
}

#endif
