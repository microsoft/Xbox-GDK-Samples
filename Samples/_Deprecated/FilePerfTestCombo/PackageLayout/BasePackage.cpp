//--------------------------------------------------------------------------------------
// BasePackage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "BasePackage.h"

using namespace Packages;

BasePackage::BasePackage() noexcept
{
}

BasePackage::~BasePackage()
{
    for (auto& iter : m_subPackages)
        delete iter;
}

FileEntry *BasePackage::FindFileEntry(uint64_t fileNameHash) const
{
    const auto iter = m_fileHashMapping.find(fileNameHash);
    if (iter != m_fileHashMapping.end())
        return iter->second->FindFileEntry(fileNameHash);

    return nullptr;
}

BaseSubPackage *BasePackage::FindFileChunk(uint64_t fileNameHash) const
{
    const auto iter = m_fileHashMapping.find(fileNameHash);
    if (iter != m_fileHashMapping.end())
        return iter->second;

    return nullptr;
}
