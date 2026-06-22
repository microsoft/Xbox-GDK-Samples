//--------------------------------------------------------------------------------------
// SetupFlatFileData.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <thread>
#include <vector>

#include "PerformSetup.h"
#include "FlatPackage.h"
#include "BaseFileInterface.h"

//#define LOTS_OF_FILES 1

class SetupFlatFileData : public PerformSetup
{
protected:
    static const uint32_t NUM_DATA_FILES = 5;

    static const uint64_t c_TOTAL_TO_READ_BYTES = c_ONE_MEG * c_NUM_MEGS_IN_GIG * 2ULL;
    static const uint32_t c_UINT_1k = 1024 / sizeof(uint32_t);

#ifndef LOTS_OF_FILES
    static const uint64_t ROOT_FILE_BLOCKS = NUM_DATA_FILES;
    static const uint64_t ROOT_BLOCK_SIZE_INTS = (2 * 1024 * 1024 * 256); // 2 gigs is the max file size
#else
    static const uint64_t ROOT_FILE_BLOCKS = 1024 * NUM_DATA_FILES;
    static const uint64_t ROOT_BLOCK_SIZE_INTS = (2 * 1024 * 256); // 2 megs is the max file size
#endif

    static const uint32_t NUM_LOAD_SIZES = LAST_DATA_SIZE;
    static const uint64_t NUM_DATA_READS[NUM_LOAD_SIZES];        // how many reads to perform per run
    static const uint32_t SIZE_DATA_READS_INTS[NUM_LOAD_SIZES];       // number of bytes to read per run
    static		 uint32_t SIZE_DATA_READS_BYTES[NUM_LOAD_SIZES];       // number of bytes to read per run

    std::vector<BaseFileInterface::FileHandle> m_testFile;
    std::vector<Packages::FileEntry*> m_files;

    std::vector<std::thread*> m_generateTestDataThreads;

    void GenerateTestDataJob(Packages::FlatPackage* layoutSet, const std::wstring filePrefix, uint32_t numberDataSets, LoadOrder order, DataSize dataSize, bool scarlett);
    void GenerateReadLocations(Packages::FlatPackage* layoutSet, LoadOrder order, DataSize dataSize, uint32_t* readLocationFile, uint64_t* readLocationBytes, uint32_t readAlignment);

    virtual void CleanOldFiles(const std::wstring& rootDirName);

public:
    SetupFlatFileData();
    virtual ~SetupFlatFileData();

    virtual std::unique_ptr<Packages::FlatPackage> GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix);
    virtual void GenerateFiles(Packages::FlatPackage* layout, const std::wstring& destDirName, uint32_t compressionRatio);
    virtual void GenerateTestData(Packages::FlatPackage* layout, uint32_t numIterations, const std::wstring& rootDirName);
};
