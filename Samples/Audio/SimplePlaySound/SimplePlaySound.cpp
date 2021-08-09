//--------------------------------------------------------------------------------------
// SimplePlaySound.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimplePlaySound.h"

#include "ATGColors.h"

#include "WAVFileReader.h"

namespace
{
    const wchar_t* g_FileList[] = {
        L"71_setup_sweep_xbox.wav",
        L"musicmono.wav",
        L"musicmono_xma.wav",
        L"musicmono_adpcm.wav",
        L"musicmono_xwma.wav",
        L"sine.wav",
    };
}

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pMasteringVoice(nullptr),
    m_pSourceVoice(nullptr),
    m_currentFile(0),
    m_CritErrorOccurred(false),
    m_xmaMemory(nullptr)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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

    if (m_xmaMemory)
    {
        ApuFree(m_xmaMemory);
        m_xmaMemory = nullptr;
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

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    if (m_CritErrorOccurred)
    {
        m_CritErrorOccurred = false;
        InitializeXAudio();
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }

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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    wchar_t str[128] = {};
    swprintf_s(str, L"Playing: %ls", g_FileList[m_currentFile]);

    m_font->DrawString(m_spriteBatch.get(), str, pos);

    if (!m_waveDesc.empty())
    {
        pos.y += m_font->GetLineSpacing();

        m_font->DrawString(m_spriteBatch.get(), m_waveDesc.c_str(), pos);
    }

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    // Don't need to clear color as the sample draws a fullscreen image background

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion

void Sample::Play(const wchar_t* szFilename)
{
    // Free any audio data from previous play (source voice must not be playing from it!)
    m_waveFile.reset();

    if (m_xmaMemory)
    {
        ApuFree(m_xmaMemory);
        m_xmaMemory = nullptr;
    }

    // Load audio data from disk
    DX::WAVData waveData;
    DX::ThrowIfFailed(DX::LoadWAVAudioFromFileEx(szFilename, m_waveFile, waveData));

    // Set up text description
    m_waveDesc.clear();

    uint32_t tag = DX::GetFormatTag(waveData.wfx);
    switch (tag)
    {
        case WAVE_FORMAT_ADPCM:
            m_waveDesc = L"ADPCM";
            break;

        case WAVE_FORMAT_WMAUDIO2:
        case WAVE_FORMAT_WMAUDIO3:
            m_waveDesc = L"xWMA";
            break;

        case WAVE_FORMAT_PCM:
            m_waveDesc = L"PCM";
            break;

        case WAVE_FORMAT_XMA2:
            m_waveDesc = L"XMA2";
            break;
    }

    {
        wchar_t buff[64];
        swprintf_s(buff, L" (%u channels, %u bits, %u samples, %zu ms duration)",
            waveData.wfx->nChannels,
            waveData.wfx->wBitsPerSample,
            waveData.wfx->nSamplesPerSec,
            waveData.GetSampleDurationMS());

        m_waveDesc += buff;

        if (waveData.loopLength > 0)
        {
            m_waveDesc += L" [loop point]";
        }
    }

    // Create the source voice
    DX::ThrowIfFailed(m_pXAudio2->CreateSourceVoice(&m_pSourceVoice, waveData.wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO));

    // Submit wave data
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = waveData.startAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;       // Indicates all the audio data is being submitted at once
    buffer.AudioBytes = waveData.audioBytes;

    if (waveData.loopLength > 0)
    {
        buffer.LoopBegin = waveData.loopStart;
        buffer.LoopLength = waveData.loopLength;
        buffer.LoopCount = 1; // We'll just assume we play the loop twice
    }

    if (tag == WAVE_FORMAT_WMAUDIO2 || tag == WAVE_FORMAT_WMAUDIO3)
    {
        //
        // xWMA includes seek tables which must be provided
        //
        XAUDIO2_BUFFER_WMA xwmaBuffer = {};
        xwmaBuffer.pDecodedPacketCumulativeBytes = waveData.seek;
        xwmaBuffer.PacketCount = waveData.seekCount;

        DX::ThrowIfFailed(m_pSourceVoice->SubmitSourceBuffer(&buffer, &xwmaBuffer));
    }
    else if (tag == WAVE_FORMAT_XMA2)
    {
        //
        // To play XMA2 data, we need the audio data in APU memory rather than system memory
        //
        DX::ThrowIfFailed(ApuAlloc(&m_xmaMemory, nullptr, static_cast<UINT32>(waveData.audioBytes), SHAPE_XMA_INPUT_BUFFER_ALIGNMENT));

        memcpy(m_xmaMemory, waveData.startAudio, waveData.audioBytes);

        buffer.pAudioData = static_cast<BYTE*>(m_xmaMemory);
        DX::ThrowIfFailed(m_pSourceVoice->SubmitSourceBuffer(&buffer));
    }
    else
    {
        DX::ThrowIfFailed(m_pSourceVoice->SubmitSourceBuffer(&buffer));
    }

    // Start playing the voice
    DX::ThrowIfFailed(m_pSourceVoice->Start());
}


