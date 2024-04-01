//--------------------------------------------------------------------------------------
// mDNS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "mDNS.h"

#include "ATGColors.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const int s_button_RegisterLocalDevice = 2001;
    const int s_button_DeRegisterLocalDevice = 2002;
    const int s_button_StartBrowse = 2003;
    const int s_button_StopBrowse = 2004;

    // NOTE: Service name must conform to the following format: <ServiceName>._<ServiceType>._<TransportProtocol>.local
    //                                                          e.g. StrategyGame._game._udp.local  
    PCWSTR c_ServiceName = L"mdnssample._game._udp.local";

    // NOTE: Service type must conform to the following format: _<ServiceType>._<TransportProtocol>.local
    //                                                          _game._udp.local
    PCWSTR c_ServiceType = L"_game._udp.local";
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_waitEvent(nullptr),
    m_dnsServiceInstance(nullptr),
    m_browseCancel{}
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    ATG::UIConfig uiconfig;
    uiconfig.colorBackground = DirectX::XMFLOAT4(0, 0, 0, 1);
    uiconfig.colorFocus = DirectX::XMFLOAT4(0, 153.f / 255.f, 1.f / 255.f, 1);
    m_ui = std::make_shared<ATG::UIManager>(uiconfig);

    m_console = std::make_unique<DX::TextConsoleImage>();
}

Sample::~Sample()
{
    DeRegisterDNS();
    StopBrowseDNS();
    WSACleanup();

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

    //  Custom Sample Code
    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets");
    m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();

    WSAData wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        m_console->WriteLine(L"Winsock Startup failed");
        return;
    }

    // UI Callbacks
    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_button_RegisterLocalDevice)->SetCallback([this](ATG::IPanel*, ATG::IControl* /*ctrl*/)
        {
            RegisterDNS();
        });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_button_DeRegisterLocalDevice)->SetCallback([this](ATG::IPanel*, ATG::IControl* /*ctrl*/)
        {
            DeRegisterDNS();
        });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_button_StartBrowse)->SetCallback([this](ATG::IPanel*, ATG::IControl* /*ctrl*/)
        {
            BrowseDNS();
        });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_button_StopBrowse)->SetCallback([this](ATG::IPanel*, ATG::IControl* /*ctrl*/)
        {
            StopBrowseDNS();
        });

    m_waitEvent = CreateEvent(nullptr, TRUE, FALSE, L"mdnsWait");
    m_console->WriteLine(L"mDNS Sample Started");
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

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

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    m_ui->Update(static_cast<float>(timer.GetElapsedSeconds()), pad);
    m_ui->Update(static_cast<float>(timer.GetElapsedSeconds()), *m_mouse, *m_keyboard);

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

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
    m_ui->Reset();
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
    width = 1920;
    height = 1080;
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        46,
        0
    );

    auto fonthandle = m_resourceDescriptors->Allocate();
    auto bghandle = m_resourceDescriptors->Allocate();

    m_console->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        L"Assets\\courier_16.spritefont",
        L"Assets\\ATGSampleBackground.DDS",
        m_resourceDescriptors->GetCpuHandle(fonthandle),
        m_resourceDescriptors->GetGpuHandle(fonthandle),
        m_resourceDescriptors->GetCpuHandle(bghandle),
        m_resourceDescriptors->GetGpuHandle(bghandle)
    );

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT size = m_deviceResources->GetOutputSize();

    static const RECT screenDisplay = { 640, 32, 1920 - 32, 1080 - 32 };
    m_console->SetWindow(screenDisplay, false);
    m_console->SetViewport(m_deviceResources->GetScreenViewport());

    m_ui->SetWindow(size);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_ui->ReleaseDevice();
    m_console->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

void RegisterDNS_Callback(DWORD /*Status*/, PVOID pQueryContext, PDNS_SERVICE_INSTANCE pInstance)
{
    Sample* pSample = reinterpret_cast<Sample*>(pQueryContext);

    pSample->SetDNSServiceInstance(pInstance);
    pSample->SetWaitEvent();
}

void Sample::SetDNSServiceInstance(PDNS_SERVICE_INSTANCE pInstance)
{
    m_dnsServiceInstance = pInstance;
}

void Sample::RegisterDNS()
{
    if (m_dnsServiceInstance == nullptr)
    {
        const uint16_t portNumber = 5000;
        PDNS_SERVICE_INSTANCE pInst = nullptr;

        // Determine hostname
        wchar_t hostnameW[DNS_MAX_LABEL_BUFFER_LENGTH];
        DWORD hostNameLength = ARRAYSIZE(hostnameW);
        GetComputerNameExW(ComputerNameDnsHostname, hostnameW, &hostNameLength);
        wcscat_s(hostnameW, L".local"); // .local must be appended in order for mDNS to work

        m_console->Format(L"Local Hostname is: %ls\n", hostnameW);

        pInst = DnsServiceConstructInstance(c_ServiceName,
            hostnameW,
            nullptr,
            nullptr,
            portNumber,
            0,
            0,
            0,
            nullptr,
            nullptr);

        DNS_SERVICE_REGISTER_REQUEST registerReq = {};

        registerReq.InterfaceIndex = 0;
        registerReq.pQueryContext = this;
        registerReq.pRegisterCompletionCallback = RegisterDNS_Callback;
        registerReq.Version = DNS_QUERY_REQUEST_VERSION1;
        registerReq.pServiceInstance = pInst; //From Previous Slide Create

        DWORD registerResult = DnsServiceRegister(&registerReq, nullptr);
        if (registerResult == DNS_REQUEST_PENDING)
        {
            m_console->WriteLine(L"DNS Registered");

            // Wait for the request to complete
            WaitForSingleObject(m_waitEvent, INFINITE);
        }
        else
        {
            m_console->Format(L"DNS Registration Failed %ul\n", registerResult);
        }

        if (pInst != nullptr)
        {
            DnsServiceFreeInstance(pInst);
        }
    }
    else
    {
        // NOTE: mDNS does support multiple registrations at one time. This sample does not as it is demonstrating basic LAN match scanning functionality
        m_console->WriteLine(L"Register DNS: DNS already registered, please de-register");
    }
}

void DeRegisterDNS_Callback(
    _In_ DWORD /*Status*/,
    _In_ PVOID pQueryContext,
    _In_ PDNS_SERVICE_INSTANCE /*pInstance*/
)
{
    Sample* pSample = reinterpret_cast<Sample*>(pQueryContext);
    pSample->m_console->WriteLine(L"DNS entry was de-registered");

    pSample->SetWaitEvent();
}

void Sample::DeRegisterDNS()
{
    if (m_dnsServiceInstance != nullptr)
    {
        DNS_STATUS dwStatus = ERROR_SUCCESS;
        DNS_SERVICE_REGISTER_REQUEST deRegisterReq = {};

        deRegisterReq.pQueryContext = this;
        deRegisterReq.pRegisterCompletionCallback = DeRegisterDNS_Callback;
        deRegisterReq.Version = DNS_QUERY_REQUEST_VERSION1;
        deRegisterReq.unicastEnabled = FALSE;
        deRegisterReq.pServiceInstance = m_dnsServiceInstance;
        deRegisterReq.InterfaceIndex = m_dnsServiceInstance->dwInterfaceIndex;

        // Deregister the service
        dwStatus = static_cast<DNS_STATUS>(DnsServiceDeRegister(&deRegisterReq, nullptr));

        if (dwStatus == DNS_REQUEST_PENDING)
        {
            // Wait for the request to complete
            dwStatus = static_cast<DNS_STATUS>(WaitForSingleObject(m_waitEvent, INFINITE));
        }

        if (m_dnsServiceInstance != nullptr)
        {
            DnsServiceFreeInstance(m_dnsServiceInstance);
            m_dnsServiceInstance = nullptr;
        }
    }
    else
    {
        m_console->WriteLine(L"Cannot De-Register: No DNS entry is registered");
    }
}

void BrowseDNS_Callback(DWORD /*Status*/, PVOID pQueryContext, PDNS_RECORD pDnsRecord)
{
    Sample* pSample = reinterpret_cast<Sample*>(pQueryContext);

    if (pDnsRecord)
    {
        for (auto record = pDnsRecord; record; record = record->pNext)
        {
            switch (record->wType)
            {
            case DNS_TYPE_PTR:
            {
                pSample->m_console->Format(L"Browse - Request Resolve: %ls\n", record->Data.PTR.pNameHost);
                pSample->Resolve(record->Data.PTR.pNameHost);
                break;
            }
            case DNS_TYPE_SRV:
            {
                pSample->m_console->Format(L"DNS_TYPE_SRV: %ls", record->Data.SRV.pNameTarget);
                break;
            }
            case DNS_TYPE_TEXT:
            {
                for (uint32_t i = 0; i < record->Data.TXT.dwStringCount; i++)
                {
                    pSample->m_console->Format(L"DNS_TYPE_TEXT: %ls", record->Data.TXT.pStringArray[i]);
                }
                break;
            }
            }
        }
        DnsFree(pDnsRecord, DnsFreeRecordList);
    }
}

void ResolveCallback(DWORD /*Status*/, PVOID pQueryContext, PDNS_SERVICE_INSTANCE pInstance)
{
    Sample* pSample = reinterpret_cast<Sample*>(pQueryContext);
    pSample->SetWaitEvent();

    if (pInstance)
    {
        pSample->m_console->WriteLine(L"RESOLVE:");

        pSample->m_console->Format(L"\tInstanceName: %ls\n", pInstance->pszInstanceName);

        wchar_t szIP[INET_ADDRSTRLEN] = {};
        InetNtopW(AF_INET, pInstance->ip4Address, szIP, INET_ADDRSTRLEN);
        if (wcslen(szIP) > 0)
        {
            pSample->m_console->Format(L"\tIPv4: %ls:%d\n", szIP, pInstance->wPort);
        }
        else
        {
            pSample->m_console->WriteLine(L"\tIPv4: Unavailable");
        }

        wchar_t szIPv6[INET6_ADDRSTRLEN] = {};
        InetNtopW(AF_INET6, pInstance->ip6Address, szIPv6, INET6_ADDRSTRLEN);
        if (wcslen(szIPv6) > 0)
        {
            pSample->m_console->Format(L"\tIPv6: %ls:%d\n", szIPv6, pInstance->wPort);
        }
        else
        {
            pSample->m_console->WriteLine(L"\tIPv6: Unavailable");
        }

        DnsServiceFreeInstance(pInstance);
    }
}

bool Sample::Resolve(PWSTR targetToResolve)
{
    DNS_SERVICE_RESOLVE_REQUEST resolveReq = {};
    DNS_SERVICE_CANCEL serviceCancel = {};

    resolveReq.QueryName = targetToResolve;
    resolveReq.Version = DNS_QUERY_REQUEST_VERSION1;
    resolveReq.pQueryContext = this;
    resolveReq.pResolveCompletionCallback = ResolveCallback;
    DNS_STATUS dwStatus = DnsServiceResolve(&resolveReq, &serviceCancel);

    WaitForSingleObject(m_waitEvent, INFINITE);

    // NOTE: You can use DnsServiceResolveCancel to cancel resolving.
    return (dwStatus == DNS_REQUEST_PENDING);
}

bool Sample::BrowseDNS()
{
    m_console->WriteLine(L"Begin DNS Browse");

    DNS_SERVICE_BROWSE_REQUEST browseReq = {};
    m_browseCancel = {};
    browseReq.QueryName = c_ServiceType;
    browseReq.InterfaceIndex = 0;
    browseReq.Version = DNS_QUERY_REQUEST_VERSION1;
    browseReq.pBrowseCallback = BrowseDNS_Callback;
    browseReq.pQueryContext = this;
    DNS_STATUS dwStatus = DnsServiceBrowse(&browseReq, &m_browseCancel);
    return (dwStatus == DNS_REQUEST_PENDING);
}

DNS_STATUS Sample::StopBrowseDNS()
{
    DNS_STATUS dwStatus = DnsServiceBrowseCancel(&m_browseCancel);

    if(dwStatus == ERROR_SUCCESS)
    {
        m_console->WriteLine(L"DNS Browse stopped successfully");
    }
    else if (dwStatus == DNS_REQUEST_PENDING)
    {
        m_console->WriteLine(L"DNS Browse canceled");
        return ERROR_SUCCESS;
    }
    else if (dwStatus == ERROR_CANCELLED) // no browse was in progress
    {
        m_console->WriteLine(L"No DNS Browse is in progress");
    }
    else
    {
        m_console->Format(L"DNS Browse cancel: Unhandled error code %lu", dwStatus);
    }

    return dwStatus;
}
