//--------------------------------------------------------------------------------------
// SimpleMeshlet.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleMeshlet.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "Meshlet.h"

#include "Shared.h"

#pragma warning( disable : 4324 4365 )

extern void ExitSample() noexcept;

using namespace ATG;
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

    const wchar_t* s_sampleTitle = L"Simple Meshlet";

    const wchar_t* s_cullMeshletFilename = L"CullMeshletMS.cso";
    const wchar_t* s_basicMeshletFilename = L"BasicMeshletMS.cso";
    const wchar_t* s_pixelShaderFilename = L"BasicMeshletPS.cso";

    const wchar_t* s_meshFilenames[] =
    {
#ifdef _GAMING_DESKTOP
        L"ATGDragon\\Dragon_LOD0.sdkmesh",
        L"ATGDragon\\Dragon_LOD1.sdkmesh",
        L"ATGDragon\\Dragon_LOD2.sdkmesh",
        L"ATGDragon\\Dragon_LOD3.sdkmesh",
        L"ATGDragon\\Dragon_LOD4.sdkmesh",
        L"ATGDragon\\Dragon_LOD5.sdkmesh",
        L"Camera\\Camera.sdkmesh"
#else
        L"Dragon_LOD0.sdkmesh",
        L"Dragon_LOD1.sdkmesh",
        L"Dragon_LOD2.sdkmesh",
        L"Dragon_LOD3.sdkmesh",
        L"Dragon_LOD4.sdkmesh",
        L"Dragon_LOD5.sdkmesh",
        L"Camera.sdkmesh"
#endif
    };

    constexpr uint32_t s_dragonLODStart = 0;
    constexpr uint32_t s_dragonLODCount = 6;

    struct ObjectDefinition
    {
        int      ModelIndex;
        XMMATRIX World;
    };

    const ObjectDefinition s_objectDefs[] =
    {
        { s_dragonLODStart, Matrix::CreateScale(0.2f) },
        { s_dragonLODCount, XMMatrixIdentity() }
    };

    template <typename T>
    size_t GetAlignedSize(T size)
    {
        const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
        return alignedSize;
    }
}

Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_cull(false)
    , m_debugCamCull(false)
    , m_drawMeshlets(true)
    , m_lodIndex(s_dragonLODStart)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

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

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());
    
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsRightShoulderPressed())
        {
            m_debugCamera.Update(elapsedTime, pad);
        }
        else
        {
            m_camera.Update(elapsedTime, pad);
        }

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            m_cull = !m_cull;
        }

        if (m_gamePadButtons.b == ButtonState::PRESSED)
        {
            m_debugCamCull = !m_debugCamCull;

            if (m_debugCamCull)
                m_cull = true;
        }

        if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            m_drawMeshlets = !m_drawMeshlets;
        }

        if (m_gamePadButtons.leftTrigger == ButtonState::PRESSED)
        {
            m_lodIndex = (m_lodIndex + s_dragonLODCount - 1) % s_dragonLODCount;
            m_scene[0].ModelIndex = s_dragonLODStart + m_lodIndex;
        }
        else if (m_gamePadButtons.rightTrigger == ButtonState::PRESSED)
        {
            m_lodIndex = (m_lodIndex + 1) % s_dragonLODCount;
            m_scene[0].ModelIndex = s_dragonLODStart + m_lodIndex;
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        auto kb = m_keyboard->GetState();
        m_keyboardButtons.Update(kb);

        if (kb.Escape)
        {
            ExitSample();
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Tab))
        {
            m_cull = !m_cull;
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Q))
        {
            m_debugCamCull = !m_debugCamCull;

            if (m_debugCamCull)
                m_cull = true;
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
        {
            m_drawMeshlets = !m_drawMeshlets;
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::OemMinus))
        {
            m_lodIndex = (m_lodIndex + s_dragonLODCount - 1) % s_dragonLODCount;
            m_scene[0].ModelIndex = s_dragonLODStart + m_lodIndex;
        }
        else if (m_keyboardButtons.IsKeyPressed(Keyboard::OemPlus))
        {
            m_lodIndex = (m_lodIndex + 1) % s_dragonLODCount;
            m_scene[0].ModelIndex = s_dragonLODStart + m_lodIndex;
        }

        if (kb.LeftShift)
        {
            m_debugCamera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
        }
        else
        {
            m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());
        }
    }

    auto& debugCam = m_scene[1];
    debugCam.World = Matrix(m_debugCamera.GetView()).Invert();
    Model::UpdateEffectMatrices(debugCam.Effects, debugCam.World, m_camera.GetView(), m_camera.GetProjection());

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

    UpdateConstants(commandList);

    // Set up the root signature & pipeline state
    auto descriptorHeaps = m_srvPile->Heap();
    commandList->SetDescriptorHeaps(1, &descriptorHeaps);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_cull ? m_cullMeshletPso.Get() : m_basicMeshletPso.Get());
#ifdef _GAMING_XBOX_SCARLETT
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif

    commandList->SetGraphicsRootConstantBufferView(0, m_viewCB->GetGPUVirtualAddress());

    // Draw our scene objects
    for (size_t i = 0; i < m_scene.size(); ++i)
    {
        auto& object = m_scene[i];

        auto& model = *m_models[object.ModelIndex].Model;
        auto& meshletData = m_models[object.ModelIndex].MeshletData;

        if (!meshletData.empty())
        {
            // Render using Mesh Shaders when meshlets are available
            commandList->SetGraphicsRootConstantBufferView(1, object.ConstantBuffer->GetGPUVirtualAddress());

            for (size_t j = 0; j < meshletData.size(); ++j)
            {
                commandList->SetGraphicsRootConstantBufferView(2, meshletData[j].GetMeshInfoResource()->GetGPUVirtualAddress());

                commandList->SetGraphicsRootShaderResourceView(3, model.meshes[j]->opaqueMeshParts[0]->vertexBuffer.Resource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(4, meshletData[j].GetMeshletResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(5, meshletData[j].GetUniqueIndexResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(6, meshletData[j].GetPrimitiveResource()->GetGPUVirtualAddress());

                commandList->DispatchMesh(meshletData[j].GetMeshletCount(), 1, 1);
            }
        }
        else
        {
            // Render using a standard VS - PS pipeline.
            m_models[object.ModelIndex].Model->DrawOpaque(commandList, object.Effects.begin());
        }
    }

    m_frustumDraw.Draw(commandList);

    DrawHUD(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::UpdateConstants(ID3D12GraphicsCommandList* commandList)
{
    {
        Matrix viewProj = m_camera.GetView() * m_camera.GetProjection();
        Matrix debugViewProj = m_debugCamera.GetView() * m_debugCamera.GetProjection();

        auto cbMem = m_graphicsMemory->AllocateConstant<Constants>();
        Constants& constants = *static_cast<Constants*>(cbMem.Memory());

        constants.View = Matrix(m_camera.GetView()).Transpose();
        constants.ViewProj = viewProj.Transpose();
        constants.DebugViewProj = debugViewProj.Transpose();
        constants.DebugCull = m_debugCamCull;
        constants.DrawMeshlets = m_drawMeshlets;

        TransitionResource(commandList, m_viewCB.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(m_viewCB.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
        TransitionResource(commandList, m_viewCB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);


        m_frustumDraw.Update(viewProj, m_debugCamCull ? debugViewProj : viewProj);
    }

    for (size_t i = 0; i < m_scene.size(); ++i)
    {
        auto& object = m_scene[i];

        auto cbMem = m_graphicsMemory->AllocateConstant<Instance>();
        Instance& instanceData = *static_cast<Instance*>(cbMem.Memory());

        instanceData.World = object.World.Transpose();
        instanceData.WorldInvTrans = object.World.Transpose().Invert().Transpose();

        TransitionResource(commandList, object.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(object.ConstantBuffer.Get(), 0, cbMem.Resource(), cbMem.ResourceOffset(), cbMem.Size());
        TransitionResource(commandList, object.ConstantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    }
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

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    m_hudBatch->Begin(commandList);

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    // Draw title.
    m_smallFont->DrawString(m_hudBatch.get(), s_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Primitive Culling: %s", m_cull ? L"Enabled" : L"Disabled");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Debug Camera Culling: %s", m_debugCamCull ? L"Enabled" : L"Disabled");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"Meshlet Visualization: %s", m_drawMeshlets ? L"Enabled" : L"Disabled");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Start from bottom
    textPos = XMFLOAT2(float(safe.left), float(safe.bottom));

#ifdef _GAMING_DESKTOP
    m_smallFont->DrawString(m_hudBatch.get(), L"[Escape] Exit Sample", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[-+] Cycle LODs", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[LShift] Move Debug Camera (Hold)", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[Q] Debug Camera Cull", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[Tab] Enable/Disable Primitive Culling", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[Space] Toggle Meshlet Visualization", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[WASD] Pan Camera", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_hudBatch.get(), L"[LMouse] Move Camera (Hold)", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();
#else
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[View] Exit Sample", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LT][RT] Cycle LODs", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[RB] Move Debug Camera (Hold)", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[B] Debug Camera Cull", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[A] Enable/Disable Primitive Culling", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[X] Toggle Meshlet Visualization", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[DPad] Pan Camera", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();

    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), L"[LThumb][RThumb] Move Camera", textPos, textColor);
    textPos.y -= m_smallFont->GetLineSpacing();
#endif

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
    m_commonStates = std::make_unique<CommonStates>(device);

    // Create heap
    m_srvPile = std::make_unique<DescriptorPile>(device,
        128,
        DescriptorHeapIndex::SRV_Count);

    m_frustumDraw.CreateDeviceResources(*m_deviceResources, *m_commonStates);

    {
        const CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(Constants)));
        
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to VERTEX_AND_CONSTANT_BUFFER on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_viewCB.ReleaseAndGetAddressOf()))
        );
    }


    // Create Mesh Shader Pipeline States
    {
        auto cullMeshletMS = DX::ReadData(s_cullMeshletFilename);
        auto basicMeshletMS = DX::ReadData(s_basicMeshletFilename);
        auto basicMeshletPS = DX::ReadData(s_pixelShaderFilename);

        DX::ThrowIfFailed(device->CreateRootSignature(0, basicMeshletMS.data(), basicMeshletMS.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        // Disable culling so we can see the backside of geometry through the culled mesh.
        CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature        = m_rootSignature.Get();
        psoDesc.MS                    = { basicMeshletMS.data(), basicMeshletMS.size() };
        psoDesc.PS                    = { basicMeshletPS.data(), basicMeshletPS.size() };
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = m_deviceResources->GetBackBufferFormat();
        psoDesc.DSVFormat             = m_deviceResources->GetDepthBufferFormat();
        psoDesc.RasterizerState       = rasterDesc;
        psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
        psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
        psoDesc.SampleMask            = UINT_MAX;
        psoDesc.SampleDesc            = DefaultSampleDesc();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_basicMeshletPso.ReleaseAndGetAddressOf())));

        meshStreamDesc.MS = { cullMeshletMS.data(), cullMeshletMS.size() };
        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_cullMeshletPso.ReleaseAndGetAddressOf())));
    }


    // Load Model data
    m_models.resize(_countof(s_meshFilenames));

    for (size_t i = 0; i < m_models.size(); ++i)
    {
        wchar_t filepath[_MAX_PATH] = {};
        DX::FindMediaFile(filepath, _MAX_PATH, s_meshFilenames[i]);

        m_models[i].Model = Model::CreateFromSDKMESH(device, filepath);

        wchar_t meshletFilepath[_MAX_PATH] = {};
        swprintf_s(meshletFilepath, L"%s.bin", filepath);

        m_models[i].MeshletData = MeshletSet::Read(meshletFilepath);
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create D3D12 resources for mesh & meshlets
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i].Model->LoadStaticBuffers(device, resourceUpload, true);

        for (auto& m : m_models[i].MeshletData)
        {
            m.CreateResources(device, resourceUpload);
        }
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    auto texOffsets = std::vector<size_t>(m_models.size());
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        if (!m_models[i].Model->textureNames.empty())
        {
            size_t _;
            m_srvPile->AllocateRange(m_models[i].Model->textureNames.size(), texOffsets[i], _);

            m_models[i].Model->LoadTextures(*m_textureFactory, int(texOffsets[i]));
        }
    }

    auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());

    const RenderTargetState objectRTState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto objectPSD = EffectPipelineStateDescription(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        objectRTState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    m_scene.resize(_countof(s_objectDefs));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        uint32_t index = s_objectDefs[i].ModelIndex;
        assert(index < m_models.size());

        m_scene[i].World    = s_objectDefs[i].World;
        m_scene[i].ModelIndex = index;
        m_scene[i].Effects  = m_models[index].Model->CreateEffects(effectFactory, objectPSD, objectPSD, int(texOffsets[index]));

        const CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(Instance)));
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to VERTEX_AND_CONSTANT_BUFFER on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_scene[i].ConstantBuffer.ReleaseAndGetAddressOf()))
        );
    }

    // HUD
    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
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
    auto const size = m_deviceResources->GetOutputSize();
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Set main camera properties
    m_camera.SetWindow(m_displayWidth, m_displayHeight);
    m_camera.SetProjectionParameters(XM_PIDIV4, 0.25f, 1000.0f, true);

    m_camera.SetFocus(Vector3(0.0f, 10.0f, 0.0f));
    m_camera.SetRadius(50.0f);
    m_camera.SetRotation(Quaternion::CreateFromAxisAngle(Vector3::Up, XM_PIDIV4 * 3.0f));

    // Set debug camera properties
    m_debugCamera.SetWindow(m_displayWidth, m_displayHeight);
    m_debugCamera.SetProjectionParameters(XM_PIDIV4, 0.25f, 100.0f, true);

    m_debugCamera.SetFocus(Vector3(0.0f, 10.0f, 0.0f));
    m_debugCamera.SetRadius(50.0f);
    m_debugCamera.SetRotation(XMQuaternionIdentity());

#ifdef _GAMING_XBOX_SCARLETT
    m_camera.SetRadiusRate(20.0f);
    m_debugCamera.SetRadiusRate(20.0f);
#endif
}

void Sample::OnDeviceLost()
{
    m_commonStates.reset();
    m_srvPile.reset();
    m_rootSignature.Reset();
    m_basicMeshletPso.Reset();
    m_cullMeshletPso.Reset();
    m_viewCB.Reset();
    m_hudBatch.reset();
    m_smallFont.reset();
    m_models.clear();
    m_textureFactory.reset();
    m_scene.clear();
    m_frustumDraw.ReleaseResources();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
