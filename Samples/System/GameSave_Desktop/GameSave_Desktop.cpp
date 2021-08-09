//--------------------------------------------------------------------------------------
// GameSave_Desktop.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameSave_Desktop.h"

#include "Helpers\TaskQueue.h"
#include <XUser.h>
#include "Helpers\HandleWrapperBase.h"
#include "Helpers\XUserHandleWrapper.h"
#include "Helpers\User.h"
#include "Helpers\UserManager.h"
#include "GameLogic\GameSave.h"

#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;
GameSaveSample::Sample* g_Game = nullptr;

const char* SCID = "00000000-0000-0000-0000-00007E750470";

namespace GameSaveSample
{
    // Loads and initializes application assets when the application is loaded.


    Sample::Sample() noexcept(false) :
        m_stateManager(m_screenManager),
        m_gameSaveManager(SCID)
    {
        g_Game = this;

        srand(static_cast<unsigned int>(time(nullptr)));

        // Renders only 2D, so no need for a depth buffer.
        m_deviceResources = std::make_shared<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
        m_deviceResources->RegisterDeviceNotify(this);
    }

    Sample::~Sample()
    {
        if (m_deviceResources)
        {
            m_deviceResources->WaitForGpu();
        }

        ATG::ShutdownUserManager();
        ATG::ShutdownThreadQueues();
    }

    // Initialize the Direct3D resources required to run.
    void Sample::Initialize(HWND window, int width, int height)
    {
        // NOTE: When running the app from the Start Menu (required for
        //	XGameSave API's to work) the Current Working Directory will be
        //	returned as C:\Windows\system32 unless you overwrite it.
        //	The sample relies on the font and image files in the .exe's
        //	directory and so we do the following to set the working
        //	directory to what we want.
        char dir[_MAX_PATH] = {};
        (void)GetModuleFileNameA(nullptr, dir, _MAX_PATH);
        std::string exePath = dir;
        exePath = exePath.substr(0, exePath.find_last_of("\\"));
        SetCurrentDirectoryA(exePath.c_str());

        m_deviceResources->SetWindow(window, width, height);

        m_deviceResources->CreateDeviceResources();  	
        CreateDeviceDependentResources();

        m_deviceResources->CreateWindowSizeDependentResources();
        CreateWindowSizeDependentResources();

        // Create our async task queue which posts tasks to the thread pool.
        assert(!m_asyncQueue.IsValid() && "m_asyncQueue should not be valid");
        m_asyncQueue = ATG::TaskQueueHandle::Create(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::ThreadPool);

        // Create per-core thread queues.
        ATG::InitializeThreadQueues();

        // The default core you should use for input device mgr work.
        constexpr uint16_t DEFAULT_INPUT_WORK_AND_CALLBACK_CORE = 3;

        XTaskQueueHandle userAndInputOpQueue = ATG::GetThreadQueueForCore(DEFAULT_INPUT_WORK_AND_CALLBACK_CORE);

        // Configure Single user manager to log in the default user silently on first tick (if it can).
        ATG::InitializeUserManager(userAndInputOpQueue, XUserAddOptions::AddDefaultUserSilently);

        std::function<ATG::UserManager::UserSigningOutCallbackFn> signingOutCallback =
            std::bind(&StateManager::OnSignOutStarted, &m_stateManager, std::placeholders::_1);
        ATG::UserManager::SubscribeUserSigningOutEvent(signingOutCallback);
    }

#pragma region Frame Update
    // Executes basic render loop.
    void Sample::Tick()
    {
        m_timer.Tick([&]()
        {
            Update(m_timer);
        });

        Render();
    }

    // Updates the world.
    void Sample::Update(DX::StepTimer const& timer)
    {
        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

        m_inputManager.Update();

        // Update scene objects
        ATG::UserManager::Tick();
        m_screenManager.Update(timer);
        m_stateManager.Update();

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

        m_screenManager.Render(commandList, m_timer);

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
        Log::Write("OnSuspending()\n");
        ATG::SuspendUserManager();
        m_stateManager.Suspend();
        m_inputManager.Suspend();
    }

    void Sample::OnResuming()
    {
        Log::Write("OnResuming()\n");
        m_inputManager.Resume();
        m_stateManager.Resume();

        m_timer.ResetElapsedTime();
        ATG::ResumeUserManager();
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

    void Sample::OnKeyUp(WPARAM wParam, LPARAM /*lParam*/)
    {
        int vk = static_cast<int>(wParam);
        m_inputManager.OnKeyUp(vk);
    }

    void Sample::OnKeyDown(WPARAM /*wParam*/, LPARAM /*lParam*/)
    {
    }

    void Sample::GetDefaultSize(int& width, int& height) const
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

        m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

        m_screenManager.CreateDeviceDependentResources(m_deviceResources);
    }

    // Allocate all memory resources that change on a window SizeChanged event.
    void Sample::CreateWindowSizeDependentResources()
    {
        m_screenManager.CreateWindowSizeDependentResources();
    }

    void Sample::OnDeviceLost()
    {
        m_screenManager.OnDeviceLost();

        m_graphicsMemory.reset();
    }

    void Sample::OnDeviceRestored()
    {
        CreateDeviceDependentResources();

        CreateWindowSizeDependentResources();
    }
#pragma endregion

    HRESULT Sample::InitializeGameSaveSystemTask()
    {
        auto currentUser = ATG::UserManager::GetCurrentUser();

        if (!currentUser || !currentUser->IsValid())
        {
            Log::WriteAndDisplay("ERROR initializing game save system: Xbox Live User not signed in\n");
            return E_FAIL;
        }
        else
        {
            return m_gameSaveManager.InitializeForUserTask(currentUser, false);
        }
    }

} // namespace GameSaveSample
