//--------------------------------------------------------------------------------------
// UnsecuredNetworkManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UnsecuredNetworkManager.h"
#include "InGameChat_Desktop.h"

using namespace PeerMeshForSamples;

UnsecuredNetworkManager::UnsecuredNetworkManager() :
    m_peerId(0),
    m_connected(false),
    m_host{false}
{
    auto hr = XNetworkingQueryPreferredLocalUdpMultiplayerPort(&m_ClientPortNumber);

    if (FAILED(hr))
    {
        DebugTrace("Failed to get local network port!");
        DX::ThrowIfFailed(hr);
    }
}

UnsecuredNetworkManager::~UnsecuredNetworkManager()
{
}

void UnsecuredNetworkManager::Initialize(uint32_t peerId)
{
    m_network = std::make_unique<PeerToPeerNetworkManager>();
    m_peerId = peerId;
 
    DebugTrace("Creating network with PeerID: %u", m_peerId);

    m_network->SetPeerAddedHandler([this](TPeerId peer)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        DebugTrace("UnsecuredNetworkManager: PeerAdded: %u", peer);
        m_addedUsers.push_back(peer);
    });

    m_network->SetPeerRemovedHandler([this](TPeerId peer, bool)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        DebugTrace("UnsecuredNetworkManager: PeerRemoved: %u", peer);
        m_removedUsers.push_back(peer);
    });

    m_network->SetDataReceivedHandler([this](TPeerId peer, TByteArray data, uint32_t size)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        m_receivedMessages.emplace(InternalMessage{ peer, std::vector<uint8_t>(data, data + size) });
    });
}

void UnsecuredNetworkManager::Shutdown()
{
    m_connected = false;

    if (m_network)
    {
        m_network->DestroyNetwork();
        m_network.reset();
    }
}

std::string UnsecuredNetworkManager::GetExernalAddress()
{
    return m_network->GetRemoteNetworkAddress();
}

bool UnsecuredNetworkManager::CreateNetwork()
{
    try
    {
        m_network->CreateNetwork(
            m_peerId,               // Peer ID generated from a user XUID
            c_EmptyString,          // Empty string to use the hostname
            m_ClientPortNumber      // local endpoint port number
            );
    }
    catch (...)
    {
        return false;
    }

    return true;
}

void UnsecuredNetworkManager::ConnectToPeer(uint32_t peerId, std::string_view hostAddress)
{
    if (!m_network)
    {
        return;
    }

    m_network->AddPeerToNetwork(
        peerId,
        hostAddress,
        m_ClientPortNumber
        );
}

void UnsecuredNetworkManager::SendNetworkMessage(uint32_t peerId, std::vector<uint8_t>& message)
{
    if (!m_network)
    {
        return;
    }

    m_network->SendDataToPeer(
        peerId,
        message.data(),
        static_cast<uint32_t>(message.size())
        );
}

void UnsecuredNetworkManager::DoWork(float_t delta)
{
    if (m_network)
    {
        m_network->DoWork(delta);
    }

    // Process our queues
    {
        std::lock_guard<std::mutex> lock(m_dataLock);

        for (auto peerid : m_addedUsers)
        {
            if (peerid == m_peerId)
            {
                DebugTrace("UnsecuredNetworkManager: Our own peer added");
            }
            else
            {
                m_connected = true;

                if (m_peerChanged)
                {
                    m_peerChanged(peerid, true);
                }
            }
        }

        m_addedUsers.clear();

        for (auto peerid : m_removedUsers)
        {
            if (peerid == m_peerId)
            {
                DebugTrace("UnsecuredNetworkManager: Our own peer removed");

                m_connected = false;
            }
            else
            {
                DebugTrace("UnsecuredNetworkManager: Peer removed: %u", peerid);

                if (m_peerChanged)
                {
                    m_peerChanged(peerid, false);
                }
            }
        }

        m_removedUsers.clear();

        if (m_messageHandler)
        {
            while (!m_receivedMessages.empty())
            {
                auto &message = m_receivedMessages.front();
                m_messageHandler(message.PeerId, message.Data);
                m_receivedMessages.pop();
            }
        }
    }
}
