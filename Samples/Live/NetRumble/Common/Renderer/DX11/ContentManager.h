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

class ContentManager : public Manager
{
public:
    ContentManager();
    ~ContentManager();

    TextureHandle LoadTexture(const std::wstring &path);

    std::shared_ptr<DirectX::SpriteFont> LoadFont(const std::wstring &path);

private:
    std::map<std::wstring, std::shared_ptr<DX::Texture>> m_textures;
    std::map<std::wstring, std::shared_ptr<DirectX::SpriteFont>> m_fonts;
};

}
