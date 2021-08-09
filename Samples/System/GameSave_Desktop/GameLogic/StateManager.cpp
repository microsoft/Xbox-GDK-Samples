// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"
#include "..\Helpers\User.h"
#include "StateManager.h"

#include "AcquireUserScreen.h"
#include "ConfirmPopUpScreen.h"
#include "ErrorPopUpScreen.h"
#include "GameBoardScreen.h"
#include "LaunchOptionsScreen.h"
#include "GameSave_Desktop.h"
#include "ScreenManager.h"
#include <memory>
#include "..\Helpers\AsyncOp.h"

namespace
{
   static bool s_autoSignIn = true;
}

namespace GameSaveSample {

   const char* StateToString( GameState state )
   {
      switch ( state )
      {
      case GameState::Reset:
         return u8"Reset";
      case GameState::Initialize:
         return u8"Initialize";
      case GameState::AcquireUser:
         return u8"AcquireUser";
      case GameState::InitializeGameSaveSystem:
         return u8"InitializeGameSaveSystem";
      case GameState::InGame:
         return u8"InGame";
      case GameState::Suspended:
         return u8"Suspended";
      case GameState::Resume:
         return u8"Resume";
      default:
         break;
      }
      return u8"(Unhandled State)";
   }

   StateManager::StateManager(ScreenManager& screenManager ) :
      m_screenManager( screenManager ),
      m_state( GameState::Reset ),
	  m_pendingState( GameState::Initialize ),
	  m_priorState( GameState::Reset )
   {
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  PopupError
   //
   void StateManager::PopupError( const char* errorMsg, bool revertToPriorState )
   {
      Log::WriteAndDisplay( u8"PopupError: %s\n", errorMsg );
      m_screenManager.AddScreen( std::make_shared<ErrorPopUpScreen>( m_screenManager, errorMsg ), -1 );

      if ( revertToPriorState )
      {
         RevertToPriorState();
      }
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  IsCurrentOrPendingState
   //      Return TRUE if the current state OR the pending state is equal to the supplied parameter
   //
   bool StateManager::IsCurrentOrPendingState( GameState state )
   {
      std::lock_guard<std::recursive_mutex> lock( m_lock );
      return ( m_state == state || m_pendingState == state );
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  RevertToPriorState
   //      Circumvents the normal SwitchState functionality - normally used to go back after an error or upon resuming
   //
   void StateManager::RevertToPriorState()
   {
      std::lock_guard<std::recursive_mutex> lock( m_lock );
      Log::Write( u8"RevertToPriorState %s -> %s\n", StateToString( m_state ), StateToString( m_priorState ) );
      m_pendingState = m_priorState;
      m_priorState = m_state;
      m_state = m_pendingState;
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  SwitchState
   //
   void StateManager::SwitchState( GameState newState )
   {
      std::lock_guard<std::recursive_mutex> lock( m_lock );

      if ( newState == GameState::Suspended )
      {
         // Process this state change immediately
         Log::Write( u8"SwitchState %s -> %s\n", StateToString( m_state ), StateToString( newState ) );
         m_priorState = m_state;
         m_state = GameState::Suspended;
         m_pendingState = GameState::Suspended;
         return;
      }

      if ( m_state == newState )
      {
         Log::Write( u8"WARNING: Attempted to switch to current state.\n" );
         return;
      }

      // Set the pending state so it will pick up on the next update tick
      m_pendingState = newState;
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  Suspend
   //
   void StateManager::Suspend()
   {
      SwitchState( GameState::Suspended );

      g_Game->GetGameSaveManager().Suspend();

      Log::WriteAndDisplay( u8"OnSuspending() complete\n" );
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  Resume
   //
   void StateManager::Resume()
   {
       Log::Write(u8"StateManager::Resume()\n");

       // Continue where the user left off
       RevertToPriorState();

       if (m_state == GameState::InitializeGameSaveSystem)
       {
           SwitchState(GameState::Reset);
       }

       // User management is now handled by various callbacks which will be handled during resuming

       g_Game->InitializeGameSaveSystemTask();
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  ResetGame
   //
   void StateManager::ResetGame()
   {
      Log::Write( u8"StateManager::ResetGame()\n" );

      m_screenManager.ExitAllScreens();

      g_Game->GetGameSaveManager().Reset();

      SwitchState( GameState::Initialize );
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  Update
   //
   void StateManager::Update()
   {
      auto priorState = GameState::Reset;
      auto state = GameState::Reset;
      auto pendingState = GameState::Reset;

      {
         std::lock_guard<std::recursive_mutex> lock( m_lock );
         priorState = m_priorState;
         state = m_state;
         pendingState = m_pendingState;
      }

      if ( pendingState != state )
      {
         // Perform a state switch
         {
            std::lock_guard<std::recursive_mutex> lock( m_lock );
            Log::Write( u8"SwitchState %s -> %s\n", StateToString( state ), StateToString( pendingState ) );
            m_priorState = state;
            state = m_state = pendingState;
         }

         switch ( state )
         {
         case GameState::Reset:
            ResetGame();
            break;
         case GameState::Initialize:
            if ( GameSaveManager::HasSyncOnDemandModeBeenSet() )
            {
               SwitchState( GameState::AcquireUser );
            }
            else
            {
               m_screenManager.AddScreen( std::make_shared<LaunchOptionsScreen>( m_screenManager ), -1 );
            }
            break;
         case GameState::AcquireUser:
            m_screenManager.AddScreen( std::make_shared<AcquireUserScreen>( m_screenManager, s_autoSignIn ), -1 );
            s_autoSignIn = false; // only the initial sign-in should be automatic
            break;
         case GameState::InitializeGameSaveSystem:
            InitializeGameSaveSystem();
            break;
         case GameState::InGame:
            m_screenManager.AddScreen( std::make_shared<GameBoardScreen>( m_screenManager ), -1/*g_Game->GetCurrentGamepadIndex()*/ );
            break;
		 case GameState::Suspended:
			 // No implementation.
			 break;
		 case GameState::Resume:
			 // No implementation.
			 break;
         default:
            break;
         }
      }
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  InitializeGameSaveSystem
   //
   ATG::AsyncAction<void>& StateManager::InitializeGameSaveSystem() noexcept
   {
      m_initSaveGameSystemTask.Reset();
      m_initSaveGameSystemTask.SetFunction(std::function<void()>(
         [this]() 
            {
               InitializeGameSaveSystemTask();
            }
         ));
      DX::ThrowIfFailed(m_initSaveGameSystemTask.Start(g_Game->GetGeneralThreadPoolTaskQueue(), 0));
      return m_initSaveGameSystemTask;
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  InitializeGameSaveSystem
   //
   void StateManager::InitializeGameSaveSystemTask() noexcept
   {
      HRESULT hr = g_Game->InitializeGameSaveSystemTask();
      if ( SUCCEEDED(hr) )
      {
         // make sure we haven't switched to a different state, e.g. suspending
         if (m_state == GameState::InitializeGameSaveSystem)
         {
            SwitchState(GameState::InGame);
         }
      }
      else if (hr == E_GAMEUSER_NO_PACKAGE_IDENTITY)
      {
          std::string error(u8"Failed to initialize game save system: E_GAMEUSER_NO_PACKAGE_IDENTITY. See readme.docx for more information.");
          PopupError(error.c_str());
          SwitchState(GameState::AcquireUser);
      }
      else
      {
         std::string error(u8"Failed to initialize game save system. hr=");
         error += FormatHResult(hr);
         PopupError(error.c_str());
         SwitchState(GameState::AcquireUser);
      }
   }

   void StateManager::OnSignOutStartedTaskBody()
   {
      HRESULT hr = g_Game->GetGameSaveManager().OnSignOutTaskBody();
      DX::ThrowIfFailed(hr);

      XUserCloseSignOutDeferralHandle(m_deferral);
      m_deferral = nullptr;

      SwitchState(GameState::Reset);
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  OnSignOutStarted
   //
   void StateManager::OnSignOutStarted(std::shared_ptr<ATG::User>& user)
   {
       Log::WriteAndDisplay(u8"OnSignOutStarted(%s)\n", FormatUserName((XUserHandle)*user).c_str());

       auto currentUser = ATG::UserManager::GetCurrentUser();

       Log::WriteAndDisplay(u8"  Current User: (%s)\n", FormatUserName((XUserHandle)*currentUser).c_str());

       if (currentUser && currentUser->GetLocalUserId() == user->GetLocalUserId())
       {
           if (m_state == GameState::InGame)
           {
               // Try to get a sign-out deferral. This can fail in rare circumstances, such as when the app was
               // suspended, user signs out, and the app is resumed. In that case, saving happened upon suspending.
               HRESULT hRes = XUserGetSignOutDeferral(&m_deferral);

               m_asyncSignOutTask.Reset();
               if (SUCCEEDED(hRes))
               {
                   m_asyncSignOutTask.SetFunction(std::function< void() >(
                       [this]()
                       {
                           OnSignOutStartedTaskBody();
                       }
                   ));
               }
               else
               {
                   m_asyncSignOutTask.SetFunction(std::function< void() >(
                       [this]()
                       {
                           SwitchState(GameState::Reset);
                       }
                   ));
               }

               DX::ThrowIfFailed(m_asyncSignOutTask.Start(g_Game->GetGeneralThreadPoolTaskQueue(), 0));
           }
           else
           {
               SwitchState(GameState::Reset);
           }
       }
   }

   ///////////////////////////////////////////////////////////////////////////////
   //
   //  OnUserChanged
   //
   void StateManager::OnUserChanged( std::shared_ptr<ATG::User>& oldUser, std::shared_ptr<ATG::User>& newUser )
   {
      Log::WriteAndDisplay( u8"OnUserChanged(%s -> %s)\n", 
         FormatUserName( (XUserHandle)*(oldUser) ).c_str(), 
         FormatUserName( (XUserHandle)*(newUser) ).c_str() );

      if ( m_state == GameState::InitializeGameSaveSystem || m_state == GameState::InGame )
      {
         SwitchState( GameState::Reset );
      }
   }

} // namespace GameSaveSample
