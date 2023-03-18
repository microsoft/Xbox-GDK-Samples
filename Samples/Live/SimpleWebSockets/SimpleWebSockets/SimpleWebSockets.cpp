//--------------------------------------------------------------------------------------
// SimpleWebSockets.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleWebSockets.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    // Public site setup for WebSocket experimentation
    const char* c_webSocketAddress = "https://ws.ifelse.io";
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SimpleWebSockets");
}

Sample::~Sample()
{
    // Websockets
    CleanUpWebSocket();

    CleanupTaskQueue();

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
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
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveInfoHUD->Initialize();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_connectWebSocketButton = m_uiManager.FindTypedById<UIButton>(ID("ConnectWebSocketButton"));
    m_closeWebSocketButton = m_uiManager.FindTypedById<UIButton>(ID("CloseWebSocketButton"));
    m_sendMessageButton = m_uiManager.FindTypedById<UIButton>(ID("SendMessageButton"));
    m_sendBinaryMessageButton = m_uiManager.FindTypedById<UIButton>(ID("SendBinaryMessageButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_connectWebSocketButton->SetEnabled(false);
    m_sendMessageButton->SetEnabled(false);
    m_sendBinaryMessageButton->SetEnabled(false);
    m_closeWebSocketButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeTaskQueue();
    InitializeLiveResources();
    CheckForNetworkInitialization();
}

void Sample::CheckForNetworkInitialization()
{
    Log("CheckForNetworkingInitialization() started.");
    m_asyncOpWidget->Show(u8"Checking for network ready");

    m_connectivityHint = {};
    auto hr = XNetworkingGetConnectivityHint(&m_connectivityHint);

    if (FAILED(hr))
    {
        Log("CheckForNetworkingInitialization() failed.");
        Log("XNetworkingGetConnectivityHint() returned 0x%08x.", hr);
        return;
    }
    if (m_connectivityHint.networkInitialized)
    {
        Log("Network is already initialized.");
        HandleNetworkInitializationComplete();
    }
    else
    {
        auto callback = [](void* context, const XNetworkingConnectivityHint* connectivityHint)
        {
            if(Sample* sample = static_cast<Sample*>(context))
            {
                sample->Log("CheckForNetworkingInitialization() callback issued...");

                if (connectivityHint->networkInitialized)
                {
                    sample->Log("Network is initialized.");
                    sample->m_connectivityHint = *connectivityHint;

                    XNetworkingUnregisterConnectivityHintChanged(sample->m_taskQueueRegToken, false);

                    sample->m_taskQueueRegToken.token = 0;
                    sample->HandleNetworkInitializationComplete();
                }
                else
                {
                    sample->Log("Network is NOT initialized.");
                }
            }
        };

        m_taskQueueRegToken.token = 0;
        hr = XNetworkingRegisterConnectivityHintChanged(m_taskQueue, this, callback, &m_taskQueueRegToken);
        if (FAILED(hr))
        {
            Log("CheckForNetworkingInitialization() failed.");
            Log("XNetworkingRegisterConnectivityHintChanged() returned 0x%08x.", hr);
        }
    }
}

void Sample::HandleNetworkInitializationComplete()
{
    m_asyncOpWidget->Hide();
    LoginToXboxLive(true);
    InitializeUIEventHandlers();
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

    m_connectWebSocketButton->SetEnabled(true);
    m_sendMessageButton->SetEnabled(false);
    m_sendBinaryMessageButton->SetEnabled(false);
    m_closeWebSocketButton->SetEnabled(false);

    m_uiManager.SetFocus(m_connectWebSocketButton);
}

void Sample::InitializeUIEventHandlers()
{
    m_connectWebSocketButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnConnectButtonPressed();
    });

    m_closeWebSocketButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnCloseButtonPressed();
    });

    m_sendMessageButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnSendMessageButtonPressed();
    });

    m_sendBinaryMessageButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnSendBinaryMessageButtonPressed();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    PumpTaskQueue();

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

    // Websockets
    CleanUpWebSocket();
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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

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
    auto const size = m_deviceResources->GetOutputSize();
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

#pragma region Sample Logic
void Sample::CleanUpWebSocket()
{
    Log("Sample::CleanUpWebSocket");

    if (m_webSocket && m_lastSocketStatus == WebSocketStatus::Connected)
    {
        Log("Closing WebSocket...");

        HCWebSocketDisconnect(m_webSocket);

        m_webSocket = nullptr;
    }
}

void Sample::OnConnectButtonPressed()
{
    if (!m_webSocket || m_lastSocketStatus == WebSocketStatus::Uninitialized || m_lastSocketStatus == WebSocketStatus::Closed)
    {
        Log("Creating WebSocket...");

        HRESULT hr = HCWebSocketCreate(&m_webSocket, message_received, binary_message_received, websocket_closed, this);
        if (FAILED(hr))
        {
            Log("Sample::OnConnectButtonPressed: HCWebSocketCreate failed with HRESULT = 0x%08x", hr);
        }

        Log("Connecting WebSocket...");

        auto asyncBlock = std::make_unique<XAsyncBlock>();
        asyncBlock->queue = m_taskQueue;
        asyncBlock->context = this;
        asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

            auto sample = static_cast<Sample*>(asyncBlock->context);
            assert(sample);

            WebSocketCompletionResult result = {};
            HRESULT hr = HCGetWebSocketConnectResult(asyncBlock, &result);
            if (FAILED(hr))
            {
                sample->Log("HCGetWebSocketConnectResult failed with HRESULT = : 0x%08x", hr);
            }
            else if (FAILED(result.errorCode))
            {
                sample->Log("HCWebSocketConnectAsync failed with HRESULT = : 0x%08x and platformErrorCode = %d", result.errorCode, result.platformErrorCode);
            }
            else
            {
                sample->OnWebSocketConnected();
            }
        };

        hr = HCWebSocketConnectAsync(c_webSocketAddress, "", m_webSocket, asyncBlock.get());
        if (SUCCEEDED(hr))
        {
            // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
            // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
            asyncBlock.release();
        }
        else
        {
            Log("Sample::OnConnectButtonPressed: HCWebSocketConnectAsync failed with HRESULT = 0x%08x", hr);
        }
    }
}

void Sample::OnWebSocketConnected()
{
    Log("WebSocket is connected.");

    m_lastSocketStatus = WebSocketStatus::Connected;

    m_connectWebSocketButton->SetEnabled(false);
    m_sendMessageButton->SetEnabled(true);
    m_sendBinaryMessageButton->SetEnabled(true);
    m_closeWebSocketButton->SetEnabled(true);
}

void Sample::OnSendMessageButtonPressed()
{
    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        auto sample = static_cast<Sample*>(asyncBlock->context);
        assert(sample);

        uint32_t size = 0;
        uint32_t used = 0;
        std::unique_ptr<char[]> buffer;

        // Get the size of the data buffer
        HRESULT hr = XGameUiShowTextEntryResultSize(
            asyncBlock,         // XAsyncBlock
            &size               // OUT size of buffer
        );

        if (SUCCEEDED(hr))
        {
            buffer = std::make_unique<char[]>(size);

            // Get the data
            hr = XGameUiShowTextEntryResult(
                asyncBlock,     // XAsyncBlock
                size,           // Size of buffer
                buffer.get(),   // Buffer
                &used           // OUT amount of buffer filled
            );
        }

        if (SUCCEEDED(hr))
        {
            // Send the text across the web socket
            sample->SendMessageToWebSocket(
                buffer.get(),   // Data buffer
                size            // Buffer size
            );
        }
    };

    // Invoke the on-screen keyboard
    HRESULT hr = XGameUiShowTextEntryAsync(
        asyncBlock.get(),
        "Test Data!",
        "WebSocket Sample",
        "Enter a message to send across the WebSocket.",
        XGameUiTextEntryInputScope::Alphanumeric,
        255
    );

    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        Log("Unable to show virtual keyboard");
    }
}

void Sample::SendMessageToWebSocket(const char* buffer, uint32_t length)
{
    std::string log(buffer, buffer + length);
    Log("Sending: " + log);

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        auto sample = static_cast<Sample*>(asyncBlock->context);
        assert(sample);

        WebSocketCompletionResult result = {};
        HRESULT hr = HCGetWebSocketSendMessageResult(asyncBlock, &result);
        if (FAILED(hr))
        {
            sample->Log("HCGetWebSocketSendMessageResult failed with HRESULT = : 0x%08x", hr);
        }
        else if (FAILED(result.errorCode))
        {
            sample->Log("HCWebSocketSendMessageAsync failed with HRESULT = : 0x%08x and platformErrorCode = %d", result.errorCode, result.platformErrorCode);
        }
    };

    HRESULT hr = HCWebSocketSendMessageAsync(m_webSocket, buffer, asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        Log("Sample::SendMessageToWebSocket: HCWebSocketSendMessageAsync failed with HRESULT = 0x%08x", hr);
    }
}

void Sample::OnSendBinaryMessageButtonPressed()
{
    std::string message = "This is a binary test message " + std::to_string(m_testNum++);

    SendBinaryMessageToWebSocket(message.c_str(), (uint32_t)message.size());
}

void Sample::SendBinaryMessageToWebSocket(const char* buffer, uint32_t length)
{
    std::string log(buffer, buffer + length);
    Log("Sending: " + log);

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        auto sample = static_cast<Sample*>(asyncBlock->context);
        assert(sample);

        WebSocketCompletionResult result = {};
        HRESULT hr = HCGetWebSocketSendMessageResult(asyncBlock, &result);
        if (FAILED(hr))
        {
            sample->Log("HCGetWebSocketSendMessageResult failed with HRESULT = : 0x%08x", hr);
        }
        else if (FAILED(result.errorCode))
        {
            sample->Log("HCWebSocketSendBinaryMessageAsync failed with HRESULT = : 0x%08x and platformErrorCode = %d", result.errorCode, result.platformErrorCode);
        }
    };

    HRESULT hr = HCWebSocketSendBinaryMessageAsync(m_webSocket, (uint8_t*)buffer, length, asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        Log("Sample::SendMessageToWebSocket: HCWebSocketSendBinaryMessageAsync failed with HRESULT = 0x%08x", hr);
    }
}

void message_received(HCWebsocketHandle /*webSocket*/, const char* incomingBodyString, void* functionContext)
{
    if (Sample* sample = static_cast<Sample*>(functionContext))
    {
        auto callback = [](void* context, bool /*cancel*/)
        {
            std::unique_ptr<MessageReceivedContext> msgContext{ static_cast<MessageReceivedContext*>(context) };
            
            msgContext->samplePtr->Log("Received message: %s", msgContext->messageBodyString.c_str());
        };

        auto msgContext = std::make_unique<MessageReceivedContext>();
        msgContext->messageBodyString = incomingBodyString;
        msgContext->samplePtr = sample;

        // Marshal this call back to the main thread
        HRESULT hr = XTaskQueueSubmitCallback(
            sample->m_taskQueue,
            XTaskQueuePort::Completion,
            msgContext.get(),
            callback);

        if (SUCCEEDED(hr))
        {
            msgContext.release();
        }
        else
        {
            // We failed to marshal the data back to the main thread so log the error message here
            DebugWrite("message_received: Error 0x%x submitting completion.", hr);
        }
    }
}

void binary_message_received(HCWebsocketHandle /*webSocket*/, const uint8_t* payloadBytes, uint32_t payloadSize, void* functionContext)
{
    if (Sample* sample = static_cast<Sample*>(functionContext))
    {
        auto callback = [](void* context, bool /*cancel*/)
        {
            std::unique_ptr<BinaryMessageReceivedContext> msgContext{ static_cast<BinaryMessageReceivedContext*>(context) };

            if (msgContext->payload.empty() == false)
            {
                // Convert the payload into a string for easy printing 
                std::string payloadStr(msgContext->payload.begin(), msgContext->payload.end());

                msgContext->samplePtr->Log("Received binary message: %s", payloadStr.c_str());
            }
            else
            {
                msgContext->samplePtr->Log("Received binary message with an empty payload");
            }
        };

        auto msgContext = std::make_unique<BinaryMessageReceivedContext>();
        msgContext->samplePtr = sample;
        msgContext->payload = std::vector<uint8_t>(payloadBytes, payloadBytes + payloadSize);

        // Marshal this call back to the main thread
        HRESULT hr = XTaskQueueSubmitCallback(
            sample->m_taskQueue,
            XTaskQueuePort::Completion,
            msgContext.get(),
            callback);

        if (SUCCEEDED(hr))
        {
            msgContext.release();
        }
        else
        {
            // We failed to marshal the data back to the main thread so log the error message here
            DebugWrite("binary_message_received: Error 0x%x submitting completion.", hr);
        }
    }
}

void Sample::OnCloseButtonPressed()
{
    if (m_webSocket && m_lastSocketStatus == WebSocketStatus::Connected)
    {
        Log("Closing WebSocket...");

        HCWebSocketDisconnect(m_webSocket);
    }
}

void websocket_closed(HCWebsocketHandle /*webSocket*/, HCWebSocketCloseStatus /*closeStatus*/, void* functionContext)
{
    auto sample = static_cast<Sample*>(functionContext);
    assert(sample);

    auto callback = [](void* context, bool /*cancel*/)
    {
        if (Sample* sample = static_cast<Sample*>(context))
        {
            sample->OnWebSocketClosed();
        }
    };

    //Marshal this call back to the main thread
    HRESULT hr = XTaskQueueSubmitCallback(
        sample->m_taskQueue,
        XTaskQueuePort::Completion,
        sample,
        callback);

    if (FAILED(hr))
    {
        DebugWrite("websocket_closed: Error 0x%x submitting completion.", hr);
    }
}

void Sample::OnWebSocketClosed()
{
    Log("WebSocket closed!");

    if (m_webSocket)
    {
        HCWebSocketCloseHandle(m_webSocket);
        m_webSocket = nullptr;
    }

    m_lastSocketStatus = WebSocketStatus::Closed;

    m_connectWebSocketButton->SetEnabled(true);
    m_sendMessageButton->SetEnabled(false);
    m_sendBinaryMessageButton->SetEnabled(false);
    m_closeWebSocketButton->SetEnabled(false);
}
#pragma endregion
