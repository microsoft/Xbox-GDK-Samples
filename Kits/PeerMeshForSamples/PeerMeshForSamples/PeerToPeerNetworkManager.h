//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "PeerNetworkTypes.h"
#include "PeerToPeerConnection.h"
#include "PeerToPeerDataSegment.h"

namespace PeerMeshForSamples
{

void SetNetworkLogPath(const std::wstring& strPath);
const std::wstring& GetNetworkLogPath();

const uint32_t DEFAULT_MAX_SEGMENT_SIZE      = 512;
const uint32_t DEFAULT_MAX_UNACKNOWLEDGED    = 10;
const uint32_t DEFAULT_PACKET_RETRY_COUNT    = 5;
const float_t  DEFAULT_PACKET_RETRY_INTERVAL = 5.0f;
const float_t  DEFAULT_CLOSEWAIT_TIMEOUT     = 5.0f;
const float_t  DEFAULT_KEEPALIVE_INTERVAL    = 5.0f;

///////////////////////////////////////////////////////////////////////////////
//  PeerToPeerNetworkManager
//

class PeerToPeerNetworkManager final
{
public:
    PeerToPeerNetworkManager(
        bool sequencedDelivery     = false,
        uint32_t maxSegmentSize    = DEFAULT_MAX_SEGMENT_SIZE,
        uint32_t maxUnacknowledged = DEFAULT_MAX_UNACKNOWLEDGED
        );

    ~PeerToPeerNetworkManager();

    void CreateNetwork(TPeerId hostId, const TByteArray hostAddress, uint32_t addrSize, uint16_t port);
    void CreateNetwork(TPeerId hostId, std::string_view hostAddress, uint16_t port);
    void AddPeerToNetwork(TPeerId peerId, const TByteArray peerAddress, uint32_t addrSize, uint16_t port);
    void AddPeerToNetwork(TPeerId peerId, std::string_view peerAddress, uint16_t port);
    TPeerId AddPeerToNetwork(std::string_view peerAddress, uint16_t port);
    void RemovePeerFromNetwork(TPeerId peer);
    void DestroyNetwork(void);
    void DoWork(float_t delta);
    void SendDataToNetwork(const TByteArray data, uint32_t size);
    void SendDataToPeer(TPeerId peerId, const TByteArray data, uint32_t size);

    uint32_t        GenerateLocalPeerId();
    std::string     GetRemoteNetworkAddress();

    inline bool     IsRunning()                                { return _running; }
    inline bool     GetSequencedDelivery()                     { return _sequencedDelivery; }
    inline uint32_t GetMaxSegmentSize()                        { return _maxSegmentSize; }
    inline uint32_t GetMaxUnacknowledged()                     { return _maxUnacknowledged; }
    inline float_t  GetSegmentRetryInterval()                  { return _retryInterval; }
    inline float_t  GetCloseWaitTimeout()                      { return _closewaitTimeout; }
    inline float_t  GetKeepAliveInterval()                     { return _keepAliveInterval; }
    inline void     SetSegmentRetryInterval(float_t value)     { _retryInterval = value; }
    inline void     SetCloseWaitTimeout(float_t value)         { _closewaitTimeout = value; }
    inline void     SetKeepAliveInterval(float_t value)        { _keepAliveInterval = value; }
    inline void     SetDataReceivedHandler(FDataReceived func) { _dataReceived = func; }
    inline void     SetPeerAddedHandler(FPeerAdded func)       { _peerAdded = func; }
    inline void     SetPeerRemovedHandler(FPeerRemoved func)   { _peerRemoved = func; }

private:
    void SendSegment(PDataSegment segment);
    void ReadSegmentsFromSocket();
    void ProcessSegments();
    void ProcessQueuedSends();
    void ProcessRetries(float_t delta);
    void ProcessConnectionTimers(float_t delta);
    void SendSegments();
    void ProcessSegment(PDataSegment segment);
    void ProcessClosedStateSegment(PConnection conn, PDataSegment segment);
    void ProcessSynchronizeSentStateSegment(PConnection conn, PDataSegment segment);
    void ProcessSynchronizeReceivedStateSegment(PConnection conn, PDataSegment segment);
    void ProcessOpenStateSegment(PConnection conn, PDataSegment segment);
    void MarkAcknowledgedSegments();
    bool ResolveAddressFromHost(std::string_view addr);
    bool GetLocalNetworkAddress();

    SOCKET                            _socket;
    SOCKADDR_IN                       _address;
    SOCKADDR_IN                       _tempAddr;
    TPeerId                           _id;
    bool                              _running;
    std::mutex                        _lock;
    FDataReceived                     _dataReceived;
    FPeerAdded                        _peerAdded;
    FPeerRemoved                      _peerRemoved;
    bool                              _sequencedDelivery; // If true packets are only accepted in sequence order
    uint32_t                          _maxSegmentSize;    // Max size of the a data segment
    uint32_t                          _maxUnacknowledged; // Max number of datagrams to send before throttling
    uint32_t                          _retryCount;        // Number of retries allowed before peer is considered closed
    float_t                           _retryInterval;     // Time in seconds between datagram resends
    float_t                           _closewaitTimeout;  // Time to wait between peer close request and peer removal
    float_t                           _keepAliveInterval; // Amount of time in seconds to wait between segments before sending NUL
    TPeerConnectionMap                _peers;             // Known peer connections
    TPeerDataQueue                    _outQueue;          // Per peer data segments that are pending send
    TSegmentQueue                     _segmentsIn;        // Data segments received but not processed
    TSegmentQueue                     _segmentsOut;       // Data segments ready to be sent to the network
    TSegmentList                      _retries;           // Data segments sent that are un-ack'd or failed
    TPeerQueue                        _pendingAdds;       // Peers waiting to complete connection
};

void SetNetworkLogPath(const std::wstring& strPath);
const std::wstring& GetNetworkLogPath();

}
