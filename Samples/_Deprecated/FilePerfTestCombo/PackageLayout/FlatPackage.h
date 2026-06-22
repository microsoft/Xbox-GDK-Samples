//--------------------------------------------------------------------------------------
// FlatPackage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "FileSystem/File.h"
#include "BasePackage.h"

namespace Packages
{
    class FlatPackage : public BasePackage
    {
    private:
        std::random_device m_randomDevice;
        std::default_random_engine m_randomEngine;

        std::stack<DirectoryEntry *> m_currentParsingDirectory;

        uint32_t m_currentDirectoryNameIndex;
        uint32_t m_currentFileNameIndex;

        std::string m_name;
        char *m_layoutSetContents;                                    // rapidxml requires the string buffer to be stable

        std::vector<char *> m_saveStrings;
        std::map<DataFileType, std::vector<FileEntry*>> m_dataTypeLookup;

        bool ParseRootLayout(XMLNode *rootNode, bool roundTo4k);
        bool ParseChunkLayout(XMLNode *rootNode, bool roundTo4k);
        bool ParseFileNode(XMLNode *fileNode, BaseSubPackage *whichChunk, bool roundTo4k);
        bool ParseDirNode(XMLNode *dirNode, BaseSubPackage *whichChunk, bool roundTo4k);
        bool ParseFileOrder(XMLNode *orderNode, BaseSubPackage *whichChunk);
        bool ReadDataSetFile(XMLDoc &document, bool roundTo4k);
        bool AppendFileData(FileEntry *currentFile, XMLDoc& document, XMLNode *parentDirectoryNode);
        bool AppendDirectoryData(DirectoryEntry *currentDirectory, XMLDoc& document, XMLNode *parentDirectoryNode);
        void AppendFileOrder(XMLDoc& document, XMLNode *parentNode, BaseSubPackage *whichChunk);

        bool CreateMatchingFileLayout(DirectoryEntry* currentDirectory, uint32_t compressionRatio);
        bool CreateSingleFile(const FileEntry* file, uint32_t compressionRatio);

        void GenerateFullNames(DirectoryEntry* currentDirectory, const std::wstring& currentPath, BaseSubPackage* whichChunk);

        void FixupChildren(DirectoryEntry *currentDirectory);

        bool ReadDataSetFile(const std::string& dataSet, bool roundTo4k = false);

    public:
        FlatPackage();
        virtual ~FlatPackage()
        {
            for (auto& iter : m_saveStrings)
            {
                delete[] iter;
            }
            delete[] m_layoutSetContents;
        }

        void ClearData();

        virtual bool ReadDataFile(const std::wstring& layoutFileName, const std::wstring& orderFileName, bool roundTo4k);
        virtual bool ReadDataFile(const std::wstring& layoutFileName, const std::wstring& orderFileName) { return ReadDataFile(layoutFileName, orderFileName, true); }
        virtual bool SaveDataFile(const std::wstring& layoutFileName, const std::wstring& orderFileName);

        virtual bool CreateMatchingFileLayout(const std::wstring& rootDirectory, uint32_t compressionRatio);

        DirectoryEntry& GetRootDirectory() { return m_subPackages[0]->GetRootDirectory(); }
        const DirectoryEntry& GetRootDirectory() const { return m_subPackages[0]->GetRootDirectory(); }

        uint64_t AddFile(const std::wstring& fileName, const std::wstring& dirName, uint64_t fileSize, DataFileType dataType = fileType_null, BaseSubPackage *whichChunk = nullptr);
        //void AddChild(uint64_t parent, uint64_t child);
        void AddDirectory(const std::wstring& dirName, BaseSubPackage *whichChunk = nullptr);
        BaseSubPackage *AddChunk(const std::wstring& chunkName);

        size_t NumFiles(DataFileType fileType) { return m_dataTypeLookup[fileType].size(); }
        size_t NumFiles()
        {
            size_t toret = 0;
            for (auto& iter : m_dataTypeLookup)
            {
                toret += iter.second.size();
            }
            return toret;
        }
        FileEntry *GetFileEntry(DataFileType fileType, uint32_t index)
        {
            if (index >= m_dataTypeLookup[fileType].size())
                return nullptr;
            return m_dataTypeLookup[fileType][index];
        }
    };
}
