//--------------------------------------------------------------------------------------
// VisibilityBuffer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "VisibilityBuffer.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ReadData.h"
#include "ReadCompressedData.h"

#include "Helpers.h"

// Setup Agility SDK exports: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_SampleTitle = L"Visibility Buffer";
    const wchar_t* g_SampleDescription = L"This sample demonstrates a visibility buffer (deferred) rendering technique.";
    const ATG::HelpButtonAssignment g_HelpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Show/Hide Help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::LEFT_STICK,          L"Translate X/Z" },
        { ATG::HelpID::RIGHT_STICK,         L"Look rotation" },
        { ATG::HelpID::LEFT_STICK_CLICK,    L"Turbo" },
        { ATG::HelpID::LEFT_TRIGGER,        L"Decrease Y position" },
        { ATG::HelpID::RIGHT_TRIGGER,       L"Increase Y position" },
        { ATG::HelpID::A_BUTTON,            L"Toggle render mode" },
        { ATG::HelpID::B_BUTTON,            L"Cycle Visibility Buffer overlay" },
#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
        { ATG::HelpID::X_BUTTON,            L"Toggle mesh shader/vertex shader" },
#endif
    };
}


Sample::Sample() noexcept(false) :
    m_frame(0),
    m_useVisibility(true),
#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
    m_useMeshShaders(true),
#else
    m_useMeshShaders(false),
#endif
    m_overlayMode(OverlayModes::None),
    m_showHelp(false),
    m_vertexBufferView{},
    m_indexBufferView{},
    m_objectData{},
    m_camera(0.1f, 1000.0f, 1.0f, 2.0f, 1.5f, false),
    m_ThreadGroupX(0),
    m_ThreadGroupY(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_visibilityBuffer = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R32_UINT);
    m_visibilityBuffer->SetClearColor(Colors::Black);

    XMVECTORF32 rightVector = { 1, 0, 0, 0 };
    XMVECTORF32 upVector = { 0, 1, 0, 0 };
    XMVECTORF32 forwardVector = { 0, 0, -1, 0 };

    XMVECTORF32 position = { 0, 2, 10, 0 };

    m_camera.SetRightVector(rightVector);
    m_camera.SetUpVector(upVector);
    m_camera.SetForwardVector(forwardVector);

    m_camera.SetPosition(position);

    m_help = std::make_unique<ATG::Help>(g_SampleTitle, g_SampleDescription, g_HelpButtons, std::size(g_HelpButtons));
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
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

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    float elapsed = static_cast<float>(m_timer.GetElapsedSeconds());

#ifdef USING_GAMEINPUT
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif
    
    if (pad.IsConnected())
    {
        m_buttonTracker.Update(pad);
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_buttonTracker.Reset();
    }

    if (m_buttonTracker.menu == GamePad::ButtonStateTracker::PRESSED)
    {
        m_showHelp = !m_showHelp;
    }
    else if (m_showHelp && m_buttonTracker.b == GamePad::ButtonStateTracker::PRESSED)
    {
        m_showHelp = false;
    }
    else if (m_buttonTracker.a == GamePad::ButtonStateTracker::PRESSED)
    {
        m_useVisibility = !m_useVisibility;
        m_gpuTimer->Reset();
    }
    else if (m_buttonTracker.b == GamePad::ButtonStateTracker::PRESSED)
    {
        m_overlayMode = static_cast<OverlayModes::Value>(m_overlayMode + 1);

        // Skip meshlet visualization in vertex shader mode, or on Xbox One (as mesh shaders are not supported).
        if (m_overlayMode == OverlayModes::MeshletID && !m_useMeshShaders)
        {
            m_overlayMode = static_cast<OverlayModes::Value>(m_overlayMode + 1);;
        }

        // Loop to start of overlay modes.
        if (m_overlayMode >= OverlayModes::Count)
        {
            m_overlayMode = OverlayModes::None;
        }
    }

#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
    if (m_buttonTracker.x == GamePad::ButtonStateTracker::PRESSED)
    {
        m_useMeshShaders = !m_useMeshShaders;

        if (m_overlayMode == OverlayModes::MeshletID)
        {
            m_overlayMode = OverlayModes::None;
        }

        m_gpuTimer->Reset();
    }
#endif

    OutputDebugStringA(("Render MS: " + std::to_string(m_gpuTimer->GetAverageMS(0)) + "\n").c_str());
    OutputDebugStringA(("Use Visibility: " + std::to_string(m_useVisibility) + "\n").c_str());

    m_camera.Update(elapsed, pad);
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

    auto commandList = m_deviceResources->GetCommandList();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    if (m_showHelp)
    {
        m_help->Render(commandList);
    }
    else
    {
        m_gpuTimer->BeginFrame(commandList);
        m_gpuTimer->Start(commandList, 0);

        ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_samplerDescriptors->Heap() };
        commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

        if (m_useVisibility)
        {
            // Render scene using "defferred" visibility buffer method.
            RenderVisibilityMode();
        }
        else
        {
            // Render scene using normal forward rendering.
            RenderForwardMode();
        }

        m_gpuTimer->Stop(commandList, 0);
        m_gpuTimer->EndFrame(commandList);

        RenderHUD();
    }

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderVisibilityMode()
{
    auto commandList = m_deviceResources->GetCommandList();
    XMMATRIX viewProjectionTransform = XMMatrixMultiply(m_camera.GetViewMatrix(), m_camera.GetProjectionMatrix());

    // Render scene to 32-bit visibility (objectID + primitiveID) buffer.
    {
        m_visibilityBuffer->BeginScene(commandList);

        commandList->SetGraphicsRootSignature(m_visibilityRootSignature.Get());

#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
        if (m_useMeshShaders)
        {
            commandList->SetPipelineState(m_visibilityMSPSO.Get());
        }
        else
#endif
        {
            commandList->SetPipelineState(m_visibilityVSPSO.Get());
        }

        auto const rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::VisibilityRTV);
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->ClearRenderTargetView(rtvDescriptor, Colors::Black, 0, nullptr);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

        for (uint32_t i = 0; i < s_numDragons; i++)
        {
            ConstantsVis rootConstants;
            rootConstants.objectIndex = i;
            rootConstants.vertexBufferIndex = 0;
            rootConstants.drawOverlay = m_overlayMode;

            XMMATRIX modelTransform = XMLoadFloat4x4(&m_objectData[i].modelTransform);
            XMMATRIX mvpTransform = XMMatrixMultiply(modelTransform, viewProjectionTransform);

            XMStoreFloat4x4(&rootConstants.mvpMatrix, mvpTransform);

            commandList->SetGraphicsRoot32BitConstants(RootParamCB, sizeof(rootConstants) / sizeof(uint32_t), &rootConstants, 0);

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
            if (m_useMeshShaders)
            {
                commandList->DispatchMesh(static_cast<UINT>(m_meshletsData[0].size()), 1, 1);
            }
            else
#endif
            {
                commandList->IASetIndexBuffer(&m_indexBufferView[0]);
                commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
                commandList->DrawIndexedInstanced(m_model[0]->indexCount, 1, 0, 0, 0);
            }
        }

        ConstantsVis rootConstants;
        rootConstants.objectIndex = s_numDragons;
        rootConstants.vertexBufferIndex = 1;
        rootConstants.drawOverlay = m_overlayMode;

        XMMATRIX modelTransform = XMLoadFloat4x4(&m_objectData[s_numDragons].modelTransform);
        XMMATRIX mvpTransform = XMMatrixMultiply(modelTransform, viewProjectionTransform);

        XMStoreFloat4x4(&rootConstants.mvpMatrix, mvpTransform);

        commandList->SetGraphicsRoot32BitConstants(RootParamCB, sizeof(rootConstants) / sizeof(uint32_t), &rootConstants, 0);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
        if (m_useMeshShaders)
        {
            commandList->DispatchMesh(static_cast<UINT>(m_meshletsData[1].size()), 1, 1);
        }
        else
#endif
        {
            commandList->IASetIndexBuffer(&m_indexBufferView[1]);
            commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[1]);
            commandList->DrawIndexedInstanced(m_model[1]->indexCount, 1, 0, 0, 0);
        }

        m_visibilityBuffer->EndScene(commandList);
    }

    // Run fullscreen reconstruction pass in compute.
    // Performs position and uv reconstruction, then runs pixel shader logic per screen pixel.
    {
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        ConstantElement fullscreenConstants;

        XMMATRIX viewTransformNoTranslation = m_camera.GetViewMatrix();
        viewTransformNoTranslation.r[3] = g_XMIdentityR3;
        XMMATRIX viewProjectionTransformNoTranslation = XMMatrixMultiply(viewTransformNoTranslation, m_camera.GetProjectionMatrix());

        XMMATRIX inverseProjectionTransform = XMMatrixInverse(nullptr, viewProjectionTransformNoTranslation);
        XMStoreFloat4x4(&fullscreenConstants.inverseViewProjection, inverseProjectionTransform);
        XMStoreFloat3(&fullscreenConstants.cameraPosition, m_camera.GetPosition());
        fullscreenConstants.uavIndex = m_deviceResources->GetCurrentFrameIndex();
        fullscreenConstants.drawOverlay = m_overlayMode;

        D3D12_RESOURCE_BARRIER barrierDesc = {};

        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.Transition.pResource = m_renderTexture[m_deviceResources->GetCurrentFrameIndex()].Get();
        barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        commandList->ResourceBarrier(1, &barrierDesc);

        commandList->SetComputeRootSignature(m_computeRootSignature.Get());

        commandList->SetComputeRoot32BitConstants(RootParamCB, sizeof(fullscreenConstants) / sizeof(uint32_t), &fullscreenConstants, 0);

        commandList->SetPipelineState(m_computePSO.Get());
        commandList->Dispatch(m_ThreadGroupX, m_ThreadGroupY, 1);

        D3D12_RESOURCE_BARRIER barrierDesc2 = {};
        barrierDesc2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc2.Transition.pResource = m_renderTexture[m_deviceResources->GetCurrentFrameIndex()].Get();
        barrierDesc2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc2.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrierDesc2.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        commandList->ResourceBarrier(1, &barrierDesc2);
    }

    // Copy output of compute pass to swapchain.
    {
        RECT outputSize = m_deviceResources->GetOutputSize();

        m_spriteBatch->Begin(commandList);
        XMUINT2 texSize(uint32_t(outputSize.right), uint32_t(outputSize.bottom));
        XMFLOAT2 texLoc(0, 0);
        auto textureSRV = m_resourceDescriptors->GetGpuHandle(Descriptors::OutputSRV + m_deviceResources->GetCurrentFrameIndex());
        m_spriteBatch->Draw(textureSRV, texSize, texLoc);
        m_spriteBatch->End();
    }
}

void Sample::RenderForwardMode()
{
    auto commandList = m_deviceResources->GetCommandList();
    XMMATRIX viewProjectionTransform = XMMatrixMultiply(m_camera.GetViewMatrix(), m_camera.GetProjectionMatrix());

    commandList->SetGraphicsRootSignature(m_rasterRootSignature.Get());
    commandList->SetPipelineState(m_rasterPSO.Get());

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::Black, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->SetGraphicsRootDescriptorTable(2, m_samplerDescriptors->GetGpuHandle(Samplers::LinearSampler));

    for (uint32_t i = 0; i < s_numDragons; i++)
    {
        ConstantsVis rootConstants;
        rootConstants.objectIndex = i;
        rootConstants.vertexBufferIndex = 0;

        XMMATRIX modelTransform = XMLoadFloat4x4(&m_objectData[i].modelTransform);
        XMMATRIX mvpTransform = XMMatrixMultiply(modelTransform, viewProjectionTransform);

        XMStoreFloat4x4(&rootConstants.mvpMatrix, mvpTransform);

        commandList->SetGraphicsRoot32BitConstants(RootParamCB, sizeof(rootConstants) / sizeof(uint32_t), &rootConstants, 0);
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::DragonTexture + m_objectData[i].textureIDs[0]));

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
        commandList->IASetIndexBuffer(&m_indexBufferView[0]);

        commandList->DrawIndexedInstanced(m_model[0]->indexCount, 1, 0, 0, 0);
    }

    ConstantsVis rootConstants;
    rootConstants.objectIndex = s_numDragons;
    rootConstants.vertexBufferIndex = 1;

    XMMATRIX modelTransform = XMLoadFloat4x4(&m_objectData[s_numDragons].modelTransform);
    XMMATRIX mvpTransform = XMMatrixMultiply(modelTransform, viewProjectionTransform);

    XMStoreFloat4x4(&rootConstants.mvpMatrix, mvpTransform);

    commandList->SetGraphicsRoot32BitConstants(RootParamCB, sizeof(rootConstants) / sizeof(uint32_t), &rootConstants, 0);
    commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::DragonTexture + m_objectData[s_numDragons].textureIDs[0]));
    commandList->SetGraphicsRootDescriptorTable(2, m_samplerDescriptors->GetGpuHandle(Samplers::PointSampler));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[1]);
    commandList->IASetIndexBuffer(&m_indexBufferView[1]);

    commandList->DrawIndexedInstanced(m_model[1]->indexCount, 1, 0, 0, 0);
}

void Sample::RenderHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    auto const size = m_deviceResources->GetOutputSize();
    D3D12_VIEWPORT viewport = { 0, 0, float(size.right), float(size.bottom), 0, 1 };

    auto& font = ((size.right - size.left) > 1920) ? m_bigFont : m_smallFont;

    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    m_spriteBatch->Begin(commandList);

    wchar_t textBuffer[128] = {};
    XMVECTOR textColor = { 1.0, 1.0, 1.0, 1.0 };
    XMVECTOR textColorShadow = { 0.0, 0.0, 0.0, 1.0 };
    float shadowOffset = font->GetLineSpacing() * 0.03f;
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMFLOAT2 textPosShadow = XMFLOAT2(float(safe.left) + shadowOffset, float(safe.top) + shadowOffset);

    font->DrawString(m_spriteBatch.get(), "Visibility Buffer", textPosShadow, textColorShadow);
    font->DrawString(m_spriteBatch.get(), "Visibility Buffer", textPos, textColor);
    textPos.y += (font->GetLineSpacing() * 2);
    textPosShadow.y += (font->GetLineSpacing() * 2);

    swprintf_s(textBuffer, L"Render Mode: %ls", m_useVisibility ? L"Visibility Buffer (Dynamic Resources)" : L"Forward (Bound Resources)");
    font->DrawString(m_spriteBatch.get(), textBuffer, textPosShadow, textColorShadow);
    font->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();
    textPosShadow.y += font->GetLineSpacing();

    if (m_useVisibility)
    {
        swprintf_s(textBuffer, L"Geometry Mode: %ls", m_useMeshShaders ? L"Mesh Shaders" : L"Vertex Shaders");
        font->DrawString(m_spriteBatch.get(), textBuffer, textPosShadow, textColorShadow);
        font->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();
        textPosShadow.y += font->GetLineSpacing();

        const wchar_t* overlayModeNames[] = { L"None", L"PrimitiveID", L"ObjectID", L"MeshletID" };

        swprintf_s(textBuffer, L"Overlay: %ls", overlayModeNames[m_overlayMode]);
        font->DrawString(m_spriteBatch.get(), textBuffer, textPosShadow, textColorShadow);
        font->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += font->GetLineSpacing();
        textPosShadow.y += font->GetLineSpacing();
    }

    float averageRenderTime = m_gpuTimer->GetAverageMS(0);
    swprintf_s(textBuffer, L"Render Time: %0.2fms (%u FPS) at %ux%u", averageRenderTime, uint32_t(1000.0f / averageRenderTime), uint32_t(viewport.Width), uint32_t(viewport.Height));
    font->DrawString(m_spriteBatch.get(), textBuffer, textPosShadow, textColorShadow);
    font->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
    textPos.y += font->GetLineSpacing();
    textPosShadow.y += font->GetLineSpacing();

    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));
    textPosShadow = XMFLOAT2(float(safe.left) + shadowOffset, float(safe.bottom) + shadowOffset);

    textPos.y -= font->GetLineSpacing();
    textPosShadow.y -= font->GetLineSpacing();
    DX::DrawControllerString(m_spriteBatch.get(), font.get(), m_ctrlFont.get(), L"[View] Exit   [Menu] Help", textPosShadow, textColorShadow);
    DX::DrawControllerString(m_spriteBatch.get(), font.get(), m_ctrlFont.get(), L"[View] Exit   [Menu] Help", textPos, textColor);

    m_spriteBatch->End();
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

    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    
#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.6 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.6 is not supported!");
    }
#endif

    // Setup rendering memory and heaps.
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);
    m_samplerDescriptors = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, Descriptors::Count);
    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    m_visibilityBuffer->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::VisibilityBuffer),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::VisibilityRTV));

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    CreateSamplers();

    LoadModels();

    BuildPSOs();

    BuildObjectBuffer();

    LoadTextures();

    BuildHUDObjects();
}

void Sample::CreateSamplers()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create samplers in heap. Can be used with SamplerDescriptorHeap[] in SM 6.6.

    D3D12_SAMPLER_DESC linearSamplerDesc
    {
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // AddressW
        0, // MipLODBias
        D3D12_MAX_MAXANISOTROPY,
        D3D12_COMPARISON_FUNC_NEVER,
        { 0, 0, 0, 0 }, // BorderColor
        0, // MinLOD
        FLT_MAX // MaxLOD
    };

    D3D12_SAMPLER_DESC pointSamplerDesc = linearSamplerDesc;
    pointSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

    device->CreateSampler(&linearSamplerDesc, m_samplerDescriptors->GetCpuHandle(Samplers::LinearSampler));
    device->CreateSampler(&pointSamplerDesc, m_samplerDescriptors->GetCpuHandle(Samplers::PointSampler));
}

void Sample::LoadModels()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Load sdkmesh models.
    OutputDebugStringA("Loading model...\n");
#ifdef _GAMING_DESKTOP
    const wchar_t* dragonPath = L"ATGDragonPosedWithBakedDiffuse\\Dragon_LOD1_993KTri.sdkmes_";
    const wchar_t* cityPath   = L"AliasSampleCityBlock\\CityBlockConcrete.sdkmesh";
#else
    const wchar_t* dragonPath = L"Dragon_LOD1_993KTri.sdkmes_";
    const wchar_t* cityPath = L"CityBlockConcrete.sdkmesh";
#endif

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, dragonPath);
    Helpers::LoadModel(device, m_deviceResources->GetCommandQueue(), strFilePath, m_model[0], &m_vertexBufferView[0], &m_indexBufferView[0], true);

    OutputDebugStringA("Splitting meshlets for dragon...\n");
    SplitMeshlets(static_cast<uint32_t*>(m_model[0]->indexBuffer.Memory()), m_model[0]->indexCount, m_uniqueIndicesData[0], m_meshletsData[0], m_primitiveIndicesData[0], MESHLET_MAX_VERTICES, MESHLET_MAX_PRIMITIVES);

    DX::FindMediaFile(strFilePath, MAX_PATH, cityPath);
    Helpers::LoadModel(device, m_deviceResources->GetCommandQueue(), strFilePath, m_model[1], &m_vertexBufferView[1], &m_indexBufferView[1], false);

    OutputDebugStringA("Splitting meshlets for city...\n");
    SplitMeshlets(static_cast<uint32_t*>(m_model[1]->indexBuffer.Memory()), m_model[1]->indexCount, m_uniqueIndicesData[1], m_meshletsData[1], m_primitiveIndicesData[1], MESHLET_MAX_VERTICES, MESHLET_MAX_PRIMITIVES);

    OutputDebugStringA("Uploading resources...\n");
    for (size_t i = 0; i < std::size(m_model); i++)
    {
        // Create SRV for vertex buffer.
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = m_model[i]->vertexCount;
            srvDesc.Buffer.StructureByteStride = m_model[i]->vertexStride;

            device->CreateShaderResourceView(m_model[i]->staticVertexBuffer.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::VertexBuffer + i));
        }

        // Create SRV for index buffer.
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = m_model[i]->indexCount;
            srvDesc.Buffer.StructureByteStride = sizeof(uint32_t); // NOTE: currently requires models to have 32-bit index buffers.

            device->CreateShaderResourceView(m_model[i]->staticIndexBuffer.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::IndexBuffer + i));
        }

        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            auto resData = CD3DX12_RESOURCE_DESC::Buffer(m_uniqueIndicesData[i].size() * sizeof(m_uniqueIndicesData[i][0]));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resData,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_uniqueIndicesBuffer[i].ReleaseAndGetAddressOf())));

            {
                D3D12_SUBRESOURCE_DATA initData = { m_uniqueIndicesData[i].data(), 0, 0 };

                ResourceUploadBatch resourceUpload(device);

                resourceUpload.Begin();

                resourceUpload.Upload(m_uniqueIndicesBuffer[i].Get(), 0, &initData, 1);

                resourceUpload.Transition(m_uniqueIndicesBuffer[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

                auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
                uploadResourcesFinished.wait();
            }

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = static_cast<UINT>(m_uniqueIndicesData[i].size());
            srvDesc.Buffer.StructureByteStride = sizeof(m_uniqueIndicesData[i][0]);

            device->CreateShaderResourceView(m_uniqueIndicesBuffer[i].Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::UniqueIndices + i));
        }

        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            auto resData = CD3DX12_RESOURCE_DESC::Buffer(m_primitiveIndicesData[i].size() * sizeof(m_primitiveIndicesData[i][0]));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resData,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_primitiveIndicesBuffer[i].ReleaseAndGetAddressOf())));

            {
                D3D12_SUBRESOURCE_DATA initData = { m_primitiveIndicesData[i].data(), 0, 0 };

                ResourceUploadBatch resourceUpload(device);

                resourceUpload.Begin();

                resourceUpload.Upload(m_primitiveIndicesBuffer[i].Get(), 0, &initData, 1);

                resourceUpload.Transition(m_primitiveIndicesBuffer[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

                auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
                uploadResourcesFinished.wait();
            }

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = static_cast<UINT>(m_primitiveIndicesData[i].size()) / 3;
            srvDesc.Buffer.StructureByteStride = sizeof(m_primitiveIndicesData[i][0]) * 3;

            device->CreateShaderResourceView(m_primitiveIndicesBuffer[i].Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::PrimitiveIndices + i));
        }

        {
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            auto resData = CD3DX12_RESOURCE_DESC::Buffer(m_meshletsData[i].size() * sizeof(m_meshletsData[i][0]));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resData,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_meshletsBuffer[i].ReleaseAndGetAddressOf())));

            {
                D3D12_SUBRESOURCE_DATA initData = { m_meshletsData[i].data(), 0, 0 };

                ResourceUploadBatch resourceUpload(device);

                resourceUpload.Begin();

                resourceUpload.Upload(m_meshletsBuffer[i].Get(), 0, &initData, 1);

                resourceUpload.Transition(m_meshletsBuffer[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

                auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
                uploadResourcesFinished.wait();
            }

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = static_cast<UINT>(m_meshletsData[i].size());
            srvDesc.Buffer.StructureByteStride = sizeof(m_meshletsData[i][0]);

            device->CreateShaderResourceView(m_meshletsBuffer[i].Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::MeshletBuffer + i));
        }
    }
}

void Sample::BuildPSOs()
{
    auto device = m_deviceResources->GetD3DDevice();

    const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    const RenderTargetState rtStateVisibilityBuffer(DXGI_FORMAT_R32_UINT, m_deviceResources->GetDepthBufferFormat());

    // Setup PSO for Visibility Buffer Rasterization.
    {
        auto vertexShaderBlob = DX::ReadData(L"VisibilityVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"VisibilityPS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, pixelShaderBlob.data(), pixelShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_visibilityRootSignature.ReleaseAndGetAddressOf())));

        D3D12_SHADER_BYTECODE vertexShader = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

#if defined(_GAMING_DESKTOP) || defined(_GAMING_XBOX_SCARLETT)
        auto meshShaderBlob = DX::ReadData(L"VisibilityMS.cso");

        D3D12_SHADER_BYTECODE meshShader = { meshShaderBlob.data(), meshShaderBlob.size() };

        // Build Mesh Shader PSO
        {
            D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoMSDesc = {};
            psoMSDesc.pRootSignature = m_visibilityRootSignature.Get();
            psoMSDesc.MS = meshShader;
            psoMSDesc.PS = pixelShader;
            psoMSDesc.NumRenderTargets = 1;
            psoMSDesc.RTVFormats[0] = DXGI_FORMAT_R32_UINT;
            psoMSDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
            psoMSDesc.RasterizerState = CommonStates::CullClockwise;
            psoMSDesc.BlendState = CommonStates::Opaque;
            psoMSDesc.DepthStencilState = CommonStates::DepthDefault;
            psoMSDesc.SampleMask = UINT_MAX;
            psoMSDesc.SampleDesc = DefaultSampleDesc();

            auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoMSDesc);

            // Point to our populated stream desc
            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
            streamDesc.SizeInBytes = sizeof(meshStreamDesc);
            streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

            // Create the PSO using the stream desc CreatePipelineState API
            DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_visibilityMSPSO.ReleaseAndGetAddressOf())));
        }
#endif

        // Build Vertex Shader PSO
        {
            D3D12_INPUT_LAYOUT_DESC layoutDesc =
            {
                m_model[0]->vbDecl->data(),
                static_cast<UINT>(m_model[0]->vbDecl->size())
            };
            EffectPipelineStateDescription pdOpaque(
                &layoutDesc,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullClockwise,
                rtStateVisibilityBuffer);

            pdOpaque.CreatePipelineState(
                device,
                m_visibilityRootSignature.Get(),
                vertexShader,
                pixelShader,
                m_visibilityVSPSO.ReleaseAndGetAddressOf());
        }
    }

    // Setup PSO for Forward Renderer
    {
        D3D12_INPUT_LAYOUT_DESC layoutDesc =
        {
            m_model[0]->vbDecl->data(),
            static_cast<UINT>(m_model[0]->vbDecl->size())
        };
        EffectPipelineStateDescription pdOpaque(
            &layoutDesc,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullClockwise,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"RasterVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"RasterPS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rasterRootSignature.ReleaseAndGetAddressOf())));

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pdOpaque.CreatePipelineState(
            device,
            m_rasterRootSignature.Get(),
            vertexShader,
            pixelShader,
            m_rasterPSO.ReleaseAndGetAddressOf());
    }

    // Setup PSO for Visibility Buffer Compute Shading.
    {
        auto computeShaderBlob = DX::ReadData(L"SceneReconstructionCS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_computeRootSignature.ReleaseAndGetAddressOf())));

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_computeRootSignature.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_computePSO.ReleaseAndGetAddressOf())));
    }
}

void Sample::BuildObjectBuffer()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Setup instance data for dragon models.
    for (size_t i = 0; i < s_numDragons; i++)
    {
        m_objectData[i].vertexBufferID = 0;
        m_objectData[i].indexBufferID = 0;
        m_objectData[i].normalBufferID = 0;
        m_objectData[i].materialID = 0;
        m_objectData[i].textureIDs[0] = 0;
        m_objectData[i].textureIDs[1] = 1;
        m_objectData[i].textureIDs[2] = 2;
        m_objectData[i].textureIDs[3] = 3;
        m_objectData[i].textureIDs[4] = 4;
        m_objectData[i].textureIDs[5] = 5;

        static const XMVECTORF32 s_axis = { 1.f, 0.f, 1.f, 0.f };
        XMVECTOR rot = XMQuaternionRotationAxis(s_axis, 0);
        XMVECTORF32 trans = { -float(i % 5), 0.f, float(i / 5) - 5.f };
        XMMATRIX modelTransform = XMMatrixAffineTransformation(g_XMOne.v, g_XMZero.v, rot, trans.v);

        XMStoreFloat4x4(&m_objectData[i].modelTransform, modelTransform);
    }

    // Setup instance data for city model.
    m_objectData[s_numDragons] = m_objectData[0];
    m_objectData[s_numDragons].vertexBufferID = 1;
    m_objectData[s_numDragons].indexBufferID = 1;
    m_objectData[s_numDragons].materialID = 1;
    m_objectData[s_numDragons].textureIDs[0] = 1;

    static const XMVECTORF32 s_scaling = { 0.1f, 0.1f, 0.1f };
    static const XMVECTORF32 s_axis = { 1.f, 0.f, 0.f, 0.f };

    XMVECTOR rot = XMQuaternionRotationAxis(s_axis, XM_PIDIV2);
    XMMATRIX modelTransform = XMMatrixAffineTransformation(s_scaling.v, g_XMZero.v, rot, g_XMZero.v);

    XMStoreFloat4x4(&m_objectData[s_numDragons].modelTransform, modelTransform);

    // Create default heap for object buffer.
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto resData = CD3DX12_RESOURCE_DESC::Buffer(sizeof(m_objectData));
    DX::ThrowIfFailed(
        device->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resData,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_objectBuffer.ReleaseAndGetAddressOf())));

    // Upload object data into default heap.
    {
        D3D12_SUBRESOURCE_DATA initData = { m_objectData, 0, 0 };

        ResourceUploadBatch resourceUpload(device);

        resourceUpload.Begin();

        resourceUpload.Upload(m_objectBuffer.Get(), 0, &initData, 1);

        resourceUpload.Transition(m_objectBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        uploadResourcesFinished.wait();
    }

    // Create SRV into object buffer.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = sizeof(m_objectData) / sizeof(ObjectInfo);
    srvDesc.Buffer.StructureByteStride = sizeof(ObjectInfo);

    device->CreateShaderResourceView(m_objectBuffer.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::ObjectInfoBuffer));
}

void Sample::LoadTextures()
{
#ifdef _GAMING_DESKTOP
    const wchar_t* dragonTexture = L"ATGDragonPosedWithBakedDiffuse\\Dragon_diffuse.DD_";
    const wchar_t* cityTexture = L"AliasSampleCityBlock\\Concrete.DDS";
#else
    const wchar_t* dragonTexture = L"Dragon_diffuse.DD_";
    const wchar_t* cityTexture = L"Concrete.DDS";
#endif

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, dragonTexture);
    LoadTexture(strFilePath, m_texture[0], 0, true);

    DX::FindMediaFile(strFilePath, MAX_PATH, cityTexture);
    LoadTexture(strFilePath, m_texture[1], 1, false);
}

void Sample::BuildHUDObjects()
{
    auto device = m_deviceResources->GetD3DDevice();

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();
    
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::FontSmall),
        m_resourceDescriptors->GetGpuHandle(Descriptors::FontSmall));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    m_bigFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::FontBig),
        m_resourceDescriptors->GetGpuHandle(Descriptors::FontBig));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    m_help->RestoreDevice(device, resourceUpload, rtState);
    
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

void Sample::SplitMeshlets(const uint32_t* indexBufferData, uint32_t indexCount, std::vector<uint32_t>& outputUniqueIndices, std::vector<MeshletDesc>& outputMeshlets, std::vector<uint32_t>& outputPrimitiveIndices, uint32_t maxVertices, uint32_t maxPrimitives)
{
    MeshletDesc currentMeshlet = {};
    currentMeshlet.startUniqueVertex = 0;
    currentMeshlet.startPrimitiveIndex = 0;

    // Loop over index buffer, splitting into meshlets.
    for (uint32_t i = 0; i < indexCount; i++)
    {
        // Get number of primitives and vertices in current meshlet.
        uint32_t numPrimitives = static_cast<uint32_t>(outputPrimitiveIndices.size() / 3) - currentMeshlet.startPrimitiveIndex;
        uint32_t numVertices = static_cast<uint32_t>(outputUniqueIndices.size()) - currentMeshlet.startUniqueVertex;

        // Start new meshlet whenever we either hit the primitive limit, or might hit the vertex limit by adding another 3 vertices (the max for one primitive).
        bool atStartOfPrimitive = outputPrimitiveIndices.size() % 3 == 0;
        if (numPrimitives == maxPrimitives || (atStartOfPrimitive && numVertices + 3 > maxVertices))
        {
            currentMeshlet.numVertices = numVertices;
            currentMeshlet.numPrimitives = numPrimitives;

            outputMeshlets.push_back(currentMeshlet);

            currentMeshlet = {};
            currentMeshlet.startUniqueVertex = static_cast<uint32_t>(outputUniqueIndices.size());
            currentMeshlet.startPrimitiveIndex = static_cast<uint32_t>(outputPrimitiveIndices.size()) / 3;
        }

        uint32_t currentIndex = indexBufferData[i];

        int oldIndexLocation = -1;
        // Check if we already used this index in the current meshlet.
        // Could use hash table, but this will always be fairly small and likely a fast iteration through cache. (Worth verifying).
        for (uint32_t j = currentMeshlet.startUniqueVertex; j < outputUniqueIndices.size(); j++)
        {
            if (currentIndex == outputUniqueIndices[j])
            {
                oldIndexLocation = static_cast<int>(j);
                break;
            }
        }

        if (oldIndexLocation == -1)
        {
            // Since we hadn't used this index in the current meshlet, add it to the list of unique indices (unique within a meshlet, not the entire list).
            outputUniqueIndices.push_back(currentIndex);
            // Push the local offset to this unique index (within the meshlet) to the list of primitive indices for our meshlet.
            outputPrimitiveIndices.push_back(static_cast<uint32_t>(outputUniqueIndices.size()) - 1 - currentMeshlet.startUniqueVertex);
        }
        else
        {
            // Push the local offset to this unique index (within the meshlet) to the list of primitive indices for our meshlet.
            outputPrimitiveIndices.push_back(oldIndexLocation - currentMeshlet.startUniqueVertex);
        }
    }

    uint32_t numPrimitives = static_cast<uint32_t>(outputPrimitiveIndices.size() / 3) - currentMeshlet.startPrimitiveIndex;
    uint32_t numVertices = static_cast<uint32_t>(outputUniqueIndices.size()) - currentMeshlet.startUniqueVertex;

    currentMeshlet.numVertices = numVertices;
    currentMeshlet.numPrimitives = numPrimitives;

    outputMeshlets.push_back(currentMeshlet);
}

void Sample::LoadTexture(const wchar_t* path, ComPtr<ID3D12Resource>& texture, uint32_t textureIndex, bool compressed)
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();
    commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);

    ComPtr<ID3D12Resource> textureUploadHeap;
    std::unique_ptr<uint8_t[]> imageData;
    std::vector<uint8_t> uncompressedData;

    // load texture data.
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    if (compressed)
    {
        uncompressedData = DX::ReadCompressedData(path);
        DX::ThrowIfFailed(LoadDDSTextureFromMemory(
            device,
            uncompressedData.data(),
            uncompressedData.size(),
            texture.ReleaseAndGetAddressOf(),
            subresources));
    }
    else
    {
        DX::ThrowIfFailed(LoadDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
            path,
            texture.ReleaseAndGetAddressOf(),
            imageData,
            subresources));
    }

    auto subresourceSize = static_cast<const uint32_t>(subresources.size());
    const auto uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, subresourceSize);

    auto const heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    // create upload heap to transfer texture data to resource.
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(textureUploadHeap.GetAddressOf())));

    // transfer texture data.
    UpdateSubresources(commandList,
        texture.Get(),
        textureUploadHeap.Get(),
        0,
        0,
        subresourceSize,
        &subresources[0]);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    // generate SRV for texture resource.
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Format = texture->GetDesc().Format;
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(texture.Get(), &SRVDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::DragonTexture + textureIndex));

    DX::ThrowIfFailed(commandList->Close());
    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, CommandListCast(&commandList));

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const size   = m_deviceResources->GetOutputSize();
    auto const width = static_cast<UINT>(size.right);
    auto const height = static_cast<UINT>(size.bottom);

    m_visibilityBuffer->SetWindow(size);
    m_help->SetWindow(size);

    auto res = m_visibilityBuffer->GetResource();
    if (res)
        res->SetName(L"Visibility Buffer");

    m_camera.SetAspectRatio(width / static_cast<float>(height));

    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    // Setup intermediate render target, with views as UAV and SRV.
    {
        const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM,
            width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_renderTexture[0].ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_renderTexture[1].ReleaseAndGetAddressOf())));

        device->CreateUnorderedAccessView(m_renderTexture[0].Get(), nullptr, nullptr, m_resourceDescriptors->GetCpuHandle(Descriptors::OutputUAV));
        device->CreateUnorderedAccessView(m_renderTexture[1].Get(), nullptr, nullptr, m_resourceDescriptors->GetCpuHandle(Descriptors::OutputUAV2));

        device->CreateShaderResourceView(m_renderTexture[0].Get(), nullptr, m_resourceDescriptors->GetCpuHandle(Descriptors::OutputSRV));
        device->CreateShaderResourceView(m_renderTexture[1].Get(), nullptr, m_resourceDescriptors->GetCpuHandle(Descriptors::OutputSRV2));

        // Determine thread group size for compute shader.
        m_ThreadGroupX = static_cast<uint32_t>(texDesc.Width) / s_numShaderThreads;
        m_ThreadGroupY = texDesc.Height / s_numShaderThreads;
    }
}

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();
    m_spriteBatch.reset();
    m_smallFont.reset();
    m_bigFont.reset();
    m_ctrlFont.reset();
    m_resourceDescriptors.reset();
    m_samplerDescriptors.reset();
    m_renderDescriptors.reset();
    m_visibilityBuffer.reset();
    m_visibilityMSPSO.Reset();
    m_visibilityVSPSO.Reset();
    m_rasterPSO.Reset();
    m_visibilityRootSignature.Reset();
    m_rasterRootSignature.Reset();

    for (size_t i = 0; i < std::size(m_texture); i++)
    {
        m_texture[i].Reset();
    }

    m_objectBuffer.Reset();

    for (size_t i = 0; i < std::size(m_model); i++)
    {
        m_model[i].reset();
    }

    for (size_t i = 0; i < std::size(m_renderTexture); i++)
    {
        m_renderTexture[i].Reset();
    }

    m_computePSO.Reset();
    m_computeRootSignature.Reset();
    m_help->ReleaseDevice();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
