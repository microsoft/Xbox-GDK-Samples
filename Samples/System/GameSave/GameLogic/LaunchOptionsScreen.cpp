// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "pch.h"

#include "LaunchOptionsScreen.h"
#include "Assets.h"
#include "GameSaveManager.h"
#include "SampleGame.h"

using namespace DirectX;

namespace GameSaveSample {

LaunchOptionsScreen::LaunchOptionsScreen(ScreenManager& screenManager) :
    MenuScreen(screenManager)
{
    m_menuEntries.push_back(MenuEntry("Use Full Sync Context",
        [](int)
    {
        g_Game->GetGameSaveManager().SetSyncMode( SyncMode::FullSyncAlways );
        g_Game->GetStateManager().SwitchState(GameState::AcquireUser);
    }));

    m_menuEntries.push_back(MenuEntry("Use Sync-on-Demand Context",
        [](int)
    {
        g_Game->GetGameSaveManager().SetSyncMode( SyncMode::SyncOnDemand );
        g_Game->GetStateManager().SwitchState(GameState::AcquireUser);
    }));
}

LaunchOptionsScreen::~LaunchOptionsScreen()
{
}

void LaunchOptionsScreen::LoadContent( ATG::AssetLoadResources& loadRes )
{
    MenuScreen::LoadContent( loadRes );
    m_backgroundTexture = ATG::GetTextureAsset( loadRes, AssetDescriptor::Blank );
    m_titleLogo = ATG::GetTextureAsset( loadRes, AssetDescriptor::TitleLogo );
}

void LaunchOptionsScreen::Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime)
{
    auto spriteBatch = Manager().GetSpriteBatch();
    auto blendStates = Manager().GetCommonStates();
    
    auto viewportBounds = Manager().GetScreenBounds();
    float viewportWidth = float(viewportBounds.right);
    float viewportHeight = float(viewportBounds.bottom);
    
    CD3DX12_VIEWPORT viewport( 0.0f, 0.0f, viewportWidth, viewportHeight );
    spriteBatch->SetViewport( viewport );

    auto scaleMatrix = ATG::GetScaleMatrixForWindow(Manager().GetWindowBounds());

    spriteBatch->Begin(commandList, SpriteSortMode_Deferred, scaleMatrix);

    // Draw the background
    RECT backgroundRectangle = { 0, 0, long(viewportWidth), long(viewportHeight) };
    
    spriteBatch->Draw( m_backgroundTexture.GetGpuHandle(), m_backgroundTexture->GetTextureSize(), backgroundRectangle, c_menuBackgroundColor );

    // Draw title logo
    XMFLOAT2 titleLogoPosition = XMFLOAT2(viewportWidth / 2.0f, (viewportHeight / 2.0f) - 200.f);
    XMFLOAT2 titleLogoOrigin = XMFLOAT2(m_titleLogo->Width() / 2.0f, m_titleLogo->Height() / 2.0f);
    spriteBatch->Draw( m_titleLogo.GetGpuHandle(), m_titleLogo->GetTextureSize(), titleLogoPosition, nullptr, Colors::White, 0.0f, titleLogoOrigin );

    spriteBatch->End();

    MenuScreen::Draw(commandList, totalTime, elapsedTime);
}

void LaunchOptionsScreen::OnCancel()
{
    // nowhere to go back to from here
}
} // namespace GameSaveSample
