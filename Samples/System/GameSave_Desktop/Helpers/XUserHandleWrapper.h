//--------------------------------------------------------------------------------------
// XUserHandleWrapper.h
//
// Wrappers around XUserHandle
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

namespace ATG
{
   /**
    *	RAII pattern wrapper class around an XUserHandle.
    *
    * This class ensures that the handle ownership is managed correctly and it
    * is destroyed rather than leaked if it goes out of scope.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    */
   class UserHandle : public GXDKHandleBase< XUserHandle, UserHandle >
   {
      friend class GXDKHandleBase< XUserHandle, UserHandle >;
   public:

      /**
       * Creates an empty UserHandle object which is not managing a handle.
       */
      UserHandle() : GXDKHandleBase() { }

      /**
       * Creates a new UserHandle object, taking ownership of the passed-in handle.
       * 
       * \param XUserHandle userHandle - the user handle to manage.
       */
      UserHandle( XUserHandle userHandle ) : GXDKHandleBase( userHandle ) { }

      /**
       * Move-constructs a UserHandle object from another UserHandle object.
       * 
       * \param UserHandle&& moveFrom - the object to move-construct from.
       */
      UserHandle( UserHandle&& moveFrom ) : GXDKHandleBase( std::move( moveFrom ) ) { }

      /**
       * Copy-constructs a UserHandle object from another UserHandle object.
       *
       * This operation calls \see XUserDuplicateHandle to duplicate the underlying XUserHandle.
       *
       * \param const UserHandle& copyFrom - the object to copy-construct from.
       */
      UserHandle( const UserHandle& copyFrom )
      {
         if ( copyFrom.IsValid() )
         {
            DX::ThrowIfFailed( ::XUserDuplicateHandle( const_cast<UserHandle&>( copyFrom ), &handle ) );
         }
      }

      /**
       * Move-assigns a UserHandle object from another UserHandle object.
       *
       * \param const UserHandle&& moveFrom - the object to move-construct from.
       */
      UserHandle& operator=( UserHandle&& moveFrom ) noexcept
      {
         if (this != &moveFrom)
         {
            GXDKHandleBase::operator=(std::move(moveFrom));
         }

         return *this;
      }

      /**
       * Copy-assigns a UserHandle object from another UserHandle object.
       *
       * This operation calls \see XUserDuplicateHandle to duplicate the underlying XUserHandle.
       *
       * \param const UserHandle& copyFrom - the object to copy-assign from.
       */
      UserHandle& operator=( const UserHandle& copyFrom ) noexcept
      {
         if (this != &copyFrom)
         {
            XUserHandle dup;
            DX::ThrowIfFailed(::XUserDuplicateHandle(const_cast<UserHandle&>(copyFrom), &dup));
            Close();
            handle = dup;
         }
         return *this;
      }

   public:

      /**
       * Compares two handles, returning the result from them.
       * 
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       *
       * \param const UserHandle& otherUser - the other user to compare
       * \return int32_t - <1 if this is less than otherUser, 0 if they are the same, +1 if this is greater than otherUser.
       */
      int32_t Compare( const UserHandle& otherUser ) const noexcept
      {
         return XUserCompare( handle, otherUser.handle );
      }

      /**
       * Compares two users, returning whether or not this user is greater than the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       * 
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is greater than the other user, false otherwise.
       */
      bool operator > ( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) > 0;
      }

      /**
       * Compares two users, returning whether or not this user is greater than or equal to the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       *
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is greater than or equal to the other user, false otherwise.
       */
      bool operator >= ( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) >= 0;
      }

      /**
       * Compares two users, returning whether or not this user is less than or equal to the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       *
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is less than or equal to the other user, false otherwise.
       */
      bool operator <= ( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) <= 0;
      }

      /**
       * Compares two users, returning whether or not this user is less than the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       * 
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is less than the other user, false otherwise.
       */
      bool operator < ( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) < 0;
      }

      /**
       * Compares two users, returning whether or not this user is the same as the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       * 
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is the same as the other user, false otherwise.
       */
      bool operator==( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) == 0;
      }

      /**
       * Compares two users, returning whether or not this user is different to the other user.
       *
       * \remarks Internally this calls XUserCompare. User Handles have a session-duration relative ordering.
       * 
       * \param const UserHandle& otherUser - the other user to compare against.
       * \return True if this user is different to the other user, false if they are the same.
       */
      bool operator!=( const UserHandle& otherUser ) const noexcept
      {
         return Compare( otherUser ) != 0;
      }

      /**
       * Explicitly duplicates a UserHandle, creating a new object.
       * 
       * \note You can implicitly duplicate a user handle via copy construction or copy-assignment.
       * ...
       * \return A ATG::UserHandle object which contains a duplicate handle to the one in this object.
       */
      UserHandle Duplicate() const
      {
         assert( IsValid() && "User handle must be valid before it can be duplicated" );

         XUserHandle dup;
         DX::ThrowIfFailed( ::XUserDuplicateHandle( handle, &dup ) );
         return UserHandle( dup );
      }

      /**
       * Gets the local ID associated with this user handle.
       * 
       * \return An XUserLocalID object which contains the user ID value of this user. Note: Zero represents an invalid
       *         XUserLocalId value.
       */
      XUserLocalId GetLocalId() const noexcept
      {
         if ( !IsValid() )
            return XUserLocalId { 0 };

         XUserLocalId userLocalId = {};
         DX::ThrowIfFailed( ::XUserGetLocalId( handle, &userLocalId ) );
         return userLocalId;
      }

      /**
       * Returns true if this user is a guest, false if they are using a full account.
       */
      bool IsGuest() const noexcept
      {
         assert( IsValid() && "User must be valid to call this method" );
         bool value;
         DX::ThrowIfFailed( ::XUserGetIsGuest( handle, &value ) );
         return value;
      }

   protected:
      /**
       * Implements closing the underlying XUserHandle for the GXDKHandleWrapper base class.
       */
      void CloseImpl() noexcept
      {
         if ( handle )
         {
            XUserHandle temp = handle;
            handle = nullptr;
            ::XUserCloseHandle( temp );
         }
      }
   };
}

// Helper operator overloads for XUserLocalId.

/**
 * Implements the expression ( lhs == rhs ) for two XUserLocalId values.
 * 
 * \param const XUserLocalId lhs - the left-hand XUserLocalId value.
 * \param const XUserLocalId rhs - the right-hand XUserLocalId value.
 * \return True if they are the same, false otherwise.
 */
inline bool operator==( const XUserLocalId lhs, const XUserLocalId rhs )
{
   return lhs.value == rhs.value;
}

/**
 * Implements the expression ( lhs != rhs ) for two XUserLocalId values.
 *
 * \param const XUserLocalId lhs - the left-hand XUserLocalId value.
 * \param const XUserLocalId rhs - the right-hand XUserLocalId value.
 * \return True if they are different, false if they are the same.
 */
inline bool operator!=( const XUserLocalId lhs, const XUserLocalId rhs )
{
   return !( lhs == rhs );
}

/**
 * Implements the expression ( lhs == rhs ) for a XUserLocalId value and an ATG::UserHandle object's local id.
 *
 * \param const XUserLocalId lhs - the left-hand XUserLocalId value.
 * \param const ATG::UserHandle& rhs - the right-hand UserHandle object.
 * \return True if they are the same, false otherwise.
 */
inline bool operator==( const XUserLocalId lhs, const ATG::UserHandle& rhs )
{
   return lhs.value == rhs.GetLocalId().value;
}

/**
 * Implements the expression ( lhs != rhs ) for a XUserLocalId value and an ATG::UserHandle object's local id.
 *
 * \param const XUserLocalId lhs - the left-hand XUserLocalId value.
 * \param const ATG::UserHandle& rhs - the right-hand UserHandle object.
 * \return True if they are different, false if they are the same.
 */
inline bool operator!=( const XUserLocalId lhs, const ATG::UserHandle& rhs )
{
   return !( lhs == rhs );
}

/**
 * Implements the expression ( lhs == rhs ) for an ATG::UserHandle object's local id and a XUserLocalId value.
 *
 * \param const ATG::UserHandle& lhs - the left-hand UserHandle object.
 * \param const XUserLocalId rhs - the right-hand XUserLocalId value.
 * \return True if they are the same, false otherwise.
 */
inline bool operator==( const ATG::UserHandle& lhs, const XUserLocalId rhs )
{
   return lhs.GetLocalId().value == rhs.value;
}

/**
 * Implements the expression ( lhs != rhs ) for an ATG::UserHandle object's local id and a XUserLocalId value.
 *
 * \param const ATG::UserHandle& lhs - the left-hand UserHandle object.
 * \param const XUserLocalId rhs - the right-hand XUserLocalId value.
 * \return True if they are different, false if they are the same.
 */
inline bool operator!=( const ATG::UserHandle& lhs, const XUserLocalId rhs )
{
   return !( lhs == rhs );
}
