//--------------------------------------------------------------------------------------
// XboxLiveManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
class UnsecuredNetworkManager;

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
    HRESULT JoinSession(const char* handle);
    HRESULT FindJoinableSessions(std::function<void()> callback);
    HRESULT InviteFriends(XUserHandle user);
    HRESULT DoWork(float delta);

    inline std::vector<XblMultiplayerActivityDetails> GetJoinableSessions() { return m_joinableSessions; }
    inline bool IsInitialized() { return m_initialized; }

    void SendNetworkMessage(uint32_t peerId, std::vector<uint8_t>& message);
    uint64_t GetXuidForPeer(uint32_t peerId);
    std::wstring GetGamertagForXuid(uint64_t xuid);

private:
    void SetJoinableSessions(std::vector<XblMultiplayerActivityDetails> sessions);
    void SetGamertagForXuid(uint64_t xuid, const char* gamertag);
    HRESULT AddUserToSession(XUserHandle user);

private:
    std::unique_ptr<UnsecuredNetworkManager> m_networkManager;
    std::vector<XblMultiplayerActivityDetails> m_joinableSessions;
    std::map<uint32_t, uint64_t> m_mapPeerToXuid;
    std::map<uint64_t, std::string> m_mapXuidToGamertag;
    char m_scid[64];
    bool m_processMessages;
    bool m_initialized;
    int m_usersAdded;
    uint32_t m_peerId;
    std::mutex m_dataLock;
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
