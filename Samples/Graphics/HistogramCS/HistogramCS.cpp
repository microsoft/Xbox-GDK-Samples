//--------------------------------------------------------------------------------------
// HistogramCS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HistogramCS.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const XMVECTORF32 c_eye = { -20.f, 10.f, -20.f };
    constexpr float c_pitch = -0.432353f;
    constexpr float c_yaw = -0.988740f;

    constexpr uint32_t c_numBins = 64;
    constexpr size_t c_maxBins = 256;

    enum CSRootParameters : uint32_t
    {
        CSRootParameterCB = 0,
        CSRootParameterSRV,
        CSRootParameterUAV,
    };

    struct CSConstantBuffer
    {
        uint32_t width;
        uint32_t height;
        uint32_t pad[2];
    };

    static_assert((sizeof(CSConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");

    struct VizConstantBuffer
    {
        uint32_t numBins;
        float    scale;
        uint32_t pad[2];
    };

    static_assert((sizeof(VizConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");

    const wchar_t* c_TechniqueNames[] =
    {
        L"[A] Brute force atomics, uncoalesced",
        L"[A] Brute force atomics, coalesced",
        L"[A] TGSM Scanline, uncoalesced",
        L"[A] TGSM Scanline, coalesced",
    };

    static_assert(_countof(c_TechniqueNames) == Sample::HistCount, "Mismatched length of technique names");
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_technique(HistTGSM)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        2,
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
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            int count = static_cast<int>(m_technique);
            ++count;
            if (count >= HistCount)
                count = 0;
            m_technique = static_cast<HistogramTechnique>(count);
        }

        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch -= pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    constexpr float limit = XM_PI / 2.0f - 0.01f;
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
        cosf(m_yaw),
        m_pitch,
        0);

    m_view = XMMatrixLookToLH(c_eye, lookAt, g_XMNegIdentityR2);
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

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    m_gpuTimer.BeginFrame(commandList);

    RenderScene(commandList);

    // Compute histogram
    {
        auto const backBuffer = m_deviceResources->GetRenderTarget();

        ScopedBarrier scopeBarrier(commandList,
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_histogram.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
            });

        UINT values[4] = {};
        commandList->ClearUnorderedAccessViewUint(m_resourceDescriptors->GetGpuHandle(UAV_Histogram), m_cpuDescriptors->GetFirstCpuHandle(),
            m_histogram.Get(), values, 0, nullptr);

        m_gpuTimer.Start(commandList);

        auto const vp = m_deviceResources->GetScreenViewport();

        auto height = static_cast<uint32_t>(vp.Height);

        CSConstantBuffer cb = { static_cast<uint32_t>(vp.Width), height, 0, 0 };
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb);

        commandList->SetComputeRootSignature(m_csRootSig.Get());

        commandList->SetComputeRootConstantBufferView(CSRootParameterCB, cbHandle.GpuAddress());
        commandList->SetComputeRootDescriptorTable(CSRootParameterSRV,
            m_resourceDescriptors->GetGpuHandle(Backbuffer0 + m_deviceResources->GetCurrentFrameIndex()));
        commandList->SetComputeRootDescriptorTable(CSRootParameterUAV, m_resourceDescriptors->GetGpuHandle(UAV_Histogram));

        commandList->SetPipelineState(m_histogramCS[m_technique].Get());

        commandList->Dispatch(height, 1, 1);

        m_gpuTimer.Stop(commandList);
    }

    // Visualize the histogram
    auto const vp = m_deviceResources->GetScreenViewport();

    {
        D3D12_VIEWPORT vizvp =
        {
            vp.TopLeftX + vp.Width / 2.f,
            vp.TopLeftY + vp.Height / 2.f,
            vp.Width / 2.f,
            vp.Height / 2.f,
            vp.MinDepth,
            vp.MaxDepth
        };

        commandList->RSSetViewports(1, &vizvp);

        // Scale here is arbitrary to make the display reasonable.
        VizConstantBuffer cb = { c_numBins, 512000, 0, 0 };
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb);

        m_fullScreenQuad->Draw(commandList, m_visualizeHistogram.Get(), m_resourceDescriptors->GetGpuHandle(SRV_Histogram), cbHandle.GpuAddress());
    }

    // Render UI
    {
        commandList->RSSetViewports(1, &vp);
        RenderUI(commandList);
    }

    m_gpuTimer.EndFrame(commandList);
    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render scene");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_modelResources->Heap(),
        m_states->Heap()
    };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(descriptorHeaps)), descriptorHeaps);

    Model::UpdateEffectMatrices(m_modelEffects, SimpleMath::Matrix::Identity, m_view, m_proj);

    m_model->Draw(commandList, m_modelEffects.cbegin());

    PIXEndEvent(commandList);
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

    m_batch->Begin(commandList);

    float y = float(safe.top);

    m_smallFont->DrawString(m_batch.get(), L"Histogram CS",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_smallFont->GetLineSpacing() * 1.5f;

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_colorCtrlFont.get(),
        c_TechniqueNames[m_technique],
        XMFLOAT2(float(safe.left), y),
        ATG::Colors::Orange);

    y += m_smallFont->GetLineSpacing() * 1.5f;

    wchar_t buff[64] = {};
    swprintf_s(buff, L"Technique time: %.2fms", m_gpuTimer.GetElapsedMS());
    m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[View] Exit   [LThumb] Rotate",
        XMFLOAT2(float(safe.left), float(safe.bottom)),
        ATG::Colors::LightGrey);

    m_batch->End();

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

    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());

    m_states = std::make_unique<CommonStates>(device);

    m_model = Model::CreateFromSDKMESH(device, L"FPSRoom.sdkmesh");

    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_dsvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_cpuDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);   

    ResourceUploadBatch upload(device);
    upload.Begin();

    m_modelResources = m_model->LoadTextures(device, upload);

    m_fxFactory = std::make_unique<EffectFactory>(m_modelResources->Heap(), m_states->Heap());

    m_model->LoadStaticBuffers(device, upload);

    {
        const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Create scene effects.
    {
        EffectPipelineStateDescription pdOpaque(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        EffectPipelineStateDescription pdAlpha(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        m_modelEffects = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);
    }

    // Create compute shader objects.
    {
        auto csBlob = DX::ReadData(L"HistogramCS_Brute.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_csRootSig.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_csRootSig.Get();
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_histogramCS[HistBrute].ReleaseAndGetAddressOf()))
        );

        m_histogramCS[HistBrute]->SetName(L"Histogram (Brute)");

        csBlob = DX::ReadData(L"HistogramCS_GSM.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_histogramCS[HistTGSM].ReleaseAndGetAddressOf()))
        );

        m_histogramCS[HistTGSM]->SetName(L"Histogram (GSM)");

        csBlob = DX::ReadData(L"HistogramCS_U.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_histogramCS[HistUncoalescedBrute].ReleaseAndGetAddressOf()))
        );

        m_histogramCS[HistUncoalescedBrute]->SetName(L"Histogram (Uncoalesced)");

        csBlob = DX::ReadData(L"HistogramCS_GSM_U.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_histogramCS[HistTGSMUncoalesced].ReleaseAndGetAddressOf()))
        );

        m_histogramCS[HistTGSMUncoalesced]->SetName(L"Histogram (GSM Uncoalesced)");
    }

    {
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(c_maxBins * sizeof(UINT),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_histogram.ReleaseAndGetAddressOf()))
        );

        m_histogram->SetName(L"Histogram Buffer");
    }

    {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Format = DXGI_FORMAT_R32_UINT;
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Buffer.NumElements = c_maxBins;
        device->CreateShaderResourceView(m_histogram.Get(), &desc, m_resourceDescriptors->GetCpuHandle(Descriptors::SRV_Histogram));
    }

    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_R32_UINT;
        desc.Buffer.NumElements = c_maxBins;
        device->CreateUnorderedAccessView(m_histogram.Get(), nullptr, &desc, m_resourceDescriptors->GetCpuHandle(Descriptors::UAV_Histogram));

        // Second UAV needed for clear operation.
        device->CreateUnorderedAccessView(m_histogram.Get(), nullptr, &desc, m_cpuDescriptors->GetFirstCpuHandle());
    }

    // Histogram visualization
    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);

        auto pixelShaderBlob = DX::ReadData(L"RenderHistogramPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_visualizeHistogram.ReleaseAndGetAddressOf());
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

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

    m_colorCtrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"XboxOneController.spritefont" : L"XboxOneControllerSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ColorCtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ColorCtrlFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 10000.f);

    // Create SRVs for backbuffers
    for (size_t j = 0; j < m_deviceResources->GetBackBufferCount(); ++j)
    {
        device->CreateShaderResourceView(m_deviceResources->GetRenderTarget(j), nullptr, m_resourceDescriptors->GetCpuHandle(Backbuffer0 + j));
    }
}
#pragma endregion
