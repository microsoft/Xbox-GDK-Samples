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
//  The segment format is based on the RDP protocol:
//    http://tools.ietf.org/html/rfc1151
//    http://tools.ietf.org/html/rfc908
//
//        0             0 0   1         1
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//        +-+-+-+-+-+-+---+---------------+
//      0 |     Flags     |    Version    |
//        +-+-+-+-+-+-+---+---------------+
//      1 |         Source Port           |
//        +---------------+---------------+
//      2 |       Destination Port        |
//        +---------------+---------------+
//      3 |          Data  Length         |
//        +---------------+---------------+
//      4 |                               |
//        +---    Sequence Number      ---+
//      5 |                               |
//        +---------------+---------------+
//      6 |                               |
//        +--- Acknowledgement Number  ---+
//      7 |                               |
//        +---------------+---------------+
//      8 |           Checksum            |
//        +---------------+---------------+

const uint8_t RDP_PROTOCOL_VERSION = 2;
const uint8_t RDP_HEADER_LENGTH = 22;

PeerToPeerDataSegment::PeerToPeerDataSegment()
{
    DataSegment = nullptr;
    SourceAddress = nullptr;
    DataSegmentLength = 0;
    SourceAddressLength = 0;
    Fields = 0;
    Version = RDP_PROTOCOL_VERSION;
    IsAcknowledge = false;
    IsReset = false;
    IsNull = false;
    IsSynchronize = false;
    Source = 0;
    Destination = 0;
    DataLength = 0;
    Sequence = 0;
    Acknowledge = 0;
    Checksum = 0;
    RetryCount = 0;
    Acknowledged = false;
}

PeerToPeerDataSegment::~PeerToPeerDataSegment()
{
    if (DataSegment != nullptr)
    {
        delete[] DataSegment;
    }

    if (SourceAddress != nullptr)
    {
        delete[] SourceAddress;
    }
}

PeerToPeerDataSegment::PeerToPeerDataSegment(
    TByteArray address,
    uint32_t addrSize,
    TByteArray data,
    uint32_t dataSize
    )
{
    if (address == nullptr || addrSize == 0)
    {
        throw std::invalid_argument("Null or empty address specified");
    }

    DataSegment = nullptr;
    SourceAddressLength = addrSize;
    SourceAddress = new uint8_t[SourceAddressLength];

    CopyMemory(SourceAddress, address, SourceAddressLength);

    if (data != nullptr && dataSize > 0)
    {
        if (dataSize < RDP_HEADER_LENGTH)
        {
            throw std::invalid_argument("Invalid packet length");
        }

        uint32_t offset = 0;

        Fields      = *((uint8_t *)(data + offset));  offset += sizeof(Fields);      // One byte for the fields
        Version     = *((uint8_t *)(data + offset));  offset += sizeof(Version);     // One byte for the header length
        Source      = *((TPeerId *)(data + offset));  offset += sizeof(Source);      // Two bytes for source peer id
        Destination = *((TPeerId *)(data + offset));  offset += sizeof(Destination); // Two bytes for destination peer id
        DataLength  = *((uint16_t *)(data + offset)); offset += sizeof(DataLength);  // Two bytes for user data segment length
        Sequence    = *((uint32_t *)(data + offset)); offset += sizeof(Sequence);    // Four bytes for the sequence number
        Acknowledge = *((uint32_t *)(data + offset)); offset += sizeof(Acknowledge); // Four bytes for the ack number
        Checksum    = *((uint16_t *)(data + offset)); offset += sizeof(Checksum);    // Two bytes for the offset

        // Validate checksum
        if (CalculateHeaderChecksum((uint16_t *)data, 10) != Checksum)
        {
            throw std::runtime_error("Checksum mismatch");
        }

        // Get the user data if any
        if (DataLength > 0)
        {
            if (offset + DataLength != dataSize)
            {
                throw std::runtime_error("Invalid packet length");
            }

            DataSegmentLength = DataLength;
            DataSegment = new uint8_t[DataSegmentLength];

            CopyMemory(DataSegment, data + offset, DataLength);
        }

        IsAcknowledge = (Fields & (uint8_t)SegmentHeaderFields::Acknowledge) ? true : false;
        IsNull        = (Fields & (uint8_t)SegmentHeaderFields::Null) ? true : false;
        IsReset       = (Fields & (uint8_t)SegmentHeaderFields::Reset) ? true : false;
        IsSynchronize = (Fields & (uint8_t)SegmentHeaderFields::Synchronize) ? true : false;
        Acknowledge   = IsSynchronize || IsAcknowledge ? Acknowledge : 0;
    }
}

void
PeerToPeerDataSegment::Serialize(
    TByteArray *buffer,
    uint32_t *size
    )
{
    // Discover the entire length of the segment
    uint32_t segmentLength = RDP_HEADER_LENGTH;

    if (DataSegment != nullptr && DataSegmentLength > 0)
    {
        segmentLength += DataSegmentLength;
        DataLength = DataSegmentLength;
    }

    // Create a buffer to hold the segment
    auto segment = new uint8_t[segmentLength];

    // Fill in out params
    if (size != nullptr)
    {
        *size = segmentLength;
    }

    if (buffer != nullptr)
    {
        *buffer = segment;
    }

    // Fill in the header
    uint32_t offset = 0;

    Fields |= IsAcknowledge ? (uint8_t)SegmentHeaderFields::Acknowledge : (uint8_t)SegmentHeaderFields::None;
    Fields |= IsSynchronize ? (uint8_t)SegmentHeaderFields::Synchronize : (uint8_t)SegmentHeaderFields::None;
    Fields |= IsReset ? (uint8_t)SegmentHeaderFields::Reset : (uint8_t)SegmentHeaderFields::None;
    Fields |= IsNull ? (uint8_t)SegmentHeaderFields::Null : (uint8_t)SegmentHeaderFields::None;

    *((uint8_t *)(segment + offset))  = Fields;               offset += sizeof(Fields);     // One byte for the fields
    *((uint8_t *)(segment + offset)) = RDP_PROTOCOL_VERSION;  offset += sizeof(Version);     // One byte for the protocol version
    *((TPeerId *)(segment + offset)) = Source;                offset += sizeof(Source);      // Two bytes for source peer id
    *((TPeerId *)(segment + offset)) = Destination;           offset += sizeof(Destination); // Two bytes for destination peer id
    *((uint16_t *)(segment + offset)) = DataLength;           offset += sizeof(DataLength);  // Two bytes for user data segment length
    *((uint32_t *)(segment + offset)) = Sequence;             offset += sizeof(Sequence);    // Four bytes for the sequence number
    *((uint32_t *)(segment + offset)) = Acknowledge;          offset += sizeof(Acknowledge); // Four bytes for the ack number
    *((uint16_t *)(segment + offset)) = 0;                    offset += sizeof(Checksum);    // Two bytes for the checksum, 0 prior to calculation

    // Calculate header checksum
    Checksum = CalculateHeaderChecksum((uint16_t *)segment, 10);
    *((uint16_t *)(segment + 20)) = Checksum;

    // Add user data
    if (DataSegment != nullptr && DataSegmentLength > 0)
    {
        CopyMemory(segment + offset, DataSegment, DataSegmentLength);
    }
}

uint16_t
PeerToPeerDataSegment::CalculateHeaderChecksum(
    uint16_t *buffer,
    int size
)
{
    uint32_t checksum = 0;

    while (size > 1)
    {
        checksum += *buffer++;
        size -= sizeof(uint16_t);
    }

    if (size)
    {
        checksum += *(uint8_t *)buffer;
    }

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);

    return (uint16_t)(~checksum);
}
}
