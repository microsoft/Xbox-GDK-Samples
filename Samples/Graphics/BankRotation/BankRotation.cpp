//--------------------------------------------------------------------------------------
// BankRotation.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BankRotation.h"

#include "ATGColors.h"
#include "CommonStates.h"
#include "ReadData.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    enum class DescriptorHeapEntry
    {
        TextFont,
        ControllerFont,
        MeshTexture,
        TotalDescriptorHeapEntryCount = 128
    };

    enum class TimerCounters
    {
        ROTATED,
        NON_ROTATED,
        TOTAL_PASS_TIMERS,
        SRV_ACCESS_ROTATED,
        SRV_ACCESS_NON_ROTATED,
        TOTAL_TIMERS,
    };

    static const wchar_t* TimerCounterNames[] =
    {
        L"Rotated",
        L"Non-Rotated",
        L"Pass timers",
        L"SRV Access Rotated",
        L"SRV Access Non-Rotated",
        L"Total Timers",
    };
    static_assert(_countof(TimerCounterNames) == static_cast<uint32_t>(TimerCounters::TOTAL_TIMERS) + 1, "Mismatch in timer counter values");

    static const wchar_t* ResourceTypeName[] =
    {
        L"COMMITTED",
        L"PLACED",
        L"COMPONENT PLACED",
    };
    static_assert(_countof(ResourceTypeName) == static_cast<uint32_t>(ResourceType::TOTAL_RESOURCE_TYPES), "Mismatch in resource type values");

    const wchar_t* g_sampleTitle = L"Bank Rotation Sample";
    const wchar_t* g_sampleDescription = L"Sample shows how to use bank rotation for render targets";
    const ATG::HelpButtonAssignment g_helpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Show/hide help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::LEFT_STICK,          L"Translate camera" },
        { ATG::HelpID::RIGHT_STICK,         L"Rotate camera" },
    };

#ifdef _GAMING_XBOX_SCARLETT
    D3D12_TEXTURE_LAYOUT ConvertSwizzleModeToTextureLayout(XG_SWIZZLE_MODE tileMode) { return static_cast<D3D12_TEXTURE_LAYOUT>(tileMode | XG_TEXTURE_LAYOUT_SWIZZLE_MODE_LINEAR); }
#else
    D3D12_TEXTURE_LAYOUT ConvertTileModeToTextureLayout(XG_TILE_MODE tileMode) { return static_cast<D3D12_TEXTURE_LAYOUT>(tileMode | D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_0); }
#endif

}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_descrptorIncrementSizeCBVSRVUAV(0),
    m_rectVBView{},
    m_showHelp(false),
    m_resourceType(ResourceType::COMMITTED)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
         DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
         2, DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_help = std::make_unique<ATG::Help>(g_sampleTitle, g_sampleDescription, g_helpButtons, _countof(g_helpButtons), true);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    m_deviceResources->CreateWindowSizeDependentResources();

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showHelp = !m_showHelp;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_resourceType = static_cast<ResourceType>((static_cast<uint32_t>(m_resourceType) + 1) % static_cast<uint32_t>(ResourceType::TOTAL_RESOURCE_TYPES));
            m_gpuTimer->Reset();
            m_cpuTimer->Reset();
        }

        m_camera->Update(elapsedTime, pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto viewProj = m_camera->GetView() * m_camera->GetProjection();
    m_modelEffect.m_effect->UpdateConstants(viewProj);
}
#pragma endregion

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

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
    m_gpuTimer->BeginFrame(commandList);

    if (m_showHelp)
    {
        m_help->Render(commandList);
    }
    else
    {
        auto currFrameIndex = m_deviceResources->GetCurrentFrameIndex();

        // Measure total frame time
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass");

        D3D12XBOX_FLUSH flushFlags = D3D12XBOX_FLUSH_BOP_PS_PARTIAL | D3D12XBOX_FLUSH_BOP_TEXTURE_L1_INVALIDATE | D3D12XBOX_FLUSH_BOP_TEXTURE_L2_INVALIDATE |
            D3D12XBOX_FLUSH_BOP_TEXTURE_L2_INVALIDATE | D3D12XBOX_FLUSH_BOP_COLOR_BLOCK_DATA | D3D12XBOX_FLUSH_BOP_COLOR_BLOCK_META |
            D3D12XBOX_FLUSH_BOP_DEPTH_BLOCK_DATA | D3D12XBOX_FLUSH_BOP_DEPTH_BLOCK_META;
        switch (m_resourceType)
        {
        case ResourceType::COMMITTED:
        default:
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Committed");
            RenderMesh(commandList, m_gbuffersCommitted, currFrameIndex, static_cast<uint32_t>(TimerCounters::NON_ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersCommitted, false);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Committed Rotated");
            RenderMesh(commandList, m_gbuffersCommittedRotated, currFrameIndex, static_cast<uint32_t>(TimerCounters::ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersCommittedRotated, true);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        break;

        case ResourceType::PLACED:
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Placed Rotated");
            RenderMesh(commandList, m_gbuffersPlacedRotated, currFrameIndex, static_cast<uint32_t>(TimerCounters::ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersPlacedRotated, true);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Placed");
            RenderMesh(commandList, m_gbuffersPlaced, currFrameIndex, static_cast<uint32_t>(TimerCounters::NON_ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersPlaced, false);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        break;

        case ResourceType::COMPONENT_PLACED:
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Component Placed Rotated");
            RenderMesh(commandList, m_gbuffersComponentPlacedRotated, currFrameIndex, static_cast<uint32_t>(TimerCounters::ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersComponentPlacedRotated, true);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GBuffer Pass - Component Placed");
            RenderMesh(commandList, m_gbuffersComponentPlaced, currFrameIndex, static_cast<uint32_t>(TimerCounters::NON_ROTATED));
            PIXEndEvent(commandList);
            RenderGbufferToBackbuffer(commandList, m_gbuffersComponentPlaced, false);
            // Flush Pipeline after the pass
            commandList->FlushPipelineX(flushFlags, {}, 0);
        }
        break;
        }

        PIXEndEvent(commandList); // GBuffer Pass
        PIXEndEvent(commandList); // Render

        RenderUI(commandList);
    }

    m_gpuTimer->EndFrame(commandList);
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
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    for (uint32_t gbufferIndex = 0; gbufferIndex < c_NumGBuffersPerPass; ++gbufferIndex)
    {
        switch (m_resourceType)
        {
        case ResourceType::COMMITTED:
        default:
            commandList->ClearRenderTargetView(m_gbuffersCommitted[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            commandList->ClearRenderTargetView(m_gbuffersCommittedRotated[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            break;

        case ResourceType::PLACED:
            commandList->ClearRenderTargetView(m_gbuffersPlaced[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            commandList->ClearRenderTargetView(m_gbuffersPlacedRotated[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            break;

        case ResourceType::COMPONENT_PLACED:
            commandList->ClearRenderTargetView(m_gbuffersComponentPlaced[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            commandList->ClearRenderTargetView(m_gbuffersComponentPlacedRotated[gbufferIndex].m_rtvCpuHandle, ATG::Colors::Background, 0, nullptr);
            break;
        }
    }

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

// Render Mesh
void Sample::RenderMesh(ID3D12GraphicsCommandList* commandList, GBuffer* gbuffers, uint32_t currFrameIndex, uint32_t timerCounter)
{
    // Clear depth stencil on each pass so that the numbers are consistent
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_cpuTimer->Start(timerCounter);
    m_gpuTimer->Start(commandList, timerCounter);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[c_NumGBuffersPerPass];
    for (uint32_t gbufferIndex = 0; gbufferIndex < c_NumGBuffersPerPass; ++gbufferIndex)
    {
        if (gbuffers[gbufferIndex].m_state != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(gbuffers[gbufferIndex].m_resource.Get(), gbuffers[gbufferIndex].m_state, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->ResourceBarrier(1, &barrier);
            gbuffers[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        rtvDescriptor[gbufferIndex] = gbuffers[gbufferIndex].m_rtvCpuHandle;
    }

    m_modelEffect.m_effect->SetRootSignature(commandList);
    m_modelEffect.m_effect->SetAllDirtyFlags();
    auto gpuDescriptorHandle = m_modelEffect.m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();
    gpuDescriptorHandle.ptr += m_descrptorIncrementSizeCBVSRVUAV * static_cast<size_t>(DescriptorHeapEntry::MeshTexture);
    m_modelEffect.m_effect->SetCurrentStateArgs(currFrameIndex, 0, gpuDescriptorHandle);
    m_modelEffect.m_effect->Apply(commandList);

    commandList->OMSetRenderTargets(c_NumGBuffersPerPass, rtvDescriptor, FALSE, &dsvDescriptor);

    m_modelEffect.m_model->Draw(commandList, m_modelEffect.m_meshPartsEffects.begin());

    m_gpuTimer->Stop(commandList, timerCounter);
    m_cpuTimer->Stop(timerCounter);
}

void Sample::RenderGbufferToBackbuffer(ID3D12GraphicsCommandList* commandList, GBuffer* currGbuffer, bool rotated)
{
    uint32_t timer = rotated ? static_cast<uint32_t>(TimerCounters::SRV_ACCESS_ROTATED) : static_cast<uint32_t>(TimerCounters::SRV_ACCESS_NON_ROTATED);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"SRV Access");
    m_cpuTimer->Start(timer);
    m_gpuTimer->Start(commandList, timer);

    D3D12_RESOURCE_BARRIER resBarrierRtvToSrv[c_NumGBuffersPerPass];
    for (uint32_t i = 0; i < c_NumGBuffersPerPass; ++i)
    {
        resBarrierRtvToSrv[i] = CD3DX12_RESOURCE_BARRIER::Transition(currGbuffer[i].m_resource.Get(), currGbuffer[i].m_state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
    }
    commandList->ResourceBarrier(_countof(resBarrierRtvToSrv), resBarrierRtvToSrv);

    RenderRect(commandList, currGbuffer, rotated);
    m_cpuTimer->Stop(timer);
    m_gpuTimer->Stop(commandList, timer);
    PIXEndEvent(commandList);

    D3D12_RESOURCE_BARRIER resBarrierSrvToRtv[c_NumGBuffersPerPass];
    for (uint32_t i = 0; i < c_NumGBuffersPerPass; ++i)
    {
        resBarrierSrvToRtv[i] = CD3DX12_RESOURCE_BARRIER::Transition(currGbuffer[i].m_resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, currGbuffer[i].m_state, 0);
    }
    commandList->ResourceBarrier(_countof(resBarrierSrvToRtv), resBarrierSrvToRtv);
}

void Sample::RenderRect(ID3D12GraphicsCommandList* commandList, GBuffer* currGbuffer, bool rotated)
{
    commandList->SetPipelineState(m_rectPso.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_RECTLIST);
    commandList->IASetVertexBuffers(0, 1, &m_rectVBView);
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    auto viewport = m_deviceResources->GetScreenViewport();
    float invAspectRatio = viewport.Height / viewport.Width;
    viewport.Width /= (c_NumGBuffersPerPass / 2.f);
    viewport.Height = viewport.Width * invAspectRatio;

    uint32_t startIndex = rotated ? 1u : 0u;

    for (uint32_t i = startIndex; i < c_NumGBuffersPerPass; i += 2)
    {
        viewport.TopLeftX = (i / 2) * viewport.Width;
        viewport.TopLeftY = (i % 2) * viewport.Height;
        commandList->RSSetViewports(1, &viewport);
        commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(RootParameterIndex::DescriptorTableSRV), currGbuffer[i].m_srvGpuHandle);
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    // Reset Viewport
    auto const originalViewport = m_deviceResources->GetScreenViewport();
    commandList->RSSetViewports(1, &originalViewport);
}

// Render UI
void Sample::RenderUI(ID3D12GraphicsCommandList * graphicsCmdList)
{
    PIXBeginEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render UI");
    {
        auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
        graphicsCmdList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        XMVECTOR textPosition = XMVectorSet(50, 50, 0, 1);
        XMVECTOR textColor = ATG::ColorsLinear::Blue;
        auto outputSize = m_deviceResources->GetOutputSize();
        XMVECTOR textDiffInY = { 0.f, (outputSize.bottom > 1080) ? 60.0f : 35.0f, 0.f, 0.f };

        m_fontBatch->Begin(graphicsCmdList);

        m_fontText->DrawString(m_fontBatch.get(), g_sampleTitle, textPosition, textColor);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        m_fontText->DrawString(m_fontBatch.get(), g_sampleDescription, textPosition, textColor);

        wchar_t bufInfo[1024] = {};
        XMVECTOR textPositionMoveX;

        // Pass timings
        for (uint32_t passID = 0; passID < static_cast<uint32_t>(TimerCounters::TOTAL_PASS_TIMERS); ++passID)
        {
            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"Pass Timings - %ls", TimerCounterNames[passID]);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

            textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));
            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"GPU Average: %.2f ms", m_gpuTimer->GetAverageMS(passID));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionMoveX, textColor);

            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"GPU Elapsed: %.2f ms", m_gpuTimer->GetElapsedMS(passID));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionMoveX, textColor);

            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"CPU Elapsed: %.2f ms", m_cpuTimer->GetElapsedMS(passID));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionMoveX, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionMoveX));
        }

        // Calculate percent gain when rotating
        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"Gain when Rotating");
        textPosition = XMVectorAdd(textPosition, XMVectorScale(textDiffInY, 2));
        m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

        double diff = (m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::NON_ROTATED)) - m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::ROTATED)));
        double percentDiff = diff * 100.0 / m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::NON_ROTATED));
        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"Writing Gbuffers: %.2f%%", percentDiff);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionMoveX, textColor);

        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[A] - Resource Type - %s", ResourceTypeName[static_cast<uint32_t>(m_resourceType)]);
        textPosition = XMVectorAdd(textPosition, XMVectorScale(textDiffInY, 2));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[Menu] - Show Help Screen");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        // End font batch
        m_fontBatch->End();
    }
    PIXEndEvent(graphicsCmdList);
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

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    m_descrptorIncrementSizeCBVSRVUAV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create a texture factory for loading the model textures
    m_modelEffect.m_textures = std::make_unique<EffectTextureFactory>(device, resourceUpload, (size_t)DescriptorHeapEntry::TotalDescriptorHeapEntryCount);

    // Load the assets
    CreateResources(device);
    LoadMesh(device, resourceUpload);
    CreateRectResources(device, resourceUpload);
    InitializeSpriteFonts(device, resourceUpload, rtState);
    m_help->RestoreDevice(device, resourceUpload, rtState);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_cpuTimer = std::make_unique<DX::CPUTimer>();
    auto resourceUploadEvent = resourceUpload.End(m_deviceResources->GetCommandQueue());
    // Wait for resources to upload
    resourceUploadEvent.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize camera
    m_camera = std::make_unique<DX::FlyCamera>();
    m_camera->SetWindow(static_cast<int>(m_deviceResources->GetScreenViewport().Width), static_cast<int>(m_deviceResources->GetScreenViewport().Height));
    static const XMVECTORF32 s_camPosition = { 48.76f, 15.47f, 5.84f, 1.0f };
    static const XMVECTORF32 s_camRotation = { 0.012f, 0.70f, -0.01f, 0.71f };
    m_camera->SetPosition(s_camPosition);
    m_camera->SetRotation(s_camRotation);
    m_camera->SetSensitivity(100.0f, 100.0f, 100.0f, 0.0f);
    m_camera->SetRotationRate(0.5f);
    m_camera->SetFlags(m_camera->GetFlags() | DX::FlyCamera::c_FlagsDisableSensitivityControl);

    // Help
    m_help->SetWindow(m_deviceResources->GetOutputSize());

    // Font
    m_fontBatch->SetViewport(m_deviceResources->GetScreenViewport());
}

// Load the mesh
void Sample::LoadMesh(ID3D12Device * device, ResourceUploadBatch& resourceUpload)
{
    RenderTargetState rtStateGBuffer;
    rtStateGBuffer.numRenderTargets = c_NumGBuffersPerPass;
    for (uint32_t i = 0; i < c_NumGBuffersPerPass; ++i)
    {
        rtStateGBuffer.rtvFormats[i] = gbufferFormat[i];
    }
    rtStateGBuffer.dsvFormat = m_deviceResources->GetDepthBufferFormat();
    auto psd = EffectPipelineStateDescription(nullptr, CommonStates::Opaque, CommonStates::DepthDefault, CommonStates::CullNone, rtStateGBuffer);

    // Load Shaders
    std::vector<uint8_t> meshVSBlob = DX::ReadData(L"MeshVS.cso");
    std::vector<uint8_t> meshPSBlob = DX::ReadData(L"MeshPS.cso");
    D3D12_SHADER_BYTECODE vsByteCode = { meshVSBlob.data(), meshVSBlob.size() };
    D3D12_SHADER_BYTECODE psByteCode = { meshPSBlob.data(), meshPSBlob.size() };

    m_modelEffect.m_model = Model::CreateFromSDKMESH(device, L"Building_Individual.sdkmesh");

    auto sampleModel = m_modelEffect.m_model.get();
    auto modelMeshPart = sampleModel->meshes[0]->opaqueMeshParts[0].get();
    D3D12_INPUT_LAYOUT_DESC inputLayoutModel =
    {
        modelMeshPart->vbDecl->data(),
        (UINT)modelMeshPart->vbDecl->size()
    };
    psd.inputLayout = inputLayoutModel;

    m_effectFactory = std::make_unique<SampleEffectFactory>(device,
        m_modelEffect.m_textures->Heap()->GetGPUDescriptorHandleForHeapStart());

    // Create Effect to create root signature and PSO
    m_modelEffect.m_effect = std::static_pointer_cast<SampleEffect>(m_effectFactory->CreateSampleEffect(&psd,
        m_deviceResources->GetBackBufferCount(),
        &vsByteCode,
        &psByteCode));

    sampleModel->LoadTextures(*(m_modelEffect.m_textures).get(), (int)(DescriptorHeapEntry::MeshTexture));

    // Effect for each of the mesh parts in the model. This effect just stores the
    // data which is different for each of the individual mesh parts
    m_modelEffect.m_meshPartsEffects = sampleModel->CreateEffects(*(m_effectFactory.get()), psd, psd, (int)(DescriptorHeapEntry::MeshTexture));

    // Optimize mesh performance
    m_modelEffect.m_model->LoadStaticBuffers(device, resourceUpload);
}

// Create all required resources
void Sample::CreateResources(ID3D12Device* device)
{
    auto outputRect = m_deviceResources->GetOutputSize();
    auto texWidth = uint32_t(outputRect.right - outputRect.left);
    auto texHeight = uint32_t(outputRect.bottom - outputRect.top);

    // Create descriptor heap for RTVs
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = c_NumGBuffersPerPass * c_NumGbufferSets;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    m_rtvDescriptorHeap->SetName(L"RTV Heap");

    // Create descriptor heap for SRVs
    D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
    srvDescriptorHeapDesc.NumDescriptors = c_NumGBuffersPerPass * c_NumGbufferSets;
    srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_srvDescriptorHeap.ReleaseAndGetAddressOf())));
    m_srvDescriptorHeap->SetName(L"SRV Heap");

    // Get the handle for each of the render targets and shader resource view
    uint32_t rtvHeapDescriptorIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    uint32_t srvHeapDescriptorIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    auto rtvCpuHandlePtr = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvCpuHandlePtr = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto srvGpuHandlePtr = m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    m_gbuffersCommitted[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersCommitted[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersCommitted[0].m_srvGpuHandle = srvGpuHandlePtr;
    rtvCpuHandlePtr.ptr += rtvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvCpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvGpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    m_gbuffersCommittedRotated[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersCommittedRotated[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersCommittedRotated[0].m_srvGpuHandle = srvGpuHandlePtr;

    rtvCpuHandlePtr.ptr += rtvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvCpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvGpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    m_gbuffersPlaced[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersPlaced[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersPlaced[0].m_srvGpuHandle = srvGpuHandlePtr;
    rtvCpuHandlePtr.ptr += rtvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvCpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvGpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    m_gbuffersPlacedRotated[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersPlacedRotated[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersPlacedRotated[0].m_srvGpuHandle = srvGpuHandlePtr;

    rtvCpuHandlePtr.ptr += rtvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvCpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvGpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    m_gbuffersComponentPlaced[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersComponentPlaced[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersComponentPlaced[0].m_srvGpuHandle = srvGpuHandlePtr;
    rtvCpuHandlePtr.ptr += rtvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvCpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    srvGpuHandlePtr.ptr += srvHeapDescriptorIncSize * c_NumGBuffersPerPass;
    m_gbuffersComponentPlacedRotated[0].m_rtvCpuHandle = rtvCpuHandlePtr;
    m_gbuffersComponentPlacedRotated[0].m_srvCpuHandle = srvCpuHandlePtr;
    m_gbuffersComponentPlacedRotated[0].m_srvGpuHandle = srvGpuHandlePtr;

    for (uint32_t i = 1; i < c_NumGBuffersPerPass; ++i)
    {
        m_gbuffersCommitted[i].m_rtvCpuHandle.ptr = m_gbuffersCommitted[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;
        m_gbuffersCommittedRotated[i].m_rtvCpuHandle.ptr = m_gbuffersCommittedRotated[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;
        m_gbuffersPlaced[i].m_rtvCpuHandle.ptr = m_gbuffersPlaced[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;
        m_gbuffersPlacedRotated[i].m_rtvCpuHandle.ptr = m_gbuffersPlacedRotated[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlaced[i].m_rtvCpuHandle.ptr = m_gbuffersComponentPlaced[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlacedRotated[i].m_rtvCpuHandle.ptr = m_gbuffersComponentPlacedRotated[0].m_rtvCpuHandle.ptr + rtvHeapDescriptorIncSize * i;

        m_gbuffersCommitted[i].m_srvCpuHandle.ptr = m_gbuffersCommitted[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersCommittedRotated[i].m_srvCpuHandle.ptr = m_gbuffersCommittedRotated[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersPlaced[i].m_srvCpuHandle.ptr = m_gbuffersPlaced[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersPlacedRotated[i].m_srvCpuHandle.ptr = m_gbuffersPlacedRotated[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlaced[i].m_srvCpuHandle.ptr = m_gbuffersComponentPlaced[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlacedRotated[i].m_srvCpuHandle.ptr = m_gbuffersComponentPlacedRotated[0].m_srvCpuHandle.ptr + srvHeapDescriptorIncSize * i;

        m_gbuffersCommitted[i].m_srvGpuHandle.ptr = m_gbuffersCommitted[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersCommittedRotated[i].m_srvGpuHandle.ptr = m_gbuffersCommittedRotated[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersPlaced[i].m_srvGpuHandle.ptr = m_gbuffersPlaced[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersPlacedRotated[i].m_srvGpuHandle.ptr = m_gbuffersPlacedRotated[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlaced[i].m_srvGpuHandle.ptr = m_gbuffersComponentPlaced[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
        m_gbuffersComponentPlacedRotated[i].m_srvGpuHandle.ptr = m_gbuffersComponentPlacedRotated[0].m_srvGpuHandle.ptr + srvHeapDescriptorIncSize * i;
    }

    XG_RESOURCE_LAYOUT* xgResLayout = new XG_RESOURCE_LAYOUT[c_NumGBuffersPerPass]();
    D3D12_RESOURCE_DESC resDescNonRotated[c_NumGBuffersPerPass];
    D3D12_CLEAR_VALUE clearValue[c_NumGBuffersPerPass];

    for (uint32_t gbufferIndex = 0; gbufferIndex < c_NumGBuffersPerPass; ++gbufferIndex)
    {
        // Set Initial resource state
        m_gbuffersCommitted[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_gbuffersCommittedRotated[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_gbuffersPlaced[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_gbuffersPlacedRotated[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_gbuffersComponentPlaced[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_gbuffersComponentPlacedRotated[gbufferIndex].m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;

        resDescNonRotated[gbufferIndex] = CD3DX12_RESOURCE_DESC::Tex2D(gbufferFormat[gbufferIndex], texWidth, texHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        clearValue[gbufferIndex] = {
            gbufferFormat[gbufferIndex],                  // DXGI_FORMAT Format;
            {
                { 0, 0, 0, 0 },                           // FLOAT Color[4];
            }
        };

        // Get Layout
        XGTextureAddressComputer* texComputer = nullptr;
        DX::ThrowIfFailed(XGCreateTextureComputer((XG_RESOURCE_DESC*)&(resDescNonRotated[gbufferIndex]), &texComputer));

        DX::ThrowIfFailed(texComputer->GetResourceLayout(&(xgResLayout[gbufferIndex])));
#ifdef _GAMING_XBOX_SCARLETT
        resDescNonRotated[gbufferIndex].Layout = ConvertSwizzleModeToTextureLayout(xgResLayout[gbufferIndex].Plane[0].MipLayout[0].SwizzleMode);
#else
        resDescNonRotated[gbufferIndex].Layout = ConvertTileModeToTextureLayout(xgResLayout[gbufferIndex].Plane[0].MipLayout[0].TileMode);
#endif
    }

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    for (uint32_t gbufferIndex = 0; gbufferIndex < c_NumGBuffersPerPass; ++gbufferIndex)
    {
        uint32_t bankRotationIndex = gbufferIndex;
        D3D12_RESOURCE_DESC resDescRotated = resDescNonRotated[gbufferIndex];

#ifdef _GAMING_XBOX_SCARLETT
        resDescRotated.Layout = D3D12XBOX_BANK_ROTATED_SWIZZLE_MODE(resDescNonRotated[gbufferIndex].Layout, bankRotationIndex);
       
#else
        resDescRotated.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(resDescNonRotated[gbufferIndex].Layout, bankRotationIndex);
#endif

        // Committed Resources
        {
            device->CreateCommittedResource(&heapProperties,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &resDescNonRotated[gbufferIndex],
                m_gbuffersCommitted[gbufferIndex].m_state,
                &clearValue[gbufferIndex],
                IID_GRAPHICS_PPV_ARGS(m_gbuffersCommitted[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersCommitted[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);

            device->CreateRenderTargetView(m_gbuffersCommitted[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersCommitted[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersCommitted[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersCommitted[gbufferIndex].m_srvCpuHandle);

            // Committed Rotated
            device->CreateCommittedResource(&heapProperties,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &resDescRotated,
                m_gbuffersCommittedRotated[gbufferIndex].m_state,
                &clearValue[gbufferIndex],
                IID_GRAPHICS_PPV_ARGS(m_gbuffersCommittedRotated[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersCommittedRotated[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);

            device->CreateRenderTargetView(m_gbuffersCommittedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersCommittedRotated[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersCommittedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersCommittedRotated[gbufferIndex].m_srvCpuHandle);
        }

        DWORD flAllocation = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
        DWORD flProtect = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE;

        // Placed Resources
        {
            auto alignedSize = AlignUp(xgResLayout[gbufferIndex].SizeBytes, xgResLayout[gbufferIndex].BaseAlignmentBytes);
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSize, flAllocation, XMEM_GRAPHICS, flProtect));
            device->CreatePlacedResourceX(gpuAddress, &resDescNonRotated[gbufferIndex], m_gbuffersPlaced[gbufferIndex].m_state, &clearValue[gbufferIndex], IID_GRAPHICS_PPV_ARGS(m_gbuffersPlaced[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersPlaced[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);
            device->CreateRenderTargetView(m_gbuffersPlaced[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersPlaced[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersPlaced[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersPlaced[gbufferIndex].m_srvCpuHandle);

            // Placed Rotated
            auto alignedSizeRotation = AlignUp(xgResLayout[gbufferIndex].SizeBytes, xgResLayout[gbufferIndex].BaseAlignmentBytes);
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddressRotated =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSizeRotation, flAllocation, XMEM_GRAPHICS, flProtect));

            // Rotate the placed resource
            D3D12_GPU_VIRTUAL_ADDRESS rotatedAddress;
            bool retValue = XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddressRotated, &xgResLayout[gbufferIndex], 0, 0, bankRotationIndex, &rotatedAddress);
            if (!retValue)
            {
                throw std::exception("XGComputeBankRotationAddress failed");
            }

            device->CreatePlacedResourceX(rotatedAddress, &resDescRotated, m_gbuffersPlacedRotated[gbufferIndex].m_state, &clearValue[gbufferIndex], IID_GRAPHICS_PPV_ARGS(m_gbuffersPlacedRotated[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersPlacedRotated[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);
            device->CreateRenderTargetView(m_gbuffersPlacedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersPlacedRotated[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersPlacedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersPlacedRotated[gbufferIndex].m_srvCpuHandle);
        }

        // Component Placed Resources
        {
            uint64_t alignedSizeColorSample = 0;
            uint64_t alignedSizeCMask = 0;
            for (uint32_t i = 0; i < xgResLayout[gbufferIndex].Planes; ++i)
            {
                switch (xgResLayout[gbufferIndex].Plane[i].Usage)
                {
                case XG_PLANE_USAGE_DEFAULT:
                    alignedSizeColorSample = AlignUp(xgResLayout[gbufferIndex].Plane[i].SizeBytes, xgResLayout[gbufferIndex].Plane[i].BaseAlignmentBytes);
                    break;
                case XG_PLANE_USAGE_COLOR_MASK:
                    alignedSizeCMask = AlignUp(xgResLayout[gbufferIndex].Plane[i].SizeBytes, xgResLayout[gbufferIndex].Plane[i].BaseAlignmentBytes);
                    break;
                default:
                    break;
                }
            }

            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSizeColorSample, flAllocation, XMEM_GRAPHICS, flProtect));
            auto cmaskGpuAddress =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSizeCMask, flAllocation, XMEM_GRAPHICS, flProtect));

            D3D12XBOX_COMPONENT_PLACED_ADDRESSES componentPlacedAddr = {};
            componentPlacedAddr.NonAARenderTarget.ColorSamples = gpuAddress;
            componentPlacedAddr.NonAARenderTarget.CMask = cmaskGpuAddress;

            device->CreateComponentPlacedResourceX(&componentPlacedAddr, &resDescNonRotated[gbufferIndex], m_gbuffersComponentPlaced[gbufferIndex].m_state, &clearValue[gbufferIndex], IID_GRAPHICS_PPV_ARGS(m_gbuffersComponentPlaced[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersComponentPlaced[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);
            device->CreateRenderTargetView(m_gbuffersComponentPlaced[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersComponentPlaced[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersComponentPlaced[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersComponentPlaced[gbufferIndex].m_srvCpuHandle);

            // Component Placed Rotated
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddressRotated =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSizeColorSample, flAllocation, XMEM_GRAPHICS, flProtect));

            auto cmaskGpuAddressRotation =
                reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, alignedSizeCMask, flAllocation, XMEM_GRAPHICS, flProtect));

            // Rotate the placed resource
            D3D12_GPU_VIRTUAL_ADDRESS rotatedAddressColorSamples = 0;
            bool retValue = XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddressRotated, &(xgResLayout[gbufferIndex]), 0, 0, bankRotationIndex, &rotatedAddressColorSamples);
            if (!retValue)
            {
                throw std::exception("XGComputeBankRotationAddress failed");
            }

            D3D12XBOX_COMPONENT_PLACED_ADDRESSES componentRotatedPlacedAddr = {};
            componentRotatedPlacedAddr.NonAARenderTarget.ColorSamples = rotatedAddressColorSamples;
            componentRotatedPlacedAddr.NonAARenderTarget.CMask = cmaskGpuAddressRotation;

            device->CreateComponentPlacedResourceX(&componentRotatedPlacedAddr, &resDescRotated, m_gbuffersComponentPlacedRotated[gbufferIndex].m_state, &clearValue[gbufferIndex], IID_GRAPHICS_PPV_ARGS(m_gbuffersComponentPlacedRotated[gbufferIndex].m_resource.ReleaseAndGetAddressOf()));
            m_gbuffersComponentPlacedRotated[gbufferIndex].m_resource->SetName(gbufferName[gbufferIndex]);
            device->CreateRenderTargetView(m_gbuffersComponentPlacedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersComponentPlacedRotated[gbufferIndex].m_rtvCpuHandle);
            device->CreateShaderResourceView(m_gbuffersComponentPlacedRotated[gbufferIndex].m_resource.Get(), nullptr, m_gbuffersComponentPlacedRotated[gbufferIndex].m_srvCpuHandle);
        }
    }
    delete[] xgResLayout;
}

// Create resources to display the buffers on screen
void Sample::CreateRectResources(ID3D12Device* device, ResourceUploadBatch& resourceUpload)
{
    static const D3D12_INPUT_ELEMENT_DESC inputElemDesc[2] =
    {
        { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
    };
    struct VertexData
    {
        XMFLOAT4 position;
        XMFLOAT2 texCoord;
    };
    static const VertexData vertexData[3] =
    {
        { { -1.0f, -1.0f, 1.0f, 1.0f },{ 0.f, 1.f } },
        { { 1.0f, -1.0f, 1.0f, 1.0f },{ 1.f, 1.f } },
        { { 1.0f,  1.0f, 1.0f, 1.0f },{ 1.f, 0.f } },
    };

    DX::ThrowIfFailed(
        CreateStaticBuffer(device, resourceUpload, vertexData, std::size(vertexData), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            m_rectVertexBuffer.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_rectVertexBuffer.Get(), L"RECT Vertex Buffer");

    m_rectVBView.BufferLocation = m_rectVertexBuffer->GetGPUVirtualAddress();
    m_rectVBView.SizeInBytes = sizeof(vertexData);
    m_rectVBView.StrideInBytes = sizeof(VertexData);

    // Shaders
    auto vsBlob = DX::ReadData(L"QuadVS.cso");
    auto psBlob = DX::ReadData(L"QuadPS.cso");

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElemDesc, _countof(inputElemDesc) };
    psoDesc.pRootSignature = m_modelEffect.m_effect->GetRootSignature();
    psoDesc.VS = { vsBlob.data(), vsBlob.size() };
    psoDesc.PS = { psBlob.data(), psBlob.size() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.SampleDesc.Count = 1;
    DX::ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_rectPso.ReleaseAndGetAddressOf())));
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* device, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(
        rtState,
        &CommonStates::AlphaBlend);

    auto const viewport = m_deviceResources->GetScreenViewport();
    m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd, &viewport);

    auto cpuDescHandleText = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_modelEffect.m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        m_descrptorIncrementSizeCBVSRVUAV);
    auto gpuDescHandleText = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_modelEffect.m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        m_descrptorIncrementSizeCBVSRVUAV);

    auto outputSize = m_deviceResources->GetOutputSize();
    m_fontText = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1440) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        cpuDescHandleText,
        gpuDescHandleText);

    auto cpuDescHandleController = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_modelEffect.m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        m_descrptorIncrementSizeCBVSRVUAV);
    auto gpuDescHandleController = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_modelEffect.m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        m_descrptorIncrementSizeCBVSRVUAV);

    m_fontController = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1440) ? L"XboxOneController.spritefont" : L"XboxOneControllerSmall.spritefont",
        cpuDescHandleController,
        gpuDescHandleController);
}

#pragma endregion
