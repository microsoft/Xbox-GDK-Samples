//--------------------------------------------------------------------------------------
// Meshlet.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <ostream>

namespace ATG
{
    struct Meshlet
    {
        uint32_t VertCount;
        uint32_t VertOffset;
        uint32_t PrimCount;
        uint32_t PrimOffset;
    };

    struct CullData
    {
        DirectX::XMFLOAT4 BoundingSphere; // xyz = center, w = radius
        uint8_t           NormalCone[4];  // xyz = axis, w = -cos(a + 90)
        float             ApexOffset;     // apex = center - axis * offset
    };

    struct Submesh
    {
        uint32_t Count;
        uint32_t Offset;
    };

    union PackedTriangle
    {
        struct
        {
            uint32_t i0 : 10;
            uint32_t i1 : 10;
            uint32_t i2 : 10;
            uint32_t _unused : 2;
        } indices;
        uint32_t packed;
    };

    struct MeshInfo
    {
        uint32_t IndexBytes;
        uint32_t MeshletCount;
        uint32_t LastMeshletSize;
        uint32_t _padding;
    };
    static_assert((sizeof(MeshInfo) % 16) == 0, "Structure misalignment.");

    class MeshletSet
    {
    public:
        // Accessors for vertex index & primitive data
        uint32_t        GetVertexIndex(uint32_t index);
        void            GetPrimitive(uint32_t index, uint32_t& v0, uint32_t& v1, uint32_t& v2);

        DXGI_FORMAT     IndexFormat() const { return m_indexFormat; }
        uint32_t        BytesPerIndex() const { return m_indexFormat == DXGI_FORMAT_R32_UINT ? 4u : 2u; }

        uint32_t        GetSubmeshCount() const { return static_cast<uint32_t>(m_submeshes.size()); }
        uint32_t        GetPrimitiveCount() const;
        uint32_t        GetIndexCount() const;

        // With submesh specifier
        uint32_t        GetMeshletCount(uint32_t submeshIndex) const { return m_submeshes[submeshIndex].Count; }
        uint32_t        GetLastMeshletSize(uint32_t submeshIndex) const;

        uint32_t        CalcThreadGroupCount(uint32_t submeshIndex, uint32_t groupSize, uint32_t instanceCount) const;
        uint32_t        InstancesPerDispatch(uint32_t submeshIndex, uint32_t groupSize) const;

        // Without submesh specifier (ignores submeshes)
        uint32_t        GetMeshletCount() const { return static_cast<uint32_t>(m_meshletData.size()); }
        uint32_t        GetLastMeshletSize() const;

        uint32_t        CalcThreadGroupCount(uint32_t groupSize, uint32_t instanceCount) const;
        uint32_t        InstancesPerDispatch(uint32_t groupSize) const;

        // Accessors for raw meshlet data
        auto&           GetMeshlets() const { return m_meshletData; }
        auto&           GetCullData() const { return m_cullData; }
        auto&           GetSubmeshes() const { return m_submeshes; }
        auto&           GetUniqueIndexData() const { return m_uniqueIndexData; }
        auto&           GetPrimitiveData() const { return m_primitiveData; }

        // D3D12 Resource Upload & Access
        void            CreateResources(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload);

        ID3D12Resource*	GetMeshletResource() const { return m_meshletResource.Get(); }
        ID3D12Resource*	GetCullDataResource() const { return m_cullDataResource.Get(); }
        ID3D12Resource*	GetUniqueIndexResource() const { return m_uniqueIndexResource.Get(); }
        ID3D12Resource*	GetPrimitiveResource() const { return m_primitiveResource.Get(); }
        ID3D12Resource*	GetMeshInfoResource() const { return m_meshInfoResource.Get(); }

        // File Loading
        void Read(std::istream& stream);
        static std::vector<MeshletSet> Read(const wchar_t* filePath);

    private:

    private:
        uint32_t                    m_maxVerts;
        uint32_t                    m_maxPrims;
        DXGI_FORMAT                 m_indexFormat;

        std::vector<Submesh>        m_submeshes;
        std::vector<Meshlet>        m_meshletData;
        std::vector<CullData>       m_cullData;
        std::vector<uint8_t>        m_uniqueIndexData;
        std::vector<PackedTriangle> m_primitiveData;

        Microsoft::WRL::ComPtr<ID3D12Resource> 	m_meshletResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> 	m_cullDataResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> 	m_uniqueIndexResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> 	m_primitiveResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> 	m_meshInfoResource;

    private:
        friend std::istream& operator<<(MeshletSet& meshlet, std::istream& stream);
    };

    std::istream& operator<<(MeshletSet& meshlet, std::istream& stream);
}
