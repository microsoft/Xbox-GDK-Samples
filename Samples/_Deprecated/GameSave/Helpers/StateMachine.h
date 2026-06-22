//--------------------------------------------------------------------------------------
// StateMachine.h
//
// Lock-free state machine implementation.
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

#include "RDTSCPStopWatch.h"

namespace ATG
{
   // This is a class which can be used with the LockFreeStateMachine classes to indicate that all 
   // transitions from any state to any other states are allowed.
   // TStateEnum - the enum of state IDs to use with this class.
   template< typename TStateEnum >
   struct AllTransitionsAllowed
   {
      static constexpr bool IsAllowedTransition( TStateEnum /*fromState*/, TStateEnum /*toState*/ )
      {
         return true;
      }
   };

   // A lock-free state machine with managed transitions, and the ability to wait on state changes.

   template<
      typename TStateEnum,       // The enum that contains all of the states.
      TStateEnum defaultState,   // The default state for the state machine.
      typename TStateAllowedTransitions = AllTransitionsAllowed< TStateEnum > // Allowed transitions 
   >
   class LockFreeStateMachine
   {
      static_assert( std::is_enum<TStateEnum>::value, "Must be enum type" );

      std::atomic<TStateEnum> state;
   public:
       LockFreeStateMachine() : state(defaultState) {}
	   LockFreeStateMachine( LockFreeStateMachine&& moveFrom ) = delete;
	   LockFreeStateMachine& operator=( LockFreeStateMachine&& moveFrom ) = delete;

      // Returns the current state of the state machine. By default this uses memory_order_relaxed.
      // NOTE: The actual state could always change after you read it; in order to guarantee a value you
      // need to transition from a public state (such as Idle) to a thread-owned private state (e.g. Busy) in your
      // design.

      TStateEnum GetCurrentState( std::memory_order order = std::memory_order_relaxed )
      {
         return state.load( order );
      }

      // Attempts to transition to a new state (which might fail). This is typical in lock-free algorithms. 
      // expectedState - the state you think you're transitioning from.
      // toState - the state to transition to (if your assertion from expectedState holds).
      // order - the memory ordering model to apply to the underlying CAS operation.
      // Returns true if the transition succeeded, false if it fails (for example, someone changed the state so that it 
      // no longer matches your expectedState).

      bool TryTransitionToState( TStateEnum expectedState, TStateEnum toState, std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
         assert( TStateAllowedTransitions::IsAllowedTransition( expectedState, toState ) 
            && "Transition between these states is not allowed" );

         return state.compare_exchange_strong( &expectedState, toState, order );
      }

      // Transitions to a state. Call this if your transition should always succeed in your program if it is 
      // well-formed, and if not doing so should be a fatal programming error.
      // If it fails, we assert in Debug. If it fails in Release, there's not much we can do, so we call DebugBreak().
      //
      // NOTE: This is typically used in lock-free algorithms when transitioning from states with only one possible owner to
      //       states without owners/informational states that cannot be owned.

      void TransitionToStateMustSucceed( TStateEnum expectedState, TStateEnum toState, std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
         assert( TStateAllowedTransitions::IsAllowedTransition( expectedState, toState )
            && "Transition between these states is not allowed" );

         if ( state.compare_exchange_strong( &expectedState, toState, order ) )
         {
            return true;
         }

#if _DEBUG
         assert( false && "Couldn't transition from expected state to new state - was in wrong state" );
#else
         DebugBreak();
#endif
         return false;
      }

      // Spin until the state machine enters the expected state.
      //
      // NOTE: there is no guarantee that it will still be in that state by the time you deal with the
      // result from this function. This is only meant to be a cheap way to spin on the state change.

      bool SpinWaitToEnterState( TStateEnum expectedState, uint32_t iterations = 1000 )
      {
         do 
         {
            if ( GetCurrentState() == expectedState )
               return true;

            _mm_pause();

         } while ( iterations-- > 0 );

         return false;
      }

   };

   // This class implements a lock-free state machine that also collaborates with WaitOnAddress to allow
   // threads monitoring it to sleep until a transition occurs. 

   template<
      typename TStateEnum,       // The enum that contains all of the states.
      TStateEnum defaultState,   // The default state for the state machine.
      typename TStateAllowedTransitions = AllTransitionsAllowed<TStateEnum> // Allowed transitions
   >
      class LockFreeStateMachineWithWaiterLock
   {
      uint64_t waiterVariable;
      std::atomic<TStateEnum> state;
   public:
	   LockFreeStateMachineWithWaiterLock()
		   : waiterVariable( 0ULL ),
             state( defaultState )
	   {
	   }

	   LockFreeStateMachineWithWaiterLock( LockFreeStateMachineWithWaiterLock&& moveFrom ) = delete;
	   LockFreeStateMachineWithWaiterLock& operator=( LockFreeStateMachineWithWaiterLock&& moveFrom ) = delete;

      TStateEnum GetCurrentState( std::memory_order order = std::memory_order_relaxed ) const
      {
         return state.load( order );
      }

      // Transitions to a state. Call this if your transition should always succeed in your program if it is 
      // well-formed.
      // If it fails, we assert in Debug. If it fails in Release, there's not much we can do, so we call DebugBreak().
      //
      // NOTE: This is typically used in lock-free algorithms when transitioning from states with only one possible owner to
      //       states without owners/informational states that cannot be owned.

      void TransitionToStateMustSucceed( TStateEnum expectedState, TStateEnum toState, std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
         assert( TStateAllowedTransitions::IsAllowedTransition( expectedState, toState )
            && "Transition between these states is not allowed" );

         if ( state.compare_exchange_strong( expectedState, toState, order ) )
         {
            WakeAllOnStateChange();
            return;
         }

#if _DEBUG
         assert( false && "Couldn't transition from expected state to new state - was in wrong state" );
#else
         DebugBreak();
#endif
      }

      // Attempts to transition to a new state (which might fail). This is typical in lock-free algorithms. 
      // expectedState - the state you think you're transitioning from.
      // toState - the state to transition to (if your assertion from expectedState holds).
      // order - the memory ordering model to apply to the underlying CAS operation.
      // Returns true if the transition succeeded, false if it fails (for example, someone changed the state so that it 
      // no longer matches your expectedState).

      bool TryTransitionToState( TStateEnum expectedState, TStateEnum toState, std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
         assert( TStateAllowedTransitions::IsAllowedTransition( expectedState, toState )
            && "Transition between these states is not allowed" );

         if ( state.compare_exchange_strong( expectedState, toState, order ) )
         {
            WakeAllOnStateChange();
            return true;
         }

         return false;
      }

      // Waits until the state machine enters a specific state (with a timeout).
      //
      // NOTE: This can "fail" if you wait to enter the expected state, but the state moves on
      //       before you can do anything about it. If multiple objects are contended on the lock
      //       you should call WaitToEnterState in a loop, and TryEnter some kind of lock. 
      // 
      // Example:
      //       CRITICAL_SECTION lock;
      //       LFSMWaiter stateMachine;
      //       void DoSomething()
      //       {
      //            for( ;; )
      //            {
      //                HRESULT hr = stateMachine.WaitToEnterState( State::TargetState, INFINITE );
      //                if ( TryEnterCriticalSection( &lock ) )
      //                   break;
      //            }
      //   
      //            // We have the lock, do what we need to here.
      //       }
      //
      //       Alternatively, rather than using a critical section to show ownership, attempt to
      //       Transition from your expected starting state to an intermediate state that implies ownership
      //       has been taken. As only one thread can transition to that state successfully, completing the 
      //       transition implies that the lock has succeeded.
      //
      //       That would look like this:
      //
      //       LFSMWaiter stateMachine;
      //       void DoSomething()
      //       {
      //            for( ;; )
      //            {
      //                HRESULT hr = stateMachine.WaitToEnterState( State::Idle, INFINITE );
      //                if ( TransitionTo( State::Idle, State::AllMine ) )
      //                    break;
      //            }
      //   
      //            // We have the lock, do what we need to here.
      //       }
      //

      HRESULT WaitToEnterState( TStateEnum stateToMatch, uint32_t timeoutInMs = INFINITE )
      {
         RDTSCPStopWatch highresolutionWaitSW;
         highresolutionWaitSW.Start();

         // If we don't have a timeout and would just return immediately (in which case, why not call GetCurrentState()?)
         if ( timeoutInMs == 0 )
         {
            TStateEnum value = GetCurrentState();

            if ( value == stateToMatch )
            {
               return S_OK;
            }
            else
            {
               return HRESULT_FROM_WIN32( ERROR_TIMEOUT );
            }
         }
         else if ( timeoutInMs == INFINITE )
         {
            for ( ;; )
            {
               uint64_t oldWaiterValue = waiterVariable;
               TStateEnum value = GetCurrentState();

               if ( value == stateToMatch )
               {
                  return S_OK;
               }

               if ( !::WaitOnAddress( &waiterVariable, &oldWaiterValue, sizeof( waiterVariable ), INFINITE ) )
               {
                  return HRESULT_FROM_WIN32( GetLastError() );
               }
            }
         }

         const long long originalTimeoutInMicrosecs = (long long)timeoutInMs * 1000LL;

         for( ;; )
         {
            uint64_t oldWaiterValue = waiterVariable;
            TStateEnum value = GetCurrentState();

            if ( value == stateToMatch )
            {
               return S_OK;
            }

            // Did we run out of time to keep looping on this?
            if ( timeoutInMs == 0 )
            {
               return HRESULT_FROM_WIN32( ERROR_TIMEOUT );
            }

            if ( !::WaitOnAddress( &waiterVariable, &oldWaiterValue, sizeof( waiterVariable ), timeoutInMs ) )
            {
               return HRESULT_FROM_WIN32( GetLastError() );
            }    

            // Check if we have remaining time left, for example, on a spurious wakeup we may wake up early.
            long long durationInMicrosecs = static_cast<long long>( highresolutionWaitSW.GetCurrentMicroseconds() );
            if ( durationInMicrosecs >= originalTimeoutInMicrosecs )
            {
               timeoutInMs = 0; // we've gone past the end of our timeout.
            }
            else
            {
               timeoutInMs = (uint32_t) ( ( durationInMicrosecs + 500 ) / 1000 ); // round up to nearest millisecond.
            }
         }
      }

      // Waits until the state machine enters one of a list of states, or times out.

      HRESULT WaitToEnterStateFromSet( const std::vector<TStateEnum>& statesToWaitFor, TStateEnum& finalState, uint32_t timeoutInMs = INFINITE )
      {
         ATG::RDTSCPStopWatch highresolutionWaitSW;
         highresolutionWaitSW.Start();

         if ( timeoutInMs == 0 ) // Immediate timeout
         {
            TStateEnum value = GetCurrentState();

            for ( TStateEnum stateToMatch : statesToWaitFor )
            {
               if ( value == stateToMatch )
               {
                  finalState = value;
                  return S_OK;
               }
            }

            return HRESULT_FROM_WIN32( ERROR_TIMEOUT );
         }
         else if ( timeoutInMs == INFINITE ) // Infinite timeout
         {
            for ( ;; )
            {
               uint64_t oldWaiterValue = waiterVariable;
               TStateEnum value = GetCurrentState();

               for ( TStateEnum stateToMatch : statesToWaitFor )
               {
                  if ( value == stateToMatch )
                  {
                     finalState = value;
                     return S_OK;
                  }
               }

               if ( !::WaitOnAddress( &waiterVariable, &oldWaiterValue, sizeof( waiterVariable ), INFINITE ) )
               {
                  return HRESULT_FROM_WIN32( GetLastError() );
               }
            }
         }

         const long long originalTimeoutInMicrosecs = (long long)timeoutInMs * 1000LL;

         for ( ;; ) // Arbitrary timeout, handling spurious wakeups.
         {
            uint64_t oldWaiterValue = waiterVariable;
            TStateEnum value = GetCurrentState();

            for ( TStateEnum stateToMatch : statesToWaitFor )
            {
               if ( value == stateToMatch )
               {
                  finalState = value;
                  return S_OK;
               }
            }

            // Exit immediately without waiting on timeout = 0.
            if ( timeoutInMs == 0 )
            {
               return HRESULT_FROM_WIN32( ERROR_TIMEOUT );
            }

            if ( !::WaitOnAddress( &waiterVariable, &oldWaiterValue, sizeof( waiterVariable ), timeoutInMs ) )
            {
               return HRESULT_FROM_WIN32( GetLastError() );
            }

            // Check if we have remaining time left, for example, on a spurious wakeup we may wake up early.
            long long durationInMicrosecs = static_cast<long long>( highresolutionWaitSW.GetCurrentMicroseconds() );

            if ( durationInMicrosecs >= originalTimeoutInMicrosecs )
            {
               timeoutInMs = 0; // we've gone past the end of our timeout.
            }
            else
            {
               timeoutInMs = (uint32_t) ( ( durationInMicrosecs + 500 ) / 1000 ); // round up to nearest millisecond.
            }
         }
      }

      // Spin until the state machine enters the expected state.
      //
      // NOTE: there is no guarantee that it will still be in that state by the time you deal with the
      // result from this function. This is only meant to be a cheap way to spin on the state change.

      bool SpinWaitToEnterState( TStateEnum expectedState, uint32_t iterations = 1000 )
      {
         do
         {
            if ( GetCurrentState() == expectedState )
               return true;

            _mm_pause();

         } while ( iterations-- > 0 );

         return false;
      }
   private:

      void WakeAllOnStateChange()
      {
         ::InterlockedIncrement( static_cast<unsigned __int64*>( &waiterVariable ) );
         ::WakeByAddressAll( &waiterVariable );
      }

   };

}



