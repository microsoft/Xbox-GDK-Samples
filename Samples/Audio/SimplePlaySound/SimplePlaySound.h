//--------------------------------------------------------------------------------------
// SimplePlaySound.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public IXAudio2EngineCallback
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

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

private:

    void InitializeXAudio();

    STDMETHOD_(void, OnProcessingPassStart) () override {}
    STDMETHOD_(void, OnProcessingPassEnd)() override {}
    STDMETHOD_(void, OnCriticalError) (THIS_ HRESULT)
    {
        //When the renderer is invalidated, restart
        if (m_pXAudio2)
        {
            m_pXAudio2->StopEngine();

            m_pXAudio2->UnregisterForCallbacks(this);

            if (m_pSourceVoice)
            {
                m_pSourceVoice->DestroyVoice();
                m_pSourceVoice = nullptr;
            }

            if (m_pMasteringVoice)
            {
                m_pMasteringVoice->DestroyVoice();
                m_pMasteringVoice = nullptr;
            }

            m_pXAudio2.Reset();
        }

        m_CritErrorOccurred = true;
    }

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void Play(const wchar_t* szFilename);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        TextFont,
        Background,
        Count,
    };

    // Audio objects.
    Microsoft::WRL::ComPtr<IXAudio2>            m_pXAudio2;
    IXAudio2MasteringVoice*                     m_pMasteringVoice;
    IXAudio2SourceVoice*                        m_pSourceVoice;
    int                                         m_currentFile;
    std::unique_ptr<uint8_t[]>                  m_waveFile;
    std::wstring                                m_waveDesc;
    std::atomic<bool>                           m_CritErrorOccurred;

    void*                                       m_xmaMemory;
};
