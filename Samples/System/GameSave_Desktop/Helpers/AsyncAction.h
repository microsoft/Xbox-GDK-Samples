//--------------------------------------------------------------------------------------
// AsyncAction.h
//
// An AsyncAction object, which calls any function you like on the XTaskQueue.
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

#include "AsyncOp.h"

namespace ATG
{
   /**
    * \brief A class which allows you to perform any std::function (or lambda function) on the async task queue.
    *
    *	Building on top of AsyncTask is AsyncAction - this class performs a lambda operation that you
    * pass in, as an asynchronous task. Get its return value from the **result** parameter, after it reaches the
    * completed state. 
    *
    * Like AsyncTask and AsyncOp, it can be used with traditional synchronization (i.e. it can be waited on), or
    * in a lock-free/polling fashion to test for completion.
    * 
    * \see ATG::AsyncTask 
    * \see ATG::AsyncOp
    *
    * Example usage:
    *
    * \code{.cpp}
    *
    * ATG::AsyncAction<bool> asyncAction;
    *
    * void PerformOperation()
    * {
    *   asyncAction.Reset(); // just in case we didn't clean up after completion last time.
    *   asyncAction.SetFunction( std::function<bool(void)> ( 
    *       [this] () -> bool
    *       { return true; }
    *   );
    *   asyncAction.Start();
    *   if ( !asyncAction.IsInTerminalState(nullptr) ) // poll to see if we're done.
    *   {
    *     asyncAction.WaitForCompletion( INFINITE ); // wait until we're done
    *   }
    *
    *   AsyncOpStates state;
    *   if (asyncAction.IsInTerminalState(&state) )
    *   {
    *     switch (state)
    *     {
    *        case AsyncOpStates::Complete:
    *        {
    *           // Operation is complete - get the results of the operation.
    *           // AsyncAction::result is valid at this point.
    *           printf( "AsyncAction result was %s\n", asyncAction.result ? "true" : "false" );
    *        }
    *        case AsyncOpStates::Error:
    *        {
    *           printf("Error HRESULT=%08X returned\n", asyncAction.GetErrorCode() );
    *        }
    *        case AsyncOpStates::Canceled:
    *        {
    *           printf("Operation was canceled\n");
    *        }
    *        default:
    *        {
    *           assert(false && "Shouldn't be in this state.");
    *        }
    *     }
    *   }
    *   else { assert(false && "Shouldn't be in this state here. Something else messed with the object"); }
    * 
    * }
    * 
    * \endcode
    */
   template < typename TReturnType >
   class AsyncAction : public AsyncTask
   {
      std::function<TReturnType(void)> lambdaFn;

   public:
      /**
       *	The return value of the lambda function. This is only valid if the async operation this action is running on
       * top of is in a terminal state, and has completed. That is:
       *
       * \code{.cpp}
       * AsyncOpStates state;
       * if (action.IsInTerminalState(&state) )
       * {
       *   switch (state)
       *   {
       *      case AsyncOpStates::Complete:
       *      {
       *         // Operation is complete - get the results of the operation.
       *         // AsyncAction::result is valid at this point.
       *      }
       *      case AsyncOpStates::Error:
       *      {
       *         // Handle error.
       *      }
       *      case AsyncOpStates::Canceled:
       *      {
       *         // Operation was canceled.
       *      }
       *      default:
       *      {
       *         assert(false && "Shouldn't be in this state.");
       *      }
       *   }
       * }
       * \endcode
       */
      TReturnType result; //#CONSIDER: Should this be std::atomic<TReturnType>?

      /**
       *	Default constructor
       */
      AsyncAction()
      {
      }

      /**
       *	Constructor which copies the function (i.e. it's provided as an l-value).
       */
      AsyncAction(const std::function< TReturnType() >& fn)
         : lambdaFn(fn)
      {
      }

      /**
       *	Constructor which moves the function parameter (i.e. it's provided as an x/r-value).
       */
      AsyncAction(std::function< TReturnType() > && fn)
         : lambdaFn(std::move(fn))
      {
      }

      /**
       *	Move construction is explicitly disabled for this type.
       */
      AsyncAction(AsyncAction&& moveFrom) = delete;

      /**
       *	Move assignment is explicitly disabled for this type.
       */
      AsyncAction& operator=(AsyncAction&& moveFrom) = delete;

      /**
       *	Copy construction is explicitly disabled for this type.
       */
      AsyncAction(const AsyncAction& copyFrom) = delete;

      /**
       *	Copy assignment is explicitly disabled for this type.
       */
      AsyncAction& operator=(const AsyncAction& copyFrom) = delete;

      /**
       *	Move assigns the function to run asynchronously.
       */
      void SetFunction(std::function<TReturnType(void)>&& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = std::move(fn);
      }

      /**
       *	Copies the function to run asynchronously.
       */
      void SetFunction(const std::function<TReturnType(void)>& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = fn;
      }

   protected:

      /**
       *	Internal function that does the work to run the provided function on the async queue.
       */
      HRESULT OnDoWork() noexcept final
      {
         if (lambdaFn)
         {
            result = lambdaFn();
            return S_OK;
         }
         else
         {
            return E_NOT_SET;
         }
      }

      /**
       *	Internal function that responds to the task being canceled. 
       * In AsyncTask you'd implement this to set a signal that your task should end (or check if its state isn't 
       * AsyncOpStates::Busy and has changed to AsyncOpStates::Canceling, which is also an acceptable mechanism). In
       * AsyncAction we don't do anything here. If you need to handle cancellation, set up your function/lambda to
       * take the async action itself as a parameter, and check its state by calling AsyncOp::GetState.
       */
      HRESULT OnTaskCanceled() noexcept override { return S_OK; };
   };

   /**
    *	Specialization of AsyncAction for functions that return HRESULTs. Instead of storing this in the result
    * variable, we short-circuit the process and return the HRESULT from DoWork(). It is available from GetErrorCode(),
    * and the operation will complete in an error state if the result is not a success code.
    */
   template < >
   class AsyncAction< HRESULT > : public AsyncTask
   {
      std::function< HRESULT() > lambdaFn;

   public:
      AsyncAction()
      {
      }

      AsyncAction(const std::function< HRESULT() >& fn)
         : lambdaFn( fn )
      {
      }

      AsyncAction(std::function< HRESULT() > && fn)
         : lambdaFn(std::move(fn))
      {
      }

      // Intentionally disable move/copy - AsyncAction relies on AsyncOp which is not designed to be moved/copied.

      AsyncAction(AsyncAction&& moveFrom) = delete;
      AsyncAction& operator=(AsyncAction&& moveFrom) = delete;
      AsyncAction(const AsyncAction& copyFrom) = delete;
      AsyncAction& operator=(const AsyncAction& copyFrom) = delete;

      void SetFunction(std::function< HRESULT() >&& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = std::move(fn);
      }

      void SetFunction(const std::function< HRESULT() >& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = fn;
      }

   protected:
      HRESULT OnDoWork() noexcept final
      {
         if (lambdaFn)
         {
            HRESULT hr = lambdaFn();
            return hr;
         }
         else
         {
            return E_NOT_SET;
         }
      }

      HRESULT OnTaskCanceled() noexcept override { return S_OK; };
   };

   /**
    *	Specialization of AsyncAction for functions which have a void return type, in which case there is no
    * result member variable.
    */
   template < >
   class AsyncAction< void > : public AsyncTask
   {
      std::function< void() > lambdaFn;

   public:
      AsyncAction()
      {
      }
      AsyncAction(const std::function< void() >& fn)
         : lambdaFn(fn)
      {
      }

      AsyncAction(std::function< void() > && fn)
         : lambdaFn(std::move(fn))
      {
      }

      // Intentionally disable move/copy - AsyncAction relies on AsyncOp which is not designed to be moved/copied.

      AsyncAction(AsyncAction&& moveFrom) = delete;
      AsyncAction& operator=(AsyncAction&& moveFrom) = delete;
      AsyncAction(const AsyncAction& copyFrom) = delete;
      AsyncAction& operator=(const AsyncAction& copyFrom) = delete;

      void SetFunction(std::function< void() >&& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = std::move(fn);
      }

      void SetFunction(const std::function< void() >& fn)
      {
         assert(IsReady() && "Can't change function while running.");
         lambdaFn = fn;
      }

   protected:
      HRESULT OnDoWork() noexcept final
      {
         if (lambdaFn)
         {
            lambdaFn();
            return S_OK;
         }
         else
         {
            return E_NOT_SET;
         }
      }

      HRESULT OnTaskCanceled() noexcept override { return S_OK; };
   };

   /** 
    *	Version of MakeAsyncAction which creates an AsyncAction, initializing its associated function call.
    */
   template <typename TFnType >
   auto MakeAsyncAction(std::function<TFnType>&& fn) -> AsyncAction< decltype(fn()) >
   {
      using TReturnType = decltype(fn());
      return AsyncAction<TReturnType>(std::forward<fn>);
   }

   /**
    *	Version of MakeAsyncAction which creates an empty AsyncAction.
    */
   template <typename TFnType >
   auto MakeAsyncAction() -> AsyncAction< decltype(TFnType()) >
   {
      using TReturnType = decltype(TFnType());
      return AsyncAction<TReturnType>();
   }

   /**
    *	Version of MakeAsyncAction which creates an AsyncAction and initializes its function call, returning a
    * unique_ptr to the AsyncAction object.
    */
   template <typename TFnType >
   auto MakeAsyncAction_Unique(std::function<TFnType>&& fn) -> std::unique_ptr< AsyncAction< decltype(fn()) > >
   {
      using TReturnType = decltype(fn());
      return std::make_unique< AsyncAction< TReturnType > >(std::forward< fn >);
   }

   /**
    *	Version of MakeAsyncAction which creates an empty AsyncAction, returning a unique_ptr to the AsyncAction object.
    */
   template <typename TFnType >
   auto MakeAsyncAction_Unique() -> std::unique_ptr< AsyncAction< decltype(TFnType()) > >
   {
      using TReturnType = decltype(TFnType());
      return std::make_unique< AsyncAction< TReturnType > >();
   }
}
