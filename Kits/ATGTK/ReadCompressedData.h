//--------------------------------------------------------------------------------------
// File: ReadCompressedData.h
//
// Helper for loading binary data files from disk compressed with the 'xbcompress' tool.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <vector>

#ifdef _GAMING_DESKTOP
#pragma comment(lib,"cabinet.lib")
#endif


namespace DX
{
    std::vector<uint8_t> ReadCompressedData(_In_z_ const wchar_t* name);
}
