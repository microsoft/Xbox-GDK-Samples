//--------------------------------------------------------------------------------------
// BuildYourOwnBundles.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BuildYourOwnBundles.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
#ifndef NDEBUG
    constexpr uint32_t SIZE_4MB = 4 * 1024 * 1024;
#endif

    enum class GPUTimerCounters
    {
        MESH_TIMING = 0,
        NUM_GPU_COUNTERS
    };

    enum class DescriptorHeapEntry
    {
        TextFont,
        ControllerFont,
        MeshTexture,
        TotalDescriptorHeapEntryCount = 128
    };

    enum class DescriptorHeapComputeEntry
    {
        InstanceData,
        RSPacketHeaderData,
        UAVAppend,
        UAVCounter,
        TotalDescriptorHeapComputeEntryCount = 128
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_camPos{},
    m_camLookAt{},
    m_camLookAtDist(0),
    m_camView{},
    m_camProj{},
    m_Near(0),
    m_Far(0),
    m_worldViewProj{},
    m_camFrustum{},
    m_camFrustumMappedCB(nullptr),
    m_gpuDescHandleBYOBCompute{},
    m_modelEffect{},
    m_meshDataMappedCB(nullptr),
    m_modelMeshPart{},
    m_meshInstances{},
    m_numMeshesDrawn(0),
    m_cullMethod(CULL_METHOD::CULL_METHOD_NO_CULL),
    m_drawMethod(DRAW_METHOD::DRAW_METHOD_BYOB_GPU),
    m_byobBufferData{},
    m_byobExecBuffer{},
    m_closeBundleXBuffer{},
    m_gpuBYOBAppendBuffer{},
    m_gpuBYOBCountBufferMapped(nullptr),
    m_gpuBYOBInstanceDataMappedCB(nullptr),
    m_appendBufferDescriptorCPU{},
    m_countBufferDescriptorCPU{},
    m_appendBufferDescriptorGPU{},
    m_countBufferDescriptorGPU{},
    m_gpuExecuteFlag(false),
    m_showInfo(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
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
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

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
    float totalTime = float(timer.GetTotalSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_cullMethod = (m_cullMethod + 1) % CULL_METHOD::CULL_METHOD_COUNT;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_drawMethod = (m_drawMethod + 1) % DRAW_METHOD::DRAW_METHOD_COUNT;
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_drawMethod = (m_drawMethod == 0) ? DRAW_METHOD::DRAW_METHOD_COUNT - 1 : m_drawMethod - 1;
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showInfo = !m_showInfo;
        }

        // Update camera movement
        XMVECTOR forward = XMVector4Normalize(XMVectorSubtract(m_camLookAt, m_camPos));
        XMVECTOR right = XMVector4Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward));
        XMVECTOR up = XMVector4Normalize(XMVector3Cross(forward, right));

        m_camPos = XMVectorAdd(m_camPos, XMVectorAdd(XMVectorScale(forward, pad.thumbSticks.leftY * elapsedTime * 100.f), XMVectorScale(right, pad.thumbSticks.leftX * elapsedTime * 100.f)));

        float rightThumbstickY = pad.thumbSticks.rightY;
        if (((XMVectorGetY(forward) >= 0.98f && rightThumbstickY > 0) ||
            (XMVectorGetY(forward) <= -0.98f && rightThumbstickY < 0)) &&
            (XMVectorGetX(forward) <= 0.01f || XMVectorGetX(forward) >= -0.01f) &&
            (XMVectorGetZ(forward) <= 0.01f || XMVectorGetZ(forward) >= -0.01f))
        {
            // Don't update the right thumbstick Y movement when the camera is pointing almost straight up or straight down
        }
        else
        {
            forward = XMVector3Rotate(forward, XMQuaternionRotationAxis(right, -1 * elapsedTime * rightThumbstickY));
        }
        forward = XMVector3Rotate(forward, XMQuaternionRotationAxis(up, elapsedTime * pad.thumbSticks.rightX));
        m_camLookAt = XMVectorAdd(m_camPos, XMVectorScale(forward, m_camLookAtDist));

        m_camView = XMMatrixLookAtLH(m_camPos, m_camLookAt, up);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_worldViewProj = m_camView * m_camProj;

    // Update camera frustum
    // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    XMMATRIX worldViewProjT = XMMatrixTranspose(m_worldViewProj);
    m_camFrustum.m_plane[0] = XMPlaneNormalize(XMVectorAdd(worldViewProjT.r[3], worldViewProjT.r[0]));            // Left
    m_camFrustum.m_plane[1] = XMPlaneNormalize(XMVectorSubtract(worldViewProjT.r[3], worldViewProjT.r[0]));       // Right
    m_camFrustum.m_plane[2] = XMPlaneNormalize(XMVectorAdd(worldViewProjT.r[3], worldViewProjT.r[1]));            // Bottom
    m_camFrustum.m_plane[3] = XMPlaneNormalize(XMVectorSubtract(worldViewProjT.r[3], worldViewProjT.r[1]));       // Top
    m_camFrustum.m_plane[4] = XMPlaneNormalize(XMVectorAdd(XMVectorZero(), worldViewProjT.r[2]));                 // Near (D3D convention --- near plane is at 0)
    m_camFrustum.m_plane[5] = XMPlaneNormalize(XMVectorSubtract(worldViewProjT.r[3], worldViewProjT.r[2]));       // Far

    // Update all the meshes
    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        Instance& meshInstance = m_meshInstances[instance];
        float scaleFactor = XMVectorGetW(meshInstance.m_translateAndScale);
        XMMATRIX scale = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
        XMMATRIX rotation = XMMatrixRotationAxis(meshInstance.m_rotAxis, meshInstance.m_rotSpeed * totalTime);
        XMMATRIX translation = XMMatrixTranslationFromVector(meshInstance.m_translateAndScale);
        meshInstance.m_modelToWorld = scale * rotation * translation;
        meshInstance.m_boundingSphere = XMVector4Transform(meshInstance.m_boundingSphereCenter, meshInstance.m_modelToWorld);
        meshInstance.m_boundingSphere = XMVectorSetW(meshInstance.m_boundingSphere, scaleFactor * meshInstance.m_boundingSphereRadius);
    }

    m_cpuTimer->Update();
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

    auto graphicsCmdList = m_deviceResources->GetCommandList();
    PIXBeginEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render");

    auto currFrameIndex = m_deviceResources->GetCurrentFrameIndex();

    m_gpuTimer->BeginFrame(graphicsCmdList);
    m_gpuTimer->Start(graphicsCmdList, static_cast<uint32_t>(GPUTimerCounters::MESH_TIMING));
    m_cpuTimer->Start();

    // As the Root Signature is the same for all models, can set it here directly
    graphicsCmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    uint32_t *graphicsRootPacketHeaders = graphicsCmdList->m_GraphicsRootPacketHeader;

    // The topology is the same for all meshes
    graphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Update Constant Buffer
    size_t startRange = sizeof(ModelLoader::ModelEffectConstants) * currFrameIndex * NUM_INSTANCES;

    uint32_t startIndex = currFrameIndex * NUM_INSTANCES;
    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        auto& meshDataCB = m_meshDataMappedCB[startIndex + instance];
        XMMATRIX worldView = XMMatrixMultiply(m_meshInstances[instance].m_modelToWorld, m_camView);
        XMMATRIX worldViewProj = XMMatrixTranspose(XMMatrixMultiply(worldView, m_camProj));
        meshDataCB.worldViewProj = worldViewProj;
        meshDataCB.world = m_meshInstances[instance].m_modelToWorld;
        meshDataCB.eyePosition = m_camPos;

        XMMATRIX worldInverse = XMMatrixInverse(nullptr, m_meshInstances[instance].m_modelToWorld);
        meshDataCB.worldInverseTranspose[0] = worldInverse.r[0];
        meshDataCB.worldInverseTranspose[1] = worldInverse.r[1];
        meshDataCB.worldInverseTranspose[2] = worldInverse.r[2];
    }
    m_numMeshesDrawn = 0;

    PIXBeginEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Draw meshes");
    {
        switch (m_drawMethod)
        {
        case DRAW_METHOD::DRAW_METHOD_DIRECT:
            // Draw the meshes directly using DrawIndexedInstanced
            for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
            {
                // Cull on CPU
                if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
                {
                    continue;
                }
                else
                {
                    ++m_numMeshesDrawn;
                    uint32_t modelID = m_meshInstances[instance].m_modelID;
                    uint32_t psoID = m_meshInstances[instance].m_psoID;

                    // Update the effect state
                    m_modelEffect[modelID]->SetStates(graphicsCmdList, psoID, startRange + instance * sizeof(ModelLoader::ModelEffectConstants));

                    // Draw the mesh
                    ModelDrawHelper::ModelDraw(m_modelMeshPart[modelID], graphicsCmdList);
                }
            }
            break;
        case DRAW_METHOD::DRAW_METHOD_DRAW_BUNDLES:
            DrawBundles(graphicsCmdList, startRange);
            break;
        case DRAW_METHOD::DRAW_METHOD_BYOB_UPDATE_EXECUTE_INSTANCES:
            DrawBYOBUpdateExecuteInstances(graphicsCmdList, startRange);
            break;
        case DRAW_METHOD::DRAW_METHOD_BYOB_UPDATE_INSTANCES_EXECUTE_SINGLE:
            DrawBYOBUpdateInstancesSingleExecute(graphicsCmdList, currFrameIndex, graphicsRootPacketHeaders, startRange);
            break;
        case DRAW_METHOD::DRAW_METHOD_BYOB_RUNTIME_CREATE:
            DrawBYOBRuntimeCreate(graphicsCmdList, currFrameIndex, graphicsRootPacketHeaders, startRange);
            break;
        case DRAW_METHOD::DRAW_METHOD_BYOB_GPU:
            DrawBYOBUsingGPU(graphicsCmdList, currFrameIndex, graphicsRootPacketHeaders);
            break;
        case DRAW_METHOD::DRAW_METHOD_BYOB_CLOSE_BUNDLE_X:
            DrawMeshesUsingCloseBundleX(graphicsCmdList, currFrameIndex, startRange);
            break;
        default: { /* Draw Nothing */ }
        }
    }

    PIXEndEvent(graphicsCmdList);

    // Stop the mesh timer
    m_cpuTimer->Stop();
    m_gpuTimer->Stop(graphicsCmdList, static_cast<uint32_t>(GPUTimerCounters::MESH_TIMING));
    m_gpuTimer->EndFrame(graphicsCmdList);

    RenderUI(graphicsCmdList);

    PIXEndEvent(graphicsCmdList);

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

// Draw using regular Bundles
void Sample::DrawBundles(ID3D12GraphicsCommandList* graphicsCmdList, size_t startRange)
{
    uint32_t currPSOID = NUM_PSO;

    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Cull on CPU
        if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
        {
            continue;
        }
        else
        {
            ++m_numMeshesDrawn;
            bool patchPSO = false;
            if (currPSOID != m_meshInstances[instance].m_psoID)
            {
                // Set PSO
                currPSOID = m_meshInstances[instance].m_psoID;
                patchPSO = true;
            }
            SetStatesBeforeDraw(graphicsCmdList, patchPSO, instance, startRange + instance * sizeof(ModelLoader::ModelEffectConstants));
            graphicsCmdList->ExecuteBundle(m_bundleDirectCmdList[m_meshInstances[instance].m_modelID].Get());
        }
    }
}

// Set states using GPU packets and execute each instance separately using ExecuteIndirectBundleX
void Sample::DrawBYOBUpdateExecuteInstances(ID3D12GraphicsCommandList* graphicsCmdList, size_t startRange)
{
    uint32_t currPSOID = NUM_PSO;

    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Cull on CPU
        if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
        {
            continue;
        }
        else
        {
            ++m_numMeshesDrawn;
            bool patchPSO = false;
            if (currPSOID != m_meshInstances[instance].m_psoID)
            {
                // Set PSO
                currPSOID = m_meshInstances[instance].m_psoID;
                patchPSO = true;
            }
            SetStatesBeforeDraw(graphicsCmdList, patchPSO, instance, startRange + instance * sizeof(ModelLoader::ModelEffectConstants));
            uint32_t bufferExecutableSize = static_cast<uint32_t>(m_byobBufferData[m_meshInstances[instance].m_modelID].m_byobBufferCurr - m_byobBufferData[m_meshInstances[instance].m_modelID].m_byobBufferStart);
            graphicsCmdList->ExecuteIndirectBundleX(m_byobBufferData[m_meshInstances[instance].m_modelID].m_byobExecutableBuffer.Get(), 0, bufferExecutableSize, nullptr, 0, 0);
        }
    }

    graphicsCmdList->FlushPipelineX(D3D12XBOX_FLUSH_TOP_MASK | D3D12XBOX_FLUSH_BOP_MASK, m_deviceResources->GetRenderTarget()->GetGPUVirtualAddress(), 35651584);
}

// Update recorded BYOB bundle data with state setting data and then send a single execute buffer to the GPU using ExecuteIndirectBundleX
void Sample::DrawBYOBUpdateInstancesSingleExecute(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders, size_t startRange)
{
    uint32_t currPSOID = NUM_PSO;
    uint32_t currModelID = NUM_MODELS;

    // Reset the buffer pointer to point to the start
    UINT8* startAddress = m_byobExecBuffer.m_byobBufferStart + MAX_PER_DRAW_SIZE * NUM_INSTANCES * currFrameIndex;
    m_byobExecBuffer.m_byobBufferCurr = startAddress;

    // Size to copy from recorded bundle
    uint32_t bundleSizeToCopy[NUM_MODELS];
    for (uint32_t modelID = 0; modelID < NUM_MODELS; ++modelID)
    {
        bundleSizeToCopy[modelID] = static_cast<uint32_t>(m_byobBufferData[modelID].m_byobBufferCurr - m_byobBufferData[modelID].m_byobBufferStart);
    }

    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Cull on CPU
        if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
        {
            continue;
        }
        else
        {
            ++m_numMeshesDrawn;
            bool patchPSO = false;
            bool setTexture = false;
            if (currPSOID != m_meshInstances[instance].m_psoID)
            {
                // Set PSO
                currPSOID = m_meshInstances[instance].m_psoID;
                patchPSO = true;
            }
            if (currModelID != m_meshInstances[instance].m_modelID)
            {
                // Set texture
                currModelID = m_meshInstances[instance].m_modelID;
                setTexture = true;
            }
            if(currModelID >= NUM_MODELS)
            {
                // Invalid model ID
                continue;
            }
            SetStatesBYOB(reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr), patchPSO, setTexture,
                graphicsRootPacketHeaders, instance, startRange + instance * sizeof(ModelLoader::ModelEffectConstants));

            // Copy the data from the recorded buffer to the buffer that will be executed
            memcpy(m_byobExecBuffer.m_byobBufferCurr, m_byobBufferData[currModelID].m_byobBufferStart, bundleSizeToCopy[currModelID]);
            m_byobExecBuffer.m_byobBufferCurr += bundleSizeToCopy[currModelID];
        }
    }

    uint32_t bufferExecutableSize = static_cast<uint32_t>(m_byobExecBuffer.m_byobBufferCurr - startAddress);

    // The size of commands to execute shouldn't exceed 4MB limit
    assert(bufferExecutableSize <= SIZE_4MB);

    // Don't execute the bundle if there's nothing to execute
    if (bufferExecutableSize > 0)
    {
        uint32_t bufferOffset = static_cast<uint32_t>(startAddress - m_byobExecBuffer.m_byobBufferStart);
        graphicsCmdList->ExecuteIndirectBundleX(m_byobExecBuffer.m_byobExecutableBuffer.Get(), bufferOffset, bufferExecutableSize, nullptr, 0, 0);
    }
}

// Build the entire buffer at runtime and then send the buffer in a single ExecuteIndirectBundleX command
void Sample::DrawBYOBRuntimeCreate(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders, size_t startRange)
{
    uint32_t currPSOID = NUM_PSO;
    uint32_t currModelID = NUM_MODELS;

    // Assuming TriangleList for all meshes, so not calling the API multiple times for different models
    graphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Reset the buffer pointer to point to the start
    UINT8* startAddress = m_byobExecBuffer.m_byobBufferStart + MAX_PER_DRAW_SIZE * NUM_INSTANCES * currFrameIndex;
    m_byobExecBuffer.m_byobBufferCurr = startAddress;

    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Cull on CPU
        if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
        {
            continue;
        }
        else
        {
            ++m_numMeshesDrawn;
            if (currPSOID != m_meshInstances[instance].m_psoID)
            {
                // Set PSO
                currPSOID = m_meshInstances[instance].m_psoID;
                m_modelEffect[m_meshInstances[instance].m_modelID]->SetPSOBYOB(reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr), currPSOID);
            }
            if (currModelID != m_meshInstances[instance].m_modelID)
            {
                // Set Vertex and Index buffers
                currModelID = m_meshInstances[instance].m_modelID;
                ModelDrawHelper::ModelSetVertexIndexBuffersBYOB(m_modelMeshPart[currModelID], reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr));

                // Set texture
                m_modelEffect[currModelID]->SetRootDescriptorTableBYOB(reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr), graphicsRootPacketHeaders);
            }

            if(currModelID >= NUM_MODELS)
            {
                // Invalid model ID
                continue;
            }
            // Update constants
            m_modelEffect[currModelID]->UpdateConstantsBYOB(
                reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr),
                graphicsRootPacketHeaders,
                startRange + instance * sizeof(ModelLoader::ModelEffectConstants));

            // Constant buffer and draw data
            ModelDrawHelper::ModelBuildOwnBundle(m_modelMeshPart[currModelID],
                reinterpret_cast<uint32_t**>(&m_byobExecBuffer.m_byobBufferCurr),
                false);

            // The address should never go pass the allocated size
            assert(m_byobExecBuffer.m_byobBufferCurr < m_byobExecBuffer.m_byobBufferEnd);
        }
    }
    uint32_t bufferExecutableSize = static_cast<uint32_t>(m_byobExecBuffer.m_byobBufferCurr - startAddress);

    // The size of commands to execute shouldn't exceed 4MB limit
    assert(bufferExecutableSize <= SIZE_4MB);

    // Don't execute the bundle if there's nothing to execute
    if (bufferExecutableSize > 0)
    {
        uint32_t bufferOffset = static_cast<uint32_t>(startAddress - m_byobExecBuffer.m_byobBufferStart);
        graphicsCmdList->ExecuteIndirectBundleX(m_byobExecBuffer.m_byobExecutableBuffer.Get(), bufferOffset, bufferExecutableSize, nullptr, 0, 0);
    }
}

// Draw the meshes by writing GPU packet data into an executable buffer using the GPU
void Sample::DrawBYOBUsingGPU(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders)
{
    // Initialize Count to 0
    uint32_t iClearValues[4] = { };
    graphicsCmdList->ClearUnorderedAccessViewUint(
        m_countBufferDescriptorGPU,
        m_countBufferDescriptorCPU,
        m_gpuBYOBCountBuffer.Get(),
        iClearValues,
        0U,
        nullptr);

    if (!m_gpuExecuteFlag)
    {
        // Fill in the data for Root Signature Packet Headers
        // This is available only after setting the Root Signature
        // Setting this just once as the Root Signature doesn't change in this sample
        uint32_t* rsPacketHeaderValue;
        DX::ThrowIfFailed(m_gpuBYOBRSHeaderData->Map(0, nullptr, reinterpret_cast<void**>(&rsPacketHeaderValue)));
        for (uint32_t packetHeader = 0; packetHeader < static_cast<uint32_t>(ModelLoader::RootParameterIndex::RootParameterCount); ++packetHeader)
        {
            rsPacketHeaderValue[packetHeader] = graphicsRootPacketHeaders[packetHeader];
        }
        m_gpuBYOBRSHeaderData->Unmap(0, nullptr);
    }

    // Update camera frustum data
    size_t startRangeCamFrustumData = sizeof(Frustum) * currFrameIndex;
    uint32_t startIndexCamFrustum = currFrameIndex;
    for (int planeID = 0; planeID < 6; ++planeID)
    {
        m_camFrustumMappedCB[startIndexCamFrustum].m_plane[planeID] = m_camFrustum.m_plane[planeID];
    }

    // Update instance buffer
    uint32_t startIndexInstance = currFrameIndex * NUM_INSTANCES;
    for (int instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Update parameters which might have changed
        m_gpuBYOBInstanceDataMappedCB[startIndexInstance + instance].m_boundingSphereCenterAndRadius = m_meshInstances[instance].m_boundingSphere;
    }

    size_t startRangeModelPerFrameData = sizeof(ModelDataUpdatePerFrame) * currFrameIndex;

    graphicsCmdList->SetComputeRootSignature(m_rootSignature.Get());
    if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL)
    {
        graphicsCmdList->SetPipelineState(m_gpuBYOBComputeCullPSO.Get());
    }
    else
    {
        graphicsCmdList->SetPipelineState(m_gpuBYOBComputePSO.Get());
    }
    graphicsCmdList->SetComputeRootConstantBufferView(static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer0), m_camFrustumCB->GetGPUVirtualAddress() + startRangeCamFrustumData);
    graphicsCmdList->SetComputeRootConstantBufferView(static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer1), m_gpuBYOBModelDataCB->GetGPUVirtualAddress());
    graphicsCmdList->SetComputeRootConstantBufferView(static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer2), m_gpuBYOBModelPerFrameDataCB->GetGPUVirtualAddress() + startRangeModelPerFrameData);

    // Set UAV
    graphicsCmdList->SetComputeRootDescriptorTable(static_cast<uint32_t>(ModelLoader::RootParameterIndex::DescriptorTable0), m_gpuDescHandleBYOBCompute);
    graphicsCmdList->Dispatch(NUM_INSTANCES / 64, 1, 1); // 64 threads in a single invocation

    // Make sure that the dispatch is complete and the buffer that has the indirect arguments is available
    CD3DX12_RESOURCE_BARRIER appendBarrierUAVToIndirect = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBAppendBuffer.m_byobExecutableBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    graphicsCmdList->ResourceBarrier(1, &appendBarrierUAVToIndirect);

    CD3DX12_RESOURCE_BARRIER countBarrierUAVToIndirect = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    graphicsCmdList->ResourceBarrier(1, &countBarrierUAVToIndirect);

    // Draw the meshes using the data written by the GPU
    // Set a max executable buffer size on the CPU. THe buffer size is available
    // from the GPU (written to m_gpuBYOBCountBuffer at an offset of sizeof(uint64_t))
    // But we need to pass in a max size on the CPU. If the size passed in using the buffer is larger
    // than that from the CPU, the contents of the buffer are not executed
    uint32_t bufferExecutableSize = static_cast<uint32_t>(SIZEOF_GPU_APPEND_BUFFER_STRUCT * NUM_INSTANCES);

    // The size of commands to execute shouldn't exceed 4MB limit
    assert(bufferExecutableSize <= SIZE_4MB);

    // Don't execute the bundle if there's nothing to execute
    if (m_gpuExecuteFlag && bufferExecutableSize > 0)
    {
        uint32_t bufferOffset = 0;
        graphicsCmdList->ExecuteIndirectBundleX(m_gpuBYOBAppendBuffer.m_byobExecutableBuffer.Get(), bufferOffset, bufferExecutableSize, m_gpuBYOBCountBuffer.Get(), sizeof(uint64_t), 0);
    }
    else
    {
        m_gpuExecuteFlag = true;
    }

    CD3DX12_RESOURCE_BARRIER appendBarrierIndirectToUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBAppendBuffer.m_byobExecutableBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    graphicsCmdList->ResourceBarrier(1, &appendBarrierIndirectToUAV);
    CD3DX12_RESOURCE_BARRIER countBarrierIndirectToUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBCountBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    graphicsCmdList->ResourceBarrier(1, &countBarrierIndirectToUAV);
}

// Draw Meshes using the CloseBundleX API
// This allows you to free up the bundle command list after recording the commands. The commands can be recorded into a buffer
// using CloseBundleX and then the buffer can be executed anytime later using ExecuteIndirectBundleX.
// GetExecutionCommandSizeX gives the size of the recorded commands in the bundle.
void Sample::DrawMeshesUsingCloseBundleX(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, size_t startRange)
{
    uint32_t bufferOffset = MAX_PER_DRAW_SIZE_CLOSE_BUNDLE_X * NUM_INSTANCES * currFrameIndex;
    UINT8* startAddress = m_closeBundleXBuffer.m_byobBufferStart + bufferOffset;
    m_closeBundleXBuffer.m_byobBufferCurr = startAddress;

    // Root Signature is the same, so set just once
    m_modelEffect[m_meshInstances[0].m_modelID]->SetRootSignature(m_bundleCloseBundleXCmdList.Get());

    uint32_t currPSOID = NUM_PSO;
    for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        // Cull on CPU
        if (m_cullMethod == CULL_METHOD::CULL_METHOD_CULL && IsSphereOutsideFrustum(m_camFrustum, m_meshInstances[instance].m_boundingSphere))
        {
            continue;
        }
        else
        {
            ++m_numMeshesDrawn;
            uint32_t modelID = m_meshInstances[instance].m_modelID;
            uint32_t psoID = m_meshInstances[instance].m_psoID;
            if (currPSOID != psoID)
            {
                // Set PSO
                currPSOID = psoID;
                m_modelEffect[modelID]->SetPSO(m_bundleCloseBundleXCmdList.Get(), psoID);
            }

            m_modelEffect[modelID]->SetDescriptorHeaps(m_bundleCloseBundleXCmdList.Get(), startRange + instance * sizeof(ModelLoader::ModelEffectConstants));

            // Draw the mesh
            ModelDrawHelper::ModelDraw(m_modelMeshPart[modelID], m_bundleCloseBundleXCmdList.Get());
        }
    }

    // Get the size of the commands recorded into the command list
    uint32_t allocationSize = m_bundleCloseBundleXCmdList->GetExecutionCommandSizeX();

    // The size of commands to execute shouldn't exceed 4MB limit
    assert(allocationSize <= SIZE_4MB);

    // Record the commands into a buffer
    m_bundleCloseBundleXCmdList->CloseBundleX(reinterpret_cast<void**>(&startAddress), allocationSize);

    // The command list can now be reset
    m_bundleCloseBundleXCmdList->Reset(m_bundleCloseBundleXCmdAllocator.Get(), nullptr);

    // Execute the recorded commands
    graphicsCmdList->ExecuteIndirectBundleX(m_closeBundleXBuffer.m_byobExecutableBuffer.Get(), bufferOffset, allocationSize, nullptr, 0, 0);
}

// Render UI
void Sample::RenderUI(ID3D12GraphicsCommandList * graphicsCmdList)
{
    PIXBeginEvent(graphicsCmdList, 1, L"Draw SpriteFont");
    {
        XMVECTOR textPosition = XMVectorSet(50, 50, 0, 1);
        XMVECTOR textColor = XMVectorSet(1, 1, 1, 1);
        m_fontBatch->Begin(graphicsCmdList);

        m_fontText->DrawString(m_fontBatch.get(), L"Build Your Own Bundles (BYOB) Sample", textPosition, textColor);

        float diffInY = (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD) ? 50.0f : 35.0f;
        float diffInX = (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD) ? 300.0f : 200.0f;

        double timeDiff = m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(GPUTimerCounters::MESH_TIMING));
        double timeDiffCPU = m_cpuTimer->GetElapsedMS();

        wchar_t buf[2048] = {};
        swprintf_s(buf, _countof(buf) - 1, L"GPU: %.2f ms", timeDiff);
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), buf, textPosition, textColor);

        swprintf_s(buf, _countof(buf) - 1, L"CPU:  %.2f ms", timeDiffCPU);
        XMVECTOR textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(diffInX, 0, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), buf, textPositionMoveX, textColor);

        // Read the value from the GPU buffer to output the number of meshes culled
        if (m_drawMethod == static_cast<uint32_t>(DRAW_METHOD::DRAW_METHOD_BYOB_GPU))
        {
            CD3DX12_RESOURCE_BARRIER countBarrierUAVToCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
            graphicsCmdList->ResourceBarrier(1, &countBarrierUAVToCopySrc);

            // Read the count to know exactly how many bytes of data
            // were written by the compute shader when building the bundle
            m_numMeshesDrawn = m_gpuBYOBCountBufferMapped[0];

            CD3DX12_RESOURCE_BARRIER countBarrierCopySrcToUAV = CD3DX12_RESOURCE_BARRIER::Transition(m_gpuBYOBCountBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            graphicsCmdList->ResourceBarrier(1, &countBarrierCopySrcToUAV);
        }
        swprintf_s(buf, _countof(buf) - 1, L"Number of meshes drawn: %d", m_numMeshesDrawn);
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), buf, textPosition, textColor);

        swprintf_s(buf, _countof(buf) - 1, L"Total number of meshes: %d", NUM_INSTANCES);
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), buf, textPosition, textColor);

        static const wchar_t* cullMethodNames[] =
        {
            L"No Cull",
            L"Cull meshes",
        };
        static_assert(static_cast<uint32_t>(CULL_METHOD::CULL_METHOD_COUNT) == _countof(cullMethodNames), "Array size mismatch");

        static const wchar_t* drawMethodNames[] =
        {
            L"Build your own bundles - GPU",
            L"Build your own bundles - Update and execute for each instance",
            L"Build your own bundles - Update all instances and execute once",
            L"Build your own bundles - Single Buffer",
            L"Draw using CloseBundleX",
            L"Bundles",
            L"Direct draw",
        };
        static_assert(static_cast<uint32_t>(DRAW_METHOD::DRAW_METHOD_COUNT) == _countof(drawMethodNames), "Array size mismatch");

        static const wchar_t* drawMethodInfo[] =
        {
            L"Draw the meshes by writing GPU packet data into an executable buffer using the GPU",
            L"Set states using GPU packets and execute each instance separately using ExecuteIndirectBundleX",
            L"Update recorded BYOB bundle data with state setting data and then send a single execute buffer to the GPU using ExecuteIndirectBundleX",
            L"Build the entire buffer at runtime and then send the buffer in a single ExecuteIndirectBundleX command",
            L"Shows usage of CloseBundleX to free up a command list by storing commands in a buffer and then executing it using ExecuteIndirectBundleX",
            L"Draw using regular Bundles",
            L"Draw the meshes directly using DrawIndexedInstanced",
        };
        static_assert(DRAW_METHOD::DRAW_METHOD_COUNT == _countof(drawMethodInfo), "Array size mismatch");

        swprintf_s(buf, _countof(buf) - 1, L"[A] or [B] - Draw Method: %s", drawMethodNames[m_drawMethod]);
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        if (m_showInfo)
            swprintf_s(buf, _countof(buf) - 1, L"[Y] - Less info");
        else
            swprintf_s(buf, _countof(buf) - 1, L"[Y] - Show more info about Draw Method");
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        if (m_showInfo)
        {
            swprintf_s(buf, _countof(buf) - 1, L"%s", drawMethodInfo[m_drawMethod]);
            textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
            DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);
        }

        swprintf_s(buf, _countof(buf) - 1, L"[X] - Culling Method: %s", cullMethodNames[m_cullMethod]);
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        swprintf_s(buf, _countof(buf) - 1, L"[LThumb] and [RThumb] - Move Camera");
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, 2.0f * diffInY, 0, 0));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        swprintf_s(buf, _countof(buf) - 1, L"[View] - Exit Sample");
        textPosition = XMVectorAdd(textPosition, XMVectorSet(0, diffInY, 0, 0));
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), buf, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        // End font batch
        m_fontBatch->End();
    }
    PIXEndEvent(graphicsCmdList);
}

// Return true if sphere is completely outside the frustum, else return false
bool Sample::IsSphereOutsideFrustum(Frustum& frustum, XMVECTOR& sphere)
{
    for (size_t i = 0; i < _countof(frustum.m_plane); ++i)
    {
        XMVECTOR dotProduct = XMPlaneDotCoord(frustum.m_plane[i], sphere);
        if (XMVectorGetX(dotProduct) < -XMVectorGetW(sphere))
        {
            return true;
        }
    }
    return false;
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

    // Timers
    m_cpuTimer = std::make_unique<DX::CPUTimer>();
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    InitializeRootSignature(device);
    LoadResources(device);

    CreateDirectBundle(device);
    CreateCloseBundleXCommandListAndBuffers(device);
    CreateBYOB(device);

    // For building up bundles on GPU
    CreateComputePSO(device);
    CreateBuffersForGPUBundles(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    InitializeCamera();
}

// Initialize Root Signature
void Sample::InitializeRootSignature(ID3D12Device* device)
{
    // On Xbox One the D3D12_ROOT_SIGNATURE_FLAG_DENY_* flags don't do anything as the
    // root signature is always set for all the shader stages
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    CD3DX12_STATIC_SAMPLER_DESC sampler(0);
    CD3DX12_DESCRIPTOR_RANGE descriptorRanges[static_cast<uint32_t>(ModelLoader::DescriptorIndex::DescriptorCount)] = {};
    descriptorRanges[static_cast<uint32_t>(ModelLoader::DescriptorIndex::Texture)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    descriptorRanges[static_cast<uint32_t>(ModelLoader::DescriptorIndex::Buffer)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    descriptorRanges[static_cast<uint32_t>(ModelLoader::DescriptorIndex::UAV)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);
    CD3DX12_ROOT_PARAMETER rootParameters[static_cast<uint32_t>(ModelLoader::RootParameterIndex::RootParameterCount)] = {};
    rootParameters[static_cast<uint32_t>(ModelLoader::RootParameterIndex::DescriptorTable0)].InitAsDescriptorTable(
        _countof(descriptorRanges),
        descriptorRanges);
    rootParameters[static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer0)].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer1)].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer2)].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
    rsigDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

    DX::ThrowIfFailed(CreateRootSignature(device, &rsigDesc, &m_rootSignature));
    m_rootSignature->SetName(L"Main Root Signature");
}

// Initialize camera and create constant buffer with camera data.
// The buffer is used to cull meshes using the GPU
void Sample::InitializeCamera()
{
    m_camPos = XMVectorSet(0.0f, 0.0f, -250.0f, 0);
    m_camLookAt = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
    m_camLookAtDist = XMVectorGetX(XMVector4Length(XMVectorSubtract(m_camLookAt, m_camPos)));
    m_Near = 2.0f;
    m_Far = 1500.0f;

    // World-View-Projection matrix
    m_camView = XMMatrixLookAtLH(m_camPos, m_camLookAt, XMVectorSet(0, 1, 0, 0));
    m_camProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_deviceResources->GetScreenViewport().Width / m_deviceResources->GetScreenViewport().Height, m_Near, m_Far);
    m_worldViewProj = m_camView * m_camProj;

    auto device = m_deviceResources->GetD3DDevice();

    // Buffer to pass camera frustum data to the GPU - used when culling meshes on the GPU
    D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC cbCamFrustumDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(Frustum) * m_deviceResources->GetBackBufferCount()           // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProp, 									                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbCamFrustumDesc,										            // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_camFrustumCB.ReleaseAndGetAddressOf())      // REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_camFrustumCB->SetName(L"Camera Frustum Buffer");

    // Keeping the camera frustum constant buffer mapped as it has to be updated each frame
    DX::ThrowIfFailed(m_camFrustumCB->Map(0, nullptr, reinterpret_cast<void**>(&m_camFrustumMappedCB)));
}

// Load resources using DirectXTK
void Sample::LoadResources(ID3D12Device * device)
{
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create a texture factory for loading the model textures
    m_textures = std::make_unique<EffectTextureFactory>(device, resourceUpload, (size_t)DescriptorHeapEntry::TotalDescriptorHeapEntryCount);

    // Load the assets
    LoadMeshes(device, rtState, resourceUpload);
    InitializeSpriteFonts(device, rtState, resourceUpload);

    auto resourceUploadEvent = resourceUpload.End(m_deviceResources->GetCommandQueue());
    resourceUploadEvent.wait(); // wait for resources to upload
}

// Load the meshes
void Sample::LoadMeshes(ID3D12Device * device, const RenderTargetState& rtState, ResourceUploadBatch& resourceUpload)
{
    EffectPipelineStateDescription psd(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        rtState);

    // Load Shaders
    std::vector<uint8_t> meshVSBlob = DX::ReadData(L"MeshVS.cso");
    std::vector<uint8_t> meshPSBlob_0 = DX::ReadData(L"MeshPS_0.cso");
    std::vector<uint8_t> meshPSBlob_1 = DX::ReadData(L"MeshPS_1.cso");
    std::vector<uint8_t> meshPSBlob_2 = DX::ReadData(L"MeshPS_2.cso");
    D3D12_SHADER_BYTECODE vsByteCode[NUM_PSO] = {
        { meshVSBlob.data(), meshVSBlob.size() },
        { meshVSBlob.data(), meshVSBlob.size() },
        { meshVSBlob.data(), meshVSBlob.size() },
    };
    D3D12_SHADER_BYTECODE psByteCode[NUM_PSO] = {
        { meshPSBlob_0.data(), meshPSBlob_0.size() },
        { meshPSBlob_1.data(), meshPSBlob_1.size() },
        { meshPSBlob_2.data(), meshPSBlob_2.size() },
    };

    // Create Constant Buffer
    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(ModelLoader::ModelEffectConstants) * NUM_INSTANCES *
        m_deviceResources->GetBackBufferCount()                             // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0 )

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProp, 											                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbDesc,										                    // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_meshDataCB.ReleaseAndGetAddressOf())	    // REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_meshDataCB->SetName(L"Mesh Data Constant Buffer");

    static const wchar_t modelNames[NUM_MODELS][50] = {
        L"MazeSphere.sdkmesh",
        L"MazeCube.sdkmesh",
        L"MazeTrapezoid.sdkmesh",
    };

    // Create the shared root signature for the model
    ModelLoader::ModelEffect::CreateRootSignature(device, m_rootSignature.ReleaseAndGetAddressOf());

    // Load the models
    for (uint32_t modelID = 0; modelID < NUM_MODELS; ++modelID)
    {
        // Load the model
        m_model[modelID] = Model::CreateFromSDKMESH(device, modelNames[modelID]);

        // This sample assumes that there's only a single mesh in the model
        // and a single part in the mesh.
        assert(m_model[modelID]->meshes.size() == 1);
        assert(m_model[modelID]->meshes[0]->opaqueMeshParts.size() == 1);

        m_modelMeshPart[modelID] = m_model[modelID]->meshes[0]->opaqueMeshParts[0].get();

        // Load the model resources
        m_model[modelID]->LoadTextures(*m_textures.get(), int(DescriptorHeapEntry::MeshTexture) + int(modelID));

        // Optimize the mesh rendering
        m_model[modelID]->LoadStaticBuffers(device, resourceUpload);

        // Create the effect data for each model
        D3D12_INPUT_LAYOUT_DESC inputLayout =
        {
            m_modelMeshPart[modelID]->vbDecl->data(),
            (UINT)m_modelMeshPart[modelID]->vbDecl->size()
        };

        EffectPipelineStateDescription meshPSD = psd;
        meshPSD.inputLayout = inputLayout;

        m_modelEffect[modelID] = new ModelLoader::ModelEffect(
            device,
            meshPSD,
            vsByteCode,
            psByteCode,
            m_rootSignature.Get(),
            m_meshDataCB.Get());

        const auto& materialInfo = m_model[modelID]->materials[m_modelMeshPart[modelID]->materialIndex];
        m_modelEffect[modelID]->CreateDefaultLighting(materialInfo);
        m_modelEffect[modelID]->SetTexture(m_textures->GetGpuDescriptorHandle((size_t)DescriptorHeapEntry::MeshTexture + modelID));
    }

    CreateMeshInstances();

    // Get data for each instance into a buffer. Used to update orientation for each mesh instance
    // Keeping the buffer mapped to update it each frame
    DX::ThrowIfFailed(m_meshDataCB->Map(0, nullptr, reinterpret_cast<void**>(&m_meshDataMappedCB)));
    for (uint32_t frameIndex = 0; frameIndex < m_deviceResources->GetBackBufferCount(); ++frameIndex)
    {
        uint32_t startIndex = frameIndex * NUM_INSTANCES;
        for (uint32_t instance = 0; instance < NUM_INSTANCES; ++instance)
        {
            memcpy(&(m_meshDataMappedCB[startIndex + instance]), &(m_modelEffect[m_meshInstances[instance].m_modelID]->m_modelEffectConstants), sizeof(ModelLoader::ModelEffectConstants));
        }
    }
}

// Create multiple instances of each model
void Sample::CreateMeshInstances()
{
    XMVECTOR meshExtent[NUM_MODELS];
    XMVECTOR meshCenter[NUM_MODELS];
    float meshRadius[NUM_MODELS];

    for (int i = 0; i < NUM_MODELS; ++i)
    {
        BoundingSphere boundingSphere = m_model[i]->meshes[0]->boundingSphere;
        XMFLOAT3 boundSphereCenter = boundingSphere.Center;
        meshExtent[i] = XMVectorSet(boundSphereCenter.x, boundSphereCenter.y, boundSphereCenter.z, boundingSphere.Radius);
        meshCenter[i] = XMVectorSet(boundSphereCenter.x, boundSphereCenter.y, boundSphereCenter.z, 1.0f);
        meshRadius[i] = boundingSphere.Radius;
    }

    auto RandFloat0ToF = [](float f) -> float { return (f * rand()) / RAND_MAX; };
    auto RandFloatNegFToF = [RandFloat0ToF](float f) -> float { return RandFloat0ToF(2.0f * f) - f; };

    for (int instance = 0; instance < NUM_INSTANCES; ++instance)
    {
        float radius = RandFloat0ToF(0.1f) + 0.01f;
        uint32_t modelID = static_cast<uint32_t>(roundf(RandFloat0ToF(NUM_MODELS - 1)));
        m_meshInstances[instance].m_modelID = modelID;
        m_meshInstances[instance].m_psoID = static_cast<uint32_t>(round(RandFloat0ToF(NUM_PSO - 1)));
        m_meshInstances[instance].m_translateAndScale = XMVectorSet(RandFloatNegFToF(300.0f), RandFloatNegFToF(200.0f), RandFloatNegFToF(350.0f), radius);
        m_meshInstances[instance].m_rotAxis = XMVectorSet(RandFloatNegFToF(1.0f), RandFloatNegFToF(1.0f), RandFloatNegFToF(1.0f), 0.0f);
        m_meshInstances[instance].m_rotSpeed = RandFloat0ToF(3.0f);
        m_meshInstances[instance].m_boundingSphere = meshExtent[modelID];
        m_meshInstances[instance].m_boundingSphereCenter = meshCenter[modelID];
        m_meshInstances[instance].m_boundingSphereRadius = meshRadius[modelID];
    }
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* device, const RenderTargetState& rtState, ResourceUploadBatch& resourceUpload)
{
    SpriteBatchPipelineStateDescription pd(
        rtState,
        &CommonStates::AlphaBlend);

    auto const viewport = m_deviceResources->GetScreenViewport();
    m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd, &viewport);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandleText = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandleText = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_fontText = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD) ? L"SegoeUI_30.spritefont" : L"SegoeUI_18.spritefont",
        cpuDescHandleText,
        gpuDescHandleText);

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandleController = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandleController = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_fontController = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        L"XboxOneController.spritefont",
        cpuDescHandleController,
        gpuDescHandleController);
}

// Create a direct bundle
void Sample::CreateDirectBundle(ID3D12Device* device)
{
    for (uint32_t i = 0; i < NUM_MODELS; ++i)
    {
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_GRAPHICS_PPV_ARGS(m_bundleDirectCmdAllocator[i].ReleaseAndGetAddressOf()));
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleDirectCmdAllocator[i].Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_bundleDirectCmdList[i].ReleaseAndGetAddressOf()));

        ModelDrawHelper::ModelDraw(m_modelMeshPart[i], m_bundleDirectCmdList[i].Get());

        m_bundleDirectCmdList[i]->Close();
    }
}

// Create command list for demonstrating CloseBundleX
void Sample::CreateCloseBundleXCommandListAndBuffers(ID3D12Device* device)
{
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_GRAPHICS_PPV_ARGS(m_bundleCloseBundleXCmdAllocator.ReleaseAndGetAddressOf()));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleCloseBundleXCmdAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_bundleCloseBundleXCmdList.ReleaseAndGetAddressOf()));

    // Create buffer to pass into ExecuteIndirectBundleX after using CloseBundleX
    {
        constexpr uint32_t c_bundleRootSignature = 16;
        constexpr uint32_t c_bundleInitialize = 36;
        constexpr uint32_t c_sizeInitAndCalledOnce = c_bundleInitialize + c_bundleRootSignature;
        uint64_t bufferSize = (MAX_PER_DRAW_SIZE_CLOSE_BUNDLE_X * NUM_INSTANCES + c_sizeInitAndCalledOnce) * m_deviceResources->GetBackBufferCount();

        // Aligning buffer to 4096 to align with the page size
        bufferSize = AlignUp(bufferSize, 4096);

        D3D12_RESOURCE_DESC closeBundleXBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            bufferSize,                                                 // UINT64 width,
            D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER				// D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                        // UINT64 alignment = 0
        );

        DWORD flAllocation = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
        DWORD flXMemAllocationFlags = XMEM_GRAPHICS;
        DWORD flProtect = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_EXECUTE_READ;
        D3D12_GPU_VIRTUAL_ADDRESS closeBundleXExecuteAddress = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, bufferSize, flAllocation, flXMemAllocationFlags, flProtect));
        assert(closeBundleXExecuteAddress);

        DX::ThrowIfFailed(device->CreatePlacedResourceX(closeBundleXExecuteAddress,
            &closeBundleXBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_closeBundleXBuffer.m_byobExecutableBuffer.ReleaseAndGetAddressOf())));
        m_closeBundleXBuffer.m_byobExecutableBuffer->SetName(L"Buffer for CloseBundleX");

        DX::ThrowIfFailed(m_closeBundleXBuffer.m_byobExecutableBuffer->Map(0, nullptr, reinterpret_cast<void**>(&(m_closeBundleXBuffer.m_byobBufferStart))));
        m_closeBundleXBuffer.m_byobBufferEnd = m_closeBundleXBuffer.m_byobBufferStart + bufferSize;
        m_closeBundleXBuffer.m_byobBufferCurr = m_closeBundleXBuffer.m_byobBufferStart;
    }
}

// Create a bundle using BYOB
void Sample::CreateBYOB(ID3D12Device* device)
{
    for (uint32_t i = 0; i < NUM_MODELS; ++i)
    {
        // Create executable buffer
        uint64_t bufferSize = MAX_PER_DRAW_SIZE * m_deviceResources->GetBackBufferCount();
        // BYOB buffer should be 16 aligned
        bufferSize = AlignUp(bufferSize, 16);

        // Aligning buffer to 4096 to align with the page size
        bufferSize = AlignUp(bufferSize, 4096);

        D3D12_RESOURCE_DESC byobExecBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            bufferSize,                                                     // UINT64 width,
            D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER					// D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
        );	        												        // UINT64 alignment = 0

        D3D12_GPU_VIRTUAL_ADDRESS byobExecuteAddress =
            reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr,
                bufferSize,
                MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT,
                XMEM_GRAPHICS,
                PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_EXECUTE_READ));
        assert(byobExecuteAddress);

        DX::ThrowIfFailed(device->CreatePlacedResourceX(byobExecuteAddress,
            &byobExecBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_byobBufferData[i].m_byobExecutableBuffer.ReleaseAndGetAddressOf())));
        wchar_t buf[32] = {};
        swprintf_s(buf, _countof(buf) - 1, L"Executable Buffer for Model %d", i);
        m_byobBufferData[i].m_byobExecutableBuffer->SetName(buf);

        // Using null range as the entire range is always mapped on Xbox One
        DX::ThrowIfFailed(m_byobBufferData[i].m_byobExecutableBuffer->Map(0, nullptr, reinterpret_cast<void**>(&(m_byobBufferData[i].m_byobBufferStart))));
        m_byobBufferData[i].m_byobBufferEnd = m_byobBufferData[i].m_byobBufferStart + bufferSize;
        m_byobBufferData[i].m_byobBufferCurr = m_byobBufferData[i].m_byobBufferStart;

        ModelDrawHelper::ModelBuildOwnBundle(m_modelMeshPart[i],
            reinterpret_cast<uint32_t**>(&(m_byobBufferData[i].m_byobBufferCurr)),
            true);

        // The address should never go past the allocated size
        assert(m_byobBufferData[i].m_byobBufferCurr < m_byobBufferData[i].m_byobBufferEnd);

        m_byobBufferData[i].m_byobExecutableBuffer->Unmap(0, nullptr);
    }

    // Create single executable buffer to run BYOB_SINGLE_EXECUTE
    {
        uint64_t bufferSize = MAX_PER_DRAW_SIZE * NUM_INSTANCES * m_deviceResources->GetBackBufferCount();
        // BYOB buffer should be 4 byte aligned
        bufferSize = AlignUp(bufferSize, 4);

        // Aligning buffer to 4096 to align with the page size
        bufferSize = AlignUp(bufferSize, 4096);

        D3D12_RESOURCE_DESC byobExecBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            bufferSize,                                                 // UINT64 width,
            D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER				// D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                        // UINT64 alignment = 0
        );

        DWORD flAllocation = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
        DWORD flXMemAllocationFlags = XMEM_GRAPHICS;
        DWORD flProtect = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_EXECUTE_READ;
        D3D12_GPU_VIRTUAL_ADDRESS byobExecuteAddress = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, bufferSize, flAllocation, flXMemAllocationFlags, flProtect));
        assert(byobExecuteAddress);

        DX::ThrowIfFailed(device->CreatePlacedResourceX(byobExecuteAddress,
            &byobExecBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_byobExecBuffer.m_byobExecutableBuffer.ReleaseAndGetAddressOf())));
        m_byobExecBuffer.m_byobExecutableBuffer->SetName(L"Buffer for BYOB Single Execute");

        DX::ThrowIfFailed(m_byobExecBuffer.m_byobExecutableBuffer->Map(0, nullptr, reinterpret_cast<void**>(&(m_byobExecBuffer.m_byobBufferStart))));
        m_byobExecBuffer.m_byobBufferEnd = m_byobExecBuffer.m_byobBufferStart + bufferSize;
        m_byobExecBuffer.m_byobBufferCurr = m_byobExecBuffer.m_byobBufferStart;
    }
}

// Create a buffer which can be executed on the GPU.
// This buffer will store the commands directly read by the GPU.
void Sample::CreateBuffersForGPUBundles(ID3D12Device * device)
{
    m_gpuExecuteFlag = false;
    m_showInfo = false;

    // New Descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,					            // D3D12_DESCRIPTOR_HEAP_TYPE Type;
        static_cast<uint32_t>(DescriptorHeapComputeEntry::TotalDescriptorHeapComputeEntryCount),	// UINT NumDescriptors;
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,				            // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
        0														            // UINT NodeMask;
    };
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(m_computeDescriptorHeap.ReleaseAndGetAddressOf())));

    // Create executable buffer
    uint32_t numElements = m_deviceResources->GetBackBufferCount() * NUM_INSTANCES;
    uint32_t structureByteStride = SIZEOF_GPU_APPEND_BUFFER_STRUCT;
    uint64_t bufferSize = static_cast<uint64_t>(numElements) * static_cast<uint64_t>(structureByteStride);
    // BYOB buffer should be 16 aligned
    bufferSize = AlignUp(bufferSize, 16);

    // Aligning buffer to 4096 to align with the page size
    bufferSize = AlignUp(bufferSize, 4096);

    D3D12_RESOURCE_DESC gpuBYOBAppendBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        bufferSize,                                                                 // UINT64 width,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
        D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER                               // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                                    // UINT64 alignment = 0
    );

    DWORD flAllocation = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
    DWORD flXMemAllocationFlags = XMEM_GRAPHICS;
    DWORD flProtect = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_EXECUTE_READWRITE;
    D3D12_GPU_VIRTUAL_ADDRESS gpuBYOBExecuteAddress = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, bufferSize, flAllocation, flXMemAllocationFlags, flProtect));
    assert(gpuBYOBExecuteAddress);

    DX::ThrowIfFailed(device->CreatePlacedResourceX(gpuBYOBExecuteAddress,
        &gpuBYOBAppendBufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBAppendBuffer.m_byobExecutableBuffer.ReleaseAndGetAddressOf())));
    m_gpuBYOBAppendBuffer.m_byobExecutableBuffer->SetName(L"GPU BYOB Append Buffer");

    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    auto countBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        m_deviceResources->GetBackBufferCount() * sizeof(uint64_t),                 // UINT64 width,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
        D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER                               // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                                    // UINT64 alignment = 0
    );
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
        &countBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
        nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                    // REFIID riidResource,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBCountBuffer.ReleaseAndGetAddressOf())));		// _Outptr_opt_ void** ppvResource
    m_gpuBYOBCountBuffer->SetName(L"GPU BYOB Count Buffer");

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHeapStart = m_computeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHeapStart = m_computeDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    uint32_t handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_appendBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHeapStart, static_cast<uint32_t>(DescriptorHeapComputeEntry::UAVAppend), handleIncrementSize);
    m_countBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHeapStart, static_cast<uint32_t>(DescriptorHeapComputeEntry::UAVCounter), handleIncrementSize);
    m_appendBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHeapStart, static_cast<uint32_t>(DescriptorHeapComputeEntry::UAVAppend), handleIncrementSize);
    m_countBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHeapStart, static_cast<uint32_t>(DescriptorHeapComputeEntry::UAVCounter), handleIncrementSize);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
    {
        DXGI_FORMAT_UNKNOWN,                                            // DXGI_FORMAT Format;
        D3D12_UAV_DIMENSION_BUFFER,                                     // D3D12_UAV_DIMENSION ViewDimension;
        {
            0,                                                          // UINT64 FirstElement;
            numElements,                                                // UINT NumElements;
            structureByteStride,                                        // UINT StructureByteStride;
            0,                                                          // UINT64 CounterOffsetInBytes;
            D3D12_BUFFER_UAV_FLAG_NONE,                                 // D3D12_BUFFER_UAV_FLAGS Flags;
        },                                                              // D3D12_BUFFER_UAV Buffer;
    };
    device->CreateUnorderedAccessView(m_gpuBYOBAppendBuffer.m_byobExecutableBuffer.Get(), m_gpuBYOBCountBuffer.Get(), &uavDesc, m_appendBufferDescriptorCPU);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavCountDesc =
    {
        DXGI_FORMAT_UNKNOWN,                                    // DXGI_FORMAT Format;
        D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
        {
            0,                                                  // UINT64 FirstElement;
            m_deviceResources->GetBackBufferCount(),            // UINT NumElements;
            sizeof(uint64_t),                                   // UINT StructureByteStride;
            0,                                                  // UINT64 CounterOffsetInBytes;
            D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
        },                                                      // D3D12_BUFFER_UAV Buffer;
    };
    device->CreateUnorderedAccessView(m_gpuBYOBCountBuffer.Get(), nullptr, &uavCountDesc, m_countBufferDescriptorCPU);

    // Leaving the append count buffer mapped as it has to be queried each frame for outputting the value to screen
    m_gpuBYOBCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_gpuBYOBCountBufferMapped));
    for (uint32_t i = 0; i < m_deviceResources->GetBackBufferCount(); ++i)
    {
        uint32_t indexFor64Bit = i * 2;
        m_gpuBYOBCountBufferMapped[indexFor64Bit] = 0;
        m_gpuBYOBCountBufferMapped[indexFor64Bit + 1] = 0;
    }

    D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_HEAP_PROPERTIES defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC cbModelBufDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(ModelData) * NUM_MODELS                                      // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0 )

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProp, 									                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbModelBufDesc,								                    // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBModelDataCB.ReleaseAndGetAddressOf())// REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_gpuBYOBModelDataCB->SetName(L"BYOB Model Data Buffer");

    ModelData* cbBYOBModelData;
    m_gpuBYOBModelDataCB->Map(0, nullptr, reinterpret_cast<void**>(&cbBYOBModelData));
    for (int modelID = 0; modelID < NUM_MODELS; ++modelID)
    {
        ModelData& modelData = cbBYOBModelData[modelID];
        auto& currMeshPart = m_modelMeshPart[modelID];

        // PSOs
        for (uint32_t psoID = 0; psoID < NUM_PSO; ++psoID)
        {
            modelData.m_pso[psoID].psoData = m_modelEffect[modelID]->GetPSODescriptors(psoID);
            modelData.m_pso[psoID].padding[0] = 0;
#ifndef _GAMING_XBOX_SCARLETT
            modelData.m_pso[psoID].padding[1] = 0;
#endif
        }

        // Vertex buffer data
        uint64_t vertexDescriptor = static_cast<uint64_t>(currMeshPart->staticVertexBuffer->GetGPUVirtualAddress())
            + (static_cast<uint64_t>(currMeshPart->vertexStride) << D3D12XBOX_SET_VERTEX_BUFFERS_STRIDE_SHIFT64);
        modelData.m_vertexBufferData.m_descriptorLo = (vertexDescriptor >> 0) & 0xFFFFFFFF;
        modelData.m_vertexBufferData.m_descriptorHi = (vertexDescriptor >> 32) & 0xFFFFFFFF;
        modelData.m_vertexBufferData.m_strideInBytes = currMeshPart->vertexStride;
        modelData.m_vertexBufferData.m_sizeInBytes = currMeshPart->vertexBufferSize;

        // Index buffer data
        D3D12_GPU_VIRTUAL_ADDRESS indexBufferGPUAddress = currMeshPart->staticIndexBuffer->GetGPUVirtualAddress();
        modelData.m_indexBufferData.m_packetHeader = D3D12XBOX_PACKET_SET_INDEX_BUFFER + currMeshPart->indexFormat;
        modelData.m_indexBufferData.m_bufferLocationLo = (indexBufferGPUAddress >> 0) & 0xFFFFFFFF;
        modelData.m_indexBufferData.m_bufferLocationHi = (indexBufferGPUAddress >> 32) & 0xFFFFFFFF;
        modelData.m_indexBufferData.m_padding = 0;

        // Descriptor table data
        modelData.m_descriptorTableData.m_gpuDescriptorHandle = static_cast<uint32_t>(m_modelEffect[modelID]->GetTextureGPUHandle());
        modelData.m_descriptorTableData.m_rootParameterIndex = static_cast<uint32_t>(ModelLoader::RootParameterIndex::DescriptorTable0);
        modelData.m_descriptorTableData.m_padding0 = 0;
        modelData.m_descriptorTableData.m_padding1 = 0;

        // Root constant buffer data
        modelData.m_rootConstantBufferData.m_rootParameterIndex = static_cast<uint32_t>(ModelLoader::RootParameterIndex::ConstantBuffer0);
        D3D12_GPU_VIRTUAL_ADDRESS constBufferGPUAddress = m_modelEffect[modelID]->GetEffectDataGPUVirtualAddress();
        modelData.m_rootConstantBufferData.m_bufferLocationLo = (constBufferGPUAddress >> 0) & 0xFFFFFFFF;
        modelData.m_rootConstantBufferData.m_bufferLocationHi = (constBufferGPUAddress >> 32) & 0xFFFFFFFF;
        modelData.m_rootConstantBufferData.m_padding = 0;

        // Draw Indexed arguments
        modelData.m_drawIndexedArgs.packet = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
        modelData.m_drawIndexedArgs.BaseVertexLocation = uint32_t(currMeshPart->vertexOffset);
        modelData.m_drawIndexedArgs.IndexCountPerInstance = currMeshPart->indexCount;
        modelData.m_drawIndexedArgs.InstanceCount = 1;
        modelData.m_drawIndexedArgs.StartIndexLocation = currMeshPart->startIndex;
        modelData.m_drawIndexedArgs.StartInstanceLocation = 0;
    }
    m_gpuBYOBModelDataCB->Unmap(0, nullptr);

    // Buffer to store model data which is updated per frame
    uint64_t modelPerFrameBufferSize = sizeof(ModelDataUpdatePerFrame) * m_deviceResources->GetBackBufferCount();
    D3D12_RESOURCE_DESC cbModelPerFrameBufDesc = CD3DX12_RESOURCE_DESC::Buffer(
        modelPerFrameBufferSize                                             // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0 )

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProp, 									                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbModelPerFrameBufDesc,								            // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBModelPerFrameDataCB.ReleaseAndGetAddressOf())// REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_gpuBYOBModelPerFrameDataCB->SetName(L"BYOB Model Data Updated Per Frame Buffer");

    // Update per frame model data
    ModelDataUpdatePerFrame* modelDataUpdatePerFrame;
    m_gpuBYOBModelPerFrameDataCB->Map(0, nullptr, reinterpret_cast<void**>(&modelDataUpdatePerFrame));
    for (uint32_t index = 0; index < m_deviceResources->GetBackBufferCount(); ++index)
    {
        D3D12_GPU_VIRTUAL_ADDRESS constBufferGPUAddress = m_meshDataCB->GetGPUVirtualAddress();
        constBufferGPUAddress += sizeof(ModelLoader::ModelEffectConstants) * NUM_INSTANCES * index;
        modelDataUpdatePerFrame[index].m_constantBufferLocationLo = (constBufferGPUAddress >> 0) & 0xFFFFFFFF;
        modelDataUpdatePerFrame[index].m_constantBufferLocationHi = (constBufferGPUAddress >> 32) & 0xFFFFFFFF;
        modelDataUpdatePerFrame[index].m_sizeofModelEffectConstants = sizeof(ModelLoader::ModelEffectConstants);
    }
    m_gpuBYOBModelPerFrameDataCB->Unmap(0, nullptr);

    // Buffer to supply instance data to the GPU
    uint64_t instanceBufferSize = sizeof(InstanceOut) * NUM_INSTANCES * m_deviceResources->GetBackBufferCount();
    D3D12_RESOURCE_DESC cbInstanceBufDesc = CD3DX12_RESOURCE_DESC::Buffer(
        instanceBufferSize                                                  // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProp, 									                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbInstanceBufDesc,										            // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBInstanceDataCB.ReleaseAndGetAddressOf())// REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_gpuBYOBInstanceDataCB->SetName(L"BYOB Instance Data Buffer");

    // Fill instance buffer data
    m_gpuBYOBInstanceDataCB->Map(0, nullptr, reinterpret_cast<void**>(&m_gpuBYOBInstanceDataMappedCB));
    for (uint32_t index = 0; index < m_deviceResources->GetBackBufferCount(); ++index)
    {
        uint32_t startIndex = index * NUM_INSTANCES;
        for (int instance = 0; instance < NUM_INSTANCES; ++instance)
        {
            auto& instanceDataCurrIndex = m_gpuBYOBInstanceDataMappedCB[startIndex + instance];
            instanceDataCurrIndex.m_modelID = m_meshInstances[instance].m_modelID;
            instanceDataCurrIndex.m_psoID = m_meshInstances[instance].m_psoID;
            instanceDataCurrIndex.m_boundingSphereCenterAndRadius = m_meshInstances[instance].m_boundingSphere;
        }
    }

    // Create SRV for the buffer
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandleInstance = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_computeDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        (int)DescriptorHeapComputeEntry::InstanceData,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescInstanceData =
    {
        DXGI_FORMAT_UNKNOWN,												// DXGI_FORMAT Format;
        D3D12_SRV_DIMENSION_BUFFER,											// D3D12_SRV_DIMENSION ViewDimension;
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,							// UINT Shader4ComponentMapping;
        {																	// D3D12_BUFFER_SRV Buffer;
            0,																// UINT64 FirstElement;
            NUM_INSTANCES * m_deviceResources->GetBackBufferCount(),		// UINT NumElements;
            sizeof(InstanceOut),											// UINT StructureByteStride;
            D3D12_BUFFER_SRV_FLAG_NONE										// D3D12_BUFFER_SRV_FLAGS Flags;
        },
    };

    device->CreateShaderResourceView(m_gpuBYOBInstanceDataCB.Get(), &srvDescInstanceData, cpuDescHandleInstance);

    // Buffer to store root signature packet header data
    D3D12_RESOURCE_DESC cbRSPacketHeaderBufDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(uint32_t) * static_cast<uint32_t>(ModelLoader::RootParameterIndex::RootParameterCount)       // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
    );													                // UINT64 alignment = 0 )

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProp, 									                // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
        &cbRSPacketHeaderBufDesc,								            // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
        nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_gpuBYOBRSHeaderData.ReleaseAndGetAddressOf())// REFIID riidResource,
                                                                            // _Outptr_opt_ void** ppvResource)
    ));
    m_gpuBYOBRSHeaderData->SetName(L"BYOB Root Signature Packet Header Data Buffer");

    // Create SRV for the buffer
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandleRSPacketHeader = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_computeDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        (int)DescriptorHeapComputeEntry::RSPacketHeaderData,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescRSPacketHeader =
    {
        DXGI_FORMAT_UNKNOWN,												// DXGI_FORMAT Format;
        D3D12_SRV_DIMENSION_BUFFER,											// D3D12_SRV_DIMENSION ViewDimension;
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,							// UINT Shader4ComponentMapping;
        {																	// D3D12_BUFFER_SRV Buffer;
            0,																// UINT64 FirstElement;
            static_cast<uint32_t>(ModelLoader::RootParameterIndex::RootParameterCount), // UINT NumElements;
            sizeof(uint32_t),	   									        // UINT StructureByteStride;
            D3D12_BUFFER_SRV_FLAG_NONE										// D3D12_BUFFER_SRV_FLAGS Flags;
        },
    };

    device->CreateShaderResourceView(m_gpuBYOBRSHeaderData.Get(), &srvDescRSPacketHeader, cpuDescHandleRSPacketHeader);

    m_gpuDescHandleBYOBCompute = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_computeDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
        (int)DescriptorHeapComputeEntry::InstanceData,
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}

void Sample::CreateComputePSO(ID3D12Device * device)
{
    // Compute PSO
    auto gpuBYOBCSBlob = DX::ReadData(L"BuildOwnBundleCS.cso");

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc =
    {
        m_rootSignature.Get(),							          // ID3D12RootSignature* pRootSignature;
        { gpuBYOBCSBlob.data(), gpuBYOBCSBlob.size()},	          // D3D12_SHADER_BYTECODE CS;
        0,												          // UINT NodeMask;
        nullptr											          // D3D12_CACHED_PIPELINE_STATE CachedPSO;
    };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_GRAPHICS_PPV_ARGS(m_gpuBYOBComputePSO.ReleaseAndGetAddressOf())));

    // Compute PSO with culling enabled
    auto gpuBYOBCullCSBlob = DX::ReadData(L"BuildOwnBundleCullCS.cso");
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeCullPSODesc =
    {
        m_rootSignature.Get(),							          // ID3D12RootSignature* pRootSignature;
        { gpuBYOBCullCSBlob.data(), gpuBYOBCullCSBlob.size() },	  // D3D12_SHADER_BYTECODE CS;
        0,												          // UINT NodeMask;
        nullptr											          // D3D12_CACHED_PIPELINE_STATE CachedPSO;
    };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computeCullPSODesc, IID_GRAPHICS_PPV_ARGS(m_gpuBYOBComputeCullPSO.ReleaseAndGetAddressOf())));
}

// Set the PSO and descriptor heap data before executing bundle data
void Sample::SetStatesBeforeDraw(ID3D12GraphicsCommandList* graphicsCmdList, bool patchPSO, uint32_t instance, uint64_t cbOffset)
{
    if (patchPSO)
    {
        m_modelEffect[m_meshInstances[instance].m_modelID]->SetPSO(graphicsCmdList, m_meshInstances[instance].m_psoID);
    }
    m_modelEffect[m_meshInstances[instance].m_modelID]->SetDescriptorHeaps(graphicsCmdList, cbOffset);
}

// Set state using GPU packets before executing the draw call
void Sample::SetStatesBYOB(uint32_t** writeAddress, bool patchPSO, bool setTexture, uint32_t* rootPacketHeader, uint32_t instance, uint64_t cbOffset)
{
    if (patchPSO)
    {
        m_modelEffect[m_meshInstances[instance].m_modelID]->SetPSOBYOB(writeAddress, m_meshInstances[instance].m_psoID);
    }
    if (setTexture)
    {
        m_modelEffect[m_meshInstances[instance].m_modelID]->SetRootDescriptorTableBYOB(writeAddress, rootPacketHeader);
    }
    m_modelEffect[m_meshInstances[instance].m_modelID]->UpdateConstantsBYOB(writeAddress, rootPacketHeader, cbOffset);
}
#pragma endregion
