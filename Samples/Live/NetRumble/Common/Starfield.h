//--------------------------------------------------------------------------------------
// Starfield.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{

class Starfield
{
public:
    Starfield(DirectX::SimpleMath::Vector2 position);
    ~Starfield();

    void Reset(DirectX::SimpleMath::Vector2 position);
    void Draw(DirectX::SimpleMath::Vector2 position);

private:
    static constexpr int numberOfStars = 256;

    DirectX::SimpleMath::Vector2 m_lastPosition;
    DirectX::SimpleMath::Vector2 m_position;
    std::array<DirectX::SimpleMath::Vector2, numberOfStars> m_stars;
    TextureHandle m_starTexture;
};

}
