#include "pch.h"
#include "GameSave_Desktop.h"
#include "Assets.h"
#include "Texture.h"
#include "ResourceUploadBatch.h"
#include "DescriptorHeap.h"
#include "SpriteFont.h"
#include "mmintrin.h"

#define GENERATE_ASSET_TABLE 1

// This brings in an array of all of the assets used by the game, with their matching asset type and descriptor index 
// (which should always match their index in this table). Normally you would want an editor to generate these for you, 
// and more control over which assets are loaded when, refcounts, etc. - adding this functionality is left as an 
// exercise for the reader.
#include "SampleSpecificAssets.inl"

#undef GENERATE_ASSET_TABLE

namespace ATG
{

   void LoadAsset( AssetLoadResources& loadRes, AssetDescriptor id );

#pragma region Type-specific Asset Requestors

#define FACTORY_DEFAULT_GET_ASSET_IMPL( assetTypeName, fqAssetTypeName ) \
   AssetRef< fqAssetTypeName > Get##assetTypeName##Asset( AssetLoadResources& loadRes, AssetDescriptor id ) \
   { \
      assert( id != AssetDescriptor::DESCRIPTOR_COUNT && "Can't load the DESCRIPTOR_COUNT value as an asset"); \
      assert( AssetTable[ (size_t) id ].type == AssetType::assetTypeName && "Asset wasn't a " #assetTypeName " but an attempt was made to load it as one" ); \
      LoadAsset( loadRes, id ); \
      return AssetRef< fqAssetTypeName >( AssetTable[ (size_t) id ] ); \
   }

   FACTORY_DEFAULT_GET_ASSET_IMPL( SpriteFont, DirectX::SpriteFont )
   FACTORY_DEFAULT_GET_ASSET_IMPL( Texture, DX::Texture )

#pragma endregion Type-specific Asset Requestors

   DirectX::DescriptorHeap* s_descriptorHeap = nullptr;

   DirectX::DescriptorHeap* GetAssetDescriptorHeap() noexcept
   {
      assert( s_descriptorHeap != nullptr && "Descriptor heap was null");
      return s_descriptorHeap;
   }

   void SetAssetDescriptorHeap(DirectX::DescriptorHeap* heap) noexcept
   {
      s_descriptorHeap = heap;
   }

#pragma region Asset Loading

   // These functions are per-asset type.

   void* LoadSpriteFont( AssetLoadResources& loadRes, AssetDescriptor id, const wchar_t* path )
   {
      DirectX::SpriteFont* font = new DirectX::SpriteFont( loadRes.device, *loadRes.upload, path,
         s_descriptorHeap->GetCpuHandle( (size_t) id ), s_descriptorHeap->GetGpuHandle( (size_t) id ) );

      return font;
   }

   void* LoadTexture( AssetLoadResources& loadRes, AssetDescriptor id, const wchar_t* path )
   {
      DX::Texture* texture = new DX::Texture( loadRes.device, *loadRes.upload,
         s_descriptorHeap->GetCpuHandle( (size_t) id ), path );

      return texture;
   }

   // Type<-->Loader function mapping table.

   using AssetFactoryLoadFn = decltype( &LoadSpriteFont );

   AssetFactoryLoadFn loadAssetFnMap[] =
   {
      &LoadTexture,         // maps to AssetType::Texture
      &LoadSpriteFont,      // maps to AssetType::SpriteFont
   };

   // Loads all assets at once.

   void LoadAssets( AssetLoadResources& loadRes )
   {
      // Note: In your own game you wouldn't load and refcount every object at once in this way except for very simple
      // games. A level or sub-level object would likely own all of the references to the assets and load/refcount them
      // at startup. Streaming games would queue up loads as you enter different areas, and so on, the base refcount being
      // held by the area you're in, but Common assets might be shared.

      for ( auto& asset : AssetTable )
      {
         LoadAsset( loadRes, asset.id );
      }
   }

   // Loads a specific asset.

   void LoadAsset( AssetLoadResources& loadRes, AssetDescriptor id )
   {
      AssetEntry& asset = AssetTable[ (size_t) id ];
      assert( asset.id == id && "Id of item in table and index of item in table must match" );

      int32_t refCount = ++asset.refcount;

      AssetState state = asset.state.load();

      if ( state == AssetState::Releasing )
      {
         // Another thread is in the middle of releasing the asset. Spin until it's finished. You might queue this load 
         // again at the back of your task queue instead.
         while ( asset.state.load() == AssetState::Releasing )
         {
            _mm_pause();
         }
      }

      if ( state == AssetState::NotLoaded )
      {
         // If the refCount is any other value, we're in a race and we lost, so let the winner load the asset.
         if ( refCount == 1 )
         {
            asset.state = AssetState::Loading;
            // In your own game, the factory load functions might add themselves to a queue.
            asset.resource = loadAssetFnMap[ (size_t) asset.type ]( loadRes, asset.id, asset.path );
            asset.state = AssetState::Loaded;
         }
      }
   }

#pragma endregion Asset Loading

#pragma region Asset Destruction

#define FACTORY_DEFAULT_DESTROY_IMPL(name, fqtypename) void Destroy##name( void* resource ) { delete ( fqtypename* )resource; }

   FACTORY_DEFAULT_DESTROY_IMPL( SpriteFont, DirectX::SpriteFont )
   FACTORY_DEFAULT_DESTROY_IMPL( Texture, DX::Texture )

   using AssetFactoryDestroyFn = decltype( &DestroySpriteFont );

   AssetFactoryDestroyFn destroyAssetFnMap[] =
   {
      &DestroyTexture,         // maps to AssetType::Texture
      &DestroySpriteFont,      // maps to AssetType::SpriteFont
   };

   void DestroyAsset( AssetEntry& asset )
   {
      asset.state = AssetState::Releasing;

      // Factory function to release an asset instance.
      destroyAssetFnMap[ (size_t) asset.id ]( asset.resource );
      asset.resource = nullptr;

      asset.state = AssetState::NotLoaded;
   }

   void ReleaseAsset( AssetDescriptor id )
   {
      AssetEntry& asset = AssetTable[ (size_t) id ];
      assert( asset.id == id && "Id of item in table and index of item in table must match" );

      if ( --(asset.refcount) == 0 )
      {
         DestroyAsset( asset );
      }
   }

   void ReleaseAssets()
   {
      for ( auto& asset : AssetTable )
      {
         ReleaseAsset( asset.id );
      }
   }

#pragma endregion Asset Destruction

   ATG::AssetType GetAssetType( AssetDescriptor id )
   {
      return AssetTable[ (size_t) id ].type;
   }

}
