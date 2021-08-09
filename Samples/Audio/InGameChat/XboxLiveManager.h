//--------------------------------------------------------------------------------------
// XboxLiveManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
class PlayFabPartyManager;

class XboxLiveManager
{
public:
    XboxLiveManager() noexcept(false);
    ~XboxLiveManager();

    XboxLiveManager(XboxLiveManager&&) = delete;
    XboxLiveManager& operator= (XboxLiveManager&&) = delete;

    XboxLiveManager(XboxLiveManager const&) = delete;
    XboxLiveManager& operator=(XboxLiveManager const&) = delete;

    HRESULT Initialize();
    HRESULT AddLocalUser(XUserHandle user);
    HRESULT CreateSession();
    HRESULT LeaveSession();
    HRESULT JoinSession(const char* sessionHandle);
    HRESULT FindJoinableSessions(std::function<void()> callback);
    HRESULT InviteFriends(XUserHandle user);
    HRESULT DoWork(float delta);

    inline std::vector<XblMultiplayerActivityDetails> GetJoinableSessions() { return m_joinableSessions; }
    inline bool IsInitialized() { return m_initialized; }

    void SendNetworkMessage(uint64_t xuid, std::vector<uint8_t>& message, bool reliable);
    std::wstring GetGamertagForXuid(uint64_t xuid);

private:
    void SetJoinableSessions(std::vector<XblMultiplayerActivityDetails> sessions);
    void SetGamertagForXuid(uint64_t xuid, const char* gamertag);
    HRESULT GetMemberByXuid(uint64_t xuid, XblMultiplayerManagerMember* member);
    std::vector<XblMultiplayerManagerMember> GetMembersOnSameDevice(uint64_t xuid);

    void OnEndpointChanged(uint64_t xuid, bool connected);

    HRESULT AddUserToSession(XUserHandle user);
    void ProcessPendingConnections();

    // MPM event handlers
    void OnMemberPropertychanged(const XblMultiplayerEvent& event);
    void OnJoinLobbyCompleted(const XblMultiplayerEvent& event);
    void OnUserAdded(const XblMultiplayerEvent& event);
    void OnUserRemoved(const XblMultiplayerEvent& event);
    void OnMemberJoined(const XblMultiplayerEvent& event);
    void OnMemberLeft(const XblMultiplayerEvent& event);

private:
    std::unique_ptr<PlayFabPartyManager> m_networkManager;
    std::vector<XblMultiplayerActivityDetails> m_joinableSessions;
    std::map<uint64_t, std::string> m_mapXuidToGamertag;
    std::map<uint64_t, std::string> m_mapXuidToDevice;
    std::map<uint64_t, uint64_t> m_mapChatIdToEndpointId;
    std::map<std::string, uint64_t> m_mapDeviceToChatId;

    std::vector<uint64_t> m_pendingConnections;
    std::string m_networkDescriptor;
    char m_scid[64];
    bool m_processMessages;
    bool m_initialized;
    int m_usersAdded;
    std::recursive_mutex m_dataLock;

    const char* c_networkId = "InGameChatNetwork";
};

struct FindSessionsContext
{
    FindSessionsContext(XboxLiveManager *manager, std::function<void()> callback) :
        Manager(manager),
        Callback(callback)
    {
    }

    XboxLiveManager *Manager;
    std::function<void()> Callback;
};
