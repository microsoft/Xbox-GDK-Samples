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

#include "pch.h"
#include "GltfRTShadowPass.h"
#include "GltfHelpers.h" 
#include "ReadData.h"

#include "CompiledShaders/GlobalRootSignature.inc"
#include "CompiledShaders/RaytracingLibrary.inc"
#include "CompiledShaders/InlineRaytracing.inc"

namespace AMDTK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfRTShadowPass::OnCreate(
        ID3D12Device* pDevice,
        GLTFResources* pGltfResources,
        DX::DeviceResources* pDeviceResources)
    {
        m_pDevice = pDevice; 
        m_pGltfResources = pGltfResources;
        m_pDeviceResources = pDeviceResources;

        const json &j3 = pGltfResources->GetGltfFile()->GetJson();

        /////////////////////////////////////////////
        // Create default material

        m_defaultMaterial.m_textureCount = 0;
        m_defaultMaterial.m_doubleSided = false;

        // Create static sampler in case there is transparency
        //
        ZeroMemory(&m_samplerDesc, sizeof(m_samplerDesc));
        m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        m_samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        m_samplerDesc.MinLOD = 0.0f;
        m_samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        m_samplerDesc.MipLODBias = 0;
        m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        m_samplerDesc.MaxAnisotropy = 1;
        m_samplerDesc.ShaderRegister = 0;
        m_samplerDesc.RegisterSpace = 0;
        m_samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Create materials (in a depth pass materials are still needed to handle non opaque textures
        //
        if (j3.find("materials") != j3.end())
        {
            const json &materials = j3["materials"];

            m_materialsData.resize(materials.size());
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                const json &material = materials[i];

                RTMaterial *tfmat = &m_materialsData[i];

                // Load material constants. This is a depth pass and we are only interested in the mask texture
                //               
                tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
                std::string alphaMode = GetElementString(material, "alphaMode", "OPAQUE");
                tfmat->m_defines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

                // If transparent use the baseColorTexture for alpha
                //
                if (alphaMode == "MASK")
                {
                    tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));

                    auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
                    if (pbrMetallicRoughnessIt != material.end())
                    {
                        const json &pbrMetallicRoughness = pbrMetallicRoughnessIt.value();

                        auto id = GetElementUnsignedInt(pbrMetallicRoughness, "baseColorTexture/index", tfNodeInvalidIndex);
                        if (id != tfNodeInvalidIndex)
                        {
                            tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";

                            tfmat->m_textureCount = 1;
                            size_t offset = pGltfResources->GetSrvPile()->Allocate();
                            tfmat->m_pTransparency = pGltfResources->GetSrvPile()->GetGpuHandle(offset);

                            auto textureDescriptor = pGltfResources->GetSrvPile()->GetCpuHandle(offset);

                            ID3D12Resource* res = m_pGltfResources->GetTextureResourceByID(id);
                            DirectX::CreateShaderResourceView(m_pDevice, res, textureDescriptor, false);

                            //Decrease Refcount after using resources
                            res->Release();

                            tfmat->m_defines["ID_baseColorTexture"] = "0";
                            tfmat->m_defines["ID_baseTexCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "baseColorTexture/texCoord", 0));
                        }
                    }
                }
            }
        }

       
        DX::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_dxrCommandAlloc.ReleaseAndGetAddressOf())));

        m_dxrCommandAlloc->SetName(L"RTShadowPassDXR Command Alloc");

        DX::ThrowIfFailed(pDevice->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, m_dxrCommandAlloc.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_dxrCommandList.ReleaseAndGetAddressOf())));

        m_dxrCommandList->SetName(L"RTShadowPassDXR Command List");
        DX::ThrowIfFailed(m_dxrCommandList->Close());

        // Load Meshes
        //
        if (j3.find("meshes") != j3.end())
        {
            const json::array_t &gltfAccessors = j3["accessors"];
            const json &meshes = j3["meshes"];
            m_meshes.resize(meshes.size());
            for (uint32_t i = 0; i < meshes.size(); i++)
            {
                RTMesh *tfmesh = &m_meshes[i];

                const json &primitives = meshes[i]["primitives"];
                tfmesh->m_pPrimitives.resize(primitives.size());

                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    const json &primitive = primitives[p];
                    RTPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];
                    {
                        //// From Shadow SDK

                        // Gets the geometry topology
                        int32_t mode = GetElementInt(primitive, "mode", 4);
                        if (mode != 4)
                        {
                            continue;   // not supported
                        }

                        // Get hold of the vertex position stream
                        std::vector<tfAccessor> attributes(1);
                        const json::object_t &gltfAttributes = primitive.at("attributes");
                        for (auto it = gltfAttributes.begin(); it != gltfAttributes.end(); ++it)
                        {
                            const json::object_t &accessor = gltfAccessors[it->second];

                            if (it->first == "POSITION")
                            {
                                // Get VB accessors
                                pGltfResources->GetGltfFile()->GetBufferDetails(it->second, &attributes[0]);
                                pPrimitive->m_positionVBFormat = GetFormat(accessor.at("type"), accessor.at("componentType"));
                            }
                        }

                        if (!attributes[0].m_data)
                        {
                            continue;   // not a valid primitive
                        }

                        // Get index buffer accessor and create the geometry
                        tfAccessor indexBuffer;
                        pGltfResources->GetGltfFile()->GetBufferDetails(primitive.at("indices"), &indexBuffer);
                        pGltfResources->CreateGeometry(indexBuffer, attributes, &pPrimitive->m_geometry);

                        //Create BLAS desc
                        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
                        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                        geometryDesc.Triangles.IndexBuffer = pPrimitive->m_geometry.m_IBV.BufferLocation;
                        geometryDesc.Triangles.IndexCount = indexBuffer.m_count;
                        geometryDesc.Triangles.IndexFormat = (indexBuffer.m_stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                        geometryDesc.Triangles.Transform3x4 = 0;
                        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                        geometryDesc.Triangles.VertexCount = attributes[0].m_count;
                        geometryDesc.Triangles.VertexBuffer.StartAddress = pPrimitive->m_geometry.m_VBV[0].BufferLocation;
                        geometryDesc.Triangles.VertexBuffer.StrideInBytes = attributes[0].m_stride;
                        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

                        tfmesh->primitiveDescs.push_back(geometryDesc);
                    }
                }
            }
        }

        CreateRTPipeline();
    }

    /***
     Static AS - Does not work with animation!
    */
    void GltfRTShadowPass::DoASBuild()
    {
        m_dxrCommandList->Reset(m_dxrCommandAlloc.Get(), nullptr);

        // Grab a DXR-capable device interface
        ID3D12Device5* pDxrDevice;

        DX::ThrowIfFailed(m_pDevice->QueryInterface(IID_GRAPHICS_PPV_ARGS(&pDxrDevice)));

        std::vector< D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;

        CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        for (uint32_t i = 0; i < m_meshes.size(); i++)
        {
            RTMesh *rtmesh = &m_meshes[i];
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
            bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            bottomLevelInputs.NumDescs = rtmesh->primitiveDescs.size() >= UINT_MAX ? UINT_MAX : static_cast<UINT>(rtmesh->primitiveDescs.size());
            bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            bottomLevelInputs.pGeometryDescs = &rtmesh->primitiveDescs[0];

            pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);
            assert(prebuildInfo.ResultDataMaxSizeInBytes > 0);

            CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(
                pDxrDevice->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &scratchDesc,
#ifdef _GAMING_XBOX
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
#else
                    // on PC buffers are created in the common state and can be promoted without a barrier to UNORDERED_ACCESS on first access
                    D3D12_RESOURCE_STATE_COMMON,
#endif
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(rtmesh->m_scratchUAV.ReleaseAndGetAddressOf())
                )
            );
            rtmesh->m_scratchUAV->SetName(L"Mesh BLAS scratch");

            CD3DX12_RESOURCE_DESC dataDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(
                pDxrDevice->CreateCommittedResource(
                    &heapProperties, D3D12_HEAP_FLAG_NONE, &dataDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_GRAPHICS_PPV_ARGS(rtmesh->m_blasUAV.ReleaseAndGetAddressOf())
                )
            );
            rtmesh->m_blasUAV->SetName(L"Mesh BLAS data");

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
            bottomLevelBuildDesc.Inputs = bottomLevelInputs;
            bottomLevelBuildDesc.ScratchAccelerationStructureData = rtmesh->m_scratchUAV->GetGPUVirtualAddress();
            bottomLevelBuildDesc.DestAccelerationStructureData = rtmesh->m_blasUAV->GetGPUVirtualAddress();

            m_dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

            auto blasUAVBarrier{ CD3DX12_RESOURCE_BARRIER::UAV( rtmesh->m_blasUAV.Get() ) };
            m_dxrCommandList->ResourceBarrier(1, &blasUAVBarrier);
        }

        // per-mesh blas created, now we can traverse nodes and create instances for the tlas
        // Set an initial transform state
        m_pGltfResources->GetGltfFile()->TransformScene(0, DirectX::XMMatrixScaling(-1, 1, 1));

        // loop through nodes
        //
        std::vector<tfNode> *pNodes = m_pGltfResources->GetGltfFile()->GetNodes();
        const DirectX::XMMATRIX *pNodesMatrices = m_pGltfResources->GetGltfFile()->GetCurrentFrameTransformedData()->worldSpaceMats.data();

        for (uint32_t i = 0; i < pNodes->size(); i++)
        {
            tfNode *pNode = &pNodes->at(i);
            if ((pNode == NULL) || (pNode->meshIndex == tfNodeInvalidIndex))
                continue;

            RTMesh* rtmesh = &m_meshes[pNode->meshIndex];

                DirectX::XMMATRIX world = DirectX::XMMatrixTranspose(pNodesMatrices[i]);//

                D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

                for (auto row = 0u; row < 3u; ++row)
                {
                    instanceDesc.Transform[row][0] = DirectX::XMVectorGetX(world.r[row]);
                    instanceDesc.Transform[row][1] = DirectX::XMVectorGetY(world.r[row]);
                    instanceDesc.Transform[row][2] = DirectX::XMVectorGetZ(world.r[row]);
                }

                for (auto k = 0u; k < 3u; ++k)
                {
                    instanceDesc.Transform[k][3u] = DirectX::XMVectorGetW(world.r[k]);
                }

                instanceDesc.InstanceMask = 0xFFu;
                instanceDesc.InstanceID = i;
                instanceDesc.AccelerationStructure = rtmesh->m_blasUAV->GetGPUVirtualAddress();

                instanceDescs.push_back(instanceDesc);
        }

        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size());
        DX::ThrowIfFailed(pDxrDevice->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_instances.ReleaseAndGetAddressOf())));

            m_instances->SetName(L"Scene TLAS Instances");
      
        void *pMappedData;
        m_instances->Map(0, nullptr, &pMappedData);
        memcpy(pMappedData, &instanceDescs[0], sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size());
        m_instances->Unmap(0, nullptr);

        // Top Level Acceleration Structure desc
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
        
        topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; 
        topLevelBuildDesc.Inputs.InstanceDescs = m_instances->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs.NumDescs = instanceDescs.size() >= UINT_MAX ? UINT_MAX : static_cast<UINT>(instanceDescs.size());
        topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; 
             
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
        pDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelBuildDesc.Inputs, &topLevelPrebuildInfo);
        assert(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

        CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            pDxrDevice->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &scratchDesc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to UNORDERED_ACCESS on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_scratchUAV.ReleaseAndGetAddressOf())
            )
        );
        m_scratchUAV->SetName(L"Scene TLAS scratch");

        CD3DX12_RESOURCE_DESC dataDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            pDxrDevice->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &dataDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_GRAPHICS_PPV_ARGS( m_tlasUAV.ReleaseAndGetAddressOf())
            )
        );
        m_tlasUAV->SetName(L"Scene TLAS data");

        topLevelBuildDesc.DestAccelerationStructureData = m_tlasUAV->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = m_scratchUAV->GetGPUVirtualAddress();

        m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

        DX::ThrowIfFailed(m_dxrCommandList->Close());
        ID3D12CommandList *commandLists[] = { m_dxrCommandList.Get() };
        m_pDeviceResources->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

        pDxrDevice->Release();
    }

    void GltfRTShadowPass::CreateRTPipeline()
    {
        // Grab a DXR-capable device interface
        ID3D12Device5* pDxrDevice;

        DX::ThrowIfFailed(m_pDevice->QueryInterface(IID_GRAPHICS_PPV_ARGS(&pDxrDevice)));

        CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

        auto raytracingLibrary = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        D3D12_SHADER_BYTECODE libraryDXIL = CD3DX12_SHADER_BYTECODE((void *)g_RaytracingLibrary, sizeof(g_RaytracingLibrary));
        raytracingLibrary->SetDXILLibrary(&libraryDXIL);

        const wchar_t* rayGenExportName = L"RayGenerationShader";
        const wchar_t* missShaderShadowsExportName = L"MissShaderShadowRay";
        const wchar_t* hitGroupShadowExportName = L"HitGroupShadow";

        {
            auto hitGroupShadow = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitGroupShadow->SetHitGroupExport(hitGroupShadowExportName);
            hitGroupShadow->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
            hitGroupShadow->SetClosestHitShaderImport(L"ClosestHitShadowShader");
            hitGroupShadow->SetAnyHitShaderImport(L"AnyHitShadowShader");
        }

        // Configure the shaders and pipeline
        {
            auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
            const UINT maxPayloadSize = 4;
            const UINT maxAttributeSize = 8;
            shaderConfig->Config(maxPayloadSize, maxAttributeSize);

            auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
            const UINT maxTraceRayRecursion = 2;
            pipelineConfig->Config(maxTraceRayRecursion);
        }

        // Create Global Root Signature
        {
            // It is not currently possible to specify the D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING flag in HLSL, so this must be created in C++ with that flag set.
            // To make that process simpler, we'll just deserialize the one we have, add the flag and create it again.
            Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> rootSigDeserializer;
            DX::ThrowIfFailed(D3D12CreateVersionedRootSignatureDeserializer(g_GlobalRootSignature, sizeof(g_GlobalRootSignature), IID_GRAPHICS_PPV_ARGS(rootSigDeserializer.GetAddressOf())));

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = *(rootSigDeserializer->GetUnconvertedRootSignatureDesc());

#ifdef _GAMING_XBOX_SCARLETT
            rsDesc.Desc_1_1.Flags |= D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING;
#endif

            Microsoft::WRL::ComPtr<ID3DBlob> mainBlob, errorBlob;
            DX::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rsDesc, mainBlob.GetAddressOf(), errorBlob.GetAddressOf()));
            DX::ThrowIfFailed(pDxrDevice->CreateRootSignature(0, mainBlob->GetBufferPointer(), mainBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(m_globalRootSignature.GetAddressOf())));
            m_globalRootSignature->SetName(L"GlobalRootSignature");

            auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            globalRootSignature->SetRootSignature(m_globalRootSignature.Get());
        }

        DX::ThrowIfFailed(pDxrDevice->CreateStateObject(raytracingPipeline, IID_GRAPHICS_PPV_ARGS(m_raytracingStateObject.GetAddressOf())));
        DX::ThrowIfFailed(m_raytracingStateObject->QueryInterface(IID_GRAPHICS_PPV_ARGS(m_raytracingStateObjectProps.GetAddressOf())));

        SimpleTriangleRecord rayGenRecord(m_raytracingStateObjectProps.Get(), rayGenExportName);
        SimpleTriangleRecord emptyMissShader;
        SimpleTriangleRecord validMissShadowShader(m_raytracingStateObjectProps.Get(), missShaderShadowsExportName);
        SimpleTriangleRecord hitGroupShadowRecord(m_raytracingStateObjectProps.Get(), hitGroupShadowExportName);

        m_shaderBindingTable.SetRayGenRecord(0, rayGenRecord);
        m_shaderBindingTable.SetMissShaderRecord(0, validMissShadowShader);
        m_shaderBindingTable.SetHitGroupRecord(0, hitGroupShadowRecord);

        // Create Inline Raytracing PSO
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
            desc.pRootSignature = m_globalRootSignature.Get();
            desc.CS = { g_InlineRaytracing, sizeof(g_InlineRaytracing) };
            DX::ThrowIfFailed(pDxrDevice->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_inlineRaytracingPipelineState.ReleaseAndGetAddressOf())));

            m_inlineRaytracingPipelineState->SetName(L"InlineRaytracingPSO");
        }

        pDxrDevice->Release();
    }


    //--------------------------------------------------------------------------------------
    //
    // PrepareDispatchRaysDesc 
    //
    //--------------------------------------------------------------------------------------
    void GltfRTShadowPass::PrepareDispatchRaysDesc(D3D12_DISPATCH_RAYS_DESC* desc)
    {
        assert(desc != nullptr);

        m_shaderBindingTable.Commit();

        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

        desc->Depth = 1;
        desc->RayGenerationShaderRecord = m_shaderBindingTable.GetRayGenerationRecord(0);
        desc->MissShaderTable = m_shaderBindingTable.GetMissShaderTable();
        desc->HitGroupTable = m_shaderBindingTable.GetHitGroupShaderTable();
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy 
    //
    //--------------------------------------------------------------------------------------
    void GltfRTShadowPass::OnDestroy()
    {
        for (size_t m = 0; m < m_meshes.size(); m++)
        {
            RTMesh *pMesh = &m_meshes[m]; 

            for (size_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                RTPrimitives *pPrimitive = &pMesh->m_pPrimitives[p]; 
                pPrimitive->m_geometry.indexBuffer.Reset();
                for (auto& entry : pPrimitive->m_geometry.m_vertexBuffers)
                { 
                    entry.vertexBuffer.Reset();
                }
            }

            pMesh->m_pPrimitives.clear();
        }
        m_meshes.clear();
    }
}
