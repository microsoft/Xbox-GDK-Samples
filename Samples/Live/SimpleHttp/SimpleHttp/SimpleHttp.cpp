//--------------------------------------------------------------------------------------
// SimpleCrossGenMPSD.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleHttp.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

#include "HttpManager.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

// TODO: Replace with your server's host name
#define MY_HOST "Xsts2018Sample.azurewebsites.net"

// TODO: Update this if you are using your own consumable
//       to test b2b consume functionality of the
//       Xsts 2018 Server Sample.
#define TEST_CONSUMABLE_STOREID "9NSV8487K9JR"

namespace
{
    // Public site setup for WebSocket experimentation
    const char* c_httpWebAddress = "https://www.msn.com";
    const char* c_xblWebAddress = "https://profile.xboxlive.com/users/me/profile/settings?settings=GameDisplayName";
    const char* c_gameServiceAddress = "https://" MY_HOST "/api/getclaims";

    // Other endpoints that you can call from this client side sample
    // see the documentation for the Game Service Sample for more details
    /*
    L"https://" MY_HOST "/api/getclaims"
    L"https://" MY_HOST "/api/b2bfriends"
    L"https://" MY_HOST "/api/collections/query"
    L"https://" MY_HOST "/api/collections/query?ids=" TEST_CONSUMABLE_STOREID ":0010"  //  Note the SKU ID for the test product in the sample's sandbox is 0010, but other products may be different                                                                                   //  likely be 0001 or another value.  Check the results in collections to find it.
    L"https://" MY_HOST "/api/collections/consume?id=" TEST_CONSUMABLE_STOREID "&quantity=1"
    L"https://" MY_HOST "/api/collections/RetryPendingConsumes"
    L"https://" MY_HOST "/api/getGDPRList";
    */


}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SimpleHttp");

    //  Create a Lambda that we can pass so that the Sample's log function can be used
    //  to display any information from the HttpManager
    m_httpManager = new HttpManager([&](const std::string& str) { Log(str); });
}

Sample::~Sample()
{
    CleanupTaskQueue();

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_httpManager)
    {    
        delete m_httpManager;
        m_httpManager = nullptr;
    }
}

void Sample::CleanupTaskQueue()
{
    if (m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
        m_taskQueue = nullptr;
    }
}

void Sample::InitializeTaskQueue()
{
    assert(nullptr == m_taskQueue);

    auto hr = XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue);
    if (FAILED(hr))
    {
        Log("Sample::InitializeTaskQueue: Task queue creation failed with HRESULT = 0x%08x", hr);
    }
    else
    {
        assert(nullptr != m_taskQueue);
    }
}

void Sample::InitializeLiveResources()
{
    m_liveResources = std::make_shared<ATG::LiveResources>(m_taskQueue);
    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {
            Log("LiveResources: Error HRESULT: %08x", error);
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
    });

    m_liveResources->Initialize();
}

void Sample::PumpTaskQueue()
{
    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0)) {}
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
#ifdef _DEBUG
    DebugInit();
#endif

    Log("Initialize");

    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
#ifdef _GAMING_DESKTOP
    m_mouse->SetWindow(window);
#endif

    m_deviceResources->SetWindow(window, width, height);
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveInfoHUD->Initialize();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_httpRequestButton = m_uiManager.FindTypedById<UIButton>(ID("HttpRequestButton"));
    m_xblRequestButton = m_uiManager.FindTypedById<UIButton>(ID("XBLRequestButton"));
    m_gameServiceRequestButton = m_uiManager.FindTypedById<UIButton>(ID("GameServiceRequestButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_httpRequestButton->SetEnabled(false);
    m_xblRequestButton->SetEnabled(false);
    m_gameServiceRequestButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeTaskQueue();
    InitializeLiveResources();
    LoginToXboxLive(true);
    InitializeUIEventHandlers();
    m_asyncOpWidget->Hide();
}

void Sample::LoginToXboxLive(bool silentAuth)
{
    Log("LoginToXboxLive");

    m_asyncOpWidget->Show(u8"Logging into Xbox Live");

    m_liveResources->SetUserChangedCallback([this](XUserHandle /*userHandle*/)
    {
        if (m_liveResources->IsUserSignedIn())
        {
            OnXboxLiveLoginComplete();
        }
    });

    if (silentAuth)
    {
        m_liveResources->SignInSilently();
    }
    else
    {
        m_liveResources->SignInWithUI();
    }
}

void Sample::OnXboxLiveLoginComplete()
{
    m_asyncOpWidget->Hide();

    m_liveInfoHUD->SetUser(m_liveResources->GetUser(), m_taskQueue);

    m_httpRequestButton->SetEnabled(true);
    m_xblRequestButton->SetEnabled(true);
    m_gameServiceRequestButton->SetEnabled(true);

    m_uiManager.SetFocus(m_httpRequestButton);
}

void Sample::InitializeUIEventHandlers()
{
    m_httpRequestButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnHttpRequestButtonPressed();
    });

    m_xblRequestButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnXBLRequestButtonPressed();
    });

    m_gameServiceRequestButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnGameServiceRequestButtonPressed();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });
}

void Sample::OnHttpRequestButtonPressed()
{
    Log("Sample::OnHttpRequestButtonPressed:\n");

    if (m_httpManager == nullptr)
    {
        return;
    }

    std::vector<HttpHeader> headers;
    uint8_t* body = nullptr;

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "GET", c_httpWebAddress, headers, body, 0, [this](HttpRequestContext* context)
    {
        if (context)
        {
            Log("Response status code: " + std::to_string(context->responseStatusCode));
            Log("Response body size: " + std::to_string(context->responseBody.size()));
            Log("Request completed.");
        }
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        Log("Sample::OnHttpRequestButtonPressed: MakeHttpRequestAsync failed with HRESULT = 0x%08x", hr);
    }
}

void Sample::OnXBLRequestButtonPressed()
{
    Log("Sample::OnXBLRequestButtonPressed:\n");

    if (m_httpManager == nullptr)
    {
        return;
    }

    if (m_liveResources->IsUserSignedIn() == false)
    {
        m_liveResources->SignInWithUI();
    }
    else
    {
        std::vector<HttpHeader> headers;
        headers.emplace_back(HttpHeader("X-XBL-Contract-Version", "2"));
        headers.emplace_back(HttpHeader("Content-Type", "application/json"));

        HRESULT hr = m_httpManager->MakeHttpRequestAsync(m_liveResources->GetUser(), "GET", c_xblWebAddress, headers, nullptr, 0, [this](HttpRequestContext* context)
        {
            if (context)
            {
                auto rawBody = context->responseBody.data();
                auto bodyLen = context->responseBody.size();
                std::string body(rawBody, rawBody + bodyLen);

                Log("Response status code: " + std::to_string(context->responseStatusCode));
                Log("Response body size: " + std::to_string(context->responseBody.size()));

                Log("Response headers:");
                for (const HttpHeader& h :context->responseHeaders)
                {
                    Log("    " + h.ToString());
                }

                Log("Response body:");
                Log("    " + body);

                Log("Request completed.");
            }
        });

        if (hr != E_PENDING && FAILED(hr))
        {
            Log("Sample::OnXBLRequestButtonPressed: MakeHttpRequestAsync failed with HRESULT = 0x%08x", hr);
        }
    }
}

void Sample::OnGameServiceRequestButtonPressed()
{
    Log("Sample::OnGameServiceRequestButtonPressed:\n");

    if (m_httpManager == nullptr)
    {
        return;
    }

    if (m_liveResources->IsUserSignedIn() == false)
    {
        m_liveResources->SignInWithUI();
    }
    else
    {
        std::vector<HttpHeader> headers;
        headers.emplace_back(HttpHeader("X-XBL-Contract-Version", "2"));
        headers.emplace_back(HttpHeader("Content-Type", "application/json"));

        HRESULT hr = m_httpManager->MakeHttpRequestAsync(m_liveResources->GetUser(), "GET", c_gameServiceAddress, headers, nullptr, 0, [this](HttpRequestContext* context)
        {
            if (context)
            {
                auto rawBody = context->responseBody.data();
                auto bodyLen = context->responseBody.size();
                std::string body(rawBody, rawBody + bodyLen);

                Log("Response status code: " + std::to_string(context->responseStatusCode));
                Log("Response body size: " + std::to_string(context->responseBody.size()));

                Log("Response headers:");
                for (const HttpHeader& h : context->responseHeaders)
                {
                    Log("    " + h.ToString());
                }

                Log("Response body:");
                Log("    " + body);

                Log("Request completed.");
            }
        });

        if (hr != E_PENDING && FAILED(hr))
        {
            Log("Sample::OnGameServiceRequestButtonPressed: MakeHttpRequestAsync failed with HRESULT = 0x%08x", hr);
        }
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    PumpTaskQueue();

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

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_liveResources->IsUserSignedIn())
            {
                m_liveResources->SignInSilently();
            }
            else
            {
                m_liveResources->SignInWithUI();
            }
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

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            m_liveResources->SignInWithUI();
        }
    }

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);
    m_asyncOpWidget->Update(elapsedTime);

    if (m_httpManager)
    {
        m_httpManager->Update();
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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

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
    m_liveResources->Refresh();
    m_inputState.Reset();
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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion
