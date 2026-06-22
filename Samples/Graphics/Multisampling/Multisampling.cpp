//--------------------------------------------------------------------------------------
// Multisampling.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Multisampling.h"

namespace
{
    constexpr DXGI_FORMAT c_DXGIFormat = DXGI_FORMAT_R11G11B10_FLOAT;
    constexpr DXGI_FORMAT c_DXGIDepthFormat = DXGI_FORMAT_D32_FLOAT;
};

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_aliasingScene{},
    m_viewMode{},
    m_selectedSample{},
    m_cameraMode{},
    m_selectedLight{},
    m_countOfVertexDataFullscreen{},
    m_vertexBufferViewFullscreen{},
    m_descriptorCpuSampleDot{},
    m_descriptorCpuFragmentDot{},
    m_countOfVertexDataWheelOfFortune{},
    m_vertexBufferViewWheelOfFortune{},
    m_zoomRectDst{},
    m_zoomRectSrc{},
    m_activeTransforms{},
    m_zoom{},
    m_ambientColor{},
    m_descriptorGpuMeshSphere{},
    m_descriptorGpuMesh{},
    m_viewportIndicator{},
    m_activeScene{},
    m_activeDescriptorGpuMesh{},
    m_activeCameraSettings{},
    m_activeZoomSettings{},
    m_activeLightSettings{},
    m_backBufferWidth(0),
    m_backBufferHeight(0),
    m_numQualityLevels{},
    m_logFragments{},
    m_logSamples{},
    m_quality{},
    m_descriptorCpuTextureResolved{},
    m_descriptorGpuTextureResolved{},
    m_descriptorCpuUAVResolved{},
    m_descriptorGpuUAVResolved{},
    m_descriptorRTVResolved{},
    m_descriptorGpuSamplerPoint{},
    m_descriptorGpuSamplerLinear{},
    m_viewportZoom{},
    m_graphicsRootElement{},
    m_computeRootElement{},
    m_pipelineStateWheelOfFortuneMSAA{},
    m_pipelineStateMeshMSAA{},
    m_pipelineStateResolveGraphicsNative{},
    m_pipelineStateResolveComputeNative{},
    m_resolveUsingPixelShader{},
    m_resolveUsingUbershader{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_GeometryShaders);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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
    InitializeScenes();

    // The section of the screen to zoom to
    m_zoomRectDst.left = -0.95f;
    m_zoomRectDst.top = 0.20f;
    m_zoomRectDst.right = -0.20f;
    m_zoomRectDst.bottom = 0.95f;

    m_ambientColor = XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f);

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // View mode
    m_aliasingScene = ALIASING_SCENE_GOTHIC_WINDOW;
    m_activeScene = &g_scene[m_aliasingScene];
    m_activeDescriptorGpuMesh = m_descriptorGpuMesh[m_aliasingScene];
    m_activeCameraSettings = &m_activeScene->m_cameraSettings;
    m_activeZoomSettings = &m_activeScene->m_zoomSettings;
    m_activeLightSettings = m_activeScene->m_lightSettings;
    m_viewMode = VIEW_MODE_RESOLVED;
    m_selectedSample = 0;
    m_cameraMode = CONTROL_MODE_ZOOM;
    m_selectedLight = 0;

    // Multisampling options
    m_logFragments = 0;
    m_logSamples = 0;
    m_quality = 0;

    UpdateZoom();
    UpdateTransform();
    UpdateLight();
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
void Sample::Update(DX::StepTimer const& /* timer */)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        uint32_t iOriginalAliasingScene = m_aliasingScene;

        // Mode changes
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_viewMode = ++m_viewMode % VIEW_MODE_COUNT;
        }
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectedSample = ++m_selectedSample % (1 << m_logSamples);
        }
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_cameraMode = ++m_cameraMode % CONTROL_MODE_COUNT;
        }
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectedLight = ++m_selectedLight % Scene::c_numLights;
        }
        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            UpdateMSAA(m_logFragments + 1, m_quality);
        }
        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            UpdateMSAA(m_logFragments + c_maxLogFragments, m_quality);
        }
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            UpdateMSAA(m_logFragments, m_quality + 1);
        }
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            UpdateMSAA(m_logFragments, m_quality + m_numQualityLevels[m_logFragments] - 1);
        }
        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_aliasingScene = ++m_aliasingScene % ALIASING_SCENE_COUNT;
        }
        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_aliasingScene = (m_aliasingScene + ALIASING_SCENE_COUNT - 1) % ALIASING_SCENE_COUNT;
        }

        // Reset camera
        if (m_gamePadButtons.rightStick == GamePad::ButtonStateTracker::PRESSED)
        {
            *m_activeCameraSettings = m_activeScene->m_originalCameraSettings;
            UpdateTransform();
            *m_activeZoomSettings = m_activeScene->m_originalZoomSettings;
            UpdateZoom();
            m_activeLightSettings = m_activeScene->m_originalLightSettings;
            UpdateLight();
        }

        // Toggle resolve shader
        if (m_gamePadButtons.leftStick == GamePad::ButtonStateTracker::PRESSED)
        {
            // Toggle resolve method, if there are multiple methods
            if (m_quality > 0)
            {
                if (!m_resolveUsingPixelShader)
                {
                    m_resolveUsingUbershader = !m_resolveUsingUbershader;
                }
                m_resolveUsingPixelShader = !m_resolveUsingPixelShader;
            }
        }

        // Update variables if scene changed
        if (iOriginalAliasingScene != m_aliasingScene)
        {
            m_activeScene = &g_scene[m_aliasingScene];
            m_activeDescriptorGpuMesh = m_descriptorGpuMesh[m_aliasingScene];
            m_activeCameraSettings = &m_activeScene->m_cameraSettings;
            UpdateTransform();
            m_activeZoomSettings = &m_activeScene->m_zoomSettings;
            UpdateZoom();
            m_activeLightSettings = m_activeScene->m_lightSettings;
            UpdateLight();
        }

        // Process thumbstick and trigger analog input, depending on control mode
        switch (m_cameraMode)
        {
        case CONTROL_MODE_ZOOM:
        {
            // Change zoom
            m_activeZoomSettings->m_zoom *= (1.0f + 0.01f * m_activeZoomSettings->m_zoomSpeed * pad.triggers.left);
            m_activeZoomSettings->m_zoom *= (1.0f - 0.01f * m_activeZoomSettings->m_zoomSpeed * pad.triggers.right);
            m_activeZoomSettings->m_zoom = std::min(m_activeZoomSettings->m_zoom, 1.0f);
            m_activeZoomSettings->m_zoom = std::max(m_activeZoomSettings->m_zoom, 1.0f / std::min(m_backBufferHeight, m_backBufferWidth));  // one pixel

                                                                                                                                                      // Change offset
            m_activeZoomSettings->m_offsetX += 0.1f * m_activeZoomSettings->m_zoomSpeed * m_activeZoomSettings->m_zoom * pad.thumbSticks.rightX;
            m_activeZoomSettings->m_offsetX = std::min(m_activeZoomSettings->m_offsetX, 1.0f * (1.1f - m_activeZoomSettings->m_zoom));
            m_activeZoomSettings->m_offsetX = std::max(m_activeZoomSettings->m_offsetX, -1.0f * (1.1f - m_activeZoomSettings->m_zoom));
            m_activeZoomSettings->m_offsetY += 0.1f * m_activeZoomSettings->m_zoomSpeed * m_activeZoomSettings->m_zoom * pad.thumbSticks.rightY;
            m_activeZoomSettings->m_offsetY = std::min(m_activeZoomSettings->m_offsetY, 1.0f * (1.1f - m_activeZoomSettings->m_zoom));
            m_activeZoomSettings->m_offsetY = std::max(m_activeZoomSettings->m_offsetY, -1.0f * (1.1f - m_activeZoomSettings->m_zoom));

            UpdateZoom();
        }
        break;

        case CONTROL_MODE_CAMERA:
        {
            // Rotate the camera using the right stick
            XMMATRIX rotation =
                XMMatrixRotationAxis(m_activeCameraSettings->m_cameraRight, -pad.thumbSticks.rightY * m_activeCameraSettings->m_rotationSpeed * 0.01f * XM_PI)
                * XMMatrixRotationAxis(m_activeCameraSettings->m_cameraUp, pad.thumbSticks.rightX * m_activeCameraSettings->m_rotationSpeed * 0.01f * XM_PI);

            m_activeCameraSettings->m_cameraDirection =
                XMVector3Normalize(XMVector3Transform(m_activeCameraSettings->m_cameraDirection, rotation));
            m_activeCameraSettings->m_cameraRight =
                XMVector3Normalize(XMVector3Cross(m_activeCameraSettings->m_cameraUp, m_activeCameraSettings->m_cameraDirection));
            m_activeCameraSettings->m_cameraUp =
                XMVector3Normalize(XMVector3Cross(m_activeCameraSettings->m_cameraDirection, m_activeCameraSettings->m_cameraRight));

            // Move the camera using the left stick
            m_activeCameraSettings->m_cameraPosition = XMVectorAdd(m_activeCameraSettings->m_cameraPosition,
                XMVectorScale(m_activeCameraSettings->m_cameraRight, pad.thumbSticks.leftX * m_activeCameraSettings->m_translationSpeed * 0.05f));
            m_activeCameraSettings->m_cameraPosition = XMVectorAdd(m_activeCameraSettings->m_cameraPosition,
                XMVectorScale(m_activeCameraSettings->m_cameraUp, pad.thumbSticks.leftY * m_activeCameraSettings->m_translationSpeed * 0.05f));
            m_activeCameraSettings->m_cameraPosition = XMVectorAdd(m_activeCameraSettings->m_cameraPosition,
                XMVectorScale(m_activeCameraSettings->m_cameraDirection, pad.triggers.right * m_activeCameraSettings->m_translationSpeed * 0.05f));
            m_activeCameraSettings->m_cameraPosition = XMVectorSubtract(m_activeCameraSettings->m_cameraPosition,
                XMVectorScale(m_activeCameraSettings->m_cameraDirection, pad.triggers.left * m_activeCameraSettings->m_translationSpeed * 0.05f));

            UpdateTransform();
        }
        break;

        case CONTROL_MODE_LIGHT:
        {
            // Rotate the light using the right stick
            XMMATRIX rotation =
                XMMatrixRotationAxis(m_activeCameraSettings->m_cameraRight, pad.thumbSticks.rightY * 0.01f * XM_PI)
                * XMMatrixRotationAxis(m_activeCameraSettings->m_cameraUp, -pad.thumbSticks.rightX * 0.01f * XM_PI);

            m_activeLightSettings[m_selectedLight].m_worldDir =
                XMVector3Normalize(XMVector3Transform(m_activeLightSettings[m_selectedLight].m_worldDir, rotation));

            // Change the light intensity using the triggers
            m_activeLightSettings[m_selectedLight].m_color = XMVectorScale(m_activeLightSettings[m_selectedLight].m_color, 1.0f - 0.01f * pad.triggers.left);
            m_activeLightSettings[m_selectedLight].m_color = XMVectorScale(m_activeLightSettings[m_selectedLight].m_color, 1.0f + 0.01f * pad.triggers.right);

            UpdateLight();
        }
        break;

        default:
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
            assert(false);
#pragma warning(pop)
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: UpdateZoom()
// Desc: Update the zoom rectangle when it changes
//-------------------------------------------------------------------------------------------------------------
void Sample::UpdateZoom()
{
    // Generate m_ZoomRectSrc in texel units from m_pActiveCameraSettings->m_fZoom, m_pActiveCameraSettings->m_fOffsetX, and m_pActiveCameraSettings->m_fOffsetY
    m_zoomRectSrc.left = -m_activeZoomSettings->m_zoom + m_activeZoomSettings->m_offsetX + 0.5f;
    m_zoomRectSrc.right = m_activeZoomSettings->m_zoom + m_activeZoomSettings->m_offsetX + 0.5f;
    m_zoomRectSrc.top = m_activeZoomSettings->m_zoom - m_activeZoomSettings->m_offsetY + 0.5f;
    m_zoomRectSrc.bottom = -m_activeZoomSettings->m_zoom - m_activeZoomSettings->m_offsetY + 0.5f;

    // Set transform
    XMMATRIX translateSrc = XMMatrixTranslation(-2.0f * m_activeZoomSettings->m_offsetX, -2.0f * m_activeZoomSettings->m_offsetY, 0.0f);
    XMMATRIX scale = XMMatrixScaling(0.5f / m_activeZoomSettings->m_zoom, 0.5f / m_activeZoomSettings->m_zoom, 1.0f);
    m_zoom = translateSrc * scale;
}

//-------------------------------------------------------------------------------------------------------------
// Name: UpdateTransform()
// Desc: Update the camera transform when it changes
//-------------------------------------------------------------------------------------------------------------
void Sample::UpdateTransform()
{
    XMMATRIX world = XMMatrixIdentity();  // possible future expansion

    XMMATRIX view = XMMatrixLookAtLH(m_activeCameraSettings->m_cameraPosition,
        XMVectorAdd(m_activeCameraSettings->m_cameraPosition, m_activeCameraSettings->m_cameraDirection),
        m_activeCameraSettings->m_cameraUp);
    auto viewport = m_deviceResources->GetScreenViewport();
    XMMATRIX proj = XMMatrixPerspectiveFovLH(3.14156f / 4.f,
        viewport.Width / viewport.Height,
        m_activeCameraSettings->m_nearPlane,
        m_activeCameraSettings->m_farPlane);
    m_activeTransforms.Set(world, view, proj);
}

//-------------------------------------------------------------------------------------------------------------
// Name: UpdateLight()
// Desc: Update the light transform when it changes
//-------------------------------------------------------------------------------------------------------------
void Sample::UpdateLight()
{
    // Here we would update any scene parameters which depended on the light settings
    // Currently, there's nothing to do
}

void Sample::UpdateMSAA(uint32_t logFragments, uint32_t quality)
{
    m_logFragments = logFragments % (c_maxLogFragments + 1);
    m_quality = quality % m_numQualityLevels[m_logFragments];
#if ENABLE_EQAA
    ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
    m_logSamples = constantBufferFMask.LOG_NUM_SAMPLES;
#else
    m_iLogSamples = m_logFragments;
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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();

    auto descriptorBackbufferRenderTargetView = m_deviceResources->GetRenderTargetView();

    auto TransitionBarrier = [](ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, const D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
    {
        if (before != after)
        {
            D3D12_RESOURCE_BARRIER Barrier = {};
            Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            Barrier.Transition.pResource = resource;
            Barrier.Transition.Subresource = 0;
            Barrier.Transition.StateBefore = before;
            Barrier.Transition.StateAfter = after;
            commandList->ResourceBarrier(1, &Barrier);
        }
    };

    // Set root signature
    commandList->SetGraphicsRootSignature(m_rootSignatureGraphics.Get());
    commandList->SetComputeRootSignature(m_rootSignatureCompute.Get());

    // Set descriptor heaps
    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_descriptorHeapResource.GetHeap(),
        m_descriptorHeapSampler.GetHeap(),
    };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(descriptorHeaps)), descriptorHeaps);

    // Set render target
    commandList->ClearRenderTargetView(m_descriptorRTVs[m_logFragments][m_quality], Colors::CornflowerBlue, 0U, nullptr);
    commandList->ClearDepthStencilView(m_descriptorDSVs[m_logFragments][m_quality], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0U, 0U, nullptr);
    commandList->OMSetRenderTargets(1, &m_descriptorRTVs[m_logFragments][m_quality], TRUE, &m_descriptorDSVs[m_logFragments][m_quality]);

    // Render Scene
    commandList->SetPipelineState(((m_aliasingScene == ALIASING_SCENE_WHEEL_OF_FORTUNE) ? m_pipelineStateWheelOfFortuneMSAA : m_pipelineStateMeshMSAA)[m_logFragments][m_quality].Get());
    RenderScene(commandList, m_activeTransforms.m_worldViewProj, m_activeTransforms.m_world);

    // Resolve MSAA
    D3D12_RESOURCE_STATES renderTargetOldState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES renderTargetNewState;
    D3D12_RESOURCE_STATES textureResolvedOldState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_RESOURCE_STATES textureResolvedNewState;
#if ENABLE_EQAA
    if (m_quality > 0)
    {
        renderTargetNewState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        textureResolvedNewState = m_resolveUsingPixelShader ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        auto preserveFlags = D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_COLOR;
        // Bypass of fmask decompress on Scorpio
        if (IsScorpioClass())
        {
            preserveFlags |= D3D12XBOX_RESOURCE_STATE_PRESERVE_SCATTERED_COLOR_FMASK;
        }
        TransitionBarrier(commandList, m_renderTargets[m_logFragments][m_quality].Get(), renderTargetOldState, renderTargetNewState | preserveFlags);
        TransitionBarrier(commandList, m_textureResolved.Get(), textureResolvedOldState, textureResolvedNewState);
        RenderManualResolve(commandList);
    }
    else
#endif
    {
        if (m_logFragments > 0)
        {
            renderTargetNewState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            textureResolvedNewState = D3D12_RESOURCE_STATE_RESOLVE_DEST;
            TransitionBarrier(commandList, m_renderTargets[m_logFragments][m_quality].Get(), renderTargetOldState, renderTargetNewState);
            TransitionBarrier(commandList, m_textureResolved.Get(), textureResolvedOldState, textureResolvedNewState);
            commandList->ResolveSubresource(m_textureResolved.Get(), 0, m_renderTargets[m_logFragments][m_quality].Get(), 0, c_DXGIFormat);
        }
        else
        {
            renderTargetNewState = D3D12_RESOURCE_STATE_COPY_SOURCE;
            textureResolvedNewState = D3D12_RESOURCE_STATE_COPY_DEST;
            TransitionBarrier(commandList, m_renderTargets[m_logFragments][m_quality].Get(), renderTargetOldState, renderTargetNewState);
            TransitionBarrier(commandList, m_textureResolved.Get(), textureResolvedOldState, textureResolvedNewState);
            commandList->CopyResource(m_textureResolved.Get(), m_renderTargets[m_logFragments][m_quality].Get());
        }

        TransitionBarrier(commandList, m_renderTargets[m_logFragments][m_quality].Get(), renderTargetNewState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        renderTargetNewState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    TransitionBarrier(commandList, m_textureResolved.Get(), textureResolvedNewState, textureResolvedOldState);

    // Restore Render Target
    commandList->OMSetRenderTargets(1, &descriptorBackbufferRenderTargetView, TRUE, nullptr);

    // Draw resolved render target to screen
    RenderResolvedSurface(commandList);

    // Render light indicator(s)
    RenderLights(commandList);

    // Draw zoom pane
    RenderZoom(commandList, m_activeTransforms.m_worldViewProj);

    // Draw UI
    RenderUI(commandList);

    TransitionBarrier(commandList, m_renderTargets[m_logFragments][m_quality].Get(), renderTargetNewState, renderTargetOldState);

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

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Render the test geometry.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderScene(ID3D12GraphicsCommandList * const commandList, const XMMATRIX & worldViewProj, const XMMATRIX & world)
{
    ScopedPixEvent RenderScene(commandList, 0, L"RenderScene");

    // Set transform constant buffer
    ConstantBufferTransform constantBufferTransform =
    {
        worldViewProj,
        world,
    };
    auto descriptorConstantBufferTransformForMesh = m_constantBufferTransform.ReplaceContents(constantBufferTransform);

    auto tableCBVVS = m_descriptorRing.AllocateTable(1U, 14U);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransformForMesh, m_constantBufferTransform.GetSlot());
    m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);

    // Set lighting constant buffer
    ConstantBufferLight constantBufferLight =
    {
        m_ambientColor,
        m_activeTransforms.m_eyeWorldPos,
        m_activeScene->m_useNormalMap,
    };
    for (uint32_t iLight = 0; iLight < Scene::c_numLights; ++iLight)
    {
        constantBufferLight.m_lightData[iLight].m_lightDir = m_activeLightSettings[iLight].m_worldDir;
        constantBufferLight.m_lightData[iLight].m_lightColor = m_activeLightSettings[iLight].m_color;
        constantBufferLight.m_lightData[iLight].m_specularPower = m_activeLightSettings[iLight].m_specularPower;
    }
    auto descriptorConstantBufferLightForMesh = m_constantBufferLight.ReplaceContents(constantBufferLight);

    auto tableCBVPS = m_descriptorRing.AllocateTable(1U, 14U);
    m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferLightForMesh, m_constantBufferLight.GetSlot());
    m_descriptorRing.SetGraphicsTable(commandList, tableCBVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_CBV]);

    // Set sampler state
    commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SAMPLER], m_descriptorGpuSamplerLinear);

    switch (m_aliasingScene)
    {
    case ALIASING_SCENE_WHEEL_OF_FORTUNE:   // procedural render
    {
        // Set input assembler state
        commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewWheelOfFortune);

        // Draw geometry
        commandList->DrawInstanced(m_countOfVertexDataWheelOfFortune, 1, 0, 0);
    }
    break;
    default:    // all other scenes are static meshes, with traditional rendering
    {
        // Set descriptors
        commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV], m_activeDescriptorGpuMesh);

        // Draw geometry
        m_sceneMesh[m_aliasingScene]->DrawOpaque(commandList);
    }
    break;
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderLights()
// Desc: Render the light indicator(s), which are spheres that uses the same lighting as the scene .
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderLights(ID3D12GraphicsCommandList * const commandList)
{
    if (m_aliasingScene == ALIASING_SCENE_WHEEL_OF_FORTUNE)
    {
        return; // this scene is unlit
    }

    ScopedPixEvent RenderLights(commandList, 0, L"RenderLights");

    commandList->SetGraphicsRootSignature(m_rootSignatureGraphics.Get());

    // Set sampler state
    commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SAMPLER], m_descriptorGpuSamplerLinear);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineStateMesh.Get());

    // Set constant buffer
    ConstantBufferTransform constantBufferTransform =
    {
        Scene::m_transformsIndicator.m_worldViewProj,
        Scene::m_transformsIndicator.m_world,
    };
    auto descriptorConstantBufferTransformForMesh = m_constantBufferTransform.ReplaceContents(constantBufferTransform);

    auto tableCBVVS = m_descriptorRing.AllocateTable(1U, 14U);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransformForMesh, m_constantBufferTransform.GetSlot());
    m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);

    // Move the light to compensate for the difference in camera views between the main pane and the light indicator pane
    ConstantBufferLight constantBufferLight =
    {
        m_ambientColor,
        Scene::m_transformsIndicator.m_eyeWorldPos,
        false,
    };

    // Draw a separate indicator for each light
    for (uint32_t light = 0; light < Scene::c_numLights; ++light)
    {
        commandList->RSSetViewports(1, &m_viewportIndicator[light]);

        D3D12_RECT rect =
        {
            (LONG)m_viewportIndicator[light].TopLeftX,
            (LONG)m_viewportIndicator[light].TopLeftY,
            (LONG)(m_viewportIndicator[light].TopLeftX + m_viewportIndicator[light].Width + 1.0f),
            (LONG)(m_viewportIndicator[light].TopLeftY + m_viewportIndicator[light].Height + 1.0f),
        };
        commandList->RSSetScissorRects(1, &rect);

        // Zero out all lights ... then fill in only the current one
        ZeroMemory(&constantBufferLight.m_lightData, sizeof(constantBufferLight.m_lightData));

        // Move the light dir into the world space of the indicator mesh
        XMVECTOR lightIndicatorLightDir = XMVector3TransformNormal(m_activeLightSettings[light].m_worldDir, m_activeTransforms.m_view);
        lightIndicatorLightDir = XMVector3TransformNormal(lightIndicatorLightDir,
            Scene::m_transformsIndicator.m_viewInverse);
        constantBufferLight.m_lightData[light].m_lightDir = lightIndicatorLightDir;
        constantBufferLight.m_lightData[light].m_lightColor = m_activeLightSettings[light].m_color;
        constantBufferLight.m_lightData[light].m_specularPower = m_activeLightSettings[light].m_specularPower;
        auto descriptorConstantBufferLightForIndicators = m_constantBufferLight.ReplaceContents(constantBufferLight);

        auto tableCBVPS = m_descriptorRing.AllocateTable(1U, 14U);
        m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferLightForIndicators, m_constantBufferLight.GetSlot());
        m_descriptorRing.SetGraphicsTable(commandList, tableCBVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_CBV]);

        // Set descriptors
        commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV], m_descriptorGpuMeshSphere);

        // Draw geometry
        m_meshSphere->Draw(commandList);
    }

    // Restore viewport and scissor rect
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Draw frames
    for (uint32_t light = 0; light < Scene::c_numLights; ++light)
    {
        D3D12_RECT rct =
        {
            (LONG)m_viewportIndicator[light].TopLeftX,
            (LONG)m_viewportIndicator[light].TopLeftY,
            (LONG)(m_viewportIndicator[light].TopLeftX + m_viewportIndicator[light].Width),
            (LONG)(m_viewportIndicator[light].TopLeftY + m_viewportIndicator[light].Height),
        };
        if (light == m_selectedLight)
        {
            RenderFrame(commandList, rct, Colors::Yellow, Colors::Blue);
        }
        else
        {
            RenderFrame(commandList, rct);
        }
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderFrame()
// Desc: Render a rectangular frame in screen space
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderFrame(ID3D12GraphicsCommandList* commandList, const D3D12_RECT & rct, const XMVECTOR & color0, const XMVECTOR & color1)
{
    ScopedPixEvent RenderFrame(commandList, 0, L"RenderFrame");

    m_frameEffect->Apply(commandList);

    auto screenViewport = m_deviceResources->GetScreenViewport();
    auto proj = XMMatrixOrthographicOffCenterLH(screenViewport.TopLeftX, screenViewport.Width, screenViewport.Height, screenViewport.TopLeftY, 0.0f, 1.0f);
    m_frameEffect->SetProjection(proj);

    m_prim->Begin(commandList);

    auto vertA = XMVectorSet((float)rct.left, (float)rct.top, 0.0f, 0.0f);
    auto vertB = XMVectorSet((float)rct.left, (float)rct.bottom, 0.0f, 0.0f);
    auto vertC = XMVectorSet((float)rct.right, (float)rct.bottom, 0.0f, 0.0f);
    auto vertD = XMVectorSet((float)rct.right, (float)rct.top, 0.0f, 0.0f);
    DX::DrawQuad(m_prim.get(), vertA, vertB, vertC, vertD, color0);

    D3D12_RECT rctBorder =
    {
        rct.left - 1,
        rct.top - 1,
        rct.right + 1,
        rct.bottom + 1,
    };
    auto borderA = XMVectorSet((float)rctBorder.left, (float)rctBorder.top, 0.0f, 0.0f);
    auto borderB = XMVectorSet((float)rctBorder.left, (float)rctBorder.bottom, 0.0f, 0.0f);
    auto borderC = XMVectorSet((float)rctBorder.right, (float)rctBorder.bottom, 0.0f, 0.0f);
    auto borderD = XMVectorSet((float)rctBorder.right, (float)rctBorder.top, 0.0f, 0.0f);
    DX::DrawQuad(m_prim.get(), borderA, borderB, borderC, borderD, color1);

    m_prim->End();
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderZoom()
// Desc: Renders the zoomed-in portion of the MSAA target.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderZoom(ID3D12GraphicsCommandList * commandList, const XMMATRIX & worldViewProj)
{
    ScopedPixEvent RenderZoom(commandList, 0, L"RenderZoom");

    // Draw source frame
    D3D12_RECT rctSrc =
    {
        (LONG)(m_zoomRectSrc.left * m_backBufferWidth),
        (LONG)(m_zoomRectSrc.top * m_backBufferHeight),
        (LONG)(m_zoomRectSrc.right * m_backBufferWidth),
        (LONG)(m_zoomRectSrc.bottom * m_backBufferHeight),
    };
    RenderFrame(commandList, rctSrc);

    // Set viewport and scissor rect
    commandList->RSSetViewports(1, &m_viewportZoom);

    D3D12_RECT rect =
    {
        (LONG)m_viewportZoom.TopLeftX,
        (LONG)m_viewportZoom.TopLeftY,
        (LONG)(m_viewportZoom.TopLeftX + m_viewportZoom.Width + 1.0f),
        (LONG)(m_viewportZoom.TopLeftY + m_viewportZoom.Height + 1.0f),
    };
    commandList->RSSetScissorRects(1, &rect);

    RenderVisualization(commandList);

    // Render wireframe of the original mesh on top
    switch (m_viewMode)
    {
    case VIEW_MODE_SINGLE_SAMPLE:
    case VIEW_MODE_NEAREST_SAMPLE:
        RenderWireframe(commandList, worldViewProj * m_zoom);
        break;
    }

    // Render pixel grid
    switch (m_viewMode)
    {
    case VIEW_MODE_SINGLE_SAMPLE:
    case VIEW_MODE_NEAREST_SAMPLE:
    case VIEW_MODE_ALL_SAMPLES:
        RenderPixelGrid(commandList, m_zoom);
        break;
    };

    // Render sample locations
    switch (m_viewMode)
    {
    case VIEW_MODE_SINGLE_SAMPLE:
    case VIEW_MODE_NEAREST_SAMPLE:
        float fSampleDotSubPixelSize = 1.0f / 16.0f;
        XMMATRIX mSampleDotZoom = XMMatrixScaling(fSampleDotSubPixelSize / (m_activeZoomSettings->m_zoom * m_backBufferWidth),
            fSampleDotSubPixelSize / (m_activeZoomSettings->m_zoom * m_backBufferHeight),
            1.0f);
        RenderSampleDots(commandList, mSampleDotZoom);
        break;
    }

    // Restore viewport and scissor rect
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Draw dest frame
    D3D12_RECT rctDst =
    {
        (LONG)((m_zoomRectDst.left + 1.0f) / 2.0f * m_backBufferWidth),
        (LONG)((m_zoomRectDst.top + 1.0f) / 2.0f * m_backBufferHeight),
        (LONG)((m_zoomRectDst.right + 1.0f) / 2.0f * m_backBufferWidth),
        (LONG)((m_zoomRectDst.bottom + 1.0f) / 2.0f * m_backBufferHeight),
    };
    RenderFrame(commandList, rctDst);
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderVisualization()
// Desc: Render the visualization of the MSAA/EQAA render target as one of:
//	- Resolved
//	- Nearest sample
//	- Single sample
//	- All samples
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderVisualization(ID3D12GraphicsCommandList * commandList)
{
    ScopedPixEvent RenderVisualization(commandList, 0, L"RenderVisualization");

    commandList->SetGraphicsRootSignature(m_rootSignatureGraphics.Get());

    // Set up input layout
    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewFullscreen);

    auto tableCBVVS = m_descriptorRing.AllocateTable(4U, 14U);
    auto tableCBVPS = m_descriptorRing.AllocateTable(4U, 14U);
    auto tableSRVPS = m_descriptorRing.AllocateTable(2U, 128U);

    // The zoom transform
    ConstantBufferTransform constantBufferTransformZoom =
    {
        m_zoom
    };
    auto descriptorConstantBufferTransform = m_constantBufferTransform.ReplaceContents(constantBufferTransformZoom);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

    m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuMultisample[m_logFragments][m_quality], 0U);

#if ENABLE_EQAA
    // The FMask parameters
    ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
    auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());
    m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());

    ConstantBufferEQAA constantBufferEQAA = { g_EQAASamplePositions, };
    auto descriptorConstantBufferEQAA = m_constantBufferEQAA.ReplaceContents(constantBufferEQAA);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferEQAA, m_constantBufferEQAA.GetSlot());
    m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferEQAA, m_constantBufferEQAA.GetSlot());

    auto descriptorCpuSRVFMask = m_descriptorCpuFMaskNative[m_logFragments][m_quality];

    if (D3D12XboxGetCachedReadPointer(descriptorCpuSRVFMask))
    {
        m_descriptorRing.SetDescriptor(tableSRVPS, descriptorCpuSRVFMask, 1U);
    }
    else
    {
        // We must set some valid descriptor here to satisfy the validation layer
        m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuMultisample[m_logFragments][m_quality], 1U);
    }
#endif

    // The single sample parameter
    ConstantBufferViewMode constantBufferViewMode =
    {
        m_selectedSample
    };
    auto descriptorConstantBufferViewMode = m_constantBufferViewMode.ReplaceContents(constantBufferViewMode);
    m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferViewMode, m_constantBufferViewMode.GetSlot());

    // Set sampler state
    commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SAMPLER], m_descriptorGpuSamplerPoint);

    // Set things which depend on viewing mode
    switch (m_viewMode)
    {
    case VIEW_MODE_RESOLVED:
        // Just render the anti-aliased texture, magnified and point sampled
        commandList->SetPipelineState(m_pipelineStateTexture.Get());
        m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuTextureResolved, 0U);
        break;

    case VIEW_MODE_SINGLE_SAMPLE:
        // Render one sample for each pixel
        commandList->SetPipelineState(m_pipelineStateTextureSingleSample.Get());
        break;

    case VIEW_MODE_ALL_SAMPLES:
        // Render all the sample values for each pixel, in a rectangular grid
        commandList->SetPipelineState(m_pipelineStateTextureAllSamples.Get());
        break;

    case VIEW_MODE_NEAREST_SAMPLE:
        // Render the nearest sample value at each magnified sub-pixel location
        commandList->SetPipelineState(m_pipelineStateTextureNearestSample.Get());
        break;

    default:
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
        assert(false);
#pragma warning(pop)
        break;
    }

    m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableCBVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_CBV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableSRVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV]);

    // Draw
    commandList->DrawInstanced(m_countOfVertexDataFullscreen, 1, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderWireframe()
// Desc: Render the scene in wireframe mode.  This doesn't use depth testing, because we no longer
// have the depth buffer handy.  But it does do backface culling.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderWireframe(ID3D12GraphicsCommandList * commandList, const XMMATRIX & worldViewProj)
{
    ScopedPixEvent RenderWireframe(commandList, 0, L"RenderWireframe");

    // Wireframe of scene geometry
    commandList->SetPipelineState(m_pipelineStateWireframe.Get());
    RenderScene(commandList, worldViewProj, XMMatrixIdentity());
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderPixelGrid()
// Desc: Render lines along the borders between pixels.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderPixelGrid(ID3D12GraphicsCommandList * commandList, const XMMATRIX & transform)
{
    ScopedPixEvent RenderPixelGrid(commandList, 0, L"RenderPixelGrid");

    // Set up input layout
    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineStateGrid.Get());

    // Set SRVS
    auto tableSRVVS = m_descriptorRing.AllocateTable(1U, 128U);
    m_descriptorRing.SetDescriptor(tableSRVVS, m_descriptorCpuMultisample[m_logFragments][m_quality], 0U);
    m_descriptorRing.SetGraphicsTable(commandList, tableSRVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_SRV]);

    ConstantBufferTransform constantBufferTransformZoom =
    {
        transform,
        XMMatrixIdentity(),
    };
    auto descriptorConstantBufferTransform = m_constantBufferTransform.ReplaceContents(constantBufferTransformZoom);

    // horizontal lines
    {
        auto tableCBVVS = m_descriptorRing.AllocateTable(3U, 14U);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

#if ENABLE_EQAA
        // The FMask parameters
        ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
        auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());
#endif

        ConstantBufferGrid constantBufferGrid =
        {
            Colors::Orange, // XMVECTOR    m_vGridColor;
            true,           // bool        m_bHorizontal;
        };
        auto descriptorConstantBufferGrid = m_constantBufferGrid.ReplaceContents(constantBufferGrid);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferGrid, m_constantBufferGrid.GetSlot());

        m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);

        commandList->DrawInstanced(2, m_backBufferHeight + 1, 0, 0);
    }

    // vertical lines
    {
        auto tableCBVVS = m_descriptorRing.AllocateTable(3U, 14U);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

#if ENABLE_EQAA
        // The FMask parameters
        ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
        auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());
#endif

        ConstantBufferGrid constantBufferGrid =
        {
            Colors::Orange, // XMVECTOR    m_vGridColor;
            false,          // bool        m_bHorizontal;
        };
        auto descriptorConstantBufferGrid = m_constantBufferGrid.ReplaceContents(constantBufferGrid);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferGrid, m_constantBufferGrid.GetSlot());

        m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);

        commandList->DrawInstanced(2, m_backBufferWidth + 1, 0, 0);
    }
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderSampleDots()
// Desc: Render a point sprite at each (selected) sample location.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderSampleDots(ID3D12GraphicsCommandList * commandList, const XMMATRIX & transform)
{
    ScopedPixEvent RenderSampleDots(commandList, 0, L"RenderSampleDots");

    uint32_t numDrawnSamples, selectedSample;
    if (m_viewMode == VIEW_MODE_SINGLE_SAMPLE)
    {
        numDrawnSamples = 1;
        selectedSample = m_selectedSample;
    }
    else
    {
        numDrawnSamples = (1u << m_logSamples);
        selectedSample = 0xffffffff;
    }

    // Set up input layout
    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Set pipeline state
    commandList->SetPipelineState(m_pipelineStatePointSprite.Get());

    // Set sampler state
    commandList->SetGraphicsRootDescriptorTable(m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SAMPLER], m_descriptorGpuSamplerLinear);

    auto tableCBVVS = m_descriptorRing.AllocateTable(4U, 14U);
    auto tableCBVGS = m_descriptorRing.AllocateTable(1U, 14U);
    auto tableSRVVS = m_descriptorRing.AllocateTable(1U, 128U);
    auto tableSRVPS = m_descriptorRing.AllocateTable(3U, 128U);

    m_descriptorRing.SetDescriptor(tableSRVVS, m_descriptorCpuMultisample[m_logFragments][m_quality], 0U);

    // Set constant buffers
    ConstantBufferTransform constantBufferTransform =
    {
        m_zoom,
        XMMatrixIdentity(),
    };
    auto descriptorConstantBufferTransform = m_constantBufferTransform.ReplaceContents(constantBufferTransform);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

    ConstantBufferPointSprite constantBufferPointSprite =
    {
        selectedSample,
    };
    auto descriptorConstantBufferPointSprite = m_constantBufferPointSprite.ReplaceContents(constantBufferPointSprite);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferPointSprite, m_constantBufferPointSprite.GetSlot());

#if ENABLE_EQAA
    // The FMask parameters
    ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
    auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());

    ConstantBufferEQAA constantBufferEQAA = { g_EQAASamplePositions, };
    auto descriptorConstantBufferEQAA = m_constantBufferEQAA.ReplaceContents(constantBufferEQAA);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferEQAA, m_constantBufferEQAA.GetSlot());
#endif

    ConstantBufferPointSpriteTransform constantBufferPointSpriteTransform =
    {
        transform,				// XMMATRIX    m_positionTransform;
        XMMatrixIdentity(),		// XMMATRIX    m_texcoordTransform;
    };
    auto descriptorConstantBufferPointSpriteTransform = m_constantBufferPointSpriteTransform.ReplaceContents(constantBufferPointSpriteTransform);
    m_descriptorRing.SetDescriptor(tableCBVGS, descriptorConstantBufferPointSpriteTransform, m_constantBufferPointSpriteTransform.GetSlot());

    m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuFragmentDot, 1U);
    m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuSampleDot, 2U);

    m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableCBVGS, m_graphicsRootElement[SHADER_TYPE_GEOMETRY][DESCRIPTOR_TYPE_CBV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableSRVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_SRV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableSRVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV]);

    // One instance per pixel, one "vertex" per sample
    commandList->DrawInstanced(numDrawnSamples, m_backBufferWidth * m_backBufferHeight, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderResolvedSurface()
// Desc: Renders the result of resolving the MSAA surface.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderResolvedSurface(ID3D12GraphicsCommandList* commandList)
{
    ScopedPixEvent RenderResolvedSurface(commandList, 0, L"RenderResolvedSurface");

    // Draw resolved result to screen
    commandList->SetPipelineState(m_pipelineStateTexture.Get());

    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewFullscreen);

    auto tableCBVVS = m_descriptorRing.AllocateTable(1U, 14U);
    auto tableSRVPS = m_descriptorRing.AllocateTable(1U, 128U);

    ConstantBufferTransform constantBufferTransform =
    {
        XMMatrixIdentity(),
        XMMatrixIdentity(),
    };
    auto descriptorConstantBufferTransform = m_constantBufferTransform.ReplaceContents(constantBufferTransform);
    m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

    m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuTextureResolved, 0U);

    m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);
    m_descriptorRing.SetGraphicsTable(commandList, tableSRVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV]);

    commandList->DrawInstanced(m_countOfVertexDataFullscreen, 1, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------
// Name: RenderUI()
// Desc: Renders the UI.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    ScopedPixEvent RenderUI(commandList, 0, L"RenderUI");

    m_draw->Begin(commandList);

    auto const viewport = m_deviceResources->GetScreenViewport();
    m_draw->SetViewport(viewport);

    wchar_t buffer[1024] = {};

    {
        float x1 = 30.0f;
        float x2 = 120.0f;
        float y = 50.0f;
        float yInc = 25.0f;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[LB][RB]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Scene = %ws", m_activeScene->m_mame);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[Dpad]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Number of fragments = %d", 1 << m_logFragments);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[Dpad]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Quality level = %d", m_quality);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[A]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Mode = %ws", g_viewModeNames[m_viewMode]);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[B]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Selected sample = %d", m_selectedSample);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[Y]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Controller adjusts: %ws", g_controlModeNames[m_cameraMode]);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[X]", XMFLOAT2(x1, y), Colors::White);
        swprintf_s(buffer, L"Selected light = %d", m_selectedLight);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[RThumb]", XMFLOAT2(x1, y), Colors::White);
        m_font->DrawString(m_draw.get(), L"Reset Camera, Lights, Zoom", XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        DX::DrawControllerString(m_draw.get(), m_font.get(), m_controllerFont.get(), L"[LThumb]", XMFLOAT2(x1, y), Colors::White);
        if (m_quality > 0)
        {
            swprintf_s(buffer, L"Resolve = %s %s", m_resolveUsingPixelShader ? L"Pixel" : L"Compute", m_resolveUsingUbershader ? L"Ubershader" : L"Bespoke");
        }
        else if (m_logFragments > 0)
        {
            wcscpy_s(buffer, L"Resolve = ResolveSubresource");
        }
        else
        {
            wcscpy_s(buffer, L"Resolve = CopyResource");
        }
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x2, y), Colors::Aqua);
        y += yInc;
        y += yInc;
    }

    for (uint32_t light = 0; light < Scene::c_numLights; ++light)
    {
        if (m_aliasingScene == ALIASING_SCENE_WHEEL_OF_FORTUNE)
        {
            continue; // this scene is unlit
        }

        float x = m_viewportIndicator[light].TopLeftX;
        float y = m_viewportIndicator[light].TopLeftY;
        swprintf_s(buffer, L"Light = %d", light);
        m_font->DrawString(m_draw.get(), buffer, XMFLOAT2(x, y), (light == m_selectedLight) ? Colors::Yellow : Colors::DarkGray);
    }

    m_draw->End();
}

#if ENABLE_EQAA
//-------------------------------------------------------------------------------------------------------------
// Name: RenderManualResolve()
// Desc: Performs a resolve manually using pixel shader or compute shader.
//-------------------------------------------------------------------------------------------------------------
void Sample::RenderManualResolve(ID3D12GraphicsCommandList* commandList)
{
    ScopedPixEvent RenderManualResolve(commandList, 0, L"RenderManualResolve");

    // Perform the resolve manually
    if (m_resolveUsingPixelShader)
    {
        commandList->OMSetRenderTargets(1, &m_descriptorRTVResolved, TRUE, nullptr);

        // Set up input layout
        commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewFullscreen);

        auto tableCBVVS = m_descriptorRing.AllocateTable(1U, 14U);
        auto tableCBVPS = m_descriptorRing.AllocateTable(4U, 14U);
        auto tableSRVPS = m_descriptorRing.AllocateTable(2U, 128U);

        // Set constant buffers
        ConstantBufferTransform constantBufferTransform =
        {
            XMMatrixIdentity(),
            XMMatrixIdentity(),
        };
        auto descriptorConstantBufferTransform = m_constantBufferTransform.ReplaceContents(constantBufferTransform);
        m_descriptorRing.SetDescriptor(tableCBVVS, descriptorConstantBufferTransform, m_constantBufferTransform.GetSlot());

        if (m_resolveUsingUbershader)
        {
            // Dummy CBV to work around validation bug (CBV 0 is required if a higher index CBV is bound)
            ConstantBufferViewMode constantBufferViewMode =
            {
                m_selectedSample
            };
            auto descriptorConstantBufferViewMode = m_constantBufferViewMode.ReplaceContents(constantBufferViewMode);
            m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferViewMode, m_constantBufferViewMode.GetSlot());

            // The FMask parameters
            ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
            auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
            m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());

            ConstantBufferEQAA constantBufferEQAA = { g_EQAASamplePositions, };
            auto descriptorConstantBufferEQAA = m_constantBufferEQAA.ReplaceContents(constantBufferEQAA);
            m_descriptorRing.SetDescriptor(tableCBVPS, descriptorConstantBufferEQAA, m_constantBufferEQAA.GetSlot());

            commandList->SetPipelineState(m_pipelineStateResolveGraphicsUbershader.Get());
        }
        else
        {
            commandList->SetPipelineState(m_pipelineStateResolveGraphicsNative[m_logFragments][m_quality].Get());
        }

        auto descriptorCpuSRVFMask = m_descriptorCpuFMaskNative[m_logFragments][m_quality];
        m_descriptorRing.SetDescriptor(tableSRVPS, m_descriptorCpuMultisample[m_logFragments][m_quality], 0U);
        m_descriptorRing.SetDescriptor(tableSRVPS, descriptorCpuSRVFMask, 1U);

        m_descriptorRing.SetGraphicsTable(commandList, tableCBVVS, m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV]);
        m_descriptorRing.SetGraphicsTable(commandList, tableCBVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_CBV]);
        m_descriptorRing.SetGraphicsTable(commandList, tableSRVPS, m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV]);

        commandList->DrawInstanced(m_countOfVertexDataFullscreen, 1, 0, 0);
    }
    else
    {
        auto tableCBVCS = m_descriptorRing.AllocateTable(4U, 14U);
        auto tableSRVCS = m_descriptorRing.AllocateTable(2U, 128U);
        auto tableUAVCS = m_descriptorRing.AllocateTable(1U, 128U);

        if (m_resolveUsingUbershader)
        {
            // Dummy CBV to work around validation bug (CBV 0 is required if a higher index CBV is bound)
            ConstantBufferViewMode constantBufferViewMode =
            {
                m_selectedSample
            };
            auto descriptorConstantBufferViewMode = m_constantBufferViewMode.ReplaceContents(constantBufferViewMode);
            m_descriptorRing.SetDescriptor(tableCBVCS, descriptorConstantBufferViewMode, m_constantBufferViewMode.GetSlot());

            // The FMask parameters
            ConstantBufferFMask constantBufferFMask = CalcFMaskParams(m_logFragments, m_quality);
            auto descriptorConstantBufferFMask = m_constantBufferFMask.ReplaceContents(constantBufferFMask);
            m_descriptorRing.SetDescriptor(tableCBVCS, descriptorConstantBufferFMask, m_constantBufferFMask.GetSlot());

            ConstantBufferEQAA constantBufferEQAA = { g_EQAASamplePositions, };
            auto descriptorConstantBufferEQAA = m_constantBufferEQAA.ReplaceContents(constantBufferEQAA);
            m_descriptorRing.SetDescriptor(tableCBVCS, descriptorConstantBufferEQAA, m_constantBufferEQAA.GetSlot());

            commandList->SetPipelineState(m_pipelineStateResolveComputeUbershader.Get());
        }
        else
        {
            commandList->SetPipelineState(m_pipelineStateResolveComputeNative[m_logFragments][m_quality].Get());
        }

        auto descriptorCpuSRVFMask = m_descriptorCpuFMaskNative[m_logFragments][m_quality];
        m_descriptorRing.SetDescriptor(tableSRVCS, m_descriptorCpuMultisample[m_logFragments][m_quality], 0U);
        m_descriptorRing.SetDescriptor(tableSRVCS, descriptorCpuSRVFMask, 1U);

        m_descriptorRing.SetDescriptor(tableUAVCS, m_descriptorCpuUAVResolved, 0U);

        m_descriptorRing.SetComputeTable(commandList, tableCBVCS, m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_CBV]);
        m_descriptorRing.SetComputeTable(commandList, tableSRVCS, m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_SRV]);
        m_descriptorRing.SetComputeTable(commandList, tableUAVCS, m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_UAV]);

        commandList->Dispatch(m_backBufferWidth / 8, m_backBufferHeight / 8, 1);
    }
}
#endif
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

    device->GetGpuHardwareConfigurationX(&g_gpuHardwareConfiguration);

    auto commandQueue = m_deviceResources->GetCommandQueue();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    DX::ThrowIfFailed(InitializeHeaps(device));

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto framePSD = EffectPipelineStateDescription(
        &VertexPositionColor::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

    m_frameEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, framePSD);
    m_prim = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    auto fontDescriptorCpu = m_descriptorHeapResource.GetCurrentCpuHandle();
    auto fontDescriptorGpu = m_descriptorHeapResource.GetCurrentGpuHandle();
    m_descriptorHeapResource.AdvanceIndex();
    m_font = std::make_unique<SpriteFont>(device,
        resourceUpload,
        L"Courier_16.spritefont",
        fontDescriptorCpu,
        fontDescriptorGpu);

    auto controllerFontDescriptorCpu = m_descriptorHeapResource.GetCurrentCpuHandle();
    auto controllerFontDescriptorGpu = m_descriptorHeapResource.GetCurrentGpuHandle();
    m_descriptorHeapResource.AdvanceIndex();
    m_controllerFont = std::make_unique<SpriteFont>(device,
        resourceUpload,
        L"XboxOneControllerSmall.spritefont",
        controllerFontDescriptorCpu,
        controllerFontDescriptorGpu);

    RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
    m_draw = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

    // This object owns the textures, so we can't let it go out of scope.
    static EffectTextureFactory effectTextureFactory(device, resourceUpload, m_descriptorHeapResource.GetHeap());

    DX::ThrowIfFailed(InitializeSceneMeshes(device, resourceUpload, effectTextureFactory));

    m_meshSphere = Model::CreateFromSDKMESH(device, L"sphere.sdkmesh");
    m_meshSphere->LoadStaticBuffers(device, resourceUpload);

    m_descriptorGpuMeshSphere = m_descriptorHeapResource.GetCurrentGpuHandle();
    for (auto name : m_meshSphere->textureNames)
    {
        effectTextureFactory.CreateTexture(name.c_str(), int(m_descriptorHeapResource.GetCurrentIndex()));
        m_descriptorHeapResource.AdvanceIndex();
    }

    DX::ThrowIfFailed(InitializeRootSignatures(device));

    DX::ThrowIfFailed(InitializeConstantBuffers(device));

    DX::ThrowIfFailed(InitializeFullScreen(device));

    // The texture used for the sample location indicator
    m_descriptorCpuSampleDot = m_descriptorHeapResource.GetCurrentCpuHandle();
    effectTextureFactory.CreateTexture(L"SampleDot.dds", int(m_descriptorHeapResource.GetCurrentIndex()));
    m_descriptorHeapResource.AdvanceIndex();
    m_descriptorCpuFragmentDot = m_descriptorHeapResource.GetCurrentCpuHandle();
    effectTextureFactory.CreateTexture(L"FragmentDot.dds", int(m_descriptorHeapResource.GetCurrentIndex()));
    m_descriptorHeapResource.AdvanceIndex();

    DX::ThrowIfFailed(InitializeWheelOfFortune(device));

    DX::ThrowIfFailed(InitializeSamplers(device));

    auto uploadResourcesFinished = resourceUpload.End(commandQueue);
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto const outputSize = m_deviceResources->GetOutputSize();
    m_backBufferWidth = std::max<uint32_t>(static_cast<uint32_t>(outputSize.right - outputSize.left), 1u);
    m_backBufferHeight = std::max<uint32_t>(static_cast<uint32_t>(outputSize.bottom - outputSize.top), 1u);

    ZeroMemory(&m_viewportZoom, sizeof(m_viewportZoom));
    m_viewportZoom.TopLeftX = (m_zoomRectDst.left + 1.0f) / 2.0f * m_backBufferWidth;
    m_viewportZoom.TopLeftY = (m_zoomRectDst.top + 1.0f) / 2.0f * m_backBufferHeight;
    m_viewportZoom.Width = (m_zoomRectDst.right - m_zoomRectDst.left) / 2.0f * m_backBufferWidth;
    m_viewportZoom.Height = (m_zoomRectDst.bottom - m_zoomRectDst.top) / 2.0f * m_backBufferHeight;
    m_viewportZoom.MinDepth = 0.0f;
    m_viewportZoom.MaxDepth = 1.0f;

    DX::ThrowIfFailed(InitializeLightIndicator(device));

    DX::ThrowIfFailed(InitializeMultisampling(device));

    DX::ThrowIfFailed(InitializePipelineStates(device));

    // Use the rest of the resource descriptors for a ring buffer
    // This must come after all other descriptors have been initialized
    auto ringStart = m_descriptorHeapResource.GetCurrentIndex();
    auto ringSize = m_descriptorHeapResource.GetMax() - ringStart;
    m_descriptorRing.Initialize(device, m_descriptorHeapResource.GetHeap(), ringStart, ringSize);
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeHeaps()
// Desc: Initialize any required heaps.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeHeaps(ID3D12Device* const device)
{
    m_descriptorHeapUpload.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false);
    m_descriptorHeapResource.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
    m_descriptorHeapSampler.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
    m_descriptorHeapRenderTarget.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
    m_descriptorHeapDepthStencil.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeSceneMeshes()
// Desc: Load all meshes
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeSceneMeshes(ID3D12Device* const device, ResourceUploadBatch& resourceUpload, EffectTextureFactory& effectTextureFactory)
{
    // Scene initialization
    for (uint32_t scene = 0; scene < ALIASING_SCENE_COUNT; ++scene)
    {
        switch (scene)
        {
        case ALIASING_SCENE_WHEEL_OF_FORTUNE:
            // Procedural: doesn't have a mesh file
            break;

        default:
        {
            wchar_t filename[_MAX_PATH];
            _snwprintf_s(filename,
                _TRUNCATE,
                L"%s.sdkmesh",
                g_scene[scene].m_meshName);

            m_sceneMesh[scene] = Model::CreateFromSDKMESH(device, filename);
            m_sceneMesh[scene]->LoadStaticBuffers(device, resourceUpload);

            m_descriptorGpuMesh[scene] = m_descriptorHeapResource.GetCurrentGpuHandle();
            for (auto name : m_sceneMesh[scene]->textureNames)
            {
                effectTextureFactory.CreateTexture(name.c_str(), int(m_descriptorHeapResource.GetCurrentIndex()));
                m_descriptorHeapResource.AdvanceIndex();
            }
        }
        break;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeLightIndicator()
// Desc: Load the data for the indicator pane to describe the light parameters.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeLightIndicator(ID3D12Device* const /* device */)
{
    // The hard-coded camera which looks directly at the outside of the sphere (really a hemisphere),
    // and which more or less fills the screen with it.
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 200.0f, 1.0f),
        XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
    auto viewport = m_deviceResources->GetScreenViewport();
    XMMATRIX proj = XMMatrixPerspectiveFovLH(3.14156f / 4.f,
        viewport.Width / viewport.Height,
        5.0f,
        10000.0f);
    Scene::m_transformsIndicator.Set(world, view, proj);

    // Screenspace location of indicator pane(s)
    float indicatorX = 0.025f;
    float indicatorY = 0.30f;
    float indicatorWidth = 0.1f;
    float indicatorHeight = 0.1f;
    float indicatorSpacing = 0.01f;
    for (uint32_t iLight = 0; iLight < Scene::c_numLights; ++iLight)
    {
        ZeroMemory(&m_viewportIndicator[iLight], sizeof(m_viewportIndicator[iLight]));
        m_viewportIndicator[iLight].TopLeftX = indicatorX * m_backBufferWidth;
        m_viewportIndicator[iLight].TopLeftY = indicatorY * m_backBufferHeight;
        m_viewportIndicator[iLight].Width = indicatorWidth * m_backBufferWidth;
        m_viewportIndicator[iLight].Height = indicatorHeight * m_backBufferHeight;
        m_viewportIndicator[iLight].MinDepth = 0.0f;
        m_viewportIndicator[iLight].MaxDepth = 1.0f;

        indicatorY += indicatorHeight + indicatorSpacing;
    }

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeConstantBuffers()
// Desc: Create all constant buffers used by the sample.  Each has an associated struct.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeConstantBuffers(ID3D12Device* const device)
{
    // Predict number of copies of each CBV in flight as: N * m_maxPendingFrames
    // This is fragile, because you need to count the number of "ReplaceContents" operations per frame
    auto nextIndex = m_descriptorHeapUpload.GetCurrentIndex();
    nextIndex = m_constantBufferViewMode.Initialize(device, nextIndex, 2U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferTransform.Initialize(device, nextIndex, 7U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferLight.Initialize(device, nextIndex, (2U + Scene::c_numLights) * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferGrid.Initialize(device, nextIndex, 2U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferPointSprite.Initialize(device, nextIndex, 2U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferPointSpriteTransform.Initialize(device, nextIndex, 1U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
#if ENABLE_EQAA
    nextIndex = m_constantBufferFMask.Initialize(device, nextIndex, 4U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
    nextIndex = m_constantBufferEQAA.Initialize(device, nextIndex, 3U * m_maxPendingFrames, m_descriptorHeapUpload.GetHeap());
#endif
    m_descriptorHeapResource.SetNextIndex(nextIndex);

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeFullScreen()
// Desc: Prepare resources for fullscreen render.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeFullScreen(ID3D12Device* const device)
{
    VertexDataFullscreen* initialData = nullptr;
    m_countOfVertexDataFullscreen = _countof(*initialData);

    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(*initialData)                                 // UINT64 width,
                                                                // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                // UINT64 alignment = 0 )
    );
    DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_bufferFullscreen.ReleaseAndGetAddressOf())));
    m_bufferFullscreen->SetName(L"m_spBufferFullscreen");

    D3D12_RANGE readRange =
    {
        0UL,                                                    // SIZE_T Begin;
        0UL,                                                    // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    DX::ThrowIfFailed(m_bufferFullscreen->Map(0, &readRange, reinterpret_cast<void**>(&initialData)));

    (*initialData)[0].position = XMVectorSet(-1.0f, 1.0f, 0.5f, 1.0f);
    (*initialData)[1].position = XMVectorSet(1.0f, -1.0f, 0.5f, 1.0f);
    (*initialData)[2].position = XMVectorSet(-1.0f, -1.0f, 0.5f, 1.0f);
    (*initialData)[3].position = XMVectorSet(-1.0f, 1.0f, 0.5f, 1.0f);
    (*initialData)[4].position = XMVectorSet(1.0f, 1.0f, 0.5f, 1.0f);
    (*initialData)[5].position = XMVectorSet(1.0f, -1.0f, 0.5f, 1.0f);

    (*initialData)[0].texcoord = XMVectorSet(0.0f, 0.0f, 0.5f, 1.0f);
    (*initialData)[1].texcoord = XMVectorSet(1.0f, 1.0f, 0.5f, 1.0f);
    (*initialData)[2].texcoord = XMVectorSet(0.0f, 1.0f, 0.5f, 1.0f);
    (*initialData)[3].texcoord = XMVectorSet(0.0f, 0.0f, 0.5f, 1.0f);
    (*initialData)[4].texcoord = XMVectorSet(1.0f, 0.0f, 0.5f, 1.0f);
    (*initialData)[5].texcoord = XMVectorSet(1.0f, 1.0f, 0.5f, 1.0f);

    D3D12_RANGE writtenRange =
    {
        0UL,                                                    // SIZE_T Begin;
        sizeof(*initialData),                                // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    m_bufferFullscreen->Unmap(0, &writtenRange);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {
        m_bufferFullscreen->GetGPUVirtualAddress(),           // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
        sizeof(*initialData),                                // uint32_t SizeInBytes;
        sizeof((*initialData)[0]),                           // uint32_t StrideInBytes;
    };
    m_vertexBufferViewFullscreen = vertexBufferView;

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeWheelOfFortune()
// Desc: Prepare resources for the procedural "wheel of fortune" scene.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeWheelOfFortune(ID3D12Device* const device)
{
    VertexDataWheelOfFortune* initialData = nullptr;
    m_countOfVertexDataWheelOfFortune = _countof(*initialData);

    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(*initialData)                                 // UINT64 width,
                                                                // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                // UINT64 alignment = 0 )
    );
    DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_bufferWheelOfFortune.ReleaseAndGetAddressOf())));
    m_bufferWheelOfFortune->SetName(L"m_spBufferWheelOfFortune");

    D3D12_RANGE readRange =
    {
        0UL,                                                    // SIZE_T Begin;
        0UL,                                                    // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    DX::ThrowIfFailed(m_bufferWheelOfFortune->Map(0, &readRange, reinterpret_cast<void**>(&initialData)));

    // Create vertex buffer containing an aliased pattern
    XMVECTOR center = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR xAxis = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR wedgeColors[] =
    {
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
        XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
        XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
        XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f),
        XMVectorSet(1.0f, 0.0f, 1.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 1.0f, 1.0f),
        XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f),
    };
    for (uint32_t wedge = 0; wedge < c_numWedges; ++wedge)
    {
        VertexColor* vA = &(*initialData)[3 * wedge + 0];
        VertexColor* vB = &(*initialData)[3 * wedge + 1];
        VertexColor* vC = &(*initialData)[3 * wedge + 2];
        vA->position = center;
        vB->position = XMVector3Transform(xAxis, XMMatrixRotationZ(XM_2PI * wedge / (float)c_numWedges));
        vC->position = XMVector3Transform(xAxis, XMMatrixRotationZ(XM_2PI * (wedge + 1) / (float)c_numWedges));
        vA->color =
            vB->color =
            vC->color = wedgeColors[wedge % _countof(wedgeColors)];
    }

    D3D12_RANGE writtenRange =
    {
        0UL,                                                    // SIZE_T Begin;
        sizeof(*initialData),                                // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    m_bufferWheelOfFortune->Unmap(0, &writtenRange);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {
        m_bufferWheelOfFortune->GetGPUVirtualAddress(),       // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
        sizeof((*initialData)),                              // uint32_t SizeInBytes;
        sizeof((*initialData)[0]),                         // uint32_t StrideInBytes;
    };
    m_vertexBufferViewWheelOfFortune = vertexBufferView;

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeSamplers()
// Desc: Create the canned sampler states used by the sample.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeSamplers(ID3D12Device* const device)
{
    auto descriptorCpuSamplerPoint = m_descriptorHeapSampler.GetCurrentCpuHandle();
    m_descriptorGpuSamplerPoint = m_descriptorHeapSampler.GetCurrentGpuHandle();
    m_descriptorHeapSampler.AdvanceIndex();

    D3D12_SAMPLER_DESC samplerDescPoint =
    {
        D3D12_FILTER_MIN_MAG_MIP_POINT,     // D3D12_FILTER Filter;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressU;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressV;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressW;
        0.0f,                               // float MipLODBias;
        1,                                  // uint32_t MaxAnisotropy;
        D3D12_COMPARISON_FUNC_ALWAYS,       // D3D12_COMPARISON_FUNC ComparisonFunc;
    { 0.0f, 0.0f, 0.0f, 1.0f, },        // float BorderColor[ 4 ];
    0.0f,                               // float MinLOD;
    0.0f,                               // float MaxLOD;
    };
    device->CreateSampler(&samplerDescPoint, descriptorCpuSamplerPoint);

    auto descriptorCpuSamplerLinear = m_descriptorHeapSampler.GetCurrentCpuHandle();
    m_descriptorGpuSamplerLinear = m_descriptorHeapSampler.GetCurrentGpuHandle();
    m_descriptorHeapSampler.AdvanceIndex();

    D3D12_SAMPLER_DESC samplerDescLinear =
    {
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // D3D12_FILTER Filter;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressU;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressV;
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // D3D12_TEXTURE_ADDRESS_MODE AddressW;
        0.0f,                               // float MipLODBias;
        1,                                  // uint32_t MaxAnisotropy;
        D3D12_COMPARISON_FUNC_ALWAYS,       // D3D12_COMPARISON_FUNC ComparisonFunc;
    { 0.0f, 0.0f, 0.0f, 1.0f, },        // float BorderColor[ 4 ];
    0.0f,                               // float MinLOD;
    0.0f,                               // float MaxLOD;
    };
    device->CreateSampler(&samplerDescLinear, descriptorCpuSamplerLinear);

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeMultisampling()
// Desc: Create the resources used for all multisampling modes.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeMultisampling(ID3D12Device* const device)
{
    wchar_t D3D12ObjectName[256] = L"";

    // Figure out multisampling support
    for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments + 1; ++logFragments)    // Go one past the end, just to be sure we see 0
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS featureSupportData =
        {
            c_DXGIFormat,                                   // _In_  DXGI_FORMAT Format;
            1U << logFragments,                             // _In_  uint32_t SampleCount;
            D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,     // _In_  D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG Flags;
            0u,												// _Out_  uint32_t NumQualityLevels;
        };
        DX::ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &featureSupportData, sizeof(featureSupportData)));
        m_numQualityLevels[logFragments] = featureSupportData.NumQualityLevels;
    }
    assert(0 == m_numQualityLevels[c_maxLogFragments + 1]);

    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    const float black[] = { 0.0f, 0.0f, 0.0f, 1.0f, };
    const CD3DX12_CLEAR_VALUE clearColor(c_DXGIFormat, black);
    const CD3DX12_CLEAR_VALUE clearDepth(c_DXGIDepthFormat, 1.0f, 0U);

    // For each number of samples
    for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
    {
        m_renderTargets[logFragments].resize(m_numQualityLevels[logFragments]);
        m_descriptorRTVs[logFragments].resize(m_numQualityLevels[logFragments]);
        m_descriptorCpuMultisample[logFragments].resize(m_numQualityLevels[logFragments]);
        m_descriptorGpuMultisample[logFragments].resize(m_numQualityLevels[logFragments]);
#if ENABLE_EQAA
        m_descriptorCpuFMaskNative[logFragments].resize(m_numQualityLevels[logFragments]);
        m_descriptorGpuFMaskNative[logFragments].resize(m_numQualityLevels[logFragments]);
#endif
        m_depthStencils[logFragments].resize(m_numQualityLevels[logFragments]);
        m_descriptorDSVs[logFragments].resize(m_numQualityLevels[logFragments]);

        // For each "Quality" setting exposed by the driver and hardware
        for (uint32_t quality = 0; quality < m_numQualityLevels[logFragments]; ++quality)
        {
#if ENABLE_EQAA
            bool colorExpand = (quality == 0);
#endif

            // The underlying render target resource
            auto colorTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                c_DXGIFormat,
                m_backBufferWidth,
                m_backBufferHeight,
                1U,
                1U,
                1U << logFragments,
                quality,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            // Causes Fmask to auto-expand after more than one unique color, allowing bypass of fmask decompress
            if (quality > 0 && IsScorpioClass())
            {
                colorTextureDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
            }
            // Signal that both DCC color and Fmask are texture-compatible (can skip decompress)
            if (!colorExpand && IsScarlettClass())
            {
                colorTextureDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
            }
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &colorTextureDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clearColor,
                IID_GRAPHICS_PPV_ARGS(m_renderTargets[logFragments][quality].ReleaseAndGetAddressOf())));
            swprintf_s(D3D12ObjectName, L"m_spRenderTargets[%d][%d]", logFragments, quality);
            m_renderTargets[logFragments][quality]->SetName(D3D12ObjectName);

            // The underlying depth buffer resource
            auto depthTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                c_DXGIDepthFormat,
                m_backBufferWidth,
                m_backBufferHeight,
                1U,
                1U,
                1U << logFragments,
                quality,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthTextureDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearDepth,
                IID_GRAPHICS_PPV_ARGS(m_depthStencils[logFragments][quality].ReleaseAndGetAddressOf())));
            swprintf_s(D3D12ObjectName, L"m_spDepthStencilTargets[%d][%d]", logFragments, quality);
            m_depthStencils[logFragments][quality]->SetName(D3D12ObjectName);

            // The views into these resources
            D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc =
            {
                c_DXGIFormat,                                    // DXGI_FORMAT Format;
                D3D12_RTV_DIMENSION_TEXTURE2DMS,                // D3D12_RTV_DIMENSION ViewDimension;
            {},                                                // D3D12_TEX2DMS_RTV Texture2DMS;
            };
            m_descriptorRTVs[logFragments][quality] = m_descriptorHeapRenderTarget.GetCurrentCpuHandle();
            m_descriptorHeapRenderTarget.AdvanceIndex();
            device->CreateRenderTargetView(m_renderTargets[logFragments][quality].Get(),
                &renderTargetViewDesc,
                m_descriptorRTVs[logFragments][quality]);

#if ENABLE_EQAA
            if (!colorExpand)  // Only need an FMask view if we specified D3D12X_RESOURCE_MISC_NO_COLOR_EXPAND
            {
                auto fmaskNativeFormat = CalcFMaskNativeFormat(logFragments, quality);
                D3D12XBOX_SHADER_RESOURCE_VIEW_DESC fmaskNativeViewDesc =
                {
                    DXGI_FORMAT_UNKNOWN,                        // DXGI_FORMAT Format;
                    D3D12_SRV_DIMENSION_TEXTURE2D,                // D3D12_SRV_DIMENSION ViewDimension;
                    fmaskNativeFormat.Shader4ComponentMapping,  // uint32_t Shader4ComponentMapping;
                {
                },                                            // D3D12_TEX2D_SRV Texture2D;
                D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN,          // D3D12_GPU_VIRTUAL_ADDRESS ResourceLocation;
#ifdef _GAMING_XBOX_SCARLETT
                fmaskNativeFormat.ImageFormat,              // D3D12XBOX_DATA_FORMAT ImageFormat;
#else
                fmaskNativeFormat.DataFormat,               // D3D12XBOX_DATA_FORMAT DataFormat;
                fmaskNativeFormat.NumberFormat,             // D3D12XBOX_NUMBER_FORMAT NumberFormat;
#endif
                0u,                                         // uint32_t MemoryType;
                0.f,										// float TextureWarnLevelOfDetail;
                0,											// INT TexturePerfModulation;
                };
                D3D12_TEX2D_SRV fmaskTex2DSRV =
                {
                    0,                                      // uint32_t MostDetailedMip;
                    1,                                      // uint32_t MipLevels;
                    0u,										// uint32_t PlaneSlice;
                    0.f,									// float ResourceMinLODClamp;
                };
                fmaskNativeViewDesc.Texture2D = fmaskTex2DSRV;
                m_descriptorCpuFMaskNative[logFragments][quality] = m_descriptorHeapResource.GetCurrentCpuHandle();
                m_descriptorGpuFMaskNative[logFragments][quality] = m_descriptorHeapResource.GetCurrentGpuHandle();
                m_descriptorHeapResource.AdvanceIndex();
                device->CreatePlacedShaderResourceViewX(m_renderTargets[logFragments][quality].Get(),
                    &fmaskNativeViewDesc,
                    m_descriptorCpuFMaskNative[logFragments][quality]);
            }
            else
            {
                m_descriptorCpuFMaskNative[logFragments][quality].ptr = 0UL;
                m_descriptorGpuFMaskNative[logFragments][quality].ptr = 0UL;
            }
#endif

            D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc =
            {
                c_DXGIFormat,                                    // DXGI_FORMAT Format;
                D3D12_SRV_DIMENSION_TEXTURE2DMS,                // D3D12_SRV_DIMENSION ViewDimension;
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,       // uint32_t Shader4ComponentMapping;
            {
            },                                                // D3D12_TEX2DMS_SRV Texture2D;
            };
            m_descriptorCpuMultisample[logFragments][quality] = m_descriptorHeapResource.GetCurrentCpuHandle();
            m_descriptorGpuMultisample[logFragments][quality] = m_descriptorHeapResource.GetCurrentGpuHandle();
            m_descriptorHeapResource.AdvanceIndex();
            device->CreateShaderResourceView(m_renderTargets[logFragments][quality].Get(),
                &shaderResourceViewDesc,
                m_descriptorCpuMultisample[logFragments][quality]);

            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc =
            {
                c_DXGIDepthFormat,                                // DXGI_FORMAT Format;
                D3D12_DSV_DIMENSION_TEXTURE2DMS,                // D3D12_RTV_DIMENSION ViewDimension;
                D3D12_DSV_FLAG_NONE,                            // D3D12_DSV_FLAGS Flags;
            {},                                                // D3D12_TEX2DMS_DSV Texture2DMS;
            };
            m_descriptorDSVs[logFragments][quality] = m_descriptorHeapDepthStencil.GetCurrentCpuHandle();
            m_descriptorHeapDepthStencil.AdvanceIndex();
            device->CreateDepthStencilView(m_depthStencils[logFragments][quality].Get(),
                &depthStencilViewDesc,
                m_descriptorDSVs[logFragments][quality]);
        }
    }

    // The post-resolve anti-aliasing results
    auto resolvedTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        c_DXGIFormat,
        m_backBufferWidth,
        m_backBufferHeight,
        1U,
        1U,
        1U,
        0U,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resolvedTextureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearColor,
        IID_GRAPHICS_PPV_ARGS(m_textureResolved.ReleaseAndGetAddressOf())));
    m_textureResolved->SetName(L"m_spTextureResolved");

    D3D12_TEX2D_SRV resolvedTex2DSRV =
    {
        0,                                      // uint32_t MostDetailedMip;
        1,                                      // uint32_t MipLevels;
        0u,										// uint32_t PlaneSlice;
        0.f,									// float ResourceMinLODClamp;
    };
    D3D12_SHADER_RESOURCE_VIEW_DESC ResolvedShaderResourceViewDesc =
    {
        c_DXGIFormat,                                // DXGI_FORMAT Format;
        D3D12_SRV_DIMENSION_TEXTURE2D,                // D3D12_SRV_DIMENSION ViewDimension;
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,   // uint32_t Shader4ComponentMapping;
    {
    },                                            // D3D12_TEX2D_SRV Texture2D;
    };
    ResolvedShaderResourceViewDesc.Texture2D = resolvedTex2DSRV;
    m_descriptorCpuTextureResolved = m_descriptorHeapResource.GetCurrentCpuHandle();
    m_descriptorGpuTextureResolved = m_descriptorHeapResource.GetCurrentGpuHandle();
    m_descriptorHeapResource.AdvanceIndex();
    device->CreateShaderResourceView(m_textureResolved.Get(), &ResolvedShaderResourceViewDesc, m_descriptorCpuTextureResolved);

#if ENABLE_EQAA
    D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc =
    {
        c_DXGIFormat,                         // DXGI_FORMAT Format;
        D3D12_UAV_DIMENSION_TEXTURE2D,        // D3D12_UAV_DIMENSION ViewDimension;
        {}                                    // D3D12_TEX2D_UAV Texture2D;
    };
    m_descriptorCpuUAVResolved = m_descriptorHeapResource.GetCurrentCpuHandle();
    m_descriptorGpuUAVResolved = m_descriptorHeapResource.GetCurrentGpuHandle();
    device->CreateUnorderedAccessView(m_textureResolved.Get(), nullptr, &unorderedAccessViewDesc, m_descriptorCpuUAVResolved);
    m_descriptorHeapResource.AdvanceIndex();

    D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc =
    {
        c_DXGIFormat,                         // DXGI_FORMAT Format;
        D3D12_RTV_DIMENSION_TEXTURE2D,        // D3D12_RTV_DIMENSION ViewDimension;
        {}                                    // D3D12_TEX2D_RTV Texture2D;
    };
    m_descriptorRTVResolved = m_descriptorHeapRenderTarget.GetCurrentCpuHandle();
    m_descriptorHeapRenderTarget.AdvanceIndex();
    device->CreateRenderTargetView(m_textureResolved.Get(), &renderTargetViewDesc, m_descriptorRTVResolved);
#endif

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeRootSignatures()
// Desc: Create all root signatures needed by the sample.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializeRootSignatures(ID3D12Device* const device)
{
    D3D12_DESCRIPTOR_RANGE descriptorRangeCBV =
    {
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,                        // D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
        14U,                                                    // uint32_t NumDescriptors;
        0U,                                                     // uint32_t BaseShaderRegister;
        0U,                                                     // uint32_t RegisterSpace;
        0U,                                                     // uint32_t OffsetInDescriptorsFromTableStart;
    };
    D3D12_DESCRIPTOR_RANGE descriptorRangeSampler =
    {
        D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,                    // D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
        16U,                                                    // uint32_t NumDescriptors;
        0U,                                                     // uint32_t BaseShaderRegister;
        0U,                                                     // uint32_t RegisterSpace;
        0U,                                                     // uint32_t OffsetInDescriptorsFromTableStart;
    };
    D3D12_DESCRIPTOR_RANGE descriptorRangeSRV =
    {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,                        // D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
        128U,                                                   // uint32_t NumDescriptors;
        0U,                                                     // uint32_t BaseShaderRegister;
        0U,                                                     // uint32_t RegisterSpace;
        0U,                                                     // uint32_t OffsetInDescriptorsFromTableStart;
    };
    D3D12_DESCRIPTOR_RANGE descriptorRangeUAV =
    {
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,                        // D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
        64U,                                                    // uint32_t NumDescriptors;
        0U,                                                     // uint32_t BaseShaderRegister;
        0U,                                                     // uint32_t RegisterSpace;
        0U,                                                     // uint32_t OffsetInDescriptorsFromTableStart;
    };

    // Graphics
    {
        CD3DX12_ROOT_PARAMETER rootSignatureElements[12] = {};
        uint32_t rootElement = 0U;

        m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_CBV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeCBV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_VERTEX);                        // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_SAMPLER] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSampler,                                // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_VERTEX);                        // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_SRV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSRV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_VERTEX);                        // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_VERTEX][DESCRIPTOR_TYPE_UAV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeUAV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_VERTEX);                        // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_GEOMETRY][DESCRIPTOR_TYPE_CBV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeCBV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_GEOMETRY);                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_GEOMETRY][DESCRIPTOR_TYPE_SAMPLER] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSampler,                                // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_GEOMETRY);                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_GEOMETRY][DESCRIPTOR_TYPE_SRV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSRV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_GEOMETRY);                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_GEOMETRY][DESCRIPTOR_TYPE_UAV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeUAV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_GEOMETRY);                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_CBV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeCBV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_PIXEL);                         // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SAMPLER] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSampler,                                // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_PIXEL);                         // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_SRV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSRV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_PIXEL);                         // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_graphicsRootElement[SHADER_TYPE_PIXEL][DESCRIPTOR_TYPE_UAV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeUAV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_PIXEL);                         // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)

        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

        descRootSignature.Init(_countof(rootSignatureElements),
            rootSignatureElements,
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* serializedRootSignature;

        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, nullptr));

        DX::ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignatureGraphics.ReleaseAndGetAddressOf())));
        m_rootSignatureGraphics->SetName(L"m_spRootSignatureGraphics");
    }

    // Compute
    {
        CD3DX12_ROOT_PARAMETER rootSignatureElements[4] = {};
        uint32_t rootElement = 0U;

        m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_CBV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeCBV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_ALL);                          // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_SAMPLER] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSampler,                                // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_ALL);                          // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_SRV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeSRV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_ALL);                          // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
        m_computeRootElement[SHADER_TYPE_COMPUTE][DESCRIPTOR_TYPE_UAV] = rootElement;
        rootSignatureElements[rootElement++].InitAsDescriptorTable(
            1U,                                                     // uint32_t numDescriptorRanges,
            &descriptorRangeUAV,                                    // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
            D3D12_SHADER_VISIBILITY_ALL);                          // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)

        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

        descRootSignature.Init(_countof(rootSignatureElements),
            rootSignatureElements,
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* serializedRootSignature;

        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, nullptr));

        DX::ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignatureCompute.ReleaseAndGetAddressOf())));
        m_rootSignatureCompute->SetName(L"m_spRootSignatureCompute");
    }

    return S_OK;
}

//-------------------------------------------------------------------------------------------------------------
// Name: InitializePipelineStates()
// Desc: Create all pipeline states needed by the sample.
//-------------------------------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Sample::InitializePipelineStates(ID3D12Device* const device)
{
    for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
    {
        m_pipelineStateWheelOfFortuneMSAA[logFragments] = new Microsoft::WRL::ComPtr<ID3D12PipelineState>[m_numQualityLevels[logFragments]];
        m_pipelineStateMeshMSAA[logFragments] = new Microsoft::WRL::ComPtr<ID3D12PipelineState>[m_numQualityLevels[logFragments]];
#if ENABLE_EQAA
        m_pipelineStateResolveGraphicsNative[logFragments] = new Microsoft::WRL::ComPtr<ID3D12PipelineState>[m_numQualityLevels[logFragments]];
        m_pipelineStateResolveComputeNative[logFragments] = new Microsoft::WRL::ComPtr<ID3D12PipelineState>[m_numQualityLevels[logFragments]];
#endif
    }

    {
        // Wireframe
        {
            auto vertexShader = DX::ReadData(L"VSConstantColor.cso");
            auto pixelShader = DX::ReadData(L"PSConstantColor.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::Wireframe,                                                                    // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateWireframe.ReleaseAndGetAddressOf())));
        }

        // Grid
        {
            auto vertexShader = DX::ReadData(L"VSGrid.cso");
            auto pixelShader = DX::ReadData(L"PSColor.cso");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                nullptr,                                                                                // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                0U,                                                                                     // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,                                                         // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateGrid.ReleaseAndGetAddressOf())));
        }

        // PointSprite
        {
            auto vertexShader = DX::ReadData(L"VSPointSprite.cso");
            auto geometryShader = DX::ReadData(L"GSPointSprite.cso");
            auto pixelShader = DX::ReadData(L"PSPointSprite.cso");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
                {
                    geometryShader.data(),
                    geometryShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::NonPremultiplied,                                                             // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                   // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                nullptr,                                                                                // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                0U,                                                                                     // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,                                                        // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStatePointSprite.ReleaseAndGetAddressOf())));
        }

        // WheelOfFortune
        {
            auto vertexShader = DX::ReadData(L"VSColor.cso");
            auto pixelShader = DX::ReadData(L"PSColor.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "COLOR",                                        // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                       // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                   // uint32_t SampleMask;
            CommonStates::CullNone,																		// D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthDefault,                                                                 // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                             // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { c_DXGIFormat, },                                                                          // DXGI_FORMAT RTVFormats[8];
            c_DXGIDepthFormat,                                                                          // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };

            for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
            {
                for (uint32_t quality = 0; quality < m_numQualityLevels[logFragments]; ++quality)
                {
                    descPipelineState.SampleDesc.Count = 1u << logFragments;
                    descPipelineState.SampleDesc.Quality = quality;
                    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateWheelOfFortuneMSAA[logFragments][quality].ReleaseAndGetAddressOf())));
                }
            }
        }

        // Mesh
        {
            auto vertexShader = DX::ReadData(L"VSMesh.cso");
            auto pixelShader = DX::ReadData(L"PSMesh.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "NORMAL",                                       // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    12,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    24,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TANGENT",                                      // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    32,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "BINORMAL",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    44,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthDefault,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateMesh.ReleaseAndGetAddressOf())));

            descPipelineState.RTVFormats[0] = c_DXGIFormat;
            for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
            {
                for (uint32_t quality = 0; quality < m_numQualityLevels[logFragments]; ++quality)
                {
                    descPipelineState.SampleDesc.Count = 1u << logFragments;
                    descPipelineState.SampleDesc.Quality = quality;
                    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateMeshMSAA[logFragments][quality].ReleaseAndGetAddressOf())));
                }
            }
        }

        // Texture
        {
            auto vertexShader = DX::ReadData(L"VSTexture.cso");
            auto pixelShader = DX::ReadData(L"PSTexture.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateTexture.ReleaseAndGetAddressOf())));
        }

        // TextureSingleSample
        {
            auto vertexShader = DX::ReadData(L"VSTexture.cso");
            auto pixelShader = DX::ReadData(L"PSTextureSingleSample.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateTextureSingleSample.ReleaseAndGetAddressOf())));
        }

        // TextureAllSamples
        {
            auto vertexShader = DX::ReadData(L"VSTexture.cso");
            auto pixelShader = DX::ReadData(L"PSTextureAllSamples.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateTextureAllSamples.ReleaseAndGetAddressOf())));
        }

        // TextureNearestSample
        {
            auto vertexShader = DX::ReadData(L"VSTexture.cso");
            auto pixelShader = DX::ReadData(L"PSTextureNearestSample.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateTextureNearestSample.ReleaseAndGetAddressOf())));
        }

#if defined(_GAMING_XBOX)
        // ResolveGraphicsUbershader
        {
            auto vertexShader = DX::ReadData(L"VSTexture.cso");
            auto pixelShader = DX::ReadData(L"PSResolveUbershader.cso");

            D3D12_INPUT_ELEMENT_DESC descInputElement[] =
            {
                {
                    "POSITION",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32B32A32_FLOAT,                 // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    0,                                              // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
                {
                    "TEXCOORD",                                     // LPCSTR SemanticName;
                    0,                                              // uint32_t SemanticIndex;
                    DXGI_FORMAT_R32G32_FLOAT,                       // DXGI_FORMAT Format;
                    0,                                              // uint32_t InputSlot;
                    16,                                             // uint32_t AlignedByteOffset;
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                    0,                                              // uint32_t InstanceDataStepRate;
                },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                                                        // ID3D12RootSignature* pRootSignature;
            {
                vertexShader.data(),
                vertexShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShader.data(),
                    pixelShader.size(),
                },                                                                                      // D3D12_SHADER_BYTECODE PS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE GS;
            {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
            CommonStates::Opaque,                                                                        // D3D12_BLEND_DESC BlendState;
            UINT_MAX,                                                                                    // uint32_t SampleMask;
            CommonStates::CullCounterClockwise,                                                         // D3D12_RASTERIZER_DESC RasterizerState;
            CommonStates::DepthNone,                                                                    // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
            {
                descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
                _countof(descInputElement),                                                           // uint32_t NumElements;
            },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
            1,                                                                                          // uint32_t NumRenderTargets;
            { c_DXGIFormat, },                                                                          // DXGI_FORMAT RTVFormats[8];
            m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
            { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateResolveGraphicsUbershader.ReleaseAndGetAddressOf())));

            for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
            {
                for (uint32_t quality = 0; quality < m_numQualityLevels[logFragments]; ++quality)
                {
                    wchar_t wstrPixelShaderName[128];

                    bool colorExpanded = (quality < logFragments) || (quality == 0U && logFragments == 0U);
                    if (colorExpanded)
                    {
                        swprintf_s(wstrPixelShaderName, L"PSResolve_%1dxMSAA.cso", 1 << logFragments);
                    }
                    else
                    {
                        swprintf_s(wstrPixelShaderName, L"PSResolve_%1d_Fragments_%02d_Samples_EQAA.cso", 1 << logFragments, 1 << quality);
                    }
                    pixelShader = DX::ReadData(wstrPixelShaderName);
                    descPipelineState.PS.pShaderBytecode = pixelShader.data();
                    descPipelineState.PS.BytecodeLength = pixelShader.size();

                    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateResolveGraphicsNative[logFragments][quality].ReleaseAndGetAddressOf())));
                }
            }
        }

        // ResolveComputeUbershader
        {
            auto computeShader = DX::ReadData(L"CSResolveUbershader.cso");

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureCompute.Get(),                                                         // ID3D12RootSignature* pRootSignature;
            {
                computeShader.data(),
                computeShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE CS;
            0,                                                                                          // uint32_t NodeMask;
            { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateResolveComputeUbershader.ReleaseAndGetAddressOf())));

            for (uint32_t logFragments = 0; logFragments <= c_maxLogFragments; ++logFragments)
            {
                for (uint32_t quality = 0; quality < m_numQualityLevels[logFragments]; ++quality)
                {
                    wchar_t wstrComputeShaderName[128] = {};

                    bool colorExpanded = (quality < logFragments) || (quality == 0U && logFragments == 0U);
                    if (colorExpanded)
                    {
                        swprintf_s(wstrComputeShaderName, L"CSResolve_%1dxMSAA.cso", 1 << logFragments);
                    }
                    else
                    {
                        swprintf_s(wstrComputeShaderName, L"CSResolve_%1d_Fragments_%02d_Samples_EQAA.cso", 1 << logFragments, 1 << quality);
                    }
                    computeShader = DX::ReadData(wstrComputeShaderName);
                    descPipelineState.CS.pShaderBytecode = computeShader.data();
                    descPipelineState.CS.BytecodeLength = computeShader.size();

                    DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateResolveComputeNative[logFragments][quality].ReleaseAndGetAddressOf())));
                }
            }
        }

        m_resolveUsingPixelShader = true;
        m_resolveUsingUbershader = false;
#endif
    }

    return S_OK;
}
#pragma endregion
