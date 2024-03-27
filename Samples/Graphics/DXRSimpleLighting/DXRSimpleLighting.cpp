//--------------------------------------------------------------------------------------
// DXRSimpleLighting.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DXRSimpleLighting.h"
#include "CompiledShaders\Raytracing.hlsl.h"

#include "ATGColors.h"
#include "ReadData.h"
#include "FindMedia.h"

#define SizeOfInUint32(obj) (static_cast<uint32_t>(sizeof(obj)) / sizeof(uint32_t))

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* c_hitGroupName = L"MyHitGroup";
    const wchar_t* c_raygenShaderName = L"MyRaygenShader";
    const wchar_t* c_closestHitShaderName = L"MyClosestHitShader";
    const wchar_t* c_missShaderName = L"MyMissShader";
} // Anonymous namespace

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_gamePadButtons{}
    , m_sceneConstantData{}
    , m_cubeConstantData{}
    , m_screenConstantData{}
    , m_indexBuffer{}
    , m_vertexBuffer{}
    , m_raytracingOutputResourceUAVGpuDescriptor{}
    , m_curRotationAngleRad(0.0f)
    , m_inlineRaytracingEnabled(true)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, c_frameCount,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableDXR);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

#pragma region Initialization Functions
// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    InitializeScene();

    m_deviceResources->CreateDeviceResources();

    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();

    CreateWindowSizeDependentResources();

    // Create GPU Timer to capture frame time statistics
    m_gpuTimer = std::make_unique<DX::GPUTimer>(m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());
}

void Sample::InitializeScene()
{
    // Setup materials.
    {
        m_cubeConstantData.albedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Setup camera.
    {
        m_camera.SetProjectionParameters(XM_PI / 4.0f, 0.01f, 100.0f, true);
        m_camera.SetLookAt(Vector4(0.0f, 4.0f, -10.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        m_camera.SetBoundingBox(BoundingBox(Vector3::Zero, Vector3(10.0f, 15.0f, 20.0f)));
        m_camera.SetSensitivity(10.0f, 0.01f, 10.0f, 0.01f);
        m_sceneConstantData.cameraPosition = m_camera.GetPosition();
        m_sceneConstantData.projectionToWorld = XMMatrixInverse(nullptr, m_camera.GetView() * m_camera.GetProjection());
    }

    // Initialize the scene lighting constant buffer values.
    m_sceneConstantData.lightPosition = Vector4(0.0f, 0.0f, -1.0f, 1.0f);
    m_sceneConstantData.directionalLightColor = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
    m_sceneConstantData.pointLightColor = Vector4(0.5f, 0.0f, 0.0f, 1.0f);
    m_sceneConstantData.staticLightDirection = Vector4(-0.577f, 0.577f, -0.577f, 1.0f);
    m_sceneConstantData.backgroundColor = Vector4(0.254901975f, 0.254901975f, 0.254901975f, 1.0f);
    m_sceneConstantData.minRayExtent = 0.001f;
    m_sceneConstantData.maxRayExtent = 10000.0f;
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

    UpdateScene();
    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Update the rotation constant
    m_curRotationAngleRad += elapsedTime / 3.0f;
    if (m_curRotationAngleRad >= XM_2PI)
    {
        m_curRotationAngleRad -= XM_2PI;
    }

    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (m_gamePadButtons.a == DirectX::GamePad::ButtonStateTracker::PRESSED)
        {
            m_inlineRaytracingEnabled = !m_inlineRaytracingEnabled;
        }

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}

#pragma endregion

#pragma region Frame Render

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD");

    auto safe = Viewport::ComputeTitleSafeArea(static_cast<uint32_t>(m_outputSize.right - m_outputSize.left), static_cast<uint32_t>(m_outputSize.bottom - m_outputSize.top));
    auto scale = 1.0f; // On Lockhart
    auto textPos = Vector2(float(safe.left), float(safe.top));
    auto textColor = ATG::Colors::White;
    auto origin = Vector2(0.0f, 0.0f);
    auto statsLineOffset = 4;

    if (XSystemGetDeviceType() > XSystemDeviceType::XboxScarlettLockhart)
    {
        scale = 1.5f;
    }

    m_hudBatch->Begin(commandList);

    m_smallFont->DrawString(m_hudBatch.get(), L"DXRSimpleLighting Sample",
        textPos, textColor, 0, XMFLOAT2(0,0), scale, SpriteEffects_None, 0);

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A] Toggle Inline Raytracing", Vector2(textPos.x,
        textPos.y + (m_smallFont->GetLineSpacing() * 2 * scale)), textColor, scale);

    wchar_t frameStats[1024] = L"";
    swprintf_s(frameStats, _countof(frameStats), L"Inline Raytracing: %s", m_inlineRaytracingEnabled ? L"Enabled" : L"Disabled");
    m_smallFont->DrawString(m_hudBatch.get(), frameStats,
        Vector2(textPos.x,
            textPos.y + (m_smallFont->GetLineSpacing() * statsLineOffset * scale)), textColor, 0, origin, scale, SpriteEffects_None, 0);


    swprintf_s(frameStats, _countof(frameStats), L"TLAS Build (ms): %.2f", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPUTimerID::BuildTLAS)));

    m_smallFont->DrawString(m_hudBatch.get(), frameStats,
        Vector2(textPos.x,
            textPos.y + (m_smallFont->GetLineSpacing() * ++statsLineOffset * scale)), textColor, 0, origin, scale, SpriteEffects_None, 0);

    swprintf_s(frameStats, _countof(frameStats), L"Raytracing (ms): %.2f", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPUTimerID::Raytracing)));

    m_smallFont->DrawString(m_hudBatch.get(), frameStats,
        Vector2(textPos.x,
            textPos.y + (m_smallFont->GetLineSpacing() * ++statsLineOffset * scale)), textColor, 0, origin, scale, SpriteEffects_None, 0);


    swprintf_s(frameStats, _countof(frameStats), L"Copy to Back Buffer (ms): %.2f", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPUTimerID::CopyToBackBuffer)));

    m_smallFont->DrawString(m_hudBatch.get(), frameStats,
        Vector2(textPos.x,
            textPos.y + (m_smallFont->GetLineSpacing() * ++statsLineOffset * scale)), textColor, 0, origin, scale, SpriteEffects_None, 0);

    swprintf_s(frameStats, _countof(frameStats), L"Total Frame Time (ms): %.2f", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(GPUTimerID::Total)));

    m_smallFont->DrawString(m_hudBatch.get(), frameStats,
        Vector2(textPos.x,
            textPos.y + (m_smallFont->GetLineSpacing() * ++statsLineOffset * scale)), textColor, 0, origin, scale, SpriteEffects_None, 0);

    m_hudBatch->End();
   
    
    PIXEndEvent(commandList);
}

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

    auto pCommandList = m_deviceResources->GetCommandList();

    m_gpuTimer->BeginFrame(pCommandList);

    m_gpuTimer->Start(pCommandList, static_cast<uint32_t>(GPUTimerID::Total));

    m_gpuTimer->Start(pCommandList, static_cast<uint32_t>(GPUTimerID::BuildTLAS));
    BuildTLAS();
    m_gpuTimer->Stop(pCommandList, static_cast<uint32_t>(GPUTimerID::BuildTLAS));

    m_gpuTimer->Start(pCommandList, static_cast<uint32_t>(GPUTimerID::Raytracing));
    if (m_inlineRaytracingEnabled)
    {
        DoInlineRaytracing();
    }
    else
    {
        DoRaytracing();
    }
    m_gpuTimer->Stop(pCommandList, static_cast<uint32_t>(GPUTimerID::Raytracing));

    m_gpuTimer->Start(pCommandList, static_cast<uint32_t>(GPUTimerID::CopyToBackBuffer));
    CopyRaytracingOutputToBackbuffer();
    m_gpuTimer->Stop(pCommandList, static_cast<uint32_t>(GPUTimerID::CopyToBackBuffer));

    m_gpuTimer->Stop(pCommandList, static_cast<uint32_t>(GPUTimerID::Total));

    m_gpuTimer->EndFrame(pCommandList);

    DrawHUD(pCommandList);

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
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

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
    m_camera.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    ResourceUploadBatch resourceUpload(device);

    // Create heaps
    m_fontSrvPile = std::make_unique<DescriptorPile>(device,
        128, // Maximum descriptors for both static and dynamic
        static_cast<size_t>(StaticDescriptors::Reserve));

    m_descriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, Descriptors::Count);

    CreateDXRDeviceResources(resourceUpload);
    CreateInlineRaytracingResources();

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

void Sample::CreateDXRDeviceResources(ResourceUploadBatch& upload)
{
    // Begin uploading texture resources
    upload.Begin();

    // Create HUD and font resources for UI
    CreateUIResources(upload);

    // Create root signatures for the DXR shader.
    CreateRootSignatures();

    // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
    CreateRaytracingPipelineStateObject();

    // Build geometry to be used in the sample.
    BuildGeometry(upload);

    // Build raytracing acceleration structures from the generated geometry.
    CreateAccelerationStructureResources(upload);

    // Build shader tables, which define shaders and their local root arguments.
    BuildShaderTables();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_outputSize = m_deviceResources->GetOutputSize();

    // The minimum number of threads in a thread group for an inline raytraced workload is 32.
    DX::ThrowIfFalse(!((m_outputSize.right - m_outputSize.left) % THREAD_GROUP_X));
    DX::ThrowIfFalse(!((m_outputSize.bottom - m_outputSize.top) % THREAD_GROUP_Y));

    auto device = m_deviceResources->GetD3DDevice();
    wchar_t absoluteFontPath[MAX_PATH] = {};

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
    m_camera.SetWindow(m_outputSize.right - m_outputSize.left, m_outputSize.bottom - m_outputSize.top);
    m_sceneConstantData.cameraPosition = m_camera.GetPosition();
    m_sceneConstantData.projectionToWorld = XMMatrixInverse(nullptr, m_camera.GetView() * m_camera.GetProjection());

    // Begin uploading texture resources
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"SegoeUI_36.spritefont");

    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        absoluteFontPath,
        m_fontSrvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::Font)),
        m_fontSrvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::Font)));

    DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"XboxOneControllerLegend.spritefont");

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        absoluteFontPath,
        m_fontSrvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::Control)),
        m_fontSrvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::Control)));

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Create an output 2D texture to store the raytracing result to.
    CreateRaytracingOutputResource();
}

void Sample::CreateUIResources(ResourceUploadBatch& upload)
{
    RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    DirectX::SpriteBatchPipelineStateDescription hudpd(
        backBufferRts,
        &CommonStates::AlphaBlend);

    m_hudBatch = std::make_unique<SpriteBatch>(m_deviceResources->GetD3DDevice(), upload, hudpd);

    auto finished = upload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

void Sample::DoRaytracing()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DoRaytracing");

    // All updates to the CPU copy of the Shader Binding Table must be done before calling Commit.
    m_shaderBindingTable.Commit();

    commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

    // Bind the heaps, acceleration structure and dispatch rays.
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

    // Set common pipeline state and root parameters
    {
        // Update constant buffer memory onto the GPU
        m_sceneCB = m_graphicsMemory->AllocateConstant(m_sceneConstantData);

        auto heap = m_descriptorHeap->Heap();
        commandList->SetDescriptorHeaps(1, &heap);
        commandList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, m_sceneCB.GpuAddress());

        // Set index and successive vertex buffer descriptor tables
        commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
        commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
        commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, m_indexBuffer.gpuDescriptorHandle);
    }

    // Setup for DispatchRays
    {
        dispatchDesc.RayGenerationShaderRecord = m_shaderBindingTable.GetRayGenerationRecord(0);
        dispatchDesc.MissShaderTable = m_shaderBindingTable.GetMissShaderTable();
        dispatchDesc.HitGroupTable = m_shaderBindingTable.GetHitGroupShaderTable();
        dispatchDesc.Width = static_cast<uint32_t>(m_outputSize.right - m_outputSize.left);
        dispatchDesc.Height = static_cast<uint32_t>(m_outputSize.bottom - m_outputSize.top);
        dispatchDesc.Depth = 1;
        commandList->SetPipelineState1(m_dxrStateObject.Get());
        commandList->DispatchRays(&dispatchDesc);
    }

    PIXEndEvent(commandList);
}

void Sample::DoInlineRaytracing()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"DoInlineRaytracing");

    commandList->SetComputeRootSignature(m_inlineRaytracingRootSignature.Get());
    auto heap = m_descriptorHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_screenConstantData.dimensions = Vector2(static_cast<float>(m_outputSize.right - m_outputSize.left), static_cast<float>(m_outputSize.bottom - m_outputSize.top));
    
    // Copy the updated scene constant buffer to GPU.
    commandList->SetComputeRootConstantBufferView(InlineRootSignatureParams::SceneConstantSlot, m_sceneCB.GpuAddress());
    commandList->SetComputeRootConstantBufferView(InlineRootSignatureParams::CubeConstantSlot, m_cubeCB.GpuAddress());
    commandList->SetComputeRootConstantBufferView(InlineRootSignatureParams::ScreenConstantSlot, m_screenCB.GpuAddress());

    // Update constant buffer memory onto the GPU
    m_sceneCB = m_graphicsMemory->AllocateConstant(m_sceneConstantData);
    m_cubeCB = m_graphicsMemory->AllocateConstant(m_cubeConstantData);
    m_screenCB = m_graphicsMemory->AllocateConstant(m_screenConstantData);

    commandList->SetComputeRootDescriptorTable(InlineRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
    commandList->SetComputeRootShaderResourceView(InlineRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(InlineRootSignatureParams::VertexBuffersSlot, m_indexBuffer.gpuDescriptorHandle);
    commandList->SetPipelineState(m_inlineRaytracingPSO.Get());
    commandList->Dispatch(static_cast<uint32_t>((m_outputSize.right - m_outputSize.left) / THREAD_GROUP_X), static_cast<uint32_t>((m_outputSize.bottom - m_outputSize.top) / THREAD_GROUP_Y), 1);

    PIXEndEvent(commandList);
}

void Sample::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    auto device = m_deviceResources->GetD3DDevice();
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    DX::ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    DX::ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(rootSig->GetAddressOf())));
}

void Sample::CreateRootSignatures()
{
    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[2] = {}; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 descriptor for output texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 descriptors for a static index buffer and vertex buffer.

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count] = {};
        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        rootParameters[GlobalRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
        rootParameters[GlobalRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        globalRootSignatureDesc.Flags |= D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING;
        SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count] = {};
        rootParameters[LocalRootSignatureParams::CubeConstantSlot].InitAsConstants(SizeOfInUint32(m_cubeConstantData), 1);
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
    }
}

void Sample::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* pRaytracingPipeline)
{
    // Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

    // Local root signature to be used in a hit group.
    auto localRootSignature = pRaytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());

    // Define explicit shader association for the local root signature. 
    {
        auto rootSignatureAssociation = pRaytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(c_hitGroupName);
    }
}

void Sample::CreateRaytracingPipelineStateObject()
{
    auto device = m_deviceResources->GetD3DDevice();

    // When using GDKs earlier than June 2023, DXR Shader PDBs need to be explicitly saved in code. 
#if (defined(_DEBUG) || defined(PROFILE)) && (_GXDK_VER < 0x585D10B0)
    // Save the RtPso PDB on the scratch drive
    device->SetCompileTimeShaderPdbPathX(L"D:\\");
#endif

    // Create 7 subobjects that combine into a RTPSO:
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject
    // which has an explicit association specified purely for demonstration purposes.
    // 1 - DXIL library
    // 1 - Triangle hit group
    // 1 - Shader config
    // 2 - Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
    lib->SetDXILLibrary(&libdxil);

    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
    {
        lib->DefineExport(c_raygenShaderName);
        lib->DefineExport(c_closestHitShaderName);
        lib->DefineExport(c_missShaderName);
    }

    // Triangle hit group
    // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
    // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
    auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetClosestHitShaderImport(c_closestHitShaderName);
    hitGroup->SetHitGroupExport(c_hitGroupName);
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    uint32_t payloadSize = sizeof(Vector4);    // float4 pixelColor
    uint32_t attributeSize = sizeof(Vector2);  // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    // Local root signature and shader association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    CreateLocalRootSignatureSubobjects(&raytracingPipeline);

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();

    // PERFOMANCE TIP: Set max recursion depth as low as needed 
    // as drivers may apply optimization strategies for low recursion depths.
    uint32_t maxRecursionDepth = 1; // ~ primary rays only. 
    pipelineConfig->Config(maxRecursionDepth);

    PrintStateObjectDesc(raytracingPipeline);

    // Create the state object.
    DX::ThrowIfFailed(device->CreateStateObject(raytracingPipeline, IID_GRAPHICS_PPV_ARGS(m_dxrStateObject.GetAddressOf())), L"Couldn't create DirectX Raytracing state object.\n");
    DX::ThrowIfFailed(m_dxrStateObject->QueryInterface(IID_GRAPHICS_PPV_ARGS(m_dxrStateObjectProperties.GetAddressOf())));
}

void Sample::CreateRaytracingOutputResource()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto backbufferFormat = m_deviceResources->GetBackBufferFormat();

    // Create the output resource. The dimensions and format should match the swap-chain.
    auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, static_cast<uint64_t>(m_outputSize.right - m_outputSize.left), static_cast<UINT>(m_outputSize.bottom - m_outputSize.top), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_GRAPHICS_PPV_ARGS(m_raytracingOutput.ReleaseAndGetAddressOf())));
    SetDebugObjectName(m_raytracingOutput.Get(), L"Raytracing Output");

    D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;

    AllocateDescriptor(&uavDescriptorHandle, Descriptors::RaytracingOutputUAV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.Format = m_deviceResources->GetBackBufferFormat();
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
    m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->Heap()->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(Descriptors::RaytracingOutputUAV), m_descriptorHeap->Increment());
}

void Sample::BuildGeometry(ResourceUploadBatch& upload)
{
    auto device = m_deviceResources->GetD3DDevice();
    upload.Begin();

    // Cube indices.
    uint16_t indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    // Cube vertices positions and corresponding triangle normals.
    Vertex vertices[] =
    {
        { Vector3(-1.0f, 1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) },
        { Vector3(-1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) },

        { Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3(1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f) },
        { Vector3(-1.0f, -1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f) },

        { Vector3(-1.0f, -1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3(-1.0f, 1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
        { Vector3(-1.0f, 1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },

        { Vector3(1.0f, -1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3(1.0f, -1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3(1.0f, 1.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f) },
        { Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },

        { Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) },
        { Vector3(1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) },
        { Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) },
        { Vector3(-1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) },

        { Vector3(-1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
        { Vector3(-1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
    };

    CreateStaticBuffer(device, upload, indices, std::size(indices), D3D12_RESOURCE_STATE_GENERIC_READ, m_indexBuffer.resource.ReleaseAndGetAddressOf());
    SetDebugObjectName(m_indexBuffer.resource.Get(), L"Index Buffer Resource");

    CreateStaticBuffer(device, upload, vertices, std::size(vertices), D3D12_RESOURCE_STATE_GENERIC_READ, m_vertexBuffer.resource.ReleaseAndGetAddressOf());
    SetDebugObjectName(m_vertexBuffer.resource.Get(), L"Vertex Buffer Resource");

    auto finish = upload.End(m_deviceResources->GetCommandQueue());

    // Vertex buffer is passed to the shader along with index buffer as a descriptor table.
    auto const StructuredByteStride = sizeof(uint16_t) * 3; // 3 UINT16 per triangle
    auto const NumElements = sizeof(indices) / StructuredByteStride;
    static_assert(NumElements * StructuredByteStride == sizeof(indices), "Unexpected indices size");
    CreateBufferSRV(&m_indexBuffer, NumElements, StructuredByteStride, static_cast<uint32_t>(Descriptors::IndexBuffer));
    CreateBufferSRV(&m_vertexBuffer, ARRAYSIZE(vertices), _countof(vertices), static_cast<uint32_t>(Descriptors::VertexBuffer));
}

// Creates UAV buffers, puts together Acceleration Structure prebuild info, and allocates scratch space for the sample's TLAS and BLAS.
// These resources only need to be initialized once before rendering.
void Sample::CreateAccelerationStructureResources(ResourceUploadBatch& upload)
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto commandAllocator = m_deviceResources->GetCommandAllocator();

    upload.Begin();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer.resource->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = static_cast<uint32_t>(m_indexBuffer.resource->GetDesc().Width) / sizeof(UINT16);
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = static_cast<uint32_t>(m_vertexBuffer.resource->GetDesc().Width) / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer.resource->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

    // Mark the geometry as opaque.
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    DX::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    CreateUAVBuffer(device, bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &m_BLASScratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    SetDebugObjectName(m_BLASScratch.Get(), L"BLASScratchResource");

    CreateUAVBuffer(device, topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_TLASScratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    SetDebugObjectName(m_TLASScratch.Get(), L"TLASScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn't need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        CreateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState);
        SetDebugObjectName(m_bottomLevelAccelerationStructure.Get(), L"BottomLevelAccelerationStructure");

        CreateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState);
        SetDebugObjectName(m_topLevelAccelerationStructure.Get(), L"TopLevelAccelerationStructure");
    }

    // Create an instance desc for the bottom-level acceleration structure.
    ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    CreateStaticBuffer(device, upload, &instanceDesc, static_cast<size_t>(1), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, instanceDescs.ReleaseAndGetAddressOf());
    SetDebugObjectName(instanceDescs.Get(), L"Raytracing Instance Descriptions");

    auto finish = upload.End(m_deviceResources->GetCommandQueue());

    // Bottom Level Acceleration Structure desc
    {
        bottomLevelBuildDesc.ScratchAccelerationStructureData = m_BLASScratch->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Build Bottom-Level Acceleration Structure
    // Since the TLAS is built every frame, there is no need to rebuild during resource initialization stage
    {
        commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get());
        commandList->ResourceBarrier(1, &uavBarrier);

        // Kick off acceleration structure construction.
        DX::ThrowIfFailed(commandList->Close());
        ID3D12CommandList* commandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
    }

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    m_deviceResources->WaitForGpu();
}

void Sample::BuildTLAS()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"BuildTLAS");

    GraphicsResource instanceDescBuffer = GraphicsMemory::Get(nullptr).Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * c_numTLASInstances);
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = (D3D12_RAYTRACING_INSTANCE_DESC*)instanceDescBuffer.Memory();

    for (uint32_t i = 0; i < c_numTLASInstances; i++)
    {
        instanceDescs[i].AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDescs[i].InstanceContributionToHitGroupIndex = 0;
        instanceDescs[i].InstanceID = i;
        instanceDescs[i].InstanceMask = 0xFF;
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDescs[i].Transform), m_transforms[i]); // Instance transforms are represented as 3x4 matrices
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    topLevelBuildDesc.Inputs.InstanceDescs = instanceDescBuffer.GpuAddress();

    topLevelBuildDesc.Inputs.NumDescs = c_numTLASInstances;
    topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = m_TLASScratch->GetGPUVirtualAddress();

    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    PIXEndEvent(commandList);
}

void Sample::UpdateScene()
{
    // Rotate the dynamic red directional light around the origin
    Matrix rotate = Matrix::CreateRotationY(-2.0f * m_curRotationAngleRad);
    Vector3 redLightPosition = Vector3::Transform(Vector3(0.0f, 0.0f, -1.0f), rotate) * 5.0f;
    m_sceneConstantData.lightPosition = Vector4(redLightPosition);
    m_sceneConstantData.lightPosition.w = 1.0f;

    m_transforms[0] = Matrix::CreateRotationY(m_curRotationAngleRad);
    m_transforms[1] = Matrix::CreateScale(0.2f) * Matrix::CreateTranslation(Vector3(m_sceneConstantData.staticLightDirection * 5.0f));
    m_transforms[2] = Matrix::CreateScale(0.2f) * Matrix::CreateTranslation(redLightPosition); // Transforms for the center cube, static directional light, and rotating point light.

    m_sceneConstantData.rotate = m_transforms[0];
}

void Sample::BuildShaderTables()
{
    SimpleLightingRecord rayGenRecord(m_dxrStateObjectProperties.Get(), c_raygenShaderName, m_cubeConstantData);
    SimpleLightingRecord missShaderRecord(m_dxrStateObjectProperties.Get(), c_missShaderName, m_cubeConstantData);
    SimpleLightingRecord hitGroupRecord(m_dxrStateObjectProperties.Get(), c_closestHitShaderName, m_cubeConstantData);

    m_shaderBindingTable.SetRayGenRecord(0, rayGenRecord);
    m_shaderBindingTable.SetMissShaderRecord(0, missShaderRecord);
    m_shaderBindingTable.SetHitGroupRecord(0, hitGroupRecord);
}

// Copy the raytracing output to the backbuffer.
void Sample::CopyRaytracingOutputToBackbuffer()
{
    auto commandList = m_deviceResources->GetCommandList();
    auto renderTarget = m_deviceResources->GetRenderTarget();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"CopyRaytracingOutputToBackbuffer");
    D3D12_RESOURCE_BARRIER preCopyBarriers[2];
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);
    commandList->CopyResource(renderTarget, m_raytracingOutput.Get());

    D3D12_RESOURCE_BARRIER postCopyBarriers[2];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
    PIXEndEvent(commandList);
}

void Sample::CreateInlineRaytracingResources()
{
    auto csBlob = DX::ReadData(L"RaytracingInline.cso");
    auto device = m_deviceResources->GetD3DDevice();

    // Root Signature
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[2] = {}; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        CD3DX12_ROOT_PARAMETER rootParameters[InlineRootSignatureParams::Count] = {};
        rootParameters[InlineRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[InlineRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
        rootParameters[InlineRootSignatureParams::CubeConstantSlot].InitAsConstantBufferView(1);
        rootParameters[InlineRootSignatureParams::ScreenConstantSlot].InitAsConstantBufferView(2);
        rootParameters[InlineRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        rootParameters[InlineRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateRaytracingRootSignature(rootSignatureDesc, &m_inlineRaytracingRootSignature);

        DX::ThrowIfFailed(device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_inlineRaytracingRootSignature.ReleaseAndGetAddressOf())));
        SetDebugObjectName(m_inlineRaytracingRootSignature.Get(), L"m_inlineRaytracingRootSignature");
    }

    // Create the pipeline state for inline raytracing
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_inlineRaytracingRootSignature.Get();
        psoDesc.CS.pShaderBytecode = csBlob.data();
        psoDesc.CS.BytecodeLength = csBlob.size();

        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_inlineRaytracingPSO.ReleaseAndGetAddressOf())));
        SetDebugObjectName(m_inlineRaytracingPSO.Get(), L"m_inlineRaytracingPSO");
    }
}

void Sample::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, uint32_t descriptorIndex)
{
    auto descriptorHeapCpuBase = m_descriptorHeap->Heap()->GetCPUDescriptorHandleForHeapStart();

    *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, static_cast<int>(descriptorIndex), m_descriptorHeap->Increment());
}

void Sample::CreateBufferSRV(D3DBuffer* pBuffer, uint32_t numElements, uint32_t elementSize, uint32_t descriptorIndex)
{
    auto device = m_deviceResources->GetD3DDevice();

    // SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = numElements;
    if (elementSize == 0)
    {
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.StructureByteStride = 0;
    }
    else
    {
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = elementSize;
    }

    AllocateDescriptor(&pBuffer->cpuDescriptorHandle, descriptorIndex);

    device->CreateShaderResourceView(pBuffer->resource.Get(), &srvDesc, pBuffer->cpuDescriptorHandle);
    pBuffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->Heap()->GetGPUDescriptorHandleForHeapStart(), static_cast<int>(descriptorIndex), m_descriptorHeap->Increment());
}

#pragma endregion
