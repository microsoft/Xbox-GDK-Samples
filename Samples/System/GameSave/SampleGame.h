//--------------------------------------------------------------------------------------
// SampleGame.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "Helpers\XTaskQueueHandleWrapper.h"
#include "DeviceResources.h"
#include "GameLogic\GameSaveManager.h"
#include "Common\InputState.h"
#include "GameLogic\ScreenManager.h"
#include "GameLogic\StateManager.h"
#include "StepTimer.h"

namespace GameSaveSample
{
   // PIX event colors
   const UINT EVT_COLOR_UPDATE = PIX_COLOR_INDEX( 1 );
   const UINT EVT_COLOR_CLEAR = PIX_COLOR_INDEX( 2 );
   const UINT EVT_COLOR_RENDER = PIX_COLOR_INDEX( 3 );
   const UINT EVT_COLOR_PRESENT = PIX_COLOR_INDEX( 4 );

   // A basic sample implementation that creates a D3D12 device and
   // provides a render loop.
   class SampleGame
   {
   public:

      SampleGame() noexcept( false );

      ~SampleGame();

      // Initialization and management
      void Initialize( HWND window );

      // Basic render loop
      void Tick();

      // Messages
      void OnSuspending();
      void OnResuming();

   private:
      void Update( DX::StepTimer const& timer );
      void Render();

      void Clear();

      void CreateDeviceDependentResources();
      void CreateWindowSizeDependentResources();

      // Device resources.
      std::shared_ptr<DX::DeviceResources>        m_deviceResources;

      // Rendering loop timer.
      uint64_t                                    m_frame;
      DX::StepTimer                               m_timer;
      
      // Input Manager
      DirectX::InputState m_inputManager;

      // Screen Manager (updates and renders all game screens)
      ScreenManager m_screenManager;

      // State Manager
      StateManager m_stateManager;

      // Gamepad and Gamepad Index
      IGameInputDevice* m_gamepadDevice;
      int m_gamepadIndex;
  
      // DirectXTK objects.
      std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

      // An async task queue for the sample which will post tasks and completions on the thread pool.
      ATG::TaskQueueHandle m_asyncQueue;

      // Game Save Manager
      GameSaveManager m_gameSaveManager;

   public:
      // Properties
      constexpr void GetDefaultSize( int& width, int& height ) const;

      // Unwrapped task - initializes the game save system for the current user. (A task wrapper calls this from
      // StateManager::InitializeGameSaveSystem).
      HRESULT InitializeGameSaveSystemTask();

      int GetCurrentGamepadIndex() {
         return m_gamepadIndex;
      }

      // Returns current gamepad paired to current user. NOTE: This is NOT refcounted - we don't own the reference.
      IGameInputDevice* GetCurrentGamepad() {
         return m_gamepadDevice;
      }

      // Returns true if a gamepad was found for Current User, or false if a gamepad could not be found for the Current User
      bool UpdateCurrentGamepad();

      // Sets current gamepad and gamepad index
      void SetCurrentGamepad( _In_ IGameInputDevice* gamepad );

      void SetCurrentGamepad( _In_ IGameInputDevice* gamepad, int index );

      StateManager& GetStateManager() noexcept
      {
         return m_stateManager;
      }

      GameSaveManager& GetGameSaveManager() { 
         return m_gameSaveManager; 
      }

      ATG::TaskQueueHandle& GetGeneralThreadPoolTaskQueue() { 
         return m_asyncQueue; 
      }

      DirectX::InputState& GetInputManager() { 
         return m_inputManager;  
      }

   };
}

extern GameSaveSample::SampleGame* g_Game;
