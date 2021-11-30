// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"

#include "Texture.h"
#include "Assets.h"
#include "ErrorPopUpScreen.h"
#include "InputState.h"
#include "ScreenManager.h"

using namespace DirectX;

namespace
{
   const XMVECTORF32 BackgroundColor = Colors::DarkSlateGray;
}

namespace GameSaveSample {

   ErrorPopUpScreen::ErrorPopUpScreen( ScreenManager& screenManager, const std::string& message ) : 
      GameScreen( screenManager ),
      m_errorMessage( message )
   {
      m_exitWhenHidden = false;
      m_isPopup = true;
   }

   ErrorPopUpScreen::~ErrorPopUpScreen( void )
   {
      UnloadContent();
   }

   void ErrorPopUpScreen::LoadContent( ATG::AssetLoadResources& loadRes )
   {
      m_backgroundTexture = ATG::GetTextureAsset( loadRes, AssetDescriptor::Blank );
   }

   void ErrorPopUpScreen::UnloadContent()
   {
      m_backgroundTexture.Unload();
   }

   void ErrorPopUpScreen::HandleInput( const DirectX::InputState& inputState )
   {
      if ( inputState.IsMenuSelect( -1, nullptr ) )
         ExitScreen();
   }

   void ErrorPopUpScreen::Draw( ID3D12GraphicsCommandList* commandList, float /*totalTime*/, float /*elapsedTime*/ )
   {
      auto& screenManager = Manager();
      auto spriteBatch = screenManager.GetSpriteBatch();
      auto spriteFont = screenManager.GetSpriteFont();
      auto blendStates = screenManager.GetCommonStates();
      auto viewportBounds = screenManager.GetScreenBounds();
      float viewportWidth = float( viewportBounds.right );
      float viewportHeight = float( viewportBounds.bottom );
      auto scaleMatrix = ATG::GetScaleMatrixForWindow( screenManager.GetWindowBounds() );

      // calculate position and size of error message
      XMFLOAT2 errorMsgPosition = XMFLOAT2( 0, viewportHeight / 2.0f );
      XMVECTORF32 errorMsgColor = Colors::Yellow;
      XMFLOAT2 origin = XMFLOAT2( 0, spriteFont->GetLineSpacing() / 2.0f );
      XMVECTOR size = spriteFont->MeasureString( m_errorMessage.c_str() );
      errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX( size ) / 2.0f;

      // create a rectangle representing the screen dimensions of the error message background rectangle
      long rectangleWidth = long( std::min( std::max( XMVectorGetX( size ) + 100.0f, 600.0f ), viewportWidth ) );
      long rectangleHeight = long( spriteFont->GetLineSpacing() * 6.0f );
      long rectangleLeft = long( viewportWidth / 2.0f ) - ( rectangleWidth / 2 );
      long rectangleTop = long( errorMsgPosition.y + spriteFont->GetLineSpacing() ) - ( rectangleHeight / 2 );
      RECT backgroundRectangle = { rectangleLeft, rectangleTop, rectangleLeft + rectangleWidth, rectangleTop + rectangleHeight };

      spriteBatch->Begin( commandList, DirectX::SpriteSortMode_Deferred, scaleMatrix );

      // draw a background color for the rectangle
      spriteBatch->Draw( m_backgroundTexture.GetGpuHandle(), m_backgroundTexture->GetTextureSize(), backgroundRectangle, BackgroundColor );

      // draw error message in the middle of the screen
      spriteFont->DrawString( spriteBatch.get(), m_errorMessage.c_str(), errorMsgPosition, errorMsgColor, 0, origin );

      // draw continuation prompt
      const char* continuePrompt = u8"Press (A) to Continue";

      errorMsgPosition.y += spriteFont->GetLineSpacing();
      size = spriteFont->MeasureString( continuePrompt );
      errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX( size ) / 2.0f;
      spriteFont->DrawString( spriteBatch.get(), continuePrompt, errorMsgPosition, Colors::Yellow, 0, origin );

      spriteBatch->End();
   }
} // namespace GameSaveSample
