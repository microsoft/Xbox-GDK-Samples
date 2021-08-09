//--------------------------------------------------------------------------------------
// SimpleMeshShader.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleMeshShader.h"

#include "ATGColors.h"
#include "FindMedia.h"

#pragma warning( disable : 4324 4365 )

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace 
{
    //--------------------------------------
    // Definitions

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

    const wchar_t* s_sampleTitle = L"Simple Mesh Shader";

    const wchar_t* s_meshShaderFilename = L"SimpleTriangleMS.cso";
    const wchar_t* s_pixelShaderFilename = L"SimpleTrianglePS.cso";
}

Sample::Sample() noexcept(false)
    : m_deviceResources(std::make_unique<DX::DeviceResources>())
    , m_frame(0)
{
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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
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
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // Set the root signature, pipeline state, and pertinent state.
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_simpleTriPso.Get());
#ifdef _GAMING_XBOX_SCARLETT
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif

    // Only need one threadgroup to draw a single triangle.
    commandList->DispatchMesh(1, 1, 1);

    DrawHUD(commandList);

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

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    m_hudBatch->Begin(commandList);

    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);

    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    // Draw title.
    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    m_hudBatch->End();
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
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
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
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
    {
        OutputDebugStringA("ERROR: Shader Model 6.5 is not supported\n");
        throw std::exception("Shader Model 6.5 is not supported");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
        || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
    {
        OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
        throw std::exception("Mesh Shaders aren't supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        DescriptorHeapIndex::SRV_Count);

    // Create Mesh Shader pipeline state object
    auto simpleTriMS = DX::ReadData(s_meshShaderFilename);
    auto simpleTriPS = DX::ReadData(s_pixelShaderFilename);

    // Strip the root signature from one of the precompiled shader bytecode.
    DX::ThrowIfFailed(device->CreateRootSignature(0, simpleTriMS.data(), simpleTriMS.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = m_rootSignature.Get();
    psoDesc.MS                    = { simpleTriMS.data(), simpleTriMS.size() };
    psoDesc.PS                    = { simpleTriPS.data(), simpleTriPS.size() };
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = m_deviceResources->GetBackBufferFormat();
    psoDesc.DSVFormat             = m_deviceResources->GetDepthBufferFormat();
    psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.SampleDesc            = DefaultSampleDesc();

    auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
    streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

    DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_simpleTriPso.ReleaseAndGetAddressOf())));


    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);

    // Upload reosurces to GPU resource
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
        m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
        m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

    resourceUpload.End(m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    const auto size = m_deviceResources->GetOutputSize();

    // Calculate display dimensions.
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_srvPile.reset();
    m_rootSignature.Reset();
    m_simpleTriPso.Reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
