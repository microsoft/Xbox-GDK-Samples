//--------------------------------------------------------------------------------------
// CustomResolution.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CustomResolution.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ResolutionSet.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    // Fixed offsets into descriptor heaps
    enum SRVDescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

    // Barebones definition of scene objects
    struct ObjectDefinition
    {
        size_t              modelIndex;
        Matrix              world;
    };

    // Camera constants
    constexpr float c_defaultCameraAngle = -0.93f;
    constexpr float c_defaultCameraElevation = 2.97f;
    constexpr float c_defaultCameraDistance = 3.3f;
    constexpr float c_defaultCameraChangeRate = 80.f;

    // Assest paths
    const wchar_t* c_modelPaths[] =
    {
        L"scanner.sdkmesh",
        L"occcity.sdkmesh",
        L"column.sdkmesh",
    };

    // Barebones definition of a scene
    const ObjectDefinition c_sceneDefinition[] =
    {
        { 0, XMMatrixIdentity() },
        { 0, XMMatrixRotationY(XM_2PI * (1.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (2.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (3.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (4.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (5.0f / 6.0f)) },
        { 1, XMMatrixIdentity() },
        { 2, XMMatrixIdentity() },
    };

    // Determine how many resolutions are possible in a particular dimension.
    uint32_t CalculateResolutionSteps(uint32_t minRez, uint32_t maxRez, uint32_t increment)
    {
        assert(minRez <= maxRez);

        if (increment == 0 || minRez == maxRez)
        {
            return 1;
        }
        else
        {
            uint32_t steps = 1 + static_cast<uint32_t>(ceil((maxRez - minRez) / static_cast<float>(increment)));

            if (maxRez - steps * increment != minRez)
            {
                // This means the steps don't get us evenly to the lowest resolution, so there will be clamping.
                // When increasing the resolution afterward, different steps will be used, except for the start and end values.
                // E.g. with minRez = 5, maxRez = 10, increment = 2, decreasing will use steps at 10, 8, 6, 5.
                // Increasing will use 5, 7, 9, 10. So we need to double the number of intermediate steps only.
                steps = steps * 2 - 2;
            }

            return steps;
        }
    }

    template <size_t bufferSize = 2048>
    void DebugPrint(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize];

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }


    struct ResolutionChoice
    {
        uint32_t width;
        uint32_t height;
    };

    // List of selectable resolution values from in-game menu
    const ResolutionChoice c_resolutionChoices[] =
    {
        { 400, 400 }, // will return 0x89245408 E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_SMALL
        { 800, 600 },
        { 1280, 720 },
        { 1680, 1050 }, // will return 0x8924540b E_GAMESTREAMING_INVALID_CUSTOM_RESOLUTION (e.g. not divisible by 8)
        { 1920, 1080 },
        { 2104, 904 }, // will return 0x89245409L E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_LARGE if console display is 1080p
        { 768, 1024 },
        { 720, 1280 }, // will return 0x89245409L E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_LARGE if console display is 1080p
        { 1080, 1920 } // will return 0x89245409L E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_LARGE in all cases
    };

    void CALLBACK ClientPropertiesChangedCallback(
        void* context,
        XGameStreamingClientId clientId,
        uint32_t updatedPropertiesCount,
        XGameStreamingClientProperty* updatedProperties)
    {
        auto sample = reinterpret_cast<Sample*>(context);
        assert(sample != nullptr);

        if (sample->m_clients.find(clientId) == sample->m_clients.end())
        {
            return;
        }

        for (uint32_t i = 0; i < updatedPropertiesCount; ++i)
        {
            // These are triggered when the streaming client is connected and is resized
            switch (updatedProperties[i])
            {
            case XGameStreamingClientProperty::StreamPhysicalDimensions:
            {
                sample->RetrieveStreamingCharacteristics();
            }
            break;
            case XGameStreamingClientProperty::DisplayDetails:
            {
                sample->SetAutoResolution();
            }
            break;

            default:
                break;
            }
        }
    }

    void CALLBACK ConnectionStateChangedCallback(
        void* context,
        XGameStreamingClientId clientId,
        XGameStreamingConnectionState state) noexcept
    {
        auto sample  = reinterpret_cast<Sample*>(context);
        assert(sample != nullptr);

        switch (state)
        {
        case XGameStreamingConnectionState::Connected:
        {
            if (!std::any_of(sample->m_clients.begin(), sample->m_clients.end(), [clientId](std::pair<XGameStreamingClientId, XTaskQueueRegistrationToken> client) { return clientId == client.first; }))
            {
                XTaskQueueRegistrationToken propertiesChangedRegistrationToken = { 0 };

                DX::ThrowIfFailed(XGameStreamingRegisterClientPropertiesChanged(clientId, sample->GetTaskQueue(), sample, ClientPropertiesChangedCallback, &propertiesChangedRegistrationToken));

                sample->m_clients[clientId] = propertiesChangedRegistrationToken;
            }

            break;
        }
        case XGameStreamingConnectionState::Disconnected:
        {
            auto propertiesChangedRegistrationToken = sample->m_clients[clientId];

            XGameStreamingUnregisterClientPropertiesChanged(clientId, propertiesChangedRegistrationToken, true);

            sample->m_clients.erase(clientId);
            break;
        }
        default:
            break;
        }
    }
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_cameraAngle(c_defaultCameraAngle)
    , m_cameraElevation(c_defaultCameraElevation)
    , m_cameraDistance(c_defaultCameraDistance)
    , m_isExiting(false)
{
    m_deviceType = XSystemGetDeviceType();
    if (m_deviceType == XSystemDeviceType::XboxOneX || m_deviceType == XSystemDeviceType::XboxOneXDevkit)
    {
        OutputDebugStringA("ERROR: This sample renders to multiple display planes, which isn't supported on Xbox One X. "
            "Please change the devkit's console mode, or run on a different device.");
        throw std::exception("Xbox One X console mode not supported");
    }

    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT, c_numBackBuffers, DX::DeviceResources::c_Enable4K_UHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    m_isExiting = true;

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

    DX::ThrowIfFailed(XTaskQueueCreate(XTaskQueueDispatchMode::Immediate, XTaskQueueDispatchMode::Immediate, &m_queue));

    DX::ThrowIfFailed(XGameStreamingInitialize());
    DX::ThrowIfFailed(XGameStreamingRegisterConnectionStateChanged(m_queue, this, ConnectionStateChangedCallback, &m_token));

    InitializeUI();

    XErrorSetOptions(XErrorOptions::DebugBreakOnError, XErrorOptions::OutputDebugStringOnError);

    XErrorSetCallback([](HRESULT hr, const char* msg, void* context) -> bool
        {
            auto pThis = reinterpret_cast<Sample*>(context);

            char buf[512] = "";
            sprintf_s(buf, "Error 0x%08x: %s", static_cast<unsigned int>(hr), msg);

            pThis->m_uiManager.FindTypedById<UIStaticText>(ID("text4"))->SetDisplayText(buf);

            return true;
        }, this);
}

void Sample::InitializeUI()
{
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/ui_layout.json");

    // Cache these for faster access
    m_sidePanel = m_uiManager.FindTypedById<UIPanel>(ID("info_panel"));
    m_dynamicRezText = m_uiManager.FindTypedById<UIStaticText>(ID("dynamic_rez_value"));
    m_presetText = m_uiManager.FindTypedById<UIStaticText>(ID("preset_label"));
    m_fpsText = m_uiManager.FindTypedById<UIStaticText>(ID("fps_num"));

    m_text1 = m_uiManager.FindTypedById<UIStaticText>(ID("text1"));
    m_text2 = m_uiManager.FindTypedById<UIStaticText>(ID("text2"));
    m_text3 = m_uiManager.FindTypedById<UIStaticText>(ID("text3"));

    auto resMenu = m_uiManager.FindTypedById<UIStackPanel>(ID("ResMenu"));
    auto streamMenu = m_uiManager.FindTypedById<UIStackPanel>(ID("StreamMenu"));
    
    const auto numResolutions = std::size(c_resolutionChoices);
    for (uint32_t i = 0; i < numResolutions; ++i)
    {
        auto button = CastPtr<UIButton>(resMenu->AddChildFromPrefab("#menu_item_prefab"));
        char buttonText[32] = "";
        uint32_t width = c_resolutionChoices[i].width;
        uint32_t height = c_resolutionChoices[i].height;

        sprintf_s(buttonText, "Render %ux%u", c_resolutionChoices[i].width, c_resolutionChoices[i].height);
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(buttonText);
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, width, height](UIButton*)
            {
                SetResolution(width, height);
            });

        button->ButtonState().AddListenerWhen(UIButton::State::Normal,
            [button](UIButton*)
            {
                button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemTextStyle"));
            });

        button->ButtonState().AddListenerWhen(UIButton::State::Focused,
            [button](UIButton*)
            {
                button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemSelectedTextStyle"));
            });

        button = CastPtr<UIButton>(streamMenu->AddChildFromPrefab("#menu_item_prefab"));
        sprintf_s(buttonText, "Stream %ux%u", c_resolutionChoices[i].width, c_resolutionChoices[i].height);
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(buttonText);
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, width, height](UIButton*)
            {
                m_uiManager.FindTypedById<UIStaticText>(ID("text4"))->SetDisplayText("");
                auto hr = XGameStreamingSetResolution(width, height);
                if (FAILED(hr))
                {
                    DebugPrint("XGameStreamingSetResolution failed 0x%x", hr);
                }
            });

        button->ButtonState().AddListenerWhen(UIButton::State::Normal,
            [button](UIButton*)
            {
                button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemTextStyle"));
            });

        button->ButtonState().AddListenerWhen(UIButton::State::Focused,
            [button](UIButton*)
            {
                button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemSelectedTextStyle"));
            });
    }
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
    using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    m_uiInputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_uiInputState);


    const auto& maxGamePads = static_cast<uint32_t>(m_uiInputState.GetMaxGamePads());
    for (unsigned int i = 0; i < maxGamePads; ++i)
    {
        const auto& pad = m_uiInputState.GetGamePadState(i);
        const auto& buttons = m_uiInputState.GetGamePadButtons(i);
        if (pad.IsConnected())
        {
            if (buttons.back == ButtonState::PRESSED)
            {
                ExitSample();
                m_isExiting = true;
            }

            if (buttons.x == ButtonState::PRESSED)
            {
                auto image = m_uiManager.FindTypedById<UIImage>(ID("xbox_logo"));
                image->SetVisible(!image->GetVisible());
            }

            if (buttons.menu == ButtonState::PRESSED)
            {
                auto menu = m_uiManager.FindTypedById<UIStackPanel>(ID("ResMenu"));
                menu->SetVisible(!menu->GetVisible());
                menu = m_uiManager.FindTypedById<UIStackPanel>(ID("StreamMenu"));
                menu->SetVisible(!menu->GetVisible());
            }

            m_cameraAngle += pad.thumbSticks.leftX / c_defaultCameraChangeRate;
            m_cameraElevation -= pad.thumbSticks.leftY / c_defaultCameraChangeRate;
            m_cameraDistance -= pad.thumbSticks.rightY / c_defaultCameraChangeRate;
        }
    }

    XMVECTOR lookFrom = XMVectorSet(
        sinf(m_cameraAngle) * m_cameraDistance,
        m_cameraElevation,
        cosf(m_cameraAngle) * m_cameraDistance,
        0);

    m_view = XMMatrixLookAtLH(lookFrom, g_XMZero, g_XMIdentityR1);

    // Update the scene.
    for (auto& obj : m_scene)
    {
        Model::UpdateEffectMatrices(obj.effects, obj.world, m_view, m_proj);
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update or during exit.
    if (m_timer.GetFrameCount() == 0 || m_isExiting)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();
        
    DetermineResolution();

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap(), m_commonStates->Heap() };
    commandList->SetDescriptorHeaps(uint32_t(_countof(heaps)), heaps);

    {
        ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, L"Render");

        // Switch to the render and depth targets for the current resolution
        SetAndClearTargets(&m_frameViewportDynamic, &m_currentRezData.rtvDescriptor, &m_currentRezData.dsvDescriptor, ATG::ColorsLinear::Background);

        {
            ScopedPixEvent Scene(commandList, PIX_COLOR_DEFAULT, L"Scene");

            // Draw the scene.
            for (auto& obj : m_scene)
            {
                obj.model->DrawOpaque(commandList, obj.effects.begin());
            }
        }

        {
            ScopedPixEvent Copy(commandList, PIX_COLOR_DEFAULT, L"Copy to swap chain");

            // Copy from the dynamically-sized render target to the upper-left corner of the swap chain.
            // In a real game, the last full-screen render pass would likely write directly to the swap chain,
            // but this sample is simple enough that it can render everything in one pass.
            D3D12_RESOURCE_BARRIER barriers[2] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST),
                CD3DX12_RESOURCE_BARRIER::Transition(m_currentRezData.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE)
            };
            commandList->ResourceBarrier(_countof(barriers), barriers);

            CD3DX12_TEXTURE_COPY_LOCATION dest(m_deviceResources->GetRenderTarget(), 0);
            CD3DX12_TEXTURE_COPY_LOCATION src(m_currentRezData.renderTarget.Get(), 0);

            commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[0].Transition.StateAfter = barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commandList->ResourceBarrier(_countof(barriers), barriers);
        }

        RenderUI();
    }

    
    // Kickoff the GPU work, to avoid forcing a full frame of latency
    // A real title would naturally have multiple kickoffs, so it wouldn't need an artificial one
    commandList->KickoffX();

    // Show the new frame.
    {

        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

        m_deviceResources->Present();

        m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

        PIXEndEvent();
    }
}

void Sample::SetAndClearTargets(
    const D3D12_VIEWPORT* viewport,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvDescriptor,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptor,
    const float* colorRGBA)
{
    auto commandList = m_deviceResources->GetCommandList();

    ScopedPixEvent Clear(commandList, PIX_COLOR_DEFAULT, L"SetAndClearTargets");
    
    // Set the viewport and scissor rect.
    commandList->RSSetViewports(1, viewport);
    D3D12_RECT scissorRect =
    {
        0,                          // LONG    left;
        0,                          // LONG    top;
        (LONG)viewport->Width,      // LONG    right;
        (LONG)viewport->Height,     // LONG    bottom;
    };
    commandList->RSSetScissorRects(1, &scissorRect);

    // Set and clear the views.
    commandList->OMSetRenderTargets(1, rtvDescriptor, false, dsvDescriptor);
    commandList->ClearRenderTargetView(*rtvDescriptor, colorRGBA, 0, nullptr);
    if (dsvDescriptor)
    {
        commandList->ClearDepthStencilView(*dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }
}

void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();

    ScopedPixEvent RenderUI(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    auto rtvDescriptor = m_deviceResources->GetUIRenderTargetView();
    
    // Clear UI plane alpha to 0.0f, so it doesn't overwrite background plane
    SetAndClearTargets(&m_frameViewportIdeal, &rtvDescriptor, nullptr, DirectX::Colors::Transparent);

    // Render predefined UI (background panels, etc.)
    {
        m_uiManager.Render();
    }
}

void Sample::DetermineResolution()
{
    char buf[1024] = "";
    sprintf_s(buf, "Resolution %ux%u", m_frameWidth, m_frameHeight);
    m_text1->SetDisplayText(buf);

    m_frameViewportDynamic.Width = (float)m_frameWidth;
    m_frameViewportDynamic.Height = (float)m_frameHeight;

    m_currentRezData = m_resolutionSet.GetOrCreateResourcesForResolution(m_frameWidth, m_frameHeight);

    m_deviceResources->SetSceneRenderSize(static_cast<long>(m_frameWidth), static_cast<long>(m_frameHeight));
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
    m_uiInputState.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // State objects
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    // Create heap
    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        SRVDescriptorHeapIndex::SRV_Count);

    // Load models from disk.
    m_models.resize(_countof(c_modelPaths));
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i] = Model::CreateFromSDKMESH(device, c_modelPaths[i]);
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Optimize meshes for rendering
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i]->LoadStaticBuffers(device, resourceUpload);
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    auto texOffsets = std::vector<size_t>(m_models.size());
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        size_t _;
        m_srvPile->AllocateRange(m_models[i]->textureNames.size(), texOffsets[i], _);

        m_models[i]->LoadTextures(*m_textureFactory, int(texOffsets[i]));
    }

    // Instantiate objects from basic scene definition.
    auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());
    auto rtState = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    auto objectPSD = EffectPipelineStateDescription(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    m_scene.resize(_countof(c_sceneDefinition));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        size_t index = c_sceneDefinition[i].modelIndex;

        assert(index < m_models.size());
        auto& model = *m_models[index];

        m_scene[i].world = c_sceneDefinition[i].world;
        m_scene[i].model = &model;
        m_scene[i].effects = model.CreateEffects(effectFactory, objectPSD, objectPSD, int(texOffsets[index]));

        std::for_each(
            m_scene[i].effects.begin(),
            m_scene[i].effects.end(),
            [&](std::shared_ptr<IEffect>& e)
            {
                static_cast<BasicEffect*>(e.get())->SetEmissiveColor(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
            });
    }

    // UI
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    auto backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_uiSpriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    // Disable warning about barrier validation, without disabling barrier validation
#ifdef _GAMING_XBOX_SCARLETT
    device->SetDebugErrorFilterX(0x6BCA2E89, D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);
#else
    device->SetDebugErrorFilterX(0xDA62126E, D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Because the window size doesn't change on Xbox, everything here is actually based on fixed values.

    auto device = m_deviceResources->GetD3DDevice();

    // The variable sizes for dynamic resolution
    m_frameWidth = c_frameWidthMaximum;
    m_frameHeight = c_frameHeightMaximum;
    m_frameViewportDynamic.TopLeftX = 0.0f;
    m_frameViewportDynamic.TopLeftY = 0.0f;
    m_frameViewportDynamic.Width = static_cast<float>(m_frameWidth);
    m_frameViewportDynamic.Height = static_cast<float>(m_frameHeight);
    m_frameViewportDynamic.MinDepth = 0.0f;
    m_frameViewportDynamic.MaxDepth = 1.0f;

    // Fixed viewport for UI rendering
    m_frameViewportIdeal.TopLeftX = 0.0f;
    m_frameViewportIdeal.TopLeftY = 0.0f;
    m_frameViewportIdeal.Width = static_cast<float>(c_frameWidthMaximum);
    m_frameViewportIdeal.Height = static_cast<float>(c_frameHeightMaximum);
    m_frameViewportIdeal.MinDepth = 0.0f;
    m_frameViewportIdeal.MaxDepth = 1.0f;

    m_uiManager.SetWindowSize(static_cast<int>(c_frameWidthMaximum), static_cast<int>(c_frameHeightMaximum));
    
    // Set up dynamic render targets
    {
        uint32_t widthSteps = CalculateResolutionSteps(c_frameWidthMinimum, c_frameWidthMaximum, c_frameWidthIncrement);
        uint32_t heightSteps = CalculateResolutionSteps(c_frameHeightMinimum, c_frameHeightMaximum, c_frameHeightIncrement);

        m_currentRezData = m_resolutionSet.InitializeMaxResolution(
            device,
            c_frameWidthMaximum,
            c_frameHeightMaximum,
            std::max(widthSteps, heightSteps),  // Maximum possible resolutions we can adjust to
            m_deviceResources->GetBackBufferFormat(),
            m_deviceResources->GetDepthBufferFormat()
        );
    }

    // Set UI sprite viewport
    m_uiSpriteBatch->SetViewport(m_frameViewportIdeal);

    // Set camera parameters.
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, c_frameWidthMaximum / static_cast<float>(c_frameHeightMaximum), 0.1f, 500.0f);

    // Begin uploading texture resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"Assets/Fonts/SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(SRVDescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(SRVDescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"Assets/Fonts/XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(SRVDescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(SRVDescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}
#pragma endregion

#pragma region Sample body
void Sample::SetResolution(uint32_t width, uint32_t height)
{
    // Change rendering resolution
    m_frameWidth = width;
    m_frameHeight = height;
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / static_cast<float>(height), 0.1f, 500.0f);

    // Adjust UI-related
    m_uiManager.SetWindowSize(static_cast<int>(width), static_cast<int>(height));

    auto image = m_uiManager.FindTypedById<UIImage>(ID("xbox_logo"));

    image->SetStyleId((width > height) ? ID("LogoHorizontal") : ID("LogoVertical"));

    // For image reverse scale multiplier that UITK applies to UI elements
    int uiReferenceResolution[2] = {};
    m_uiManager.GetReferenceResolution(uiReferenceResolution);
    auto scaleW = float(width) / float(uiReferenceResolution[0]);
    auto scaleH = float(height) / float(uiReferenceResolution[1]);
    auto scale = fabsf(1.0f - scaleW) < fabsf(1.0f - scaleH) ? scaleW : scaleH;

    // Maintain 16:9 aspect ratio of source image
    auto aspectHeight = float(width) * 9.f / 16.f;
    image->SetRelativeSizeInRefUnits(Vector2(float(width) / scale, aspectHeight / scale));

    // Center image vertically
    image->SetRelativePositionInRefUnits(Vector2(0.f, (height - aspectHeight) / 2.f));
}

void Sample::RetrieveStreamingCharacteristics()
{
    // Retrieve some dimensions to show in the UI
    if (m_clients.size() > 0)
    {
        // Assumes only a single connected client at a time
        XGameStreamingClientId clientId = m_clients.begin()->first;

        uint32_t clientWidthMm = 0;
        uint32_t clientHeightMm = 0;
        char buf[1024] = "";
        if (SUCCEEDED(XGameStreamingGetStreamPhysicalDimensions(clientId, &clientWidthMm, &clientHeightMm)))
        {
            sprintf_s(buf, "Streaming client width %u height %u", clientWidthMm, clientHeightMm);
            DebugPrint(buf);
            m_text2->SetDisplayText(buf);
        }

        XGameStreamingDisplayDetails details{};
        if (SUCCEEDED(XGameStreamingGetDisplayDetails(clientId, c_frameWidthMaximum * c_frameHeightMaximum, c_aspectWidest, c_aspectTallest, &details)))
        {
            sprintf_s(buf, "Max pixels %u max %ux%u preferred %ux%u flags %u", details.maxPixels, details.maxWidth, details.maxHeight, details.preferredWidth, details.preferredHeight, details.flags);
            DebugPrint(buf);
            m_text3->SetDisplayText(buf);
        }
    }
    else
    {
        if (m_text2)
        {
            m_text2->SetDisplayText("Not streaming");
        }
    }
}

void Sample::SetAutoResolution()
{
    // Set the rendering resolution and set streaming resolution to match
    if (m_clients.size() > 0)
    {
        // Assumes only a single connected client at a time
        XGameStreamingClientId clientId = m_clients.begin()->first;

        XGameStreamingDisplayDetails details{};
        if (SUCCEEDED(XGameStreamingGetDisplayDetails(clientId, c_frameWidthMaximum * c_frameHeightMaximum, c_aspectWidest, c_aspectTallest, &details)))
        {
            m_uiManager.FindTypedById<UIStaticText>(ID("text4"))->SetDisplayText("");

            SetResolution(details.preferredWidth, details.preferredHeight);
            // No need to check for failure as preferred values will always be valid
            XGameStreamingSetResolution(details.preferredWidth, details.preferredHeight);
        }
    }
}
#pragma endregion
