//--------------------------------------------------------------------------------------
// SystemInfo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SystemInfo.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"
#endif

#pragma warning(disable : 4061 4062)

#if defined(_GAMING_XBOX) && !defined(NTDDI_WIN10_FE)
// The xgameplatform.lib contains GetLogicalProcessorInformationEx, but the API is not in the WINAPI_FAMILY_GAMES partition with Windows 10 SDK (19041)

extern "C"
BOOL
WINAPI
GetLogicalProcessorInformationEx(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_writes_bytes_to_opt_(*ReturnedLength, *ReturnedLength) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
    _Inout_ PDWORD ReturnedLength
);
#endif

namespace
{
    inline float XM_CALLCONV DrawStringCenter(SpriteBatch* batch, SpriteFont* font, const wchar_t* text, float mid, float y, FXMVECTOR color, float scale)
    {
        XMVECTOR size = font->MeasureString(text);
        XMFLOAT2 pos(mid - XMVectorGetX(size)*scale / 2.f, y);
        font->DrawString(batch, text, pos, color, 0.f, Vector2::Zero, scale);
        return font->GetLineSpacing() * scale;
    }

    inline void DrawStringLeft(SpriteBatch* batch, SpriteFont* font, const wchar_t* text, float mid, float y, float scale)
    {
        XMVECTOR size = font->MeasureString(text);
        XMFLOAT2 pos(mid - XMVectorGetX(size)*scale, y);
        font->DrawString(batch, text, pos, ATG::Colors::Blue, 0.f, Vector2::Zero, scale);
    }

    inline float DrawStringRight(SpriteBatch* batch, SpriteFont* font, const wchar_t* text, float mid, float y, float scale)
    {
        XMFLOAT2 pos(mid, y);
        font->DrawString(batch, text, pos, ATG::Colors::White, 0.f, Vector2::Zero, scale);
        return font->GetLineSpacing()*scale;
    }

#ifdef _GAMING_DESKTOP
    inline long ComputeIntersectionArea(
        long ax1, long ay1, long ax2, long ay2,
        long bx1, long by1, long bx2, long by2) noexcept
    {
        return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
    }

    HRESULT GetContainingOutput(_In_ HWND hWnd, _In_ IDXGIFactory* dxgiFactory, _COM_Outptr_ IDXGIOutput** ppOutput)
    {
        if (!ppOutput)
        {
            return E_INVALIDARG;
        }

        *ppOutput = nullptr;

        if (!hWnd || !dxgiFactory)
        {
            return E_INVALIDARG;
        }

        // Get the retangle bounds of the app window.
        RECT windowBounds;
        if (!GetWindowRect(hWnd, &windowBounds))
        {
            return HRESULT_FROM_WIN32(GetLastError());;
        }

        const long ax1 = windowBounds.left;
        const long ay1 = windowBounds.top;
        const long ax2 = windowBounds.right;
        const long ay2 = windowBounds.bottom;

        ComPtr<IDXGIOutput> bestOutput;
        long bestIntersectArea = -1;

        ComPtr<IDXGIAdapter> adapter;
        for (UINT adapterIndex = 0;
            SUCCEEDED(dxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()));
            ++adapterIndex)
        {
            ComPtr<IDXGIOutput> output;
            for (UINT outputIndex = 0;
                SUCCEEDED(adapter->EnumOutputs(outputIndex, output.ReleaseAndGetAddressOf()));
                ++outputIndex)
            {
                // Get the rectangle bounds of current output.
                DXGI_OUTPUT_DESC desc;
                HRESULT hr = output->GetDesc(&desc);
                if (FAILED(hr))
                    return hr;

                const auto& r = desc.DesktopCoordinates;

                // Compute the intersection
                const long intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, r.left, r.top, r.right, r.bottom);
                if (intersectArea > bestIntersectArea)
                {
                    bestOutput.Swap(output);
                    bestIntersectArea = intersectArea;
                }
            }
        }

        if (bestOutput)
        {
            *ppOutput = bestOutput.Detach();
            return S_OK;
        }

        return E_FAIL;
    }
#endif // _GAMING_DESKTOP
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_scale(1.25f),
    m_current(0),
    m_gamepadPresent(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    m_gamepadPresent = pad.IsConnected();
    if (m_gamepadPresent)
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

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Right)
        || m_keyboardButtons.IsKeyPressed(Keyboard::Enter)
        || m_keyboardButtons.IsKeyPressed(Keyboard::Space)
        || m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED
        || m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
    {
        ++m_current;
        if (m_current >= int(InfoPage::MAX))
            m_current = 0;
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Left)
        || m_keyboardButtons.IsKeyPressed(Keyboard::Back)
        || m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED
        || m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
    {
        --m_current;
        if (m_current < 0)
            m_current = int(InfoPage::MAX) - 1;
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    float mid = float(safeRect.left) + float(safeRect.right - safeRect.left) / 2.f;

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    float y = float(safeRect.top);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.bottom) - m_smallFont->GetLineSpacing());

    if (m_gamepadPresent)
    {
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), L"Use [A], [B], or [DPad] to cycle pages", pos, ATG::Colors::LightGrey, m_scale);
    }
    else
    {
        m_smallFont->DrawString(m_batch.get(), L"Use Left/Right to cycle pages", pos, ATG::Colors::LightGrey, 0, Vector2::Zero, m_scale);
    }

    float spacer = XMVectorGetX(XMVectorScale(m_smallFont->MeasureString(L"X"), m_scale));

    float left = mid - spacer;
    float right = mid + spacer;

    switch (static_cast<InfoPage>(m_current))
    {
    case InfoPage::SYSTEMINFO:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GetNativeSystemInfo", mid, y, ATG::Colors::LightGrey, m_scale);

        SYSTEM_INFO info = {};
        GetNativeSystemInfo(&info);

        const wchar_t* arch = L"UNKNOWN";
        switch (info.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:  arch = L"AMD64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL:  arch = L"INTEL"; break;
        }

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"wProcessorArchitecture", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), arch, right, y, m_scale);

        wchar_t buff[128] = {};
        swprintf_s(buff, L"%u", info.wProcessorLevel);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"wProcessorLevel", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%04X", info.wProcessorRevision);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"wProcessorRevision", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%zX", info.dwActiveProcessorMask);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"dwActiveProcessorMask", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%u", info.dwNumberOfProcessors);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"dwNumberOfProcessors", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%u", info.dwPageSize);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"dwPageSize", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%u", info.dwAllocationGranularity);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"dwAllocationGranularity", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%p", info.lpMinimumApplicationAddress);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"lpMinimumApplicationAddress", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%p", info.lpMaximumApplicationAddress);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"lpMaximumApplicationAddress", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
    }
    break;

    case InfoPage::GLOBALMEMORYSTATUS:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GlobalMemoryStatusEx", mid, y, ATG::Colors::LightGrey, m_scale);

        MEMORYSTATUSEX info = {};
        info.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&info))
        {
            auto tphys = static_cast<uint32_t>(info.ullTotalPhys / (1024 * 1024));
            auto aphys = static_cast<uint32_t>(info.ullAvailPhys / (1024 * 1024));
            auto tvirt = static_cast<uint32_t>(info.ullTotalVirtual / (1024 * 1024));
            auto avirt = static_cast<uint32_t>(info.ullAvailVirtual / (1024 * 1024));

            wchar_t buff[128] = {};
            swprintf_s(buff, L"%u / %u (MB)", aphys, tphys);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Physical Memory", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            swprintf_s(buff, L"%u (MB)", tvirt);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Total Virtual Memory", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            swprintf_s(buff, L"%u (MB)", avirt);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Available VM", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

#ifdef _GAMING_DESKTOP
            auto tpage = static_cast<uint32_t>(info.ullTotalPageFile / (1024 * 1024));
            auto apage = static_cast<uint32_t>(info.ullAvailPageFile / (1024 * 1024));

            swprintf_s(buff, L"%u / %u (MB)", apage, tpage);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Page File", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            if (info.ullAvailExtendedVirtual > 0)
            {
                auto axvirt = static_cast<uint32_t>(info.ullAvailExtendedVirtual / (1024 * 1024));

                swprintf_s(buff, L"%u (MB)", axvirt);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Available Extended VM", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }
#endif // _GAMING_DESKTOP
        }
    }
    break;

    case InfoPage::GETDEVICETYPE:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"XSystemGetDeviceType", mid, y, ATG::Colors::LightGrey, m_scale);

        XSystemDeviceType deviceType = XSystemGetDeviceType();

        wchar_t buff[128] = {};
        swprintf_s(buff, L"%02X", deviceType);

        switch (deviceType)
        {
        case XSystemDeviceType::Pc:                     wcscat_s(buff, L" (Desktop PC)"); break;
        case XSystemDeviceType::XboxOne:                wcscat_s(buff, L" (Xbox One)"); break;
        case XSystemDeviceType::XboxOneS:               wcscat_s(buff, L" (Xbox One S)"); break;
        case XSystemDeviceType::XboxOneX:               wcscat_s(buff, L" (Xbox One X)"); break;
        case XSystemDeviceType::XboxOneXDevkit:         wcscat_s(buff, L" (Xbox One X DevKit)"); break;
        case XSystemDeviceType::XboxScarlettLockhart:   wcscat_s(buff, L" (Xbox Series S)"); break;
        case XSystemDeviceType::XboxScarlettAnaconda:   wcscat_s(buff, L" (Xbox Series X)"); break;
        case XSystemDeviceType::XboxScarlettDevkit:     wcscat_s(buff, L" (Xbox Series X DevKit)"); break;
        case XSystemDeviceType::Unknown:                wcscat_s(buff, L" (Unknown)"); break;
        }

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"DeviceType", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
    }
    break;

    case InfoPage::ANALYTICSINFO:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"XSystemGetAnalyticsInfo", mid, y, ATG::Colors::LightGrey, m_scale);

        auto analyticsInfo = XSystemGetAnalyticsInfo();

        wchar_t buff[128] = {};
        int result = MultiByteToWideChar(CP_UTF8, 0, analyticsInfo.form, -1, buff, _countof(buff));
        if (result > 0)
        {
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"form", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
        }

        result = MultiByteToWideChar(CP_UTF8, 0, analyticsInfo.family, -1, buff, _countof(buff));
        if (result > 0)
        {
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"family", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
        }

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"osVersion (Title OS)", left, y, m_scale);
        swprintf_s(buff, L"%u.%u.%u.%u", analyticsInfo.osVersion.major, analyticsInfo.osVersion.minor, analyticsInfo.osVersion.build, analyticsInfo.osVersion.revision);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"hostingOsVersion (Host OS)", left, y, m_scale);

        swprintf_s(buff, L"%u.%u.%u.%u", analyticsInfo.hostingOsVersion.major, analyticsInfo.hostingOsVersion.minor, analyticsInfo.hostingOsVersion.build, analyticsInfo.hostingOsVersion.revision);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

#ifdef _GAMING_XBOX
        // The _GXDK_VER captures at compile-time the version of the Gaming SDK used to build the application
        y += m_smallFont->GetLineSpacing() * 2;

        swprintf_s(buff, L"%08X (%ls)", _GXDK_VER, _GXDK_VER_STRING_COMPACT_W);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"_GXDK_VER", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%u.%u", HIWORD(_GXDK_VER), LOWORD(_GXDK_VER));
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
#else // _GAMING_DESKTOP
        // The _GRDK_VER captures at compile-time the version of the Gaming SDK used to build the application
        y += m_smallFont->GetLineSpacing() * 2;

        swprintf_s(buff, L"%08X (%ls)", _GRDK_VER, _GRDK_VER_STRING_COMPACT_W);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"_GRDK_VER", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        swprintf_s(buff, L"%u.%u", HIWORD(_GRDK_VER), LOWORD(_GRDK_VER));
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
#endif

        // Standard GetVersionEx
        y += m_smallFont->GetLineSpacing() * 2;

        OSVERSIONINFO osVer = {};
        osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#pragma warning(suppress : 4996)
        GetVersionEx(&osVer);

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"GetVersionEx", left, y, m_scale);

        swprintf_s(buff, L"%u.%u.%u", osVer.dwMajorVersion, osVer.dwMinorVersion, osVer.dwBuildNumber);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
    }
    break;

    case InfoPage::CONSOLEINFO:
    {
        wchar_t buff[128] = {};

#ifdef _GAMING_XBOX
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"XSystemGetConsoleId", mid, y, ATG::Colors::LightGrey, m_scale);

        char id[XSystemConsoleIdBytes] = {};
        if (SUCCEEDED(XSystemGetConsoleId(sizeof(id), id, nullptr)))
        {
            int result = MultiByteToWideChar(CP_UTF8, 0, id, -1, buff, _countof(buff));
            if (result > 0)
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Console ID", left, y, m_scale);
                DrawStringRight(m_batch.get(), m_smallFont.get(), buff, left, y, m_scale);
            }
        }

        y += m_smallFont->GetLineSpacing() * 2;
#endif // _GAMING_XBOX

        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"XSystemGetXboxLiveSandboxId", mid, y, ATG::Colors::LightGrey, m_scale);

        char sandbox[XSystemXboxLiveSandboxIdMaxBytes] = {};
        if (SUCCEEDED(XSystemGetXboxLiveSandboxId(sizeof(sandbox), sandbox, nullptr)))
        {
            int result = MultiByteToWideChar(CP_UTF8, 0, sandbox, -1, buff, _countof(buff));
            if (result > 0)
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Sandbox ID", left, y, m_scale);
                DrawStringRight(m_batch.get(), m_smallFont.get(), buff, left, y, m_scale);
            }
        }
    }
    break;

    case InfoPage::GLPI:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GetLogicalProcessorInformation", mid, y, ATG::Colors::LightGrey, m_scale);

        bool success = false;
        DWORD bufferSize = 0;

        (void)GetLogicalProcessorInformation(nullptr, &bufferSize);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::unique_ptr<uint8_t[]> glpiBuffer(new uint8_t[bufferSize]);
            auto glpi = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(glpiBuffer.get());

            if (GetLogicalProcessorInformation(glpi, &bufferSize))
            {
                success = true;

                size_t itemCount = bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

                // Number of cores available to this process
                size_t processCores = 0;

                DWORD_PTR dwProcessAffinity = 0, dwSystemAffinity = 0;
                (void)GetProcessAffinityMask(GetCurrentProcess(), &dwProcessAffinity, &dwSystemAffinity);

                // Number of physical cores
                size_t physicalCores = 0;

                for (size_t j = 0; j < itemCount; ++j)
                {
                    if (glpi[j].Relationship == RelationProcessorCore)
                    {
                        ++physicalCores;

                        if (glpi[j].ProcessorMask & dwProcessAffinity)
                            ++processCores;
                    }
                }

                wchar_t buff[128] = {};
                if (physicalCores == processCores)
                {
                    swprintf_s(buff, L"%zu", physicalCores);
                }
                else
                {
                    swprintf_s(buff, L"%zu (%zu available)", physicalCores, processCores);
                }
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Physical cores", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                if (physicalCores <= 32)
                {
                    size_t coreIdx = 0;
                    for (size_t j = 0; j < itemCount; ++j)
                    {
                        if (glpi[j].Relationship == RelationProcessorCore)
                        {
                            swprintf_s(buff, L"%zu", coreIdx);
                            DrawStringLeft(m_batch.get(), m_smallFont.get(), buff, left, y, m_scale);

                            ULONG_PTR mask = glpi[j].ProcessorMask;
                            wchar_t* ptr = buff;
                            for (ULONG_PTR msb = ULONG_PTR(1) << 31; msb; msb >>= 1)
                                *ptr++ = ((msb & mask) ? L'1' : L'0');
                            *ptr = 0;
                            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                            coreIdx++;
                        }
                    }
                }
            }
        }

        if (!success)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Failed GetLogicalProcessorInformation", mid, y, ATG::Colors::Orange, m_scale);
        }
    }
    break;

    case InfoPage::GLPIEX:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GetLogicalProcessorInformationEx", mid, y, ATG::Colors::LightGrey, m_scale);

        bool success = false;
        DWORD bufferSize = 0;

        (void)GetLogicalProcessorInformationEx(RelationCache, nullptr, &bufferSize);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::unique_ptr<uint8_t[]> glpiBuffer(new uint8_t[bufferSize]);
            auto glpi = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(glpiBuffer.get());

            if (GetLogicalProcessorInformationEx(RelationCache, glpi, &bufferSize))
            {
                success = true;

                struct Info
                {
                    size_t l1cache;
                    size_t l2cache;
                    size_t l3cache;
                };

                std::map<ULONG_PTR, Info> cacheInformation;

                for (auto glpiptr = glpiBuffer.get(); glpiptr < glpiBuffer.get() + bufferSize;)
                {
                    glpi = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(glpiptr);

                    if (glpi->Relationship == RelationCache && glpi->Cache.GroupMask.Group == 0)
                    {
                        switch (glpi->Cache.Level)
                        {
                        case 1:
                            cacheInformation[glpi->Cache.GroupMask.Mask].l1cache = glpi->Cache.CacheSize;
                            break;

                        case 2:
                            cacheInformation[glpi->Cache.GroupMask.Mask].l2cache = glpi->Cache.CacheSize;
                            break;

                        case 3:
                            cacheInformation[glpi->Cache.GroupMask.Mask].l3cache = glpi->Cache.CacheSize;
                            break;
                        }
                    }

                    if (!glpi->Size)
                        break;

                    glpiptr += glpi->Size;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Group Mask", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), L"L1 / L2 / L3 Cache", right, y, m_scale);

                for (auto it : cacheInformation)
                {
                    wchar_t buff[128] = {};

                    wchar_t* ptr = buff;
                    for (ULONG_PTR msb = ULONG_PTR(1) << 31; msb; msb >>= 1)
                        *ptr++ = ((msb & it.first) ? L'1' : L'0');
                    *ptr = 0;
                    DrawStringLeft(m_batch.get(), m_smallFont.get(), buff, left, y, m_scale);

                    swprintf_s(buff, L"%-10zu / %-10zu  / %-10zu", it.second.l1cache, it.second.l2cache, it.second.l3cache);
                    y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
                }
            }
        }

        if (!success)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Failed GetLogicalProcessorInformationEx", mid, y, ATG::Colors::Orange, m_scale);
        }
    }
    break;

#ifdef _GAMING_XBOX

    case InfoPage::DIRECT3D_XBOX:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig = {};
        device->GetGpuHardwareConfigurationX(&hwConfig);

        wchar_t buff[128] = {};
        swprintf_s(buff, L"%llu", hwConfig.GpuFrequency);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"GPU Frequency", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        const wchar_t* hwver = L"Unknown";
        switch (hwConfig.HardwareVersion)
        {
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE: hwver = L"Xbox One"; break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S: hwver = L"Xbox One S"; break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X: hwver = L"Xbox One X"; break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT: hwver = L"Xbox One X (DevKit)"; break;
    #ifdef _GAMING_XBOX_SCARLETT
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART: hwver = L"Xbox Series S"; break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA: hwver = L"Xbox Series X"; break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT: hwver = L"Xbox Series X (DevKit)"; break;
    #endif
        }

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Hardware Version", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), hwver, right, y, m_scale);
    }
    break;

    case InfoPage::DXGI_XBOX:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"DXGI", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        ComPtr<IDXGIDevice> dxgiDevice;
        DX::ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiDevice.GetAddressOf())));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        ComPtr<IDXGIOutput> output;
        DX::ThrowIfFailed(dxgiAdapter->EnumOutputs(0, output.GetAddressOf()));

#ifdef _GAMING_XBOX_SCARLETT
        bool vrrSupported = false;

        UINT outputModeCount = 0;
        std::ignore = output->GetDisplayModeListX(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, nullptr);

        auto outputModes = std::make_unique<DXGIXBOX_MODE_DESC[]>(outputModeCount);

        DX::ThrowIfFailed(output->GetDisplayModeListX(
            m_deviceResources->GetBackBufferFormat(),
            0,
            &outputModeCount,
            outputModes.get()));

        for (uint32_t i = 0; i < outputModeCount; ++i)
        {
            vrrSupported |= (outputModes[i].Flags & DXGIXBOX_MODE_FLAG_VARIABLE_REFRESH_RATE) != 0;
        }
#else
        UINT outputModeCount = 0;
        std::ignore = output->GetDisplayModeList(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, nullptr);

        auto outputModes = std::make_unique<DXGI_MODE_DESC[]>(outputModeCount);

        DX::ThrowIfFailed(output->GetDisplayModeList(
            m_deviceResources->GetBackBufferFormat(),
            0,
            &outputModeCount,
            outputModes.get()));
#endif

        bool supports24Hz = false;
        bool supports40Hz = false;
        bool supports120Hz = false;

        // 30Hz and 60Hz are always supported.

        for (uint32_t i = 0; i < outputModeCount; ++i)
        {
            switch (outputModes[i].RefreshRate.Numerator)
            {
            case 24: supports24Hz = true; break;
            case 40: supports40Hz = true; break;
            case 120: supports120Hz = true; break;
            }
        }

        // Note that on Xbox, the IDXGIOutput::GetDesc method is not supported.

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12XBOX_FRAME_INTERVAL_30_HZ", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), L"true", right, y, m_scale);

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12XBOX_FRAME_INTERVAL_60_HZ", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), L"true", right, y, m_scale);

        y += m_smallFont->GetLineSpacing() * 2;

        // Note use of D3D12XBOX_FRAME_INTERVAL_24_HZ requires March 2022 GDKX or later.
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12XBOX_FRAME_INTERVAL_24_HZ", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), supports24Hz ? L"true" : L"false", right, y, m_scale);

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12XBOX_FRAME_INTERVAL_40_HZ", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), supports40Hz ? L"true" : L"false", right, y, m_scale);

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12XBOX_FRAME_INTERVAL_120_HZ", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), supports120Hz ? L"true" : L"false", right, y, m_scale);

#ifdef _GAMING_XBOX_SCARLETT
        y += m_smallFont->GetLineSpacing() * 2;

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Variable Refresh Rate Supported", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), vrrSupported ? L"true" : L"false", right, y, m_scale);
#endif
    }
    break;

#else // _GAMING_DESKTOP

    case InfoPage::GETPROCESSINFO:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GetProcessInformation", mid, y, ATG::Colors::LightGrey, m_scale);

        APP_MEMORY_INFORMATION info = {};
        if (GetProcessInformation(GetCurrentProcess(), ProcessAppMemoryInfo, &info, sizeof(info)))
        {
            auto ac = static_cast<uint32_t>(info.AvailableCommit / (1024 * 1024));
            auto pc = static_cast<uint32_t>(info.PrivateCommitUsage / (1024 * 1024));
            auto ppc = static_cast<uint32_t>(info.PeakPrivateCommitUsage / (1024 * 1024));
            auto tc = static_cast<uint32_t>(info.TotalCommitUsage / (1024 * 1024));

            wchar_t buff[128] = {};
            swprintf_s(buff, L"%u (MiB)", ac);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AvailableCommit", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            swprintf_s(buff, L"%u (MiB)", pc);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"PrivateCommitUsage", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            swprintf_s(buff, L"%u (MiB)", ppc);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"PeakPrivateCommitUsage", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            swprintf_s(buff, L"%u (MiB)", tc);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"TotalCommitUsage", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
        }
    }
    break;

    case InfoPage::CPUSETS:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"GetSystemCpuSetInformation", mid, y, ATG::Colors::LightGrey, m_scale);

        ULONG retsize = 0;
        (void)GetSystemCpuSetInformation(nullptr, 0, &retsize, GetCurrentProcess(), 0);

        std::unique_ptr<uint8_t[]> data(new uint8_t[retsize]);
        if (GetSystemCpuSetInformation(
            reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(data.get()),
            retsize, &retsize, GetCurrentProcess(), 0))
        {
            size_t logicalProcessors = 0;
            size_t parkedProcessors = 0;
            size_t allocatedProcessors = 0;
            size_t allocatedElsewhere = 0;
            size_t availableProcessors = 0;
            std::set<DWORD> cores;
            bool moreThanOneGroup = false;

            uint8_t const* ptr = data.get();
            for (DWORD size = 0; size < retsize; )
            {
                auto info = reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION*>(ptr);
                if (info->Type == CpuSetInformation)
                {
                    if (info->CpuSet.Group > 0)
                    {
                        moreThanOneGroup = true;
                    }
                    else
                    {
                        ++logicalProcessors;

                        if (info->CpuSet.Parked)
                        {
                            ++parkedProcessors;
                        }
                        else
                        {
                            if (info->CpuSet.Allocated)
                            {
                                if (info->CpuSet.AllocatedToTargetProcess)
                                {
                                    ++allocatedProcessors;
                                    ++availableProcessors;
                                    cores.insert(info->CpuSet.CoreIndex);
                                }
                                else
                                {
                                    ++allocatedElsewhere;
                                }
                            }
                            else
                            {
                                ++availableProcessors;
                                cores.insert(info->CpuSet.CoreIndex);
                            }
                        }
                    }
                }
                ptr += info->Size;
                size += info->Size;
            }

            wchar_t buff[128] = {};
            swprintf_s(buff, L"%zu", logicalProcessors);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Total logical processors", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            if (parkedProcessors > 0)
            {
                swprintf_s(buff, L"%zu", parkedProcessors);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Parked processors", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }

            if (allocatedElsewhere > 0)
            {
                swprintf_s(buff, L"%zu", allocatedElsewhere);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Allocated to other processes", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }

            swprintf_s(buff, L"%zu", availableProcessors);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Available logical processors", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            if (allocatedProcessors > 0)
            {
                swprintf_s(buff, L"%zu", allocatedProcessors);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Allocated logical processors", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }

            swprintf_s(buff, L"%zu", cores.size());
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Available physical cores", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            if (moreThanOneGroup)
            {
                y += m_smallFont->GetLineSpacing() * m_scale;
                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Note more than one group found; ignored extra groups!", mid, y, ATG::Colors::Orange, m_scale);
            }
        }
    }
    break;

    case InfoPage::DIRECT3D:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        // Determine highest feature level
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
            D3D_FEATURE_LEVEL_12_2,
#endif
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
        {
            _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
        };

        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels))))
        {
            featLevels.MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        }

        const wchar_t* featLevel = L"Unknown";
        switch (featLevels.MaxSupportedFeatureLevel)
        {
        case D3D_FEATURE_LEVEL_11_0: featLevel = L"11.0"; break;
        case D3D_FEATURE_LEVEL_11_1: featLevel = L"11.1"; break;
        case D3D_FEATURE_LEVEL_12_0: featLevel = L"12.0"; break;
        case D3D_FEATURE_LEVEL_12_1: featLevel = L"12.1"; break;
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        case D3D_FEATURE_LEVEL_12_2: featLevel = L"12.2"; break;
#endif
        }

        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Hardware Feature Level", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), featLevel, right, y, m_scale);

        // Determine maximum shader model / root signature
        D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSig = {};
        rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSig, sizeof(rootSig))))
        {
            rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        const wchar_t* rootSigVer = L"Unknown";
        switch (rootSig.HighestVersion)
        {
        case D3D_ROOT_SIGNATURE_VERSION_1_0: rootSigVer = L"1.0"; break;
        case D3D_ROOT_SIGNATURE_VERSION_1_1: rootSigVer = L"1.1"; break;
        }

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
#else
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
#endif
        HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        while (hr == E_INVALIDARG && shaderModel.HighestShaderModel > D3D_SHADER_MODEL_6_0)
        {
            shaderModel.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(static_cast<int>(shaderModel.HighestShaderModel) - 1);
            hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        }
        if (FAILED(hr))
        {
            shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
        }

        const wchar_t* shaderModelVer = L"Unknown";
        switch (shaderModel.HighestShaderModel)
        {
        case D3D_SHADER_MODEL_5_1: shaderModelVer = L"5.1"; break;
        case D3D_SHADER_MODEL_6_0: shaderModelVer = L"6.0"; break;
        case D3D_SHADER_MODEL_6_1: shaderModelVer = L"6.1"; break;
        case D3D_SHADER_MODEL_6_2: shaderModelVer = L"6.2"; break;
        case D3D_SHADER_MODEL_6_3: shaderModelVer = L"6.3"; break;
        case D3D_SHADER_MODEL_6_4: shaderModelVer = L"6.4"; break;
        case D3D_SHADER_MODEL_6_5: shaderModelVer = L"6.5"; break;
        case D3D_SHADER_MODEL_6_6: shaderModelVer = L"6.6"; break;

#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        case D3D_SHADER_MODEL_6_7: shaderModelVer = L"6.7"; break;
#endif
        }

        wchar_t buff[128] = {};
        swprintf_s(buff, L"%ls / %ls", shaderModelVer, rootSigVer);
        DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Shader Model / Root Signature", left, y, m_scale);
        y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

        // Optional Direct3D 12 features
        D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(d3d12opts))))
        {
            const wchar_t* tiledTier = L"Unknown";
            switch (d3d12opts.TiledResourcesTier)
            {
            case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED: tiledTier = L"Not supported"; break;
            case D3D12_TILED_RESOURCES_TIER_1: tiledTier = L"Tier 1"; break;
            case D3D12_TILED_RESOURCES_TIER_2: tiledTier = L"Tier 2"; break;
            case D3D12_TILED_RESOURCES_TIER_3: tiledTier = L"Tier 3"; break;
            case D3D12_TILED_RESOURCES_TIER_4: tiledTier = L"Tier 4"; break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"TiledResourcesTier", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), tiledTier, right, y, m_scale);

            const wchar_t* resourceTier = L"Unknown";
            switch (d3d12opts.ResourceBindingTier)
            {
            case D3D12_RESOURCE_BINDING_TIER_1: resourceTier = L"Tier 1"; break;
            case D3D12_RESOURCE_BINDING_TIER_2: resourceTier = L"Tier 2"; break;
            case D3D12_RESOURCE_BINDING_TIER_3: resourceTier = L"Tier 3"; break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ResourceBindingTier", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), resourceTier, right, y, m_scale);

            const wchar_t* cRastTier = L"Unknown";
            switch (d3d12opts.ConservativeRasterizationTier)
            {
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED: cRastTier = L"Not supported"; break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1: cRastTier = L"Tier 1"; break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2: cRastTier = L"Tier 2"; break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3: cRastTier = L"Tier 3"; break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ConservativeRasterizationTier", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), cRastTier, right, y, m_scale);

            const wchar_t* heapTier = L"Unknown";
            switch (d3d12opts.ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1: heapTier = L"Tier 1"; break;
            case D3D12_RESOURCE_HEAP_TIER_2: heapTier = L"Tier 2"; break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ResourceHeapTier", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), heapTier, right, y, m_scale);

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"StandardSwizzle64KBSupported", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts.StandardSwizzle64KBSupported ? L"true" : L"false", right, y, m_scale);

            const wchar_t* crossTier = L"Unknown";
            switch (d3d12opts.CrossNodeSharingTier)
            {
            case D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED: crossTier = L"Not supported"; break;
            case D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED: crossTier = L"Tier 1 (emulated)"; break;
            case D3D12_CROSS_NODE_SHARING_TIER_1: crossTier = L"Tier 1"; break;
            case D3D12_CROSS_NODE_SHARING_TIER_2: crossTier = L"Tier 2"; break;
            case D3D12_CROSS_NODE_SHARING_TIER_3: crossTier = L"Tier 3"; break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"CrossNodeSharingTier", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), crossTier, right, y, m_scale);

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"CrossAdapterRowMajorTextureSupported", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts.CrossAdapterRowMajorTextureSupported ? L"true" : L"false", right, y, m_scale);

            swprintf_s(buff, L"%u", d3d12opts.MaxGPUVirtualAddressBitsPerResource);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MaxGPUVirtualAddressBitsPerResource", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
        }
    }
    break;

    case InfoPage::DIRECT3D_OPT1:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12 Optional Features (1 of 5)", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        if (!device)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Not supported", mid, y, ATG::Colors::Orange, m_scale);
        }
        else
        {
            // Optional Direct3D 12 features for Windows 10 Anniversary Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12opts1 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12opts1, sizeof(d3d12opts1))))
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"WaveOps", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts1.WaveOps ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ExpandedComputeResourceStates", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts1.ExpandedComputeResourceStates ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Int64ShaderOps", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts1.Int64ShaderOps ? L"true" : L"false", right, y, m_scale);
            }

            // Optional Direct3D 12 features for Windows 10 Creators Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS2 d3d12opts2 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &d3d12opts2, sizeof(d3d12opts2))))
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"DepthBoundsTestSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts2.DepthBoundsTestSupported ? L"true" : L"false", right, y, m_scale);

                const wchar_t* psmpTier = L"Unknown";
                switch (d3d12opts2.ProgrammableSamplePositionsTier)
                {
                case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED: psmpTier = L"Not supported"; break;
                case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1: psmpTier = L"Tier 1"; break;
                case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2: psmpTier = L"Tier 2"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ProgrammableSamplePositionsTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), psmpTier, right, y, m_scale);
            }

            // Optional Direct3D 12 features for Windows 10 Fall Creators Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS3 d3d12opts3 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &d3d12opts3, sizeof(d3d12opts3))))
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"CopyQueueTimestampQueriesSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts3.CopyQueueTimestampQueriesSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"CastingFullyTypedFormatSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts3.CastingFullyTypedFormatSupported ? L"true" : L"false", right, y, m_scale);

                d3d12opts3.WriteBufferImmediateSupportFlags = static_cast<D3D12_COMMAND_LIST_SUPPORT_FLAGS>(0xFFFFFFFF);
                wchar_t vbSupportFlags[128] = {};
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_DIRECT)
                {
                    wcscat_s(vbSupportFlags, L"DIRECT ");
                }
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_BUNDLE)
                {
                    wcscat_s(vbSupportFlags, L"BUNDLE ");
                }
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_COMPUTE)
                {
                    wcscat_s(vbSupportFlags, L"COMPUTE ");
                }
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_COPY)
                {
                    wcscat_s(vbSupportFlags, L"COPY ");
                }
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_DECODE)
                {
                    wcscat_s(vbSupportFlags, L"VDECODE ");
                }
                if (d3d12opts3.WriteBufferImmediateSupportFlags & D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_PROCESS)
                {
                    wcscat_s(vbSupportFlags, L"VPROCESS");
                }
                if (!*vbSupportFlags)
                {
                    wcscat_s(vbSupportFlags, L"None");
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"WriteBufferImmediateSupportFlags", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), vbSupportFlags, right, y, m_scale);

                const wchar_t* vinstTier = L"Unknown";
                switch (d3d12opts3.ViewInstancingTier)
                {
                case D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED: vinstTier = L"Not supported"; break;
                case D3D12_VIEW_INSTANCING_TIER_1: vinstTier = L"Tier 1"; break;
                case D3D12_VIEW_INSTANCING_TIER_2: vinstTier = L"Tier 2"; break;
                case D3D12_VIEW_INSTANCING_TIER_3: vinstTier = L"Tier 3"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ViewInstancingTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), vinstTier, right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"BarycentricsSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts3.BarycentricsSupported ? L"true" : L"false", right, y, m_scale);
            }

            D3D12_FEATURE_DATA_EXISTING_HEAPS d3d12heaps = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_EXISTING_HEAPS, &d3d12heaps, sizeof(d3d12heaps))))
            {
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Existing Heaps", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12heaps.Supported ? L"true" : L"false", right, y, m_scale);
            }
        }
    }
    break;

    case InfoPage::DIRECT3D_OPT2:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12 Optional Features (2 of 5)", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        if (!device)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Not supported", mid, y, ATG::Colors::Orange, m_scale);
        }
        else
        {
            bool found = false;

            // Optional Direct3D 12 features for Windows 10 April 2018 Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12opts4 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12opts4, sizeof(d3d12opts4))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MSAA64KBAlignedTextureSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts4.MSAA64KBAlignedTextureSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Native16BitShaderOpsSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts4.Native16BitShaderOpsSupported ? L"true" : L"false", right, y, m_scale);

                const wchar_t* srcompatTier = L"Unknown";
                switch (d3d12opts4.SharedResourceCompatibilityTier)
                {
                case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0: srcompatTier = L"Tier 0"; break;
                case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_1: srcompatTier = L"Tier 1"; break;
                case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_2: srcompatTier = L"Tier 2"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"SharedResourceCompatibilityTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), srcompatTier, right, y, m_scale);
            }

            D3D12_FEATURE_DATA_SERIALIZATION d3d12serial = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SERIALIZATION, &d3d12serial, sizeof(d3d12serial))))
            {
                found = true;

                const wchar_t* serialTier = L"Unknown";
                switch (d3d12serial.HeapSerializationTier)
                {
                case D3D12_HEAP_SERIALIZATION_TIER_0: serialTier = L"Tier 0"; break;
                case D3D12_HEAP_SERIALIZATION_TIER_10: serialTier = L"Tier 10"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"HeapSerializationTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), serialTier, right, y, m_scale);

                wchar_t buff[128] = {};
                swprintf_s(buff, L"%u", d3d12serial.NodeIndex);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Serialization NodeIndex", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }

            D3D12_FEATURE_DATA_CROSS_NODE d3d12xnode = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_CROSS_NODE, &d3d12xnode, sizeof(d3d12xnode))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Cross node AtomicShaderInstructions", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12xnode.AtomicShaderInstructions ? L"true" : L"false", right, y, m_scale);

                const wchar_t* shareTier = L"Unknown";
                switch (d3d12xnode.SharingTier)
                {
                case D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED: shareTier = L"Not supported"; break;
                case D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED: shareTier = L"Tier 1 (Emulated)"; break;
                case D3D12_CROSS_NODE_SHARING_TIER_1: shareTier = L"Tier 1"; break;
                case D3D12_CROSS_NODE_SHARING_TIER_2: shareTier = L"Tier 2"; break;
                case D3D12_CROSS_NODE_SHARING_TIER_3: shareTier = L"Tier 3"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Cross node SharingTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), shareTier, right, y, m_scale);
            }

            // Optional Direct3D 12 features for Windows 10 October 2018 Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12opts5 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12opts5, sizeof(d3d12opts5))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"SRVOnlyTiledResourceTier3", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts5.SRVOnlyTiledResourceTier3 ? L"true" : L"false", right, y, m_scale);

                const wchar_t* passTier = L"Unknown";
                switch (d3d12opts5.RenderPassesTier)
                {
                case D3D12_RENDER_PASS_TIER_0: passTier = L"Tier 0"; break;
                case D3D12_RENDER_PASS_TIER_1: passTier = L"Tier 1"; break;
                case D3D12_RENDER_PASS_TIER_2: passTier = L"Tier 2"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"RenderPassesTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), passTier, right, y, m_scale);

                const wchar_t* rtTier = L"Unknown";
                switch (d3d12opts5.RaytracingTier)
                {
                case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: rtTier = L"Not Supported"; break;
                case D3D12_RAYTRACING_TIER_1_0: rtTier = L"Tier 1"; break;
                case D3D12_RAYTRACING_TIER_1_1: rtTier = L"Tier 1.1"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"RaytracingTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), rtTier, right, y, m_scale);
            }

            if (!found)
            {
                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Requires Windows 10 April 2018 Update or later", mid, y, ATG::Colors::Orange, m_scale);
            }
        }
    }
    break;

    case InfoPage::DIRECT3D_OPT3:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12 Optional Features (3 of 5)", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        if (!device)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Not supported", mid, y, ATG::Colors::Orange, m_scale);
        }
        else
        {
            bool found = false;

            // Optional Direct3D 12 features for Windows 10 May 2019 Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12opts6 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12opts6, sizeof(d3d12opts6))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AdditionalShadingRatesSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts6.AdditionalShadingRatesSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"PerPrimitive VRS ViewportIndexing", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing ? L"true" : L"false", right, y, m_scale);

                const wchar_t* vrsTier = L"Unknown";
                switch (d3d12opts6.VariableShadingRateTier)
                {
                case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: vrsTier = L"Not Supported"; break;
                case D3D12_VARIABLE_SHADING_RATE_TIER_1: vrsTier = L"Tier 1"; break;
                case D3D12_VARIABLE_SHADING_RATE_TIER_2: vrsTier = L"Tier 2"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"VariableShadingRateTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), vrsTier, right, y, m_scale);

                wchar_t buff[128] = {};
                swprintf_s(buff, L"%u", d3d12opts6.ShadingRateImageTileSize);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ShadingRateImageTileSize", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"BackgroundProcessingSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts6.BackgroundProcessingSupported ? L"true" : L"false", right, y, m_scale);
            }

            // Optional Direct3D 12 features for Windows 10 "20H1" Update
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 d3d12opts7 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &d3d12opts7, sizeof(d3d12opts7))))
            {
                found = true;

                // https://devblogs.microsoft.com/directx/coming-to-directx-12-more-control-over-memory-allocation/
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), L"true", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"D3D12_HEAP_FLAG_CREATE_NOT_ZEROED", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), L"true", right, y, m_scale);

                const wchar_t* msTier = L"Unknown";
                switch (d3d12opts7.MeshShaderTier)
                {
                case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED: msTier = L"Not Supported"; break;
                case D3D12_MESH_SHADER_TIER_1: msTier = L"Tier 1"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MeshShaderTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), msTier, right, y, m_scale);

                const wchar_t* sfTier = L"Unknown";
                switch (d3d12opts7.SamplerFeedbackTier)
                {
                case D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED: sfTier = L"Not Supported"; break;
                case D3D12_SAMPLER_FEEDBACK_TIER_0_9: sfTier = L"Tier 0.9"; break;
                case D3D12_SAMPLER_FEEDBACK_TIER_1_0: sfTier = L"Tier 1.0"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"SamplerFeedbackTier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), sfTier, right, y, m_scale);
            }

            if (!found)
            {
                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Requires Windows 10 May 2019 Update or later", mid, y, ATG::Colors::Orange, m_scale);
            }
        }
    }
    break;

    case InfoPage::DIRECT3D_OPT4:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12 Optional Features (4 of 5)", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        if (!device)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Not supported", mid, y, ATG::Colors::Orange, m_scale);
        }
        else
        {
            bool found = false;

#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
            // Optional Direct3D 12 features for Agility SDK or Windows 11
            D3D12_FEATURE_DATA_D3D12_OPTIONS8 d3d12opts8 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &d3d12opts8, sizeof(d3d12opts8))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"UnalignedBlockTexturesSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts8.UnalignedBlockTexturesSupported ? L"true" : L"false", right, y, m_scale);
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS9 d3d12opts9 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &d3d12opts9, sizeof(d3d12opts9))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MS PipelineStatsSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts9.MeshShaderPipelineStatsSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MS FullRange RTArrayIndex", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts9.MeshShaderSupportsFullRangeRenderTargetArrayIndex ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AtomicInt64 OnTypedResource", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts9.AtomicInt64OnTypedResourceSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AtomicInt64 OnGroupShared", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts9.AtomicInt64OnGroupSharedSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Derivatives in MS & AS", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts9.DerivativesInMeshAndAmplificationShadersSupported ? L"true" : L"false", right, y, m_scale);

                const wchar_t* wavemma = L"Unknown";
                switch (d3d12opts9.WaveMMATier)
                {
                case D3D12_WAVE_MMA_TIER_NOT_SUPPORTED: wavemma = L"Not Supported"; break;
                case D3D12_WAVE_MMA_TIER_1_0: wavemma = L"Tier 1.0"; break;
                }

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"WaveMMATier", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), wavemma, right, y, m_scale);
            }
#endif

#if defined(NTDDI_WIN10_CO) || defined(USING_D3D12_AGILITY_SDK)
            // Optional Direct3D 12 features for Agility SDK or Windows 11
            D3D12_FEATURE_DATA_D3D12_OPTIONS10 d3d12opts10 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &d3d12opts10, sizeof(d3d12opts10))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"VRS SumCombinerSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts10.VariableRateShadingSumCombinerSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MS PerPrimitive VRS", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts10.MeshShaderPerPrimitiveShadingRateSupported ? L"true" : L"false", right, y, m_scale);
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS11 d3d12opts11 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &d3d12opts11, sizeof(d3d12opts11))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AtomicInt64 OnDescriptorHeapResource", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts11.AtomicInt64OnDescriptorHeapResourceSupported ? L"true" : L"false", right, y, m_scale);
            }
#endif

            if (!found)
            {
                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Requires Agilty SDK or Windows 11", mid, y, ATG::Colors::Orange, m_scale);
            }
        }
    }
    break;

    case InfoPage::DIRECT3D_OPT5:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"Direct3D 12 Optional Features (5 of 5)", mid, y, ATG::Colors::LightGrey, m_scale);

        auto device = m_deviceResources->GetD3DDevice();

        if (!device)
        {
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Not supported", mid, y, ATG::Colors::Orange, m_scale);
        }
        else
        {
            bool found = false;

#if defined(NTDDI_WIN10_NI) || defined(USING_D3D12_AGILITY_SDK)
            // Optional Direct3D 12 features for Agility SDK or Windows 11, Version 22H2
            D3D12_FEATURE_DATA_D3D12_OPTIONS12 d3d12opts12 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &d3d12opts12, sizeof(d3d12opts12))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"MS Stats Includes Culled Primitives", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"EnhancedBarriersSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts12.EnhancedBarriersSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"RelaxedFormatCastingSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts12.RelaxedFormatCastingSupported ? L"true" : L"false", right, y, m_scale);
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS13 d3d12opts13 = {};
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &d3d12opts13, sizeof(d3d12opts13))))
            {
                found = true;

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"UnrestrictedBufferTextureCopyPitchSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.UnrestrictedBufferTextureCopyPitchSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"UnrestrictedVertexElementAlignmentSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.UnrestrictedVertexElementAlignmentSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"InvertedViewportHeightFlipsYSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.InvertedViewportHeightFlipsYSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"InvertedViewportDepthFlipsZSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.InvertedViewportDepthFlipsZSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"TextureCopyBetweenDimensionsSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.TextureCopyBetweenDimensionsSupported ? L"true" : L"false", right, y, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"AlphaBlendFactorSupported", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), d3d12opts13.AlphaBlendFactorSupported  ? L"true" : L"false", right, y, m_scale);
            }
#endif

            if (!found)
            {
                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"Requires Agilty SDK or Windows 11, Version 22H2", mid, y, ATG::Colors::Orange, m_scale);
            }
        }
    }
    break;

    case InfoPage::DXGI:
    {
        y += DrawStringCenter(m_batch.get(), m_largeFont.get(), L"DXGI", mid, y, ATG::Colors::LightGrey, m_scale);

        y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"DXGI_OUTPUT_DESC", mid, y, ATG::Colors::OffWhite, m_scale);

        // IDXGISwapChain::GetContainingOutput no longer works here, so we
        // use our own implementation.

        ComPtr<IDXGIOutput> output;
        HRESULT hr = GetContainingOutput(
            m_deviceResources->GetWindow(),
            m_deviceResources->GetDXGIFactory(),
            output.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            DXGI_OUTPUT_DESC outputDesc = {};
            DX::ThrowIfFailed(output->GetDesc(&outputDesc));

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"DeviceName", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), outputDesc.DeviceName, right, y, m_scale);

            wchar_t buff[128] = {};
            swprintf_s(buff, L"%u,%u,%u,%u", outputDesc.DesktopCoordinates.left, outputDesc.DesktopCoordinates.top, outputDesc.DesktopCoordinates.right, outputDesc.DesktopCoordinates.bottom);
            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"DesktopCoordinates", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

            const wchar_t* rotation = L"UNSPECIFIED";
            switch (outputDesc.Rotation)
            {
            case DXGI_MODE_ROTATION_IDENTITY: rotation = L"IDENTITY"; break;
            case DXGI_MODE_ROTATION_ROTATE90: rotation = L"ROTATE90"; break;
            case DXGI_MODE_ROTATION_ROTATE180: rotation = L"ROTATE180"; break;
            case DXGI_MODE_ROTATION_ROTATE270: rotation = L"ROTATION270"; break;
            case DXGI_MODE_ROTATION_UNSPECIFIED: break;
            }

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Rotation", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), rotation, right, y, m_scale) * 1.25f;

            ComPtr<IDXGIOutput6> output6;
            if (SUCCEEDED(output.As(&output6)))
            {
                DXGI_OUTPUT_DESC1 outputDesc6;
                DX::ThrowIfFailed(output6->GetDesc1(&outputDesc6));

                const wchar_t* colorSpace = L"sRGB";

                switch (outputDesc6.ColorSpace)
                {
                case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020: colorSpace = L"HDR10"; break;
                case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709: colorSpace = L"Linear"; break;
                default: break;
                }

                y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"DXGI_OUTPUT_DESC1", mid, y, ATG::Colors::OffWhite, m_scale);

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"ColorSpace", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), colorSpace, right, y, m_scale);
            }

            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), L"DXGI_ADAPTER_DESC", mid, y, ATG::Colors::OffWhite, m_scale);

            ComPtr<IDXGIAdapter> adapter;
            if (SUCCEEDED(output->GetParent(IID_PPV_ARGS(adapter.GetAddressOf()))))
            {
                DXGI_ADAPTER_DESC adapterDesc = {};
                DX::ThrowIfFailed(adapter->GetDesc(&adapterDesc));

                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Description", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), adapterDesc.Description, right, y, m_scale);

                swprintf_s(buff, L"%04X / %04X", adapterDesc.VendorId, adapterDesc.DeviceId);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"VendorId / DeviceId", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                swprintf_s(buff, L"%08X / %u", adapterDesc.SubSysId, adapterDesc.Revision);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"SubSysId / Revision", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                auto dvm = static_cast<uint32_t>(adapterDesc.DedicatedVideoMemory / (1024 * 1024));
                auto dsm = static_cast<uint32_t>(adapterDesc.DedicatedSystemMemory / (1024 * 1024));
                auto ssm = static_cast<uint32_t>(adapterDesc.SharedSystemMemory / (1024 * 1024));

                swprintf_s(buff, L"%u (MiB)", dvm);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"DedicatedVideoMemory", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);

                swprintf_s(buff, L"%u (MiB) / %u (MiB)", dsm, ssm);
                DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Dedicated / Shared SystemMemory", left, y, m_scale);
                y += DrawStringRight(m_batch.get(), m_smallFont.get(), buff, right, y, m_scale);
            }
        }
        else
        {
            wchar_t buff[128] = {};
            swprintf_s(buff, L"GetContainingOutput failed with %08X", static_cast<unsigned int>(hr));
            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), buff, mid, y, ATG::Colors::Orange, m_scale);
        }

        y += m_smallFont->GetLineSpacing() * m_scale;

        auto dxgiFactory = m_deviceResources->GetDXGIFactory();

        BOOL allowTearing = FALSE;
        if (SUCCEEDED(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL))))
        {
            const wchar_t* dxgiver = L"DXGI 1.6";

            y += DrawStringCenter(m_batch.get(), m_smallFont.get(), dxgiver, mid, y, ATG::Colors::OffWhite, m_scale);

            DrawStringLeft(m_batch.get(), m_smallFont.get(), L"Allow Tearing", left, y, m_scale);
            y += DrawStringRight(m_batch.get(), m_smallFont.get(), allowTearing ? L"true" : L"false", right, y, m_scale);
        }
    }
    break;

#endif // _GAMING_DESKTOP

    case InfoPage::MAX:
    break;
    }

    m_batch->End();

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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SmallFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::SmallFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    m_largeFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.DDS");
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, strFilePath, m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);

#ifdef _GAMING_DESKTOP
    auto size = m_deviceResources->GetOutputSize();

    if (size.bottom <= 200)
        m_scale = 0.25f;
    else if (size.bottom <= 480)
        m_scale = 0.5f;
    else if (size.bottom <= 600)
        m_scale = 0.75f;
    else if (size.bottom >= 1080)
        m_scale = 1.5f;
    else if (size.bottom >= 720)
        m_scale = 1.25f;
    else
        m_scale = 1.f;
#endif
}

void Sample::OnDeviceLost()
{
    m_batch.reset();
    m_smallFont.reset();
    m_largeFont.reset();
    m_ctrlFont.reset();
    m_background.Reset();
    m_resourceDescriptors.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
