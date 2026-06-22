//--------------------------------------------------------------------------------------
// NetworkManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UnsecuredNetworkManager.h"

#include "ws2tcpip.h"

using namespace NetRumble;

#include <XGameRuntime.h>

UnsecuredNetworkManager::UnsecuredNetworkManager() :
    m_connected(false),
    m_host{false}
{
    //XNetworkingQueryPreferredLocalUdpMultiplayerPort(&c_ClientPortNumber);
}

UnsecuredNetworkManager::~UnsecuredNetworkManager()
{
}

void UnsecuredNetworkManager::Initialize()
{
    //m_network = std::make_unique<PeerToPeerNetworkManager>();

    //m_peerId = m_network->GenerateLocalPeerId();
    //DEBUGLOG("Generated PeerID: %u\n", m_peerId);
    

    /*m_network->SetPeerAddedHandler([this](TPeerId peer)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        DEBUGLOG("UnsecuredNetworkManager: PeerAdded: %u\n", peer);
        m_addedUsers.push_back(peer);
    });

    m_network->SetPeerRemovedHandler([this](TPeerId peer, bool)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        DEBUGLOG("UnsecuredNetworkManager: PeerRemoved: %u\n", peer);
        m_removedUsers.push_back(peer);
    });

    m_network->SetDataReceivedHandler([this](TPeerId peer, TByteArray data, uint32_t size)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);

        m_receivedMessages.emplace(InternalMessage{ peer, std::vector<uint8_t>(data, data + size) });
    });*/
}

void UnsecuredNetworkManager::Shutdown()
{
    /*m_connected = false;

    m_network->DestroyNetwork();*/
}

uint32_t UnsecuredNetworkManager::GenerateConsoleId()
{
    /*srand(static_cast<unsigned int>(GetTickCount64()));
    return static_cast<uint32_t>((((rand() % 4) + 1) * 100) + (rand() % ((rand() % 98) + 1)));*/
    return -1;
}

std::string UnsecuredNetworkManager::GetExernalAddress()
{
    return "";// m_network->GetRemoteNetworkAddress();
}

uint32_t UnsecuredNetworkManager::GetPeerIdFromAddress(std::string_view)
{
    return 0;
}

bool UnsecuredNetworkManager::CreateNetwork()
{
    //try
    //{
    //    m_network->CreateNetwork(
    //        m_peerId,               // pseudo-random client id
    //        "",                   // bind ip addr associated with the hostname
    //        c_ClientPortNumber     // local endpoint port number
    //        );
    //}
    //catch (...)
    //{
    //    return false;
    //}

    return true;
}

uint32_t UnsecuredNetworkManager::ConnectToPeer(std::string_view hostAddress)
{
    /*if (!m_network)
    {
        return 0xFFFFFFFF;
    }

    return m_network->AddPeerToNetwork(
        hostAddress,
        c_ClientPortNumber
        );*/
    return -1;
}

void UnsecuredNetworkManager::SendGameMessage(std::vector<uint8_t>& message)
{
    /*if (!m_network)
    {
        return;
    }

    m_network->SendDataToNetwork(
        message.data(),
        static_cast<uint32_t>(message.size())
        );*/
}

void UnsecuredNetworkManager::DoWork(float_t delta)
{
    //if (m_network)
    //{
    //    m_network->DoWork(delta);
    //}

    //// Process our queues
    //{
    //    std::lock_guard<std::mutex> lock(m_dataLock);

    //    for (auto peerid : m_addedUsers)
    //    {
    //        if (peerid == m_peerId)
    //        {
    //            DEBUGLOG("UnsecuredNetworkManager: Our own peer added\n");
    //        }
    //        else
    //        {
    //            m_connected = true;

    //            if (m_peerChanged)
    //            {
    //                m_peerChanged(peerid, true);
    //            }
    //        }
    //    }

    //    m_addedUsers.clear();

    //    for (auto peerid : m_removedUsers)
    //    {
    //        if (peerid == m_peerId)
    //        {
    //            DEBUGLOG("UnsecuredNetworkManager: Our own peer removed\n");

    //            m_connected = false;
    //        }
    //        else
    //        {
    //            DEBUGLOG("UnsecuredNetworkManager: Peer removed: %u\n", peerid);

    //            if (m_peerChanged)
    //            {
    //                m_peerChanged(peerid, false);
    //            }
    //        }
    //    }

    //    m_removedUsers.clear();

    //    if (m_messageHandler)
    //    {
    //        while (!m_receivedMessages.empty())
    //        {
    //            auto &message = m_receivedMessages.front();
    //            m_messageHandler(message.PeerId, message.Data);
    //            m_receivedMessages.pop();
    //        }
    //    }
    //}
}
