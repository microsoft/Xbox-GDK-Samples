//--------------------------------------------------------------------------------------
// DirectStorageWin32WrapperFile.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DirectStorageWin32Wrapper.h"

using namespace DirectStorageWin32Wrapper;
using namespace DirectStorageWin32Wrapper::Internal;

//////////////////////////////////////////////////////////////////////////
/// \brief ~DirectStorageFile
/// \details DirectStorageFile::~DirectStorageFile
/// \details Instance does not own the matching file handle and must request the DirectStorageFactory instance to close the file
//////////////////////////////////////////////////////////////////////////
DirectStorageFile::~DirectStorageFile()
{
	if (m_file)
		DirectStorageFactory::GetInstance()->RemoveFile(m_file);
}

//////////////////////////////////////////////////////////////////////////
/// \brief GetHandle
/// \details DirectStorageFile::GetHandle
/// \details Returns a copy of the file handle being used by DirectStorage
/// \details This file handle needs to closed as well to avoid leaks
//////////////////////////////////////////////////////////////////////////
HANDLE DirectStorageFile::GetHandle()
{
	HANDLE toret;
	if (DuplicateHandle(GetCurrentProcess(), m_file, GetCurrentProcess(), &toret, 0, TRUE, DUPLICATE_SAME_ACCESS) == 0)
		return INVALID_HANDLE_VALUE;
	return toret;
}

//////////////////////////////////////////////////////////////////////////
/// \brief Close
/// \details DirectStorageFile::Close
/// \details Title can request the file to close as opposed to letting the destructor close the file
/// \details Instance does not own the matching file handle and must request the DirectStorageFactory instance to close the file
//////////////////////////////////////////////////////////////////////////
void DirectStorageFile::Close()
{
	assert(m_file != nullptr);
	DirectStorageFactory::GetInstance()->RemoveFile(m_file);
	m_file = nullptr;
}

//////////////////////////////////////////////////////////////////////////
/// \brief QueryInterface
/// \details DirectStorageFile::QueryInterface
/// \details Instantiation of IUnknown::QueryInterface
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE DirectStorageFile::QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	if (riid != __uuidof(IDStorageFileWin32))
		return E_NOINTERFACE;
	*ppvObject = this;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AddRef
/// \details DirectStorageFile::AddRef
/// \details Instantiation of IUnknown::AddRef
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageFile::AddRef(void)
{
	m_refCount++;
	return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief Release
/// \details DirectStorageFile::Release
/// \details Instantiation of IUnknown::Release
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageFile::Release(void)
{
	assert(m_refCount > 0);
	if (m_refCount.fetch_sub(1) == 1)
	{
		delete this;
		return 0;
	}
	return m_refCount.load();
}
