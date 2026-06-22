//--------------------------------------------------------------------------------------
// ReadSet.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ReadSet.h"
#include "FileLogger.h"

using namespace rapidxml;
std::mutex FullReadSet::s_closeWriteLog;

bool FullReadSet::ParseReadSet(XMLNode* rootNode)
{
    ReadSet currentDataSet;
    std::string rootName("unnamed");

    XMLAttribute* attr = rootNode->first_attribute("name");
    if (attr)
        rootName = attr->value();
    attr = rootNode->first_attribute("maxQueueDepth");
    if (attr)
        currentDataSet.m_qdi = static_cast<uint32_t> (atoi(attr->value()));

    XMLNode* dataNode = rootNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "DataRead") == 0)
        {
            Read newData;
            XMLAttribute* curAttr = dataNode->first_attribute();
            while (curAttr)
            {
                if (strcmp(curAttr->name(), "fileName") == 0)
                {
                    int inputSize = static_cast<int> (strlen(curAttr->value()) + 1);
                    wchar_t outputBuffer[256];
                    int outputSize = 256;
                    outputSize = MultiByteToWideChar(CP_UTF8, 0, curAttr->value(), inputSize, outputBuffer, outputSize);
                    outputBuffer[outputSize] = 0;
                    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                    //newData.fileName = converter.from_bytes(curAttr->value());
                    newData.fileName = outputBuffer;
                }
                else if (strcmp(curAttr->name(), "location") == 0)
                {
                    newData.location = static_cast<uint64_t> (atoll(curAttr->value()));
                }
                else if (strcmp(curAttr->name(), "startTime") == 0)
                {
                    newData.startTime = static_cast<uint64_t> (atoll(curAttr->value()));
                }
                else if (strcmp(curAttr->name(), "stopTime") == 0)
                {
                    newData.stopTime = static_cast<uint64_t> (atoll(curAttr->value()));
                }
                else if (strcmp(curAttr->name(), "size") == 0)
                {
                    newData.size = static_cast<uint32_t> (atoi(curAttr->value()));
                    if ((newData.size % 4096) != 0)
                        currentDataSet.m_needUnaligned = true;
                }
                curAttr = curAttr->next_attribute();
            }
            newData.fileName = BaseFileInterface::NormalizeFilename(newData.fileName);
            currentDataSet.m_reads.emplace_back(newData);
        }
        dataNode = dataNode->next_sibling();
    }

    if (currentDataSet.m_needUnaligned)
        assert(currentDataSet.m_qdi == 1);

    if (m_fullDataSet.find(rootName) != m_fullDataSet.end())
    {
        char newName[256];
        uint32_t index = 1;
        do
        {
            sprintf_s(newName, 256, "%s_%d", rootName.c_str(), index);
            ++index;
        } while (m_fullDataSet.find(newName) != m_fullDataSet.end());
        rootName = newName;
    }

    currentDataSet.m_name = rootName;
    if (currentDataSet.m_needUnaligned)
        assert(currentDataSet.m_qdi == 1);
    m_fullDataSet[currentDataSet.m_name] = currentDataSet;
    m_dataSetNames.emplace_back(currentDataSet.m_name);
    return true;
}

bool FullReadSet::ReadDataSetXMLFile(const std::string& dataContents)
{
    auto xmlDoc = std::make_unique<XMLDoc>();

    // RapidXML requires the  contents of the string buffer be stable through the lifetime of the xml document
    // so we want to make a copy of that buffer so the caller can free their copy
    size_t contentsLen = dataContents.size();
    std::unique_ptr<char[]> writableContents(new char[contentsLen + 1]);
    memset(writableContents.get(), 0, contentsLen + 1);
    memcpy(writableContents.get(), dataContents.c_str(), contentsLen);
    try
    {
        xmlDoc->parse<parse_full>(writableContents.get());
    }
    catch (std::exception& /*error*/)
    {
        return false;
    }
    return ReadDataSetXMLFile(*xmlDoc);
}

bool FullReadSet::ParseFullReadSet(XMLNode* rootNode)
{
    XMLNode* dataNode = rootNode->first_node();
    while (dataNode)
    {
        if (strcmp(dataNode->name(), "DataSet") == 0)
        {
            ParseReadSet(dataNode);
        }
        dataNode = dataNode->next_sibling();
    }
    return true;
}

bool FullReadSet::ReadDataSetXMLFile(XMLDoc& document)
{
    XMLNode* curNode = document.first_node();
    while (curNode)
    {
        if (strcmp(curNode->name(), "FullDataSet") == 0)
        {
            return ParseFullReadSet(curNode);
        }
        if (strcmp(curNode->name(), "DataSet") == 0)
        {
            if (!ParseReadSet(curNode))
                return false;
        }
        curNode = curNode->next_sibling();
    }
    return true;
}

bool FullReadSet::WriteDataSetXMLFile(const std::wstring& dataSetFileName)
{
    if (m_name.size() == 0)
    {
        std::filesystem::path path(dataSetFileName);
        m_name = path.stem().wstring();
    }
    wchar_t tempTextBuffer[128];
    ATG::FileLogger* readSetOutput = new ATG::FileLogger(dataSetFileName, false, false, false, false, true, L"ReadSets");
    for (const auto& readSet : m_fullDataSet)
    {
        swprintf(tempTextBuffer, 128, L"<DataSet name = \"%hs\" maxQueueDepth=\"%d\">", readSet.first.c_str(), readSet.second.m_qdi);
        readSetOutput->Log(tempTextBuffer);
        for (const auto& read : readSet.second.m_reads)
        {
            wchar_t setTextBuffer[256];
            swprintf(setTextBuffer, 256, L"<DataRead fileName=\"%s\" location=\"%Iu\" size=\"%u\" startTime=\"%Iu\" stopTime=\"%Iu\"/>", read.fileName.c_str(), read.location, read.size, read.startTime, read.stopTime);
            readSetOutput->Log(setTextBuffer);
        }
        readSetOutput->Log(L"</DataSet>");
    }
    {
        std::lock_guard fred(s_closeWriteLog);
        delete readSetOutput;
    }
    return true;
}

bool FullReadSet::OpenFiles(const std::string& dataSet, BaseFileInterface* fileInterface)
{
    CloseFiles(fileInterface);
    FullDataMap::iterator curDataSet = m_fullDataSet.find(dataSet);
    if (curDataSet == m_fullDataSet.end())
        return false;
    for (auto& curRead : curDataSet->second.m_reads)
    {
        uintptr_t platformHandle;
        BaseFileInterface::FileHandle newFile = BaseFileInterface::s_invalidFileHandle;
        if ((curDataSet->second.m_needUnaligned) || (curDataSet->second.m_qdi == 1))
            newFile = fileInterface->OpenFile(curRead.fileName, OPEN_EXISTING, 0, platformHandle);
        else
            newFile = fileInterface->OpenFile(curRead.fileName, OPEN_EXISTING, FileSystem::File::e_openFlag_EnableAsync, platformHandle);
        if (newFile == BaseFileInterface::s_invalidFileHandle)
            return false;
        curRead.fileHandle = newFile;
        curRead.platformFileHandle = platformHandle;
        m_openFiles.push_back(newFile);
    }
    return true;
}

void FullReadSet::CloseFiles(BaseFileInterface* fileInterface)
{
    for (auto& iter : m_openFiles)
    {
        fileInterface->CloseFile(iter);
    }
    m_openFiles.clear();
}
