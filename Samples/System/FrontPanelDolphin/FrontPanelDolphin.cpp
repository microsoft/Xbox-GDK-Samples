//--------------------------------------------------------------------------------------
// FrontPanelDolphin.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FrontPanelDolphin.h"

#include "ATGColors.h"
#include "ReadData.h"

extern void ExitSample();

using namespace DirectX;
using namespace ATG;
using Microsoft::WRL::ComPtr;

namespace
{
    // Custom effect for sea floor
    class SeaEffect : public IEffect
    {
    public:
        SeaEffect(ID3D12Device* device, DirectX::RenderTargetState rtState)
        {
            auto seaFloorVSBlob = DX::ReadData(L"SeaFloorVS.cso");
            auto causticsPSBlob = DX::ReadData(L"CausticsPS.cso");

            // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.

            DX::ThrowIfFailed(
                device->CreateRootSignature(0, seaFloorVSBlob.data(), causticsPSBlob.size(),
                    IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

            const D3D12_INPUT_ELEMENT_DESC layout[] =
            {
                { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = { layout, _countof(layout) };
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.VS = { seaFloorVSBlob.data(), seaFloorVSBlob.size() };
            psoDesc.PS = { causticsPSBlob.data(), causticsPSBlob.size() };
            psoDesc.RasterizerState = CommonStates::CullNone;
            psoDesc.BlendState = CommonStates::Opaque;
            psoDesc.DepthStencilState = CommonStates::DepthDefault;
            psoDesc.DSVFormat = rtState.dsvFormat;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = rtState.rtvFormats[0];
            psoDesc.SampleDesc.Count = 1;

            DX::ThrowIfFailed(
                device->CreateGraphicsPipelineState(&psoDesc,
                    IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
        }

        void Apply(ID3D12GraphicsCommandList* commandList) override
        {
            // Set the root signature and pipeline state for the command list
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());
        }

    private:

        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineState;
    };
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_currentCausticTextureView(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();

    // Create the dolphins
    for (unsigned int i = 0; i < m_dolphins.size(); i++)
    {
        auto d = std::make_shared<Dolphin>();
        d->Translate(XMVectorSet(0, -1.0f + 2.0f * static_cast<float>(i), 10, 0));
        m_dolphins[i] = d;
    }

    // Set the water color to a nice blue.
    SetWaterColor(0.0f, 0.5f, 1.0f);

    // Set the ambient light.
    m_ambient[0] = 0.25f;
    m_ambient[1] = 0.25f;
    m_ambient[2] = 0.25f;
    m_ambient[3] = 0.25f;

    m_available = XFrontPanelIsSupported();
    if (m_available)
    {
        // Construct the front panel render target
        m_frontPanelRenderTarget = std::make_unique<FrontPanelRenderTarget>();

        // Initialize the FrontPanelDisplay object
        m_frontPanelDisplay = std::make_unique<FrontPanelDisplay>();

        // Initialize the FrontPanelInput object
        m_frontPanelInput = std::make_unique<FrontPanelInput>();
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

    // Update all the dolphins
    for (unsigned int i = 0; i < m_dolphins.size(); i++)
        m_dolphins[i]->Update(totalTime, elapsedTime);

    SetWaterColor(0.0f, 0.5f, 1.0f);

    // Animate the caustic textures
    m_currentCausticTextureView = (static_cast<unsigned int>(totalTime *32)) % 32;

    unsigned int constantBufferIndex = m_deviceResources->GetCurrentFrameIndex() % m_deviceResources->GetBackBufferCount();

    // Fill the VS constants for the sea floor
    {
        // Size of constant buffer is padded to a multiple of D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
        VS_CONSTANT_BUFFER* vertShaderConstData = reinterpret_cast<VS_CONSTANT_BUFFER*>(
            reinterpret_cast<BYTE*>(m_mappedVSConstantData) + 
            AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * constantBufferIndex);

        vertShaderConstData->vZero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        vertShaderConstData->vConstants = XMVectorSet(1.0f, 0.5f, 0.2f, 0.05f);

        // weight is for dolphins, so the value doesn't matter here (since we're setting it for the sea floor)
        vertShaderConstData->vWeight = XMVectorSet(0, 0, 0, 0);

        // Lighting vectors (in world space and in dolphin model space)
        // and other constants
        vertShaderConstData->vLight = XMVectorSet(0.00f, 1.00f, 0.00f, 0.00f);
        vertShaderConstData->vLightDolphinSpace = XMVectorSet(0.00f, 1.00f, 0.00f, 0.00f);
        vertShaderConstData->vDiffuse = XMVectorSet(1.00f, 1.00f, 1.00f, 1.00f);
        vertShaderConstData->vAmbient = XMVectorSet(m_ambient[0], m_ambient[1], m_ambient[2], m_ambient[3]);
        vertShaderConstData->vFog = XMVectorSet(0.50f, 50.00f, 1.00f / (50.0f - 1.0f), 0.00f);
        vertShaderConstData->vCaustics = XMVectorSet(0.05f, 0.05f, sinf(totalTime) / 8, cosf(totalTime) / 10);

        XMVECTOR vDeterminant;
        XMMATRIX matDolphin = XMMatrixIdentity();
        XMMATRIX matDolphinInv = XMMatrixInverse(&vDeterminant, matDolphin);
        vertShaderConstData->vLightDolphinSpace = XMVector4Normalize(XMVector4Transform(vertShaderConstData->vLight, matDolphinInv));

        // Vertex shader operations use transposed matrices
        XMMATRIX mat, matCamera;
        matCamera = XMMatrixMultiply(matDolphin, XMLoadFloat4x4(&m_matView));
        mat = XMMatrixMultiply(matCamera, m_matProj);
        vertShaderConstData->matTranspose = XMMatrixTranspose(mat);
        vertShaderConstData->matCameraTranspose = XMMatrixTranspose(matCamera);
        vertShaderConstData->matViewTranspose = XMMatrixTranspose(m_matView);
        vertShaderConstData->matProjTranspose = XMMatrixTranspose(m_matProj);
    }

    // Fill the PS constants used by all draw calls in this sample
    {
        // Size of constant buffer is padded to a multiple of D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
        PS_CONSTANT_BUFFER* pPsConstData = reinterpret_cast<PS_CONSTANT_BUFFER*>(
            reinterpret_cast<BYTE*>(m_mappedPSConstantData) + 
            AlignUp(sizeof(PS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * constantBufferIndex);

        memcpy(pPsConstData->fAmbient, m_ambient, sizeof(m_ambient));
        memcpy(pPsConstData->fFogColor, m_waterColor, sizeof(m_waterColor));
    }

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

    // Use the front panel select button to capture the front panel display
    if (m_available)
    {
        auto fpInput = m_frontPanelInput->GetState();
        m_frontPanelInputButtons.Update(fpInput);

        using ButtonState = FrontPanelInput::ButtonStateTracker::ButtonState;

        if (m_frontPanelInputButtons.buttonSelect == ButtonState::PRESSED)
        {
            m_frontPanelDisplay->SaveDDSToFile(L"D:\\FrontPanelScreen.dds");
        }
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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto renderTarget = m_deviceResources->GetRenderTarget();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    SetWaterColor(0.0f, 0.5f, 1.0f);

    /////////////////////////////////////////////////////
    //
    // Render sea floor
    //
    ////////////////////////////////////////////////////

    m_seaEffect->Apply(commandList);

    unsigned int constantBufferIndex = m_deviceResources->GetCurrentFrameIndex() % m_deviceResources->GetBackBufferCount();
    commandList->SetGraphicsRootConstantBufferView(0, 
        m_VSConstantDataGpuAddr + AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * constantBufferIndex);
    commandList->SetGraphicsRootConstantBufferView(1, 
        m_PSConstantDataGpuAddr + AlignUp(sizeof(PS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * constantBufferIndex);

    auto pHeap = m_textureFactory->Heap();
    commandList->SetDescriptorHeaps(1, &pHeap);
    commandList->SetGraphicsRootDescriptorTable(2, m_textureFactory->GetGpuDescriptorHandle(TextureDescriptors::SeaFloor));
    commandList->SetGraphicsRootDescriptorTable(3, m_textureFactory->GetGpuDescriptorHandle(TextureDescriptors::CausticFirst + m_currentCausticTextureView));

    assert(!m_seafloor->meshes.empty());
    assert(!m_seafloor->meshes[0]->opaqueMeshParts.empty());
    m_seafloor->meshes[0]->opaqueMeshParts[0]->Draw(commandList);

    /////////////////////////////////////////////////////
    //
    // Render Dolphins  
    //
    ////////////////////////////////////////////////////
    for (unsigned int i = 0; i < m_dolphins.size(); i++)
        DrawDolphin(*m_dolphins[i].get());

    if (m_available)
    {
        // Blit to the Front Panel render target and then present to the Front Panel
        auto device = m_deviceResources->GetD3DDevice();
        unsigned int frameIndex = m_deviceResources->GetCurrentFrameIndex();
        m_frontPanelRenderTarget->GPUBlit(commandList, renderTarget, frameIndex);
        auto fpDesc = m_frontPanelDisplay->GetBufferDescriptor();
        m_frontPanelRenderTarget->CopyToBuffer(device, commandQueue, frameIndex, fpDesc);
        m_frontPanelDisplay->Present();
    }

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
    commandList->ClearRenderTargetView(rtvDescriptor, m_waterColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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
    auto commandQueue = m_deviceResources->GetCommandQueue();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Set up for rendering to the front panel via the GPU
    if (m_available)
    {
        // Create the front panel render target resources
        m_frontPanelRenderTarget->CreateDeviceDependentResources(device,m_deviceResources->GetBackBufferCount());
    }

    ////////////////////////////////
    //
    // Create constant buffers
    //
    ////////////////////////////////
    const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    // VS constant buffer
    {
        size_t cbSize = m_deviceResources->GetBackBufferCount() * AlignUp(sizeof(VS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_VSConstants.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_VSConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVSConstantData)));

        m_VSConstantDataGpuAddr = m_VSConstants->GetGPUVirtualAddress();
    }

    // PS constant buffer
    {
        size_t cbSize = m_deviceResources->GetBackBufferCount() * AlignUp(sizeof(PS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_PSConstants.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_PSConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPSConstantData)));

        m_PSConstantDataGpuAddr = m_PSConstants->GetGPUVirtualAddress();
    }

    ////////////////////////////////
    //
    // Create resource upload batch and texture factory
    //
    ////////////////////////////////
    {
        m_resourceUploadBatch = std::make_unique<ResourceUploadBatch>(device);
        m_textureFactory = std::make_unique<EffectTextureFactory>(device, *m_resourceUploadBatch.get(), TextureDescriptors::Count);
        m_textureFactory->SetDirectory(L"Assets\\textures");
    }

    ////////////////////////////////
    //
    // Load the dolphins
    //
    //////////////////////////////
    for (unsigned int i = 0; i < m_dolphins.size(); i++)
        m_dolphins[i]->Load(device, rtState, m_deviceResources->GetBackBufferCount());

    ////////////////////////////////
    //
    // Create the textures resources
    //
    ////////////////////////////////
    m_resourceUploadBatch->Begin();

    m_textureFactory->CreateTexture(L"Seafloor.bmp", TextureDescriptors::SeaFloor);
    m_textureFactory->CreateTexture(L"dolphin.bmp", TextureDescriptors::DolphinSkin);

    for (DWORD t = 0; t < 32; t++)
    {
        std::wstring path;
        if (t < 10)
        {
            int count = _scwprintf(L"caust0%i.DDS", t);
            path.resize(size_t(count + 1));
            swprintf_s(&path[0], path.size(), L"caust0%i.DDS", t);
        }
        else
        {
            int count = _scwprintf(L"caust%i.DDS", t);
            path.resize(size_t(count + 1));
            swprintf_s(&path[0], path.size(), L"caust%i.DDS", t);
        }

        m_textureFactory->CreateTexture(path.c_str(), static_cast<int>(TextureDescriptors::CausticFirst) + int(t));
    }

    m_resourceUploadBatch->End(commandQueue);

    ////////////////////////////////
    //
    // Create the mesh resources
    //
    ////////////////////////////////

    // Create sea floor objects
    m_seafloor = Model::CreateFromSDKMESH(device, L"assets\\mesh\\seafloor.sdkmesh");

    m_seaEffect = std::make_unique<SeaEffect>(device, rtState);

    // Determine the aspect ratio
    float fAspectRatio = 1920.0f / 1080.0f;

    // Set the transform matrices
    static const XMVECTORF32 c_vEyePt = { 0.0f, 0.0f, -5.0f, 0.0f };
    static const XMVECTORF32 c_vLookatPt = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const XMVECTORF32 c_vUpVec = { 0.0f, 1.0f, 0.0f, 0.0f };

    m_matView = XMMatrixLookAtLH(c_vEyePt, c_vLookatPt, c_vUpVec);
    m_matProj = XMMatrixPerspectiveFovLH(XM_PI / 3, fAspectRatio, 1.0f, 10000.0f);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    if (m_available)
    {
        // Assuming max of 3 render targets
        ID3D12Resource* pRenderTargets[3] = {};
        for(unsigned int rtIndex = 0; rtIndex < m_deviceResources->GetBackBufferCount(); ++rtIndex)
        {
            pRenderTargets[rtIndex] = m_deviceResources->GetRenderTarget(rtIndex);
        }
        auto device = m_deviceResources->GetD3DDevice();
        m_frontPanelRenderTarget->CreateWindowSizeDependentResources(device, m_deviceResources->GetBackBufferCount(), pRenderTargets);
    }
}
#pragma endregion

void Sample::SetWaterColor(float red, float green, float blue)
{
    m_waterColor[0] = red;
    m_waterColor[1] = green;
    m_waterColor[2] = blue;
    m_waterColor[3] = 1.0f;
}

void Sample::DrawDolphin(Dolphin &dolphin)
{
    auto commandList = m_deviceResources->GetCommandList();
    unsigned int constantBufferIndex = m_deviceResources->GetCurrentFrameIndex() % m_deviceResources->GetBackBufferCount();

    VS_CONSTANT_BUFFER* vertShaderConstData = dolphin.MapVSConstants(constantBufferIndex);
    {
        vertShaderConstData->vZero = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        vertShaderConstData->vConstants = XMVectorSet(1.0f, 0.5f, 0.2f, 0.05f);

        FLOAT fBlendWeight = dolphin.GetBlendWeight();
        FLOAT fWeight1;
        FLOAT fWeight2;
        FLOAT fWeight3;

        if (fBlendWeight > 0.0f)
        {
            fWeight1 = fabsf(fBlendWeight);
            fWeight2 = 1.0f - fabsf(fBlendWeight);
            fWeight3 = 0.0f;
        }
        else
        {
            fWeight1 = 0.0f;
            fWeight2 = 1.0f - fabsf(fBlendWeight);
            fWeight3 = fabsf(fBlendWeight);
        }
        vertShaderConstData->vWeight = XMVectorSet(fWeight1, fWeight2, fWeight3, 0.0f);

        // Lighting vectors (in world space and in dolphin model space)
        // and other constants
        vertShaderConstData->vLight = XMVectorSet(0.00f, 1.00f, 0.00f, 0.00f);
        vertShaderConstData->vLightDolphinSpace = XMVectorSet(0.00f, 1.00f, 0.00f, 0.00f);
        vertShaderConstData->vDiffuse = XMVectorSet(1.00f, 1.00f, 1.00f, 1.00f);
        vertShaderConstData->vAmbient = XMVectorSet(m_ambient[0], m_ambient[1], m_ambient[2], m_ambient[3]);
        vertShaderConstData->vFog = XMVectorSet(0.50f, 50.00f, 1.00f / (50.0f - 1.0f), 0.00f);

        float totalSeconds = float(m_timer.GetTotalSeconds());
        vertShaderConstData->vCaustics = XMVectorSet(0.05f, 0.05f, sinf(totalSeconds) / 8, cosf(totalSeconds) / 10);

        XMVECTOR vDeterminant;
        XMMATRIX matDolphin = dolphin.GetWorld();
        XMMATRIX matDolphinInv = XMMatrixInverse(&vDeterminant, matDolphin);
        vertShaderConstData->vLightDolphinSpace = XMVector4Normalize(XMVector4Transform(vertShaderConstData->vLight, matDolphinInv));

        // Vertex shader operations use transposed matrices
        XMMATRIX matCamera = XMMatrixMultiply(matDolphin, m_matView);
        XMMATRIX mat = XMMatrixMultiply(matCamera, m_matProj);
        vertShaderConstData->matTranspose = XMMatrixTranspose(mat);
        vertShaderConstData->matCameraTranspose = XMMatrixTranspose(matCamera);
        vertShaderConstData->matViewTranspose = XMMatrixTranspose(m_matView);
        vertShaderConstData->matProjTranspose = XMMatrixTranspose(m_matProj);

    }
    dolphin.UnmapAndSetVSConstants(commandList, constantBufferIndex, m_textureFactory->Heap(), 
        m_textureFactory->GetGpuDescriptorHandle(TextureDescriptors::DolphinSkin),
        m_textureFactory->GetGpuDescriptorHandle(TextureDescriptors::CausticFirst + m_currentCausticTextureView));

    commandList->SetGraphicsRootConstantBufferView(1, 
        m_PSConstantDataGpuAddr + AlignUp(sizeof(PS_CONSTANT_BUFFER), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * constantBufferIndex);

    dolphin.Render(commandList);
}
