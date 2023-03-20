//--------------------------------------------------------------------------------------
// SimpleCustomAPO.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleCustomAPO.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "WAVFileReader.h"

#include "SimpleAPO.h"

namespace
{
    constexpr float c_paramValueScale = 1.f;

    const wchar_t* g_FileList[] = {
        L"71_setup_sweep_xbox.wav",
        L"musicmono.wav",
        L"sine.wav",
        nullptr,
    };
}

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_currentFile(0),
    m_currentEffectParam(1.f),
    m_CritErrorOccurred(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

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
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    InitializeXAudio();
}

void Sample::InitializeXAudio()
{
    // Initialize XAudio2 objects
    DX::ThrowIfFailed(XAudio2Create(m_pXAudio2.GetAddressOf(), 0));

#ifdef _DEBUG
    // Enable debugging features
    XAUDIO2_DEBUG_CONFIGURATION debug = {};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    m_pXAudio2->SetDebugConfiguration(&debug, 0);
#endif
    m_pXAudio2->RegisterForCallbacks(this);

    DX::ThrowIfFailed(m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice));

    // Start playing the first file
    m_currentFile = 0;
    Play(g_FileList[m_currentFile]);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    if (m_CritErrorOccurred)
    {
        m_CritErrorOccurred = false;
        InitializeXAudio();
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (pad.IsDPadLeftPressed())
        {
            m_currentEffectParam -= elapsedTime * c_paramValueScale;
            if (m_currentEffectParam < 0)
                m_currentEffectParam = 0.f;
        }
        else if (pad.IsDPadRightPressed())
        {
            m_currentEffectParam += elapsedTime * c_paramValueScale;
            if (m_currentEffectParam > 1)
                m_currentEffectParam = 1.f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if (m_pSourceVoice)
    {
        // Update xAPO params.
        SimpleAPOParams iparam{};
        iparam.gain = m_currentEffectParam;
        DX::ThrowIfFailed(m_pSourceVoice->SetEffectParameters(0, &iparam, sizeof(SimpleAPOParams)));

        // Check to see if buffer has finished playing, then move on to next sound
        XAUDIO2_VOICE_STATE state;
        m_pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued == 0)
        {
            m_pSourceVoice->DestroyVoice();

            if (g_FileList[++m_currentFile] == nullptr)
            {
                m_currentFile = 0;
            }

            Play(g_FileList[m_currentFile]);
        }
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto const fullscreen = m_deviceResources->GetOutputSize();

    auto const safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    wchar_t str[128] = {};
    swprintf_s(str, L"Playing: %ls", g_FileList[m_currentFile]);

    m_font->DrawString(m_spriteBatch.get(), str, pos);

    pos.y += m_font->GetLineSpacing() * 1.5f;

    swprintf_s(str, L"xAPO effect parameter = %.1f", m_currentEffectParam);

    m_font->DrawString(m_spriteBatch.get(), str, pos);

    DX::DrawControllerString(m_spriteBatch.get(),
        m_font.get(), m_ctrlFont.get(),
        L"[View] Exit   [DPad] Change parameter",
        XMFLOAT2(float(safeRect.left),
            float(safeRect.bottom) - m_font->GetLineSpacing()),
        ATG::Colors::LightGrey);

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();

    // Suspend audio engine
    m_pXAudio2->StopEngine();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();

    // Resume audio engine
    m_pXAudio2->StartEngine();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion

void Sample::Play(const wchar_t* szFilename)
{
    // Free any audio data from previous play (source voice must not be playing from it!)
    m_waveFile.reset();

    // Load audio data from disk
    DX::WAVData waveData;
    DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(szFilename, m_waveFile, waveData));

    // Create the source voice
    DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, waveData.wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO));

    // Create the custom APO instances
    CSimpleAPO* pSimpleAPO = nullptr;
    CSimpleAPO::CreateInstance(nullptr, 0, &pSimpleAPO);

    XAUDIO2_EFFECT_DESCRIPTOR apoDesc[1] = {};
    apoDesc[0].InitialState = true;
    apoDesc[0].OutputChannels = waveData.wfx->nChannels;
    apoDesc[0].pEffect = static_cast<IXAPO*>(pSimpleAPO);

    XAUDIO2_EFFECT_CHAIN chain = {};
    chain.EffectCount = static_cast<UINT32>(std::size(apoDesc));
    chain.pEffectDescriptors = apoDesc;

    DX::ThrowIfFailed(m_pSourceVoice->SetEffectChain(&chain));

    // Don't need to keep them now that XAudio2 has ownership
    pSimpleAPO->Release();

    // Submit wave data
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = waveData.startAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = waveData.audioBytes;

    DX::ThrowIfFailed(m_pSourceVoice->SubmitSourceBuffer(&buffer));

    // Start playing the voice
    DX::ThrowIfFailed(m_pSourceVoice->Start());

    // Set the initial effect params
    SimpleAPOParams iparam{};
    iparam.gain = m_currentEffectParam;
    DX::ThrowIfFailed(m_pSourceVoice->SetEffectParameters(0, &iparam, sizeof(SimpleAPOParams)));
}
