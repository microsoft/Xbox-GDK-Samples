//--------------------------------------------------------------------------------------
// CustomHttp.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CustomHttp.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "SampleWinHttpProxy.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    const char* c_requestUrl = "https://www.example.com";
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    CleanupCurl();

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
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

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_httpRequestButton = m_uiManager.FindTypedById<UIButton>(ID("HttpRequestButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_httpRequestButton->SetEnabled(false);

    m_httpRequestButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnHttpRequestButtonPressed();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });

    Log("Waiting for network initialization...");
    WaitForNetworkInitialization();
    InitializeCurl();
}

void Sample::WaitForNetworkInitialization()
{
    XNetworkingConnectivityHint hint{};
    while (SUCCEEDED(XNetworkingGetConnectivityHint(&hint)) &&
           hint.networkInitialized == false)
    {
        SwitchToThread();
    }
    Log("Network initialized.");
}

void Sample::InitializeCurl()
{
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK)
    {
        Log("curl_global_init failed: %s", curl_easy_strerror(res));
        return;
    }

    m_curlMulti = curl_multi_init();
    if (!m_curlMulti)
    {
        curl_global_cleanup();
        Log("curl_multi_init failed.");
        return;
    }

    m_curlInitialized = true;
    Log("libcurl initialized.");

    ResolveAndCacheProxy();
    LogDebugCertificates();

    m_httpRequestButton->SetEnabled(true);
    m_uiManager.SetFocus(m_httpRequestButton);
}

void Sample::CleanupCurl()
{
    if (!m_curlInitialized)
        return;

    // Remove and cleanup every in-flight request before destroying the multi handle
    for (CURL* easy : m_activeRequests)
    {
        curl_multi_remove_handle(m_curlMulti, easy);

        // Free the per-request response buffer stored in CURLOPT_PRIVATE
        char* privatePtr = nullptr;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &privatePtr);
        delete reinterpret_cast<std::string*>(privatePtr);

        curl_easy_cleanup(easy);
    }
    m_activeRequests.clear();

    curl_multi_cleanup(m_curlMulti);
    m_curlMulti = nullptr;

    curl_global_cleanup();
    m_curlInitialized = false;
    Log("libcurl cleaned up.");
}

// Forward declaration for WinHttpCreateProxyResolver -- see SampleWinHttpProxy.h

void Sample::ResolveAndCacheProxy()
{
    // Return early - proxy is cached after first successful resolution
    if (!m_proxyAddress.empty())
        return;

    HINTERNET session = WinHttpOpen(
        L"CustomHttp/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC);
    if (!session)
    {
        Log("WinHttpOpen failed: %lu", GetLastError());
        return;
    }

    // Context passed to the async callback
    struct ProxyContext
    {
        HANDLE event;
        DWORD  error;
    };

    ProxyContext ctx{ CreateEventW(nullptr, TRUE, FALSE, nullptr), ERROR_SUCCESS };
    if (!ctx.event)
    {
        Log("CreateEventW failed: %lu", GetLastError());
        WinHttpCloseHandle(session);
        return;
    }

    // Status callback ? signals the event when proxy resolution completes or errors
    WinHttpSetStatusCallback(session,
        [](HINTERNET, DWORD_PTR context, DWORD status, LPVOID info, DWORD)
        {
            auto* c = reinterpret_cast<ProxyContext*>(context);
            if (status == WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE)
            {
                c->error = ERROR_SUCCESS;
                SetEvent(c->event);
            }
            else if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
            {
                c->error = reinterpret_cast<WINHTTP_ASYNC_RESULT*>(info)->dwError;
                SetEvent(c->event);
            }
        },
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS,
        0);

    HINTERNET resolver = nullptr;
    DWORD result = WinHttpCreateProxyResolver(session, &resolver);
    if (result != ERROR_SUCCESS)
    {
        Log("WinHttpCreateProxyResolver failed: %lu", result);
        CloseHandle(ctx.event);
        WinHttpCloseHandle(session);
        return;
    }

    DWORD getResult = WinHttpGetProxySettingsEx(
        resolver,
        WinHttpProxySettingsTypeXBox,
        nullptr,
        reinterpret_cast<DWORD_PTR>(&ctx));

    if (getResult == ERROR_IO_PENDING)
    {
        // Async path ? wait for the callback to fire (5 second timeout)
        WaitForSingleObject(ctx.event, 5000);
        getResult = ctx.error;
    }

    if (getResult != ERROR_SUCCESS)
    {
        Log("WinHttpGetProxySettingsEx failed: %lu", getResult);
        CloseHandle(ctx.event);
        WinHttpCloseHandle(resolver);
        WinHttpCloseHandle(session);
        return;
    }

    WINHTTP_PROXY_SETTINGS_EX proxySettings{};
    DWORD resultResult = WinHttpGetProxySettingsResultEx(resolver, &proxySettings);
    if (resultResult != ERROR_SUCCESS)
    {
        Log("WinHttpGetProxySettingsResultEx failed: %lu", resultResult);
        CloseHandle(ctx.event);
        WinHttpCloseHandle(resolver);
        WinHttpCloseHandle(session);
        return;
    }

    PCWSTR proxy =
        proxySettings.pcwszSecureProxy != nullptr
            ? proxySettings.pcwszSecureProxy
            : proxySettings.pcwszProxy;

    if (proxy != nullptr)
    {
        m_proxyAddress = DX::WideToUtf8(std::wstring(proxy));
        Log("Proxy address: %s", m_proxyAddress.c_str());
    }
    else
    {
        Log("No proxy configured.");
    }

    WinHttpFreeProxySettingsEx(WinHttpProxySettingsTypeXBox, &proxySettings);
    CloseHandle(ctx.event);
    WinHttpCloseHandle(resolver);
    WinHttpCloseHandle(session);
}

void Sample::LogDebugCertificates()
{
    HCERTSTORE store = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,
        0,
        CERT_SYSTEM_STORE_CURRENT_USER |
            CERT_STORE_OPEN_EXISTING_FLAG |
            CERT_STORE_READONLY_FLAG,
        L"Root");

    if (!store)
    {
        Log("CertOpenStore failed: 0x%08x", HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    PCCERT_CONTEXT cert = nullptr;
    while ((cert = CertEnumCertificatesInStore(store, cert)) != nullptr)
    {
        wchar_t subject[512] = {};
        CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, subject, 512);

        if (wcsstr(subject, L"Fiddler") || wcsstr(subject, L"Xbox Multiplayer"))
        {
            Log("Found debug certificate: %s", DX::WideToUtf8(std::wstring(subject)).c_str());
        }
    }

    CertCloseStore(store, 0);
}

void Sample::OnHttpRequestButtonPressed()
{
    if (!m_curlInitialized)
    {
        Log("curl not initialized");
        return;
    }

    CURL* easy = curl_easy_init();
    if (!easy)
    {
        Log("curl_easy_init failed");
        return;
    }

    auto* responseBody = new std::string();

    auto writeCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t
    {
        static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
        return size * nmemb;
    };

    curl_easy_setopt(easy, CURLOPT_URL, c_requestUrl);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, +writeCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, responseBody);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, responseBody);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);

    if (!m_proxyAddress.empty())
        curl_easy_setopt(easy, CURLOPT_PROXY, m_proxyAddress.c_str());

    m_activeRequests.push_back(easy);
    curl_multi_add_handle(m_curlMulti, easy);
    Log("HTTP request queued: %s", c_requestUrl);
}

void Sample::PumpCurlMulti()
{
    if (!m_curlMulti)
        return;

    int runningHandles = 0;
    curl_multi_perform(m_curlMulti, &runningHandles);

    int msgsLeft = 0;
    CURLMsg* msg;
    while ((msg = curl_multi_info_read(m_curlMulti, &msgsLeft)) != nullptr)
    {
        if (msg->msg == CURLMSG_DONE)
        {
            CURL* easy = msg->easy_handle;
            CURLcode result = msg->data.result;

            long httpCode = 0;
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpCode);

            char* privatePtr = nullptr;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &privatePtr);
            auto* body = reinterpret_cast<std::string*>(privatePtr);

            if (result == CURLE_OK && body != nullptr)
            {
                Log("Response code: %ld", httpCode);
                Log("Response body: %zu bytes", body->size());
            }
            else if (result == CURLE_OK)
            {
                Log("Response code: %ld", httpCode);
            }
            else
            {
                Log("Request failed: %s", curl_easy_strerror(result));
            }

            delete body;
            curl_multi_remove_handle(m_curlMulti, easy);
            m_activeRequests.erase(
                std::remove(m_activeRequests.begin(), m_activeRequests.end(), easy),
                m_activeRequests.end());
            curl_easy_cleanup(easy);
        }
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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
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

    auto mouse = m_mouse->GetState();
    mouse;

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

    PumpCurlMulti();

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
    const auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
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
    CleanupCurl();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_inputState.Reset();

    Log("Resumed ? waiting for network...");
    WaitForNetworkInitialization();
    InitializeCurl();
}

void Sample::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    const auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
