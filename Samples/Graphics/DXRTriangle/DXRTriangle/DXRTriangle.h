//--------------------------------------------------------------------------------------
// DXRTriangle.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "DXRHelper.h"

struct SimpleTriangleRecord : public ShaderRecord
{
    SimpleTriangleRecord() = default;

    SimpleTriangleRecord(ID3D12StateObjectProperties* props, LPCWSTR exportName)
    {
        Initialize(props, exportName);
    }
};

struct DescriptorIndex
{
    enum Enum
    {
        UAVOutput,

        Font,
        ControllerFont,

        EnumCount
    };
    
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderHUD(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void CreateRaytracingPipeline();
    void BuildBottomLevelAccelerationStructure(bool buildEveryFrame);
    void BuildTopLevelAccelerationStructure(bool buildEveryFrame);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DXR Objects
    Microsoft::WRL::ComPtr<ID3D12StateObject>   m_raytracingStateObject;
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>   m_raytracingStateObjectProps;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_globalRootSignature;

    Microsoft::WRL::ComPtr<ID3D12Resource>		m_UAVOutput;
    std::unique_ptr<DirectX::DescriptorHeap>    m_csuDescriptorHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource>		m_TLAS, m_TLASScratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>		m_triangleBLAS;
    Microsoft::WRL::ComPtr<ID3D12Resource>		m_VB, m_IB, m_scratch;

    ShaderBindingTable<SimpleTriangleRecord, 1, 2, 1> m_shaderBindingTable;

    static constexpr uint32_t MAX_INSTANCES_IN_TLAS = 10;
    uint32_t m_numInstancesInTLAS;

    DirectX::GraphicsResource m_instanceDescBuffer;
    D3D12_RAY_FLAGS m_rayFlags;
    float m_holeSize;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
