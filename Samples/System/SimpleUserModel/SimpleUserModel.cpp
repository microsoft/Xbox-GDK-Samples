//--------------------------------------------------------------------------------------
// SimpleUserModel.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleUserModel.h"

#include "ATGColors.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue);

    // [Simple User Model]
    // Register for any change events for users.
    // Change events are triggered when:
    //  - A user's sign-in status changes
    //  - A user's Gamertag, GamerPicture, or Privileges changed
    XUserRegisterForChangeEvent(
        m_taskQueue,
        this,
        &UserChangeEventCallback,
        &m_userChangeEventCallbackToken
    );

    // [Simple User Model]
    // Register for any change events for user-device associations.
    // Change events are triggered when:
    //  - A user signs out from the console.
    //  - Someone uses the input device to select a different user in the Account Picker.
    XUserRegisterForDeviceAssociationChanged(
        m_taskQueue,
        this,
        &UserDeviceAssociationChangedCallback,
        &m_userDeviceAssociationChangedCallbackToken
    );
}

Sample::~Sample()
{
    XUserUnregisterForChangeEvent(m_userChangeEventCallbackToken, false);
    XUserUnregisterForDeviceAssociationChanged(m_userDeviceAssociationChangedCallbackToken, false);

    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);
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

    InitializeUI();

    SignInDefaultUser();
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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0);

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

    m_inputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_inputState);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
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
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}
#pragma endregion

// [Simple User Model]
// This method signs in the default user that launched the application
void Sample::SignInDefaultUser()
{
    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto pThis = static_cast<Sample*>(asyncBlock->context);

        // [Simple User Model]
        // XUserAddResult is guaranteed to return S_OK when signing in the default user silently with
        // "XUserAddOptions::AddDefaultUserSilently".
        DX::ThrowIfFailed(XUserAddResult(asyncBlock, &pThis->m_user));
        DX::ThrowIfFailed(XUserGetLocalId(pThis->m_user, &pThis->m_userLocalId));
        pThis->UpdateUserUIData();

        delete asyncBlock;
    };

    // [Simple User Model]
    // The default user that launched the application can be added silently. When using the simple user model,
    // this is guaranteed to succeed in acquiring the user later with XUserAddResult.
    DX::ThrowIfFailed(XUserAddAsync(XUserAddOptions::AddDefaultUserSilently, asyncBlock));
}

void Sample::InitializeUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_gamertagText = layout->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));
    m_gamerpicImage = layout->GetTypedChildById<UIImage>(ID("Gamerpic"));
}

// [Simple User Model]
// This method updates the user's Gamertag and GamerPicture data that's rendered to the screen.
void Sample::UpdateUserUIData()
{
    // Set gamertag
    char gamertagBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1] = {};
    size_t gamertagSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_user, XUserGamertagComponent::UniqueModern, sizeof(gamertagBuffer), gamertagBuffer, &gamertagSize));
    m_gamertagText->SetDisplayText(gamertagBuffer);

    // Setup gamerpic request
    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto pThis = static_cast<Sample*>(asyncBlock->context);

        // Get buffer size
        size_t bufferSize = 0;
        DX::ThrowIfFailed(XUserGetGamerPictureResultSize(asyncBlock, &bufferSize));

        // Get buffer data
        std::vector<uint8_t> buffer;
        buffer.resize(bufferSize);
        size_t bufferUsed = 0;
        DX::ThrowIfFailed(XUserGetGamerPictureResult(asyncBlock, bufferSize, buffer.data(), &bufferUsed));

        // Set in UI
        pThis->m_gamerpicImage->UseTextureData(buffer.data(), buffer.size());

        delete asyncBlock;
    };

    // Request gamerpic
    DX::ThrowIfFailed(XUserGetGamerPictureAsync(m_user, XUserGamerPictureSize::Medium, asyncBlock));
}

// [Simple User Model]
// This callback is called whenever a user event happens. It was registered with XUserRegisterForChangeEvent in the Sample constructor.
void CALLBACK Sample::UserChangeEventCallback(
    _In_opt_ void* context,
    _In_ XUserLocalId userLocalId,
    _In_ XUserChangeEvent event
)
{
    // Log the callback
    {
        char debugString[512] = {};
        sprintf_s(debugString, 512, u8"UserChangeEventCallback() : userLocalId = 0x%llx, event = %d\n",
            userLocalId.value,
            event
        );
        OutputDebugStringA(debugString);
    }

    // Only handle events for the default user
    auto pThis = static_cast<Sample*>(context);
    if (userLocalId.value != pThis->m_userLocalId.value)
    {
        return;
    }

    // [Simple User Model]
    // In the simple user model, these Sign-in/Sign-out events are not intended to be received for the default user by design.
    //
    // Instead, the title is suspended if the user ever signs out. If a different user were to then attempt to
    // start the title, the title is terminated and re-started with the new default user instead of requiring handling
    // based on these events.
    //
    // However, note that they can be caused on a devkit when manually resuming using DevHome or other PLM testing tools.
    // These will not be received in retail or if testing using the standard Xbox user's home screens to launch.
    //
    // See the readme for more in-depth information.
    if (event == XUserChangeEvent::SignedInAgain ||
        event == XUserChangeEvent::SignedOut ||
        event == XUserChangeEvent::SigningOut)
    {
        throw std::runtime_error("Got an unexpected user change event that shouldn't happen with the simple user model.");
    }

    // Update user data if it changed
    if (event == XUserChangeEvent::GamerPicture ||
        event == XUserChangeEvent::Gamertag)
    {
        pThis->UpdateUserUIData();
    }
}

// [Simple User Model]
// This callback is called whenever a user-device association changes. It was registered with XUserRegisterForDeviceAssociationChanged in the Sample constructor.
void CALLBACK Sample::UserDeviceAssociationChangedCallback(
    _In_opt_ void* context,
    _In_ const XUserDeviceAssociationChange* change
)
{
    // Log the callback
    {
        char debugString[512] = {};
        sprintf_s(debugString, 512, u8"UserDeviceAssociationChangedCallback() : deviceId = %08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x, oldUser = 0x%llx, newUser = 0x%llx\n",
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[0]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[4]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[8]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[12]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[16]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[20]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[24]),
            *reinterpret_cast<const unsigned int*>(&change->deviceId.value[28]),
            change->oldUser.value,
            change->newUser.value
        );
        OutputDebugStringA(debugString);
    }

    auto pThis = static_cast<Sample*>(context);
    // If default user doesn't have a controller attached, display the UI to assign a controller
    if (change->oldUser.value == pThis->m_userLocalId.value){
        LoopFindControllerForUserWithUiAsync(pThis);
    }
}

void Sample::LoopFindControllerForUserWithUiAsync(_In_ void* context)
{
    auto pThis = static_cast<Sample*>(context);

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = pThis->m_taskQueue;
    asyncBlock->context = pThis;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        APP_LOCAL_DEVICE_ID deviceId = {};
        if (SUCCEEDED(XUserFindControllerForUserWithUiResult(asyncBlock, &deviceId)))
        {
            // Log the newly associated deviceId.
            char debugString[512] = {};
            sprintf_s(debugString, 512, u8"Found deviceId:  deviceId = %08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x\n",
                *reinterpret_cast<const unsigned int*>(&deviceId.value[0]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[4]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[8]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[12]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[16]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[20]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[24]),
                *reinterpret_cast<const unsigned int*>(&deviceId.value[28])
            );
            OutputDebugStringA(debugString);

            delete asyncBlock;
        }
        else
        {
            // Keep trying until the default user is assigned to a controller.
            LoopFindControllerForUserWithUiAsync(asyncBlock->context);
        }
    };

    XUserFindControllerForUserWithUiAsync(pThis->m_user, asyncBlock);
}
