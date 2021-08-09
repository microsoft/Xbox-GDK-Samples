//--------------------------------------------------------------------------------------
// Meshlet.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "d3dx12.h"
#include "IResourceUploader.h"

#include <DirectXMath.h>
#include <memory>
#include <ostream>
#include <vector>
#include <wrl.h>


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

    class MeshletSet
    {
    public:

        DXGI_FORMAT     IndexFormat() const { return m_indexFormat; }
        uint32_t        BytesPerIndex() const { return m_indexFormat == DXGI_FORMAT_R32_UINT ? 4u : 2u; }

        uint32_t        GetSubmeshCount() const { return static_cast<uint32_t>(m_submeshes.size()); }
        uint32_t        GetPrimitiveCount() const;

        // With submesh specifier
        uint32_t        GetMeshletCount(uint32_t submeshIndex) const { return m_submeshes[submeshIndex].Count; }
        uint32_t        GetLastMeshletSize(uint32_t submeshIndex) const;

        uint32_t        CalcThreadGroupCount(uint32_t submeshIndex, uint32_t groupSize, uint32_t instanceCount) const;
        uint32_t        InstancesPerDispatch(uint32_t submeshIndex, uint32_t groupSize) const;

        // Without submesh specifier (uses all the meshlets)
        uint32_t        GetMeshletCount() const { return static_cast<uint32_t>(m_meshletData.size()); }
        uint32_t        GetLastMeshletSize() const;
                        
        uint32_t        CalcThreadGroupCount(uint32_t groupSize, uint32_t instanceCount) const;
        uint32_t        InstancesPerDispatch(uint32_t groupSize) const;

        // Accessors for vertex index & primitive data
        uint32_t        GetVertexIndex(uint32_t index);
        void            GetPrimitive(uint32_t index, uint32_t& v0, uint32_t& v1, uint32_t& v2);

        void            CreateResources(ID3D12Device* device, IResourceUploader* uploader);

        // Accessors for raw meshlet data
        auto&           GetMeshlets() const { return m_meshletData; }
        auto&           GetCullData() const { return m_cullData; }
        auto&           GetSubmeshes() const { return m_submeshes; }
        auto&           GetUniqueIndexData() const { return m_uniqueIndexData; }
        auto&           GetPrimitiveData() const { return m_primitiveData; }

        // Accessors for D3D12 resources
        ID3D12Resource* GetMeshletBuffer() const { return m_meshletBuffer.Get(); }
        ID3D12Resource* GetUniqueIndexBuffer() const { return m_uniqueIndexBuffer.Get(); }
        ID3D12Resource* GetPrimitiveBuffer() const { return m_primitiveBuffer.Get(); }
        ID3D12Resource* GetMeshInfoBuffer() const { return m_meshInfoBuffer.Get(); }

        void Read(std::istream& stream);
        static std::vector<MeshletSet> ReadMeshlets(const wchar_t* filePath);

    private:
        struct MeshInfo
        {
            uint32_t IndexBytes;
            uint32_t MeshletCount;
            uint32_t LastMeshletSize;
        };

        union PackedIndices
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

    private:
        uint32_t                    m_maxVerts;
        uint32_t                    m_maxPrims;
        DXGI_FORMAT                 m_indexFormat;

        std::vector<Submesh>        m_submeshes;
        std::vector<Meshlet>        m_meshletData;
        std::vector<CullData>       m_cullData;
        std::vector<uint8_t>        m_uniqueIndexData;
        std::vector<PackedIndices>  m_primitiveData;

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_meshletBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_cullDataBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_uniqueIndexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_primitiveBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_meshInfoBuffer;
    };
}
