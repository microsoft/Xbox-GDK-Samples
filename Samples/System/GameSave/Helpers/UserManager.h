//--------------------------------------------------------------------------------------
// UserManager.h
//
// Handles user management for scenarios where there is only a single signed-in user.
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

namespace ATG
{
#pragma region Startup and Shutdown

   void InitializeUserManager( _In_opt_ XTaskQueueHandle taskQueue,
      _In_ XUserAddOptions options = XUserAddOptions::AddDefaultUserSilently | XUserAddOptions::AllowGuests );
   void ShutdownUserManager() noexcept;

   // Called when the title is Suspending.
   void SuspendUserManager();

   // Called when the title is Resuming from suspend.
   void ResumeUserManager();


#pragma endregion Startup and Shutdown

   namespace UserManager
   {

#pragma region User functions

      // Gets a ref-counted copy of the current signed-in user (or null if there is none).
      std::shared_ptr<ATG::User> GetCurrentUser() noexcept;

      // Asks the system to get a new user.  Returns false if the system was already busy acquiring users, true if the
      // request was posted.
      bool AskSystemForNewUser();

      // Should be called every frame once the game is up and running (we get the default user in some scenarios the first
      // time that the user is ticked).
      void Tick();


      // Finds a matching user for the provided local ID in the tracking collection.
      std::shared_ptr<ATG::User> Find( _In_ XUserLocalId id );

#pragma endregion User functions   
      
#pragma region Events

      enum class CurrentUserChangedResult
      {
         Error,     // A failure occurred.
         Canceled,  // The player canceled the request to sign in from the UI.
         Changed,   // A new user was selected.
         NoChange,  // The same user was selected as before.
         SignedOut, // The user signed out.
      };

      using CurrentUserChangedCallbackFn = void( ATG::UserManager::CurrentUserChangedResult result, 
                                           std::shared_ptr<ATG::User>& oldUser, std::shared_ptr<ATG::User>& newUser );

      void SubscribeCurrentUserChangedEvent( std::function<CurrentUserChangedCallbackFn>& currentUserChangedCallback ) noexcept;
      void UnsubscribeCurrentUserChangedEvent() noexcept; 
      
      using UserSignedInCallbackFn = void( std::shared_ptr<ATG::User>& userRef );
      
      void SubscribeUserSignedInEvent( std::function<UserSignedInCallbackFn>& userSignedInCallback ) noexcept;
      void UnsubscribeUserSignedInEvent() noexcept;
      
      using UserSigningOutCallbackFn = void( std::shared_ptr<ATG::User>& userRef );

      void SubscribeUserSigningOutEvent( std::function<UserSigningOutCallbackFn>& userSigningOutCallback ) noexcept;
      void UnsubscribeUserSigningOutEvent() noexcept;

      using UserSignedOutCallbackFn = void( std::shared_ptr<ATG::User>& userRef );

      void SubscribeUserSignedOutEvent( std::function<UserSignedOutCallbackFn>& userSignedOutCallback ) noexcept;
      void UnsubscribeUserSignedOutEvent() noexcept;
      
#pragma endregion Events
      
   }
}

#pragma once
