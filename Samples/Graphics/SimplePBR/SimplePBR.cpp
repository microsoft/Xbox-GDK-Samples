//--------------------------------------------------------------------------------------
// SimplePBR.cpp
//
// Demonstrates PBRModel and PBREffect in DirectX 12 on Xbox One, Xbox Series X|S, and PC devices
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimplePBR.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr DXGI_FORMAT c_HDRFormat        = DXGI_FORMAT_R11G11B10_FLOAT;
    constexpr DXGI_FORMAT c_backBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    constexpr DXGI_FORMAT c_depthFormat      = DXGI_FORMAT_D32_FLOAT;

    // PBR Assest paths.
    const wchar_t* s_modelPaths[] =
    {
        L"Floor.sdkmesh",
        L"ToyRobot.sdkmesh",
        L"WoodBlocks.sdkmesh"
    };

    const wchar_t* s_folderPaths[] =
    {
        L"Media\\PBR\\ToyRobot\\Floor",
        L"Media\\PBR\\ToyRobot\\ToyRobot",
        L"Media\\PBR\\ToyRobot\\WoodBlocks",
        L"Media\\PBR\\XboxOrb"
    };

    // A simple test scene for material parameters
    struct TestScene
    {
        std::unique_ptr<DirectX::Model> m_model;
        std::unique_ptr<DirectX::GeometricPrimitive> m_sphere;
        std::unique_ptr<PBREffect> m_effect;

        void Init(ID3D12Device* device,
            ResourceUploadBatch& upload,
            D3D12_GPU_DESCRIPTOR_HANDLE radianceTex, int numMips,
            D3D12_GPU_DESCRIPTOR_HANDLE irradianceTex,
            D3D12_GPU_DESCRIPTOR_HANDLE sampler)
        {
            const RenderTargetState hdrBufferRts(c_HDRFormat, c_depthFormat);

            m_sphere = DirectX::GeometricPrimitive::CreateSphere(1.5);

            // Create PBR Effect
            EffectPipelineStateDescription pbrEffectPipelineState(
                &GeometricPrimitive::VertexType::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullClockwise,
                hdrBufferRts,
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
            m_effect = std::make_unique<PBREffect>(device, EffectFlags::None, pbrEffectPipelineState);

            // Lighting
            m_effect->SetIBLTextures(
                radianceTex,
                numMips,
                irradianceTex,
                sampler);

            // Model
            wchar_t absoluteModelPath[MAX_PATH] = {};
            DX::FindMediaFile(absoluteModelPath, MAX_PATH, L"XboxOrb.sdkmesh", &s_folderPaths[3]);

            m_model = Model::CreateFromSDKMESH(device, absoluteModelPath);

            // Optimize model for rendering
            m_model->LoadStaticBuffers(device, upload);
        }

        void XM_CALLCONV Render(ID3D12GraphicsCommandList* commandList, FXMMATRIX camView, CXMMATRIX camProj)
        {
            constexpr size_t numSpheres = 3;
            constexpr float step = 15.f;

            Vector3 modelPos((-step * (numSpheres - 1)) / 2.f, 0, 0);

            m_effect->SetConstantAlbedo(Vector3(1, 1, 1));
            m_effect->SetConstantMetallic(1);

            for (size_t i = 0; i < numSpheres; i++)
            {
                m_effect->SetView(camView);
                m_effect->SetProjection(camProj);
                m_effect->SetWorld(Matrix::CreateTranslation(modelPos));

                modelPos += Vector3(step, 0, 0);

                m_effect->SetConstantRoughness(float(i) / float(numSpheres - 1));

                m_effect->Apply(commandList);
                m_model->DrawOpaque(commandList);
            }

            modelPos = Vector3((-step * (numSpheres - 1)) / 2.f, 0, 0);
            modelPos += Vector3(0, step, 0);

            m_effect->SetConstantMetallic(0);

            for (size_t i = 0; i < numSpheres; i++)
            {
                m_effect->SetView(camView);
                m_effect->SetProjection(camProj);
                m_effect->SetWorld(Matrix::CreateTranslation(modelPos));

                modelPos += Vector3(step, 0, 0);

                m_effect->SetConstantRoughness(float(i) / float(numSpheres - 1));

                m_effect->Apply(commandList);
                m_model->DrawOpaque(commandList);
            }
        }
    };

    std::unique_ptr<TestScene> s_testScene;
}

Sample::Sample() :
    m_frame(0),
    m_gamepadConnected(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_backBufferFormat, c_depthFormat, 2);
    m_deviceResources->RegisterDeviceNotify(this);
    m_gamePad = std::make_unique<GamePad>();
    m_hdrScene = std::make_unique<DX::RenderTexture>(c_HDRFormat);
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
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    // Initialize Camera
    {
        const auto size = m_deviceResources->GetOutputSize();
        constexpr float fovAngleY = 70.0f * XM_PI / 180.0f;

        m_camera = std::make_unique<DX::OrbitCamera>();
        m_camera->SetWindow(size.right, size.bottom);
        m_camera->SetProjectionParameters(fovAngleY, 0.1f, 1000.f, false);
        m_camera->SetRadius(25.f);
        m_camera->SetRadiusRate(5.f);
        m_camera->SetFocus(Vector3(0, 4, -5));
        // Rotate to face front
        m_camera->SetRotation(Vector3(0, XM_PI, XM_PI / 10));
    }

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
#ifdef _GAMING_DESKTOP
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
#endif
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
    const float elapsedSeconds = static_cast<float>(timer.GetElapsedSeconds());

    // Update camera via game pad and mouse and keyboard
#ifdef _GAMING_XBOX
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif

    if (pad.IsConnected())
    {
        m_gamepadConnected = true;

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamepadConnected = false;
        m_gamePadButtons.Reset();
    }
    m_camera->Update(elapsedSeconds, pad);
#ifdef _GAMING_DESKTOP
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);
    if (m_mouse->IsConnected() || m_keyboard->IsConnected())
    {
        if (kb.Escape)
        {
            ExitSample();
        }
    }
    // Keyboard and mouse are currently only supported on PC in this sample.
    m_camera->Update(elapsedSeconds, *(m_mouse.get()), *(m_keyboard.get()));
#endif
    // Update model effects
    for (auto& m : m_pbrModels)
    {
        auto effect = m->GetEffect();
        effect->SetView(m_camera->GetView());
        effect->SetProjection(m_camera->GetProjection());
        effect->SetWorld(Matrix::CreateRotationY(XM_PI));
    }

    // Update skybox
    m_skybox->Update(m_camera->GetView(), m_camera->GetProjection());

    PIXEndEvent();
}
#pragma endregion

void Sample::RenderHUD(ID3D12GraphicsCommandList* commandList)
{
    auto const size = m_deviceResources->GetOutputSize();

    // Safe area dimensions are in screenspace, with y-axis inverted.
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    const wchar_t* legendStr = (m_gamepadConnected) ?
        L"[RThumb] [LThumb]: Move Camera   [View] Exit "
        : L"Mouse, W,A,S,D: Move Camera   Esc: Exit ";
    auto TitleStringBounds = m_smallFont->MeasureString(L"SimplePBR Sample");
    auto LegendStringBounds = m_smallFont->MeasureString(legendStr);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render HUD");

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI Rectangles");
    // Draw dark semi-transparent rectangles behind UI text
    // to make the words more visible and distinguishable from
    // the rendered scene in the background.

    m_basicEffect->Apply(commandList);
    m_vertexBatch->Begin(commandList);

    // UI Rectangle Bounds
    auto  UIBackground = Vector4(0.f, 0.f, 0.f, 0.95f);
    float UIRectangleSceneDepth = 0.05f;
    float padding = 0.02f;

    // Convert screenspace coordinates to NDC.
    float minX = 2.0f * (safe.left / (float)size.right) - 1.0f - padding;
    float maxX = 2.0f * ((XMVectorGetX(LegendStringBounds) + (float)safe.left) / (float)size.right) - 1.0f + padding;
    float minYTitle = (2.0f * ((XMVectorGetY(TitleStringBounds) + (float)safe.top) / (float)size.bottom) - 1.0f) * -1.0f - padding;
    float maxYTitle = (2.0f * (safe.top / (float)size.bottom) - 1.0f) * -1.0f + padding;

    float minYControls = (2.0f * (safe.bottom / (float)size.bottom) - 1.0f) * -1.0f - padding;
    float maxYControls = (2.0f * (((float)safe.bottom - XMVectorGetY(LegendStringBounds)) / (float)size.bottom) - 1.0f) * -1.0f + padding;

    // Sample Title UI Rectangle
    VertexPositionColor vTitle0(Vector3(minX, maxYTitle, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vTitle1(Vector3(maxX, maxYTitle, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vTitle2(Vector3(maxX, minYTitle, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vTitle3(Vector3(minX, minYTitle, UIRectangleSceneDepth), UIBackground);

    // Sample Control Instructions UI Rectangle
    VertexPositionColor vControls0(Vector3(minX, maxYControls, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vControls1(Vector3(maxX, maxYControls, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vControls2(Vector3(maxX, minYControls, UIRectangleSceneDepth), UIBackground);
    VertexPositionColor vControls3(Vector3(minX, minYControls, UIRectangleSceneDepth), UIBackground);

    m_vertexBatch->DrawQuad(vTitle0, vTitle1, vTitle2, vTitle3);
    m_vertexBatch->DrawQuad(vControls0, vControls1, vControls2, vControls3);

    m_vertexBatch->End();
    PIXEndEvent(commandList); // UI Rectangles

    m_hudBatch->Begin(commandList);

#ifdef _GAMING_XBOX
    auto fontColor = ATG::ColorsHDR::White;
#else
    auto fontColor = ATG::ColorsHDR::LightGrey;
#endif

    m_smallFont->DrawString(m_hudBatch.get(), L"SimplePBR Sample",
        XMFLOAT2(float(safe.left), float(safe.top)), fontColor);

    DX::DrawControllerString(m_hudBatch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        legendStr,
        XMFLOAT2(float(safe.left), float(safe.bottom) - m_smallFont->GetLineSpacing()),
        fontColor);

    m_hudBatch->End();
    
    PIXEndEvent(commandList); // HUD
}

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

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap(), m_commonStates->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Draw to HDR buffer
    m_hdrScene->BeginScene(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render HDR");

    auto depthStencilDescriptor = m_deviceResources->GetDepthStencilView();
    auto toneMapRTVDescriptor = m_rtvHeap->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(1, &toneMapRTVDescriptor, FALSE, &depthStencilDescriptor);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Model Draw");
#ifndef TEST_SCENE
    for (auto& m : m_pbrModels)
    {
        m->GetEffect()->Apply(commandList);
        m->GetModel()->DrawOpaque(commandList);
    }
#else
    s_testScene->Render(commandList, m_camera->GetView(), m_camera->GetProjection());
#endif
    PIXEndEvent(commandList); // Model Draw

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Sky box");
    {
        // Render test skybox
        m_skybox->Render(commandList);
    }
    PIXEndEvent(commandList);

    PIXEndEvent(commandList); // Render HDR

    RenderHUD(commandList);

    m_hdrScene->EndScene(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Tonemap");

#if defined(_GAMING_XBOX)

    // Generate both HDR10 and tonemapped SDR signal
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptors[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    commandList->OMSetRenderTargets(2, rtvDescriptors, FALSE, nullptr);

    m_HDR10->SetHDRSourceTexture(m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::SceneTex)));
    m_HDR10->Process(commandList);
#else

    {
        auto rtv = static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(m_deviceResources->GetRenderTargetView());
        commandList->OMSetRenderTargets(1, &rtv, FALSE, NULL);

        if (m_deviceResources->GetColorSpace() == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
            // HDR10 signal
            m_HDR10->SetHDRSourceTexture(m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::SceneTex)));
            m_HDR10->Process(commandList);
        }
        else
        {
            // Tonemap for SDR signal
            m_toneMap->SetHDRSourceTexture(m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::SceneTex)));
            m_toneMap->Process(commandList);
        }
    }

#endif

    PIXEndEvent(commandList); // Tonemap

    PIXEndEvent(commandList); // Render

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
#ifdef _GAMING_DESKTOP
    m_keyboardButtons.Reset();
#endif
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
#ifdef _GAMING_DESKTOP
    m_deviceResources->UpdateColorSpace();
#endif
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
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    wchar_t absoluteModelPath[MAX_PATH] = {};

    // State objects
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    // create heaps
    m_srvPile = std::make_unique<DescriptorPile>(device,
        128, // Maximum descriptors for both static and dynamic
        static_cast<size_t>(StaticDescriptors::Reserve));
    m_rtvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    // Set up HDR render target.
    m_hdrScene->SetDevice(device, m_srvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::SceneTex)), m_rtvHeap->GetFirstCpuHandle());

    // UI Geometry Setup
    m_vertexBatch = std::make_unique<PrimitiveBatch<VertexType>>(device);
    const RenderTargetState UIRectRtState(c_HDRFormat, c_depthFormat);

    EffectPipelineStateDescription UIRectPd(
        &VertexType::InputLayout,
        CommonStates::AlphaBlend,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        UIRectRtState);

    m_basicEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, UIRectPd);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    // Begin uploading texture resources
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Radiance (specular environment) texture
    DX::FindMediaFile(absoluteModelPath, MAX_PATH, L"Stonewall_Ref_radiance.dds");
    DX::ThrowIfFailed(
        DirectX::CreateDDSTextureFromFile(
            device,
            resourceUpload,
            absoluteModelPath,
            m_radianceTexture.ReleaseAndGetAddressOf(),
            false
        ));

    DirectX::CreateShaderResourceView(device, m_radianceTexture.Get(), m_srvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::RadianceTex)), true);

    // Irradiance (diffuse environment) texture
    DX::FindMediaFile(absoluteModelPath, MAX_PATH, L"Stonewall_Ref_irradiance.dds");
    DX::ThrowIfFailed(
        DirectX::CreateDDSTextureFromFile(
            device,
            resourceUpload,
            absoluteModelPath,
            m_irradianceTexture.ReleaseAndGetAddressOf(),
            false
        ));

    DirectX::CreateShaderResourceView(device, m_irradianceTexture.Get(), m_srvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::IrradianceTex)), true);

    // Pipeline state - for rendering direct to back buffer
    {
        RenderTargetState backBufferRts(c_backBufferFormat, c_depthFormat);

        // Create HDR10 color space effect
#if defined(_GAMING_XBOX)
        backBufferRts.numRenderTargets = 2;
        backBufferRts.rtvFormats[1] = m_deviceResources->GetGameDVRFormat();

        m_HDR10 = std::make_unique<ToneMapPostProcess>(device, backBufferRts,
            ToneMapPostProcess::ACESFilmic, ToneMapPostProcess::SRGB, true);
#else
        m_HDR10 = std::make_unique<ToneMapPostProcess>(device, backBufferRts,
            ToneMapPostProcess::None, ToneMapPostProcess::ST2084);

        // Create tone mapping effect
        m_toneMap = std::make_unique<ToneMapPostProcess>(device, backBufferRts,
            ToneMapPostProcess::ACESFilmic, ToneMapPostProcess::SRGB);
#endif

    }

    // Pipeline state - for rendering to HDR buffer
    {
        const RenderTargetState hdrBufferRts(c_HDRFormat, c_depthFormat);

        // HUD
        DirectX::SpriteBatchPipelineStateDescription hudpd(
            hdrBufferRts,
            &CommonStates::AlphaBlend);

        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, hudpd);

        // Sky rendering batch
        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(device, resourceUpload, SpriteBatchPipelineStateDescription(hdrBufferRts, &CommonStates::Opaque));

        // PBR Model
        const auto numModels = _countof(s_modelPaths);
        m_pbrModels.resize(numModels);

        for (unsigned int i = 0; i < numModels; i++)
        {
#ifdef _GAMING_DESKTOP
            DX::FindMediaFile(absoluteModelPath, MAX_PATH, s_modelPaths[i], &s_folderPaths[i]);
            m_pbrModels[i] = std::make_unique<ATG::PBRModel>(absoluteModelPath);
#else
            m_pbrModels[i] = std::make_unique<ATG::PBRModel>(s_modelPaths[i]);
#endif
            m_pbrModels[i]->Create(device, hdrBufferRts, m_commonStates.get(), resourceUpload, m_srvPile.get());
        }

        // Skybox
        m_skybox = std::make_unique<DX::Skybox>(device, m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::RadianceTex)), hdrBufferRts, *m_commonStates);
    }

    // The current map has too much detail removed at last mips, scale back down to
    // match reference.
    const int numMips = m_radianceTexture->GetDesc().MipLevels - 3;

    // Set lighting textures for each model
    for (auto& m : m_pbrModels)
    {
        m->GetEffect()->SetIBLTextures(
            m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::RadianceTex)),
            numMips,
            m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::IrradianceTex)),
            m_commonStates->LinearWrap());
    }

    s_testScene = std::make_unique<TestScene>();
    s_testScene->Init(device,
        resourceUpload,
        m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::RadianceTex)),
        numMips,
        m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::IrradianceTex)),
        m_commonStates->LinearWrap());

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();
    wchar_t absoluteFontPath[MAX_PATH] = {};

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
    m_camera->SetWindow(size.right, size.bottom);

    // HDR render target resource
    m_hdrScene->SetWindow(size);

    // Begin uploading texture resources
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    if (size.bottom > 1200)
    {
        DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"SegoeUI_36.spritefont");
    }
    else
    {
        DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"SegoeUI_18.spritefont");
    }

    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        absoluteFontPath,
        m_srvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::Font)),
        m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::Font)));

    if (size.bottom > 1200)
    {
        DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    }
    else
    {
        DX::FindMediaFile(absoluteFontPath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    }

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        absoluteFontPath,
        m_srvPile->GetCpuHandle(static_cast<size_t>(StaticDescriptors::CtrlFont)),
        m_srvPile->GetGpuHandle(static_cast<size_t>(StaticDescriptors::CtrlFont)));

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

void Sample::OnDeviceLost()
{
    m_hudBatch.reset();
    m_vertexBatch.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();

    m_camera.reset();
    m_commonStates.reset();

    m_srvPile.reset();

    m_spriteBatch.reset();
    m_toneMap.reset();
    m_HDR10.reset();

    m_hdrScene->ReleaseDevice();
    m_rtvHeap.reset();

    for (auto& m : m_pbrModels)
    {
        m.reset();
    }

    m_graphicsMemory.reset();

    m_radianceTexture.Reset();
    m_irradianceTexture.Reset();
    m_skybox.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
