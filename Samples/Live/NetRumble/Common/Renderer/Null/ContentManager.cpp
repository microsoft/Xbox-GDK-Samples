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
}

void ContentManager::Initialize()
{
}

TextureHandle ContentManager::LoadTexture(const std::wstring &)
{
    return TextureHandle{ nullptr };
}

std::shared_ptr<DirectX::SpriteFont> ContentManager::LoadFont(const std::wstring &)
{
    return nullptr;
}