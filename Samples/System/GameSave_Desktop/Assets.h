// Assets.h
//
// Simple example asset handling for the game.
//
// This is not meant to be an exhaustive or complete implementation, but is intended to
// provide a starting point.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

namespace DX { class Texture;  }
namespace DirectX { class SpriteFont; }

/**
 *	Bring in the asset names/ID values for all assets used by this sample.
 */
#define GENERATE_DESCRIPTOR_ENUM 1
// NOTE: The last value in the enum class AssetDescriptor : size_t enum type should be the count of elements, 
//       called DESCRIPTOR_COUNT
#include "SampleSpecificAssets.inl"
#undef GENERATE_DESCRIPTOR_ENUM

namespace ATG
{

   /**
    *	The type of the asset. (Currently we only support two types - arbitrary textures and fonts).
    */
   enum class AssetType
   {
      Texture,          //< The asset is a Texture
      SpriteFont,       //< The asset is a SpriteFont
   };

   /**
    *	Current state of the asset.
    */
   enum class AssetState
   {
      NotLoaded = 0,    //< Has not been loaded or is not in memory.
      Loading,          //< We're trying to create the asset, and are loading it from storage now.
      Loaded,           //< The asset has been loaded and is ready for use.
      Releasing         //< The asset is being unloaded (but is not yet fully unloaded).
   };

   /**
    *	Helper structure which is used to allow assets to be bound appropriately on load.
    */
   struct AssetLoadResources
   {
      ID3D12Device* device;
      DirectX::ResourceUploadBatch* upload;
   };

   /**
    *	Gets the descriptor heap that the assets are associated with.
    */
   DirectX::DescriptorHeap* GetAssetDescriptorHeap() noexcept;

   /**
    *	Sets the asset descriptor heap that the assets are associated with.
    */
   void SetAssetDescriptorHeap( DirectX::DescriptorHeap* heap ) noexcept;

   struct AssetEntry
   {
      AssetDescriptor id;
      const wchar_t* path;
      AssetType type;
      void* resource = nullptr;
      std::atomic<AssetState> state;
      std::atomic<int32_t> refcount;

	  AssetEntry() noexcept
		  : state( AssetState::NotLoaded ), refcount( 0 )
	  {
	  }

	  AssetEntry( AssetDescriptor id, const wchar_t* path, AssetType type ) noexcept
		  : id ( id ), path( path ), type( type ), state( AssetState::NotLoaded), refcount( 0 )
	  {
	  }

	  AssetEntry( const AssetEntry& copyFrom ) noexcept = delete;
	  AssetEntry& operator=( const AssetEntry& copyFrom ) noexcept = delete;

	  AssetEntry( AssetEntry&& moveFrom ) noexcept
	  {
		  assert( moveFrom.state == AssetState::NotLoaded && "Can't move if asset is in use");
		  assert( moveFrom.refcount == 0 && "Can't move if asset is in use" );
		  assert( resource == nullptr && "Can't move if asset has been loaded" );

		  id = moveFrom.id;
		  path = moveFrom.path;
		  type = moveFrom.type;
		  resource = moveFrom.resource;
		  state = AssetState::NotLoaded;
		  refcount = 0;
	  }

	  AssetEntry& operator=( AssetEntry&& moveFrom ) noexcept
	  {
		  if ( &moveFrom != this )
		  {
			  assert( moveFrom.state == AssetState::NotLoaded && "Can't move if asset is in use" );
			  assert( moveFrom.refcount == 0 && "Can't move if asset is in use" );
			  assert( resource == nullptr && "Can't move if asset has been loaded" );

			  id = moveFrom.id;
			  path = moveFrom.path;
			  type = moveFrom.type;
			  resource = moveFrom.resource;
			  state = AssetState::NotLoaded;
			  refcount = 0;
		  }

		  return *this;
	  }
   };

   void DestroyAsset( AssetEntry& asset );


#pragma region AssetRef class

   /**
    *	An AssetRef<T> keeps alive a reference to an underlying asset-type T, while exposing useful helper functions to
    * make it easier to use the asset in DX12.
    */

   template<typename TAssetType>
   class AssetRef
   {
      AssetEntry* ptr;
   public:

      /**
       *	Empty reference
       */ 
      AssetRef() : ptr(nullptr)
      {
      };

      /**
       *	Constructs a new reference to the specified asset
       */
      AssetRef( AssetEntry& asset )
         : ptr( &asset )
      {
         ptr->refcount++;
      }

      /**
       *	 Copy constructor (copies the pointer to the underlying asset and increases the refcount).
       */
      AssetRef( const AssetRef& copyFrom )
         : ptr( copyFrom.ptr )
      {
         ptr->refcount++;
      }

      /**
       *	Move constructor (transfers ownership of the underlying asset to another AssetRef)
       */
      AssetRef( AssetRef&& moveFrom )
         : ptr( nullptr )
      {
         *this = std::move( moveFrom );
      }

      /**
       *	Copy-assignment operator
       */
      AssetRef& operator=( const AssetRef& copyFrom )
      {
         ptr = copyFrom.ptr;
         ptr->refcount++;
      }

      /**
       *	Move assignment operator (transfers ownership of this reference).
       */
      AssetRef& operator=( AssetRef&& moveFrom )
      {
         if ( this != &moveFrom )
         {
            if ( ptr != nullptr )
            {
               Unload();
            }

            ptr = moveFrom.ptr;
            moveFrom.ptr = nullptr;
         }
         return *this;
      }

      /**
       *	Destructor. Releases this reference to the asset.
       */
      ~AssetRef()
      {
         Unload();
      }

      /**
       *	Dereference operator for the asset type.
       */
      TAssetType& operator*()
      {
         assert( ptr != nullptr );
         return *( static_cast<TAssetType*>( ptr->resource ) );
      }

      /**
       *	Smart pointer reference to the asset type.
       */
      TAssetType* operator->()
      {
         assert( ptr != nullptr );
         return static_cast<TAssetType*>( ptr->resource );
      }

      /**
       *	Gets the AssetDescriptor ID for the asset referred to by this reference.
       */
      AssetDescriptor GetID() const noexcept
      {
         assert( ptr != nullptr );
         return ptr->id;
      }

      /**
       *	Gets the current state of the asset. 
       */
      AssetState GetState() const noexcept
      {
         assert( ptr != nullptr );
         return ptr->state.load();
      }

      /**
       *	Returns true if the asset is loaded into memory (and thus ready to use).
       */
      bool CanUse() const
      {
         assert( ptr != nullptr );
         return ptr->state == AssetState::Loaded;
      }

      /**
       *	Gets a DX12 GPU Descriptor Handle for the asset.
       */
      D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const
      {
         assert( ptr != nullptr );
         auto descriptorHeap = GetAssetDescriptorHeap();
         return descriptorHeap->GetGpuHandle( (size_t) ptr->id );
      }

      /**
       *	Gets a DX12 CPU Descriptor Handle for the asset.
       */
      D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const
      {
         assert( ptr != nullptr );
         auto descriptorHeap = GetAssetDescriptorHeap();
         return descriptorHeap->GetCpuHandle( (size_t) ptr->id );
      }

      /**
       *	Releases the reference on the asset. If this is the last reference to the asset, frees up the asset from memory.
       */
      void Unload()
      {
         AssetEntry* entry = ptr;
         if ( entry != nullptr )
         {
            ptr = nullptr;

            if ( --( entry->refcount ) == 0 )
            {
               DestroyAsset( *entry );
            }
         }
      }
   };

#pragma endregion AssetRef class

   /**
    *	Loads all of the assets from disk into memory.
    */
   void LoadAssets( AssetLoadResources& loadRes );

   /**
    *	Releases all of the assets from disk.
    */
   void ReleaseAssets();

   /**
    *	Gets the type of an asset.
    * \param id - the asset descriptor to look up.
    */
   AssetType GetAssetType( AssetDescriptor id );

   /**
    *	Gets a SpriteFont from a descriptor id.
    * \param id - the asset descriptor to look up.
    */
   AssetRef<DirectX::SpriteFont> GetSpriteFontAsset( AssetLoadResources& loadRes, AssetDescriptor id );

   /*
    * Gets a Texture from a descriptor id.
    * \param id - the asset descriptor to look up.
    */
   AssetRef<DX::Texture> GetTextureAsset( AssetLoadResources& loadRes, AssetDescriptor id );

}

