//--------------------------------------------------------------------------------------
// XGameSaveHandleWrappers.h
//
// Handle wrappers for XGameSaveXXX handle types.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

#include "HandleWrapperBase.h"
#include <XTaskQueue.h>

namespace ATG
{
   /**
    *	RAII pattern wrapper class around an XTaskQueueHandle.
    *
    * This class ensures that the handle ownership is managed correctly and it
    * is destroyed rather than leaked if it goes out of scope.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    */
   class TaskQueueHandle final : public GXDKHandleBase< XTaskQueueHandle, TaskQueueHandle >
   {
      friend class GXDKHandleBase< XTaskQueueHandle, TaskQueueHandle >;
   public:
      /**
       * Creates an empty TaskQueueHandle object which is not managing a handle.
       */
      TaskQueueHandle() : GXDKHandleBase() { }

      /**
       * Creates a new TaskQueueHandle object, taking ownership of the passed-in handle.
       *
       * \param XTaskQueueHandle taskQueueHandle - the task queue handle to manage.
       */
      TaskQueueHandle( XTaskQueueHandle taskQueueHandle ) : GXDKHandleBase(taskQueueHandle) { }

      /**
       * Move-constructs a TaskQueueHandle object from another TaskQueueHandle object.
       *
       * \param TaskQueueHandle&& moveFrom - the object to move-construct from.
       */
      TaskQueueHandle( TaskQueueHandle&& moveFrom ) : GXDKHandleBase(std::move(moveFrom)) { }

      /**
       *	Deleted copy-constructor. Implicit copying and duplication of TaskQueue is not allowed.
       */
      TaskQueueHandle(const TaskQueueHandle& copyFrom) = delete;

      /**
       *	Deleted copy-assignment operator. Implicit copying and duplication of TaskQueue is not allowed.
       */
      TaskQueueHandle& operator=(const TaskQueueHandle& copyFrom) = delete;

      /**
       * Move-assigns a TaskQueueHandle object from another TaskQueueHandle object.
       * 
       * \param TaskQueueHandle&& moveFrom - the object to move from.
       * \return ATG::TaskQueueHandle& - a reference to the assigned-to object.
       */
      TaskQueueHandle& operator=(TaskQueueHandle&& moveFrom) noexcept
      {
         if (this != &moveFrom)
         {
            GXDKHandleBase::operator=(std::move(moveFrom));
         }
         return *this;
      }

      operator XTaskQueueHandle() const noexcept
      {
         return handle;
      }

      /**
       * Duplicates the XTaskQueueHandle wrapped by this object, returning a new TaskQueueHandle object.
       * 
       * \return ATG::TaskQueueHandle - the new wrapper instance, which contains a newly duplicated handle.
       */
      TaskQueueHandle Duplicate() const
      {
         assert(IsValid() && "Task queue handle must be valid before it can be duplicated");

         XTaskQueueHandle dup;
         DX::ThrowIfFailed(::XTaskQueueDuplicateHandle(handle, &dup));
         return XTaskQueueHandle(dup);
      }

   protected:
      /**
       * Implements closing the underlying XTaskQueueHandle for the GXDKHandleWrapper base class.
       */
      void CloseImpl() noexcept
      {
         if (handle != nullptr)
         {
            auto handleTemp = handle;
            handle = nullptr;
            ::XTaskQueueCloseHandle(handleTemp);
         }
      }

   public:

      /**
       * Creates a new XTaskQueue object, returning the TaskQueueHandle object which owns its handle.
       * 
       * \param XTaskQueueDispatchMode workDispatchMode - the mode to use when creating the task queue's worker port.
       * \param XTaskQueueDispatchMode completionDispatchMode - the mode to use when creating the task queue's completion port.
       * \return ATG::TaskQueueHandle - the wrapper object which contains the handle to the new XTaskQueue instance.
       */
      static TaskQueueHandle Create(XTaskQueueDispatchMode workDispatchMode = XTaskQueueDispatchMode::ThreadPool,
                                    XTaskQueueDispatchMode completionDispatchMode = XTaskQueueDispatchMode::ThreadPool);
   };
};
