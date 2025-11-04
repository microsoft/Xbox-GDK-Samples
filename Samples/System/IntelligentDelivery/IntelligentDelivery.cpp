//--------------------------------------------------------------------------------------
// IntelligentDelivery.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "IntelligentDelivery.h"

// ATG TK
#include "ATGColors.h"
#include "Texture.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    const std::map<XPackageChunkAvailability, std::string> c_packageChunkAvailabilityMap =
    {
        { XPackageChunkAvailability::Installable, "Installable"},
        { XPackageChunkAvailability::Pending, "Pending"},
        { XPackageChunkAvailability::Ready, "Ready"},
        { XPackageChunkAvailability::Unavailable, "Unavailable"},
    };

    std::string PackageChunkAvailabilityToString(XPackageChunkAvailability& availability)
    {
        return c_packageChunkAvailabilityMap.at(availability);
    }

    std::string SelectorToString(XPackageChunkSelector& selector)
    {
        std::string str;
        switch (selector.type)
        {
        case XPackageChunkSelectorType::Chunk:
            str += "Chunk ";
            str += std::to_string(selector.chunkId);
            break;
        case XPackageChunkSelectorType::Feature:
            str += "Feature ";
            str += selector.feature;
            break;
        case XPackageChunkSelectorType::Language:
            str += "Language ";
            str += selector.language;
            break;
        case XPackageChunkSelectorType::Tag:
            str += "Tag ";
            str += selector.tag;
            break;
        default:
            str += "Invalid Selector type";
            break;
        }

        return str;
    }

    struct FeatureInfo
    {
        const char* id;
        const char* name;
    };

    struct ChunkInfo
    {
        uint16_t id;
        const char* name;
        const char* specifier;
        XPackageChunkSelectorType type;
    };

    FeatureInfo g_features[] =
    {
        { "feature1", "Required"    },
        { "feature2", "Solo race"   },
        { "feature3", "Battle mode" },
#if _GAMING_DESKTOP
        { "feature4", "Bonus + 8K"  }
#else
        { "feature4", "Bonus"       }
#endif
    };

    ChunkInfo g_chunks[] =
    {
        { 1000, "Launch",           nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1001, "Story data",       nullptr,        XPackageChunkSelectorType::Chunk      },
#ifdef _GAMING_XBOX_SCARLETT
        { 1002, "Xbox Series S",    nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1003, "Xbox Series X",    nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1004, "English",          "en",           XPackageChunkSelectorType::Language   },
        { 1005, "French",           "fr",           XPackageChunkSelectorType::Language   },
        { 1006, "Japanese",         "ja",           XPackageChunkSelectorType::Language   },
#elif _GAMING_XBOX_XBOXONE
        { 1002, "Xbox One X",       nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1003, "Xbox One",         nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1004, "English",          "en",           XPackageChunkSelectorType::Language   },
        { 1005, "German",           "de",           XPackageChunkSelectorType::Language   },
        { 1006, "Korean",           "ko",           XPackageChunkSelectorType::Language   },
#elif _GAMING_DESKTOP
        { 1002, "Textures",         nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1003, "8K Textures",      nullptr,        XPackageChunkSelectorType::Chunk      },
        { 1004, "English",          "en",           XPackageChunkSelectorType::Language   },
        { 1005, "Spanish",          "es",           XPackageChunkSelectorType::Language   },
        { 1006, "Chinese",          "zh-hant",      XPackageChunkSelectorType::Language   },
#endif
        { 1007, "Bonus",            nullptr,        XPackageChunkSelectorType::Chunk      },
        { 2000, "Mode 2",           nullptr,        XPackageChunkSelectorType::Chunk      },
        { 2001, "Map 1",            nullptr,        XPackageChunkSelectorType::Chunk      },
        { 2002, "Map 2",            nullptr,        XPackageChunkSelectorType::Chunk      },
        { 2003, "Bonus",            nullptr,        XPackageChunkSelectorType::Chunk      },
        { 3000, "Mode 3",           nullptr,        XPackageChunkSelectorType::Chunk      },
        { 3001, "Shared",           nullptr,        XPackageChunkSelectorType::Chunk      },
        { 3002, "Bonus",            nullptr,        XPackageChunkSelectorType::Chunk      },
        { 4000, "Bonus",            nullptr,        XPackageChunkSelectorType::Chunk      }
    };
}

Sample::Sample() noexcept(false) :
    m_packageIdentifier{},
    m_packageIdentifierValid(false),
    m_pimHandleOverall(nullptr),
    m_pimHandleOverallActive(false),
    m_taskQueue(nullptr),
    m_frame(0),
    m_showConsole(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue)
    );
}

Sample::~Sample()
{
    if(m_pimHandleOverallActive)
    {
        XPackageCloseInstallationMonitorHandle(m_pimHandleOverall);
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if(m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // UITK load layout
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/layout.json");

    if (!XPackageIsPackagedProcess())
    {
        const char* msg = "WARNING: Title is not packaged, XPackage API will not function";
        ConsoleWriteLine(msg);

        auto statusLabel = m_uiManager.FindTypedById<UIStaticText>(ID("StatusLabel"));
        statusLabel->SetDisplayText(msg);
    }
    else
    {
        auto statusLabel = m_uiManager.FindTypedById<UIStaticText>(ID("StatusLabel"));
        statusLabel->SetDisplayText("Select a feature to see and install/remove associated chunks");
    }

    m_packageIdentifierValid =
        SUCCEEDED(XPackageGetCurrentProcessPackageIdentifier(ARRAYSIZE(m_packageIdentifier), m_packageIdentifier));

    if (!m_packageIdentifierValid)
    {
        return;
    }

    // Get the locale name for the system using XPackageGetUserLocale(). This will return the locale selected through the 
    // settings app only if that locale has been added to the Resources section of the application's MicrosoftGame.config.
    // In case the resource is absent from the config, this API will return the first locale in the Resource Language.
    char localeName[LOCALE_NAME_MAX_LENGTH] = { 0 };

    auto hr = XPackageGetUserLocale(ARRAYSIZE(localeName), localeName);

    ConsoleWriteLine("XPackageGetUserLocale %s result 0x%x", localeName, hr);

    if (SUCCEEDED(hr))
    {
        auto languageLabel = m_uiManager.FindTypedById<UIStaticText>(ID("LanguageLabel"));
        languageLabel->SetDisplayText("Language setting from XPackageGetUserLocale API: " + std::string(localeName));
    }

#pragma region UI Setup
    auto mainPanel = m_uiManager.FindTypedById<UIPanel>(ID("MainPanel"));

    auto feature1Frame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature1Frame"));
    auto feature2Frame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature2Frame"));
    auto feature3Frame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature3Frame"));
    auto feature4Frame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature4Frame"));
    auto feature1HDFrame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature1HDFrame"));
    auto feature2HDFrame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature2HDFrame"));
    auto feature3HDFrame = mainPanel->GetTypedChildById<UIPanel>(ID("Feature3HDFrame"));

    auto feature1Button = mainPanel->GetTypedChildById<UIStackPanel>(ID("FeatureSelectors"))->GetTypedChildById<UIButton>(ID("Feature1"));
    auto feature2Button = mainPanel->GetTypedChildById<UIStackPanel>(ID("FeatureSelectors"))->GetTypedChildById<UIButton>(ID("Feature2"));
    auto feature3Button = mainPanel->GetTypedChildById<UIStackPanel>(ID("FeatureSelectors"))->GetTypedChildById<UIButton>(ID("Feature3"));
    auto feature4Button = mainPanel->GetTypedChildById<UIStackPanel>(ID("FeatureSelectors"))->GetTypedChildById<UIButton>(ID("Feature4"));

#ifdef _GAMING_DESKTOP
    auto highlightState = UIButton::State::Hovered;
#else
    auto highlightState = UIButton::State::Focused;
#endif

    feature1Button->ButtonState().AddListenerWhen(highlightState,
        [feature1Frame](UIButton*)
    {
        feature1Frame->SetStyleId(ID("RedFrameHighlightStyle"));
    });
    feature1Button->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [feature1Frame](UIButton*)
    {
        feature1Frame->SetStyleId(ID("RedFrameStyle"));
    });
    feature2Button->ButtonState().AddListenerWhen(highlightState,
        [feature2Frame](UIButton*)
    {
        feature2Frame->SetStyleId(ID("GreenFrameHighlightStyle"));
    });
    feature2Button->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [feature2Frame](UIButton*)
    {
        feature2Frame->SetStyleId(ID("GreenFrameStyle"));
    });
    feature3Button->ButtonState().AddListenerWhen(highlightState,
        [feature3Frame](UIButton*)
    {
        feature3Frame->SetStyleId(ID("BlueFrameHighlightStyle"));
    });
    feature3Button->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [feature3Frame](UIButton*)
    {
        feature3Frame->SetStyleId(ID("BlueFrameStyle"));
    });
    feature4Button->ButtonState().AddListenerWhen(highlightState,
        [feature4Frame, feature1HDFrame, feature2HDFrame, feature3HDFrame](UIButton*)
    {
        feature4Frame->SetStyleId(ID("GrayFrameHighlightStyle"));
        feature1HDFrame->SetStyleId(ID("RedFrameHighlightStyle"));
        feature2HDFrame->SetStyleId(ID("GreenFrameHighlightStyle"));
        feature3HDFrame->SetStyleId(ID("BlueFrameHighlightStyle"));
    });
    feature4Button->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [feature4Frame, feature1HDFrame, feature2HDFrame, feature3HDFrame](UIButton*)
    {
        feature4Frame->SetStyleId(ID("GrayFrameStyle"));
        feature1HDFrame->SetStyleId(ID("RedFrameStyle"));
        feature2HDFrame->SetStyleId(ID("GreenFrameStyle"));
        feature3HDFrame->SetStyleId(ID("BlueFrameStyle"));
    });

    std::vector<std::shared_ptr<UIButton>> buttons;
    buttons.push_back(feature1Button);
    buttons.push_back(feature2Button);
    buttons.push_back(feature3Button);
    buttons.push_back(feature4Button);

    std::map<uint16_t, std::shared_ptr<UIButton>> chunkButtons;
    chunkButtons[g_chunks[0].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk1"));
    chunkButtons[g_chunks[1].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk2"));
    chunkButtons[g_chunks[2].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk3"));
    chunkButtons[g_chunks[3].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk4"));
    chunkButtons[g_chunks[4].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk5"));
    chunkButtons[g_chunks[5].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk6"));
    chunkButtons[g_chunks[6].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk7"));
    chunkButtons[g_chunks[7].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature1Chunks"))->GetTypedChildById<UIButton>(ID("Chunk8"));

    chunkButtons[g_chunks[8].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature2Chunks"))->GetTypedChildById<UIButton>(ID("Chunk9"));
    chunkButtons[g_chunks[9].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature2Chunks"))->GetTypedChildById<UIButton>(ID("Chunk10"));
    chunkButtons[g_chunks[10].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature2Chunks"))->GetTypedChildById<UIButton>(ID("Chunk11"));
    chunkButtons[g_chunks[11].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature2Chunks"))->GetTypedChildById<UIButton>(ID("Chunk12"));

    chunkButtons[g_chunks[12].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature3Chunks"))->GetTypedChildById<UIButton>(ID("Chunk13"));
    chunkButtons[g_chunks[13].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature3Chunks"))->GetTypedChildById<UIButton>(ID("Chunk14"));
    chunkButtons[g_chunks[14].id] = mainPanel->GetTypedChildById<UIStackPanel>(ID("Feature3Chunks"))->GetTypedChildById<UIButton>(ID("Chunk15"));

    chunkButtons[g_chunks[15].id] = mainPanel->GetTypedChildById<UIButton>(ID("Chunk16"));

#pragma endregion // UI setup

    // Create Feature objects
    for (uint16_t i = 0; i < ARRAYSIZE(g_features); ++i)
    {
        auto& button = buttons.at(i);
        std::unique_ptr<UIChunk> feature = std::make_unique<UIChunk>(
            m_packageIdentifier,
            i,
            g_features[i].name,
            g_features[i].id,
            XPackageChunkSelectorType::Feature,
            button);

        m_UIFeatures.insert(std::make_pair(g_features[i].id, std::move(feature)));
    }

    // Assign button actions
    for (auto& uiFeature : m_UIFeatures)
    {
        auto& feature = uiFeature.second;
        auto button = feature->GetButton();

        button->ButtonState().AddListenerWhen(highlightState,
            [this, &feature](UIButton*)
        {
            feature->ShowStatus(true);

            SetLegend(feature->IsInstallable() ? "[A] Install feature" :
                feature->IsRemovable() ? "[A] Remove feature" : "");
        });
        button->ButtonState().AddListenerWhen(UIButton::State::Normal,
            [&feature](UIButton*)
        {
            feature->ShowStatus(false);
        });
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, &feature](UIButton*)
        {
            HandlePressed(feature.get());
        });
    }

    // Create Chunk objects
    for (unsigned int i = 0; i < ARRAYSIZE(g_chunks); ++i)
    {
        auto chunkId = g_chunks[i].id;
        std::unique_ptr<UIChunk> chunk = std::make_unique<UIChunk>(
            m_packageIdentifier,
            chunkId,
            g_chunks[i].name,
            g_chunks[i].specifier,
            g_chunks[i].type,
            chunkButtons[chunkId]);

        m_UIChunks.insert(std::make_pair(chunkId, std::move(chunk)));
    }

    // Assign button actions
    for (auto& c : m_UIChunks)
    {
        auto& chunk = c.second;

        chunk->GetButton()->ButtonState().AddListenerWhen(highlightState,
            [this, &chunk](UIButton*)
        {
            chunk->ShowStatus(true);

            SetLegend(chunk->IsInstallable()? "[A] Install chunk" :
                chunk->IsRemovable()? "[A] Remove chunk" : "");
        });

        chunk->GetButton()->ButtonState().AddListenerWhen(UIButton::State::Normal,
            [&chunk](UIButton*)
        {
            chunk->ShowStatus(false);
        });

        chunk->GetButton()->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, &chunk](UIButton*)
        {
            HandlePressed(chunk.get());
        });
    }

    if (XPackageIsPackagedProcess())
    {
        // Start monitoring installation progress
        // This can only work when run in packaged form

        // Monitor features and chunks
        for (auto& feature : m_UIFeatures)
        {
            WatchChunkOrFeature(feature.second.get());
        }

        for (auto& chunk : m_UIChunks)
        {
            WatchChunkOrFeature(chunk.second.get());
        }

        // Monitor full package
        SetupTransferPackageProgress();
    }
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
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0));
        
    // update our UI input state and managed layout

    float elapsedTime = float(timer.GetElapsedSeconds());

    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    auto keys = m_uiInputState.GetKeyboardKeys();

    if (m_uiInputState.GetGamePadButtons(0).view == GamePad::ButtonStateTracker::RELEASED ||
        keys.IsKeyPressed(DirectX::Keyboard::OemTilde))
    {
        m_showConsole = !m_showConsole;
    }

    if(m_uiInputState.GetGamePadButtons(0).y == GamePad::ButtonStateTracker::RELEASED ||
        keys.IsKeyPressed(DirectX::Keyboard::Y))
    {
        Enumerate(XPackageChunkSelectorType::Feature);
        Enumerate(XPackageChunkSelectorType::Chunk);
        Enumerate(XPackageChunkSelectorType::Language);
        Enumerate(XPackageChunkSelectorType::Tag);
    }

    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
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
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    if (m_showConsole)
    {
        m_console->Render(commandList);
    }

    // Render the UI scene
    m_uiManager.Render();

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, Descriptors::Count);

    SpriteBatchPipelineStateDescription spritePsoDesc(rtState, &CommonStates::AlphaBlend);
    ResourceUploadBatch upload(device);
    upload.Begin();
   
    m_console = std::make_unique <DX::TextConsole>(
        device, upload, rtState,
        L"Assets/Fonts/Courier_16.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    // Set up text console
    m_console->SetDebugOutput(true);
    m_console->SetForegroundColor(DirectX::Colors::Yellow);

    // Create the style renderer for the UI manager to use for rendering the UI scene styles
    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    upload.End(commandQueue);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();

    m_console->SetViewport(vp);

    // Notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);

    m_console->SetWindow(SimpleMath::Viewport::ComputeTitleSafeArea(UINT(os.right), UINT(os.bottom)));
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
void Sample::GetDefaultSize(int& width, int& height) const
{
    width = 1920;
    height = 1080;
}
#pragma endregion

void Sample::SetLegend(const char* text)
{
    m_uiManager.FindTypedById<UIStaticText>(ID("legend"))->SetDisplayText(text);
}

void Sample::HandlePressed(UIChunk* selection)
{
    auto selector = selection->GetSelector();
    auto label = selection->GetName();

    ConsoleWriteLine("\"%s\" pressed! (selector %s)", label, SelectorToString(selector).c_str());

    if (selection->IsInstallable())
    {
        // Install chunk or feature
        struct Context
        {
            Sample *pThis;
            std::string label;
            XPackageChunkSelector selector;
            UIChunk* chunk;
        };

        auto asyncBlock = new XAsyncBlock{};
        asyncBlock->queue = m_taskQueue;
        asyncBlock->context = new Context{ this, label, selector, selection };
        asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            auto context = reinterpret_cast<Context*>(asyncBlock->context);
            auto pThis = context->pThis;

            XPackageInstallationMonitorHandle pimHandle;
            HRESULT result = XPackageInstallChunksResult(asyncBlock, &pimHandle);

            pThis->ConsoleWriteLine("XPackageInstallChunksResult (%s) selector %s result 0x%x",
                context->label.c_str(), SelectorToString(context->selector).c_str(), result);

            if (SUCCEEDED(result))
            {
                pThis->CreateInstallationMonitor(context->chunk, context->selector);

                if (context->selector.type == XPackageChunkSelectorType::Feature)
                {
                    // Go through and create monitors for each chunk the feature contains
                    // For visualization purposes only, normally just monitoring the feature is sufficient
                    for (auto& c : pThis->m_UIChunks)
                    {
                        auto& chunk = c.second;
                        pThis->CreateInstallationMonitor(chunk.get(), chunk->GetSelector());
                    }
                }

                //Re-initialize the overall package transfer watching code so that the progress bar reflects the additional data coming down
                //(Including if the package had been shown as completely installed before)
                pThis->SetupTransferPackageProgress();
            }

            delete asyncBlock;
        };

        auto hr = XPackageInstallChunksAsync(
            m_packageIdentifier,
            1,
            &selector,
            1000,
            false, // false = do not suppress tcui prompt
            asyncBlock);

        if (FAILED(hr))
        {
            ConsoleWriteLine("XPackageInstallChunksAsync 0x%x", hr);
            delete asyncBlock;
        }
    }
    else if (selection->IsInstalled() && selection->IsRemovable())
    {
        // Remove feature or chunk
        auto hr = XPackageUninstallChunks(m_packageIdentifier, 1, &selector);

        ConsoleWriteLine("XPackageUninstallChunks %s selector %s result 0x%x",
            label, SelectorToString(selector).c_str(), hr);

        // Check if uninstall worked as the last language cannot be removed
        XPackageChunkAvailability availability;
        XPackageFindChunkAvailability(m_packageIdentifier, 1, &selector, &availability);

        ConsoleWriteLine("XPackageFindChunkAvailability %s availability %s result 0x%x",
            label, PackageChunkAvailabilityToString(availability).c_str(), hr);

        if (!selection->IsInstalled())
        {
            selection->SetProgress(0.0f);

            if (selector.type == XPackageChunkSelectorType::Feature)
            {
                // Go through and update chunks that were affected by removal
                // For visualization purposes only, normally just monitoring the feature is sufficient
                for (auto& c : m_UIChunks)
                {
                    auto& chunk = c.second;
                    if (!chunk->IsInstalled())
                    {
                        chunk->SetProgress(0.f);
                    }
                }
            }

            RefreshStatus();
        }
    }
    else if (selection->IsPending())
    {
        // Make attmept to prioritize installation of chunk or feature
        auto hr = XPackageChangeChunkInstallOrder(m_packageIdentifier, 1, &selector);

        ConsoleWriteLine("XPackageChangeChunkInstallOrder (%s) selector %s result 0x%x",
            label, SelectorToString(selector).c_str(), hr);
    }
    
}

void Sample::Enumerate(XPackageChunkSelectorType type)
{
    // Utility function to print out chunk or feature utility for a particular type

    if (type == XPackageChunkSelectorType::Feature)
    {
        auto hr = XPackageEnumerateFeatures(
            m_packageIdentifier,
            this,
            [](void* context, const XPackageFeature* feature)->bool
        {
            auto pThis = reinterpret_cast<Sample*>(context);

            XPackageChunkAvailability availability;
            XPackageChunkSelector selector;
            selector.type = XPackageChunkSelectorType::Feature;
            selector.feature = feature->id;
            auto hr = XPackageFindChunkAvailability(pThis->m_packageIdentifier, 1, &selector, &availability);

            if (hr != S_OK)
            {
                pThis->ConsoleWriteLine("XPackageFindChunkAvailability error 0x%x", hr);
            }

            pThis->ConsoleWriteLine("FEATURE: [%s] \"%s\" Tags: %s Availability: %s ",
                feature->id, (feature->hidden) ? "(hidden)" : feature->displayName, feature->tags,
                PackageChunkAvailabilityToString(availability).c_str());

            return true;
        });

        if (hr != S_OK)
        {
            ConsoleWriteLine("XPackageEnumerateFeatures error 0x%x", hr);
        }
    }

    auto hr = XPackageEnumerateChunkAvailability(
        m_packageIdentifier,
        type,
        this,
        [](void* context, const XPackageChunkSelector* selector, XPackageChunkAvailability availability)->bool
    {
        auto pThis = reinterpret_cast<Sample*>(context);
        
        pThis->ConsoleWriteLine("[%s] Availability: %s",
            SelectorToString(*(const_cast<XPackageChunkSelector*>(selector))).c_str(),
            PackageChunkAvailabilityToString(availability).c_str());

        return true;
    });

    if (hr != S_OK)
    {
        ConsoleWriteLine("XPackageEnumerateChunkAvailability error 0x%x", hr);
    }
}

void Sample::SetupTransferPackageProgress()
{
    // Create installation monitor for the full package
    // This is called when installing individual feature/chunks to adjust for new install size/progress
    if(m_pimHandleOverallActive)
    {
        return;
    }

    auto hr = XPackageCreateInstallationMonitor(m_packageIdentifier, 0, nullptr, 1000, m_taskQueue, &m_pimHandleOverall);
    m_pimHandleOverallActive = true;

    ConsoleWriteLine("XPackageCreateInstallationMonitor (%s) result 0x%x", m_packageIdentifier, hr);

    // Register a callback to monitor the overall installation state.  In GXDK, the progress-changed event
    // will also notify if installation completes.
    // Alternatively, you can manually check progress on your own by calling XPackageGetInstallationProgress()
    // whenever you want after creating the installation monitor.
    XTaskQueueRegistrationToken callbackToken;
    hr = XPackageRegisterInstallationProgressChanged(m_pimHandleOverall, this, 
        [](void* context, XPackageInstallationMonitorHandle pimHandleOverall)
    {
        Sample* pThis = static_cast<Sample*>(context);

        XPackageInstallationProgress progress;
        XPackageGetInstallationProgress(pimHandleOverall, &progress);

        auto pb = pThis->m_uiManager.FindTypedById<UIProgressBar>(ID("progressbarMain"));
        auto label = pThis->m_uiManager.FindTypedById<UIStaticText>(ID("progressbarLabel"));
        auto meter = pThis->m_uiManager.FindTypedById<UIStaticText>(ID("progressbarMeter"));

        label->SetDisplayText("Overall package install progress");
        meter->SetDisplayText(std::to_string(progress.installedBytes >> 20) + "/" + std::to_string(progress.totalBytes >> 20) + " MB");

        if(!progress.completed)
        {
            pb->SetProgressPercentage(static_cast<float>(static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
        }
        else
        {
            pb->SetProgressPercentage(1.f);

            //It's possible to get future updates after completed is true. This can happen if chunks are later added by the game. Because of this the monitor handle is kept active
            //XPackageCloseInstallationMonitorHandle(pimHandleOverall);
            //pThis->m_pimHandleOverallActive = false;

            pThis->RefreshStatus();
        }
    }, &callbackToken);

    ConsoleWriteLine("XPackageRegisterInstallationProgressChanged (%s) result 0x%x", m_packageIdentifier, hr);
}

void Sample::RefreshStatus()
{
    // Refresh the download sizes for each feature/chunk after an install/removal completion

    for (auto& feature : m_UIFeatures)
    {
        feature.second->EstimateDownloadSize();
        feature.second->UpdateStatus();
    }
    for (auto& chunk : m_UIChunks)
    {
        chunk.second->EstimateDownloadSize();
        chunk.second->UpdateStatus();
    }
}

void Sample::WatchChunkOrFeature(UIChunk* item)
{
    // Setup which chunk or feature to watch

    auto selector = item->GetSelector();

    // Check and update availability
    XPackageChunkAvailability availability;
    XPackageFindChunkAvailability(m_packageIdentifier, 1, &selector, &availability);

    // If already installed, then just set as so
    if(item->IsInstalled())
    {
        item->SetProgress(1.f);
    }

    // If pending, then setup install progress watching
    if(item->IsPending())
    {
        CreateInstallationMonitor(item, selector);
    }

    item->UpdateStatus();
}

void Sample::CreateInstallationMonitor(UIChunk* item, XPackageChunkSelector selector)
{
    // Create installation monitor
    XPackageInstallationMonitorHandle pimHandle;
    auto hr = XPackageCreateInstallationMonitor(
        m_packageIdentifier,
        1,
        &selector,
        1000,
        m_taskQueue,
        &pimHandle
    );

    ConsoleWriteLine("XPackageCreateInstallationMonitor %d selector %s result 0x%x", item->GetId(), SelectorToString(selector).c_str(), hr);

    // Register a callback to monitor the updates.  Update callbacks will also notify when install completes.
    XTaskQueueRegistrationToken callbackToken;
    hr = XPackageRegisterInstallationProgressChanged(pimHandle, item,
        [](void* context, XPackageInstallationMonitorHandle pimHandle)
    {
        UIChunk* item = static_cast<UIChunk*>(context);

        XPackageInstallationProgress progress;
        XPackageGetInstallationProgress(pimHandle, &progress);

        item->UpdateStatus();

        if (!progress.completed)
        {
            item->SetProgress(static_cast<float>(static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
        }
        else
        {
            if (item->IsInstalled())
            {
                item->SetProgress(1.f);
            }
            XPackageCloseInstallationMonitorHandle(pimHandle);
        }
    }, &callbackToken);

    ConsoleWriteLine("XPackageRegisterInstallationProgressChanged %d (%s) result 0x%x", item->GetId(), item->GetName(), hr);
}
