//--------------------------------------------------------------------------------------
// PerformSetup.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <thread>
#include <vector>

#include "FlatPackage.h"
#include "BaseFileInterface.h"

class PerformSetup
{
protected:
    static const uint32_t s_maxDataReads = 1310720 * 2;  //65536 * 4;

    static const uint64_t c_ONE_MEG = 1024 * 1024;
    static const uint64_t c_NUM_MEGS_IN_GIG = 1024;

    virtual void CleanOldFiles(const std::wstring& rootDirName) = 0;

public:
    PerformSetup();
    virtual ~PerformSetup();

    virtual void PerformFullSetup(bool createPackedFile, uint32_t numIterations, uint32_t compressionRatio, const std::wstring& rootDirName, const std::wstring& fileNameBase);
    virtual std::unique_ptr<Packages::FlatPackage> GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix) = 0;
    virtual void GenerateFiles(Packages::FlatPackage* layout, const std::wstring& destDirName, uint32_t compressionRatio = 0) = 0;
    virtual void GeneratePackedFile(Packages::FlatPackage* layout, const std::wstring& packFileName, const std::wstring& rootDirName, bool zipped = false);
    virtual void GenerateTestData(Packages::FlatPackage* layout, uint32_t numIterations, const std::wstring& rootDirName) = 0;
};
