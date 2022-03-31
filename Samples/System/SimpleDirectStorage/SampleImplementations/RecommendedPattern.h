//--------------------------------------------------------------------------------------
// RecommendedPattern.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// DirectStorage supports two main methods of notification, status block and ID3D12Fence
// This sample shows how to use a status block for notification
class RecommendedPattern : public ImplementationBase
{
private:
	static constexpr uint32_t c_dataBatches = 20;
	static constexpr uint32_t c_readsPerBatch = 50;

	Microsoft::WRL::ComPtr <IDStorageStatusArrayX> m_statusEntries;

public:
	RecommendedPattern() {}
	~RecommendedPattern() {}

	bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};
