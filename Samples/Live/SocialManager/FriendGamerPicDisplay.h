//--------------------------------------------------------------------------------------
// FriendGamerPicDisplay.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <atomic>
#include <map>

#include "SampleGUI.h"

class FriendGamerPicDisplay
{
public:
    explicit FriendGamerPicDisplay() noexcept(false) :
        m_gamerPicDataSize(0),
        m_gamerPicReady(false),
        m_gamerPicCPU{},
        m_gamerPicGPU{}
    {};

    void ReleaseDevice();
    void RestoreDevice(DirectX::DescriptorPile& pile);
    void SetInitlized(bool isInit = true) { m_initialized = isInit; }
    void SetXuid(uint64_t xuid) { m_xuid = xuid; }
    void Update(
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue
    );

    bool CheckGamerPicCache(
        ATG::UIManager* mgr,
        std::map<uint64_t, std::pair<uint8_t*, size_t>>* profilePicCache);

    bool IsInitialized() { return m_initialized; }

    D3D12_GPU_DESCRIPTOR_HANDLE GetImgGPU() { return m_gamerPicGPU; }

private:
    // Gamerpic Resources
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_gamerPic;

    uint8_t*                                    m_gamerPicData;
    size_t                                      m_gamerPicDataSize;
    std::atomic<bool>                           m_gamerPicReady;

    // Direct3D resources
    D3D12_CPU_DESCRIPTOR_HANDLE                 m_gamerPicCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                 m_gamerPicGPU;

    uint64_t                                    m_xuid = 0;
    bool                                        m_initialized = false;
};
