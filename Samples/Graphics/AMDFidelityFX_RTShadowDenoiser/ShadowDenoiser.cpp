//--------------------------------------------------------------------------------------
// ShadowDenoiser.cpp
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"
#include "FindMedia.h"
#include "NoiseBuffers.h"

#include "ShadowDenoiser.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)
 

namespace
{
    /**
    Performs a rounded division.

    \param value The value to be divided.
    \param divisor The divisor to be used.
    \return The rounded divided value.
    */
    template<typename TYPE>
    static inline TYPE RoundedDivide(TYPE value, TYPE divisor)
    {
        return (value + divisor - 1) / divisor;
    }

    const XMVECTORF32 c_eye = { -1.2f, 1.4f, 0.0f };
    constexpr float c_pitch = -0.05f;
    constexpr float c_yaw = 2.10f;

    struct GpuTimerIndex
    {
        enum
        {
            Denoise,
            Raytrace,
        };
    };

}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_eye(c_eye),
    m_currentCamera(0),
    m_useInlineRT(false)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        3);

    // Set up for post-process rendering.
    m_scene = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_scene->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    m_gltfPBR->OnDestroy();
    delete m_gltfPBR;

    m_gltfMotionVectors->OnDestroy();
    delete m_gltfMotionVectors;

    m_gltfRTShadows->OnDestroy();
    delete m_gltfRTShadows;

    m_gltfResources.OnDestroy();

    m_gltfModel->Unload();
    delete m_gltfModel;
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

void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 3840;
    height = 2160;
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

#ifdef _GAMING_XBOX
    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif

    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }


        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch += pad.thumbSticks.leftY * 0.1f;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_useInlineRT = !m_useInlineRT;
        }

    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    constexpr float c_limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-c_limit, std::min(+c_limit, m_pitch));

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    m_lookAt = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    m_view = XMMatrixLookToLH(m_eye, m_lookAt, g_XMIdentityR1);
    

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

    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    m_gpuTimer->BeginFrame(commandList);

    m_scene->BeginScene(commandList);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GLTF");

    m_gltfModel->GetCommonPerFrameData().iblFactor = 0.36f;
    m_gltfModel->GetCommonPerFrameData().emmisiveFactor = 0;
  
    if (m_timer.GetFrameCount() == 1)
    {
        TransitionTo(commandList, m_previousDepth, &m_previousDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::PreviousDepthDSV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    } 

    XMVECTOR lightPosition =
        XMVectorSet(0.1f, 1.6f + 0.4f*sin(0.008f*m_timer.GetFrameCount()), 0.1f + 0.35f*cos(0.015f*m_timer.GetFrameCount()), 0);

    InitNoiseBuffers(commandList);

    // move the light to make the test more interesting
    if (m_gltfModel->NumLightInstances() >= 1)
    {
        tfNode n; 
        n.m_tranform.LookAt(lightPosition, XMVectorSet(0, 0, 0, 0));

        tfLight l;
        l.m_type = tfLight::LIGHT_POINTLIGHT;
        l.m_intensity = 0.8f;
        l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
        l.m_range = 20.0f;
        l.m_outerConeAngle = XM_PI / 4.0f;
        l.m_innerConeAngle = (XM_PI / 4.0f) * 0.9f;

        // update the node data in the existing light instance
        m_gltfModel->UpdateLightInstanceNode(0, n);
    }

    //disable shadow maps - we are using RT shadows
    for (uint32_t i = 0; i < m_gltfModel->GetCommonPerFrameData().lightCount; i++)
    {
        m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex = 0xffffffffu;   // no shadows for this light
    }
 
    // No animaton support in the DXR builder currently
    m_gltfModel->SetAnimationTime(0, 0);
    m_gltfModel->TransformScene(0, XMMatrixScaling(-1, 1, 1));

    DirectX::XMMATRIX currentViewProj = m_view * m_proj;

    m_gltfModel->GetCommonPerFrameData().mCameraViewProj = currentViewProj;
    m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj = XMMatrixInverse(nullptr, m_gltfModel->GetCommonPerFrameData().mCameraViewProj);
    m_gltfModel->GetCommonPerFrameData().cameraPos = m_eye;
    m_gltfModel->UpdatePerFrameLights();
    m_gltfResources.SetSkinningMatricesForSkeletons();
    m_gltfResources.SetPerFrameConstants();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shadow Atlas");

    // No shadow maps in this sample - RT shadows composited later 
    commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    PIXEndEvent(commandList);

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Motion Vectors Pass");

        TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
         
        const float clearValues[4] = { 0.0f , 0.0f , 0.0f , 0.0f };
        commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV), &clearValues[0], 0, nullptr);

        auto const viewport = m_deviceResources->GetScreenViewport();
        auto const scissorRect = m_deviceResources->GetScissorRect();
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);


        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[1];
        rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV);

        auto motionDepthCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV) };

        commandList->OMSetRenderTargets(1, &rtvs[0], false, &motionDepthCPUHandle);

        AMDTK::GltfMotionVectorsPass::per_frame perPassConstants;
        perPassConstants.mCurrViewProj = currentViewProj;
        perPassConstants.mPrevViewProj = m_prevViewProj;
        perPassConstants.normalizedFormat = 0;

        m_gltfMotionVectors->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

        m_gltfMotionVectors->Draw(commandList);
        PIXEndEvent(commandList);
    }
     
    Clear(true);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"PBR Pass");
     
    m_gltfPBR->Draw(commandList, m_gltfResources.GetSrvPile()->GetGpuHandle(m_shadowAtlasIdx));
    
    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    PIXEndEvent(commandList);


    PIXEndEvent(commandList);
    {

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Scene Transition");
        m_scene->TransitionTo(commandList,  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);
  
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RT Shadows");
             

            D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
            m_gltfRTShadows->PrepareDispatchRaysDesc(&dispatchRaysDesc);

            dispatchRaysDesc.Width = (UINT)m_deviceResources->GetOutputSize().right;
            dispatchRaysDesc.Height = (UINT)m_deviceResources->GetOutputSize().bottom;

            TransitionTo(commandList, m_noisyRTResult, &m_noisyRTResultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_normals, &m_normalsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
             

            struct RTShadowParams
            {
                DirectX::XMMATRIX  cameraViewProjInverse;
                uint32_t    dispatchWidth;
                uint32_t    dispatchHeight;
                float       dispatchWidthInverse;
                float       dispatchHeightInverse;
                float       lightPos[4];
                float       lightSize;
                uint32_t    frameNum;
            };
             
            RTShadowParams cb2;
            cb2.cameraViewProjInverse = m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj;
            cb2.lightPos[0] = XMVectorGetX(lightPosition);
            cb2.lightPos[1] = XMVectorGetY(lightPosition);
            cb2.lightPos[2] = XMVectorGetZ(lightPosition);
            cb2.lightSize = 0.15f;
            cb2.dispatchWidth = dispatchRaysDesc.Width;
            cb2.dispatchHeight = dispatchRaysDesc.Height;
            cb2.dispatchWidthInverse = 1.0f / dispatchRaysDesc.Width;
            cb2.dispatchHeightInverse = 1.0f / dispatchRaysDesc.Height;
            cb2.frameNum = m_timer.GetFrameCount();

            auto cbHandle = m_graphicsMemory->AllocateConstant(cb2);


            ID3D12GraphicsCommandList6* dxrCommandList; 
            DX::ThrowIfFailed(commandList->QueryInterface(IID_GRAPHICS_PPV_ARGS(&dxrCommandList)));

            dxrCommandList->SetComputeRootSignature(m_gltfRTShadows->GetRootSignature());
            dxrCommandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);

            if (m_useInlineRT)
            {
                dxrCommandList->SetPipelineState(m_gltfRTShadows->GetInlineRTPipelineState());
            }
            else
            {
                dxrCommandList->SetPipelineState1(m_gltfRTShadows->GetRTPSO());
            }

            dxrCommandList->SetComputeRootShaderResourceView(0, m_gltfRTShadows->GetTLAS());
            dxrCommandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::SobolBufferSRV));
            dxrCommandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::ScramblingTileBufferSRV));
            dxrCommandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV));
            dxrCommandList->SetComputeRootDescriptorTable(4, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalsSRV));
            dxrCommandList->SetComputeRootConstantBufferView(5, cbHandle.GpuAddress());

            dxrCommandList->SetComputeRootDescriptorTable(6, m_resourceDescriptors->GetGpuHandle(Descriptors::NoisyRTResultUAV));
            m_gpuTimer->Start(commandList, GpuTimerIndex::Raytrace);
            if (m_useInlineRT)
            {
                dxrCommandList->Dispatch(dispatchRaysDesc.Width / 8, dispatchRaysDesc.Height / 4, 1);
            }
            else
            {
                dxrCommandList->DispatchRays(&dispatchRaysDesc);
            }
            m_gpuTimer->Stop(commandList, GpuTimerIndex::Raytrace);

            dxrCommandList->Release();

            PIXEndEvent(commandList);
        }

        m_gpuTimer->Start(commandList, GpuTimerIndex::Denoise);
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX Denoiser");
        {
            //Prepare ShadowMask
            struct PassData_PrepareShadowMask
            {
                int32_t buffer_dimensions_x;
                int32_t buffer_dimensions_y;
            };

            PassData_PrepareShadowMask cb0;
            cb0.buffer_dimensions_x = m_deviceResources->GetOutputSize().right;
            cb0.buffer_dimensions_y = m_deviceResources->GetOutputSize().bottom;

            
            TransitionTo(commandList, m_raytraceResult, &m_raytraceResultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_noisyRTResult, &m_noisyRTResultState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            auto cbHandle = m_graphicsMemory->AllocateConstant(cb0);
            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_prepareShadowMapRS.Get());
            commandList->SetPipelineState(m_prepareShadowMapPSO.Get());

            commandList->SetComputeRootConstantBufferView(0u, cbHandle.GpuAddress()); 

            commandList->SetComputeRootDescriptorTable(1u, m_resourceDescriptors->GetGpuHandle(Descriptors::NoisyRTResultSRV));
            commandList->SetComputeRootDescriptorTable(2u, m_resourceDescriptors->GetGpuHandle(Descriptors::RaytraceResultUAV));

            commandList->Dispatch(RoundedDivide((UINT)m_deviceResources->GetOutputSize().right, 32u), RoundedDivide((UINT)m_deviceResources->GetOutputSize().bottom, 16u), 1u);
        }

        {
            //Tile Classification
            struct PassData_TileClassification
            {
                float eye[3];
                uint32_t first_frame;
                int32_t buffer_dimensions_x;
                int32_t buffer_dimensions_y;
                float inv_buffer_dimensions_x;
                float inv_buffer_dimensions_y;
                DirectX::XMMATRIX projection_inverse;
                DirectX::XMMATRIX reprojection_matrix;
                DirectX::XMMATRIX view_projection_inverse;
            };

            PassData_TileClassification cb0;
            cb0.eye[0] = XMVectorGetX(m_eye);
            cb0.eye[1] = XMVectorGetY(m_eye);
            cb0.eye[2] = XMVectorGetZ(m_eye);
            cb0.first_frame = m_timer.GetFrameCount() == 1u ? 1u : 0u;
            cb0.buffer_dimensions_x = m_deviceResources->GetOutputSize().right;
            cb0.buffer_dimensions_y = m_deviceResources->GetOutputSize().bottom;
            cb0.inv_buffer_dimensions_x = 1.0f / static_cast<float>(cb0.buffer_dimensions_x);
            cb0.inv_buffer_dimensions_y = 1.0f / static_cast<float>(cb0.buffer_dimensions_y);
 

            SimpleMath::Matrix const view_projection_inverse = XMMatrixInverse(nullptr, XMMatrixMultiply( m_view, m_proj));

            
            cb0.projection_inverse = XMMatrixInverse(nullptr, m_proj);
            cb0.reprojection_matrix = view_projection_inverse * m_prevViewProj;
            cb0.view_projection_inverse = view_projection_inverse;

            m_prevViewProj = m_view * m_proj;


            TransitionTo(commandList, m_historyBuffer1, &m_historyBuffer1State, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); //previous fraome output from filter pass 0
            TransitionTo(commandList, m_historyMomentsBuffer, &m_historyMomentsBufferState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);// previous frame outptu from tileclassification
            TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_previousDepth, &m_previousDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_raytraceResult, &m_raytraceResultState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_normals, &m_normalsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            TransitionTo(commandList, m_momentsBuffer, &m_momentsBufferState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_tileMetadata, &m_tileMetadataState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_reprojectionResultIntermediateBuffer, &m_reprojectionResultIntermediateBufferState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            auto cbHandle = m_graphicsMemory->AllocateConstant(cb0);

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_denoiseTileClassificationRS.Get());
            commandList->SetPipelineState(m_denoiseTileClassificationPSO.Get());

            commandList->SetComputeRootConstantBufferView(0u, cbHandle.GpuAddress());
            commandList->SetComputeRootDescriptorTable(1u, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV)); // DepthBuffer
            commandList->SetComputeRootDescriptorTable(2u, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorsSRV)); // VelocityBuffer
            commandList->SetComputeRootDescriptorTable(3u, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalsSRV)); // NormalBuffer
            commandList->SetComputeRootDescriptorTable(4u, m_resourceDescriptors->GetGpuHandle(Descriptors::HistoryBuffer1SRV)); // HistoryBuffer
            commandList->SetComputeRootDescriptorTable(5u, m_resourceDescriptors->GetGpuHandle(Descriptors::MomentsBuffer0SRV)); // PreviousMomentsBuffer
            commandList->SetComputeRootDescriptorTable(6u, m_resourceDescriptors->GetGpuHandle(Descriptors::PreviousDepthSRV)); // PreviousDepthBuffer
            commandList->SetComputeRootDescriptorTable(7u, m_resourceDescriptors->GetGpuHandle(Descriptors::RaytraceResultSRV)); // RaytracedShadowMask
            commandList->SetComputeRootDescriptorTable(8u, m_resourceDescriptors->GetGpuHandle(Descriptors::TileMetadataUAV)); // TileMetaData
            commandList->SetComputeRootDescriptorTable(9u, m_resourceDescriptors->GetGpuHandle(Descriptors::MomentsBuffer1UAV)); // NewMomentsBuffer
            commandList->SetComputeRootDescriptorTable(10u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateReprojectionUAV)); // ReprojectionResults

            commandList->Dispatch(RoundedDivide((UINT)m_deviceResources->GetOutputSize().right, 8u), RoundedDivide((UINT)m_deviceResources->GetOutputSize().bottom, 8u), 1u);
        }

        struct PassData_FilterSoftShadow
        {
            DirectX::XMMATRIX  projection_inverse;
            std::int32_t buffer_dimensions_x;
            std::int32_t buffer_dimensions_y;
            float inv_buffer_dimensions_x;
            float inv_buffer_dimensions_y;
            float depth_similarity_sigma;
            std::int32_t step_size;
        };

        PassData_FilterSoftShadow cb0; 
        cb0.projection_inverse = XMMatrixInverse(nullptr, m_proj); 
        cb0.buffer_dimensions_x = m_deviceResources->GetOutputSize().right;
        cb0.buffer_dimensions_y = m_deviceResources->GetOutputSize().bottom;
        cb0.inv_buffer_dimensions_x = 1.0f / static_cast<float>(cb0.buffer_dimensions_x);
        cb0.inv_buffer_dimensions_y = 1.0f / static_cast<float>(cb0.buffer_dimensions_y);
        cb0.depth_similarity_sigma = 1.0f;

     
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb0);
        {
            //Filter Soft Shadows Pass 0

            TransitionTo(commandList, m_tileMetadata, &m_tileMetadataState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_reprojectionResultIntermediateBuffer, &m_reprojectionResultIntermediateBufferState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            TransitionTo(commandList, m_historyBuffer0, &m_historyBuffer0State, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);   
            TransitionTo(commandList, m_intermediateFilterResult, &m_intermediateFilterResultState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);            

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_denoiseFilterPass0RS.Get());
            commandList->SetPipelineState(m_denoiseFilterPass0PSO.Get());

            commandList->SetComputeRootConstantBufferView(0u, cbHandle.GpuAddress());
            commandList->SetComputeRootDescriptorTable(1u, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV)); // DepthBuffer
            commandList->SetComputeRootDescriptorTable(2u, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalsSRV)); // NormalBuffer
            commandList->SetComputeRootDescriptorTable(3u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateReprojectionSRV)); // ReprojectionResults
            commandList->SetComputeRootDescriptorTable(4u, m_resourceDescriptors->GetGpuHandle(Descriptors::TileMetadataSRV)); // TileMetaData
             
            commandList->SetComputeRootDescriptorTable(5u, m_resourceDescriptors->GetGpuHandle(Descriptors::HistoryBuffer0UAV)); // HistoryBuffer
            commandList->SetComputeRootDescriptorTable(6u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateFilterUAV)); // IntermediateFilterResult
 
            commandList->Dispatch(RoundedDivide((UINT)m_deviceResources->GetOutputSize().right, 8u), RoundedDivide((UINT)m_deviceResources->GetOutputSize().bottom, 8u), 1u);
        }

        {
            // //Filter Soft Shadows Pass 1
            TransitionTo(commandList, m_historyBuffer0, &m_historyBuffer0State, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_intermediateFilterResult, &m_intermediateFilterResultState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_reprojectionResultIntermediateBuffer, &m_reprojectionResultIntermediateBufferState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_denoiseFilterPass1RS.Get());
            commandList->SetPipelineState(m_denoiseFilterPass1PSO.Get());
             
            commandList->SetComputeRootConstantBufferView(0u, cbHandle.GpuAddress());
            commandList->SetComputeRootDescriptorTable(1u, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV)); // DepthBuffer
            commandList->SetComputeRootDescriptorTable(2u, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalsSRV)); // NormalBuffer
            commandList->SetComputeRootDescriptorTable(3u, m_resourceDescriptors->GetGpuHandle(Descriptors::HistoryBuffer0SRV)); // ReprojectionResults
            commandList->SetComputeRootDescriptorTable(4u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateFilterSRV)); // ReprojectionResults
            commandList->SetComputeRootDescriptorTable(5u, m_resourceDescriptors->GetGpuHandle(Descriptors::TileMetadataSRV)); // TileMetaData

            commandList->SetComputeRootDescriptorTable(6u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateReprojectionUAV)); // IntermediateFilterResult

            commandList->Dispatch(RoundedDivide((UINT)m_deviceResources->GetOutputSize().right, 8u), RoundedDivide((UINT)m_deviceResources->GetOutputSize().bottom, 8u), 1u);
        }

        {
            // //Filter Soft Shadows Pass 2 
            TransitionTo(commandList, m_reprojectionResultIntermediateBuffer, &m_reprojectionResultIntermediateBufferState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_denoisedOutput, &m_denoisedOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_denoiseFilterPass2RS.Get());
            commandList->SetPipelineState(m_denoiseFilterPass2PSO.Get());

            commandList->SetComputeRootConstantBufferView(0u, cbHandle.GpuAddress());
            commandList->SetComputeRootDescriptorTable(1u, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV)); // DepthBuffer
            commandList->SetComputeRootDescriptorTable(2u, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalsSRV)); // NormalBuffer
            commandList->SetComputeRootDescriptorTable(3u, m_resourceDescriptors->GetGpuHandle(Descriptors::IntermediateReprojectionSRV)); // ReprojectionResults
            commandList->SetComputeRootDescriptorTable(4u, m_resourceDescriptors->GetGpuHandle(Descriptors::TileMetadataSRV)); // TileMetaData

            commandList->SetComputeRootDescriptorTable(5u, m_resourceDescriptors->GetGpuHandle(Descriptors::DenoisedOutputUAV)); // IntermediateFilterResult

            commandList->Dispatch(RoundedDivide((UINT)m_deviceResources->GetOutputSize().right, 8u), RoundedDivide((UINT)m_deviceResources->GetOutputSize().bottom, 8u), 1u);
        }

        PIXEndEvent(commandList);

        m_gpuTimer->Stop(commandList, GpuTimerIndex::Denoise);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"History Copies");
        {
            //copy histories
            TransitionTo(commandList, m_momentsBuffer, &m_momentsBufferState, D3D12_RESOURCE_STATE_COPY_SOURCE);
            TransitionTo(commandList, m_historyMomentsBuffer, &m_historyMomentsBufferState, D3D12_RESOURCE_STATE_COPY_DEST);

            commandList->CopyResource(m_historyMomentsBuffer.Get(), m_momentsBuffer.Get());

            TransitionTo(commandList, m_previousDepth, &m_previousDepthState, D3D12_RESOURCE_STATE_COPY_DEST);
            TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_COPY_SOURCE); 

            commandList->CopyResource(m_previousDepth.Get(), m_motionVectorDepth.Get()); 

            TransitionTo(commandList, m_historyBuffer0, &m_historyBuffer0State, D3D12_RESOURCE_STATE_COPY_SOURCE);
            TransitionTo(commandList, m_historyBuffer1, &m_historyBuffer1State, D3D12_RESOURCE_STATE_COPY_DEST);

            commandList->CopyResource(m_historyBuffer1.Get(), m_historyBuffer0.Get());

        }
        PIXEndEvent(commandList);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compose");
        {

            TransitionTo(commandList, m_finalScene, &m_finalSceneOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_noisyRTResult, &m_noisyRTResultState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_denoisedOutput, &m_denoisedOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            TransitionTo(commandList, m_reprojectionResultIntermediateBuffer, &m_reprojectionResultIntermediateBufferState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            // composite shadows to scene
            auto size = m_deviceResources->GetOutputSize();

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_compositeShadowsRS.Get()); 
            commandList->SetComputeRootDescriptorTable(0, m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex));      // Scene
            commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::DenoisedOutputSRV)); // Shadows
            commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::FinalSceneUAV)); // Output
            commandList->SetPipelineState(m_compositeShadowsPSO.Get());

            const unsigned int threadGroupWorkRegionDim = 8u;
            unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
            unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

            commandList->Dispatch(dispatchX, dispatchY, 1u);

            TransitionTo(commandList, m_finalScene, &m_finalSceneOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); 
        }
        PIXEndEvent(commandList);

    }


    // Render final scene
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
  
    m_fullScreenQuad->Draw(commandList, m_passthroughPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::FinalSceneSRV));


    // Render UI
    RenderUI(commandList);

    PIXEndEvent(commandList);


    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_gpuTimer->EndFrame(commandList);
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}


void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    m_batch->Begin(commandList);

    float y = float(safe.top);

    m_font->DrawString(m_batch.get(), L"AMD FidelityFX Raytraced Shadow Denoiser Sample",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);


    wchar_t textBuffer[128] = {};


    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"RT Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Denoiser Time:  %0.3fms", m_gpuTimer->GetAverageMS(GpuTimerIndex::Denoise));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Raytracing Time:  %0.3fms", m_gpuTimer->GetAverageMS(GpuTimerIndex::Raytrace));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Inline RT: %ls", m_useInlineRT ? L"on" : L"off");
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();

    const wchar_t* legendStr = L"[View] Exit [LThumb] View [A] Toggle Inline Raytracing";

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom)),
        ATG::Colors::LightGrey);

    m_batch->End();

    PIXEndEvent(commandList);
}

// Helper method to clear the back buffers.
void Sample::Clear(bool includenormals)
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    scissorRect.bottom = static_cast<LONG>(scissorRect.bottom);
    scissorRect.right = static_cast<LONG>(scissorRect.right);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

    dsvDescriptor = m_deviceResources->GetDepthStencilView();

    if (includenormals)
    {

        TransitionTo(commandList, m_normals, &m_normalsState, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];
        rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV);
        rtvs[1] = m_renderDescriptors->GetCpuHandle(RTDescriptors::NormalsRTV);

        commandList->OMSetRenderTargets(2, &rtvs[0], FALSE, &dsvDescriptor);

        commandList->ClearRenderTargetView(rtvs[0], ATG::ColorsLinear::Background, 0, nullptr);

        float clearVal[4] = { 0.5f , 0.5f , 1.0f , 1.0f };
        commandList->ClearRenderTargetView(rtvs[1], clearVal, 0, nullptr);
    }
    else
    {
        rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

        commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    }

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);


    // Use linear clear color for gamma-correct rendering.
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();

    m_font.reset();
    m_colorCtrlFont.reset();
    m_ctrlFont.reset();

    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();

    m_gltfModel->Unload();
    m_gltfResources.OnDestroy();
    m_gltfPBR->OnDestroy();
    m_gltfMotionVectors->OnDestroy();
    m_gltfRTShadows->OnDestroy(); 
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_3 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_3))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.3 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.3 is not supported!");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData, sizeof(featureData)))
        || (featureData.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED))
    {
        extern LPCWSTR g_szAppName;
        std::ignore = MessageBoxW(nullptr, L"D3D12 device does not support DXR", g_szAppName, MB_ICONERROR | MB_OK);
        throw std::runtime_error("DXR is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    m_depthDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        DSDescriptors::DSCount);

    m_states = std::make_unique<CommonStates>(device);
    m_gltfModel = new AMDTK::GLTFFile();

    wchar_t filepath[MAX_PATH];
#ifdef _GAMING_XBOX
    DX::FindMediaFile(filepath, _countof(filepath), L"Corridor.gltf");
#else
    DX::FindMediaFile(filepath, _countof(filepath), L"AMDCorridor\\Corridor.gltf");
#endif
     
    bool status = m_gltfModel->Load(filepath);

    if (!status)
    {
        std::string message = "The model couldn't be found";
        assert(false);
    }
    
    // Force to use user defined lights below
    m_gltfModel->ClearLights();

    {
        float yaw = 0.0f;
        float pitch = 0.0f;
        float factor = 3.5;

        tfNode n;
        XMVECTOR x = XMVectorSet(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
        x = XMVectorMultiply(x, XMVectorSet(factor, factor, factor, factor));
        n.m_tranform.LookAt(x, XMVectorSet(0, 0, 0, 0));


        tfLight l;
        l.m_type = tfLight::LIGHT_POINTLIGHT;
        l.m_intensity = 0.9f;
        l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
        l.m_range = 20.0f;
        l.m_outerConeAngle = XM_PI / 4.0f;
        l.m_innerConeAngle = (XM_PI / 4.0f) * 0.9f;

        m_gltfModel->AddLight(n, l);

        n.m_tranform.LookAt(XMVectorSet(0.0f, 2.8f, -6.0f, 0.0f), XMVectorSet(0, 0, 0, 0));
        l.m_type = tfLight::LIGHT_SPOTLIGHT;
        l.m_intensity = 0.8f;
        l.m_color = XMVectorSet(0.8f, 0.3f, 0.3f, 0.0f);
        l.m_range = 25.0f;
        l.m_outerConeAngle = XM_PI / 2.0f;
        l.m_innerConeAngle = (XM_PI / 2.0f) * 0.6f;

        m_gltfModel->AddLight(n, l);

    }

     
    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_gltfResources.OnCreate(device, m_gltfModel, &upload , m_graphicsMemory.get());
    // here we are loading onto the GPU all the textures and the inverse matrices
    // this data will be used to create the passes 
    m_gltfResources.LoadTextures(m_gltfModel->GetFilePath().c_str());

    m_gltfPBR = new AMDTK::GltfPbrPass();
    m_gltfPBR->OnCreate(
        device,
        &m_gltfResources,
        m_scene->GetFormat(),
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_R16G16B16A16_FLOAT, //export normals
        1
    ); 

    m_gltfMotionVectors = new AMDTK::GltfMotionVectorsPass();
    m_gltfMotionVectors->OnCreate(
        device,
        &m_gltfResources
    );



    m_gltfRTShadows = new AMDTK::GltfRTShadowPass();
    m_gltfRTShadows->OnCreate(
        device,
        &m_gltfResources,
        m_deviceResources.get()
    );

    const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    //Now the resources have uploded from the gltf passes, we can initiate DXR AS generation
    m_gltfRTShadows->DoASBuild();

    // Render targets
    m_scene->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV));

    // Post-process quad
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        auto rootSig = m_fullScreenQuad->GetRootSignature();
        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_passthroughPSO.ReleaseAndGetAddressOf());
    }

    {
        //Shadow composition CS
        
            auto computeShaderBlob = DX::ReadData(L"CompositeShadowsCS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_compositeShadowsRS.ReleaseAndGetAddressOf())));

        m_compositeShadowsRS->SetName(L"CompositeShadowsCS RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_compositeShadowsRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_compositeShadowsPSO.ReleaseAndGetAddressOf())));

        m_compositeShadowsPSO->SetName(L"CompositeShadowsCS PSO");
    }


    {
        // Denoiser PSO objects
        {
            //prepare_shadow_mask_d3d12
            auto computeShaderBlob = DX::ReadData(L"prepare_shadow_mask_d3d12.cso");

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_prepareShadowMapRS.ReleaseAndGetAddressOf())));

            m_prepareShadowMapRS->SetName(L"prepare_shadow_mask_d3d12 RS");

            // Create compute pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_prepareShadowMapRS.Get();
            descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
            descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

            DX::ThrowIfFailed(
                device->CreateComputePipelineState(&descComputePSO,
                    IID_GRAPHICS_PPV_ARGS(m_prepareShadowMapPSO.ReleaseAndGetAddressOf())));

            m_prepareShadowMapPSO->SetName(L"prepare_shadow_mask_d3d12 PSO");
        }
        {
            //tile_classification_d3d12
            auto computeShaderBlob = DX::ReadData(L"tile_classification_d3d12.cso");

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_denoiseTileClassificationRS.ReleaseAndGetAddressOf())));

            m_denoiseTileClassificationRS->SetName(L"tile_classification_d3d12 RS");

            // Create compute pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_denoiseTileClassificationRS.Get();
            descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
            descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

            DX::ThrowIfFailed(
                device->CreateComputePipelineState(&descComputePSO,
                    IID_GRAPHICS_PPV_ARGS(m_denoiseTileClassificationPSO.ReleaseAndGetAddressOf())));

            m_denoiseTileClassificationPSO->SetName(L"tile_classification_d3d12 PSO");
        }
        {
            //filter_soft_shadows_pass_0_d3d12
            auto computeShaderBlob = DX::ReadData(L"filter_soft_shadows_pass_0_d3d12.cso");

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass0RS.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass0RS->SetName(L"filter_soft_shadows_pass_0_d3d12 RS");

            // Create compute pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_denoiseFilterPass0RS.Get();
            descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
            descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

            DX::ThrowIfFailed(
                device->CreateComputePipelineState(&descComputePSO,
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass0PSO.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass0PSO->SetName(L"filter_soft_shadows_pass_0_d3d12 PSO"); 
        }
        {
            //filter_soft_shadows_pass_1_d3d12
            auto computeShaderBlob = DX::ReadData(L"filter_soft_shadows_pass_1_d3d12.cso");

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass1RS.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass1RS->SetName(L"filter_soft_shadows_pass_1_d3d12 RS");

            // Create compute pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_denoiseFilterPass1RS.Get();
            descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
            descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

            DX::ThrowIfFailed(
                device->CreateComputePipelineState(&descComputePSO,
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass1PSO.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass1PSO->SetName(L"filter_soft_shadows_pass_1_d3d12 PSO");
        }
        {
            //filter_soft_shadows_pass_2_d3d12
            auto computeShaderBlob = DX::ReadData(L"filter_soft_shadows_pass_2_d3d12.cso");

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass2RS.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass2RS->SetName(L"filter_soft_shadows_pass_2_d3d12 RS");

            // Create compute pipeline state
            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_denoiseFilterPass2RS.Get();
            descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
            descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

            DX::ThrowIfFailed(
                device->CreateComputePipelineState(&descComputePSO,
                    IID_GRAPHICS_PPV_ARGS(m_denoiseFilterPass2PSO.ReleaseAndGetAddressOf())));

            m_denoiseFilterPass2PSO->SetName(L"filter_soft_shadows_pass_2_d3d12 PSO");
        }
    }

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
}



// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

    auto const size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();


    wchar_t strFilePath[MAX_PATH] = {};

    if (size.bottom > 1440)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    }
    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    if (size.bottom > 1440)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    }
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

    if (size.bottom > 1440)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerSmall.spritefont");
    }
    m_colorCtrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ColorCtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ColorCtrlFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

   
    // Offscreen render targets
    m_scene->SetWindow(size);

    auto res = m_scene->GetResource();
    if (res)
        res->SetName(L"Scene");

    // Camera
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 1000.f);


    UINT resourceSizeWidth{ static_cast<UINT>(size.right) };
    UINT resourceSizeHeight{ static_cast<UINT>(size.bottom) };

    CreateResource(m_noisyRTResult, L"Noisy RT Shadows", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_noisyRTResultState,
        { 0u }, m_resourceDescriptors->GetCpuHandle(Descriptors::NoisyRTResultSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::NoisyRTResultUAV), { 0u });

    CreateResource(m_finalScene, L"FinalScene", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_finalSceneOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FinalSceneSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FinalSceneUAV), { 0 });

    CreateResource(m_denoisedOutput, L"DenoisedOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_denoisedOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::DenoisedOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::DenoisedOutputUAV), { 0 });

    D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_R16G16B16A16_FLOAT, {1.0f,1.0f,1.0f,1.0f} };
    D3D12_CLEAR_VALUE normalsClearValue = { DXGI_FORMAT_R16G16B16A16_FLOAT, {0.5f,0.5f,1.0f,1.0f} };

    CreateResource(m_normals, L"Normals", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &normalsClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_normalsState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::NormalsRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::NormalsSRV), { 0 }, { 0 });

    D3D12_CLEAR_VALUE motionDepthClearValue = {};
    motionDepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    motionDepthClearValue.DepthStencil.Depth = 1.0f;
    motionDepthClearValue.DepthStencil.Stencil = 0;

    CreateResource(m_motionVectorDepth, L"MotionVectorDepth", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &motionDepthClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_DEPTH_WRITE, &m_motionVectorDepthState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::MotionVectorDepthSRV), { 0 }, m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV));

    CreateResource(m_previousDepth, L"PreviousDepth", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &motionDepthClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_DEPTH_WRITE, &m_previousDepthState,
        { 0u }, m_resourceDescriptors->GetCpuHandle(Descriptors::PreviousDepthSRV), { 0 }, m_depthDescriptors->GetCpuHandle(DSDescriptors::PreviousDepthDSV));

    D3D12_CLEAR_VALUE motionClearValue = { DXGI_FORMAT_R16G16_FLOAT, {0.0f,0.0f,0.0f,0.0f} };
    CreateResource(m_motionVectors, L"MotionVectors", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &motionClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_motionVectorsState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::MotionVectorsSRV), { 0 }, { 0 });

    CreateResource(m_historyMomentsBuffer, L"MomentsBuffer0", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_historyMomentsBufferState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::MomentsBuffer0SRV), m_resourceDescriptors->GetCpuHandle(Descriptors::MomentsBuffer0UAV), { 0 });

    CreateResource(m_momentsBuffer, L"MomentsBuffer1", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R11G11B10_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_momentsBufferState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::MomentsBuffer1SRV), m_resourceDescriptors->GetCpuHandle(Descriptors::MomentsBuffer1UAV), { 0 });

    CreateResource(m_reprojectionResultIntermediateBuffer, L"Intermediate Buffer And Reprojection Target", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_reprojectionResultIntermediateBufferState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateReprojectionSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateReprojectionUAV), { 0 });

    CreateResource(m_intermediateFilterResult, L"Intermediate Filter Target", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_intermediateFilterResultState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateFilterSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateFilterUAV), { 0 });

    CreateResource(m_intermediateFilterResult2, L"Intermediate Filter Target2", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_intermediateFilterResult2State,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateFilter2SRV), m_resourceDescriptors->GetCpuHandle(Descriptors::IntermediateFilter2UAV), { 0 });


    CreateResource(m_historyBuffer0, L"HistoryBuffer0", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_historyBuffer0State,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::HistoryBuffer0SRV), m_resourceDescriptors->GetCpuHandle(Descriptors::HistoryBuffer0UAV), { 0 });

    CreateResource(m_historyBuffer1, L"HistoryBuffer1", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_historyBuffer1State,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::HistoryBuffer1SRV), m_resourceDescriptors->GetCpuHandle(Descriptors::HistoryBuffer1UAV), { 0 });

    {
        //Tile Metadata Buffer
        D3D12_HEAP_PROPERTIES heap_properties = { D3D12_HEAP_TYPE_DEFAULT };

        // 8x8 tiles
        UINT element_count = ((resourceSizeWidth + 7u) / 8u) * ((resourceSizeHeight + 7u) / 8u);
        UINT element_size_in_bytes = 4u;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = element_count * element_size_in_bytes;
        resource_desc.Height = 1u;
        resource_desc.DepthOrArraySize = 1u;
        resource_desc.MipLevels = 1u;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1u;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to NON_PIXEL_SHADER_RESOURCE on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_tileMetadata.ReleaseAndGetAddressOf())));

        m_tileMetadata->SetName(L"Tile Metadata");

        D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
        shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        shader_resource_view_desc.Buffer.FirstElement = 0;
        shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        shader_resource_view_desc.Buffer.NumElements = element_count;
        shader_resource_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(m_tileMetadata.Get(), &shader_resource_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::TileMetadataSRV));

        D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
        unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
        unordered_access_view_desc.Buffer.FirstElement = 0;
        unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        unordered_access_view_desc.Buffer.NumElements = element_count;
        unordered_access_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        unordered_access_view_desc.Format = DXGI_FORMAT_UNKNOWN;

        device->CreateUnorderedAccessView(m_tileMetadata.Get(), nullptr, &unordered_access_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::TileMetadataUAV));

        m_tileMetadataState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    {
        //Sobol buffer
        D3D12_HEAP_PROPERTIES heap_properties = { D3D12_HEAP_TYPE_DEFAULT };

        // 256x256 int array
        UINT element_count = 256*256;
        UINT element_size_in_bytes = 4u;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = element_count * element_size_in_bytes;
        resource_desc.Height = 1u;
        resource_desc.DepthOrArraySize = 1u;
        resource_desc.MipLevels = 1u;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1u;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_COPY_DEST,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_sobolBuffer.ReleaseAndGetAddressOf())));

        m_sobolBuffer->SetName(L"SobolBuffer");

        D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
        shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        shader_resource_view_desc.Buffer.FirstElement = 0;
        shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        shader_resource_view_desc.Buffer.NumElements = element_count;
        shader_resource_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(m_sobolBuffer.Get(), &shader_resource_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::SobolBufferSRV));

        D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
        unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
        unordered_access_view_desc.Buffer.FirstElement = 0;
        unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        unordered_access_view_desc.Buffer.NumElements = element_count;
        unordered_access_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        unordered_access_view_desc.Format = DXGI_FORMAT_UNKNOWN;

        device->CreateUnorderedAccessView(m_sobolBuffer.Get(), nullptr, &unordered_access_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::SobolBufferUAV));


        m_sobolBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    { 
        D3D12_HEAP_PROPERTIES heap_properties = { D3D12_HEAP_TYPE_DEFAULT };

        // 256x256x8 int array
        UINT element_count = 128 * 128 * 8;
        UINT element_size_in_bytes = 4u;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = element_count * element_size_in_bytes;
        resource_desc.Height = 1u;
        resource_desc.DepthOrArraySize = 1u;
        resource_desc.MipLevels = 1u;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1u;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_COPY_DEST,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_scramblingTileBuffer.ReleaseAndGetAddressOf())));

        m_scramblingTileBuffer->SetName(L"ScramblingTileBuffer");

        D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
        shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        shader_resource_view_desc.Buffer.FirstElement = 0;
        shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        shader_resource_view_desc.Buffer.NumElements = element_count;
        shader_resource_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(m_scramblingTileBuffer.Get(), &shader_resource_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::ScramblingTileBufferSRV));

        D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
        unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
        unordered_access_view_desc.Buffer.FirstElement = 0;
        unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        unordered_access_view_desc.Buffer.NumElements = element_count;
        unordered_access_view_desc.Buffer.StructureByteStride = element_size_in_bytes;
        unordered_access_view_desc.Format = DXGI_FORMAT_UNKNOWN;

        device->CreateUnorderedAccessView(m_scramblingTileBuffer.Get(), nullptr, &unordered_access_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::SobolBufferUAV));

        m_scramblingTileBufferState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    {
        // Raytrace Result buffer (compressed 1bpp)

        D3D12_HEAP_PROPERTIES heap_properties = { D3D12_HEAP_TYPE_DEFAULT };


        //8x4 pixels into 4byte uint
        UINT element_count = ((resourceSizeWidth + 7u) / 8u) * ((resourceSizeHeight + 3u) / 4u); 

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = element_count * 4u;
        resource_desc.Height = 1u;
        resource_desc.DepthOrArraySize = 1u;
        resource_desc.MipLevels = 1u;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1u;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to NON_PIXEL_SHADER_RESOURCE on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_raytraceResult.ReleaseAndGetAddressOf())));

        m_raytraceResult->SetName(L"Raytrace Result Buffer");

        D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
        shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        shader_resource_view_desc.Buffer.FirstElement = 0;
        shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        shader_resource_view_desc.Buffer.NumElements = element_count;
        shader_resource_view_desc.Buffer.StructureByteStride = 4;
        shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
        shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(m_raytraceResult.Get(), &shader_resource_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RaytraceResultSRV));

        D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
        unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        unordered_access_view_desc.Buffer.CounterOffsetInBytes = 0;
        unordered_access_view_desc.Buffer.FirstElement = 0;
        unordered_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        unordered_access_view_desc.Buffer.NumElements = element_count;
        unordered_access_view_desc.Buffer.StructureByteStride = 4;
        unordered_access_view_desc.Format = DXGI_FORMAT_UNKNOWN;

        device->CreateUnorderedAccessView(m_raytraceResult.Get(), nullptr, &unordered_access_view_desc,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RaytraceResultUAV));

        m_raytraceResultState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
         

 
    // shadow Atlas generation
    {
        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        CreateResource(m_shadowAtlas, L"Shadow Atlas", 2048, 2048,
            DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depthOptimizedClearValue,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr,
            { 0 }, { 0 }, { 0 }, m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV));

        // SRV of different format
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;

        m_shadowAtlasIdx = m_gltfResources.GetSrvPile()->Allocate();

        device->CreateShaderResourceView(m_shadowAtlas.Get(), &srvDesc, m_gltfResources.GetSrvPile()->GetCpuHandle(m_shadowAtlasIdx));
    }
}

void Sample::InitNoiseBuffers(ID3D12GraphicsCommandList* commandList)
{
    if (m_sobolBufferState == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        size_t size = 256 * 256 * 4;
        auto res = GraphicsMemory::Get().Allocate(size, 16, GraphicsMemory::TAG_COMPUTE);
        std::memcpy(res.Memory(), &SobolBuffer[0], size);

        commandList->CopyBufferRegion(m_sobolBuffer.Get(), 0, res.Resource(), res.ResourceOffset(), size);
        TransitionResource(commandList, m_sobolBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_sobolBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (m_scramblingTileBufferState == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        size_t size = 128 * 128 * 8 * 4;
        auto res = GraphicsMemory::Get().Allocate(size, 16, GraphicsMemory::TAG_COMPUTE);
        std::memcpy(res.Memory(), &ScramblingTileBuffer[0], size);

        commandList->CopyBufferRegion(m_scramblingTileBuffer.Get(), 0, res.Resource(), res.ResourceOffset(), size);
        TransitionResource(commandList, m_scramblingTileBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_scramblingTileBufferState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
}

#pragma endregion

void Sample::TransitionTo(ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES* stateTracker, D3D12_RESOURCE_STATES afterState)
{
    assert(stateTracker != nullptr);
    if (*stateTracker != afterState)
    {
        TransitionResource(commandList, resource.Get(), *stateTracker, afterState);
        *stateTracker = afterState;
    }
}

// Helper for 2D resources
void Sample::CreateResource(
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
    LPCWSTR name,
    UINT64 width, UINT height,
    DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_CLEAR_VALUE* clearValue,
    CD3DX12_HEAP_PROPERTIES heapProps,
    D3D12_RESOURCE_STATES initalState,
    D3D12_RESOURCE_STATES* stateTracker,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
    )
{
    auto device = m_deviceResources->GetD3DDevice();

    const CD3DX12_HEAP_PROPERTIES heapProperties(heapProps);

    {
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.MipLevels = 1;
        texDesc.Format = format;
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.Flags = flags;
        texDesc.DepthOrArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                initalState,
                clearValue,
                IID_GRAPHICS_PPV_ARGS(resource.ReleaseAndGetAddressOf())));

        resource->SetName(name);

        if (rtvHandle.ptr)
        {
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = format;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);
        }

        if (uavHandle.ptr)
        {
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;

            device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, uavHandle);
        }

        if (srvHandle.ptr)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

            if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            { 
                srvDesc.Format = format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_R32_FLOAT : format;
            }
            else
            {
                srvDesc.Format = format;
            }

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0;

            device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
        }

        if (dsvHandle.ptr)
        {
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_D32_FLOAT : format;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

            device->CreateDepthStencilView(resource.Get(), &dsvDesc, dsvHandle);
        }

        if (stateTracker != nullptr)
        {
            *stateTracker = initalState;
        }
    }
}
