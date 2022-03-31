//--------------------------------------------------------------------------------------
// dstorageerr_win32.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This is an implementation of the console DirectStorage API set that uses only standard Win32 calls
// This implementation is only used for the Xbox One family consoles which does not have native support for DirectStorage
// Titles are free to use these interfaces if desired, but it's strongly recommended to use the native interfaces which will have much better performance and a richer feature set

#pragma once

/*++

 MessageId's 0x0000 - 0x00ff (inclusive) are reserved for DirectStorage.

--*/
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_GAME                    2340

//
// Define the severity codes
//

//
// MessageId: E_DSTORAGE_ALREADY_RUNNING
//
// MessageText:
//
// DStorage is already running exclusively.
//
#define E_DSTORAGE_ALREADY_RUNNING       ((HRESULT)0x89240001L)

//
// MessageId: E_DSTORAGE_NOT_RUNNING
//
// MessageText:
//
// DStorage is not running.
//
#define E_DSTORAGE_NOT_RUNNING           ((HRESULT)0x89240002L)

//
// MessageId: E_DSTORAGE_INVALID_QUEUE_CAPACITY
//
// MessageText:
//
// Invalid queue capacity parameter.
//
#define E_DSTORAGE_INVALID_QUEUE_CAPACITY ((HRESULT)0x89240003L)

//
// MessageId: E_DSTORAGE_XVD_DEVICE_NOT_SUPPORTED
//
// MessageText:
//
// The specified XVD is not on a supported file device.
//
#define E_DSTORAGE_XVD_DEVICE_NOT_SUPPORTED ((HRESULT)0x89240004L)

//
// MessageId: E_DSTORAGE_UNSUPPORTED_VOLUME
//
// MessageText:
//
// The specified XVD is not on a supported volume.
//
#define E_DSTORAGE_UNSUPPORTED_VOLUME    ((HRESULT)0x89240005L)

//
// MessageId: E_DSTORAGE_VOLUME_DISMOUNTED
//
// MessageText:
//
// The volume containing the specified file is already dismounted.
//
#define E_DSTORAGE_VOLUME_DISMOUNTED     ((HRESULT)0x89240006L)

//
// MessageId: E_DSTORAGE_END_OF_FILE
//
// MessageText:
//
// The specified offset and length exceeds the size of the file.
//
#define E_DSTORAGE_END_OF_FILE           ((HRESULT)0x89240007L)

//
// MessageId: E_DSTORAGE_REQUEST_TOO_LARGE
//
// MessageText:
//
// The IO request is too large.
//
#define E_DSTORAGE_REQUEST_TOO_LARGE     ((HRESULT)0x89240008L)

//
// MessageId: E_DSTORAGE_ACCESS_VIOLATION
//
// MessageText:
//
// The destination buffer for the DStorage request is not accessible.
//
#define E_DSTORAGE_ACCESS_VIOLATION      ((HRESULT)0x89240009L)

//
// MessageId: E_DSTORAGE_UNSUPPORTED_FILE
//
// MessageText:
//
// The file is not supported by DStorage. Possible reasons include the file is a
// sparse file, or is compressed in NTFS.
//
#define E_DSTORAGE_UNSUPPORTED_FILE      ((HRESULT)0x8924000AL)

//
// MessageId: E_DSTORAGE_FILE_NOT_OPEN
//
// MessageText:
//
// The file is not open.
//
#define E_DSTORAGE_FILE_NOT_OPEN         ((HRESULT)0x8924000BL)

//
// MessageId: E_DSTORAGE_RESERVED_FIELDS
//
// MessageText:
//
// A reserved field is not set to 0.
//
#define E_DSTORAGE_RESERVED_FIELDS       ((HRESULT)0x8924000CL)

//
// MessageId: E_DSTORAGE_INVALID_BCPACK_MODE
//
// MessageText:
//
// The request has invalid BCPack decompression mode.
//
#define E_DSTORAGE_INVALID_BCPACK_MODE   ((HRESULT)0x8924000DL)

//
// MessageId: E_DSTORAGE_INVALID_SWIZZLE_MODE
//
// MessageText:
//
// The request has invalid swizzle mode.
//
#define E_DSTORAGE_INVALID_SWIZZLE_MODE  ((HRESULT)0x8924000EL)

//
// MessageId: E_DSTORAGE_INVALID_DESTINATION_SIZE
//
// MessageText:
//
// The request's destination size is invalid. If no decompression is used, it must
// be equal to the request's length; If decompression is used, it must be larger
// than the request's length.
//
#define E_DSTORAGE_INVALID_DESTINATION_SIZE ((HRESULT)0x8924000FL)

//
// MessageId: E_DSTORAGE_QUEUE_CLOSED
//
// MessageText:
//
// The request targets a queue that is closed.
//
#define E_DSTORAGE_QUEUE_CLOSED          ((HRESULT)0x89240010L)

//
// MessageId: E_DSTORAGE_INVALID_CLUSTER_SIZE
//
// MessageText:
//
// The volume is formatted with an unsupported cluster size.
//
#define E_DSTORAGE_INVALID_CLUSTER_SIZE  ((HRESULT)0x89240011L)

//
// MessageId: E_DSTORAGE_TOO_MANY_QUEUES
//
// MessageText:
//
// The number of queues has reached the maximum limit.
//
#define E_DSTORAGE_TOO_MANY_QUEUES       ((HRESULT)0x89240012L)

//
// MessageId: E_DSTORAGE_INVALID_QUEUE_PRIORITY
//
// MessageText:
//
// Invalid priority is specified for the queue.
//
#define E_DSTORAGE_INVALID_QUEUE_PRIORITY ((HRESULT)0x89240013L)

//
// MessageId: E_DSTORAGE_TOO_MANY_FILES
//
// MessageText:
//
// The number of files has reached the maximum limit.
//
#define E_DSTORAGE_TOO_MANY_FILES        ((HRESULT)0x89240014L)

//
// MessageId: E_DSTORAGE_INDEX_BOUND
//
// MessageText:
//
// The index parameter is out of bound.
//
#define E_DSTORAGE_INDEX_BOUND           ((HRESULT)0x89240015L)

//
// MessageId: E_DSTORAGE_IO_TIMEOUT
//
// MessageText:
//
// The IO operation has timed out.
//
#define E_DSTORAGE_IO_TIMEOUT            ((HRESULT)0x89240016L)

//
// MessageId: E_DSTORAGE_INVALID_FILE_HANDLE
//
// MessageText:
//
// The specified file has not been opened.
//
#define E_DSTORAGE_INVALID_FILE_HANDLE   ((HRESULT)0x89240017L)

//
// MessageId: E_DSTORAGE_DEPRECATED_PREVIEW_GDK
//
// MessageText:
//
// This GDK preview is deprecated. Update to a supported GDK version.
//
#define E_DSTORAGE_DEPRECATED_PREVIEW_GDK ((HRESULT)0x89240018L)

//
// MessageId: E_DSTORAGE_XVD_NOT_REGISTERED
//
// MessageText:
//
// The specified XVD is not registered or unmounted.
//
#define E_DSTORAGE_XVD_NOT_REGISTERED    ((HRESULT)0x89240019L)

//
// MessageId: E_DSTORAGE_INVALID_FILE_OFFSET
//
// MessageText:
//
// The request has invalid file offset for the specified decompression mode.
//
#define E_DSTORAGE_INVALID_FILE_OFFSET   ((HRESULT)0x8924001AL)

//
// MessageId: E_DSTORAGE_INVALID_SOURCE_TYPE
//
// MessageText:
//
// A memory source request was enqueued into a file source queue, or a file source
// request was enqueued into a memory source queue.
//
#define E_DSTORAGE_INVALID_SOURCE_TYPE   ((HRESULT)0x8924001BL)

//
// MessageId: E_DSTORAGE_INVALID_INTERMEDIATE_SIZE
//
// MessageText:
//
// The request has invalid intermediate size for the specified decompression modes.
//
#define E_DSTORAGE_INVALID_INTERMEDIATE_SIZE ((HRESULT)0x8924001CL)

//
// MessageId: E_DSTORAGE_SYSTEM_NOT_SUPPORTED
//
// MessageText:
//
// This console generation doesn't support DirectStorage.
//
#define E_DSTORAGE_SYSTEM_NOT_SUPPORTED  ((HRESULT)0x8924001DL)

//
// MessageId: E_DSTORAGE_GET_FENCE_NAME_FAILED
//
// MessageText:
//
// A request to get D3D fence name has failed.
//
#define E_DSTORAGE_GET_FENCE_NAME_FAILED ((HRESULT)0x8924001EL)
