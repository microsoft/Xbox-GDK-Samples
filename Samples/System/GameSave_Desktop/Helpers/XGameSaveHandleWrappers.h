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
#include <XGameSave.h>

namespace ATG
{
   /**
    *	RAII pattern wrapper class around an XGameSaveProviderHandle.
    *
    * This class ensures that the handle ownership is managed correctly and it
    * is destroyed rather than leaked if it goes out of scope.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    */
   class GameSaveProviderHandle final : public GXDKHandleBase< XGameSaveProviderHandle, GameSaveProviderHandle >
   {
      friend class GXDKHandleBase< XGameSaveProviderHandle, GameSaveProviderHandle >;
   public:

      /**
       *	Creates an empty GameSaveProviderHandle object. (Which is not managing a handle).
       */
      GameSaveProviderHandle() : GXDKHandleBase() { }

      /**
       * Creates a new GameSaveProviderHandle, wrapping the passed-in XGameSaveProviderHandle, and taking ownership of 
       * it.
       * 
       * \param XGameSaveProviderHandle providerHandle - the handle to wrap and own.
       */
      GameSaveProviderHandle( XGameSaveProviderHandle providerHandle ) : GXDKHandleBase( providerHandle ) { }

      /**
       * Move-constructs a GameSaveProviderHandle object from another GameSaveProviderHandle object.
       * 
       * \param GameSaveProviderHandle&& moveFrom - the object to move-construct from.
       */
      GameSaveProviderHandle( GameSaveProviderHandle&& moveFrom ) : GXDKHandleBase( std::move( moveFrom ) ) { }

      /**
       * Deleted copy constructor. Implicit copying of GameSaveProviderHandle is not allowed.
       */
      GameSaveProviderHandle( const GameSaveProviderHandle& copyFrom ) = delete;

      /**
       * Deleted copy-assignment operator. Implicit copying of GameSaveProviderHandle is not allowed.
       */
      GameSaveProviderHandle& operator=( const GameSaveProviderHandle& copyFrom ) = delete;

      /**
       * Move-assignment operator implementation. 
       * 
       * \param GameSaveProviderHandle&& moveFrom - the object to move-assign from.
       * \return A reference to the ATG::GameSaveProviderHandle object that was assigned into.
       */
      GameSaveProviderHandle& operator=( GameSaveProviderHandle&& moveFrom ) noexcept
      {
         if (this != &moveFrom)
         {
            GXDKHandleBase::operator=(std::move(moveFrom));
         }
         return *this;
      }

      /**
       * Implicit cast-to-XGameSaveProviderHandle operator.
       * \returns The underlying handle owned by this object.
       */
      operator XGameSaveProviderHandle() const
      {
         return handle;
      }

   protected:

      /**
       * Implements closing the underlying XGameSaveProviderHandle for the GXDKHandleWrapper base class.
       */
      void CloseImpl() noexcept
      {
         if ( handle )
         {
            auto handleTemp = handle;
            handle = nullptr;
            XGameSaveCloseProvider( handleTemp );
         }
      }

   };

   /**
    *	RAII pattern wrapper class around an XGameSaveContainerHandle.
    *
    * This class ensures that the handle ownership is managed correctly and it
    * is destroyed rather than leaked if it goes out of scope.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    */
   class GameSaveContainerHandle final : public GXDKHandleBase< XGameSaveContainerHandle, GameSaveContainerHandle >
   {
      friend class GXDKHandleBase< XGameSaveContainerHandle, GameSaveContainerHandle >;
   public:

      /**
       *	Creates an empty GameSaveContainerHandle object. (Which is not managing a handle).
       */
      GameSaveContainerHandle() : GXDKHandleBase() { }

      /**
       * Creates a new GameSaveContainerHandle, wrapping the passed-in XGameSaveContainerHandle, and taking ownership of
       * it.
       *
       * \param XGameSaveContainerHandle containerHandle - the handle to wrap and own.
       */
      GameSaveContainerHandle( XGameSaveContainerHandle containerHandle ) : GXDKHandleBase( containerHandle ) { }

      /**
       * Move-constructs a GameSaveContainerHandle object from another GameSaveContainerHandle object.
       * 
       * \param GameSaveContainerHandle&& moveFrom - the object to move-construct from.
       */
      GameSaveContainerHandle( GameSaveContainerHandle&& moveFrom ) : GXDKHandleBase( std::move( moveFrom ) ) { }

      /**
       * Deleted copy constructor. Implicit copying of GameSaveContainerHandle is not allowed.
       */
      GameSaveContainerHandle( const GameSaveContainerHandle& copyFrom ) = delete;

      /**
       * Deleted copy-assignment operator. Implicit copying of GameSaveContainerHandle is not allowed.
       */
      GameSaveContainerHandle& operator=( const GameSaveContainerHandle& copyFrom ) = delete;

      /**
       * Move-assignment operator implementation. 
       * 
       * \param GameSaveContainerHandle&& moveFrom - the object to move-assign from.
       * \return A reference to the ATG::GameSaveContainerHandle object that was assigned into.
       */
      GameSaveContainerHandle& operator=( GameSaveContainerHandle&& moveFrom ) noexcept
      {
         if (this != &moveFrom)
         {
            GXDKHandleBase::operator=(std::move(moveFrom));
         }
         return *this;
      }

      /**
       * Implicit cast-to-XGameSaveContainerHandle operator.
       * \returns The underlying handle owned by this object.
       */
      operator XGameSaveContainerHandle() const 
      {
         return handle;
      }

   protected:
      /**
       * Implements closing the underlying XGameSaveContainerHandle for the GXDKHandleWrapper base class.
       */
      void CloseImpl() noexcept
      {
         if ( handle )
         {
            auto handleTemp = handle;
            handle = nullptr;
            XGameSaveCloseContainer( handleTemp );
         }
      }
   };

   /**
    *	RAII pattern wrapper class around an XGameSaveUpdateHandle.
    *
    * This class ensures that the handle ownership is managed correctly and it
    * is destroyed rather than leaked if it goes out of scope.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    */
   class GameSaveUpdateHandle final : public GXDKHandleBase < XGameSaveUpdateHandle, GameSaveUpdateHandle >
   {
      friend class GXDKHandleBase< XGameSaveUpdateHandle, GameSaveUpdateHandle >;
   public:

      /**
       *	Creates an empty GameSaveUpdateHandle object. (Which is not managing a handle).
       */
      GameSaveUpdateHandle() : GXDKHandleBase() { }

      /**
       * Creates a new GameSaveUpdateHandle, wrapping the passed-in XGameSaveUpdateHandle, and taking ownership of
       * it.
       *
       * \param XGameSaveUpdateHandle updateHandle - the handle to wrap and own.
       */
      GameSaveUpdateHandle( XGameSaveUpdateHandle updateHandle ) : GXDKHandleBase( updateHandle ) { }

      /**
       * Move-constructs a GameSaveUpdateHandle object from another GameSaveUpdateHandle object.
       * 
       * \param GameSaveUpdateHandle&& moveFrom - the object to move-construct from.
       */
      GameSaveUpdateHandle( GameSaveUpdateHandle&& moveFrom ) : GXDKHandleBase( std::move( moveFrom ) ) { }

      /**
       * Deleted copy constructor. Implicit copying of GameSaveUpdateHandle is not allowed.
       */
      GameSaveUpdateHandle( const GameSaveUpdateHandle& copyFrom ) = delete;

      /**
       * Deleted copy-assignment operator. Implicit copying of GameSaveUpdateHandle is not allowed.
       */
      GameSaveUpdateHandle& operator=( const GameSaveUpdateHandle& copyFrom ) = delete;

      /**
       * Move-assignment operator implementation. 
       * 
       * \param GameSaveUpdateHandle&& moveFrom - the object to move-assign from.
       * \return A reference to the ATG::GameSaveUpdateHandle object that was assigned into.
       */
      GameSaveUpdateHandle& operator=( GameSaveUpdateHandle&& moveFrom ) noexcept
      {
         if (this != &moveFrom)
         {
            GXDKHandleBase::operator=(std::move(moveFrom));
         }
         return *this;
      }

      /**
       * Implicit cast-to-XGameSaveUpdateHandle operator.
       * \returns The underlying handle owned by this object.
       */
      operator XGameSaveUpdateHandle() const
      {
         return handle;
      }

   protected:
      /**
       * Implements closing the underlying XGameSaveUpdateHandle for the GXDKHandleWrapper base class.
       */
      void CloseImpl() noexcept
      {
         if ( handle )
         {
            auto handleTemp = handle;
            handle = nullptr;
            XGameSaveCloseUpdate( handleTemp );
         }
      }
   };
}
