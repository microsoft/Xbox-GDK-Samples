#pragma once

#include <playfab/PlayFabError.h>
#include <playfab/PlayFabCallRequestContainerBase.h>

namespace PlayFab
{
    class PlayFabApiSettings;

    /// <summary>
    /// Internal PlayFabHttp container for each API call
    /// </summary>
    class CallRequestContainer : public CallRequestContainerBase
    {
    public:
        CallRequestContainer(const std::string& url,
            const std::unordered_map<std::string, std::string>& headers,
            const std::string& requestBody,
            CallRequestContainerCallback callback,
            std::shared_ptr<PlayFabApiSettings> apiSettings,
            std::shared_ptr<PlayFabAuthenticationContext> context,
            void* customData);

        virtual ~CallRequestContainer() override;
        std::string GetFullUrl() const;
        std::shared_ptr<PlayFabApiSettings> GetApiSettings() const;
        std::shared_ptr<PlayFabAuthenticationContext> GetContext() const;
        std::string GetRequestId() const;
        void SetRequestId(const std::string& newRequestId);
        void ThrowIfSettingsInvalid();

        // TODO: clean up these public variables with setters/getters when you have the chance.

        bool finished;
        std::string responseString;
        Json::Value responseJson = Json::Value::null;
        std::string requestId;
        PlayFabError errorWrapper;
        std::shared_ptr<void> successCallback;
        ErrorCallback errorCallback;
        std::shared_ptr<PlayFabApiSettings> m_settings;
        std::shared_ptr<PlayFabAuthenticationContext> m_context;
    };
}
