//--------------------------------------------------------------------------------------
// DXRTriangle.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DXRTriangle.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

// Shaders
#include "CompiledShaders/GlobalRootSignature.inc"
#include "CompiledShaders/RaytracingLibrary.inc"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_numInstancesInTLAS(1),
    m_rayFlags(D3D12_RAY_FLAG_NONE),
    m_holeSize(0.1f)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN,
        2, DX::DeviceResources::c_EnableDXR);
    m_deviceResources->RegisterDeviceNotify(this);
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
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);


    constexpr float SPEED_OF_CHANGE = 0.25f;

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        m_holeSize += pad.triggers.right * elapsedTime * SPEED_OF_CHANGE;
        m_holeSize -= pad.triggers.left * elapsedTime * SPEED_OF_CHANGE;

        if (pad.IsViewPressed() || kb.Escape)
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
        
    if (m_keyboardButtons.IsKeyPressed(Keyboard::OemPlus) || m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
    {
        m_numInstancesInTLAS++;
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::OemMinus) || m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
    {
        m_numInstancesInTLAS--;        
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::A) || m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
    {
        m_rayFlags = (D3D12_RAY_FLAGS)(m_rayFlags ^ D3D12_RAY_FLAG_FORCE_OPAQUE);
    }

    m_holeSize += kb.W * elapsedTime * SPEED_OF_CHANGE;
    m_holeSize -= kb.S * elapsedTime * SPEED_OF_CHANGE;

    // Clamp from 1 to MAX
    m_numInstancesInTLAS = std::min(m_numInstancesInTLAS, MAX_INSTANCES_IN_TLAS);
    m_numInstancesInTLAS = std::max(m_numInstancesInTLAS, 1U);
    m_holeSize = std::min(std::max(m_holeSize, 0.0f), 1.0f);
    
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
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

    auto commandList = m_deviceResources->GetCommandList();

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Build Acceleration Structures");

        BuildBottomLevelAccelerationStructure(true);
        BuildTopLevelAccelerationStructure(true);

        PIXEndEvent(commandList);
    }

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

        // All updates to the CPU copy of the Shader Binding Table must be done before calling Commit.
        m_shaderBindingTable.Commit();

        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
        dispatchRaysDesc.Width = (UINT)m_deviceResources->GetOutputSize().right;
        dispatchRaysDesc.Height = (UINT)m_deviceResources->GetOutputSize().bottom;
        dispatchRaysDesc.Depth = 1;
        dispatchRaysDesc.RayGenerationShaderRecord = m_shaderBindingTable.GetRayGenerationRecord(0);
        dispatchRaysDesc.MissShaderTable = m_shaderBindingTable.GetMissShaderTable();
        dispatchRaysDesc.HitGroupTable = m_shaderBindingTable.GetHitGroupShaderTable();

        UINT rootConstants[4] = { dispatchRaysDesc.Width, dispatchRaysDesc.Height, (UINT)m_rayFlags, *(UINT*)&m_holeSize };

        auto heap = m_csuDescriptorHeap->Heap();
        commandList->SetComputeRootSignature(m_globalRootSignature.Get());
        commandList->SetDescriptorHeaps(1, &heap);
        commandList->SetPipelineState1(m_raytracingStateObject.Get());
        commandList->SetComputeRootShaderResourceView(0, m_TLAS->GetGPUVirtualAddress());
        commandList->SetComputeRoot32BitConstants(1, ARRAYSIZE(rootConstants), rootConstants, 0);
        commandList->SetComputeRootDescriptorTable(2, m_csuDescriptorHeap->GetFirstGpuHandle());
        commandList->DispatchRays(&dispatchRaysDesc);

        PIXEndEvent(commandList);
    }
    
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Copy To Swap Chain");

        D3D12_RESOURCE_BARRIER uavToCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(m_UAVOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &uavToCopySrc);

        commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_UAVOutput.Get());

        D3D12_RESOURCE_BARRIER barriers[2];
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_UAVOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        PIXEndEvent(commandList);
    }

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD");
        RenderHUD(commandList);
        PIXEndEvent(commandList);
    }

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present(D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderHUD(ID3D12GraphicsCommandList* commandList)
{
    m_hudBatch->Begin(commandList);

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    m_smallFont->DrawString(m_hudBatch.get(), "DXR Triangle", textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"# of Triangles %u / %u", m_numInstancesInTLAS, MAX_INSTANCES_IN_TLAS);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    bool anyHitDisabled = m_rayFlags & D3D12_RAY_FLAG_FORCE_OPAQUE;
    swprintf_s(textBuffer, L"AnyHit Shader: %s", anyHitDisabled ? L"Disabled" : L"Enabled");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Hole size: %0.2f", m_holeSize);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));

    textPos.y -= m_smallFont->GetLineSpacing();
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A] / A : Toggle AnyHit Shader", textPos, textColor);

    textPos.y -= m_smallFont->GetLineSpacing();
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[RT][LT] / W,S : Increase/Decrease size of hole", textPos, textColor);

    textPos.y -= m_smallFont->GetLineSpacing();
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[DPAD] / +- : Increase/Decrease # of Triangles", textPos, textColor);
    

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
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources

void Sample::CreateRaytracingPipeline()
{
    auto device = m_deviceResources->GetD3DDevice();

#if defined(_GAMING_XBOX_SCARLETT) && (_GRDK_VER >= 0x55F00C58 /* GDK Edition 220300 */)
    // Save the RtPso PDB on the scratch drive
    device->SetCompileTimeShaderPdbPathX(L"D:\\");
#endif

    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    auto raytracingLibrary = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libraryDXIL = CD3DX12_SHADER_BYTECODE((void *)g_RaytracingLibrary, sizeof(g_RaytracingLibrary));
    raytracingLibrary->SetDXILLibrary(&libraryDXIL);

    const wchar_t* rayGenExportName = L"RayGenerationShader";
    const wchar_t* missShaderExportName = L"MissShader";
    const wchar_t* hitGroupExportName = L"HitGroup";

    // Setup our one and only Hit Group
    {
        auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetHitGroupExport(hitGroupExportName);
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        hitGroup->SetClosestHitShaderImport(L"ClosestHitShader");
        hitGroup->SetAnyHitShaderImport(L"AnyHitShader");
    }

    // Configure the shaders and pipeline
    {
        auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(4, 8);

        auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(1);
    }

    // Create Global Root Signature
    {
        // It is not currently possible to specify the D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING flag in HLSL, so this must be created in C++ with that flag set.
        // To make that process simpler, we'll just deserialize the one we have, add the flag and create it again.
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> rootSigDeserializer;
        DX::ThrowIfFailed(D3D12CreateVersionedRootSignatureDeserializer(g_GlobalRootSignature, sizeof(g_GlobalRootSignature), IID_GRAPHICS_PPV_ARGS(rootSigDeserializer.GetAddressOf())));

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = *(rootSigDeserializer->GetUnconvertedRootSignatureDesc());

#ifdef _GAMING_XBOX_SCARLETT
        rsDesc.Desc_1_1.Flags |= D3D12XBOX_ROOT_SIGNATURE_FLAG_RAYTRACING;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> mainBlob, errorBlob;
        DX::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rsDesc, mainBlob.GetAddressOf(), errorBlob.GetAddressOf()));
        DX::ThrowIfFailed(device->CreateRootSignature(0, mainBlob->GetBufferPointer(), mainBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(m_globalRootSignature.GetAddressOf())));
        m_globalRootSignature->SetName(L"GlobalRootSignature");
        
        auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        globalRootSignature->SetRootSignature(m_globalRootSignature.Get());
    }


    DX::ThrowIfFailed(device->CreateStateObject(raytracingPipeline, IID_GRAPHICS_PPV_ARGS(m_raytracingStateObject.GetAddressOf())));
    DX::ThrowIfFailed(m_raytracingStateObject->QueryInterface(IID_GRAPHICS_PPV_ARGS(m_raytracingStateObjectProps.GetAddressOf())));

    SimpleTriangleRecord rayGenRecord(m_raytracingStateObjectProps.Get(), rayGenExportName);
    SimpleTriangleRecord emptyMissShader;
    SimpleTriangleRecord validMissShader(m_raytracingStateObjectProps.Get(), missShaderExportName);
    SimpleTriangleRecord hitGroupRecord(m_raytracingStateObjectProps.Get(), hitGroupExportName);

    m_shaderBindingTable.SetRayGenRecord(0, rayGenRecord);
    m_shaderBindingTable.SetMissShaderRecord(0, emptyMissShader);
    m_shaderBindingTable.SetMissShaderRecord(1, validMissShader);
    m_shaderBindingTable.SetHitGroupRecord(0, hitGroupRecord);
}

void Sample::BuildBottomLevelAccelerationStructure(bool buildEveryFrame)
{
    if (!buildEveryFrame && m_triangleBLAS.Get() != nullptr)
        return;

    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();

    constexpr UINT vertexCount = 3;
    constexpr UINT vertexSize = sizeof(XMFLOAT3);
    constexpr UINT indexCount = 3;
    constexpr UINT indexSize = sizeof(UINT);

    // Build a BLAS for one triangle...
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = { D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES, D3D12_RAYTRACING_GEOMETRY_FLAG_NONE, {} };
    geometryDesc.Triangles.VertexCount = vertexCount;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexSize;
    geometryDesc.Triangles.IndexCount = indexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS rtInputs;
    rtInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    rtInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    rtInputs.NumDescs = 1;
    rtInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    rtInputs.pGeometryDescs = &geometryDesc;

    if (m_VB.Get() == nullptr)
    {
        // Create all the resources only once, even if we're going to rebuild the BLAS every frame.		
        D3D12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexCount * vertexSize);
        D3D12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(indexCount * indexSize);

        D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_VB.GetAddressOf())));
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_IB.GetAddressOf())));

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo;
        device->GetRaytracingAccelerationStructurePrebuildInfo(&rtInputs, &preBuildInfo);

        D3D12_RESOURCE_DESC blasDesc = CD3DX12_RESOURCE_DESC::Buffer(preBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(preBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &blasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            nullptr, IID_GRAPHICS_PPV_ARGS(m_triangleBLAS.GetAddressOf())));
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_GRAPHICS_PPV_ARGS(m_scratch.GetAddressOf())));
    }

    geometryDesc.Triangles.VertexBuffer.StartAddress = m_VB->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexBuffer = m_IB->GetGPUVirtualAddress();

    // Upload one triangle's worth of vertices and indices...
    XMFLOAT3 vertexPositions[3] = { { -0.5f, -0.5f, 0 }, { 0, 0.5f, 0 }, { 0.5f, -0.5f, 0 } };
    UINT indices[3] = { 0, 1, 2 };

    D3D12_SUBRESOURCE_DATA vbData = { vertexPositions, sizeof(vertexPositions), 0 };
    D3D12_SUBRESOURCE_DATA ibData = { indices, sizeof(indices), 0 };

    DirectX::ResourceUploadBatch uploadBatch(device);

    uploadBatch.Begin();
    uploadBatch.Transition(m_VB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    uploadBatch.Transition(m_IB.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    uploadBatch.Upload(m_VB.Get(), 0, &vbData, 1);
    uploadBatch.Upload(m_IB.Get(), 0, &ibData, 1);
    uploadBatch.Transition(m_VB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    uploadBatch.Transition(m_IB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    uploadBatch.End(m_deviceResources->GetCommandQueue());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = { m_triangleBLAS->GetGPUVirtualAddress(), rtInputs, 0, m_scratch->GetGPUVirtualAddress() };
    commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    commandList->ResourceBarrier(1, &uavBarrier);	// Need the bottom level build to finish before we can build a top-level acceleration structure.
}

void Sample::BuildTopLevelAccelerationStructure(bool buildEveryFrame)
{
    if (!buildEveryFrame && m_TLAS.Get() != nullptr)
        return;

    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();

    GraphicsResource instanceDescBuffer = GraphicsMemory::Get(nullptr).Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * m_numInstancesInTLAS);
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = (D3D12_RAYTRACING_INSTANCE_DESC*)instanceDescBuffer.Memory();

    float time = (float)m_timer.GetTotalSeconds() / 5.0f;  // Slow it down
    float sizeReductionPerInstance = 1.0f / m_numInstancesInTLAS;

    for (UINT i = 0; i < m_numInstancesInTLAS; i++)
    {
        instanceDescs[i].AccelerationStructure = m_triangleBLAS->GetGPUVirtualAddress();
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDescs[i].InstanceContributionToHitGroupIndex = 0;
        instanceDescs[i].InstanceID = i;
        instanceDescs[i].InstanceMask = 0xFF;

        ZeroMemory(instanceDescs[i].Transform, sizeof(instanceDescs[i].Transform));
        float size = 1.0f - (i * sizeReductionPerInstance);

        instanceDescs[i].Transform[0][0] = instanceDescs[i].Transform[1][1] = instanceDescs[i].Transform[2][2] = size;	// Scaled "Identity" matrix.
                
        instanceDescs[i].Transform[0][3] = sinf(time + i);
        instanceDescs[i].Transform[1][3] = 0;
        instanceDescs[i].Transform[2][3] = (float)i;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    topLevelBuildDesc.Inputs.InstanceDescs = instanceDescBuffer.GpuAddress();
    
    if (m_TLAS.Get() == nullptr)
    {
        // Create all the resources only once, even if we're going to rebuild the TLAS every frame.
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo;
        topLevelBuildDesc.Inputs.NumDescs = MAX_INSTANCES_IN_TLAS;  // Create enough room for several when we create the buffers.
        device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelBuildDesc.Inputs, &tlasPrebuildInfo);

        D3D12_RESOURCE_DESC tlasDesc = CD3DX12_RESOURCE_DESC::Buffer(tlasPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_RESOURCE_DESC tlasScratchDesc = CD3DX12_RESOURCE_DESC::Buffer(tlasPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &tlasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            nullptr, IID_GRAPHICS_PPV_ARGS(m_TLAS.GetAddressOf())));
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &tlasScratchDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_GRAPHICS_PPV_ARGS(m_TLASScratch.GetAddressOf())));
    }

    topLevelBuildDesc.Inputs.NumDescs = m_numInstancesInTLAS;
    topLevelBuildDesc.DestAccelerationStructureData = m_TLAS->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = m_TLASScratch->GetGPUVirtualAddress();

    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    commandList->ResourceBarrier(1, &uavBarrier);	// Need the top level build to finish before we can render with it...
}

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

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 features = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features)))
        || (features.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED))
    {
        OutputDebugStringA("ERROR: DirectX Raytracing (DXR) is not supported!\n");
        throw std::exception("DirectX Raytracing (DXR) is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_csuDescriptorHeap = std::make_unique<DescriptorHeap>(device, DescriptorIndex::EnumCount);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // HUD
    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    const SpriteBatchPipelineStateDescription spritePSD(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuDescriptorHeap->GetCpuHandle(DescriptorIndex::Font),
        m_csuDescriptorHeap->GetGpuHandle(DescriptorIndex::Font));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_csuDescriptorHeap->GetCpuHandle(DescriptorIndex::ControllerFont),
        m_csuDescriptorHeap->GetGpuHandle(DescriptorIndex::ControllerFont));
    
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    CreateRaytracingPipeline();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const outputSize = m_deviceResources->GetOutputSize();

    D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC uavOutputDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_deviceResources->GetBackBufferFormat(), (UINT64)outputSize.right, (UINT)outputSize.bottom, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &uavOutputDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_GRAPHICS_PPV_ARGS(m_UAVOutput.GetAddressOf())));

    device->CreateUnorderedAccessView(m_UAVOutput.Get(), nullptr, nullptr, m_csuDescriptorHeap->GetCpuHandle(DescriptorIndex::UAVOutput));

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
}

void Sample::OnDeviceLost()
{
    m_raytracingStateObject.Reset();
    m_raytracingStateObjectProps.Reset();
    m_globalRootSignature.Reset();
    m_UAVOutput.Reset();
    m_csuDescriptorHeap.reset();
    m_TLAS.Reset();
    m_TLASScratch.Reset();
    m_triangleBLAS.Reset();
    m_VB.Reset();
    m_IB.Reset();
    m_scratch.Reset();
    m_shaderBindingTable.Reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
