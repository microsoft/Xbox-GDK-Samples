//--------------------------------------------------------------------------------------
// HandleWrapperBase.h
//
// Base wrapper type for Gaming Runtime handles
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
   /**
    *	The base class for GXDK-defined RAII handle wrapper classes. This implements all of the standard functionality 
    * common between all handle types defined by the GXDK.
    *
    * It uses the curiously-recurring template pattern to avoid unnecessary virtual functions.
    *
    * \param THandleType - the concrete handle type to wrap.
    * \param TDerived - the child class which is implementing type-specific functionality for this handle (e.g. 
    *                   \see UserHandle).
    */
   template < typename THandleType, typename TDerived >
   class GXDKHandleBase
   {
   protected:
      THandleType handle = nullptr;    //< The handle which is being wrapped. By default its value is empty/null.
   public:
      /**
       *	Constructs an empty handle.
       */
      GXDKHandleBase() {};


      /**
       * Constructs a handle wrapper from a raw handle value.
       * 
       * \param THandleType handleToWrap - the handle which will be wrapped.
       * \return The handle wrapper object.
       */
      GXDKHandleBase( THandleType handleToWrap )
         : handle( handleToWrap )
      {
      };

      /**
       *	Closes the handle when the type is destroyed.
       */
      ~GXDKHandleBase()
      {
         Close();
      }

      /**
       *	Deleted copy constructor. Implicit copying of handles is not allowed by default.
       */
      GXDKHandleBase( const GXDKHandleBase& copyFrom ) = delete;

      /**
       *	Move-constructor.
       * \param GXDKHandleBase&& moveFrom - the object to move-construct from.
       */
      GXDKHandleBase( GXDKHandleBase&& moveFrom ) noexcept
      {
         handle = moveFrom.handle;
         moveFrom.handle = nullptr;
      }

      /**
       *	Deleted copy-assignment operator. Implicit copying of handles is not allowed by default.
       */
      GXDKHandleBase& operator=( const GXDKHandleBase& copyFrom ) = delete;

      /**
       *	Move-assignment operator.
       * \param GXDKHandleBase&& moveFrom - the object to move-assign from.
       * \return GXDKHandleBase& - a reference to this object.
       */
      GXDKHandleBase& operator=( GXDKHandleBase&& moveFrom ) noexcept
      {
         if ( &moveFrom != this )
         {
            Close();
            handle = moveFrom.handle;
            moveFrom.handle = nullptr;
         }

         return *this;
      }

      /**
       *	Returns true if the underlying handle is valid, false otherwise (that is, if it's empty or invalid).
       */
      bool IsValid() const noexcept
      {
         return handle != nullptr;
      }

      /**
       * Implicit cast-to-bool operator.
       * 
       *	\returns true if the handle is valid, false otherwise.
       */
      operator bool() const noexcept
      {
         return IsValid();
      }

      /**
       *	Implicit cast-to-THandleType operator.
       *
       * \returns The wrapped handle value.
       */
      operator THandleType() const noexcept
      {
         return handle;
      }

      /**
       * Closes the handle.
       * 
       * Calls the child class to close the attached handle. It is the child class's responsibility to set the value
       * of handle to empty.
       */
      void Close()
      {
         static_cast<TDerived*>( this )->CloseImpl();
      }

      /**
       *	Attaches a handle to this object.
       *
       * You can only call this function if the object has not already been assigned ownership of a handle.
       *
       * \param THandleType handleToAttach - the handle to wrap.
       */
      void Attach( THandleType handleToAttach ) noexcept
      {
         assert( handle == nullptr && "Handle already attached" );
         handle = handleToAttach;
      }

      /**
       *	Detaches a handle from this object.
       *
       * \note This function asserts if you call it on an empty object - usually this implies an error in your code.
       *
       * \return the Handle which was attached to the object.
       */
      THandleType Detach() noexcept
      {
         assert( handle != nullptr && "No handle to detach");
         THandleType h = handle;
         handle = nullptr;
         return h;
      }

      /**
       *	Returns a pointer to the storage for the raw handle.
       *
       * \note This function asserts if you call it on an object which already owns a handle - usually this implies an
       *       error in your code.
       * \return THandleType* - a pointer to the location in memory where the handle is stored in this wrapper object.
       */
      THandleType* GetPtrToHandle() noexcept
      {
         assert( handle == nullptr && "Handle already attached" );
         return &handle;
      }
   };
}
