//--------------------------------------------------------------------------------------
// ReadSet.h
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

#include "BaseFileInterface.h"

typedef rapidxml::xml_document<char> XMLDoc;
typedef rapidxml::xml_node<char> XMLNode;
typedef rapidxml::xml_attribute<char> XMLAttribute;

class FullReadSet
{
public:
    struct Read
    {
        uint32_t size;
        uint64_t location;
        std::wstring fileName;
        uint64_t startTime;			// in microseconds
        uint64_t stopTime;			// in microseconds
        BaseFileInterface::FileHandle fileHandle;
        uintptr_t platformFileHandle;
        char* dataBuffer;
        uintptr_t* physicalPageArray;
        size_t  sizePhysicalPageArray;
        uint16_t physicalOffset;

        Read()
        {
            size = 0;
            location = 0;
            startTime = 0;
            stopTime = 0;
            platformFileHandle = 0;
            dataBuffer = nullptr;
            fileHandle = BaseFileInterface::s_invalidFileHandle;
            physicalPageArray = nullptr;
            sizePhysicalPageArray = 0;
            physicalOffset = 0;
        }
        Read(const Read& rhs) = default;
        Read(Read&& rhs) noexcept(false)
        {
            fileName = std::move(rhs.fileName);
            size = rhs.size;
            location = rhs.location;
            fileHandle = rhs.fileHandle;
            dataBuffer = rhs.dataBuffer;
            startTime = rhs.startTime;
            stopTime = rhs.stopTime;
            platformFileHandle = rhs.platformFileHandle;
            physicalPageArray = rhs.physicalPageArray;
            sizePhysicalPageArray = rhs.sizePhysicalPageArray;
            physicalOffset = rhs.physicalOffset;
        }
        Read(const std::wstring& newName, uint32_t newSize, uint64_t newLocation, uint64_t newStart, uint64_t newStop) : Read()
        {
            fileName = newName;
            size = newSize;
            location = newLocation;
            startTime = newStart;
            stopTime = newStop;
        }
        Read& operator= (const Read& rhs) = default;
    };
    typedef std::vector<Read> ReadList;

    struct ReadSet
    {
        std::string m_name;
        uint32_t m_qdi;
        ReadList m_reads;
        bool m_needUnaligned = false;

		ReadSet()
		{
			m_qdi = 1;
			m_needUnaligned = false;
		}
		ReadSet(const ReadSet&) = default;
		ReadSet(ReadSet&& rhs) noexcept(false)
		{
			m_name = std::move(rhs.m_name);
			m_qdi = rhs.m_qdi;
			m_reads = std::move(rhs.m_reads);
		}
		ReadSet(const std::string& newName, uint32_t newQdi) { m_name = newName; m_qdi = std::max<uint32_t>(1, newQdi); }
		ReadSet& operator= (const ReadSet& rhs) = default;
		void AddRead(const Read& newRead) { m_reads.emplace_back(newRead); }
	};

private:
	static std::mutex s_closeWriteLog;
	std::wstring m_name;
	std::vector<BaseFileInterface::FileHandle> m_openFiles;
	bool ParseFullReadSet(XMLNode* rootNode);
	bool ParseReadSet(XMLNode* rootNode);

    typedef std::unordered_map<std::string, ReadSet> FullDataMap;
    FullDataMap m_fullDataSet;

    std::vector<std::string> m_dataSetNames;

    bool ReadDataSetXMLFile(XMLDoc& document);

public:
    typedef ReadList::iterator iterator;
    typedef ReadList::const_iterator const_iterator;
    typedef std::vector<std::string>::iterator dataSetNamesIterator;
    typedef std::vector<std::string>::const_iterator dataSetNamesConstIterator;

    FullReadSet(const std::wstring& name = L"") : m_name(name) {}
    virtual ~FullReadSet() {  }

    bool ReadDataSetXMLFile(const std::string& dataContents);
    bool WriteDataSetXMLFile(const std::wstring& dataSetFileName);

    void AddReadSet(const ReadSet& newSet) { if (m_fullDataSet.find(newSet.m_name) == m_fullDataSet.end()) m_dataSetNames.push_back(newSet.m_name); m_fullDataSet[newSet.m_name] = newSet; }

    const std::wstring& GetName() const { return m_name; }
    void SetName(const std::wstring& newName) { m_name = newName; }

    uint32_t GetReadSetQDI(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName);  return iter->second.m_qdi; }
    size_t GetNumReads(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName);  return iter->second.m_reads.size(); }
    bool GetNeedUnaligned(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName);  return iter->second.m_needUnaligned; }

    uint32_t GetReadSetMaxQDI() const
    {
        uint32_t toret = 0;
        for (const auto& iter : m_fullDataSet)
        {
            if (iter.second.m_qdi > toret)
                toret = iter.second.m_qdi;
        }
        return toret;
    }
    size_t GetNumCapturedSets() const { return m_fullDataSet.size(); }

    bool OpenFiles(const std::string& dataSet, BaseFileInterface* fileInterface);
    void CloseFiles(BaseFileInterface* fileInterface);

    iterator begin(const std::string& dataSetName) { return m_fullDataSet[dataSetName].m_reads.begin(); }
    const_iterator begin(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName); return iter->second.m_reads.begin(); }
    iterator end(const std::string& dataSetName) { return m_fullDataSet[dataSetName].m_reads.end(); }
    const_iterator end(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName); return iter->second.m_reads.end(); }

    dataSetNamesIterator begin() { return m_dataSetNames.begin(); }
    dataSetNamesConstIterator begin() const { return m_dataSetNames.begin(); }
    dataSetNamesIterator end() { return m_dataSetNames.end(); }
    dataSetNamesConstIterator end() const { return m_dataSetNames.end(); }

    ReadSet& GetReadSet(const std::string& dataSetName) { FullDataMap::iterator iter = m_fullDataSet.find(dataSetName);  return iter->second; }
    ReadSet& GetReadSet(size_t index) { FullDataMap::iterator iter = m_fullDataSet.find(m_dataSetNames[index]);  return iter->second; }
    const ReadSet& GetReadSet(const std::string& dataSetName) const { FullDataMap::const_iterator iter = m_fullDataSet.find(dataSetName);  return iter->second; }
    const ReadSet& GetReadSet(size_t index) const { FullDataMap::const_iterator iter = m_fullDataSet.find(m_dataSetNames[index]);  return iter->second; }
};
