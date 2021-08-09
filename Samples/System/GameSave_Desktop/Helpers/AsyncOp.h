//--------------------------------------------------------------------------------------
// AsyncOp.h
//
// Wrapper for async operations, and async tasks that run on the XTaskQueue.
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
#include "StateMachine.h"
#include <memory>

namespace ATG
{
   constexpr HRESULT E_TIMEOUT = HRESULT_FROM_WIN32( ERROR_TIMEOUT );

   // Valid states for the async operation state machine.
   enum class AsyncOpStates
   {
      // Ready to perform a new async operation.
      Ready,

      // The call is being prepared (a caller currently owns this async op object)
      SettingUpCall,

      // An asynchronous operation is in progress, and has not yet completed.
      Busy,

      // An asynchronous operation was in progress, and has not yet completed, but we have request
      // that it be canceled.
      Canceling,

      // The asynchronous operation has completed and the results should be ready to gather.
      Complete,

      // The operation has either been aborted by the system, a user, or been canceled by a call to Cancel.
      Canceled,

      // There was an error while attempting to perform the asynchronous operation.
      Error,

      // The async op object is owned by a caller, which is resetting this object.
      Resetting
   };

   extern const std::vector<AsyncOpStates> s_TerminalStatesList;

   // The transitions we allow in our state machine.
   struct AsyncOpAllowedTransitions
   {
      static constexpr bool IsAllowedTransition( AsyncOpStates fromState, AsyncOpStates toState )
      {
         switch ( fromState )
         {
         case AsyncOpStates::Ready:
            return toState == AsyncOpStates::SettingUpCall;
         case AsyncOpStates::SettingUpCall:
            return toState == AsyncOpStates::Busy || toState == AsyncOpStates::Canceling;
         case AsyncOpStates::Busy:
            return toState == AsyncOpStates::Canceling || 
                   toState == AsyncOpStates::Canceled ||
                   toState == AsyncOpStates::Complete ||
                   toState == AsyncOpStates::Error;
         case AsyncOpStates::Canceling:
            // Could complete while we're trying to cancel with OK or error, so transition
            // to those states is allowed.
            return toState == AsyncOpStates::Canceled ||
                   toState == AsyncOpStates::Complete ||
                   toState == AsyncOpStates::Error;
         case AsyncOpStates::Canceled:
         case AsyncOpStates::Complete:
         case AsyncOpStates::Error:
            // These are all terminal (aka readout) states, and are the only ones allowed to 
            // transition to Ready - but they must go via Resetting.
            return toState == AsyncOpStates::Resetting;
         case AsyncOpStates::Resetting:
            return toState == AsyncOpStates::Ready;
         default:
            return false;
         }
      }
   };

   // Class which wraps and manages an async operation. 
   
   class AsyncOp
   {
   protected:
      // The async block for the async operation.
      XAsyncBlock async;

      // The error state of this async op - meaningless unless we're in the AsyncOpStates::Error state.
      std::atomic<HRESULT> error;

      // The state machine tracking this operation.
      using AsyncOpStateMachine = LockFreeStateMachineWithWaiterLock< AsyncOpStates, AsyncOpStates::Ready, AsyncOpAllowedTransitions>;
      AsyncOpStateMachine state;

   public:

      // Constructor
      AsyncOp() noexcept
      {
      }

      // Destructor
      virtual ~AsyncOp() noexcept;

      AsyncOp(AsyncOp&& moveFrom) noexcept = delete;

      AsyncOp& operator=(AsyncOp&& moveFrom) noexcept = delete;

      AsyncOp(const AsyncOp& copyFrom) noexcept = delete;

      AsyncOp& operator=(const AsyncOp& copyFrom) noexcept = delete;

      HRESULT WaitForCompletion(uint32_t timeoutInMs = INFINITE) noexcept;

      // Waits until the AsyncOp is ready for its next operation to proceed - although another thread will
      // still need to consume the result/error and put the AsyncOp back in the Ready state before this will wake.
      HRESULT WaitUntilReady(uint32_t timeoutInMs = INFINITE) noexcept;

      // Signals if the AsyncOp is in a state where an 
      // NOTE: This value is only valid for the moment it was snapshotted, and can change in multithreaded scenarios after
      // it is reported. 
      bool IsReady() const noexcept
      {
         return state.GetCurrentState(std::memory_order_relaxed) == AsyncOpStates::Ready;
      }

      // Gets the current state of the state machine. Mainly for diagnostics; you should probably
      // be using IsInTerminalState or IsReady instead. 
      // NOTE: This operation is performed using memory_order_relaxed.
      AsyncOpStates GetState() const noexcept
      {
         return state.GetCurrentState(std::memory_order_relaxed);
      }

      // Gets the error value of the last operation. (For this to make sense, state should be AsyncOpsState::Error).
      HRESULT GetErrorValue() const noexcept
      {
         assert(GetState() == AsyncOpStates::Error
            && "Error values only make sense if you're in the error state. Check the state first.");

         // use memory_order_acquire; transitioning to Error state was performed by a memory_order_release. 
         return error.load(std::memory_order_acquire);
      }

      // Signals if the AsyncOp is in a terminal state ( Complete, Canceled, Error ).
      // NOTE: This value is only valid for the moment it was snapshotted, and can change in multithreaded scenarios after
      // it is reported.
      bool IsInTerminalState(_Out_opt_ AsyncOpStates* outState = nullptr) const noexcept
      {
         AsyncOpStates temp = state.GetCurrentState(std::memory_order_relaxed);

         if (outState != nullptr)
         {
            *outState = temp;
         }

         return (temp == AsyncOpStates::Complete || temp == AsyncOpStates::Canceled || temp == AsyncOpStates::Error);
      }

      // Starts an asynchronous operation. The other parameters for the call should have been set using functions in the
      // TBase class.
      // Returns E_ACCESSDENIED if there was already an operation happening.
      // If you specify a non-zero timeout value, we will wait for the AsyncOp to return to 
      // the Ready state, and then start the operation (or we will timeout with an error - E_TIMEOUT).
      // (NOTE: Another thread will need to consume any completed/canceled/error states and call Reset before this can occur).
      HRESULT Start(_In_ XTaskQueueHandle taskQueue, uint32_t timeoutInMs = 0) noexcept;

      // Resets back to Ready state, ready for the next operation. This operation only works if you're in a terminal 
      // state such as Completed, Canceled or Error.
      //
      // You should typically call Reset in your completion handling routine after you've either used the data that the 
      // operation returned, or handled any error/cancel cases.
      bool Reset();

      // Cancels any pending operation. This may take some time; the operation is canceled, and then the completion callback
      // for the operation is run. (This call only signals to the asynchronous system that the operation must be canceled). 
      // NOTE: In the event of a race, this operation may complete /instead/ of being canceld. Your code should be prepared
      // to handle that eventuality.
      bool Cancel();

   protected:
      // Called when the operation completes successfully, after all data has sequestered. Override this in your code.
      virtual void OnCompleted() noexcept {};

      // Called when the operation completes with an error. Override this in your code.
      virtual void OnError() noexcept {};

      // Called when the operation is canceled(and the completion routine runs).
      virtual void OnCanceled() noexcept {};

      // Called when the consumer is done with the results, and resets the state machine by calling Reset()
      // (or the AsyncOp object is destroyed). Use this to cleanup any data that was allocated (if you need to). 
      virtual void OnReset() noexcept {};

      // This is called to start up the asynchronous operation. You should call the XXXXXXXXXAsync method here,
      // returning any HRESULT you get back, and using the XAsyncBlock provided. For example:
      //
      // HRESULT InvokeAsync( XAsyncBlock& block ) noexcept override
      // {
      //    return XUserAddAsync( this->options, &block );
      // }
      // 
      virtual HRESULT InvokeAsync(XAsyncBlock& block) noexcept = 0;

      // This is called when the operation is complete. This routine
      // gathers data from the operation. If an error has occurred when you attempt to obtain the result,
      // simply return the error and the appropriate OnCanceled/OnError function will be called. If you
      // return a success code, OnCompleted will be called for you.
      //
      // For example:
      //   HRESULT RetrieveAsyncResult( XAsyncBlock& block ) noexcept 
      //   {
      //     HRESULT hr = XUserAddResult( &block, &userHandle );
      //     return hr;
      //   }

      virtual HRESULT RetrieveAsyncResult(XAsyncBlock& block) noexcept = 0;

      // Internal call that is run when the async routine is finished and the completion callback fires.
      void OnAsyncCompletionCallback( XAsyncBlock& block );

   private:
      // Static callback used to trampoline to the TBase::OnAsyncCompletion function for this async op instance.
      static void CALLBACK AsyncCompletionCallbackStatic( XAsyncBlock* block );
   };

   /*
    *	This is a wrapper for Async tasks. These are async operations that you can create and post to task queues, which
    * will then run on the thread pool. The advantage to this design is that the result, and async op are all bound up
    * in a single object which you can wait on for results.
    *
    * Unlike normal XAsync callbacks, the only context information/results storage provided is in the object itself.
    *
    * To implement an AsyncTask you will need to implement the following functions in your child class:
    *
    * void OnTaskCanceled(); - handle any cancelation of the operation.
    * void OnDoWork(); - handle doing the work that the task requires.
    * 
    * You should also override the following functions from AsyncOp:
    *   OnCompleted, OnError, OnCanceled, OnReset.
    */
   class AsyncTask : public AsyncOp
   {
   protected:
      virtual HRESULT OnDoWork() = 0;
      virtual HRESULT OnTaskCanceled() { return S_OK;  };

      HRESULT InvokeAsync(XAsyncBlock& block) noexcept final;

      HRESULT RetrieveAsyncResult(XAsyncBlock& block) noexcept final 
      {
         // We're using our enclosing object as our data store, so results will be in that, and we don't need
         // to actually get any data back from XAsyncGetResult - all we're doing is telling the task queue that
         // we're done with the XAsyncBlock here.
         return XAsyncGetResult(&block, this, 0, nullptr, nullptr);
      }

   private:
      HRESULT AsyncCallImpl(_In_ XAsyncOp op, _In_ const XAsyncProviderData* data);
            
      static HRESULT CALLBACK AsyncCallImplStatic(_In_ XAsyncOp op, _In_ const XAsyncProviderData* data);
   };


}
