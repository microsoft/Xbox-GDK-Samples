//--------------------------------------------------------------------------------------
// BaseSubPackage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BaseSubPackage.h"
#include "BasePackage.h"

using namespace Packages;

BaseSubPackage::BaseSubPackage(BasePackage *parent, const std::wstring& name) : m_parent(parent), m_subPackageName(name)
{
    std::hash<std::wstring> hash_fn;
    m_subPackageHashName = hash_fn(m_subPackageName);
}

BaseSubPackage::~BaseSubPackage()
{
}

void BaseSubPackage::AddFileToMap(FileEntry* whichFile)
{
    if (whichFile->nameHash != 0)
    {
        m_fileHashMapping[whichFile->nameHash] = whichFile;
        m_parent->AddFileToMap(whichFile->nameHash, this);
    }
    m_fileNameMapping[whichFile->fileName] = whichFile;
    m_parent->AddFileToMap(whichFile->fileName, this);
}

void BaseSubPackage::AddFileToMap(const std::wstring& name, FileEntry* whichFile)
{
    m_fileNameMapping[name] = whichFile;
    m_parent->AddFileToMap(name, this);
}

void BaseSubPackage::AddFileToMap(uint64_t nameHash, FileEntry* whichFile)
{
    m_fileHashMapping[nameHash] = whichFile;
    m_parent->AddFileToMap(nameHash, this);
}

FileEntry *BaseSubPackage::FindFileEntry(uint64_t fileNameHash) const
{
    const auto iter = m_fileHashMapping.find(fileNameHash);
    if (iter != m_fileHashMapping.end())
        return iter->second;
    return nullptr;
}

namespace Packages
{
    DataFileType ConvertStringToDataFileType(const std::string& str)
    {
        if (str.compare("null") == 0)
            return fileType_null;
        if (str.compare("meta") == 0)
            return fileType_meta;
        if (str.compare("animation") == 0)
            return fileType_animation;
        if (str.compare("skeleton") == 0)
            return fileType_skeleton;
        if (str.compare("skeletonMesh") == 0)
            return fileType_skeletonMesh;
        if (str.compare("terrainMesh") == 0)
            return fileType_terrainMesh;
        if (str.compare("skeletonTexture") == 0)
            return fileType_skeletonTexture;
        if (str.compare("terrainTexture") == 0)
            return fileType_terrainTexture;
        if (str.compare("terrain") == 0)
            return fileType_terrain;
        if (str.compare("data") == 0)
            return fileType_data;
        return fileType_null;
    }

    std::string ConvertDataFileTypeToString(DataFileType fileType)
    {
        switch (fileType)
        {
        case DataFileType::fileType_null:
            return "null";
            break;
        case DataFileType::fileType_meta:
            return "meta";
            break;
        case DataFileType::fileType_animation:
            return "animation";
            break;
        case DataFileType::fileType_skeleton:
            return "skeleton";
            break;
        case DataFileType::fileType_terrainMesh:
            return "terrainMesh";
            break;
        case DataFileType::fileType_skeletonMesh:
            return "skeletonMesh";
            break;
        case DataFileType::fileType_terrainTexture:
            return "terrainTexture";
            break;
        case DataFileType::fileType_skeletonTexture:
            return "skeletonTexture";
            break;
        case DataFileType::fileType_terrain:
            return "terrain";
            break;
        case DataFileType::fileType_data:
            return "data";
            break;
        case DataFileType::NUM_DATA_FILE_TYPE:
            return "null";
            break;
        }
        return "null";
    }

    std::wstring ConvertDataFileTypeToFileName(DataFileType fileType)
    {
        switch (fileType)
        {
        case DataFileType::fileType_null:
            return L"null_file";
            break;
        case DataFileType::fileType_meta:
            return L"meta_file";
            break;
        case DataFileType::fileType_animation:
            return L"animation_file";
            break;
        case DataFileType::fileType_skeleton:
            return L"skeleton_file";
            break;
        case DataFileType::fileType_terrainMesh:
            return L"terrainMesh_file";
            break;
        case DataFileType::fileType_skeletonMesh:
            return L"skeletonMesh_file";
            break;
        case DataFileType::fileType_terrainTexture:
            return L"terrainTexture_file";
            break;
        case DataFileType::fileType_skeletonTexture:
            return L"skeletonTexture_file";
            break;
        case DataFileType::fileType_terrain:
            return L"terrain_file";
            break;
        case DataFileType::fileType_data:
            return L"data_file";
            break;
        case DataFileType::NUM_DATA_FILE_TYPE:
            return L"null_file";
            break;
        }
        return L"null_file";
    }

    std::wstring ConvertDataFileTypeToDirName(DataFileType fileType)
    {
        switch (fileType)
        {
        case DataFileType::fileType_null:
            return L"";
            break;
        case DataFileType::fileType_meta:
            return L"meta";
            break;
        case DataFileType::fileType_animation:
            return L"animation";
            break;
        case DataFileType::fileType_skeleton:
            return L"skeleton";
            break;
        case DataFileType::fileType_terrainMesh:
            return L"terrainMesh";
            break;
        case DataFileType::fileType_skeletonMesh:
            return L"skeletonMesh";
            break;
        case DataFileType::fileType_terrainTexture:
            return L"terrainTexture";
            break;
        case DataFileType::fileType_skeletonTexture:
            return L"skeletonTexture";
            break;
        case DataFileType::fileType_terrain:
            return L"terrain";
            break;
        case DataFileType::fileType_data:
            return L"data";
            break;
        case DataFileType::NUM_DATA_FILE_TYPE:
            return L"";
            break;
        }
        return L"";
    }
}

DirectoryEntry *DirectoryEntry::FindDirectory(const std::wstring& findName)
{
    for (const auto& iter : directories)
    {
        if (findName == iter->dirName)
            return iter.get();
    }
    return nullptr;
}

//FileEntry *DirectoryEntry::FindFile(const std::wstring& findName)
//{
//	for (const auto& iter : files)
//	{
//		if (findName == iter->fileName)
//			return iter.get();
//	}
//	return nullptr;
//}

FileEntry *DirectoryEntry::FindFile(uint64_t fileNameHash)
{
    for (const auto& iter : files)
    {
        if (fileNameHash == iter->nameHash)
            return iter.get();
    }

    for (const auto& iter : directories)
    {
        auto toret = iter->FindFile(fileNameHash);
        if (toret != 0)
            return toret;
    }

    return nullptr;
}
