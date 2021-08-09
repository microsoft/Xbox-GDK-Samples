// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "..\DeviceResources.h"
#include "..\StepTimer.h"
#include "GameScreen.h"
#include "..\Common\InputState.h"
#include "..\Assets.h"
#include <memory>
#include <vector>
#include <string>
#include <CommonStates.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>

namespace GameSaveSample
{
   class ScreenManager
   {
   public:
      ScreenManager();

      void CreateDeviceDependentResources( const std::shared_ptr<DX::DeviceResources>& deviceResources );
      void CreateWindowSizeDependentResources();
      void OnDeviceLost();

      // Call ExitScreen() on all screens, from topmost down, to gracefully shut them all down in preparation for a game state reset
      void ExitAllScreens();

      // Add a new screen to the manager with the specified controlling player. Pass -1 for controllingPlayer to enable all game pads to control the screen.
      void AddScreen( const std::shared_ptr<GameScreen>& screen, int controllingPlayer );

      // Removes a screen from the manager. You should normally use GameScreen::ExitScreen instead of calling this directly so the screen can gradually
      // transition off rather than just being instantly removed.
      void RemoveScreen( const std::shared_ptr<GameScreen>& screen );

      // Update controlling player in all screens where this value is not -1
      void UpdateControllingPlayer( int newControllingPlayer );

      // Updates all active screens in the ScreenManager
      void Update( DX::StepTimer const& timer );

      // Draws all active screens in the ScreenManager
      void Render( ID3D12GraphicsCommandList* commandList, DX::StepTimer const& timer );

      inline std::shared_ptr<DirectX::DescriptorHeap> GetDescriptorHeap() const { return m_resourceDescriptors; }
      inline std::shared_ptr<DirectX::CommonStates> GetCommonStates() const { return m_commonStates; }
      inline ATG::AssetRef<DirectX::SpriteFont> GetDebugFont() const { return m_debugFont; }
      inline std::shared_ptr<DirectX::SpriteBatch> GetSpriteBatch() const { return m_spriteBatch; }
      inline ATG::AssetRef<DirectX::SpriteFont> GetSpriteFont() const { return m_spriteFont; }
       inline std::shared_ptr<DX::DeviceResources> GetDeviceResources() const noexcept { return m_deviceResources; }
      inline RECT GetScreenBounds() const { return { 0, 0, 1920, 1080 }; } // the local size of all the screens
      inline RECT GetWindowBounds() const { return m_deviceResources->GetOutputSize(); } // the current window size

   private:
      // Used by GameScreen base class to remove itself from the manager
      void RemoveScreen( GameScreen* screen );

      // Cached pointer to device resources
      std::shared_ptr<DX::DeviceResources> m_deviceResources;

      // Stack of the current screens
      std::vector<std::shared_ptr<GameScreen> > m_screens;

      // Typedef the iterators for our screens collection
      typedef std::vector<std::shared_ptr<GameScreen> >::iterator ScreensIterator;
      typedef std::vector<std::shared_ptr<GameScreen> >::reverse_iterator ReverseScreensIterator;

      bool m_screenStackCleared;

      // Toggle to display state debugging info
      bool m_isDebugOverlayOn;

      // Some resources that are shared to all the screens
      std::shared_ptr<DirectX::CommonStates> m_commonStates;
      std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch;
      std::shared_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;

      ATG::AssetRef<DirectX::SpriteFont> m_spriteFont;
      ATG::AssetRef<DirectX::SpriteFont> m_debugFont;

      std::recursive_mutex m_screenLock;

      friend class GameScreen;
   };
}
