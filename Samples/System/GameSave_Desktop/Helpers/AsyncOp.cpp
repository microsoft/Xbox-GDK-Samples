//--------------------------------------------------------------------------------------
// AsyncOp.cpp
//
// Implements an asynchronous operation.
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
#include <vector>
#include "AsyncOp.h"

namespace ATG
{
   const std::vector<AsyncOpStates> s_TerminalStatesList = {
         AsyncOpStates::Canceled,
         AsyncOpStates::Complete,
         AsyncOpStates::Error
   };

   AsyncOp::~AsyncOp() noexcept
   {
      assert((IsInTerminalState() || IsReady()) && "This operation is still pending, but we're being destroyed!");

      // We shouldn't have to do this, but if we end up in this state in release, try to recover
      // cleanly.

      if (IsInTerminalState())
      {
         // We reset here to allow flexibility. Calling code might not have consumed the results of the operation,
         // but if we're being destroyed that's fine.

         bool success = Reset();
#ifdef _DEBUG
         assert(success && "Couldn't reset asyncop");
#else
		 //NOTE: This causes build errors in Release mode in our build system. Normally I wouldn't expect this to be necessary.
		 (void)success;
#endif
      }

      if (!IsReady())
      {
         // Try to cancel the operation.

         bool success = Cancel();
         assert(success && "Couldn't cancel asyncop");

         WaitForCompletion(); // infinite wait until we hit a terminal state. Even if we couldn't cancel, 
                              // we should get here eventually.

         // Now that we've hit a terminal state, Reset.
         success = Reset();
         assert(success && "Couldn't reset asyncop");
      }
   }

   HRESULT AsyncOp::WaitForCompletion(uint32_t timeoutInMs /*= INFINITE*/) noexcept
   {
      assert(timeoutInMs != 0 && "Timeout should not be 0; to check if ready, call IsReady instead");
      if (timeoutInMs == 0)
      {
         return E_INVALIDARG;
      }

      AsyncOpStates lastState;
      HRESULT hr = state.WaitToEnterStateFromSet(s_TerminalStatesList, lastState, timeoutInMs);
      return hr;
   }

   HRESULT AsyncOp::WaitUntilReady(uint32_t timeoutInMs /*= INFINITE*/) noexcept
   {
      return state.WaitToEnterState(AsyncOpStates::Ready, timeoutInMs);
   }

   HRESULT AsyncOp::Start(_In_ XTaskQueueHandle taskQueue, uint32_t timeoutInMs /*= 0*/) noexcept
   {
      if (timeoutInMs != 0)
      {
         HRESULT hr = state.WaitToEnterState(AsyncOpStates::Ready, timeoutInMs);
         if (FAILED(hr)) // Timeout?
            return hr;

         // NOTE: Someone else can still beat us to it, even if we're Ready now, and steal it from under us.
         //       Callers should consider looping if they get E_ACCESSDENIED.
      }

      if (state.TryTransitionToState(AsyncOpStates::Ready, AsyncOpStates::SettingUpCall))
      {
         // This thread owns the AsyncOp now, as it is the one that successfully transition from Ready to SettingUpCall.
         async = {};
         async.queue = taskQueue;
         async.context = this;
         async.callback = &AsyncCompletionCallbackStatic;

         HRESULT hr = InvokeAsync(async);

         // Did we fail the async op on setup/initial call? If so, we need to transition to the error state.
         if (FAILED(hr))
         {
            error = hr;
            state.TransitionToStateMustSucceed(AsyncOpStates::SettingUpCall, AsyncOpStates::Error);
            return hr;
         }

         state.TransitionToStateMustSucceed(AsyncOpStates::SettingUpCall, AsyncOpStates::Busy);

         return S_OK; // E_PENDING would also work, but client code might be checking for SUCCESS()
      }
      else
      {
         // Not in the right state to start a call - someone else is already using the object.
         return E_ACCESSDENIED;
      }
   }

   bool AsyncOp::Reset()
   {
      AsyncOpStates currentState;
      do
      {
         currentState = state.GetCurrentState(std::memory_order_relaxed);
         if (currentState != AsyncOpStates::Complete && currentState != AsyncOpStates::Canceled && currentState != AsyncOpStates::Error)
         {
            // Not in a terminal state - can't reset.
            return false;
         }
      } while (!state.TryTransitionToState(currentState, AsyncOpStates::Resetting, std::memory_order_acquire));

      // If we get to here, we migrated from our current state to Resetting without anyone else
      // grabbing it in between.

      // Call our child class to ask it to release any data it may be holding onto in its current state.
      OnReset();
      error = S_OK;

      // Make sure that no matter what else, we end up in the Ready state here.

      state.TransitionToStateMustSucceed(AsyncOpStates::Resetting, AsyncOpStates::Ready, std::memory_order_release);

      return true;
   }

   bool AsyncOp::Cancel()
   {
      if (!state.TryTransitionToState(AsyncOpStates::Busy, AsyncOpStates::Canceling))
      {
         return false;
      }

      XAsyncCancel(&async);

      // NOTE: The call to XAsyncCancel can't fail in any meaningful/way you can react appropriately to.

      // The completion callback will be called, with the result status being E_ABORT. 
      // We transition to the AsyncOpStates::Canceled state inside that callback.

      return true;
   }

   void AsyncOp::OnAsyncCompletionCallback(XAsyncBlock& block)
   {
      HRESULT hr = RetrieveAsyncResult(block);
      error.store(hr, std::memory_order_relaxed);

      //NOTE: We force-set the states here, because while we should be coming here from Busy,
      //      we could be in Canceling or Canceled - but given that we completed successfully
      //      we still want to move to the Complete or Error state depending on the result.

      AsyncOpStates currentState = state.GetCurrentState(std::memory_order_acquire);
      assert(currentState == AsyncOpStates::Busy || currentState == AsyncOpStates::Canceling &&
         "In unexpected state when completion callback occurred");

      if (FAILED(hr))
      {
         if (error == E_ABORT)
         {
            state.TransitionToStateMustSucceed(currentState, AsyncOpStates::Canceled, std::memory_order_release);
            OnCanceled();
         }
         else
         {
            state.TransitionToStateMustSucceed(currentState, AsyncOpStates::Error, std::memory_order_release);
            OnError();
         }
      }
      else
      {
         state.TransitionToStateMustSucceed(currentState, AsyncOpStates::Complete, std::memory_order_release);
         OnCompleted();
      }
   }

   void CALLBACK AsyncOp::AsyncCompletionCallbackStatic(XAsyncBlock* block)
   {
      assert(block != nullptr && "XAsyncBlock parameter should not be null");

      AsyncOp* obj = static_cast<AsyncOp*>(block->context);
      assert(&(obj->async) == block && "Callback async block didn't match containing object async block ptr");
      obj->OnAsyncCompletionCallback(*block);
   }

   HRESULT AsyncTask::InvokeAsync(XAsyncBlock& block) noexcept
   {
      // We don't pass in any additional buffer for this call - it's not needed, as we can use our object as storage.
      HRESULT hr = XAsyncBegin(&block, static_cast<void*>(this), static_cast<void*>(this), __FUNCTION__, &AsyncCallImplStatic);

      if (SUCCEEDED(hr))
      {
         hr = XAsyncSchedule(&block, 0);
      }
      return hr;
   }
   
   HRESULT AsyncTask::AsyncCallImpl( _In_ XAsyncOp op, _In_ const XAsyncProviderData* /*data - unused*/)
   {
      switch (op)
      {
      case XAsyncOp::Cancel:
      {
         return OnTaskCanceled();
      }
      case XAsyncOp::DoWork:
      {
         return OnDoWork();
      }
	  case XAsyncOp::Begin:
      case XAsyncOp::Cleanup:
      case XAsyncOp::GetResult:
      default:
         {
            // Nothing to do here due to the model we're using to manage these operations.
            return S_OK;
         }
      }
   }

   HRESULT CALLBACK AsyncTask::AsyncCallImplStatic(_In_ XAsyncOp op, _In_ const XAsyncProviderData* data)
   {
      return static_cast<AsyncTask*>(data->context)->AsyncCallImpl(op, data);
   }

}
