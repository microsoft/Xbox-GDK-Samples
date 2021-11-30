//--------------------------------------------------------------------------------------
// User.cpp
//
// ATG implementation of a User object wrapper.
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

#include "HandleWrapperBase.h"
#include "XUserHandleWrapper.h"
#include "ScopedLockWrappers.h"
#include "User.h"
#include "InputDeviceManager.h"

namespace ATG
{

#pragma region GamerTag functionality

   // NOTE: The user's GamerTag is updated lazily when you access it, and then cached. When it changes, we zero it out
   //       and the next time you access it, we'll get it again.
   
   std::string User::GetGamerTag() const noexcept
   {
      assert( userHandle.IsValid() && "User must be valid" );
      
      ScopedSharedLock userLock( srwLock );
      std::string gamertag(XUserGamertagComponentClassicMaxBytes, '\0' );

      size_t gamertagSize = 0;
      
      HRESULT hr = XUserGetGamertag( userHandle, XUserGamertagComponent::Classic, gamertag.size(), &gamertag[ 0 ], &gamertagSize );
      DX::ThrowIfFailed( hr ); // should never happen.

      gamertag.resize( gamertagSize - 1 );
      
      return gamertag;
   }

#pragma endregion GamerTag functionality

   std::shared_ptr<ATG::User> User::MakeSharedUserObject( XUserLocalId id, XUserHandle handle )
   {
      return std::make_shared<ATG::User>( id, handle );
   }

   User::User( XUserLocalId localId, XUserHandle handle )
      : userLocalId( localId ),
        userHandle( handle )
   {
   }

   User::~User()
   {
      UnsubscribeUserChangedEvent();
   }

   void User::NotifyUserChanged( XUserChangeEvent change )
   {
      if ( userChangedCallbackFn )
      {
         userChangedCallbackFn( *this, change );
      }
   }
   
   void User::SubscribeUserChangedEvent( std::function<UserChangedStateCallback>& fn )
   {
      ATG::ScopedExclusiveLock userLock( srwLock );
      userChangedCallbackFn = fn;
   }

   void User::UnsubscribeUserChangedEvent()
   {
      ATG::ScopedExclusiveLock userLock( srwLock );
      userChangedCallbackFn = nullptr;
   }

   bool User::QueryHasGamePad() const noexcept
   {
      return ATG::HasBoundGamePad( userLocalId );
   }

   XUserState User::GetUserState() const
   {
      assert( IsValid() && "User have a valid handle to check its state" );
      XUserState state;
      HRESULT hr = XUserGetState( userHandle, &state );
      DX::ThrowIfFailed( hr );
      return state;
   }

   void User::AttachUser( XUserHandle handle )
   {
      ScopedExclusiveLock lock( srwLock );
      assert( userLocalId.value == 0 && !userHandle.IsValid() && "User reference must be null" );
      HRESULT hr = ::XUserGetLocalId( handle, &userLocalId );
      DX::ThrowIfFailed( hr );
   }

#pragma endregion User function implementations
}
