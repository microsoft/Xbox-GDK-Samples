//--------------------------------------------------------------------------------------
// ShadowMapSample.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ShadowMapSample.h"
#include "Utils.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    // Sample default width and height
    constexpr uint32_t DEFAULT_WIDTH = 1920;
    constexpr uint32_t DEFAULT_HEIGHT = 1080;

    // Auxiliary shadow variables
    MSAA_LEVEL msaaLvl = MSAA_LEVEL_NONE;
    DEPTH_REMAPPING filterChoice = DEPTH_REMAPPING_STANDARD;
    BLUR_TYPE blurType = BLUR_TYPE_GAUSSIAN;
    SHADOW_MAP_DIMS shadowMapDims = SHADOW_MAP_DIMS_1024x1024;
    SHADOW_CULL shadowCullMode = SHADOW_CULL_FRONT;
    BLUR_KERNEL kernelSize = BLUR_KERNEL_2x2;
    PCF_MODE pcfMode = PCF_MODE_NONE;
    bool shadowsOn = true;

    // Directional light starting parameters
    constexpr Vector4 AMBIENT_COLOR = Vector4(0.05f, 0.05f, 0.05f, 1.0f);
    constexpr float DEPTH_BIAS = 0.005f;
    constexpr float SPECULAR_PWR = 0.01f;
    constexpr Vector4 LIGHT_COLOR = Vector4(0.7f, 0.7f, 0.5f, 1.0f);
    constexpr Vector4 LIGHT_POS = Vector4(0, -80, -300, 1);
    constexpr Vector4 LIGHT_FWD = Vector4(0, -1, 1, 0);
    constexpr float LIGHT_NEAR = 10.0f;
    constexpr float LIGHT_FAR = 150.0f;
    constexpr float LIGHT_VIEWPORT_WIDTH = 250.0f;
    constexpr float LIGHT_VIEWPORT_HEIGHT = 250.0f;

    // Fly-Camera
    constexpr Vector3 CAMERA_START_POS = Vector3(0, -90, -60);
    constexpr float CAMERA_NEAR = 0.25f;
    constexpr float CAMERA_FAR = 1000.0f;
}


Sample::Sample() noexcept(false) :
    m_frame(0),
    m_deltaTime{},
    m_shadowViewport{},
    m_shadowScissorRect{},
    m_dirLightCBAddress{},
    m_dirLightCBMappedMem{},
    m_remapCBAddress{},
    m_remapCBMappedMem{},
    m_resolveFSQVertexBuffer{},
    m_resolveFSQIndexBuffer{},
    m_lightMeshCBMappedMem{},
    m_sceneConstantsCBMappedMem{},
    m_sceneConstantsCBAddress{},
    m_pActiveScene{}
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_GeometryShaders);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    // Delete Scene
    if (m_pActiveScene)
    {
        delete m_pActiveScene;
    }

    // Unmaps
    if (m_dirLightCBResource)
    {
        m_dirLightCBResource->Unmap(0, nullptr);
    }
    if (m_remapCBResource)
    {
        m_remapCBResource->Unmap(0, nullptr);
    }
    if (m_lightMeshCBResource)
    {
        m_lightMeshCBResource->Unmap(0, nullptr);
    }
    if (m_sceneConstantsCBResource)
    {
        m_sceneConstantsCBResource->Unmap(0, nullptr);
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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;

        m_gamePadButtons.Update(pad);

        // For now
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (pad.IsDPadUpPressed())
        {
            m_dirLight.lightPosition += m_dirLight.lightForward;
        }
        if (pad.IsDPadDownPressed())
        {
            m_dirLight.lightPosition -= m_dirLight.lightForward;
        }

        // Change PCF mode - If this is selected, it will turn off any filtering technique and MSAA first
        // Since kernel might be in a invalid state, this also corrects that case
        if (m_gamePadButtons.y == ButtonState::RELEASED)
        {
            if (msaaLvl != MSAA_LEVEL_NONE)
            {
                ChangeMsaaLevel(MSAA_LEVEL_NONE);
            }

            if (filterChoice != DEPTH_REMAPPING_STANDARD)
            {
                filterChoice = DEPTH_REMAPPING_STANDARD;
            }

            // For PCF, we only allow 2x2 and up
            kernelSize = (kernelSize == BLUR_KERNEL_1x1) ? BLUR_KERNEL_2x2 : kernelSize;

            uint32_t curr = pcfMode;
            curr = (curr + 1) % PCF_MODE_COUNT;
            pcfMode = (PCF_MODE)curr;
        }
        // Kernel Size - This acts different for PCF and for filtering, since PCF does not support 1x1 kernel
        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            if (IsUsingPCF(pcfMode))
            {
                uint32_t curr = kernelSize;
                curr = (curr + 1) % BLUR_KERNEL_COUNT;
                kernelSize = (BLUR_KERNEL)std::max<uint32_t>(curr, 1);
            }
            else
            {
                kernelSize = (BLUR_KERNEL)((uint32_t(kernelSize) + 1) % BLUR_KERNEL_COUNT);
            }
        }
        // Change Filter technique. If this is selected, it will first leave PCF as none, since both together are not compatible
        if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            pcfMode = PCF_MODE_NONE;
            filterChoice = (DEPTH_REMAPPING)((uint32_t(filterChoice) + 1) % DEPTH_REMAPPING_COUNT);
            if (filterChoice == DEPTH_REMAPPING_STANDARD)
            {
                ChangeMsaaLevel(MSAA_LEVEL_NONE);
            }
        }
        // Change MSAA level. Only relevant if a filtering technique is active
        if (m_gamePadButtons.x == ButtonState::RELEASED)
        {
            if (IsFilterCorrect(filterChoice))
            {
                uint32_t newLvl = (uint32_t(msaaLvl) + 1) % MSAA_LEVEL_COUNT;
                ChangeMsaaLevel((MSAA_LEVEL)newLvl);
            }
        }
        // Change shadowmap dimension. Requires regenerating all shadow resources
        if (m_gamePadButtons.leftShoulder == ButtonState::RELEASED)
        {
            uint32_t newDim = (uint32_t(shadowMapDims) + 1) % SHADOW_MAP_DIMS_COUNT;
            shadowMapDims = (SHADOW_MAP_DIMS)newDim;

            m_deviceResources->WaitForGpu();

            InitializeShadowParameters(m_deviceResources->GetD3DDevice(), m_deviceResources->GetDepthBufferFormat());
        }
        // Change shadow cull mode
        if (m_gamePadButtons.rightShoulder == ButtonState::RELEASED)
        {
            uint32_t cullmode = (uint32_t(shadowCullMode) + 1) % SHADOW_CULL_COUNT;
            shadowCullMode = (SHADOW_CULL)std::max<uint32_t>(cullmode, 1);

            // Only ShadowMap PSO cares about the shadowCullMode
            SetupShadowMapPSO();
        }
        // Turns all shadows off
        if (m_gamePadButtons.start == ButtonState::RELEASED)
        {
            shadowsOn = !shadowsOn;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
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

    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();

    UpdateConstantResources(commandList);

    // Since all passes use the same descriptor pile, we should be able to just bind this at the start and not change it
    ID3D12DescriptorHeap* shaderHeaps[1] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(1, shaderHeaps);

    // Begin frame timer
    m_gpuTimer->BeginFrame(commandList);

    auto dsv = m_deviceResources->GetDepthStencilView();
    auto rtv = m_deviceResources->GetRenderTargetView();

    uint32_t sceneConstantBindIdx = m_deviceResources->GetCurrentFrameIndex() * m_sceneConstantsNumBindings;

    /////////////////////
    ///  Shadow Pass  ///
    /////////////////////
    if (shadowsOn)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (Shadow Pass)");
        m_gpuTimer->Start(commandList, (uint32_t)SHADOW_MAP_PASS);

        // Transition resource from Shader resource to Depth Write
        auto SRVToDepthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_shadowTexResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(1, &SRVToDepthBarrier);

        auto shadowDSV = m_shadowDSHeap->GetCPUDescriptorHandleForHeapStart();

        // Clear shadow DSV and set shadow viewport
        commandList->ClearDepthStencilView(shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        commandList->RSSetViewports(1, &m_shadowViewport);
        commandList->RSSetScissorRects(1, &m_shadowScissorRect);

        commandList->OMSetRenderTargets(0, nullptr, TRUE, &shadowDSV);

        commandList->SetPipelineState(m_shadowPassPSO.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        // Since we are rendering from light's point of view, we need to bind the matrices into CB
        CBSceneConstStruct sceneConst = {};
        sceneConst.g_mView = m_dirLight.dirLightView.Transpose();
        sceneConst.g_mProj = m_dirLight.dirLightProj.Transpose();
        memcpy(&m_sceneConstantsCBMappedMem[sceneConstantBindIdx].data, &sceneConst, sizeof(CBSceneConstStruct));
        auto baseGpuAddress = m_sceneConstantsCBAddress + sizeof(CBSceneConstStructPadded) * sceneConstantBindIdx;
        commandList->SetGraphicsRootConstantBufferView(SCENE_CONSTANTS_CBV, baseGpuAddress);

        //Render the scene
        m_pActiveScene->Render(commandList, m_srvPile);

        m_gpuTimer->Stop(commandList, (uint32_t)SHADOW_MAP_PASS);
        m_deltaTime[SHADOW_MAP_PASS] = m_gpuTimer->GetAverageMS((uint32_t)SHADOW_MAP_PASS);
        PIXEndEvent(commandList);
    }


    ///////////////////////
    ///  RESOLVE PASS   ///
    ///////////////////////
    if (shadowsOn && IsMsaa(msaaLvl, filterChoice))
    {
        // Setup
        ID3D12Resource* RTVResource = nullptr;
        uint32_t rtvIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
        switch (filterChoice)
        {
        case DEPTH_REMAPPING_VARIANCE:
            RTVHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeapVariance->GetCPUDescriptorHandleForHeapStart(), 0, rtvIncrementSize);
            RTVResource = m_resourceVariance[0].Get();
            break;
        case DEPTH_REMAPPING_EXPONENTIAL:
            RTVHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeapExponential->GetCPUDescriptorHandleForHeapStart(), 0, rtvIncrementSize);
            RTVResource = m_resourceExponential[0].Get();
            break;
        case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
            RTVHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeapExpVariance->GetCPUDescriptorHandleForHeapStart(), 0, rtvIncrementSize);
            RTVResource = m_resourceExpVariance[0].Get();
            break;
        default:
            throw std::runtime_error("Depth Remapping mode unknown.");
            break;
        };

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (Resolve Pass)");
        m_gpuTimer->Start(commandList, (uint32_t)RESOLVE_PASS);

        // This pass Resolves the shadowmap. Since we are filtering, it also leaves the resource ready in the
        // format required by the filtering pass (for variance, with depth and depth sqr)

        // transition varianceResource to RT
        D3D12_RESOURCE_BARRIER DepthWriteToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            m_shadowTexResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        D3D12_RESOURCE_BARRIER resolveDstBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            RTVResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_RESOURCE_BARRIER pBarriers[2] = { DepthWriteToSRV, resolveDstBarrier };
        commandList->ResourceBarrier(2, pBarriers);
       
        // Fire a draw call with a fullscreen quad, and the resolve pixel shader
        commandList->SetPipelineState(m_resolvePassPSO[filterChoice].Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        // Set the render target view and the DSV
        commandList->OMSetRenderTargets(1, &RTVHandle, true, nullptr);

        // Input layout stuff
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        commandList->IASetVertexBuffers(0, 1, &m_resolveFSQVertexBuffer);
        commandList->IASetIndexBuffer(&m_resolveFSQIndexBuffer);

        D3D12_GPU_DESCRIPTOR_HANDLE shadowSRVHandle = m_srvPile->GetGpuHandle(SHADOW_MAP_OFFSET);
        commandList->SetGraphicsRootDescriptorTable(SHADOW_TEX_TABLE, shadowSRVHandle);

        commandList->DrawIndexedInstanced(1, 1, 0, 0, 0); // Find out how others launch GS

        m_gpuTimer->Stop(commandList, (uint32_t)RESOLVE_PASS);
        m_deltaTime[RESOLVE_PASS] = m_gpuTimer->GetAverageMS((uint32_t)RESOLVE_PASS);
        PIXEndEvent(commandList);
    }


    ////////////////////////
    ///  FILTERING PASS  ///
    ////////////////////////
    if (shadowsOn && IsFilterCorrect(filterChoice))
    {
        ID3D12Resource* filterResourceHrz = nullptr;
        ID3D12Resource* filterResourceVrt = nullptr;
        ID3D12DescriptorHeap* filterRTVHeap = nullptr;
        uint32_t filterSRVOffset_1 = 0;
        switch (filterChoice)
        {
        case DEPTH_REMAPPING_VARIANCE:
            filterResourceHrz = m_resourceVariance[0].Get();
            filterResourceVrt = m_resourceVariance[1].Get();
            filterRTVHeap = m_RTVHeapVariance.Get();
            filterSRVOffset_1 = VARIANCE_PASS1_OFFSET;
            break;
        case DEPTH_REMAPPING_EXPONENTIAL:
            filterResourceHrz = m_resourceExponential[0].Get();
            filterResourceVrt = m_resourceExponential[1].Get();
            filterRTVHeap = m_RTVHeapExponential.Get();
            filterSRVOffset_1 = EXPONENTIAL_PASS1_OFFSET;
            break;
        case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
            filterResourceHrz = m_resourceExpVariance[0].Get();
            filterResourceVrt = m_resourceExpVariance[1].Get();
            filterRTVHeap = m_RTVHeapExpVariance.Get();
            filterSRVOffset_1 = EXP_VAR_PASS1_OFFSET;
            break;
        default:
            throw std::runtime_error("Depth Remapping mode unknown.");
            break;
        };

        // Setup. Here we will set the SRV's depending on whether the shadow map was rendered with or wout MSAA
        SRV_HEAP_OFFSETS SRVHeapsOffsets[2] = { SHADOW_MAP_OFFSET, (SRV_HEAP_OFFSETS)(filterSRVOffset_1 + 1) };
        ID3D12PipelineState* pipelines[2] = {
            m_filterHrzPassPSO[1][filterChoice][kernelSize][blurType].Get(),
            m_filterVrtPassPSO[filterChoice][kernelSize][blurType].Get()
        };
        ID3D12Resource* SRVResources[2] = { m_shadowTexResource.Get(), filterResourceVrt };
        D3D12_RESOURCE_STATES SRVStateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;

        if (IsMsaa(msaaLvl, filterChoice))
        {
            pipelines[0] = m_filterHrzPassPSO[0][filterChoice][kernelSize][blurType].Get();
            SRVHeapsOffsets[0] = (SRV_HEAP_OFFSETS)filterSRVOffset_1;
            SRVResources[0] = filterResourceHrz;
            SRVStateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (Filter Pass)");
        m_gpuTimer->Start(commandList, (uint32_t)FILTERING_PASS);
        wchar_t const* subpasses[2] = { L"Horizontal Pass", L"Vertical Pass" };

        // Two passes: First we blur in x direction, then in y direction
        for (size_t i = 0; i < 2; ++i)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, subpasses[i]);

            // Transition RTV[1-i] to be an SRV, since we will sample from it on the filtering pass
            auto transitionToSRV = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources[i], SRVStateBefore, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            auto targetToRTV = CD3DX12_RESOURCE_BARRIER::Transition((i == 0) ? filterResourceVrt : filterResourceHrz,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            D3D12_RESOURCE_BARRIER barriers[2] = { transitionToSRV, targetToRTV };
            commandList->ResourceBarrier(2, barriers);

            // Setup the correct RT for this pass
            uint32_t rtvIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); //Duped
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE((filterRTVHeap->GetCPUDescriptorHandleForHeapStart()), 1-(int)i, rtvIncrementSize);
            commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, nullptr);

            // PSO and RS - these pass, 0 is horizontal, 1 is vertical
            commandList->SetPipelineState(pipelines[i]);
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // Input layout stuff
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
            commandList->IASetVertexBuffers(0, 1, &m_resolveFSQVertexBuffer);
            commandList->IASetIndexBuffer(&m_resolveFSQIndexBuffer);

            // constant buffer for linearizing depth
            commandList->SetGraphicsRootConstantBufferView(REMAPPING_CBV, m_remapCBAddress);

            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_srvPile->GetGpuHandle(SRVHeapsOffsets[i]);
            commandList->SetGraphicsRootDescriptorTable(SHADOW_TEX_TABLE, srvHandle);

            // Dispatch Draw
            commandList->DrawIndexedInstanced(1, 1, 0, 0, 0);

            // for the transition at the start of the loop, since 2d pass always takes the previous RT
            SRVStateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

            PIXEndEvent(commandList);
        }

        m_gpuTimer->Stop(commandList, (uint32_t)FILTERING_PASS);
        m_deltaTime[FILTERING_PASS] = m_gpuTimer->GetAverageMS((uint32_t)FILTERING_PASS);
        PIXEndEvent(commandList);
    }


    //////////////////////
    ///  SHADING Pass  ///
    //////////////////////
    {
        uint32_t SRVHeapOffset = SHADOW_MAP_OFFSET;
        ID3D12Resource* SRVResources = m_shadowTexResource.Get();
        D3D12_RESOURCE_STATES SRVStateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        ID3D12PipelineState* pipeline = m_shadingPassNoPCFPSO[DEPTH_REMAPPING_STANDARD].Get();

        // Setup of resources and pipeline will depend on current config
        if (shadowsOn)
        {
            // Setup. Here we will set the SRV's depending on whether we used filtering or PCF (or none)
            if (IsFilterCorrect(filterChoice))
            {
                switch (filterChoice)
                {
                case DEPTH_REMAPPING_VARIANCE:
                    SRVResources = m_resourceVariance[0].Get();
                    SRVHeapOffset = VARIANCE_PASS1_OFFSET;
                    break;
                case DEPTH_REMAPPING_EXPONENTIAL:
                    SRVResources = m_resourceExponential[0].Get();
                    SRVHeapOffset = EXPONENTIAL_PASS1_OFFSET;
                    break;
                case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
                    SRVResources = m_resourceExpVariance[0].Get();
                    SRVHeapOffset = EXP_VAR_PASS1_OFFSET;
                    break;
                default:
                    throw std::runtime_error("Depth Remapping mode unknown.");
                    break;
                };

                pipeline = m_shadingPassNoPCFPSO[filterChoice].Get();
                SRVStateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
            else if (IsUsingPCF(pcfMode))
            {
                switch (pcfMode)
                {
                case PCF_MODE_GATHERCMP:
                    pipeline = m_shadingPassPCFGatherCmpPSO[kernelSize - 1][blurType].Get();
                    break;
                case PCF_MODE_SAMPLECMP_STEP1:
                    pipeline = m_shadingPassPCFSampleCmpStep1PSO[kernelSize - 1][blurType].Get();
                    break;
                case PCF_MODE_SAMPLECMP_STEP2:
                    pipeline = m_shadingPassPCFSampleCmpStep2PSO[kernelSize - 1][blurType].Get();
                    break;
                default:
                    throw std::runtime_error("PCF mode unknown.");
                    break;
                }
            }
        }
        else
        {
            pipeline = m_litWithoutShadowsPSO.Get();
        }

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (Shading Pass)");
        m_gpuTimer->Start(commandList, (uint32_t)SHADING_PASS);

        Clear();

        commandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);

        commandList->SetPipelineState(pipeline);
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        // Scene view-proj, now from camera's point of view
        CBSceneConstStruct sceneConst = {};
        sceneConst.g_mView = Matrix(m_camera.GetView()).Transpose();
        sceneConst.g_mProj = Matrix(m_camera.GetProjection()).Transpose();
        sceneConstantBindIdx++;
        memcpy(&m_sceneConstantsCBMappedMem[sceneConstantBindIdx].data, &sceneConst, sizeof(CBSceneConstStruct));
        auto baseGpuAddress = m_sceneConstantsCBAddress + sizeof(CBSceneConstStructPadded) * sceneConstantBindIdx;
        commandList->SetGraphicsRootConstantBufferView(SCENE_CONSTANTS_CBV, baseGpuAddress);

        // Directional light cbuffer. We pass it even with shadows off since it has info for scene shading
        baseGpuAddress = m_dirLightCBAddress + sizeof(CBLightStructPadded) * m_deviceResources->GetCurrentFrameIndex();
        commandList->SetGraphicsRootConstantBufferView(LIGHT_TRANSFORM_CBV, baseGpuAddress);

        if (shadowsOn)
        {
            // CB for remapping (linearize depth)
            commandList->SetGraphicsRootConstantBufferView(REMAPPING_CBV, m_remapCBAddress);

            // Transition resource used for shadows (either the shadow map or the filtered texture)
            auto transitionToSRV = CD3DX12_RESOURCE_BARRIER::Transition(SRVResources, SRVStateBefore, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            D3D12_RESOURCE_BARRIER tempbarriers[1] = { transitionToSRV };
            commandList->ResourceBarrier(1, tempbarriers);

            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_srvPile->GetGpuHandle(SRVHeapOffset);
            commandList->SetGraphicsRootDescriptorTable(SHADOW_TEX_TABLE, srvHandle);
        }

        m_pActiveScene->Render(commandList, m_srvPile);

        m_gpuTimer->Stop(commandList, (uint32_t)SHADING_PASS);
        m_deltaTime[SHADING_PASS] = m_gpuTimer->GetAverageMS((uint32_t)SHADING_PASS);
        PIXEndEvent(commandList);
    }

    ///////////////////
    ///   UI Pass   ///
    ///////////////////
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (UI Pass)");
        m_hudBatch->Begin(commandList);

        auto size = m_deviceResources->GetOutputSize();
        auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));
        
        XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
        const XMVECTOR textColorEnabled = DirectX::Colors::Aquamarine;
        const XMVECTOR textColorDisabled = DirectX::Colors::DarkKhaki;
        const XMVECTOR textTimerColor = DirectX::Colors::Crimson;
        const XMVECTOR textColor = shadowsOn ? textColorEnabled : textColorDisabled;

        wchar_t buffer[100];

        wchar_t const* remapName = g_wstrDepthRemappingNames[filterChoice];
        swprintf_s(buffer, std::size(buffer), L"(B) - RemapFunction: %s", remapName);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        wchar_t const* PCFName = g_wstrPCFModeNames[pcfMode];
        swprintf_s(buffer, std::size(buffer), L"(Y) - PCF Mde: %s", PCFName);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        wchar_t const* MSAAName = g_wstrMSAALevelNames[msaaLvl];
        swprintf_s(buffer, std::size(buffer), L"(X) - MSAA level: %s", MSAAName);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, IsFilterCorrect(filterChoice) ? textColor : textColorDisabled);
        textPos.y += m_smallFont->GetLineSpacing();

        wchar_t const* kernelName = g_wstrBlurKernelNames[kernelSize];
        swprintf_s(buffer, std::size(buffer), L"(A) - Kernel size: %s", kernelName);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        wchar_t const* shadowDims = g_wstrShadowMapDimsNames[shadowMapDims];
        swprintf_s(buffer, std::size(buffer), L"(LB) - Shadowmap Dimensions: %s", shadowDims);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        wchar_t const* shadowCull = g_wstrShadowCullNames[shadowCullMode];
        swprintf_s(buffer, std::size(buffer), L"(RB) - Shadow Culling: %s", shadowCull);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"(Start) - Shadows: %s", shadowsOn ? L"ON" : L"OFF");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, shadowsOn ? textColorDisabled : textColorEnabled);
        textPos.y += m_smallFont->GetLineSpacing();

        // Per-Pass Timer Data
        textPos.y += 20 * m_smallFont->GetLineSpacing();
        for (size_t i = 0; i < RENDER_PASSES_COUNT; ++i)
        {
            wchar_t const* renderPassName = g_wstrRenderPassNames[i];
            double const timeElapsedInPassMS = m_deltaTime[i];

            // Print '-' for a pass that did not take place
            if (timeElapsedInPassMS < 0)
                swprintf_s(buffer, std::size(buffer), L"Time (ms) elapsed during %s pass: -", renderPassName);
            else
                swprintf_s(buffer, std::size(buffer), L"Time (ms) elapsed during %s pass: %f", renderPassName, timeElapsedInPassMS);

            m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textTimerColor);
            textPos.y += m_smallFont->GetLineSpacing();

            // After print, set to -1 so next frame we know if this pass took place
            m_deltaTime[i] = -1.0f;
        }
        
        m_hudBatch->End();
        PIXEndEvent(commandList);
    }

    /////////////////////////
    ///  LIGHT MESH Pass  ///
    /////////////////////////
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render (DirLight mesh Pass)");

        commandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_lightMeshPSO.Get());

        CBTransformStruct temp = {};
        Matrix lightworld = XMMatrixIdentity();
        lightworld._41 = m_dirLight.lightPosition.x;
        lightworld._42 = m_dirLight.lightPosition.y;
        lightworld._43 = m_dirLight.lightPosition.z;
        temp.g_mWorld = lightworld.Transpose();
        memcpy(&m_lightMeshCBMappedMem->data, &temp, sizeof(CBTransformStruct));

        commandList->SetGraphicsRootConstantBufferView(OBJ_TRANSFORM_CBV, m_lightMeshCBResource->GetGPUVirtualAddress());

        // Set the texture for this light
        uint32_t offsetIntoHeap = FIXED_PORTION_OFFSET;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_srvPile->GetGpuHandle(offsetIntoHeap);
        commandList->SetGraphicsRootDescriptorTable(DIFFUSE_TEX_TABLE, srvHandle);

        m_lightMesh->DrawInstanced(commandList, 1);

        PIXEndEvent(commandList);
    }

    m_gpuTimer->EndFrame(commandList);

    // Present the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Updates the values of the resource for each scene
void Sample::UpdateConstantResources(ID3D12GraphicsCommandList *commandList)
{
    // Constant buffer for scene
    GraphicsResource tempResource = m_graphicsMemory->AllocateConstant<CBTransformStruct>();
    CBTransformStruct* mappedMem = static_cast<CBTransformStruct*>(tempResource.Memory());
    mappedMem->g_mWorld = Matrix::Identity; // No need to transpose this since its identity
    TransitionResource(commandList, m_pActiveScene->m_obj.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(m_pActiveScene->m_obj.ConstantBuffer.Get(), 0, tempResource.Resource(), tempResource.ResourceOffset(), tempResource.Size());
    TransitionResource(commandList, m_pActiveScene->m_obj.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    
    // Directional Light Parameters Update
    {
        // Update LightInfo values
        m_dirLight.UpdateMatrices();

        // Fill out the Constant buffer
        CBLightStruct dirLight = {};
        dirLight.g_vAmbientColor = AMBIENT_COLOR;
        dirLight.g_vEye = m_dirLight.lightPosition;
        dirLight.g_LightData[0].m_fDepthBias = DEPTH_BIAS;
        dirLight.g_LightData[0].m_fSpecularPower = SPECULAR_PWR;
        dirLight.g_LightData[0].m_mLightViewProj = (m_dirLight.dirLightView * m_dirLight.dirLightProj).Transpose();
        dirLight.g_LightData[0].m_vLightWorldDir = Vector4(m_dirLight.lightForward);
        dirLight.g_LightData[0].m_vLightColor = LIGHT_COLOR;

        // Copy the data into the mapped memory so the resource is pointing at it
        memcpy(&m_dirLightCBMappedMem[m_deviceResources->GetCurrentFrameIndex()].data, &dirLight, sizeof(dirLight));
    }
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
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

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
    width = DEFAULT_WIDTH;
    height = DEFAULT_HEIGHT;
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        0);
    m_srvPile->Heap()->SetName(L"SRVPileHeap");

    // Load shaders and create all the PSOs
    SetRootSignature();
    PipelineStateSetup();

    // Some directional light params
    m_dirLight.lightPosition = LIGHT_POS;
    m_dirLight.SetForwardVector(LIGHT_FWD);
    m_dirLight.lightNear = LIGHT_NEAR;
    m_dirLight.lightFar = LIGHT_FAR;
    m_dirLight.lightViewportWidth = LIGHT_VIEWPORT_WIDTH;
    m_dirLight.lightViewportHeight = LIGHT_VIEWPORT_HEIGHT;

    // Load scene
    m_pActiveScene = new BakedScene(device, L"Cathedral", L"Cathedral_high");
    auto& model = m_pActiveScene->m_obj.Model;

    auto commandList = m_deviceResources->GetCommandList();
    commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);

    // Resource for Constant Buffer used for the lights
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t buffersize = sizeof(CBLightStructPadded) * m_deviceResources->GetBackBufferCount();
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_dirLightCBResource.ReleaseAndGetAddressOf())));

        m_dirLightCBAddress = m_dirLightCBResource->GetGPUVirtualAddress();

        CD3DX12_RANGE const range(0, 0);
        DX::ThrowIfFailed(m_dirLightCBResource->Map(0, &range, reinterpret_cast<void**>(&m_dirLightCBMappedMem)));
    }

    // Resource for Constant Buffer used for the remapping info
    {
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // We don't need one per draw call since this remains constant for all the calls
        size_t buffersize = sizeof(CBRemapStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_remapCBResource.ReleaseAndGetAddressOf())));

        m_remapCBAddress = m_remapCBResource->GetGPUVirtualAddress();

        CD3DX12_RANGE const range(0, 0);
        DX::ThrowIfFailed(m_remapCBResource->Map(0, &range, reinterpret_cast<void**>(&m_remapCBMappedMem)));

        // Since its constant, we can pass the value here on initialization
        CBRemapStruct cbRemapping = {};
        cbRemapping.g_fFarNearRatio = (m_dirLight.lightFar / m_dirLight.lightNear);
        memcpy(&m_remapCBMappedMem->data, &cbRemapping, sizeof(cbRemapping));
    }
    commandList->Close();

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // UI (HUD)
    {
        auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        size_t start, end;
        m_srvPile->AllocateRange(1, start, end);

        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_srvPile->GetCpuHandle(start),
            m_srvPile->GetGpuHandle(start));
    }

    // Allocate the resource pile, since we will use this for all the descriptors:
    //      1 descriptor for the shadow map SRV
    //      2 descriptors per technique (6)
    {
        size_t start, end;
        m_srvPile->AllocateRange(FIXED_PORTION_OFFSET - 1, start, end);
        assert(start == (size_t)SHADOW_MAP_OFFSET &&
            end == (size_t)EXP_VAR_PASS2_OFFSET + 1);
    }

    // Create heaps, resources and descriptors for all shadow related passes
    InitializeShadowParameters(device, m_deviceResources->GetDepthBufferFormat());

    model->LoadStaticBuffers(device, resourceUpload, true);
    
    // Upload textures to GPU 
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    size_t texOffsets;
    if (!model->textureNames.empty())
    {
        std::wstring mediaDirectory = m_pActiveScene->m_mediaDirectory;
        m_textureFactory->SetDirectory(mediaDirectory.c_str());

        size_t end;
        m_srvPile->AllocateRange(model->textureNames.size(), texOffsets, end);
        assert(texOffsets == (uint32_t)FIXED_PORTION_OFFSET);
    
        model->LoadTextures(*m_textureFactory, INT(texOffsets));
    }
    
    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);

    // Create resource for scene constants (view-proj and more)
    {
        // This is allocated for two draws, per backbuffer
        uint32_t bufferCount = m_deviceResources->GetBackBufferCount() * m_sceneConstantsNumBindings;

        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t buffersize = static_cast<size_t>(bufferCount) * sizeof(CBSceneConstStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_sceneConstantsCBResource.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_sceneConstantsCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_sceneConstantsCBMappedMem)));
        m_sceneConstantsCBAddress = m_sceneConstantsCBResource->GetGPUVirtualAddress();
    }

    // Primitive to represent directional light in scene (and resource)
    {
        m_lightMesh = GeometricPrimitive::CreateSphere(6.0f, 16Ui64, true, false, device);

        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t buffersize = sizeof(CBTransformStructPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_lightMeshCBResource.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_lightMeshCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_lightMeshCBMappedMem)));
    }

    // For FSQ pass (with Geometry shader)
    {
        static const float vertexList[3] = { 0.0f, 0.0f, 0.0f };
        static uint16_t indices[1] = { 0 };

        // Vertex buffer
        DX::ThrowIfFailed(
            CreateStaticBuffer(device, resourceUpload, vertexList, std::size(vertexList), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                m_resolveFSQVertexBufferRes.ReleaseAndGetAddressOf()));

        m_resolveFSQVertexBuffer.BufferLocation = m_resolveFSQVertexBufferRes->GetGPUVirtualAddress();
        m_resolveFSQVertexBuffer.SizeInBytes = sizeof(vertexList);
        m_resolveFSQVertexBuffer.StrideInBytes = sizeof(vertexList);

        // Index buffer
        DX::ThrowIfFailed(
            CreateStaticBuffer(device, resourceUpload, indices, std::size(indices), D3D12_RESOURCE_STATE_INDEX_BUFFER,
                m_resolveFSQIndexBufferRes.ReleaseAndGetAddressOf()));

        m_resolveFSQIndexBuffer.BufferLocation = m_resolveFSQIndexBufferRes->GetGPUVirtualAddress();
        m_resolveFSQIndexBuffer.Format = DXGI_FORMAT_R16_UINT;
        m_resolveFSQIndexBuffer.SizeInBytes = sizeof(indices);
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}


void Sample::ChangeMsaaLevel(MSAA_LEVEL newLevel)
{
    if (msaaLvl != newLevel && newLevel < MSAA_LEVEL_COUNT)
    {
        msaaLvl = newLevel;

        m_deviceResources->WaitForGpu();

        InitializeShadowParameters(m_deviceResources->GetD3DDevice(), m_deviceResources->GetDepthBufferFormat());

        // Only Shadowmap PSO can have more than one sample, so no need to regen all PSOs
        SetupShadowMapPSO();
    }
}


BOOL Sample::IsFilterCorrect(uint32_t iRemapping)
{
    switch (iRemapping)
    {
    case DEPTH_REMAPPING_VARIANCE:
    case DEPTH_REMAPPING_EXPONENTIAL:
    case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
        return TRUE;
    default:
        return FALSE;
    }
}


BOOL Sample::IsUsingPCF(PCF_MODE mode)
{
    switch (mode)
    {
    case PCF_MODE_GATHERCMP:
    case PCF_MODE_SAMPLECMP_STEP1:
    case PCF_MODE_SAMPLECMP_STEP2:
        return TRUE;
    default:
        return FALSE;
    }
}


BOOL Sample::IsMsaa(MSAA_LEVEL lvl, DEPTH_REMAPPING filter)
{
    if (lvl != MSAA_LEVEL_NONE)
    {
        return IsFilterCorrect(filter);
    }
    else
    {
        return FALSE;
    }
}


void Sample::SetRootSignature()
{
    ID3D12Device* device = m_deviceResources->GetD3DDevice();
    uint32_t NodeMask = 0;
    auto& shaderBlobVS = m_BLOBS.VS_BLOB;

    // Extract root signature
    DX::ThrowIfFailed(
        device->CreateRootSignature(NodeMask,
            (void*)shaderBlobVS.data(),
            shaderBlobVS.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
}


// Create all the PSO's needed and store them by their config
void Sample::PipelineStateSetup()
{
    ID3D12Device *device = m_deviceResources->GetD3DDevice();

    // Blobs that do not depend on multiple config values
    auto& shaderBlobVS = m_BLOBS.VS_BLOB;
    auto& shaderBlobVSNull = m_BLOBS.VSNull_BLOB;
    auto& shaderBlobGS = m_BLOBS.GS_BLOB;
    auto& shaderBlobPSUnlit = m_BLOBS.PS_Unlit_BLOB;
    auto& shaderBlobPSLightSrc = m_BLOBS.PS_LightSrc_BLOB;

    //////////////////////////////
    ////    Shadow pass PSO   ////
    //////////////////////////////
    SetupShadowMapPSO();

    ////////////////////////////
    ////  RESOLVE pass PSO  ////
    ////////////////////////////
    {
        // Specify Input Layout and elements
        D3D12_INPUT_ELEMENT_DESC elemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        D3D12_INPUT_ELEMENT_DESC inputElemArr[] = { elemDescPosition };

        // create PSO (one or more)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElemArr, static_cast<uint32_t>(std::size(inputElemArr)) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { shaderBlobVSNull.data(), shaderBlobVSNull.size() };
        psoDesc.GS = { shaderBlobGS.data(), shaderBlobGS.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.NumRenderTargets = 1;
        psoDesc.SampleDesc.Count = 1;

        for (size_t i = 0; i < DEPTH_REMAPPING_COUNT - 1; ++i)
        {
            switch (i)
            {
            case DEPTH_REMAPPING_VARIANCE:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case DEPTH_REMAPPING_EXPONENTIAL:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
                break;
            case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            };

            auto& blob = m_BLOBS.Resolve_BLOBS[i];
            psoDesc.PS = { blob.data(), blob.size() };
            DX::ThrowIfFailed(
                device->CreateGraphicsPipelineState(&psoDesc,
                    IID_GRAPHICS_PPV_ARGS(m_resolvePassPSO[i].ReleaseAndGetAddressOf())));
        }
    }

    /////////////////////////////
    ////  Filtering pass PSO ////
    /////////////////////////////
    {
        // Specify Input Layout and elements
        D3D12_INPUT_ELEMENT_DESC elemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        D3D12_INPUT_ELEMENT_DESC inputElemArr[] = { elemDescPosition };

        // create PSO (HORIZONTAL BLUR - NO MSAA)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElemArr, static_cast<uint32_t>(std::size(inputElemArr)) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { shaderBlobVSNull.data(), shaderBlobVSNull.size() };
        psoDesc.GS = { shaderBlobGS.data(), shaderBlobGS.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.NumRenderTargets = 1;
        psoDesc.SampleDesc.Count = 1;

        // Horizontal blurring pass
        for (size_t i = 0; i < 2; ++i) // we are only counting msaa as a binary on/off, not the amount of msaa options
        {
            for (size_t j = 0; j < DEPTH_REMAPPING_COUNT-1; ++j)
            {
                switch (j) //to skip REMAPPING_STANDARD, temporal solution
                {
                case DEPTH_REMAPPING_VARIANCE:
                    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
                    break;
                case DEPTH_REMAPPING_EXPONENTIAL:
                    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
                    break;
                case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
                    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;
                };

                for (size_t k = 0; k < BLUR_KERNEL_COUNT; ++k)
                {
                    for (size_t l = 0; l < BLUR_TYPE_COUNT; ++l)
                    {
                        auto& blob = m_BLOBS.FilterHrz_BLOBS[i][j][k][l];
                        psoDesc.PS = { blob.data(), blob.size()};
                        DX::ThrowIfFailed(
                            device->CreateGraphicsPipelineState(&psoDesc,
                                IID_GRAPHICS_PPV_ARGS(m_filterHrzPassPSO[i][j][k][l].ReleaseAndGetAddressOf())));
                    }
                }
            }
        }

        // Vertical blurring pass
        for (size_t j = 0; j < DEPTH_REMAPPING_COUNT-1; ++j)
        {
            switch (j) //to skip REMAPPING_STANDARD, temporal solution
            {
            case DEPTH_REMAPPING_VARIANCE:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case DEPTH_REMAPPING_EXPONENTIAL:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
                break;
            case DEPTH_REMAPPING_EXPONENTIAL_VARIANCE:
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            };

            for (size_t k = 0; k < BLUR_KERNEL_COUNT; ++k)
            {
                for (size_t l = 0; l < BLUR_TYPE_COUNT; ++l)
                {
                    auto& blob = m_BLOBS.FilterVrt_BLOBS[j][k][l];
                    psoDesc.PS = { blob.data(), blob.size() };
                    DX::ThrowIfFailed(
                        device->CreateGraphicsPipelineState(&psoDesc,
                            IID_GRAPHICS_PPV_ARGS(m_filterVrtPassPSO[j][k][l].ReleaseAndGetAddressOf())));
                }
            }
        }
    }

    //////////////////////////////
    ////   Shading pass PSO   ////
    //////////////////////////////
    {
        // create PSO (one or more)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { shaderBlobVS.data(), shaderBlobVS.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;

        // PCF using SAMPLECMP PSO
        for (size_t i = 0; i < BLUR_KERNEL_COUNT - 1; ++i)
        {
            for (size_t j = 0; j < BLUR_TYPE_COUNT; ++j)
            {
                auto& blobStep1 = m_BLOBS.ShadingPCFSampleCmpStep1_BLOBS[i][j];
                psoDesc.PS = { blobStep1.data(), blobStep1.size() };
                DX::ThrowIfFailed(
                    device->CreateGraphicsPipelineState(&psoDesc,
                        IID_GRAPHICS_PPV_ARGS(m_shadingPassPCFSampleCmpStep1PSO[i][j].ReleaseAndGetAddressOf())));

                auto& blobStep2 = m_BLOBS.ShadingPCFSampleCmpStep2_BLOBS[i][j];
                psoDesc.PS = { blobStep2.data(), blobStep2.size() };
                DX::ThrowIfFailed(
                    device->CreateGraphicsPipelineState(&psoDesc,
                        IID_GRAPHICS_PPV_ARGS(m_shadingPassPCFSampleCmpStep2PSO[i][j].ReleaseAndGetAddressOf())));

                auto& blobGather = m_BLOBS.ShadingPCFGatherCmp_BLOBS[i][j];
                psoDesc.PS = { blobGather.data(), blobGather.size() };
                DX::ThrowIfFailed(
                    device->CreateGraphicsPipelineState(&psoDesc,
                        IID_GRAPHICS_PPV_ARGS(m_shadingPassPCFGatherCmpPSO[i][j].ReleaseAndGetAddressOf())));
            }
        }

        // non PCF techniques for shading and shadows PSO
        for (size_t i = 0; i < DEPTH_REMAPPING_COUNT; ++i)
        {
            auto& blob = m_BLOBS.ShadingNoPCF_BLOBS[i];
            psoDesc.PS = { blob.data(), blob.size() };
            DX::ThrowIfFailed(
                device->CreateGraphicsPipelineState(&psoDesc,
                    IID_GRAPHICS_PPV_ARGS(m_shadingPassNoPCFPSO[i].ReleaseAndGetAddressOf())));
        }

        // Lit without shadows
        psoDesc.PS = { shaderBlobPSUnlit.data(), shaderBlobPSUnlit.size() };
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_litWithoutShadowsPSO.ReleaseAndGetAddressOf())));
    }

    /////////////////////////////////////
    ////  Simple PSO for Light Mesh  ////
    /////////////////////////////////////
    {
        // create PSO (one or more)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { shaderBlobVS.data(), shaderBlobVS.size() };
        psoDesc.PS = { shaderBlobPSLightSrc.data(), shaderBlobPSLightSrc.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_lightMeshPSO.ReleaseAndGetAddressOf())));
    }
}


// Separating this logic from the PSO creation, since we will call it when changing MSAA lvl
void Sample::SetupShadowMapPSO()
{
    // Device for PSO creation
    ID3D12Device* device = m_deviceResources->GetD3DDevice();

    // Blobs that do not depend on multiple config values
    auto& shaderBlobVS = m_BLOBS.VS_BLOB;

    // create PSO (one or more)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { shaderBlobVS.data(), shaderBlobVS.size() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = (D3D12_CULL_MODE)(shadowCullMode);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    psoDesc.SampleMask = UINT32_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 0;
    psoDesc.SampleDesc.Count = IsMsaa(msaaLvl, filterChoice) ? g_iMSAALevelNumSamples[msaaLvl] : 1;
    DX::ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_shadowPassPSO.ReleaseAndGetAddressOf())));
}


// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();
    LONG displayWidth = size.right - size.left;
    LONG displayHeight = size.bottom - size.top;

    /// Set main camera properties
    Vector3 cameraPos(CAMERA_START_POS);

    /// Fly camera
    m_camera.SetWindow(displayWidth, displayHeight);
    m_camera.SetProjectionParameters(XM_PIDIV4, CAMERA_NEAR, CAMERA_FAR, true);
    m_camera.SetSensitivity(80.0f, 80.0f, 80.0f, 0.0f);
    m_camera.SetRotation(Quaternion::CreateFromAxisAngle(Vector3::Up, XM_PI));
    m_camera.SetPosition(cameraPos);

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);
}


// Initializes the shadow texture resource and the descriptor heaps needed for them
void Sample::InitializeShadowParameters(ID3D12Device* device, DXGI_FORMAT depthFormat)
{
    // Shadow texture dimensions
    uint32_t Width = g_iShadowMapDims[shadowMapDims];
    uint32_t Height = Width;

    // Shadow viewport setup
    m_shadowViewport = { 0.0f, 0.0f, (float)Width, (float)Height, 0.0f, 1.0f };
    m_shadowScissorRect = { 0, 0, (int)Width, (int)Height };

    uint32_t rtvIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // ShadowMap
    {
        // If we use MSAA, we need a special dimension for the views
        D3D12_DSV_DIMENSION dsvDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        D3D12_SRV_DIMENSION srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        uint32_t samplecount = 1;
        if (IsMsaa(msaaLvl, filterChoice))
        {
            dsvDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            srvDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            samplecount = g_iMSAALevelNumSamples[msaaLvl];
        }

        // Resouce for shadow map texture
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        resDesc.MipLevels = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resDesc.Width = Width;
        resDesc.Height = Height;
        resDesc.SampleDesc.Count = samplecount;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = depthFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &depthOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_shadowTexResource.ReleaseAndGetAddressOf())));
        m_shadowTexResource->SetName(L"shadowTexResource");

        // Create descriptor heap for rendering depth component
        D3D12_DESCRIPTOR_HEAP_DESC DSHeapDesc = {};
        DSHeapDesc.NumDescriptors = 1; //For now, only for shadow texture
        DSHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        DX::ThrowIfFailed(device->CreateDescriptorHeap(
            &DSHeapDesc,
            IID_GRAPHICS_PPV_ARGS(m_shadowDSHeap.ReleaseAndGetAddressOf())));

        // Create the depthStencilView with the DS HEAP
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = depthFormat;
        dsvDesc.ViewDimension = dsvDimension;
        device->CreateDepthStencilView(m_shadowTexResource.Get(), &dsvDesc, m_shadowDSHeap->GetCPUDescriptorHandleForHeapStart());

        // Create the SRV with the SRV HEAP
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = srvDimension;
        srvDesc.Texture2D.MipLevels = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE shadowSRVHandle = m_srvPile->GetCpuHandle(SHADOW_MAP_OFFSET);
        device->CreateShaderResourceView(m_shadowTexResource.Get(), &srvDesc, shadowSRVHandle);
    }

    // Variance
    {
        // Resources
        for (int i = 0; i < 2; ++i)
        {
            // Resources (2 will be created)
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            D3D12_RESOURCE_DESC resDesc = {};
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.Format = DXGI_FORMAT_R32G32_TYPELESS;
            resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            resDesc.MipLevels = 1;
            resDesc.DepthOrArraySize = 1;
            resDesc.Width = Width;
            resDesc.Height = Height;
            resDesc.SampleDesc.Count = 1;

            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resDesc,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    nullptr,//&clearValue,
                    IID_GRAPHICS_PPV_ARGS(m_resourceVariance[i].ReleaseAndGetAddressOf())));
        }
        m_resourceVariance[0]->SetName(L"VarianceResource[0]");
        m_resourceVariance[1]->SetName(L"VarianceResource[1]");

        // 2 RTVs
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 2;
        DX::ThrowIfFailed(
            device->CreateDescriptorHeap(
                &rtvHeapDesc,
                IID_GRAPHICS_PPV_ARGS(m_RTVHeapVariance.ReleaseAndGetAddressOf())));

        for (int i = 0; i < 2; ++i)
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvdesc = {};
            rtvdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            rtvdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            device->CreateRenderTargetView(
                m_resourceVariance[i].Get(),
                &rtvdesc,
                CD3DX12_CPU_DESCRIPTOR_HANDLE(
                    m_RTVHeapVariance->GetCPUDescriptorHandleForHeapStart(),
                    i,
                    rtvIncrementSize));
        }

        for (int i = 0; i < 2; ++i)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvdesc.Texture2D.MipLevels = 1;

            D3D12_CPU_DESCRIPTOR_HANDLE varianceSRVHandle = m_srvPile->GetCpuHandle((uint32_t)VARIANCE_PASS1_OFFSET + i);
            device->CreateShaderResourceView(
                m_resourceVariance[i].Get(),
                &srvdesc,
                varianceSRVHandle);
        }
    }

    // Exponential
    {
        // Resources
        for (int i = 0; i < 2; ++i)
        {
            // Resources (2 will be created)
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            D3D12_RESOURCE_DESC resDesc = {};
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            resDesc.MipLevels = 1;
            resDesc.DepthOrArraySize = 1;
            resDesc.Width = Width;
            resDesc.Height = Height;
            resDesc.SampleDesc.Count = 1;

            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resDesc,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    nullptr,//&clearValue,
                    IID_GRAPHICS_PPV_ARGS(m_resourceExponential[i].ReleaseAndGetAddressOf())));
        }
        m_resourceExponential[0]->SetName(L"ExponentialResource[0]");
        m_resourceExponential[1]->SetName(L"ExponentialResource[1]");

        // 2 RTVs
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 2;
        DX::ThrowIfFailed(
            device->CreateDescriptorHeap(
                &rtvHeapDesc,
                IID_GRAPHICS_PPV_ARGS(m_RTVHeapExponential.ReleaseAndGetAddressOf())));

        for (int i = 0; i < 2; ++i)
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvdesc = {};
            rtvdesc.Format = DXGI_FORMAT_R32_FLOAT;
            rtvdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            device->CreateRenderTargetView(
                m_resourceExponential[i].Get(),
                &rtvdesc,
                CD3DX12_CPU_DESCRIPTOR_HANDLE(
                    m_RTVHeapExponential->GetCPUDescriptorHandleForHeapStart(),
                    i,
                    rtvIncrementSize));
        }

        for (int i = 0; i < 2; ++i)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Format = DXGI_FORMAT_R32_FLOAT;
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvdesc.Texture2D.MipLevels = 1;

            D3D12_CPU_DESCRIPTOR_HANDLE exponentialSRVHandle = m_srvPile->GetCpuHandle((uint32_t)EXPONENTIAL_PASS1_OFFSET + i);
            device->CreateShaderResourceView(
                m_resourceExponential[i].Get(),
                &srvdesc,
                exponentialSRVHandle);
        }
    }

    // ExponentialVariance
    {
        // Resources
        for (int i = 0; i < 2; ++i)
        {
            // Resources (2 will be created)
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            D3D12_RESOURCE_DESC resDesc = {};
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.Format = DXGI_FORMAT_R32G32B32A32_TYPELESS;
            resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            resDesc.MipLevels = 1;
            resDesc.DepthOrArraySize = 1;
            resDesc.Width = Width;
            resDesc.Height = Height;
            resDesc.SampleDesc.Count = 1;

            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resDesc,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    nullptr,//&clearValue,
                    IID_GRAPHICS_PPV_ARGS(m_resourceExpVariance[i].ReleaseAndGetAddressOf())));
        }
        m_resourceExpVariance[0]->SetName(L"ExpVarianceResource[0]");
        m_resourceExpVariance[1]->SetName(L"ExpVarianceResource[1]");

        // 2 RTVs
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 2;
        DX::ThrowIfFailed(
            device->CreateDescriptorHeap(
                &rtvHeapDesc,
                IID_GRAPHICS_PPV_ARGS(m_RTVHeapExpVariance.ReleaseAndGetAddressOf())));

        for (int i = 0; i < 2; ++i)
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvdesc = {};
            rtvdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            rtvdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            device->CreateRenderTargetView(
                m_resourceExpVariance[i].Get(),
                &rtvdesc,
                CD3DX12_CPU_DESCRIPTOR_HANDLE(
                    m_RTVHeapExpVariance->GetCPUDescriptorHandleForHeapStart(),
                    i,
                    rtvIncrementSize));
        }

        for (int i = 0; i < 2; ++i)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvdesc.Texture2D.MipLevels = 1;

            D3D12_CPU_DESCRIPTOR_HANDLE expVarianceSRVHandle = m_srvPile->GetCpuHandle((uint32_t)EXP_VAR_PASS1_OFFSET + i);
            device->CreateShaderResourceView(
                m_resourceExpVariance[i].Get(),
                &srvdesc,
                expVarianceSRVHandle);
        }
    }
}


void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_gpuTimer.reset();
    m_srvPile.reset();
    m_textureFactory.reset();
    m_gamePad.reset();
    m_lightMesh.reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();

    m_rootSignature.Reset();
    m_shadowPassPSO.Reset();
    m_litWithoutShadowsPSO.Reset();
    m_lightMeshPSO.Reset();
    m_shadingPassNoPCFPSO[DEPTH_REMAPPING_VARIANCE].Reset();
    m_shadingPassNoPCFPSO[DEPTH_REMAPPING_EXPONENTIAL].Reset();
    m_shadingPassNoPCFPSO[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE].Reset();
    m_shadingPassNoPCFPSO[DEPTH_REMAPPING_STANDARD].Reset();
    m_resolvePassPSO[DEPTH_REMAPPING_VARIANCE].Reset();
    m_resolvePassPSO[DEPTH_REMAPPING_EXPONENTIAL].Reset();
    m_resolvePassPSO[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE].Reset();

    for (size_t i = 0; i < MSAA_ON_OFF_COUNT; ++i)
    {
        for (size_t j = 0; j < DEPTH_REMAPPING_COUNT - 1; ++j)
        {
            for (size_t k = 0; k < BLUR_KERNEL_COUNT; ++k)
            {
                for (size_t l = 0; l < BLUR_TYPE_COUNT; ++l)
                {
                    m_filterHrzPassPSO[i][j][k][l].Reset();
                }
            }
        }
    }

    for (size_t i = 0; i < DEPTH_REMAPPING_COUNT - 1; ++i)
    {
        for (size_t j = 0; j < BLUR_KERNEL_COUNT; ++j)
        {
            for (size_t k = 0; k < BLUR_TYPE_COUNT; ++k)
            {
                m_filterVrtPassPSO[i][j][k].Reset();
            }
        }
    }

    for (size_t i = 0; i < BLUR_KERNEL_COUNT - 1; ++i)
    {
        for (size_t j = 0; j < BLUR_TYPE_COUNT; ++j)
        {
            m_shadingPassPCFSampleCmpStep1PSO[i][j].Reset();
            m_shadingPassPCFSampleCmpStep2PSO[i][j].Reset();
            m_shadingPassPCFGatherCmpPSO[i][j].Reset();
        }
    }

    m_shadowDSHeap.Reset();
    m_RTVHeapVariance.Reset();
    m_RTVHeapExponential.Reset();
    m_RTVHeapExpVariance.Reset();
    m_SRVHeapExpVariance.Reset();

    m_shadowTexResource.Reset();
    m_resourceExpVariance[0].Reset();
    m_resourceExpVariance[1].Reset();
    m_resourceVariance[0].Reset();
    m_resourceVariance[1].Reset();
    m_resourceExponential[0].Reset();
    m_resourceExponential[1].Reset();
    m_dirLightCBResource.Reset();
    m_remapCBResource.Reset();
    m_resolveFSQVertexBufferRes.Reset();
    m_resolveFSQIndexBufferRes.Reset();
    m_lightMeshCBResource.Reset();
    m_sceneConstantsCBResource.Reset();
}


void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}


#pragma endregion
