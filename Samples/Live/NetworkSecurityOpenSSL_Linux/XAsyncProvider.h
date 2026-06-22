// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#if !defined(__cplusplus)
   #error C++11 required
#endif

#pragma once

#include <stdint.h>
#include "XAsync.h"

extern "C"
{

enum class XAsyncOp : uint32_t
{
    /// <summary>
    /// An async provider is invoked with this opcode during XAsyncBegin or XAsyncBeginAlloc.
    /// If the provider implements this op code, they should start their asynchronous task,
    /// either by calling XAsyncSchedule or through exterior means.  This callback is
    /// called synchronously in the XAsyncBegin call chain, so it should never block.
    /// </summary>
    Begin,

    /// <summary>
    /// An async provider is invoked with this opcode when XAsyncSchedule has been called to
    /// schedule work. Implementations should perform their asynchronous work and then call
    /// XAsyncComplete with the data payload size. If additional work needs to be done they
    /// can schedule it and return E_PENDING.
    /// </summary>
    DoWork,

    /// <summary>
    /// An async provider is invoked with this opcode after an async call completes and the
    /// user needs to get the resulting data payload. The buffer and bufferSize have
    /// been arg checked.
    /// </summary>
    GetResult,

    /// <summary>
    /// An async provider is invoked with this opcode when the async work should be canceled. If
    /// you can cancel your work you should call XAsyncComplete with an error code of E_ABORT when
    /// the work has been canceled.
    /// </summary>
    Cancel,

    /// <summary>
    /// An async provider is invoked with this opcode when the async call is over and
    /// data in the context can be cleaned up.
    /// </summary>
    Cleanup
};

/// <summary>
/// A data block passed to an async provider callback.  Fields in this structure are filled
/// in as the call progresses.
/// </summary>
struct XAsyncProviderData
{
    /// <summary>
    /// The async block for the call.
    /// </summary>
    XAsyncBlock* async;

    /// <summary>
    /// Valid during a GetResult opcode and holds the size of the buffer.  This will
    /// be at least as large as the data size provided to XAsyncComplete.
    /// </summary>
    size_t bufferSize;

    /// <summary>
    /// Valid during a GetResult opcode and holds the output data buffer.
    /// </summary>
    void* buffer;

    /// <summary>
    /// Valid during any opcode this is a user provided context pointer that was provided
    /// to XAsyncBegin.  It should be freed during the Cleanup opcode.
    /// </summary>
    void* context;
};

/// <summary>
/// A callback function that implements the async call. This function will be invoked
/// multiple times with different XAsyncOp operation codes to indicate what work it
/// should perform.
/// </summary>
/// <param name='op'>The async operatiopn to perform.</param>
/// <param name='data'>Data used to track the async call.</param>
/// <seealso cref='XAsyncProviderData' />
typedef HRESULT CALLBACK XAsyncProvider(_In_ XAsyncOp op, _Inout_ const XAsyncProviderData* data);

/// <summary>
/// Initializes an async block for use.  Once begun calls such
/// as XAsyncGetStatus will provide meaningful data. It is assumed the
/// async work will begin on some system defined thread after this call
/// returns. The token and function parameters can be used to help identify
/// mismatched Begin/GetResult calls.  The token is typically the function
/// pointer of the async API you are implementing, and the functionName parameter
/// is typically the __FUNCTION__ compiler macro.
/// </summary>
/// <param name='asyncBlock'>A pointer to the XAsyncBlock that holds data for the call.</param>
/// <param name='context'>An optional context pointer that will be stored in the XAsyncProviderData object passed back to the XAsyncProvider callback.</param>
/// <param name='identity'>An optional arbitrary pointer that can be used to identify this call.</param>
/// <param name='identityName'>An optional string that names the async call.  This is typically the __FUNCTION__ compiler macro.</param>
/// <param name='provider'>The function callback to invoke to implement the async call.</param>
STDAPI XAsyncBegin(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_opt_ void* context,
    _In_opt_ const void* identity,
    _In_opt_ const char* identityName,
    _In_ XAsyncProvider* provider
    ) noexcept;

/// <summary>
/// Schedules a callback to do async work.  Calling this is optional.  If the async work can be done
/// through a system - async mechanism like overlapped IO or async COM, there is no need to schedule
/// work.  If work should be scheduled after a delay, pass the number of ms XAsyncSchedule should wait
/// before it schedules work.
/// </summary>
/// <param name='asyncBlock'>A pointer to the XAsyncBlock that was passed to XAsyncBegin.</param>
/// <param name='delayInMs'>How long the system should wait before scheduling the async call with the async queue.</param>
STDAPI XAsyncSchedule(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ uint32_t delayInMs
    ) noexcept;

/// <summary>
/// Called when async work is completed and the results can be returned.
/// The caller should supply the resulting data payload size.  If the call
/// has no data payload, pass zero.
/// </summary>
/// <param name='asyncBlock'>A pointer to the XAsyncBlock that was passed to XAsyncBegin.</param>
/// <param name='result'>The resut of the call.  This should not be E_PENDING as that result is reserved for an incomplete call. If you are canceling this call you should pass E_ABORT.</param>
/// <param name='requiredBufferSize'>The required size in bytes of the call result.  If the call has no data to return this should be zero.</param>
STDAPI_(void) XAsyncComplete(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_ HRESULT result,
    _In_ size_t requiredBufferSize
    ) noexcept;

/// <summary>
/// Returns the result data for the asynchronous operation.  After this call
/// the async block is completed and no longer associated with the 
/// operation. The token parameter should match the token passed into
/// XAsyncBegin.
/// </summary>
/// <param name='asyncBlock'>A pointer to the XAsyncBlock that was passed to XAsyncBegin.</param>
/// <param name='identity'>An optional pointer used to match this result call with a prior XAsyncBegin call. If an identity pointer was passed to XAsyncBegin, the same pointer must be passed here.</param>
/// <param name='bufferSize'>The size of the provided buffer, in bytes.</param>
/// <param name='buffer'>A pointer to the result buffer.</param>
/// <param name='bufferUsed'>An optional pointer that contains the number of bytes written to the buffer.  This is defined as the requiredResultSize passed to XAsyncComplete.</param>
STDAPI XAsyncGetResult(
    _Inout_ XAsyncBlock* asyncBlock,
    _In_opt_ const void* identity,
    _In_ size_t bufferSize,
    _Out_writes_bytes_to_opt_(bufferSize, *bufferUsed) void* buffer,
    _Out_opt_ size_t* bufferUsed
    ) noexcept;

/// <summary>
/// This is used generate an identity to pass to XAsyncBegin/XAsyncGetResult
/// </summary>
#define XASYNC_IDENTITY(method) #method

} // extern "C"
