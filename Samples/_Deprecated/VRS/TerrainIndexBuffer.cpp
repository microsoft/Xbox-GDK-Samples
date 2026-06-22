//--------------------------------------------------------------------------------------
// TerrainIndexBuffer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Terrain.h"

#define i00     GetIndex(uint16_t(x + 0 * scale), uint16_t(y + 0 * scale))
#define i01     GetIndex(uint16_t(x + 0 * scale), uint16_t(y + 1 * scale))
#define i02     GetIndex(uint16_t(x + 0 * scale), uint16_t(y + 2 * scale))
#define i03     GetIndex(uint16_t(x + 0 * scale), uint16_t(y + 3 * scale))
#define i04     GetIndex(uint16_t(x + 0 * scale), uint16_t(y + 4 * scale))

#define i10     GetIndex(uint16_t(x + 1 * scale), uint16_t(y + 0 * scale))
#define i11     GetIndex(uint16_t(x + 1 * scale), uint16_t(y + 1 * scale))
#define i12     GetIndex(uint16_t(x + 1 * scale), uint16_t(y + 2 * scale))
#define i13     GetIndex(uint16_t(x + 1 * scale), uint16_t(y + 3 * scale))
#define i14     GetIndex(uint16_t(x + 1 * scale), uint16_t(y + 4 * scale))

#define i20     GetIndex(uint16_t(x + 2 * scale), uint16_t(y + 0 * scale))
#define i21     GetIndex(uint16_t(x + 2 * scale), uint16_t(y + 1 * scale))
#define i22     GetIndex(uint16_t(x + 2 * scale), uint16_t(y + 2 * scale))
#define i23     GetIndex(uint16_t(x + 2 * scale), uint16_t(y + 3 * scale))
#define i24     GetIndex(uint16_t(x + 2 * scale), uint16_t(y + 4 * scale))

#define i30     GetIndex(uint16_t(x + 3 * scale), uint16_t(y + 0 * scale))
#define i31     GetIndex(uint16_t(x + 3 * scale), uint16_t(y + 1 * scale))
#define i32     GetIndex(uint16_t(x + 3 * scale), uint16_t(y + 2 * scale))
#define i33     GetIndex(uint16_t(x + 3 * scale), uint16_t(y + 3 * scale))
#define i34     GetIndex(uint16_t(x + 3 * scale), uint16_t(y + 4 * scale))

#define i40     GetIndex(uint16_t(x + 4 * scale), uint16_t(y + 0 * scale))
#define i41     GetIndex(uint16_t(x + 4 * scale), uint16_t(y + 1 * scale))
#define i42     GetIndex(uint16_t(x + 4 * scale), uint16_t(y + 2 * scale))
#define i43     GetIndex(uint16_t(x + 4 * scale), uint16_t(y + 3 * scale))
#define i44     GetIndex(uint16_t(x + 4 * scale), uint16_t(y + 4 * scale))

using namespace DirectX;

__forceinline uint16_t Terrain::GetIndex(uint16_t x, uint16_t y)
{
    assert(x <= m_tileSize && y <= m_tileSize);

    return x + (y * (m_tileSize + 1));
}


__forceinline uint16_t *Terrain::WriteTriangle(uint16_t *indices, uint16_t i0, uint16_t i1, uint16_t i2)
{
    indices[0] = i0;
    indices[1] = i1;
    indices[2] = i2;

    return indices + 3;
}


uint16_t *Terrain::WriteTriangleFan2(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2)
{
    indices = WriteTriangle(indices, i0, i1, sharedIndex);
    indices = WriteTriangle(indices, i1, i2, sharedIndex);
    return indices;
}


uint16_t *Terrain::WriteTriangleFan3(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3)
{
    indices = WriteTriangle(indices, i0, i1, sharedIndex);
    indices = WriteTriangle(indices, i1, i2, sharedIndex);
    indices = WriteTriangle(indices, i2, i3, sharedIndex);
    return indices;
}


uint16_t *Terrain::WriteTriangleFan4(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4)
{
    indices = WriteTriangle(indices, i0, i1, sharedIndex);
    indices = WriteTriangle(indices, i1, i2, sharedIndex);
    indices = WriteTriangle(indices, i2, i3, sharedIndex);
    indices = WriteTriangle(indices, i3, i4, sharedIndex);
    return indices;
}


uint16_t *Terrain::WriteQuad(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
        +---+
        |  /|
        | / |
        |/  |
        +---+
    */
    indices = WriteTriangle(indices, i00, i10, i01);
    indices = WriteTriangle(indices, i01, i10, i11);
    return indices;
}


uint16_t *Terrain::WriteSideB_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
        +---+---+
        |  / \  |
        | /   \ |
        |/     \|
        +-------+
    */
    return WriteTriangleFan3(indices, i10, i20, i21, i01, i00);
}

uint16_t *Terrain::WriteSideT_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
       +-------+
       |\     /|
       | \   / |
       |  \ /  |
       +---+---+
    */
    return WriteTriangleFan3(indices, i11, i01, i00, i20, i21);
}

uint16_t *Terrain::WriteSideR_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
       +---+
       |  /|
       | / |
       |/  |
       +   |
       |\  |
       | \ |
       |  \|
       +---+
   */
    return WriteTriangleFan3(indices, i01, i00, i10, i12, i02);
}

uint16_t *Terrain::WriteSideL_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
        +---+
        |\  |
        | \ |
        |  \|
        |   +
        |  /|
        | / |
        |/  |
        +---+
    */
    return WriteTriangleFan3(indices, i11, i12, i02, i00, i10);
}


uint16_t *Terrain::WriteSideT_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
        +-+-+-+-+
        |  / \  |
        | /   \ |
        |/     \|
        +-------+
    */
    indices = WriteTriangleFan2(indices, i02, i00, i10, i20);
    indices = WriteTriangleFan2(indices, i42, i20, i30, i40);
    indices = WriteTriangle(indices, i02, i20, i42);
    return indices;
}


uint16_t *Terrain::WriteSideB_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
       +-------+
       |\     /|
       | \   / |
       |  \ /  |
       +-+-+-+-+
    */
    indices = WriteTriangleFan2(indices, i40, i42, i32, i22);
    indices = WriteTriangleFan2(indices, i00, i22, i12, i02);
    indices = WriteTriangle(indices, i00, i40, i22);
    return indices;
}

uint16_t *Terrain::WriteSideL_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
       +---+
       |  /|
       + / |
       |/  |
       +   |
       |\  |
       + \ |
       |  \|
       +---+
    */
    indices = WriteTriangleFan2(indices, i24, i04, i03, i02);
    indices = WriteTriangleFan2(indices, i20, i02, i01, i00);
    indices = WriteTriangle(indices, i24, i02, i20);
    return indices;
}

uint16_t *Terrain::WriteSideR_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    /*
        +---+
        |\  |
        | \ +
        |  \|
        |   +
        |  /|
        | / +
        |/  |
        +---+
    */
    indices = WriteTriangleFan2(indices, i00, i20, i21, i22);
    indices = WriteTriangleFan2(indices, i04, i22, i23, i24);
    indices = WriteTriangle(indices, i00, i22, i04);
    return indices;
}


uint16_t *Terrain::WriteCornerFan_TL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    indices = WriteTriangle(indices, i00, i10, i01);
    indices = WriteTriangle(indices, i10, i20, i22);
    indices = WriteTriangle(indices, i01, i10, i22);
    indices = WriteTriangle(indices, i02, i01, i22);
    return indices;
}

uint16_t *Terrain::WriteCornerFan_TR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    indices = WriteTriangle(indices, i00, i10, i02);
    indices = WriteTriangle(indices, i10, i20, i21);
    indices = WriteTriangle(indices, i10, i21, i02);
    indices = WriteTriangle(indices, i02, i21, i22);
    return indices;
}

uint16_t *Terrain::WriteCornerFan_BR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    indices = WriteTriangle(indices, i00, i20, i21);
    indices = WriteTriangle(indices, i00, i21, i12);
    indices = WriteTriangle(indices, i21, i22, i12);
    indices = WriteTriangle(indices, i00, i12, i02);
    return indices;
}

uint16_t *Terrain::WriteCornerFan_BL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    indices = WriteTriangle(indices, i00, i20, i01);
    indices = WriteTriangle(indices, i01, i20, i12);
    indices = WriteTriangle(indices, i12, i20, i22);
    indices = WriteTriangle(indices, i02, i01, i12);
    return indices;
}

uint16_t *Terrain::WriteCornerCutOut_TL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    return WriteTriangleFan4(indices, i11, i12, i02, i00, i20, i21);
}

uint16_t *Terrain::WriteCornerCutOut_TR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    return WriteTriangleFan4(indices, i11, i01, i00, i20, i22, i12);
}

uint16_t *Terrain::WriteCornerCutOut_BR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    return WriteTriangleFan4(indices, i11, i10, i20, i22, i02, i01);
}

uint16_t *Terrain::WriteCornerCutOut_BL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale)
{
    return WriteTriangleFan4(indices, i11, i21, i22, i02, i00, i10);
}



void Terrain::CreateIndexBuffers(std::unique_ptr<DX::DeviceResources> &deviceResources, std::unique_ptr<GraphicsMemory> &graphicsMemory, ResourceUploadBatch &upload)
{
    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    auto device = deviceResources->GetD3DDevice();

    // high res index buffer
    {
        // high res interior, half res edges
        constexpr uint32_t numTrisCornerPiece = 4;
        constexpr uint32_t numTrisEdgePiece = 3;
        constexpr uint32_t numEdgePiecesPerEdge = (m_tileSize - 4) >> 1;
        constexpr uint32_t interiorSize = m_tileSize - 2;
        constexpr uint32_t numQuads = interiorSize * interiorSize;
        constexpr uint32_t numTriangles = (numTrisCornerPiece * 4) + (numTrisEdgePiece * numEdgePiecesPerEdge * 4) + (numQuads * 2);
        constexpr uint32_t numIndices = numTriangles * 3;
        constexpr uint32_t indexBufferSize = numIndices * sizeof(uint16_t);
        m_numIndices[LODs::High] = numIndices;

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_indexBuffer[LODs::High].GetAddressOf())
        ));
        m_indexBuffer[LODs::High]->SetName(L"m_indexBufferHighRes");
        m_indexBufferView[LODs::High].BufferLocation = m_indexBuffer[LODs::High]->GetGPUVirtualAddress();
        m_indexBufferView[LODs::High].Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView[LODs::High].SizeInBytes = indexBufferSize;

        SharedGraphicsResource indexBufferUpload = graphicsMemory->Allocate(indexBufferSize);
        auto index = static_cast<uint16_t *>(indexBufferUpload.Memory());
#ifndef NDEBUG
        uint16_t *indexEnd = index + numIndices;
#endif
        uint16_t m = m_tileSize - 2;

        // corners
        index = WriteCornerCutOut_TL(index, 0, 0, 1);
        index = WriteCornerCutOut_TR(index, m, 0, 1);
        index = WriteCornerCutOut_BL(index, 0, m, 1);
        index = WriteCornerCutOut_BR(index, m, m, 1);

        // edges
        m = m_tileSize - 1;
        for (uint16_t i = 2; i < (m_tileSize - 2); i += 2)
        {
            index = WriteSideT_21(index, i, 0, 1);
            index = WriteSideL_21(index, 0, i, 1);
            index = WriteSideR_21(index, m, i, 1);
            index = WriteSideB_21(index, i, m, 1);      // B is the low res edge, interior is high res
        }
        // interior
        for (uint16_t y = 1; y < m; ++y)
        {
            for (uint16_t x = 1; x < m; ++x)
            {
                index = WriteQuad(index, x, y, 1);
            }
        }
        assert(index == indexEnd);
        upload.Upload(m_indexBuffer[LODs::High].Get(), indexBufferUpload);
        upload.Transition(m_indexBuffer[LODs::High].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
    // med res index buffer, regular 2x2 quads
    {
        constexpr uint32_t numQuads = (m_tileSize >> 1) * (m_tileSize >> 1);
        constexpr uint32_t numTriangles = numQuads * 2;
        constexpr uint32_t numIndices = numTriangles * 3;
        constexpr uint32_t indexBufferSize = numIndices * sizeof(uint16_t);
        m_numIndices[LODs::Medium] = numIndices;

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_indexBuffer[LODs::Medium].GetAddressOf())
        ));
        m_indexBuffer[LODs::Medium]->SetName(L"m_indexBufferMedRes");
        m_indexBufferView[LODs::Medium].BufferLocation = m_indexBuffer[LODs::Medium]->GetGPUVirtualAddress();
        m_indexBufferView[LODs::Medium].Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView[LODs::Medium].SizeInBytes = indexBufferSize;

        SharedGraphicsResource indexBufferUpload = graphicsMemory->Allocate(indexBufferSize);
        auto index = static_cast<uint16_t *>(indexBufferUpload.Memory());
#ifndef NDEBUG
        uint16_t *indexEnd = index + numIndices;
#endif

        // interior
        for (uint16_t y = 0; y < m_tileSize; y += 2)
        {
            for (uint16_t x = 0; x < m_tileSize; x += 2)
            {
                index = WriteQuad(index, x, y, 2);
            }
        }
        assert(index == indexEnd);
        upload.Upload(m_indexBuffer[LODs::Medium].Get(), indexBufferUpload);
        upload.Transition(m_indexBuffer[LODs::Medium].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
    // low res index buffer, 4x4 quads, 2x2 edges
    {
        constexpr uint32_t numTrisCornerPiece = 2;
        constexpr uint32_t numTrisEdgePiece = 3;
        constexpr uint32_t numEdgePiecesPerEdge = (m_tileSize - 4) >> 2;
        constexpr uint32_t interiorEdgeLength = 4;
        constexpr uint32_t interiorSize = (m_tileSize - interiorEdgeLength) / interiorEdgeLength;
        constexpr uint32_t numQuads = interiorSize * interiorSize;
        constexpr uint32_t numTriangles = (numTrisCornerPiece * 4) + (numTrisEdgePiece * numEdgePiecesPerEdge * 4) + (numQuads * 2);
        constexpr uint32_t numIndices = numTriangles * 3;
        constexpr uint32_t indexBufferSize = numIndices * sizeof(uint16_t);
        m_numIndices[LODs::Low] = numIndices;

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_indexBuffer[LODs::Low].GetAddressOf())
        ));
        m_indexBuffer[LODs::Low]->SetName(L"m_indexBufferLowRes");
        m_indexBufferView[LODs::Low].BufferLocation = m_indexBuffer[LODs::Low]->GetGPUVirtualAddress();
        m_indexBufferView[LODs::Low].Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView[LODs::Low].SizeInBytes = indexBufferSize;

        SharedGraphicsResource indexBufferUpload = graphicsMemory->Allocate(indexBufferSize);
        auto index = static_cast<uint16_t *>(indexBufferUpload.Memory());
#ifndef NDEBUG
        uint16_t *indexEnd = index + numIndices;
#endif
        uint16_t m = m_tileSize - 2;

        // corners
        index = WriteQuad(index, 0, 0, 2);
        index = WriteQuad(index, m, 0, 2);
        index = WriteQuad(index, 0, m, 2);
        index = WriteQuad(index, m, m, 2);

        // edges
        for (uint16_t i = 2; i < m; i += 4)
        {
            index = WriteSideB_21(index, i, 0, 2);      // B is the low res edge, so interior in this case
            index = WriteSideR_21(index, 0, i, 2);
            index = WriteSideL_21(index, m, i, 2);
            index = WriteSideT_21(index, i, m, 2);
        }
        // interior
        for (uint16_t y = 2; y < m; y += 4)
        {
            for (uint16_t x = 2; x < m; x += 4)
            {
                index = WriteQuad(index, x, y, 4);
            }
        }
        assert(index == indexEnd);
        upload.Upload(m_indexBuffer[LODs::Low].Get(), indexBufferUpload);
        upload.Transition(m_indexBuffer[LODs::Low].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
    // very low res index buffer, 8x8 quads, double fan edges
    {
        constexpr uint32_t numTrisCornerPiece = 4;
        constexpr uint32_t numTrisEdgePiece = 5;
        constexpr uint32_t numEdgePiecesPerEdge = (m_tileSize - 8) >> 3;
        constexpr uint32_t interiorEdgeLength = 8;
        constexpr uint32_t interiorSize = (m_tileSize - interiorEdgeLength) / interiorEdgeLength;
        constexpr uint32_t numQuads = interiorSize * interiorSize;
        constexpr uint32_t numTriangles = (numTrisCornerPiece * 4) + (numTrisEdgePiece * numEdgePiecesPerEdge * 4) + (numQuads * 2);
        constexpr uint32_t numIndices = numTriangles * 3;
        constexpr uint32_t indexBufferSize = numIndices * sizeof(uint16_t);
        m_numIndices[LODs::VeryLow] = numIndices;

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_indexBuffer[LODs::VeryLow].GetAddressOf())
        ));
        m_indexBuffer[LODs::VeryLow]->SetName(L"m_indexBufferLowRes");
        m_indexBufferView[LODs::VeryLow].BufferLocation = m_indexBuffer[LODs::VeryLow]->GetGPUVirtualAddress();
        m_indexBufferView[LODs::VeryLow].Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView[LODs::VeryLow].SizeInBytes = indexBufferSize;

        SharedGraphicsResource indexBufferUpload = graphicsMemory->Allocate(indexBufferSize);
        auto index = static_cast<uint16_t *>(indexBufferUpload.Memory());
#ifndef NDEBUG
        uint16_t *indexEnd = index + numIndices;
#endif
        uint16_t m = m_tileSize - 4;

        // corners
        index = WriteCornerFan_TL(index, 0, 0, 2);
        index = WriteCornerFan_TR(index, m, 0, 2);
        index = WriteCornerFan_BL(index, 0, m, 2);
        index = WriteCornerFan_BR(index, m, m, 2);

        // edges
        for (uint16_t i = 4; i < m; i += 8)
        {
            index = WriteSideT_42(index, i, 0, 2);
            index = WriteSideL_42(index, 0, i, 2);
            index = WriteSideR_42(index, m, i, 2);
            index = WriteSideB_42(index, i, m, 2);      // B is the low res edge, interior is high res
        }
        // interior
        for (uint16_t y = 4; y < m; y+=8)
        {
            for (uint16_t x = 4; x < m; x+=8)
            {
                index = WriteQuad(index, x, y, 8);
            }
        }
        assert(index == indexEnd);
        upload.Upload(m_indexBuffer[LODs::VeryLow].Get(), indexBufferUpload);
        upload.Transition(m_indexBuffer[LODs::VeryLow].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
}
