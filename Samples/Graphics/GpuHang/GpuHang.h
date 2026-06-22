//--------------------------------------------------------------------------------------
// GpuHang.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "Hang.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample();
    ~Sample() = default;

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void ParseCommandLine(const wchar_t* commandLine);

    // Initialization and management
    void Initialize(HWND window);

    // Basic Sample loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

private:
    void RenderUI();

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons[DirectX::GamePad::MAX_PLAYER_COUNT];
        
    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;

    // Desriptors for m_resourceDescriptorHeap
    struct ResourceDescriptors
    {
        enum : uint32_t
        {
            FontHangList,
            FontInstructions,
            FontController, 
            FontDescription, 

            Count
        };
    };

    // UI
    std::unique_ptr<DirectX::SpriteFont>            m_fontHangList;
    std::unique_ptr<DirectX::SpriteFont>            m_fontInstructions;
    std::unique_ptr<DirectX::SpriteFont>            m_fontController;
    std::unique_ptr<DirectX::SpriteFont>            m_fontDescription;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    float                                           m_scrollY;

    bool                                            m_hang;
    bool                                            m_takeCapture;
    uint32_t                                        m_queueType;
    uint32_t                                        m_hangAction;
    uint32_t                                        m_selectedHangIndex;
    IHang* SelectedHang() const                     {return Hang::HangList()[m_selectedHangIndex];}

    std::wstring DumpFileName() const;
    std::wstring CaptureFileName() const;
    void BeginCapture(ID3D12CommandQueue* commandQueue) const;
    void EndCapture(ID3D12CommandQueue* commandQueue) const;

    static std::wstring                             m_dumpFileName;
};

__declspec(selectany) std::wstring Sample::m_dumpFileName = L"";
