//--------------------------------------------------------------------------------------
// SetupZipFileData.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SetupZipFileData.h"

#include <shellapi.h>
#include <direct.h>

#include "Random.h"
#include "PackedFile.h"
#include "LooseFiles.h"
#include "ReadSet.h"
#include "CapturedReadDataXML.h"

const uint32_t SetupZipFileData::SIZE_DATA_READS_BYTES[SetupZipFileData::NUM_LOAD_SIZES] = {
    0 * 1024 ,				//size_8k
    0 * 1024 ,				//size_12k
    16 * 1024 ,				//size_16k
    32 * 1024 ,            //size_32k
    64 * 1024 ,            //size_64k
    128 * 1024 ,           //size_128k
    0 * 1024 ,           //size_192k
    256 * 1024 ,           //size_256k
    512 * 1024 ,           //size_512k
    1024 * 1024 ,          //size_1024k
    2048 * 1024 ,				//size_2048k
    4096 * 1024 ,				//size_4096k
        0 * 1024 ,          //size_8192k,
        0 * 1024 ,          //size_16386k,
        0 * 1024 ,          //size_32768k,
};

SetupZipFileData::SetupZipFileData()
{
}

SetupZipFileData::~SetupZipFileData()
{
}

void SetupZipFileData::CleanOldFiles(const std::wstring& rootDirName)
{
    wchar_t currentWorkingDirectory[2048];
    GetCurrentDirectoryW(2048, currentWorkingDirectory);
    std::filesystem::path workingPath(currentWorkingDirectory);

    for (uint32_t dataSize = FIRST_DATA_SIZE; dataSize < LAST_DATA_SIZE; dataSize++)
    {
        if (SIZE_DATA_READS_BYTES[dataSize] != 0)
        {
            std::wstring newName(rootDirName);
            newName += L"_";
            newName += ConvertDataSizeToString((DataSize)dataSize).c_str();

            std::wstring fileName(newName);
            fileName += L".packed";
            DeleteFile(fileName.c_str());

            fileName = newName;
            fileName += L"_graph.packed";
            DeleteFile(fileName.c_str());

            workingPath = currentWorkingDirectory;
            workingPath /= newName;
            std::wstring fullPath = workingPath.wstring();
            SHFILEOPSTRUCT whichFunc;
            wchar_t clearDirName[1024] = {};
            wcscpy_s(clearDirName, 1024, fullPath.c_str());
            clearDirName[fullPath.size()] = 0;
            clearDirName[fullPath.size() + 1] = 0;
            memset(&whichFunc, 0, sizeof(whichFunc));
            whichFunc.wFunc = FO_DELETE;
            whichFunc.pFrom = clearDirName;
            whichFunc.fFlags = FOF_NO_UI;

            SHFileOperation(&whichFunc);
        }
    }
}

void SetupZipFileData::PerformFullSetup(bool createPackedFile, uint32_t numIterations, uint32_t compressionRatio, const std::wstring& rootDirName, const std::wstring& fileNameBase)
{
    CleanOldFiles(rootDirName);

    _mkdir("LayoutSets");
    for (uint32_t dataSize = FIRST_DATA_SIZE; dataSize < LAST_DATA_SIZE; dataSize++)
    {
        if (SIZE_DATA_READS_BYTES[dataSize] != 0)
        {
            std::filesystem::path layoutFileName;
            layoutFileName = L"LayoutSets";
            layoutFileName /= rootDirName;
            layoutFileName += L"_";
            layoutFileName += ConvertDataSizeToString((DataSize)dataSize).c_str();
            layoutFileName += L".xml";

            std::unique_ptr<Packages::FlatPackage> layout = GenerateLayoutFile(layoutFileName, fileNameBase, SIZE_DATA_READS_BYTES[dataSize]);

            std::wstring newName(rootDirName);
            newName += L"_";
            newName += ConvertDataSizeToString((DataSize)dataSize).c_str();
            layout->SetLayoutName(newName);

            GenerateFiles(layout.get(), newName, compressionRatio);
            if (createPackedFile)
            {
                std::wstring packedFileName(newName);
                packedFileName += L".packed";
                GeneratePackedFile(layout.get(), packedFileName, newName, true);
            }
            GenerateTestData(layout.get(), numIterations, newName);
        }
    }
}

std::unique_ptr<Packages::FlatPackage> SetupZipFileData::GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix, size_t fileSize)
{
    if (fileSize == 0)
        return nullptr;

    std::unique_ptr<Packages::FlatPackage> layout(new Packages::FlatPackage());

    Packages::BaseSubPackage* dataChunk = layout->AddChunk(L"data_chunk");
    for (uint32_t i = 0; i < NUM_DATA_DIRS; i++)
    {
        wchar_t dirName[256];
        swprintf_s(dirName, 256, L"data_dir_%d", i);
        layout->AddDirectory(dirName, dataChunk);
        size_t numFiles = NUM_BYTES_PER_DIR / fileSize;
        for (uint32_t j = 0; j < numFiles; j++)
        {
            wchar_t fileName[256];
            swprintf_s(fileName, 256, L"%s_%d.data", dataFilePrefix.c_str(), j);

            layout->AddFile(fileName, dirName, fileSize, Packages::fileType_data, dataChunk);
        }
    }
    layout->SaveDataFile(layoutFileName, L"");
    return layout;
}

void SetupZipFileData::GenerateFiles(Packages::FlatPackage* layout, const std::wstring& destDirName, uint32_t compressionRatio)
{
    layout->CreateMatchingFileLayout(destDirName, compressionRatio);
}

void SetupZipFileData::GenerateTestData(Packages::FlatPackage* layout, uint32_t numIterations, const std::wstring& rootDirName)
{
    std::unique_ptr <BaseFileInterface> fileInterface(nullptr);
    fileInterface = std::make_unique<LooseFiles>();
    fileInterface->SetDirectoryOffset(rootDirName);

    //m_testFile.reserve(layout->NumFiles());
    //m_files.reserve(layout->NumFiles());
    m_fileNames.clear();
    for (uint32_t i = Packages::FIRST_DATA_FILE_TYPE; i <= Packages::LAST_DATA_FILE_TYPE; i++)
    {
        if (i == Packages::fileType_null)
            continue;
        uint32_t numFiles = (uint32_t)layout->NumFiles((Packages::DataFileType)i);
        for (uint32_t j = 0; j < numFiles; j++)
        {
            Packages::FileEntry* file = layout->GetFileEntry((Packages::DataFileType)i, j);
            m_fileNames.push_back(file->fullName);
        }
    }

    for (uint32_t dataOrder = FIRST_LOAD_ORDER; dataOrder <= LAST_PERF_ORDER; dataOrder++)
    {
        std::thread* newThread = new std::thread(&SetupZipFileData::GenerateTestDataJob, this, layout, layout->GetLayoutName(), numIterations, (LoadOrder)dataOrder);
        m_generateTestDataThreads.push_back(newThread);
    }

    for (auto& iter : m_generateTestDataThreads)
    {
        iter->join();
        delete iter;
    }
    m_generateTestDataThreads.clear();
}

void SetupZipFileData::GenerateTestDataJob(Packages::FlatPackage* layoutSet, const std::wstring filePrefix, uint32_t numberDataSets, LoadOrder order)
{
    uint32_t readSize(0);
    wchar_t readSetFileName[256];
    swprintf(readSetFileName, 256, L"%s_%s.xml", filePrefix.c_str(), ConvertLoadOrderToString(order).c_str());

    for (uint32_t i = Packages::FIRST_DATA_FILE_TYPE; i <= Packages::LAST_DATA_FILE_TYPE; i++)
    {
        if (i == Packages::fileType_null)
            continue;
        uint32_t numFiles = (uint32_t)layoutSet->NumFiles((Packages::DataFileType)i);
        if (numFiles != 0)
        {
            Packages::FileEntry* fileEntry = layoutSet->GetFileEntry((Packages::DataFileType)i, 0);
            readSize = static_cast<uint32_t> (fileEntry->fileSize);
            break;
        }
    }
    assert(readSize > 0);
    uint64_t numReads = c_TOTAL_TO_READ_BYTES / readSize;
    std::unique_ptr<uint32_t[]> readLocationFile(new uint32_t[numReads]);

    {
        FullReadSet fullReadSet;
        for (uint32_t curPass = 0; curPass < numberDataSets; curPass++)
        {
            char nameBuffer[128];
            GenerateReadLocations(static_cast<uint32_t> (m_fileNames.size()), order, readLocationFile.get(), readSize);
            sprintf_s(nameBuffer, 128, "Iteration_%d", curPass);
            FullReadSet::ReadSet oneSet(nameBuffer, 1);
            for (uint32_t j = 0; j < numReads; j++)
            {
                oneSet.AddRead(FullReadSet::Read(m_fileNames[readLocationFile[j]].c_str(), readSize, 0, 0, 0));
            }
            fullReadSet.AddReadSet(oneSet);
        }
        CapturedReadDataXML readDataXML;
        readDataXML.AddReadSet(fullReadSet);
        readDataXML.WriteDataSetFile(readSetFileName, "", "");
    }
}

void SetupZipFileData::GenerateReadLocations(uint32_t numFiles, LoadOrder order, uint32_t* readLocationFile, uint32_t readSize)
{
    uint32_t lastReadFileLocation = 0;
    std::vector <uint32_t> readList;

    uint64_t totalNumReads = c_TOTAL_TO_READ_BYTES / readSize;

    for (uint32_t j = 0; j < totalNumReads; j++)
    {
        switch (order)
        {
        case LoadOrder::Random:
        case LoadOrder::Backwards:
        case LoadOrder::RandomSequential:
        {
            readLocationFile[j] = static_cast<uint32_t> (ATG::GetRandomValue(numFiles));
            readList.push_back(readLocationFile[j]);
        }
        break;
        case LoadOrder::Redundant:
        {
            readLocationFile[j] = j % 2 ? readLocationFile[j - 1] : static_cast<uint32_t> (ATG::GetRandomValue(numFiles));
            readList.push_back(readLocationFile[j]);
        }
        case LoadOrder::TrueSequential:
        {
            readLocationFile[j] = lastReadFileLocation;
            lastReadFileLocation++;
            lastReadFileLocation %= numFiles;
        }
        break;
        case LoadOrder::LAST_LOAD_ORDER:
            break;
        }
    }
    if (order == LoadOrder::RandomSequential)
    {
        std::sort(readList.begin(), readList.end(), [](const uint32_t& lhsValue, const uint32_t& rhsValue)
        {
            return lhsValue < rhsValue;
        });
        for (uint32_t j = 0; j < totalNumReads; j++)
        {
            readLocationFile[j] = readList[j];
        }
    }
    else if (order == LoadOrder::Backwards)
    {
        std::sort(readList.begin(), readList.end(), [](const uint32_t& lhsValue, const uint32_t& rhsValue)
        {
            return lhsValue > rhsValue;
        });
        for (uint32_t j = 0; j < totalNumReads; j++)
        {
            readLocationFile[j] = readList[j];
        }
    }
}
