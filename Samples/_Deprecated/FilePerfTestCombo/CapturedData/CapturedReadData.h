//--------------------------------------------------------------------------------------
// CapturedReadData.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterface.h"
#include "ReadSet.h"

class CapturedReadData
{
protected:
    std::string m_name;
    std::vector<BaseFileInterface::FileHandle> m_openFiles;
    std::vector<uint64_t> m_openDStorageFiles;

    typedef std::unordered_map<std::string, FullReadSet> FullDataMap;
    FullDataMap m_fullDataSet;

    std::vector<std::string> m_dataSetNames;

public:
    typedef FullReadSet::ReadList::iterator readIterator;
    typedef FullReadSet::ReadList::const_iterator const_readIterator;
    typedef std::vector<std::string>::iterator dataSetNamesIterator;
    typedef std::vector<std::string>::const_iterator dataSetNamesConstIterator;

    CapturedReadData() {}

    virtual ~CapturedReadData() {}

    virtual bool ReadSingleDataSetFile(const std::wstring& file, const std::wstring& baseDirectory)
    {
        std::vector<std::wstring> tempVector;
        tempVector.push_back(file);
        return ReadDataSetFile(tempVector, baseDirectory);
    }
    virtual bool ReadDataSetFile(const std::vector<std::wstring>& files, const std::wstring& baseDirectory) = 0;
    virtual bool WriteDataSetFile(const std::wstring& outputFileName, const std::string& dataName, const std::string& dataSetName) = 0;

    virtual bool AddReadSet(const FullReadSet& newSet)
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;
        std::string narrowString = converter.to_bytes(newSet.GetName());

        if (m_fullDataSet.find(narrowString) == m_fullDataSet.end())
            m_dataSetNames.push_back(narrowString);
        m_fullDataSet[narrowString] = newSet;
        return true;
    }

    const FullReadSet& GetFullReadSet(const std::string& name) { return m_fullDataSet[name]; }

    uint32_t GetCapturedDataQDI(const std::string& dataName, const std::string& dataSetName) const { FullDataMap::const_iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName);  return iter->second.GetReadSetQDI(dataName); }
    size_t GetNumReads(const std::string& dataName, const std::string& dataSetName) const { FullDataMap::const_iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName);  return iter->second.GetNumReads(dataName); }
    bool GetNeedUnaligned(const std::string& dataName, const std::string& dataSetName) const { FullDataMap::const_iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName);  return iter->second.GetNeedUnaligned(dataName); }

    size_t GetNumCapturedSets() const { return m_fullDataSet.size(); }

    virtual bool OpenFiles(const std::string& dataName, const std::string& dataSetName, BaseFileInterface *fileInterface, uint32_t creationFlags);
    virtual void CloseFiles(BaseFileInterface *fileInterface);

    readIterator begin(const std::string& dataName, const std::string& dataSetName) { FullDataMap::iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName); return iter->second.begin(dataName); }
    const_readIterator begin(const std::string& dataName, const std::string& dataSetName) const { FullDataMap::const_iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName); return iter->second.begin(dataName); }
    readIterator end(const std::string& dataName, const std::string& dataSetName) { FullDataMap::iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName); return iter->second.end(dataName); }
    const_readIterator end(const std::string& dataName, const std::string& dataSetName) const { FullDataMap::const_iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataSetName); return iter->second.end(dataName); }

    dataSetNamesIterator begin() { return m_dataSetNames.begin(); }
    dataSetNamesConstIterator begin() const { return m_dataSetNames.begin(); }
    dataSetNamesIterator end() { return m_dataSetNames.end(); }
    dataSetNamesConstIterator end() const { return m_dataSetNames.end(); }

    const FullReadSet& GetCapturedSet(const std::string& dataSetName) const { if (dataSetName.size() == 0) return m_fullDataSet.begin()->second; FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName);  return iter->second; }
    FullReadSet& GetCapturedSet(const std::string& dataSetName) { if (dataSetName.size() == 0) return m_fullDataSet.begin()->second; FullDataMap::iterator iter = m_fullDataSet.find(dataSetName);  return iter->second; }
};
