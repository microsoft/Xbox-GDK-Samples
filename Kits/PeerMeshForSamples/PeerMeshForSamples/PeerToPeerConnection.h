//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace PeerMeshForSamples
{
///////////////////////////////////////////////////////////////////////////////
//  PeerToPeerConnectionState
//

enum class EConnectionState
{
    Open,
    Closed,
    SynchronizeSent,
    SynchronizeReceived,
    CloseWait
};

///////////////////////////////////////////////////////////////////////////////
//  PeerToPeerConnection
//

class PeerToPeerConnection
{
public:
    PeerToPeerConnection(
        TPeerId peerId,
        TByteArray peerAddress,
        uint32_t addrSize
        )
    {
        Id                      = peerId;
        AddressLength           = addrSize;
        State                   = EConnectionState::Closed;
        NextSendSequence        = 0;
        OldestUnacknowledged    = 0;
        InitialSendSequence     = 0;
        LastGoodSegment         = 0;
        InitialReceiveSequence  = 0;
        CloseWaitTimeout        = 0.0f;
        KeepAliveTimeout        = 0.0f;

        Address = new uint8_t[AddressLength];
        CopyMemory(Address, peerAddress, AddressLength);
    }

    ~PeerToPeerConnection()
    {
        if (Address != nullptr)
        {
            delete[] Address;
        }
    }

    TPeerId  Id;
    uint32_t NextSendSequence;
    uint32_t OldestUnacknowledged;
    uint32_t InitialSendSequence;
    uint32_t LastGoodSegment;
    uint32_t InitialReceiveSequence;
    float_t  CloseWaitTimeout;
    float_t  KeepAliveTimeout;

    TByteArray Address;
    uint32_t   AddressLength;

    EConnectionState State;
};
}
