//--------------------------------------------------------------------------------------
// AdvancedLighting.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedLighting.h"
#include "utils.h"

#include "ATGColors.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr uint32_t INSTANCE_COUNT = 1200;

    wchar_t const* const s_folderPaths[2] = { L"Media\\Meshes\\AbstractCathedral", 0 };

    // Fly-Camera
    constexpr Vector3 CAMERA_START_POS = Vector3(0.0f, -150.0f, -400.0f);
    constexpr float CAMERA_NEAR = 0.25f;
    constexpr float CAMERA_FAR = 1000.0f;

    constexpr uint32_t RADIUS_CHANGE_STEP = 5;
}

Sample::Sample() noexcept(false) :
    m_lightTechniqueMode(LIGHT_TECHNIQUE::LIGHT_VOLUMES),
    m_lightDiameter(80),
    m_drawParticles(true),
    m_lightsOn(true),
    m_displayWidth(0),
    m_displayHeight(0),
    m_frame(0),
    m_cpuTimer(std::make_unique<DX::CPUTimer>()),
    m_deltaTime{},
    m_frameDeltaTime(0),
    m_GBuferClrVal{},
    m_GPassAlbedoRTV{},
    m_GPassNormalsRTV{},
    m_GPassPositionRTV{},
    m_depthSRVGPUHandle{},
    m_TiledComputeGPUHandle{},
    m_perObjectCBMapped(nullptr),
    m_perObjectCBVAddress{},
    m_zClusterRangesCBMapped(nullptr),
    m_zClusterRangesCBVAddress{},
    m_sceneConstantsMappedMem(nullptr),
    m_sceneConstantsVirtualAddress{}
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_cpuTimer->Start();
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

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

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

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    m_mouse->EndOfInputFrame();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;
        m_gamePadButtons.Update(pad);

        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.dpadRight == ButtonState::RELEASED)
        {
            m_lightDiameter += RADIUS_CHANGE_STEP;
        }
        else if (m_gamePadButtons.dpadLeft == ButtonState::RELEASED)
        {
            if (m_lightDiameter > RADIUS_CHANGE_STEP)
                m_lightDiameter -= RADIUS_CHANGE_STEP;
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_lightTechniqueMode = static_cast<LIGHT_TECHNIQUE>(((static_cast<uint32_t>(m_lightTechniqueMode) + 1) % LIGHT_TECHNIQUE::NUM_TECHNIQUES));
        }

        if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            m_lightTechniqueMode = (m_lightTechniqueMode == 0) ?
                static_cast<LIGHT_TECHNIQUE>((static_cast<uint32_t>(LIGHT_TECHNIQUE::NUM_TECHNIQUES) - 1)) :
                static_cast<LIGHT_TECHNIQUE>((static_cast<uint32_t>(m_lightTechniqueMode) - 1));
        }

        // Turn off the drawing of particles sources (meshes)
        if (m_gamePadButtons.rightShoulder == ButtonState::RELEASED)
        {
            m_drawParticles = !m_drawParticles;
        }

        // Turn lights off, but keep drawing the light sources
        if (m_gamePadButtons.leftShoulder == ButtonState::RELEASED)
        {
            m_lightsOn = !m_lightsOn;
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::X))
    {
        m_lightTechniqueMode = static_cast<LIGHT_TECHNIQUE>(((static_cast<uint32_t>(m_lightTechniqueMode) + 1) % LIGHT_TECHNIQUE::NUM_TECHNIQUES));
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Z))
    {
        m_lightTechniqueMode = (m_lightTechniqueMode == 0) ?
            static_cast<LIGHT_TECHNIQUE>((static_cast<uint32_t>(LIGHT_TECHNIQUE::NUM_TECHNIQUES) - 1)) :
            static_cast<LIGHT_TECHNIQUE>((static_cast<uint32_t>(m_lightTechniqueMode) - 1));
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::V))
    {
        m_lightDiameter += RADIUS_CHANGE_STEP;
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::C))
    {
        if (m_lightDiameter > RADIUS_CHANGE_STEP)
            m_lightDiameter -= RADIUS_CHANGE_STEP;
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Space))
    {
        m_lightsOn = !m_lightsOn;
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Enter))
    {
        m_drawParticles = !m_drawParticles;
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

    m_cpuTimer->Stop();
    m_frameDeltaTime = m_cpuTimer->GetElapsedMS();
    m_cpuTimer->Start();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Begin frame timer
    m_gpuTimer->BeginFrame(commandList);

    Matrix proj = m_camera.GetProjection();
    Matrix view = m_camera.GetView();

    auto dsv = m_deviceResources->GetDepthStencilView();
    auto rtv = m_deviceResources->GetRenderTargetView();
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();

    UpdateSceneConstants(view, proj, frameIndex);

    // Update the particle data
    m_particles.Update(commandList, view, proj, 16.67f, frameIndex, m_lightDiameter / 2.0f);
     
    // Deferred pass (GPASS)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"G-Pass");
        m_gpuTimer->Start(commandList, static_cast<uint32_t>(GPU_TIMER_GEOMETRY_PASS));

        // Transition SRVs to RTVs for gbuffer RTs
        D3D12_RESOURCE_BARRIER SRVtoRTV_Albedo = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceAlbedo.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_RESOURCE_BARRIER SRVtoRTV_Normals = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceNormals.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_RESOURCE_BARRIER SRVtoRTV_Position = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourcePosition.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_RESOURCE_BARRIER barriers[] = { SRVtoRTV_Albedo, SRVtoRTV_Normals, SRVtoRTV_Position };
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

        commandList->ClearRenderTargetView(m_GPassAlbedoRTV, m_GBuferClrVal.Color, 0, nullptr);
        commandList->ClearRenderTargetView(m_GPassNormalsRTV, m_GBuferClrVal.Color, 0, nullptr);
        commandList->ClearRenderTargetView(m_GPassPositionRTV, m_GBuferClrVal.Color, 0, nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { m_GPassAlbedoRTV, m_GPassNormalsRTV, m_GPassPositionRTV };
        commandList->OMSetRenderTargets(static_cast<uint32_t>(std::size(rtvHandles)), rtvHandles, false, &dsv);

        ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
        commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

        commandList->SetGraphicsRootSignature(m_gPassRS.Get());

        constexpr uint32_t SCENE_CONSTANTS_ROOT_INDEX = 2;
        auto sceneCstCBAddress = m_sceneConstantsVirtualAddress + frameIndex * sizeof(SceneConstantsStructPadded);
        commandList->SetGraphicsRootConstantBufferView(SCENE_CONSTANTS_ROOT_INDEX, sceneCstCBAddress);

        // Background mesh is static, so identity for model matrix is ok.
        constexpr uint32_t PER_OBJECT_ROOT_INDEX = 3;
        PerObjectStruct perObjData = {};
        perObjData.worldMatRotation = perObjData.worldMat = Matrix::Identity;
        memcpy(&m_perObjectCBMapped[0].data, &perObjData, sizeof(PerObjectStruct));
        commandList->SetGraphicsRootConstantBufferView(PER_OBJECT_ROOT_INDEX, m_perObjectCBVAddress);

        commandList->SetPipelineState(m_gPassPSO.Get());

        // Drawing all submeshes
        for (size_t i = 0; i < m_model->meshes.size(); ++i)
        {
            auto& opaqueParts = m_model->meshes[i]->opaqueMeshParts;
            for (size_t j = 0; j < opaqueParts.size(); ++j)
            {
                uint32_t materialIndex = opaqueParts[j]->materialIndex;
                int texIndex = m_model->materials[materialIndex].diffuseTextureIndex;
                constexpr uint32_t DIFFUSE_TEX_TABLE_ROOT_INDEX = 0;
                D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_srvPile->GetGpuHandle((size_t)(SCENE_TEXTURE_OFFSET + texIndex));
                commandList->SetGraphicsRootDescriptorTable(DIFFUSE_TEX_TABLE_ROOT_INDEX, srvHandle);
                opaqueParts[j]->Draw(commandList);
            }
        }

        m_gpuTimer->Stop(commandList, static_cast<uint32_t>(GPU_TIMER_GEOMETRY_PASS));
        m_deltaTime[GPU_TIMER_GEOMETRY_PASS] = m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPU_TIMER_GEOMETRY_PASS));
        PIXEndEvent(commandList);
    }

    // For the FSQ we dont use depth
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    // Get resource from the particle system about lights position
    ID3D12Resource* lightPositionsSRVRes = m_particles.GetParticlePositionResource();
    D3D12_GPU_VIRTUAL_ADDRESS lightPositionSRVAddress = lightPositionsSRVRes->GetGPUVirtualAddress();

    // Ambient Pass
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Ambient Pass");
        m_gpuTimer->Start(commandList, static_cast<uint32_t>(GPU_TIMER_AMBIENT));

        // Transition GBuffer's from RTV to SRV
        D3D12_RESOURCE_BARRIER RTVToSRV_Albedo = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceAlbedo.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        D3D12_RESOURCE_BARRIER RTVToSRV_Normals = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourceNormals.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        D3D12_RESOURCE_BARRIER RTVToSRV_Position = CD3DX12_RESOURCE_BARRIER::Transition(m_GBufferResourcePosition.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        D3D12_RESOURCE_BARRIER barriers[3] = { RTVToSRV_Albedo, RTVToSRV_Normals, RTVToSRV_Position };
        commandList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap()};
        commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);
        
        commandList->SetGraphicsRootSignature(m_ambientPassRS.Get());
       
        constexpr uint32_t DIFFUSE_TEX_TABLE_ROOT_INDEX = 0;
        commandList->SetGraphicsRootDescriptorTable(DIFFUSE_TEX_TABLE_ROOT_INDEX, m_GBufferSRHeap->GetFirstGpuHandle());
        
        commandList->SetPipelineState(m_ambientPassPSO.Get());
        commandList->DrawInstanced(3, 1, 0, 0);

        m_gpuTimer->Stop(commandList, static_cast<uint32_t>(GPU_TIMER_AMBIENT));
        m_deltaTime[GPU_TIMER_AMBIENT] = m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPU_TIMER_AMBIENT));
        PIXEndEvent(commandList);
    }

    // Light Volume Pass
    if (m_lightsOn && m_lightTechniqueMode == LIGHT_VOLUMES)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Light Volumes");
        m_gpuTimer->Start(commandList, static_cast<uint32_t>(GPU_TIMER_LIGHT_VOLUMES));

        ScopedBarrier scopeBarrier(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(lightPositionsSRVRes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            });
        
        // Make sure the SRV has views for the 3 gbuffer textures
        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap() };
        commandList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);
        
        commandList->SetGraphicsRootSignature(m_lightVolumePassRS.Get());
        
        // Table with the SRV's, in this case for the gBuffer
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_GBufferSRHeap->GetFirstGpuHandle();
        constexpr uint32_t DIFFUSE_TEX_TABLE_ROOT_INDEX = 0;
        commandList->SetGraphicsRootDescriptorTable(DIFFUSE_TEX_TABLE_ROOT_INDEX, srvHandle);

        constexpr uint32_t SCENE_CONSTANTS_ROOT_INDEX = 1;
        auto sceneCstCBAddress = m_sceneConstantsVirtualAddress + frameIndex * sizeof(SceneConstantsStructPadded);
        commandList->SetGraphicsRootConstantBufferView(SCENE_CONSTANTS_ROOT_INDEX, sceneCstCBAddress);

        constexpr uint32_t LIGHT_BUFFER_ROOT_INDEX = 2;
        commandList->SetGraphicsRootShaderResourceView(LIGHT_BUFFER_ROOT_INDEX, lightPositionSRVAddress);

        commandList->SetPipelineState(m_lightVolumePassPSO.Get());
        m_lightMesh->DrawInstanced(commandList, INSTANCE_COUNT, 0);

        m_gpuTimer->Stop(commandList, static_cast<uint32_t>(GPU_TIMER_LIGHT_VOLUMES));
        m_deltaTime[GPU_TIMER_LIGHT_VOLUMES] = m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPU_TIMER_LIGHT_VOLUMES));
        PIXEndEvent(commandList);
    }
    else if (m_lightsOn)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Light Binning + Tiling");
        GPU_TIMER_PASSES chosen_pass = (m_lightTechniqueMode == TILED) ? GPU_TIMER_TILED_PASS : GPU_TIMER_CLUSTERED_PASS;
        m_gpuTimer->Start(commandList, (uint32_t)chosen_pass);

        ScopedBarrier scopeBarrier(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(lightPositionsSRVRes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            });

        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap() };
        commandList->SetDescriptorHeaps((uint32_t)std::size(heaps), heaps);

        if (m_lightTechniqueMode == TILED)
            commandList->SetComputeRootSignature(m_lightTiledPassRS.Get());
        else if(m_lightTechniqueMode == CLUSTERED)
            commandList->SetComputeRootSignature(m_ClusteringPassRS.Get());
        
        constexpr uint32_t UAV_ROOT_INDEX = 0;
        commandList->SetComputeRootDescriptorTable(UAV_ROOT_INDEX, m_TiledComputeGPUHandle);

        // If we update every frame, we would need to update passing the correct address into the virtual mem, but for now using constant data
        constexpr uint32_t LIGHT_BUFFER_ROOT_INDEX = 1;
        commandList->SetComputeRootShaderResourceView(LIGHT_BUFFER_ROOT_INDEX, lightPositionSRVAddress);

        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_GBufferSRHeap->GetFirstGpuHandle();
        constexpr uint32_t GBUFFER_TABLE_ROOT_INDEX = 2;
        commandList->SetComputeRootDescriptorTable(GBUFFER_TABLE_ROOT_INDEX, srvHandle);

        constexpr uint32_t DEPTH_SRV_ROOT_INDEX = 3;
        commandList->SetComputeRootDescriptorTable(DEPTH_SRV_ROOT_INDEX, m_depthSRVGPUHandle);

        constexpr uint32_t SCENE_CONSTANTS_CB_ROOT_INDEX = 4;
        auto sceneCstCBAddress = m_sceneConstantsVirtualAddress + frameIndex * sizeof(SceneConstantsStructPadded);
        commandList->SetComputeRootConstantBufferView(SCENE_CONSTANTS_CB_ROOT_INDEX, sceneCstCBAddress);

        if (m_lightTechniqueMode == TILED)
        {
            commandList->SetPipelineState(m_lightTiledPassPSO.Get());
        }
        else if (m_lightTechniqueMode == CLUSTERED)
        {
            constexpr uint32_t Z_RANGES_CB_ROOT_INDEX = 5;
            commandList->SetComputeRootConstantBufferView(Z_RANGES_CB_ROOT_INDEX, m_zClusterRangesCBVAddress);

            commandList->SetPipelineState(m_ClusteringPassPSO.Get());
        }

        uint32_t threadGroupCountX = m_displayWidth / GROUP_WIDTH;
        uint32_t threadGroupCountY = m_displayHeight / GROUP_WIDTH;

        if (m_lightTechniqueMode == TILED)
            commandList->Dispatch(threadGroupCountX, threadGroupCountY, 1);
        else if (m_lightTechniqueMode == CLUSTERED)
            commandList->Dispatch(threadGroupCountX, threadGroupCountY, 1);

        m_gpuTimer->Stop(commandList, (uint32_t)chosen_pass);
        m_deltaTime[chosen_pass] = m_gpuTimer->GetAverageMS((uint32_t)chosen_pass);
        PIXEndEvent(commandList);
    }

    //Copy compute contents into backbuffer for final render
    if (m_lightsOn && (m_lightTechniqueMode == TILED || m_lightTechniqueMode == CLUSTERED))
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Copy UAV to RT");
        m_gpuTimer->Start(commandList, (uint32_t)GPU_TIMER_UAV_COPY_TO_RT);

        ScopedBarrier scopeBarrier(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_tiledOutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            });

        auto const outputSize = m_deviceResources->GetOutputSize();

        ID3D12DescriptorHeap* heaps[] = { m_GBufferSRHeap->Heap() };
        commandList->SetDescriptorHeaps(1, heaps);

        m_spriteBatch->Begin(commandList);
        XMUINT2 texSize(uint32_t(outputSize.right), uint32_t(outputSize.bottom));
        XMFLOAT2 texLoc(0, 0);
        m_spriteBatch->Draw(m_TiledComputeGPUHandle, texSize, texLoc);
        m_spriteBatch->End();

        m_gpuTimer->Stop(commandList, (uint32_t)GPU_TIMER_UAV_COPY_TO_RT);
        m_deltaTime[GPU_TIMER_UAV_COPY_TO_RT] = m_gpuTimer->GetAverageMS((uint32_t)GPU_TIMER_UAV_COPY_TO_RT);
        PIXEndEvent(commandList);
    }

    // Draw particles to represent light sources
    if (m_drawParticles)
    {
        commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
        m_particles.Render(commandList, frameIndex, view, proj);
    }

    // UI Pass
    DrawHUD();

    PIXEndEvent(commandList);
    m_gpuTimer->EndFrame(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DrawHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (UI Pass)");
    m_hudBatch->Begin(commandList);

    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMUINT2 texSize(uint32_t(size.right / 4), uint32_t(size.bottom / 3));
    XMFLOAT2 texLoc(float(safe.left), float(safe.top));
    m_hudBatch->Draw(m_srvPile->GetGpuHandle(15), texSize, texLoc, Vector4(1.0f, 1.0f, 1.0f, 0.6f));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textColorEnabled = DirectX::Colors::Aquamarine;
    const XMVECTOR textColorDisabled = DirectX::Colors::DarkKhaki;
    const XMVECTOR textTimerColor = DirectX::Colors::AntiqueWhite;
    const XMVECTOR textColor = true ? textColorEnabled : textColorDisabled;

    wchar_t buffer[100];
    {
        wchar_t const* lightTechniqueName = g_wstrLightTechniquesNames[m_lightTechniqueMode];
        uint32_t const deltaArrayIndex = m_lightTechniqueMode + 2u;
        double const timeElapsedInPassMS = m_deltaTime[deltaArrayIndex];

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"Advanced Lighting", textPos, textColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Current state section
        swprintf_s(buffer, std::size(buffer), L"Technique: %s", lightTechniqueName);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Lights %s", (m_lightsOn) ? L"On" : L"Off");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Draw Light Sources %s", (m_drawParticles) ? L"On" : L"Off");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Light Radius: %d", m_lightDiameter / 2);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Light Count: %d", INSTANCE_COUNT);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Timer section (render at bottom of screen)
        if (timeElapsedInPassMS < 0)
            swprintf_s(buffer, std::size(buffer), L"Time (ms) during %s pass: -", lightTechniqueName);
        else
            swprintf_s(buffer, std::size(buffer), L"Time (ms) during %s pass: %.2f", lightTechniqueName, timeElapsedInPassMS);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Frame Time: %.2f", m_frameDeltaTime);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (7.0f * m_smallFont->GetLineSpacing());

        // Instructions
#ifdef _GAMING_DESKTOP
        swprintf_s(buffer, std::size(buffer), L"[Z]/[X] To switch Technique");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Enter] Toggle Light Source Rendering");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Space] Toggle lights");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[C]/[V]: Modify Light Radius");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Esc] Exit Sample");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
#else // _GAMING_XBOX 
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A]/[B] To switch Technique", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[RB] Toggle Light Source Rendering", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LB] Toggle lights", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[DPad] Left/Right: Modify Light Radius", textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor);
#endif

        // After print, set to -1 so next frame we know if this pass took place
        m_deltaTime[deltaArrayIndex] = -1.0f;
    }

    m_hudBatch->End();
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

// Updates the CBuffer which is used my multiples draw/dispatches, but remains constant for the whole frame
void Sample::UpdateSceneConstants(Matrix const& view, Matrix const& proj, uint32_t frameIndex)
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
    sceneConstantsCB.sceneLightCount = INSTANCE_COUNT;
    sceneConstantsCB.tileCorrectScreenWidth = ((m_displayWidth + GROUP_WIDTH - 1) / GROUP_WIDTH) * GROUP_WIDTH;
    sceneConstantsCB.tileCorrectScreenHeight = ((m_displayHeight + GROUP_WIDTH - 1) / GROUP_WIDTH) * GROUP_WIDTH;
    sceneConstantsCB.zMinView = 1.0f / (0.0f * invProj._34 + invProj._44);
    sceneConstantsCB.zMaxView = 1.0f / (1.0f * invProj._34 + invProj._44);

    memcpy(&m_sceneConstantsMappedMem[frameIndex].data, &sceneConstantsCB, sizeof(SceneConstantsStruct));
}

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

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1920;
    height = 1080;
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
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Particle System
    m_particles.initialize(m_deviceResources, INSTANCE_COUNT);

    // Full screen Quad
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    // Create PSOs
    CreatePipelineStates();

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
        DX::ThrowIfFailed(m_sceneConstantsResource->Map(0, nullptr, reinterpret_cast<void**>(&m_sceneConstantsMappedMem)));
    }

    // CB for per-object data (world matrix)
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // We call draw on each submesh, but they all share a model matrix.
        // Since the mesh does not move, we can allocate space for one struct
        uint32_t bufferSize = sizeof(PerObjectStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_perObjectCBResource.ReleaseAndGetAddressOf())));
    
        DX::ThrowIfFailed(m_perObjectCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_perObjectCBMapped)));
        m_perObjectCBVAddress = m_perObjectCBResource->GetGPUVirtualAddress();
    }

    // CB for ZRanges (clustered)
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        uint32_t bufferSize = sizeof(zClusterRangesCBPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_zClusterRangesCBResource.ReleaseAndGetAddressOf())));
    
        m_zClusterRangesCBVAddress = m_zClusterRangesCBResource->GetGPUVirtualAddress();

        DX::ThrowIfFailed(m_zClusterRangesCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_zClusterRangesCBMapped)));
        zClusterRangesCB zRangesData = {};
        for (uint32_t i = 0; i < CLUSTERS_Z_COUNT; ++i)
        {
            zRangesData.zClusterRanges[i].range.x = (float)i / CLUSTERS_Z_COUNT;
            zRangesData.zClusterRanges[i].range.y = ((float)i + 1) / CLUSTERS_Z_COUNT;
        }
        memcpy(&m_zClusterRangesCBMapped->data, &zRangesData, sizeof(zClusterRangesCB));
        m_zClusterRangesCBResource->Unmap(0, nullptr);
    }

    // Create the gpu timer
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    // Load Model and other stuff
    std::wstring mediaDirectory;
    {
        m_srvPile = std::make_unique<DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 128, 0);

        {
            wchar_t s_meshFilename[1024];
            wchar_t filepath[1024];
            _snwprintf_s(s_meshFilename, std::size(s_meshFilename), _TRUNCATE, L"AbstractCathedral.sdkmes_");

            DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), s_meshFilename, s_folderPaths);

            // Store the media directory
            std::wstring pathTemp = filepath;
            mediaDirectory = pathTemp.substr(0, pathTemp.find_last_of('\\'));

            auto modelBlob = DX::ReadCompressedData(filepath);
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
            m_srvPile->AllocateRange(2, start, end);
            assert(TEXT_FONT_OFFSET == start && L"Range does not match.");
            assert(CONTROLLER_FONT_OFFSET == (start + 1) && L"Range does not match.");

            wchar_t strFilePath[MAX_PATH] = {};
            DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
            m_smallFont = std::make_unique<SpriteFont>(device, upload,
                strFilePath,
                m_srvPile->GetCpuHandle(TEXT_FONT_OFFSET),
                m_srvPile->GetGpuHandle(TEXT_FONT_OFFSET));

            DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
            m_ctrlFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
                m_srvPile->GetCpuHandle(CONTROLLER_FONT_OFFSET),
                m_srvPile->GetGpuHandle(CONTROLLER_FONT_OFFSET));
        }

        m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, m_srvPile->Heap());
        size_t texOffsets;
        if (!m_model->textureNames.empty())
        {
            m_texFactory->SetDirectory(mediaDirectory.c_str());

            size_t end;
            m_srvPile->AllocateRange(m_model->textureNames.size(), texOffsets, end);
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

    // Light Volume's (Instanced) PSO
    {
        auto vsBlob = DX::ReadData(L"InstancedLightVolumesVS.cso");
        auto psBlob = DX::ReadData(L"InstancedLightVolumesPS.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_lightVolumePassRS.ReleaseAndGetAddressOf())));

        D3D12_INPUT_ELEMENT_DESC gElemDescPositionLV = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        D3D12_INPUT_ELEMENT_DESC gInputElemArrLV[] = { gElemDescPositionLV };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArrLV, static_cast<uint32_t>(std::size(gInputElemArrLV)) };
        psoDesc.pRootSignature = m_lightVolumePassRS.Get();
        psoDesc.VS = { vsBlob.data(), vsBlob.size() };
        psoDesc.PS = { psBlob.data(), psBlob.size() };

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

        // We don't want to test depth, since it would fail when the sphere overlaps planes.
        // We also do not want to write depth, since this spheres do not occlude.
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT32_MAX;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_lightVolumePassPSO.ReleaseAndGetAddressOf())));
        m_lightVolumePassPSO->SetName(L"LightVolumePassPSO");
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

    // Clustered Pass PSO
    {
        auto csBlob = DX::ReadData(L"ClusteredCompute.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_ClusteringPassRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_ClusteringPassRS.Get();
        psoDesc.CS = { csBlob.data(), csBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_ClusteringPassPSO.ReleaseAndGetAddressOf())));
        m_ClusteringPassPSO->SetName(L"ClusteringPassPSO");
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint64_t>(size.right - size.left);
    m_displayHeight = static_cast<uint64_t>(size.bottom - size.top);

    // Create Resources, RTVs and SRVs for the G-Buffer, needs to go here since GBuffer depends on window size
    CreateGeometryBufferResourcesAndViews(m_displayWidth, m_displayHeight);

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

        // UAV for the tiled pass output
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = m_deviceResources->GetBackBufferFormat();
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        positionInHeap = 4;
        auto tiledComputeCpuHandle = m_GBufferSRHeap->GetCpuHandle(positionInHeap);
        device->CreateUnorderedAccessView(m_tiledOutputResource.Get(), nullptr, &uavDesc, tiledComputeCpuHandle);
        m_TiledComputeGPUHandle = m_GBufferSRHeap->GetGpuHandle(positionInHeap);
    }
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_srvPile.reset();
    m_GBufferRTHeap.reset();
    m_GBufferSRHeap.reset();
    m_model.reset();
    m_texFactory.reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
    m_cpuTimer.reset();
    m_gpuTimer.reset();

    // Particles
    m_particles.Reset();

    // G-Buffer Resources
    m_GBufferResourceAlbedo.Reset();
    m_GBufferResourceNormals.Reset();
    m_GBufferResourcePosition.Reset();
    m_tiledOutputResource.Reset();
    m_perObjectCBResource.Reset();
    m_zClusterRangesCBResource.Reset();
    
    m_gPassRS.Reset();
    m_ambientPassRS.Reset();
    m_lightVolumePassRS.Reset();
    m_lightTiledPassRS.Reset();
    m_ClusteringPassRS.Reset();

    m_gPassPSO.Reset();
    m_ambientPassPSO.Reset();
    m_lightVolumePassPSO.Reset();
    m_lightTiledPassPSO.Reset();
    m_ClusteringPassPSO.Reset();
    m_sceneConstantsResource.Reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
