// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"

#include "XUserHandleWrapper.h"
#include "User.h"
#include "Assets.h"
#include "AcquireUserScreen.h"
#include "InputState.h"
#include "SampleGame.h"
#include "UTF8Helper.h"
#include "StateManager.h"

using namespace DirectX;

namespace GameSaveSample {

AcquireUserScreen::AcquireUserScreen(ScreenManager& screenManager, bool /*autoSignIn*/) : MenuScreen(screenManager),
    m_status(AcquireUserStatus::SigningIn)
{
    m_transitionOnTime = 1.0f;
    m_transitionOffTime = 0.0f;

    m_menuEntries.push_back(MenuEntry(u8"Play",
        [this](int /*selectingPlayer*/)
    {
        AcquireUser();
    }));

    m_menuEntries.push_back(MenuEntry(u8"Select Profile",
        [this](int /*selectingPlayer*/)
    {
        SwitchUser();
    }));
}

AcquireUserScreen::~AcquireUserScreen()
{
}

void AcquireUserScreen::LoadContent( ATG::AssetLoadResources& loadRes )
{
    MenuScreen::LoadContent( loadRes );

    m_backgroundTexture = ATG::GetTextureAsset( loadRes, AssetDescriptor::Blank);
    m_titleLogo = ATG::GetTextureAsset( loadRes, AssetDescriptor::TitleLogo );
}

void AcquireUserScreen::Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime)
{
    auto spriteBatch = Manager().GetSpriteBatch();
    auto blendStates = Manager().GetCommonStates();
    auto viewportBounds = Manager().GetScreenBounds();
    float viewportWidth = float(viewportBounds.right);
    float viewportHeight = float(viewportBounds.bottom);

    spriteBatch->Begin(commandList, SpriteSortMode_Deferred );

    // Draw the background
    RECT backgroundRectangle = { 0, 0, long(viewportWidth), long(viewportHeight) };
    spriteBatch->Draw( m_backgroundTexture.GetGpuHandle(), m_backgroundTexture->GetTextureSize(), backgroundRectangle, c_menuBackgroundColor );
    
    // Draw title logo
    XMFLOAT2 titleLogoPosition = XMFLOAT2(viewportWidth / 2.0f, (viewportHeight / 2.0f) - 200.f);
    XMFLOAT2 titleLogoOrigin = XMFLOAT2(m_titleLogo->Width() / 2.0f, m_titleLogo->Height() / 2.0f);
    spriteBatch->Draw( m_titleLogo.GetGpuHandle(), m_titleLogo->GetTextureSize(), titleLogoPosition, nullptr, Colors::White, 0.0f, titleLogoOrigin );

    spriteBatch->End();

    MenuScreen::Draw( commandList, totalTime, elapsedTime);
}

void AcquireUserScreen::HandleInput(const DirectX::InputState& input)
{
    if (m_status == AcquireUserStatus::Exiting)
    {
        return;
    }

    MenuScreen::HandleInput(input);
}

void AcquireUserScreen::OnCancel()
{
    // nothing to do... there's no backing out from here
}

void AcquireUserScreen::AcquireUser()
{
    Log::Write(u8"AcquireUserScreen::AcquireUser\n");

    auto currentUser = ATG::UserManager::GetCurrentUser();
    if ( !currentUser )
    {
       SelectUser();
    }
    else
    {
       assert(currentUser->IsValid() && "Current user not valid - this shouldn't happen");
       PrepareToExit();
    }
}

void AcquireUserScreen::SelectUser()
{
   std::function<ATG::UserManager::CurrentUserChangedCallbackFn> callback = std::bind( &AcquireUserScreen::OnCurrentUserChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
   ATG::UserManager::SubscribeCurrentUserChangedEvent( callback );

   if ( !ATG::UserManager::AskSystemForNewUser() )
   {
      // Failed because someone else is already acquiring a user...
      ATG::UserManager::UnsubscribeCurrentUserChangedEvent();
      DX::ThrowIfFailed( HRESULT_FROM_WIN32( ERROR_BUSY ) ); 
   }
}

void AcquireUserScreen::OnCurrentUserChanged( ATG::UserManager::CurrentUserChangedResult result, 
                                              std::shared_ptr<ATG::User>& /*unused: oldUser*/, 
                                              std::shared_ptr<ATG::User>& newUser )
{
   ATG::UserManager::UnsubscribeCurrentUserChangedEvent();
   
   if ( result == ATG::UserManager::CurrentUserChangedResult::Canceled )
   {
      Log::Write( u8"Profile Picker UI canceled by user" );
      return;
   }
   if ( result == ATG::UserManager::CurrentUserChangedResult::Error )
   {
      g_Game->GetStateManager().PopupError( "Could not select a user - an error occurred (see log)" );
   }
   else if ( result == ATG::UserManager::CurrentUserChangedResult::NoChange || !newUser )
   {
      // Nothing changes, so continue.
      return;
   }
   
   if ( !newUser->QueryHasGamePad() )
   {
      g_Game->GetStateManager().PopupError( u8"Could not pair controller to user" );
      return;
   }

   // Have a user with a controller, so now we can continue to the game.

   PrepareToExit();
}

void AcquireUserScreen::SwitchUser()
{
    Log::Write( u8"AcquireUserScreen::SwitchUser\n" );

    SelectUser();
}

void AcquireUserScreen::PrepareToExit()
{
    std::shared_ptr<ATG::User> currentUser = ATG::UserManager::GetCurrentUser();
    assert( currentUser && currentUser->IsValid() && "Need to have a valid user to exit this screen" );

    Log::WriteAndDisplay(u8"User set (%s, localId=%u)\n", FormatUserName(*currentUser).c_str(), 
       currentUser->GetLocalUserId() );

    ATG::GamePadUserBindingInfo gamePad;
    
    if ( !ATG::FindGamePadAndUserByUserId( currentUser->GetLocalUserId(), &gamePad ) )
    {
       // For some reason, even though we have the current user, we couldn't find them in the list.
       //assert( false && "Need to have a valid current user and gamepad to exit" );
       return;
    }

    g_Game->SetCurrentGamepad( gamePad.gamePad.Get(), gamePad.gamePadIndex );

    Log::WriteAndDisplay(u8"Set gamepad for current user (index = %d)\n", gamePad.gamePadIndex );

    ClearMenuEntries();
    m_menuEntries.push_back(MenuEntry(u8"Loading . . ."));
    m_status = AcquireUserStatus::Exiting;

    g_Game->GetStateManager().SwitchState(GameState::InitializeGameSaveSystem);
}

} // namespace GameSaveSample
