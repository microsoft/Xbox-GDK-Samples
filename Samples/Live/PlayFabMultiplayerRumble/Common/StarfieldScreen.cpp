//--------------------------------------------------------------------------------------
// StarfieldScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Managers.h"
#include "ScreenManager.h"
#include "StarfieldScreen.h"
#include "Starfield.h"
#include "Game.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

static const float starsParallaxPeriod = 30.0f;
static const float starsParallaxAmplitude = 2048.0f;

StarfieldScreen::StarfieldScreen() : GameScreen(),
    m_starfield(nullptr), 
    m_movement(0.0f)
{
    m_transitionPosition = 0.0f;
}

void StarfieldScreen::LoadContent()
{
    m_title = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\title.png");

    m_movement = 0.0f;

    SimpleMath::Vector2 starfieldPosition;
    starfieldPosition.x = cos(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;
    starfieldPosition.y = sin(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;

    m_starfield = std::make_unique<Starfield>(starfieldPosition);

    Reset();
}

void StarfieldScreen::Reset()
{
    if (m_starfield)
    {
        m_movement = 0.0f;

        SimpleMath::Vector2 starfieldPosition;
        starfieldPosition.x = cos(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;
        starfieldPosition.y = sin(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;

        m_starfield->Reset(starfieldPosition);
    }
}

// Updates the background screen. Unlike most screens, this should not transition off even if it has been covered by another screen: it is
// supposed to be covered, after all! This overload forces the coveredByOtherScreen parameter to false in order to stop the base
// Update method wanting to transition off.
void StarfieldScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
    UNREFERENCED_PARAMETER(coveredByOtherScreen);
    GameScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, false);

}

void StarfieldScreen::Draw(float totalTime, float elapsedTime)
{
    UNREFERENCED_PARAMETER(totalTime);

    if (m_starfield)
    {
        // update the parallax m_movement
        m_movement += elapsedTime;
        SimpleMath::Vector2 starfieldPosition;
        starfieldPosition.x = cos(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;
        starfieldPosition.y = sin(m_movement / starsParallaxPeriod) * starsParallaxAmplitude;

        // draw the stars
        m_starfield->Draw(starfieldPosition);
    }

    // draw the title texture
    if (m_title.Texture)
    {
        auto renderManager = Managers::Get<RenderManager>();
        auto renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);

        float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
        float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
        float scale = GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
        SimpleMath::Vector2 titlePosition = SimpleMath::Vector2(viewportWidth / 2.0f, (viewportHeight / 2.0f) - (185.f * scale));
        titlePosition.y -= powf(TransitionPosition(), 2) * titlePosition.y;
        XMVECTORF32 color = { 1.0f, 1.0f, 1.f, TransitionAlpha() };

        renderContext->Begin();
        renderContext->Draw(m_title, titlePosition, 0, scale, color);
        renderContext->End();
    }
}
