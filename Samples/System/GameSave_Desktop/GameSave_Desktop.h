//--------------------------------------------------------------------------------------
// GameSave_Desktop.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "Helpers\XTaskQueueHandleWrapper.h"
#include "GameLogic\GameSaveManager.h"
#include "Common\InputState.h"
#include "GameLogic\ScreenManager.h"
#include "GameLogic\StateManager.h"

namespace GameSaveSample
{
    // PIX event colors
    const UINT EVT_COLOR_UPDATE = PIX_COLOR_INDEX(1);
    const UINT EVT_COLOR_CLEAR = PIX_COLOR_INDEX(2);
    const UINT EVT_COLOR_RENDER = PIX_COLOR_INDEX(3);
    const UINT EVT_COLOR_PRESENT = PIX_COLOR_INDEX(4);

    // A basic sample implementation that creates a D3D12 device and
    // provides a render loop.
    class Sample final : public DX::IDeviceNotify
    {
    public:

        Sample() noexcept(false);
        ~Sample();

        // Initialization and management
        void Initialize(HWND window, int width, int height);

        // Basic render loop
        void Tick();

        // IDeviceNotify
        void OnDeviceLost() override;
        void OnDeviceRestored() override;

        // Messages
        void OnActivated();
        void OnDeactivated();
        void OnSuspending();
        void OnResuming();
        void OnWindowMoved();
        void OnWindowSizeChanged(int width, int height);
        void OnKeyUp(WPARAM wParam, LPARAM lParam);
        void OnKeyDown(WPARAM wParam, LPARAM lParam);

        // Properties
        void GetDefaultSize(int& width, int& height) const;

    private:

        void Update(DX::StepTimer const& timer);
        void Render();

        void Clear();

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();

        // Device resources.
        std::shared_ptr<DX::DeviceResources>        m_deviceResources;

        // Rendering loop timer.
        DX::StepTimer                               m_timer;

        // Input Manager
        DirectX::InputState                         m_inputManager;

        // Screen Manager (updates and renders all game screens)
        ScreenManager                               m_screenManager;

        // State Manager
        StateManager                                m_stateManager;

        // DirectXTK objects.
        std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

        // An async task queue for the sample which will post tasks and completions on the thread pool.
        ATG::TaskQueueHandle                        m_asyncQueue;

        // Game Save Manager
        GameSaveManager                             m_gameSaveManager;

    public:

        // Unwrapped task - initializes the game save system for the current user. (A task wrapper calls this from
        // StateManager::InitializeGameSaveSystem).
        HRESULT InitializeGameSaveSystemTask();

        StateManager& GetStateManager() noexcept
        {
            return m_stateManager;
        }

        GameSaveManager& GetGameSaveManager()
        {
            return m_gameSaveManager;
        }

        ATG::TaskQueueHandle& GetGeneralThreadPoolTaskQueue()
        {
            return m_asyncQueue;
        }

        DirectX::InputState& GetInputManager()
        {
            return m_inputManager;
        }
    };
}

extern GameSaveSample::Sample* g_Game;
