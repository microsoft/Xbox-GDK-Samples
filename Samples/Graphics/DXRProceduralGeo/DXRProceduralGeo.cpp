//--------------------------------------------------------------------------------------
// DXRProceduralGeo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DXRProceduralGeo.h"
#include "CompiledShaders\Raytracing.inc"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace InlineRootSignature;

using Microsoft::WRL::ComPtr;

namespace
{
    // Constants
    constexpr float c_aabbWidth = 2.0f;      // AABB width.
    constexpr float c_aabbDistance = 2.0f;   // Distance between AABBs.

    constexpr Vector3 CameraSrc = { -11.27f, 7.66f, -11.79f };
    constexpr Vector3 CameraDst = { -1.9f, 1.6f, -1.8f };
    constexpr float CameraNear = 0.1f;
    constexpr float CameraFar = 1000.0f;

    // Shader entry points.
    static const wchar_t* c_raygenShaderName = L"MyRaygenShader";

    const wchar_t* c_intersectionShaderNames[] =
    {
        L"MyIntersectionShader_AnalyticPrimitive",
        L"MyIntersectionShader_VolumetricPrimitive",
        L"MyIntersectionShader_SignedDistancePrimitive",
    };

    const wchar_t* c_closestHitShaderNames[] =
    {
        L"MyClosestHitShader_Triangle",
        L"MyClosestHitShader_AABB",
    };

    const wchar_t* c_missShaderNames[] =
    {
        L"MyMissShader",
#ifdef _GAMING_DESKTOP
        L"MyMissShader_ShadowRay"
#else
#ifdef BUG_47048430_IS_FIXED
        L""
#else
        L"MyMissShader_ShadowRay"
#endif // BUG_47048430_IS_FIXED
#endif // _GAMING_DESKTOP
    };

    // Hit groups.
    const wchar_t* c_hitGroupNames_TriangleGeometry[] =
    {
        L"MyHitGroup_Triangle",
        L"MyHitGroup_Triangle_ShadowRay"
    };

    const wchar_t* c_hitGroupNames_AABBGeometry[][RayType::Count] =
    {
        { L"MyHitGroup_AABB_AnalyticPrimitive", L"MyHitGroup_AABB_AnalyticPrimitive_ShadowRay" },
        { L"MyHitGroup_AABB_VolumetricPrimitive", L"MyHitGroup_AABB_VolumetricPrimitive_ShadowRay" },
        { L"MyHitGroup_AABB_SignedDistancePrimitive", L"MyHitGroup_AABB_SignedDistancePrimitive_ShadowRay" },
    };
}

Sample::Sample() noexcept(false) :
    m_width(0u),
    m_height(0u),
    m_frame(0u),
    m_gpuTimerMeasuresMS{},
    m_animateGeometryTime(0.0f),
    m_animateGeometry(true),
    m_animateLight(false),
    m_useInlineRaytracing(false),
    m_pSceneCBMappedData(nullptr),
    m_pAabbPrimitiveAttributeMappedData(nullptr),
    m_DXROutputResourceUAVGpuDescriptorHandle{},
    m_indexBufferGPUHandle{},
    m_vertexBufferGPUHandle{}
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        3, DX::DeviceResources::c_EnableDXR | DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_sceneCBResource.Get() && m_pSceneCBMappedData)
    {
        m_sceneCBResource->Unmap(0, nullptr);
        m_pSceneCBMappedData = nullptr;
    }

    if (m_aabbPrimitiveAttributeBufferResource.Get() && m_pAabbPrimitiveAttributeMappedData)
    {
        m_aabbPrimitiveAttributeBufferResource->Unmap(0, nullptr);
        m_pAabbPrimitiveAttributeMappedData = nullptr;
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

    // Initialize constant buffers
    InitializeConstantBuffers();

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Create the gpu timer
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
        m_camera.Update(elapsedTime, pad);

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::RELEASED)
        {
            m_animateGeometry = !m_animateGeometry;
        }

        if (m_gamePadButtons.b == ButtonState::RELEASED)
        {
            m_animateLight = !m_animateLight;
        }

#ifdef _GAMING_XBOX_SCARLETT
        if (m_gamePadButtons.x == ButtonState::RELEASED)
        {
            m_useInlineRaytracing = !m_useInlineRaytracing;
        }
#endif
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

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::V))
    {
        m_animateLight = !m_animateLight;
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::C))
    {
        m_animateGeometry = !m_animateGeometry;
    }

#ifdef _GAMING_XBOX_SCARLETT
    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::X))
    {
        m_useInlineRaytracing = !m_useInlineRaytracing;
    }
#endif

    // Update geometry and buffers
    auto frameIndex = m_deviceResources->GetCurrentFrameIndex();

    // Rotate the second light around Y axis.
    if (m_animateLight)
    {
        float rotationPeriod = 8.0f;
        float angleToRotateBy = -360.0f * (elapsedTime / rotationPeriod);
        XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
        m_sceneCB.lightPosition = XMVector3Transform(m_sceneCB.lightPosition, rotate);
    }

    // Transform the procedural geometry.
    if (m_animateGeometry)
    {
        m_animateGeometryTime += elapsedTime;
    }

    UpdateAABBPrimitiveAttributes(m_animateGeometryTime);
    m_sceneCB.elapsedTime = m_animateGeometryTime;

    Matrix proj = m_camera.GetProjection();
    Matrix view = m_camera.GetView();

    m_sceneCB.cameraPosition = m_camera.GetPosition();
    m_sceneCB.projectionToWorld = (view * proj).Invert().Transpose();
    m_sceneCB.reflectance = 0.8f;
    memcpy(&(m_pSceneCBMappedData[frameIndex]), &m_sceneCB, sizeof(m_sceneCB));

    m_sceneCB.width = m_width;
    m_sceneCB.height = m_height;

    PIXEndEvent();
}

// Update AABB primitive attributes buffers passed into the shader.
void Sample::UpdateAABBPrimitiveAttributes(float animationTime)
{
    uint32_t frameIndex = m_deviceResources->GetCurrentFrameIndex();

    Matrix mIdentity;
    Matrix mScale15y = XMMatrixScaling(1.0f, 1.5f, 1.0f);
    Matrix mScale15 = XMMatrixScaling(1.5f, 1.5f, 1.5f);
    Matrix mScale3 = XMMatrixScaling(3.0f, 3.0f, 3.0f);
    Matrix mRotation = XMMatrixRotationY(-2.0f * animationTime);

    // Apply scale, rotation and translation transforms.
    // The intersection shader tests in this sample work with local space, so here
    // we apply the BLAS object space translation that was passed to geometry descs.
    auto SetTransformForAABB = [&](size_t primitiveIndex, Matrix& mScale, Matrix& mRotation)
    {
        XMVECTOR vTranslation = 0.5f * (
            Vector3(m_AABBList[primitiveIndex].MinX, m_AABBList[primitiveIndex].MinY, m_AABBList[primitiveIndex].MinZ) +
            Vector3(m_AABBList[primitiveIndex].MaxX, m_AABBList[primitiveIndex].MaxY, m_AABBList[primitiveIndex].MaxZ));

        Matrix mTranslation = XMMatrixTranslationFromVector(vTranslation);
        Matrix mTransform = mScale * mRotation * mTranslation;
        Matrix invTransform = XMMatrixInverse(nullptr, mTransform);
        mTransform = mTransform.Transpose();
        invTransform = invTransform.Transpose();

        // Update buffer
        void* addrDest01 = reinterpret_cast<void*>(&(m_pAabbPrimitiveAttributeMappedData[frameIndex * IntersectionShaderType::TotalPrimitiveCount + primitiveIndex].localSpaceToBLAS));
        void* addrDest02 = reinterpret_cast<void*>(&(m_pAabbPrimitiveAttributeMappedData[frameIndex * IntersectionShaderType::TotalPrimitiveCount + primitiveIndex].BLASToLocalSpace));
        memcpy(addrDest01, &(mTransform), sizeof(mTransform));
        memcpy(addrDest02, &(invTransform), sizeof(invTransform));
    };

    uint32_t offset = 0;
    // Analytic primitives.
    {
        SetTransformForAABB(offset + AnalyticPrimitive::AABB, mScale15y, mIdentity);
        SetTransformForAABB(offset + AnalyticPrimitive::Spheres, mScale15, mRotation);
        offset += AnalyticPrimitive::Count;
    }

    // Volumetric primitives.
    {
        SetTransformForAABB(offset + VolumetricPrimitive::Metaballs, mScale15, mRotation);
        offset += VolumetricPrimitive::Count;
    }

    // Signed distance primitives.
    {
        SetTransformForAABB(offset + SignedDistancePrimitive::MiniSpheres, mIdentity, mIdentity);
        SetTransformForAABB(offset + SignedDistancePrimitive::IntersectedRoundCube, mIdentity, mIdentity);
        SetTransformForAABB(offset + SignedDistancePrimitive::SquareTorus, mScale15, mIdentity);
        SetTransformForAABB(offset + SignedDistancePrimitive::TwistedTorus, mIdentity, mRotation);
        SetTransformForAABB(offset + SignedDistancePrimitive::Cog, mIdentity, mRotation);
        SetTransformForAABB(offset + SignedDistancePrimitive::Cylinder, mScale15y, mIdentity);
        SetTransformForAABB(offset + SignedDistancePrimitive::FractalPyramid, mScale3, mIdentity);
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Begin frame timer
    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer->BeginFrame(commandList);

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    ///////////////////////////
    ///  Dispatch rays      ///
    ///////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DXR Render");
    m_gpuTimer->Start(commandList, TIMER_RAYTRACING_PASS);

    auto frameIndex = m_deviceResources->GetCurrentFrameIndex();

    commandList->SetComputeRootSignature(m_DXRGlobalRootSignature.Get());

    auto heap = m_descriptorHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    if (m_useInlineRaytracing)
    {
        // Set compute PSO
        commandList->SetPipelineState(m_InlineRaytracingPSO.Get());

        // RootS and descriptor heap stuff
        commandList->SetComputeRootSignature(m_InlineRaytracingRS.Get());

        // Bind acceleration structure
        commandList->SetComputeRootShaderResourceView(RootParams::AccelerationStructure, m_topLevelAS->GetGPUVirtualAddress());

        // Bind shader table
        commandList->SetComputeRootShaderResourceView(RootParams::ShaderTable, m_shaderBindingTable.GetHitGroupShaderTable().StartAddress);

        // SceneConstantBuffer
        uint64_t sceneCBVirtualAddress = m_sceneCBResource->GetGPUVirtualAddress() + frameIndex * sizeof(SceneConstantBuffer);
        commandList->SetComputeRootConstantBufferView(RootParams::SceneConstant, sceneCBVirtualAddress/*gpu descriptor handle*/);

        // Bind SRV structured buffer
        uint64_t virtualAddressOffset = frameIndex * static_cast<uint64_t>(IntersectionShaderType::TotalPrimitiveCount) * sizeof(PrimitiveInstancePerFrameBuffer);
        uint64_t aabbPrimitiveAttributeVirtualAddress = m_aabbPrimitiveAttributeBufferResource->GetGPUVirtualAddress() + virtualAddressOffset;
        commandList->SetComputeRootShaderResourceView(RootParams::AABBattributeBuffer, aabbPrimitiveAttributeVirtualAddress);

        // Set index and vertex buffer in descriptor table.
        commandList->SetComputeRootDescriptorTable(RootParams::VertexBuffers, m_indexBufferGPUHandle);

        // Bind output UAV
        commandList->SetComputeRootDescriptorTable(RootParams::OutputView, m_DXROutputResourceUAVGpuDescriptorHandle/*gpu descriptor handle*/);

        uint32_t xThreadCount = 8;
        uint32_t yThreadCount = 4;
        uint32_t groupX = (m_width + xThreadCount - 1) / xThreadCount;
        uint32_t groupY = (m_height + yThreadCount - 1) / yThreadCount;
        commandList->Dispatch(groupX, groupY, 1);
    }
    else
    {
        // Bind scene ConstantBuffer
        uint64_t sceneCBVirtualAddress = m_sceneCBResource->GetGPUVirtualAddress() + frameIndex * sizeof(SceneConstantBuffer);
        commandList->SetComputeRootConstantBufferView(GlobalRootSignature::Slot::SceneConstant, sceneCBVirtualAddress);

        // Bind SRV structured buffer
        uint64_t virtualAddressOffset = frameIndex * static_cast<uint64_t>(IntersectionShaderType::TotalPrimitiveCount) * sizeof(PrimitiveInstancePerFrameBuffer);
        uint64_t aabbPrimitiveAttributeVirtualAddress = m_aabbPrimitiveAttributeBufferResource->GetGPUVirtualAddress() + virtualAddressOffset;
        commandList->SetComputeRootShaderResourceView(GlobalRootSignature::Slot::AABBattributeBuffer, aabbPrimitiveAttributeVirtualAddress);

        // Set index and vertex buffer in descriptor table.
        commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::VertexBuffers, m_indexBufferGPUHandle);

        commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::OutputView, m_DXROutputResourceUAVGpuDescriptorHandle);

        commandList->SetComputeRootShaderResourceView(GlobalRootSignature::Slot::AccelerationStructure, m_topLevelAS->GetGPUVirtualAddress());

        // Set pipeline state
        commandList->SetPipelineState1(m_dxrStateObject.Get());

        // Bind the heaps, acceleration structure and dispatch rays.
        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        dispatchDesc.RayGenerationShaderRecord = m_shaderBindingTable.GetRayGenerationRecord(0);
        dispatchDesc.MissShaderTable = m_shaderBindingTable.GetMissShaderTable();
        dispatchDesc.HitGroupTable = m_shaderBindingTable.GetHitGroupShaderTable();
        dispatchDesc.Width = m_width;
        dispatchDesc.Height = m_height;
        dispatchDesc.Depth = 1;

        // Title allocated scratch for DXR. Remember to disable driver allocated scratch
        // on device creation (pDXRStackBuffer = -1).
#ifdef _GAMING_XBOX_SCARLETT
        static D3D12_GPU_VIRTUAL_ADDRESS scratchAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        static uint64_t sizeBytes = 0ULL;
        if (scratchAddress == D3D12_GPU_VIRTUAL_ADDRESS_NULL)
    {
        // Using GetInformationX, we can get data on how much scratch is consumed per lane by the dispatchRay
        D3D12XBOX_RAYTRACING_INFORMATION stats{};
        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        DX::ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));
        stateObjectProperties->GetInformationX(D3D12XBOX_RAYTRACING_INFORMATION_FLAG_GET_BASE_INFO, &stats);

        // We can then calculate how much is the maximum amount of scratch the sample needs for DXR. This amount
        // can be more than what the driver allocates or less. If its more, it can help occupancy, while if less,
        // it can save memory for the title. Here we are calculating the maximum scratch we would need if we had
        // full occupancy (DXR has a cap on 16 waves per SIMD in occupancy, due to the traversal code using 2KB LDS).
        bool runsOnAnaconda = m_deviceResources->GetConsoleType() == DX::ConsoleType::ANACONDA;
        uint64_t threadsPerWave = 32ULL;
        uint64_t maxSimdOccupancy = 16ULL;
        uint64_t numWgps = (runsOnAnaconda) ? 26ULL : 10ULL;
        uint64_t simdsPerWgp = 4ULL;

        // Scratch is allocated in blocks of 1024 bytes. To get the safe allocation for one wave, we
        // need to compute ScratchSize * 32 (XDXR always runs wave 32 internally) and round it up to 1KB.
        sizeBytes = (stats.BaseInfo.ScratchSize + stats.BaseInfo.SftStackSize) * threadsPerWave;
        sizeBytes = (sizeBytes + 1023) / 1024 * 1024;
        sizeBytes *= maxSimdOccupancy * numWgps * simdsPerWgp;

        // Allocate the scratch. This is used in DXR for the stack used during traversal, as well as
        // the maximum of all scratch used by the shader exports.
        scratchAddress = (D3D12_GPU_VIRTUAL_ADDRESS) XMemVirtualAlloc(nullptr,
            sizeBytes,
            MEM_2MB_PAGES | MEM_RESERVE | MEM_COMMIT,
            XMEM_GRAPHICS,
            PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE);

        if (scratchAddress == D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
            DX::ThrowIfFailed(E_FAIL);
        }
    }

        D3D12XBOX_DISPATCH_RAYS_DESC dispatchDescX = {};
        dispatchDescX.DispatchArgs = dispatchDesc;
        dispatchDescX.Scratch.StartAddress = scratchAddress;
        dispatchDescX.Scratch.SizeInBytes = sizeBytes;

        commandList->DispatchRaysX(&dispatchDescX);
#else
        commandList->DispatchRays(&dispatchDesc);
#endif
    }

    m_gpuTimer->Stop(commandList, TIMER_RAYTRACING_PASS);
    m_gpuTimerMeasuresMS[TIMER_RAYTRACING_PASS] = m_gpuTimer->GetAverageMS(TIMER_RAYTRACING_PASS);
    PIXEndEvent(commandList);

    ///////////////////////////////////////////
    ///  CopyRaytracingOutputToBackbuffer   ///
    ///////////////////////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"CopyRaytracingOutputToBackbuffer");
    m_gpuTimer->Start(commandList, TIMER_COPY_PASS);

    auto renderTarget = m_deviceResources->GetRenderTarget();

    D3D12_RESOURCE_BARRIER preCopyBarriers[2];
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_DXROutputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(preCopyBarriers)), preCopyBarriers);

    commandList->CopyResource(renderTarget, m_DXROutputResource.Get());

    D3D12_RESOURCE_BARRIER postCopyBarriers[2];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_DXROutputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(static_cast<uint32_t>(std::size(postCopyBarriers)), postCopyBarriers);

    m_gpuTimer->Stop(commandList, TIMER_COPY_PASS);
    m_gpuTimerMeasuresMS[TIMER_COPY_PASS] = m_gpuTimer->GetAverageMS(TIMER_COPY_PASS);
    PIXEndEvent(commandList);

    //////////////////
    ///  DrawHUD   ///
    //////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD rendering");
    m_gpuTimer->Start(commandList, TIMER_HUD_PASS);

    DrawHUD();

    m_gpuTimer->Stop(commandList, TIMER_HUD_PASS);
    m_gpuTimerMeasuresMS[TIMER_HUD_PASS] = m_gpuTimer->GetAverageMS(TIMER_HUD_PASS);
    PIXEndEvent(commandList);

    // Show the new frame.
    m_gpuTimer->EndFrame(commandList);
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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

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

    ID3D12DescriptorHeap* heaps[] = { m_HUDDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Text is always rendered at 1080p
    RECT size = { 0, 0, 1920, 1080 };
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(uint32_t(size.right), uint32_t(size.bottom));

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    const XMVECTOR textColor1 = DirectX::Colors::DarkBlue;
    const XMVECTOR textColor2 = DirectX::Colors::Black;

    wchar_t buffer[100];
    {
        swprintf_s(buffer, std::size(buffer), L"DXR Procedural Geometry");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Current state section
        swprintf_s(buffer, std::size(buffer), L"Animated Geometry: %s", (m_animateGeometry) ? L"True" : L"False");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Animated Light: %s", (m_animateLight) ? L"True" : L"False");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

#ifdef _GAMING_XBOX_SCARLETT
        swprintf_s(buffer, std::size(buffer), L"Raytracing type: %s", (m_useInlineRaytracing) ? L"Inline RT" : L"DXR 1.0");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();
#endif

        // Timer section (render at bottom of screen)
        swprintf_s(buffer, std::size(buffer), L"Raytracing dispatch time: %.2f ms", m_gpuTimerMeasuresMS[TIMER_RAYTRACING_PASS]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"Backbuffer copy time: %.2f ms", m_gpuTimerMeasuresMS[TIMER_COPY_PASS]);
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor2);
        textPos.y += 2.0f * m_smallFont->GetLineSpacing();

        // Jump to bottom of screen
        textPos.y = size.bottom - (8.0f * m_smallFont->GetLineSpacing());

        // Instructions
#ifdef _GAMING_DESKTOP
        swprintf_s(buffer, std::size(buffer), L"[C] Toggle animated geometry");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[V] Toggle animated light");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        swprintf_s(buffer, std::size(buffer), L"[Esc] Exit Sample");
        m_smallFont->DrawString(m_hudBatch.get(), buffer, textPos, textColor1);

#else // _GAMING_XBOX
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A] Toggle animated geometry", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[B] Toggle animated light", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[X] Toggle raytracing type", textPos, textColor1);
        textPos.y += m_smallFont->GetLineSpacing();

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor1);
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

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Root Signatures & DXR Pipeline
void Sample::SerializeAndCreateDXRRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>& rootSig)
{
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    DX::ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
    if (error)
    {
        auto msg = static_cast<const char *>(error->GetBufferPointer());
        if (msg)
        {
            OutputDebugStringA(msg);
        }
        throw std::runtime_error("D3D12SerializeRootSignature");
    }

    // OG sample used nodemask 1
    auto device = m_deviceResources->GetD3DDevice();
    DX::ThrowIfFailed(device->CreateRootSignature(0/*NodeMask*/, blob->GetBufferPointer(), blob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(rootSig.ReleaseAndGetAddressOf())));
}

void Sample::CreateDXRRootSignatures()
{
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        // Performance TIP: Order from most frequent to least frequent usage.
        CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignature::Slot::Count] = {};
        rootParameters[GlobalRootSignature::Slot::OutputView].InitAsDescriptorTable(1, &ranges[0]);     // u0
        rootParameters[GlobalRootSignature::Slot::AccelerationStructure].InitAsShaderResourceView(0);   // t0
        rootParameters[GlobalRootSignature::Slot::SceneConstant].InitAsConstantBufferView(0);           // b0
        rootParameters[GlobalRootSignature::Slot::AABBattributeBuffer].InitAsShaderResourceView(3);     // t3
        rootParameters[GlobalRootSignature::Slot::VertexBuffers].InitAsDescriptorTable(1, &ranges[1]);  // t1,t2

        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(static_cast<uint32_t>(std::size(rootParameters)), rootParameters);
#ifdef _GAMING_XBOX_SCARLETT
        globalRootSignatureDesc.Flags |= D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING;
#endif
        SerializeAndCreateDXRRootSignature(globalRootSignatureDesc, m_DXRGlobalRootSignature);
        SetDebugObjectName(m_DXRGlobalRootSignature.Get(), L"GlobalRootSignature");
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        // Triangle geometry
        {
            CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
            rootParameters[/*MaterialConstant*/ 0].InitAsConstants(SizeOfInUint32(PrimitiveConstantBuffer), 1);

            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(static_cast<uint32_t>(std::size(rootParameters)), rootParameters);
            localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
            SerializeAndCreateDXRRootSignature(localRootSignatureDesc, m_DXRLocalRootSignature[LocalRootSignature::Type::Triangle]);
            SetDebugObjectName(m_DXRLocalRootSignature[0].Get(), L"LocalRootSignatureTriangles");
        }

        // AABB geometry
        {
            CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
            rootParameters[/*MaterialConstant */ 0].InitAsConstants(SizeOfInUint32(PrimitiveConstantBuffer), 1);
            rootParameters[/*GeometryIndex    */ 1].InitAsConstants(SizeOfInUint32(PrimitiveInstanceConstantBuffer), 2);

            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(static_cast<uint32_t>(std::size(rootParameters)), rootParameters);
            localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
            SerializeAndCreateDXRRootSignature(localRootSignatureDesc, m_DXRLocalRootSignature[LocalRootSignature::Type::AABB]);
            SetDebugObjectName(m_DXRLocalRootSignature[1].Get(), L"LocalRootSignatureAABB");
        }
    }
}

void Sample::CreateDXRPipelineStateObject()
{
    // Create 18 subobjects that combine into a RTPSO:
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject which has an explicit association specified
    // purely for demonstration purposes.
    // 1 - DXIL library
    // 8 - Hit group types - 4 geometries (1 triangle, 3 aabb) x 2 ray types (ray, shadowRay)
    // 1 - Shader config
    // 6 - 3 x Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config

    auto device = m_deviceResources->GetD3DDevice();
#if defined(_GAMING_XBOX_SCARLETT) && (_GRDK_VER >= 0x55F00C58 /* GDK Edition 220300 */)
    // Save the RtPso PDB on the scratch drive
    device->SetCompileTimeShaderPdbPathX(L"D:\\");
#endif

    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    // DXIL library
    {
        // This contains the shaders and their entrypoints for the state object.
        // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
        auto raytracingLibrary = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, std::size(g_pRaytracing));
        raytracingLibrary->SetDXILLibrary(&libdxil);
        // Use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
    }

    // 2 Triangle geometry Hit groups (one for radiance, one for shadow rays).
    {
        for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
        {
            auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            if (rayType == RayType::RadianceRay)
            {
                hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[GeometryType::Triangle]);
            }
            hitGroup->SetHitGroupExport(c_hitGroupNames_TriangleGeometry[rayType]);
            hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        }
    }

    // 6 AABB geometry Hit groups (radiance/shadow ray, times 3 procedural types).
    {
        // Create hit groups for each intersection shader.
        for (uint32_t t = 0; t < IntersectionShaderType::Count; t++)
        {
            for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
            {
                auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitGroup->SetIntersectionShaderImport(c_intersectionShaderNames[t]);
                if (rayType == RayType::RadianceRay)
                {
                    hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[GeometryType::AABB]);
                }
                hitGroup->SetHitGroupExport(c_hitGroupNames_AABBGeometry[t][rayType]);
                hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
            }
        }
    }

    // Shader Config
    // Defines the maximum sizes in bytes for the ray rayPayload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    uint32_t payloadSize = static_cast<uint32_t>(std::max(sizeof(RayPayload), sizeof(ShadowRayPayload)));
    uint32_t attributeSize = sizeof(struct ProceduralPrimitiveAttributes);
    shaderConfig->Config(payloadSize, attributeSize);

    // Local root signature and shader association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        // Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.
        // Triangle geometry
        {
            auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            localRootSignature->SetRootSignature(m_DXRLocalRootSignature[LocalRootSignature::Type::Triangle].Get());

            // Shader association
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            rootSignatureAssociation->AddExports(c_hitGroupNames_TriangleGeometry);
        }

        // AABB geometry
        {
            auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            localRootSignature->SetRootSignature(m_DXRLocalRootSignature[LocalRootSignature::Type::AABB].Get());

            // Shader association
            auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
            for (auto& hitGroupsForIntersectionShaderType : c_hitGroupNames_AABBGeometry)
            {
                rootSignatureAssociation->AddExports(hitGroupsForIntersectionShaderType);
            }
        }
    }

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_DXRGlobalRootSignature.Get());

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();

    // PERFORMANCE TIP: Set max recursion depth as low as needed
    // as drivers may apply optimization strategies for low recursion depths.
    uint32_t maxRecursionDepth = MAX_RAY_RECURSION_DEPTH;
    pipelineConfig->Config(maxRecursionDepth);

    PrintStateObjectDesc(raytracingPipeline);

    // Create the state object.
    DX::ThrowIfFailed(device->CreateStateObject(raytracingPipeline, IID_GRAPHICS_PPV_ARGS(m_dxrStateObject.ReleaseAndGetAddressOf())));
}
#pragma endregion

#pragma region Acceleration Structures
AccelerationStructureBuffers Sample::BuildBottomLevelAS(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    ComPtr<ID3D12Resource> scratch;
    ComPtr<ID3D12Resource> bottomLevelAS;

    // Get the size requirements for the scratch and AS buffers.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = static_cast<uint32_t>(geometryDescs.size());
    bottomLevelInputs.pGeometryDescs = geometryDescs.data();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    auto device = m_deviceResources->GetD3DDevice();
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    assert(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    // Create a scratch buffer.
    DX::ThrowIfFailed(
        CreateUAVBuffer(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, scratch.GetAddressOf(), c_initialUAVTargetState));
    SetDebugObjectName(scratch.Get(), L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent).
    // Default heap is OK since the application doesn't need CPU read/write access to them.
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both:
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    DX::ThrowIfFailed(
        CreateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, bottomLevelAS.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE));
    SetDebugObjectName(bottomLevelAS.Get(), L"BottomLevelAccelerationStructure");

    // bottom-level AS desc.
    bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
    bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAS->GetGPUVirtualAddress();

    // Build the acceleration structure.
    auto commandList = m_deviceResources->GetCommandList();
    commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

    AccelerationStructureBuffers bottomLevelASBuffers;
    bottomLevelASBuffers.accelerationStructure = bottomLevelAS;
    bottomLevelASBuffers.scratch = scratch;
    bottomLevelASBuffers.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    return bottomLevelASBuffers;
}

AccelerationStructureBuffers Sample::BuildTopLevelAS(AccelerationStructureBuffers* pBottomLevelAS,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    ComPtr<ID3D12Resource> scratch;
    ComPtr<ID3D12Resource> topLevelAS;

    // Get required sizes for an acceleration structure.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = BottomLevelASType::Count;

    auto device = m_deviceResources->GetD3DDevice();
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    assert(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    // Create a scratch buffer.
    DX::ThrowIfFailed(
        CreateUAVBuffer(device, topLevelPrebuildInfo.ScratchDataSizeInBytes, scratch.GetAddressOf(), c_initialUAVTargetState));
    SetDebugObjectName(scratch.Get(), L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent).
    // Default heap is OK since the application doesn't need CPU read/write access to them.
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both:
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    DX::ThrowIfFailed(
        CreateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, topLevelAS.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE));
    SetDebugObjectName(topLevelAS.Get(), L"TopLevelAccelerationStructure");

    // Create instance descs for the bottom-level acceleration structures.
    ComPtr<ID3D12Resource> instanceDescsResource;
    {
        D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[BottomLevelASType::Count] =
        {
            pBottomLevelAS[0].accelerationStructure->GetGPUVirtualAddress(),
            pBottomLevelAS[1].accelerationStructure->GetGPUVirtualAddress()
        };
        BuildBottomLevelASInstanceDescs(bottomLevelASaddresses, instanceDescsResource);
    }

    // Top-level AS desc
    topLevelBuildDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
    topLevelInputs.InstanceDescs = instanceDescsResource->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

    // Build acceleration structure.
    auto commandList = m_deviceResources->GetCommandList();
    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    AccelerationStructureBuffers topLevelASBuffers;
    topLevelASBuffers.accelerationStructure = topLevelAS;
    topLevelASBuffers.instanceDesc = instanceDescsResource;
    topLevelASBuffers.scratch = scratch;
    topLevelASBuffers.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    return topLevelASBuffers;
}

void Sample::BuildBottomLevelASInstanceDescs(D3D12_GPU_VIRTUAL_ADDRESS* pBottomLevelASaddresses,
    ComPtr<ID3D12Resource>& outInstanceDescsResource)
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    instanceDescs.resize(BottomLevelASType::Count);

    // BLAS for the BottomLevelASType::Triangle plane.
    {
        // Width of BLAS geometry (plane).
        // Make the plane a little larger than the actual number of primitives in each dimension.
        Vector3 const NUM_AABB = Vector3(700.0f, 1.0f, 700.0f);
        Vector3 const vWidth = (NUM_AABB * c_aabbWidth) + (NUM_AABB - Vector3(1.0f)) * c_aabbDistance;

        // Calculate transformation matrix.
        XMVECTOR const vBasePosition = vWidth * Vector3(-0.35f, 0.0f, -0.35f);

        // Scale in XZ dimensions.
        XMMATRIX mTranslation = XMMatrixTranslationFromVector(vBasePosition);
        XMMATRIX mScale = XMMatrixScaling(vWidth.x, vWidth.y, vWidth.z);
        XMMATRIX mTransform = mScale * mTranslation;

        auto& instanceDesc = instanceDescs[BottomLevelASType::Triangle];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;
        instanceDesc.InstanceContributionToHitGroupIndex = 0;
        instanceDesc.AccelerationStructure = pBottomLevelASaddresses[BottomLevelASType::Triangle];
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), mTransform);
    }

    // Create instanced BLAS for procedural geometry AABBs.
    // Instances share all the data, except for a transform.
    {
        auto& instanceDesc = instanceDescs[BottomLevelASType::AABB];
        instanceDesc = {};
        instanceDesc.InstanceMask = 1;

        // Set hit group offset to beyond the shader records for the triangle AABB.
        instanceDesc.InstanceContributionToHitGroupIndex = BottomLevelASType::AABB * RayType::Count;
        instanceDesc.AccelerationStructure = pBottomLevelASaddresses[BottomLevelASType::AABB];

        // Move all AABBS above the ground plane.
        XMMATRIX mTranslation = XMMatrixTranslationFromVector(Vector3(0.0f, c_aabbWidth / 2.0f, 0.0f));
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), mTranslation);
    }

    auto device = m_deviceResources->GetD3DDevice();

    DX::ThrowIfFailed(
        CreateUploadBuffer(device, instanceDescs,
            outInstanceDescsResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(outInstanceDescsResource.Get(), L"InstanceDescs");
}

void Sample::BuildAccelerationStructures()
{
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto commandAllocator = m_deviceResources->GetCommandAllocator();
    auto commandList = m_deviceResources->GetCommandList();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    // Build bottom-level AS.
    AccelerationStructureBuffers bottomLevelAS[BottomLevelASType::Count];
    std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, BottomLevelASType::Count> geometryDescs;
    {
        // Build geometryDescs for BLAS
        {
            // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
            // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
            D3D12_RAYTRACING_GEOMETRY_FLAGS geometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            // Triangle geometry desc
            {
                // Triangle bottom-level AS contains a single plane geometry.
                geometryDescs[BottomLevelASType::Triangle].resize(1);

                // Plane geometry
                auto& geometryDesc = geometryDescs[BottomLevelASType::Triangle][0];
                geometryDesc = {};
                geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                geometryDesc.Triangles.IndexBuffer = m_indexBufferResource->GetGPUVirtualAddress();
                geometryDesc.Triangles.IndexCount = static_cast<uint32_t>(m_indexBufferResource->GetDesc().Width) / sizeof(uint16_t);
                geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
                geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                geometryDesc.Triangles.VertexCount = static_cast<uint32_t>(m_vertexBufferResource->GetDesc().Width) / sizeof(Vertex);
                geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBufferResource->GetGPUVirtualAddress();
                geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
                geometryDesc.Flags = geometryFlags;
            }

            // AABB geometry desc
            {
                D3D12_RAYTRACING_GEOMETRY_DESC aabbDescTemplate = {};
                aabbDescTemplate.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
                aabbDescTemplate.AABBs.AABBCount = 1;
                aabbDescTemplate.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
                aabbDescTemplate.Flags = geometryFlags;

                // One AABB primitive per geometry.
                geometryDescs[BottomLevelASType::AABB].resize(IntersectionShaderType::TotalPrimitiveCount, aabbDescTemplate);

                // Create AABB geometries.
                // Having separate geometries allows of separate shader record binding per geometry.
                // In this sample, this lets us specify custom hit groups per AABB geometry.
                for (size_t i = 0; i < IntersectionShaderType::TotalPrimitiveCount; ++i)
                {
                    auto& geometryDesc = geometryDescs[BottomLevelASType::AABB][i];
                    geometryDesc.AABBs.AABBs.StartAddress = m_AABBResource->GetGPUVirtualAddress() + i * sizeof(D3D12_RAYTRACING_AABB);
                }
            }
        }

        // Build all bottom-level AS.
        for (size_t i = 0; i < BottomLevelASType::Count; i++)
        {
            bottomLevelAS[i] = BuildBottomLevelAS(geometryDescs[i]);
        }
    }

    // Batch all resource barriers for bottom-level AS builds.
    D3D12_RESOURCE_BARRIER resourceBarriers[BottomLevelASType::Count];
    for (size_t i = 0; i < BottomLevelASType::Count; i++)
    {
        resourceBarriers[i] = CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAS[i].accelerationStructure.Get());
    }
    commandList->ResourceBarrier(BottomLevelASType::Count, resourceBarriers);

    // Build top-level AS.
    AccelerationStructureBuffers topLevelAS = BuildTopLevelAS(bottomLevelAS);

    // Kick off acceleration structure construction.
    DX::ThrowIfFailed(commandList->Close());
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(static_cast<uint32_t>(std::size(commandLists)), commandLists);

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    m_deviceResources->WaitForGpu();

    // Store the AS buffers. The rest (scratch temporary buffers) will be released once we exit the function.
    for (uint32_t i = 0; i < BottomLevelASType::Count; i++)
    {
        m_bottomLevelAS[i] = bottomLevelAS[i].accelerationStructure;
    }
    m_topLevelAS = topLevelAS.accelerationStructure;
}
#pragma endregion

#pragma region Scene constant buffers, SRVs & UAVs
void Sample::InitializeConstantBuffers()
{
    // Setup materials.
    {
        auto SetAttributes = [&](
            uint32_t primitiveIndex,
            const Vector4& albedo,
            float reflectanceCoef = 0.0f,
            float diffuseCoef = 0.9f,
            float specularCoef = 0.7f,
            float specularPower = 50.0f,
            float stepScale = 1.0f)
            {
                auto& attributes = m_aabbMaterialCB[primitiveIndex];
                attributes.albedo = albedo;
                attributes.reflectanceCoef = reflectanceCoef;
                attributes.diffuseCoef = diffuseCoef;
                attributes.specularCoef = specularCoef;
                attributes.specularPower = specularPower;
                attributes.stepScale = stepScale;
            };

        // Scene floor plane material (CB)
        m_planeMaterialCB.albedo = Vector4(0.9f, 0.9f, 0.9f, 1.0f);
        m_planeMaterialCB.reflectanceCoef = 0.25f;
        m_planeMaterialCB.diffuseCoef = 1.0f;
        m_planeMaterialCB.specularCoef = 0.4f;
        m_planeMaterialCB.specularPower = 50.0f;
        m_planeMaterialCB.stepScale = 1.0f;

        // Albedos
        Vector4 green = Vector4(0.1f, 1.0f, 0.5f, 1.0f);
        Vector4 red = Vector4(1.0f, 0.5f, 0.5f, 1.0f);
        Vector4 yellow = Vector4(1.0f, 1.0f, 0.5f, 1.0f);

        uint32_t offset = 0;

        // Analytic primitives.
        SetAttributes(offset + AnalyticPrimitive::AABB, red);
        SetAttributes(offset + AnalyticPrimitive::Spheres, ChromiumReflectance, 1.0f);
        offset += AnalyticPrimitive::Count;

        // Volumetric primitives.
        SetAttributes(offset + VolumetricPrimitive::Metaballs, ChromiumReflectance, 1.0f);
        offset += VolumetricPrimitive::Count;

        // Signed distance primitives.
        SetAttributes(offset + SignedDistancePrimitive::MiniSpheres, green);
        SetAttributes(offset + SignedDistancePrimitive::IntersectedRoundCube, green);
        SetAttributes(offset + SignedDistancePrimitive::SquareTorus, ChromiumReflectance, 1.0f);
        SetAttributes(offset + SignedDistancePrimitive::TwistedTorus, yellow, 0.0f, 1.0f, 0.7f, 50.0f, 0.5f);
        SetAttributes(offset + SignedDistancePrimitive::Cog, yellow, 0.0f, 1.0f, 0.1f, 2.0f);
        SetAttributes(offset + SignedDistancePrimitive::Cylinder, red);
        SetAttributes(offset + SignedDistancePrimitive::FractalPyramid, green, 0.0f, 1.0f, 0.1f, 4.0f, 0.8f);
    }

    // Setup lights.
    {
        float d = 0.6f;
        m_sceneCB.lightPosition = Vector4(0.0f, 18.0f, -20.0f, 0.0f);
        m_sceneCB.lightAmbientColor = Vector4(0.25f, 0.25f, 0.25f, 1.0f);
        m_sceneCB.lightDiffuseColor = Vector4(d, d, d, 1.0f);
    }
}

void Sample::CreateAABBPrimitiveAttributesBuffers()
{
    auto device = m_deviceResources->GetD3DDevice();
    uint64_t frameCount = m_deviceResources->GetBackBufferCount();

    DX::ThrowIfFailed(
        CreateUploadBuffer(device, nullptr, IntersectionShaderType::TotalPrimitiveCount, frameCount * sizeof(PrimitiveInstancePerFrameBuffer), m_aabbPrimitiveAttributeBufferResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_aabbPrimitiveAttributeBufferResource.Get(), L"AABBPrimitiveAttributeBufferResource");

    // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
    DX::ThrowIfFailed(m_aabbPrimitiveAttributeBufferResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pAabbPrimitiveAttributeMappedData)));
}

void Sample::CreateConstantBuffers()
{
    auto device = m_deviceResources->GetD3DDevice();
    uint64_t frameCount = m_deviceResources->GetBackBufferCount();

    DX::ThrowIfFailed(
        CreateUploadBuffer(device, nullptr, frameCount, sizeof(SceneConstantBuffer), m_sceneCBResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_sceneCBResource.Get(), L"SceneCBResource");

    // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
    DX::ThrowIfFailed(m_sceneCBResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pSceneCBMappedData)));
}

void Sample::CreateRaytracingOutputResource()
{
    auto backbufferFormat = m_deviceResources->GetBackBufferFormat();

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto const uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto device = m_deviceResources->GetD3DDevice();
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_DXROutputResource.ReleaseAndGetAddressOf())));
    SetDebugObjectName(m_DXROutputResource.Get(), L"RaytracingOutput");

    auto uavDescriptorHandle = m_descriptorHeap->GetCpuHandle(DESCRIPTOR_INDEX_DXR_OUTPUT);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_DXROutputResource.Get(), nullptr, &UAVDesc, uavDescriptorHandle);

    m_DXROutputResourceUAVGpuDescriptorHandle = m_descriptorHeap->GetGpuHandle(DESCRIPTOR_INDEX_DXR_OUTPUT);
}
#pragma endregion

#pragma region Shader Tables
void Sample::BuildShaderTables()
{
    /*-------- - Shader table layout-----------------------------
    | Shader table - HitGroupShaderTable:
    | ---------------------------------------------
    | [0] : MyHitGroup_Triangle
    | [1] : MyHitGroup_Triangle_ShadowRay
    | ---------------------------------------------
    | [2] : MyHitGroup_AABB_AnalyticPrimitive
    | [3] : MyHitGroup_AABB_AnalyticPrimitive_ShadowRay
    | ...
    | ---------------------------------------------
    | [6] : MyHitGroup_AABB_VolumetricPrimitive
    | [7] : MyHitGroup_AABB_VolumetricPrimitive_ShadowRay
    | ---------------------------------------------
    | [8] : MyHitGroup_AABB_SignedDistancePrimitive
    | [9] : MyHitGroup_AABB_SignedDistancePrimitive_ShadowRay,
    | ...
    | [20] : MyHitGroup_AABB_SignedDistancePrimitive
    | [21] : MyHitGroup_AABB_SignedDistancePrimitive_ShadowRay
    | ---------------------------------------------------------*/

    ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    DX::ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));

    // Create raygen shader table records.
    CustomShaderRecord rayGenRecord;
    rayGenRecord.Initialize(stateObjectProperties.Get(), c_raygenShaderName);

    // Create miss shader table records.
    CustomShaderRecord missShaderRecords[RayType::Count];
    for (size_t i = 0; i < RayType::Count; ++i)
    {
        if (std::wcscmp(c_missShaderNames[i], L"") != 0)
        {
            missShaderRecords[i].Initialize(stateObjectProperties.Get(), c_missShaderNames[i]);
        }
        else
        {
            missShaderRecords[i].InitializeNull();
        }
    }

    // Create hitgroup table records.
    CustomShaderRecord hitGroupRecord[hitgroupRecordCount];
    uint32_t hitShaderRecordIndex = 0U;

    // Setup hitgroup records for the triangle geometry
    for (size_t i = 0; i < RayType::Count; i++)
    {
        // Initialize the shader record with the shader identifier
        auto& shaderRecordEntry = hitGroupRecord[hitShaderRecordIndex++];
        shaderRecordEntry.Initialize(stateObjectProperties.Get(), c_hitGroupNames_TriangleGeometry[i]);

        // Copy the local binding data into the entry
        shaderRecordEntry.materialCb = m_planeMaterialCB;
    }

    // Setup hitgroup records for the procedural geometry
    for (size_t iShaderType = 0, instanceIndex = 0; iShaderType < IntersectionShaderType::Count; iShaderType++)
    {
        uint32_t shaderTypePrimCount = IntersectionShaderType::PerPrimitiveTypeCount(static_cast<IntersectionShaderType::Enum>(iShaderType));

        // Number of primitives for each intersection shader type.
        for (size_t primitiveIndex = 0; primitiveIndex < shaderTypePrimCount; primitiveIndex++, instanceIndex++)
        {
            for (size_t rayType = 0; rayType < RayType::Count; rayType++)
            {
                // Initialize the shader record with the shader identifier
                auto& shaderRecordEntry = hitGroupRecord[hitShaderRecordIndex++];

                // Initialize the shader record with the shader identifier
                shaderRecordEntry.Initialize(stateObjectProperties.Get(), c_hitGroupNames_AABBGeometry[iShaderType][rayType]);

                // Copy the local binding data into the entry
                shaderRecordEntry.materialCb = m_aabbMaterialCB[instanceIndex];
                shaderRecordEntry.aabbCB.instanceIndex = static_cast<uint32_t>(instanceIndex);
                shaderRecordEntry.aabbCB.primitiveType = static_cast<uint32_t>(primitiveIndex);
            }
        }
    }

    // Add the records to the shader table
    m_shaderBindingTable.SetRayGenRecord(0, rayGenRecord);

    for (int i = 0; i < RayType::Count; ++i)
    {
        m_shaderBindingTable.SetMissShaderRecord(i, missShaderRecords[i]);
    }

    for (int i = 0; i < static_cast<int>(hitgroupRecordCount); ++i)
    {
        m_shaderBindingTable.SetHitGroupRecord(i, hitGroupRecord[i]);
    }

    // Allocate the resource that holds all the table entries.
    // For this sample, this only needs to be done on initialization, since
    // bindings are not being updated afterwards.
    m_shaderBindingTable.Commit();
}
#pragma endregion

#pragma region Geometry Building
void Sample::BuildProceduralGeometryAABBs()
{
    // Set up AABBs on a grid.
    XMFLOAT3 aabbGrid(4.0f, 1.0f, 4.0f);

    // Base position is going to be the minX, minY, minZ of a box that encompasses all
    // AABBs and the distance between them.
    // This box is centered in the origin, therefore it needs to be divided by 2.
    const XMFLOAT3 minCorner =
    {
        -((aabbGrid.x * c_aabbWidth) + (aabbGrid.x - 1.0f) * c_aabbDistance) / 2.0f,
        -((aabbGrid.y * c_aabbWidth) + (aabbGrid.y - 1.0f) * c_aabbDistance) / 2.0f,
        -((aabbGrid.z * c_aabbWidth) + (aabbGrid.z - 1.0f) * c_aabbDistance) / 2.0f,
    };

    XMFLOAT3 stride = XMFLOAT3(c_aabbWidth + c_aabbDistance, c_aabbWidth + c_aabbDistance, c_aabbWidth + c_aabbDistance);

    auto InitializeAABB = [&](auto const& offsetIndex, auto const& size)
    {
        return D3D12_RAYTRACING_AABB {
            minCorner.x + offsetIndex.x * stride.x,
            minCorner.y + offsetIndex.y * stride.y,
            minCorner.z + offsetIndex.z * stride.z,
            minCorner.x + offsetIndex.x * stride.x + size.x,
            minCorner.y + offsetIndex.y * stride.y + size.y,
            minCorner.z + offsetIndex.z * stride.z + size.z,
        };
    };

    m_AABBList.resize(IntersectionShaderType::TotalPrimitiveCount);
    uint32_t offset = 0;

    // Analytic primitives.
    m_AABBList[offset + AnalyticPrimitive::AABB] =    InitializeAABB(XMINT3(3, 0, 0), XMFLOAT3(2.0f, 3.0f, 2.0f));
    m_AABBList[offset + AnalyticPrimitive::Spheres] = InitializeAABB(XMFLOAT3(2.25f, 0.0f, 0.75f), XMFLOAT3(3.0f, 3.0f, 3.0f));
    offset += AnalyticPrimitive::Count;

    // Volumetric primitives.
    m_AABBList[offset + VolumetricPrimitive::Metaballs] = InitializeAABB(XMINT3(0, 0, 0), XMFLOAT3(3.0f, 3.0f, 3.0f));
    offset += VolumetricPrimitive::Count;

    // Signed distance primitives.
    m_AABBList[offset + SignedDistancePrimitive::MiniSpheres] =          InitializeAABB(XMINT3(2, 0, 0), XMFLOAT3(2.0f, 2.0f, 2.0f));
    m_AABBList[offset + SignedDistancePrimitive::IntersectedRoundCube] = InitializeAABB(XMINT3(0, 0, 2), XMFLOAT3(2.0f, 2.0f, 2.0f));
    m_AABBList[offset + SignedDistancePrimitive::SquareTorus] =          InitializeAABB(XMFLOAT3(0.75f, -0.1f, 2.25f), XMFLOAT3(3.0f, 3.0f, 3.0f));
    m_AABBList[offset + SignedDistancePrimitive::TwistedTorus] =         InitializeAABB(XMINT3(0, 0, 1), XMFLOAT3(2.0f, 2.0f, 2.0f));
    m_AABBList[offset + SignedDistancePrimitive::Cog] =                  InitializeAABB(XMINT3(1, 0, 0), XMFLOAT3(2.0f, 2.0f, 2.0f));
    m_AABBList[offset + SignedDistancePrimitive::Cylinder] =             InitializeAABB(XMINT3(0, 0, 3), XMFLOAT3(2.0f, 3.0f, 2.0f));
    m_AABBList[offset + SignedDistancePrimitive::FractalPyramid] =       InitializeAABB(XMINT3(2, 0, 2), XMFLOAT3(6.0f, 6.0f, 6.0f));

    // Allocate resource with the list of AABBs
    auto device = m_deviceResources->GetD3DDevice();
    DX::ThrowIfFailed(
        CreateUploadBuffer(device, m_AABBList,
            m_AABBResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_AABBResource.Get(), L"AABBResource");
}

void Sample::BuildPlaneGeometry(ResourceUploadBatch& upload)
{
    // Plane indices.
    uint16_t indices[] =
    {
        3,1,0,
        2,1,3,
    };

    // Cube vertices positions and corresponding triangle normals.
    Vertex vertices[] =
    {
        { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) }, // vertex 0, Normal 0
        { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) }, // vertex 1, Normal 1
        { Vector3(1.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) }, // vertex 2, Normal 2
        { Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) }, // vertex 3, Normal 3
    };

    auto device = m_deviceResources->GetD3DDevice();

    // Index buffer
    DX::ThrowIfFailed(
        CreateStaticBuffer(device, upload, indices, std::size(indices), D3D12_RESOURCE_STATE_GENERIC_READ,
            m_indexBufferResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_indexBufferResource.Get(), L"IndexBufferResource");

    // Vertex buffer
    DX::ThrowIfFailed(
        CreateStaticBuffer(device, upload, vertices, std::size(vertices), D3D12_RESOURCE_STATE_GENERIC_READ,
            m_vertexBufferResource.ReleaseAndGetAddressOf()));
    SetDebugObjectName(m_vertexBufferResource.Get(), L"VertexBufferResource");

    // Create Index Buffer SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.NumElements = static_cast<uint32_t>(std::size(indices) / 2);
        srvDesc.Buffer.StructureByteStride = 0;

        auto srvCpuHandle = m_descriptorHeap->GetCpuHandle(DESCRIPTOR_INDEX_INDEX_BUFFER);
        device->CreateShaderResourceView(m_indexBufferResource.Get(), &srvDesc, srvCpuHandle);

        m_indexBufferGPUHandle = m_descriptorHeap->GetGpuHandle(DESCRIPTOR_INDEX_INDEX_BUFFER);
    }

    // Create Vertex Buffer SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = static_cast<uint32_t>(std::size(vertices));
        srvDesc.Buffer.StructureByteStride = sizeof(Vertex);

        auto srvCpuHandle = m_descriptorHeap->GetCpuHandle(DESCRIPTOR_INDEX_VERTEX_BUFFER);
        device->CreateShaderResourceView(m_vertexBufferResource.Get(), &srvDesc, srvCpuHandle);
        m_vertexBufferGPUHandle = m_descriptorHeap->GetGpuHandle(DESCRIPTOR_INDEX_VERTEX_BUFFER);
    }

    // Vertex Buffer descriptor index must follow that of Index Buffer descriptor index
    assert(DESCRIPTOR_INDEX_VERTEX_BUFFER == DESCRIPTOR_INDEX_INDEX_BUFFER + 1 && "Vertex Buffer descriptor index must follow that of Index Buffer descriptor index");
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Check for Shader Model 6.3 and DirectX Raytracing (DXR) feature support
#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_3 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_3))
    {
        OutputDebugStringA("ERROR: Shader Model 6.3 is not supported\n");
        throw std::exception("Shader Model 6.3 is not supported");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        || (featureSupportData.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED))
    {
#ifndef _GAMING_XBOX
        MessageBox(nullptr, L"DirectX Raytracing (DXR) is not supported on this system. The sample will now exit.", L"Unsupported Feature", MB_OK | MB_ICONERROR);
#endif
        OutputDebugStringA("ERROR: DirectX Raytracing (DXR) is not supported!\n");
        throw std::exception("DirectX Raytracing (DXR) is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Initialize raytracing pipeline.

    // Create root signatures for the shaders.
    CreateDXRRootSignatures();

    // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
    CreateDXRPipelineStateObject();

    // Create a heap for descriptors.
    m_descriptorHeap = std::make_unique<DescriptorHeap>(device, 3);

    // Build geometry to be used in the sample.
    BuildProceduralGeometryAABBs();

    ResourceUploadBatch upload(device);
    upload.Begin();
    BuildPlaneGeometry(upload);

    // UI (HUD)
    {
        m_HUDDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device, FONT_TYPE::FONT_COUNT);

        auto backBufferRTS = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRTS, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, upload, spritePSD);

        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_smallFont = std::make_unique<SpriteFont>(device, upload,
            strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(TEXT_FONT_OFFSET),
            m_HUDDescriptorHeap->GetGpuHandle(TEXT_FONT_OFFSET));

        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, upload, strFilePath,
            m_HUDDescriptorHeap->GetCpuHandle(CONTROLLER_FONT_OFFSET),
            m_HUDDescriptorHeap->GetGpuHandle(CONTROLLER_FONT_OFFSET));
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    // Build raytracing acceleration structures from the generated geometry.
    BuildAccelerationStructures();

    // Create constant buffers for the geometry and the scene.
    CreateConstantBuffers();

    // Create AABB primitive attribute buffers.
    CreateAABBPrimitiveAttributesBuffers();

    // Build shader tables, which define shaders and their local root arguments.
    BuildShaderTables();

#ifdef _GAMING_XBOX_SCARLETT
    // Initialize inline raytracing pipeline
    CreateInlineDXRPipelineElements();
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Get rendering width and height
    auto outputSize = m_deviceResources->GetOutputSize();
    m_width = static_cast<uint32_t>(outputSize.right - outputSize.left);
    m_height = static_cast<uint32_t>(outputSize.bottom - outputSize.top);

    // Create an output 2D texture to store the raytracing result to.
    CreateRaytracingOutputResource();

    /// Fly camera
    m_camera.SetWindow(static_cast<int32_t>(m_width), static_cast<int32_t>(m_height));
    m_camera.SetProjectionParameters(XM_PIDIV4, CameraNear, CameraFar, true);
    m_camera.SetLookAt(CameraSrc, CameraDst);
    m_camera.SetSensitivity(50.0f, 50.0f, 50.0f, 0.0f);

    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_hudBatch->SetViewport(viewportUI);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_deviceResources.reset();
    m_gpuTimer.reset();
    m_gamePad.reset();
    m_keyboard.reset();
    m_mouse.reset();
    m_HUDDescriptorHeap.reset();
    m_descriptorHeap.reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();

    if (m_sceneCBResource.Get() && m_pSceneCBMappedData)
    {
        m_sceneCBResource->Unmap(0, nullptr);
        m_pSceneCBMappedData = nullptr;
    }

    if (m_aabbPrimitiveAttributeBufferResource.Get() && m_pAabbPrimitiveAttributeMappedData)
    {
        m_aabbPrimitiveAttributeBufferResource->Unmap(0, nullptr);
        m_pAabbPrimitiveAttributeMappedData = nullptr;
    }

    m_dxrStateObject.Reset();
    m_DXRGlobalRootSignature.Reset();
    m_sceneCBResource.Reset();
    m_aabbPrimitiveAttributeBufferResource.Reset();
    m_DXROutputResource.Reset();
    m_AABBResource.Reset();
    m_topLevelAS.Reset();
    m_indexBufferResource.Reset();
    m_vertexBufferResource.Reset();
    for (uint32_t i = 0; i < BottomLevelASType::Count; ++i)
    {
        m_bottomLevelAS[i].Reset();
    }
    for (uint32_t i = 0; i < GeometryType::Count; ++i)
    {
        m_DXRLocalRootSignature[i].Reset();
    }
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region Inline Raytracing Methods
void Sample::CreateInlineDXRPipelineElements()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Load shader blob
    auto csBlob = DX::ReadData(L"InlineRaytracing.cso");

    // Create root signature
    DX::ThrowIfFailed(
        device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_InlineRaytracingRS.ReleaseAndGetAddressOf())));
    m_InlineRaytracingRS->SetName(L"InlineRaytracingRS");

    // Create Pipeline State Object
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_InlineRaytracingRS.Get();
    psoDesc.CS = { csBlob.data(), csBlob.size() };
    DX::ThrowIfFailed(
        device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_InlineRaytracingPSO.ReleaseAndGetAddressOf())));
    m_InlineRaytracingPSO->SetName(L"InlineRaytracingPSO");

}
#pragma endregion

