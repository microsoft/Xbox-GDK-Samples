//--------------------------------------------------------------------------------------
// SimpleSamplerFeedback.h
//
// Sampler Feedback is a Direct3D feature for capturing and recording texture sampling information
// and locations. It can be used for things like texture - space shading and texture streaming. This
// sample only demonstrates a very simple implementation of sampler feedback. The sample renders a
// textured quad with a camera that can move toward or away from the quad. As the camera moves closer
// to the quad, a higher detail mip, i.e.lower mip level, is used during rendering. Sampler feedback
// writes out this information to a MinMip feedback map.
//
// Note: Sampler feedback is not supported on Xbox One, therefore this is a Scarlett only sample.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "FlyCamera.h"

// Shader constants
struct ConstantBuffer
{
    DirectX::XMMATRIX worldMatrix;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
};
static_assert((sizeof(ConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");

// Helper struct for a staging buffer
struct StagingTexture
{
    ID3D12Resource* pStagingBuffer;
    void* pLockedBits;
    D3D12_TEXTURE_COPY_LOCATION CopyLocation;
    D3D12_RANGE CopyRange;
    D3D12_RESOURCE_DESC Desc;

    void* Map()
    {
        if (pStagingBuffer != nullptr)
        {
            HRESULT hr = pStagingBuffer->Map(0, nullptr, &pLockedBits);
            return SUCCEEDED(hr) ? pLockedBits : nullptr;
        }
        return pLockedBits;
    }

    void Unmap()
    {
        if (pStagingBuffer != nullptr)
        {
            pStagingBuffer->Unmap(0, nullptr);
            pLockedBits = nullptr;
        }
    }
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

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

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptorHeap;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                    m_vertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                     m_indexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;
    ConstantBuffer*                             m_mappedConstantBufferData;
    D3D12_GPU_VIRTUAL_ADDRESS                   m_constantBufferDataGpuAddr;    

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_diffuseTexture;               // The texture paired with the feedback map

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_feedbackMapResource;          // The MinMip feedback map holding the minimum mip level requested by the paired texture
    StagingTexture                              m_feedbackMapStagingTexture;    // Staging texture to access the feedback map values on the CPU

    // Camera
    std::unique_ptr<DX::FlyCamera>              m_camera;
    DirectX::SimpleMath::Matrix                 m_worldMatrix;

    // Desriptors for m_resourceDescriptorHeap
    enum class ResourceDescriptors
    {
        DiffuseTextureSRV,      // SRV for texture paired with the feedback map
        FeedbackMapUAV,         // UAV for the feedback map
        TextFont,
        ControllerFont,
        Count
    };

    // Read back the feedback map value
    float ReadBackFeedbackMapValue();

    // Helper function for a stating texture
    HRESULT CreateStagingBufferForTexture(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC* pTextureDesc, bool readback,
                                          ID3D12Resource** ppBuffer, D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation, D3D12_RANGE* pEntireRange);
};
