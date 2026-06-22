//--------------------------------------------------------------------------------------
// PerformSetup.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PerformSetup.h"

#include <direct.h>

#include "Random.h"
#include "PackedFileCreator.h"
#include "LooseFiles.h"
#include "ReadSet.h"
#include "CapturedReadDataXML.h"
#include "FlatPackage.h"

PerformSetup::PerformSetup()
{
}

PerformSetup::~PerformSetup()
{
}

void PerformSetup::PerformFullSetup(bool createPackedFile, uint32_t numIterations, uint32_t /*compressionRatio*/, const std::wstring& rootDirName, const std::wstring& fileNameBase)
{
    CleanOldFiles(rootDirName);

    std::filesystem::path layoutFileName;
    _mkdir("LayoutSets");
    layoutFileName = L"LayoutSets";
    layoutFileName /= rootDirName;
    layoutFileName += L".xml";

    std::unique_ptr<Packages::FlatPackage> layout = GenerateLayoutFile(layoutFileName, fileNameBase);
    GenerateFiles(layout.get(), rootDirName);
    if (createPackedFile)
    {
        std::wstring packedFileName(rootDirName);
        packedFileName += L".packed";
        GeneratePackedFile(layout.get(), packedFileName, rootDirName);
    }
    GenerateTestData(layout.get(), numIterations, rootDirName);
}

void PerformSetup::GeneratePackedFile(Packages::FlatPackage* layout, const std::wstring& packFileName, const std::wstring& rootDirName, bool zipped)
{
    PackedFileCreator packedFile;
    packedFile.CreatePackedFile(packFileName, layout, rootDirName, zipped);
}
