//--------------------------------------------------------------------------------------
// TextureFiles.h
//
// Methods for generating texture content filenames and checking for their presence in
// storage.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>

void LocateTextures();
UINT32 GetSceneTextureCount();
const WCHAR* GetTextureBasePath();
void SetTextureBasePath(const WCHAR* strPath);

enum TextureSuffix
{
    TextureSuffix_Diffuse = 0,
    TextureSuffix_Normal,
    TextureSuffix_Specular
};
void CreateTextureFilename(WCHAR* strOutput, SIZE_T OutputSizeChars, UINT32 TextureIndex, TextureSuffix Suffix, bool IsDataFile);
