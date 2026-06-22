//--------------------------------------------------------------------------------------
// MP4Encoder.cpp
//
// Demonstrates how to use the DX12 pipeline to encode an mp4 file using the HW encoder
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MP4Encoder.h"

#include <codecapi.h>

#include "ATGColors.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    // Write the encoded video to the devkit's system scratch folder
    // You can access it from XboxManager or using "xbdir xd:\"
    const wchar_t* c_videoOutputFilePath = L"D:\\MP4EncoderSampleOutput.h264";
    const wchar_t* c_videoMP4FilePath = L"D:\\MP4EncoderSampleOutput.mp4";

    constexpr UINT32 c_videoWidth = 1920;
    constexpr UINT32 c_videoHeight = 1080;
    constexpr UINT32 c_videoBitRate = 20000000;
    constexpr LONGLONG c_videoFrameDuration = MFCLOCK_FREQUENCY_HNS / 30;
}

Sample::Sample() noexcept(false)
    : m_numFramesEncoded(0)
    , m_startEncoding(false)
    , m_isEncoding(false)
    , m_stopEncoding(false)
    , m_processInputDeferred(false)
    , m_videoTimeStamp(0)
    , m_videoStreamIndex(0xFFFFFFFF)
    , m_hOutputFileHandle(nullptr)
    , m_NV12Textures{}
    , m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_UNKNOWN,
        c_NumSwapBuffers);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    // Wait until encoder thread is done encoding
    while (m_isEncoding.load())
    {
        m_stopEncoding = true;

        SwitchToThread();
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    std::ignore = DeleteFileW(c_videoMP4FilePath);
    std::ignore = DeleteFileW(c_videoOutputFilePath);

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    auto device = m_deviceResources->GetD3DDevice();

    // SRVs for back buffers
    for (size_t i = 0; i < c_NumSwapBuffers; i++)
    {
        auto pBackBuffer = m_deviceResources->GetRenderTarget(i);
        device->CreateShaderResourceView(pBackBuffer, nullptr, m_srvPile->GetCpuHandle(static_cast<size_t>(SRV_SwapBuffer + i)));
    }

    // Create pool of NV12 textures
    const D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_NV12, c_videoWidth, c_videoHeight, 1, 1, 1, 0,
                                                                            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    wchar_t name[64] = {};

    for (size_t i = 0; i < c_NumNV12Textures; i++)
    {
        m_NV12Textures[i].m_fenceValue = 0;
        m_NV12Textures[i].m_status = NV12Texture::Status_Free;
        m_AvailableTextures.push_back(static_cast<int>(i));

        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_GRAPHICS_PPV_ARGS(m_NV12Textures[i].m_pTexture.GetAddressOf())));

        swprintf_s(name, L"NV12 Texture %zu", i);
        m_NV12Textures[i].m_pTexture->SetName(name);

        // Y buffer inside the NV12 texture
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_R8_UNORM;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;   // Y is on plane 0 in NV12 texture
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(m_NV12Textures[i].m_pTexture.Get(), nullptr, &uavDesc,
                                                m_srvPile->GetCpuHandle(static_cast<size_t>(UAV_NV12Texture_Y_Data + i)));
        }

        // UV buffer inside the NV12 texture
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_R8G8_UNORM;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 1;   // UV is on plane 1 in NV12 texture
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            device->CreateUnorderedAccessView(m_NV12Textures[i].m_pTexture.Get(), nullptr, &uavDesc,
                                                m_srvPile->GetCpuHandle(static_cast<size_t>(UAV_NV12Texture_UV_Data + i)));
        }
    }

    // Thread for video encoding
    concurrency::task<bool> t([this]()
    {
        InitializeEncoder();

        while (!m_startEncoding.load())
        {
            SwitchToThread();
        }

        m_isEncoding = true;

        while (m_isEncoding.load())
        {
            EncodeVideo();
        }

        ShutdownEncoder();

        return true;
    });
}

void Sample::InitializeEncoder()
{
    m_isEncoding = false;
    IMFActivate** ppActivate = nullptr;
    
    // Initialize the Media Foundation platform.
    DX::ThrowIfFailed(MFStartup(MF_VERSION));

    // Call the MFCreateDXGIDeviceManager function to create the Direct3D device manager
    UINT32 uResetToken = 0;
    DX::ThrowIfFailed(MFCreateDXGIDeviceManager(&uResetToken, &m_pDXGIManager));

    // Call the MFResetDXGIDeviceManagerX function with a pointer to the Direct3D device
    auto device = m_deviceResources->GetD3DDevice();
    DX::ThrowIfFailed(MFResetDXGIDeviceManagerX(m_pDXGIManager.Get(), device, uResetToken));

    // Create H264 encoder MFT
    {
        MFT_REGISTER_TYPE_INFO inputInfo = {};
        inputInfo.guidMajorType = MFMediaType_Video;
        inputInfo.guidSubtype = MFVideoFormat_NV12;

        MFT_REGISTER_TYPE_INFO outputInfo = {};
        outputInfo.guidMajorType = MFMediaType_Video;
        outputInfo.guidSubtype = MFVideoFormat_H264;

        UINT32 count = 0;
        DX::ThrowIfFailed(MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, 0, &inputInfo, &outputInfo, &ppActivate, &count));
        assert(count == 1);

        DX::ThrowIfFailed(ppActivate[0]->ActivateObject(IID_PPV_ARGS(&m_pEncoder)));
    }

    // Unlock encoder - this is special for asynchronous MFT
    ComPtr<IMFAttributes> pMFAttributes;
    DX::ThrowIfFailed(m_pEncoder->GetAttributes(&pMFAttributes));
    DX::ThrowIfFailed(pMFAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, 1));

    // Set output type
    ComPtr<IMFMediaType> pOutputStreamType;
    DX::ThrowIfFailed(m_pEncoder->GetOutputAvailableType(0, 0, &pOutputStreamType));
    DX::ThrowIfFailed(pOutputStreamType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));

    // Encoder Settings
    // Using Main Profile as example, but any profile could be used. See the definition of eAVEncH264VProfile in codecapi.h
    DX::ThrowIfFailed(pOutputStreamType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Main));

    // Encoder level
    // Setting to -1 allows the encoder to set automatic default level based on other settings (Recommended)
    // For specific settings, see the definition of eAVEncH264VLevel in codecapi.h
    DX::ThrowIfFailed(pOutputStreamType->SetUINT32(MF_MT_MPEG2_LEVEL, (UINT32)-1));

    // Video width and height, then framerate
    DX::ThrowIfFailed(MFSetAttributeSize(pOutputStreamType.Get(), MF_MT_FRAME_SIZE, c_videoWidth, c_videoHeight));
    DX::ThrowIfFailed(MFSetAttributeSize(pOutputStreamType.Get(), MF_MT_FRAME_RATE, 30, 1));

    // Encoded video average bitrate
    DX::ThrowIfFailed(pOutputStreamType->SetUINT32(MF_MT_AVG_BITRATE, c_videoBitRate));
    DX::ThrowIfFailed(m_pEncoder->SetOutputType(0, pOutputStreamType.Get(), 0));

    // Set input type
    // NV12 video format is the only supported input type
    ComPtr<IMFMediaType> pInputStreamType;
    DX::ThrowIfFailed(MFCreateMediaType(&pInputStreamType));
    DX::ThrowIfFailed(pInputStreamType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    DX::ThrowIfFailed(pInputStreamType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
    DX::ThrowIfFailed(MFSetAttributeSize(pInputStreamType.Get(), MF_MT_FRAME_SIZE, c_videoWidth, c_videoHeight));
    DX::ThrowIfFailed(m_pEncoder->SetInputType(0, pInputStreamType.Get(), 0));

    // Send a SET_D3D_MANAGER message to the encoder MFT, passing in the IMFDXGIDeviceManager interface, and inherently the Direct3D device
    DX::ThrowIfFailed(m_pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(m_pDXGIManager.Get())));

    // When MFT_INPUT_STREAM_DOES_NOT_ADDREF flag is not set, the MFT might hold a reference count on 
    // the samples passed to the ProcessInput method. This means we cannot re-use or delete the input 
    // buffer memory until the MFT releases the sample's IMFSample pointer. Our implementation in this
    // sample is based on assumption that MFT_INPUT_STREAM_DOES_NOT_ADDREF is not set.
    MFT_INPUT_STREAM_INFO inputStreamInfo;
    DX::ThrowIfFailed(m_pEncoder->GetInputStreamInfo(0, &inputStreamInfo));
    assert((inputStreamInfo.dwFlags & MFT_INPUT_STREAM_DOES_NOT_ADDREF) == 0);

    // Get MFT event generator
    // This interface allows the encoder to communicate its state to the application
    DX::ThrowIfFailed(m_pEncoder->QueryInterface(__uuidof(IMFMediaEventGenerator), (void**)m_pMftEventGen.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(m_pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0));

    // Video output file
    m_hOutputFileHandle = CreateFile2(c_videoOutputFilePath, GENERIC_WRITE, 0, CREATE_ALWAYS, NULL);

    if (INVALID_HANDLE_VALUE == m_hOutputFileHandle)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Initialize the IMFSinkWriter for the output MP4 file
    ComPtr<IMFMediaType> pSinkOutputVideoMediaType;
    DX::ThrowIfFailed(MFCreateMediaType(&pSinkOutputVideoMediaType));
    DX::ThrowIfFailed(pSinkOutputVideoMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    DX::ThrowIfFailed(pSinkOutputVideoMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
    DX::ThrowIfFailed(pSinkOutputVideoMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    DX::ThrowIfFailed(MFSetAttributeRatio(pSinkOutputVideoMediaType.Get(), MF_MT_FRAME_RATE, 30, 1));
    DX::ThrowIfFailed(MFSetAttributeSize(pSinkOutputVideoMediaType.Get(), MF_MT_FRAME_SIZE, c_videoWidth, c_videoHeight));
    DX::ThrowIfFailed(MFSetAttributeRatio(pSinkOutputVideoMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
    DX::ThrowIfFailed(MFCreateSinkWriterFromURL(c_videoMP4FilePath, nullptr, nullptr, m_pSinkWriter.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(m_pSinkWriter->AddStream(pSinkOutputVideoMediaType.Get(), &m_videoStreamIndex));
    DX::ThrowIfFailed(m_pSinkWriter->BeginWriting());

    if (ppActivate[0])
    {
        ppActivate[0]->Release();
        ppActivate[0] = nullptr;
    }

    CoTaskMemFree(ppActivate);
    pMFAttributes.Reset();
    pOutputStreamType.Reset();
    pInputStreamType.Reset();
    pSinkOutputVideoMediaType.Reset();
}

void Sample::ShutdownEncoder()
{
    m_pMftEventGen.Reset();
    m_pMftEventGen = nullptr;

    m_pEncoder.Reset();
    m_pEncoder = nullptr;

    MFShutdown();
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
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    // Wait a few frames before we start encoding
    if (m_timer.GetFrameCount() >= 10)
    {
        m_startEncoding = true;
    }

    // Check if the GPU finished converting some backbuffers to NV12 textures
    while (!m_TexturesUsedByGPU.empty())
    {
        auto index = m_TexturesUsedByGPU.front();
        assert(m_NV12Textures[index].m_status == NV12Texture::Status_UsedByGPU);

        if (IsGPUDoneWithTexture(m_NV12Textures[index].m_fenceValue))
        {
            std::lock_guard<std::mutex> lock(m_sync);

            m_TexturesUsedByGPU.pop_front();
            m_TexturesReadyForEncoder.push_back(index);
            m_NV12Textures[index].m_status = NV12Texture::Status_ReadyForEncoder;
        }
    }

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Stop decoding
        if (pad.IsAPressed() && m_isEncoding.load())
        {
            m_stopEncoding = true;
        }

        if (pad.IsViewPressed())
        {
            if (m_isEncoding.load())
            {
                m_stopEncoding = true;
            }

            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
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

    // Render the frame
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    {
        // Render animated triangle
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Set constants
            float time = float(m_timer.GetTotalSeconds());
            commandList->SetGraphicsRoot32BitConstants(0, 1, &time, 0);

            // Draw triangle.
            commandList->DrawInstanced(3, 1, 0, 0);
        }

        // Render the UI
        RenderUI(commandList);
    }
    PIXEndEvent(commandList);

    // Convert this frame to NV12 using the GPU
    ConvertRGB2NV12();

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    {
        std::lock_guard<std::mutex> lock(m_syncCmdQueue);
        m_deviceResources->Present();
        m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    }
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
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    m_hudBatch->Begin(commandList);

    m_smallFont->DrawString(m_hudBatch.get(), L"MP4Encoder Sample", textPos, textColor);

    textPos.y += 80;
    m_smallFont->DrawString(m_hudBatch.get(), m_isEncoding.load() ? L"Encoding in progress" : L"Encoding finished", textPos, textColor);

    textPos.y += 40;
    wchar_t text[256] = {};
    swprintf_s(text, L"Number of frames encoded: %u", m_numFramesEncoded);
    m_smallFont->DrawString(m_hudBatch.get(), text, textPos, textColor);

    textPos.y += 40;
    m_smallFont->DrawString(m_hudBatch.get(), L"Encoded file: xbdir xd:\\MP4EncoderSampleOutput.mp4", textPos, textColor);

    textPos = XMFLOAT2(float(safe.left), float(safe.bottom - m_smallFont->GetLineSpacing()));

    const wchar_t* controlString = L"[A] - Stop Encoding           [View] - Exit Sample";
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), controlString, textPos, textColor);

    m_hudBatch->End();

    PIXEndEvent(commandList);
}

// Convert a texture to NV12 so that it can be used by the encoder
void Sample::ConvertRGB2NV12()
{
    if (m_AvailableTextures.empty())
    {
        // All textures are used either by the GPU or encoder, so we'll have to skip encoding this frame
        return;
    }

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ConvertRGB2NV12");

    // Get an available texture that we can use on the GPU
    int index = m_AvailableTextures.front();
    m_AvailableTextures.pop_front();
    assert(m_NV12Textures[index].m_status == NV12Texture::Status_Free);

    m_TexturesUsedByGPU.push_back(index);
    m_NV12Textures[index].m_status = NV12Texture::Status_UsedByGPU;

    auto src = m_deviceResources->GetRenderTarget();
    auto dst = m_NV12Textures[index].m_pTexture.Get();

    TransitionResource(commandList, src, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(commandList, dst, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetComputeRootSignature(m_nv12RootSignature.Get());
    commandList->SetComputeRootDescriptorTable(0, m_srvPile->GetGpuHandle(SRV_SwapBuffer + m_deviceResources->GetCurrentFrameIndex()));
    commandList->SetComputeRootDescriptorTable(1, m_srvPile->GetGpuHandle(static_cast<size_t>(UAV_NV12Texture_Y_Data + index)));
    commandList->SetComputeRootDescriptorTable(2, m_srvPile->GetGpuHandle(static_cast<size_t>(UAV_NV12Texture_UV_Data + index)));
    commandList->SetPipelineState(m_nv12PSO.Get());
    commandList->Dispatch(m_displayWidth / 8, m_displayHeight / 8, 1);

    TransitionResource(commandList, src, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionResource(commandList, dst, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);

    // Add a fence so that we know when it's safe to hand off to the encoder
    m_NV12Textures[index].m_fenceValue = AddTextureFence();

    PIXEndEvent(commandList);
}

void Sample::ProcessInput()
{
    ComPtr<IMFSample> pInputSample;
    pInputSample.Attach(CreateInputSample());

    if (pInputSample.Get())
    {
        m_pEncoder->ProcessInput(0, pInputSample.Get(), 0);
        m_processInputDeferred = false;
    }
}

// Encode a frame
void Sample::EncodeVideo()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"EncodeVideo");
    
    IMFMediaEvent* mftEvent = nullptr;
    MediaEventType eventType;

    if (m_stopEncoding.load())
    {
        DX::ThrowIfFailed(m_pEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0));
    }

    if (m_isEncoding.load())
    {
        // If there were no textures available during a METransformNeedInput event, we defer
        // the processing of the input sample. We cannot call GetEvent() until the previous
        // METransformNeedInput event was processed, since MFT won't send any more events
        // until ProcessInput() is called
        if (m_processInputDeferred.load())
        {
            std::lock_guard<std::mutex> lock(m_sync);

            if (m_TexturesReadyForEncoder.size())
            {
                ProcessInput();
            }
            else
            {
                // Still no textures available
                PIXEndEvent();  // EncodeVideo
                return;
            }
        }

        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Get MF Event");
        if (SUCCEEDED((m_pMftEventGen->GetEvent(0, &mftEvent))))
        {
            PIXEndEvent();  // "Get MF Event"

            DX::ThrowIfFailed(mftEvent->GetType(&eventType));

            if (eventType == METransformNeedInput)
            {
                // The encoder is ready for a new frame to encode, so we need to create a new
                // MF sample with a D3D resource and send it to the encoder

                PIXBeginEvent(PIX_COLOR_DEFAULT, L"Event: METransformNeedInput");

                std::lock_guard<std::mutex> lock(m_sync);

                if (m_TexturesReadyForEncoder.size())
                {
                    ProcessInput();
                }
                else
                {
                    // No teture is ready, so we defer ProcessInput
                    m_processInputDeferred = true;
                    OutputDebugStringA("ProcessInput was deferred");
                }

                PIXEndEvent();
            }
            else if (eventType == METransformHaveOutput)
            {
                // The encoder is done encoding a frame, so write it out to disk

                PIXBeginEvent(PIX_COLOR_DEFAULT, L"Event: METransformHaveOutput");

                DX::ThrowIfFailed(WriteEncodedFrameToDisk());

                // Encoder is done with this texture, it's now available again
                auto index = m_TexturesUsedByEncoder.front();
                m_TexturesUsedByEncoder.pop_front();
                assert(m_NV12Textures[index].m_status == NV12Texture::Status_UsedByEncoder);

                m_AvailableTextures.push_back(index);
                m_NV12Textures[index].m_status = NV12Texture::Status_Free;

                PIXEndEvent();
            }
            else if (eventType == METransformDrainComplete)
            {
                // We're done encoding, so lets start shutting down the encoder

                ComPtr<IMFShutdown> pShutdownObj;
                DX::ThrowIfFailed(m_pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0));

                // Shutdown the H264 encoder MFT
                DX::ThrowIfFailed(m_pEncoder->QueryInterface(__uuidof(IMFShutdown), (void**)&pShutdownObj));
                DX::ThrowIfFailed(pShutdownObj->Shutdown());
                m_isEncoding = false;

                // Finalize the output Mp4
                m_pSinkWriter->Finalize();

                pShutdownObj.Reset();

                // Close the output file
                if (m_hOutputFileHandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_hOutputFileHandle);
                    m_hOutputFileHandle = INVALID_HANDLE_VALUE;
                }
            }
        }
        else
        {
            PIXEndEvent();  // "Get MF Event"
        }
    }

    if (mftEvent)
    {
        mftEvent->Release();
    }

    PIXEndEvent();
}

IMFSample* Sample::CreateInputSample()
{
    if (!m_TexturesReadyForEncoder.size())
    {
        // We don't have a texture ready to encode
        return nullptr;
    }

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Create MF Sample for Encoder");

    ComPtr<IMFSample> pMFSample = nullptr;

    // Create a new sample
    DX::ThrowIfFailed(MFCreateSample(&pMFSample));
    DX::ThrowIfFailed(pMFSample->SetSampleTime(m_videoTimeStamp));
    DX::ThrowIfFailed(pMFSample->SetSampleDuration(c_videoFrameDuration));
    m_videoTimeStamp += c_videoFrameDuration;

    // Get a texture that's ready to be encoded
    auto index = m_TexturesReadyForEncoder.front();
    m_TexturesReadyForEncoder.pop_front();
    assert(m_NV12Textures[index].m_status == NV12Texture::Status_ReadyForEncoder);

    m_TexturesUsedByEncoder.push_back(index);
    m_NV12Textures[index].m_status = NV12Texture::Status_UsedByEncoder;

    auto pNV12Texture = m_NV12Textures[index].m_pTexture.Get();

    ID3D12Device* pDevice = nullptr;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    ComPtr<IMFDXGIBuffer> pDXGIBuffer;
    ComPtr<IMFMediaBuffer> pInputBuffer;
    ComPtr<IMFD3D12SynchronizationObjectCommands> pMFSyncObj;

    // Set a NV12 texture as input for the sample
    DX::ThrowIfFailed(m_pDXGIManager->OpenDeviceHandle(&hDevice));
    DX::ThrowIfFailed(m_pDXGIManager->LockDevice(hDevice, IID_GRAPHICS_PPV_ARGS(&pDevice), true));   
    DX::ThrowIfFailed(MFCreateDXGISurfaceBufferX(__uuidof(pNV12Texture), pNV12Texture, 0, FALSE, &pInputBuffer));
    DX::ThrowIfFailed(pMFSample->AddBuffer(pInputBuffer.Get()));

    // Mark sample as ready to use
    if (SUCCEEDED(pInputBuffer.As(&pDXGIBuffer)))
    {
        if (SUCCEEDED(pDXGIBuffer->GetUnknown(MF_D3D12_SYNCHRONIZATION_OBJECT, IID_PPV_ARGS(&pMFSyncObj))))
        {
            // We're using the same D3D command queue on this encoder thread and the rendering thread,
            // so put a mutex lock around it
            std::lock_guard<std::mutex> lock(m_syncCmdQueue);
            pMFSyncObj->EnqueueResourceReady(m_deviceResources->GetCommandQueue());
        }
    }

    if (hDevice != INVALID_HANDLE_VALUE)
    {
        DX::ThrowIfFailed(m_pDXGIManager->UnlockDevice(hDevice, 0));
        DX::ThrowIfFailed(m_pDXGIManager->CloseDeviceHandle(hDevice));
    }

    PIXEndEvent();

    return pMFSample.Detach();
}

HRESULT Sample::WriteEncodedFrameToDisk()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"WriteEncodedFrameToDisk");

    HRESULT hr = S_OK;

    MFT_OUTPUT_STREAM_INFO outputStreamInfo = {};
    DX::ThrowIfFailed(m_pEncoder->GetOutputStreamInfo(0, &outputStreamInfo));

    ComPtr<IMFMediaBuffer> pOutputBuffer = nullptr;
    ComPtr<IMFSample> outputSample;
    MFT_OUTPUT_DATA_BUFFER mftOutputDataBuffer {};

    DX::ThrowIfFailed(m_pEncoder->GetOutputStreamInfo(0, &outputStreamInfo));

    if ((outputStreamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) == 0)
    {
        DX::ThrowIfFailed(MFCreateAlignedMemoryBuffer(outputStreamInfo.cbSize, outputStreamInfo.cbAlignment, &pOutputBuffer));
        DX::ThrowIfFailed(MFCreateSample(&outputSample));
        DX::ThrowIfFailed(outputSample->AddBuffer(pOutputBuffer.Get()));
        mftOutputDataBuffer.pSample = outputSample.Detach();
    }
    else
    {
        mftOutputDataBuffer.pSample = nullptr;
    }

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"MF ProcessOutput");
    DWORD status = 0;
    HRESULT processOutputResult = m_pEncoder->ProcessOutput(0, 1, &mftOutputDataBuffer, &status);
    PIXEndEvent();

    if (SUCCEEDED(processOutputResult) && (mftOutputDataBuffer.pSample != nullptr))
    {
        m_numFramesEncoded++;

        outputSample.Attach(mftOutputDataBuffer.pSample);

        if ((outputStreamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
        {
            DX::ThrowIfFailed(outputSample->GetBufferByIndex(0, &pOutputBuffer));
        }

        DWORD bufferCount = 0;
        DX::ThrowIfFailed(outputSample->GetBufferCount(&bufferCount));
        assert(bufferCount == 1);

        BYTE* p;
        DWORD length;
        DX::ThrowIfFailed(pOutputBuffer->Lock(&p, nullptr, &length));

        if (INVALID_HANDLE_VALUE != m_hOutputFileHandle)
        {
            PIXBeginEvent(PIX_COLOR_DEFAULT, L"WriteFile");

            DWORD writeCount;
            if (0 == WriteFile(m_hOutputFileHandle, p, length, &writeCount, nullptr))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DX::ThrowIfFailed(hr);
            }

            PIXEndEvent();
        }

        DX::ThrowIfFailed(pOutputBuffer->Unlock());

        if (m_pSinkWriter)
        {
            if (m_numFramesEncoded == 1)
            {
                outputSample->SetUINT32(MFSampleExtension_CleanPoint, TRUE);
            }

            // Both encoder and decoder MFTs are required to emit sample time for output samples.
            // There is no need to set estimated sample time here by the app.
            // 
            // Sample duration is optional. However, we already set sample duration on input samples,
            // which will be passed to output samples by encoder MFT.
            DX::ThrowIfFailed(m_pSinkWriter->WriteSample(m_videoStreamIndex, outputSample.Get()));
        }
    }

    PIXEndEvent();

    return processOutputResult;
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
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_srvPile = std::make_unique<DescriptorPile>(
        device,
        128,
        SRV_Count);

    // Create PSO for rendering triangle
    {
        auto vertexShaderBlob = DX::ReadData(L"VertexShader.cso");
        auto pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

        DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                                                        IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
    }

    // Create PSO for converting from RGB to NV12
    {
        auto shaderBlob = DX::ReadData(L"RGB2NV12.cso");

        DX::ThrowIfFailed(device->CreateRootSignature(0, shaderBlob.data(), shaderBlob.size(),
                                                        IID_GRAPHICS_PPV_ARGS(m_nv12RootSignature.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_nv12RootSignature.Get();
        psoDesc.CS.pShaderBytecode = shaderBlob.data();
        psoDesc.CS.BytecodeLength = shaderBlob.size();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_nv12PSO.ReleaseAndGetAddressOf())));
    }

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();

    // Create the UI sprite batch
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        auto backBufferRTs = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTs, &CommonStates::AlphaBlend);

        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }

    // Create a fence for tracking when the GPU has produced a NV12 texture for the encoder
    DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_NV12TextureFence.ReleaseAndGetAddressOf())));
    m_NV12TextureFence->SetName(L"NV12Texture Fence");

    m_NV12TextureFenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_NV12TextureFenceEvent.IsValid())
    {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto const size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint32_t>(size.right - size.left);
    m_displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload, L"SegoeUI_18.spritefont",
                                                    m_srvPile->GetCpuHandle(SRV_Font), m_srvPile->GetGpuHandle(SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerLegendSmall.spritefont",
                                                    m_srvPile->GetCpuHandle(SRV_CtrlFont), m_srvPile->GetGpuHandle(SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}

UINT64 Sample::AddTextureFence()
{
    static UINT64 fenceValue = 0;

    fenceValue++;
    DX::ThrowIfFailed(m_deviceResources->GetCommandQueue()->Signal(m_NV12TextureFence.Get(), fenceValue));

    return fenceValue;
}

void Sample::WaitForTextureFence(UINT64 fenceValue, DWORD timeout)
{
    if (m_NV12TextureFence->GetCompletedValue() < fenceValue)
    {
        DX::ThrowIfFailed(m_NV12TextureFence->SetEventOnCompletion(fenceValue, m_NV12TextureFenceEvent.Get()));
        WaitForSingleObject(m_NV12TextureFenceEvent.Get(), timeout);
    }
}

bool Sample::IsGPUDoneWithTexture(UINT64 fenceValue)
{
    return (m_NV12TextureFence->GetCompletedValue() >= fenceValue);
}

#pragma endregion
