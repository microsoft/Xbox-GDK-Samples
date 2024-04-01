//--------------------------------------------------------------------------------------
// FriendGamerPicDisplay.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FriendGamerPicDisplay.h"

#include "WICTextureLoader.h"

using namespace DirectX;

bool FriendGamerPicDisplay::CheckGamerPicCache(
    ATG::UIManager* mgr,
    std::map<uint64_t, std::pair<uint8_t*, size_t>>* profilePicCache)
{
    // Check if user's xuid has been saved to the cache yet
    if (profilePicCache->find(m_xuid) != profilePicCache->end()) {
        m_gamerPicData = profilePicCache->at(m_xuid).first;
        m_gamerPicDataSize = profilePicCache->at(m_xuid).second;
        m_gamerPicReady = true;

        mgr->RegisterImage(static_cast<unsigned int>(m_xuid), GetImgGPU(), XMUINT2(40, 40));
        return true;
    }
    else
    {
        m_gamerPicDataSize = 0;
        m_gamerPicData = nullptr;
        return false;
    }
}

void FriendGamerPicDisplay::Update(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    if (m_gamerPicReady && device)
    {
        if (m_gamerPicDataSize && m_gamerPicData)
        {
            ResourceUploadBatch upload(device);

            upload.Begin();

            HRESULT hr = CreateWICTextureFromMemory(
                device,
                upload,
                m_gamerPicData,
                m_gamerPicDataSize,
                m_gamerPic.ReleaseAndGetAddressOf());

            if (SUCCEEDED(hr))
            {
                CreateShaderResourceView(device, m_gamerPic.Get(), m_gamerPicCPU);
            }

            auto result = upload.End(commandQueue);
            result.wait();
        }

        m_gamerPicReady = false;
    }
}

void FriendGamerPicDisplay::ReleaseDevice()
{
    m_gamerPic.Reset();

    m_gamerPicCPU = {};
    m_gamerPicGPU = {};
}

void FriendGamerPicDisplay::RestoreDevice(DirectX::DescriptorPile& pile)
{
    size_t index = pile.Allocate();
    m_gamerPicCPU = pile.GetCpuHandle(index);
    m_gamerPicGPU = pile.GetGpuHandle(index);
}
