//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace PeerMeshForSamples
{
enum class SegmentHeaderFields : uint8_t
{
    None        = 0x00,
    Synchronize = 0x01,
    Acknowledge = 0x02,
    Reset       = 0x04,
    Null        = 0x08,
    Version     = 0xc0
};

///////////////////////////////////////////////////////////////////////////////
//  PeerToPeerDataSegment
//

class PeerToPeerDataSegment
{
public:

    PeerToPeerDataSegment();

    PeerToPeerDataSegment(
        TByteArray address,
        uint32_t addrSize,
        TByteArray data,
        uint32_t datasize
        );

    ~PeerToPeerDataSegment();

    uint16_t CalculateHeaderChecksum(
        uint16_t *buffer,
        int size
        );

    void Serialize(
        TByteArray *buffer,
        uint32_t *size
        );

public:
    bool       IsSynchronize;
    bool       IsReset;
    bool       IsNull;
    bool       IsAcknowledge;
    uint8_t    Version;
    uint8_t    Fields;
    TPeerId    Source;
    TPeerId    Destination;
    uint32_t   Sequence;
    uint32_t   Acknowledge;
    uint16_t   Checksum;
    uint16_t   DataLength;
    uint32_t   RetryCount;
    float_t    Timestamp;
    bool       Acknowledged;
    TByteArray DataSegment;
    uint32_t   DataSegmentLength;
    TByteArray SourceAddress;
    uint32_t   SourceAddressLength;
};
}
