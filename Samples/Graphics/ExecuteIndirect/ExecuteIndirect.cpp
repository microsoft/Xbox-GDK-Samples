//--------------------------------------------------------------------------------------
// ExecuteIndirect.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ExecuteIndirect.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    enum DRAW_METHOD
    {
        DRAW_METHOD_DIRECT, 
        DRAW_METHOD_INDIRECT, 

        DRAW_METHOD_COUNT
    };

    const WCHAR * const drawMethodNames[] =
    {
        L"Direct",
        L"Indirect",
    };
    static_assert(DRAW_METHOD_COUNT == _countof(drawMethodNames), "Array size mismatch");

    const D3D12_RANGE nullRange = { 0UL, 0UL };
};

const uint32_t Sample::m_indexCountInElements[ MESH_SHAPE_COUNT ] = 
{
    12,                                                  // MESH_SHAPE_TETRAHEDRON
    36,                                                  // MESH_SHAPE_CUBE
    24,                                                  // MESH_SHAPE_OCTAHEDRON
    108,                                                 // MESH_SHAPE_DODECAHEDRON
    60,                                                  // MESH_SHAPE_ICOSAHEDRON
};
const uint32_t Sample::m_vertexCountInElements[ MESH_SHAPE_COUNT ] = 
{
    4,                                                   // MESH_SHAPE_TETRAHEDRON
    8,                                                   // MESH_SHAPE_CUBE
    6,                                                   // MESH_SHAPE_OCTAHEDRON
    20,                                                  // MESH_SHAPE_DODECAHEDRON
    12,                                                  // MESH_SHAPE_ICOSAHEDRON
};
uint32_t Sample::m_indexBufferStart[ MESH_SHAPE_COUNT ];
int32_t Sample::m_vertexBufferStart[ MESH_SHAPE_COUNT ];

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_GeometryShaders);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    m_timeTotal = float(timer.GetTotalSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MostRecent);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_drawMethod = ++m_drawMethod % DRAW_METHOD_COUNT;
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_cull = !m_cull;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    auto rollPitchYaw = XMVectorSet(0.0f, cosf(m_rotationSpeed * m_timeTotal), sinf(m_rotationSpeed * m_timeTotal), 0.0f);

    auto rotation =
        XMMatrixRotationAxis(m_cameraDirection, XMVectorGetX(rollPitchYaw) * m_rotationScale)
        * XMMatrixRotationAxis(m_cameraRight, XMVectorGetY(rollPitchYaw) * m_rotationScale)
        * XMMatrixRotationAxis(m_cameraUp, XMVectorGetZ(rollPitchYaw) * m_rotationScale);

    m_cameraDirection = XMVector3Normalize(XMVector3Transform(m_cameraDirection, rotation));
    m_cameraRight = XMVector3Normalize(XMVector3Cross(m_cameraUp, m_cameraDirection));

    // Move the camera (should really be a matrix mul)
    m_cameraPosition = XMVectorAdd(m_cameraPosition, XMVectorScale(m_cameraRight, XMVectorGetX(translation) * m_translationSpeed));
    m_cameraPosition = XMVectorAdd(m_cameraPosition, XMVectorScale(m_cameraUp, XMVectorGetY(translation) * m_translationSpeed));
    m_cameraPosition = XMVectorAdd(m_cameraPosition, XMVectorScale(m_cameraDirection, XMVectorGetZ(translation) * m_translationSpeed));

    auto view = XMMatrixLookAtLH(m_cameraPosition, XMVectorAdd(m_cameraPosition, m_cameraDirection), m_cameraUp);
    auto proj = XMMatrixPerspectiveFovLH(3.14156f / 4.0f,
        m_viewportWidth / (FLOAT)m_viewportHeight,
        m_nearPlane,
        m_farPlane);
    m_viewProj = view * proj;

    // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    auto viewProjT = XMMatrixTranspose(m_viewProj);
    m_cameraFrustum.m_plane[0] = XMVectorAdd(viewProjT.r[3], viewProjT.r[0]);            // Left
    m_cameraFrustum.m_plane[1] = XMVectorSubtract(viewProjT.r[3], viewProjT.r[0]);       // Right
    m_cameraFrustum.m_plane[2] = XMVectorAdd(viewProjT.r[3], viewProjT.r[1]);            // Bottom
    m_cameraFrustum.m_plane[3] = XMVectorSubtract(viewProjT.r[3], viewProjT.r[1]);       // Top
    m_cameraFrustum.m_plane[4] = XMVectorAdd(XMVectorZero(), viewProjT.r[2]);               // Near (D3D convention --- near plane is at 0)
    m_cameraFrustum.m_plane[5] = XMVectorSubtract(viewProjT.r[3], viewProjT.r[2]);       // Far

    for (auto plane = 0U; plane < _countof(m_cameraFrustum.m_plane); ++plane)
    {
        m_cameraFrustum.m_plane[plane] = XMPlaneNormalize(m_cameraFrustum.m_plane[plane]);
    }
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

    auto maxFramesInFlight = m_deviceResources->GetBackBufferCount();
    auto frame = m_frame % maxFramesInFlight;
    auto firstSlot = frame * m_maxInstances;

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    Clear();

    m_gpuTimerFrame->BeginFrame(commandList);
    m_gpuTimerFrame->Start(commandList);
    m_cpuTimerFrame->Start();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Set transforms
    D3D12_RANGE constantBufferTransformRange =
    {
        sizeof(ConstantBufferTransform) * firstSlot,                           // SIZE_T Begin;
        sizeof(ConstantBufferTransform) * (firstSlot + m_maxInstances),     // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    ConstantBufferTransform* constantBufferTransformData = nullptr;
    DX::ThrowIfFailed(m_constantBufferTransform->Map(0, &nullRange, reinterpret_cast<void**>(&constantBufferTransformData)));

    for (auto instance = 0U; instance < m_maxInstances; ++instance)
    {
        auto slot = firstSlot + instance;

        auto pInstance = &m_instances[instance];
        auto matWorld = XMMatrixRotationAxis(pInstance->m_rotAxis, pInstance->m_rotSpeed * m_timeTotal)
            * XMMatrixTranslationFromVector(pInstance->m_boundingSphere.m_centerAndRadius);
        constantBufferTransformData[slot].m_world = matWorld;
        constantBufferTransformData[slot].m_worldViewProj = matWorld * m_viewProj;
    }

    m_constantBufferTransform->Unmap(0, &constantBufferTransformRange);

    // Set lighting parameters (these don't actually change, but we leave open that possibility)
    const XMVECTOR vColor[m_maxInstances] =
    {
        XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
        XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f),
        XMVectorSet(0.0f, 1.0f, 1.0f, 1.0f),
        XMVectorSet(1.0f, 0.0f, 1.0f, 1.0f),
    };

    D3D12_RANGE constantBufferTintRange =
    {
        sizeof(ConstantBufferTint) * firstSlot,                                // SIZE_T Begin;
        sizeof(ConstantBufferTint) * (firstSlot + m_maxInstances),          // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    ConstantBufferTint* constantBufferTintData = nullptr;
    DX::ThrowIfFailed(m_constantBufferTint->Map(0, &nullRange, reinterpret_cast<void**>(&constantBufferTintData)));

    XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(1.0f, 1.0f, -1.0f, 0.0f));
    for (auto instance = 0U; instance < m_maxInstances; ++instance)
    {
        auto iMesh = m_instances[instance].m_mesh;
        auto iSlot = firstSlot + instance;

        constantBufferTintData[iSlot].m_color = vColor[iMesh];
        constantBufferTintData[iSlot].m_lightDir = lightDir;
    }

    m_constantBufferTint->Unmap(0, &constantBufferTintRange);

    RenderScene();

    m_cpuTimerFrame->Stop();
    m_gpuTimerFrame->Stop(commandList);
    m_gpuTimerFrame->EndFrame(commandList);

    RenderUI();

    PIXEndEvent(commandList);

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

//------------------------------------------------------------------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Render the scene indirectly or directly
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::RenderScene()
{
    const auto commandList = m_deviceResources->GetCommandList();

    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, __FUNCTIONW__);

    auto maxFramesInFlight = m_deviceResources->GetBackBufferCount();
    auto frame = m_frame % maxFramesInFlight;
    auto firstSlot = frame * m_maxInstances;
    auto slot = firstSlot;

    commandList->SetGraphicsRootSignature(m_rootSignatureMesh);
    commandList->IASetIndexBuffer(&m_indexBufferView);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    switch (m_drawMethod)
    {
    case DRAW_METHOD_DIRECT:
    {
        commandList->SetPipelineState(m_pipelineStateMesh);

        m_executeCount = 0UL;
        for (auto instance = 0U; instance < m_maxInstances; ++instance, ++slot) 
        {
            auto iMesh = m_instances[instance].m_mesh;
            if (!m_cull || CullSphereFrustum(m_instances[instance].m_boundingSphere, m_cameraFrustum))
            {
                ++m_executeCount;
                commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferTransform->GetGPUVirtualAddress() + slot * sizeof(ConstantBufferTransform));
                commandList->SetGraphicsRootConstantBufferView(1, m_constantBufferTint->GetGPUVirtualAddress() + slot * sizeof(ConstantBufferTint));
                commandList->DrawIndexedInstanced(m_indexCountInElements[iMesh], 1, m_indexBufferStart[iMesh], m_vertexBufferStart[iMesh], 0);
            }
        }
    }
    break;

    case DRAW_METHOD_INDIRECT:
    {
        ID3D12Resource* argumentBuffer = nullptr;
        ID3D12Resource* countBuffer = nullptr;
        auto argumentBufferOffset = 0ULL;
        auto countBufferOffset = 0ULL;

        // This step is really only required once, but we simulate a real title, where the objects might vary per frame
        UploadIndirectArguments();

        if (m_cull)
        {
            ComputeCulling();   // Will overwrite pipeline state

            argumentBuffer = m_argumentBuffer[INDIRECT_BUFFER_POST_CULL];
            countBuffer = m_countBuffer[INDIRECT_BUFFER_POST_CULL];
            argumentBufferOffset = 0U;
            countBufferOffset = 0U;
        }
        else
        {
            m_executeCount = m_maxInstances;

            argumentBuffer = m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL];
            countBuffer = m_countBuffer[INDIRECT_BUFFER_PRE_CULL];
            argumentBufferOffset = firstSlot * sizeof(s_IndirectArgs);
            countBufferOffset = frame * sizeof(s_IndirectCount);
        }

        commandList->SetPipelineState(m_pipelineStateMesh);
        commandList->ExecuteIndirect(m_commandSignature, m_maxInstances, argumentBuffer, argumentBufferOffset, countBuffer, countBufferOffset);
    }
    break;

    default:
#pragma warning(suppress:4127) // conditional expression is constant
        assert(FALSE);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: RenderUI()
// Desc: Render the UI
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::RenderUI()
{
    const auto commandList = m_deviceResources->GetCommandList();

    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, __FUNCTIONW__);

    float margin = 30.0f;
    float x = margin;
    float tab = 300.0f;
    float xTab = x + tab;
    float y = margin;
    float yInc = 30.0f;

    m_spriteBatch->Begin(commandList);

    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    //m_fontUI SetScaleFactors(1.2f, 1.2f);
    m_fontUI->DrawString(m_spriteBatch.get(), L"ExecuteIndirect", XMFLOAT2(x, y));
    y += yInc;
    y += yInc;
    y += yInc;

    //m_fontUI.SetScaleFactors(1.0f, 1.0f);
    DX::DrawControllerString(m_spriteBatch.get(), m_fontUI.get(), m_fontController.get(), L"[A] Draw method:", XMFLOAT2(x, y));
    m_fontUI->DrawString(m_spriteBatch.get(), drawMethodNames[m_drawMethod], XMFLOAT2(xTab, y));
    y += yInc;
    DX::DrawControllerString(m_spriteBatch.get(), m_fontUI.get(), m_fontController.get(), L"[B] Culling:", XMFLOAT2(x, y));
    m_fontUI->DrawString(m_spriteBatch.get(), m_cull ? L"On" : L"Off", XMFLOAT2(xTab, y));
    y += yInc;

    y = m_deviceResources->GetOutputSize().bottom - margin - 3 * yInc;

    //m_fontUI.SetScaleFactors(1.0f, 1.0f);
    std::wostringstream strExecuteCount;
    strExecuteCount << m_executeCount;
    m_fontUI->DrawString(m_spriteBatch.get(), L"Drawn instances:", XMFLOAT2(x, y));
    m_fontUI->DrawString(m_spriteBatch.get(), strExecuteCount.str().c_str(), XMFLOAT2(xTab, y));
    y += yInc;
    std::wostringstream strCpuTimeInMs;
    strCpuTimeInMs << m_cpuTimerFrame->GetElapsedMS();
    m_fontUI->DrawString(m_spriteBatch.get(), L"Cpu time (ms):", XMFLOAT2(x, y));
    m_fontUI->DrawString(m_spriteBatch.get(), strCpuTimeInMs.str().c_str(), XMFLOAT2(xTab, y));
    y += yInc;
    std::wostringstream strGpuTimeInMs;
    strGpuTimeInMs << m_gpuTimerFrame->GetElapsedMS();
    m_fontUI->DrawString(m_spriteBatch.get(), L"Gpu time (ms):", XMFLOAT2(x, y));
    m_fontUI->DrawString(m_spriteBatch.get(), strGpuTimeInMs.str().c_str(), XMFLOAT2(xTab, y));
    y += yInc;

    m_spriteBatch->End();
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: UploadIndirectArguments()
// Desc: Perform culling of instances
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::UploadIndirectArguments()
{
    const auto commandList = m_deviceResources->GetCommandList();

    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, __FUNCTIONW__);

    auto TransitionBarrier = [commandList] (ID3D12Resource* pResource, const D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After) -> void
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pResource;
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = Before;
        barrier.Transition.StateAfter = After;
        commandList->ResourceBarrier(1, &barrier);
    };

    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(m_countBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList->CopyResource(m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL], m_argumentBuffer[INDIRECT_BUFFER_UPLOAD]);
    commandList->CopyResource(m_countBuffer[INDIRECT_BUFFER_PRE_CULL], m_countBuffer[INDIRECT_BUFFER_UPLOAD]);

    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionBarrier(m_countBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: ComputeCulling()
// Desc: Perform culling of instances
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::ComputeCulling()
{
    const auto commandList = m_deviceResources->GetCommandList();

    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, __FUNCTIONW__);

    auto maxFramesInFlight = m_deviceResources->GetBackBufferCount();
    auto frame = m_frame % maxFramesInFlight;
    auto firstSlot = frame * m_maxInstances;

    auto TransitionBarrier = [commandList] (ID3D12Resource* pResource, const D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After) -> void
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pResource;
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = Before;
        barrier.Transition.StateAfter = After;
        commandList->ResourceBarrier(1, &barrier);
    };

    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_POST_CULL], D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionBarrier(m_countBuffer[INDIRECT_BUFFER_POST_CULL], D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Set view frustum for culling
    D3D12_RANGE constantBufferFrustumRange =
    {
        sizeof(ConstantBufferFrustum) * frame,
        sizeof(ConstantBufferFrustum) * (frame + 1),    // One past end, so (End - Begin) = Size
    };

    ConstantBufferFrustum* constantBufferFrustumData = nullptr;
    DX::ThrowIfFailed(m_constantBufferFrustum->Map(0, &nullRange, reinterpret_cast<void**>(&constantBufferFrustumData)));

    constantBufferFrustumData[frame].m_frustum = m_cameraFrustum;

    m_constantBufferFrustum->Unmap(0, &constantBufferFrustumRange);

    // Initialize Count to 0
    const UINT clearValues[4] = {};
    commandList->ClearUnorderedAccessViewUint(
        m_countDescriptorGPU,
        m_countDescriptorCPU,
        m_countBuffer[INDIRECT_BUFFER_POST_CULL],
        clearValues,
        0U,
        nullptr);

    commandList->SetComputeRootSignature(m_rootSignatureCull);
    commandList->SetPipelineState(m_pipelineStateCull);
    commandList->SetDescriptorHeaps(1, &m_indirectArgsGPUDescriptorHeap);

    commandList->SetComputeRootConstantBufferView(0, m_constantBufferFrustum->GetGPUVirtualAddress());
    commandList->SetComputeRootShaderResourceView(1, m_instanceBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootShaderResourceView(2, m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL]->GetGPUVirtualAddress() + firstSlot * sizeof(s_IndirectArgs));
    commandList->SetComputeRootDescriptorTable(3, m_argumentDescriptorGPU);

    commandList->Dispatch(m_maxInstances / m_cullThreadgroupSize, 1, 1);

    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionBarrier(m_argumentBuffer[INDIRECT_BUFFER_POST_CULL], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    TransitionBarrier(m_countBuffer[INDIRECT_BUFFER_POST_CULL], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    // Readback the post-culling count
    commandList->CopyBufferRegion(m_countReadbackBuffer,
        0UL,
        m_countBuffer[INDIRECT_BUFFER_POST_CULL],
        0UL,
        sizeof(s_IndirectCount));

    s_IndirectCount* count = nullptr;
    D3D12_RANGE countRange =
    {
        0,                                                                      // SIZE_T Begin;
        sizeof(*count),                                                      // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    DX::ThrowIfFailed(m_countReadbackBuffer->Map(0, &countRange, reinterpret_cast<void**>(&count)));

    m_executeCount = count[0];  // Lag by a frame or two

    m_countReadbackBuffer->Unmap(0, &nullRange);

    TransitionBarrier(m_countBuffer[INDIRECT_BUFFER_POST_CULL], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: CullSphereFrustum()
// Desc: Perform conservative culling. 
//          TRUE means "maybe intersect"
//          FALSE means "definitely do not intersect"
//------------------------------------------------------------------------------------------------------------------------------------
BOOL Sample::CullSphereFrustum(const s_BoundingSphere& sphere, const Frustum& frustum)
{
    for (auto i = 0U; i < _countof(frustum.m_plane); ++i)
    {
        if (XMVectorGetX(XMPlaneDotCoord(frustum.m_plane[i], sphere.m_centerAndRadius)) < -XMVectorGetW(sphere.m_centerAndRadius))
        {
            return FALSE;
        }
    }

    return TRUE;
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
    auto commandQueue = m_deviceResources->GetCommandQueue();

    m_frame = 0;

    m_drawMethod = DRAW_METHOD_DIRECT;
    m_cull = FALSE;

    InitializeMeshes();

    InitializeInstances();

    InitializeConstantBuffers();

    InitializeRootSignatures();

    InitializePipelineStates();

    InitializeIndirectParameters();

    m_cpuTimerFrame = std::make_unique<DX::CPUTimer>();
    m_gpuTimerFrame = std::make_unique<DX::GPUTimer>(device, commandQueue);

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    m_fontUI = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"SegoeUI_18.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontUI), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontUI));
    m_fontController = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"XboxOneControllerSmall.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontController), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontController));

    const RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

    resourceUpload.End(commandQueue);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const outputSize = m_deviceResources->GetOutputSize();

    // Initial camera values
    m_cameraPosition = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
    m_cameraDirection = XMVector3Normalize(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f));
    m_cameraRight = XMVector3Normalize(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
    m_cameraUp = XMVector3Normalize(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    m_translationSpeed = 0.0f;
    m_rotationSpeed = 1.0f;
    m_rotationScale = 0.001f;
    m_nearPlane = 5.0f;
    m_farPlane = 1000.0f;
    m_viewportWidth = outputSize.right - outputSize.left;
    m_viewportHeight = outputSize.bottom - outputSize.top;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializeMeshes()
// Desc: Initialize meshes
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializeMeshes()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_meshRadius = 1.0f; // All meshes are arranged to have radius of 1.0f

    uint32_t indexBufferTotalSizeInElements = 0;
    int32_t vertexBufferTotalSizeInElements = 0;
    for (uint32_t iMesh = 0; iMesh < MESH_SHAPE_COUNT; ++iMesh)
    {
        m_indexBufferStart[iMesh] = indexBufferTotalSizeInElements;
        m_vertexBufferStart[iMesh] = vertexBufferTotalSizeInElements;

        indexBufferTotalSizeInElements += m_indexCountInElements[iMesh];
        vertexBufferTotalSizeInElements += m_vertexCountInElements[iMesh];
    }

    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        indexBufferTotalSizeInElements * sizeof(Index));
    DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(&m_indexBuffer)));

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {
        m_indexBuffer->GetGPUVirtualAddress(),
        static_cast<UINT>(indexBufferTotalSizeInElements * sizeof(Index)),
        DXGI_FORMAT_R16_UINT };
    m_indexBufferView = indexBufferView;

    auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        vertexBufferTotalSizeInElements * sizeof(Vertex));
    DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(&m_vertexBuffer)));

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {
        m_vertexBuffer->GetGPUVirtualAddress(),
        static_cast<UINT>(vertexBufferTotalSizeInElements * sizeof(Vertex)),
        sizeof(Vertex) };
    m_vertexBufferView = vertexBufferView;

    Index* indexData = nullptr;
    DX::ThrowIfFailed(m_indexBuffer->Map(0, &nullRange, reinterpret_cast<void**>(&indexData)));

    Vertex* vertexData = nullptr;
    DX::ThrowIfFailed(m_vertexBuffer->Map(0, &nullRange, reinterpret_cast<void**>(&vertexData)));

    // https://en.wikipedia.org/wiki/Platonic_solid#Cartesian_coordinates
    for (uint32_t mesh = 0; mesh < MESH_SHAPE_COUNT; ++mesh)
    {
        switch (mesh)
        {
        case MESH_SHAPE_TETRAHEDRON:
        {
            const float scale = m_meshRadius * 1.0f / sqrtf(3.0f);

            vertexData[0].m_position = XMVectorSet(scale, scale, scale, 1.0f);
            vertexData[1].m_position = XMVectorSet(scale, -scale, -scale, 1.0f);
            vertexData[2].m_position = XMVectorSet(-scale, scale, -scale, 1.0f);
            vertexData[3].m_position = XMVectorSet(-scale, -scale, scale, 1.0f);

            auto faceIndexData = indexData;
            const uint32_t faces = 4;
            const uint32_t indicesPerFace = 3;
            Index faceIndices[faces][indicesPerFace] =
            {
                { 0, 1, 2, },
                { 0, 2, 3, },
                { 0, 3, 1, },
                { 3, 2, 1, },
            };
            for (uint32_t iFace = 0; iFace < faces; ++iFace)
            {
                faceIndexData[0] = faceIndices[iFace][0];
                faceIndexData[1] = faceIndices[iFace][1];
                faceIndexData[2] = faceIndices[iFace][2];

                faceIndexData += 3;
            }
        }

        break;

        case MESH_SHAPE_CUBE:
        {
            const float scale = m_meshRadius * 1.0f / sqrtf(3.0f);

            vertexData[0].m_position = XMVectorSet(-scale, -scale, -scale, 1.0f);
            vertexData[1].m_position = XMVectorSet(-scale, -scale, scale, 1.0f);
            vertexData[2].m_position = XMVectorSet(-scale, scale, -scale, 1.0f);
            vertexData[3].m_position = XMVectorSet(-scale, scale, scale, 1.0f);
            vertexData[4].m_position = XMVectorSet(scale, -scale, -scale, 1.0f);
            vertexData[5].m_position = XMVectorSet(scale, -scale, scale, 1.0f);
            vertexData[6].m_position = XMVectorSet(scale, scale, -scale, 1.0f);
            vertexData[7].m_position = XMVectorSet(scale, scale, scale, 1.0f);

            auto faceIndexData = indexData;
            const uint32_t faces = 6;
            const uint32_t indicesPerFace = 4;
            Index faceIndices[faces][indicesPerFace] =
            {
                { 0, 1, 3, 2, },
                { 0, 4, 5, 1, },
                { 0, 2, 6, 4, },
                { 4, 6, 7, 5, },
                { 2, 3, 7, 6, },
                { 1, 5, 7, 3, },
            };
            for (uint32_t iFace = 0; iFace < faces; ++iFace)
            {
                faceIndexData[0] = faceIndices[iFace][0];
                faceIndexData[1] = faceIndices[iFace][1];
                faceIndexData[2] = faceIndices[iFace][2];

                faceIndexData[3] = faceIndices[iFace][0];
                faceIndexData[4] = faceIndices[iFace][2];
                faceIndexData[5] = faceIndices[iFace][3];

                faceIndexData += 6;
            }
        }

        break;

        case MESH_SHAPE_OCTAHEDRON:
        {
            const float scale = m_meshRadius * 1.0f;

            vertexData[0].m_position = XMVectorSet(-scale, 0.0f, 0.0f, 1.0f);
            vertexData[1].m_position = XMVectorSet(scale, 0.0f, 0.0f, 1.0f);
            vertexData[2].m_position = XMVectorSet(0.0f, -scale, 0.0f, 1.0f);
            vertexData[3].m_position = XMVectorSet(0.0f, scale, 0.0f, 1.0f);
            vertexData[4].m_position = XMVectorSet(0.0f, 0.0f, -scale, 1.0f);
            vertexData[5].m_position = XMVectorSet(0.0f, 0.0f, scale, 1.0f);

            auto faceIndexData = indexData;
            const uint32_t faces = 8;
            const uint32_t indicesPerFace = 3;
            Index faceIndices[faces][indicesPerFace] =
            {
                { 0, 3, 4 },
                { 3, 1, 4 },
                { 1, 2, 4 },
                { 2, 0, 4 },
                { 3, 0, 5 },
                { 1, 3, 5 },
                { 2, 1, 5 },
                { 0, 2, 5 },
            };
            for (uint32_t iFace = 0; iFace < faces; ++iFace)
            {
                faceIndexData[0] = faceIndices[iFace][0];
                faceIndexData[1] = faceIndices[iFace][1];
                faceIndexData[2] = faceIndices[iFace][2];

                faceIndexData += 3;
            }
        }

        break;

        case MESH_SHAPE_DODECAHEDRON:
        {
            const float scale = m_meshRadius * 1.0f / sqrtf(3.0f);
            const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
            const float phiScale = phi * scale;
            const float invPhiScale = scale / phi;

            vertexData[0].m_position = XMVectorSet(-scale, -scale, -scale, 1.0f);
            vertexData[1].m_position = XMVectorSet(-scale, -scale, scale, 1.0f);
            vertexData[2].m_position = XMVectorSet(-scale, scale, -scale, 1.0f);
            vertexData[3].m_position = XMVectorSet(-scale, scale, scale, 1.0f);
            vertexData[4].m_position = XMVectorSet(scale, -scale, -scale, 1.0f);
            vertexData[5].m_position = XMVectorSet(scale, -scale, scale, 1.0f);
            vertexData[6].m_position = XMVectorSet(scale, scale, -scale, 1.0f);
            vertexData[7].m_position = XMVectorSet(scale, scale, scale, 1.0f);
            vertexData[8].m_position = XMVectorSet(-phiScale, -invPhiScale, 0.0f, 1.0f);
            vertexData[9].m_position = XMVectorSet(-phiScale, invPhiScale, 0.0f, 1.0f);
            vertexData[10].m_position = XMVectorSet(phiScale, -invPhiScale, 0.0f, 1.0f);
            vertexData[11].m_position = XMVectorSet(phiScale, invPhiScale, 0.0f, 1.0f);
            vertexData[12].m_position = XMVectorSet(0.0f, -phiScale, -invPhiScale, 1.0f);
            vertexData[13].m_position = XMVectorSet(0.0f, -phiScale, invPhiScale, 1.0f);
            vertexData[14].m_position = XMVectorSet(0.0f, phiScale, -invPhiScale, 1.0f);
            vertexData[15].m_position = XMVectorSet(0.0f, phiScale, invPhiScale, 1.0f);
            vertexData[16].m_position = XMVectorSet(-invPhiScale, 0.0f, -phiScale, 1.0f);
            vertexData[17].m_position = XMVectorSet(invPhiScale, 0.0f, -phiScale, 1.0f);
            vertexData[18].m_position = XMVectorSet(-invPhiScale, 0.0f, phiScale, 1.0f);
            vertexData[19].m_position = XMVectorSet(invPhiScale, 0.0f, phiScale, 1.0f);

            auto faceIndexData = indexData;
            const uint32_t faces = 12;
            const uint32_t indicesPerFace = 5;
            Index faceIndices[faces][indicesPerFace] =
            {
                {  0, 12, 13,  1,  8, },
                {  0,  8,  9,  2, 16, },
                {  0, 16, 17,  4, 12, },
                {  1, 18,  3,  9,  8, },
                {  1, 13,  5, 19, 18, },
                {  2,  9,  3, 15, 14, },
                {  2, 14,  6, 17, 16, },
                {  3, 18, 19,  7, 15, },
                {  4, 10,  5, 13, 12, },
                {  4, 17,  6, 11, 10, },
                {  5, 10, 11,  7, 19, },
                {  6, 14, 15,  7, 11, },
            };
            for (uint32_t iFace = 0; iFace < faces; ++iFace)
            {
                faceIndexData[0] = faceIndices[iFace][0];
                faceIndexData[1] = faceIndices[iFace][1];
                faceIndexData[2] = faceIndices[iFace][2];

                faceIndexData[3] = faceIndices[iFace][0];
                faceIndexData[4] = faceIndices[iFace][2];
                faceIndexData[5] = faceIndices[iFace][3];

                faceIndexData[6] = faceIndices[iFace][0];
                faceIndexData[7] = faceIndices[iFace][3];
                faceIndexData[8] = faceIndices[iFace][4];

                faceIndexData += 9;
            }
        }

        break;

        case MESH_SHAPE_ICOSAHEDRON:
        {
            const float scale = m_meshRadius * 1.0f / sqrtf((5.0f + sqrtf(5.0f)) / 2.0f);
            const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
            const float phiScale = phi * scale;

            vertexData[0].m_position = XMVectorSet(-scale, -phiScale, 0.0f, 1.0f);
            vertexData[1].m_position = XMVectorSet(-scale, phiScale, 0.0f, 1.0f);
            vertexData[2].m_position = XMVectorSet(scale, -phiScale, 0.0f, 1.0f);
            vertexData[3].m_position = XMVectorSet(scale, phiScale, 0.0f, 1.0f);
            vertexData[4].m_position = XMVectorSet(0.0f, -scale, -phiScale, 1.0f);
            vertexData[5].m_position = XMVectorSet(0.0f, -scale, phiScale, 1.0f);
            vertexData[6].m_position = XMVectorSet(0.0f, scale, -phiScale, 1.0f);
            vertexData[7].m_position = XMVectorSet(0.0f, scale, phiScale, 1.0f);
            vertexData[8].m_position = XMVectorSet(-phiScale, 0.0f, -scale, 1.0f);
            vertexData[9].m_position = XMVectorSet(phiScale, 0.0f, -scale, 1.0f);
            vertexData[10].m_position = XMVectorSet(-phiScale, 0.0f, scale, 1.0f);
            vertexData[11].m_position = XMVectorSet(phiScale, 0.0f, scale, 1.0f);

            auto faceIndexData = indexData;
            const uint32_t faces = 20;
            const uint32_t indicesPerFace = 3;
            Index faceIndices[faces][indicesPerFace] =
            {
                {  0,  4,  2, },
                {  0,  2,  5, },
                {  0,  5, 10, },
                {  0, 10,  8, },
                {  0,  8,  4, },
                {  3,  1,  7, },
                {  3,  7, 11, },
                {  3, 11,  9, },
                {  3,  9,  6, },
                {  3,  6,  1, },
                {  4,  9,  2, },
                {  2,  9, 11, },
                {  5,  2, 11, },
                {  5, 11,  7, },
                {  5,  7, 10, },
                { 10,  7,  1, },
                { 10,  1,  8, },
                {  8,  1,  6, },
                {  8,  6,  4, },
                {  4,  6,  9, },
            };
            for (uint32_t iFace = 0; iFace < faces; ++iFace)
            {
                faceIndexData[0] = faceIndices[iFace][0];
                faceIndexData[1] = faceIndices[iFace][1];
                faceIndexData[2] = faceIndices[iFace][2];

                faceIndexData += 3;
            }
        }

        break;
        }

        indexData += m_indexCountInElements[mesh];
        vertexData += m_vertexCountInElements[mesh];
    }

    D3D12_RANGE WrittenIndexRange =
    {
        0,                                                      // SIZE_T Begin;
        indexBufferTotalSizeInElements * sizeof(Index),      // SIZE_T End; // One past end, so (End - Begin) = Size
    };
    m_indexBuffer->Unmap(0, &WrittenIndexRange);

    D3D12_RANGE WrittenVertexRange =
    {
        0,                                                      // SIZE_T Begin;
        vertexBufferTotalSizeInElements * sizeof(Vertex),    // SIZE_T End; // One past end, so (End - Begin) = Size
    };
    m_vertexBuffer->Unmap(0, &WrittenVertexRange);
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializeInstances()
// Desc: Initialize instances
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializeInstances()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto RandInt0ToN = [] (uint32_t N) -> uint32_t { return (N * rand()) / RAND_MAX; };
    auto RandFloat0ToF = [] (float F) -> float { return (F * rand()) / RAND_MAX; };
    auto RandFloatNegFToF = [RandFloat0ToF] (float F) -> float { return RandFloat0ToF(2.0f * F) - F; };

    srand(0U);
    for (auto instance = 0U; instance < m_maxInstances; ++instance)
    {
        auto pInstance = &m_instances[instance];
        pInstance->m_mesh = RandInt0ToN(MESH_SHAPE_COUNT);
        pInstance->m_boundingSphere.m_centerAndRadius = XMVectorSet(RandFloatNegFToF(50.0f), RandFloatNegFToF(50.0f), RandFloat0ToF(100.0f), m_meshRadius);
        pInstance->m_rotAxis = XMVectorSet(RandFloatNegFToF(1.0f), RandFloatNegFToF(1.0f), RandFloatNegFToF(1.0f), 1.0f);
        pInstance->m_rotSpeed = RandFloat0ToF(3.0f);
    }

    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        m_maxInstances * sizeof(s_Instance)                  // UINT64 width,
                                                                // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                // UINT64 alignment = 0 )
    );
    DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &instanceBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(&m_instanceBuffer)));

    D3D12_RANGE instanceBufferRange =
    {
        0,                                                      // SIZE_T Begin;
        sizeof(s_BoundingSphere) * m_maxInstances,           // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    s_BoundingSphere* instanceBufferData = nullptr;
    DX::ThrowIfFailed(m_instanceBuffer->Map(0, &nullRange, reinterpret_cast<void**>(&instanceBufferData)));

    for (auto instance = 0U; instance < m_maxInstances; ++instance)
    {
        auto pInstance = &m_instances[instance];
        instanceBufferData[instance] = pInstance->m_boundingSphere;
    }

    m_instanceBuffer->Unmap(0, &instanceBufferRange);
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializeConstantBuffers()
// Desc: Initialize constant buffers
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializeConstantBuffers()
{
    auto maxFramesInFlight = m_deviceResources->GetBackBufferCount();
    auto device = m_deviceResources->GetD3DDevice();

    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    {
        auto constantBufferTransformDesc = CD3DX12_RESOURCE_DESC::Buffer(
            maxFramesInFlight * m_maxInstances
            * sizeof(ConstantBufferTransform)                   // UINT64 width,
                                                                    // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                    // UINT64 alignment = 0 )
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferTransformDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_constantBufferTransform)));
    }

    {
        auto constantBufferTintDesc = CD3DX12_RESOURCE_DESC::Buffer(
            maxFramesInFlight * m_maxInstances
            * sizeof(ConstantBufferTint)                        // UINT64 width,
                                                                    // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                    // UINT64 alignment = 0 )
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferTintDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_constantBufferTint)));
    }

    {
        auto constantBufferFrustumDesc = CD3DX12_RESOURCE_DESC::Buffer(
            maxFramesInFlight
            * sizeof(ConstantBufferFrustum)                     // UINT64 width,
                                                                    // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                    // UINT64 alignment = 0 )
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferFrustumDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_constantBufferFrustum)));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializeRootSignatures()
// Desc: Initialize root signature
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializeRootSignatures()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Mesh
    {
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

        CD3DX12_ROOT_PARAMETER rootSignatureElements[2];
        rootSignatureElements[0].InitAsConstantBufferView(
            0,                                                      // UINT shaderRegister,
            0,                                                      // UINT registerSpace = 0,
            D3D12_SHADER_VISIBILITY_VERTEX);                       // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
        rootSignatureElements[1].InitAsConstantBufferView(
            1,                                                      // UINT shaderRegister,
            0,                                                      // UINT registerSpace = 0,
            D3D12_SHADER_VISIBILITY_PIXEL);                        // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL

        descRootSignature.Init(_countof(rootSignatureElements),
            rootSignatureElements,
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* serializedRootSignature = nullptr;

        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, nullptr));

        DX::ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_GRAPHICS_PPV_ARGS(&m_rootSignatureMesh)));
    }

    // Cull
    {
        CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

        D3D12_DESCRIPTOR_RANGE DescriptorRanges[1] =
        {
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV,                        // D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
            1,                                                      // UINT NumDescriptors;
            0,                                                      // UINT BaseShaderRegister;
            0,                                                      // UINT RegisterSpace;
            0,                                                      // UINT OffsetInDescriptorsFromTableStart;
        };

        CD3DX12_ROOT_PARAMETER rootSignatureElements[4];
        rootSignatureElements[0].InitAsConstantBufferView(
            0                                                       // UINT shaderRegister,
                                                                    // UINT registerSpace = 0,
        );                                                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
        rootSignatureElements[1].InitAsShaderResourceView(
            0                                                       // UINT shaderRegister,
                                                                    // UINT registerSpace = 0,
        );                                                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
        rootSignatureElements[2].InitAsShaderResourceView(
            1                                                       // UINT shaderRegister,
                                                                    // UINT registerSpace = 0,
        );                                                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
        rootSignatureElements[3].InitAsDescriptorTable(
            1,                                                      // UINT numDescriptorRanges,
            DescriptorRanges                                        // _In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE* pDescriptorRanges,
        );                                                      // D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)

        descRootSignature.Init(_countof(rootSignatureElements),
            rootSignatureElements);

        ID3DBlob* serializedRootSignature = nullptr;

        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, nullptr));

        DX::ThrowIfFailed(device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_GRAPHICS_PPV_ARGS(&m_rootSignatureCull)));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializePipelineStates()
// Desc: Initialize pipeline states
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializePipelineStates()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Mesh
    {
        auto vertexShader = DX::ReadData(L"MeshVs.cso");
        auto geometryShader = DX::ReadData(L"MeshGs.cso");
        auto pixelShader = DX::ReadData(L"MeshPs.cso");

        D3D12_INPUT_ELEMENT_DESC descInputElement[] =
        {
            {
                "POSITION",                                     // LPCSTR SemanticName;
                0,                                              // UINT SemanticIndex;
                DXGI_FORMAT_R32G32B32_FLOAT,                    // DXGI_FORMAT Format;
                0,                                              // UINT InputSlot;
                0,                                              // UINT AlignedByteOffset;
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     // D3D12_INPUT_CLASSIFICATION InputSlotClass;
                0,                                              // UINT InstanceDataStepRate;
            },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
        {
            m_rootSignatureMesh,                                                                   // ID3D12RootSignature* pRootSignature;
        {
            vertexShader.data(),
            vertexShader.size(),
        },                                                                                          // D3D12_SHADER_BYTECODE VS;
            {
                pixelShader.data(),
                pixelShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE PS;
        { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE DS;
        { nullptr, 0, },                                                                            // D3D12_SHADER_BYTECODE HS;
            {
                geometryShader.data(),
                geometryShader.size(),
            },                                                                                          // D3D12_SHADER_BYTECODE GS;
        {},                                                                                         // D3D12_STREAM_OUTPUT_DESC StreamOutput;
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),                                                          // D3D12_BLEND_DESC BlendState;
        0xffffffff,                                                                                 // UINT SampleMask;
        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),                                                     // D3D12_RASTERIZER_DESC RasterizerState;
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),                                                  // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
        {
            descInputElement,                                                                       // _Field_size_full_(NumElements) const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs;
            _countof(descInputElement),                                                             // UINT NumElements;
        },                                                                                          // D3D12_INPUT_LAYOUT_DESC InputLayout;
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,                                                // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,                                                     // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
        1,                                                                                          // UINT NumRenderTargets;
        { m_deviceResources->GetBackBufferFormat(), },                                              // DXGI_FORMAT RTVFormats[8];
        m_deviceResources->GetDepthBufferFormat(),                                                  // DXGI_FORMAT DSVFormat;
        { 1, 0, },                                                                                  // DXGI_SAMPLE_DESC SampleDesc;
        0,                                                                                          // UINT NodeMask;
        { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
        D3D12_PIPELINE_STATE_FLAG_NONE,                                                             // D3D12_GRAPHICS_PIPELINE_STATE_FLAGS Flags;
        };
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(&m_pipelineStateMesh)));
    }

    // Cull
    {
        auto computeShader = DX::ReadData(L"CullCs.cso");

        D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
        {
            m_rootSignatureCull,                                                                       // ID3D12RootSignature* pRootSignature;
        {
            computeShader.data(),
            computeShader.size(),
        },                                                                                          // D3D12_SHADER_BYTECODE CS;
        0,                                                                                          // UINT NodeMask;
        { nullptr, 0, },                                                                            // D3D12_CACHED_PIPELINE_STATE CachedPSO;
        };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(&m_pipelineStateCull)));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------
// Name: InitializeIndirectParameters()
// Desc: Initialize parameters for ExecuteIndirect
//------------------------------------------------------------------------------------------------------------------------------------
void Sample::InitializeIndirectParameters()
{
    auto maxFramesInFlight = m_deviceResources->GetBackBufferCount();
    auto device = m_deviceResources->GetD3DDevice();

    // Create command signature with two SetGraphicsRootConstantBufferView calls followed by a DrawIndexedInstanced
    D3D12_INDIRECT_ARGUMENT_DESC descIndirectArgument[] =
    {
        { D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW, 0 },
        { D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW, 1 },
        { D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED },
    };
    D3D12_COMMAND_SIGNATURE_DESC descCommandSignature =
    {
        sizeof(s_IndirectArgs),                               // UINT ByteStride; 
        _countof(descIndirectArgument),                       // UINT NumArgumentDescs;
        descIndirectArgument,                                   // const D3D12_INDIRECT_ARGUMENT_DESC* pArgumentDescs;
                                                                // UINT NodeMask;
    };
    DX::ThrowIfFailed(device->CreateCommandSignature(&descCommandSignature, m_rootSignatureMesh, IID_GRAPHICS_PPV_ARGS(&m_commandSignature)));

    // Create indirect args buffers
    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_HEAP_PROPERTIES readbackHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

    for (auto indirectBuffer = 0U; indirectBuffer < INDIRECT_BUFFER_COUNT; ++indirectBuffer)
    {
        auto heapProperties = defaultHeapProperties;
        auto initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        auto resourceFlags = D3D12_RESOURCE_FLAG_NONE;

        switch (indirectBuffer)
        {
        case INDIRECT_BUFFER_UPLOAD:
            heapProperties = uploadHeapProperties;
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            break;

        case INDIRECT_BUFFER_PRE_CULL:
            heapProperties = defaultHeapProperties;
            initialState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER;
            break;

        case INDIRECT_BUFFER_POST_CULL:
            heapProperties = defaultHeapProperties;
            initialState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER;
            break;

        }

        auto argumentBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            maxFramesInFlight * m_maxInstances
            * sizeof(s_IndirectArgs),                             // UINT64 width,
            resourceFlags                                           // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                    // UINT64 alignment = 0 )
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &argumentBufferDesc,
            initialState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_argumentBuffer[indirectBuffer])));

        auto countBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            maxFramesInFlight * sizeof(s_IndirectCount),       // UINT64 width,
            resourceFlags                                           // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                    // UINT64 alignment = 0 )
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &countBufferDesc,
            initialState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(&m_countBuffer[indirectBuffer])));
    }

    m_argumentBuffer[INDIRECT_BUFFER_UPLOAD]->SetName(L"m_argumentBuffer[INDIRECT_BUFFER_UPLOAD]");
    m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL]->SetName(L"m_argumentBuffer[INDIRECT_BUFFER_PRE_CULL]");
    m_argumentBuffer[INDIRECT_BUFFER_POST_CULL]->SetName(L"m_argumentBuffer[INDIRECT_BUFFER_POST_CULL]");
    m_countBuffer[INDIRECT_BUFFER_UPLOAD]->SetName(L"m_countBuffer[INDIRECT_BUFFER_UPLOAD]");
    m_countBuffer[INDIRECT_BUFFER_PRE_CULL]->SetName(L"m_countBuffer[INDIRECT_BUFFER_PRE_CULL]");
    m_countBuffer[INDIRECT_BUFFER_POST_CULL]->SetName(L"m_countBuffer[INDIRECT_BUFFER_POST_CULL]");

    auto countReadbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(s_IndirectCount),                              // UINT64 width,
        D3D12_RESOURCE_FLAG_NONE                                // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                // UINT64 alignment = 0 )
    );
    DX::ThrowIfFailed(device->CreateCommittedResource(&readbackHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &countReadbackBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(&m_countReadbackBuffer)));

    // Create UAVs, descriptors, and descriptor table for post-cull argument and count buffers
    auto numDescriptors = 2U;
    D3D12_DESCRIPTOR_HEAP_DESC descCPUDescriptorHeap =
    {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,                 // D3D12_DESCRIPTOR_HEAP_TYPE Type;
        numDescriptors,                                        // UINT NumDescriptors;
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                        // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
        0,                                                      // UINT NodeMask;
    };
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&descCPUDescriptorHeap, IID_GRAPHICS_PPV_ARGS(&m_indirectArgsCPUDescriptorHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC descGPUDescriptorHeap =
    {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,                 // D3D12_DESCRIPTOR_HEAP_TYPE Type;
        numDescriptors,                                        // UINT NumDescriptors;
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,              // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
        0,                                                      // UINT NodeMask;
    };
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&descGPUDescriptorHeap, IID_GRAPHICS_PPV_ARGS(&m_indirectArgsGPUDescriptorHeap)));

    D3D12_CPU_DESCRIPTOR_HANDLE indirectArgsCPUHeapStart = m_indirectArgsCPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE indirectArgsGPUHeapStart = m_indirectArgsGPUDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    UINT32 handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_argumentDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(indirectArgsCPUHeapStart, 0, handleIncrementSize);
    m_argumentDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(indirectArgsGPUHeapStart, 0, handleIncrementSize);
    m_countDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(indirectArgsCPUHeapStart, 1, handleIncrementSize);
    m_countDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(indirectArgsGPUHeapStart, 1, handleIncrementSize);

    D3D12_UNORDERED_ACCESS_VIEW_DESC descArgumentUAV =
    {
        DXGI_FORMAT_UNKNOWN,
        D3D12_UAV_DIMENSION_BUFFER,
        {
            0,                                  // UINT64 FirstElement;
            maxFramesInFlight * m_maxInstances, // UINT NumElements;
            sizeof(s_IndirectArgs),             // UINT StructureByteStride;
            0,                                  // UINT64 CounterOffsetInBytes;
            D3D12_BUFFER_UAV_FLAG_NONE,         // D3D12_BUFFER_UAV_FLAGS Flags;
        },                                      // D3D12_BUFFER_UAV Buffer;
    };
    device->CreateUnorderedAccessView(
        m_argumentBuffer[INDIRECT_BUFFER_POST_CULL],
        m_countBuffer[INDIRECT_BUFFER_POST_CULL],
        &descArgumentUAV,
        m_argumentDescriptorCPU);

    D3D12_UNORDERED_ACCESS_VIEW_DESC descCountUAV =
    {
        DXGI_FORMAT_UNKNOWN,
        D3D12_UAV_DIMENSION_BUFFER,
        {
            0,                          // UINT64 FirstElement;
            maxFramesInFlight,          // UINT NumElements;
            sizeof(s_IndirectCount),    // UINT StructureByteStride;
            0,                          // UINT64 CounterOffsetInBytes;
            D3D12_BUFFER_UAV_FLAG_NONE, // D3D12_BUFFER_UAV_FLAGS Flags;
        },                              // D3D12_BUFFER_UAV Buffer;
    };
    device->CreateUnorderedAccessView(
        m_countBuffer[INDIRECT_BUFFER_POST_CULL],
        nullptr,
        &descCountUAV,
        m_countDescriptorCPU);

    device->CopyDescriptorsSimple(numDescriptors, m_indirectArgsGPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), indirectArgsCPUHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Populate pre-cull indirect args buffers
    s_IndirectArgs* indirectArguments = nullptr;
    D3D12_RANGE argumentRange =
    {
        0,                                                              // SIZE_T Begin;
        maxFramesInFlight * m_maxInstances * sizeof(*indirectArguments),  // SIZE_T End; // One past end, so (End - Begin) = Size
    };

    DX::ThrowIfFailed(m_argumentBuffer[INDIRECT_BUFFER_UPLOAD]->Map(0, &nullRange, reinterpret_cast<void**>(&indirectArguments)));

    for (auto frame = 0U; frame < maxFramesInFlight; ++frame)
    {
        for (auto instance = 0U; instance < m_maxInstances; ++instance)
        {
            auto mesh = m_instances[instance].m_mesh;
            auto slot = frame * m_maxInstances + instance;

            indirectArguments[slot].m_constantBuffer0 = m_constantBufferTransform->GetGPUVirtualAddress() + slot * sizeof(ConstantBufferTransform);
            indirectArguments[slot].m_constantBuffer1 = m_constantBufferTint->GetGPUVirtualAddress() + slot * sizeof(ConstantBufferTint);
            indirectArguments[slot].m_drawIndexedArgs.BaseVertexLocation = m_vertexBufferStart[mesh];
            indirectArguments[slot].m_drawIndexedArgs.IndexCountPerInstance = m_indexCountInElements[mesh];
            indirectArguments[slot].m_drawIndexedArgs.InstanceCount = 1;
            indirectArguments[slot].m_drawIndexedArgs.StartIndexLocation = m_indexBufferStart[mesh];
            indirectArguments[slot].m_drawIndexedArgs.StartInstanceLocation = 0;
        }
    }

    m_argumentBuffer[INDIRECT_BUFFER_UPLOAD]->Unmap(0, &argumentRange);

    s_IndirectCount* indirectCount = nullptr;
    D3D12_RANGE countRange =
    {
        0,
        maxFramesInFlight * sizeof(*indirectCount)
    };

    DX::ThrowIfFailed(m_countBuffer[INDIRECT_BUFFER_UPLOAD]->Map(0, &nullRange, reinterpret_cast<void**>(&indirectCount)));

    for (auto frame = 0U; frame < maxFramesInFlight; ++frame)
    {
        indirectCount[frame] = m_maxInstances;
    }

    m_countBuffer[INDIRECT_BUFFER_UPLOAD]->Unmap(0, &countRange);
}
#pragma endregion
