//--------------------------------------------------------------------------------------
// UnsecuredNetworkManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PeerToPeerNetworkManager.h"

using NetworkMessageHandler = std::function<void(uint32_t,std::vector<uint8_t> &)>;
using NetworkPeerChangeHandler = std::function<void(uint32_t, bool)>;

class UnsecuredNetworkManager
{
public:
    UnsecuredNetworkManager() noexcept(false);
    ~UnsecuredNetworkManager();

    UnsecuredNetworkManager(UnsecuredNetworkManager&&) = delete;
    UnsecuredNetworkManager& operator= (UnsecuredNetworkManager&&) = delete;

    UnsecuredNetworkManager(UnsecuredNetworkManager const&) = delete;
    UnsecuredNetworkManager& operator=(UnsecuredNetworkManager const&) = delete;

    const char* c_EmptyString = "";

    void Initialize(uint32_t peerId);
    bool CreateNetwork();
    void ConnectToPeer(uint32_t peerId, std::string_view hostAddress);

    void SendNetworkMessage(uint32_t peerId, std::vector<uint8_t>& message);
    void Shutdown();

    std::string GetExernalAddress();

    void DoWork(float_t delta);

    inline bool IsHost() const { return m_host == true; }
    inline void SetHost(bool isHost) { m_host = isHost; }
    inline bool IsConnected() const { return m_connected; }
    inline uint32_t LocalPeerId() const { return m_peerId; }

    void RegisterNetworkMessageHandler(NetworkMessageHandler handler) { m_messageHandler = handler; }
    void RegisterPeerChangeHandler(NetworkPeerChangeHandler handler) { m_peerChanged = handler; }

private:
    struct InternalMessage
    {
        uint32_t PeerId;
        std::vector<uint8_t> Data;
    };

    std::queue<InternalMessage> m_receivedMessages;

    std::unique_ptr<PeerMeshForSamples::PeerToPeerNetworkManager> m_network;
    std::vector<PeerMeshForSamples::TPeerId> m_addedUsers;
    std::vector<PeerMeshForSamples::TPeerId> m_removedUsers;
        
    std::mutex m_dataLock;
    uint32_t m_peerId;
    bool m_connected;
    bool m_host;
    NetworkPeerChangeHandler m_peerChanged;
    NetworkMessageHandler m_messageHandler;
    uint16_t m_ClientPortNumber;
};
