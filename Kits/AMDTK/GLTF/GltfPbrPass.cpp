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

#include "pch.h"
#include "GltfPbrPass.h"
#include "GltfHelpers.h" 
#include "ReadData.h"

namespace AMDTK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnCreate(
        ID3D12Device *pDevice,
        GLTFResources* pGltfResources,
        DXGI_FORMAT outForwardFormat,
        DXGI_FORMAT outSpecularRoughnessFormat,
        DXGI_FORMAT outDiffuseColor,
        DXGI_FORMAT outNormals,
        uint32_t sampleCount,
        bool inverseDepth
    ) 
    {
        assert(sampleCount == 1); //msaa not currently supported
        m_pDevice = pDevice;
        m_sampleCount = sampleCount;
        m_pGltfResources = pGltfResources;
        m_inverseDepth = inverseDepth;
        m_doLighting = true;  

        DefineList rtDefines;
        {
            //set bindings for the render targets
            int rtIndex = 0;
            if (outForwardFormat != DXGI_FORMAT_UNKNOWN)
            {
                m_outFormats.push_back(outForwardFormat);
                rtDefines["HAS_FORWARD_RT"] = std::to_string(rtIndex++);
            }
            if (outSpecularRoughnessFormat != DXGI_FORMAT_UNKNOWN)
            {
                m_outFormats.push_back(outSpecularRoughnessFormat);
                rtDefines["HAS_SPECULAR_ROUGHNESS_RT"] = std::to_string(rtIndex++);
            }
            if (outDiffuseColor != DXGI_FORMAT_UNKNOWN)
            {
                m_outFormats.push_back(outDiffuseColor);
                rtDefines["HAS_DIFFUSE_RT"] = std::to_string(rtIndex++);
            }
            if (outNormals != DXGI_FORMAT_UNKNOWN)
            {
                m_outFormats.push_back(outNormals);
                rtDefines["HAS_NORMALS_RT"] = std::to_string(rtIndex++);
            }
        }

        const json &j3 = m_pGltfResources->GetGltfFile()->GetJson();

        // Create default material, this material will be used if none is assigned
        //
        {
            SetDefaultMaterialParamters(&m_defaultMaterial.m_pbrMaterialParameters);
            
            std::map<std::string, ID3D12Resource *> texturesBase;
            CreateDescriptorTableForMaterialTextures(&m_defaultMaterial, texturesBase);
        }

        // Load PBR 2.0 Materials
        //
        const json &materials = j3["materials"];
        m_materialsData.resize(materials.size());
        for (uint32_t i = 0; i < materials.size(); i++)
        {
            PBRMaterial *tfmat = &m_materialsData[i];

            // Get PBR material parameters and texture IDs
            //
            std::map<std::string, unsigned> textureIds;
            ProcessMaterials(materials[i], &tfmat->m_pbrMaterialParameters, textureIds);

            // translate texture IDs into textureViews
            //
            std::map<std::string, ID3D12Resource *> texturesBase;
            for (auto const& value : textureIds)
                texturesBase[value.first] = m_pGltfResources->GetTextureResourceByID(value.second);

            CreateDescriptorTableForMaterialTextures(tfmat, texturesBase);

            //Decrease Refcount after using resources
            for (auto const& value : texturesBase)
                value.second->Release();

        }

        // Load Meshes
        //
        if (j3.find("meshes") != j3.end())
        {
            const json &meshes = j3["meshes"];
            m_meshes.resize(meshes.size());
            for (uint32_t i = 0; i < meshes.size(); i++)
            {
                const json &primitives = meshes[i]["primitives"];

                // Loop through all the primitives (sets of triangles with a same material) and 
                // 1) create an input layout for the geometry
                // 2) then take its material and create a Root descriptor
                // 3) With all the above, create a pipeline
                //
                PBRMesh *tfmesh = &m_meshes[i];
                tfmesh->m_pPrimitives.resize(primitives.size());
                for (uint32_t p = 0; p < primitives.size(); p++)
                {
                    const json &primitive = primitives[p];
                    PBRPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];
                     
                    {
                        // Sets primitive's material, or set a default material if none was specified in the GLTF
                        //
                        auto mat = primitive.find("material");
                        pPrimitive->m_pMaterial = (mat != primitive.end()) ? &m_materialsData[mat.value()] : &m_defaultMaterial;

                        // holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
                        //
                        DefineList defines = pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_defines + rtDefines;

                        // make a list of all the attribute names our pass requires, in the case of PBR we need them all
                        //
                        std::vector<std::string> requiredAttributes;
                        for (auto const & it : primitive["attributes"].items())
                            requiredAttributes.push_back(it.key());

                        // create an input layout from the required attributes
                        // shader's can tell the slots from the #defines
                        //
                        std::vector<std::string> semanticNames;
                        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
                        m_pGltfResources->CreateGeometry(primitive, requiredAttributes, semanticNames, inputLayout, defines, &pPrimitive->m_geometry);

                        // Create the descriptors, the root signature and the pipeline
                        //
                        bool bUsingSkinning = m_pGltfResources->GetGltfFile()->FindMeshSkinId(i) != tfNodeInvalidIndex;
                        CreateDescriptors(bUsingSkinning, defines, pPrimitive);
                        CreatePipeline(inputLayout, defines, pPrimitive);
                       
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptorTableForMaterialTextures
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptorTableForMaterialTextures(PBRMaterial *tfmat, std::map<std::string, ID3D12Resource *> &texturesBase)
    {
        uint32_t cnt = 0;
        
        {
            // count the number of textures to init bindings and descriptor
            {
                tfmat->m_textureCount = (uint32_t)texturesBase.size();

                size_t end;
                if (tfmat->m_textureCount > 0)
                { 
                    m_pGltfResources->GetSrvPile()->AllocateRange(tfmat->m_textureCount, tfmat->m_textureTableHeapOffset, end);
                    tfmat->m_textureTableHandle = m_pGltfResources->GetSrvPile()->GetGpuHandle(tfmat->m_textureTableHeapOffset);
                }
            }


            // Create SRV for the PBR materials
            //
            for (auto const &it : texturesBase)
            {
                tfmat->m_pbrMaterialParameters.m_defines[std::string("ID_") + it.first] = std::to_string(cnt);

                // we need to create an srv using the looked up index from the texture table, to the resource in the texture collection
                auto textureDescriptor = m_pGltfResources->GetSrvPile()->GetCpuHandle(static_cast<size_t>(tfmat->m_textureTableHeapOffset + cnt));
                DirectX::CreateShaderResourceView(m_pDevice, it.second, textureDescriptor, false);

                CreateSamplerForPBR(cnt, &tfmat->m_samplers[cnt]);

                cnt++;
            }
            
        }
            
        // the SRVs for the shadows is provided externally, here we just create the #defines for the shader bindings
        //
        {
            assert(cnt <= 9);   // 10th slot is reserved for shadow buffer
            tfmat->m_pbrMaterialParameters.m_defines["ID_shadowMap"] = std::to_string(9);
            CreateSamplerForShadowMap(9, &tfmat->m_samplers[cnt]);
        }        
    } 

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::OnDestroy()
    {
        for (uint32_t m = 0; m < m_meshes.size(); m++)
        {
            PBRMesh *pMesh = &m_meshes[m];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];
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

    //--------------------------------------------------------------------------------------
    //
    // CreateDescriptors for a combination of material and geometry
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::CreateDescriptors(bool bUsingSkinning, DefineList &defines, PBRPrimitives *pPrimitive)
    {              
        uint32_t rootParamCnt = 0;
        CD3DX12_ROOT_PARAMETER rootParameter[6];
        uint32_t desccRangeCnt = 0;
        CD3DX12_DESCRIPTOR_RANGE descRange[3];

        // b0 <- Constant buffer 'per frame'
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
            rootParamCnt++;
        }

        // textures table
        if (pPrimitive->m_pMaterial->m_textureCount > 0)
        {
            descRange[desccRangeCnt].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, pPrimitive->m_pMaterial->m_textureCount, 0); // texture table
            rootParameter[rootParamCnt].InitAsDescriptorTable(1, &descRange[desccRangeCnt], D3D12_SHADER_VISIBILITY_PIXEL);
            desccRangeCnt++;
            rootParamCnt++;
        }

        // shadow buffer (only if we are doing lighting, for example in the forward pass)

        if (m_doLighting)
        {
            descRange[desccRangeCnt].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9);                                       // shadow buffer
            rootParameter[rootParamCnt].InitAsDescriptorTable(1, &descRange[desccRangeCnt], D3D12_SHADER_VISIBILITY_PIXEL);
            desccRangeCnt++;
            rootParamCnt++;
        }

        // b1 <- Constant buffer 'per object', these are mainly the material data
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
            rootParamCnt++;
        }

        // b2 <- Constant buffer holding the skinning matrices
        if (bUsingSkinning)
        {
            rootParameter[rootParamCnt].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            defines["ID_SKINNING_MATRICES"] = std::to_string(2);
            rootParamCnt++;
        }

        // the root signature contains up to 5 slots to be used
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC();
        descRootSignature.pParameters = rootParameter;
        descRootSignature.NumParameters = rootParamCnt;
        descRootSignature.pStaticSamplers = pPrimitive->m_pMaterial->m_samplers;
        descRootSignature.NumStaticSamplers = pPrimitive->m_pMaterial->m_textureCount;
        if (m_doLighting)
        {
            descRootSignature.NumStaticSamplers += 1;   // account for shadow sampler
        }

        // deny uneccessary access to certain pipeline stages
        descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
            | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ID3DBlob *pOutBlob, *pErrorBlob = NULL;
        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));

        DX::ThrowIfFailed(m_pDevice->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(pPrimitive->m_rootSignature.ReleaseAndGetAddressOf())));

        DirectX::SetDebugObjectName(pPrimitive->m_rootSignature.Get(), "GltfPbr::m_RootSignature");

        pOutBlob->Release();
        if (pErrorBlob)
            pErrorBlob->Release();

    }

    //--------------------------------------------------------------------------------------
    //
    // CreatePipeline
    //
    //--------------------------------------------------------------------------------------
    bool IsIdChannel(const DefineList &defines, const char* IDname, const char channel)
    {
        // search if any *TexCoord mentions this channel
        //

        char id = channel;

        for (auto def : defines)
        {
            if (!def.first.compare(IDname))
            {
                if (id == def.second.c_str()[0])
                {
                    return true;
                }
            }
        }
        return false;
    }

    void GltfPbrPass::CreatePipeline(std::vector<D3D12_INPUT_ELEMENT_DESC> layout, const DefineList &defines, PBRPrimitives *pPrimitive)
    {
        // Compile and create shaders
        //
        uint32_t hash = static_cast<uint32_t>(defines.Hash() & 0xFFFFFFFF);

        if (m_psoCache.find(hash) != m_psoCache.end())
        {
            pPrimitive->m_pso = m_psoCache[hash].m_pso;
            pPrimitive->m_rootSignature = m_psoCache[hash].m_rootSignature;
            return;
        }

        D3D12_SHADER_BYTECODE shaderVert, shaderPixel;
        bool shadernotfound = false;

        wchar_t hashbufferVS[128];
        swprintf(hashbufferVS, 128, L"GLTFPbrPass-VS_Permutation_0x%08x.cso", hash);

        std::vector<uint8_t> shaderVS, shaderPS;
        try 
        {
            shaderVS = DX::ReadData(hashbufferVS);
        }
        catch (std::exception e)
        {
            wchar_t buff[128];
            swprintf(buff, 128, L"Precompiled VS permutation 0x%08x not found.\n", hash);
            OutputDebugString(buff);
            shadernotfound = true;
        }
         

        wchar_t hashbufferPS[128];
        swprintf(hashbufferPS, 128, L"GLTFPbrPass-PS_Permutation_0x%08x.cso", hash);

        try 
        {
            shaderPS = DX::ReadData(hashbufferPS);
        }
        catch (std::exception e)
        {
            wchar_t buff[128];
            swprintf(buff, 128, L"Precompiled PS permutation 0x%08x not found.\n", hash);
            OutputDebugString(buff);
            shadernotfound = true;
        }

        static std::map<size_t, size_t> shaderperms;

        if (shadernotfound  && (shaderperms.find(hash) == shaderperms.end()))
        {
             shaderperms[hash] = 1;
            // new permutation - print out the defines required, so we can add a precompiled Permutation
            wchar_t buff[128];
            swprintf(buff, 128, L"//compile, 0x%08x \n", hash);
            OutputDebugString(buff);
            for (auto it = defines.begin(); it != defines.end(); it++)
            {
                swprintf(buff, 128, L"#define %S %S \n", it->first.c_str(), it->second.c_str());
                OutputDebugString(buff);
            }

            OutputDebugString(L"\n");
        }

        shaderVert = { shaderVS.data(), shaderVS.size() };
        shaderPixel = { shaderPS.data(), shaderPS.size() };
        
        assert((shaderVS.size()>0) && "Permutation must be populated");

        // Set blending
        //
        CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
        {
            (defines.Has("DEF_alphaMode_BLEND")),
            FALSE,
            D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        }; 

        // Create a PSO description
        //
        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
        descPso.InputLayout = { layout.data(), (UINT)layout.size() };
        descPso.pRootSignature = pPrimitive->m_rootSignature.Get(); 
        descPso.VS = shaderVert;
        descPso.PS = shaderPixel;
        descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descPso.RasterizerState.CullMode = (pPrimitive->m_pMaterial->m_pbrMaterialParameters.m_doubleSided) ? D3D12_CULL_MODE_NONE : D3D12_CULL_MODE_FRONT;
        descPso.BlendState = blendState;
        descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descPso.DepthStencilState.DepthFunc = m_inverseDepth ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS;
        descPso.SampleMask = UINT_MAX;
        descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPso.NumRenderTargets = (UINT)m_outFormats.size();
        for (size_t i = 0; i < m_outFormats.size(); i++)
        {
            descPso.RTVFormats[i] = m_outFormats[i];
        }
        descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        descPso.SampleDesc.Count = m_sampleCount;
        descPso.NodeMask = 0;

        DX::ThrowIfFailed(
            m_pDevice->CreateGraphicsPipelineState(&descPso, IID_GRAPHICS_PPV_ARGS(pPrimitive->m_pso.ReleaseAndGetAddressOf()))
        );

        wchar_t namebuffer[128];
        swprintf(namebuffer, 128, L"GltfPbrPass::m_pso Permutation 0x%08x", hash);
        pPrimitive->m_pso.Get()->SetName(&namebuffer[0]);

        //cache this pso
        CachedPSO entry;
        entry.m_pso = pPrimitive->m_pso;
        entry.m_rootSignature = pPrimitive->m_rootSignature;
        m_psoCache[hash] = entry;
    }


    //
    // Frustrum culls an AABB. The culling is done in clip space. 
    //
    bool GltfPbrPass::FrustumCulled(const DirectX::XMMATRIX mCameraViewProj, const DirectX::XMVECTOR center, const DirectX::XMVECTOR extent)
    {
        float ex = DirectX::XMVectorGetX(extent);
        float ey = DirectX::XMVectorGetY(extent);
        float ez = DirectX::XMVectorGetZ(extent);

        DirectX::XMVECTOR p[8];
        p[0] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(ex, ey, ez, 0)), mCameraViewProj);
        p[1] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(ex, ey, -ez, 0)), mCameraViewProj);
        p[2] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(ex, -ey, ez, 0)), mCameraViewProj);
        p[3] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(ex, -ey, -ez, 0)), mCameraViewProj);
        p[4] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(-ex, ey, ez, 0)), mCameraViewProj);
        p[5] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(-ex, ey, -ez, 0)), mCameraViewProj);
        p[6] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(-ex, -ey, ez, 0)), mCameraViewProj);
        p[7] = DirectX::XMVector4Transform(DirectX::XMVectorAdd(center, DirectX::XMVectorSet(-ex, -ey, -ez, 0)), mCameraViewProj);

        uint32_t left = 0;
        uint32_t right = 0;
        uint32_t top = 0;
        uint32_t bottom = 0;
        uint32_t back = 0;
        for (int i = 0; i < 8; i++)
        {
            float x = DirectX::XMVectorGetX(p[i]);
            float y = DirectX::XMVectorGetY(p[i]);
            float z = DirectX::XMVectorGetZ(p[i]);
            float w = DirectX::XMVectorGetW(p[i]);

            if (x < -w) left++;
            if (x > w) right++;
            if (y < -w) bottom++;
            if (y > w) top++;
            if (z < 0) back++;
        }

        return left == 8 || right == 8 || top == 8 || bottom == 8 || back == 8;
    }


    //--------------------------------------------------------------------------------------
    //
    // BuildLists
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::BuildLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent)
    {
        // loop through nodes
        //
        std::vector<tfNode> *pNodes = m_pGltfResources->GetGltfFile()->GetNodes();
        const DirectX::XMMATRIX *pNodesMatrices = m_pGltfResources->GetGltfFile()->GetCurrentFrameTransformedData()->worldSpaceMats.data();

        for (uint32_t i = 0; i < pNodes->size(); i++)
        {
            tfNode *pNode = &pNodes->at(i);
            if ((pNode == NULL) || (pNode->meshIndex == tfNodeInvalidIndex))
                continue;

            // skinning matrices constant buffer
            D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton = m_pGltfResources->GetSkinningMatricesBuffer(pNode->skinIndex);

            DirectX::XMMATRIX mModelViewProj = pNodesMatrices[i] * m_pGltfResources->GetGltfFile()->GetCommonPerFrameData().mCameraViewProj;

            // loop through primitives
            //
            PBRMesh *pMesh = &m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                PBRPrimitives *pPrimitive = &pMesh->m_pPrimitives[p];

                if (pPrimitive->m_pso == NULL)
                    continue;

                // do frustrum culling
                //
                tfPrimitives boundingBox = m_pGltfResources->GetGltfFile()->GetMeshes()->at(pNode->meshIndex).m_pPrimitives[p];
                if (FrustumCulled(mModelViewProj, boundingBox.m_center, boundingBox.m_radius))
                   continue;

                PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMaterial->m_pbrMaterialParameters;

                // Set per Object constants from material
                //
                per_object cbPerObject;
                cbPerObject.mWorld = pNodesMatrices[i];
                cbPerObject.m_pbrParams = pPbrParams->m_params;

                // compute depth for sorting
                //                
                DirectX::XMVECTOR v = m_pGltfResources->GetGltfFile()->GetMeshes()->at(pNode->meshIndex).m_pPrimitives[p].m_center;
                float depth = DirectX::XMVectorGetW(XMVector4Transform(v, mModelViewProj));

                BatchList t;
                t.m_depth = depth;
                t.m_pPrimitive = pPrimitive;
                t.m_perFrameDesc = m_pGltfResources->GetPerFrameConstants();
                t.m_perObjectDesc = m_pGltfResources->GetGPUMemory()->AllocateConstant(cbPerObject).GpuAddress();
                t.m_pPerSkeleton = pPerSkeleton;

                // append primitive to list 
                //
                if (pPbrParams->m_blending == false)
                {
                    pSolid->push_back(t);
                }
                else
                {
                    pTransparent->push_back(t);
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfPbrPass::Draw(ID3D12GraphicsCommandList *pCommandList, D3D12_GPU_DESCRIPTOR_HANDLE shadowAtlas)
    {

        // Get list of opaque and transparent primitives
        //
        std::vector<BatchList> solid, transparent;
        BuildLists(&solid, &transparent);

        // Set descriptor heaps
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12DescriptorHeap *pDescriptorHeaps[] = { m_pGltfResources->GetSrvPile()->Heap() };

        pCommandList->SetDescriptorHeaps(1, pDescriptorHeaps);

        // draw solid primitives
        //        
        {
            std::sort(solid.begin(), solid.end(), [](const BatchList & a, const BatchList & b) -> bool { return a.m_pPrimitive->m_pso.Get() > b.m_pPrimitive->m_pso.Get(); });
            for (auto &t : solid)
            {
                t.m_pPrimitive->DrawPrimitive(pCommandList, shadowAtlas, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton);
            }
        }

        // Sort transparent primitives and draw them
        //
        {
            std::sort(transparent.begin(), transparent.end());
            for (auto &t : transparent)
            {
                t.m_pPrimitive->DrawPrimitive(pCommandList, shadowAtlas, t.m_perFrameDesc, t.m_perObjectDesc, t.m_pPerSkeleton);
            }
        }
    }

    void PBRPrimitives::DrawPrimitive(ID3D12GraphicsCommandList *pCommandList, D3D12_GPU_DESCRIPTOR_HANDLE shadowAtlas, D3D12_GPU_VIRTUAL_ADDRESS perFrameDesc, D3D12_GPU_VIRTUAL_ADDRESS perObjectDesc, D3D12_GPU_VIRTUAL_ADDRESS pPerSkeleton)
    {
        // Bind indices and vertices using the right offsets into the buffer
        //
        pCommandList->IASetIndexBuffer(&m_geometry.m_IBV);
        pCommandList->IASetVertexBuffers(0, (UINT)m_geometry.m_VBV.size(), m_geometry.m_VBV.data());

        // Bind Descriptor sets
        //
        pCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
        uint32_t paramIndex = 0;

        // bind the per scene constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perFrameDesc);

        // bind the textures and samplers descriptors
        if (m_pMaterial->m_textureCount > 0)
        {
            pCommandList->SetGraphicsRootDescriptorTable(paramIndex++,m_pMaterial->m_textureTableHandle);
        }

        // bind the shadow buffer
        if (shadowAtlas.ptr != 0)
        {
            pCommandList->SetGraphicsRootDescriptorTable(paramIndex++, shadowAtlas);
        }
        else
        {
            paramIndex++;
        }

        // bind the per object constant buffer descriptor
        pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, perObjectDesc);

        // bind the skeleton bind matrices constant buffer descriptor
        if (pPerSkeleton != 0)
            pCommandList->SetGraphicsRootConstantBufferView(paramIndex++, pPerSkeleton);

        // Bind Pipeline
        //
        pCommandList->SetPipelineState(m_pso.Get());

        // Draw
        //
        pCommandList->DrawIndexedInstanced(m_geometry.m_NumIndices, 1, 0, 0, 0);
    }
}
