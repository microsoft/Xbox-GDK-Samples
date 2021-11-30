//--------------------------------------------------------------------------------------
// Meshlet.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "Meshlet.h"

#include <algorithm>
#include <fstream>
#include <utility>


using namespace ATG;
using namespace DirectX;

namespace
{
    template <typename T, typename U>
    constexpr T RoundUpDiv(T num, U denom)
    {
        return (num + denom - 1) / denom;
    }

    template <typename T>
    size_t GetAlignedSize(T size)
    {
        const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
        return alignedSize;
    }
    
    struct MeshletFileHeader
    {
        enum
        {
            MESHLET_VERSION_INITIAL = 0x0,
            MESHLET_VERSION_CULLDATA = 0x1,
            MESHLET_VERSION_CULLDATA_UPDATE = 0x2,
            MESHLET_VERSION_GEN_UPDATE = 0x3,
            MESHLET_VERSION_CURRENT = MESHLET_VERSION_GEN_UPDATE
        };

        uint32_t Prolog;
        uint32_t Version;
        uint32_t Count;
    };
}

namespace ATG 
{
    std::istream& operator<<(MeshletSet& m, std::istream& stream)
    {
        m.Read(stream);
        return stream;
    }
}

uint32_t MeshletSet::GetPrimitiveCount() const
{
    uint32_t count = 0;

    for (auto& m : m_meshletData)
        count += m.PrimCount;

    return count;
}

uint32_t MeshletSet::GetIndexCount() const
{
    return static_cast<uint32_t>(m_uniqueIndexData.size() / BytesPerIndex());
}

uint32_t MeshletSet::GetVertexIndex(uint32_t index)
{
    if (m_indexFormat == DXGI_FORMAT_R32_UINT)
    {
        return *(reinterpret_cast<uint32_t*>(m_uniqueIndexData.data()) + index);
    }
    else
    {
        return *(reinterpret_cast<uint16_t*>(m_uniqueIndexData.data()) + index);
    }
}

void MeshletSet::GetPrimitive(uint32_t index, uint32_t& v0, uint32_t& v1, uint32_t& v2)
{
    auto prim = m_primitiveData[index];
    v0 = prim.indices.i0;
    v1 = prim.indices.i1;
    v2 = prim.indices.i2;
}

uint32_t MeshletSet::GetLastMeshletSize(uint32_t submeshIndex) const 
{
    auto& submesh = m_submeshes[submeshIndex];
    auto& meshlet = m_meshletData[submesh.Offset + submesh.Count - 1];

    return std::max(meshlet.PrimCount, meshlet.VertCount);
}

uint32_t MeshletSet::CalcThreadGroupCount(uint32_t submeshIndex, uint32_t groupSize, uint32_t instanceCount) const
{
    auto unpackedCount = (GetMeshletCount(submeshIndex) - 1) * instanceCount;
    auto packedCount = RoundUpDiv(GetLastMeshletSize(submeshIndex) * instanceCount, groupSize);
    auto totalCount = unpackedCount + packedCount;

    assert(totalCount < 64 * 1024);
    return totalCount;
}

uint32_t MeshletSet::InstancesPerDispatch(uint32_t submeshIndex, uint32_t groupSize) const
{
    constexpr uint32_t s_maxThreadGroups = 65536; // Only use X dimension - limited to 65536

    return uint32_t(float(s_maxThreadGroups - 1) / (GetMeshletCount(submeshIndex) - 1 + float(GetLastMeshletSize(submeshIndex)) / groupSize));
}

uint32_t MeshletSet::GetLastMeshletSize() const
{
    auto& meshlet = m_meshletData.back();

    return std::max(meshlet.PrimCount, meshlet.VertCount);
}

uint32_t MeshletSet::CalcThreadGroupCount(uint32_t groupSize, uint32_t instanceCount) const
{
    auto unpackedCount = (GetMeshletCount() - 1) * instanceCount;
    auto packedCount = RoundUpDiv(GetLastMeshletSize() * instanceCount, groupSize);
    auto totalCount = unpackedCount + packedCount;

    assert(totalCount < 64 * 1024);
    return totalCount;
}

uint32_t MeshletSet::InstancesPerDispatch(uint32_t groupSize) const
{
    constexpr uint32_t s_maxThreadGroups = 65536; // Only use X dimension - limited to 65536

    return uint32_t(float(s_maxThreadGroups - 1) / (GetMeshletCount() - 1 + float(GetLastMeshletSize()) / groupSize));
}

void MeshletSet::CreateResources(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload)
{
    // Create committed D3D resources of proper sizes
    auto meshletDesc    = CD3DX12_RESOURCE_DESC::Buffer(m_meshletData.size() * sizeof(m_meshletData[0]));
    auto cullDataDesc   = CD3DX12_RESOURCE_DESC::Buffer(m_cullData.size() * sizeof(m_cullData[0]));
    auto indexDesc      = CD3DX12_RESOURCE_DESC::Buffer(RoundUpDiv(m_uniqueIndexData.size(), 4) * 4);
    auto primitiveDesc  = CD3DX12_RESOURCE_DESC::Buffer(m_primitiveData.size() * sizeof(m_primitiveData[0]));
    auto meshInfoDesc   = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(MeshInfo)));

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &meshletDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_meshletResource.ReleaseAndGetAddressOf()));
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &cullDataDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_cullDataResource.ReleaseAndGetAddressOf()));
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &indexDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_uniqueIndexResource.ReleaseAndGetAddressOf()));
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &primitiveDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_primitiveResource.ReleaseAndGetAddressOf()));
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &meshInfoDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_meshInfoResource.ReleaseAndGetAddressOf()));

    // Upload to D3D resources
    auto meshletMem = GraphicsMemory::Get().Allocate(meshletDesc.Width);
    std::memcpy(meshletMem.Memory(), m_meshletData.data(), meshletDesc.Width);
    resourceUpload.Upload(m_meshletResource.Get(), std::move(meshletMem));

    auto cullDataMem = GraphicsMemory::Get().Allocate(cullDataDesc.Width);
    std::memcpy(cullDataMem.Memory(), m_cullData.data(), cullDataDesc.Width);
    resourceUpload.Upload(m_cullDataResource.Get(), std::move(cullDataMem));

    auto indexMem = GraphicsMemory::Get().Allocate(indexDesc.Width);
    std::memcpy(indexMem.Memory(), m_uniqueIndexData.data(), m_uniqueIndexData.size());
    resourceUpload.Upload(m_uniqueIndexResource.Get(), std::move(indexMem));

    auto primitiveMem = GraphicsMemory::Get().Allocate(primitiveDesc.Width);
    std::memcpy(primitiveMem.Memory(), m_primitiveData.data(), primitiveDesc.Width);
    resourceUpload.Upload(m_primitiveResource.Get(), std::move(primitiveMem));

    MeshInfo info;
    info.IndexBytes      = BytesPerIndex();
    info.LastMeshletSize = GetLastMeshletSize();
    info.MeshletCount    = GetMeshletCount();

    resourceUpload.Upload(m_meshInfoResource.Get(), GraphicsMemory::Get().AllocateConstant(info));

    // Transition to read state
    resourceUpload.Transition(m_meshletResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    resourceUpload.Transition(m_cullDataResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    resourceUpload.Transition(m_uniqueIndexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    resourceUpload.Transition(m_primitiveResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    resourceUpload.Transition(m_meshInfoResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void MeshletSet::Read(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&m_maxVerts), sizeof(m_maxVerts));
    stream.read(reinterpret_cast<char*>(&m_maxPrims), sizeof(m_maxPrims));

    {
        uint32_t meshletCount;
        stream.read(reinterpret_cast<char*>(&meshletCount), 4);

        m_meshletData.resize(meshletCount);
        stream.read(reinterpret_cast<char*>(m_meshletData.data()), std::streamsize(meshletCount * sizeof(m_meshletData[0])));

        m_cullData.resize(meshletCount);
        stream.read(reinterpret_cast<char*>(m_cullData.data()), std::streamsize(meshletCount * sizeof(m_cullData[0])));
    }

    {
        uint32_t submeshCount;
        stream.read(reinterpret_cast<char*>(&submeshCount), 4);

        m_submeshes.resize(submeshCount);
        stream.read(reinterpret_cast<char*>(m_submeshes.data()), std::streamsize(submeshCount * sizeof(m_submeshes[0])));
    }

    {
        uint32_t indexBytes, indexCount;
        stream.read(reinterpret_cast<char*>(&indexBytes), 4);
        stream.read(reinterpret_cast<char*>(&indexCount), 4);

        m_uniqueIndexData.resize(indexCount * indexBytes);
        stream.read(reinterpret_cast<char*>(m_uniqueIndexData.data()), indexCount * indexBytes);

        m_indexFormat = indexBytes == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    }

    {
        uint32_t primCount;
        stream.read(reinterpret_cast<char*>(&primCount), 4);

        m_primitiveData.resize(primCount);
        stream.read(reinterpret_cast<char*>(m_primitiveData.data()), std::streamsize(primCount * sizeof(m_primitiveData[0])));
    }
}

std::vector<MeshletSet> MeshletSet::Read(const wchar_t* filePath)
{
    auto file = std::ifstream(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::vector<MeshletSet>();
    }

    MeshletFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.Prolog != 'MSHL')
        throw std::exception("Opened file is not of the meshlet file format.");

    if (header.Version != MeshletFileHeader::MESHLET_VERSION_CURRENT)
        throw std::exception("Meshlet version is out of date! Please update meshlet runtime code.");

    std::vector<MeshletSet> meshlets;
    meshlets.resize(header.Count);

    for (auto& m : meshlets)
    {
        m.Read(file);
    }

    return meshlets;
}
