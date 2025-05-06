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

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "GltfFile.h"
#include "GraphicsMemory.h"

class DefineList;

namespace AMDTK
{
    struct GeometryVbuff
    { 
        DirectX::SharedGraphicsResource                         vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>                  staticVertexBuffer;
    };
    struct Geometry
    {
        DXGI_FORMAT m_indexType;
        uint32_t m_NumIndices;
        D3D12_INDEX_BUFFER_VIEW m_IBV;
        std::vector<D3D12_VERTEX_BUFFER_VIEW> m_VBV;

        DirectX::SharedGraphicsResource                         indexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>                  staticIndexBuffer;

        std::vector<GeometryVbuff> m_vertexBuffers;
    };

    class GLTFResources
    {
        ID3D12Device *m_pDevice;

        const json *m_pTextureNodes;

        std::map<tfNodeIdx, D3D12_GPU_VIRTUAL_ADDRESS> m_skeletonMatricesBuffer;
        std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> m_InverseBindMatrices;

        struct TextureConfig {
            bool useSRGB;
            float cutOff;
        };

        std::vector<std::wstring> m_textureNames;
        std::vector<TextureConfig> m_textureConfigs;
        GLTFFile* m_pGltfFile;
        DirectX::ResourceUploadBatch *m_pUpload;

        DirectX::GraphicsMemory* m_graphicsMemory;

        D3D12_GPU_VIRTUAL_ADDRESS m_perFrameConstants;
         
        std::unique_ptr<DirectX::DescriptorPile> m_srvPile;
        std::unique_ptr<DirectX::EffectTextureFactory>  m_gltfTexResources;

    public:
        GLTFFile* GetGltfFile() { return m_pGltfFile; };
        DirectX::DescriptorPile* GetSrvPile() { return m_srvPile.get(); };
        DirectX::GraphicsMemory* GetGPUMemory() { return m_graphicsMemory; };

        bool OnCreate(ID3D12Device* pDevice, GLTFFile *pGltfFile, DirectX::ResourceUploadBatch *pUpload , DirectX::GraphicsMemory* gpumem);
        void FindTextures();

        int LoadTextures(const wchar_t* path, int destinationDescriptorOffset = 0) ;
        void OnDestroy();

        void CreateVertexBuffer(tfAccessor* vertexBuffers, uint32_t idx, Geometry *pGeometry);
        void CreateIndexBuffer(tfAccessor indexBuffer, Geometry *pGeometry);
        void CreateGeometry(tfAccessor indexBuffer, std::vector<tfAccessor> &vertexBuffers, Geometry *pGeometry);

        void CreateGeometry(const json &primitive, const std::vector<std::string > &requiredAttributes, std::vector<std::string> &semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, Geometry *pGeometry);

        void SetPerFrameConstants();
        void SetSkinningMatricesForSkeletons();
         
        ID3D12Resource* GetTextureResourceByID(uint32_t id);
        D3D12_GPU_VIRTUAL_ADDRESS GetSkinningMatricesBuffer(const tfNodeIdx skinIndex) const;
        D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameConstants() { return m_perFrameConstants; }
    };
}
