//--------------------------------------------------------------------------------------
// LooseFiles.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LooseFiles.h"

LooseFiles::LooseFiles()
{
}

LooseFiles::~LooseFiles()
{
}

LooseFiles::FileHandle LooseFiles::OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle)
{
    std::wstring fullName;

	platformHandle = 0;
	if (m_directoryOffset.size())
	{
		fullName = m_directoryOffset;
		fullName += L"\\";
		fullName += fileName;
	}
	else
	{
		fullName = fileName;
	}
	uint64_t nameHash = GenerateFilenameHash(fullName);
	{
		auto iter = m_openFiles.find(nameHash);
		if (iter != m_openFiles.end())
		{
			iter->second.refCount++;
			return nameHash;
		}
	}

    HRESULT errorResult;
    FileSystem::File* file = FileSystem::File::OpenFile(fullName, errorResult, creationDisposition, flags);
    if (!file)
        return s_invalidFileHandle;
    m_openFiles[nameHash] = OpenFileEntry(nameHash, file);
    platformHandle = 0;
    return nameHash;
}

void LooseFiles::CloseFile(FileHandle file)
{
    auto iter = m_openFiles.find(file);
    if (iter != m_openFiles.end())
    {
        iter->second.refCount--;
        if (iter->second.refCount == 0)
        {
            iter->second.file->CloseFile(iter->second.file);
            m_openFiles.erase(iter);
        }
    }
}

size_t LooseFiles::SetReadPointer(FileHandle nameHash, uint64_t newLocation)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return SIZE_MAX;
    return iter->second.file->SetReadPointer(newLocation);
}

HRESULT LooseFiles::ReadFile(FileHandle nameHash, void* data, uint32_t size)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);

    return iter->second.file->Read(data, size);
}

HRESULT LooseFiles::AsyncRead(FileHandle nameHash, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, FileSystem::File::AsyncRequestId& requestId, FileSystem::File::AsyncCallback callback, bool useEvent)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);

    return iter->second.file->AsyncRead(buffer, bytesToRead, readLocation, userData, requestId, callback, useEvent);
}
