//--------------------------------------------------------------------------------------
// MP4Encoder.h
//
// Demonstrates how to use the DX12 pipeline to encode an mp4 file using the HW encoder
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:
    static constexpr int c_NumSwapBuffers = 3;
    static constexpr int c_NumNV12Textures = 4;

    int m_numFramesEncoded;
    std::atomic_bool m_startEncoding;
    std::atomic_bool m_isEncoding;
    std::atomic_bool m_stopEncoding;
    std::atomic_bool m_processInputDeferred;

    Microsoft::WRL::ComPtr<IMFTransform>            m_pEncoder;
    Microsoft::WRL::ComPtr<IMFMediaEventGenerator>  m_pMftEventGen;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager>    m_pDXGIManager;
    Microsoft::WRL::ComPtr<IMFSinkWriter>           m_pSinkWriter;

    void InitializeEncoder();
    void ShutdownEncoder();
    void EncodeVideo();

    IMFSample* CreateInputSample();
    HRESULT WriteEncodedFrameToDisk();
    void ProcessInput();

    LONGLONG    m_videoTimeStamp;
    DWORD       m_videoStreamIndex;
    HANDLE      m_hOutputFileHandle;

    struct NV12Texture
    {
        enum Status
        {
            Status_Free,
            Status_UsedByGPU,
            Status_ReadyForEncoder,
            Status_UsedByEncoder
        };

        Status                                  m_status;
        UINT64                                  m_fenceValue;
        Microsoft::WRL::ComPtr<ID3D12Resource>  m_pTexture;
    };

    NV12Texture                                 m_NV12Textures[c_NumNV12Textures];

    std::deque<int>                             m_AvailableTextures;
    std::deque<int>                             m_TexturesUsedByGPU;
    std::deque<int>                             m_TexturesReadyForEncoder;
    std::deque<int>                             m_TexturesUsedByEncoder;

    std::mutex                                  m_sync;
    std::mutex                                  m_syncCmdQueue;

    Microsoft::WRL::ComPtr<ID3D12Fence>         m_NV12TextureFence;
    Microsoft::WRL::Wrappers::Event             m_NV12TextureFenceEvent;

    UINT64 AddTextureFence();
    void WaitForTextureFence(UINT64 fenceValue, DWORD timeout = INFINITE);
    bool IsGPUDoneWithTexture(UINT64 fenceValue);

    // RGB to NV12 shader
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rsRGB2NV12;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_psoRGB2NV12;
    void ConvertRGB2NV12();

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    uint32_t                                    m_displayWidth;
    uint32_t                                    m_displayHeight;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // HUD
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;
    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertexBuffer;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_nv12RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_nv12PSO;

    enum ShaderResourceViews
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_SwapBuffer,
        UAV_NV12Texture_Y_Data = SRV_SwapBuffer + c_NumSwapBuffers,
        UAV_NV12Texture_UV_Data = UAV_NV12Texture_Y_Data + c_NumNV12Textures,
        SRV_Count = UAV_NV12Texture_UV_Data + c_NumNV12Textures
    };
};
