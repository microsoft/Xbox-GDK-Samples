//--------------------------------------------------------------------------------------
// WriteOwnBundlesHelper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This file has APIs which write GPU packet data into user supplied buffer address.
// All the packet data written to the buffers in the APIs below are taken from 
// d3d12_x.h.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace WriteOwnBundle
{
    // User Data register Command Processor Op Codes 
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_1_DWORD          0xDD // Fast common case versions that only set VS+PS
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_2_DWORD          0xDE //
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_4_DWORD          0xDF //
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA                  0xE0 //
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_1_DWORD_GSHS     0xE1 // 'Broadcast' versions that set VS+GS+HS+DS+PS
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_2_DWORD_GSHS     0xE2 //
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_4_DWORD_GSHS     0xE3 //
    #define D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_GSHS             0xE4 //
    #define D3D12XBOX_CP_PACKET_TYPE_MASK                         0xc0000000
    #define D3D12XBOX_CP_OP_CODE_SHIFT                            8
    #define D3D12XBOX_CP_OP_CODE_MASK                             0xff

    static UINT32 GetPacketOpCode(UINT Header)
    {
        return (Header >> D3D12XBOX_CP_OP_CODE_SHIFT) & D3D12XBOX_CP_OP_CODE_MASK;
    }

    // Type 0 Packets are handled by the Constant Engine (CE)
    static bool IsType0Packet(UINT32 Header)
    {
        return (Header & D3D12XBOX_CP_PACKET_TYPE_MASK) == 0;
    }

    // The base packet for each Root Argument is encoded during Root Signature creation time; however, it's not until Commandlist record time
    // that the size and offset is known for each set therefore we need to adjust the base packet with this information. Prior to the 2303 GDK
    // the Command Processor (CP) was tasked with handling the offset and size calculation but for better system throughput its more efficient to
    // do this on the CPU side. This helper function is adapted from a function by the same name that exists in d3d12_xs.h.
    static UINT32 AdjustRootDataHeader(bool Gfx, bool PsoUsesGSorHS, UINT32 Header, UINT32 NumDwords, UINT32 OffsetDwords)
    {
        if (IsType0Packet(Header) == false) // is this a user register packet?
        {
            if (Gfx) // For Graphics adjust the opcode to pick the most efficient version
            {
                switch (NumDwords)
                {
                case 1:     break; // nothing to do
                case 2:     Header += 0x100; break; // Convert _GRAPHICS_ROOT_DATA_1 to _GRAPHICS_ROOT_DATA_2
                case 4:     Header += 0x200; break; // etc. 
                default:    Header += 0x300; break;
                }

                // Root User Data packets are defined with the assumption that regular hardware Vs+PS will be used (common case); however, sometimes
                // API Vertex Shaders are converted to the Hardware Geometry Shader stage (NGG is an example) so we must account for that. The same
                // applies for other uncommon pipeline configurations such as Tessellation.
                if (PsoUsesGSorHS && (GetPacketOpCode(Header) < D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_1_DWORD_GSHS))
                {
                    Header += 0x400; // Covert to broadcast packet e.g. D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_1_DWORD -> D3D12XBOX_IT1_SET_GRAPHICS_ROOT_DATA_1_DWORD_GSHS
                }
            }

            Header += (OffsetDwords << 2); // Low 2 bits unused
        }
        else // CERAM / shmem write
        {
            Header += (OffsetDwords * sizeof(UINT32)); // CERAM works in bytes
        }

        return Header;
    }

    inline void IASetVertexBuffersBYOB(UINT32** writeAddress,
        _In_range_(0, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot,
        _In_range_(1, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) UINT NumViews,
        _In_reads_opt_(NumViews) const D3D12_VERTEX_BUFFER_VIEW* pViews)
    {
        UINT packetHeader = D3D12XBOX_PACKET_SET_VERTEX_BUFFERS
            + StartSlot
            + (NumViews << (D3D12XBOX_PACKET_COUNT_SHIFT + 1));

        **writeAddress = packetHeader;
        *writeAddress += 1;

        do {
            if (pViews) {
                uint64_t descriptor = uint64_t(pViews->BufferLocation)
                    + (uint64_t(pViews->StrideInBytes) << D3D12XBOX_SET_VERTEX_BUFFERS_STRIDE_SHIFT64);

                *(reinterpret_cast<uint64_t*>(*writeAddress)) = descriptor;
                *writeAddress += 2;
                pViews++;
            }
        } while (--NumViews != 0);
    }

    inline void IASetIndexBufferBYOB(UINT32** writeAddress,
        _In_opt_ const D3D12_INDEX_BUFFER_VIEW* pDesc)
    {
        if (pDesc) {
            **writeAddress     = D3D12XBOX_PACKET_SET_INDEX_BUFFER + pDesc->Format;
            *(reinterpret_cast<uint64_t*>(*writeAddress + 1)) = pDesc->BufferLocation;
            *writeAddress += 3;
        }
    }

    inline void IASetPrimitiveTopologyBYOB(UINT32** writeAddress,
        _In_ D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
    {
        **writeAddress = D3D12XBOX_PACKET_SET_PRIMITIVE_TOPOLOGY;
        *(*writeAddress + 1) = D3D12XBOX_SET_PRIMITIVE_TOPOLOGY_LOOKUP[PrimitiveTopology];
        *writeAddress += 2;
    }

    inline void DrawIndexedInstancedBYOB(_Inout_ UINT32** writeAddress,
        _In_ UINT IndexCountPerInstance,
        _In_ UINT InstanceCount,
        _In_ UINT StartIndexLocation,
        _In_ INT BaseVertexLocation,
        _In_ UINT StartInstanceLocation)
    {
        **writeAddress       = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
        *(*writeAddress + 1) = InstanceCount;
        *(*writeAddress + 2) = StartIndexLocation;
        *(*writeAddress + 3) = IndexCountPerInstance;
        *(*writeAddress + 4) = UINT32(BaseVertexLocation);
        *(*writeAddress + 5) = StartInstanceLocation;
        *writeAddress += 6;
    }


    inline void SetGraphicsRootDescriptorTableBYOB(
        UINT32** writeAddress,
        UINT* rootPacketHeader,
        _In_range_(0, D3D12XBOX_ROOT_PARAMETER_MAX_INDEX) UINT RootParameterIndex,
        _In_ D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
    {
        **writeAddress = AdjustRootDataHeader(true /*Gfx*/, false /*PsoUsesGSorHS*/, rootPacketHeader[RootParameterIndex], 1, 0);
        *(*writeAddress + 1) = (UINT32)BaseDescriptor.ptr;
        *writeAddress += 2;
    }

    inline void SetGraphicsRootConstantBufferViewBYOB(
        UINT32** writeAddress,
        UINT* rootPacketHeader,
        _In_range_(0, D3D12XBOX_ROOT_PARAMETER_MAX_INDEX) UINT RootParameterIndex,
        _In_ D3D12_GPU_VIRTUAL_ADDRESS BufferLocation)
    {
        *(*writeAddress)     = AdjustRootDataHeader(true /*Gfx*/, false /*PsoUsesGSorHS*/, rootPacketHeader[RootParameterIndex], 2, 0);
        *(reinterpret_cast<uint64_t*>(*writeAddress + 1)) = BufferLocation;
        *writeAddress += 3;
    }

    inline void SetPipelineStateBYOB(UINT32** writeAddress, D3D12XBOX_DESCRIPTOR_PIPELINE_STATE m_psoDescriptor)
    {
        *(D3D12XBOX_DESCRIPTOR_PIPELINE_STATE*)(*writeAddress) = m_psoDescriptor;
        *writeAddress += sizeof(D3D12XBOX_DESCRIPTOR_PIPELINE_STATE) / 4; // size in DWORDs
    }
};
