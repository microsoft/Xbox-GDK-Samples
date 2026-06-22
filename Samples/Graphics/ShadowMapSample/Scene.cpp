//--------------------------------------------------------------------------------------
// Scene.cpp
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Scene.h"
#include "Utils.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace
{
    wchar_t const* const s_folderPaths[2] = { L"Media\\Meshes\\Cathedral", 0 };
}

/////////////////
////  Scene  ////
/////////////////
Scene::Scene(const wchar_t* wstrDisplayName)
{
	wcscpy_s(m_wstrDisplayName, wstrDisplayName);
}


//////////////////////
////  bakedScene  ////
//////////////////////
BakedScene::BakedScene(ComPtr<ID3D12Device> pDev, const wchar_t* wstrDisplayName,
    const wchar_t* wstrMeshName) : Scene(wstrDisplayName)
{
    wchar_t s_meshFilename[1024];
    wchar_t filepath[1024];
    _snwprintf_s(s_meshFilename, std::size(s_meshFilename), _TRUNCATE, L"%s.sdkmesh", wstrMeshName);

    // Search CWD with folder names
    DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), s_meshFilename, s_folderPaths);
    m_obj.Model = Model::CreateFromSDKMESH(pDev.Get(), filepath);

    // Store the media directory
    std::wstring pathTemp = filepath;
    m_mediaDirectory = pathTemp.substr(0, pathTemp.find_last_of('\\'));

    // Create the resource for the constant buffer for this object
    // We can do one CB for all submeshes since the scene remains constant and all submeshes share world matrix
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        uint32_t buffersize = sizeof(CBTransformStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        DX::ThrowIfFailed(
            pDev->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_obj.ConstantBuffer.ReleaseAndGetAddressOf())));
    }
}

//D3DDeviceContext* pCtx* used to be a param
void BakedScene::Render(ID3D12GraphicsCommandList* commandList, std::unique_ptr<DescriptorPile>& srvPile)
{
    auto& model = m_obj.Model;

    // This has to be already updated to the new values
    commandList->SetGraphicsRootConstantBufferView(OBJ_TRANSFORM_CBV, m_obj.ConstantBuffer->GetGPUVirtualAddress());

    auto& opaqueParts = model->meshes[0]->opaqueMeshParts;
    for (size_t i = 0; i < opaqueParts.size(); ++i)
    {
        uint32_t materialIndex = opaqueParts[i]->materialIndex;

        uint32_t offsetIntoHeap = FIXED_PORTION_OFFSET + materialIndex * 2;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvPile->GetGpuHandle(offsetIntoHeap);
        commandList->SetGraphicsRootDescriptorTable(DIFFUSE_TEX_TABLE, srvHandle);

        opaqueParts[i]->Draw(commandList);
    }
}
