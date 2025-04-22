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

#ifdef __clang__
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#endif

#include "pch.h" 
#include "GltfHelpers.h"
#include "ShaderCompiler.h" 
#include "GltfResources.h"
#include "GltfPbrMaterial.h"

#include <codecvt>
#include <string>

class DefineList;

namespace AMDTK
{
    bool GLTFResources::OnCreate(ID3D12Device *pDevice, GLTFFile *pGltfFile, DirectX::ResourceUploadBatch *pUpload, DirectX::GraphicsMemory* gpumem)
    {
        m_pDevice = pDevice;
        m_pGltfFile = pGltfFile;
        m_pUpload = pUpload; 
        m_graphicsMemory = gpumem;
        return true;
    }

    void GLTFResources::FindTextures()
    {
        m_textureNames.clear();

        // load texture names
        if (m_pGltfFile->j3.find("images") != m_pGltfFile->j3.end())
        {
            m_pTextureNodes = &m_pGltfFile->j3["textures"];
            const json &images = m_pGltfFile->j3["images"];
            const json &materials = m_pGltfFile->j3["materials"];

            m_textureNames.resize(images.size());
            m_textureConfigs.resize(images.size());
            for (uint64_t imageIndex = 0; imageIndex < images.size(); imageIndex++)
            {
                std::string filename = images[imageIndex]["uri"];

                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

                TextureConfig tConfig;
                GetSrgbAndCutOffOfImageGivenItsUse(static_cast<int>(imageIndex), materials, &tConfig.useSRGB, &tConfig.cutOff);

                m_textureConfigs[imageIndex] = tConfig;
                m_textureNames[imageIndex] = converter.from_bytes(filename);
            }
        }
    }

    int GLTFResources::LoadTextures(const wchar_t* path, int destinationDescriptorOffset)
    {
        FindTextures();

        m_srvPile = std::make_unique<DirectX::DescriptorPile>(m_pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            512);

        m_gltfTexResources = std::make_unique<DirectX::EffectTextureFactory>(m_pDevice, *m_pUpload, m_srvPile->Heap());
        m_gltfTexResources->SetDirectory(path);

        for (size_t i = 0; i < m_textureNames.size(); ++i)
        {
            m_gltfTexResources->EnableForceSRGB(m_textureConfigs[i].useSRGB);
            m_gltfTexResources->EnableAutoGenMips(true);
            m_gltfTexResources->CreateTexture(m_textureNames[i].c_str(), destinationDescriptorOffset + static_cast<int>(i));
        }

        return static_cast<int>(m_textureNames.size());
    }

    void GLTFResources::OnDestroy()
    {
        m_gltfTexResources->ReleaseCache();
        m_gltfTexResources.reset();
    }

    ID3D12Resource* GLTFResources::GetTextureResourceByID(uint32_t id)
    {
        uint32_t tex = m_pTextureNodes->at(id)["source"];

        ID3D12Resource* resource;
        m_gltfTexResources->GetResource(tex, &resource);

        return resource;
    }

    // Creates a Index Buffer from the accessor
    //
    void GLTFResources::CreateIndexBuffer(tfAccessor indexBuffer, Geometry *pGeometry)
    {
        pGeometry->m_NumIndices = indexBuffer.m_count;
        pGeometry->m_indexType = (indexBuffer.m_stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        uint32_t indexBufferSize = static_cast<uint32_t>(indexBuffer.m_count * indexBuffer.m_stride);
        pGeometry->indexBuffer = DirectX::GraphicsMemory::Get(m_pDevice).Allocate(indexBufferSize);
        memcpy(pGeometry->indexBuffer.Memory(), indexBuffer.m_data, indexBufferSize);
    
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        DX::ThrowIfFailed(m_pDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_COPY_DEST,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(pGeometry->staticIndexBuffer.GetAddressOf())
        ));

        DirectX::SetDebugObjectName(pGeometry->staticIndexBuffer.Get(), L"GltfIndexBuffer");

        m_pUpload->Upload(pGeometry->staticIndexBuffer.Get(), pGeometry->indexBuffer);

        m_pUpload->Transition(pGeometry->staticIndexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
         
        pGeometry->m_IBV.BufferLocation = pGeometry->staticIndexBuffer->GetGPUVirtualAddress();
        pGeometry->m_IBV.SizeInBytes = indexBufferSize;
        pGeometry->m_IBV.Format = pGeometry->m_indexType;
    }


    void GLTFResources::CreateVertexBuffer(tfAccessor* vertexBuffer, uint32_t idx, Geometry *pGeometry)
    {
        GeometryVbuff& vbuffer = pGeometry->m_vertexBuffers[idx];

        uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer->m_count * vertexBuffer->m_stride);
        vbuffer.vertexBuffer = DirectX::GraphicsMemory::Get(m_pDevice).Allocate(vertexBufferSize);
        memcpy(vbuffer.vertexBuffer.Memory(), vertexBuffer->m_data, vertexBufferSize);


        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        DX::ThrowIfFailed(m_pDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_COPY_DEST,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(vbuffer.staticVertexBuffer.GetAddressOf())
        ));

        DirectX::SetDebugObjectName(vbuffer.staticVertexBuffer.Get(), L"GltfVertexBuffer");

        m_pUpload->Upload(vbuffer.staticVertexBuffer.Get(), vbuffer.vertexBuffer);

        m_pUpload->Transition(vbuffer.staticVertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        //create a view.
        D3D12_VERTEX_BUFFER_VIEW& view = pGeometry->m_VBV[idx];
        view.BufferLocation = vbuffer.staticVertexBuffer->GetGPUVirtualAddress();
        view.StrideInBytes = vertexBuffer->m_stride;
        view.SizeInBytes = vertexBufferSize;
    }

    // Creates Vertex Buffers from accessors and sets them in the Primitive struct.
    //
    void GLTFResources::CreateGeometry(tfAccessor indexBuffer, std::vector<tfAccessor> &vertexBuffers, Geometry *pGeometry)
    {
        CreateIndexBuffer(indexBuffer, pGeometry);

        // load the rest of the buffers onto the GPU
        pGeometry->m_vertexBuffers.resize(vertexBuffers.size());
        pGeometry->m_VBV.resize(vertexBuffers.size());
        for (uint32_t i = 0; i < vertexBuffers.size(); i++)
        {
            tfAccessor *pVertexAccessor = &vertexBuffers[i];
            CreateVertexBuffer(pVertexAccessor, i, pGeometry); 
        }
    }

    // Creates buffers and the input assemby at the same time. It needs a list of attributes to use.
    //
    void GLTFResources::CreateGeometry(const json &primitive, const std::vector<std::string>& requiredAttributes, std::vector<std::string> &semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, Geometry *pGeometry)
    {
        // Get Index Buffer accessor
        //
        tfAccessor indexBuffer;
        unsigned indexAcc = primitive.value("indices", 0u);
        m_pGltfFile->GetBufferDetails(indexAcc, &indexBuffer);
        CreateIndexBuffer(indexBuffer, pGeometry);

        // Create vertex buffers and input layout
        //
        uint32_t cnt = 0;
        layout.resize(requiredAttributes.size());
        semanticNames.resize(requiredAttributes.size());
        pGeometry->m_vertexBuffers.resize(requiredAttributes.size());
        pGeometry->m_VBV.resize(requiredAttributes.size());
        const json &attributes = primitive.at("attributes");
        for (const auto& attrName : requiredAttributes)
        {
            // get vertex buffer into static pool
            // 
            tfAccessor acc;
            const unsigned attr = attributes.find(attrName).value();
            m_pGltfFile->GetBufferDetails(attr, &acc);

            CreateVertexBuffer(&acc, cnt, pGeometry);
 
            // Set define so the shaders knows this stream is available
            //
            defines[std::string("HAS_") + attrName] = std::string("1");

            // split semantic name from index, DX doesnt like the trailing number
            uint32_t semanticIndex = 0;
            SplitGltfAttribute(attrName, &semanticNames[cnt], &semanticIndex);

            const json &inAccessor = m_pGltfFile->m_pAccessors->at((uint32_t)attr);

            // Create Input Layout
            //
            D3D12_INPUT_ELEMENT_DESC l = {};
            l.SemanticName = semanticNames[cnt].c_str(); // we need to set it in the pipeline function (because of multithreading)
            l.SemanticIndex = semanticIndex;
            l.Format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
            l.InputSlot = (UINT)cnt;
            l.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            l.InstanceDataStepRate = 0;
            l.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            layout[cnt] = l;

            cnt++;            
        }
    }

    void GLTFResources::SetPerFrameConstants()
    { 
        m_perFrameConstants = m_graphicsMemory->AllocateConstant(m_pGltfFile->m_perFrameData).GpuAddress();
    }

    void GLTFResources::SetSkinningMatricesForSkeletons()
    {
        for (auto &t : m_pGltfFile->m_pCurrentFrameTransformedData->worldSpaceSkeletonMats)
        {
            std::vector<DirectX::XMMATRIX> *matrices = &t.second;

            uint32_t size = (uint32_t)(matrices->size() * sizeof(DirectX::XMMATRIX));

            DirectX::GraphicsResource mem = m_graphicsMemory->Allocate(size);
            memcpy(mem.Memory(), matrices->data(), size);

            m_skeletonMatricesBuffer[t.first] = mem.GpuAddress();
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GLTFResources::GetSkinningMatricesBuffer(const tfNodeIdx skinIndex) const
    {
        auto it = m_skeletonMatricesBuffer.find(skinIndex);

        if (it == m_skeletonMatricesBuffer.end())
            return NULL;

        return it->second;
    }
}
