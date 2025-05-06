// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
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

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "GltfResources.h"
#include "GltfPbrMaterial.h"

namespace AMDTK
{
    // Material, primitive and mesh structs specific for the depth pass (you might want to compare these structs with the ones used for the depth pass in GltfPbrPass.h)

    struct DepthMaterial
    {
        int m_textureCount = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE m_pTransparency = { 0 };

        DefineList m_defines;
        bool m_doubleSided = false;
    };

    struct DepthPrimitives
    {
        Geometry m_geometry;

        DepthMaterial *m_pMaterial = NULL;

        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pso;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    };

    struct DepthMesh
    {
        std::vector<DepthPrimitives> m_pPrimitives;
    };

    class GltfDepthPass
    {
    public:

        struct per_frame
        {
            DirectX::XMMATRIX mViewProj;
        };

        struct per_object
        {
            DirectX::XMMATRIX mWorld;
        };

        void OnCreate(
            ID3D12Device* pDevice,
            GLTFResources* pGltfResources,
            bool inverseDepth = false
        );

        void OnDestroy();
        void Draw(ID3D12GraphicsCommandList* pCommandList);
        void SetPerFrameCB(D3D12_GPU_VIRTUAL_ADDRESS cb);
    private:
        D3D12_GPU_VIRTUAL_ADDRESS m_perFrameDesc;
        ID3D12Device* m_pDevice;

        struct CachedPSO
        {
            Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
            Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        };
        std::map<uint32_t, CachedPSO> m_psoCache;

        DirectX::EffectTextureFactory* m_texFactory;

        std::vector<DepthMesh> m_meshes;
        std::vector<DepthMaterial> m_materialsData;

        DepthMaterial m_defaultMaterial;

        GLTFResources* m_pGltfResources;
        D3D12_STATIC_SAMPLER_DESC m_samplerDesc;

        void CreatePipeline(bool bUsingSkinning, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, DepthPrimitives *pPrimitive);

        std::vector<uint8_t> m_bcVsDefault;
        std::vector<uint8_t> m_bcPsDefault;
        bool m_inverseDepth;
    };
}


