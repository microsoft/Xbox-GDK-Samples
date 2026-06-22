//--------------------------------------------------------------------------------------
// Starfield.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Managers.h"
#include "Starfield.h"
#include "Game.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

namespace
{

    inline XMVECTOR MakeVectorRegister(float X, float Y, float Z, float W)
    {
#if defined(_XM_NO_INTRINSICS_)
        XMVECTORF32 toret = { {{X,Y,Z,W}} };
        return toret;
#elif defined (_M_ARM64)
        XMVECTORF32 toret = { {{X,Y,Z,W}} };
        return toret;
#else
        return _mm_setr_ps(X, Y, Z, W);
#endif
    }

    constexpr float c_maximumMovementPerUpdate = 128.0f;
    constexpr size_t c_numberOfLayers = 8;
    constexpr int c_starSize = 2;

    const XMVECTOR backgroundColor = MakeVectorRegister(0, 0, 16.0f / 255.0f, 1.0f);

    const XMVECTOR LayerColor(float a)
    {
        return MakeVectorRegister(1.0f, 1.0f, 1.0f, a / 255.0f);
    }
    std::array<XMVECTOR, c_numberOfLayers> layerColors{
        LayerColor(255.0f),
        LayerColor(216.0f),
        LayerColor(192.0f),
        LayerColor(160.0f),
        LayerColor(128.0f),
        LayerColor(96.0f),
        LayerColor(64.0f),
        LayerColor(32.0f)
    };

    std::array<int, c_numberOfLayers> layerSizes;

    constexpr std::array<float, c_numberOfLayers> movementFactors = { 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f };
}

Starfield::Starfield(DirectX::SimpleMath::Vector2 position) :
    m_lastPosition(position),
    m_position(position)
{
    for (size_t i = 0; i < m_stars.size(); i++)
    {
        m_stars[i] = SimpleMath::Vector2(0, 0);
    }

    // load the star texture
    m_starTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\Blank.png");
}

Starfield::~Starfield()
{
}

void Starfield::Reset(DirectX::SimpleMath::Vector2 position)
{
    // recreate the stars
    int viewportWidth = g_game->GetWindowWidth();
    int viewportHeight = g_game->GetWindowHeight();

    for (size_t i = 0; i < m_stars.size(); i++)
    {
        m_stars[i] = SimpleMath::Vector2(static_cast<float>(rand() % viewportWidth), static_cast<float>(rand() % viewportHeight));
    }
    
    // reset the position
    m_lastPosition = m_position = position;
}

void Starfield::Draw(DirectX::SimpleMath::Vector2 position)
{
    auto renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);

    // update the current position
    m_lastPosition = m_position;
    m_position = position;

    // determine the movement vector of the stars
    // -- for the purposes of the parallax effect, 
    //    this is the opposite direction as the position movement.
    SimpleMath::Vector2 movement = m_lastPosition - position;

    // create a rectangle representing the screen dimensions of the starfield
    int viewportWidth = g_game->GetWindowWidth();
    int viewportHeight = g_game->GetWindowHeight();
    RECT starfieldRectangle = { 0, 0, viewportWidth, viewportHeight };

    //// draw a background color for the starfield
    renderContext->Begin();
    renderContext->Draw(m_starTexture, starfieldRectangle, backgroundColor);
    renderContext->End();


    // if we've moved too far, then reset, as the stars will be moving too fast
    if (movement.LengthSquared() > c_maximumMovementPerUpdate * c_maximumMovementPerUpdate)
    {
        Reset(position);
        return;
    }

    // draw all of the stars
    renderContext->Begin();

    for (size_t i = 0; i < m_stars.size(); i++)
    {
        // move the star based on the depth
        size_t depth = i % movementFactors.size();

        SimpleMath::Vector2 p = m_stars[i];
        p += movement * movementFactors[depth];

        // wrap the stars around
        if (p.x < 0 - layerSizes[depth])
        {
            p.x = static_cast<float>(viewportWidth);
            p.y = static_cast<float>(rand() % viewportHeight);
        }
        if (p.x > viewportWidth)
        {
            p.x = static_cast<float>(0 - layerSizes[depth]);
            p.y = static_cast<float>(rand() % viewportHeight);
        }
        if (p.y < 0 - layerSizes[depth])
        {
            p.x = static_cast<float>(rand() % viewportWidth);
            p.y = static_cast<float>(viewportHeight);
        }
        if (p.y > viewportHeight)
        {
            p.x = static_cast<float>(rand() % viewportWidth);
            p.y = static_cast<float>(0 - layerSizes[depth]);
        }

        m_stars[i] = p;

        // draw the star
        RECT b = { (int)p.x, (int)p.y, (int)p.x + c_starSize, (int)p.y + c_starSize };
        renderContext->Draw(m_starTexture, b, layerColors[depth]);
    }

    renderContext->End();
}
