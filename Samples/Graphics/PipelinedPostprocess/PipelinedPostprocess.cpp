//--------------------------------------------------------------------------------------
// PipelinedPostprocess.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PipelinedPostprocess.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#pragma warning(disable : 4061)

namespace
{
    // Fly-Camera
    constexpr Vector3 CAMERA_START_POS = Vector3(0.0f, -150.0f, 0.0f);
    constexpr float CAMERA_NEAR = 0.25f;
    constexpr float CAMERA_FAR = 1000.0f;

    // Timers
    static uint64_t g_cpuPerfFreq = 0ULL;

    // Dragons
    constexpr Vector3 dragonPositions[] =
    {
        Vector3(0.0f, -160.0f, 40.0f),
        Vector3(20.0f, -160.0f, 60.0f),
        Vector3(0.0f, -160.0f, 80.0f),
        Vector3(-20.0f, -160.0f, 60.0f),
        Vector3(40.0f, -160.0f, 80.0f),
        Vector3(20.0f, -160.0f, 80.0f),
        Vector3(0.0f, -160.0f, 100.0f),
        Vector3(-20.0f, -160.0f, 80.0f),
        Vector3(-40.0f, -160.0f, 80.0f)
    };
}

Sample::Sample() noexcept(false) :
    m_isFirstPipelineCall(true),
    m_usePipelinedPostProcess(true),
    m_transition(false),
    m_useFakeGPULoad(true),
    m_frame(0ULL),
    m_cpuTimer(std::make_unique<DX::CPUTimer>()),
    m_frameDeltaTime(),
    m_GPUGeometryFrameTime(),
    m_GPUPostprocessFrameTime(),
    m_fenceCmpToGfx(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
    m_prepassCBVA(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
    m_prepassCBMappedMemory(nullptr),
    m_fxaaCBVA(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
    m_fxaaCBMappedMemory(nullptr),
    m_fakeGPULoadGeometryUS(9000.0f),
#ifdef _GAMING_XBOX_SCARLETT
    m_fakeGPULoadTimePostProcessUS(7000.0f),
#else
    m_fakeGPULoadTimePostProcessUS(5000.0f),
#endif
    m_originToPresentTimeMS()
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT, BACKBUFFER_COUNT, DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);

    m_deviceType = XSystemGetDeviceType();

    m_cpuTimer->Start();

    // Get the query performance frequency
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
    {
        throw std::exception("QueryPerformanceFrequency");
    }
    g_cpuPerfFreq = static_cast<UINT64>(freq.QuadPart);
}

Sample::~Sample()
{
    auto *device = m_deviceResources->GetD3DDevice();
#ifdef _GAMING_XBOX_SCARLETT
    device->SetDebugErrorFilterX(0x6BCA2E89, D3D12XBOX_DEBUG_FILTER_FLAG_NONE);
#else
    device->SetDebugErrorFilterX(0xDA62126E, D3D12XBOX_DEBUG_FILTER_FLAG_NONE);
#endif

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
        m_deviceResources->WaitForGpuCompute();
    }

    if (m_prepassCBMappedMemory)
    {
        m_prepassCBRes->Unmap(0, nullptr);
        m_prepassCBMappedMemory = nullptr;
    }

    if (m_fxaaCBMappedMemory)
    {
        m_fxaaCBRes->Unmap(0, nullptr);
        m_fxaaCBMappedMemory = nullptr;
    }

    auto success = VirtualFree(m_fenceAddress, 0ULL, MEM_RELEASE);
    if (!success)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
    m_fenceAddress = nullptr;
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

    // Model (world) matrix for main scenario
    m_modelMatrix = Matrix::Identity;

    // Model (world) matrix for dragon
    m_dragonModelMatrix = Matrix::Identity;
    m_dragonModelMatrix._11 = 0.2f;
    m_dragonModelMatrix._22 = 0.2f;
    m_dragonModelMatrix._33 = 0.2f;
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    uint32_t currentFrameIndex = m_deviceResources->GetCurrentFrameIndex();
    PIXBeginEvent(PixColorPerFrame[currentFrameIndex], L"Frame %llu", m_frame);

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
    std::ignore = elapsedTime;

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_transition = true;
            m_usePipelinedPostProcess = !m_usePipelinedPostProcess;
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

    m_cpuTimer->Stop();
    double frameTime = m_cpuTimer->GetElapsedMS();
    m_frameDeltaTime.AddValue(frameTime);
    m_cpuTimer->Start();

    auto computeCmdList = m_deviceResources->GetComputeCommandList();
    auto commandList = m_deviceResources->GetCommandList();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare(m_usePipelinedPostProcess || m_transition);
    Clear();

    uint32_t currentFrameIndex = m_deviceResources->GetCurrentFrameIndex();

    // Transition from and to pipelined post process
    if (m_transition && !m_usePipelinedPostProcess)
    {
        m_deviceResources->RegisterFrameEvents(m_deviceResources->GetBackBufferCount() - 1, 0U);
        m_deviceResources->GetComputeCommandList()->Close();
        m_transition = false;
    }
    else if (m_transition)
    {
        m_deviceResources->RegisterFrameEvents(m_deviceResources->GetBackBufferCount() - 1, ASYNC_INTERVAL_OFFSET);
        m_transition = false;
    }

    m_gpuTimer->BeginFrame(commandList);
    if (m_usePipelinedPostProcess)
    {
        m_gpuComputeTimer->BeginFrame(computeCmdList);
    }

    m_gpuTimer->Start(commandList, GPU_TIMER_GEOMETRY_PASSES);

    //////////////////////////
    ///   Depth Pre-Pass   ///
    //////////////////////////
    PIXBeginEvent(commandList, PixColorPerFrame[currentFrameIndex], L"Graphics - DepthPrePass (Frame N)");

    commandList->ClearDepthStencilView(m_postprocessDSVCpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(0, nullptr, 1, &m_postprocessDSVCpuHandle);
    commandList->SetPipelineState(m_depthPrePassPSO.Get());
    commandList->SetGraphicsRootSignature(m_defaultRS.Get());

    SceneConstants constants;
    size_t dragonCount = std::size(dragonPositions);
    for (size_t i = 0; i < dragonCount; ++i)
    {
        m_dragonModelMatrix.Translation(dragonPositions[i]);
        Matrix dragonMvp = m_dragonModelMatrix * m_sceneCamera.GetView() * m_sceneCamera.GetProjection();
        constants.m_mvp = dragonMvp.Transpose();
        size_t cbIndex = i + currentFrameIndex * (dragonCount + 1);
        memcpy(&m_prepassCBMappedMemory[cbIndex], &constants, sizeof(constants));
        commandList->SetGraphicsRootConstantBufferView(0, m_prepassCBVA + cbIndex * sizeof(SceneConstants));
        m_dragonModel->Draw(commandList);
    }

    Matrix mvp = m_modelMatrix * m_sceneCamera.GetView() * m_sceneCamera.GetProjection();
    constants.m_mvp = mvp.Transpose();
    size_t cbIndex = dragonCount + currentFrameIndex * (dragonCount + 1);
    memcpy(&m_prepassCBMappedMemory[cbIndex], &constants, sizeof(constants));
    commandList->SetGraphicsRootConstantBufferView(0, m_prepassCBVA + cbIndex * sizeof(SceneConstants));
    m_model->Draw(commandList);

    PIXEndEvent(commandList);

    // Simulate fake gpu load
    if (m_useFakeGPULoad)
    {
        GetGpuStartTime(commandList);
        SimulateFakeGPUPass(commandList, L"Geometry - Fake GPU load", m_fakeGPULoadGeometryUS);
    }

    // GFX Waits before starting color pass for Compute to finish the postProcess of previous frame
    // Compute wont execute on first frame (GetFrameCount() == 1), so we skip it.
    if (m_usePipelinedPostProcess && m_isFirstPipelineCall == false)
    {
        commandList->Wait64BitValueX(m_fenceCmpToGfx, D3D12_COMPARISON_FUNC_EQUAL, m_fenceCmpToGfxValue - 1, D3D12XBOX_WAIT_FLAG_NONE);
    }

    //////////////////////
    ///   Color Pass   ///
    //////////////////////
    PIXBeginEvent(commandList, PixColorPerFrame[currentFrameIndex], L"Graphics - ColorPass (Frame N)");

    // Transition resource to RT
#ifdef _GAMING_XBOX_SCARLETT
    D3D12_TEXTURE_BARRIER barriers[1];
    barriers[0] = CD3DX12_TEXTURE_BARRIER(
        D3D12_BARRIER_SYNC_NON_PIXEL_SHADING, D3D12_BARRIER_SYNC_RENDER_TARGET,
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_ACCESS_RENDER_TARGET,
        D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_RENDER_TARGET,
        m_postProcessRT.Get(),
        CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff));
    D3D12_BARRIER_GROUP TexBarrierGroups[] = { CD3DX12_BARRIER_GROUP(static_cast<uint32_t>(std::size(barriers)), barriers) };
    commandList->Barrier(1, TexBarrierGroups);
#else
    D3D12_RESOURCE_BARRIER barriers[1] = {};
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_postProcessRT.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
#endif

    // Clear the postprocess targets and then set as RT
    commandList->ClearRenderTargetView(m_postprocessRTVCpuHandle, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->OMSetRenderTargets(1, &m_postprocessRTVCpuHandle, 1, &m_postprocessDSVCpuHandle);

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_texFactory->Heap() };
    commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(descriptorHeaps)), descriptorHeaps);

    Model::UpdateEffectMatrices(m_modelEffect, m_modelMatrix, m_sceneCamera.GetView(), m_sceneCamera.GetProjection());
    m_model->Draw(commandList, m_modelEffect.begin());

    for (size_t i = 0; i < std::size(dragonPositions); ++i)
    {
        m_dragonModelMatrix.Translation(dragonPositions[i]);
        Model::UpdateEffectMatrices(m_dragonModelEffect, m_dragonModelMatrix, m_sceneCamera.GetView(), m_sceneCamera.GetProjection());
        m_dragonModel->Draw(commandList, m_dragonModelEffect.begin());
    }

    PIXEndEvent(commandList);

    ///////////////////
    ///   UI Pass   ///
    ///////////////////
    PIXBeginEvent(commandList, PixColorPerFrame[currentFrameIndex], L"Graphics - DrawHUD (Frame N)");

    DrawHUD();

    PIXEndEvent(commandList);

    // Rendertarget resource needs to be transitioned to be used as non pixel shader resource
#ifdef _GAMING_XBOX_SCARLETT
    barriers[0] = CD3DX12_TEXTURE_BARRIER(
        D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NON_PIXEL_SHADING,
        D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
        D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
        m_postProcessRT.Get(),
        CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff));
    TexBarrierGroups[0] = { CD3DX12_BARRIER_GROUP(static_cast<uint32_t>(std::size(barriers)), barriers) };
    commandList->Barrier(1, TexBarrierGroups);
#else
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_postProcessRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
#endif

    // End frame for gpu timer
    m_gpuTimer->Stop(commandList, GPU_TIMER_GEOMETRY_PASSES);
    double GPUGeometryFrameTime = m_gpuTimer->GetAverageMS(GPU_TIMER_GEOMETRY_PASSES);
    m_GPUGeometryFrameTime.AddValue(GPUGeometryFrameTime);

    // This is the final bit of work the GFX queue will do in a frame when pipelined post process is on
    if (m_usePipelinedPostProcess)
    {
        // End frame for GFX-queue gpu timer
        m_gpuTimer->EndFrame(commandList);
        m_deviceResources->ExecuteGraphicsQueue();
    }

    //////////////////////////
    ///   PostProcess AA   ///
    //////////////////////////
    if (m_usePipelinedPostProcess)
    {
        PIXBeginEvent(computeCmdList, PixColorPerFrame[currentFrameIndex], L"COMPUTE - PostProcess AA (Frame N-1)");

        m_gpuComputeTimer->Start(computeCmdList, GPU_TIMER_POST_PROCESS_PASSES);

        PostProcessDispatch(computeCmdList, currentFrameIndex);

        PIXEndEvent(computeCmdList);

        // Simulate fake gpu load
        if (m_useFakeGPULoad)
        {
            GetGpuStartTime(computeCmdList);
            SimulateFakeGPUPass(computeCmdList, L"Postprocess - Fake GPU load", m_fakeGPULoadTimePostProcessUS);
        }

        m_gpuComputeTimer->Stop(computeCmdList, GPU_TIMER_POST_PROCESS_PASSES);

        double GPUPostprocessFrameTime = m_gpuComputeTimer->GetAverageMS(GPU_TIMER_POST_PROCESS_PASSES);
        m_GPUPostprocessFrameTime.AddValue(GPUPostprocessFrameTime);

        // Signal GFX so it can proceed with the color pass (which has dependencies with the postprocess pass)
        uint64_t const fenceValueToWrite = m_fenceCmpToGfxValue;
        computeCmdList->Write64BitValueBottomOfPipeX(m_fenceCmpToGfx, fenceValueToWrite, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
        m_fenceCmpToGfxValue++;

        m_gpuComputeTimer->EndFrame(computeCmdList);

        // Present on compute queue
        PIXBeginEvent(PixColorPerFrame[currentFrameIndex], L"Compute - Present (Frame N-1)");

        m_deviceResources->ExecuteComputeQueue();
        m_graphicsMemory->Commit(m_deviceResources->GetComputeCommandQueue());
        m_deviceResources->PresentFromCompute();

        PIXEndEvent();
    }
    else
    {
        PIXBeginEvent(commandList, PixColorPerFrame[currentFrameIndex], L"Graphics - PostProcess AA (Frame N)");

        m_gpuTimer->Start(commandList, GPU_TIMER_POST_PROCESS_PASSES);
        PostProcessDispatch(commandList, currentFrameIndex);

        PIXEndEvent(commandList);

        // Simulate fake gpu load
        if (m_useFakeGPULoad)
        {
            GetGpuStartTime(commandList);
            SimulateFakeGPUPass(commandList, L"Postprocess - Fake GPU load", m_fakeGPULoadTimePostProcessUS);
        }

        m_gpuTimer->Stop(commandList, GPU_TIMER_POST_PROCESS_PASSES);

        // End frame for GFX-queue gpu timer
        double GPUPostprocessFrameTime = m_gpuTimer->GetAverageMS(GPU_TIMER_POST_PROCESS_PASSES);
        m_GPUPostprocessFrameTime.AddValue(GPUPostprocessFrameTime);

        m_gpuTimer->EndFrame(commandList);

        // Present on graphics queue
        PIXBeginEvent(PixColorPerFrame[currentFrameIndex], L"Graphics - Present (Frame N)");
        m_deviceResources->Present();
        m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
        PIXEndEvent();
    }

    FrameStatistics stats = {};
    D3D12XBOX_FRAME_PIPELINE_TOKEN oldestSavedFrameToken = m_deviceResources->GetOldestFrameToken();
    if (GetFrameStatistics(oldestSavedFrameToken, stats))
    {
        double originToPresentTimeMS = 1000.0 * ((stats.present.GPUProcessTime - stats.origin.SignalTime) / (double)g_cpuPerfFreq);
        m_originToPresentTimeMS.AddValue(originToPresentTimeMS);
    }

    m_isFirstPipelineCall = false;
}

#ifdef _GAMING_XBOX_SCARLETT
void Sample::PostProcessDispatch(ID3D12GraphicsCommandList7* cmdList, uint32_t backbufferUAVIndex)
#else
void Sample::PostProcessDispatch(ID3D12GraphicsCommandList* cmdList, uint32_t backbufferUAVIndex)
#endif
{
    // Barrier to transition backbuffer from present into UAV
#ifdef _GAMING_XBOX_SCARLETT
    D3D12_TEXTURE_BARRIER barriers[1];
    barriers[0] = CD3DX12_TEXTURE_BARRIER(
        D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        D3D12_BARRIER_ACCESS_COMMON, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        m_deviceResources->GetRenderTarget(),
        CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff));
    D3D12_BARRIER_GROUP TexBarrierGroups[] = { CD3DX12_BARRIER_GROUP(static_cast<uint32_t>(std::size(barriers)), barriers) };
    cmdList->Barrier(1, TexBarrierGroups);
#else
    D3D12_RESOURCE_BARRIER barriers[1];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
#endif

    // Bind descriptor heap for the SRV-UAV table
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
    cmdList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

    cmdList->SetComputeRootSignature(m_computeRS.Get());

    FXAAConstantBuffer fxaaCB;
    fxaaCB.width = static_cast<float>(m_displayWidth);
    fxaaCB.height = static_cast<float>(m_displayHeight);
    fxaaCB.fxaaPixelSize = Vector2( 1.0f / m_displayWidth, 1.0f / m_displayHeight );
    size_t cbIndex = m_deviceResources->GetCurrentFrameIndex();
    memcpy(&m_fxaaCBMappedMemory[cbIndex], &fxaaCB, sizeof(fxaaCB));
    cmdList->SetComputeRootConstantBufferView(0, m_fxaaCBVA + cbIndex * sizeof(FXAAConstantBuffer));

    cmdList->SetComputeRootDescriptorTable(1, m_srvPile->GetGpuHandle(SRV_HANDLE_FXAA_IN_TEXTURE));
    uint32_t fxaaUavGpuHandle = UAV_HANDLE_FXAA_OUT_TEXTURE_RT0 + backbufferUAVIndex;
    cmdList->SetComputeRootDescriptorTable(2, m_srvPile->GetGpuHandle(fxaaUavGpuHandle));

    cmdList->SetPipelineState(m_FXAAPassPSO.Get());
    cmdList->Dispatch(m_displayWidth / 8, m_displayHeight / 8, 1);

    // Barrier to transition backbuffer from UAV into present
#ifdef _GAMING_XBOX_SCARLETT
    barriers[0] = CD3DX12_TEXTURE_BARRIER(
        D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_NONE,
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_ACCESS_COMMON,
        D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_PRESENT,
        m_deviceResources->GetRenderTarget(),
        CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff));
    TexBarrierGroups[0] = { CD3DX12_BARRIER_GROUP(static_cast<uint32_t>(std::size(barriers)), barriers) };
    cmdList->Barrier(1, TexBarrierGroups);
#else
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
#endif
}

void Sample::DrawHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    m_hudBatch->Begin(commandList);

    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textTimerColor = DirectX::Colors::AntiqueWhite;
    const XMVECTOR gpuTimerColor = DirectX::Colors::Orange;
    const XMVECTOR titleColor = DirectX::Colors::AntiqueWhite;
    const XMVECTOR textColor = DirectX::Colors::Aquamarine;

    wchar_t buffer[100];
    {
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"Pipelined Post-Process Sample", textPos, titleColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Current state section
        swprintf_s(buffer, std::size(buffer), L"Using pipelinedPostProcess: %s", m_usePipelinedPostProcess ? L"true" : L"false");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (8.0f * m_smallFont->GetLineSpacing());

        // Timer section
        swprintf_s(buffer, std::size(buffer), L"Frame Time: %.2f ms", m_frameDeltaTime.GetWindowsAvg());
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Origin to present Time: %.2f ms", m_originToPresentTimeMS.GetWindowsAvg());
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"GPU geometry passes frame time: %.2f ms", m_GPUGeometryFrameTime.GetWindowsAvg());
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, gpuTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"GPU postprocessing passes frame time: %.2f ms", m_GPUPostprocessFrameTime.GetWindowsAvg());
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, gpuTimerColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Instructions
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[A] To switch Technique", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();
    }

    m_hudBatch->End();
}

/// For this sample, we want to show how to schedule the postprocess work to minimize latency.
/// The gains from this technique are easier to show when the GPU interval (in our case 16.67 ms)
/// is being used fully. Given that this sample's postprocess and geometry work is not enough to fill
/// the interval, SimulateFakeGPUPass is used to spin the GPU with fake work in order to emulate the
/// desired behaviour.
#ifdef _GAMING_XBOX_SCARLETT
void Sample::SimulateFakeGPUPass(ID3D12GraphicsCommandList7* cmdList, wchar_t const* name, float desiredGpuFrameTimeInUs)
#else
void Sample::SimulateFakeGPUPass(ID3D12GraphicsCommandList* cmdList, wchar_t const *name, float desiredGpuFrameTimeInUs)
#endif
{
    uint64_t clockRate = 0;
    switch (m_deviceType)
    {
    case XSystemDeviceType::XboxOne:
        clockRate = 853;
        break;
    case XSystemDeviceType::XboxOneS:
        clockRate = 914;
        break;
    case XSystemDeviceType::XboxOneX:
    case XSystemDeviceType::XboxOneXDevkit:
        clockRate = 1172;
        break;
    case XSystemDeviceType::XboxScarlettLockhart:
        clockRate = 1565;
        break;
    case XSystemDeviceType::XboxScarlettAnaconda:
    case XSystemDeviceType::XboxScarlettDevkit:
        clockRate = 1825;
        break;
    case XSystemDeviceType::Pc:
    case XSystemDeviceType::Unknown:
    default:
        assert(false);
    }

    uint32_t currentFrameIndex = m_deviceResources->GetCurrentFrameIndex();
    auto desiredFrameTimeInTicks = static_cast<uint64_t>(desiredGpuFrameTimeInUs) * clockRate;

    PIXBeginEvent(cmdList, PixColorPerFrame[currentFrameIndex], name);

    cmdList->SetComputeRootSignature(m_gpuEnforceTimeRS.Get());
    cmdList->SetComputeRoot32BitConstants(0, 2, &desiredFrameTimeInTicks, 0);
    cmdList->SetComputeRootUnorderedAccessView(1, m_gpuTimeResource->GetGPUVirtualAddress());
    cmdList->SetPipelineState(m_gpuEnforceTimePSO.Get());

    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuTimeResource.Get());
    cmdList->ResourceBarrier(1, &barrier);
    cmdList->Dispatch(1, 1, 1);
    cmdList->ResourceBarrier(1, &barrier);

    PIXEndEvent(cmdList);
}

#ifdef _GAMING_XBOX_SCARLETT
void Sample::GetGpuStartTime(ID3D12GraphicsCommandList7* cmdList)
#else
void Sample::GetGpuStartTime(ID3D12GraphicsCommandList* cmdList)
#endif
{
    uint32_t currentFrameIndex = m_deviceResources->GetCurrentFrameIndex();
    PIXBeginEvent(cmdList, PixColorPerFrame[currentFrameIndex], L"Get GPU start time");

    cmdList->SetComputeRootSignature(m_getStartTimeRS.Get());
    cmdList->SetComputeRootUnorderedAccessView(0, m_gpuTimeResource->GetGPUVirtualAddress());
    cmdList->SetPipelineState(m_getStartTimePSO.Get());

    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuTimeResource.Get());
    cmdList->ResourceBarrier(1, &barrier);
    cmdList->Dispatch(1, 1, 1);
    cmdList->ResourceBarrier(1, &barrier);

    PIXEndEvent(cmdList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    uint32_t currentFrameIndex = m_deviceResources->GetCurrentFrameIndex();
    PIXBeginEvent(commandList, PixColorPerFrame[currentFrameIndex], L"SetViewport");

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
}

void Sample::OnResuming()
{
    m_deviceResources->Resume(m_usePipelinedPostProcess);
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
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Filter warning: Barrier validation may raise false positives or hang when custom synchronization via Wait32BitValueX or Wait64BitValueX is in use.
    D3D12XBOX_DEBUG_FILTER_FLAGS dflags = D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_FAILURE | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT;
#ifdef _GAMING_XBOX_SCARLETT
    device->SetDebugErrorFilterX(0x6BCA2E89, dflags);
    // [0x81F8A4C7] Temporary, to prevent validation error with EB due to missing support for layout present on direct queues.
    device->SetDebugErrorFilterX(0x81F8A4C7, dflags | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);
#else
    device->SetDebugErrorFilterX(0xDA62126E, dflags);
#endif

    // Create resources for the Constant Buffers
    {
        // Constant buffer used by depth prepass
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t NUM_CALLS = std::size(dragonPositions) + 1ULL;
        size_t bufferSize = m_deviceResources->GetBackBufferCount() * NUM_CALLS * sizeof(SceneConstants);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_prepassCBRes.ReleaseAndGetAddressOf())));
        m_prepassCBRes->SetName(L"prepassCBResource");
        m_prepassCBVA = m_prepassCBRes->GetGPUVirtualAddress();
        DX::ThrowIfFailed(
            m_prepassCBRes->Map(0, nullptr, reinterpret_cast<void**>(&m_prepassCBMappedMemory)));

        // Constant buffer used by FXAA
        NUM_CALLS = 1;
        bufferSize = m_deviceResources->GetBackBufferCount() * NUM_CALLS * sizeof(FXAAConstantBuffer);
        resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_fxaaCBRes.ReleaseAndGetAddressOf())));
        m_fxaaCBRes->SetName(L"fxaaCBResource");
        m_fxaaCBVA = m_fxaaCBRes->GetGPUVirtualAddress();
        DX::ThrowIfFailed(
            m_fxaaCBRes->Map(0, nullptr, reinterpret_cast<void**>(&m_fxaaCBMappedMemory)));
    }

    // Load Model, Textures and HUD batch
    {
        m_srvPile = std::make_unique<DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 128, 0);

        auto modelBlob = DX::ReadData(L"AbstractCathedral.sdkmesh");
        m_model = Model::CreateFromSDKMESH(device, modelBlob.data(), modelBlob.size());

        auto dragonModelBlob = DX::ReadData(L"dragon_LOD0.sdkmesh");
        m_dragonModel = Model::CreateFromSDKMESH(device, dragonModelBlob.data(), dragonModelBlob.size());

        // Create the gpu timers
        m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
        m_gpuComputeTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetComputeCommandQueue());

        // Upload resources to video memory
        ResourceUploadBatch upload(device);
        upload.Begin();

        // Upload fonts for the HUD
        {
            auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
            auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
            m_hudBatch = std::make_unique<SpriteBatch>(device, upload, spritePSD);

            size_t start, end;
            uint32_t fontCount = SRV_HANDLE_CONTROLLER_FONT - SRV_HANDLE_HUD_FONT + 1;
            m_srvPile->AllocateRange(fontCount, start, end);
            assert(start == SRV_HANDLE_HUD_FONT && L"SRVPile miss-alignment on resource.");

            wchar_t strFilePath[MAX_PATH] = {};
            DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
            m_smallFont = std::make_unique<SpriteFont>(device, upload,
                strFilePath,
                m_srvPile->GetCpuHandle(SRV_HANDLE_HUD_FONT),
                m_srvPile->GetGpuHandle(SRV_HANDLE_HUD_FONT));

            DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
            m_controllerFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
                m_srvPile->GetCpuHandle(SRV_HANDLE_CONTROLLER_FONT),
                m_srvPile->GetGpuHandle(SRV_HANDLE_CONTROLLER_FONT));
        }

        m_model->LoadStaticBuffers(device, upload);
        m_dragonModel->LoadStaticBuffers(device, upload);

        m_states = std::make_unique<CommonStates>(device);
        m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, m_model->textureNames.size());

        m_texFactory->EnableForceSRGB(true);
        m_model->LoadTextures(*m_texFactory);

        m_fxFactory = std::make_unique<EffectFactory>(m_texFactory->Heap(), m_states->Heap());

        // Allocate space for the FXAA SRV and UAV
        {
            size_t start, end;
            uint32_t fxaaBindingCount = 1 + m_deviceResources->GetBackBufferCount();
            m_srvPile->AllocateRange(fxaaBindingCount, start, end);
            assert(start == SRV_HANDLE_FXAA_IN_TEXTURE && L"SRVPile miss-alignment on resource.");
        }

        // Effects
        {
            D3D12_DEPTH_STENCIL_DESC depthEqualNoWrite = {};
            depthEqualNoWrite.DepthEnable = true;
            depthEqualNoWrite.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
            depthEqualNoWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

            EffectPipelineStateDescription pdOpaque(
                nullptr,
                CommonStates::Opaque,
                depthEqualNoWrite,
                CommonStates::CullCounterClockwise,
                rtState);

            EffectPipelineStateDescription pdAlpha(
                nullptr,
                CommonStates::AlphaBlend,
                depthEqualNoWrite,
                CommonStates::CullCounterClockwise,
                rtState);

            m_modelEffect = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);
            m_dragonModelEffect = m_dragonModel->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);
        }

        auto finish = upload.End(m_deviceResources->GetCommandQueue());
        finish.wait();
    }

    // Manual fences
    {
        // Allocate GPU memory for manual fences
        using Fence = uint64_t;
        m_fenceAddress = XMemVirtualAlloc(
            nullptr,
            sizeof(uint64_t),
            MEM_RESERVE | MEM_COMMIT | MEM_64K_PAGES,
            XMEM_GRAPHICS,
            PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE);
        if (nullptr == m_fenceAddress)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        ZeroMemory(m_fenceAddress, sizeof(uint64_t));

        auto fenceAddress = reinterpret_cast<Fence*>(m_fenceAddress);
        m_fenceCmpToGfx = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&fenceAddress[0]);
        m_fenceCmpToGfxValue = 1ULL;
    }

    // Resource used by EnforceGpuTime passes
    {
        const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const D3D12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(uint64_t), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_gpuTimeResource.ReleaseAndGetAddressOf())));
        m_gpuTimeResource->SetName(L"EnforceGPUTimeResource");
    }

    CreatePipelineStateObjects();
}

void Sample::CreatePipelineStateObjects()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto vsBlob = DX::ReadData(L"VertexShader.cso");

    // Depth Pre-Pass PSO
    {
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_defaultRS.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = VertexPositionNormalTexture::InputLayout;
        psoDesc.pRootSignature = m_defaultRS.Get();
        psoDesc.VS = { vsBlob.data(), vsBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 0;
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_depthPrePassPSO.ReleaseAndGetAddressOf())));
        m_depthPrePassPSO->SetName(L"DepthPrePassPSO");
    }

    // Compute resolve pass
    {
        auto csBlob = DX::ReadData(L"FXAA.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_computeRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_computeRS.Get();
        psoDesc.CS = { csBlob.data(), csBlob.size() };
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        psoDesc.NodeMask = 0;

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_FXAAPassPSO.ReleaseAndGetAddressOf())));
        m_FXAAPassPSO->SetName(L"FXAAPassPSO");
    }

    // Pipeline for shader that spins to fake gpu load
    {
        auto computeShaderBlob = DX::ReadData(L"EnforceGpuTime.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_gpuEnforceTimeRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_gpuEnforceTimeRS.Get();
        psoDesc.CS.pShaderBytecode = computeShaderBlob.data();
        psoDesc.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_gpuEnforceTimePSO.ReleaseAndGetAddressOf())));
        m_gpuEnforceTimePSO->SetName(L"GpuEnforceTimePSO");
    }

    // Pipeline for getting frame start time (used by enforceGpuTime)
    {
        // Shader to read frame start time
        auto computeShaderBlob = DX::ReadData(L"GetStartTime.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_getStartTimeRS.ReleaseAndGetAddressOf())));
        m_getStartTimeRS->SetName(L"GetGPUTimeRS");

        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_getStartTimeRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_getStartTimePSO.ReleaseAndGetAddressOf())));
        m_getStartTimePSO->SetName(L"GetGPUTimePSO");
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint64_t>(size.right - size.left);
    m_displayHeight = static_cast<uint64_t>(size.bottom - size.top);

    /// Fly camera
    m_sceneCamera.SetWindow(static_cast<int32_t>(m_displayWidth), static_cast<int32_t>(m_displayHeight));
    m_sceneCamera.SetProjectionParameters(XM_PIDIV4, CAMERA_NEAR, CAMERA_FAR, true);
    m_sceneCamera.SetSensitivity(0.0f, 0.0f, 0.0f, 0.0f);
    m_sceneCamera.SetFlags(m_sceneCamera.c_FlagsDisableRotateX & m_sceneCamera.c_FlagsDisableRotateY);
    m_sceneCamera.SetPosition(CAMERA_START_POS);

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920.0f;
    viewportUI.Height = 1080.0f;
    m_hudBatch->SetViewport(viewportUI);

    auto *device = m_deviceResources->GetD3DDevice();

    // PostProcess Resource
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

#ifdef _GAMING_XBOX_SCARLETT
        D3D12_RESOURCE_DESC1 resDesc = {};
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
#else
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
#endif
        resDesc.Format = m_deviceResources->GetBackBufferFormat();
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Width = m_displayWidth;
        resDesc.Height = m_displayHeight;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.Alignment = 0;
        resDesc.SampleDesc.Count = 1;

#ifdef _GAMING_XBOX_SCARLETT
        DX::ThrowIfFailed(
            device->CreateCommittedResource3(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
                nullptr,
                nullptr,
                0,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_postProcessRT.ReleaseAndGetAddressOf())));
#else
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_postProcessRT.ReleaseAndGetAddressOf())));
#endif
        m_postProcessRT->SetName(L"PostProcessRTResource");

        D3D12_DESCRIPTOR_HEAP_DESC rtHeapDesc = {};
        rtHeapDesc.NumDescriptors = 1;
        rtHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&rtHeapDesc, IID_GRAPHICS_PPV_ARGS(m_RTHeap.ReleaseAndGetAddressOf())));
        m_postprocessRTVCpuHandle = m_RTHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_RENDER_TARGET_VIEW_DESC postprocessRTVDesc = {};
        postprocessRTVDesc.Format = m_deviceResources->GetBackBufferFormat();
        postprocessRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(m_postProcessRT.Get(), &postprocessRTVDesc, m_postprocessRTVCpuHandle);
    }

    // PostProcess Depth Stencil Resource and View
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Format = m_deviceResources->GetDepthBufferFormat();
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Width = m_displayWidth;
        resDesc.Height = m_displayHeight;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resDesc.Alignment = 0;
        resDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_postProcessDS.ReleaseAndGetAddressOf())));
        m_postProcessDS->SetName(L"PostProcessDSResource");

        D3D12_DESCRIPTOR_HEAP_DESC dsHeapDesc = {};
        dsHeapDesc.NumDescriptors = 1;
        dsHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&dsHeapDesc, IID_GRAPHICS_PPV_ARGS(m_DSHeap.ReleaseAndGetAddressOf())));
        m_postprocessDSVCpuHandle = m_DSHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_DEPTH_STENCIL_VIEW_DESC postprocessDSVDesc = {};
        postprocessDSVDesc.Format = m_deviceResources->GetDepthBufferFormat();
        postprocessDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(m_postProcessDS.Get(), &postprocessDSVDesc, m_postprocessDSVCpuHandle);
    }

    // Create views for FXAA (needs to be here since we need the renderTargets to be created)
    {
        // Create the SRV and UAV for FXAA
        // SRV is gonna be for the intermediate render target
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = m_deviceResources->GetBackBufferFormat();
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_postProcessRT.Get(), &srvDesc, m_srvPile->GetCpuHandle(SRV_HANDLE_FXAA_IN_TEXTURE));

        // UAV is gonna be for the m_device rendertarget (which we will write into)
        for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = m_deviceResources->GetBackBufferFormat();
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uint32_t handleIndex = UAV_HANDLE_FXAA_OUT_TEXTURE_RT0 + i;
            device->CreateUnorderedAccessView(m_deviceResources->GetRenderTarget(i), nullptr, &uavDesc, m_srvPile->GetCpuHandle(handleIndex));
        }
    }
}
#pragma endregion

bool Sample::GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FrameStatistics& stats)
{
    if (frameToken == D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL)
    {
        return false;
    }

    auto device = m_deviceResources->GetD3DDevice();
    constexpr uint32_t expectedStatCount = 3;
    D3D12XBOX_FRAME_STATISTICS rawStats[expectedStatCount];
    D3D12XBOX_FRAME_STATISTICS_TYPE typeSet =
        D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT |
        D3D12XBOX_FRAME_STATISTICS_TYPE_RENDER |
        D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT;
    assert(__popcnt(typeSet) == expectedStatCount);

    uint32_t statCount = expectedStatCount;
    HRESULT hr = device->GetFrameStatisticsX(
        frameToken,
        typeSet,
        &statCount,
        rawStats);

    if (hr == S_FALSE || statCount < expectedStatCount)
    {
        // Results not available yet
        return false;
    }

    DX::ThrowIfFailed(hr);

    for (auto& rawStat : rawStats)
    {
        switch (rawStat.Type)
        {
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT:
            stats.origin = rawStat.Event;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_RENDER:
            if (rawStat.Render.PlaneIndex == 0)
                stats.render = rawStat.Render;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT:
            if (rawStat.Present.PlaneIndex == 0)
                stats.present = rawStat.Present;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_INTERVAL:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_DISPLAY:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_INPUT:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_NONE:
 #if defined(_GAMING_XBOX_SCARLETT) && (_GXDK_VER >= 0x633610AF /* GDK Edition 240600 */)
        case D3D12XBOX_FRAME_STATISTICS_TYPE_POWERSCALING:
#endif
        default:
            assert(false);
            break;
        }
    }

    return true;
}
