// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"

#include "SampleGame.h"
#include "Assets.h"
#include "UserManager.h"
#include "GameBoardScreen.h"
#include "ConfirmPopUpScreen.h"
#include "GameSaveManager.h"

using namespace DirectX;

namespace GameSaveSample {

namespace
{
#pragma region Game Board Draw Constants
    //const auto c_saveSlotLeftControlHelp_Center = DirectX::XMFLOAT2(314.f, 118.f);
    //const auto c_saveSlotRightControlHelp_Center = DirectX::XMFLOAT2(854.f, 118.f);
    const auto c_saveSlotFirstNumber_TopCenter = DirectX::XMFLOAT2(399.f, 89.f);
    const auto c_saveSlotNumber_PixelsBetweenCenters = 196.f;
    const auto c_saveSlotNumber_Radius = 23.f;
    //const auto c_saveSlotSecondNumber_TopCenter = DirectX::XMFLOAT2(595.f, 89.f);
    //const auto c_saveSlotThirdNumber_TopCenter = DirectX::XMFLOAT2(781.f, 89.f);
    const auto c_saveSlotFirstNumber_CircleCenter = DirectX::XMFLOAT2(399.f, 116.f);
    const auto c_saveSlotSecondNumber_CircleCenter = DirectX::XMFLOAT2(595.f, 116.f);
    const auto c_saveSlotThirdNumber_CircleCenter = DirectX::XMFLOAT2(781.f, 116.f);

    const auto c_gameTitle_UpperLeft = DirectX::XMFLOAT2(190.f, 170.f);

    const auto c_playerName_UpperLeft = DirectX::XMFLOAT2(200.f, 245.f);
    const auto c_playerName_Scale = 0.85f;

    const auto c_gameSaveMetadata_UpperLeft = DirectX::XMFLOAT2(200.f, 270.f);
    const auto c_gameSaveMetadata_Scale = 0.85f;
    const auto c_gameSaveAdditionalMetadata_UpperLeft = DirectX::XMFLOAT2(200.f, 295.f);

    const auto c_currentScore_UpperLeft = DirectX::XMFLOAT2(100.f, 375.f);
    const auto c_currentScore_Scale = 1.75f;
    const auto c_currentScoreLabel_UpperLeft = DirectX::XMFLOAT2(110.f, 440.f);

    const DirectX::XMVECTORF32 c_lettersRemaining_Color = { 230.f / 255.f, 178.f / 255.f, 138.f / 255.f, 1.f };
    const auto c_lettersRemaining_UpperLeft = DirectX::XMFLOAT2(255.f, 375.f);
    const auto c_lettersRemaining_LetterOffset = DirectX::XMFLOAT2(75.f, 23.f);
    const auto c_lettersRemaining_Scale = 0.8f;
    const auto c_lettersRemainingHelp_Center = DirectX::XMFLOAT2(600.f, 460.f);

    const auto c_letterTileRegion = RECT{ 160 - 42, 530 - 42, 538 + 42, 918 + 42 };
    const auto c_letterTileFirstTile_Center = DirectX::XMFLOAT2(160.f, 530.f);
    const auto c_letterTileRadius = 42.f;
    const auto c_letterTile_CenterOffset = DirectX::XMFLOAT2(94.f, 97.f);
    const auto c_letterTileValue_OffsetFromTileCenter = DirectX::XMFLOAT2(16.f, 8.f);
    const auto c_letterTileScrollDelay_Seconds = 0.2f;

    const auto c_wordLinkFirstHorizontal_Center = DirectX::XMFLOAT2(210.f, 530.f);
    const auto c_wordLinkFirstVertical_Center = DirectX::XMFLOAT2(160.f, 582.f);

    const auto c_menu_Region = DX::RectangleF(635.f, 490.f, 500.f, 500.f);

    //const auto c_inputControlsHelp_UpperLeft = DirectX::XMFLOAT2(100.f, 1010.f);
    const auto c_inputControlsHelp_Scale = 0.6f;

    const auto c_debugLog_Region = DX::RectangleF(1240.f, 150.f, 620.f, 800.f);
    const auto c_debugLogScrollUpIcon_Center = DirectX::XMFLOAT2(1560.f, 130.f);
    const auto c_debugLogScrollDownIcon_Center = DirectX::XMFLOAT2(1560.f, 970.f);
    const auto c_debugLogScrollDelay_Seconds = 0.1f;

    // Letter count
    const int c_letterCounts[] =
    {
        5, // A
        3, // B
        3, // C
        3, // D
        6, // E
        3, // F
        3, // G
        3, // H
        4, // I
        1, // J
        2, // K
        4, // L
        3, // M
        4, // N
        5, // O
        3, // P
        1, // Q
        4, // R
        4, // S
        6, // T
        3, // U
        2, // V
        2, // W
        1, // X
        3, // Y
        2 // Z
    };

    // Letter values
    const int c_letterValues[] =
    {
        1, // A
        3, // B
        3, // C
        2, // D
        1, // E
        4, // F
        2, // G
        4, // H
        1, // I
        8, // J
        5, // K
        1, // L
        3, // M
        1, // N
        1, // O
        3, // P
        10, // Q
        1, // R
        1, // S
        1, // T
        1, // U
        4, // V
        4, // W
        8, // X
        4, // Y
        10 // Z
    };
#pragma endregion
}

GameBoardScreen::GameBoardScreen(ScreenManager& screenManager) : 
    MenuScreen(screenManager),
    m_logFontHeight(0),
    m_cursorPosition{},
    m_firstSlotToDisplay(1),
	m_isGameSaveManagerInitialized(false),
    m_isOverLimitOnLetters(false),
    m_lettersRemaining(c_letterCounts, c_letterCounts + sizeof(c_letterCounts) / sizeof(c_letterCounts[0])),
    m_logLineBegin(0),
    m_logScrollDownDelay(0),
    m_logScrollUpDelay(0),
    m_score(0),
    m_tileScrollDownDelay(0),
    m_tileScrollUpDelay(0),
    m_wordListLoaded(false)
{
    // Setup word tracker
    auto boardTiles = c_boardWidth * c_boardHeight;
    for (uint32_t i = 1; i <= boardTiles; ++i)
    {
        m_wordTracker.push_back(WordTrackerTile());
    }

    // Setup menu display parameters
    m_animateSelected = false;
    m_menuActive = false;
    m_menuSpacing = 1.4f;
    m_showCurrentUser = false;
    SetCenterJustified(false);

    // Setup menu entries
    ClearMenuEntries();

    m_menuEntries.push_back(MenuEntry(u8"Read Board",
        [&](int)
    {
         if (m_readBoardTask.GetState() == ATG::AsyncOpStates::Busy)
         {
            Log::WriteAndDisplay(u8"Read Board task is still running.");
            return;
         }

         if (!g_Game->GetGameSaveManager().HasActiveBoard())
         {
            Log::WriteAndDisplay(u8"You do not have an active game board");
            return;
         }

        // make sure log auto-scroll is on so that sample user sees the results from this action
        m_logLineBegin = 0;

        // load or reload game state
        auto activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();
        auto& activeBoardMetadata = g_Game->GetGameSaveManager().GetActiveBoardGameSave()->GetContainerMetadata();
        auto& activeBoardContainerName = activeBoardMetadata.m_containerName;
        auto activeBoardNeedsSync = activeBoardMetadata.m_needsSync;

        m_readBoardTask.Reset();
        m_readBoardTask.SetFunction(std::function<void()>(
           [this, activeBoard, activeBoardNeedsSync, activeBoardContainerName ]()
           {
              ReadBoardTask( activeBoard, activeBoardNeedsSync, activeBoardContainerName );
           }
        ));

        m_readBoardTask.Start(g_Game->GetGeneralThreadPoolTaskQueue());

    }));

    m_menuEntries.push_back(MenuEntry("Save Board",
        [&](int)
    {
          if (m_saveBoardTask.GetState() == ATG::AsyncOpStates::Busy)
          {
             Log::WriteAndDisplay(u8"Save Board task is still running.");
             return;
          }

          if (!g_Game->GetGameSaveManager().HasActiveBoard())
          {
             Log::WriteAndDisplay(u8"You do not have an active game board");
             return;
          }

        // make sure log auto-scroll is on so that sample user sees the results from this action
        m_logLineBegin = 0;

        // save game state
        auto activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();
        auto& activeBoardContainerName = g_Game->GetGameSaveManager().GetActiveBoardGameSave()->GetContainerMetadata().m_containerName;

        m_saveBoardTask.Reset();
        m_saveBoardTask.SetFunction(std::function<void()>(
           [this, activeBoard, activeBoardContainerName]()
           {
              SaveBoardTask( activeBoard, activeBoardContainerName );
           }
        ));

        m_saveBoardTask.Start(g_Game->GetGeneralThreadPoolTaskQueue());
    }));

    m_menuEntries.push_back(MenuEntry(u8"Reset Board",
        [&](int)
    {
        // make sure log auto-scroll is on so that sample user sees the results from this action
        m_logLineBegin = 0;

        if (!g_Game->GetGameSaveManager().HasActiveBoard())
        {
           Log::WriteAndDisplay(u8"You do not have an active game board");
           return;
        }

        // clear board of letter tiles, marking it dirty if appropriate
        std::lock_guard<std::mutex> lock(g_Game->GetGameSaveManager().GetActiveBoardGameSave()->m_mutex);

        auto& gameBoard = g_Game->GetGameSaveManager().GetActiveBoard();
        gameBoard.ResetBoard();

        Log::WriteAndDisplay(u8"Game board %d reset\n", g_Game->GetGameSaveManager().GetActiveBoardNumber());

        auto activeGameSave = g_Game->GetGameSaveManager().GetActiveBoardGameSave();
        // (being explicit here for readability...)
        if (activeGameSave->m_isGameDataLoaded)
        {
            activeGameSave->m_isGameDataDirty = true;
        }
        else
        {
            activeGameSave->m_isGameDataDirty = false;
        }
    }));

    m_menuEntries.push_back(MenuEntry(u8"Delete Board",
        [&](int)
    {
          // make sure log auto-scroll is on so that sample user sees the results from this action
          m_logLineBegin = 0;

          if (m_deleteBoardTask.GetState() == ATG::AsyncOpStates::Busy)
          {
             Log::WriteAndDisplay(u8"Delete Board task is still running.");
             return;
          }

          if (!g_Game->GetGameSaveManager().HasActiveBoard())
          {
             Log::WriteAndDisplay(u8"You do not have an active game board");
             return;
          }

        // delete game state (resets board)
        auto& activeBoardContainerName = g_Game->GetGameSaveManager().GetActiveBoardGameSave()->GetContainerMetadata().m_containerName;
        uint32_t activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();

        m_deleteBoardTask.Reset();
        m_deleteBoardTask.SetFunction(std::function<void()>(
           [this, activeBoard, activeBoardContainerName]()
           {
              DeleteBoardTask( activeBoard, activeBoardContainerName );
           }
        ));
        m_deleteBoardTask.Start(g_Game->GetGeneralThreadPoolTaskQueue());
    }));

    m_menuEntries.push_back(MenuEntry("Delete Board Blob",
        [&](int)
    {
          if (m_deleteBoardBlobTask.GetState() == ATG::AsyncOpStates::Busy)
          {
             Log::WriteAndDisplay(u8"Delete Board Blob task is still running.");
             return;
          }

          if (!g_Game->GetGameSaveManager().HasActiveBoard())
          {
             Log::WriteAndDisplay(u8"You do not have an active game board");
             return;
          }

        // make sure log auto-scroll is on so that sample user sees the results from this action
        m_logLineBegin = 0;

        // delete game state blobs (but not container) (resets board)
        auto& activeBoardContainerName = g_Game->GetGameSaveManager().GetActiveBoardGameSave()->GetContainerMetadata().m_containerName;
        auto activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();

        m_deleteBoardBlobTask.Reset();
        m_deleteBoardBlobTask.SetFunction(std::function<void()>(
           [this, activeBoard, activeBoardContainerName]()
           {
              DeleteBoardBlobTask(activeBoard, activeBoardContainerName);
           }
        ));

        m_deleteBoardBlobTask.Start(g_Game->GetGeneralThreadPoolTaskQueue());
    }));

    m_menuEntries.push_back(MenuEntry(u8"List Containers",
        [&](int)
    {
          if (m_listContainersTask.GetState() == ATG::AsyncOpStates::Busy)
          {
             Log::WriteAndDisplay(u8"List Containers task is still running.");
             return;
          }

        // make sure log auto-scroll is on so that sample user sees the results from this action
        m_logLineBegin = 0;

        // start a container query
        Log::WriteAndDisplay(u8"Container query started...\n");

        m_listContainersTask.Reset();
        m_listContainersTask.SetFunction(std::function<void()>(
           [this]()
           {
              ListContainersTask();
           }
        ));

        m_listContainersTask.Start(g_Game->GetGeneralThreadPoolTaskQueue());

    }));

    m_menuEntries.push_back(MenuEntry("List Containers & Blobs",
      [&](int)
      {
         if (m_listContainersAndBlobsTask.GetState() == ATG::AsyncOpStates::Busy)
         {
            Log::WriteAndDisplay(u8"List Containers & Blobs task is still running.");
            return;
         }

         // make sure log auto-scroll is on so that sample user sees the results from this action
         m_logLineBegin = 0;

         // start a container AND blob query
         Log::WriteAndDisplay(u8"Container and blob query started...\n");
        
         m_listContainersAndBlobsTask.Reset();
         m_listContainersAndBlobsTask.SetFunction(std::function<void()>(
            [this]() 
            { 
               ListContainersAndBlobsTask();
            }
            ));

         m_listContainersAndBlobsTask.Start( g_Game->GetGeneralThreadPoolTaskQueue() );
    }));
}

void GameBoardScreen::ReadBoardTask( uint32_t boardNumber, bool needsSync, const std::string& boardContainerName )
{
   g_Game->GetGameSaveManager().ReadBlocking( boardNumber );

   if (needsSync)
   {
      g_Game->GetGameSaveManager().LoadContainerMetadataBlocking(boardContainerName.c_str(), true);
   }
}


void GameBoardScreen::SaveBoardTask( uint32_t boardNumber, const std::string& boardContainerName )
{
   g_Game->GetGameSaveManager().SaveBlocking( boardNumber, SaveMode::SaveAlways );
   
   // update the index while we're saving this board, if it's dirty
   g_Game->GetGameSaveManager().SaveIndexBlocking(true);

   g_Game->GetGameSaveManager().LoadContainerMetadataBlocking(boardContainerName.c_str(), true);
   
   g_Game->GetGameSaveManager().UpdateRemainingQuotaBlocking();
}

void GameBoardScreen::DeleteBoardTask( uint32_t boardNumber, const std::string& boardContainerName)
{
   g_Game->GetGameSaveManager().DeleteBlocking(boardNumber);
   
   g_Game->GetGameSaveManager().LoadContainerMetadataBlocking(boardContainerName.c_str(), true);
   
   g_Game->GetGameSaveManager().UpdateRemainingQuotaBlocking();
}

void GameBoardScreen::DeleteBoardBlobTask( uint32_t boardNumber, const std::string& boardContainerName )
{
   g_Game->GetGameSaveManager().DeleteBlobsBlocking( boardNumber );

   g_Game->GetGameSaveManager().LoadContainerMetadataBlocking(boardContainerName.c_str(), true);
   
   g_Game->GetGameSaveManager().UpdateRemainingQuotaBlocking();
}


void GameBoardScreen::ListContainersTask()
{
   g_Game->GetGameSaveManager().EnumerateContainersBlocking(false);
   
   Log::WriteAndDisplay(u8"Container query complete\n");
   
   g_Game->GetGameSaveManager().WriteGameSaveMetadataToDisplayLog(false);
}

void GameBoardScreen::ListContainersAndBlobsTask()
{
   g_Game->GetGameSaveManager().EnumerateContainersBlocking(true);
   
   Log::WriteAndDisplay(u8"Container and blob query complete\n");
   
   g_Game->GetGameSaveManager().WriteGameSaveMetadataToDisplayLog(true);
}

GameBoardScreen::~GameBoardScreen()
{
}

void GameBoardScreen::LoadContent( ATG::AssetLoadResources& loadRes )
{
    MenuScreen::LoadContent( loadRes );

    // Bind texture references
    m_background = ATG::GetTextureAsset(loadRes, AssetDescriptor::GameBoardBackground);
    m_cursor = ATG::GetTextureAsset( loadRes, AssetDescriptor::Cursor);
    m_horizontalWordLinker = ATG::GetTextureAsset( loadRes, AssetDescriptor::HorizontalWordLinker);
    m_verticalWordLinker = ATG::GetTextureAsset( loadRes, AssetDescriptor::VerticalWordLinker);

    for (uint32_t i = 1; i <= c_saveSlotCount; ++i)
    {
        m_saveSlotNumbers.push_back(ATG::GetTextureAsset( loadRes, (AssetDescriptor)((int)AssetDescriptor::SaveSlot1Glyph + i - 1)));
        m_saveSlotActiveNumbers.push_back(ATG::GetTextureAsset( loadRes, (AssetDescriptor) ((int)AssetDescriptor::SaveSlot1GlyphHighlighted + i - 1 ) ));
    }

    // Load input controls textures
    m_clearLetterControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerXButtonIcon);
    m_logScrollDownControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerRightStickIcon);
    m_logScrollUpControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerRightStickIcon );
    m_menuControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerDPadIcon );
    m_moveCursorControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerLeftStickIcon );
    m_saveSlotLeftControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerLeftBumperIcon );
    m_saveSlotRightControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerRightBumperIcon );
    m_swapLetterControl = ATG::GetTextureAsset( loadRes, AssetDescriptor::ControllerRightStickIcon );

    // Load fonts
    m_gameBoardControlsHelpFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::SegoeUI24Font );
    m_gameBoardLettersRemainingFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::Consolas16Font );
    m_gameBoardMetadataFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::SegoeUISemilight18Font );
	 m_gameBoardMetadataFont->SetDefaultCharacter(L'*');
    m_gameBoardTitleFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::SegoeUILight42Font );
    m_gameBoardTileValueFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::SegoeUI24Font );
    m_gameBoardTileFont = ATG::GetSpriteFontAsset( loadRes, AssetDescriptor::SegoeUISemilight42Font );
    m_logFont = Manager().GetDebugFont();
    XMVECTOR logFontMeasurement = m_logFont->MeasureString(L"X");
    m_logFontHeight = XMVectorGetY(logFontMeasurement);

    // Load word list
    LoadWordListTask( L"Assets\\TWL06_2to5.txt" );
}

void GameBoardScreen::LoadWordListTask( const wchar_t* path )
{
   Log::WriteAndDisplay( "Loading word list...\n" );

   bool success = false;
   m_wordListLoaded = false;
   m_wordList.reset( nullptr );

   HANDLE hWordFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
   {
      if ( hWordFile == INVALID_HANDLE_VALUE )
      {
         Log::Write( u8"ERROR opening word list file (%s)\n", FormatHResult( E_INVALIDARG ).c_str() );
         return;
      }

      FILE_STANDARD_INFO fileInfo;
      if ( GetFileInformationByHandleEx( hWordFile, FileStandardInfo, &fileInfo, sizeof( fileInfo ) ) == FALSE )
      {
         Log::Write( u8"ERROR getting file size of word list file (%s)\n", FormatHResult( HRESULT_FROM_WIN32( GetLastError() ) ).c_str() );
         goto ExitFn;
      }

      LARGE_INTEGER fileSize = fileInfo.EndOfFile;
      if ( fileSize.HighPart > 0 )
      {
         Log::Write( u8"ERROR file size of word list too big\n" );
         goto ExitFn;
      }

      m_wordList.reset( new WordList );

      std::unique_ptr<char[]> buffer;
      buffer.reset( new char[ fileSize.LowPart + 1 ] );

      DWORD bytesRead;
      if ( ReadFile( hWordFile, buffer.get(), fileSize.LowPart, &bytesRead, nullptr ) == FALSE )
      {
         Log::Write( u8"ERROR reading word list file (%s)\n", FormatHResult( HRESULT_FROM_WIN32( GetLastError() ) ).c_str() );
         goto ExitFn;
      }

      if ( bytesRead < fileSize.LowPart )
      {
         Log::Write( u8"ERROR incomplete read of word list file (%d of %d bytes read)\n", bytesRead, fileSize.LowPart );
         goto ExitFn;
      }

      auto readBuffer = buffer.get();
      readBuffer[ bytesRead ] = 0;
      char* word = 0;
      char* token = 0;
      size_t converted = 0;
      wchar_t wword[ 11 ] = {}; // longest word supported
      word = strtok_s( readBuffer, "\r\n", &token );
      while ( word != nullptr )
      {
         mbstowcs_s( &converted, wword, word, 10 );
         m_wordList->insert( wword );
         word = strtok_s( nullptr, "\r\n", &token );
      }
   }

   success = true;

ExitFn:
   if ( hWordFile != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hWordFile );
   }

   if ( success )
   {
      m_wordListLoaded = true;
      Log::WriteAndDisplay( "Word list loaded\n" );
   }
   else
   {
      m_wordList.reset( nullptr );
      Log::WriteAndDisplay( "Word list load failed\n" );
   }
}

void GameBoardScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
    
    if (m_logScrollDownDelay > 0)
    {
        m_logScrollDownDelay -= elapsedTime;
    }
    if (m_logScrollUpDelay > 0)
    {
        m_logScrollUpDelay -= elapsedTime;
    }

    if (m_tileScrollDownDelay > 0)
    {
        m_tileScrollDownDelay -= elapsedTime;
    }
    if (m_tileScrollUpDelay > 0)
    {
        m_tileScrollUpDelay -= elapsedTime;
    }

	if (!m_isGameSaveManagerInitialized)
	{
		// Has the manager initialized since last checked?
		if (g_Game->GetGameSaveManager().IsInitialized())
		{
			// Perform save slot display setup
			auto activeBoardNum = g_Game->GetGameSaveManager().GetActiveBoardNumber();
			if (activeBoardNum <= 3)
			{
				m_firstSlotToDisplay = 1;
			}
			else
			{
				m_firstSlotToDisplay = std::min(activeBoardNum, c_saveSlotCount - 2);
			}
		}
	}
	 m_isGameSaveManagerInitialized = g_Game->GetGameSaveManager().IsInitialized();

    if (g_Game->GetGameSaveManager().HasActiveBoard())
    {
        std::lock_guard<std::mutex> lock(g_Game->GetGameSaveManager().GetActiveBoardGameSave()->m_mutex);

        UpdateLettersRemaining();

        if (m_wordListLoaded)
        {
            UpdateScore();
        }
    }
}

void GameBoardScreen::HandleInput(const DirectX::InputState & input)
{
    // Handle menu input
    MenuScreen::HandleInput(input);

    // Handle non-menu game board input if there's an active board
    auto& gameSaveManager = g_Game->GetGameSaveManager();

    if (gameSaveManager.HasActiveBoard())
    {
        std::lock_guard<std::mutex> lock(gameSaveManager.GetActiveBoardGameSave()->m_mutex);

        int controllingPlayer = GetControllingPlayer();
        auto activeBoard = gameSaveManager.GetActiveBoard();
        auto activeBoardNum = gameSaveManager.GetActiveBoardNumber();
        auto& gameBoard = gameSaveManager.GetActiveBoard();
        auto& currentTile = gameBoard.GetGameTile(m_cursorPosition);
        auto letterPressed = input.GetLetterPressed();
        auto numberPressed = input.GetNumberPressed();
        XMINT2 mouseClick = XMINT2();

        if (!m_menuActive && letterPressed > 0)
        {
            if (!currentTile.m_placed || (currentTile.m_letter != letterPressed))
            {
                currentTile.m_placed = true;
                currentTile.m_letter = letterPressed;
                gameSaveManager.MarkActiveBoardDirty();
            }
        }
        else if (numberPressed > 0 && numberPressed <= c_saveSlotCount)
        {
            if (numberPressed < activeBoardNum)
            {
                gameSaveManager.SetActiveBoardNumberAsync( numberPressed );
                m_firstSlotToDisplay = std::min(numberPressed, m_firstSlotToDisplay);
            }
            else if (numberPressed > activeBoardNum)
            {
                gameSaveManager.SetActiveBoardNumberAsync( numberPressed );
                m_firstSlotToDisplay = std::max(m_firstSlotToDisplay, numberPressed - 2);
            }
        }
        else if (input.IsMouseSelect(mouseClick))
        {
            auto localMouseClick = ATG::ConvertWindowPixelToLocalCoord(Manager().GetWindowBounds(), mouseClick);
            auto localMouseClickF = XMFLOAT2(float(localMouseClick.x), float(localMouseClick.y));
            if (ATG::IsPointInsideRectangle(localMouseClick, c_letterTileRegion))
            {
                for (uint32_t j = 0; j < activeBoard.m_boardHeight; ++j)
                {
                    for (uint32_t i = 0; i < activeBoard.m_boardWidth; ++i)
                    {
                        XMFLOAT2 tileCenter = XMFLOAT2(c_letterTileFirstTile_Center.x + (c_letterTile_CenterOffset.x * i), c_letterTileFirstTile_Center.y + (c_letterTile_CenterOffset.y * j));
                        if (ATG::IsPointInsideCircle(localMouseClickF, tileCenter, c_letterTileRadius))
                        {
                            m_cursorPosition.x = i;
                            m_cursorPosition.y = j;
                            m_menuActive = false;
                            return;
                        }
                    }
                }
            }
            else if (ATG::IsPointInsideCircle(localMouseClickF, c_saveSlotFirstNumber_CircleCenter, c_saveSlotNumber_Radius))
            {
               gameSaveManager.SetActiveBoardNumberAsync( m_firstSlotToDisplay );
            }
            else if (ATG::IsPointInsideCircle(localMouseClickF, c_saveSlotSecondNumber_CircleCenter, c_saveSlotNumber_Radius))
            {
                gameSaveManager.SetActiveBoardNumberAsync( m_firstSlotToDisplay + 1 );
            }
            else if (ATG::IsPointInsideCircle(localMouseClickF, c_saveSlotThirdNumber_CircleCenter, c_saveSlotNumber_Radius))
            {
               gameSaveManager.SetActiveBoardNumberAsync( m_firstSlotToDisplay + 2 );
            }
        }
        else if (!m_menuActive &&
            (input.IsNewButtonPress(GameInputGamepadX, controllingPlayer)
            || input.IsNewKeyPress(Keyboard::Keys::Space)
            || input.IsNewKeyPress(Keyboard::Keys::Delete)))
        {
            if (currentTile.m_placed)
            {
                gameSaveManager.MarkActiveBoardDirty();
            }
            currentTile.m_letter = 0;
            currentTile.m_placed = false;
        }
        else if (!m_menuActive && input.IsTileScrollUp(controllingPlayer))
        {
            if (m_tileScrollUpDelay <= 0)
            {
                // advance tile value
                if (!currentTile.m_placed)
                {
                    currentTile.m_placed = true;
                    currentTile.m_letter = L'A';
                }
                else
                {
                    if (currentTile.m_letter >= L'A'
                        && currentTile.m_letter < L'Z')
                    {
                        currentTile.m_letter++;
                    }
                    else
                    {
                        currentTile.m_letter = L'A';
                    }
                }

                gameSaveManager.MarkActiveBoardDirty();
                m_tileScrollUpDelay = c_letterTileScrollDelay_Seconds;
            }
        }
        else if (!m_menuActive && input.IsTileScrollDown(controllingPlayer))
        {
            if (m_tileScrollDownDelay <= 0)
            {
                // decrease tile value
                if (!currentTile.m_placed)
                {
                    currentTile.m_placed = true;
                    currentTile.m_letter = L'Z';
                }
                else
                {
                    if (currentTile.m_letter > L'A'
                        && currentTile.m_letter <= L'Z')
                    {
                        currentTile.m_letter--;
                    }
                    else
                    {
                        currentTile.m_letter = L'Z';
                    }
                }

                gameSaveManager.MarkActiveBoardDirty();
                m_tileScrollDownDelay = c_letterTileScrollDelay_Seconds;
            }
        }
        else if (input.IsLogScrollUp())
        {
            size_t maxDisplayLines = static_cast<size_t>(c_debugLog_Region.Height / m_logFontHeight);
            
            size_t logIndexSize;
            {
               logIndexSize = Log::GetDisplayLog_ScopedSharedLock()->size();
               // lock is released when it goes out of scope.
            }

            if (m_logLineBegin == 0)
            {
                if (logIndexSize > maxDisplayLines)
                {
                    // stop auto-scroll at current top line
                    m_logLineBegin = logIndexSize - maxDisplayLines;
                }
            }
            else
            {
                if (m_logScrollUpDelay <= 0)
                {
                    // scroll up
                    m_logLineBegin = std::max(m_logLineBegin - 1, size_t(1));
                    m_logScrollUpDelay = c_debugLogScrollDelay_Seconds;
                }
            }
        }
        else if (input.IsLogScrollDown())
        {
            if (m_logLineBegin > 0)
            {
                size_t maxDisplayLines = static_cast<size_t>(c_debugLog_Region.Height / m_logFontHeight);
                
                size_t logIndexSize;
                {
                  logIndexSize = Log::GetDisplayLog_ScopedSharedLock()->size();
                  // log lock is released when it goes out of scope.
                }
                
                if ((m_logLineBegin + 1) > (logIndexSize - maxDisplayLines))
                {
                    // turn auto-scroll back on
                    m_logLineBegin = 0;
                }

                if (m_logScrollDownDelay <= 0)
                {
                    // scroll down
                    m_logLineBegin++;
                    m_logScrollDownDelay = c_debugLogScrollDelay_Seconds;
                }
            }
        }
        else if (input.IsNewKeyPress(Keyboard::Keys::Home))
        {
            size_t maxDisplayLines = static_cast<size_t>(c_debugLog_Region.Height / m_logFontHeight);
            
            size_t logIndexSize;
            {
               logIndexSize = Log::GetDisplayLog_ScopedSharedLock()->size();
               // lock is released when it goes out of scope.
            }

            if (logIndexSize > maxDisplayLines)
            {
                m_logLineBegin = 1;
            }
            else
            {
                m_logLineBegin = 0;
            }
        }
        else if (input.IsNewKeyPress(Keyboard::Keys::End))
        {
            m_logLineBegin = 0;
        }
        else if (input.IsNewButtonPress(GameInputGamepadLeftShoulder, controllingPlayer) && activeBoardNum > 1)
        {
            auto newActiveBoardNum = activeBoardNum - 1;
            gameSaveManager.SetActiveBoardNumberAsync( newActiveBoardNum );
            m_firstSlotToDisplay = std::min(newActiveBoardNum, m_firstSlotToDisplay);
        }
        else if (input.IsNewButtonPress(GameInputGamepadRightShoulder, controllingPlayer) && activeBoardNum < c_saveSlotCount)
        {
            auto newActiveBoardNum = activeBoardNum + 1;
            gameSaveManager.SetActiveBoardNumberAsync( newActiveBoardNum );
            m_firstSlotToDisplay = std::max(m_firstSlotToDisplay, newActiveBoardNum - 2);
        }
        else if (input.IsCursorLeft(controllingPlayer, nullptr))
        {
            if (m_menuActive)
            {
                m_menuActive = false;
                m_cursorPosition.x = gameBoard.m_boardWidth - 1;
            }
            else if (m_cursorPosition.x > 0)
            {
                m_cursorPosition.x--;
            }
            else
            {
                m_menuActive = true;
            }
        }
        else if (input.IsCursorRight(controllingPlayer, nullptr))
        {
            if (m_menuActive)
            {
                m_menuActive = false;
                m_cursorPosition.x = 0;
            }
            else if (m_cursorPosition.x < gameBoard.m_boardWidth - 1)
            {
                m_cursorPosition.x++;
            }
            else
            {
                m_menuActive = true;
            }

        }
        else if (!m_menuActive && input.IsCursorUp(controllingPlayer, nullptr))
        {
            if (m_cursorPosition.y > 0)
            {
                m_cursorPosition.y--;
            }
            else
            {
                m_cursorPosition.y = gameBoard.m_boardHeight - 1;
            }
        }
        else if (!m_menuActive && input.IsCursorDown(controllingPlayer, nullptr))
        {
            if (m_cursorPosition.y < gameBoard.m_boardHeight - 1)
            {
                m_cursorPosition.y++;
            }
            else
            {
                m_cursorPosition.y = 0;
            }
        }
    } // HasActiveBoard
}

void GameBoardScreen::Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime)
{
    auto spriteBatch = Manager().GetSpriteBatch();
    auto gameFont = Manager().GetSpriteFont();
    auto blendStates = Manager().GetCommonStates();
    auto scaleMatrix = ATG::GetScaleMatrixForWindow(Manager().GetWindowBounds());

    // Draw background
    spriteBatch->Begin(commandList, SpriteSortMode_Deferred, scaleMatrix);
    spriteBatch->Draw( m_background.GetGpuHandle(), m_background->GetTextureSize(), XMFLOAT2(0, 0) );
    spriteBatch->End();

    if (IsActive() && m_isGameSaveManagerInitialized)
    {
        MenuScreen::Draw(commandList, totalTime, elapsedTime);
    }

    if (m_state != ScreenState::Active)
    {
        return;
    }

    auto& gameSaveManager = g_Game->GetGameSaveManager();
    assert(gameSaveManager.HasActiveBoard());

    std::lock_guard<std::mutex> lock(gameSaveManager.GetActiveBoardGameSave()->m_mutex);

    auto activeBoardNum = gameSaveManager.GetActiveBoardNumber();
    auto activeBoard = gameSaveManager.GetActiveBoard();
    auto activeBoardGameSave = gameSaveManager.GetActiveBoardGameSave();
    auto& activeBoardMetadata = activeBoardGameSave->m_containerMetadata;

    spriteBatch->Begin(commandList, SpriteSortMode_Deferred, scaleMatrix);

    // Draw numbers of the visible game boards and highlight the active one
    XMFLOAT2 position = c_saveSlotFirstNumber_TopCenter;
    XMFLOAT2 origin = XMFLOAT2(m_saveSlotNumbers[0]->Width() / 2.0f, 0.0f);

    for (auto i = m_firstSlotToDisplay; i < m_firstSlotToDisplay + 3; ++i)
    {
        if (i == m_firstSlotToDisplay + 2)
            position.x -= 10; // UI correction: 3rd board number is not the same distance from the 2nd as the 2nd is from the 1st

        if (i == activeBoardNum)
        {
           auto& activeNumber = m_saveSlotActiveNumbers[ i - 1 ];

           spriteBatch->Draw(activeNumber.GetGpuHandle(), activeNumber->GetTextureSize(), position, nullptr, Colors::White, 0.0f, origin);
        }
        else
        {
           auto& number = m_saveSlotNumbers[ i - 1 ];
           spriteBatch->Draw(number.GetGpuHandle(), number->GetTextureSize(), position, nullptr, Colors::White, 0.0f, origin);
        }

        position.x += c_saveSlotNumber_PixelsBetweenCenters;
    }

    // Draw active board title and dirty status
    std::string gameBoardTitleStr( "WordGame" );
    if (activeBoardNum > 0)
    {
        gameBoardTitleStr += " Board " + std::to_string(activeBoardNum);
    }
    if (gameSaveManager.IsActiveBoardDirty())
    {
        gameBoardTitleStr += "*";
    }
    m_gameBoardTitleFont->DrawString(spriteBatch.get(), gameBoardTitleStr.c_str(), c_gameTitle_UpperLeft);

    // Draw player name
    std::string userDisplayStr( "User: " );
    auto user = ATG::UserManager::GetCurrentUser();
    if ( user )
    {
       userDisplayStr += user->GetGamerTag();
    }

    m_gameBoardMetadataFont->DrawString(spriteBatch.get(), userDisplayStr.c_str(), c_playerName_UpperLeft, Colors::White, 0.0f, XMFLOAT2(0, 0), c_playerName_Scale);

    // Draw active board metadata (last save date, current user gamertag)
    std::string gameBoardMetadataDisplay;
    if (activeBoardMetadata.m_isGameDataOnDisk)
    {
        gameBoardMetadataDisplay += activeBoardGameSave->m_isGameDataLoaded ? "BOARD LOADED" : "BOARD NOT LOADED";

        if (activeBoardMetadata.m_needsSync)
        {
            gameBoardMetadataDisplay += "  (NEEDS SYNC)";
        }

        gameBoardMetadataDisplay += "  (Last Modified: " + FormatLocalTimeFromDateTime(activeBoardMetadata.m_lastModified) + ")";
    }
    else
    {
        if (gameSaveManager.IsActiveBoardDirty())
        {
            gameBoardMetadataDisplay += "BOARD NOT SAVED";
        }
        else
        {
            gameBoardMetadataDisplay += "BOARD NOT STARTED";
        }
    }
    m_gameBoardMetadataFont->DrawString(spriteBatch.get(), gameBoardMetadataDisplay.c_str(), c_gameSaveMetadata_UpperLeft, Colors::White, 0.0f, XMFLOAT2(0, 0), c_gameSaveMetadata_Scale);

    // Draw active board dev metadata (sync status/mode)
    if (m_isGameSaveManagerInitialized)
    {
        std::string devModeMetadata("Sync Mode: ");
        if (gameSaveManager.GetSyncMode() == SyncMode::SyncOnDemand )
        {
            devModeMetadata += "Sync-On-Demand";
        }
        else
        {
            devModeMetadata += "Full Sync";
        }
        devModeMetadata += "     Remaining Quota: " + std::to_string( gameSaveManager.GetRemainingQuotaInBytes() ) + " bytes";
        m_gameBoardMetadataFont->DrawString(spriteBatch.get(), devModeMetadata.c_str(), c_gameSaveAdditionalMetadata_UpperLeft, Colors::White, 0.0f, XMFLOAT2(0, 0), c_gameSaveMetadata_Scale);
    }
    else
    {
        m_gameBoardMetadataFont->DrawString(spriteBatch.get(), L"Sync Status: Not Initialized", c_gameSaveAdditionalMetadata_UpperLeft);
    }

    // Draw current score
    auto scoreColor = m_isOverLimitOnLetters ? Colors::Red : Colors::White;
    gameFont->DrawString(spriteBatch.get(), std::to_string( m_score ).c_str(), c_currentScore_UpperLeft, scoreColor, 0.0f, XMFLOAT2(0, 0), c_currentScore_Scale);
    m_gameBoardMetadataFont->DrawString(spriteBatch.get(), L"SCORE", c_currentScoreLabel_UpperLeft);

    // Draw remaining letters count
    if (m_lettersRemaining.size() >= 26)
    {
        XMFLOAT2 letterPosition = c_lettersRemaining_UpperLeft;
        for (char letter = 'A'; letter <= 'Z'; ++letter)
        {
            auto letterIndex = letter - 'A';
            auto lettersRemaining = m_lettersRemaining[size_t(letterIndex)];
            auto letterValue = c_letterValues[size_t(letterIndex)];
            std::string countStr;
            countStr += letter;
            countStr += "=";
            countStr += std::to_string( letterValue );
            countStr += "(";
            countStr += std::to_string( lettersRemaining );
            countStr += ")";

            auto countColor = c_lettersRemaining_Color;
            if (lettersRemaining < 0)
            {
                countColor = Colors::Red;
            }
            else if (lettersRemaining == 0)
            {
                countColor = Colors::Yellow;
            }
            m_gameBoardLettersRemainingFont->DrawString(spriteBatch.get(), countStr.c_str(), letterPosition, countColor, 0.0f, XMFLOAT2(0, 0), c_lettersRemaining_Scale);

            // Get next letter position
            if ((letterIndex + 1) % 9 == 0)
            {
                letterPosition.x = c_lettersRemaining_UpperLeft.x;
                letterPosition.y += c_lettersRemaining_LetterOffset.y;
            }
            else
            {
                letterPosition.x += c_lettersRemaining_LetterOffset.x;
            }
        }
    }

    // Draw help text for remaining letters section
    std::string lettersRemainingHelp( "Letter = X pts (# remaining)" );
    auto lettersRemainingHelpSize = m_gameBoardLettersRemainingFont->MeasureString(lettersRemainingHelp.c_str());
    XMFLOAT2 lettersRemainingHelpOrigin = XMFLOAT2(XMVectorGetX(lettersRemainingHelpSize) / 2.0f, m_gameBoardLettersRemainingFont->GetLineSpacing() / 2.0f);
    m_gameBoardLettersRemainingFont->DrawString(spriteBatch.get(), lettersRemainingHelp.c_str(), c_lettersRemainingHelp_Center, c_lettersRemaining_Color, 0.0f, lettersRemainingHelpOrigin);

    // Draw tile cursor
    if (!m_menuActive)
    {
        XMFLOAT2 cursorCenter = XMFLOAT2(c_letterTileFirstTile_Center.x + (c_letterTile_CenterOffset.x * m_cursorPosition.x), c_letterTileFirstTile_Center.y + (c_letterTile_CenterOffset.y * m_cursorPosition.y));
        spriteBatch->Draw(m_cursor.GetGpuHandle(), m_cursor->GetTextureSize(), cursorCenter, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_cursor->Width() / 2), float(m_cursor->Height() / 2)));
    }

    // Draw placed letters
    for (uint32_t j = 0; j < activeBoard.m_boardHeight; ++j)
    {
        for (uint32_t i = 0; i < activeBoard.m_boardWidth; ++i)
        {
            XMFLOAT2 tileCenter = XMFLOAT2(c_letterTileFirstTile_Center.x + (c_letterTile_CenterOffset.x * i), c_letterTileFirstTile_Center.y + (c_letterTile_CenterOffset.y * j));
            auto currentTile = activeBoard.GetGameTile(XMUINT2(i, j));

            if (currentTile.m_placed)
            {
                if (currentTile.m_letter)
                {
                    // draw letter (centered on tile)
                   char letter[ 2 ];
                   letter[ 0 ] = (char) ( currentTile.m_letter ); // NOTE: Hacky, but works for A-Z in ASCII range.
                   letter[ 1 ] = '\0';

                    XMVECTOR letterSize = m_gameBoardTileFont->MeasureString(letter);
                    XMFLOAT2 letterOrigin = XMFLOAT2(XMVectorGetX(letterSize) / 2.0f + 6.0f, XMVectorGetY(letterSize) / 2.0f);
                    if (letter[0] == 'Q')
                    {
                        letterOrigin.x += 2.0f; // correct some display issues
                    }
                    m_gameBoardTileFont->DrawString(spriteBatch.get(), letter, tileCenter, Colors::Pink, 0.0f, letterOrigin, 0.75f);

                    // draw letter value (top-left-justified, offset from tile center)
                    std::string letterValue = std::to_string( c_letterValues[ (size_t)( currentTile.m_letter - L'A') ]);
                    XMFLOAT2 letterValuePosition = XMFLOAT2(tileCenter.x + c_letterTileValue_OffsetFromTileCenter.x, tileCenter.y + c_letterTileValue_OffsetFromTileCenter.y);
                    //XMVECTOR letterValueSize = m_gameBoardTileValueFont->MeasureString( letterValue.c_str() );
                    float letterValueScale = 0.45f;
                    m_gameBoardTileValueFont->DrawString(spriteBatch.get(), letterValue.c_str(), letterValuePosition, Colors::Yellow, 0.0f, XMFLOAT2(0, 0), letterValueScale);
                }
                else
                {
                    // draw the non-alphabetic texture
                }
            }

            // draw word arrows
            auto trackerTile = GetWordTrackerTile(XMUINT2(i, j));
            if (trackerTile.m_wordRight)
            {
                XMFLOAT2 horizontalWordLinkerCenter = XMFLOAT2(c_wordLinkFirstHorizontal_Center.x + (c_letterTile_CenterOffset.x * i), c_wordLinkFirstHorizontal_Center.y + (c_letterTile_CenterOffset.y * j));
                spriteBatch->Draw(m_horizontalWordLinker.GetGpuHandle(), m_horizontalWordLinker->GetTextureSize(), horizontalWordLinkerCenter, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_horizontalWordLinker->Width() / 2), float(m_horizontalWordLinker->Height() / 2)));
            }
            if (trackerTile.m_wordDown)
            {
                XMFLOAT2 verticalWordLinkerCenter = XMFLOAT2(c_wordLinkFirstVertical_Center.x + (c_letterTile_CenterOffset.x * i), c_wordLinkFirstVertical_Center.y + (c_letterTile_CenterOffset.y * j));
                spriteBatch->Draw(m_verticalWordLinker.GetGpuHandle(), m_verticalWordLinker->GetTextureSize(), verticalWordLinkerCenter, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_verticalWordLinker->Width() / 2), float(m_verticalWordLinker->Height() / 2)));
            }
        }
    }

    // Draw input control help
    bool displayGamepadControls = true;

#if defined(RENDER_ARBITRARY_USER_CONTROLS) 
    // current Xbox game board background already has controls baked on to it
    // We're leaving this code here in case we port this again later to a platform with indeterminate
    // controls. GameInput provides a font and button labels/enumeration to help with this scenario.

    XMFLOAT2 controlHelpPosition = c_inputControlsHelp_UpperLeft;
    displayGamepadControls = InputState::IsAnyGamepadConnected();
    if (displayGamepadControls)
    {
        // draw save slot controls
        spriteBatch->Draw(m_saveSlotLeftControl.GetGpuHandle(), m_saveSlotLeftControl->GetTextureSize(), c_saveSlotLeftControlHelp_Center, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_saveSlotLeftControl->Width() / 2), float(m_saveSlotLeftControl->Height() / 2)));
        spriteBatch->Draw(m_saveSlotRightControl.GetGpuHandle(), m_saveSlotRightControl->GetTextureSize(), c_saveSlotRightControlHelp_Center, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_saveSlotRightControl->Width() / 2), float(m_saveSlotRightControl->Height() / 2)));

        // draw swap letter control
        spriteBatch->Draw(m_swapLetterControl.GetGpuHandle(), m_swapLetterControl->GetTextureSize(), controlHelpPosition);
        controlHelpPosition.x += float(m_swapLetterControl->Width());
        const char* helpText = " Swap Letter";
        m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), helpText, controlHelpPosition, Colors::SandyBrown, 0.0f, XMFLOAT2(0, 0), c_inputControlsHelp_Scale);
        controlHelpPosition.x += XMVectorGetX(m_gameBoardControlsHelpFont->MeasureString(helpText)) * c_inputControlsHelp_Scale + 40.f;

        // draw cursor movement control
        spriteBatch->Draw(m_moveCursorControl.GetGpuHandle(), m_moveCursorControl->GetTextureSize(), controlHelpPosition);
        controlHelpPosition.x += float(m_moveCursorControl->Width());
        helpText = " Move Cursor";
        m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), helpText, controlHelpPosition, Colors::SandyBrown, 0.0f, XMFLOAT2(0, 0), c_inputControlsHelp_Scale);
        controlHelpPosition.x += XMVectorGetX(m_gameBoardControlsHelpFont->MeasureString(helpText)) * c_inputControlsHelp_Scale + 40.f;

        // draw clear letter button
        spriteBatch->Draw(m_clearLetterControl.GetGpuHandle(), m_clearLetterControl->GetTextureSize(), controlHelpPosition);
        controlHelpPosition.x += float(m_clearLetterControl->Width());
        helpText = " Clfar Letter";
        m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), helpText, controlHelpPosition, Colors::SandyBrown, 0.0f, XMFLOAT2(0, 0), c_inputControlsHelp_Scale);
        controlHelpPosition.x += XMVectorGetX(m_gameBoardControlsHelpFont->MeasureString(helpText)) * c_inputControlsHelp_Scale + 40.f;

        // draw menu control
        spriteBatch->Draw( m_menuControl.GetGpuHandle(), m_menuControl->GetTextureSize(), controlHelpPosition );
        controlHelpPosition.x += float(m_menuControl->Width());
        helpText = " Cycle Commands List";
        m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), helpText, controlHelpPosition, Colors::SandyBrown, 0.0f, XMFLOAT2(0, 0), c_inputControlsHelp_Scale);
    }
    else
    {
        // draw save slot keys
        std::string helpText = "[1-9]: Select Save Slot";

        // draw cursor movement keys
        helpText += "   Arrows: Move Cursor";

        // draw letter keys
        helpText += "   [A-Z]: Place Letter";

        // draw clear letter key
        helpText += "   [Del/Spc]: Clear Letter";

        // draw log scroll keys
        helpText += "   [PgUp/PgDn]: Scroll Log";

        m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), helpText.c_str(), controlHelpPosition, Colors::SandyBrown, 0.0f, XMFLOAT2(0, 0), c_inputControlsHelp_Scale);
    }
#endif //defined(RENDER_ARBITRARY_USER_CONTROLS) 

    // Draw game debug log
    auto logPosition = XMFLOAT2(c_debugLog_Region.Left, c_debugLog_Region.Top);
    size_t maxDisplayLines = static_cast<size_t>(c_debugLog_Region.Height / m_logFontHeight);

    size_t logIndexSize;
    {
       logIndexSize = Log::GetDisplayLog_ScopedSharedLock()->size();
       // lock is released when it goes out of scope.
    }

    size_t logIndexBegin = size_t(0);
    size_t logIndexEnd = logIndexSize - 1;

    if (m_logLineBegin > 0)
    {
        // no auto-scrolling, display starting at requested line number
        logIndexBegin = m_logLineBegin - 1;
        logIndexEnd = std::min(logIndexEnd, logIndexBegin + maxDisplayLines - 1);
    }
    else if (logIndexSize > maxDisplayLines)
    {
        // auto-scroll
        logIndexBegin = logIndexSize - maxDisplayLines;
    }

    for (auto i = logIndexBegin; i <= logIndexEnd; ++i)
    {
       // Grab the shared-lock only long enough to get the log line string's text pointer.
       // (We assume that DrawString's runtime is large enough to want to hold the lock for as little time as possible).
       const char* logLine;
       {
          logLine = (*Log::GetDisplayLog_ScopedSharedLock())[i].c_str();
          // lock is released when it goes out of scope.
       }

        m_logFont->DrawString(spriteBatch.get(), logLine, logPosition);
        logPosition.y += m_logFontHeight;
    }

    // Draw scroll indicators
    if (logIndexBegin > 0)
    {
        if (displayGamepadControls)
        {
            spriteBatch->Draw(m_logScrollUpControl.GetGpuHandle(), m_logScrollUpControl->GetTextureSize(), c_debugLogScrollUpIcon_Center, nullptr,
               Colors::White, 0.0f, XMFLOAT2(float(m_logScrollUpControl->Width() / 2), float(m_logScrollUpControl->Height() / 2)));
        }
        else
        {
            const char* logScrollUpKeyString = "PgUp";
            XMVECTOR logScrollUpKeySize = m_gameBoardControlsHelpFont->MeasureString(logScrollUpKeyString);
            XMFLOAT2 logScrollUpKeyOrigin = XMFLOAT2(XMVectorGetX(logScrollUpKeySize) / 2.f, m_gameBoardControlsHelpFont->GetLineSpacing() / 2.f);
            m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), logScrollUpKeyString, c_debugLogScrollUpIcon_Center, Colors::SandyBrown, 0.0f, logScrollUpKeyOrigin, c_inputControlsHelp_Scale);
        }
    }
    if (m_logLineBegin > 0)
    {
        if (displayGamepadControls)
        {
            spriteBatch->Draw(m_logScrollDownControl.GetGpuHandle(), m_logScrollDownControl->GetTextureSize(), c_debugLogScrollDownIcon_Center, nullptr, Colors::White, 0.0f, XMFLOAT2(float(m_logScrollDownControl->Width() / 2), float(m_logScrollDownControl->Height() / 2)));
        }
        else
        {
            const char* logScrollDownKeyString = "PgDn";
            XMVECTOR logScrollDownKeySize = m_gameBoardControlsHelpFont->MeasureString(logScrollDownKeyString);
            XMFLOAT2 logScrollDownKeyOrigin = XMFLOAT2(XMVectorGetX(logScrollDownKeySize) / 2.f, m_gameBoardControlsHelpFont->GetLineSpacing() / 2.f);
            m_gameBoardControlsHelpFont->DrawString(spriteBatch.get(), logScrollDownKeyString, c_debugLogScrollDownIcon_Center, Colors::SandyBrown, 0.0f, logScrollDownKeyOrigin, c_inputControlsHelp_Scale);
        }
    }

    spriteBatch->End();
}

void GameBoardScreen::ExitScreen(bool immediate)
{
    if (!immediate)
    {
        g_Game->GetGameSaveManager().Reset();
    }

    MenuScreen::ExitScreen(immediate);
}

void GameBoardScreen::OnCancel()
{
#if !(defined(_XBOX_ONE) && defined(_TITLE)) && !defined(_GAMING_XBOX)
    return; // PC does not support user switching
#else
    if (g_Game->GetGameSaveManager().IsActiveBoardDirty())
    {
        int controllingPlayer = GetControllingPlayer();
        std::string exitConfirmationTitle = u8"Exit Without Saving?";
        std::string exitConfirmationMessage = u8"The current game board has not been saved.\nAre you sure you wish to exit?";
        ConfirmChoiceFn exitConfirmationFn = [this](bool exitWithoutSaving)
        {
            if (exitWithoutSaving)
            {
                Log::Write("ConfirmChoiceFn: Exit without saving confirmed\n");
                g_Game->GetStateManager().SwitchState(GameState::AcquireUser);
                ExitScreen();
            }
            else
            {
                Log::Write("ConfirmChoiceFn: User chose not to exit without saving\n");
            }
        };

        Manager().AddScreen(std::make_shared<ConfirmPopUpScreen>(Manager(), exitConfirmationTitle, exitConfirmationMessage, exitConfirmationFn), controllingPlayer);
    }
    else
    {
        g_Game->GetStateManager().SwitchState(GameState::AcquireUser);
        ExitScreen();
    }
#endif
}

void GameBoardScreen::ComputeMenuBounds(float /*viewportWidth*/, float /*viewportHeight*/)
{
    m_menuBounds = c_menu_Region;
}

int GameBoardScreen::GetWordScore(const std::wstring& word)
{
    auto wordLength = word.length();
    if (wordLength < 2)
    {
        return 0;
    }

    auto it = m_wordList->find(word);
    if (it == m_wordList->end())
    {
        return 0;
    }

    int score = 0;
    std::for_each(word.cbegin(), word.cend(), [&score](wchar_t letter)
    {
        score += c_letterValues[letter - L'A'];
    });

    return score;
}

GameBoardScreen::WordTrackerTile& GameBoardScreen::GetWordTrackerTile(const XMUINT2& position)
{
    size_t tile = size_t(position.x + (c_boardWidth * position.y));
    if (tile >= c_boardWidth * c_boardHeight)
    {
        throw std::invalid_argument("position is not valid for the current board size");
    }

    return m_wordTracker[tile];
}

void GameBoardScreen::TrackWord(bool isHorizontal, XMUINT2 startTile, size_t wordLength)
{
    if (isHorizontal)
    {
        auto lastX = startTile.x + wordLength - 2;
        for (auto i = startTile.x; i <= lastX; ++i)
        {
            auto& trackerTile = GetWordTrackerTile(startTile);
            trackerTile.m_wordRight = true;
            startTile.x++;
        }
    }
    else
    {
        auto lastY = startTile.y + wordLength - 2;
        for (auto i = startTile.y; i <= lastY; ++i)
        {
            auto& trackerTile = GetWordTrackerTile(startTile);
            trackerTile.m_wordDown = true;
            startTile.y++;
        }
    }
}

void GameBoardScreen::UpdateLettersRemaining()
{
    m_lettersRemaining.assign(c_letterCounts, c_letterCounts + sizeof(c_letterCounts)/sizeof(c_letterCounts[0]));

    auto gameBoard = g_Game->GetGameSaveManager().GetActiveBoard();
    for (uint32_t j = 0; j < gameBoard.m_boardHeight; ++j)
    {
        for (uint32_t i = 0; i < gameBoard.m_boardWidth; ++i)
        {
            auto currentTile = gameBoard.GetGameTile(XMUINT2(i, j));
            auto currentLetter = currentTile.m_letter;
            if (currentLetter != 0 && currentTile.m_placed)
            {
                m_lettersRemaining[size_t(currentLetter - L'A')]--;
            }
        }
    }

    // Has player used too many of any letter?
    m_isOverLimitOnLetters = false;
    for (auto it = m_lettersRemaining.cbegin(); it != m_lettersRemaining.cend(); ++it)
    {
        if (*it < 0)
        {
            m_isOverLimitOnLetters = true;
            break;
        }
    }

}

void GameBoardScreen::UpdateScore()
{
    assert(g_Game->GetGameSaveManager().HasActiveBoard());

    m_score = 0;
    if (m_wordList == nullptr || m_wordList->empty())
    {
        return;
    }

    auto gameBoard = g_Game->GetGameSaveManager().GetActiveBoard();
    int newScore = 0;
    std::wstring currentWord;

    // Reset word tracker
    auto boardTiles = c_boardWidth * c_boardHeight;
    for (uint32_t i = 0; i < boardTiles; ++i)
    {
        m_wordTracker[i].m_wordDown = false;
        m_wordTracker[i].m_wordRight = false;
    }

    // Score the horizontal words
    for (uint32_t j = 0; j < gameBoard.m_boardHeight; ++j)
    {
        for (uint32_t i = 0; i < gameBoard.m_boardWidth; ++i)
        {
            auto currentTile = gameBoard.GetGameTile(XMUINT2(i, j));
            auto currentLetter = currentTile.m_letter;
            if (currentLetter != 0 && currentTile.m_placed)
            {
                currentWord += currentLetter;
            }
            else
            {
                if (!currentWord.empty())
                {
                    auto wordScore = GetWordScore(currentWord);
                    if (wordScore > 0)
                    {
                        auto wordLength = uint32_t(currentWord.length());
                        TrackWord(true, XMUINT2(i - wordLength, j), wordLength);
                    }
                    newScore += wordScore;
                    currentWord.clear();
                }
            }
        }
        if (!currentWord.empty())
        {
            auto wordScore = GetWordScore(currentWord);
            if (wordScore > 0)
            {
                auto wordLength = uint32_t(currentWord.length());
                TrackWord(true, XMUINT2(gameBoard.m_boardWidth - wordLength, j), wordLength);
            }
            newScore += wordScore;
            currentWord.clear();
        }
    }

    // Score the vertical words
    for (uint32_t i = 0; i < gameBoard.m_boardWidth; ++i)
    {
        for (uint32_t j = 0; j < gameBoard.m_boardHeight; ++j)
        {
            auto currentTile = gameBoard.GetGameTile(XMUINT2(i, j));
            auto currentLetter = currentTile.m_letter;
            if (currentLetter != 0 && currentTile.m_placed)
            {
                currentWord += currentLetter;
            }
            else
            {
                if (!currentWord.empty())
                {
                    auto wordScore = GetWordScore(currentWord);
                    if (wordScore > 0)
                    {
                        auto wordLength = uint32_t(currentWord.length());
                        TrackWord(false, XMUINT2(i, j - wordLength), wordLength);
                    }
                    newScore += wordScore;
                    currentWord.clear();
                }
            }
        }
        if (!currentWord.empty())
        {
            auto wordScore = GetWordScore(currentWord);
            if (wordScore > 0)
            {
                auto wordLength = uint32_t(currentWord.length());
                TrackWord(false, XMUINT2(i, gameBoard.m_boardHeight - wordLength), wordLength);
            }
            newScore += wordScore;
            currentWord.clear();
        }
    }

    if (!m_isOverLimitOnLetters)
    {
        m_score = newScore;
    }
}

} // namespace GameSaveSample
