//--------------------------------------------------------------------------------------
// SetupFlatFileData.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SetupFlatFileData.h"

#include <shellapi.h>
#include <direct.h>

#include "Random.h"
#include "PackedFile.h"
#include "LooseFiles.h"
#include "ReadSet.h"
#include "CapturedReadDataXML.h"
const uint32_t DS_READ_ALIGNMENT = ~4095U;
const uint32_t WIN32_READ_ALIGNMENT = ~4095U;

const uint32_t SetupFlatFileData::SIZE_DATA_READS_INTS[SetupFlatFileData::NUM_LOAD_SIZES] = {
    8 * c_UINT_1k ,             //size_8k
    12 * c_UINT_1k ,            //size_12k
    16 * c_UINT_1k ,            //size_16k
    32 * c_UINT_1k ,            //size_32k
    64 * c_UINT_1k ,            //size_64k
    128 * c_UINT_1k ,           //size_128k
    192 * c_UINT_1k ,           //size_192k
    256 * c_UINT_1k ,           //size_256k
    512 * c_UINT_1k ,           //size_512k
    1024 * c_UINT_1k ,          //size_1024k
    2048 * c_UINT_1k ,          //size_2048k
    4096 * c_UINT_1k ,          //size_4096k
    8192 * c_UINT_1k ,          //size_8192k,
    16384 * c_UINT_1k ,          //size_16386k,
    32768 * c_UINT_1k ,          //size_32768k,
};

const uint64_t SetupFlatFileData::NUM_DATA_READS[SetupFlatFileData::NUM_LOAD_SIZES] = {
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[0]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[1]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[2]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[3]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[4]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[5]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[6]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[7]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[8]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[9]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[10]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[11]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[12]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[13]),
    c_TOTAL_TO_READ_BYTES / (sizeof(uint32_t) * SIZE_DATA_READS_INTS[14]),
};

uint32_t SetupFlatFileData::SIZE_DATA_READS_BYTES[SetupFlatFileData::NUM_LOAD_SIZES];

SetupFlatFileData::SetupFlatFileData()
{
    for (uint32_t i = 0; i < NUM_LOAD_SIZES; i++)
    {
        SIZE_DATA_READS_BYTES[i] = SIZE_DATA_READS_INTS[i] * sizeof(uint32_t);
    }
}

SetupFlatFileData::~SetupFlatFileData()
{
}

void SetupFlatFileData::CleanOldFiles(const std::wstring& rootDirName)
{
    wchar_t currentWorkingDirectory[2048];
    GetCurrentDirectoryW(2048, currentWorkingDirectory);
    std::filesystem::path workingPath(currentWorkingDirectory);

    std::wstring fileName(rootDirName);
    fileName += L".packed";
    DeleteFile(fileName.c_str());

    workingPath /= rootDirName;
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

std::unique_ptr<Packages::FlatPackage> SetupFlatFileData::GenerateLayoutFile(const std::wstring& layoutFileName, const std::wstring& dataFilePrefix)
{
    std::unique_ptr<Packages::FlatPackage> layout(new Packages::FlatPackage());

    Packages::BaseSubPackage* dataChunk = layout->AddChunk(L"data_chunk");
    layout->AddDirectory(L"data_dir", dataChunk);
    for (uint32_t i = 0; i < ROOT_FILE_BLOCKS; i++)
    {
        wchar_t fileName[256];
        swprintf_s(fileName, 256, L"%s_%d.data", dataFilePrefix.c_str(), i);

        uint64_t fileSize = sizeof(uint32_t);
        fileSize *= ROOT_BLOCK_SIZE_INTS;
        layout->AddFile(fileName, L"data_dir", fileSize, Packages::fileType_data, dataChunk);
    }
    layout->SaveDataFile(layoutFileName, L"");
    return layout;
}
void SetupFlatFileData::GenerateFiles(Packages::FlatPackage* layout, const std::wstring& destDirName, uint32_t compressionRatio)
{
    layout->CreateMatchingFileLayout(destDirName, compressionRatio);
}

void SetupFlatFileData::GenerateTestData(Packages::FlatPackage* layout, uint32_t numIterations, const std::wstring& rootDirName)
{
    std::unique_ptr <BaseFileInterface> fileInterface(nullptr);
    fileInterface = std::make_unique<LooseFiles>();
    fileInterface->SetDirectoryOffset(rootDirName);

    m_testFile.reserve(layout->NumFiles());
    m_files.reserve(layout->NumFiles());

    for (uint32_t i = Packages::FIRST_DATA_FILE_TYPE; i <= Packages::LAST_DATA_FILE_TYPE; i++)
    {
        if (i == Packages::fileType_null)
            continue;
        uint32_t numFiles = (uint32_t)layout->NumFiles((Packages::DataFileType)i);
        for (uint32_t j = 0; j < numFiles; j++)
        {
            Packages::FileEntry* file = layout->GetFileEntry((Packages::DataFileType)i, j);
            uintptr_t platformHandle;
            BaseFileInterface::FileHandle fileHandle = fileInterface->OpenFile(file->fullName, OPEN_EXISTING, FileSystem::File::e_openFlag_NoBuffering, platformHandle);
            if (fileHandle == BaseFileInterface::s_invalidFileHandle)
            {
                assert(false);
                return;
            }
            m_testFile.push_back(fileHandle);
            m_files.push_back(file);
        }
    }

    for (uint32_t dataOrder = FIRST_LOAD_ORDER; dataOrder <= LAST_PERF_ORDER; dataOrder++)
    {
        for (uint32_t dataSize = FIRST_DATA_SIZE; dataSize < LAST_DATA_SIZE; dataSize++)
        {
            std::thread* newThread = new std::thread(&SetupFlatFileData::GenerateTestDataJob, this, layout, L"directedReadSet", numIterations, (LoadOrder)dataOrder, (DataSize)dataSize, true);
            m_generateTestDataThreads.push_back(newThread);
            if constexpr (DS_READ_ALIGNMENT != WIN32_READ_ALIGNMENT)
            {
                newThread = new std::thread(&SetupFlatFileData::GenerateTestDataJob, this, layout, L"directedReadSet", numIterations, (LoadOrder)dataOrder, (DataSize)dataSize, false);
                m_generateTestDataThreads.push_back(newThread);
            }
        }
    }
    for (auto& iter : m_generateTestDataThreads)
    {
        iter->join();
        delete iter;
    }
    m_generateTestDataThreads.clear();
}

void SetupFlatFileData::GenerateTestDataJob(Packages::FlatPackage* layoutSet, const std::wstring filePrefix, uint32_t numberDataSets, LoadOrder order, DataSize dataSize, bool directStorage)
{
    std::unique_ptr<uint64_t[]> readLocationBytes(new uint64_t[s_maxDataReads]);
    std::unique_ptr<uint32_t[]> readLocationFile(new uint32_t[s_maxDataReads]);

    wchar_t readSetFileName[256];
    uint32_t readAlignment = 0;
    if constexpr (DS_READ_ALIGNMENT == WIN32_READ_ALIGNMENT)
    {
        readAlignment = DS_READ_ALIGNMENT;
        swprintf(readSetFileName, 256, L"%s_%s_%s.xml", filePrefix.c_str(), ConvertLoadOrderToString(order).c_str(), ConvertDataSizeToString(dataSize).c_str());
    }
    else if (directStorage)
    {
        readAlignment = DS_READ_ALIGNMENT;
        swprintf(readSetFileName, 256, L"ds_%s_%s_%s.xml", filePrefix.c_str(), ConvertLoadOrderToString(order).c_str(), ConvertDataSizeToString(dataSize).c_str());
    }
    else
    {
        readAlignment = WIN32_READ_ALIGNMENT;
        swprintf(readSetFileName, 256, L"win32_%s_%s_%s.xml", filePrefix.c_str(), ConvertLoadOrderToString(order).c_str(), ConvertDataSizeToString(dataSize).c_str());
    }

    {
        FullReadSet fullReadSet;
        for (uint32_t curPass = 0; curPass < numberDataSets; curPass++)
        {
            char nameBuffer[128];
            GenerateReadLocations(layoutSet, order, dataSize, readLocationFile.get(), readLocationBytes.get(), readAlignment);
            sprintf_s(nameBuffer, 128, "Iteration_%d", curPass);
            FullReadSet::ReadSet oneSet(nameBuffer, 1);
            for (uint32_t j = 0; j < NUM_DATA_READS[dataSize]; j++)
            {
                oneSet.AddRead(FullReadSet::Read(m_files[readLocationFile[j]]->fullName.c_str(), SIZE_DATA_READS_BYTES[dataSize], readLocationBytes[j], 0, 0));
            }
            fullReadSet.AddReadSet(oneSet);
        }
        CapturedReadDataXML readDataXML;
        readDataXML.AddReadSet(fullReadSet);
        readDataXML.WriteDataSetFile(readSetFileName, "", "");
    }
}

void SetupFlatFileData::GenerateReadLocations(Packages::FlatPackage* layoutSet, LoadOrder order, DataSize dataSize, uint32_t* readLocationFile, uint64_t* readLocationBytes, uint32_t readAlignment)
{
    uint32_t nextReadLocation = 0;
    uint32_t lastReadFile = 0;
    std::vector < std::pair<uint32_t, uint64_t>> readList;

    uint32_t readSizeBytes = (SIZE_DATA_READS_BYTES[dataSize]);
    std::vector<Packages::FileEntry*> files;
    files.reserve(layoutSet->NumFiles());
    for (uint32_t i = Packages::FIRST_DATA_FILE_TYPE; i <= Packages::LAST_DATA_FILE_TYPE; i++)
    {
        if (i == Packages::fileType_null)
            continue;
        uint32_t numFiles = (uint32_t)layoutSet->NumFiles((Packages::DataFileType)i);
        for (uint32_t j = 0; j < numFiles; j++)
        {
            Packages::FileEntry* fileEntry = layoutSet->GetFileEntry((Packages::DataFileType)i, j);
            if (fileEntry->fileSize < readSizeBytes)
                continue;
            files.push_back(fileEntry);
        }
    }
    assert(files.size() > 0);
    if (files.size() == 0)
        return;

    for (uint32_t j = 0; j < NUM_DATA_READS[dataSize]; j++)
    {
        switch (order)
        {
        case LoadOrder::Random:
        case LoadOrder::Backwards:
        case LoadOrder::RandomSequential:
        {
            readLocationFile[j] = static_cast<uint32_t> (ATG::GetRandomValue(files.size()));

            uint64_t maxLocation = files[readLocationFile[j]]->fileSize - readSizeBytes;
            if (maxLocation == 0)
            {
                readLocationBytes[j] = 0;
            }
            else
            {
                readLocationBytes[j] = files[readLocationFile[j]]->fileSize;
                while ((readLocationBytes[j] + readSizeBytes) > files[readLocationFile[j]]->fileSize)
                {
                    readLocationBytes[j] = ATG::GetRandomValue(maxLocation);
                }
            }
            readLocationBytes[j] &= readAlignment;
            readList.push_back(std::make_pair(readLocationFile[j], readLocationBytes[j]));
        }
        break;
        case LoadOrder::Redundant:
        {
            readLocationFile[j] = j % 2 ? readLocationFile[j - 1] : static_cast<uint32_t> (ATG::GetRandomValue(files.size()));
            uint64_t maxLocation = files[readLocationFile[j]]->fileSize - readSizeBytes;
            if (maxLocation == 0)
            {
                readLocationBytes[j] = 0;
            }
            else
            {
                readLocationBytes[j] = files[readLocationFile[j]]->fileSize;
                while ((readLocationBytes[j] + SIZE_DATA_READS_BYTES[dataSize]) > maxLocation)
                {
                    readLocationBytes[j] = j % 2 ? readLocationBytes[j - 1] : ATG::GetRandomValue(maxLocation);
                }
            }
            readList.push_back(std::make_pair(readLocationFile[j], readLocationBytes[j]));
        }
        case LoadOrder::TrueSequential:
        {
            readLocationFile[j] = lastReadFile;
            uint64_t maxLocation = files[readLocationFile[j]]->fileSize - readSizeBytes;
            readLocationBytes[j] = nextReadLocation;
            if (readLocationBytes[j] > maxLocation)
            {
                readLocationBytes[j] = 0;
                nextReadLocation = 0;
                lastReadFile++;
                lastReadFile %= files.size();
                readLocationFile[j] = lastReadFile;
            }
            else
            {
                nextReadLocation += readSizeBytes;
            }
        }
        break;
        case LoadOrder::LAST_LOAD_ORDER:
            break;
        }
        readLocationBytes[j] &= readAlignment;
    }

    if (order == LoadOrder::RandomSequential)
    {
        std::sort(readList.begin(), readList.end(), [](const std::pair<uint32_t, uint64_t>& lhs, const std::pair<uint32_t, uint64_t>& rhs)
        {
            uint64_t lhsValue, rhsValue;
            lhsValue = lhs.first * lhs.second;
            rhsValue = rhs.first * rhs.second;
            return lhsValue < rhsValue;
        });
        for (uint32_t j = 0; j < NUM_DATA_READS[dataSize]; j++)
        {
            readLocationFile[j] = readList[j].first;
            readLocationBytes[j] = readList[j].second;
        }
    }
    else if (order == LoadOrder::Backwards)
    {
        std::sort(readList.begin(), readList.end(), [](const std::pair<uint32_t, uint64_t>& lhs, const std::pair<uint32_t, uint64_t>& rhs)
        {
            uint64_t lhsValue, rhsValue;
            lhsValue = lhs.first * lhs.second;
            rhsValue = rhs.first * rhs.second;
            return lhsValue > rhsValue;
        });
        for (uint32_t j = 0; j < NUM_DATA_READS[dataSize]; j++)
        {
            readLocationFile[j] = readList[j].first;
            readLocationBytes[j] = readList[j].second;
        }
    }
}
