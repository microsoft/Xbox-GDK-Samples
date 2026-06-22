//--------------------------------------------------------------------------------------
// SampleGame.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Helpers\TaskQueue.h"
#include <XUser.h>
#include "Helpers\HandleWrapperBase.h"
#include "Helpers\XUserHandleWrapper.h"
#include "Helpers\User.h"
#include "Helpers\UserManager.h"
#include "SampleGame.h"
#include "GameLogic\GameSave.h"

#include "ATGColors.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;
GameSaveSample::SampleGame* g_Game = nullptr;

const char* SCID = "00000000-0000-0000-0000-00007E750470";

namespace GameSaveSample {

   // Loads and initializes application assets when the application is loaded.


   SampleGame::SampleGame() noexcept( false ) :
      m_frame( 0 ),
      m_stateManager(m_screenManager),
      m_gamepadDevice(nullptr),
      m_gamepadIndex( -1 ),
      m_gameSaveManager( SCID )
   {
      g_Game = this;

      srand( static_cast<unsigned int>( time( nullptr ) ) );

      // Renders only 2D, so no need for a depth buffer.
      m_deviceResources = std::make_unique<DX::DeviceResources>( DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN );
   }

   SampleGame::~SampleGame()
   {
      ATG::ShutdownUserManager();
      ATG::ShutdownInputDeviceManager();
      ATG::ShutdownThreadQueues();
   }

   // Initialize the Direct3D resources required to run.
   void SampleGame::Initialize( HWND window )
   {
      ZeroMemory( &m_gamepadDevice, sizeof( APP_LOCAL_DEVICE_ID ) );

      m_deviceResources->SetWindow( window );

      m_deviceResources->CreateDeviceResources();
      CreateDeviceDependentResources();

      m_deviceResources->CreateWindowSizeDependentResources();
      CreateWindowSizeDependentResources();

      // Create our async task queue which posts tasks to the thread pool.
      assert(!m_asyncQueue.IsValid() && "m_asyncQueue should not be valid");
      m_asyncQueue = ATG::TaskQueueHandle::Create(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::ThreadPool );

      // Create per-core thread queues.
      ATG::InitializeThreadQueues();

      XTaskQueueHandle userAndInputOpQueue = ATG::GetThreadQueueForCore( ATG::DEFAULT_INPUT_WORK_AND_CALLBACK_CORE );
      
      // Configure Single user manager to log in the default user silently on first tick (if it can).
      ATG::InitializeUserManager( userAndInputOpQueue, XUserAddOptions::AddDefaultUserSilently );
      ATG::InitializeInputDeviceManager( userAndInputOpQueue );

      std::function<ATG::UserManager::UserSigningOutCallbackFn> signingOutCallback = 
            std::bind( &StateManager::OnSignOutStarted, &m_stateManager, std::placeholders::_1 );
      ATG::UserManager::SubscribeUserSigningOutEvent( signingOutCallback );
   }

#pragma region Frame Update
   // Executes basic render loop.
   void SampleGame::Tick()
   {
      PIXBeginEvent( PIX_COLOR_DEFAULT, L"Frame %llu", m_frame );

      m_timer.Tick( [ & ] ()
      {
         Update( m_timer );
      } );

      Render();

      PIXEndEvent();
      m_frame++;
   }

   // Updates the world.
   void SampleGame::Update( DX::StepTimer const& timer )
   {
      PIXScopedEvent( PIX_COLOR_DEFAULT, L"Update" );

      m_inputManager.Update();
      if ( m_inputManager.IsAnyGamepadConnected() )
      {
         if ( m_inputManager.IsNewButtonPress( GameInputGamepadView, ATG::GamePad::MERGED_CONTROLLER_INPUT_PLAYER_INDEX ) )
         {
            ExitSample();
         }
      }

      // Update scene objects
      ATG::UserManager::Tick();
      m_screenManager.Update( timer );
      m_stateManager.Update();
   }
#pragma endregion

#pragma region Frame Render
   // Draws the scene.
   void SampleGame::Render()
   {
      // Don't try to render anything before the first Update.
      if ( m_timer.GetFrameCount() == 0 )
      {
         return;
      }

      // Prepare the command list to render a new frame.
      m_deviceResources->Prepare();

      Clear();

      auto commandList = m_deviceResources->GetCommandList();
      PIXBeginEvent( commandList, EVT_COLOR_RENDER, L"Render" );

      m_screenManager.Render( commandList, m_timer );

      PIXEndEvent( commandList );

      // Show the new frame.
      PIXBeginEvent( EVT_COLOR_PRESENT, L"Present" );
      m_deviceResources->Present();
      m_graphicsMemory->Commit( m_deviceResources->GetCommandQueue() );
      PIXEndEvent();
   }

   // Helper method to clear the back buffers.
   void SampleGame::Clear()
   {
      auto commandList = m_deviceResources->GetCommandList();
      PIXBeginEvent( commandList, EVT_COLOR_CLEAR, L"Clear" );

      // Clear the views.
      auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

      commandList->OMSetRenderTargets( 1, &rtvDescriptor, FALSE, nullptr );
      commandList->ClearRenderTargetView( rtvDescriptor, Colors::Black, 0, nullptr );

      // Set the viewport and scissor rect.
      auto viewport = m_deviceResources->GetScreenViewport();
      auto scissorRect = m_deviceResources->GetScissorRect();
      commandList->RSSetViewports( 1, &viewport );
      commandList->RSSetScissorRects( 1, &scissorRect );

      PIXEndEvent( commandList );
   }
#pragma endregion

#pragma region Message Handlers
   // Message handlers
   void SampleGame::OnSuspending()
   {
      Log::Write( "OnSuspending()\n" );
      ATG::SuspendUserManager();
      m_deviceResources->Suspend();
      m_stateManager.Suspend();
      m_inputManager.Suspend();
   }

   void SampleGame::OnResuming()
   {
      Log::Write( "OnResuming()\n" );
      m_deviceResources->Resume();
      m_inputManager.Resume();
      m_stateManager.Resume();

      m_timer.ResetElapsedTime();
      ATG::ResumeUserManager();
   }
#pragma endregion

#pragma region Direct3D Resources
   // These are the resources that depend on the device.
   void SampleGame::CreateDeviceDependentResources()
   {
      auto device = m_deviceResources->GetD3DDevice();

      m_graphicsMemory = std::make_unique<GraphicsMemory>( device );

      m_screenManager.CreateDeviceDependentResources( m_deviceResources );
   }

   // Allocate all memory resources that change on a window SizeChanged event.
   void SampleGame::CreateWindowSizeDependentResources()
   {
      m_screenManager.CreateWindowSizeDependentResources();
   }
#pragma endregion

   constexpr void SampleGame::GetDefaultSize( int& width, int& height ) const
   {
      width = 1920;
      height = 1080;
   }

   HRESULT SampleGame::InitializeGameSaveSystemTask()
   {
      auto currentUser = ATG::UserManager::GetCurrentUser();

      if ( !currentUser || !currentUser->IsValid() )
      {
         Log::WriteAndDisplay("ERROR initializing game save system: Xbox Live User not signed in\n");
         return E_FAIL;
      }
      else
      {
         return m_gameSaveManager.InitializeForUserTask(currentUser, false);
      }
   }

   void SampleGame::SetCurrentGamepad( _In_opt_ IGameInputDevice* gamepad )
   {
      m_gamepadDevice = gamepad;
      m_gamepadIndex = ( gamepad == nullptr ) ? -1 : m_inputManager.GetGamepadIndex( gamepad );
      m_screenManager.UpdateControllingPlayer( m_gamepadIndex );
   }

   void SampleGame::SetCurrentGamepad( _In_ IGameInputDevice* gamepad, int index )
   {
      m_gamepadDevice = gamepad;
      m_gamepadIndex = index;
      m_screenManager.UpdateControllingPlayer( m_gamepadIndex );
   }

   bool SampleGame::UpdateCurrentGamepad()
   {
      auto user = ATG::UserManager::GetCurrentUser();
      if ( user )
      {
         ATG::GamePadUserBindingInfo pair;

         if ( ATG::FindGamePadAndUserByUserId( user->GetLocalUserId(), &pair ) )
         {
            SetCurrentGamepad( pair.gamePad.Get(), pair.gamePadIndex );
            Log::WriteAndDisplay( "Gamepad refreshed for current user (index = %d)\n", m_gamepadIndex );
            return true;
         }
      }

      // Didn't find a match.
      SetCurrentGamepad( nullptr, -1 );

      return false;
   }

} // namespace GameSaveSample

