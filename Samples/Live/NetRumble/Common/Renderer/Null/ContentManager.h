//--------------------------------------------------------------------------------------
// ContentManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RenderContext.h"

namespace ThunderRumble
{

class Manager;

struct Texture;

class ContentManager : public Manager
{
public:
    ContentManager();
    ~ContentManager();

    void Initialize();

    TextureHandle LoadTexture(const std::wstring &path);

    std::shared_ptr<DirectX::SpriteFont> LoadFont(const std::wstring &path);

private:
};

}
