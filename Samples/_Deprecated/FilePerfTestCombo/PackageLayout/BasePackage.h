//--------------------------------------------------------------------------------------
// BasePackage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseSubPackage.h"

namespace Packages
{
    class BasePackage
    {
    protected:
        std::wstring m_layoutFileName;
        std::vector<BaseSubPackage *> m_subPackages;

        std::map<std::wstring, BaseSubPackage*> m_fileNameMapping;
        std::map<uint64_t, BaseSubPackage*> m_fileHashMapping;

    public:
        BasePackage() noexcept;
        virtual ~BasePackage();

        virtual bool ReadDataFile(const std::wstring& /*layoutFileName*/, const std::wstring& /*orderFileName*/) { return false; }
        virtual bool SaveDataFile(const std::wstring& /*layoutFileName*/, const std::wstring& /*orderFileName*/) { return false; }

        virtual bool CreateMatchingFileLayout(const std::wstring& /*rootDirectory*/, uint32_t /*compressionRatio*/) { return false; }

        void SetLayoutName(const std::wstring& newName) { m_layoutFileName = newName; }
        const std::wstring& GetLayoutName() const { return m_layoutFileName; }

        FileEntry *FindFileEntry(uint64_t fileNameHash) const;
        BaseSubPackage *FindFileChunk(uint64_t fileNameHash) const;

        virtual void AddFileToMap(const std::wstring& name, BaseSubPackage* whichChunk) { m_fileNameMapping[name] = whichChunk; }
        virtual void AddFileToMap(uint64_t nameHash, BaseSubPackage* whichChunk) { m_fileHashMapping[nameHash] = whichChunk; }

        virtual size_t NumFiles() { return 00; }

        size_t NumSubPackages() const { return m_subPackages.size(); }
        std::vector<BaseSubPackage*>::iterator beginSubPackage() { return m_subPackages.begin(); }
        std::vector<BaseSubPackage*>::const_iterator beginSubPackage() const { return m_subPackages.begin(); }
        std::vector<BaseSubPackage*>::iterator endSubPackage() { return m_subPackages.end(); }
        std::vector<BaseSubPackage*>::const_iterator endSubPackage() const { return m_subPackages.end(); }
    };
}
