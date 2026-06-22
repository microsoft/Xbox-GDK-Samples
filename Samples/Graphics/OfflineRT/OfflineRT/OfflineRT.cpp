//--------------------------------------------------------------------------------------
// OfflineRT.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "OfflineRT/FileFormat.h"

#include "OfflineRT.h"

#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition March 2022 Update 1 */
#include <xmem.h>
#include <xg_xs.h>
#endif

#if (_GXDK_VER >= 0x63360C4B)   /* GDK Edition March 2024 */
#define DEHYDRATED_BVH_SUPPORTED
#endif

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
#include "RuntimeRTPSO.dxil-lib.h"  // Embedded runtime DXIL library (generated from RuntimeRTPSO.hlsl)

    ComPtr<ID3D12Resource> AllocateBuffer(ID3D12Device* pDevice, UINT64 bufferSize, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
    {
        ComPtr<ID3D12Resource> rv;
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
        DX::ThrowIfFailed(pDevice->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            initialResourceState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(rv.ReleaseAndGetAddressOf())));
        if (resourceName)
        {
            rv->SetName(resourceName);
        }
        return rv;
    }

#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition March 2022 Update 1 */
    ComPtr<ID3D12Resource> CreatePlacedBuffer(ID3D12Device* pDevice, D3D12_GPU_VIRTUAL_ADDRESS resourceToPlace, UINT64 bufferSize, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
    {
        ComPtr<ID3D12Resource> rv;
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
        DX::ThrowIfFailed(pDevice->CreatePlacedResourceX(
            resourceToPlace,
            &bufferDesc,
            initialResourceState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(rv.ReleaseAndGetAddressOf())));
        if (resourceName)
        {
            rv->SetName(resourceName);
        }
        return rv;
    }
#endif 

    static constexpr size_t kDescriptor_Count = 128;

    enum DescriptorHeapIndex : size_t
    {
        SRV_Font,
        SRV_CtrlFont,
        UAV_RTOutput,           // <-- Raytracing expects these to be continuous (global root signature slot 0)
        SRV_TLAS,               // <
        CBV_Scene,              // <
        SRV_Model_Start         // ModelsInfos will allocate linearly from here
    };

    enum GPUTimerIndex : uint32_t
    {
        BLASCreation,
        ModelRaytrace
    };

    const wchar_t* gPipelineTypeNames[] =
    {
        L"Runtime",
        L"Offline collections",
        L"Offline"
    };

    const wchar_t* BVHCompressionModeToString(BVHCompressionMode mode)
    {
        switch (mode)
        {
        case BVHCompressionMode::NONE: return L"None";
        case BVHCompressionMode::CPU: return L"CPU";
        case BVHCompressionMode::GPU: return L"GPU";
        case BVHCompressionMode::DEHYDRATED: return L"Dehydrated";
        default: return L"Unknown";
        }
    }

    const wchar_t* BVHCompressionModeToDecompressionString(BVHCompressionMode mode)
    {
        switch (mode)
        {
        case BVHCompressionMode::NONE: return L"No BVH Decompression";
        case BVHCompressionMode::CPU: return L"CPU Decompressed";
        case BVHCompressionMode::GPU: return L"GPU Decompressed";
        case BVHCompressionMode::DEHYDRATED: return L"Rehydrated";
        default: return L"Unknown";
        }
    }
} // End unnamed namespace

OfflineRT::OfflineRT() noexcept(false)
    : m_newModelIndex(0)
    , m_currentModelIndex(-1)
    , m_TLASBuildDesc()
    , m_currentPipelineIndex(0)
    , m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD
        | DX::DeviceResources::c_EnableDXR);
}

// Initialize the Direct3D resources required to run.
void OfflineRT::Initialize(HWND window)
{
    // Start by resetting the flag to decompress GPU Compressed BVHs or Rehydrate Dehydrated BVHs
    m_needToGPUDecompressOrRehydrateBVHs = false;

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Initialize GPU timer
    m_gpuTimer.RestoreDevice(m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());

    // Load model data and build/ deserialize bottom-level acceleration structure
    // (BLAS) data
    AddModelFromSDKMesh(L"dragon_LOD0.sdkmesh", { 1.0f, 0.0f, 0.0f, 1.0f });
    AddModelFromMDat(L"dragon_LOD0.mdat", { 0.0f, 1.0f, 0.0f, 1.0f });
    AddModelFromMDat(L"dragon_LOD0_bvhtoy_triangles.mdat", { 0.0f, 1.0f, 0.0f, 1.0f });
    AddModelFromMDat(L"dragon_LOD0_bvhtoy_quads.mdat", { 0.0f, 1.0f, 0.0f, 1.0f });

#if (_GXDK_VER >= 0x585D0BD0) /* GDK Edition 230300 */
    // BVH GPU Compression available in March 2023 GDK onwards
    DX::ThrowIfFailed(XGCreateBVHComputer2(m_deviceResources->GetD3DDevice(), &m_bvhComputer));
#else
    #if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition 220301 */
        // BVH CPU Compression available in March 2022 Update 1 GDK onwards
        DX::ThrowIfFailed(XGCreateBVHComputer(&m_bvhComputer));
    #endif 
#endif


#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition 220301 */
    // BVH CPU Compression available in March 2022 Update 1 GDK onwards
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0.compressed.mdat", { 0.0f, 0.0f, 1.0f, 1.0f }, BVHCompressionMode::CPU);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_triangles.compressed.mdat", { 0.0f, 0.0f, 1.0f, 1.0f }, BVHCompressionMode::CPU);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_quads.compressed.mdat", { 0.0f, 0.0f, 1.0f, 1.0f }, BVHCompressionMode::CPU);

#if (_GXDK_VER >= 0x585D0BD0) /* GDK Edition 230300 */
    // BVH GPU Compression available in March 2023 GDK onwards
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0.compressed.gpu.mdat", { 0.4f, 0.6f, 0.6f, 1.0f }, BVHCompressionMode::GPU);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_triangles.compressed.gpu.mdat", { 0.4f, 0.6f, 0.6f, 1.0f }, BVHCompressionMode::GPU);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_quads.compressed.gpu.mdat", { 0.4f, 0.6f, 0.6f, 1.0f }, BVHCompressionMode::GPU);

    UINT64 totalScratchSpaceRequiredToDecompress = 0;

    for (auto& bvh : m_modelInfos)
    {
        if (bvh->CompressionMode == BVHCompressionMode::GPU)
        {
            UINT64 scratchRequired = m_bvhComputer->GetRequiredGpuScratchSizeInBytes(1);

            bvh->ScratchRequiredToDecompress = scratchRequired;

            totalScratchSpaceRequiredToDecompress += scratchRequired;
        }
    }

    // Check
    if ( totalScratchSpaceRequiredToDecompress == 0 )
    {
        // Code below will fail when allocating 0 bytes
        totalScratchSpaceRequiredToDecompress = 32;
    }

    D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(totalScratchSpaceRequiredToDecompress);
    D3D12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &scratchDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_bvhDecompressionScratchBuffer.ReleaseAndGetAddressOf())));
#endif // (_GXDK_VER >= 0x55F00C6D) /* GDK Edition March 2023 */
#endif // (_GXDK_VER >= 0x55F00C6D) /* GDK Edition March 2022 Update 1 */

#if defined(DEHYDRATED_BVH_SUPPORTED)
    // BVH Dehydration available in March 2024 GDK onwards
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0.compressed.Dehydrated.mdat", { 0.8f, 0.6f, 0.2f, 1.0f }, BVHCompressionMode::DEHYDRATED);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_triangles.compressed.Dehydrated.mdat", { 0.8f, 0.6f, 0.2f, 1.0f }, BVHCompressionMode::DEHYDRATED);
    AddModelFromCompressedMDat<uint32_t>(L"dragon_LOD0_bvhtoy_quads.compressed.Dehydrated.mdat", { 0.8f, 0.6f, 0.2f, 1.0f }, BVHCompressionMode::DEHYDRATED);
#endif

    // Allocate top-level acceleration structure (TLAS) resources
    AllocateTLAS();

    // Create shared global root signature for all pipelines
    CreateGlobalRootSignature();

    // Load/ compile and deserialize raytracing pipelines
    AddPipelineFromEmbeddedDXILLib();
    AddPipelineFromSerializedCollections();
    AddPipelineFromSerializedRTPSO();

    // Setup camera
    m_camera = std::make_unique<DX::OrbitCamera>();
    m_camera->SetFocus(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f));
    m_camera->SetRadius(110.0f);
    m_camera->SetRotation(XMQuaternionRotationRollPitchYaw(0.0f, -XM_2PI / 8.0f, -0.2f));
    m_camera->SetRadiusRate(20.0f);
    m_camera->SetFlags(DX::OrbitCamera::c_FlagsDisableTranslation);
    m_camera->SetProjectionParameters(XMConvertToRadians(90.0f), 0.25f, 500.0f, true);

    // Setup game pad
    m_gamePad = std::make_unique<GamePad>();
}

void OfflineRT::Shutdown()
{
    m_deviceResources->WaitForGpu();

    m_gamePad.reset();
    m_camera.reset();

    m_pipelineInfos.clear();

    m_TLAS.Reset();
    m_TLASBuildScratch.Reset();

    m_modelInfos.clear();

    m_gpuTimer.ReleaseDevice();

    m_ctrlFont.reset();
    m_font.reset();
    m_rtOutput.Reset();

    m_sceneConstants.Reset();
    m_hudBatch.reset();
    m_descriptorHeap.reset();
    m_graphicsMemory.reset();
    m_deviceResources.reset();
}

#pragma region Frame Update
// Executes the basic game loop.
void OfflineRT::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %I64u", m_frame);

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
void OfflineRT::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Get merged pad state
    auto padState = m_gamePad->GetState(GamePad::c_MergedInput);

    // Update button tracker
    if (padState.IsConnected())
    {
        m_gamePadButtons.Update(padState);
        if (padState.IsViewPressed())
        {
            ExitGame();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Update camera
    m_camera->Update(elapsedTime, padState);

    // Switch model
    if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
    {
        m_newModelIndex = std::max(0, m_currentModelIndex - 1);
    }
    else if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
    {
        m_newModelIndex = std::min(m_currentModelIndex + 1, static_cast<int>(m_modelInfos.size()) - 1);
    }

    // Switch pipeline
    if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
    {
        m_currentPipelineIndex = std::min(m_currentPipelineIndex + 1, static_cast<int>(m_pipelineInfos.size()) - 1);
    }
    else if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
    {
        m_currentPipelineIndex = std::max(0, m_currentPipelineIndex - 1);
    }
}
#pragma endregion

#pragma region Frame Render

void OfflineRT::GPUDecompressAndRehydrateBVHs(ID3D12GraphicsCommandList* commandList)
{
#if (_GXDK_VER >= 0x585D0BD0) /* GDK Edition 230300 */
    // BVH GPU Compression available in March 2023 GDK onwards

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"BVH GPU Decompression");

    UINT numBVHsToGPUDecompress = 0;
    UINT numBVHsToRehydrate = 0;

    UINT64 scratchOffset = 0;
    D3D12_RAYTRACING_GEOMETRY_DESC gpuDescs[32]{};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC gpuInputs[32]{};

    D3D12_RAYTRACING_GEOMETRY_DESC rehydrationDescs[32]{};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC rehydrationInputs[32]{};

    for (auto& bvh : m_modelInfos)
    {
        bool gpuCompressed = bvh->CompressionMode == BVHCompressionMode::GPU;
        bool dehydrated = bvh->CompressionMode == BVHCompressionMode::DEHYDRATED;

        if (gpuCompressed || dehydrated)
        {
            MDatModelInfo* compressedBVH = (MDatModelInfo*)bvh.get();

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* input;
            D3D12_RAYTRACING_GEOMETRY_DESC* desc;

            if (gpuCompressed)
            {
                // GPU Compression
                input = &gpuInputs[numBVHsToGPUDecompress];
                desc = &gpuDescs[numBVHsToGPUDecompress];
                numBVHsToGPUDecompress++;
            }
            else
            {
                // Rehydration
                input = &rehydrationInputs[numBVHsToRehydrate];
                desc = &rehydrationDescs[numBVHsToRehydrate];
                numBVHsToRehydrate++;
            }

            // Rehydration supports setting Source and Dest to the same memory, GPU compression does not.
            // This sample does an "out-of-place" (Source != Dest) rehydration in order to demonstrate the feature
            // and allow the rehydration operation to be run every frame and PIX'ed if necessary.
            input->SourceAccelerationStructureData = compressedBVH->CompressedBVHBuffer->GetGPUVirtualAddress();
            input->DestAccelerationStructureData = compressedBVH->BLASMemory;

            // Scratch is not needed for rehydration
            input->ScratchAccelerationStructureData = (gpuCompressed) ? m_bvhDecompressionScratchBuffer->GetGPUVirtualAddress() + scratchOffset : D3D12_GPU_VIRTUAL_ADDRESS_NULL;

            scratchOffset += compressedBVH->ScratchRequiredToDecompress;

            input->Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
#if defined(DEHYDRATED_BVH_SUPPORTED)
            input->Inputs.Flags = (dehydrated ? D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_DEHYDRATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
#else
            input->Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
#endif
            input->Inputs.NumDescs = 1;  // This sample has one geometry per BLAS
            input->Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            input->Inputs.pGeometryDescs = desc;

            desc->Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc->Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            desc->Triangles.Transform3x4 = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            desc->Triangles.IndexFormat = compressedBVH->SizeOfIndices == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            desc->Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            desc->Triangles.IndexCount = (UINT)(compressedBVH->IndexBuffer->GetDesc().Width / compressedBVH->SizeOfIndices);
            desc->Triangles.IndexBuffer = compressedBVH->IndexBuffer->GetGPUVirtualAddress();
            desc->Triangles.VertexBuffer.StartAddress = compressedBVH->VertexBuffer->GetGPUVirtualAddress();
            desc->Triangles.VertexBuffer.StrideInBytes = 2 * sizeof(XMFLOAT3);
            desc->Triangles.VertexCount =  (UINT)(compressedBVH->VertexBuffer->GetDesc().Width / desc->Triangles.VertexBuffer.StrideInBytes);
        }
    }

    if (numBVHsToGPUDecompress > 0)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"BVH GPU Decompression");
        m_bvhComputer->DecompressBVH_BatchedGPU(commandList, gpuInputs, numBVHsToGPUDecompress);
        PIXEndEvent(commandList);
    }

#if defined(DEHYDRATED_BVH_SUPPORTED)
    if (numBVHsToRehydrate > 0)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"BVH Rehydration");

        ComPtr<ID3D12GraphicsCommandList8> commandList8;
        DX::ThrowIfFailed(commandList->QueryInterface(IID_GRAPHICS_PPV_ARGS(commandList8.GetAddressOf())));

        commandList8->RehydrateRaytracingAccelerationStructureBatchedX(numBVHsToRehydrate, rehydrationInputs);

        PIXEndEvent(commandList);
    }
#endif

    PIXEndEvent(commandList);
#else
    (commandList);
#endif
}

// Draws the scene.
void OfflineRT::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT); // This leaves the backbuffer in present state
    auto commandList = m_deviceResources->GetCommandList();

    m_gpuTimer.BeginFrame(commandList);

    // Check if we have loaded any GPU Compressed BVH models that we need to decompress
    if ( m_needToGPUDecompressOrRehydrateBVHs )
    {
        // Now we have a command list we can decompress them
        GPUDecompressAndRehydrateBVHs(commandList);

        // Reset the flag. Comment out this name if you want to decompress every frame
        m_needToGPUDecompressOrRehydrateBVHs = false;
    }

    if (m_newModelIndex != m_currentModelIndex)
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"TLAS rebuild");

        // New model selected -> rebuild TLAS
        BuildTLAS(commandList, m_newModelIndex);
        m_currentModelIndex = m_newModelIndex;
    }

    const auto outputSize = m_deviceResources->GetOutputSize();

    // Update scene constants
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Update scene constants");

        auto cbUploadMem = m_graphicsMemory->Allocate(sizeof(SceneConstants));
        SceneConstants& constants = *static_cast<SceneConstants*>(cbUploadMem.Memory());

        XMMATRIX camView = m_camera->GetView();
        XMMATRIX camProj = m_camera->GetProjection();
        XMVECTOR camPos  = m_camera->GetPosition();
        XMMATRIX worldViewProjection = camView * camProj;

        constants.projectionViewWorld   =  XMMatrixInverse(nullptr, worldViewProjection);
        XMStoreFloat3(&constants.cameraWorldPos, camPos);
        constants.rayMaxLength          = 500.0f;
        constants.lightWorldPos         = { 0.0f, 0.0f, -200.0f };
        constants.lightDiffuseColor     = { 1.0f, 1.0f, 1.0f, 1.0f };
        constants.lightAmbientColor     = { 0.0f, 0.0f, 0.2f, 1.0f };

        // Copy upload data to constant buffer
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrier);

        commandList->CopyBufferRegion(m_sceneConstants.Get(), 0, cbUploadMem.Resource(), cbUploadMem.ResourceOffset(), cbUploadMem.Size());
    }

    // Trace rays
    {
        auto pipelineInfo = m_pipelineInfos[m_currentPipelineIndex].get();

        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Raytrace");

        m_gpuTimer.Start(commandList, GPUTimerIndex::ModelRaytrace);

        D3D12_RESOURCE_BARRIER barriers[2] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap->Heap() };
        commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

        commandList->SetComputeRootSignature(m_globalRootSignature.Get());
        commandList->SetComputeRootDescriptorTable(0, m_descriptorHeap->GetGpuHandle(DescriptorHeapIndex::UAV_RTOutput));

        commandList->SetPipelineState1(pipelineInfo->StateObject.Get());

        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        dispatchDesc.RayGenerationShaderRecord = pipelineInfo->RayGenRecord;
        dispatchDesc.MissShaderTable = pipelineInfo->MissTable;
        dispatchDesc.HitGroupTable = pipelineInfo->HitGroupTable;
        dispatchDesc.Width = outputSize.right;
        dispatchDesc.Height = outputSize.bottom;
        dispatchDesc.Depth = 1;

        commandList->DispatchRays(&dispatchDesc);

        m_gpuTimer.Stop(commandList, GPUTimerIndex::ModelRaytrace);
    }

    // Copy from raytracing output buffer to backbuffer
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Copy to backbuffer");

        D3D12_RESOURCE_BARRIER barriers[2] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_rtOutput.Get());
    }

    // HUD draw
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"HUD draw");

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);

        auto const viewport = m_deviceResources->GetScreenViewport();
        auto const scissorRect = m_deviceResources->GetScissorRect();
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        auto const safeSize = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(outputSize.right), UINT(outputSize.bottom));
        XMFLOAT2 textPos = XMFLOAT2(float(safeSize.left), float(safeSize.top));
        XMVECTOR textColor = ATG::Colors::White;

        m_hudBatch->Begin(commandList);

        m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

        const auto& modelInfo = *m_modelInfos[m_currentModelIndex];
        const auto& pipelineInfo = *m_pipelineInfos[m_currentPipelineIndex];
        wchar_t textBuffer[512] = {};
        swprintf_s(textBuffer, _countof(textBuffer),
            L"OfflineRT\n"
            L"\nModel: %s\n"
            L"  Build type: %s\n"
            L"  Build time: %6.2f ms (%s)\n"
            L"  Build size: %4.2f KB\n"
            L"  Build scratch: %5.2f KB\n"
            L"  Draw time: %4.2f ms\n"
            L"\nPipeline: %s\n"
            L"  Type: %s\n"
            L"  Build time: %5.2f ms (offline)\n"
            L"  Create/ deserialize time: %5.2f ms",
            modelInfo.Name.c_str(),
            modelInfo.OfflineModel ? L"Offline" : L"Runtime",
            modelInfo.BLASCreationTime,
            modelInfo.OfflineModel ? BVHCompressionModeToDecompressionString(modelInfo.CompressionMode) : L"Runtime GPU",
            static_cast<float>(modelInfo.BLASSize) / 1024,
            static_cast<float>(modelInfo.BLASCreationScratchBytes) / 1024,
            m_gpuTimer.GetAverageMS(GPUTimerIndex::ModelRaytrace),
            pipelineInfo.Name.c_str(),
            gPipelineTypeNames[static_cast<size_t>(pipelineInfo.Type)],
            pipelineInfo.BuildTime,
            pipelineInfo.CreateTime);
        m_font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);

        // Controller instructions
        swprintf_s(textBuffer, _countof(textBuffer),
            L"[DPad] Switch model/ pipeline\n"
            L"[LThumb] Zoom/ spin camera\n"
            L"[RThumb] Orbit camera");

        textPos.y = float(safeSize.bottom - m_font->GetLineSpacing() * 3);
        DX::DrawControllerString(m_hudBatch.get(), m_font.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

        m_hudBatch->End();
    }

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    PIXEndEvent();

    m_gpuTimer.EndFrame(commandList);

    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
}
#pragma endregion

#pragma region Message Handlers
// Occurs when the game is being suspended.
void OfflineRT::OnSuspending()
{
    m_deviceResources->Suspend();
}

// Occurs when the game is resuming.
void OfflineRT::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void OfflineRT::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create graphics memory allocator
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Create descriptor heap
    m_descriptorHeap = std::make_unique<DescriptorHeap>(device, kDescriptor_Count);

    // Create HUD helper
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        const SpriteBatchPipelineStateDescription spritePSD(rtState, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();
    }

    // Create scene constant buffer
    {
        constexpr size_t sceneConstantsSize = sizeof(SceneConstants);
        m_sceneConstants = AllocateBuffer(device, sceneConstantsSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, L"SceneConstants");
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_sceneConstants->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = sceneConstantsSize;
        device->CreateConstantBufferView(&cbvDesc, m_descriptorHeap->GetCpuHandle(DescriptorHeapIndex::CBV_Scene));
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void OfflineRT::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const backbufferFormat = m_deviceResources->GetBackBufferFormat();
    auto const outputSize = m_deviceResources->GetOutputSize();
    bool const aboveHDResolution = outputSize.bottom > 1080;
    
    // Create output resource for raytracing
    auto rtOutputDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, outputSize.right, outputSize.bottom, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &rtOutputDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_rtOutput.ReleaseAndGetAddressOf())));
    m_rtOutput->SetName(L"Raytracing output");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(m_rtOutput.Get(), nullptr, &uavDesc, m_descriptorHeap->GetCpuHandle(DescriptorHeapIndex::UAV_RTOutput));

    // Load font and other UI element resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_font = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
            aboveHDResolution ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_descriptorHeap->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_descriptorHeap->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            aboveHDResolution ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_descriptorHeap->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_descriptorHeap->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();
    }
}
#pragma endregion

// Function loads an SDKMESH model and builds a bottom-level acceleration
// structure (BLAS).
void OfflineRT::AddModelFromSDKMesh(const wchar_t* filename, float4 albedoColor)
{
    auto device = m_deviceResources->GetD3DDevice();

    // Load model data
    auto model = Model::CreateFromSDKMESH(device, filename);

    // Create GPU resources for model data
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        model->LoadStaticBuffers(device, resourceUpload);
        resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();
    }

    // We only care about the first mesh of the first opaque part in the model
    auto modelMesh = model->meshes[0]->opaqueMeshParts[0].get();
    const auto& posVBDecl = (*modelMesh->vbDecl)[0];
    UINT alignedPosOffset = posVBDecl.AlignedByteOffset == 0xffffffff ? 0 : posVBDecl.AlignedByteOffset;
#ifdef _DEBUG
    const auto& normalVBDecl = (*modelMesh->vbDecl)[1];
    UINT alignedNormalOffset = normalVBDecl.AlignedByteOffset == 0xffffffff ? 12 : normalVBDecl.AlignedByteOffset;
#endif
    assert(!strcmp("SV_Position", posVBDecl.SemanticName) && (posVBDecl.Format == DXGI_FORMAT_R32G32B32_FLOAT) && (alignedPosOffset == 0));
    assert(!strcmp("NORMAL", normalVBDecl.SemanticName) && (normalVBDecl.Format == DXGI_FORMAT_R32G32B32_FLOAT) && (alignedNormalOffset == 12));

    // Create SRVs for model resources
    size_t descriptorIndex = SRV_Model_Start + (m_modelInfos.size() * 2);           // 2 SRVs per model in local root signature: index buffer SRV, normal buffer SRV
    auto modelSRVs = m_descriptorHeap->GetGpuHandle(descriptorIndex);
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Raw buffer for index data
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.FirstElement = modelMesh->startIndex * sizeof(uint32_t);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.NumElements = modelMesh->indexCount;
        device->CreateShaderResourceView(modelMesh->staticIndexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex));

        // Structured buffer view for normal data
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = modelMesh->vertexOffset;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = modelMesh->vertexStride;
        srvDesc.Buffer.NumElements = modelMesh->vertexCount;
        device->CreateShaderResourceView(modelMesh->staticVertexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex + 1));
    }

    // Build bottom-level-acceleration structure (BLAS)
    ComPtr<ID3D12Resource>  blas;
    UINT64 blasSize = {};
    float blasCreationTime = {};
    UINT64 blasCreationScratchBytes = {};
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometryDesc.Triangles.IndexBuffer = modelMesh->staticIndexBuffer->GetGPUVirtualAddress() + (modelMesh->startIndex * sizeof(uint32_t));
        geometryDesc.Triangles.IndexCount = modelMesh->indexCount;
        geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        geometryDesc.Triangles.Transform3x4 = 0;
        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometryDesc.Triangles.VertexCount = modelMesh->vertexCount;
        geometryDesc.Triangles.VertexBuffer.StartAddress = modelMesh->staticVertexBuffer->GetGPUVirtualAddress() + (modelMesh->vertexOffset * modelMesh->vertexStride) + alignedPosOffset;
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = modelMesh->vertexStride;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
        bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        bottomLevelInputs.Flags = buildFlags;
        bottomLevelInputs.NumDescs = 1;
        bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInputs.pGeometryDescs = &geometryDesc;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
        DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal BLAS size");

        // Allocate scratch buffer for build
        blasCreationScratchBytes = bottomLevelPrebuildInfo.ScratchDataSizeInBytes;
        auto scratchResource = AllocateBuffer(device, blasCreationScratchBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"BLASScratch");

        // Allocate acceleration structure buffer
        D3D12_RESOURCE_STATES initialASResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        blas = AllocateBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, initialASResourceState, L"BLAS");

        auto postBuildCurrentSize = m_graphicsMemory->Allocate(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC));
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo[] =
        {
            {postBuildCurrentSize.GpuAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE}
        };

        // Set resource pointers in BLAS description
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = blas->GetGPUVirtualAddress();

        // Build acceleration structure
        auto commandList = m_deviceResources->GetCommandList();
        commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);

        m_gpuTimer.BeginFrame(commandList);
        m_gpuTimer.Start(commandList, GPUTimerIndex::BLASCreation);
        commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, ARRAYSIZE(postbuildInfo), postbuildInfo);
        m_gpuTimer.Stop(commandList, GPUTimerIndex::BLASCreation);
        m_gpuTimer.EndFrame(commandList);

        m_deviceResources->ExecuteCommandList();
        m_deviceResources->WaitForGpu();

        m_gpuTimer.Flush(commandList);
        blasCreationTime = static_cast<float>(m_gpuTimer.GetElapsedMS(GPUTimerIndex::BLASCreation));

        blasSize = reinterpret_cast<const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC*>(postBuildCurrentSize.Memory())->CurrentSizeInBytes;
    }

    // Add model to list
    struct SDKMeshModelInfo : public ModelInfo
    {
        SDKMeshModelInfo(const wchar_t* name) : ModelInfo(name) { }

        std::unique_ptr<DirectX::Model>         Model;
    };

    auto modelInfo = std::make_unique<SDKMeshModelInfo>(filename);
    modelInfo->OfflineModel = false;
    modelInfo->CompressionMode = BVHCompressionMode::NONE;
    modelInfo->ModelSRVs = modelSRVs;
    modelInfo->BLAS = blas;
    modelInfo->BLASSize = blasSize;
    modelInfo->BLASCreationTime = blasCreationTime;
    modelInfo->BLASCreationScratchBytes = blasCreationScratchBytes;
    modelInfo->AlbedoColor = albedoColor;
    modelInfo->Model = std::move(model);

    m_modelInfos.push_back(std::move(modelInfo));
}

void OfflineRT::AddModelFromMDat(const wchar_t* filename, float4 albedoColor)
{
    auto device = m_deviceResources->GetD3DDevice();

    std::ifstream inStream(filename, std::ios::in | std::ios::binary);
    if (!inStream)
    {
        char buff[128] = {};
        sprintf_s(buff, "ERROR: %ls file not found\n", filename);
        OutputDebugStringA(buff);
        throw std::runtime_error(".mdat not found");
    }

    OfflineRTFiles::ModelFileHeader header;
    inStream.read(reinterpret_cast<char*>(&header), sizeof(header));
    assert(header.Magic == MAKEFOURCC('M', 'D', 'A', 'T'));

    // Allocate buffers for bottom-level acceleration structure (BLAS) and vertex data
    D3D12_RESOURCE_STATES initialASResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    auto blas = AllocateBuffer(device, header.PostbuildCurrentSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, initialASResourceState, L"BLAS");

    D3D12_RESOURCE_STATES initialBufferResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
    size_t indexBufferSize = header.IndexCount * sizeof(uint32_t);
    size_t vertexBufferSize = header.VertexCount * 2 * sizeof(XMFLOAT3);
    auto indexBuffer = AllocateBuffer(device, indexBufferSize, D3D12_RESOURCE_FLAG_NONE, initialBufferResourceState, L"Index buffer");
    auto vertexBuffer = AllocateBuffer(device, vertexBufferSize, D3D12_RESOURCE_FLAG_NONE, initialBufferResourceState, L"Vertex buffer");

    // Allocate upload memory and load data
    auto blasUploadMem = m_graphicsMemory->Allocate(header.PostbuildSerializedSize, 256);
    inStream.read(reinterpret_cast<char*>(blasUploadMem.Memory()), header.PostbuildSerializedSize);

    auto indexBufferUploadMem = m_graphicsMemory->Allocate(indexBufferSize);
    inStream.read(reinterpret_cast<char*>(indexBufferUploadMem.Memory()), indexBufferSize);
    auto vertexBufferUploadMem = m_graphicsMemory->Allocate(vertexBufferSize);
    inStream.read(reinterpret_cast<char*>(vertexBufferUploadMem.Memory()), vertexBufferSize);

    inStream.close();

    // Deserialize BLAS and copy vertex data
    {
        auto commandList = m_deviceResources->GetCommandList();
        commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);

        commandList->CopyRaytracingAccelerationStructure(blas.Get()->GetGPUVirtualAddress(), blasUploadMem.GpuAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE);

        commandList->CopyBufferRegion(indexBuffer.Get(), 0, indexBufferUploadMem.Resource(), indexBufferUploadMem.ResourceOffset(), indexBufferSize);
        commandList->CopyBufferRegion(vertexBuffer.Get(), 0, vertexBufferUploadMem.Resource(), vertexBufferUploadMem.ResourceOffset(), vertexBufferSize);
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER),
            CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        m_deviceResources->ExecuteCommandList();
        m_deviceResources->WaitForGpu();
    }

    // Create SRVs for model resources
    size_t descriptorIndex = SRV_Model_Start + (m_modelInfos.size() * 2);           // 2 SRVs per model in local root signature: index buffer SRV, normal buffer SRV
    auto modelSRVs = m_descriptorHeap->GetGpuHandle(descriptorIndex);
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Raw buffer for index data
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.NumElements = header.IndexCount;
        device->CreateShaderResourceView(indexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex));

        // Structured buffer view for normal data
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
        srvDesc.Buffer.NumElements = header.VertexCount;
        device->CreateShaderResourceView(vertexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex + 1));
    }

    // Add model to list
    struct MDatModelInfo : public ModelInfo
    {
        MDatModelInfo(const wchar_t* name) : ModelInfo(name) { }

        ComPtr<ID3D12Resource> IndexBuffer;
        ComPtr<ID3D12Resource> VertexBuffer;
    };

    auto modelInfo = std::make_unique<MDatModelInfo>(filename);
    modelInfo->OfflineModel = true;
    modelInfo->CompressionMode = BVHCompressionMode::NONE;
    modelInfo->ModelSRVs = modelSRVs;
    modelInfo->BLAS = blas;
    modelInfo->BLASSize = header.PostbuildCurrentSize;
    modelInfo->BLASCreationTime = header.BuildTime + header.SerializationTime;
    modelInfo->BLASCreationScratchBytes = header.PrebuildScratchSize;
    modelInfo->AlbedoColor = albedoColor;
    modelInfo->IndexBuffer = std::move(indexBuffer);
    modelInfo->VertexBuffer = std::move(vertexBuffer);

    m_modelInfos.push_back(std::move(modelInfo));
}

#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition 220301 */
// BVH Compression available in March 2022 Update 1 GDK onwards
template<class index_t>
void OfflineRT::AddModelFromCompressedMDat(const wchar_t* filename, float4 albedoColor, BVHCompressionMode compressionMode)
{
    auto device = m_deviceResources->GetD3DDevice();

    std::ifstream inStream(filename, std::ios::in | std::ios::binary);

    OfflineRTFiles::ModelFileHeader header;
    inStream.read(reinterpret_cast<char*>(&header), sizeof(header));
    assert(header.Magic == MAKEFOURCC('M', 'D', 'A', 'T'));

    size_t   indexBufferSize  = header.IndexCount * sizeof(uint32_t);
    uint32_t vertexStride     = 2 * sizeof(XMFLOAT3);
    size_t   vertexBufferSize = header.VertexCount * vertexStride;

    // Dehydrated BVHs don't undergo serialisation, so the 'currentSize' is all we need.
    bool wasSerialized = (compressionMode == BVHCompressionMode::DEHYDRATED) ? false : true;
    size_t buildSize = wasSerialized ? header.PostbuildSerializedSize : header.PostbuildCurrentSize;

    // Create buffer memory with the right page protections
    PVOID compressedBVHBufferMem = XMemVirtualAlloc(NULL,
        buildSize,
        MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT,
        XMEM_GRAPHICS,
        PAGE_READWRITE | PAGE_GRAPHICS_READONLY);

    PVOID indexBufferMem = XMemVirtualAlloc ( NULL,
                                              indexBufferSize,
                                              MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT,
                                              XMEM_GRAPHICS,
                                              PAGE_READWRITE | PAGE_GRAPHICS_READONLY);

    PVOID vertexBufferMem = XMemVirtualAlloc ( NULL,
                                               vertexBufferSize,
                                               MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT,
                                               XMEM_GRAPHICS,
                                               PAGE_READWRITE | PAGE_GRAPHICS_READONLY);

    // Read the header data
    size_t serializedSize, deserializedSize, headerSize;

    if (wasSerialized)
    {
        D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER blasHeader;
        inStream.read(reinterpret_cast<char*>(&blasHeader), sizeof(blasHeader));

        serializedSize = header.PostbuildSerializedSize;
        deserializedSize = blasHeader.DeserializedSizeInBytes;
        headerSize = sizeof(blasHeader);
    }
    else
    {
        serializedSize = header.PostbuildCurrentSize;
        deserializedSize = header.PostbuildCurrentSize;
        headerSize = 0;
    }    

    // Allocate upload memory and load BVH data (minus the already read header size)
    auto blasCompressedBuffer = CreatePlacedBuffer( device, (D3D12_GPU_VIRTUAL_ADDRESS)compressedBVHBufferMem, serializedSize,  D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"Compressed BVH"  );
    inStream.read(reinterpret_cast<char*>(blasCompressedBuffer->GetGPUVirtualAddress()), serializedSize - headerSize);
       
    auto indexBuffer  = CreatePlacedBuffer( device, (D3D12_GPU_VIRTUAL_ADDRESS)indexBufferMem,  indexBufferSize,  D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER, L"Index Buffer"  );
    inStream.read(reinterpret_cast<char*>(indexBuffer->GetGPUVirtualAddress()), indexBufferSize);
    
    auto vertexBuffer = CreatePlacedBuffer( device, (D3D12_GPU_VIRTUAL_ADDRESS)vertexBufferMem, vertexBufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, L"Vertex Buffer" );
    inStream.read(reinterpret_cast<char*>(vertexBuffer->GetGPUVirtualAddress()), vertexBufferSize);

    inStream.close();

    // Allocate buffers for bottom-level acceleration structure (BLAS) with the right page protection
    PVOID blasDecompressedMem = XMemVirtualAlloc ( NULL,
                                                   deserializedSize,
                                                   MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT,
                                                   XMEM_GRAPHICS,
                                                   ((compressionMode == BVHCompressionMode::CPU) ? PAGE_READWRITE : PAGE_NOACCESS) | PAGE_GRAPHICS_READWRITE);
    DX::ThrowIfFalse(blasDecompressedMem, "Failed to allocate memory for Decompressed BVH");

    auto blas = CreatePlacedBuffer( device, (D3D12_GPU_VIRTUAL_ADDRESS)blasDecompressedMem, deserializedSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BLAS" );
    
    // Create the arrays of Geometries required to Decompress
    const uint32_t numberOfGeometries = 1;

    XG_FORMAT      vertexFormats[numberOfGeometries];
    XG_FORMAT      indexFormats[numberOfGeometries];
    const void*    vertexBuffers[numberOfGeometries];
    const void*    indexBuffers[numberOfGeometries];
    
    // And set the values for each Geometry
    vertexFormats[0] = XG_FORMAT_R32G32B32_FLOAT;
    indexFormats[0]  = (sizeof(index_t) == 2)?XG_FORMAT_R16_UINT:XG_FORMAT_R32_UINT;
    vertexBuffers[0] = (const void*)vertexBuffer->GetGPUVirtualAddress();
    indexBuffers[0]  = (const void*)indexBuffer->GetGPUVirtualAddress();

    // Add timings to show how quick the decompression is
    LARGE_INTEGER startTime, endTime;
    LARGE_INTEGER  timerFrequency;
    QueryPerformanceFrequency( &timerFrequency );

    HRESULT decompressResult = S_FALSE;

    // Put all the data into the required structures
    auto pDescs = new D3D12_RAYTRACING_GEOMETRY_DESC[numberOfGeometries];

    for (uint32_t i = 0; i < numberOfGeometries; ++i)
    {
        D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc = pDescs[i];
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometryDesc.Triangles.IndexBuffer  = (D3D12_GPU_VIRTUAL_ADDRESS)indexBuffers[i];
        geometryDesc.Triangles.IndexCount   = 0;    // Currently unused
        geometryDesc.Triangles.IndexFormat  = (DXGI_FORMAT)indexFormats[i];
        geometryDesc.Triangles.Transform3x4 = 0;
        geometryDesc.Triangles.VertexFormat = (DXGI_FORMAT)vertexFormats[i];
        geometryDesc.Triangles.VertexCount  = 0;    // Currently unused
        geometryDesc.Triangles.VertexBuffer.StartAddress = (D3D12_GPU_VIRTUAL_ADDRESS)vertexBuffers[i];
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC RTStructureDesc;
    RTStructureDesc.DestAccelerationStructureData = (D3D12_GPU_VIRTUAL_ADDRESS)blas->GetGPUVirtualAddress();
    RTStructureDesc.SourceAccelerationStructureData = (D3D12_GPU_VIRTUAL_ADDRESS)compressedBVHBufferMem;
    RTStructureDesc.ScratchAccelerationStructureData = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

    // Pass in Flags = 0xFFFFFFFF to indicate that we don't have index and vertex counts, so take the slow path
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs = RTStructureDesc.Inputs;
    Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    Inputs.Flags = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)0xFFFFFFFF;
    Inputs.NumDescs = numberOfGeometries;
    Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    Inputs.pGeometryDescs = pDescs;

    // Check on the compression type
    if ( compressionMode == BVHCompressionMode::CPU )
    {
        // Query the start time
        QueryPerformanceCounter( &startTime );

        // Decompress the BVH into the buffer using the CPU decompression method
        decompressResult =  m_bvhComputer->DecompressBVH( RTStructureDesc );    }
    else
    {
        // Query the start time (although this doesn't really apply for GPU Decompression)
        QueryPerformanceCounter( &startTime );

        // Note: At this point in the sample we do not have a Command List that we can
        // use to call the GPU decompression, so we can only set any flags required
        // to notify the sample to do the decompression during the first call to Render()
        decompressResult = S_OK;
        m_needToGPUDecompressOrRehydrateBVHs = true;
    }

    // Query the stop time
    QueryPerformanceCounter( &endTime );

    // Check that everything is good
    if ( decompressResult != S_OK )
    {
        wchar_t buff[128] = {};
        swprintf_s( buff,
                   L"ERROR: %s BVH Decompression failed with HRESULT %X\n",
                   BVHCompressionModeToString(compressionMode),
                   decompressResult);
        OutputDebugString(buff);
        throw std::runtime_error("BVH Decompression failed");
    }

    // How long did it take ?
    LARGE_INTEGER elapsedTime;
    elapsedTime.QuadPart = endTime.QuadPart - startTime.QuadPart;
    elapsedTime.QuadPart *= 1000000;
    elapsedTime.QuadPart /= timerFrequency.QuadPart;

    // Create SRVs for model resources
    size_t descriptorIndex = SRV_Model_Start + (m_modelInfos.size() * 2);           // 2 SRVs per model in local root signature: index buffer SRV, normal buffer SRV
    auto modelSRVs = m_descriptorHeap->GetGpuHandle(descriptorIndex);
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // Raw buffer for index data
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.NumElements = (sizeof(index_t) == 2)?(header.IndexCount/2):(header.IndexCount);
        device->CreateShaderResourceView(indexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex));

        // Structured buffer view for normal data
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
        srvDesc.Buffer.NumElements = header.VertexCount;
        device->CreateShaderResourceView(vertexBuffer.Get(), &srvDesc, m_descriptorHeap->GetCpuHandle(descriptorIndex + 1));
    }

    auto modelInfo = std::make_unique<MDatModelInfo>(filename);
    modelInfo->OfflineModel = true;
    modelInfo->CompressionMode = compressionMode;
    modelInfo->BLASMemory = blas->GetGPUVirtualAddress();
    modelInfo->CompressedBVHBuffer = std::move(blasCompressedBuffer);
    modelInfo->ModelSRVs = modelSRVs;
    modelInfo->BLAS = blas;
    modelInfo->BLASSize = header.PostbuildCurrentSize;
    modelInfo->BLASCreationTime = (float)((float)elapsedTime.QuadPart / 1000.0f); // Decompression time in milliseconds
    modelInfo->BLASCreationScratchBytes = header.PrebuildScratchSize;
    modelInfo->AlbedoColor   = albedoColor;
    modelInfo->IndexBuffer   = std::move(indexBuffer);
    modelInfo->VertexBuffer  = std::move(vertexBuffer);
    modelInfo->SizeOfIndices = sizeof(index_t);

    m_modelInfos.push_back(std::move(modelInfo));

    // Delete temporary data
    delete [] pDescs;
}
#endif // _GXDK_VER >= 0x55F00C6D /* GDK Edition March 2022 Update 1 */

// Function allocates resources for the top-level acceleration structure (TLAS).
// The TLAS will remain uninitialized until BuildTLAS has been called at least
// once.
void OfflineRT::AllocateTLAS()
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    // Top-level-acceleration (TLAS) description
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = m_TLASBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = kTLASInstanceCount;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    DX::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal TLAS size");

    // Allocate scratch buffer for build
    UINT64 scratchSize = topLevelPrebuildInfo.ScratchDataSizeInBytes;
    m_TLASBuildScratch = AllocateBuffer(device, scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"TLASBuildScratch");

    // Allocate acceleration structure buffer
    D3D12_RESOURCE_STATES initialASResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    m_TLAS = AllocateBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, initialASResourceState, L"TLAS");

    // Set resource pointers in TLAS description
    m_TLASBuildDesc.DestAccelerationStructureData = m_TLAS->GetGPUVirtualAddress();
    m_TLASBuildDesc.ScratchAccelerationStructureData = m_TLASBuildScratch->GetGPUVirtualAddress();

    // Create TLAS SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.RaytracingAccelerationStructure.Location = m_TLAS->GetGPUVirtualAddress();
    device->CreateShaderResourceView(nullptr, &srvDesc, m_descriptorHeap->GetCpuHandle(DescriptorHeapIndex::SRV_TLAS));
}

// Function builds the top-level acceleration structure with an instance of the
// model. The build is done on the GPU.
void OfflineRT::BuildTLAS(ID3D12GraphicsCommandList6* commandList, size_t instanceModelIndex)
{
    static_assert(kTLASInstanceCount == 1, "Code must be updated here to initialize more instances");
    auto instanceDescUploadMem = m_graphicsMemory->Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * kTLASInstanceCount);

    // Fill out single instance info
    auto& instanceDesc = *reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instanceDescUploadMem.Memory());
    instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.Transform[1][3] = -65.68f;             // Model is off center
    instanceDesc.InstanceMask = 1;
    instanceDesc.InstanceContributionToHitGroupIndex = static_cast<UINT>(instanceModelIndex);
    instanceDesc.AccelerationStructure = m_modelInfos[instanceModelIndex]->BLAS->GetGPUVirtualAddress();

    // Set instance memory pointer in TLAS description
    m_TLASBuildDesc.Inputs.InstanceDescs = instanceDescUploadMem.GpuAddress();

    // Build acceleration structure
    commandList->BuildRaytracingAccelerationStructure(&m_TLASBuildDesc, 0, nullptr);
}

void OfflineRT::CreateGlobalRootSignature()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create shared global root signature from embedded RuntimeRTPSO DXIL
    // library.
    //
    // The root signature could also be created fully at runtime (or fully
    // offline) from a desc via D3D12SerializeRootSignature or
    // D3D12SerializeVersionedRootSignature.
    DX::ThrowIfFailed(device->CreateRootSignature(
        1,
        g_RuntimeRTPSO_DXIL_Lib, ARRAYSIZE(g_RuntimeRTPSO_DXIL_Lib),
        IID_GRAPHICS_PPV_ARGS(m_globalRootSignature.ReleaseAndGetAddressOf())));
    m_globalRootSignature->SetName(L"Shared global root signature");
}

// Functions adds a pipeline from an embedded DXIL library. This incurs a
// (potentially) non-trivial runtime cost because the driver has to compile the
// referenced shaders and link the whole pipeline together.
//
// When the driver compiles shaders, you will see warnings like this in the Output window:
//
// XBSC W1003: Runtime Recompilation Required (DxStage=COMPUTE, HwStage=CS) - No precompiled shader available
void OfflineRT::AddPipelineFromEmbeddedDXILLib()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto pipelineInfo = std::make_unique<PipelineInfo>(L"RuntimeRTPSO", PipelineType::Runtime);

    CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // DXIL library contains all subobjects needed for the pipeline - we rely on default association
    // (https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#subobject-association-behavior)
    // to link shaders/ hitgroups to root signatures
    D3D12_SHADER_BYTECODE dxilLibBytecode = { g_RuntimeRTPSO_DXIL_Lib, ARRAYSIZE(g_RuntimeRTPSO_DXIL_Lib) };
    auto libSubObject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    libSubObject->SetDXILLibrary(&dxilLibBytecode);

    DX::CPUTimer cpuTimer;
    cpuTimer.Start();
    DX::ThrowIfFailed(device->CreateStateObject(stateObjectDesc, IID_GRAPHICS_PPV_ARGS(pipelineInfo->StateObject.ReleaseAndGetAddressOf())));
    cpuTimer.Stop();
    pipelineInfo->CreateTime = static_cast<float>(cpuTimer.GetElapsedMS());

    pipelineInfo->StateObject->SetName(pipelineInfo->Name.c_str());

    // Create shader binding table for pipeline
    AddShaderBindingTable(pipelineInfo.get(), L"RuntimeRTPSO_RayGenShader", L"RuntimeRTPSO_HitGroup", L"RuntimeRTPSO_MissShader");

    // Add to list of available pipelines
    m_pipelineInfos.push_back(std::move(pipelineInfo));
}

// Function adds a pipeline that is runtime linked from multiple collections.
// This is fast because all shaders have been fully precompiled by the PC UMD.
void OfflineRT::AddPipelineFromSerializedCollections()
{
    auto device = m_deviceResources->GetD3DDevice();

    struct CollectionPipelineInfo : public PipelineInfo
    {
        CollectionPipelineInfo(const wchar_t* name, PipelineType type)
            : PipelineInfo(name, type)
        {
        }

        // RTPSOs that reference collections via
        // D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION don't currently AddRef
        // correctly (that is: collection objects must be kept manually alive
        // for the duration of any RTPSO referencing them).
        //
        // Bug 33668549: Graphics: XDXR does not correctly AddRef collections referenced from RTPSOs via D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION
        ComPtr<ID3D12StateObject>   CollectionObjects[2];
    };

    auto pipelineInfo = std::make_unique<CollectionPipelineInfo>(L"OfflineCollectionRTPSO", PipelineType::OfflineCollections);

    // Deserialize collections and add them into the RTPSO as
    // D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION subobjects.
    const wchar_t* collectionFiles[2] =
    {
        L"OfflineCollectionRaygen.sobj",
        L"OfflineCollectionHitgroups.sobj",
    };

    CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    for (size_t i = 0; i < ARRAYSIZE(CollectionPipelineInfo::CollectionObjects); i++)
    {
        std::ifstream inStream(collectionFiles[i], std::ios::in | std::ios::binary);
        if (!inStream)
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: %ls file not found\n", collectionFiles[i]);
            OutputDebugStringA(buff);
            throw std::runtime_error("sobj collection file not found");
        }

        OfflineRTFiles::StateObjectFileHeader header;
        inStream.read(reinterpret_cast<char*>(&header), sizeof(header));
        assert(header.Magic == MAKEFOURCC('S', 'O', 'B', 'J'));
        assert(header.Type == OfflineRTFiles::StateObjectType::Collection);

        // Allocated buffer and load serialized collection data
        std::vector<uint8_t> serializedCollectionData;
        serializedCollectionData.resize(header.SerializedSize);
        inStream.read(reinterpret_cast<char*>(serializedCollectionData.data()), header.SerializedSize);
        inStream.close();

        // Deserialize collection object
        DX::ThrowIfFailed(device->DeserializeStateObjectX(
            serializedCollectionData.data(), static_cast<UINT>(serializedCollectionData.size()),
            m_globalRootSignature.Get(),
            0,
            IID_GRAPHICS_PPV_ARGS(pipelineInfo->CollectionObjects[i].ReleaseAndGetAddressOf())));

        // Tally up build times
        pipelineInfo->BuildTime += header.BuildTime + header.SerializationTime;

        // Add to state object
        auto collectionSubObject = stateObjectDesc.CreateSubobject<CD3DX12_EXISTING_COLLECTION_SUBOBJECT>();
        collectionSubObject->SetExistingCollection(pipelineInfo->CollectionObjects[i].Get());
    }

    // Create state object
    DX::CPUTimer cpuTimer;
    cpuTimer.Start();
    DX::ThrowIfFailed(device->CreateStateObject(stateObjectDesc, IID_GRAPHICS_PPV_ARGS(pipelineInfo->StateObject.ReleaseAndGetAddressOf())));
    cpuTimer.Stop();
    pipelineInfo->CreateTime = static_cast<float>(cpuTimer.GetElapsedMS());

    pipelineInfo->StateObject->SetName(pipelineInfo->Name.c_str());

    // Create shader binding table for pipeline
    // Notice how it is possible to reference raygen and hitgroups from
    // different collections and have the state object resolve the shader entry
    // points correctly.
    AddShaderBindingTable(pipelineInfo.get(), L"OfflineCollection_RayGenShader", L"OfflineCollection_HitGroup", L"OfflineCollection_MissShader");

    // Add to list of available pipelines
    m_pipelineInfos.push_back(std::move(pipelineInfo));
}

// Function adds a pipeline from a fully pre-serialized state object. This is
// even faster because all shaders have been fully precompiled and linked by the
// PC UMD.
void OfflineRT::AddPipelineFromSerializedRTPSO()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto pipelineInfo = std::make_unique<PipelineInfo>(L"OfflineRTPSO", PipelineType::Offline);

    std::ifstream inStream(L"OfflineRTPSO.sobj", std::ios::in | std::ios::binary);
    if (!inStream)
    {
        OutputDebugStringA("ERROR OfflineRTPSO.sobj not found\n");
        throw std::runtime_error("OfflineRTPSO.sobj");
    }

    OfflineRTFiles::StateObjectFileHeader header;
    inStream.read(reinterpret_cast<char*>(&header), sizeof(header));
    assert(header.Magic == MAKEFOURCC('S', 'O', 'B', 'J'));
    assert(header.Type == OfflineRTFiles::StateObjectType::RTPSO);

    // Allocated buffer and load serialized state object data
    std::vector<uint8_t> serializedStateObjectData;
    serializedStateObjectData.resize(header.SerializedSize);
    inStream.read(reinterpret_cast<char*>(serializedStateObjectData.data()), header.SerializedSize);
    inStream.close();

    // Deserialize state object
    DX::CPUTimer cpuTimer;
    cpuTimer.Start();
    DX::ThrowIfFailed(device->DeserializeStateObjectX(
        serializedStateObjectData.data(), static_cast<UINT>(serializedStateObjectData.size()),
        m_globalRootSignature.Get(),
        0,
        IID_GRAPHICS_PPV_ARGS(pipelineInfo->StateObject.ReleaseAndGetAddressOf())));
    cpuTimer.Stop();
    pipelineInfo->BuildTime = header.BuildTime + header.SerializationTime;
    pipelineInfo->CreateTime = static_cast<float>(cpuTimer.GetElapsedMS());

    pipelineInfo->StateObject->SetName(pipelineInfo->Name.c_str());

    // Create shader binding table for pipeline
    AddShaderBindingTable(pipelineInfo.get(), L"OfflineRTPSO_RayGenShader", L"OfflineRTPSO_HitGroup", L"OfflineRTPSO_MissShader");

    // Add to list of available pipelines
    m_pipelineInfos.push_back(std::move(pipelineInfo));
}

// Function adds a shader binding table to pipeline with the given entrypoints
void OfflineRT::AddShaderBindingTable(PipelineInfo* pipelineInfo, const wchar_t* rayGenShaderName, const wchar_t* hitGroupName, const wchar_t* missShaderName)
{
    auto device = m_deviceResources->GetD3DDevice();

    ComPtr<ID3D12StateObjectProperties> stateObjectProps;
    DX::ThrowIfFailed(pipelineInfo->StateObject.As(&stateObjectProps));

    const void* rayGenShaderIdentifier = stateObjectProps->GetShaderIdentifier(rayGenShaderName);
    const void* hitGroupShaderIdentifier = stateObjectProps->GetShaderIdentifier(hitGroupName);
    const void* missShaderIdentifier = stateObjectProps->GetShaderIdentifier(missShaderName);

    // Calculate combined shader table layout
    struct HitGroupLocalRootData
    {
        D3D12_GPU_DESCRIPTOR_HANDLE modelResources;     // slot 0: g_indices, g_vertices
        MaterialConstants materialCB;                   // slot 1: g_materialCB (inlined)
    };

    constexpr size_t rayGenTableRecordCount = 1;
    const size_t hitGroupTableRecordCount = m_modelInfos.size();     // One hitgroup per model (selection via D3D12_RAYTRACING_INSTANCE_DESC::InstanceContributionToHitGroupIndex in BuildTLAS)
    constexpr size_t missTableRecordCount = 1;

    constexpr size_t rayGenTableStride = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    const size_t hitGroupTableStride = DirectX::AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(HitGroupLocalRootData), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    constexpr size_t missTableStride = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    constexpr size_t rayGenTableSize = rayGenTableRecordCount * rayGenTableStride;
    const size_t hitGroupTableSize = hitGroupTableRecordCount * hitGroupTableStride;
    constexpr size_t missTableSize = missTableRecordCount * missTableStride;

    constexpr size_t rayGenTableOffset = 0;
    const size_t hitGroupTableOffset = rayGenTableOffset + DirectX::AlignUp(rayGenTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    const size_t missTableOffset = hitGroupTableOffset + DirectX::AlignUp(hitGroupTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    const size_t totalTableSize = missTableOffset + missTableSize;

    auto shaderTable = AllocateBuffer(device, totalTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, pipelineInfo->Name.c_str());
    auto shaderTableUploadMem = m_graphicsMemory->Allocate(totalTableSize);

    // Set raygen shader record(s)
    auto rayGenTable = static_cast<uint8_t*>(shaderTableUploadMem.Memory()) + rayGenTableOffset;
    memcpy(rayGenTable, rayGenShaderIdentifier, rayGenTableStride);

    // Set hitgroup shader records
    auto hitGroupTable = static_cast<uint8_t*>(shaderTableUploadMem.Memory()) + hitGroupTableOffset;
    for (size_t modelIndex = 0; modelIndex < m_modelInfos.size(); modelIndex++)
    {
        memcpy(hitGroupTable, hitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        HitGroupLocalRootData hitGroupRootData;
        hitGroupRootData.modelResources = m_modelInfos[modelIndex]->ModelSRVs;
        hitGroupRootData.materialCB.albedo = m_modelInfos[modelIndex]->AlbedoColor;
        memcpy(hitGroupTable + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &hitGroupRootData, sizeof(hitGroupRootData));
        hitGroupTable += hitGroupTableStride;
    }

    // Set miss shader records
    auto missTable = static_cast<uint8_t*>(shaderTableUploadMem.Memory()) + missTableOffset;
    memcpy(missTable, missShaderIdentifier, missTableStride);

    // Copy shader table to GPU resource
    auto commandAllocator = m_deviceResources->GetCommandAllocator();
    auto commandList = m_deviceResources->GetCommandList();
    commandList->Reset(commandAllocator, nullptr);
    commandList->CopyBufferRegion(shaderTable.Get(), 0, shaderTableUploadMem.Resource(), shaderTableUploadMem.ResourceOffset(), totalTableSize);
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(shaderTable.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    // Commit all GPU work and wait for completion
    m_deviceResources->ExecuteCommandList();
    m_deviceResources->WaitForGpu();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    // Store table in pipeline info
    D3D12_GPU_VIRTUAL_ADDRESS baseTableAddress = shaderTable->GetGPUVirtualAddress();

    pipelineInfo->ShaderBindingTable = std::move(shaderTable);
    pipelineInfo->RayGenRecord = { baseTableAddress, rayGenTableSize };
    pipelineInfo->HitGroupTable = { baseTableAddress + hitGroupTableOffset, hitGroupTableSize, hitGroupTableStride };
    pipelineInfo->MissTable = { baseTableAddress + missTableOffset, missTableSize, missTableStride };
}
