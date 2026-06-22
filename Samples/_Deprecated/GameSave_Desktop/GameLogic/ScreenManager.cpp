// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"
#include "..\Helpers\UTF8Helper.h"
#include "..\Assets.h"
#include "ScreenManager.h"

#include "..\Common\InputState.h"
#include "GameSave_Desktop.h"

#include <CommonStates.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <algorithm>

using namespace DirectX;

//using namespace Windows::Xbox::Input;

namespace GameSaveSample {

   ScreenManager::ScreenManager() :
       m_screenStackCleared(false),
       m_isDebugOverlayOn( false )
   {
   }

   void ScreenManager::CreateDeviceDependentResources( const std::shared_ptr<DX::DeviceResources>& deviceResources )
   {
      m_deviceResources = deviceResources;
      auto device = m_deviceResources->GetD3DDevice();

      m_resourceDescriptors = std::make_shared<DirectX::DescriptorHeap>(
         deviceResources->GetD3DDevice(),
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
         D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
         (size_t)AssetDescriptor::DESCRIPTOR_COUNT
         );

      ATG::SetAssetDescriptorHeap( m_resourceDescriptors.get() );

      RenderTargetState rtState( m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat() );
      SpriteBatchPipelineStateDescription pd( rtState, &CommonStates::NonPremultiplied );
      ResourceUploadBatch upload( device );
      upload.Begin();

      ATG::AssetLoadResources alr;
      alr.device = device;
      alr.upload = &upload;
      
      m_commonStates = std::make_shared<CommonStates>( device );
      m_spriteBatch = std::make_shared<SpriteBatch>( device, upload, pd );

      m_spriteFont =ATG::GetSpriteFontAsset( alr, AssetDescriptor::SegoeUI24Font );
      m_spriteFont->SetDefaultCharacter( L'*' );

      m_debugFont = ATG::GetSpriteFontAsset( alr, AssetDescriptor::Consolas12Font );
      m_debugFont->SetDefaultCharacter( L'*' );

      auto finish = upload.End( m_deviceResources->GetCommandQueue() );
      finish.wait();
   }

   void ScreenManager::CreateWindowSizeDependentResources()
   {
   }

   void ScreenManager::OnDeviceLost()
   {
       m_commonStates.reset();
       m_spriteBatch.reset();
       m_resourceDescriptors.reset();
       m_spriteFont.Unload();
       m_debugFont.Unload();
   }

   void ScreenManager::ExitAllScreens()
   {
      while ( m_screens.size() > 0 )
      {
         auto screen = m_screens.back();
         screen->ExitScreen( true );
      }
      m_screenStackCleared = true;
   }

   void ScreenManager::AddScreen( const std::shared_ptr<GameScreen>& screen, int controllingPlayer )
   {
      std::lock_guard<std::recursive_mutex> lock( m_screenLock );

      screen->m_controllingPlayer = controllingPlayer;
      screen->m_isExiting = false;

      if ( !screen->IsPopup() && m_screens.size() > 0 )
      {
         // insert the new screen underneath any existing popup screens
         ScreensIterator it = m_screens.begin();
         for ( ; it != m_screens.end(); ++it )
         {
            if ( ( *it )->IsPopup() )
               break;
         }
         m_screens.insert( it, screen );
      }
      else
      {
         // otherwise just tack on the new screen at the end (top) of the list
         m_screens.push_back( screen );
      }

      auto device = m_deviceResources->GetD3DDevice();
      ResourceUploadBatch upload( device );
      upload.Begin();
      ATG::AssetLoadResources loadRes;
      loadRes.device = device;
      loadRes.upload = &upload;

      // Allow the screen to load content now that it's being added to the screen manager
      screen->LoadContent( loadRes );

      upload.End( m_deviceResources->GetCommandQueue() );
      //m_deviceResources->WaitForGpu();
   }

   void ScreenManager::RemoveScreen( const std::shared_ptr<GameScreen>& screen )
   {
      // Shell out to our private helper since it's going to do the same thing
      RemoveScreen( screen.get() );
   }

   void ScreenManager::UpdateControllingPlayer( int newControllingPlayer )
   {
      std::lock_guard<std::recursive_mutex> lock( m_screenLock );

      std::for_each( m_screens.begin(), m_screens.end(), [ & ] ( const std::shared_ptr<GameScreen>& screen )
      {
         if ( screen->m_controllingPlayer != -1 )
            screen->UpdateControllingPlayer( newControllingPlayer );
      } );
   }

   void ScreenManager::Update( DX::StepTimer const& timer )
   {
      float totalTime = float( timer.GetTotalSeconds() );
      float elapsedTime = float( timer.GetElapsedSeconds() );

      if ( g_Game->GetInputManager().IsNewKeyPress( Keyboard::Keys::OemTilde ) )
      {
         m_isDebugOverlayOn = !m_isDebugOverlayOn;
      }

      // Make a copy of the master screen list, to avoid confusion if the process of updating one screen adds or removes others
      std::vector<std::shared_ptr<GameScreen>> screensToUpdate;
      {
         std::lock_guard<std::recursive_mutex> lock( m_screenLock );
         screensToUpdate = m_screens;
      }

      bool otherScreenHasFocus = false;
      bool coveredByOtherScreen = false; // used to tell non-popup screens that they are covered by another non-popup screen
      bool coveredByPopup = false; // used to tell other popup screens that they are covered by a popup above them
      m_screenStackCleared = false;

      // Iterate the screens in reverse order so the last screen added is considered the top of the "stack"
      for ( ReverseScreensIterator itr = screensToUpdate.rbegin(); itr != screensToUpdate.rend(); itr++ )
      {
         std::shared_ptr<GameScreen> screen = ( *itr );
         bool isPopup = screen->m_isPopup;

         // Update the screen
         screen->Update( totalTime, elapsedTime, otherScreenHasFocus, isPopup ? coveredByPopup : coveredByOtherScreen );

         // Update this value for the next screen
         coveredByPopup = isPopup;

         if ( screen->m_state == ScreenState::TransitionOn || screen->m_state == ScreenState::Active )
         {
            // If this is the first active screen we came across, give it a chance to handle input
            if ( !otherScreenHasFocus )
            {
               screen->HandleInput( g_Game->GetInputManager() );
               otherScreenHasFocus = true;
            }

            // If this is an active non-popup, inform any subsequent screens that they are covered by it
            if ( !isPopup )
            {
               coveredByOtherScreen = true;
            }
         }

         if ( m_screenStackCleared )
         {
            // ExitAllScreens() was called by the current screen so don't process any more screens
            break;
         }
      }

      // If the background screen needed updating, we would do it here. So far, the Update() is unnecessary.
   }

   void ScreenManager::Render( ID3D12GraphicsCommandList* commandList, DX::StepTimer const& timer )
   {
      float totalTime = float( timer.GetTotalSeconds() );
      float elapsedTime = float( timer.GetElapsedSeconds() );

      // Make a copy of the master screen list so we can loop through and draw the screens even if another thread alters the collection during the draw
      std::vector<std::shared_ptr<GameScreen>> screensToRender;
      {
         std::lock_guard<std::recursive_mutex> lock( m_screenLock );
         screensToRender = m_screens;
      }

      std::for_each( screensToRender.begin(), screensToRender.end(), [ & ] ( const std::shared_ptr<GameScreen>& screen )
      {
         if ( screen->m_state != ScreenState::Hidden )
            screen->Draw( commandList, totalTime, elapsedTime );
      } );

      if ( m_isDebugOverlayOn )
      {
         auto viewportSize = GetScreenBounds();
         auto scaleMatrix = ATG::GetScaleMatrixForWindow( GetWindowBounds() );
         m_spriteBatch->Begin( commandList, SpriteSortMode_Deferred, scaleMatrix );

         float viewportWidth = float( viewportSize.right );
         float viewportHeight = float( viewportSize.bottom );

         // draw current state at the top left of the screen
         XMFLOAT2 position = XMFLOAT2( viewportWidth * 0.05f, viewportHeight * 0.05f );
         XMFLOAT2 origin = XMFLOAT2( 0, m_spriteFont->GetLineSpacing() / 2.0f );

         const char* state = StateToString( g_Game->GetStateManager().GetState() );
         m_spriteFont->DrawString( m_spriteBatch.get(), state, position, Colors::White, 0, origin );

         // draw game save index update count at the top middle of the screen
         position = XMFLOAT2( viewportWidth * 0.5f, viewportHeight * 0.05f );

         char* updateMsg = ATG::Text::FormatStringScratch( u8"Index Update #%u", g_Game->GetGameSaveManager().GetIndexUpdateCount() );

         auto countDrawWidth = m_spriteFont->MeasureString( updateMsg );
         origin = XMFLOAT2( XMVectorGetX( countDrawWidth ) / 2.0f, m_spriteFont->GetLineSpacing() / 2.0f );
         m_spriteFont->DrawString( m_spriteBatch.get(), updateMsg, position, Colors::White, 0, origin );

         // draw FPS at the top right of the screen
         char* fpsMsg = ATG::Text::FormatStringScratch( "%u FPS", timer.GetFramesPerSecond() );

         position = XMFLOAT2( viewportWidth * 0.95f, viewportHeight * 0.05f );
         auto fpsDrawWidth = m_spriteFont->MeasureString( fpsMsg );
         origin = XMFLOAT2( XMVectorGetX( fpsDrawWidth ), m_spriteFont->GetLineSpacing() / 2.0f );
         m_spriteFont->DrawString( m_spriteBatch.get(), fpsMsg, position, Colors::White, 0, origin );

         m_spriteBatch->End();
      }
   }

   void ScreenManager::RemoveScreen( GameScreen* screen )
   {
      std::lock_guard<std::recursive_mutex> lock( m_screenLock );

      // Find the screen in our collection
      for ( ScreensIterator itr = m_screens.begin(); itr != m_screens.end(); ++itr )
      {
         if ( ( *itr ).get() == screen )
         {
            // Let the screen unload any content if it wants
            screen->UnloadContent();

            // Remove it from the vector
            m_screens.erase( itr );
            return;
         }
      }
   }
} // namespace GameSaveSample
