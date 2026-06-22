#pragma once

#define SET_NAME_TO_SELF(a) (a)->SetName(L#a) 

std::vector<uint8_t> LoadBGRAImage(const wchar_t* filename, uint32_t& width, uint32_t& height);

static constexpr UINT vendorIdAMD = 0x1002;
static constexpr UINT vendorIdNvidia = 0x10DE;
static constexpr UINT vendorIdIntel = 0x8086;
static constexpr UINT vendorIdQualcomm = 0x13B5;
inline bool IsAMD(UINT vendorId) { return vendorId == vendorIdAMD; }
inline bool IsNvidia(UINT vendorId) { return vendorId == vendorIdNvidia; }
inline bool IsIntel(UINT vendorId) { return vendorId == vendorIdIntel; }
inline bool IsQualcomm(UINT vendorId) { return vendorId == vendorIdQualcomm; }


