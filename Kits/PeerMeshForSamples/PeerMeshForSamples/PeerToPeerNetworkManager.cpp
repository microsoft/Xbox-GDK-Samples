//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "PeerToPeerNetworkManager.h"

namespace PeerMeshForSamples
{

std::wstring g_strLogPath;

void SetNetworkLogPath(const std::wstring& strPath)
{
    g_strLogPath = strPath;
}

const std::wstring& GetNetworkLogPath()
{
    return g_strLogPath;
}


///////////////////////////////////////////////////////////////////////////////
//  PeerToPeerNetworkManager
//
PeerToPeerNetworkManager::PeerToPeerNetworkManager(
    bool sequencedDelivery,
    uint32_t maxSegmentSize,
    uint32_t maxUnacknowledged
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::PeerToPeerNetworkManager()\n");

    _running           = false;
    _sequencedDelivery = sequencedDelivery;
    _maxSegmentSize    = maxSegmentSize;
    _maxUnacknowledged = maxUnacknowledged;
    _retryInterval     = DEFAULT_PACKET_RETRY_INTERVAL;
    _retryCount        = DEFAULT_PACKET_RETRY_COUNT;
    _closewaitTimeout  = DEFAULT_CLOSEWAIT_TIMEOUT;
    _keepAliveInterval = DEFAULT_KEEPALIVE_INTERVAL;
    _peerAdded         = nullptr;
    _peerRemoved       = nullptr;
    _dataReceived      = nullptr;
    _id                = 0;
    _socket            = 0;
    _address           = {};
    _tempAddr          = {};

    // Initialize WinSock
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);

    if (result != 0)
    {
        DEBUGLOG(L"WSAStartup failed with error: %ul\n", WSAGetLastError());
        throw std::runtime_error("WSAStartup failed");
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ~PeerToPeerNetworkManager
//
PeerToPeerNetworkManager::~PeerToPeerNetworkManager()
{
    DEBUGLOG(L"PeerToPeerNetworkManager::~PeerToPeerNetworkManager()\n");

    if (_running)
    {
        DestroyNetwork();
    }

    WSACleanup();
}

///////////////////////////////////////////////////////////////////////////////
//  CreateNetwork
//
void
PeerToPeerNetworkManager::CreateNetwork(
    TPeerId          hostId,
    std::string_view addr,
    uint16_t         port
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::CreateNetwork(%u, %ws)\n", hostId, addr.data());

    if (addr.empty())
    {
        auto result = GetLocalNetworkAddress();

        if (result == false)
        {
            throw std::runtime_error("Failed to get local IP address");
        }
    }
    else
    {
        if (!ResolveAddressFromHost(addr))
        {
            DEBUGLOG(L"Failed to find address for host.\n");
            throw std::runtime_error("Failed to resolve address");
        }
    }

    CreateNetwork(
        hostId,
        (TByteArray) &_tempAddr,
        static_cast<uint32_t>(sizeof(_tempAddr)),
        port
        );
}

void
PeerToPeerNetworkManager::CreateNetwork(
    TPeerId           hostId,
    const TByteArray  hostAddress,
    uint32_t          addrSize,
    uint16_t          port
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::CreateNetwork(%u)\n", hostId);

    if (addrSize != sizeof(_address))
    {
        throw std::invalid_argument("Size of address is incorrect");
    }

    // std::lock_guard<std::mutex> lock(m_lock);

    // Create an IPv6 UDP socket
    _socket = WSASocket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED
        );

    if (_socket == INVALID_SOCKET)
    {
        DEBUGLOG(L"WSASocket failed with error: %ul\n", WSAGetLastError());
        throw std::runtime_error("WSASocket failed");
    }

    //// Enforce IPv6 only
    //int v6only = 0;
    //int result = setsockopt(
    //    _socket,
    //    IPPROTO_IPV6,
    //    IPV6_V6ONLY,
    //    (char*) &v6only,
    //    sizeof(v6only)
    //    );

    //if (result != 0)
    //{
    //    DEBUGLOG(L"setsockopt failed with error: %ul\n", WSAGetLastError());
    //    throw std::runtime_error("setsockopt failed");
    //}

    // Make the socket non-blocking
    ULONG value = 1;
    auto result = ioctlsocket(
        _socket, 
        FIONBIO, 
        &value
        );

    if (result != 0)
    {
        DEBUGLOG(L"ioctlsocket failed with error: %ul\n", WSAGetLastError());
        throw std::runtime_error("ioctlsocket failed");
    }

    // Bind socket to local address
    if (port != 0)
    {
        CopyMemory(&_address, hostAddress, sizeof(_address));

        // Here we convert the port to network byte order
        _address.sin_port = htons(port);
        _address.sin_family = AF_INET;

        result = bind(
            _socket,
            (SOCKADDR*)&_address,
            sizeof(_address)
        );

        if (result != 0)
        {
            DEBUGLOG(L"bind failed with error: %ul\n", WSAGetLastError());
            throw std::runtime_error("bind failed");
        }
    }

    // Good to go
    _running = true;
    _id      = hostId;
}

///////////////////////////////////////////////////////////////////////////////
//  AddPeerToNetwork
//
TPeerId
PeerToPeerNetworkManager::AddPeerToNetwork(
    std::string_view addr,
    uint16_t         port
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::AddPeerToNetwork(%S)\n", addr.data());

    if (!ResolveAddressFromHost(addr))
    {
        DEBUGLOG(L"Failed to find address for host.\n");
        throw std::runtime_error("Failed to resolve address");
    }

    auto peerId = _tempAddr.sin_addr.S_un.S_addr;

    AddPeerToNetwork(
        peerId,
        (TByteArray) &_tempAddr,
        static_cast<uint32_t>(sizeof(_tempAddr)),
        port
        );

    return peerId;
}

void
PeerToPeerNetworkManager::AddPeerToNetwork(
    TPeerId          peerId,
    std::string_view addr,
    uint16_t         port
)
{
    DEBUGLOG(L"PeerToPeerNetworkManager::AddPeerToNetwork(%S)\n", addr.data());

    if (!ResolveAddressFromHost(addr))
    {
        DEBUGLOG(L"Failed to find address for host.\n");
        throw std::runtime_error("Failed to resolve address");
    }

    AddPeerToNetwork(
        peerId,
        (TByteArray)&_tempAddr,
        static_cast<uint32_t>(sizeof(_tempAddr)),
        port
        );
}

void
PeerToPeerNetworkManager::AddPeerToNetwork(
    TPeerId           peerId,
    const TByteArray  peerAddress,
    uint32_t          addrSize,
    uint16_t          port
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::AddPeerToNetwork(%u)\n", peerId);

    // std::lock_guard<std::mutex> lock(m_lock);

    if(!_running)
    {
        return;
    }

    // If the peer
    auto peer = _peers.find(peerId);
    if (peer != _peers.end())
    {
        return;
    }

    SOCKADDR_IN sockaddr = {};

    CopyMemory(&sockaddr, peerAddress, sizeof(sockaddr));

    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;

    // Create a new connection object with the peer id and sockaddr
    auto newUser = std::make_shared<PeerToPeerConnection>(
        peerId,
        (TByteArray) &sockaddr,
        static_cast<uint32_t>(sizeof(sockaddr))
        );

    // Begin connection protocol
    newUser->State                  = EConnectionState::SynchronizeSent;
    newUser->InitialSendSequence    = (_id + 1) * 1000;
    newUser->NextSendSequence       = newUser->InitialSendSequence + 1;
    newUser->OldestUnacknowledged   = newUser->InitialSendSequence;

    _peers[peerId] = newUser;

    _pendingAdds.push_back(peerId);

    DEBUGLOG(L"New peer connecting: ");
    DUMP_CONNECTION(newUser);

    auto reply = std::make_shared<PeerToPeerDataSegment>();

    reply->IsSynchronize      = true;
    reply->Source             = _id;
    reply->Destination        = peerId;
    reply->Sequence           = newUser->InitialSendSequence;

    _segmentsOut.push(reply);
}

///////////////////////////////////////////////////////////////////////////////
//  RemovePeerFromNetwork
//
void
PeerToPeerNetworkManager::RemovePeerFromNetwork(
    TPeerId peerId
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::RemovePeerFromNetwork(%u)\n", peerId);

    std::lock_guard<std::mutex> lock(_lock);

    if(!_running)
    {
        return;
    }

    auto search = _peers.find(peerId);
    if (search != _peers.end())
    {
        auto conn = (*search).second;

        if (conn->State != EConnectionState::Closed &&
            conn->State != EConnectionState::CloseWait)
        {
            conn->State            = EConnectionState::CloseWait;
            conn->CloseWaitTimeout = 0.0f;

            DEBUGLOG(L"Removing peer: ");
            DUMP_CONNECTION(conn);

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->IsReset     = true;
            reply->Source      = _id;
            reply->Destination = conn->Id;
            reply->Sequence    = conn->NextSendSequence;

            _segmentsOut.push(reply);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  DestroyNetwork
//
void
PeerToPeerNetworkManager::DestroyNetwork(void)
{
    DEBUGLOG(L"PeerToPeerNetworkManager::DestroyNetwork()\n");

    std::lock_guard<std::mutex> lock(_lock);

    _running = false;

    // Normally we put connections into CloseWait for a timeout period
    // before killing them, but since this needs to be quick we just tear it all down.

    for (const auto& pair : _peers)
    {
        if (pair.second->State == EConnectionState::Open ||
            pair.second->State == EConnectionState::SynchronizeReceived ||
            pair.second->State == EConnectionState::SynchronizeSent)
        {
            auto reset = std::make_shared<PeerToPeerDataSegment>();

            reset->IsReset     = true;
            reset->Source      = _id;
            reset->Destination = pair.second->Id;
            reset->Sequence    = pair.second->NextSendSequence;

            // Send packet immediately bypassing queues
            SendSegment(reset);
        }
    }

    _peers.clear();
    _outQueue.clear();
    (void)_segmentsIn.empty();
    (void)_segmentsOut.empty();

    closesocket(_socket);
    WSACleanup();

    _socket = INVALID_SOCKET;
    ZeroMemory(&_address, sizeof(_address));
}

///////////////////////////////////////////////////////////////////////////////
//  DoWork
//
void 
PeerToPeerNetworkManager::DoWork(float_t delta)
{
    std::lock_guard<std::mutex> lock(_lock);

    if (!_running)
    {
        return;
    }

    // Read a segment from the network if any are waiting
    ReadSegmentsFromSocket();

    // If there is a segment available, process it
    ProcessSegments();

    // Clean up pending retries
    MarkAcknowledgedSegments();
    
    // Process retry queue
    ProcessRetries(delta);

    // Process connection output queues - packets queued behind the send window
    ProcessQueuedSends();

    // Send pending segments
    // TODO: throttle sends
    SendSegments();

    // Process closewait timers
    ProcessConnectionTimers(delta);
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessSegments
//
void 
PeerToPeerNetworkManager::ProcessSegments()
{
    while (_segmentsIn.size() > 0)
    {
        ProcessSegment(_segmentsIn.front());
        _segmentsIn.pop();
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessQueuedSends
//
void
PeerToPeerNetworkManager::ProcessQueuedSends()
{
    for(auto& pair : _outQueue)
    {
        // Look for segments that have been queued to send
        if (pair.second.size() > 0)
        {
            auto peer = _peers.find(pair.first);
            if (peer != _peers.end())
            {
                auto conn = (*peer).second;

                if (conn->NextSendSequence < conn->OldestUnacknowledged + _maxUnacknowledged)
                {
                    // It's ready to be sent now
                    auto segment = pair.second.front();

                    segment->Sequence      = conn->NextSendSequence;
                    segment->Acknowledge   = conn->LastGoodSegment;
                    segment->IsAcknowledge = (segment->Acknowledge != 0);

                    _segmentsOut.push(segment);
                    pair.second.pop();

                    conn->NextSendSequence += 1;
                }
                else
                {
                    // Leave it in the queue
                }
            }
            else
            {
                // The peer record wasn't found - this can happen if a connection closes with segments in the queue
                pair.second.pop();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessRetries
//
void
PeerToPeerNetworkManager::ProcessRetries(float_t delta)
{
    if (_retries.size() > 0)
    {
        auto segment = _retries.front();

        // Skip any ack'd segments
        while (segment != nullptr && segment->Acknowledged == true && _retries.size() > 0)
        {
            _retries.pop_front();

            if (_retries.size() > 0)
            {
                segment = _retries.front();
            }
            else
            {
                segment = nullptr;
            }
        }

        // See if we should send it again
        if (segment != nullptr)
        {
            segment->Timestamp += delta;

            if (segment->Timestamp >= _retryInterval)
            {
                if (segment->RetryCount >= _retryCount)
                {
                    // Too many retries on this connection so start to close it
                    DEBUGLOG(L"Retry death of peer %u.\n", segment->Destination);
                    DUMP_SEGMENT(segment);

                    auto peer = _peers.find(segment->Destination);
                    if (peer != _peers.end())
                    {
                        auto  conn = (*peer).second;

                        if (conn->State != EConnectionState::Closed &&
                            conn->State != EConnectionState::CloseWait)
                        {
                            conn->State            = EConnectionState::CloseWait;
                            conn->CloseWaitTimeout = 0;
                        }
                    }
                }
                else
                {
                    // Try again
                    DEBUGLOG(L"Retry segment: ");
                    DUMP_SEGMENT(segment);

                    segment->Timestamp = 0;
                    segment->RetryCount += 1;

                    _segmentsOut.push(segment);
                }

                // Remove processed item
                _retries.pop_front();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessConnectionTimers
//
void
PeerToPeerNetworkManager::ProcessConnectionTimers(float_t delta)
{
    std::list<TPeerId> closedPeers;
    for(auto& pair : _peers)
    {
        if (pair.second->State == EConnectionState::CloseWait)
        {
            // This connection is in the CloseWait state so update the timeout
            // and check if it's time to close it completely.
            pair.second->CloseWaitTimeout += delta;

            if (pair.second->CloseWaitTimeout >= _closewaitTimeout)
            {
                pair.second->State = EConnectionState::Closed;

                closedPeers.push_back(pair.second->Id);
            }
        }
        else if (pair.second->State == EConnectionState::Open)
        {
            // If no packets have been received from this connection for the specified
            // interval value, send a NUL packet as a ping/keepalive.
            pair.second->KeepAliveTimeout += delta;

            if (pair.second->KeepAliveTimeout >= _keepAliveInterval)
            {
                auto nul = std::make_shared<PeerToPeerDataSegment>();

                nul->IsNull      = true;
                nul->Source      = _id;
                nul->Destination = pair.second->Id;
                nul->Sequence    = pair.second->NextSendSequence;

                _segmentsOut.push(nul);

                pair.second->NextSendSequence += 1;
                pair.second->KeepAliveTimeout  = 0.0f;
            }
        }
    }

    if (closedPeers.size() > 0)
    {
        for(const auto& peer : closedPeers)
        {
            DEBUGLOG(L"CloseWait timeout for peer %u.\n", peer);

            _peers.erase(peer);
            _outQueue.erase(peer);

            // Fire PeerRemoved callback
            if (_peerRemoved != nullptr)
            {
                _peerRemoved(peer, _peers.size() == 0);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SendSegments
//
void
PeerToPeerNetworkManager::SendSegments()
{
    while (_segmentsOut.size() > 0)
    {
        SendSegment(_segmentsOut.front());
        _segmentsOut.pop();
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SendSegment
//
void
PeerToPeerNetworkManager::SendSegment(
    PDataSegment segment
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::SendSegment()\n");

    TByteArray  buffer       = nullptr;
    uint32_t    bufferLen    = 0;
    DWORD       numBytesSent = 0;
    WSABUF      wsabuf;

    DEBUGLOG(L"Sending: ");
    DUMP_SEGMENT(segment);

    auto peer = _peers.find(segment->Destination);
    if (peer == _peers.end())
    {
        DEBUGLOG(L"Destination peer not connected.\n");
        return;
    }

    // Get the segment data
    segment->Serialize(&buffer, &bufferLen);

    wsabuf.len = bufferLen;
    wsabuf.buf = reinterpret_cast<CHAR*>(buffer);

    auto connection = (*peer).second;

    int result = WSASendTo(
        _socket,
        &wsabuf,
        1,
        &numBytesSent,
        0,
        reinterpret_cast<SOCKADDR*>(connection->Address),
        connection->AddressLength,
        nullptr, 
        nullptr
        );

    if (result != 0)
    {
        DEBUGLOG(L"WSASendTo failed with error %d.\n", WSAGetLastError());

        if (connection->State != EConnectionState::Closed &&
            connection->State != EConnectionState::CloseWait)
        {
            connection->State            = EConnectionState::CloseWait;
            connection->CloseWaitTimeout = 0.0f;
        }
    }

    // Place in the retry list.  It'll be removed when it gets ack'd.
    if (connection->State == EConnectionState::Open)
    {
        segment->Timestamp = 0;
        _retries.push_back(segment);
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ReadSegmentsFromSocket
//
void
PeerToPeerNetworkManager::ReadSegmentsFromSocket()
{
    DWORD            flags = 0;
    DWORD            numberBytesReceived = 0;
    SOCKADDR_STORAGE senderSocketAddress;
    int              senderSocketAddressSize = sizeof(senderSocketAddress);
    TByteArray       buffer;
    WSABUF           wsabuf;
    int              result = 0;

    buffer = new uint8_t[_maxSegmentSize];

    wsabuf.len = _maxSegmentSize;
    wsabuf.buf = reinterpret_cast<CHAR*>(buffer);
 
    do
    {
        result = WSARecvFrom(
            _socket,
            &wsabuf,
            1,
            &numberBytesReceived,
            &flags,
            reinterpret_cast<SOCKADDR*>(&senderSocketAddress),
            &senderSocketAddressSize,
            nullptr,
            nullptr
            );

        if (result == 0 && numberBytesReceived != 0 )
        {
            // Push the segment onto the incoming queue for processing
            try
            {
                auto segment = std::make_shared<PeerToPeerDataSegment>(
                    reinterpret_cast<TByteArray>(&senderSocketAddress),
                    senderSocketAddressSize,
                    buffer,
                    numberBytesReceived
                    );

                _segmentsIn.push(segment);

                DEBUGLOG(L"Received: ");
                DUMP_SEGMENT(segment);
            }
#ifdef NETWORK_DEBUG_SPEW
            catch(std::exception &ex)
            {
                DEBUGLOG(L"Invalid packet received: %s\n", ex.what());
            }
#else
            catch (...)
            {
                // Just ignore it
            }
#endif
        }
        else
        {
            int error = WSAGetLastError();

            // The only error we expect is WOULDBLOCK since we may be issuing a
            // recv() when no data is available.
            if (error != WSAEWOULDBLOCK)
            {
                // Errors could be transient so we'll rely on higher level connectivity
                // checks deal with networking failures.
                DEBUGLOG(L"WSARecvFrom failed with error %d.\n", WSAGetLastError());
            }
        }
    } while (result == 0 && numberBytesReceived != 0);

    if (buffer)
    {
        delete[] buffer;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessSegment
//
void
PeerToPeerNetworkManager::ProcessSegment(PDataSegment segment)
{
    DEBUGLOG(L"PeerToPeerNetworkManager::ProcessSegment()\n");
    DEBUGLOG(L"Process: ");
    DUMP_SEGMENT(segment);

    auto search = _peers.find(segment->Source);
    if (search == _peers.end())
    {
        DEBUGLOG(L"Unknown peer %u\n", segment->Source);

        // Unknown peer sent us data, it's either a new peer attempting to connect
        // or erroneous data (sent before/after the connection is ready/destroyed)
        if (segment->IsSynchronize)
        {
            // New peer connecting.  Create the connection record
            auto newUser = std::make_shared<PeerToPeerConnection>(
                segment->Source,
                segment->SourceAddress,
                segment->SourceAddressLength
                );

            newUser->State                  = EConnectionState::SynchronizeReceived;
            newUser->InitialSendSequence    = (_id + 1) * 1000;
            newUser->NextSendSequence       = newUser->InitialSendSequence + 1;
            newUser->OldestUnacknowledged   = newUser->InitialSendSequence;
            newUser->LastGoodSegment        = segment->Sequence;
            newUser->InitialReceiveSequence = segment->Sequence;

            DEBUGLOG(L"New connection attempt incoming.\n");
            DUMP_CONNECTION(newUser);

            // Respond to the incoming SYN with a SYN ACK
            auto reply = std::make_shared<PeerToPeerDataSegment>();
            
            reply->IsSynchronize            = true;
            reply->IsAcknowledge            = true;
            reply->Source                   = _id;
            reply->Destination              = newUser->Id;
            reply->Sequence                 = newUser->InitialSendSequence;
            reply->Acknowledge              = newUser->LastGoodSegment;

            _segmentsOut.push(reply);
            _peers[newUser->Id] = newUser;

            _pendingAdds.push_back(newUser->Id);
        }
        else
        {
            // Erroneous data.  Skip it.
            DEBUGLOG(L"Received non-SYN segment for unknown peer %u.\n", segment->Source);
        }
    }
    else
    {
        auto conn = (*search).second;

        switch (conn->State)
        {
            case EConnectionState::Closed:
                ProcessClosedStateSegment(conn, segment);
                break;

            case EConnectionState::CloseWait:
                // No segments are accepted in this state
                break;

            case EConnectionState::SynchronizeSent:
                ProcessSynchronizeSentStateSegment(conn, segment);
                break;

            case EConnectionState::SynchronizeReceived:
                ProcessSynchronizeReceivedStateSegment(conn, segment);
                break;

            case EConnectionState::Open:
                ProcessOpenStateSegment(conn, segment);
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessClosedStateSegment
//
void
PeerToPeerNetworkManager::ProcessClosedStateSegment(
    PConnection  conn, 
    PDataSegment segment
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::ProcessClosedStateSegment()\n");
    DUMP_CONNECTION(conn);
    DUMP_SEGMENT(segment);

    if (segment->IsReset)
    {
        // Ignore segment
        DEBUGLOG(L"Reset segment ignored.\n");
        return;
    }

    auto reply = std::make_shared<PeerToPeerDataSegment>();

    reply->IsReset     = true;
    reply->Source      = _id;
    reply->Destination = conn->Id;

    if (segment->IsNull || segment->IsAcknowledge)
    {
        reply->Sequence = segment->Acknowledge + 1;
    }
    else
    {
        reply->Sequence    = 0;
        reply->Acknowledge = reply->Sequence;
    }

    _segmentsOut.push(reply);
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessSynchronizeSentStateSegment
//
void
PeerToPeerNetworkManager::ProcessSynchronizeSentStateSegment(
    PConnection  conn, 
    PDataSegment segment
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::ProcessSynchronizeSentStateSegment()\n");
    DUMP_CONNECTION(conn);
    DUMP_SEGMENT(segment);

    if (segment->IsSynchronize)
    {
        DEBUGLOG(L"Synchronize segment received.\n");

        conn->LastGoodSegment          = segment->Sequence;
        conn->InitialReceiveSequence   = segment->Sequence;

        if (segment->IsAcknowledge)
        {
            DEBUGLOG(L"Segment includes ack.\n");

            conn->OldestUnacknowledged = segment->Acknowledge + 1;
            conn->State                = EConnectionState::Open;

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->Source        = _id;
            reply->Destination   = conn->Id;
            reply->Sequence      = conn->NextSendSequence;
            reply->Acknowledge   = conn->LastGoodSegment;
            reply->IsAcknowledge = (reply->Acknowledge != 0);

            _segmentsOut.push(reply);

            for (size_t i = 0; i < _pendingAdds.size(); i++)
            {
                if (_pendingAdds[i] == conn->Id)
                {
                    if (_peerAdded != nullptr)
                    {
                        _peerAdded(conn->Id);
                    }

                    _pendingAdds.erase(_pendingAdds.begin() + i);
                    break;
                }
            }
        }
        else
        {
            DEBUGLOG(L"Segment does not include ack.  Probably a simultaneous connection attempt.\n");

            conn->State = EConnectionState::SynchronizeReceived;

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->IsSynchronize    = true;
            reply->Source           = _id;
            reply->Destination      = conn->Id;
            reply->Sequence         = conn->InitialSendSequence;
            reply->Acknowledge      = conn->LastGoodSegment;
            reply->IsAcknowledge    = (reply->Acknowledge != 0);

            _segmentsOut.push(reply);
        }

        return;
    }

    if (segment->IsReset)
    {
        if (segment->IsAcknowledge)
        {
            // Connection refused by peer
            DEBUGLOG(L"Segment includes reset ack; connection refused.\n");

            conn->State = EConnectionState::Closed;

            _peers.erase(conn->Id);
            _outQueue.erase(conn->Id);

            // Call PeerRemoved callback
            if (_peerRemoved != nullptr)
            {
                _peerRemoved(conn->Id, _peers.size() == 0);
            }
        }

        return;
    }
                    
    if (segment->IsAcknowledge)
    {
        DEBUGLOG(L"Segment includes ack.\n");

        if (!segment->IsReset && segment->Acknowledge != conn->InitialSendSequence)
        {
            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->IsReset     = true;
            reply->Source      = _id;
            reply->Destination = conn->Id;
            reply->Sequence    = segment->Acknowledge + 1;

            _segmentsOut.push(reply);

            // Connection refused by peer
            conn->State = EConnectionState::Closed;

            _peers.erase(conn->Id);
            _outQueue.erase(conn->Id);

            // Call PeerRemoved callback
            if (_peerRemoved != nullptr)
            {
                _peerRemoved(conn->Id, _peers.size() == 0);
            }
        }

        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessSynchronizeReceivedStateSegment
//
void
PeerToPeerNetworkManager::ProcessSynchronizeReceivedStateSegment(
    PConnection  conn, 
    PDataSegment segment
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::ProcessSynchronizeReceivedStateSegment()\n");
    DUMP_CONNECTION(conn);
    DUMP_SEGMENT(segment);

    if (conn->InitialReceiveSequence < segment->Sequence && 
        segment->Sequence <= (conn->LastGoodSegment + (_maxUnacknowledged * 2)))
    {
        // Sequence number is acceptable
    }
    else
    {
        // This segment has unexpected data; try to establish the connection
        DEBUGLOG(L"Sequence unacceptable (%u < %u <= %u)\n", conn->InitialReceiveSequence, segment->Sequence, (conn->LastGoodSegment + (_maxUnacknowledged * 2)));

        auto reply = std::make_shared<PeerToPeerDataSegment>();

        reply->IsSynchronize                = true;
        reply->Source                       = _id;
        reply->Destination                  = conn->Id;
        reply->Sequence                     = conn->NextSendSequence;
        reply->Acknowledge                  = conn->LastGoodSegment;
        reply->IsAcknowledge                = (reply->Acknowledge != 0);

        _segmentsOut.push(reply);
        return;
    }

    if (segment->IsReset)
    {
        // Connection refused by peer
        DEBUGLOG(L"Segment contains reset; connection refused.\n");

        conn->State = EConnectionState::Closed;

        _peers.erase(conn->Id);
        _outQueue.erase(conn->Id);

        // Call PeerRemoved callback
        if (_peerRemoved != nullptr)
        {
            _peerRemoved(conn->Id, _peers.size() == 0);
        }

        return;
    }

    if (segment->IsSynchronize)
    {
        DEBUGLOG(L"Reset during synchronize; connection reset.\n");

        auto reply = std::make_shared<PeerToPeerDataSegment>();

        reply->IsReset     = true;
        reply->Source      = _id;
        reply->Destination = conn->Id;
        reply->Sequence    = segment->Acknowledge + 1;

        _segmentsOut.push(reply);

        // Connection reset
        conn->State = EConnectionState::Closed;

        _peers.erase(conn->Id);
        _outQueue.erase(conn->Id);

        // Call PeerRemoved callback
        if (_peerRemoved != nullptr)
        {
            _peerRemoved(conn->Id, _peers.size() == 0);
        }

        return;
    }

    if (segment->IsAcknowledge)
    {
        if (segment->Acknowledge == conn->InitialSendSequence)
        {
            DEBUGLOG(L"SYN ack'd, connection now open.\n");
            conn->State = EConnectionState::Open;

            for (size_t i = 0; i < _pendingAdds.size(); i++)
            {
                if (_pendingAdds[i] == conn->Id)
                {
                    if (_peerAdded != nullptr)
                    {
                        _peerAdded(conn->Id);
                    }

                    _pendingAdds.erase(_pendingAdds.begin() + i);
                    break;
                }
            }
        }
        else
        {
            DEBUGLOG(L"SYN ACK mistmatch.  %u != %u\n", segment->Acknowledge, conn->InitialSendSequence);

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->IsReset     = true;
            reply->Source      = _id;
            reply->Destination = conn->Id;
            reply->Sequence    = segment->Acknowledge + 1;

            _segmentsOut.push(reply);

            return;
        }
    }
    else
    {
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  ProcessOpenStateSegment
//
void
PeerToPeerNetworkManager::ProcessOpenStateSegment(
    PConnection  conn, 
    PDataSegment segment
    )
{
    DEBUGLOG(L"PeerToPeerNetworkManager::ProcessOpenStateSegment()\n");
    DUMP_CONNECTION(conn);
    DUMP_SEGMENT(segment);

    if (conn->LastGoodSegment < segment->Sequence && 
        segment->Sequence <= conn->LastGoodSegment + (_maxUnacknowledged * 2))
    {
        // Sequence number is acceptable
        DEBUGLOG(L"Sequence number accepted.\n");
    }
    else
    {
        DEBUGLOG(L"Sequence number invalid, resend last ACK.\n");

        auto reply = std::make_shared<PeerToPeerDataSegment>();

        reply->Source        = _id;
        reply->Destination   = conn->Id;
        reply->Sequence      = conn->NextSendSequence;
        reply->Acknowledge   = conn->LastGoodSegment;
        reply->IsAcknowledge = true;

        _segmentsOut.push(reply);

        return;
    }

    // A valid packet has been received for this connection so reset the KeepAlive timer
    conn->KeepAliveTimeout = 0.0f;

    if (segment->IsReset)
    {
        // Connection Reset
        DEBUGLOG(L"Segment contains reset.\n");

        conn->State = EConnectionState::CloseWait;
        return;
    }

    if (segment->IsNull)
    {
        DEBUGLOG(L"Segment contains null.\n");

        conn->LastGoodSegment = segment->Sequence;

        auto reply = std::make_shared<PeerToPeerDataSegment>();

        reply->Source        = _id;
        reply->Destination   = conn->Id;
        reply->Sequence      = conn->NextSendSequence;
        reply->Acknowledge   = conn->LastGoodSegment;
        reply->IsAcknowledge = true;

        _segmentsOut.push(reply);
        return;
    }

    if (segment->IsSynchronize)
    {
        DEBUGLOG(L"Segment contains synchronize.\n");

        auto reply = std::make_shared<PeerToPeerDataSegment>();

        reply->IsReset     = true;
        reply->Source      = _id;
        reply->Destination = conn->Id;
        reply->Sequence    = segment->Acknowledge + 1;

        _segmentsOut.push(reply);

        // Connection reset
        conn->State = EConnectionState::Closed;

        _peers.erase(conn->Id);
        _outQueue.erase(conn->Id);

        // Call PeerRemoved callback
        if (_peerRemoved != nullptr)
        {
            _peerRemoved(conn->Id, _peers.size() == 0);
        }
        return;
    }

    if (segment->IsAcknowledge)
    {
        DEBUGLOG(L"Segment contains acknowledge.\n");

        if (conn->OldestUnacknowledged <= segment->Acknowledge &&
            segment->Acknowledge < conn->NextSendSequence)
        {
            conn->OldestUnacknowledged = segment->Acknowledge + 1;
        }
    }

    // We only get here if the segment is a valid ACK segment so we can also process any data
    if (segment->DataSegment != nullptr && segment->DataSegmentLength > 0)
    {
        DEBUGLOG(L"Segment has user data of length %d.\n", segment->DataSegmentLength);

        if (segment->Sequence == conn->LastGoodSegment + 1)
        {
            DEBUGLOG(L"Segment received in sequence.\n");

            // Segment was in sequence so process accordingly
            if (segment->DataSegment != nullptr && segment->DataSegmentLength > 0)
            {
                if (_dataReceived != nullptr)
                {
                    _dataReceived(conn->Id, segment->DataSegment, segment->DataSegmentLength);
                }
            }

            conn->LastGoodSegment = segment->Sequence;

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->Source        = _id;
            reply->Destination   = conn->Id;
            reply->Sequence      = conn->NextSendSequence;
            reply->Acknowledge   = conn->LastGoodSegment;
            reply->IsAcknowledge = (reply->Acknowledge != 0);

            _segmentsOut.push(reply);
        }
        else
        {
            DEBUGLOG(L"Segment received out of sequence.\n");

            // Segment is out of order so we need to check the user setting for delivery
            if (!_sequencedDelivery)
            {
                conn->LastGoodSegment = segment->Sequence;

                if (segment->DataSegment != nullptr && segment->DataSegmentLength > 0)
                {
                    if (_dataReceived != nullptr)
                    {
                        _dataReceived(conn->Id, segment->DataSegment, segment->DataSegmentLength);
                    }
                }
            }
            else
            {
                DEBUGLOG(L"Dropping out of sequence message.\n");
                // Drop out of order segments
                return;
            }

            auto reply = std::make_shared<PeerToPeerDataSegment>();

            reply->Source        = _id;
            reply->Destination   = conn->Id;
            reply->Sequence      = conn->NextSendSequence;
            reply->Acknowledge   = conn->LastGoodSegment;
            reply->IsAcknowledge = (reply->Acknowledge != 0);

            _segmentsOut.push(reply);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  MarkAcknowledgedSegments
//
void 
PeerToPeerNetworkManager::MarkAcknowledgedSegments()
{
    for (auto segment : _retries)
    {
        auto peer = _peers.find(segment->Destination);
        if (peer != _peers.end())
        {
            auto conn = (*peer).second;

            if (conn != nullptr && segment->Sequence <= conn->OldestUnacknowledged)
            {
                DEBUGLOG(L"Peer %u ack'd sequence %u\n", conn->Id, segment->Sequence);
                segment->Acknowledged = true;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SendDataToPeer
//
void
PeerToPeerNetworkManager::SendDataToPeer(
    TPeerId peerId, 
    TByteArray data,
    uint32_t dataLen
    )
{
    std::lock_guard<std::mutex> lock(_lock);

    if (_running)
    {
        auto search = _peers.find(peerId);
        if (search != _peers.end())
        {
            auto conn = (*search).second;

            if (dataLen > _maxSegmentSize)
            {
                throw std::runtime_error("Segment size exceeds maximum size specified by peer");
            }

            auto segment = std::make_shared<PeerToPeerDataSegment>();

            segment->Source            = _id;
            segment->Destination       = conn->Id;

            if (data != nullptr)
            {
                segment->DataLength = dataLen;
                segment->DataSegmentLength = dataLen;
                segment->DataSegment = new uint8_t[dataLen];
                CopyMemory(segment->DataSegment, data, dataLen);
            }

            if (conn->NextSendSequence < conn->OldestUnacknowledged + _maxUnacknowledged)
            {
                // Still room in the send window so queue for sending
                segment->Sequence      = conn->NextSendSequence;
                segment->Acknowledge   = conn->LastGoodSegment;
                segment->IsAcknowledge = (segment->Acknowledge != 0);

                _segmentsOut.push(segment);

                conn->NextSendSequence += 1;
            }
            else
            {
                // Too many un-ack'd segments so put it in a send queue
                DEBUGLOG(L"Segment is out of the window; queued.\n");

                _outQueue[peerId].push(segment);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SendDataToNetwork
//
void 
PeerToPeerNetworkManager::SendDataToNetwork(
    TByteArray data,
    uint32_t dataLen
    )
{
    if (_running)
    {
        for (const auto& pair : _peers)
        {
            if (pair.second->State == EConnectionState::Open)
            {
                SendDataToPeer(pair.second->Id, data, dataLen);
            }
        }
    }
}

bool
PeerToPeerNetworkManager::ResolveAddressFromHost(
    std::string_view addr
    )
{
    ADDRINFO hints, *result;

    DEBUGLOG(L"PeerToPeerNetworkManager::ResolveAddressFromHost: %S\n", addr.data());

    ZeroMemory(&hints, sizeof(hints));
    ZeroMemory(&_tempAddr, sizeof(_tempAddr));

    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_ALL;

    std::string address(addr.begin(), addr.end());

    auto errcode = getaddrinfo(address.c_str(), NULL, &hints, &result);
    if (errcode != 0)
    {
        DEBUGLOG(L"getaddrinfo failed with error: %d\n", errcode);
        return false;
    }

    bool found = false;
    auto orig = result;

    while (result)
    {
        if (result->ai_family == AF_INET)
        {
            // Use memcpy since we're going to free the res variable later
            CopyMemory(&_tempAddr, result->ai_addr, result->ai_addrlen);
            found = true;
            break;
        }

        result = result->ai_next;
    }

    freeaddrinfo(orig);

    return found;
}

uint32_t PeerToPeerNetworkManager::GenerateLocalPeerId()
{
    if (GetLocalNetworkAddress())
    {
        return _tempAddr.sin_addr.S_un.S_addr;
    }
    return 0xFFFFFFFF;
}

std::string
PeerToPeerNetworkManager::GetRemoteNetworkAddress()
{
    //auto result = ResolveAddressFromHost(L"checkip.dyndns.org");

    //if (result == false)
    //{
    //    return std::wstring();
    //}

    //_tempAddr.sin_port = htons(80);

    //auto sock = socket(
    //    AF_INET,
    //    SOCK_STREAM,
    //    IPPROTO_TCP
    //    );

    //auto err = connect(
    //    sock,
    //    (PSOCKADDR) &_tempAddr,
    //    sizeof(_tempAddr)
    //    );

    //if ( err == SOCKET_ERROR)
    //{
    //    closesocket(sock);
    //    return std::wstring();
    //}

    //std::string request = "GET / HTTP/1.1\r\nHost: checkip.dyndns.org\r\nConnection: close\r\n\r\n";

    //err = send(
    //    sock,
    //    request.c_str(),
    //    static_cast<int>(request.length() + 1),
    //    0
    //    );

    //if (err == SOCKET_ERROR)
    //{
    //    closesocket(sock);
    //    return std::wstring();
    //}

    //char inBuf[1024], outBuf[1024], *p;
    //int i, j;

    //while ((i = recv(sock, inBuf, sizeof inBuf - 1, 0)) > 0 && i != SOCKET_ERROR)
    //{
    //    if (p = strstr(inBuf, "Current IP Address: "))
    //    {
    //        p += strlen("Current IP Address: ");

    //        for (j = 0; (isdigit(*p) || *p == '.') && j < 1024; p++, j++)
    //        {
    //            outBuf[j] = *p;
    //        }

    //        outBuf[j] = '\0';
    //    }
    //}

    //closesocket(sock);

    //std::string str = outBuf;
    //std::wstring wstr(str.begin(), str.end());
    std::string str;

    if (GetLocalNetworkAddress())
    {
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(_tempAddr.sin_addr), ipAddress, INET_ADDRSTRLEN);
        str = std::string{ ipAddress, ipAddress + INET_ADDRSTRLEN };
    }

    return str;
}

bool
PeerToPeerNetworkManager::GetLocalNetworkAddress()
{
    char host[255] = {};
    auto err = gethostname(host, 255);

    if (err == SOCKET_ERROR)
    {
        return false;
    }

    auto result = ResolveAddressFromHost(host);

    return result;
}

}
