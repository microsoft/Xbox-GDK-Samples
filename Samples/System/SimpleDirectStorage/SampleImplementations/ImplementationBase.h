//--------------------------------------------------------------------------------------
// ImplementationBase.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <random>

#ifdef _GAMING_XBOX_SCARLETT
#include <dstorage_xs.h>
#else
#include <dstorage_x.h>
#endif

// Provides repeated functionality for each sample to avoid cluttering the main sample
class ImplementationBase
{
protected:

	static constexpr uint32_t c_queueCapacity = DSTORAGE_MIN_QUEUE_CAPACITY;
	static constexpr uint32_t c_minDataReadSize = 32 * 1024;			// recommended to read in 32k or greater sizes
	static constexpr uint32_t c_maxDataReadSize = 2 * 1024 * 1024;

	std::vector<std::unique_ptr<char>> m_destinationBuffers;
	std::vector<size_t> m_readLocations;
	std::vector<uint32_t> m_readSizes;
	//std::vector<RequestStatus> m_requestStatus;

	Microsoft::WRL::ComPtr<IDStorageFactoryX> m_factory;
	Microsoft::WRL::ComPtr <IDStorageFileX> m_file;
	Microsoft::WRL::ComPtr <IDStorageQueueX> m_queue;

	std::random_device randomDevice;
	std::default_random_engine randomEngine;

	void SetupDirectStorage(const std::wstring& fileName, uint16_t queueCapacity = c_queueCapacity);
	void ShutdownDirectStorage();

	void GenerateRandomReadLocationsAndDestinationBuffers(size_t dataFileSize, uint32_t numLocations, uint32_t readSizeOverride = 0);

public:
	ImplementationBase() :randomEngine(randomDevice()) {}
	virtual ~ImplementationBase() {}
};
