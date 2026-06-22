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

using namespace NetRumble;

ContentManager::ContentManager() noexcept : m_lastIndex(0)
{
}

ContentManager::~ContentManager() noexcept
{
    m_textures.clear();
}

void ContentManager::Initialize(const std::shared_ptr<DirectX::DescriptorPile>& descriptorPile)
{
    m_descriptors = descriptorPile;
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
        return TextureHandle{ itr->second.Resource, m_descriptors->GetGpuHandle(itr->second.Index) };
    }

    auto index = m_lastIndex++;
    auto texture = Managers::Get<RenderManager>()->LoadTexture(path.c_str(), m_descriptors->GetCpuHandle(index));

    m_textures[path] = Storage<DX::Texture>{ texture, index };

    //return std::make_shared<DX::Texture>(Managers::Get<RenderManager>()->GetD3DDevice(), path.c_str());
    return TextureHandle{ texture, m_descriptors->GetGpuHandle(index) };
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
        return itr->second.Resource;
    }

    // Otherwise use the WICTextureLoader APIs to load the texture into our cache
    //auto font = std::make_shared<SpriteFont>(Managers::Get<RenderManager>()->GetD3DDevice(), path.c_str());
    auto index = m_lastIndex++;
    auto font = Managers::Get<RenderManager>()->LoadFont(path.c_str(), m_descriptors->GetCpuHandle(index), m_descriptors->GetGpuHandle(index));
    m_fonts[path] = Storage<DirectX::SpriteFont>{ font, index };

    return font;
}
