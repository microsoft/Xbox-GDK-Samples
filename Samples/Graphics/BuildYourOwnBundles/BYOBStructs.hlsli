// Constant packet headers from d3d12_x.h
#define D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED 0x4004c200
#define D3D12XBOX_PACKET_DRAW_INSTANCED 0x4003c300
#define D3D12XBOX_PACKET_DISPATCH 0x4002c400
#define D3D12XBOX_PACKET_SET_PRIMITIVE_TOPOLOGY 0xc000b600
#define D3D12XBOX_PACKET_SET_INDEX_BUFFER 0x4001b700
#define D3D12XBOX_PACKET_SET_STENCIL_REF 0xc000b800
#define D3D12XBOX_PACKET_SET_VERTEX_BUFFERS 0x3fffc600 // Encoded dword count of 0
#define D3D12XBOX_PACKET_CLEAR_VERTEX_BUFFERS 0x4000c700
#define D3D12XBOX_PACKET_DISPATCH_X 0x4003cc00
#define D3D12XBOX_PACKET_DRAW_INDEXED_X 0x4004cd00
#define D3D12XBOX_PACKET_NOP 0x80000000
#define D3D12XBOX_PACKET_NOP_WITH_COUNT 0xbffe1000 // Encoded total packet size of 0
#define D3D12XBOX_PACKET_ADVANCE_PREDICATION_X 0x4000d100
#define D3D12XBOX_PACKET_COUNT_SHIFT 16

struct DrawIndexedArgs
{
	uint packetHeader;
	uint instanceCount;
	uint startIndexLocation;
	int  indexCountPerInstance;
	uint baseVertexLocation;
	uint startInstanceLocation;
};

struct PrimitiveTopology
{
	uint packetHeader;
	uint setTopology;
};

struct IndexBuffer
{
	uint  packetHeader;
	uint2 bufferLocation;
};

struct PipelineStateObject
{
	uint data0;
	uint data1;
	uint data2;
	uint data3;
	uint data4;
	uint data5;
#ifdef __XBOX_SCARLETT
    uint data6;
#endif
};
