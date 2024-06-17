//--------------------------------------------------------------------------------------
// ModernGamerTag.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ModernGamertag.h"
#include "StringUtil.h"
#include "LogUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace TextRenderer;

using Microsoft::WRL::ComPtr;

namespace
{
    const XMVECTORF32 c_backgroundColor = { { { 0.254901975f, 0.254901975f, 0.254901975f, 1.f } } }; // #414141

    std::string notoSansRegular = "Assets/Fonts/NotoSans-Regular.ttf";
    std::string notoSansJp = "Assets/Fonts/NotoSansCJKjp-Regular.otf";
    std::string notoSansKor = "Assets/Fonts/NotoSansCJKkr-Regular.otf";
    std::string notoSansArabic = "Assets/Fonts/NotoNaskhArabic-Regular.ttf";

    uint16_t textureDimension = 2048;
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_userHandle(nullptr),
    m_userAddInProgress(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(c_backgroundColor);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);

    XUserCloseHandle(m_userHandle);
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

    // Fonts are mapped to specific unicode ranges in the StringShaper. Font mapping covers some key issues.
    // 1.   If the String was composed of multiple languages, it is probable that the characters will not all be
    //      available in one font. In that case, you would have to describe what font file can display that missing unicode codepoint.
    //
    // 2.   Many fonts support the same codepoints such as alphanumeric numbers "1,2,3,4,5,6,6,7,8,9,0". If you have a multi language String
    //      with alphanumeric numbers, you end up in a situation where there are MULTIPLE ways to draw those numbers depending on the font file. By
    //      assigning only ONE font file to alphanumeric, drawing of certain characters become consistent.
    //
    // 3.   In the case of modenr gamertags, not all characters in a specific language will be avialabel for that langauge. A UnicodeRange can be modified
    //      to remove unsupported characters for a specific language.

    m_stringShaper = std::make_unique<StringShaper>();
    m_stringShaper->MapFontToUnicodeRange(notoSansRegular, { UnicodeRangeInfo::LatinSymbols, UnicodeRangeInfo::LatinSupplement, UnicodeRangeInfo::LatinAlphaNumeric });
    m_stringShaper->MapFontToUnicodeRange(notoSansJp, { UnicodeRangeInfo::Japanese });
    m_stringShaper->MapFontToUnicodeRange(notoSansKor, { UnicodeRangeInfo::Korean });
    m_stringShaper->MapFontToUnicodeRange(notoSansArabic, { UnicodeRangeInfo::Arabic });


    m_stringTextureAtlas = std::make_unique<StringTextureAtlas>(textureDimension, m_deviceResources->GetD3DDevice());
    m_stringRenderer = std::make_unique<StringRenderer>(m_stringShaper.get(), m_stringTextureAtlas.get(), m_deviceResources->GetD3DDevice());

    m_stringRenderer->CreateDeviceDependentResources(m_deviceResources->GetCommandQueue(), m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    m_stringRenderer->CreateWindowSizeDependentResources(m_deviceResources->GetScreenViewport());

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue));

    GetUserHandle(XUserAddOptions::AllowGuests);
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
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        if (pad.IsXPressed())
        {
            GetUserHandle(XUserAddOptions::AllowGuests);
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

    if (kb.X)
    {
        GetUserHandle(XUserAddOptions::AllowGuests);
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
    m_deviceResources->Prepare();
    Clear();
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_stringRenderer->QueuePendingAtlasUpdates(commandList);

    m_stringRenderer->DrawString(u8"Unique Modern gamertag:", 35, 10, 120);
    m_stringRenderer->DrawString(u8"Modern Gamertag:", 35, 10, 190);
    m_stringRenderer->DrawString(u8"Classic Gamertag:", 35, 10, 260);
    m_stringRenderer->DrawString(u8"Suffix:", 35, 10, 330);
    m_stringRenderer->DrawString(u8"Press [X] on the controller or keyboard to switch users.", 25, 50, 530);

    if (m_SetDataDisplayable)
    {
        m_stringRenderer->DrawString(m_suggestedRendering.c_str(), 35, 475, 120);
        m_stringRenderer->DrawString(m_modernGamerTag.c_str(), 35, 475, 190);
        m_stringRenderer->DrawString(m_classicGamerTag.c_str(), 35, 475, 260);
        m_stringRenderer->DrawString(m_suffix.c_str(), 35, 475, 330);
    }
    else
    {
        // The strings in the example gamertag are using english, korean, and japanese.
        m_stringRenderer->DrawString(u8"Example: エ微GĂM3Rツ안#3279", 35, 475, 120);
        m_stringRenderer->DrawString(u8"Example: エ微GĂM3Rツ안", 35, 475, 190);
        m_stringRenderer->DrawString(u8"Example: 2Player29513279", 35, 475, 260);
        m_stringRenderer->DrawString(u8"Example: 3279", 35, 475, 330);
    }

    m_stringRenderer->Render(commandList);
    PIXEndEvent(commandList);

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
    commandList->ClearRenderTargetView(rtvDescriptor, c_backgroundColor, 0, nullptr);
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
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t>(size.right), static_cast<uint32_t>(size.bottom));
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region UserManagement
HRESULT Sample::GetUserHandle(XUserAddOptions userAddOption)
{
    if (m_userAddInProgress)
    {
        return S_FALSE;
    }

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = static_cast<Sample*>(asyncBlock->context);

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_userHandle)
            {
                XUserCloseHandle(pThis->m_userHandle);
            }
            pThis->m_userHandle = newUser;
            pThis->UpdateUserUIData();
            LogUtil::Log("Successfully obtained User Handle");
        }
        else
        {
            LogUtil::LogFailedHR("XUserAddResult", hr);
        }

        pThis->m_userAddInProgress = false;
        delete asyncBlock;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock);
    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
    }
    else
    {
        LogUtil::LogFailedHR("XUserAddAsync", hr);
        delete asyncBlock;
    }

    return hr;
}


HRESULT Sample::UpdateUserUIData()
{
    //Reset UI
    char uniqueModernBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1] = {};
    char modernSuffixBuffer[XUserGamertagComponentModernSuffixMaxBytes + 1] = {};
    char modernBuffer[XUserGamertagComponentModernMaxBytes + 1] = {};
    char classicBuffer[XUserGamertagComponentClassicMaxBytes + 1] = {};

    size_t uniqueModernSize = 0;
    size_t modernSuffixSize = 0;
    size_t modernSize = 0;
    size_t classicSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::UniqueModern, sizeof(uniqueModernBuffer), uniqueModernBuffer, &uniqueModernSize));
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::ModernSuffix, sizeof(modernSuffixBuffer), modernSuffixBuffer, &modernSuffixSize));
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::Modern, sizeof(modernBuffer), modernBuffer, &modernSize));
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::Classic, sizeof(classicBuffer), classicBuffer, &classicSize));

    m_suggestedRendering = uniqueModernBuffer;
    m_suffix = modernSuffixBuffer;
    m_modernGamerTag = modernBuffer;
    m_classicGamerTag = classicBuffer;

    m_SetDataDisplayable = true;

    return S_OK;
}

void Sample::SetDataDisplayable(bool value)
{
    if (value != m_SetDataDisplayable)
    {
        m_SetDataDisplayable = value;
    }
}
#pragma endregion
