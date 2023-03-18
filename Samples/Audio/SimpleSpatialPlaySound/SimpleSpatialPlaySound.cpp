//--------------------------------------------------------------------------------------
// SimpleSpatialPlaySound.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleSpatialPlaySound.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "../Audio/WAVFileReader.h"

namespace
{
    const LPCWSTR g_FileList[] =
    {
        L"Jungle_RainThunder_mix714.wav",
        L"ChannelIDs714.wav",
    };

    const int numFiles = _countof(g_FileList);
}

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    VOID CALLBACK SpatialWorkCallback(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WORK)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        DX::ThrowIfFailed(hr);

        auto Sink = reinterpret_cast<Sample *>(Context);

        while (Sink->m_bThreadActive)
        {
            while (Sink->m_bPlayingSound && Sink->m_Renderer->IsActive())
            {
                // Wait for a signal from the audio-engine to start the next processing pass 
                if (WaitForSingleObject(Sink->m_Renderer->m_bufferCompletionEvent, 100) != WAIT_OBJECT_0)
                {
                    //make a call to stream to see why we didn't get a signal after 100ms
                    hr = Sink->m_Renderer->m_SpatialAudioStream->Reset();

                    //if we have a stream error, set the renderer state to reset
                    if (FAILED(hr))
                    {
                        Sink->m_Renderer->Reset();
                    }
                    continue;
                }

                UINT32 frameCount;
                UINT32 availableObjectCount;


                // Begin the process of sending object data and metadata 
                // Get the number of active object that can be used to send object-data 
                // Get the number of frame count that each buffer be filled with  
                hr = Sink->m_Renderer->m_SpatialAudioStream->BeginUpdatingAudioObjects(
                    &availableObjectCount,
                    &frameCount);

                //if we have a stream error, set the renderer state to reset
                if (FAILED(hr))
                {
                    Sink->m_Renderer->Reset();
                }

                Sink->m_availableObjects = static_cast<int>(availableObjectCount);

                for (int chan = 0; chan < MAX_CHANNELS; chan++)
                {
                    //Activate the object if not yet done
                    if (Sink->WavChannels[chan].object == nullptr)
                    {
                        // If this method called more than activeObjectCount times 
                        // It will fail with this error HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) 
                        hr = Sink->m_Renderer->m_SpatialAudioStream->ActivateSpatialAudioObject(
                            Sink->WavChannels[chan].objType,
                            &Sink->WavChannels[chan].object);
                        if (FAILED(hr))
                        {
                            continue;
                        }

                    }

                    //Get the object buffer
                    BYTE* buffer = nullptr;
                    UINT32 bytecount;
                    hr = Sink->WavChannels[chan].object->GetBuffer(&buffer, &bytecount);
                    if (FAILED(hr))
                    {
                        continue;
                    }

                    Sink->WavChannels[chan].object->SetVolume(Sink->WavChannels[chan].volume);

                    UINT32 readsize = bytecount;

                    for (UINT32 i = 0; i < readsize; i++)
                    {
                        UINT32 fileLoc = Sink->WavChannels[chan].curBufferLoc;
                        if (chan < Sink->m_numChannels)
                        {
                            buffer[i] = Sink->WavChannels[chan].wavBuffer[fileLoc];
                        }
                        else
                        {
                            buffer[i] = 0;
                        }

                        Sink->WavChannels[chan].curBufferLoc++;
                        if (Sink->WavChannels[chan].curBufferLoc == Sink->WavChannels[chan].buffersize)
                        {
                            Sink->WavChannels[chan].curBufferLoc = 0;
                        }


                    }
                }

                // Let the audio-engine know that the object data are available for processing now 
                hr = Sink->m_Renderer->m_SpatialAudioStream->EndUpdatingAudioObjects();
                if (FAILED(hr))
                {
                    Sink->m_Renderer->Reset();
                }
            }
        }

    }
}

Sample::Sample() noexcept(false) :
    m_Renderer(nullptr),
    m_numChannels(0),
    WavChannels{},
    m_bThreadActive(false),
    m_bPlayingSound(false),
    m_availableObjects(0),
    m_frame(0),
    m_fileLoaded(false),
    m_curFile(0),
    m_playPressed(false),
    m_nextPressed(false),
    m_workThread(nullptr)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_workThread)
    {
        //stop and shutdown spatial worker thread
        m_bThreadActive = false;
        m_bPlayingSound = false;
        WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
        CloseThreadpoolWork(m_workThread);
        m_workThread = nullptr;
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

    InitializeSpatialStream();

    SetChannelPosVolumes();

    for (UINT32 i = 0; i < MAX_CHANNELS; i++)
    {
        WavChannels[i].buffersize = 0;
        WavChannels[i].curBufferLoc = 0;
        WavChannels[i].object = nullptr;

    }

    m_fileLoaded = LoadFile(g_FileList[m_curFile]);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    //Are we resetting the renderer?  This will happen if we get an invalid stream 
    //  which can happen when render mode changes or device changes
    if (m_Renderer->IsResetting())
    {
        // Initialize Default Audio Device
        m_Renderer->InitializeAudioDevice();

        //Reset all the Objects that were being used
        for (int chan = 0; chan < MAX_CHANNELS; chan++)
        {
            WavChannels[chan].object = nullptr;
        }
    }
    
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (pad.IsAPressed() && !m_playPressed)
        {
            m_playPressed = true;

            //if we have an active renderer
            if (m_Renderer && m_Renderer->IsActive())
            {
                //Start spatial worker thread
                if (!m_bThreadActive)
                {
                    //reload file to start again
                    m_fileLoaded = LoadFile(g_FileList[m_curFile]);
                    //startup spatial thread
                    m_bThreadActive = true;
                    m_bPlayingSound = true;
                    m_workThread = CreateThreadpoolWork(SpatialWorkCallback, this, nullptr);
                    SubmitThreadpoolWork(m_workThread);
                }
                else
                {
                    //stop and shutdown spatial worker thread
                    m_bThreadActive = false;
                    m_bPlayingSound = false;
                    WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
                    CloseThreadpoolWork(m_workThread);
                    m_workThread = nullptr;
                }
            }
        }
        else if (!pad.IsAPressed() && m_playPressed)
        {
            m_playPressed = false;
        }

        if (pad.IsBPressed() && !m_nextPressed)
        {
            m_nextPressed = true;

            //if we have an active renderer
            if (m_Renderer && m_Renderer->IsActive())
            {
                //if spatial thread active and playing, shutdown and start new file
                if (m_bThreadActive && m_bPlayingSound)
                {
                    m_bThreadActive = false;
                    m_bPlayingSound = false;
                    WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
                    CloseThreadpoolWork(m_workThread);
                    m_workThread = nullptr;

                    //load next file
                    m_curFile++;
                    if (m_curFile == numFiles) m_curFile = 0;
                    m_fileLoaded = LoadFile(g_FileList[m_curFile]);

                    //if successfully loaded, start up thread and playing new file
                    if (m_fileLoaded)
                    {
                        m_bThreadActive = true;
                        m_bPlayingSound = true;
                        m_workThread = CreateThreadpoolWork(SpatialWorkCallback, this, nullptr);
                        SubmitThreadpoolWork(m_workThread);
                    }
                }
                else
                {
                    //if thread active but paused, shutdown thread to load new file
                    if (m_bThreadActive)
                    {
                        m_bThreadActive = false;
                        m_bPlayingSound = false;
                        WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
                        CloseThreadpoolWork(m_workThread);
                        m_workThread = nullptr;
                    }

                    //load next file
                    m_curFile++;
                    if (m_curFile == numFiles) m_curFile = 0;
                    m_fileLoaded = LoadFile(g_FileList[m_curFile]);
                }
            }
        }
        else if (!pad.IsBPressed() && m_nextPressed)
        {
            m_nextPressed = false;
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    float spacing = m_font->GetLineSpacing();

    m_font->DrawString(m_spriteBatch.get(), L"Simple Spatial Playback:", pos, ATG::Colors::White);
    pos.y += spacing * 1.5f;

    if (!m_Renderer->IsActive())
    {
        m_font->DrawString(m_spriteBatch.get(), L"Spatial Renderer Not Available", pos, ATG::Colors::Orange);
        pos.y += spacing * 2.f;
    }
    else
    {
        wchar_t str[256] = {};
        swprintf_s(str, L"   File: %ls", g_FileList[m_curFile]);
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
        pos.y += spacing;
        swprintf_s(str, L"   State: %ls", ((m_bPlayingSound) ? L"Playing" : L"Stopped"));
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
        pos.y += spacing * 1.5f;

        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [A] to start/stop playback", pos, ATG::Colors::White);
        pos.y += spacing;
        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [B] to change to next file", pos, ATG::Colors::White);
        pos.y += spacing;
        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [View] to exit", pos, ATG::Colors::White);
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
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
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
        m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
    
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

HRESULT Sample::InitializeSpatialStream(void)
{
    HRESULT hr = S_OK;

    if (!m_Renderer)
    {
        // Create a new ISAC instance
        m_Renderer = new ISACRenderer();
        // Selects the Default Audio Device
        hr = m_Renderer->InitializeAudioDevice();
    }

    return hr;
}

bool Sample::LoadFile(LPCWSTR inFile)
{
    //clear and reset wavchannels
    for (UINT32 i = 0; i < MAX_CHANNELS; i++)
    {
        if (WavChannels[i].buffersize)
        {
            delete[] WavChannels[i].wavBuffer;
        }
        WavChannels[i].buffersize = 0;
        WavChannels[i].curBufferLoc = 0;
        WavChannels[i].object = nullptr;
    }

    HRESULT hr = S_OK;
    std::unique_ptr<uint8_t[]>              m_waveFile;
    DirectX::WAVData WavData;
    hr = DirectX::LoadWAVAudioFromFileEx(inFile, m_waveFile, WavData);
    if (FAILED(hr))
    {
        return false;
    }

    if ((WavData.wfx->wFormatTag == 1 || WavData.wfx->wFormatTag == 65534) && WavData.wfx->nSamplesPerSec == 48000)
    {
        m_numChannels = WavData.wfx->nChannels;

        unsigned numSamples = WavData.audioBytes / (2u * m_numChannels);
        for (int i = 0; i < m_numChannels; i++)
        {
            WavChannels[i].wavBuffer = new BYTE[numSamples * 4];
            WavChannels[i].buffersize = numSamples * 4;
        }

        float * tempnew;
        short * tempdata = (short *)WavData.startAudio;

        for (unsigned i = 0; i < numSamples; i++)
        {
            for (int j = 0; j < m_numChannels; j++)
            {
                int chan = j;
                tempnew = (float *)WavChannels[chan].wavBuffer;
                unsigned sample = (i * m_numChannels) + chan;
                tempnew[i] = (float)tempdata[sample] / 32768;
            }
        }
    }
    else if ((WavData.wfx->wFormatTag == 3) && WavData.wfx->nSamplesPerSec == 48000)
    {
        m_numChannels = WavData.wfx->nChannels;
        unsigned numSamples = WavData.audioBytes / (4u * m_numChannels);
        for (int i = 0; i < m_numChannels; i++)
        {
            WavChannels[i].wavBuffer = new BYTE[numSamples * 4];
            WavChannels[i].buffersize = numSamples * 4;
        }

        float * tempnew;
        float * tempdata = (float *)WavData.startAudio;

        for (unsigned i = 0; i < numSamples; i++)
        {
            for (int j = 0; j < m_numChannels; j++)
            {
                int chan = j;
                tempnew = (float *)WavChannels[chan].wavBuffer;
                unsigned sample = (i * m_numChannels) + chan;
                tempnew[i] = (float)tempdata[sample];
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}



void Sample::SetChannelPosVolumes(void)
{
    WavChannels[0].volume = 1.f;
    WavChannels[0].objType = AudioObjectType_FrontLeft;

    WavChannels[1].volume = 1.f;
    WavChannels[1].objType = AudioObjectType_FrontRight;

    WavChannels[2].volume = 1.f;
    WavChannels[2].objType = AudioObjectType_FrontCenter;

    WavChannels[3].volume = 1.f;
    WavChannels[3].objType = AudioObjectType_LowFrequency;

    WavChannels[4].volume = 1.f;
    WavChannels[4].objType = AudioObjectType_BackLeft;

    WavChannels[5].volume = 1.f;
    WavChannels[5].objType = AudioObjectType_BackRight;

    WavChannels[6].volume = 1.f;
    WavChannels[6].objType = AudioObjectType_SideLeft;

    WavChannels[7].volume = 1.f;
    WavChannels[7].objType = AudioObjectType_SideRight;

    WavChannels[8].volume = 1.f;
    WavChannels[8].objType = AudioObjectType_TopFrontLeft;

    WavChannels[9].volume = 1.f;
    WavChannels[9].objType = AudioObjectType_TopFrontRight;

    WavChannels[10].volume = 1.f;
    WavChannels[10].objType = AudioObjectType_TopBackLeft;

    WavChannels[11].volume = 1.f;
    WavChannels[11].objType = AudioObjectType_TopBackRight;

}

