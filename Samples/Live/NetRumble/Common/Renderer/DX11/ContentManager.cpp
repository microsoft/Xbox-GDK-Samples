//--------------------------------------------------------------------------------------
// ContentManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Manager.h"
#include "ContentManager.h"
#include "Managers.h"

using namespace ThunderRumble;

ContentManager::ContentManager()
{
}

ContentManager::~ContentManager()
{
    m_textures.clear();
}

TextureHandle ContentManager::LoadTexture(const std::wstring &path)
{
    std::wstring lowerPath = path;
    // Lower case the path for our map
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](wchar_t wc) { return static_cast<wchar_t>(std::tolower(wc)); });

    // Look in our cache first
    auto itr = m_textures.find(path);
    if (itr != m_textures.end())
    {
        return itr->second;
    }

    return std::make_shared<DX::Texture>(Managers::Get<RenderManager>()->GetD3DDevice(), path.c_str());
}

std::shared_ptr<DirectX::SpriteFont> ContentManager::LoadFont(const std::wstring &path)
{
    std::wstring lowerPath = path;
    // Lower case the path for our map
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](wchar_t wc) { return static_cast<wchar_t>(std::tolower(wc)); });

    // Look in our cache first
    auto itr = m_fonts.find(path);
    if (itr != m_fonts.end())
    {
        return itr->second;
    }

    // Otherwise use the WICTextureLoader APIs to load the texture into our cache
    auto font = std::make_shared<DirectX::SpriteFont>(Managers::Get<RenderManager>()->GetD3DDevice(), path.c_str());
    m_fonts[path] = font;

    return font;
}