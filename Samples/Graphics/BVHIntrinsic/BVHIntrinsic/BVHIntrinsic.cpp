//--------------------------------------------------------------------------------------
// BVHIntrinsic.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BVHIntrinsic.h"

// Shaders
#include "CompiledShaders/RaytraceTrianglesEmulatedIntrinsic.inc"
#include "CompiledShaders/RaytraceVoxelsEmulatedIntrinsic.inc"

#if defined(_GAMING_XBOX_SCARLETT)
#include "CompiledShaders/RaytraceTrianglesNativeIntrinsic.inc"
#include "CompiledShaders/RaytraceVoxelsNativeIntrinsic.inc"
#endif

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ReadCompressedData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* GetDeviceName()
    {
        switch (XSystemGetDeviceType())
        {
        case XSystemDeviceType::Pc: return L"PC";
        case XSystemDeviceType::XboxOne: return L"Xbox One";
        case XSystemDeviceType::XboxOneS: return L"Xbox One S";
        case XSystemDeviceType::XboxOneX: return L"Xbox One X";
        case XSystemDeviceType::XboxOneXDevkit: return L"Xbox One X Devkit";
        case XSystemDeviceType::XboxScarlettLockhart: return L"Xbox Series S";
        case XSystemDeviceType::XboxScarlettAnaconda: return L"Xbox Series X";
        case XSystemDeviceType::XboxScarlettDevkit: return L"Xbox Series X Devkit";
        case XSystemDeviceType::Unknown:
        default: return L"Unknown";
        }
    }

    struct Resolution
    {
        enum Enum
        {
            Resolution_720p,
            Resolution_1080p,
            Resolution_2160p,
            EnumCount
        };
    };

    const RECT g_Resolutions[Resolution::EnumCount] =
    {
        { 0, 0, 1280, 720 },
        { 0, 0, 1920, 1080 },
        { 0, 0, 3840, 2160 },
    };

    const DirectX::XMVECTORF32 FONT_COLORS[8] =
    {
        { 1, 1, 1, 1 },
        { 1, 1, 0, 1 },
        { 1, 0, 0, 1 },
        { 1, 0, 0, 1 },
        { 1, 1, 0, 1 },
        { 0, 1, 0, 1 },
        { 1, 0, 0, 1 },
        { 1, 0, 0, 1 },
    };

    const wchar_t* g_RenderModeNames[RenderMode::EnumCount] =
    {
        L"Normal", L"Depth", L"Traversal Cost", L"Primitive"
    };
}

Sample::Sample() noexcept(false) :
    m_trianglesPerLOD{},
    m_numVoxels(0),
    m_firstLeafNodeIndex(0),
    m_csuDescriptorSize(0),
    m_frame(0),
    m_renderMode(0),
    m_currentSceneIndex(Scenes::Triangles),
    m_currentColour(ARRAYSIZE(FONT_COLORS) - 1),
    m_currentResolution(Resolution::Resolution_720p),
    m_numResolutions(0),
    m_currentLOD(0),
    m_emulatedIntrinsic(false),
    m_hideHUD(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableDXR);

    m_cameras[Scenes::Triangles].SetPosition(XMVectorSet(130, 50, 70, 1));
    m_cameras[Scenes::Triangles].SetYaw(2.0f);
    
    //For rungholt
    m_cameras[Scenes::Voxels].SetPosition(XMVectorSet(1555.20764, 123.540230, 1705.15552, 1));
    m_cameras[Scenes::Voxels].SetYaw(8.82209301);
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
    
    if (XSystemGetDeviceType() == XSystemDeviceType::XboxOne)
    {
        // Durango doesn't support a 4K swapchain.
        m_numResolutions = ARRAYSIZE(g_Resolutions) - 1;        
    }
    else
    {
        m_numResolutions = ARRAYSIZE(g_Resolutions);
    }

    m_currentResolution = m_numResolutions - 1;
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

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_emulatedIntrinsic = !m_emulatedIntrinsic;
        }
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentColour = (m_currentColour + 1) % ARRAYSIZE(FONT_COLORS);
            m_currentColour = std::max(m_currentColour, 1U); // Exclude black.
        }
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentSceneIndex++;
        }
        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentLOD++;
        }
        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentLOD += (NUM_LODS - 1);
        }
        if (m_gamePadButtons.start == GamePad::ButtonStateTracker::PRESSED)
        {
            m_hideHUD = !m_hideHUD;
        }
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentResolution++;
        }
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_currentResolution += (m_numResolutions - 1);
        }
        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_renderMode++;
        }
        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_renderMode += (RenderMode::EnumCount - 1);
        }

        m_currentSceneIndex = (m_currentSceneIndex % Scenes::EnumCount);
        m_renderMode = (m_renderMode % RenderMode::EnumCount);
        m_currentResolution = (m_currentResolution % m_numResolutions);
        m_currentLOD = (m_currentLOD % NUM_LODS);
        
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_cameras[m_currentSceneIndex].Update(elapsedTime, pad);

#if !defined(_GAMING_XBOX_SCARLETT)
    m_emulatedIntrinsic = true;
#endif
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

    auto currentResolution = g_Resolutions[m_currentResolution];
    m_deviceResources->SetPresentSize(currentResolution);

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer.BeginFrame(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, "CopyAndClearStats");

    D3D12_RESOURCE_BARRIER defaultBufferToCopySource = CD3DX12_RESOURCE_BARRIER::Transition(m_statsBufferDefault.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, &defaultBufferToCopySource);

    commandList->CopyBufferRegion(m_statsBufferReadback.Get(), 0, m_statsBufferDefault.Get(), 0, 32);

    D3D12_RESOURCE_BARRIER defaultBufferToUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_statsBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &defaultBufferToUAV);

    commandList->FillMemoryWith32BitValueX(m_statsBufferDefault->GetGPUVirtualAddress(), 32, 0, D3D12XBOX_COPY_FLAG_NONE);
    PIXEndEvent(commandList);

    auto heap = m_csuHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    wchar_t renderStr[256];
    swprintf_s(renderStr, L"Render %u (%u by %u)", m_timer.GetFrameCount() - 1, currentResolution.right, currentResolution.bottom);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, renderStr);

    D3D12_RESOURCE_BARRIER outputToUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_outputTex.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &outputToUAV);
    
    ShaderConstants constants = {};

    switch (m_currentSceneIndex)
    {
    case Scenes::Triangles: PrepareToRenderTriangles(commandList, constants); break;
    case Scenes::Voxels: PrepareToRenderVoxels(commandList, constants); break;
    default: break;
    }
        
    commandList->SetComputeRootDescriptorTable(1, m_csuHeap->GetGpuHandle(SRVUAVDescriptors::OutputUAV));
    
    auto cameraIndex = m_currentSceneIndex < Scenes::EnumCount ? m_currentSceneIndex : 0;
    _Analysis_assume_(cameraIndex < Scenes::EnumCount);
    FreeCamera& currentCamera = m_cameras[cameraIndex];

    float screenToClipScaleX = 2.0f / currentResolution.right;
    float screenToClipScaleY = -2.0f / currentResolution.bottom;

    XMMATRIX screenToClip = XMMatrixAffineTransformation(XMVectorSet(screenToClipScaleX, screenToClipScaleY, 0, 0), XMVectorZero(), XMQuaternionIdentity(), XMVectorSet(-1, 1, 0, 0));
    XMMATRIX view = currentCamera.GetViewMatrix();
    XMMATRIX proj = currentCamera.GetProjectionMatrix();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invTemp = XMMatrixInverse(nullptr, viewProj);

    constants.screenToCameraSpace = XMMatrixMultiply(screenToClip, invTemp);
    constants.cameraPosition = currentCamera.GetPosition();
    constants.colour = m_currentColour;
    constants.renderMode = m_renderMode;
    constants.elapsedTime = float(m_timer.GetTotalSeconds()) / 10.0f;  // Slow down the sun
            
    commandList->SetComputeRoot32BitConstants(3, sizeof(constants) / sizeof(UINT), &constants, 0);
    commandList->SetComputeRootUnorderedAccessView(4, m_statsBufferDefault->GetGPUVirtualAddress());

    m_gpuTimer.Start(commandList, 0);

#if defined(_GAMING_XBOX_SCARLETT)
    commandList->Dispatch(UINT(currentResolution.right) / 8, UINT(currentResolution.bottom) / 4, 1);  // 8x4 thread group for Scarlett
#else
    commandList->Dispatch(UINT(currentResolution.right) / 8, UINT(currentResolution.bottom) / 8, 1);  // 8x8 thread group for Xbox One
#endif
    m_gpuTimer.Stop(commandList, 0);

    PIXEndEvent(commandList);

    // Pre copy barrier
    D3D12_RESOURCE_BARRIER outputToCopySource = CD3DX12_RESOURCE_BARRIER::Transition(m_outputTex.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, &outputToCopySource);
    commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_outputTex.Get());

    D3D12_RESOURCE_BARRIER swapChainToRTV = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &swapChainToRTV);

    if (!m_hideHUD)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD");
        RenderHUD(commandList);
        PIXEndEvent(commandList);
    }

    m_gpuTimer.EndFrame(commandList);



    // Show the new frame.
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
}

void Sample::PrepareToRenderVoxels(ID3D12GraphicsCommandList* cl, ShaderConstants& constants)
{
    constants.bvhAddress = m_voxelsBVH->GetGPUVirtualAddress();
    constants.bvhSize = uint32_t(m_voxelsBVH->GetDesc().Width);
    constants.firstLeafNodeIndex = m_firstLeafNodeIndex;

    cl->SetComputeRootSignature(m_voxelsRootSig.Get());
    cl->SetPipelineState(m_emulatedIntrinsic ? m_voxelsEmulatedPSO.Get() : m_voxelsNativePSO.Get());
    cl->SetComputeRootShaderResourceView(2, m_voxelsBVH->GetGPUVirtualAddress());
    cl->SetComputeRootShaderResourceView(5, m_voxelsBlocks->GetGPUVirtualAddress());
}

void Sample::PrepareToRenderTriangles(ID3D12GraphicsCommandList* cl, ShaderConstants& constants)
{
    constants.bvhAddress = m_trianglesBVH[m_currentLOD]->GetGPUVirtualAddress();
    constants.bvhSize = uint32_t(m_trianglesBVH[m_currentLOD]->GetDesc().Width);

    cl->SetComputeRootSignature(m_trianglesRootSig.Get());
    cl->SetPipelineState(m_emulatedIntrinsic ? m_trianglesEmulatedPSO.Get() : m_trianglesNativePSO.Get());
    cl->SetComputeRootShaderResourceView(2, m_trianglesBVH[m_currentLOD]->GetGPUVirtualAddress());
}

void Sample::RenderHUD(ID3D12GraphicsCommandList* cl)
{
    auto const size = g_Resolutions[m_currentResolution];
    D3D12_VIEWPORT viewport = { 0, 0, float(size.right), float(size.bottom), 0, 1 };

    m_hudBatch->SetViewport(viewport);
    m_hudBatch->Begin(cl);

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    cl->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);    
    cl->RSSetViewports(1, &viewport);
    cl->RSSetScissorRects(1, &size);

    auto& font = (m_currentResolution == Resolution::Resolution_2160p) ? m_bigFont : m_smallFont;
    
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    UINT* stats;
    m_statsBufferReadback->Map(0, nullptr, (void**)&stats);

    UINT totalPrimaryRays = stats[0] + stats[1];
    UINT totalShadowRays = stats[2] + stats[3];
    UINT totalSecondaryReflectionRays = stats[4] + stats[5];
    UINT totalSecondaryShadowRays = stats[6] + stats[7];
    UINT totalRays = totalPrimaryRays + totalShadowRays + totalSecondaryReflectionRays + totalSecondaryShadowRays;

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = FONT_COLORS[m_currentColour];

    font->DrawString(m_hudBatch.get(), "Custom Raytracing - No DXR", textPos, textColor);
    textPos.y += (font->GetLineSpacing() * 2);

    if (m_currentSceneIndex == Scenes::Triangles)
    {
        swprintf_s(textBuffer, L"Current Scene: Triangles (%0.1f MB)", m_trianglesBVH[m_currentLOD]->GetDesc().Width / 1024.0f / 1024.0f);
    }
    else
    {
        swprintf_s(textBuffer, L"Current Scene: Voxels (%u voxels, %0.1f MB)", m_numVoxels, m_voxelsBVH->GetDesc().Width / 1024.0f / 1024.0f);
    }
    
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    if (m_currentSceneIndex == Scenes::Triangles)
    {
        swprintf_s(textBuffer, L"Current LOD: %u (%u triangles)", m_currentLOD, m_trianglesPerLOD[m_currentLOD]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();
    }

    swprintf_s(textBuffer, L"Scarlett BVH Intrinsic: %ls", m_emulatedIntrinsic ? L"Emulated" : L"Native");
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    float averageRenderTime = m_gpuTimer.GetAverageMS(0);
    swprintf_s(textBuffer, L"Render Time: %0.2fms (%u FPS) at %ux%u", averageRenderTime, uint32_t(1000.0f / averageRenderTime), uint32_t(viewport.Width), uint32_t(viewport.Height));
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();
        
    float raysPerSecond = totalRays * (1000.0f / averageRenderTime);
    float gigaraysPerSecond = float(raysPerSecond / 1E9);

    swprintf_s(textBuffer, L"Gigarays per second: %0.2f", gigaraysPerSecond);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Device Type: %ls", GetDeviceName());
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();
    
    swprintf_s(textBuffer, L"Primary Ray Hits: %u", stats[0]);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    swprintf_s(textBuffer, L"Primary Ray Misses: %u", stats[1]);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    if (m_currentSceneIndex == Scenes::Voxels)
    {
        swprintf_s(textBuffer, L"Shadow Ray Hits: %u", stats[2]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();

        swprintf_s(textBuffer, L"Shadow Ray Misses: %u", stats[3]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();

        swprintf_s(textBuffer, L"Reflection Ray Hits: %u", stats[4]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();

        swprintf_s(textBuffer, L"Reflection Ray Misses: %u", stats[5]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();

        swprintf_s(textBuffer, L"Reflection Shadow Ray Hits: %u", stats[6]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();

        swprintf_s(textBuffer, L"Reflection Shadow Ray Misses: %u", stats[7]);
        font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();
    }

    swprintf_s(textBuffer, L"Render Mode: %ls", g_RenderModeNames[m_renderMode]);
    font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();

    m_statsBufferReadback->Unmap(0, nullptr);

    // It doesn't fit on the screen at 720p...
    if(m_currentResolution != Resolution::Resolution_720p)
    {
        textPos = XMFLOAT2(float(safe.left), float(safe.bottom));

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[Menu] : Toggle HUD", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[A] : Emulated / Native BVH intrinsic", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[B] : Change colour", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[X] : Change scene", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[LThumb] : Turbo (click)", textPos, textColor);

        if (m_currentSceneIndex == Scenes::Triangles)
        {
            textPos.y -= font->GetLineSpacing();
            DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[DPAD] : Change LOD", textPos, textColor);
        }

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[DPAD] : Change Resolution", textPos, textColor);

        textPos.y -= font->GetLineSpacing();
        DX::DrawControllerString(m_hudBatch.get(), font.get(), m_ctrlFont.get(), L"[LB][RB] : Change Render Mode", textPos, textColor);
    }

    m_hudBatch->End();
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
    m_csuHeap = std::make_unique<DescriptorHeap>(device, SRVUAVDescriptors::EnumCount);

    LoadCompressedBuffer(L"Assets/Models/rungholt.bi_", m_voxelsBVH);

    //Extract prim count and firstLeafNodeIndex from the end of the buffer
    UINT64 bufferSize = m_voxelsBVH->GetDesc().Width / 4;

    UINT* addr;
    m_voxelsBVH->Map(0, nullptr, (void**)&addr);
    m_numVoxels = addr[bufferSize - 2];
    m_firstLeafNodeIndex = addr[bufferSize - 1];

    LoadBuffer(L"Assets/Models/rungholtblocks.bin", m_voxelsBlocks);

    for (uint32_t i = 0; i < NUM_LODS; i++)
    {
        if (!i)
        {
            LoadCompressedBuffer(L"Assets/Models/dragon_LOD0.bi_", m_trianglesBVH[0]);
        }
        else
        {
            wchar_t str[256] = {};
            swprintf_s(str, L"Assets/Models/dragon_LOD%u.bin", i);
            LoadBuffer(str, m_trianglesBVH[i]);
        }

        // Last 4 bytes are the index of the first leaf node. Extract that.
        bufferSize = m_trianglesBVH[i]->GetDesc().Width / 4;
        m_trianglesBVH[i]->Map(0, nullptr, (void**)&addr);
        m_trianglesPerLOD[i] = addr[bufferSize - 2];
    }

    LoadSky(L"Assets/Textures/NoonGrassEnvHDR.dds", L"Assets/Textures/NoonGrassDiffuseHDR.dds");
    
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // HUD
    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FontSRVSmall),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FontSRVSmall));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    m_bigFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::FontSRVBig),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::FontSRVBig));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuHeap->GetCpuHandle(SRVUAVDescriptors::ControllerFontSRV),
        m_csuHeap->GetGpuHandle(SRVUAVDescriptors::ControllerFontSRV));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    {
        DX::ThrowIfFailed(device->CreateRootSignature(0, g_RaytraceTrianglesEmulatedIntrinsic, sizeof(g_RaytraceTrianglesEmulatedIntrinsic), IID_GRAPHICS_PPV_ARGS(m_trianglesRootSig.GetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.CS = { g_RaytraceTrianglesEmulatedIntrinsic, sizeof(g_RaytraceTrianglesEmulatedIntrinsic) };
        psoDesc.pRootSignature = m_trianglesRootSig.Get();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_trianglesEmulatedPSO.GetAddressOf())));

#if defined(_GAMING_XBOX_SCARLETT)
        psoDesc.CS = { g_RaytraceTrianglesNativeIntrinsic, sizeof(g_RaytraceTrianglesNativeIntrinsic) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_trianglesNativePSO.GetAddressOf())));
#endif
    }

    {
        DX::ThrowIfFailed(device->CreateRootSignature(0, g_RaytraceVoxelsEmulatedIntrinsic, sizeof(g_RaytraceVoxelsEmulatedIntrinsic), IID_GRAPHICS_PPV_ARGS(m_voxelsRootSig.GetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.CS = { g_RaytraceVoxelsEmulatedIntrinsic, sizeof(g_RaytraceVoxelsEmulatedIntrinsic) };
        psoDesc.pRootSignature = m_voxelsRootSig.Get();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_voxelsEmulatedPSO.GetAddressOf())));

#if defined(_GAMING_XBOX_SCARLETT)
        psoDesc.CS = { g_RaytraceVoxelsNativeIntrinsic, sizeof(g_RaytraceVoxelsNativeIntrinsic) };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_voxelsNativePSO.GetAddressOf())));
#endif
    }

    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_HEAP_PROPERTIES readbackProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    
    D3D12_RESOURCE_DESC defaultStatsDesc = CD3DX12_RESOURCE_DESC::Buffer(65536, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D12_RESOURCE_DESC readbackStatsDesc = CD3DX12_RESOURCE_DESC::Buffer(65536);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &defaultStatsDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_GRAPHICS_PPV_ARGS(m_statsBufferDefault.GetAddressOf())));
    DX::ThrowIfFailed(device->CreateCommittedResource(&readbackProps, D3D12_HEAP_FLAG_NONE, &readbackStatsDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_statsBufferReadback.GetAddressOf())));
    
    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());
}

void Sample::LoadBuffer(const wchar_t* file, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer)
{
    auto device = m_deviceResources->GetD3DDevice();
    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    HANDLE fh = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (fh != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        GetFileSizeEx(fh, &fileSize);

        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(UINT64(fileSize.QuadPart));
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(buffer.GetAddressOf())));

        void* ptr;
        buffer->Map(0, nullptr, &ptr);

        if (!ReadFile(fh, ptr, DWORD(bufferDesc.Width), nullptr, nullptr))
        {
            auto hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(fh);
            DX::ThrowIfFailed(hr);
        }
#pragma warning(suppress : 6001) // fh is initialized by CreateFileW and guarded by INVALID_HANDLE_VALUE check
        CloseHandle(fh);
    }
}

void Sample::LoadCompressedBuffer(const wchar_t* file, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer)
{
    auto device = m_deviceResources->GetD3DDevice();
    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    auto data = DX::ReadCompressedData(file);

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(UINT64(data.size()));
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(buffer.GetAddressOf())));

    void* ptr;
    buffer->Map(0, nullptr, &ptr);

    memcpy(ptr, data.data(), data.size());
}

void Sample::LoadSky(const wchar_t* radianceTex, const wchar_t* irradianceTex)
{
    const wchar_t* filenames[2] = { radianceTex, irradianceTex };
    ID3D12Resource** resources[2] = { m_radianceResource.GetAddressOf(), m_irradianceResource.GetAddressOf() };

    auto device = m_deviceResources->GetD3DDevice();

    std::vector<D3D12_SUBRESOURCE_DATA> subresourceDatas;
    std::unique_ptr<uint8_t[]> ddsData;

    for (size_t i = 0; i < ARRAYSIZE(filenames); ++i)
    {
        DX::ThrowIfFailed(DirectX::LoadDDSTextureFromFile(device, filenames[i], resources[i], ddsData, subresourceDatas));

        ResourceUploadBatch uploadBatch(device);
        uploadBatch.Begin();
        uploadBatch.Upload(*resources[i], 0, &subresourceDatas[0], (UINT)subresourceDatas.size());
        uploadBatch.Transition(*resources[i], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        uploadBatch.End(m_deviceResources->GetCommandQueue());
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC cubeSRVDesc = {};
    cubeSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    cubeSRVDesc.Format = m_radianceResource->GetDesc().Format;
    cubeSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    cubeSRVDesc.TextureCube.MipLevels = 0xFFFFFFFF; 

    device->CreateShaderResourceView(m_radianceResource.Get(), &cubeSRVDesc, m_csuHeap->GetCpuHandle(SRVUAVDescriptors::SkyRadianceResource));
    device->CreateShaderResourceView(m_irradianceResource.Get(), &cubeSRVDesc, m_csuHeap->GetCpuHandle(SRVUAVDescriptors::SkyIrradianceResource));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const outputSize = m_deviceResources->GetOutputSize();

    D3D12_RESOURCE_DESC outputDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_deviceResources->GetBackBufferFormat(), UINT64(outputSize.right), UINT(outputSize.bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &outputDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_outputTex.GetAddressOf())));

    device->CreateUnorderedAccessView(m_outputTex.Get(), nullptr, nullptr, m_csuHeap->GetCpuHandle(SRVUAVDescriptors::OutputUAV));
}
#pragma endregion
