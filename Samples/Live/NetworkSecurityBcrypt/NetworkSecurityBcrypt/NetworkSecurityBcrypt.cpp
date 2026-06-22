//--------------------------------------------------------------------------------------
// NetworkSecurityBcrypt.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NetworkSecurityBcrypt.h"
#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace ATG
{
    bool GetLocalNetworkAddress(sockaddr_in& outAddr);
}

Sample::Sample() noexcept(false) :
    m_taskQueueRegToken{},
    m_connectivityHint{},
    m_frame(0),
    m_port{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("NetworkSecurityBcrypt");
}

Sample::~Sample()
{
    CleanupTaskQueue();

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

    m_liveInfoHUD->Initialize();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_openConnectionButton = m_uiManager.FindTypedById<UIButton>(ID("OpenConnectionButton"));
    m_sendMessageButton = m_uiManager.FindTypedById<UIButton>(ID("SendMessageButton"));
    m_closeConnectionButton = m_uiManager.FindTypedById<UIButton>(ID("CloseConnectionButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_openConnectionButton->SetEnabled(false);
    m_sendMessageButton->SetEnabled(false);
    m_closeConnectionButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeTaskQueue();
    InitializeLiveResources();
    CheckForNetworkInitialization();
}

void Sample::InitializeTaskQueue()
{
    assert(nullptr == m_taskQueue);

    HRESULT hr = XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue);
    if (FAILED(hr))
    {
        Log("Sample::InitializeTaskQueue: Task queue creation failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
    }
    else
    {
        assert(nullptr != m_taskQueue);
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

void Sample::InitializeLiveResources()
{
    m_liveResources = std::make_shared<ATG::LiveResources>(m_taskQueue);
    m_liveResources->SetErrorHandler([this](HRESULT hr)
    {
        if (hr == E_GAMEUSER_NO_DEFAULT_USER || hr == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {
            Log("LiveResources: failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
    });

    m_liveResources->Initialize();
}

void Sample::CheckForNetworkInitialization()
{
    Log("CheckForNetworkingInitialization() started.");
    m_asyncOpWidget->Show(u8"Checking for network ready");

    m_connectivityHint = {};
    HRESULT hr = XNetworkingGetConnectivityHint(&m_connectivityHint);
    if (FAILED(hr))
    {
        Log("CheckForNetworkingInitialization() failed.");
        Log("XNetworkingGetConnectivityHint() failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
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
            if (Sample* sample = static_cast<Sample*>(context))
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

HRESULT Sample::CreateLocalSocket()
{
    ATG::DtlsSocket* socket{};
    auto hr = ATG::DtlsSocket::Create(m_port, m_taskQueue, &socket);

    if (SUCCEEDED(hr))
    {
        m_socket.reset(socket);

        SOCKADDR localAddress{};

        SOCKADDR_IN* addrIn = reinterpret_cast<SOCKADDR_IN*>(&localAddress);

        ATG::GetLocalNetworkAddress(*addrIn);
        addrIn->sin_port = m_port;

        Log("Local Address: %s", ATG::AddressToString(localAddress).c_str());

        m_openConnectionButton->SetEnabled(true);
        m_sendMessageButton->SetEnabled(false);
        m_closeConnectionButton->SetEnabled(false);

        m_uiManager.SetFocus(m_openConnectionButton);

        PrintLocalIdentityString();
    }

    return hr;
}

void Sample::HandleNetworkInitializationComplete()
{
    LoginToXboxLive();
    InitializeUIEventHandlers();

    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_taskQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        std::unique_ptr<XAsyncBlock> asyncPtr{ async };

        auto sample = reinterpret_cast<Sample*>(async->context);

        sample->m_port = 4444;
        HRESULT hr = XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult(async, &sample->m_port);
        if (SUCCEEDED(hr))
        {
            hr = sample->CreateLocalSocket();
            if (FAILED(hr))
            {
                sample->Log("Failed to create socket with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
            }
        }
        else
        {
            sample->Log("XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
        }

        sample->m_asyncOpWidget->Hide();
    };

    HRESULT hr = XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync(async.get());
    if (SUCCEEDED(hr))
    {
        async.release();
    }
    else
    {
        Log("XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync failed with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
    }
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
    m_liveInfoHUD->SetUser(m_liveResources->GetUser(), m_taskQueue);
}

void Sample::PumpTaskQueue()
{
    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0)) {}
}

void Sample::InitializeUIEventHandlers()
{
    m_openConnectionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnOpenConnectionButtonPressed();
    });

    m_sendMessageButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnSendMessageButtonPressed();
    });

    m_closeConnectionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnCloseConnectionButtonPressed();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });
}

void Sample::PrintLocalIdentityString()
{
    uint32_t fingerprintSize = m_socket->GetFingerprintSize();
    std::vector<uint8_t> fingerprint(fingerprintSize);
    m_socket->GetFingerprint(fingerprint.data(), fingerprintSize, &fingerprintSize);
    
    uint32_t subjectNameSize = m_socket->GetSubjectNameSize();
    std::vector<uint8_t> subjectName(subjectNameSize);
    m_socket->GetSubjectName(subjectName.data(), subjectNameSize, &subjectNameSize);
    
    auto identityString = ATG::BytesToHexString(fingerprint.data(), fingerprintSize);
    identityString += ":";
    identityString += ATG::BytesToHexString(subjectName.data(), subjectNameSize);
    
    Log("Local Identity: %s", identityString.c_str());
}

#pragma region Open Connection
void Sample::OnOpenConnectionButtonPressed()
{
    Log("Opening Connection");

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
            sample->m_ipAddr = buffer.get();

            sample->GetExpectedIdentityString_OpenConnection();
        }
    };

    // Invoke the on-screen keyboard
    HRESULT hr = XGameUiShowTextEntryAsync(
        asyncBlock.get(),
        "Network Security Sample",
        "Enter an IP address to connect to.",
        "",
        XGameUiTextEntryInputScope::Alphanumeric,
        1024
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

void Sample::GetExpectedIdentityString_OpenConnection()
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
            sample->m_expectedIdentityString = buffer.get();

            sample->OpenConnection();
        }
    };

    // Invoke the on-screen keyboard
    HRESULT hr = XGameUiShowTextEntryAsync(
        asyncBlock.get(),
        "Network Security Sample",
        "Enter the identity string to expect for the connection",
        "",
        XGameUiTextEntryInputScope::Alphanumeric,
        1024
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

void Sample::OpenConnection()
{
    if (m_ipAddr.empty())
    {
        Log("The IP address entered was invalid");
        return;
    }

    if (m_expectedIdentityString.empty())
    {
        Log("The identity entered was invalid");
        return;
    }

    auto destination = ATG::AddressFromString(m_ipAddr);

    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_taskQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        std::unique_ptr<XAsyncBlock> asyncBlock{ async };

        ATG::DtlsConnectionHandle connection{};
        HRESULT hr = ATG::DtlsSocket::CreateConnectionAsyncResult(async, &connection);

        auto sample = reinterpret_cast<Sample*>(async->context);

        if (SUCCEEDED(hr))
        {
            sample->m_connection = connection;

            sample->Log("Connection Created");

            sample->m_openConnectionButton->SetEnabled(false);
            sample->m_sendMessageButton->SetEnabled(true);
            sample->m_closeConnectionButton->SetEnabled(true);
        }
        else
        {
            sample->Log("Failed to create connection with HRESULT = (0x%08x) %s", hr, GetErrorMessage(hr).c_str());

            sample->m_openConnectionButton->SetEnabled(true);
            sample->m_sendMessageButton->SetEnabled(false);
            sample->m_closeConnectionButton->SetEnabled(false);
        }
    };

    HRESULT hr = m_socket->CreateConnectionAsync(&destination, m_expectedIdentityString, async.get());

    if (SUCCEEDED(hr))
    {
        async.release();
    }
}
#pragma endregion

void Sample::OnSendMessageButtonPressed()
{
    Log("Sending Message");

    if (!m_connection)
    {
        Log("Cannot send without an established connection");
        return;
    }

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
            assert(size <= ATG::c_MaxPayloadSize);

            auto async = std::make_unique<XAsyncBlock>();
            async->queue = sample->m_taskQueue;
            async->context = sample;
            async->callback = [](XAsyncBlock* asyncBlock)
            {
                std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*
                auto sample = static_cast<Sample*>(asyncBlock->context);
                assert(sample);

                HRESULT hr = XAsyncGetStatus(asyncBlock, false);

                sample->Log("Result (0x%08x) %s", hr, GetErrorMessage(hr).c_str());
            };

            ATG::SocketPayload data{};

            data.Fill(buffer.get(), used);

            hr = sample->m_socket->SendToAsync(sample->m_connection, &data, async.get());
            if (SUCCEEDED(hr))
            {
                async.release();
            }
        }
    };

    // Invoke the on-screen keyboard
    HRESULT hr = XGameUiShowTextEntryAsync(
        asyncBlock.get(),
        "Network Security Sample",
        "Enter a message to send across the connection.",
        "",
        XGameUiTextEntryInputScope::Alphanumeric,
        1024
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

void Sample::OnCloseConnectionButtonPressed()
{
    Log("Closing Connection");

    if (m_connection)
    {
        m_socket->CloseConnection(m_connection);
        m_connection = nullptr;
    }

    CreateLocalSocket();

    m_openConnectionButton->SetEnabled(true);
    m_sendMessageButton->SetEnabled(false);
    m_closeConnectionButton->SetEnabled(false);
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

    if (m_socket)
    {
        ATG::DtlsConnectionHandle source{};
        ATG::SocketPayload data;

        HRESULT hr = S_OK;

        do
        {
            hr = m_socket->RecvFrom(&source, &data);

            if (SUCCEEDED(hr))
            {
                Log("Message from connection %p: %s", source, data.ToString().c_str());
            }
            else if (hr == E_ABORT)
            {
                Log("Connection %p closed", source);
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))
            {
                Log("Invalid data received on connection %p", source);
            }
            else
            {

            }
        } while (hr != HRESULT_FROM_WIN32(ERROR_EMPTY));
    }

    (void)elapsedTime;

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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

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

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
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
