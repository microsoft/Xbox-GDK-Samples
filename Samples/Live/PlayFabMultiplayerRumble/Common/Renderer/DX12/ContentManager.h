//--------------------------------------------------------------------------------------
// ContentManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DescriptorHeap.h"

namespace PlayFabMultiplayerRumble
{

struct TextureHandle;

class ContentManager : public IManager
{
public:
    ContentManager() noexcept;
    ~ContentManager() noexcept;

    void Initialize(const std::shared_ptr<DirectX::DescriptorPile>& descriptorPile);

    TextureHandle LoadTexture(const std::wstring &path);

    std::shared_ptr<DirectX::SpriteFont> LoadFont(const std::wstring &path);

private:

    template<typename ResourceType>
    struct Storage
    {
        std::shared_ptr<ResourceType> Resource;
        DirectX::DescriptorPile::IndexType Index = 0;
    };

    DirectX::DescriptorPile::IndexType m_lastIndex;

    std::map<std::wstring, Storage<DX::Texture>> m_textures;
    std::map<std::wstring, Storage<DirectX::SpriteFont>> m_fonts;

    std::shared_ptr<DirectX::DescriptorPile> m_descriptors;
};

}
