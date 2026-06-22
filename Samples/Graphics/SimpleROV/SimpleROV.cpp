//--------------------------------------------------------------------------------------
// SimpleROV.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleROV.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "SampleObject.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float RTVClearColor[4] = { 0.4f, 0.4f, 0.4f, 1.0f };

    constexpr Vector3 CAMERA_POSITION = Vector3(0.0f, 60.0f, 0.0f);
    constexpr float CAMERA_NEAR = 1.0f;
    constexpr float CAMERA_FAR = 10000.0f;

    Vector3 ObjectColors[TRANSLUCENT_COUNT] =
    {
        Vector3(1.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(1.0f, 1.0f, 0.0f),
        Vector3(1.0f, 0.0f, 1.0f),
        Vector3(0.0f, 1.0f, 1.0f),
        Vector3(0.3f, 0.7f, 0.9f),
        Vector3(0.8f, 0.7f, 0.8f),
        Vector3(0.5f, 0.7f, 0.1f),
    };
};

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_blendMode(ROP_BLEND),
    m_translucentChoice(MODEL_DRAGONS),
    m_gpuTimerMeasuresMS{0},
    m_MLABSpaceRequired(0),
    m_PPLLSpaceRequired(0),
    m_screenWidth(0),
    m_screenHeight(0),
    m_OpaqueObjs{},
    m_translucentObjs{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    for (uint32_t i = 0; i < OPAQUE_COUNT; ++i)
    {
        delete m_OpaqueObjs[i];
    }

    for (uint32_t i = 0; i < TRANSLUCENT_COUNT; ++i)
    {
        delete m_translucentObjs[MODEL_DRAGONS][i];
        delete m_translucentObjs[MODEL_GARGOYLES][i];
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    auto const size = m_deviceResources->GetOutputSize();
    m_screenWidth = static_cast<uint32_t>(size.right - size.left);
    m_screenHeight = static_cast<uint32_t>(size.bottom - size.top);

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // SampleObjects: initialize positions, rotation and scale.

    m_OpaqueObjs[OPAQUE_CITY]->SetRotation(Vector3(XM_PIDIV2, 0.0f, 0.0f));
    m_OpaqueObjs[OPAQUE_CITY]->SetScale(Vector3(20.0f));

    m_OpaqueObjs[OPAQUE_TERRAIN]->SetPosition(Vector3(0.0f, 40.0f, 0.0f));
    m_OpaqueObjs[OPAQUE_TERRAIN]->SetScale(Vector3(1.0f, 0.125f, 1.0f));

    Vector3 gargoylesObjsCenter = Vector3(-200.0f, 50.0f, -650.0f);
    Vector3 dragonsObjsCenter = Vector3(-200.0f, 30.0f, -600.0f);
    float margin = 200.0f;
    for (uint32_t i = 0, count = 0; i < TRANSLUCENT_DIM_X; ++i)
    {
        for (uint32_t j = 0; j < TRANSLUCENT_DIM_Y; ++j, ++count)
        {
            m_translucentObjs[MODEL_GARGOYLES][count]->SetScale(Vector3(20.0f, 20.0f, 20.0f));
            m_translucentObjs[MODEL_GARGOYLES][count]->SetPosition(gargoylesObjsCenter + Vector3(i * margin, 0.0f, j * margin));
            m_translucentObjs[MODEL_GARGOYLES][count]->Rotate(Vector3(0.0f, XM_PI, 0.0f));
            m_translucentObjs[MODEL_GARGOYLES][count]->SetColor(ObjectColors[count]);

            m_translucentObjs[MODEL_DRAGONS][count]->SetPosition(dragonsObjsCenter + Vector3(i * margin, 0.0f, j * margin));
            m_translucentObjs[MODEL_DRAGONS][count]->Rotate(Vector3(0.0f, XM_PI, 0.0f));
            m_translucentObjs[MODEL_DRAGONS][count]->SetColor(ObjectColors[count]);
        }
    }

    // Create the gpu timer.
    auto device = m_deviceResources->GetD3DDevice();
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_blendMode = static_cast<BlendMode>((m_blendMode + 1) % BLEND_MODE_COUNT);
        }
        else if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            m_blendMode = (m_blendMode > 0) ?
                static_cast<BlendMode>(m_blendMode - 1) :
                static_cast<BlendMode>(BLEND_MODE_COUNT - 1);
        }

        if (m_gamePadButtons.x == ButtonState::RELEASED)
        {
            m_translucentChoice = static_cast<TranslucentChoice>((m_translucentChoice + 1) % MODEL_COUNT);
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
    }

#ifdef _GAMING_DESKTOP
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Enter))
    {
        m_blendMode = static_cast<BlendMode>((m_blendMode + 1) % BLEND_MODE_COUNT);
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Back))
    {
        m_blendMode = (m_blendMode > 0) ?
            static_cast<BlendMode>(m_blendMode - 1) :
            static_cast<BlendMode>(BLEND_MODE_COUNT - 1);
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Space))
    {
        m_translucentChoice = static_cast<TranslucentChoice>((m_translucentChoice + 1) % MODEL_COUNT);
    }
#endif
    
    // Update sample objects.
    for (uint32_t i = 0; i < OPAQUE_COUNT; ++i)
    {
        m_OpaqueObjs[i]->Update();
    }

    for (uint32_t i = 0; i < TRANSLUCENT_COUNT; ++i)
    {
        m_translucentObjs[m_translucentChoice][i]->Update();
    }

    PIXEndEvent();
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

    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();
    std::ignore = frameIndex;

    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer->BeginFrame(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    Matrix const& view = m_camera.GetView();
    Matrix const& proj = m_camera.GetProjection();

    // OPAQUE GEOMETRY RENDERING.

    // Set descriptor heap.
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_ShaderVisibleHeap->Heap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // Desktop will render into an intermediateRT, since swapchain cannot be used as an UAV.
#ifdef _GAMING_DESKTOP
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();
    auto rtvDescriptor = m_RTVDescHeap->GetCpuHandle(RTV_INDEX_INTERMEDIATE_RT);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, RTVClearColor, 0, nullptr);
#endif

    commandList->SetPipelineState(m_opaquePassPSO.Get());

    // Set root signature.
    commandList->SetGraphicsRootSignature(m_opaquePassRS.Get());

    SceneConstants sceneCts = {};
    sceneCts.viewMatrix = view.Transpose();
    sceneCts.projMatrix = proj.Transpose();
    auto cb = m_graphicsMemory->AllocateConstant(sceneCts);
    commandList->SetGraphicsRootConstantBufferView(/*RootIndex*/0, cb.GpuAddress());

    for (uint32_t i = 0; i < OPAQUE_COUNT; ++i)
    {
        ObjectConstants objCts = {};
        objCts.worldMatrix = m_OpaqueObjs[i]->GetWorldMatrix().Transpose();
        cb = m_graphicsMemory->AllocateConstant(objCts);
        commandList->SetGraphicsRootConstantBufferView(/*RootIndex*/1, cb.GpuAddress());

        m_OpaqueObjs[i]->Render(commandList, m_ShaderVisibleHeap);
    }

    PIXEndEvent(commandList);

    // CLEAR UAV(S).

    if (m_blendMode == CUSTOM_BLEND_PPLL || m_blendMode == CUSTOM_BLEND_MLAB)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UAV clears");
        m_gpuTimer->Start(commandList, TIMER_CLEAR_UAV_PASS);

        if (m_blendMode == CUSTOM_BLEND_PPLL)
        {
            uint32_t values1[4] = { PPLL_CLEAR_VALUE, 0u, 0u, 0u };
            commandList->ClearUnorderedAccessViewUint(
                m_ShaderVisibleHeap->GetGpuHandle(UAV_PPLL_HEAD_POINTER_BUFFER),
                m_NonShaderVisibleHeap->GetCpuHandle(UAV_PPLL_HEAD_POINTER_BUFFER),
                m_PPLLHeadPointer.Get(),
                values1,
                0,
                nullptr);

            uint32_t values2[4] = {};
            commandList->ClearUnorderedAccessViewUint(
                m_ShaderVisibleHeap->GetGpuHandle(UAV_PPLL_BUFFER_COUNTER),
                m_NonShaderVisibleHeap->GetCpuHandle(UAV_PPLL_BUFFER_COUNTER),
                m_PPLLBufferCounter.Get(),
                values2,
                0,
                nullptr);
        }
        else if (m_blendMode == CUSTOM_BLEND_MLAB)
        {
            uint32_t values[4] = {};
            commandList->ClearUnorderedAccessViewUint(
                m_ShaderVisibleHeap->GetGpuHandle(UAV_MLAB_CLEAR_MASK),
                m_NonShaderVisibleHeap->GetCpuHandle(UAV_MLAB_CLEAR_MASK),
                m_MLABClearMaskRes.Get(),
                values,
                0, nullptr);
        }

        m_gpuTimer->Stop(commandList, TIMER_CLEAR_UAV_PASS);
        m_gpuTimerMeasuresMS[TIMER_CLEAR_UAV_PASS] = m_gpuTimer->GetAverageMS(TIMER_CLEAR_UAV_PASS);
        PIXEndEvent(commandList);
    }

    // TRANSLUCENT GEOMETRY RENDERING.

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Translucent");
    m_gpuTimer->Start(commandList, TIMER_BLEND_PASS);
    
    if (m_blendMode == ROP_BLEND)
    {
        commandList->SetPipelineState(m_regularBlendPassPSO.Get());
    }
    else
    {
        auto dsv = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(0, nullptr, false, &dsv);

        commandList->SetGraphicsRootSignature(m_CustomBlendRS.Get());

        BlendPassConstants cts = {};
        cts.resolution = Vector2(static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));
        cts.cameraFar = CAMERA_FAR;
        auto gm = m_graphicsMemory->AllocateConstant(cts);
        commandList->SetGraphicsRootConstantBufferView(/*RootIndex*/2, gm.GpuAddress());

        commandList->SetGraphicsRootDescriptorTable(/*RootIndex*/4, m_ShaderVisibleHeap->GetGpuHandle(
            (m_blendMode == CUSTOM_BLEND_PPLL) ? UAV_PPLL_HEAD_POINTER_BUFFER : UAV_MLAB_CLEAR_MASK));

        if (m_blendMode == CUSTOM_BLEND_PPLL)
        {
            commandList->SetGraphicsRootDescriptorTable(/*RootIndex*/5, m_ShaderVisibleHeap->GetGpuHandle(UAV_PPLL_BUFFER_COUNTER));
        }

        commandList->SetPipelineState((m_blendMode == CUSTOM_BLEND_PPLL) ?
            m_PPLLTranslucencyPassPSO.Get() : m_MLABTranslucencyPassPSO.Get());
    }

    if (m_blendMode == ROP_BLEND)
    {
        SortGeometryByViewZ();
    }

    // Render objects in translucent container.
    for (uint32_t i = 0; i < TRANSLUCENT_COUNT; ++i)
    {
        ObjectConstants objCts = {};
        objCts.worldMatrix = m_translucentObjs[m_translucentChoice][i]->GetWorldMatrix().Transpose();
        objCts.diffuseColor = m_translucentObjs[m_translucentChoice][i]->GetColor();
        cb = m_graphicsMemory->AllocateConstant(objCts);
        commandList->SetGraphicsRootConstantBufferView(/*RootIndex*/1, cb.GpuAddress());

        m_translucentObjs[m_translucentChoice][i]->Render(commandList, m_ShaderVisibleHeap);
    }

    m_gpuTimer->Stop(commandList, TIMER_BLEND_PASS);
    m_gpuTimerMeasuresMS[TIMER_BLEND_PASS] = m_gpuTimer->GetAverageMS(TIMER_BLEND_PASS);
    PIXEndEvent(commandList);

    // For PC, we transition its RenderTarget into UAV for the following passes.
#ifdef _GAMING_DESKTOP
    D3D12_RESOURCE_BARRIER barriers[1] = {};
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
#endif

    // BLENDING COMPOSITE PASS.

    if (m_blendMode != ROP_BLEND)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"OIT sort and composite results");
        m_gpuTimer->Start(commandList, TIMER_COMPOSITE_PASS);

        ID3D12Resource* uavRes1 = (m_blendMode == CUSTOM_BLEND_PPLL) ? m_PPLLHeadPointer.Get() : m_MLABClearMaskRes.Get();
        ID3D12Resource* uavRes2 = (m_blendMode == CUSTOM_BLEND_PPLL) ? m_perPixelLinkedListBuffer.Get() : m_MLABNodeListRes.Get();

#ifdef _GAMING_XBOX
        D3D12_RESOURCE_BARRIER CompositeBarriers[4] = {};
#else
        D3D12_RESOURCE_BARRIER CompositeBarriers[3] = {};
#endif
        CompositeBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(uavRes1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        CompositeBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(uavRes2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        CompositeBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_PPLLBufferCounter.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#ifdef _GAMING_XBOX
        CompositeBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(CompositeBarriers)), CompositeBarriers);

        commandList->SetPipelineState((m_blendMode == CUSTOM_BLEND_PPLL) ?
            m_compositePassPPLL_PSO.Get() : m_compositePassMLAB_PSO.Get());

        commandList->SetComputeRootSignature(m_compositePassRS.Get());

        BlendPassConstants cts = {};
        cts.resolution = Vector2(static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));
        cts.cameraFar = CAMERA_FAR;
        auto cbuff = m_graphicsMemory->AllocateConstant(cts);
        commandList->SetComputeRootConstantBufferView(/*RootIndex*/0, cbuff.GpuAddress());

        // SRV table for previously written UAVs
        D3D12_GPU_DESCRIPTOR_HANDLE srvTableHandle = m_ShaderVisibleHeap->GetGpuHandle((m_blendMode == CUSTOM_BLEND_PPLL) ? SRV_PPLL_HEAD_POINTER_BUFFER : SRV_MLAB_CLEAR_MASK);
        commandList->SetComputeRootDescriptorTable(/*RootIndex*/1, srvTableHandle);

#ifdef _GAMING_DESKTOP
        commandList->SetComputeRootDescriptorTable(/*RootIndex*/2, m_ShaderVisibleHeap->GetGpuHandle(UAV_INTERMEDIATE_RT));
#else
        commandList->SetComputeRootDescriptorTable(/*RootIndex*/2, m_ShaderVisibleHeap->GetGpuHandle(UAV_SWAPCHAIN_01 + frameIndex));
#endif

        uint32_t groupCountX = (m_screenWidth + TILE_SIZE_X - 1) / TILE_SIZE_X;
        uint32_t groupCountY = (m_screenHeight + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
        commandList->Dispatch(groupCountX, groupCountY, 1);

        CompositeBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(uavRes1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        CompositeBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(uavRes2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        CompositeBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_PPLLBufferCounter.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#ifdef _GAMING_XBOX
        CompositeBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
#endif
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(CompositeBarriers)), CompositeBarriers);

        m_gpuTimer->Stop(commandList, TIMER_COMPOSITE_PASS);
        m_gpuTimerMeasuresMS[TIMER_COMPOSITE_PASS] = m_gpuTimer->GetAverageMS(TIMER_COMPOSITE_PASS);
        PIXEndEvent(commandList);
    }

    // DESKTOP ONLY - COPY INTERMEDIATE RT TO SWAPCHAIN.

#ifdef _GAMING_DESKTOP
    {
        D3D12_RESOURCE_BARRIER copyBarriers[2] = {};
        copyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRT.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        copyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(copyBarriers)), copyBarriers);

        D3D12_TEXTURE_COPY_LOCATION copyLocationSrc = {};
        copyLocationSrc.pResource = m_intermediateRT.Get();

        D3D12_TEXTURE_COPY_LOCATION copyLocationDst = {};
        copyLocationDst.pResource = m_deviceResources->GetRenderTarget();

        commandList->CopyTextureRegion(&copyLocationDst, 0, 0, 0, &copyLocationSrc, nullptr);

        copyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRT.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        copyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(copyBarriers)), copyBarriers);
    }
#endif

    // HUD RENDERING.

    DrawHUD();

    m_gpuTimer->EndFrame(commandList);

    // PRESENT.

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
    commandList->ClearRenderTargetView(rtvDescriptor, RTVClearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::DrawHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (UI Pass)");
    m_hudBatch->Begin(commandList);

    auto rtv = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { m_HUDDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textColor1 = DirectX::Colors::Black;
    const XMVECTOR textColor2 = DirectX::Colors::Gray;
    const XMVECTOR textColor3 = DirectX::Colors::LightYellow;

    wchar_t buffer[100];
    {
        swprintf_s(buffer, std::size(buffer), L"SimpleROV");
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor3);
        textPos.y += 2.0f * m_smallFontBold->GetLineSpacing();

        // Current state section
        swprintf_s(buffer, std::size(buffer), L"Resolution: (%u, %u)", m_screenWidth, m_screenHeight);
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFontBold->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Scene: %ls", (m_translucentChoice == MODEL_DRAGONS) ? L"Dragons" : L"Gargoyles");
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFontBold->GetLineSpacing();

        uint32_t const blendModeIndex = static_cast<uint32_t>(m_blendMode);
        swprintf_s(buffer, std::size(buffer), L"Blend mode: %ls", s_blendModesNames[blendModeIndex]);
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += 2.0f * m_smallFontBold->GetLineSpacing();

        // Timer section
        if (m_blendMode == CUSTOM_BLEND_PPLL || m_blendMode == CUSTOM_BLEND_MLAB)
        {
            swprintf_s(buffer, std::size(buffer), L"Clear UAV pass: %.2f ms", m_gpuTimerMeasuresMS[TIMER_CLEAR_UAV_PASS]);
            m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
            textPos.y += m_smallFontBold->GetLineSpacing();
        }

        swprintf_s(buffer, std::size(buffer), L"Blend pass (pixel shader): %.2f ms", m_gpuTimerMeasuresMS[TIMER_BLEND_PASS]);
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, (m_blendMode == ROP_BLEND) ? textColor3 : textColor1);
        textPos.y += m_smallFontBold->GetLineSpacing();

        if (m_blendMode == CUSTOM_BLEND_PPLL || m_blendMode == CUSTOM_BLEND_MLAB)
        {
            swprintf_s(buffer, std::size(buffer), L"Composite pass: %.2f ms", m_gpuTimerMeasuresMS[TIMER_COMPOSITE_PASS]);
            m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
            textPos.y += m_smallFontBold->GetLineSpacing();

            float totalTimeSpent = m_gpuTimerMeasuresMS[TIMER_BLEND_PASS] + m_gpuTimerMeasuresMS[TIMER_COMPOSITE_PASS] +
                m_gpuTimerMeasuresMS[TIMER_CLEAR_UAV_PASS];
            swprintf_s(buffer, std::size(buffer), L"Total blend time: %.2f ms", totalTimeSpent);
            m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor3);
            textPos.y += m_smallFontBold->GetLineSpacing();
        }

        // Space required for each technique.
        uint32_t reqSpace = (m_blendMode == ROP_BLEND) ? 0 :
            (m_blendMode == CUSTOM_BLEND_PPLL) ? m_PPLLSpaceRequired : m_MLABSpaceRequired;
        swprintf_s(buffer, std::size(buffer), L"Required space: %.2f MB", reqSpace / (1024.0f * 1024.0f));
        m_smallFontBold->DrawString(m_hudBatch.get(), buffer, textPos, textColor3);
        textPos.y += 2.0f * m_smallFontBold->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (6.0f * m_smallFont->GetLineSpacing());

        // Instructions
#ifdef _GAMING_XBOX
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A]/[B] Change blend mode", textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[X] Switch translucent model", textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor2);
#else
        swprintf_s(buffer, std::size(buffer), L"[Enter]/[Back] Change blend mode");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Space] Switch translucent model");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Esc] Exit Sample");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
#endif
    }

    m_hudBatch->End();
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
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    auto const size = m_deviceResources->GetOutputSize();
    m_screenWidth = static_cast<uint32_t>(size.right - size.left);
    m_screenHeight = static_cast<uint32_t>(size.bottom - size.top);

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 3840;
    height = 2160;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData)))
        || (featureData.ROVsSupported == false))
    {
#ifdef _DEBUG
        OutputDebugStringA("ROVs not supported!\n");
#endif
        throw std::runtime_error("ROVs not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    CreatePipelineStateObjects(device);

    // Create Descriptor heaps
    {
        // Create descriptor heaps and views.
        m_ShaderVisibleHeap = std::make_unique<DescriptorPile>(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            SHADER_VISIBLE_DESC_HEAP_COUNT + TRANSLUCENT_COUNT * 2u + OPAQUE_COUNT * 2u,
            SHADER_VISIBLE_DESC_HEAP_COUNT);
        m_ShaderVisibleHeap->Heap()->SetName(L"ShaderVisibleHeap");

        m_NonShaderVisibleHeap = std::make_unique<DescriptorHeap>(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            SHADER_VISIBLE_DESC_HEAP_COUNT);
        m_NonShaderVisibleHeap->Heap()->SetName(L"NonShaderVisibleHeap");

        m_RTVDescHeap = std::make_unique<DescriptorHeap>(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            RTV_INDEX_COUNT); 
        m_RTVDescHeap->Heap()->SetName(L"RTVDescHeap");
    }

    // Upload resources to video memory.
    ResourceUploadBatch upload(device);
    upload.Begin();

    // City.
    m_OpaqueObjs[OPAQUE_CITY] = new SampleObject(device, upload, m_ShaderVisibleHeap, L"CityBlockConcrete.sdkmesh");

    // Terrain.
    m_OpaqueObjs[OPAQUE_TERRAIN] = new SampleObject(device, upload, m_ShaderVisibleHeap, L"terrain.sdkmesh");

    // Translucent objects.
    for (uint32_t i = 0; i < TRANSLUCENT_COUNT; ++i)
    {
        m_translucentObjs[MODEL_GARGOYLES][i] = new SampleObject(device, upload, m_ShaderVisibleHeap, L"gargoyle.sdkmesh");
        m_translucentObjs[MODEL_DRAGONS][i] = new SampleObject(device, upload, m_ShaderVisibleHeap, L"dragon_LOD3.sdkmesh");
    }

    // Upload fonts for the HUD.
    {
        m_HUDDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device, 3);

        auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, upload, spritePSD);

        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, upload,
            strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(0),
            m_HUDDescriptorHeap->GetGpuHandle(0));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18_Bold.spritefont");
        m_smallFontBold = std::make_unique<SpriteFont>(device, upload,
            strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(1),
            m_HUDDescriptorHeap->GetGpuHandle(1));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(2),
            m_HUDDescriptorHeap->GetGpuHandle(2));
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);

    m_camera.SetWindow(static_cast<int>(m_screenWidth), static_cast<int>(m_screenHeight));
    m_camera.SetProjectionParameters(XM_PIDIV4, CAMERA_NEAR, CAMERA_FAR, true);
    m_camera.SetLookAt(SimpleMath::Vector3(0.0f, 0.0f, 0.0f), SimpleMath::Vector3::Forward);
    m_camera.SetSensitivity(200.0f, 400.0f, 500.0f, 10.0f);
    m_camera.SetPosition(CAMERA_POSITION);

    auto device = m_deviceResources->GetD3DDevice();

    // Intermediate copy target, needed for desktop scenario before swapchain copy.
#ifdef _GAMING_DESKTOP
    {
        const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resDesc = m_deviceResources->GetRenderTarget()->GetDesc();
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        CD3DX12_CLEAR_VALUE clearVal(m_deviceResources->GetBackBufferFormat(), RTVClearColor);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearVal,
            IID_GRAPHICS_PPV_ARGS(m_intermediateRT.ReleaseAndGetAddressOf())));
        m_intermediateRT->SetName(L"IntermediateRT");
    }
#endif

    CreateBlendPPLLResourcesAndViews();

    CreateBlendMLABResourcesAndViews();

    // Swapchain UAVs.
#ifndef _GAMING_DESKTOP
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavSwapchainDesc = {};
    uavSwapchainDesc.Format = m_deviceResources->GetBackBufferFormat();
    uavSwapchainDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_deviceResources->GetRenderTarget(i), nullptr,
            &uavSwapchainDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_SWAPCHAIN_01 + i));
    }
#else // _GAMING_DESKTOP
    // Needed due to desktop not accepting UAV for swapchain.
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavIntermediateDesc = {};
    uavIntermediateDesc.Format = m_deviceResources->GetBackBufferFormat();
    uavIntermediateDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_intermediateRT.Get(), nullptr, &uavIntermediateDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_INTERMEDIATE_RT));

    // Needed due to desktop not accepting UAV for swapchain.
    D3D12_RENDER_TARGET_VIEW_DESC rtvdesc = {};
    rtvdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvdesc.Format = m_deviceResources->GetBackBufferFormat();
    device->CreateRenderTargetView(m_intermediateRT.Get(), &rtvdesc, m_RTVDescHeap->GetCpuHandle(RTV_INDEX_INTERMEDIATE_RT));
#endif

    device->CopyDescriptorsSimple(SHADER_VISIBLE_DESC_HEAP_COUNT,
        m_ShaderVisibleHeap->GetFirstCpuHandle(),
        m_NonShaderVisibleHeap->GetFirstCpuHandle(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Common state promotion semantics are disabled by default on Xbox, so these need to be transitioned to UAV.
#ifdef _GAMING_XBOX_SCARLETT
    m_deviceResources->ResetCommandList();
    auto cmdList = m_deviceResources->GetCommandList();

    D3D12_RESOURCE_BARRIER barriers[4] = {};
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_MLABNodeListRes.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_PPLLHeadPointer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_perPixelLinkedListBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_PPLLBufferCounter.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    cmdList->Close();
    ID3D12CommandList* cmdLists[1] = { cmdList };
    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
    m_deviceResources->WaitForGpu();
#endif
}

void Sample::CreateBlendPPLLResourcesAndViews()
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D12_HEAP_PROPERTIES heapPropDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    uint32_t bufferSize1 = m_screenWidth * m_screenHeight * sizeof(uint32_t);
    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize1, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_PPLLHeadPointer.ReleaseAndGetAddressOf())));

    // For now, assume we want llNumEntries entries per linked-list (per pixel), each of 12bytes
    // (sizeof Fragment struct). This will be for 4k and 8 entries this is around 800MB.
    uint32_t numElements = m_screenWidth * m_screenHeight * NODE_COUNT_PPLL;
    uint32_t bufferSize2 = numElements * sizeof(Fragment);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize2, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_perPixelLinkedListBuffer.ReleaseAndGetAddressOf())));
    m_perPixelLinkedListBuffer->SetName(L"PPLLNodeList");

    D3D12_RESOURCE_DESC bufferDescCounter = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &bufferDescCounter,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_PPLLBufferCounter.ReleaseAndGetAddressOf())));
    m_PPLLBufferCounter->SetName(L"PPLLNodeListCounter");

    // Space required by this algorithm.
    m_PPLLSpaceRequired = bufferSize1 + bufferSize2 + sizeof(uint32_t);

    // UAV resource that keeps the head for each pixel's linked list.
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    uavDesc.Buffer.NumElements = (m_screenWidth * m_screenHeight);
    uavDesc.Buffer.StructureByteStride = 0;
    device->CreateUnorderedAccessView(m_PPLLHeadPointer.Get(), nullptr, &uavDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_PPLL_HEAD_POINTER_BUFFER));

    // UAV resource that keeps all the entries for the per pixel lists.
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc2 = {};
    uavDesc2.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc2.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc2.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    uavDesc2.Buffer.NumElements = numElements;
    uavDesc2.Buffer.StructureByteStride = sizeof(Fragment);
    device->CreateUnorderedAccessView(m_perPixelLinkedListBuffer.Get(), nullptr, &uavDesc2, m_NonShaderVisibleHeap->GetCpuHandle(UAV_PPLL_BUFFER));
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavCounterDesc = {};
    uavCounterDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavCounterDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavCounterDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    uavCounterDesc.Buffer.NumElements = 1;
    uavCounterDesc.Buffer.StructureByteStride = 0;
    device->CreateUnorderedAccessView(m_PPLLBufferCounter.Get(), nullptr, &uavCounterDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_PPLL_BUFFER_COUNTER));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescasd = {};
    srvDescasd.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDescasd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescasd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescasd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDescasd.Buffer.NumElements = (m_screenWidth * m_screenHeight);
    device->CreateShaderResourceView(m_PPLLHeadPointer.Get(), &srvDescasd, m_NonShaderVisibleHeap->GetCpuHandle(SRV_PPLL_HEAD_POINTER_BUFFER));

    srvDescasd = {};
    srvDescasd.Format = DXGI_FORMAT_UNKNOWN;
    srvDescasd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescasd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescasd.Buffer.NumElements = numElements;
    srvDescasd.Buffer.StructureByteStride = sizeof(Fragment);
    device->CreateShaderResourceView(m_perPixelLinkedListBuffer.Get(), &srvDescasd, m_NonShaderVisibleHeap->GetCpuHandle(SRV_PPLL_BUFFER));
}

void Sample::CreateBlendMLABResourcesAndViews()
{
    auto device = m_deviceResources->GetD3DDevice();
    D3D12_HEAP_PROPERTIES heapPropDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // Clear mask resource. A 2D texture which shows which pixel have been touched.
    D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_UINT, m_screenWidth, m_screenHeight);
    resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_MLABClearMaskRes.ReleaseAndGetAddressOf())));
    m_MLABClearMaskRes->SetName(L"UAVClearMaskRes");

    uint32_t imgSize = static_cast<uint32_t>(device->GetResourceAllocationInfo(0, 1, &resDesc).SizeInBytes);

    // Resource containing (width * height) elements, each of size (NODE_COUNT * sizeof(TransparentFragment)).
    // For 4K, 4 nodes and TransparentFragment of size 2 bytes, this equals 66MB. PPLL method, with 4 nodes, uses ~400MB.
    uint32_t bufferSize = m_screenWidth * m_screenHeight * NODE_COUNT_MLAB * sizeof(TransparentFragment);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapPropDefault,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_MLABNodeListRes.ReleaseAndGetAddressOf())));
    m_MLABNodeListRes->SetName(L"UAVNodeListRes");

    // Space required by this algorithm.
    m_MLABSpaceRequired = bufferSize + imgSize;

    // Create UAVs for pass. Written to in the Translucency PS pass.
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_MLABClearMaskRes.Get(), nullptr, &uavDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_MLAB_CLEAR_MASK));

    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = m_screenWidth * m_screenHeight;
    uavDesc.Buffer.StructureByteStride = NODE_COUNT_MLAB * sizeof(TransparentFragment);
    device->CreateUnorderedAccessView(m_MLABNodeListRes.Get(), nullptr, &uavDesc, m_NonShaderVisibleHeap->GetCpuHandle(UAV_MLAB_NODE_LIST));

    // Create SRVs for pass. Read in the composite compute pass.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_UINT;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_MLABClearMaskRes.Get(), &srvDesc, m_NonShaderVisibleHeap->GetCpuHandle(SRV_MLAB_CLEAR_MASK));

    srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = m_screenWidth * m_screenHeight;
    srvDesc.Buffer.StructureByteStride = NODE_COUNT_MLAB * sizeof(TransparentFragment);
    device->CreateShaderResourceView(m_MLABNodeListRes.Get(), &srvDesc, m_NonShaderVisibleHeap->GetCpuHandle(SRV_MLAB_NODE_LIST));
}

void Sample::CreatePipelineStateObjects(ID3D12Device* device)
{
    D3D12_INPUT_ELEMENT_DESC gElemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescNormal = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTexcoord0 = { "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTexcoord1 = { "TEXCOORDS", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gInputElemArr[] = { gElemDescPosition, gElemDescNormal, gElemDescTexcoord0, gElemDescTexcoord1 };

    // Create Pipeline State Objects and their associated Root Signatures.
    {
        // Opaque pass PSO and RS.
        {
            auto vsBlob = DX::ReadData(L"OpaqueVS.cso");
            auto psBlob = DX::ReadData(L"OpaquePS.cso");

            DX::ThrowIfFailed(device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_opaquePassRS.ReleaseAndGetAddressOf())));
            m_opaquePassRS->SetName(L"OpaquePassRS");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
            psoDesc.pRootSignature = m_opaquePassRS.Get();
            psoDesc.VS = { vsBlob.data(), vsBlob.size() };
            psoDesc.PS = { psBlob.data(), psBlob.size() };
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // Default for opaque ?
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Depth enabled, less 
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
            psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleMask = UINT32_MAX;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_opaquePassPSO.ReleaseAndGetAddressOf())));
            m_opaquePassPSO->SetName(L"OpaquePassPSO");

            // Blending pass (fixed HW)

            psBlob = DX::ReadData(L"TranslucentStandardPS.cso");

            psoDesc.PS = { psBlob.data(), psBlob.size() };
            psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_regularBlendPassPSO.ReleaseAndGetAddressOf())));
            m_regularBlendPassPSO->SetName(L"RegularBlendPassPSO");

            // Translucent using per-pixel linked lists (using atomics).

            psBlob = DX::ReadData(L"TranslucentPPLL_PS.cso");

            DX::ThrowIfFailed(device->CreateRootSignature(0, psBlob.data(), psBlob.size(), IID_GRAPHICS_PPV_ARGS(m_CustomBlendRS.ReleaseAndGetAddressOf())));
            m_CustomBlendRS->SetName(L"CustomBlendRS");

            psoDesc.pRootSignature = m_CustomBlendRS.Get();
            psoDesc.PS = { psBlob.data(), psBlob.size() };
            psoDesc.NumRenderTargets = 0;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_PPLLTranslucencyPassPSO.ReleaseAndGetAddressOf())));
            m_PPLLTranslucencyPassPSO->SetName(L"PPLLTranslucencyPassPSO");

            // Translucent using MLAB and ROVs.

            psBlob = DX::ReadData(L"TranslucentMLAB_ROV_PS.cso");

            psoDesc.PS = { psBlob.data(), psBlob.size() };

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_MLABTranslucencyPassPSO.ReleaseAndGetAddressOf())));
            m_MLABTranslucencyPassPSO->SetName(L"MLABTranslucencyPassPSO");
        }

        // Compute PSO for composite pass PPLL.
        {
            auto csBlob = DX::ReadData(L"CompositeTranslucentPPLL_CS.cso");

            DX::ThrowIfFailed(device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_compositePassRS.ReleaseAndGetAddressOf())));
            m_compositePassRS->SetName(L"CompositePassRS");

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_compositePassRS.Get();
            psoDesc.CS = { csBlob.data(), csBlob.size() };

            DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_compositePassPPLL_PSO.ReleaseAndGetAddressOf())));
            m_compositePassPPLL_PSO->SetName(L"CompositePassPPLL_PSO");
        }

        // Compute PSO for composite pass MLAB.
        {
            auto csBlob = DX::ReadData(L"CompositeTranslucentMLAB_CS.cso");

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_compositePassRS.Get();
            psoDesc.CS = { csBlob.data(), csBlob.size() };

            DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_compositePassMLAB_PSO.ReleaseAndGetAddressOf())));
            m_compositePassMLAB_PSO->SetName(L"CompositePassMLAB_PSO");
        }
    }
}
#pragma endregion

void Sample::OnDeviceLost()
{
    m_opaquePassPSO.Reset();
    m_opaquePassRS.Reset();
    m_regularBlendPassPSO.Reset();
    m_PPLLTranslucencyPassPSO.Reset();
    m_MLABTranslucencyPassPSO.Reset();
    m_CustomBlendRS.Reset();
    m_translucentPassPSO.Reset();
    m_compositePassPPLL_PSO.Reset();
    m_compositePassMLAB_PSO.Reset();
    m_compositePassRS.Reset();
    m_PPLLHeadPointer.Reset();
    m_perPixelLinkedListBuffer.Reset();
    m_PPLLBufferCounter.Reset();
    m_intermediateRT.Reset();
    m_MLABClearMaskRes.Reset();
    m_MLABNodeListRes.Reset();

    m_graphicsMemory.reset();
    m_RTVDescHeap.reset();
    m_HUDDescriptorHeap.reset();
    m_ShaderVisibleHeap.reset();
    m_NonShaderVisibleHeap.reset();
    m_texFactory.reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_smallFontBold.reset();
    m_ctrlFont.reset();
    m_deviceResources.reset();
    m_gpuTimer.reset();
    m_gamePad.reset();
    m_keyboard.reset();
    m_mouse.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

void Sample::SortGeometryByViewZ()
{
    Vector3 cameraPos = m_camera.GetPosition();
    for (uint32_t i = 0; i < TRANSLUCENT_COUNT; ++i)
    {
        Vector3 cameraToObj = m_translucentObjs[m_translucentChoice][i]->GetPosition() - cameraPos;
        float sqrMagnitude1 = cameraToObj.Dot(cameraToObj);

        for (uint32_t j = i + 1; j < TRANSLUCENT_COUNT; ++j)
        {
            cameraToObj = m_translucentObjs[m_translucentChoice][j]->GetPosition() - cameraPos;
            float sqrMagnitude2 = cameraToObj.Dot(cameraToObj);

            if (sqrMagnitude2 > sqrMagnitude1)
            {
                auto temp = m_translucentObjs[m_translucentChoice][j];
                m_translucentObjs[m_translucentChoice][j] = m_translucentObjs[m_translucentChoice][i];
                m_translucentObjs[m_translucentChoice][i] = temp;
            }
        }
    }
}
