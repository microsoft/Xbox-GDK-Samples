//--------------------------------------------------------------------------------------
// AutoRGB.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AutoRGB.h"
#include "Heuristics.h"

#define CPP_SHARED
#include "shared.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    // Offsets the scene's ambient color to enhance one of the components (r, g, b).
    Vector3 AMBIENT_OFFSET = Vector3(0.0f, 0.0f, 0.0f);

    // Intensity of the ambient color. When zero, turning off lights will result in no visibility.
    float ambientIntensity = 1.0f;
    float ambientTint = 0.5f;

    // Show histograms as UI elements.
    bool showHistogram = true;

    // Related to Exponential Moving Average (EMA) window for smooth transition between computed colors.
    float emaAlpha = 0.1f;

    // Transition speed determines how fast we transition to the new color. This delta represents how much we
    // increase/decrease the transition speed.
    float transitionSpeedDelta = 0.025f;

    // Color band margin. The bigger this is, the bigger the color band around the scene.
    constexpr uint32_t colorBandMargin = 100;

    // This sample has two modes. If running on sample mode, we can move around as in other samples.
    // When running in screenshot mode (renderBGImages = true) we can go through a set of game images.
    bool renderBGImages = false;

    // Starting heuristic for calculating computed color
    COLOR_SELECTION_METHODS colorHeuristic = COLOR_SELECTION_METHODS::BUCKETS_X_AVG;
    ColorSpace colorSpace = ColorSpace::RGB;

    // For loading meshes
    wchar_t const* const s_folderPaths[3] = { L"Media\\Meshes\\AbstractCathedral", L"Media\\Textures\\Game Images", 0 };

    // Fly-Camera
    constexpr Vector3 CAMERA_START_POS = Vector3(0, -150, -400);
    constexpr float CAMERA_NEAR = 0.25f;
    constexpr float CAMERA_FAR = 1500.0f;
}

Sample::Sample() noexcept(false) :
    m_lightDiameter(500),
    m_lightsOn(true),
    m_currentSDRTexture(0),
    m_displayWidth(0),
    m_displayHeight(0),
    m_frame(0),
    m_cpuTimer(std::make_unique<DX::CPUTimer>()),
    m_totalGPUTime(0.0),
    m_computeHistogramsTime(0.0),
    m_frameDeltaTime(0.0),
    m_readbackTime(0.0),
    m_GBuferClrVal{},
    m_GPassAlbedoRTV{},
    m_GPassNormalsRTV{},
    m_GPassPositionRTV{},
    m_depthSRVGPUHandle{},
    m_sceneConstantsVirtualAddress{},
    m_nonShaderVisibleHeap(nullptr)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, BACKBUFFER_COUNT,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);

    // Exponential Moving average for smooth change in computed color
    uint32_t transitionFrames = 30u;
    m_movingAvg = std::make_unique<MovingAvg>(transitionFrames, emaAlpha);

    // Setup the heuristics for color selection
    m_heuristicsSet[COLOR_SELECTION_METHODS::AVERAGE] = new Heuristic_Avg();
    m_heuristicsSet[COLOR_SELECTION_METHODS::TOP_X_BUCKETS] = new Heuristic_TopXBuckets();
    m_heuristicsSet[COLOR_SELECTION_METHODS::BUCKETS_X_AVG] = new Heuristic_BucketsXAverage();

    m_cpuTimer->Start(CPU_TIMER_FRAME_TIME);
}

Sample::~Sample() noexcept(false)
{
    // unregister the LampArray callback
    if (m_callbackToken)
    {
        UnregisterLampArrayCallback(m_callbackToken, 5000);
    }

    for (int i = 0; i < COLOR_SELECTION_METHODS_COUNT; ++i)
    {
        if (m_heuristicsSet[i])
            delete m_heuristicsSet[i];
    }

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
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Setup the LampArray connect/disconnect callback
    DX::ThrowIfFailed(RegisterLampArrayCallback(Sample::LampArrayCallback, this, &m_callbackToken));
}

#pragma region LampArray Methods
// Sets the LampArray compatible device's color (single color for all lamps).
void Sample::LampArraySetUniqueColor(uint8_t red, uint8_t green, uint8_t blue)
{
    LampArrayColor color = {};
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 255;

    for (auto lampArray : m_lampArrays)
    {
        lampArray->SetColor(color);
    }
}

// Called any time a LampArray device is connected or disconnected
void Sample::LampArrayCallback(void* context, bool isAttached, ILampArray* lampArray)
{
    Sample* sample = reinterpret_cast<Sample*>(context);

    if (isAttached)
    {
        sample->m_lampArrays.push_back(lampArray);
    }
    else
    {
        // Find the attached lampArray and remove it
        for (auto iter = sample->m_lampArrays.begin(); iter != sample->m_lampArrays.end(); iter++)
        {
            if ((*iter) == lampArray)
            {
                sample->m_lampArrays.erase(iter);
                break;
            }
        }
    }
}
#pragma endregion

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
        using ButtonState = GamePad::ButtonStateTracker;
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.rightShoulder == ButtonState::RELEASED)
        {
            m_lightsOn = !m_lightsOn;
        }
        if (m_gamePadButtons.leftShoulder == ButtonState::RELEASED)
        {
            showHistogram = !showHistogram;
        }
        if (m_gamePadButtons.y == ButtonState::RELEASED)
        {
            renderBGImages = !renderBGImages;
        }

        if (m_gamePadButtons.dpadRight == ButtonState::RELEASED)
        {
            emaAlpha += 0.025f;
            emaAlpha = std::min(emaAlpha, 1.0f);
            m_movingAvg->UpdateAlpha(emaAlpha);
        }
        else if (m_gamePadButtons.dpadLeft == ButtonState::RELEASED)
        {
            emaAlpha -= 0.025f;
            emaAlpha = std::max(emaAlpha, 0.01f);
            m_movingAvg->UpdateAlpha(emaAlpha);
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            colorHeuristic = static_cast<COLOR_SELECTION_METHODS>((colorHeuristic + 1u) % COLOR_SELECTION_METHODS_COUNT);
        }
        if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            if (AMBIENT_OFFSET.x > 0.0f)
            {
                AMBIENT_OFFSET.x = 0.0f;
                AMBIENT_OFFSET.y = ambientTint;
            }
            else if (AMBIENT_OFFSET.y > 0.0f)
            {
                AMBIENT_OFFSET.y = 0.0f;
                AMBIENT_OFFSET.z = ambientTint;
            }
            else if (AMBIENT_OFFSET.z > 0.0f)
            {
                AMBIENT_OFFSET.z = 0.0f;
            }
            else
            {
                AMBIENT_OFFSET.x = ambientTint;
            }
        }

        if (m_gamePadButtons.rightTrigger == ButtonState::RELEASED)
        {
            emaAlpha += transitionSpeedDelta;
            emaAlpha = std::min(emaAlpha, 1.0f);
            m_movingAvg->UpdateAlpha(emaAlpha);
        }
        else if (m_gamePadButtons.leftTrigger == ButtonState::RELEASED)
        {
            emaAlpha -= transitionSpeedDelta;
            emaAlpha = std::max(emaAlpha, 0.01f);
            m_movingAvg->UpdateAlpha(emaAlpha);
        }

        if (renderBGImages)
        {
            if (m_gamePadButtons.rightTrigger == ButtonState::RELEASED)
            {
                m_currentSDRTexture = (m_currentSDRTexture == c_NumImages - 1) ? 0 : m_currentSDRTexture + 1;
            }
            else if (m_gamePadButtons.leftTrigger == ButtonState::RELEASED)
            {
                m_currentSDRTexture = (m_currentSDRTexture == 0) ? c_NumImages - 1 : m_currentSDRTexture - 1;
            }
        }
        else
        {
            if (m_gamePadButtons.dpadUp == ButtonState::RELEASED)
            {
                ambientIntensity += 0.1f;
                ambientIntensity = std::min(ambientIntensity, 1.0f);
            }
            else if (m_gamePadButtons.dpadDown == ButtonState::RELEASED)
            {
                ambientIntensity -= 0.1f;
                ambientIntensity = std::max(ambientIntensity, 0.0f);
            }

            m_camera.Update(elapsedTime, pad);
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}

// Updates the scene constant buffer
void Sample::UpdateSceneConstants(Matrix const& view, Matrix const& proj)
{
    Matrix invProj = Matrix::Identity;
    invProj._11 = 1.0f / proj._11;
    invProj._22 = 1.0f / proj._22;
    invProj._33 = 0.0f;
    invProj._34 = 1.0f / proj._43;
    invProj._43 = 1.0f;
    invProj._44 = -proj._33 / proj._43;

    SceneConstantsStruct sceneConstantsCB = {};
    sceneConstantsCB.matViewProj = (view * proj).Transpose();
    sceneConstantsCB.matView = view.Transpose();
    sceneConstantsCB.matInvProj = invProj.Transpose();
    sceneConstantsCB.cameraPos = m_camera.GetPosition();
    sceneConstantsCB.lightRadius = m_lightDiameter / 2.0f;
    sceneConstantsCB.sceneLightCount = NUM_INSTANCES;
    sceneConstantsCB.tileCorrectScreenWidth = ((m_displayWidth + GROUP_WIDTH - 1) / GROUP_WIDTH) * GROUP_WIDTH;;
    sceneConstantsCB.tileCorrectScreenHeight = ((m_displayHeight + GROUP_WIDTH - 1) / GROUP_WIDTH) * GROUP_WIDTH;

    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();
    memcpy(&m_pSceneConstantsMappedMem[frameIndex].data, &sceneConstantsCB, sizeof(SceneConstantsStruct));
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

    m_cpuTimer->Stop(CPU_TIMER_FRAME_TIME);
    m_frameDeltaTime = m_cpuTimer->GetElapsedMS(CPU_TIMER_FRAME_TIME);
    m_cpuTimer->Start(CPU_TIMER_FRAME_TIME);

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Begin frame timer
    m_gpuTimer->BeginFrame(commandList);

    Matrix proj = m_camera.GetProjection();
    Matrix view = m_camera.GetView();

    // Get readback resource and calculate the ambient color using the chosen heuristic
    GetDataFromReadback();

    // Change lampArrays colors. We need to transform them from [0.0f,1.0f] range to [0,255]
    uint8_t r = static_cast<uint8_t>(m_sceneColorFromHistogram.r * 255);
    uint8_t g = static_cast<uint8_t>(m_sceneColorFromHistogram.g * 255);
    uint8_t b = static_cast<uint8_t>(m_sceneColorFromHistogram.b * 255);
    LampArraySetUniqueColor(r, g, b);

    // Update scene constant buffers
    UpdateSceneConstants(view, proj);

    // Start timer to measure total gpu time for the frame
    m_gpuTimer->Start(commandList, static_cast<uint32_t>(GPU_TIMER_TOTAL));

    // Render either the scene or the game screenshots
    if (renderBGImages)
    {
        RenderBGImage(commandList);
    }
    else
    {
        RenderDeferredScene(commandList);
    }

    // Compute histograms
    ComputeHistograms();

    // From here on out we dont use depth and render directly to backbuffer
    auto rtv = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    // Clear RT to the ambient color we extracted from last frame. This will show in the final frame as a unique color band
    // surrounding the actual rendered scene. It is here as a way to easily visualize the computed color.
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear to computed color");
    float color[4] = { m_sceneColorFromHistogram.r, m_sceneColorFromHistogram.g, m_sceneColorFromHistogram.b, 1.0f };
    commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
    PIXEndEvent(commandList);

    // Render intermediate target (with the scene) in a smaller viewport
    RenderScene();

    // UI and Histograms
    DrawHUD();
    if (showHistogram)
    {
        DrawHistograms();
    }

    m_gpuTimer->Stop(commandList, static_cast<uint32_t>(GPU_TIMER_TOTAL));
    m_totalGPUTime = m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPU_TIMER_TOTAL));

    PIXEndEvent(commandList);
    m_gpuTimer->EndFrame(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::GetDataFromReadback()
{
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();

    m_cpuTimer->Start(CPU_TIMER_READBACK_AND_GEN_AMBIENT_COLOR);

    uint32_t prevFrame = (frameIndex == 0) ? 1u : 0u;
    D3D12_RANGE range{ 0,  NUMBER_OF_HISTOGRAMS * NUM_BINS_PER_HISTOGRAM * sizeof(uint32_t) };
    DX::ThrowIfFailed(
        m_histogramFrameRB[prevFrame]->Map(0, &range, reinterpret_cast<void**>(&m_pReadbackBufferData)));

    // Calculate color from histogram data obtained from the previous frame.
    Vector3 finalColor = GatherSceneColorInfoFromHistogram(m_pReadbackBufferData);
    m_sceneColorFromHistogram = { finalColor.x, finalColor.y, finalColor.z, 1.0f };

    D3D12_RANGE emptyRange{ 0, 0 };
    m_histogramFrameRB[prevFrame]->Unmap(0, &emptyRange);

    m_cpuTimer->Stop(CPU_TIMER_READBACK_AND_GEN_AMBIENT_COLOR);
    m_readbackTime = m_cpuTimer->GetElapsedMS(CPU_TIMER_READBACK_AND_GEN_AMBIENT_COLOR);
}

void Sample::ComputeHistograms()
{
    auto commandList = m_deviceResources->GetCommandList();
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ComputeHistograms Pass");
    m_gpuTimer->Start(commandList, (uint32_t)GPU_TIMER_GENERATE_HISTOGRAM);

    // Here we transition the RTV to which we have been rendering into an SRV
    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_histogramFrame[frameIndex].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTgt[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

    UINT values[4] = {};
    commandList->ClearUnorderedAccessViewUint(
        m_shaderVisibleHeap->GetGpuHandle(HISTOGRAM_UAV_0 + frameIndex),
        m_nonShaderVisibleHeap->GetCpuHandle(frameIndex),
        m_histogramFrame[frameIndex].Get(),
        values,
        0,
        nullptr);

    auto const vp = m_deviceResources->GetScreenViewport();
    auto height = static_cast<uint32_t>(vp.Height);

    CSConstantBuffer cb = { static_cast<uint32_t>(vp.Width), height, {0, 0} };
    auto cbHandle = m_graphicsMemory->AllocateConstant(cb);

    commandList->SetComputeRootSignature(m_histogramCS_RS.Get());

    commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());
    commandList->SetComputeRootDescriptorTable(1, m_shaderVisibleHeap->GetGpuHandle(INTERMEDIATE_TARGET_SRV_0 + frameIndex));
    commandList->SetComputeRootDescriptorTable(2, m_shaderVisibleHeap->GetGpuHandle(HISTOGRAM_UAV_0 + frameIndex));

    commandList->SetPipelineState(m_histogramCS_PSO.Get());

    commandList->Dispatch(height, 1, 1);

    // Copy into readback resource for next frame
    {
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_histogramFrame[frameIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, barriers);

        commandList->CopyResource(m_histogramFrameRB[frameIndex].Get(), m_histogramFrame[frameIndex].Get());
    }

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTgt[frameIndex].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, barriers);

    m_gpuTimer->Stop(commandList, static_cast<uint32_t>(GPU_TIMER_GENERATE_HISTOGRAM));
    m_computeHistogramsTime = m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPU_TIMER_GENERATE_HISTOGRAM));
    PIXEndEvent(commandList);
}

void Sample::RenderScene()
{
    auto commandList = m_deviceResources->GetCommandList();
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderScene Pass");

    // Here we transition the rtv to which we have been rendering into an SRV
    ScopedBarrier scopeBarrier(commandList,
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTgt[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        });

    auto const vp = m_deviceResources->GetScreenViewport();
    D3D12_VIEWPORT bgvp = {
        vp.TopLeftX + colorBandMargin, vp.TopLeftY + colorBandMargin,
        vp.Width - 2 * colorBandMargin, vp.Height - 2 * colorBandMargin,
        vp.MinDepth, vp.MaxDepth
    };
    commandList->RSSetViewports(1, &bgvp);
    D3D12_RECT scissorRect = { static_cast<long>(bgvp.TopLeftX), static_cast<long>(bgvp.TopLeftY), static_cast<long>(bgvp.TopLeftX + bgvp.Width), static_cast<long>(bgvp.TopLeftY + bgvp.Height) };
    commandList->RSSetScissorRects(1, &scissorRect);
    m_fullScreenQuad->Draw(commandList, m_BGImagesPSO.Get(), m_shaderVisibleHeap->GetGpuHandle(INTERMEDIATE_TARGET_SRV_0 + frameIndex));

    PIXEndEvent(commandList);
}

void Sample::RenderDeferredScene(ID3D12GraphicsCommandList* commandList)
{
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();
    auto dsv = m_deviceResources->GetDepthStencilView();

    // Deferred pass (G-Pass)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"G-Pass");

        // Transition SRVs to RTVs for gbuffer RTs
        D3D12_RESOURCE_BARRIER barriers[3] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceAlbedo.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceNormals.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourcePosition.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

        commandList->ClearRenderTargetView(m_GPassAlbedoRTV, m_GBuferClrVal.Color, 0, nullptr);
        commandList->ClearRenderTargetView(m_GPassNormalsRTV, m_GBuferClrVal.Color, 0, nullptr);
        commandList->ClearRenderTargetView(m_GPassPositionRTV, m_GBuferClrVal.Color, 0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_GPassAlbedoRTV, m_GPassNormalsRTV, m_GPassPositionRTV };
        commandList->OMSetRenderTargets(static_cast<uint32_t>(std::size(rtvHandles)), rtvHandles, false, &dsv);

        ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
        commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

        commandList->SetGraphicsRootSignature(m_gPassRS.Get());

        auto sceneCstCBAddress = m_sceneConstantsVirtualAddress + frameIndex * sizeof(SceneConstantsStructPadded);
        commandList->SetGraphicsRootConstantBufferView(2, sceneCstCBAddress);

        // Background mesh is static, so identity for model matrix is ok.
        PerObjectStruct perObjData = {};
        perObjData.worldMatRotation = perObjData.worldMat = Matrix::Identity;
        auto gfxRes = m_graphicsMemory->AllocateConstant(perObjData);
        commandList->SetGraphicsRootConstantBufferView(3, gfxRes.GpuAddress());

        commandList->SetPipelineState(m_gPassPSO.Get());

        // Draw all submeshes
        for (size_t i = 0; i < m_model->meshes.size(); ++i)
        {
            auto& opaqueParts = m_model->meshes[i]->opaqueMeshParts;
            for (size_t j = 0; j < opaqueParts.size(); ++j)
            {
                uint32_t materialIndex = opaqueParts[j]->materialIndex;
                int texIndex = m_model->materials[materialIndex].diffuseTextureIndex;
                D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_shaderVisibleHeap->GetGpuHandle((size_t)(SCENE_TEXTURE_OFFSET + texIndex));
                commandList->SetGraphicsRootDescriptorTable(0, srvHandle);
                opaqueParts[j]->Draw(commandList);
            }
        }

        PIXEndEvent(commandList);
    }

    // Transition GBuffer's from RTV to SRV
    D3D12_RESOURCE_BARRIER barriers[3] = {};
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceAlbedo.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceNormals.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourcePosition.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    // For the FSQ we don't use depth
    auto rtv = m_intermediateRTV[frameIndex];
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    // Deferred tiled light pass
    if (m_lightsOn)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Light Binning + Tiling");

        ScopedBarrier scopeBarrier(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTgt[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            });

        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap() };
        commandList->SetDescriptorHeaps((uint32_t)std::size(heaps), heaps);

        commandList->SetComputeRootSignature(m_lightTiledPassRS.Get());

        commandList->SetComputeRootDescriptorTable(0, m_shaderVisibleHeap->GetGpuHandle(INTERMEDIATE_TARGET_UAV_0 + frameIndex));

        commandList->SetComputeRootConstantBufferView(1, m_lightPosCBVAddress);

        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_GBufferSRHeap->GetFirstGpuHandle();
        commandList->SetComputeRootDescriptorTable(2, srvHandle);

        commandList->SetComputeRootDescriptorTable(3, m_depthSRVGPUHandle);

        auto sceneCstCBAddress = m_sceneConstantsVirtualAddress + frameIndex * sizeof(SceneConstantsStructPadded);
        commandList->SetComputeRootConstantBufferView(4, sceneCstCBAddress);

        constexpr uint32_t ROOT_CONSTANT_ROOT_INDEX = 5;
        AmbientConstants tint = {};
        tint.r = AMBIENT_OFFSET.x;
        tint.g = AMBIENT_OFFSET.y;
        tint.b = AMBIENT_OFFSET.z;
        tint.intensity = ambientIntensity;
        commandList->SetComputeRoot32BitConstants(ROOT_CONSTANT_ROOT_INDEX, sizeof(AmbientConstants) / sizeof(uint32_t), &tint, 0);

        commandList->SetPipelineState(m_lightTiledPassPSO.Get());

        uint32_t threadGroupCountX = m_displayWidth / GROUP_WIDTH;
        uint32_t threadGroupCountY = m_displayHeight / GROUP_WIDTH;

        commandList->Dispatch(threadGroupCountX, threadGroupCountY, 1);

        PIXEndEvent(commandList);
    }
    else
    { 
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Lights-Off Ambient Pass");
        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap() };
        commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);
        
        commandList->SetGraphicsRootSignature(m_ambientPassRS.Get());
        
        commandList->SetGraphicsRootDescriptorTable(0, m_GBufferSRHeap->GetFirstGpuHandle());
        
        AmbientConstants tint = {};
        tint.r = AMBIENT_OFFSET.x;
        tint.g = AMBIENT_OFFSET.y;
        tint.b = AMBIENT_OFFSET.z;
        tint.intensity = ambientIntensity;
        commandList->SetGraphicsRoot32BitConstants(1, sizeof(AmbientConstants) / sizeof(uint32_t), &tint, 0);
        
        commandList->SetPipelineState(m_ambientPassPSO.Get());
        commandList->DrawInstanced(3, 1, 0, 0);
        
        PIXEndEvent(commandList);
    }
}

void Sample::RenderBGImage(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render BG Image");

    // For the FSQ we don't use depth
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();
    auto rtv = m_intermediateRTV[frameIndex];
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

    commandList->SetGraphicsRootSignature(m_ambientPassRS.Get());

    auto srv = m_shaderVisibleHeap->GetGpuHandle(BG_TEXTURE_0 + m_currentSDRTexture);
    m_fullScreenQuad->Draw(commandList, m_BGImagesPSO.Get(), srv);

    PIXEndEvent(commandList);
}

Vector3 Sample::GatherSceneColorInfoFromHistogram(uint32_t* histograms)
{
    HeuristicsBase* heuristic = m_heuristicsSet[colorHeuristic];

    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint32_t>(size.right - size.left);
    m_displayHeight = static_cast<uint32_t>(size.bottom - size.top);
    float const pixelCount = static_cast<float>(m_displayWidth * m_displayHeight);

    heuristic->CalculateStatistics(histograms, pixelCount);

    Vector3 heuristicColor = heuristic->GetColor(colorSpace, m_displayWidth, m_displayHeight);
    return m_movingAvg->GetEMA(heuristicColor);
}

void Sample::DrawHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DrawHUD Pass");
    m_hudBatch->Begin(commandList);

    ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textColorEnabled = DirectX::Colors::Aquamarine;
    const XMVECTOR textColorDisabled = DirectX::Colors::DarkKhaki;
    const XMVECTOR textTimerColor = DirectX::Colors::AntiqueWhite;
    const XMVECTOR textColor = true ? textColorEnabled : textColorDisabled;

    wchar_t buffer[100];
    {
        // Description
        swprintf_s(buffer, std::size(buffer), L"AutoRGB Sample");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += 2 * m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Heuristic: %s", wstrp_ColorSelectionTechniqueName[colorHeuristic].c_str());
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Color Transition Speed: %.4f", emaAlpha);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Base light intensity: %.4f", ambientIntensity);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (14.0f * m_smallFont->GetLineSpacing());
        
        // Timers
        swprintf_s(buffer, std::size(buffer), L"Frame time: %.2f ms", m_frameDeltaTime);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();
        
        swprintf_s(buffer, std::size(buffer), L"Readback + compute ambient color: %.2f ms", m_readbackTime);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Compute Histograms pass: %.2f ms", m_computeHistogramsTime);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Total GPU time: %.2f ms", m_totalGPUTime);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Instruction section
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[A] Change Heuristic.", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[LB] Toggle UI.", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        if (renderBGImages)
        {
            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[Y] Switch to Sample Mode.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[LT]/[RT] Change Screenshot.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();
        }
        else
        {
            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[RB] Toggle Lights.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[B] Switch Scene Tint.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[Y] Switch to Screenshot Mode.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[DPad] (up/down) Base Light Intensity.", textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();
        }

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[DPad] (right/left) Transition Speed.", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_controllerFont.get(), L"[View] Exit Sample", textPos, textColor);
        textPos.y += 2 * m_smallFont->GetLineSpacing();
    }

    m_hudBatch->End();
    PIXEndEvent(commandList);
}

void Sample::DrawHistograms()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DrawHistograms Pass");

    ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Render Histograms + Text
    auto const vp = m_deviceResources->GetScreenViewport();
    {
        // We have 4 histograms (rgb-luma)
        uint32_t const numHistograms = 4;
        uint32_t const yDivision = numHistograms + 1;
        uint32_t const xDivision = 3;
        uint32_t const topMargin = 100;

        // Pushing text this many pixels to the left so it does not overlap the histograms
        uint32_t horizontalOffset = 50;

        // Bg panel for the histograms
        float borderTop = vp.TopLeftY + topMargin;
        D3D12_VIEWPORT bgvp =
        {
            vp.TopLeftX + (2 * vp.Width / xDivision) - (2 * horizontalOffset),  // top left
            borderTop,                                                          // top right
            (vp.Width / xDivision) + (2 * horizontalOffset),                    // width
            numHistograms * vp.Height / yDivision,                              // height
            vp.MinDepth, vp.MaxDepth
        };
        commandList->RSSetViewports(1, &bgvp);

        D3D12_RECT scissorRect = { static_cast<long>(bgvp.TopLeftX), static_cast<long>(bgvp.TopLeftY), static_cast<long>(bgvp.TopLeftX + bgvp.Width), static_cast<long>(bgvp.TopLeftY + bgvp.Height) };
        commandList->RSSetScissorRects(1, &scissorRect);

        SingleColor colorCst{ 0.0f, 0.0f , 0.0f , 0.5f };
        auto cbHandle = m_graphicsMemory->AllocateConstant(colorCst);
        m_fullScreenQuad->Draw(commandList, m_singleColorBlockPSO.Get(), m_shaderVisibleHeap->GetGpuHandle(15), cbHandle.GpuAddress());

        // Text for each histogram
        m_hudBatch->Begin(commandList);
        const XMVECTOR textColor = DirectX::Colors::AntiqueWhite;

        // Margin for histograms (and their text) w.r.t. their top border
        uint32_t borderMargin = 20;
        float HistogramTop = borderTop + borderMargin;

        commandList->RSSetViewports(1, &vp);
        wchar_t buffer[100];
        for (uint32_t i = 0; i < numHistograms; ++i)
        {
            //HUD always renders 1080p
            XMFLOAT2 textPos = XMFLOAT2(
                float(vp.TopLeftX + (xDivision - 1) * (1920 / xDivision) - horizontalOffset),
                float(HistogramTop + i * (1080 / yDivision))
            );
            swprintf_s(buffer, std::size(buffer), L"%s", wstrp_HistogramName[i].c_str());
            m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        }

        m_hudBatch->End();

        // Render each histogram
        uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();
        uint32_t const marginPixels = 20;
        for (uint32_t i = 0; i < numHistograms; ++i)
        {
            D3D12_VIEWPORT vizvp =
            {
                vp.TopLeftX + (xDivision - 1) * (vp.Width / xDivision),
                HistogramTop + i * (vp.Height / yDivision),
                (vp.Width / xDivision) - (2 * marginPixels),
                (vp.Height / yDivision) - (2 * marginPixels),
                vp.MinDepth, vp.MaxDepth
            };
            commandList->RSSetViewports(1, &vizvp);

            scissorRect = { static_cast<long>(vizvp.TopLeftX), static_cast<long>(vizvp.TopLeftY), static_cast<long>(vizvp.TopLeftX + vizvp.Width), static_cast<long>(vizvp.TopLeftY + vizvp.Height) };
            commandList->RSSetScissorRects(1, &scissorRect);

            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_histogramFrame[frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            commandList->ResourceBarrier(1, &barrier);

            uint32_t offset = i * NUM_BINS_PER_HISTOGRAM;
            VizConstantBuffer cb = { NUM_BINS_PER_HISTOGRAM, vp.Width * vp.Height, offset, 0 };
            auto vizCB = m_graphicsMemory->AllocateConstant(cb);
            m_fullScreenQuad->Draw(commandList, m_visualizeHistogram_PSO.Get(), m_shaderVisibleHeap->GetGpuHandle(HISTOGRAM_SRV_0 + frameIndex), vizCB.GpuAddress());
        }
    }

    PIXEndEvent(commandList);
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
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Full screen Quad
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    // Create PSOs
    CreatePipelineStates();

    // Light positions
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        uint32_t bufferSize = sizeof(lightPositionsCB);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_lightPosCBResource.ReleaseAndGetAddressOf())));
        m_lightPosCBResource->SetName(L"lightPosCBResource");

        m_lightPosCBVAddress = m_lightPosCBResource->GetGPUVirtualAddress();

        // Copy positions data to gpu
        InitializePerParticleData();
    }

    // Scene Constants
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        uint8_t NUM_CALLS = 1;
        uint32_t bufferSize = m_deviceResources->GetBackBufferCount() * NUM_CALLS * sizeof(SceneConstantsStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_sceneConstantsResource.ReleaseAndGetAddressOf())));
        m_sceneConstantsResource->SetName(L"m_sceneConstantsResource");

        m_sceneConstantsVirtualAddress = m_sceneConstantsResource->GetGPUVirtualAddress();
        DX::ThrowIfFailed(m_sceneConstantsResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pSceneConstantsMappedMem)));
    }

    // Create compute shader objects.
    {
        auto csBlob = DX::ReadData(L"HistogramCS_GSM.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_histogramCS_RS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_histogramCS_RS.Get();
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_histogramCS_PSO.ReleaseAndGetAddressOf())));
        m_histogramCS_PSO->SetName(L"Histogram (GSM)");
    }

    // Create the gpu timer
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    // Load Model and other stuff
    std::wstring mediaDirectory;
    {
        m_shaderVisibleHeap = std::make_unique<DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 128, 0);
        m_shaderVisibleHeap->Heap()->SetName(L"shaderVisibleHeap");

        {
            wchar_t s_meshFilename[1024];
            wchar_t filepath[1024];
            _snwprintf_s(s_meshFilename, std::size(s_meshFilename), _TRUNCATE, L"AbstractCathedral.sdkmesh");

            DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), s_meshFilename, s_folderPaths);

            // Store the media directory
            std::wstring pathTemp = filepath;
            mediaDirectory = pathTemp.substr(0, pathTemp.find_last_of('\\'));

            auto modelBlob = DX::ReadData(filepath);
            m_model = Model::CreateFromSDKMESH(device, modelBlob.data(), modelBlob.size());
        }

        // Upload resources to video memory
        ResourceUploadBatch upload(device);
        upload.Begin();

        // Sprite batch
        {
            RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
            SpriteBatchPipelineStateDescription pd(rtState);
            m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
        }

        // UI (HUD)
        {
            auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
            auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
            m_hudBatch = std::make_unique<SpriteBatch>(device, upload, spritePSD);

            size_t start, end;
            m_shaderVisibleHeap->AllocateRange(2, start, end);
            assert(TEXT_FONT_OFFSET == start && L"Range does not match.");
            assert(CONTROLLER_FONT_OFFSET == (start + 1) && L"Range does not match.");

            wchar_t strFilePath[MAX_PATH] = {};
            DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
            m_smallFont = std::make_unique<SpriteFont>(device, upload,
                strFilePath,
                m_shaderVisibleHeap->GetCpuHandle(TEXT_FONT_OFFSET),
                m_shaderVisibleHeap->GetGpuHandle(TEXT_FONT_OFFSET));

            DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
            m_controllerFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
                m_shaderVisibleHeap->GetCpuHandle(CONTROLLER_FONT_OFFSET),
                m_shaderVisibleHeap->GetGpuHandle(CONTROLLER_FONT_OFFSET));
        }

        // Between fonts and textures, need to allocate for the reminder of the heap
        size_t start, end;
        m_shaderVisibleHeap->AllocateRange((uint32_t)(HISTOGRAM_READBACK_1 - CONTROLLER_FONT_OFFSET), start, end);
        assert(start == (uint32_t)(CONTROLLER_FONT_OFFSET + 1));

        // Also allocate for the bg textures
        m_shaderVisibleHeap->AllocateRange((uint32_t)(BG_TEXTURE_FINAL - BG_TEXTURE_0 + 1), start, end);
        assert(start == (uint32_t)(BG_TEXTURE_0));

        // Histogram buffer resource (2, since we use one per backbuffer)
        {
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(NUMBER_OF_HISTOGRAMS * NUM_BINS_PER_HISTOGRAM * sizeof(UINT));
            CD3DX12_HEAP_PROPERTIES heapPropsREADBACK(D3D12_HEAP_TYPE_READBACK);
            CD3DX12_HEAP_PROPERTIES heapPropsDEFAULT(D3D12_HEAP_TYPE_DEFAULT);

            for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
            {
                DX::ThrowIfFailed(
                    device->CreateCommittedResource(&heapPropsREADBACK,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(m_histogramFrameRB[i].ReleaseAndGetAddressOf())));
            }

            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
            {
                DX::ThrowIfFailed(
                    device->CreateCommittedResource(&heapPropsDEFAULT,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(m_histogramFrame[i].ReleaseAndGetAddressOf())));
            }
            m_histogramFrame[0]->SetName(L"Histogram Buffer Frame 1");
            m_histogramFrame[1]->SetName(L"Histogram Buffer Frame 2");
            m_histogramFrameRB[0]->SetName(L"Histogram Buffer RB Frame 1");
            m_histogramFrameRB[1]->SetName(L"Histogram Buffer RB Frame 2");
        }

        // Views for the histogram pass and heap
        {
            m_nonShaderVisibleHeap = std::make_unique<DescriptorHeap>(
                device,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                m_deviceResources->GetBackBufferCount());
            m_nonShaderVisibleHeap->Heap()->SetName(L"NonShaderVisibleHeap");

            // 2 UAVs for the histogram buffers, and also their cpu descriptor (for clear)
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_R32_UINT;
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.NumElements = NUMBER_OF_HISTOGRAMS * NUM_BINS_PER_HISTOGRAM;

            // 2 SRV for the histogram buffers for drawing histograms
            D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvdesc.Format = DXGI_FORMAT_R32_UINT;
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvdesc.Buffer.NumElements = NUMBER_OF_HISTOGRAMS * NUM_BINS_PER_HISTOGRAM;

            for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
            {
                // Create shader visible handle
                device->CreateUnorderedAccessView(m_histogramFrame[i].Get(), nullptr, &desc, m_shaderVisibleHeap->GetCpuHandle(HISTOGRAM_UAV_0 + i));

                // Create non shader visible handle
                device->CreateUnorderedAccessView(m_histogramFrame[i].Get(), nullptr, &desc, m_nonShaderVisibleHeap->GetCpuHandle(i));

                // Create SRV (for debug drawing the histogram)
                device->CreateShaderResourceView(m_histogramFrame[i].Get(), &srvdesc, m_shaderVisibleHeap->GetCpuHandle(HISTOGRAM_SRV_0 + i));
            }
        }

        m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, m_shaderVisibleHeap->Heap());
        size_t texOffsets;
        if (!m_model->textureNames.empty())
        {
            m_texFactory->SetDirectory(mediaDirectory.c_str());

            size_t texEnd;
            m_shaderVisibleHeap->AllocateRange(m_model->textureNames.size(), texOffsets, texEnd);
            assert(texOffsets == (uint32_t)SCENE_TEXTURE_OFFSET);
            m_texFactory->EnableForceSRGB(true);
            m_model->LoadTextures(*m_texFactory, INT(texOffsets));
        }

        m_model->LoadStaticBuffers(device, upload);

        auto finish = upload.End(m_deviceResources->GetCommandQueue());
        finish.wait();
    }

    // Primitive to represent directional light in scene (and resource)
    m_lightMesh = GeometricPrimitive::CreateSphere(1.0f, 8Ui64, true, false, device);

    m_deviceResources->WaitForGpu();

    // Load SDR textures
    for (size_t i = 0; i < c_NumImages; i++)
    {
        std::unique_ptr<ResourceUploadBatch> resourceUpload = std::make_unique<ResourceUploadBatch>(device);
        resourceUpload->Begin();

        auto srv = m_shaderVisibleHeap->GetCpuHandle(size_t(BG_TEXTURE_0 + i));

        wchar_t s_texFilename[1024] = {};
        wchar_t filepath[1024] = {};
        _snwprintf_s(s_texFilename, std::size(s_texFilename), _TRUNCATE, m_sdrTextureFiles[i]);
        DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), s_texFilename, s_folderPaths);

        m_sdrTexture[i] = std::make_unique<DX::Texture>(device, *(resourceUpload.get()), srv, filepath, false);

        auto uploadResourcesFinished = resourceUpload->End(m_deviceResources->GetCommandQueue());
        uploadResourcesFinished.wait();
        m_sdrTextureFinishedLoading[i] = true;
    }
}

void Sample::CreateIntermediateRTAndViews(uint32_t newWidth, uint32_t newHeight)
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = m_deviceResources->GetBackBufferFormat();
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = static_cast<uint32_t>(newWidth);
    resDesc.Height = static_cast<uint32_t>(newHeight);
    resDesc.MipLevels = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;

    D3D12_CLEAR_VALUE clrVal = CD3DX12_CLEAR_VALUE(m_deviceResources->GetBackBufferFormat(), DirectX::Colors::Black);

    // Make heap to create the RTV for this resource
    m_intermediateHeap.reset();
    m_intermediateHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        m_deviceResources->GetBackBufferCount());
    m_intermediateHeap->Heap()->SetName(L"IntermediateRTHeap");

    // Resources
    for (size_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clrVal,
            IID_GRAPHICS_PPV_ARGS(m_intermediateRenderTgt[i].ReleaseAndGetAddressOf())));
    }
    m_intermediateRenderTgt[0]->SetName(L"IntermediateRenderTarget_0");
    m_intermediateRenderTgt[1]->SetName(L"IntermediateRenderTarget_1");

    // Render Target View
    for (size_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_deviceResources->GetBackBufferFormat();
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        m_intermediateRTV[i] = m_intermediateHeap->GetCpuHandle(i);
        device->CreateRenderTargetView(m_intermediateRenderTgt[i].Get(), &rtvDesc, m_intermediateRTV[i]);
    }

    // SRVs for intermediate buffer
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = m_deviceResources->GetBackBufferFormat();
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = 1;
    for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_intermediateRenderTgt[i].Get(), &desc, m_shaderVisibleHeap->GetCpuHandle(INTERMEDIATE_TARGET_SRV_0 + i));
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = m_deviceResources->GetBackBufferFormat();
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_intermediateRenderTgt[i].Get(), nullptr, &uavDesc, m_shaderVisibleHeap->GetCpuHandle(INTERMEDIATE_TARGET_UAV_0 + i));
    }
}

void Sample::CreateGeometryBufferResourcesAndViews(uint32_t newWidth, uint32_t newHeight)
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resDesc.Width = static_cast<uint32_t>(newWidth);
    resDesc.Height = static_cast<uint32_t>(newHeight);
    resDesc.MipLevels = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;

    m_GBuferClrVal = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::Colors::Black);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        &m_GBuferClrVal,
        IID_GRAPHICS_PPV_ARGS(m_GBufferResourceAlbedo.ReleaseAndGetAddressOf())));
    m_GBufferResourceAlbedo->SetName(L"GBufferResourceAlbedo");

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        &m_GBuferClrVal,
        IID_GRAPHICS_PPV_ARGS(m_GBufferResourceNormals.ReleaseAndGetAddressOf())));
    m_GBufferResourceNormals->SetName(L"GBufferResourceNormals");

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        &m_GBuferClrVal,
        IID_GRAPHICS_PPV_ARGS(m_GBufferResourcePosition.ReleaseAndGetAddressOf())));
    m_GBufferResourcePosition->SetName(L"GBufferResourcePosition");

    // Resource for the tiled pass output
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Format = m_deviceResources->GetBackBufferFormat();
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_tiledOutputResource.ReleaseAndGetAddressOf())));
    m_tiledOutputResource->SetName(L"TiledOutputResource");

    // Descriptor heap for GPass Render Targets
    {
        m_GBufferRTHeap.reset();
        m_GBufferRTHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, GPASS_RT_COUNT);
        m_GBufferRTHeap->Heap()->SetName(L"GBufferRenderTargetHeap");

        // Render Target View
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        m_GPassAlbedoRTV = m_GBufferRTHeap->GetCpuHandle(0);
        m_GPassNormalsRTV = m_GBufferRTHeap->GetCpuHandle(1);
        m_GPassPositionRTV = m_GBufferRTHeap->GetCpuHandle(2);
        device->CreateRenderTargetView(m_GBufferResourceAlbedo.Get(), &rtvDesc, m_GPassAlbedoRTV);
        device->CreateRenderTargetView(m_GBufferResourceNormals.Get(), &rtvDesc, m_GPassNormalsRTV);
        device->CreateRenderTargetView(m_GBufferResourcePosition.Get(), &rtvDesc, m_GPassPositionRTV);
    }

    // Create Shader Resource Heap for GBuffers
    {
        // 3 G buffer, 1 for depth texture and 1 for UAV
        m_GBufferSRHeap.reset();
        m_GBufferSRHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 5);
        m_GBufferSRHeap->Heap()->SetName(L"GBufferSRHeap");

        // Create SRVs for gbuffer
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_GBufferResourceAlbedo.Get(), &srvDesc, m_GBufferSRHeap->GetCpuHandle(0));
        device->CreateShaderResourceView(m_GBufferResourceNormals.Get(), &srvDesc, m_GBufferSRHeap->GetCpuHandle(1));
        device->CreateShaderResourceView(m_GBufferResourcePosition.Get(), &srvDesc, m_GBufferSRHeap->GetCpuHandle(2));

        // SRV for depth texture
        D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
        depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthSrvDesc.Texture2D.MipLevels = 1;
        uint32_t positionInHeap = 3;
        auto depthSRVCpuHandle = m_GBufferSRHeap->GetCpuHandle(positionInHeap);
        device->CreateShaderResourceView(m_deviceResources->GetDepthStencil(), &depthSrvDesc, depthSRVCpuHandle);
        m_depthSRVGPUHandle = m_GBufferSRHeap->GetGpuHandle(positionInHeap);
    }
}

void Sample::CreatePipelineStates()
{
    ID3D12Device* device = m_deviceResources->GetD3DDevice();

    D3D12_INPUT_ELEMENT_DESC gElemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescNormal = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTexcoord0 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTangent = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gInputElemArr[] = { gElemDescPosition , gElemDescNormal, gElemDescTexcoord0, gElemDescTangent };

    // G-Pass PSO
    {
        auto vsBlob = DX::ReadData(L"GPassVertex.cso");
        auto psBlob = DX::ReadData(L"GPassPS.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_gPassRS.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        psoDesc.pRootSignature = m_gPassRS.Get();
        psoDesc.VS = { vsBlob.data(), vsBlob.size() };
        psoDesc.PS = { psBlob.data(), psBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = GPASS_RT_COUNT;
        psoDesc.RTVFormats[0] = GPASS_RT_FORMAT;
        psoDesc.RTVFormats[1] = GPASS_RT_FORMAT;
        psoDesc.RTVFormats[2] = GPASS_RT_FORMAT;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_gPassPSO.ReleaseAndGetAddressOf())));
        m_gPassPSO->SetName(L"GPassPSO");
    }

    // PSO for rendering the fullscreen background images
    {
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_fullScreenQuad->GetRootSignature();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_BGImagesPSO.ReleaseAndGetAddressOf())));
        m_BGImagesPSO->SetName(L"BGImagesPSO");
    }

    // Ambient Pass PSO
    {
        auto vsBlob = DX::ReadData(L"AmbientVS.cso");
        auto psBlob = DX::ReadData(L"AmbientPS.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_ambientPassRS.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        psoDesc.pRootSignature = m_ambientPassRS.Get();
        psoDesc.VS = { vsBlob.data(), vsBlob.size() };
        psoDesc.PS = { psBlob.data(), psBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthNone;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_ambientPassPSO.ReleaseAndGetAddressOf())));
        m_ambientPassPSO->SetName(L"AmbientPassPSO");
    }

    // Tiled Pass PSO
    {
        auto csBlob = DX::ReadData(L"LightBinningCompute.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_lightTiledPassRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_lightTiledPassRS.Get();
        psoDesc.CS = { csBlob.data(), csBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_lightTiledPassPSO.ReleaseAndGetAddressOf())));
        m_lightTiledPassPSO->SetName(L"LightTiledPassPSO");
    }

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Histogram visualization
    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);

        auto pixelShaderBlob = DX::ReadData(L"RenderHistogramPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_visualizeHistogram_PSO.ReleaseAndGetAddressOf());
    }

    // Single Color Box (for visualizing the color)
    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);

        auto pixelShaderBlob = DX::ReadData(L"SingleColorBlockPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_singleColorBlockPSO.ReleaseAndGetAddressOf());
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint64_t>(size.right - size.left);
    m_displayHeight = static_cast<uint64_t>(size.bottom - size.top);

    /// Set main camera properties
    Vector3 cameraPos(CAMERA_START_POS);

    /// Fly camera
    m_camera.SetWindow(static_cast<int32_t>(m_displayWidth), static_cast<int32_t>(m_displayHeight));
    m_camera.SetProjectionParameters(XM_PIDIV4, CAMERA_NEAR, CAMERA_FAR, true);
    m_camera.SetSensitivity(80.0f, 80.0f, 80.0f, 0.0f);
    m_camera.SetPosition(cameraPos);

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);

    auto const viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    // Gbuffer resources and VIEWS
    CreateGeometryBufferResourcesAndViews(m_displayWidth, m_displayHeight);

    // Intermediate RT where we will render the results from the scene (instead of backbuffer)
    CreateIntermediateRTAndViews(m_displayWidth, m_displayHeight);
}
#pragma endregion

// Function that initializes the list of light's positions
void Sample::InitializePerParticleData()
{
    lightPositionsCB cbInstance;

    // Order lights in a grid so they spread through the scene
    int width = 4;                      // (4) lights wide
    int depth = NUM_INSTANCES / width;  // (NUM_INSTANCES/width) lights deep
    int index = 0;

    for (int x = -(width / 2); x < (width / 2); ++x)
    {
        for (int z = -(depth / 2); z < (depth / 2); ++z)
        {
            // Generate a world coord for this light and add to the list
            float xcoord = x * 100.0f;
            float ycoord = -160.0f;
            float zcoord = z * 150.0f;
            Vector4 lightPackedPos = Vector4(xcoord, ycoord, zcoord, static_cast<float>(index));

            cbInstance.lightPositions[index++] = lightPackedPos;
        }
    }

    // Into the SRV with the per particle data
    DX::ThrowIfFailed(m_lightPosCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pLightPosCBMapped)));
    memcpy(m_pLightPosCBMapped, &cbInstance, sizeof(lightPositionsCB));
    m_lightPosCBResource->Unmap(0, nullptr);
}
