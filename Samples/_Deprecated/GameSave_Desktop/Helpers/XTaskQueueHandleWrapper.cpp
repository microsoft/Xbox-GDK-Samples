//--------------------------------------------------------------------------------------
// XTaskQueueHandleWrapper.cpp
//
// Task Queue Wrapper.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "XTaskQueueHandleWrapper.h"

/**
 * Creates a new XTaskQueue object, returning the TaskQueueHandle object which owns its handle.
 * 
 * \param XTaskQueueDispatchMode workDispatchMode - the mode to use when creating the task queue's worker port.
 * \param XTaskQueueDispatchMode completionDispatchMode - the mode to use when creating the task queue's completion port.
 * \return ATG::TaskQueueHandle - the wrapper object which contains the handle to the new XTaskQueue instance.
 */
ATG::TaskQueueHandle ATG::TaskQueueHandle::Create(XTaskQueueDispatchMode workDispatchMode /*= XTaskQueueDispatchMode::ThreadPool*/, XTaskQueueDispatchMode completionDispatchMode /*= XTaskQueueDispatchMode::ThreadPool*/)
{
   XTaskQueueHandle handle;
   HRESULT hr = ::XTaskQueueCreate(workDispatchMode, completionDispatchMode, &handle);
   DX::ThrowIfFailed(hr);
   return TaskQueueHandle(handle);
}
