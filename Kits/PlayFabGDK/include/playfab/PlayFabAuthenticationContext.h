#pragma once

#include <string>

namespace PlayFab
{
    /// <summary>
    /// Container for PlayFab authentication credentials data.
    /// </summary>
    class PlayFabAuthenticationContext
    {
#if !defined(DISABLE_PLAYFABCLIENT_API)
    public: // Client-only variables should only be visible when appropriate
#else
    private: // But, static library memory size and alloc issues mean it always needs to exist
#endif
        std::string playFabId; // Master_Player_Entity Id for the Player that logged in
        std::string clientSessionTicket; // Client session ticket that is used as an authentication token in many PlayFab API methods.

    public:
        std::string entityId; // Entity Id for the active entity
        std::string entityType; // Entity Type for the active entity
        std::string entityToken; // User's entity token. Entity tokens are required by all Entity API methods.

        PlayFabAuthenticationContext();
        PlayFabAuthenticationContext(const PlayFabAuthenticationContext& other) = delete;
        PlayFabAuthenticationContext(PlayFabAuthenticationContext&& other) = delete;
        PlayFabAuthenticationContext& operator=(const PlayFabAuthenticationContext& other) = delete;
        PlayFabAuthenticationContext& operator=(PlayFabAuthenticationContext&& other) = delete;
        ~PlayFabAuthenticationContext() = default;

        void HandlePlayFabLogin(const std::string& _playFabId, const std::string& _clientSessionTicket, const std::string& _entityId, const std::string& _entityType, const std::string& _entityToken);
        void ForgetAllCredentials();
        bool IsClientLoggedIn();
        bool IsEntityLoggedIn();
    };
}
