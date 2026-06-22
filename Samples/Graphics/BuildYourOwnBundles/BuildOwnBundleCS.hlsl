//--------------------------------------------------------------------------------------
// BuildOwnBundleCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Write UAV buffer with GPU packet data
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "BYOBStructs.hlsli"

struct Frustum
{
	float4 m_plane[6];
};

cbuffer CameraFrustum : register(b0)
{
	Frustum cb_camFrustum;
};

// Structures below are padded to make align it to a 16 byte boundary.
// You can pack it much tighter by not having separate structures and
// instead define it directly in the output buffer.
// The sample keeps the structs separate just for clarity.
struct PSOData
{
	PipelineStateObject psoData;
#ifdef __XBOX_SCARLETT
    uint  padding;
#else
    uint2 padding;
#endif
};

struct VertexBufferData
{
	uint  m_descriptorLo;
	uint  m_descriptorHi;
	uint  m_strideInBytes;
	uint  m_sizeInBytes;
};
struct IndexBufferData
{
	uint  m_packetHeader;
	uint  m_bufferLocationLo;
	uint  m_bufferLocationHi;
	uint  m_padding;
};
struct DescriptorTableData
{
	uint  m_rootParameterIndex;
	uint  m_gpuDescriptorHandle;
	uint  m_padding0;
	uint  m_padding1;
};
struct RootConstantBufferData
{
	uint   m_rootParameterIndex;
	uint   m_bufferLocationLo;
	uint   m_bufferLocationHi;
	uint   m_padding;
};

struct ModelData
{
	PSOData                 m_pso[NUM_PSO];
	VertexBufferData        m_vertexBufferData;
	IndexBufferData         m_indexBufferData;
	DescriptorTableData     m_descriptorTableData;
	RootConstantBufferData  m_rootConstantBufferData;
	DrawIndexedArgs         m_drawIndexedArgs;
};

cbuffer byobInputModelData : register(b1)
{
	ModelData modelData[NUM_MODELS];
};

struct ModelDataUpdatePerFrame
{
	uint   m_constantBufferLocationLo;
	uint   m_constantBufferLocationHi;
	uint   m_sizeofModelEffectConstants;
};
cbuffer byobInputModelUpdatePerFrameData : register(b2)
{
	ModelDataUpdatePerFrame modelDataUpdatePerFrame;
};

struct InstanceData
{
	float4   m_sphereCenterRadius;
	uint     m_modelID;
	uint     m_psoID;
};
StructuredBuffer<InstanceData> cb_instanceData : register(t0);
StructuredBuffer<uint> rootPacketHeaderBuffer : register(t1);

// Return true if sphere is completely outside the frustum, else return false
bool IsSphereOutsideFrustum(float4 boundingSphere)
{
	float4 boundingSphereCenter = float4(boundingSphere.xyz, 1.f);
	float boundingSphereRadius = boundingSphere.w;
	bool result = false;

	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		float dotProduct = dot(cb_camFrustum.m_plane[i], boundingSphereCenter);
		if (dotProduct < -boundingSphereRadius)
		{
			result = true;
		}
	}
	return result;
}

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

uint GetPacketOpCode(uint Header)
{
    return (Header >> D3D12XBOX_CP_OP_CODE_SHIFT) & D3D12XBOX_CP_OP_CODE_MASK;
}

// Type 0 Packets are handled by the Constant Engine (CE)
bool IsType0Packet(uint Header)
{
    return (Header & D3D12XBOX_CP_PACKET_TYPE_MASK) == 0;
}

// The base packet for each Root Argument is encoded during Root Signature creation time; however, it's not until Commandlist record time
// that the size and offset is known for each set therefore we need to adjust the base packet with this information. Prior to the 2303 GDK
// the Command Processor (CP) was tasked with handling the offset and size calculation but for better system throughput its more efficient to
// do this on the CPU side. This helper function is adapted from a function by the same name that exists in d3d12_xs.h.
uint AdjustRootDataHeader(bool Gfx, bool PsoUsesGSorHS, uint Header, uint NumDwords, uint OffsetDwords)
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
        Header += (OffsetDwords * 4); // CERAM works in bytes
    }

    return Header;
}

// Change the SIZEOF_GPU_APPEND_BUFFER_STRUCT define when modifying this struct
struct GPUAppendBufferOutput
{
	// Include some info in the GPU buffer.
	// This is just to show how you can pass some data into the buffer
	// without actually executing it on the GPU. ModelID and PSOID will 
	// be ignored by the GPU when executing the commands in the buffer
	uint  m_nopPacket;
	uint  m_modelID;
	uint  m_psoID;

	// PSO
	PipelineStateObject m_pso;

	// Index buffer
	uint  m_indexBufferPacketHeader;
	uint2 m_indexBufferLocation;

	// Vertex buffer
	uint  m_vertexBufferPacketHeader;
	uint2 m_vertexBufferDescriptor;

	// Root Descriptor Table
	uint  m_rootDescTableRootPacketHeaderData;
	uint  m_rootDescTableDescriptorHandle;

	// Root Constant Buffer
	uint  m_rootConstBufferRootPacketHeaderData;
	uint2 m_rootConstBufferLocation;

	// Draw Indexed Instanced
	uint m_drawIndexedInstancedHeader;
	uint m_instanceCount;
	uint m_startIndexLocation;
	uint m_indexCountPerInstance;
	int  m_baseVertexLocation;
	uint m_startInstanceLocation;
};

// Append buffer which actually stores the execute command packets
AppendStructuredBuffer<GPUAppendBufferOutput> bufBYOBOut : register(u0);

// .x stores the counter for number of values added to the Append buffer above
// .y is used to store the actual number of bytes present in the Append buffer 
// that the GPU should read from the buffer being executed in ExecuteIndirectBundleX. 
RWStructuredBuffer<uint2> bufCountBYOB : register(u1);

[ROOT_SIGNATURE_MESH]
[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
#ifdef CULL_MESHES
	if (!IsSphereOutsideFrustum(cb_instanceData[DTid.x].m_sphereCenterRadius))
#endif
	{
		uint currModelID = cb_instanceData[DTid.x].m_modelID;
		uint currPSOID = cb_instanceData[DTid.x].m_psoID;
		GPUAppendBufferOutput outBuffer;

		uint nopHeaderSizeBytes = 4;
		uint packetContentSizeBytes = 4 + 4; // modelID and psoID
		uint nopSizeBytes = nopHeaderSizeBytes + packetContentSizeBytes;
		outBuffer.m_nopPacket = D3D12XBOX_PACKET_NOP_WITH_COUNT + (nopSizeBytes << (D3D12XBOX_PACKET_COUNT_SHIFT - 2));
		outBuffer.m_modelID = currModelID;
		outBuffer.m_psoID = currPSOID;

		// PSO
		outBuffer.m_pso.data0 = modelData[currModelID].m_pso[currPSOID].psoData.data0;
		outBuffer.m_pso.data1 = modelData[currModelID].m_pso[currPSOID].psoData.data1;
		outBuffer.m_pso.data2 = modelData[currModelID].m_pso[currPSOID].psoData.data2;
		outBuffer.m_pso.data3 = modelData[currModelID].m_pso[currPSOID].psoData.data3;
		outBuffer.m_pso.data4 = modelData[currModelID].m_pso[currPSOID].psoData.data4;
		outBuffer.m_pso.data5 = modelData[currModelID].m_pso[currPSOID].psoData.data5;
#ifdef __XBOX_SCARLETT
        outBuffer.m_pso.data6 = modelData[currModelID].m_pso[currPSOID].psoData.data6;
#endif

		// Index Buffer
		outBuffer.m_indexBufferPacketHeader = modelData[currModelID].m_indexBufferData.m_packetHeader;
		outBuffer.m_indexBufferLocation.x   = modelData[currModelID].m_indexBufferData.m_bufferLocationLo;
		outBuffer.m_indexBufferLocation.y   = modelData[currModelID].m_indexBufferData.m_bufferLocationHi;

		// Vertex Buffer
		// Hard coding the below values for vertex buffer as the model currently being used has only 1 vertex buffer
		uint startSlot = 0;
		uint numViews = 1;
		outBuffer.m_vertexBufferPacketHeader = D3D12XBOX_PACKET_SET_VERTEX_BUFFERS + startSlot + (numViews << (D3D12XBOX_PACKET_COUNT_SHIFT + 1));
		outBuffer.m_vertexBufferDescriptor.x = modelData[currModelID].m_vertexBufferData.m_descriptorLo;
		outBuffer.m_vertexBufferDescriptor.y = modelData[currModelID].m_vertexBufferData.m_descriptorHi;

		// SRV
		uint descTableRootParameterIndex = modelData[currModelID].m_descriptorTableData.m_rootParameterIndex;
		outBuffer.m_rootDescTableRootPacketHeaderData = AdjustRootDataHeader(true /*Gfx*/, false /*PsoUsesGSorHS*/, rootPacketHeaderBuffer[descTableRootParameterIndex], 1, 0);
		outBuffer.m_rootDescTableDescriptorHandle     = modelData[currModelID].m_descriptorTableData.m_gpuDescriptorHandle;

		// Root Constant Buffer
		uint constBufferRootParameterIndex = modelData[currModelID].m_rootConstantBufferData.m_rootParameterIndex;
		outBuffer.m_rootConstBufferRootPacketHeaderData = AdjustRootDataHeader(true /*Gfx*/, false /*PsoUsesGSorHS*/, rootPacketHeaderBuffer[constBufferRootParameterIndex], 2, 0);
		outBuffer.m_rootConstBufferLocation.x = modelDataUpdatePerFrame.m_constantBufferLocationLo + DTid.x * modelDataUpdatePerFrame.m_sizeofModelEffectConstants;
		outBuffer.m_rootConstBufferLocation.y = modelDataUpdatePerFrame.m_constantBufferLocationHi;

		// To add a carry or not, that's the question
		if (outBuffer.m_rootConstBufferLocation.x < modelData[currModelID].m_rootConstantBufferData.m_bufferLocationLo)
		{
			outBuffer.m_rootConstBufferLocation.y += 1;
		}

		// DrawIndexedInstanced
		outBuffer.m_drawIndexedInstancedHeader     = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
		outBuffer.m_instanceCount                  = modelData[currModelID].m_drawIndexedArgs.instanceCount;
		outBuffer.m_startIndexLocation             = modelData[currModelID].m_drawIndexedArgs.startIndexLocation;
		outBuffer.m_indexCountPerInstance          = modelData[currModelID].m_drawIndexedArgs.indexCountPerInstance;
		outBuffer.m_baseVertexLocation             = modelData[currModelID].m_drawIndexedArgs.baseVertexLocation;
		outBuffer.m_startInstanceLocation          = modelData[currModelID].m_drawIndexedArgs.startInstanceLocation;
	
		bufBYOBOut.Append(outBuffer);

		// Increment the number of bytes written to the buffer,
		// which will be executed by the GPU. Not using the Append Counter in this 
		// case as the buffer size has to be incremented by SIZEOF_GPU_APPEND_BUFFER_STRUCT
		// for each entry in the Append buffer
		InterlockedAdd(bufCountBYOB[1].x, SIZEOF_GPU_APPEND_BUFFER_STRUCT);
	}
}
