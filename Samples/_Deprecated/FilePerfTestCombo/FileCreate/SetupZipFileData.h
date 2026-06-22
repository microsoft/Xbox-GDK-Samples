//--------------------------------------------------------------------------------------
// SetupZipFileData.h
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

class SetupZipFileData : public PerformSetup
{
protected:
    static const uint32_t NUM_DATA_DIRS = 5;
    static const uint64_t NUM_BYTES_PER_DIR = (2 * 1024 * 1024 * 1024ULL); // 2 gigs is the max file size

    static const uint64_t c_TOTAL_TO_READ_BYTES = c_ONE_MEG * c_NUM_MEGS_IN_GIG * 2ULL;

    static const uint32_t NUM_LOAD_SIZES = LAST_DATA_SIZE;
    static const uint64_t NUM_DATA_READS[NUM_LOAD_SIZES];        // how many reads to perform per run
    static const uint32_t SIZE_DATA_READS_BYTES[NUM_LOAD_SIZES];       // number of bytes to read per run

    //std::vector<BaseFileInterface::FileHandle> m_testFile;
    //std::vector<Packages::FileEntry*> m_files;
    std::vector<std::wstring> m_fileNames;

    std::vector<std::thread*> m_generateTestDataThreads;

    void GenerateTestDataJob(Packages::FlatPackage* layoutSet, const std::wstring filePrefix, uint32_t numberDataSets, LoadOrder order);
    void GenerateReadLocations(uint32_t numFiles, LoadOrder order, uint32_t* readLocationFile, uint32_t readSize);

    virtual void CleanOldFiles(const std::wstring& rootDirName);

public:
    SetupZipFileData();
    virtual ~SetupZipFileData();

    void PerformFullSetup(bool createPackedFile, uint32_t numIterations, uint32_t compressionRatio, const std::wstring& rootDirName, const std::wstring& fileNameBase);
    virtual std::unique_ptr<Packages::FlatPackage> GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix)
    {
        return GenerateLayoutFile(layoutFileName, dataFilePrefix, 0);
    }
    virtual std::unique_ptr<Packages::FlatPackage> GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix, size_t fileSize);
    virtual void GenerateFiles(Packages::FlatPackage* layout, const std::wstring& destDirName, uint32_t compressionRatio);
    virtual void GenerateTestData(Packages::FlatPackage* layout, uint32_t numIterations, const std::wstring& rootDirName);
};
