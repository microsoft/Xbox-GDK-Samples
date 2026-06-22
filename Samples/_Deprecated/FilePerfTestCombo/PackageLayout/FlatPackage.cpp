//--------------------------------------------------------------------------------------
// FlatPackage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FlatPackage.h"

#include "BaseFileInterface.h"
#include "JobSystem.h"

using namespace rapidxml;
using namespace Packages;

FlatPackage::FlatPackage() : m_randomEngine(m_randomDevice()), m_currentDirectoryNameIndex(0), m_currentFileNameIndex(0), m_layoutSetContents(nullptr)
{
    m_subPackages.push_back(new BaseSubPackage(this, L"DefaultChunk"));
}

bool FlatPackage::ParseFileNode(XMLNode* fileNode, BaseSubPackage* whichChunk, bool roundTo4k)
{
    FileEntry* currentFileEntry = new FileEntry();

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    XMLAttribute* attr = fileNode->first_attribute("name");
    if (attr)
    {
        currentFileEntry->fileName = converter.from_bytes(attr->value());
    }
    else
    {
        wchar_t buffer[128];
        swprintf_s(buffer, 128, L"test_file_%d", m_currentFileNameIndex++);
        currentFileEntry->fileName = buffer;
    }
    currentFileEntry->fullName = currentFileEntry->fileName;

    char* end;
    attr = fileNode->first_attribute("length");
    if (attr)
        currentFileEntry->fileSize = std::strtoull(attr->value(), &end, 0);
    if (currentFileEntry->fileSize == 0)
        currentFileEntry->fileSize = 4096;
    if (roundTo4k)
    {
        currentFileEntry->fileSize += (4096 - 1);
        currentFileEntry->fileSize &= ~(4096 - 1);
    }

    attr = fileNode->first_attribute("nameHash");
    if (attr)
        currentFileEntry->nameHash = std::strtoull(attr->value(), &end, 0);

    attr = fileNode->first_attribute("fileType");
    if (attr)
        currentFileEntry->fileType = ConvertStringToDataFileType(attr->value());
    else
        currentFileEntry->fileType = DataFileType::fileType_data;

    attr = fileNode->first_attribute("child");
    while (attr)
    {
        currentFileEntry->childrenRootName.push_back(converter.from_bytes(attr->value()));
        attr = attr->next_attribute("child");
    }

    m_currentParsingDirectory.top()->AddFileEntry(currentFileEntry);
    m_dataTypeLookup[currentFileEntry->fileType].push_back(currentFileEntry);
    whichChunk->AddFileToMap(currentFileEntry->nameHash, currentFileEntry);
    return true;
}

bool FlatPackage::ParseDirNode(XMLNode* dirNode, BaseSubPackage* whichChunk, bool roundTo4k)
{
    DirectoryEntry* currentDirEntry = nullptr;
    currentDirEntry = new DirectoryEntry();

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    XMLAttribute* attr = dirNode->first_attribute("name");
    if (attr)
    {
        currentDirEntry->dirName = converter.from_bytes(attr->value());
    }
    else
    {
        wchar_t buffer[128];
        swprintf_s(buffer, 128, L"test_dir_%d", m_currentDirectoryNameIndex++);
        currentDirEntry->dirName = buffer;
    }

    m_currentParsingDirectory.top()->AddDirectoryEntry(currentDirEntry);
    m_currentParsingDirectory.push(currentDirEntry);

    XMLNode* dataNode = dirNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "file") == 0)
        {
            ParseFileNode(dataNode, whichChunk, roundTo4k);
        }
        else if (strcmp(dataNode->name(), "dir") == 0)
        {
            ParseDirNode(dataNode, whichChunk, roundTo4k);
        }
        dataNode = dataNode->next_sibling();
    }

    m_currentParsingDirectory.pop();
    return true;
}

bool FlatPackage::ParseFileOrder(XMLNode* orderNode, BaseSubPackage* whichChunk)
{
    XMLNode* dataNode = orderNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "file") == 0)
        {
            char* end;
            uint64_t nameHash(0);
            std::wstring fileName;
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            XMLAttribute* attr = dataNode->first_attribute("name");
            if (attr)
            {
                fileName = converter.from_bytes(attr->value());
            }
            attr = dataNode->first_attribute("nameHash");
            if (attr)
            {
                nameHash = strtoull(attr->value(), &end, 0);
            }
            auto iter = whichChunk->FindFileEntry(nameHash);
            assert(iter != nullptr);
            whichChunk->GetFileOrder().push_back(iter);
        }
        dataNode = dataNode->next_sibling();
    }

    return true;
}

bool FlatPackage::ParseChunkLayout(XMLNode* chunkNode, bool roundTo4k)
{
    BaseSubPackage* currentChunk = nullptr;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    XMLAttribute* attr = chunkNode->first_attribute("name");
    if (attr)
    {
        currentChunk = new BaseSubPackage(this, converter.from_bytes(attr->value()));
    }
    else
    {
        wchar_t buffer[128];
        swprintf_s(buffer, 128, L"test_chunk_%lld", m_subPackages.size() + 1);
        currentChunk = new BaseSubPackage(this, buffer);
    }

    m_subPackages.push_back(currentChunk);
    assert(m_currentParsingDirectory.size() == 0);
    m_currentParsingDirectory.push(&currentChunk->GetRootDirectory());

    XMLNode* dataNode = chunkNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "dir") == 0)
        {
            ParseDirNode(dataNode, currentChunk, roundTo4k);
        }
        else if (strcmp(dataNode->name(), "file") == 0)
        {
            ParseFileNode(dataNode, currentChunk, roundTo4k);
        }
        else if (strcmp(dataNode->name(), "fileOrder") == 0)
        {
            ParseFileOrder(dataNode, currentChunk);
        }
        dataNode = dataNode->next_sibling();
    }
    m_currentParsingDirectory.pop();
    return true;
}

bool FlatPackage::ParseRootLayout(XMLNode* rootNode, bool roundTo4k)
{
    XMLNode* dataNode = rootNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "chunk") == 0)
        {
            ParseChunkLayout(dataNode, roundTo4k);
        }
        dataNode = dataNode->next_sibling();
    }
    return true;
}

bool FlatPackage::ReadDataSetFile(XMLDoc& document, bool roundTo4k)
{
    XMLNode* curNode = document.first_node();
    while (curNode)
    {
        if (strcmp(curNode->name(), "root") == 0)
        {
            return ParseRootLayout(curNode, roundTo4k);
        }
        curNode = curNode->next_sibling();
    }
    return false;
}

bool FlatPackage::ReadDataSetFile(const std::string& dataSet, bool roundTo4k)
{
    ClearData();
    auto xmlDoc = std::make_unique<XMLDoc>();

    // RapidXML requires the  contents of the string buffer be stable through the lifetime of the xml document
    // so we want to make a copy of that buffer so the caller can free their copy
    size_t contentsLen = dataSet.size();
    m_layoutSetContents = new char[contentsLen + 1];
    memset(m_layoutSetContents, 0, contentsLen + 1);
    memcpy(m_layoutSetContents, dataSet.c_str(), contentsLen);
    try
    {
        xmlDoc->parse<parse_full>(m_layoutSetContents);
    }
    catch (std::exception& /*error*/)
    {
        return false;
    }
    if (!ReadDataSetFile(*xmlDoc, roundTo4k))
        return false;

    for (auto& chunkIter : m_subPackages)
    {
        GenerateFullNames(&chunkIter->GetRootDirectory(), L"", chunkIter);
        FixupChildren(&chunkIter->GetRootDirectory());
    }

    return true;
}

bool FlatPackage::ReadDataFile(const std::wstring& layoutFileName, const std::wstring& /*orderFileName*/, bool roundTo4k)
{
    m_layoutFileName = layoutFileName;
    std::string fileContents;

    BaseFileInterface::ReadFile(layoutFileName, fileContents);
    return ReadDataSetFile(fileContents, roundTo4k);
}

bool FlatPackage::CreateSingleFile(const FileEntry* entry, uint32_t compressionRatio)
{
    CREATEFILE2_EXTENDED_PARAMETERS params;
    HANDLE fileMap;
    HANDLE file;
    uint64_t* baseAddress64;
    std::mt19937_64 dataFileRandomValue;
    memset(&params, 0, sizeof(params));

    params.dwSize = sizeof(params);
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    file = CreateFileW(entry->fileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    uint64_t totalSize = entry->fileSize;
    fileMap = CreateFileMapping(file, nullptr, PAGE_READWRITE, totalSize >> 32ULL, totalSize & 0xffffffff, nullptr);
    if (fileMap == nullptr)
    {
        CloseHandle(file);
        return false;
    }

    uint64_t currentValue64 = 0;
    float compressionPercentage = compressionRatio / 100.0f;
    baseAddress64 = static_cast<uint64_t*>(MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (!baseAddress64)
    {
        CloseHandle(fileMap);
        CloseHandle(file);
        return false;
    }
    memset(baseAddress64, 0, entry->fileSize);
    uint64_t numBlocks64 = entry->fileSize / sizeof(uint64_t);
    uint64_t compressedChunkSize = (32768 / sizeof(uint64_t));
    uint64_t numSubChunks = numBlocks64 / compressedChunkSize;
    if ((numSubChunks * compressedChunkSize) < numBlocks64)
        numSubChunks++;
    compressedChunkSize = (numBlocks64 / numSubChunks);
    uint64_t clearChunkSize = static_cast<uint64_t> (compressedChunkSize * compressionPercentage);
    for (uint64_t j = 0; j < numBlocks64; j++)
    {
        if (compressionRatio == 0)
            baseAddress64[j] = currentValue64++;
        else if ((j % compressedChunkSize) < clearChunkSize)
            baseAddress64[j] = 0;
        else
            baseAddress64[j] = dataFileRandomValue();
    }

    uint8_t leftOverBytes = entry->fileSize % sizeof(uint64_t);
    uint8_t* leftOverAddress = reinterpret_cast<uint8_t*> (baseAddress64);
    leftOverAddress += numBlocks64 * sizeof(uint64_t);
    for (uint32_t j = 0; j < leftOverBytes; j++)
    {
        DebugBreak();
        leftOverAddress[j] = 0;
    }

    UnmapViewOfFile(baseAddress64);
    CloseHandle(fileMap);
    CloseHandle(file);
    return true;
}

bool FlatPackage::CreateMatchingFileLayout(DirectoryEntry* currentDirectory, uint32_t compressionRatio)
{
    std::uniform_int_distribution<uint32_t> dataFileRandomValue(0, UINT32_MAX);

    char currentWorkingDirectory[2048];
    (void)GetCurrentDirectoryA(2048, currentWorkingDirectory);
    if (currentDirectory->dirName.size())
    {
        (void)_wmkdir(currentDirectory->dirName.c_str());
        (void)SetCurrentDirectoryW(currentDirectory->dirName.c_str());
    }
    for (const auto& iter : currentDirectory->files)
    {
        (void)CreateSingleFile(iter.get(), compressionRatio);
    }
    for (const auto& iter : currentDirectory->directories)
    {
        (void)CreateMatchingFileLayout(iter.get(), compressionRatio);
    }

    (void)SetCurrentDirectoryA(currentWorkingDirectory);

    return true;
}

bool FlatPackage::CreateMatchingFileLayout(const std::wstring& rootDirectory, uint32_t compressionRatio)
{
    wchar_t currentWorkingDirectory[2048];
    (void)GetCurrentDirectoryW(2048, currentWorkingDirectory);
    (void)_wmkdir(rootDirectory.c_str());
    (void)SetCurrentDirectoryW(rootDirectory.c_str());
    for (auto& chunkIter : m_subPackages)
    {
        assert(chunkIter->GetRootDirectory().dirName.size() == 0);
        (void)CreateMatchingFileLayout(&chunkIter->GetRootDirectory(), compressionRatio);
    }
    (void)SetCurrentDirectoryW(currentWorkingDirectory);
    return true;
}

BaseSubPackage* FlatPackage::AddChunk(const std::wstring& chunkName)
{
    BaseSubPackage* newChunk = new BaseSubPackage(this, chunkName);
    m_subPackages.push_back(newChunk);
    return newChunk;
}

uint64_t FlatPackage::AddFile(const std::wstring& fileName, const std::wstring& dirName, uint64_t fileSize, DataFileType dataType, BaseSubPackage* whichChunk)
{
    if (whichChunk == nullptr)
        whichChunk = m_subPackages[0];
    std::wstring fullFileName = dirName;
    if (fullFileName.size() != 0)
        fullFileName += L"\\";
    fullFileName += fileName;

    FileEntry* newEntry = new FileEntry(fileName, fileSize);
    newEntry->fullName = fullFileName;
    newEntry->nameHash = BaseFileInterface::GenerateFilenameHash(fullFileName);
    newEntry->fileType = dataType;
    whichChunk->AddFileToMap(newEntry);
    whichChunk->GetFileOrder().push_back(newEntry);
    if (dirName.size() != 0)
    {
        DirectoryEntry* dir = whichChunk->GetRootDirectory().FindDirectory(dirName);
        assert(dir);
        dir->AddFileEntry(newEntry);
    }
    else
    {
        whichChunk->GetRootDirectory().AddFileEntry(newEntry);
    }

    m_dataTypeLookup[newEntry->fileType].push_back(newEntry);

    return newEntry->nameHash;
}

void FlatPackage::AddDirectory(const std::wstring& dirName, BaseSubPackage* whichChunk)
{
    if (whichChunk == nullptr)
        whichChunk = m_subPackages[0];

    DirectoryEntry* dir = whichChunk->GetRootDirectory().FindDirectory(dirName);
    if (dir)
        return;

    DirectoryEntry* newEntry = new DirectoryEntry();
    newEntry->dirName = dirName;
    whichChunk->GetRootDirectory().AddDirectoryEntry(newEntry);
}

bool FlatPackage::AppendFileData(FileEntry* currentFile, XMLDoc& document, XMLNode* parentDirectoryNode)
{
    XMLNode* fileNode = document.allocate_node(node_element, "file");

    {
        char* newString = new char[32];
        memset(newString, 0, 32);
        sprintf_s(newString, 32, "%lld", currentFile->fileSize);
        m_saveStrings.push_back(newString);
        XMLAttribute* lenAttr = document.allocate_attribute("length", newString);
        fileNode->append_attribute(lenAttr);
    }

    {
        char* newString = new char[currentFile->fileName.size() + 1];
        sprintf_s(newString, currentFile->fileName.size() + 1, "%S", currentFile->fileName.c_str());
        m_saveStrings.push_back(newString);
        XMLAttribute* nameAttr = document.allocate_attribute("name", newString);
        fileNode->append_attribute(nameAttr);
    }

    {
        char* newString = new char[32];
        memset(newString, 0, 32);
        sprintf_s(newString, 32, "%s", ConvertDataFileTypeToString(currentFile->fileType).c_str());
        m_saveStrings.push_back(newString);
        XMLAttribute* typeAttr = document.allocate_attribute("fileType", newString);
        fileNode->append_attribute(typeAttr);
    }

    {
        char* newString = new char[32];
        memset(newString, 0, 32);
        sprintf_s(newString, 32, "%llu", currentFile->nameHash);
        m_saveStrings.push_back(newString);
        XMLAttribute* lenAttr = document.allocate_attribute("nameHash", newString);
        fileNode->append_attribute(lenAttr);
    }

    for (const auto& iter : currentFile->children)
    {
        char* newString = new char[iter->fullName.size() + 1];
        sprintf_s(newString, iter->fullName.size() + 1, "%S", iter->fullName.c_str());
        m_saveStrings.push_back(newString);
        XMLAttribute* nameAttr = document.allocate_attribute("child", newString);
        fileNode->append_attribute(nameAttr);
    }

    parentDirectoryNode->append_node(fileNode);

    return true;
}

bool FlatPackage::AppendDirectoryData(DirectoryEntry* currentDirectory, XMLDoc& document, XMLNode* parentDirectoryNode)
{
    for (const auto& iter : currentDirectory->files)
    {
        AppendFileData(iter.get(), document, parentDirectoryNode);
    }
    for (const auto& iter : currentDirectory->directories)
    {
        XMLNode* dirNode = document.allocate_node(node_element, "dir");

        char* newString = new char[iter->dirName.size() + 1];
        sprintf_s(newString, iter->dirName.size() + 1, "%ls", iter->dirName.c_str());
        //memset(newString, 0, iter->dirName.size() + 1);
        //memcpy(newString, iter->dirName.c_str(), iter->dirName.size());
        m_saveStrings.push_back(newString);
        XMLAttribute* nameAttr = document.allocate_attribute("name", newString);
        dirNode->append_attribute(nameAttr);

        parentDirectoryNode->append_node(dirNode);
        AppendDirectoryData(iter.get(), document, dirNode);
    }
    return true;
}

void FlatPackage::AppendFileOrder(XMLDoc& document, XMLNode* parentNode, BaseSubPackage* whichChunk)
{
    if (whichChunk == nullptr)
        whichChunk = m_subPackages[0];
    XMLNode* topNode = document.allocate_node(node_element, "fileOrder");
    for (const auto& iter : whichChunk->GetFileOrder())
    {
        XMLNode* fileNode = document.allocate_node(node_element, "file");

        {
            char* newString = new char[iter->fullName.size() + 1];
            sprintf_s(newString, iter->fullName.size() + 1, "%S", iter->fullName.c_str());
            m_saveStrings.push_back(newString);
            XMLAttribute* nameAttr = document.allocate_attribute("name", newString);
            fileNode->append_attribute(nameAttr);
        }

        {
            char* newString = new char[32];
            memset(newString, 0, 32);
            sprintf_s(newString, 32, "%llu", iter->nameHash);
            m_saveStrings.push_back(newString);
            XMLAttribute* lenAttr = document.allocate_attribute("nameHash", newString);
            fileNode->append_attribute(lenAttr);
        }
        topNode->append_node(fileNode);
    }

    parentNode->append_node(topNode);
}

bool FlatPackage::SaveDataFile(const std::wstring& layoutFileName, const std::wstring& /*orderFileName*/)
{
    std::string outputData;

    auto xmlDoc = std::make_unique<XMLDoc>();
    XMLNode* rootNode = xmlDoc->allocate_node(node_element, "root");
    xmlDoc->append_node(rootNode);

    for (auto& chunkIter : m_subPackages)
    {
        XMLNode* chunkNode = xmlDoc->allocate_node(node_element, "chunk");

        char* newString = new char[chunkIter->GetName().size() + 1];
        sprintf_s(newString, chunkIter->GetName().size() + 1, "%ls", chunkIter->GetName().c_str());
        m_saveStrings.push_back(newString);
        XMLAttribute* nameAttr = xmlDoc->allocate_attribute("name", newString);
        chunkNode->append_attribute(nameAttr);

        rootNode->append_node(chunkNode);

        AppendDirectoryData(&chunkIter->GetRootDirectory(), *xmlDoc, chunkNode);
        AppendFileOrder(*xmlDoc, chunkNode, chunkIter);
    }

    rapidxml::print(std::back_inserter(outputData), *xmlDoc, 0);

    FILE* newFile = nullptr;
    _wfopen_s(&newFile, layoutFileName.c_str(), L"w");
    if (newFile == nullptr)
        return false;
    fwrite(outputData.c_str(), 1, outputData.size(), newFile);
    fclose(newFile);
    return true;
}

void FlatPackage::GenerateFullNames(DirectoryEntry* currentDirectory, const std::wstring& currentPath, BaseSubPackage* whichChunk)
{
    for (auto& iter : currentDirectory->directories)
    {
        std::wstring newPath = currentPath;
        if (currentPath != L"")
            newPath += L"\\";
        newPath += iter->dirName;
        GenerateFullNames(iter.get(), newPath, whichChunk);
    }
    for (auto& iter : currentDirectory->files)
    {
        std::wstring newPath = currentPath;
        if (currentPath != L"")
            newPath += L"\\";
        newPath += iter->fileName;
        iter->fullName = newPath;
        assert(iter->nameHash == BaseFileInterface::GenerateFilenameHash(iter->fullName));
        //iter->nameHash = BaseFileInterface::GenerateFilenameHash(iter->fullName);
        whichChunk->AddFileToMap(iter.get());
    }
}

void FlatPackage::FixupChildren(DirectoryEntry* currentDirectory)
{
    for (auto& iter : currentDirectory->directories)
    {
        FixupChildren(iter.get());
    }
    for (auto& iter : currentDirectory->files)
    {
        for (auto& nameIter : iter->childrenRootName)
        {
            uint64_t childNameHash = BaseFileInterface::GenerateFilenameHash(nameIter);
            const FileEntry* file = FindFileEntry(childNameHash);
            assert(file);
            iter->children.push_back(file);
        }
    }
}

void FlatPackage::ClearData()
{
    delete[] m_layoutSetContents;
    m_layoutSetContents = nullptr;

    for (auto iter : m_saveStrings)
    {
        delete[] iter;
    }
    m_saveStrings.clear();
    m_dataTypeLookup.clear();
    for (auto& chunkIter : m_subPackages)
    {
        delete chunkIter;
    }
    m_subPackages.clear();
    m_fileNameMapping.clear();
}
