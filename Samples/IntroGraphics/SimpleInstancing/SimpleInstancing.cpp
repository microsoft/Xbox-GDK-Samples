//--------------------------------------------------------------------------------------
// SimpleInstancing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleInstancing.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    //--------------------------------------------------------------------------------------
    // Constants
    //--------------------------------------------------------------------------------------
    constexpr uint32_t c_maxInstances = 20000;
    constexpr uint32_t c_startInstanceCount = 5000;
    constexpr uint32_t c_minInstanceCount = 1000;
    constexpr float    c_boxBounds = 60.0f;
    constexpr size_t   c_cubeIndexCount = 36;
    constexpr float    c_velocityMultiplier = 500.0f;

    //--------------------------------------------------------------------------------------
    // Cube vertex definition
    //--------------------------------------------------------------------------------------
    struct Vertex
    {
        XMFLOAT3 pos;
        XMFLOAT3 norm;
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_mappedInstanceData(nullptr),
    m_instanceDataGpuAddr(0),
    m_usedInstanceCount(c_startInstanceCount),
    m_lights{},
    m_pitch(0.0f),
    m_yaw(0.0f)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
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
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!m_fenceEvent.IsValid())
    {
        throw std::exception("CreateEvent");
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            ResetSimulation();
        }

        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_usedInstanceCount = std::min(c_maxInstances, m_usedInstanceCount + 1000);
        }
        else if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_usedInstanceCount = std::max(c_minInstanceCount, m_usedInstanceCount - 1000);
        }

        if (pad.IsLeftStickPressed())
        {
            m_yaw = m_pitch = 0.f;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch += pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    const float limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-limit, std::min(+limit, m_pitch));

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    XMVECTOR lookAt = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    // Update transforms.
    XMMATRIX camera = XMMatrixLookAtLH(g_XMZero, lookAt, g_XMIdentityR1);
    m_view = XMMatrixTranspose(XMMatrixMultiply(camera, m_proj));

    // Update instance data for the next frame.
    for (size_t i = 1; i < m_usedInstanceCount; ++i)
    {
        // Update positions...
        float velocityMultiplier = i <= c_pointLightCount ? 5.0f * c_velocityMultiplier : c_velocityMultiplier;
        XMVECTOR position = XMLoadFloat4(&m_CPUInstanceData[i].positionAndScale);
        position = XMVectorAdd(position, XMVectorScale(m_velocities[i], elapsedTime * velocityMultiplier));
        XMStoreFloat4(&m_CPUInstanceData[i].positionAndScale, position);

        float X = m_CPUInstanceData[i].positionAndScale.x;
        float Y = m_CPUInstanceData[i].positionAndScale.y;
        float Z = m_CPUInstanceData[i].positionAndScale.z;

        bool bounce = false;

        // If an instance pops out of bounds in any dimension, reverse velocity in that dimension...
        if (X < -c_boxBounds || X > c_boxBounds)
        {
            m_velocities[i] = XMVectorMultiply(m_velocities[i], XMVectorSet(-1.0f, 1.0f, 1.0f, 1.0f));
            bounce = true;
        }
        if (Y < -c_boxBounds || Y > c_boxBounds)
        {
            m_velocities[i] = XMVectorMultiply(m_velocities[i], XMVectorSet(1.0f, -1.0f, 1.0f, 1.0f));
            bounce = true;
        }
        if (Z < -c_boxBounds || Z > c_boxBounds)
        {
            m_velocities[i] = XMVectorMultiply(m_velocities[i], XMVectorSet(1.0f, 1.0f, -1.0f, 1.0f));
            bounce = true;
        }

        // Apply bounce here.
        if (bounce)
        {
            position = XMLoadFloat4(&m_CPUInstanceData[i].positionAndScale);
            position = XMVectorAdd(position, XMVectorScale(m_velocities[i], elapsedTime * c_velocityMultiplier));
            XMStoreFloat4(&m_CPUInstanceData[i].positionAndScale, position);
        }

        // Set up point light info.
        if (i <= c_pointLightCount)
        {
            m_lights.pointPositions[i - 1] = m_CPUInstanceData[i].positionAndScale;
        }

        XMVECTOR q = XMLoadFloat4(&m_CPUInstanceData[i].quaternion);
        q = XMQuaternionNormalizeEst(XMQuaternionMultiply(m_rotationQuaternions[i], q));
        XMStoreFloat4(&m_CPUInstanceData[i].quaternion, q);
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

    // Check to see if the GPU is keeping up
    auto const frameIdx = m_deviceResources->GetCurrentFrameIndex();
    auto const numBackBuffers = m_deviceResources->GetBackBufferCount();
    const uint64_t completedValue = m_fence->GetCompletedValue();
    if ((frameIdx > completedValue) // if frame index is reset to zero it may temporarily be smaller than the last GPU signal
        && (frameIdx - completedValue > numBackBuffers))
    {
        // GPU not caught up, wait for at least one available frame
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(frameIdx - numBackBuffers, m_fenceEvent.Get()));
        WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_pipelineState.Get());

    // We use the DirectX Tool Kit helper for managing constants memory
    // (see SimpleLighting12 for how to provide constants without this helper)
    auto vertexConstants = m_graphicsMemory->AllocateConstant<XMFLOAT4X4>(m_view);
    auto pixelConstants = m_graphicsMemory->AllocateConstant<Lights>(m_lights);

    commandList->SetGraphicsRootConstantBufferView(0, vertexConstants.GpuAddress());
    commandList->SetGraphicsRootConstantBufferView(1, pixelConstants.GpuAddress());

    // Set necessary state.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Provide per-frame instance data
    uint32_t instanceIdx = (frameIdx % numBackBuffers);
    uint32_t frameOffset = (c_maxInstances * sizeof(Instance)) * instanceIdx;

    memcpy(m_mappedInstanceData + frameOffset, m_CPUInstanceData.get(), sizeof(Instance) * m_usedInstanceCount);

    m_vertexBufferView[1].BufferLocation = m_instanceDataGpuAddr + frameOffset;
    m_vertexBufferView[1].StrideInBytes = sizeof(Instance);
    m_vertexBufferView[1].SizeInBytes = sizeof(Instance) * m_usedInstanceCount;

    // Set up the vertex buffers. We have 3 streams:
    // Stream 1 contains per-primitive vertices defining the cubes.
    // Stream 2 contains the per-instance data for scale, position and orientation
    // Stream 3 contains the per-instance data for color.
    commandList->IASetVertexBuffers(0, _countof(m_vertexBufferView), m_vertexBufferView);

    // The per-instance data is referenced by index...
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Draw the entire scene...
    commandList->DrawIndexedInstanced(c_cubeIndexCount, m_usedInstanceCount, 0, 0, 0);

    // Draw UI.
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    auto size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    m_batch->Begin(commandList);

    wchar_t str[32] = {};
    swprintf_s(str, L"Instancing count: %u", m_usedInstanceCount);
    m_smallFont->DrawString(m_batch.get(), str, XMFLOAT2(float(safe.left), float(safe.top)), ATG::Colors::White);

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[LThumb] Rotate   [A] Reset   [LB]/[RB] Change instance count   [View] Exit",
        XMFLOAT2(float(safe.left),
            float(safe.bottom) - m_smallFont->GetLineSpacing()),
        ATG::Colors::LightGrey);

    m_batch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    // GPU will signal an increasing value each frame
    m_deviceResources->GetCommandQueue()->Signal(m_fence.Get(), frameIdx);

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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    {
        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

        SpriteBatchPipelineStateDescription pd(rtState);

        m_batch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
    }

    // Create a root signature.
    auto const vertexShaderBlob = DX::ReadData(L"VertexShader.cso");

    // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.

    DX::ThrowIfFailed(
        device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    // Create the pipeline state, which includes loading shaders.
    auto const pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

    static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[] =
    {
        // SemanticName SemanticIndex   Format                          InputSlot AlignedByteOffset             InputSlotClass                                  InstanceDataStepRate
        { "POSITION",   0,              DXGI_FORMAT_R32G32B32_FLOAT,    0,        0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     0 },  // Vertex local position
        { "NORMAL",     0,              DXGI_FORMAT_R32G32B32_FLOAT,    0,        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,     0 },  // Vertex normal
        { "I_ROTATION", 0,              DXGI_FORMAT_R32G32B32A32_FLOAT, 1,        0,                            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,   1 },  // Instance rotation quaternion
        { "I_POSSCALE", 0,              DXGI_FORMAT_R32G32B32A32_FLOAT, 1,        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,   1 },  // Instance position and scale (scale in "w")
        { "I_COLOR",    0,              DXGI_FORMAT_R8G8B8A8_UNORM,     2,        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,   1 },  // Instance color
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
    psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.SampleDesc.Count = 1;
    DX::ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));

    const CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);

    // Create and initialize the vertex buffer defining a cube.
    {
        static const Vertex s_vertexData[] =
        {
            { XMFLOAT3(-1,  -1,  -1), XMFLOAT3(0,    0,  -1) },
            { XMFLOAT3(1,  -1,  -1), XMFLOAT3(0,    0,  -1) },
            { XMFLOAT3(1,   1,  -1), XMFLOAT3(0,    0,  -1) },
            { XMFLOAT3(-1,   1,  -1), XMFLOAT3(0,    0,  -1) },    // Z negative face

            { XMFLOAT3(1,  -1,   1), XMFLOAT3(0,    0,   1) },
            { XMFLOAT3(-1,  -1,   1), XMFLOAT3(0,    0,   1) },
            { XMFLOAT3(-1,   1,   1), XMFLOAT3(0,    0,   1) },
            { XMFLOAT3(1,   1,   1), XMFLOAT3(0,    0,   1) },    // Z Positive face

            { XMFLOAT3(-1,  -1,  -1), XMFLOAT3(-1,   0,   0) },
            { XMFLOAT3(-1,   1,  -1), XMFLOAT3(-1,   0,   0) },
            { XMFLOAT3(-1,   1,   1), XMFLOAT3(-1,   0,   0) },
            { XMFLOAT3(-1,  -1,   1), XMFLOAT3(-1,   0,   0) },    // X negative face

            { XMFLOAT3(1,   1,  -1), XMFLOAT3(1,   0,   0) },
            { XMFLOAT3(1,  -1,  -1), XMFLOAT3(1,   0,   0) },
            { XMFLOAT3(1,  -1,   1), XMFLOAT3(1,   0,   0) },
            { XMFLOAT3(1,   1,   1), XMFLOAT3(1,   0,   0) },    // X Positive face

            { XMFLOAT3(-1,  -1,   1), XMFLOAT3(0,   -1,   0) },
            { XMFLOAT3(1,  -1,   1), XMFLOAT3(0,   -1,   0) },
            { XMFLOAT3(1,  -1,  -1), XMFLOAT3(0,   -1,   0) },
            { XMFLOAT3(-1,  -1,  -1), XMFLOAT3(0,   -1,   0) },    // Y negative face

            { XMFLOAT3(1,   1,   1), XMFLOAT3(0,    1,   0) },
            { XMFLOAT3(-1,   1,   1), XMFLOAT3(0,    1,   0) },
            { XMFLOAT3(-1,   1,  -1), XMFLOAT3(0,    1,   0) },
            { XMFLOAT3(1,   1,  -1), XMFLOAT3(0,    1,   0) },    // Y Positive face
        };

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_vertexData));

        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapUpload,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf())));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(
            m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, s_vertexData, sizeof(s_vertexData));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView[0].BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView[0].StrideInBytes = sizeof(Vertex);
        m_vertexBufferView[0].SizeInBytes = sizeof(s_vertexData);
    }

    // Create vertex buffer memory for per-instance data.
    {
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t cbSize = c_maxInstances * m_deviceResources->GetBackBufferCount() * sizeof(Instance);

        const D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &instanceBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_instanceData.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_instanceData->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedInstanceData)));

        m_instanceDataGpuAddr = m_instanceData->GetGPUVirtualAddress();
    }

    // Create a static vertex buffer with color data.
    {
        static const XMVECTORF32 s_bigCubeColor = { 1.f, 1.f, 1.f, 0.f };
        uint32_t colors[c_maxInstances];
        colors[0] = PackedVector::XMCOLOR(s_bigCubeColor);
        for (uint32_t i = 1; i < c_maxInstances; ++i)
        {
            if (i <= c_pointLightCount)
            {
                m_lights.pointColors[i - 1] = XMFLOAT4(FloatRand(0.25f, 1.0f), FloatRand(0.25f, 1.0f), FloatRand(0.25f, 1.0f), 1.0f);
                colors[i] = PackedVector::XMCOLOR(m_lights.pointColors[i - 1].x, m_lights.pointColors[i - 1].y, m_lights.pointColors[i - 1].z, 1.f);
            }
            else
            {
                colors[i] = PackedVector::XMCOLOR(FloatRand(0.25f, 1.0f), FloatRand(0.25f, 1.0f), FloatRand(0.25f, 1.0f), 0.f);
            }
        }

        auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t) * c_maxInstances);

        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapUpload,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_boxColors.ReleaseAndGetAddressOf())));

        // Copy the color data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(
            m_boxColors->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, colors, sizeof(uint32_t) * c_maxInstances);
        m_boxColors->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView[2].BufferLocation = m_boxColors->GetGPUVirtualAddress();
        m_vertexBufferView[2].StrideInBytes = sizeof(uint32_t);
        m_vertexBufferView[2].SizeInBytes = sizeof(uint32_t) * c_maxInstances;
    }

    // Create and initialize the index buffer for the cube geometry.
    {
        static const uint16_t s_indexData[] =
        {
            0,  2,  1,
            0,  3,  2,
            4,  6,  5,
            4,  7,  6,
            8,  10, 9,
            8,  11, 10,
            12, 14, 13,
            12, 15, 14,
            16, 18, 17,
            16, 19, 18,
            20, 22, 21,
            20, 23, 22,
        };

        auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_indexData));

        // See note above
        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapUpload,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_indexBuffer.ReleaseAndGetAddressOf())));

        // Copy the data to the index buffer.
        uint8_t* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(
            m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, s_indexData, sizeof(s_indexData));
        m_indexBuffer->Unmap(0, nullptr);

        // Initialize the index buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = sizeof(s_indexData);
    }

    m_CPUInstanceData.reset(new Instance[c_maxInstances]);
    m_rotationQuaternions.reset(reinterpret_cast<XMVECTOR*>(_aligned_malloc(sizeof(XMVECTOR) * c_maxInstances, 16)));
    m_velocities.reset(reinterpret_cast<XMVECTOR*>(_aligned_malloc(sizeof(XMVECTOR) * c_maxInstances, 16)));

    // Set up the position and scale for the container box. Scale is negative to turn the box inside-out
    // (this effectively reverses the normals and backface culling).
    // Scale the outside box to slightly larger than our scene boundary, so bouncing boxes never actually clip it.
    m_CPUInstanceData[0].positionAndScale = XMFLOAT4(0.0f, 0.0f, 0.0f, -(c_boxBounds + 5));
    m_CPUInstanceData[0].quaternion = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize the directional light.
    XMStoreFloat4(&m_lights.directional, XMVector3Normalize(XMVectorSet(1.0f, 4.0f, -2.0f, 0)));

    // Initialize the positions/state of all the cubes in the scene.
    ResetSimulation();

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    // Create a fence for synchronizing between the CPU and the GPU
    DX::ThrowIfFailed(device->CreateFence(m_deviceResources->GetCurrentFrameIndex(), D3D12_FENCE_FLAG_NONE,
        IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));

    // Start off the fence with the current frame index
    const uint64_t currentIdx = m_deviceResources->GetCurrentFrameIndex();
    m_deviceResources->GetCommandQueue()->Signal(m_fence.Get(), currentIdx);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize the projection matrix.
    auto const size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 500.0f);

    // Set the viewport for our SpriteBatch.
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());
}
#pragma endregion

void Sample::ResetSimulation()
{
    // Reset positions to starting point, and orientations to identity.
    // Note that instance 0 is the scene bounding box, and the position, orientation and scale are static (i.e. never update).
    for (size_t i = 1; i < c_maxInstances; ++i)
    {
        m_CPUInstanceData[i].positionAndScale = XMFLOAT4(0.0f, 0.0f, c_boxBounds / 2.0f, FloatRand(0.1f, 0.4f));
        m_CPUInstanceData[i].quaternion = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

        // For the first c_pointLightCount in the updated array, we scale up by a small factor so they stand out.
        if (i <= c_pointLightCount)
        {
            m_CPUInstanceData[i].positionAndScale.w = 1.53f;
            m_lights.pointPositions[i - 1] = m_CPUInstanceData[i].positionAndScale;
        }

        // Apply a random spin to each instance.
        m_rotationQuaternions[i] = XMQuaternionRotationAxis(XMVector3Normalize(XMVectorSet(FloatRand(), FloatRand(), FloatRand(), 0)), FloatRand(0.001f, 0.1f));

        // ...and a random velocity.
        m_velocities[i] = XMVectorSet(FloatRand(-0.01f, 0.01f), FloatRand(-0.01f, 0.01f), FloatRand(-0.01f, 0.01f), 0);
    }
}

inline float Sample::FloatRand(float lowerBound, float upperBound)
{
    if (lowerBound == upperBound)
        return lowerBound;

    std::uniform_real_distribution<float> dist(lowerBound, upperBound);

    return dist(m_randomEngine);
}
