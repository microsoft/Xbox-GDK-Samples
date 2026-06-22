// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "MenuScreen.h"
#include "Texture.h"
#include <set>

namespace GameSaveSample
{
   using WordList = std::set< std::wstring >;

   class GameBoardScreen : public MenuScreen
   {
   public:
      GameBoardScreen( ScreenManager& screenManager );
      virtual ~GameBoardScreen();

      virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
      virtual void Update( float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen ) override;
      virtual void HandleInput( const DirectX::InputState& input ) override;
      virtual void Draw( ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime ) override;
      virtual void ExitScreen( bool immediate = false ) override;

   protected:
      virtual void OnCancel() override;
      virtual void ComputeMenuBounds( float viewportWidth, float viewportHeight ) override;

      void ReadBoardTask( uint32_t boardNumber, bool activeBoardNeedsSync, const std::string& activeBoardContainerName);
      void SaveBoardTask( uint32_t boardNumber, const std::string& activeBoardContainerName);
      void DeleteBoardTask( uint32_t boardNumber, const std::string& activeBoardContainerName);
      void DeleteBoardBlobTask( uint32_t boardNumber, const std::string& activeBoardContainerName );
      void ListContainersTask();
      void ListContainersAndBlobsTask();

   private:
      struct WordTrackerTile
      {
         WordTrackerTile() {}
         bool m_wordDown = false;
         bool m_wordRight = false;
      };

      void LoadWordListTask( const wchar_t* path );
      int GetWordScore( const std::wstring& word );
      WordTrackerTile& GetWordTrackerTile( const DirectX::XMUINT2& position ); // returns WordTrackerTile given a zero-based (x, y) position
      void TrackWord( bool isHorizontal, DirectX::XMUINT2 startTile, size_t wordLength );
      void UpdateLettersRemaining();
      void UpdateScore();

      //
      // Assets
      //

      // Fonts

      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardControlsHelpFont;
      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardLettersRemainingFont;
      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardMetadataFont;
      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardTileFont;
      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardTileValueFont;
      ATG::AssetRef<DirectX::SpriteFont> m_gameBoardTitleFont;
      ATG::AssetRef<DirectX::SpriteFont> m_logFont;
      float                              m_logFontHeight;

      // Textures

      ATG::AssetRef<DX::Texture>         m_background;
      ATG::AssetRef<DX::Texture>         m_clearLetterControl;
      ATG::AssetRef<DX::Texture>         m_cursor;
      ATG::AssetRef<DX::Texture>         m_horizontalWordLinker;
      ATG::AssetRef<DX::Texture>         m_logScrollDownControl;
      ATG::AssetRef<DX::Texture>         m_logScrollUpControl;
      ATG::AssetRef<DX::Texture>         m_menuControl;
      ATG::AssetRef<DX::Texture>         m_moveCursorControl;
      ATG::AssetRef<DX::Texture>         m_saveSlotLeftControl;
      ATG::AssetRef<DX::Texture>         m_saveSlotRightControl;
      ATG::AssetRef<DX::Texture>         m_swapLetterControl;
      ATG::AssetRef<DX::Texture>         m_verticalWordLinker;
      std::vector<ATG::AssetRef<DX::Texture>>    m_saveSlotNumbers;
      std::vector<ATG::AssetRef<DX::Texture>>    m_saveSlotActiveNumbers;

      //
      // Game State
      //

      DirectX::XMUINT2                                    m_cursorPosition; // 0-based (x, y) position on board
      uint32_t                                            m_firstSlotToDisplay;
      bool												  m_isGameSaveManagerInitialized;
      bool                                                m_isOverLimitOnLetters;
      std::vector<int>                                    m_lettersRemaining;
      size_t                                              m_logLineBegin; // set to non-zero value to stop auto-scrolling
      float                                               m_logScrollDownDelay;
      float                                               m_logScrollUpDelay;
      int                                                 m_score;
      float                                               m_tileScrollDownDelay;
      float                                               m_tileScrollUpDelay;
      std::unique_ptr<WordList>                           m_wordList;
      bool                                                m_wordListLoaded;
      std::vector<WordTrackerTile>                        m_wordTracker;

      //
      // Async tasks
      //

      ATG::AsyncAction<void> m_resetBoardTask;
      ATG::AsyncAction<void> m_readBoardTask;
      ATG::AsyncAction<void> m_saveBoardTask;
      ATG::AsyncAction<void> m_deleteBoardTask;
      ATG::AsyncAction<void> m_deleteBoardBlobTask;
      ATG::AsyncAction<void> m_listContainersTask;
      ATG::AsyncAction<void> m_listContainersAndBlobsTask;
   };
}
