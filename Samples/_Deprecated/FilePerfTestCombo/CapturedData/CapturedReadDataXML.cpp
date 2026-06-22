//--------------------------------------------------------------------------------------
// CapturedReadDataXML.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CapturedReadDataXML.h"
#include "FileLogger.h"

using namespace rapidxml;

bool CapturedReadDataXML::ReadDataSetFile(const std::vector<std::wstring>& files, const std::wstring& /*baseDirectory*/)
{
    for (const auto& fileName : files)
    {
        FullReadSet readSet(L"");
        std::string fileContents;
        BaseFileInterface::ReadFile(fileName, fileContents);
        if (!readSet.ReadDataSetXMLFile(fileContents))
            return false;
        if (readSet.GetName().size() == 0)
        {
            std::filesystem::path path(fileName);
            readSet.SetName(path.stem().wstring());
        }
        AddReadSet(readSet);
    }
    return true;
}

bool CapturedReadDataXML::WriteDataSetFile(const std::wstring& outputFileName, const std::string& dataName, const std::string& dataSetName)
{
    FullDataMap::iterator iter = dataSetName.size() == 0 ? m_fullDataSet.begin() : m_fullDataSet.find(dataName);

    return iter->second.WriteDataSetXMLFile(outputFileName);
}
