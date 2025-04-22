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

#include "DeviceResources.h"
#include "GltfResources.h"
#include "GltfPbrMaterial.h"
#include "DXRHelper.h"

namespace AMDTK
{
    // Material, primitive and mesh structs specific for the depth pass (you might want to compare these structs with the ones used for the depth pass in GltfPbrPass.h)

    struct SimpleTriangleRecord : public ShaderRecord
    {
        SimpleTriangleRecord() : ShaderRecord()
        {

        }

        SimpleTriangleRecord(ID3D12StateObjectProperties* props, LPCWSTR exportName)
        {
            Initialize(props, exportName);
        }
    };

    struct RTMaterial
    {
        int m_textureCount = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE m_pTransparency = { 0 };

        DefineList m_defines;
        bool m_doubleSided = false;
    };

    struct RTPrimitives
    {
        Geometry m_geometry;

        DXGI_FORMAT m_positionVBFormat;
        RTMaterial *m_pMaterial = NULL;
    };

    struct RTMesh
    {
        std::vector<RTPrimitives> m_pPrimitives;

        //DXR
        Microsoft::WRL::ComPtr<ID3D12Resource> m_scratchUAV;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_blasUAV;

        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> primitiveDescs;
    };

    class GltfRTShadowPass
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
            GLTFResources* pGLTFTexturesAndBuffers,
            DX::DeviceResources* pDeviceResources
        );

        void DoASBuild();
        void CreateRTPipeline();
        void OnDestroy();
        void Draw(ID3D12GraphicsCommandList* pCommandList);

        D3D12_GPU_VIRTUAL_ADDRESS GetTLAS() { return m_tlasUAV->GetGPUVirtualAddress(); };

        ID3D12StateObject* GetRTPSO() { return m_raytracingStateObject.Get(); };

        ID3D12PipelineState* GetInlineRTPipelineState() { return m_inlineRaytracingPipelineState.Get(); };

        ID3D12RootSignature* GetRootSignature() { return m_globalRootSignature.Get(); };

        void PrepareDispatchRaysDesc(D3D12_DISPATCH_RAYS_DESC* desc);

    private:
        ID3D12Device* m_pDevice;
        DX::DeviceResources* m_pDeviceResources;

        // Used for AS Building
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_dxrCommandAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_dxrCommandList; 

        Microsoft::WRL::ComPtr<ID3D12Resource> m_scratchUAV;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_tlasUAV;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_instances;

        // DXR Objects
        Microsoft::WRL::ComPtr<ID3D12StateObject>   m_raytracingStateObject;
        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>   m_raytracingStateObjectProps;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>    m_globalRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_inlineRaytracingPipelineState;

        ShaderBindingTable<SimpleTriangleRecord, 1, 3, 2> m_shaderBindingTable;

        DirectX::EffectTextureFactory* m_texFactory;

        std::vector<RTMesh> m_meshes;
        std::vector<RTMaterial> m_materialsData;

        RTMaterial m_defaultMaterial;

        GLTFResources *m_pGltfResources;
        D3D12_STATIC_SAMPLER_DESC m_samplerDesc;

        std::vector<uint8_t> m_bcVsDefault;
        std::vector<uint8_t> m_bcPsDefault;
    };
}


