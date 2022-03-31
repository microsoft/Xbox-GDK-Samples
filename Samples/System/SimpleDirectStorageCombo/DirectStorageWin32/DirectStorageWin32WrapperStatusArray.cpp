//--------------------------------------------------------------------------------------
// DirectStorageWin32WrapperStatusArray.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DirectStorageWin32Wrapper.h"

using namespace DirectStorageWin32Wrapper;
using namespace DirectStorageWin32Wrapper::Internal;

//////////////////////////////////////////////////////////////////////////
/// \brief DirectStorageStatusArray
/// \details DirectStorageStatusArray::DirectStorageStatusArray
/// \details Construct a new instance of DirectStorageStatusArray
/// \param capacity The number of entries in the status array
//////////////////////////////////////////////////////////////////////////
DirectStorageStatusArray::DirectStorageStatusArray(uint32_t capacity, const std::string& name) : m_refCount(0), m_name(name)
{
	m_completeFlags.resize(capacity, true);
	m_resultCodes.resize(capacity, HRESULT_FROM_WIN32(S_OK));
}

//////////////////////////////////////////////////////////////////////////
/// \brief IsComplete
/// \details DirectStorageStatusArray::IsComplete
/// \details Check if a slot in the status array is marked as completed
/// \param index Which slot to check for completion
/// \return Whether the matching slot is complete, all previous requests in the queue have completed
//////////////////////////////////////////////////////////////////////////
bool DirectStorageStatusArray::IsComplete(UINT32 index)
{
	if (index >= m_completeFlags.size())
		return false;
	return m_completeFlags[index];
}

//////////////////////////////////////////////////////////////////////////
/// \brief GetHResult
/// \details DirectStorageStatusArray::GetHResult
/// \details Returns the HRESULT of all previous requests in the queue
/// \details E_PENDING if there are still requests that are not complete
/// \details S_OK if all requests completed successfully
/// \details Error code for first failed request before this status entry
/// \param index Which slot to check for matching HRESULT
/// \return HRESULT of previous requests, first failed, pending, or ok
//////////////////////////////////////////////////////////////////////////
HRESULT DirectStorageStatusArray::GetHResult(UINT32 index)
{
	if (index >= m_completeFlags.size())
		return HRESULT_FROM_WIN32(S_FALSE);
	if (!m_completeFlags[index])
		return E_PENDING;
	return m_resultCodes[index];
}

//////////////////////////////////////////////////////////////////////////
/// \brief MarkState
/// \details DirectStorageStatusArray::MarkState
/// \details Updates the slot entry with its completion state and matching HRESULT
/// \param index Which slot to update
/// \param complete Did the slot complete, false is used when the entry is placed in a queue
/// \param error The matching HRESULT for the requested slot
//////////////////////////////////////////////////////////////////////////
void DirectStorageStatusArray::MarkState(uint32_t index, bool complete, HRESULT error)
{
	assert(index < m_completeFlags.size());
	if (index < m_completeFlags.size())
	{
		m_resultCodes[index] = error;
		m_completeFlags[index] = complete;
	}
}

//////////////////////////////////////////////////////////////////////////
/// \brief QueryInterface
/// \details DirectStorageStatusArray::QueryInterface
/// \details Instantiation of IUnknown::QueryInterface
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE DirectStorageStatusArray::QueryInterface(/* [in] */ REFIID riid,/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
{
	if (riid != __uuidof(IDStorageStatusArrayWin32))
		return E_NOINTERFACE;
	*ppvObject = this;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
/// \brief AddRef
/// \details DirectStorageStatusArray::AddRef
/// \details Instantiation of IUnknown::AddRef
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageStatusArray::AddRef(void)
{
	m_refCount++;
	return m_refCount.load();
}

//////////////////////////////////////////////////////////////////////////
/// \brief Release
/// \details DirectStorageStatusArray::Release
/// \details Instantiation of IUnknown::Release
//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE DirectStorageStatusArray::Release(void)
{
	assert(m_refCount > 0);
	if (m_refCount.fetch_sub(1) == 1)
	{
		delete this;
		return 0;
	}
	return m_refCount.load();
}