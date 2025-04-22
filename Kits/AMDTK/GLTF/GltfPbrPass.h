// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#pragma once

#include "GltfResources.h"
#include "GltfPbrMaterial.h"

namespace AMDTK
{
    class Texture
    {
    public:
        ID3D12Resource*         m_pResource = nullptr;
    };

    struct PBRMaterial
    {
        uint32_t m_textureCount = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE m_textureTableHandle;
        size_t m_textureTableHeapOffset;
        D3D12_STATIC_SAMPLER_DESC m_samplers[10];

        PBRMaterialParameters m_pbrMaterialParameters;
    };

    struct PBRPrimitives
    {
        Geometry m_geometry;

        PBRMaterial *m_pMaterial = NULL;

        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pso;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;

        void DrawPrimitive(ID3D12GraphicsCommandList *pCommandList, D3D12_GPU_DESCRIPTOR_HANDLE shadowAtlas, D3D12_GPU_VIRTUAL_ADDRESS perSceneDesc, D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc, D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton);
    };

    struct PBRMesh
    {
        std::vector<PBRPrimitives> m_pPrimitives;
    };

    class GltfPbrPass
    {
    public:

        struct per_object
        {
            DirectX::XMMATRIX mWorld;

            PBRMaterialParametersConstantBuffer m_pbrParams;
        };

        struct BatchList
        {
            float m_depth;
            PBRPrimitives *m_pPrimitive;
            D3D12_GPU_VIRTUAL_ADDRESS m_perFrameDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_perObjectDesc;
            D3D12_GPU_VIRTUAL_ADDRESS m_pPerSkeleton;
            operator float() { return -m_depth; }
        };


        void OnCreate(
            ID3D12Device* pDevice,
            GLTFResources* pGltfResources,
            DXGI_FORMAT outForwardFormat,
            DXGI_FORMAT outSpecularRoughnessFormat,
            DXGI_FORMAT outDiffuseColor,
            DXGI_FORMAT outNormals,
            uint32_t sampleDescCount,
            bool inverseDepth = false
        );

        void OnDestroy();
        void BuildLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent);
        void Draw(ID3D12GraphicsCommandList *pCommandList, D3D12_GPU_DESCRIPTOR_HANDLE shadowAtlas);
    private:
        ID3D12Device* m_pDevice;
        GLTFResources* m_pGltfResources;

        struct CachedPSO
        {
            Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
            Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        };
        std::map<uint32_t, CachedPSO> m_psoCache;

        DirectX::EffectTextureFactory* m_texFactory;

        std::vector<PBRMesh> m_meshes;
        std::vector<PBRMaterial> m_materialsData;

        PBRMaterial m_defaultMaterial; 

        bool m_doLighting;
        bool m_inverseDepth;

        std::vector<DXGI_FORMAT> m_outFormats;
        uint32_t m_sampleCount;

        void CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, ID3D12Resource *> &texturesBase);
        void CreateDescriptors(bool bUsingSkinning, DefineList &defines, PBRPrimitives *pPrimitive);
        void CreatePipeline(std::vector<D3D12_INPUT_ELEMENT_DESC> layout, const DefineList &defines, PBRPrimitives *pPrimitive);
        bool FrustumCulled(const DirectX::XMMATRIX mCameraViewProj, const DirectX::XMVECTOR center, const DirectX::XMVECTOR extent);
  
    };
}


