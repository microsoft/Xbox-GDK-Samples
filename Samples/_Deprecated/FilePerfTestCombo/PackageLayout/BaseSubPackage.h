//--------------------------------------------------------------------------------------
// BaseSubPackage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#pragma warning (push,0)
#include "rapidxml-1.13\rapidxml.hpp"
#include "rapidxml-1.13\rapidxml_utils.hpp"
#include "rapidxml-1.13\rapidxml_print.hpp"
#pragma warning (pop)

typedef rapidxml::xml_document<char> XMLDoc;
typedef rapidxml::xml_node<char> XMLNode;
typedef rapidxml::xml_attribute<char> XMLAttribute;

namespace Packages
{
    class BasePackage;
    struct FileEntry;
    struct DirectoryEntry;

    typedef std::vector<std::unique_ptr<FileEntry>> FileVector;
    typedef std::vector<std::unique_ptr<DirectoryEntry>> DirectoryVector;
    typedef FileVector::iterator fileIterator;
    typedef FileVector::const_iterator const_fileIterator;
    typedef DirectoryVector::iterator dirIterator;
    typedef DirectoryVector::const_iterator const_dirIterator;

    enum DataFileType
    {
        fileType_null,
        fileType_meta,
        fileType_data,
        fileType_skeletonTexture,
        fileType_terrainTexture,

        fileType_animation,
        fileType_skeleton,
        fileType_skeletonMesh,
        fileType_terrain,
        fileType_terrainMesh,

        FIRST_DATA_FILE_TYPE = fileType_null,
        LAST_DATA_FILE_TYPE = fileType_terrainMesh,
        NUM_DATA_FILE_TYPE = fileType_terrainMesh + 1,

        FIRST_RANDOM_DATA_FILE_TYPE = fileType_animation,
        LAST_RANDOM_DATA_FILE_TYPE = fileType_terrainMesh,
    };

    struct FileEntry
    {
        std::wstring fileName;
        std::wstring fullName;
        uint64_t nameHash;
        uint64_t fileSize;
        DataFileType fileType;
        std::vector<const FileEntry *> children;			// files that are dependent on this file for reading
        std::vector<std::wstring> childrenRootName;
        FileEntry(const std::wstring& newName = L"", uint64_t newSize = 0) noexcept { fileName = newName, fileSize = newSize; fileType = fileType_null; nameHash = 0; }
        virtual ~FileEntry() {}
    };

    struct DirectoryEntry
    {
        std::wstring dirName;
        FileVector files;
        DirectoryVector directories;

        void AddFileEntry(FileEntry *newEntry)
        {
            files.emplace_back(std::unique_ptr<FileEntry>(newEntry));
        }

        void AddDirectoryEntry(DirectoryEntry *dirEntry)
        {
            directories.emplace_back(std::unique_ptr<DirectoryEntry>(dirEntry));
        }

        DirectoryEntry *FindDirectory(const std::wstring& dirName);
        FileEntry *FindFile(uint64_t fileNameHash);
        fileIterator beginFile() { return files.begin(); }
        const_fileIterator beginFile() const { return files.begin(); }
        fileIterator endFile() { return files.end(); }
        const_fileIterator endFile() const { return files.end(); }

        dirIterator beginDirectory() { return directories.begin(); }
        const_dirIterator beginDirectory() const { return directories.begin(); }
        dirIterator endDirectory() { return directories.end(); }
        const_dirIterator endDirectory() const { return directories.end(); }
    };

    DataFileType ConvertStringToDataFileType(const std::string& str);
    std::string ConvertDataFileTypeToString(DataFileType fileType);
    std::wstring ConvertDataFileTypeToFileName(DataFileType fileType);
    std::wstring ConvertDataFileTypeToDirName(DataFileType fileType);

    class BaseSubPackage
    {
    protected:

        DirectoryEntry m_rootDirectory;
        std::vector<FileEntry*> m_fileOrder;
        std::map<std::wstring, FileEntry*> m_fileNameMapping;
        std::map<uint64_t, FileEntry*> m_fileHashMapping;

        BasePackage *m_parent;
        std::wstring m_subPackageName;
        uint64_t m_subPackageHashName;

    public:
        BaseSubPackage(BasePackage *parent, const std::wstring& name);
        virtual ~BaseSubPackage();

        void AddFileToMap(FileEntry* whichFile);
        void AddFileToMap(const std::wstring& name, FileEntry* whichFile);
        void AddFileToMap(uint64_t nameHash, FileEntry* whichFile);

        const std::wstring& GetName() const { return m_subPackageName; }
        void SetName(const std::wstring& newName)
        {
            m_subPackageName = newName;
            std::hash<std::wstring> hash_fn;
            m_subPackageHashName = hash_fn(m_subPackageName);
        }
        uint64_t GetNameHash() const { return m_subPackageHashName; }

        FileEntry *FindFileEntry(uint64_t fileNameHash) const;

        DirectoryEntry& GetRootDirectory() { return m_rootDirectory; }
        const DirectoryEntry& GetRootDirectory() const { return m_rootDirectory; }

        std::vector<FileEntry*>& GetFileOrder() { return m_fileOrder; }
        const std::vector<FileEntry*>& GetFileOrder() const { return m_fileOrder; }

        size_t NumOrderFiles() const { return m_fileOrder.size(); }
        std::vector<FileEntry*>::iterator beginFileOrder() { return m_fileOrder.begin(); }
        std::vector<FileEntry*>::const_iterator beginFileOrder() const { return m_fileOrder.begin(); }
        std::vector<FileEntry*>::iterator endFileOrder() { return m_fileOrder.end(); }
        std::vector<FileEntry*>::const_iterator endFileOrder() const { return m_fileOrder.end(); }
    };
}
