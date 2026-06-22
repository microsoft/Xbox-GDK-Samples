//--------------------------------------------------------------------------------------
// User.h
//
// ATG implementation of a User object.
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

#include "XUserHandleWrapper.h"

namespace ATG { namespace UserManager { class SingleSignedInUserManager; }; };

namespace ATG
{
   
   // A user object, representing one of the players of a game.
   class User : public std::enable_shared_from_this< User >
   {
      friend class ATG::UserManager::SingleSignedInUserManager;
   public:
      using UserChangedStateCallback = void( ATG::User&, XUserChangeEvent );
      using UserGamePadChangedCallback = void( ATG::User&, XUserDeviceAssociationChange& changeType );
   private:
      mutable SRWLOCK srwLock = SRWLOCK_INIT;
      XUserLocalId userLocalId = {};
      ATG::UserHandle userHandle;
      std::function<UserChangedStateCallback> userChangedCallbackFn = nullptr;

   public:
      User( XUserLocalId localId, XUserHandle handle );

      // Destructor.
      ~User();

      // Gets the user's handle.
      operator XUserHandle() noexcept
      {
         return (XUserHandle)userHandle;
      }

      //NOTE: No lock necessary - this is immutable for the entire lifetime of the object once created.
      XUserLocalId GetLocalUserId() const noexcept { return userLocalId; }

      //NOTE: No lock necessary - this is immutable for the entire lifetime of the object once created.
      bool IsValid() const noexcept { return userHandle.IsValid(); }

      // Allows a single subscriber to sign up for user change event notifications.
      void SubscribeUserChangedEvent( std::function<UserChangedStateCallback>& fn );

      // Allows the single subscriber to unsubscribe from the event.
      void UnsubscribeUserChangedEvent();

      bool QueryHasGamePad() const noexcept;
      
      // Gets the current state of the user.
      XUserState GetUserState() const;

      // We cache the user's gamertag, but it could change during a gameplay session. Whenever it changes we clear it 
      // out, and ask the OS for it again the next time you call. This operation should always be roughly on the order 
      // of a memcpy in duration.
      //
      // If the gamertag is marked invalid, then on the next request for it we request the value from the system. 
      std::string GetGamerTag() const noexcept;

   protected:
      void AttachUser( _In_ XUserHandle handle );

      // NOTE: Entry point for UserManager - do not call yourself.
      void NotifyUserChanged( XUserChangeEvent change );
      
      // NOTE: Entry point for UserManager - do not call yourself.
      static std::shared_ptr<ATG::User> MakeSharedUserObject( XUserLocalId id, XUserHandle handle );
   };
}
