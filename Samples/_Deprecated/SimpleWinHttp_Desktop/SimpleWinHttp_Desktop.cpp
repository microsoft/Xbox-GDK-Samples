//--------------------------------------------------------------------------------------
// SimpleWinHttp_Desktop.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleWinHttp_Desktop.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample();
extern std::unique_ptr<Sample> g_sample;

using namespace DirectX;
using namespace ATG;

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
    const wchar_t *c_websocketAddress   = L"https://echo.websocket.org";
    const wchar_t *c_httpWebAddress     = L"https://www.msn.com";
    const wchar_t *c_xblWebAddress      = L"https://profile.xboxlive.com/users/me/profile/settings?settings=GameDisplayName";
    const wchar_t *c_gameServiceAddress = L"https://" MY_HOST "/api/getclaims";

    // Other endpoints that you can call from this client side sample
    // see the documentaiton for the Game Service Sample for more details
    /*
    L"https://" MY_HOST "/api/getclaims"
    L"https://" MY_HOST "/api/b2bfriends"
    L"https://" MY_HOST "/api/collections/query"
    L"https://" MY_HOST "/api/collections/query?ids=" TEST_CONSUMABLE_STOREID ":0010"  //  Note the SKU ID for the test product in the sample's sandbox is 0010, but other products may be different                                                                                   //  likely be 0001 or another value.  Check the results in collections to find it.
    L"https://" MY_HOST "/api/collections/consume?id=" TEST_CONSUMABLE_STOREID "&quantity=1"
    L"https://" MY_HOST "/api/collections/RetryPendingConsumes"
    L"https://" MY_HOST "/api/getGDPRList";
    */

    // UI element ID's from SampleUI.csv
    const int c_sampleUIPanel  = 2000;
    const int c_connectBtn     = 2101;
    const int c_sendMessageBtn = 2102;
    const int c_makeRequestBtn = 2103;
    const int c_xblRequestBtn  = 2104;
    const int c_gameServiceBtn = 2105;
}

Sample::Sample() noexcept(false) :
    m_lastSocketStatus(ATG::WebSocketStatus::Uninitialized)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SimpleWinHttp_Desktop");

    ATG::UIConfig uiconfig;
    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
    m_console = std::make_unique<DX::TextConsoleImage>();
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
    // NOTE: When running the app from the Start Menu (required for
    //	Store API's to work) the Current Working Directory will be
    //	returned as C:\Windows\system32 unless you overwrite it.
    //	The sample relies on the font and image files in the .exe's
    //	directory and so we do the following to set the working
    //	directory to what we want.
    char dir[_MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, dir, _MAX_PATH) > 0)
    {
        std::string exe = dir;
        exe = exe.substr(0, exe.find_last_of("\\"));
        std::ignore = SetCurrentDirectoryA(exe.c_str());
    }

    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();

    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets");

    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
    });

    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {

        }
    });

    // Before we can make an Xbox Live call we need to ensure that the Game OS has intialized the network stack
    // For sample purposes we block user interaction with the sample.  A game should wait for the network to be
    // initialized before the main menu appears.  For samples, we will wait at the end of initialization.
    while (!m_liveResources->IsNetworkAvailable())
    {
        SwitchToThread();
    }

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();

    SetupUI();

    XGameRuntimeInitialize();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Update the UX state
    if (m_webSocket && m_webSocket->GetConnectionStatus() != m_lastSocketStatus)
    {
        switch (m_webSocket->GetConnectionStatus())
        {
        case ATG::WebSocketStatus::Uninitialized:
            SendMessageToScreen("WebSocket is closed.");
            break;

        case ATG::WebSocketStatus::Connected:
        {
            SendMessageToScreen("WebSocket is connected.");

            auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_connectBtn);
            button->SetText(L"Close WebSocket");

            button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_sendMessageBtn);
            button->SetEnabled(true);
        }
        break;

        case ATG::WebSocketStatus::Error:
            SendMessageToScreen("WebSocket encountered an error.");
            break;

        case ATG::WebSocketStatus::Closed:
            break;
        }

        m_lastSocketStatus = m_webSocket->GetConnectionStatus();
    }
    else if (m_webSocket == nullptr)
    {
        auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_connectBtn);
        button->SetText(L"Connect WebSocket");

        button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_sendMessageBtn);
        button->SetEnabled(false);

        m_lastSocketStatus = ATG::WebSocketStatus::Uninitialized;
    }

    //  Gamepad control input
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

    //  Keyboard control input
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

    m_ui->Update(elapsedTime, m_mouse->Get(), m_keyboard->Get());

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    PIXEndEvent();
}
#pragma endregion

#pragma region UI Methods
void Sample::SetupUI()
{
    using namespace ATG;

    // Show our only UI panel
    m_ui->FindPanel<ATG::Overlay>(c_sampleUIPanel)->Show();

    // Handle the Connect/Close button press
    m_ui->FindControl<Button>(c_sampleUIPanel, c_connectBtn)->SetCallback([this](IPanel*, IControl *)
        {
            if (!m_webSocket ||
                m_webSocket->GetConnectionStatus() == ATG::WebSocketStatus::Uninitialized ||
                m_webSocket->GetConnectionStatus() == ATG::WebSocketStatus::Closed)
            {
                m_console->Format(L"Connecting to %s...\n", c_websocketAddress);

                auto async = new XAsyncBlock{};

                async->context = this;
                async->callback = [](XAsyncBlock* async)
                {
                    WebSocket* webSocket = nullptr;
                    auto pThis = reinterpret_cast<Sample*>(async->context);

                    auto hr = WinHttpManager::OpenWebSocketAsyncResult(
                        async,
                        &webSocket
                        );

                    if (SUCCEEDED(hr))
                    {
                        // Tell the sample the socket is ready
                        pThis->WebSocketConnected(std::shared_ptr<WebSocket>(webSocket));
                    }
                    else
                    {
                        pThis->SendMessageToScreen("Failed to create WebSocket connection.");
                    }

                    delete async;
                };

                HRESULT hr = WinHttpManager::OpenWebSocketAsync(
                    async,
                    nullptr,
                    c_websocketAddress,
                    nullptr,
                    0
                    );

                if (FAILED(hr))
                {
                    delete async;
                }
            }
            else if (m_webSocket->GetConnectionStatus() == ATG::WebSocketStatus::Connected)
            {
                m_console->WriteLine(L"Closing WebSocket...");

                // Perform the synchronus websocket shutdown on a worker thread
                auto async = new XAsyncBlock{};

                async->context = this;
                async->callback = [](XAsyncBlock* async)
                {
                    auto pThis = reinterpret_cast<Sample*>(async->context);

                    pThis->WebSocketConnected(nullptr);
                    pThis->SendMessageToScreen("WebSocket closed.");

                    delete async;
                };

                HRESULT hr = WinHttpManager::CloseWebSocketAsync(
                    async,
                    m_webSocket.get()
                    );

                if (FAILED(hr))
                {
                    delete async;
                }
            }
        });

    // Handle the Send Message button press
    m_ui->FindControl<Button>(c_sampleUIPanel, c_sendMessageBtn)->SetCallback([this](IPanel*, IControl*)
    {
        auto async = new XAsyncBlock{};

        async->context = this;
        async->callback = [](XAsyncBlock* async)
        {
            uint32_t size = 0;
            uint32_t used = 0;
            std::unique_ptr<char[]> buffer;

            // Get the size of the data buffer
            HRESULT hr = XGameUiShowTextEntryResultSize(
                async,              // XAsyncBlock
                &size               // OUT size of buffer
                );

            if (SUCCEEDED(hr))
            {
                buffer = std::make_unique<char[]>(size);

                // Get the data
                hr = XGameUiShowTextEntryResult(
                    async,          // XAsyncBlock
                    size,           // Size of buffer
                    buffer.get(),   // Buffer
                    &used           // OUT amount of buffer filled
                    );
            }

            if (SUCCEEDED(hr))
            {
                auto pThis = reinterpret_cast<Sample*>(async->context);

                // Send the text across the web socket
                pThis->SendMessageToWebSocket(
                    buffer.get(),   // Data buffer
                    size            // Buffer size
                    );
            }

            delete async;
        };

        // Invoke the on-screen keyboard
        HRESULT hr = XGameUiShowTextEntryAsync(
            async,
            "Test Data!",
            "WebSocket Sample",
            "Enter a message to send across the WebSocket.",
            XGameUiTextEntryInputScope::Alphanumeric,
            255
            );

        if (FAILED(hr))
        {
            delete async;
            SendMessageToScreen("Unable to show virtual keyboard");
        }
    });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_makeRequestBtn)->SetCallback([this](IPanel*, IControl*)
    {
        auto async = new XAsyncBlock{};

        async->context = this;
        async->callback = [](XAsyncBlock* async)
        {
            WinHttpRequest* request = nullptr;

            auto hr = WinHttpManager::MakeHttpRequestAsyncResult(
                async,
                &request
                );

            if (SUCCEEDED(hr))
            {
                g_sample->SendMessageToScreen("Response status code: " + std::to_string(request->GetStatusCode()));
                g_sample->SendMessageToScreen("Response body size: " + std::to_string(request->GetBodyLength()));
                g_sample->SendMessageToScreen("Request completed.");

                delete request;
            }
            else
            {
                g_sample->SendMessageToScreen("Failed to make http request: " + std::to_string(hr));
            }

            delete async;
        };

        m_console->Format(L"Connecting to %s...\n", c_httpWebAddress);

        HRESULT hr = WinHttpManager::MakeHttpRequestAsync(
            async,
            nullptr,
            L"GET",
            c_httpWebAddress,
            nullptr,
            0,
            nullptr,
            0
            );

        if (FAILED(hr))
        {
            delete async;
        }
    });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_xblRequestBtn)->SetCallback([this](IPanel*, IControl*)
    {
        if (m_liveResources->IsUserSignedIn() == false)
        {
            m_liveResources->SignInWithUI();
        }
        else
        {
            auto async = new XAsyncBlock{};

            async->context = this;
            async->callback = [](XAsyncBlock* async)
            {
                WinHttpRequest* request = nullptr;

                auto hr = WinHttpManager::MakeHttpRequestAsyncResult(
                    async,
                    &request
                    );

                if (SUCCEEDED(hr))
                {
                    auto rawBody = request->GetBody();
                    auto bodyLen = request->GetBodyLength();
                    std::string body(rawBody, rawBody + bodyLen);

                    g_sample->SendMessageToScreen("Response status code: " + std::to_string(request->GetStatusCode()));
                    g_sample->SendMessageToScreen("Response body size: " + std::to_string(request->GetBodyLength()));
                    g_sample->SendMessageToScreen(body);
                    g_sample->SendMessageToScreen("Request completed.");

                    delete request;
                }
                else
                {
                    g_sample->SendMessageToScreen("Failed to make http request: " + std::to_string(hr));
                }

                delete async;
            };

            m_console->Format(L"Connecting to %s...\n", c_xblWebAddress);

            WINHTTP_EXTENDED_HEADER winhttpHeader[2];

            winhttpHeader[0].pwszName = L"X-XBL-Contract-Version";
            winhttpHeader[0].pwszValue = L"2";
            winhttpHeader[1].pwszName = L"Content-Type";
            winhttpHeader[1].pwszValue = L"application/json";

            HRESULT hr = WinHttpManager::MakeHttpRequestAsync(
                async,
                m_liveResources->GetUser(),
                L"GET",
                c_xblWebAddress,
                winhttpHeader,
                ARRAYSIZE(winhttpHeader),
                nullptr,
                0
                );

            if (FAILED(hr))
            {
                delete async;
            }
        }
    });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_gameServiceBtn)->SetCallback([this](IPanel*, IControl*)
    {
        if (m_liveResources->IsUserSignedIn() == false)
        {
            m_liveResources->SignInWithUI();
        }
        else
        {
            auto async = new XAsyncBlock{};

            async->context = this;
            async->callback = [](XAsyncBlock* async)
            {
                WinHttpRequest* request = nullptr;

                auto hr = WinHttpManager::MakeHttpRequestAsyncResult(
                    async,
                    &request
                    );

                if (SUCCEEDED(hr))
                {
                    auto rawBody = request->GetBody();
                    auto bodyLen = request->GetBodyLength();
                    std::string body(rawBody, rawBody + bodyLen);

                    g_sample->SendMessageToScreen("Response status code: " + std::to_string(request->GetStatusCode()));
                    g_sample->SendMessageToScreen("Response body size: " + std::to_string(request->GetBodyLength()));
                    g_sample->SendMessageToScreen(body);
                    g_sample->SendMessageToScreen("Request completed.");

                    delete request;
                }
                else
                {
                    g_sample->SendMessageToScreen("Failed to make http request: " + std::to_string(hr));
                }

                delete async;
            };

            m_console->Format(L"Connecting to %s...\n", c_gameServiceAddress);

            WINHTTP_EXTENDED_HEADER winhttpHeader[2];

            winhttpHeader[0].pwszName = L"X-XBL-Contract-Version";
            winhttpHeader[0].pwszValue = L"2";
            winhttpHeader[1].pwszName = L"Content-Type";
            winhttpHeader[1].pwszValue = L"application/json";

            HRESULT hr = WinHttpManager::MakeHttpRequestAsync(
                async,
                m_liveResources->GetUser(),
                L"GET",
                c_gameServiceAddress,
                winhttpHeader,
                ARRAYSIZE(winhttpHeader),
                nullptr,
                0
                );

            if (FAILED(hr))
            {
                delete async;
            }
        }
    });
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
    m_console->Render(commandList);
    m_ui->Render(commandList);

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
    if (m_webSocket)
    {
        SendMessageToScreen("WebSocket is closing from suspend.");
        m_webSocket = nullptr;
    }
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
    m_ui->Reset();

    WinHttpManager::Reset();
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_console->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        L"courier_16.spritefont",
        L"ATGSampleBackground.DDS",
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
        );

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);
    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();

    m_ui->SetWindow(m_deviceResources->GetOutputSize());
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());

    static const RECT screenDisplay = { 500, 100, 1250, 650 };

    m_console->SetWindow(screenDisplay, false);
    m_console->SetViewport(viewport);

    m_console->WriteLine(L"SimpleWinHttp Sample ready");
}

void Sample::OnDeviceLost()
{
    m_ui->ReleaseDevice();
    m_console->ReleaseDevice();
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma region WebSocket Code
void Sample::SendMessageToWebSocket(const char* buffer, uint32_t length)
{
    std::string log(buffer, buffer + length);
    SendMessageToScreen("Sending: " + log);

    auto async = new XAsyncBlock{};

    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        DWORD bytesWritten = 0;

        HRESULT hr = WinHttpManager::WriteToWebSocketAsyncResult(
            async,
            &bytesWritten
            );

        if (SUCCEEDED(hr))
        {
            g_sample->SendMessageToScreen("Sent " + std::to_string(bytesWritten) + " bytes to web socket.");
        }
        else
        {
            g_sample->SendMessageToScreen("Failed to get write status.");
        }

        // Kick off the async read for the response
        g_sample->GetMessageFromWebSocket();

        delete async;
    };

    auto hr = WinHttpManager::WriteToWebSocketAsync(
        async,
        m_webSocket.get(),
        reinterpret_cast<const uint8_t*>(buffer),
        length
        );

    if (FAILED(hr))
    {
        delete async;
        SendMessageToScreen("Failed to write to web socet!");
    }
}

void Sample::GetMessageFromWebSocket()
{
    auto async = new XAsyncBlock{};

    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        DWORD bytesAvailable = 0;

        HRESULT hr = WinHttpManager::ReadFromWebSocketAsyncResultSize(
            async,
            &bytesAvailable
            );

        if (SUCCEEDED(hr))
        {
            g_sample->SendMessageToScreen(std::to_string(bytesAvailable) + " bytes available to read.");

            std::vector<uint8_t> message;

            message.clear();
            message.resize(bytesAvailable);

            hr = WinHttpManager::ReadFromWebSocketAsyncResult(
                async,
                message.data(),
                bytesAvailable
                );

            if (SUCCEEDED(hr))
            {
                // Our byte array data is a null-terminated UTF-8 string
                std::string response = reinterpret_cast<char*>(message.data());
                g_sample->SendMessageToScreen("Received: " + response);
            }
            else
            {
                g_sample->SendMessageToScreen("Failed to get read result.");
            }
        }
        else
        {
            g_sample->SendMessageToScreen("Failed to get read result size.");
        }

        delete async;
    };

    auto hr = WinHttpManager::ReadFromWebSocketAsync(
        async,
        m_webSocket.get()
        );

    if (FAILED(hr))
    {
        delete async;
        SendMessageToScreen("Failed to read from web socet!");
    }
}

void Sample::WebSocketConnected(std::shared_ptr<ATG::WebSocket> socket)
{
    m_webSocket = socket;
}

void Sample::SendMessageToScreen(std::string message)
{
    if (m_console)
    {
        m_console->WriteLine(DX::Utf8ToWide(message).c_str());
    }

    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
}
#pragma endregion
