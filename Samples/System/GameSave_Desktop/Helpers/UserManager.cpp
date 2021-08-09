//--------------------------------------------------------------------------------------
// UserManager.cpp
//
// Implementation of the User Manager.
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
#include <memory.h>
#include <functional>
#include <XUser.h>
#include "HandleWrapperBase.h"
#include "XUserHandleWrapper.h"
#include "User.h"
#include "UserManager.h"
#include "AsyncOp.h"
#include "ScopedLockWrappers.h"
#include <unordered_map>
#include <mutex>

#pragma region Private implementation

namespace ATG {
   namespace UserManager {
       class SingleSignedInUserManager;
   }
}

namespace Internal
{
   class UserAddAsyncOp;
   
   SRWLOCK s_userManagerInitLock = SRWLOCK_INIT;
   ATG::UserManager::SingleSignedInUserManager* s_userMgr = nullptr;

   // This class implements a UserAddAsyncOp - a wrapper for the XUserAddAsync call.
   class UserAddAsyncOp final : public ATG::AsyncOp
   {
      XUserAddOptions options = XUserAddOptions::None;
      XUserHandle userHandle = nullptr;
   public:
      void SetUserAddOptions( XUserAddOptions options );
      XUserAddOptions GetUserAddOptions() const noexcept;
      // XUserHandle GetUserHandleResult() noexcept; ... not provided here.
      // - normally we'd probably want a way of reading back the result of an asyncop but instead we 
      // pass the result directly to the user manager when OnCompleted/OnError are executed.
      void OnCompleted() noexcept final;
      void OnError() noexcept final;
      void OnCanceled() noexcept final;
      void OnReset() noexcept final;
      HRESULT InvokeAsync(XAsyncBlock& block) noexcept final;
      HRESULT RetrieveAsyncResult(XAsyncBlock& block) noexcept final;
   };


   // Hashes a user local ID value. 
   struct XUserLocalIdHash {
      size_t operator()( const XUserLocalId& key ) const
      {
         // If size_t were uint64_t we could return key.value directly.
         return std::hash<unsigned long long>{}( key.value );
      }
   };

   // Compares two XUserLocalId values for equality.
   struct XUserLocalIdEqualTo {
      constexpr bool operator()( const XUserLocalId& lhs, const XUserLocalId& rhs )const
      {
         return lhs.value == rhs.value;
      }
   };

};

using namespace Internal;

namespace ATG {
   namespace UserManager {

      class SingleSignedInUserManager final
      {
         friend class ATG::User;
         friend class Internal::UserAddAsyncOp;

         SRWLOCK instanceLock;

         // The current active user.
         std::shared_ptr<ATG::User> currentUser;

         //NOTE: Use of an unordered_map may be overkill - you could get similar results with little to no perf difference
         //      with a walk through a std::vector (which is what we do for GamePads in InputDeviceManager).
         std::unordered_map< XUserLocalId, std::weak_ptr<ATG::User>, XUserLocalIdHash, XUserLocalIdEqualTo > userMap;

         // Count of currently tracked User handles/User local IDs. For diagnostic and debugging purposes.
         std::atomic<int> trackedUserCount;

         // Options to use when we ask for a new user.
         XUserAddOptions userSignInOptions;

         // The async op object that handles the call the XUserAddAsync.
         UserAddAsyncOp addNewUserAsyncOp;

         // Whether or not we're inside the first tick of the game (this is where silent sign-in behavior can occur).
         bool insideFirstTickCall;

         // Callbacks

         std::recursive_mutex currentUserChangedEventLock;
         std::function<ATG::UserManager::CurrentUserChangedCallbackFn> currentUserChangedCallbackFn;

         std::recursive_mutex signedInEventLock;
         std::function<ATG::UserManager::UserSignedInCallbackFn> signedInCallbackFn;

         std::recursive_mutex signedOutEventLock;
         std::function<ATG::UserManager::UserSignedOutCallbackFn> signedOutCallbackFn;

         std::recursive_mutex signingOutEventLock;
         std::function<ATG::UserManager::UserSigningOutCallbackFn> signingOutCallbackFn;

         // The task queue that we use to schedule work on.
         XTaskQueueHandle userOpsTaskQueue;

         // The 
         XTaskQueueRegistrationToken userChangeEventRegistrationToken;

      public:
         // Constructor
         SingleSignedInUserManager();

         // Destructor
         ~SingleSignedInUserManager();

         // Initializer
         void Initialize( _In_opt_ XTaskQueueHandle taskQueue, _In_ XUserAddOptions options );

         void SubscribeCurrentUserChanged( _In_ std::function<ATG::UserManager::CurrentUserChangedCallbackFn>& currentUserChangedCallback );
         void UnsubscribeCurrentUserChangedEvent();

         void SubscribeUserSignedInEvent( _In_ std::function<ATG::UserManager::UserSignedInCallbackFn>& userSignedInCallback );
         void UnsubscribeUserSignedInEvent();

         void SubscribeUserSignedOutEvent( _In_ std::function<ATG::UserManager::UserSignedOutCallbackFn>& userSignedOutCallback );
         void UnsubscribeUserSignedOutEvent();

         void SubscribeUserSigningOutEvent( _In_ std::function<ATG::UserManager::UserSigningOutCallbackFn>& userSigningOutCallback );
         void UnsubscribeUserSigningOutEvent();

         // Gets a reference to the user that is currently viewed as the primary user for the game. If this value is
         // null, try calling AskSystemForNewUserProfile() to get a user to sign in.
         std::shared_ptr<ATG::User> GetCurrentUser() noexcept;

         // Asks the system for a new user profile (to replace the current one, or it could be a no-op if the user
         // picks the same profile as before). Normally this is called in response to the user trying to start the game
         // when you don't have one, or the user trying to switch their profile on the main menu by pressing (Y).
         // 
         // Returns true if the operation was queued up, or false if another operation is running or it failed on startup.
         // Completion calls the function provided to SubscribeCurrentUserChanged with the result of the operation.
         bool AskSystemForNewUser();

         // Called once a frame - although we only need this for the first frame when you enter the game (we simply return
         // immediately on subsequent frames).
         void Tick() noexcept;

         // Call this when you know that the title is suspending.
         void Suspend() noexcept;

         // Call this when the title is resumed from suspend.
         void Resume() noexcept;

         // Finds a matching user for the provided local ID in the tracking collection.
         std::shared_ptr<ATG::User> Find( _In_ XUserLocalId id );

      private:
         // UserAddAsyncOp calls this with its result when we try to add a new user.
         void AddNewUserFromSystemCompleted( XUserHandle handle, HRESULT result );

         // Instance-level callback for User Changed event.
         void OnUserChanged( _In_ XUserLocalId userLocalId, _In_ XUserChangeEvent change );

         void NotifyUserSignedIn( std::shared_ptr<ATG::User>& userRef );

         // Called when the currently tracked user changes.
         void NotifyCurrentUserChanged( _In_ ATG::UserManager::CurrentUserChangedResult result,
            _In_ std::shared_ptr<ATG::User>& oldUser, _In_ std::shared_ptr<ATG::User>& newUser );

         void NotifyUserSigningOut( std::shared_ptr<ATG::User>& userRef );

         void NotifyUserSignedOut( std::shared_ptr<ATG::User>& userRef );

         // Adds a new user to the user tracking collection.
         std::shared_ptr<ATG::User> AddUserToMap( _In_ XUserHandle handle, _Out_ bool& isNewUser );

         void DeleteUserFromMap( _In_ ATG::User& user );

         // Static callback for user changed events. Used to bounce to the per-instance equivalent.
         static void CALLBACK OnUserChangedStatic( _In_opt_ void* context, _In_ XUserLocalId userLocalId,
            _In_ XUserChangeEvent change );

      };

   }
}

#pragma region UserAddAsyncOp implementation

   void UserAddAsyncOp::SetUserAddOptions( XUserAddOptions params )
   {
      options = params;
   }

   HRESULT UserAddAsyncOp::InvokeAsync( XAsyncBlock& block ) noexcept
   {
      assert( ( options == XUserAddOptions::AddDefaultUserSilently || options == XUserAddOptions::AllowGuests ||
         options == XUserAddOptions::None ) && "Must only have one or zero flags set on options for this call" );

      return XUserAddAsync( options, &block );
   }

   HRESULT UserAddAsyncOp::RetrieveAsyncResult( XAsyncBlock& block ) noexcept
   {
      assert( !userHandle && "User handle was expected to be null at this point - invalid state" );

      HRESULT hr = XUserAddResult( &block, &userHandle );
      return hr;
   }

   void UserAddAsyncOp::OnCompleted() noexcept
   {
      assert( s_userMgr != nullptr && "User manager not initialized" );

      // AddNewUserFromSystemCompleted takes ownership of the handle.
      XUserHandle user = userHandle;
      userHandle = nullptr;

      s_userMgr->AddNewUserFromSystemCompleted( user, error );
   }

   void UserAddAsyncOp::OnCanceled() noexcept
   {
      OnError();
   }

   void UserAddAsyncOp::OnError() noexcept
   {
      assert( s_userMgr != nullptr && "User manager not initialized" );
      assert( userHandle == nullptr && "User handle should be null here." );

      s_userMgr->AddNewUserFromSystemCompleted( nullptr, error );
   }

   void UserAddAsyncOp::OnReset() noexcept
   {
      // We'd normally clean up resources here, but based on how data flows through the system, the System should have
      // ownership of the handle instance. 

      assert( userHandle == nullptr && "User handle should be null here." );

      // Clear params
      options = XUserAddOptions::None;
   }

   XUserAddOptions UserAddAsyncOp::GetUserAddOptions() const noexcept
   {
      return options;
   }

#pragma endregion UserAddAsyncOp implementation

#pragma region SingleSignOnUserManager implementation

ATG::UserManager::SingleSignedInUserManager::SingleSignedInUserManager() :
    instanceLock{},
    trackedUserCount(0),
    userSignInOptions(XUserAddOptions::None),
    insideFirstTickCall(true),
    userOpsTaskQueue(nullptr),
    userChangeEventRegistrationToken{}
{
}

ATG::UserManager::SingleSignedInUserManager::~SingleSignedInUserManager()
{
   if ( userChangeEventRegistrationToken.token != 0 )
   {
      ::XUserUnregisterForChangeEvent( userChangeEventRegistrationToken, true );
      userChangeEventRegistrationToken = {};
   }

   if ( userOpsTaskQueue != nullptr )
   {
      ::XTaskQueueCloseHandle( userOpsTaskQueue );
      userOpsTaskQueue = nullptr;
   }
}

void ATG::UserManager::SingleSignedInUserManager::Initialize( _In_ XTaskQueueHandle taskQueue, _In_ XUserAddOptions options )
{
   userSignInOptions = options;

   if ( taskQueue )
   {
      DX::ThrowIfFailed( XTaskQueueDuplicateHandle( taskQueue, &userOpsTaskQueue ) );
      // Failure should never happen except in low-memory scenarios.
   }

   // Hook events.

   HRESULT hr = ::XUserRegisterForChangeEvent( userOpsTaskQueue, this, &SingleSignedInUserManager::OnUserChangedStatic,
      &userChangeEventRegistrationToken );

   DX::ThrowIfFailed( hr ); // Failure should never happen except maybe in low-memory scenarios.
}

void ATG::UserManager::SingleSignedInUserManager::SubscribeCurrentUserChanged( std::function<ATG::UserManager::CurrentUserChangedCallbackFn>& currentUserChangedCallback )
{
   std::lock_guard<std::recursive_mutex> lock( currentUserChangedEventLock );
   currentUserChangedCallbackFn = currentUserChangedCallback;
}

void ATG::UserManager::SingleSignedInUserManager::SubscribeUserSignedInEvent( std::function<ATG::UserManager::UserSignedInCallbackFn>& userSignedInCallback )
{
   std::lock_guard<std::recursive_mutex> lock( signedInEventLock );
   signedInCallbackFn = userSignedInCallback;
}

void ATG::UserManager::SingleSignedInUserManager::SubscribeUserSignedOutEvent( std::function<ATG::UserManager::UserSignedOutCallbackFn>& userSignedOutCallback )
{
   std::lock_guard<std::recursive_mutex> lock( signedOutEventLock );
   signedOutCallbackFn = userSignedOutCallback;
}

void ATG::UserManager::SingleSignedInUserManager::SubscribeUserSigningOutEvent( std::function<ATG::UserManager::UserSigningOutCallbackFn>& userSigningOutCallback )
{
   std::lock_guard<std::recursive_mutex> lock( signingOutEventLock );
   signingOutCallbackFn = userSigningOutCallback;
}

void ATG::UserManager::SingleSignedInUserManager::UnsubscribeCurrentUserChangedEvent()
{
   std::lock_guard<std::recursive_mutex> lock( currentUserChangedEventLock );
   currentUserChangedCallbackFn = nullptr;
}

void ATG::UserManager::SingleSignedInUserManager::UnsubscribeUserSignedInEvent()
{
   std::lock_guard<std::recursive_mutex> lock( signedInEventLock );
   signedInCallbackFn = nullptr;
}

void ATG::UserManager::SingleSignedInUserManager::UnsubscribeUserSignedOutEvent()
{
   std::lock_guard<std::recursive_mutex> lock( signedOutEventLock );
   signedOutCallbackFn = nullptr;
}

void ATG::UserManager::SingleSignedInUserManager::UnsubscribeUserSigningOutEvent()
{
   std::lock_guard<std::recursive_mutex> lock( signingOutEventLock );
   signingOutCallbackFn = nullptr;
}

void ATG::UserManager::SingleSignedInUserManager::NotifyCurrentUserChanged( _In_ ATG::UserManager::CurrentUserChangedResult result,
   _In_ std::shared_ptr<ATG::User>& oldUser, _In_ std::shared_ptr<ATG::User>& newUser )
{
   std::function<ATG::UserManager::CurrentUserChangedCallbackFn> tempCopy;

   {
      std::lock_guard<std::recursive_mutex> lock( currentUserChangedEventLock );
      tempCopy = currentUserChangedCallbackFn;
   }

   if ( tempCopy )
   {
      tempCopy( result, oldUser, newUser );
   }
}


void ATG::UserManager::SingleSignedInUserManager::NotifyUserSigningOut( std::shared_ptr<ATG::User>& userRef )
{
   std::function<ATG::UserManager::UserSigningOutCallbackFn> tempCopy;

   {
      std::lock_guard<std::recursive_mutex> lock( signingOutEventLock );
      tempCopy = signingOutCallbackFn;
   }

   if ( tempCopy )
   {
      tempCopy( userRef );
   }
}

void ATG::UserManager::SingleSignedInUserManager::NotifyUserSignedOut( std::shared_ptr<ATG::User>& userRef )
{
   std::function<ATG::UserManager::UserSignedOutCallbackFn> tempCopy;

   {
      std::lock_guard<std::recursive_mutex> lock( signedOutEventLock );
      tempCopy = signedOutCallbackFn;
   }

   if ( tempCopy )
   {
      tempCopy( userRef );
   }
}

void ATG::UserManager::SingleSignedInUserManager::NotifyUserSignedIn( std::shared_ptr<ATG::User>& userRef )
{
   std::function<ATG::UserManager::UserSignedInCallbackFn> tempCopy;

   {
      std::lock_guard<std::recursive_mutex> lock( signedInEventLock );
      tempCopy = signedInCallbackFn;
   }

   if ( tempCopy )
   {
      tempCopy( userRef );
   }
}

void CALLBACK ATG::UserManager::SingleSignedInUserManager::OnUserChangedStatic( _In_opt_ void* context, _In_ XUserLocalId userLocalId, _In_ XUserChangeEvent change )
{
   assert( context != nullptr && "Event context was null" );
   return static_cast<SingleSignedInUserManager*>( context )->OnUserChanged( userLocalId, change );
}

void ATG::UserManager::SingleSignedInUserManager::OnUserChanged( _In_ XUserLocalId userLocalId, _In_ XUserChangeEvent event )
{
   std::shared_ptr<ATG::User> userRef = Find( userLocalId );
   assert( userRef && "Untracked user referenced by callback - this shouldn't happen" );

   if ( userRef )
   {
      // If the current user is signing out, we need to signal that they're leaving to anyone tracking the identity of
      // the current user. 
      if ( event == XUserChangeEvent::SigningOut )
      {
         bool userIsCurrent;
         {
            ATG::ScopedSharedLock shared( instanceLock );
            userIsCurrent = ( currentUser && currentUser->GetLocalUserId() == userLocalId );
         }

         NotifyUserSigningOut( userRef );

         if ( userIsCurrent )
         {
            {
               ATG::ScopedExclusiveLock shared( instanceLock );
               currentUser = nullptr;
            }

            std::shared_ptr<ATG::User> nullUser = nullptr;
            NotifyCurrentUserChanged( ATG::UserManager::CurrentUserChangedResult::SignedOut, userRef, nullUser );
         }
      }
      else if ( event == XUserChangeEvent::SignedOut )
      {
         NotifyUserSignedOut( userRef );
      }
      else if ( event == XUserChangeEvent::SignedInAgain )
      {
         NotifyUserSignedIn( userRef );
      }

      userRef->NotifyUserChanged( event );

      // userRef goes out of scope here, dropping the user handle if the user has signed out.
   }
   else
   {
      // The user couldn't be found in our map. This is unusual, but could theoretically happen on sign-in 
      // if we implement sign-in events differently in the future. (In which case here we'd take a lock, get the
      // handle by calling XUserIdToUserHandle, and add it to the map). For now, treat this as an error case.
      assert( false && "Shouldn't end up here - user not found in map" );
   }
}

std::shared_ptr<ATG::User> ATG::UserManager::SingleSignedInUserManager::AddUserToMap( _In_ XUserHandle handle, _Out_ bool& isNewUser )
{
   assert( handle != nullptr && "User handle is not valid" );

   isNewUser = false;

   ATG::ScopedExclusiveLock exclusive( instanceLock );

   XUserLocalId localId = {};

   HRESULT hr = XUserGetLocalId( handle, &localId );
   DX::ThrowIfFailed( hr ); // Should never happen.

   auto it = userMap.find( localId );
   if ( it == userMap.end() )
   {
      // User not in map already.
      std::shared_ptr<ATG::User> userInstance = ATG::User::MakeSharedUserObject( localId, handle );
      userMap[ localId ] = userInstance->weak_from_this();
      isNewUser = true;
      ++trackedUserCount;
      return userInstance;
   }
   else // we found an existing reference
   {
      std::shared_ptr<ATG::User> user = it->second.lock();
      if ( !user )
      {
         // We have an entry already in the map, but our weak reference is dead, and our original object has not 
         // cleaned up yet - even though there are no outstanding references. So we create a new entry while we hold
         // the lock on the map. When the user tries to remove itself from the map (during deletion), we'll notice
         // that there's a valid reference to the object and skip the deletion. 

         std::shared_ptr<ATG::User> userInstance = ATG::User::MakeSharedUserObject( localId, handle );
         userMap[ localId ] = userInstance->weak_from_this();
         isNewUser = true;
         ++trackedUserCount;
         return userInstance;
      }
      else
      {
         return user;
      }
   }

}

void ATG::UserManager::SingleSignedInUserManager::DeleteUserFromMap( _In_ ATG::User& user )
{
   // This occurs after we decide to stop tracking the user in the map entirely. 

   // Take the lock on the map.
   ATG::ScopedExclusiveLock exclusive( instanceLock );

   // Find the user by id.
   auto it = userMap.find( user.GetLocalUserId() );

   if ( it != userMap.end() )
   {
      // We found an entry in the map.
      std::shared_ptr<ATG::User> instance = it->second.lock();
      if ( !instance )
      {
         // The reference is dead, and we have the lock, so we can kill our entry - no other thread
         // should be trying to add this user, or if they are, they can do so afterwards.
         userMap.erase( it );
         --trackedUserCount;
      }
      // else - the reference was revived with a new entry before we got here, so allow it to live.
   }
#ifndef _DEBUG
   else
   {
      // we weren't in the map, so no need to remove us. (Shouldn't ever happen - we should always be in the map if
      // delete is being called).
      assert( false && "User wasn't in the map, and cannot be removed" );
   }
#endif

}

std::shared_ptr<ATG::User> ATG::UserManager::SingleSignedInUserManager::Find( XUserLocalId id )
{
   assert( id.value != 0 && "User local id is invalid" );

   ATG::ScopedSharedLock shared( instanceLock );

   std::shared_ptr<ATG::User> userInstance = nullptr;

   auto it = userMap.find( id );
   if ( it != userMap.end() )
   {
      userInstance = it->second.lock();
   }

   return userInstance;
}

std::shared_ptr<ATG::User> ATG::UserManager::SingleSignedInUserManager::GetCurrentUser() noexcept
{
   ATG::ScopedSharedLock lock( instanceLock );
   return currentUser;
}

bool ATG::UserManager::SingleSignedInUserManager::AskSystemForNewUser()
{
   if ( !addNewUserAsyncOp.IsInTerminalState() && !addNewUserAsyncOp.IsReady() )
   {
      // Operation is busy doing something still; we can't ask for a new user at this time.
      return false;
   }
   else
   {
      if ( addNewUserAsyncOp.IsInTerminalState() )
      {
         addNewUserAsyncOp.Reset();
      }

      addNewUserAsyncOp.SetUserAddOptions( (XUserAddOptions) ( userSignInOptions & XUserAddOptions::AllowGuests ) );

      HRESULT hr = addNewUserAsyncOp.Start( userOpsTaskQueue, 0 );

      if ( hr != E_PENDING )
      {
         DX::ThrowIfFailed( hr );
      }
      return true; // true = we posted the request.
   }
}

void ATG::UserManager::SingleSignedInUserManager::Tick() noexcept
{
   // The first time the frame is updated by the game, if the userSignInOptions has the AddDefaultUserSilently flag set,
   // we silently add the current user. 

   if ( insideFirstTickCall &&
      ( userSignInOptions & XUserAddOptions::AddDefaultUserSilently ) == XUserAddOptions::AddDefaultUserSilently )
   {
      assert( addNewUserAsyncOp.IsReady() && "No one should be in the middle of adding a new user yet..." );

      // The default/silent user can't be a guest account, so we ignore if client code set it. (XUserAddAsync will
      // fail if it is called with both XUserAddOptions::AddDefaultUserSilently and XUserAddOptions::AllowGuests set
      // at the same time).
      addNewUserAsyncOp.SetUserAddOptions( XUserAddOptions::AddDefaultUserSilently );

      HRESULT hr = addNewUserAsyncOp.Start( userOpsTaskQueue, 0 );

      //NOTE: This will complete inside of UserAddAsyncOp, which will call back into 
      //      SingleSignedInUserManager::AddNewUserFromSystemCompleted on completion.

      if ( ( hr != E_PENDING ) && FAILED( hr ) )
      {
         if ( hr == E_ACCESSDENIED )
         {
            // Someone else must be getting a user, but they shouldn't be attempting right now. This should be an error.
            // We'll assert, but silently allow it in release builds - we'll just skip adding the default user in this case.
            assert( hr != E_ACCESSDENIED && "Already trying to add a user when we were called - this should not happen" );
            insideFirstTickCall = false;
            return;
         }
         // Any other error means an invalid argument, or the operation couldn't be queued at all, which is a bigger problem.

         DX::ThrowIfFailed( hr );
      }

      insideFirstTickCall = false;
   }
}

void ATG::UserManager::SingleSignedInUserManager::AddNewUserFromSystemCompleted( XUserHandle handle, HRESULT result )
{
   // This function handles the completion of the UserAddAsyncOp operation 

   if ( FAILED( result ) )
   {
      if ( result == E_ABORT ) // user canceled?
      {
         Log::Write( u8"UserManager: Select user profile canceled, continuing with current user: ID=%llu, GamerTag=%s\n", 
                     currentUser ? currentUser->GetLocalUserId().value : 0ULL, 
                     currentUser ? currentUser->GetGamerTag().c_str() : "<No Current User>" );

         NotifyCurrentUserChanged( CurrentUserChangedResult::Canceled, currentUser, currentUser );
      }
      else
      {
         if ( result == E_GAMEUSER_NO_DEFAULT_USER && (uint32_t)( addNewUserAsyncOp.GetUserAddOptions() & XUserAddOptions::AddDefaultUserSilently ) != 0 )
         {
            Log::WriteAndDisplay( u8"UserManager: Default user silent sign-in attempt failed - there was no default user.\n");
            // Do not notify here - it's ok to have an invalid current user at this time.
         }
         else
         {
            NotifyCurrentUserChanged( CurrentUserChangedResult::Error, currentUser, currentUser );
            Log::WriteAndDisplay( u8"Error in AddNewUserFromSystemCompleted - async operation failed with hr=0x%08X\n", result );
         }
      }

      // We don't need to do anything in this case, but it means that we couldn't get a different user. Your needs 
      // may vary.

   }
   else
   {
      // We have a new current user.
      assert( handle != nullptr && "User handle must be valid here" );

      XUserLocalId newUserId;
      HRESULT hr = XUserGetLocalId( handle, &newUserId ); // This should never fail under normal circumstances.
      DX::ThrowIfFailed( hr );

      // If the user id is the same as the current user we're tracking for sign-in, we don't need to do anything.
      if ( currentUser && currentUser->IsValid() && currentUser->GetLocalUserId() == newUserId )
      {
         XUserCloseHandle( handle );
         Log::Write( u8"UserManager: Existing user re-selected: ID=%llu, GamerTag=%s\n", 
                     newUserId.value, currentUser->GetGamerTag().c_str() );
         NotifyCurrentUserChanged( CurrentUserChangedResult::NoChange, currentUser, currentUser );
         return;
      }

      //NOTE: This may return an existing user we already know about, but they are guaranteed not to be the same as
      //      the existing current user.
      bool isNewUser;
      std::shared_ptr<ATG::User> newPrimaryUser = AddUserToMap( handle, isNewUser );

      if ( isNewUser )
      {
         Log::Write( u8"UserManager: Sign-In: New (untracked) user ID selected: ID=%llu, GamerTag=%s\n", 
                     newPrimaryUser->GetLocalUserId().value, newPrimaryUser->GetGamerTag().c_str() );
         NotifyUserSignedIn( newPrimaryUser );
      }
      else
      {
         Log::Write( u8"UserManager: Sign-In: Existing user ID selected: ID=%llu, GamerTag=%s\n", 
                     newPrimaryUser->GetLocalUserId().value, newPrimaryUser->GetGamerTag().c_str() );
      }

      // Replace current user.
      {
         ATG::ScopedExclusiveLock exclusive( instanceLock );
         //#FUTURE: Replace lock with std::atomic<std::shared_ptr> if using C++ 20.
         currentUser.swap( newPrimaryUser );
      }

      // Notify people that the users have changed. (NOTE: They were swapped above so have opposite meaning). They
      // should release their own references.
      NotifyCurrentUserChanged( CurrentUserChangedResult::Changed, newPrimaryUser /* actually old current user */, currentUser /* actually new user */ );

      // Release old currentUser (happens as variable newPrimaryUser goes out of scope).
   }
}

void ATG::UserManager::SingleSignedInUserManager::Suspend() noexcept
{

}

void ATG::UserManager::SingleSignedInUserManager::Resume() noexcept
{

}

#pragma endregion SingleSignOnUserManager implementation

#pragma endregion Private implementation

#pragma region Public API

void ATG::InitializeUserManager( _In_opt_ XTaskQueueHandle queue, 
   XUserAddOptions options /*= XUserAddOptions::AddDefaultUserSilently | XUserAddOptions::AllowGuests */ )
{
   ::ATG::ScopedExclusiveLock lock( s_userManagerInitLock );
   if ( !s_userMgr )
   {
      s_userMgr = new ATG::UserManager::SingleSignedInUserManager();
      s_userMgr->Initialize( queue, options );
   }
   else
   {
      assert( false && "Already initialized" );
      DX::ThrowIfFailed( HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED ) );
   }
}

void ATG::ShutdownUserManager() noexcept
{
   ATG::ScopedExclusiveLock lock( s_userManagerInitLock );
   assert( s_userMgr != nullptr && "Not initialized or already shut down" );
   delete s_userMgr;
   s_userMgr = nullptr;
}

std::shared_ptr<ATG::User> ATG::UserManager::GetCurrentUser() noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   return s_userMgr->GetCurrentUser();
}

bool ATG::UserManager::AskSystemForNewUser()
{
   assert( s_userMgr != nullptr && "Not initialized" );
   return s_userMgr->AskSystemForNewUser();
}

void ATG::UserManager::Tick()
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->Tick();
}

void ATG::SuspendUserManager()
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->Suspend();
}

void ATG::ResumeUserManager()
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->Resume();
}

std::shared_ptr<ATG::User> ATG::UserManager::Find( _In_ XUserLocalId id )
{
   assert( s_userMgr != nullptr && "Not initialized" );
   return s_userMgr->Find( id );
}

void ATG::UserManager::SubscribeCurrentUserChangedEvent( std::function<ATG::UserManager::CurrentUserChangedCallbackFn>& currentUserChangedCallback ) noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->SubscribeCurrentUserChanged( currentUserChangedCallback );
}

void ATG::UserManager::UnsubscribeCurrentUserChangedEvent() noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->UnsubscribeCurrentUserChangedEvent();
}

void ATG::UserManager::SubscribeUserSignedInEvent( std::function<ATG::UserManager::UserSignedInCallbackFn>& userSignedInCallback ) noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->SubscribeUserSignedInEvent( userSignedInCallback );
}

void ATG::UserManager::UnsubscribeUserSignedInEvent() noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->UnsubscribeUserSignedInEvent();
}

void ATG::UserManager::SubscribeUserSignedOutEvent( std::function<ATG::UserManager::UserSignedOutCallbackFn>& userSignedOutCallback ) noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->SubscribeUserSignedOutEvent( userSignedOutCallback );
}

void ATG::UserManager::UnsubscribeUserSignedOutEvent() noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->UnsubscribeUserSignedOutEvent();
}

void ATG::UserManager::SubscribeUserSigningOutEvent( std::function<ATG::UserManager::UserSigningOutCallbackFn>& userSigningOutCallback ) noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->SubscribeUserSigningOutEvent( userSigningOutCallback );
}

void ATG::UserManager::UnsubscribeUserSigningOutEvent() noexcept
{
   assert( s_userMgr != nullptr && "Not initialized" );
   s_userMgr->UnsubscribeUserSigningOutEvent();
}

#pragma endregion Public API
