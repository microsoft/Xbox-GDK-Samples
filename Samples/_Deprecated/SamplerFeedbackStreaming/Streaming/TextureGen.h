//--------------------------------------------------------------------------------------
// TextureGen.h
//
// Methods for generating large mipmapped textures and saving them to temporary storage.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

bool GenerateTextures(UINT32 Count, UINT32 Dimension, ID3D12Device* pd3dDevice);
bool IsTextureGenerationComplete(UINT32* pCompleted, UINT32* pTotal, UINT64* pBytes);
