//--------------------------------------------------------------------------------------
// CapturedReadData.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CapturedReadData.h"

bool CapturedReadData::OpenFiles(const std::string& dataName, const std::string& dataSetName, BaseFileInterface *fileInterface, uint32_t creationFlags)
{
    CloseFiles(fileInterface);

    FullDataMap::iterator curDataSet = m_fullDataSet.begin();
    if (dataSetName.size() != 0)
        curDataSet = m_fullDataSet.find(dataSetName);
    if (curDataSet == m_fullDataSet.end())
        return false;

    bool needUnaligned = curDataSet->second.GetNeedUnaligned(dataName);
    readIterator iter, endIter;
    endIter = curDataSet->second.end(dataName);
    for (iter = curDataSet->second.begin(dataName); iter != endIter; ++iter)
    {
        BaseFileInterface::FileHandle newFile = BaseFileInterface::s_invalidFileHandle;
        uintptr_t platformHandle;
        if (needUnaligned)
            newFile = fileInterface->OpenFile(iter->fileName, OPEN_EXISTING, 0, platformHandle);
        else
            newFile = fileInterface->OpenFile(iter->fileName, OPEN_EXISTING, creationFlags, platformHandle);
        if (newFile == BaseFileInterface::s_invalidFileHandle)
            DebugBreak();
        //return false;
        iter->fileHandle = newFile;
        iter->platformFileHandle = platformHandle;
        m_openFiles.push_back(newFile);
    }
    return true;
}

void CapturedReadData::CloseFiles(BaseFileInterface *fileInterface)
{
    for (auto& iter : m_openFiles)
    {
        fileInterface->CloseFile(iter);
    }
    m_openFiles.clear();
}
