//--------------------------------------------------------------------------------------
// SimpleSFS.h
//
// Sampler Feedback is a Direct3D feature for capturing and recording texture sampling information
// and locations during rendering. When combining sampler feedback with tiled textures, we get an
// easy mechanism to manage tile residency when implementing partially resident textures (PRT) for
// texture streaming. This sample shows a simple implementation of Sampler Feedback for Streaming (SFS),
// showing how to use a MinMip Feedback map and a MinMip map. DirectStorage is used to load and
// hardware decompress tiles of a titled texture directly into graphics memory.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "TiledTexture.h"

// Indicates a texel that wasn't sampled, i.e. no feedback value was written
constexpr uint8_t g_InvalidFeedbackValue = 0xff;

// Data for a tile streaming request
struct StreamingRequest
{
public:
    StreamingRequest()
        : pTile(nullptr)
        , FeedbackValue(g_InvalidFeedbackValue)
        , StreamingMemoryAddress(0) {}

    // The tile to stream
    Tile* pTile;

    // The 5.3 fixed point feedback value for for the requested tile
    // Used to prioritize streaming requests
    BYTE FeedbackValue;

    // Keep track of the memory address where we're streaming to, since we need to issue a flush on it
    D3D12_GPU_VIRTUAL_ADDRESS StreamingMemoryAddress;

    // Compare two streaming requests based on the feedback value during sorting
    static bool Compare(const StreamingRequest& request1, const StreamingRequest& request2)
    {
        return request2.FeedbackValue < request1.FeedbackValue;
    }
};

struct ConstantBuffer
{
    DirectX::XMMATRIX WorldMatrix;
    DirectX::XMMATRIX ViewMatrix;
    DirectX::XMMATRIX ProjectionMatrix;
};
static_assert((sizeof(ConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");

// Helper struct for a staging buffer
struct StagingTexture
{
    StagingTexture();

    Microsoft::WRL::ComPtr<ID3D12Resource>  StagingBuffer;
    void*                                   pLockedBits;
    D3D12_TEXTURE_COPY_LOCATION             CopyLocation;
    D3D12_RANGE                             CopyRange;
    D3D12_RESOURCE_DESC                     Desc;

    void* Map();
    void Unmap();
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void VisualizeMip(UINT mip, LONG x, LONG y, LONG size);
    void RenderUI();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_samplerDescriptorHeap;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteFont>        m_controllerFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                    m_vertexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                     m_indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;
    ConstantBuffer*                             m_pMappedConstantBufferData;
    D3D12_GPU_VIRTUAL_ADDRESS                   m_constantBufferDataGpuAddr;    

    // Camera
    std::unique_ptr<DX::FlyCamera>              m_camera;
    DirectX::XMFLOAT4X4                         m_worldMatrix;
    DirectX::XMFLOAT4X4                         m_viewMatrix;
    DirectX::XMFLOAT4X4                         m_projectionMatrix;

    // Feedback map holding the minimum mip level requested by the paired tiled texture during scene rendering
    UINT                                        m_feedbackMapWidth;
    UINT                                        m_feedbackMapHeight;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_feedbackMapResource;
    StagingTexture                              m_feedbackMapStagingTexture;    // Staging texture to access the feedback map values on the CPU
    BYTE*                                       m_pFeedbackMapData;

    // MinMip map holding the minimum mip level that's currently resident
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_minMipMapResource;
    BYTE*                                       m_pMinMipMapData;
    UINT64                                      m_minMipMapPitch;
    SIZE_T                                      m_minMipMapSizeBytes;

    // Desriptors for m_samplerDescriptorHeap
    enum class SamplerDescriptors
    {
        MinMipMapSampler,       // Scarlett specific HW sampler for sampling the MinMip map
        Count
    };

    // Desriptors for m_resourceDescriptorHeap
    enum class ResourceDescriptors
    {
        TiledTextureSRV,        // SRV for the tiled texture
        MinMipMapSRV,           // SRV for the MinMip map
        FeedbackMapUAV,         // UAV for the feedback map
        DebugTexture,           // SRV for debug texture
        TextFont,
        ControllerFont,
        Count
    };

    // Compressed tiled texture file created using the Xbox Texture Compression Tool (xbtc) in the GDK

#if _GXDK_VER < 0x4A611B35 /* GDK Edition 210600 */
    // "xbtc.exe ToyRobot_BaseColor.dds -o ToyRobot_BaseColor.xbtc -t SFS -e BC1"
    const std::wstring                          m_tiledTextureFileName = L"assets\\ToyRobot_BaseColor.xbtc";
#else
    // "xbtc.exe ToyRobot_BaseColor.dds -o ToyRobot_BaseColor.xbtc --streamsize 64KB -e BC1"
    const std::wstring                          m_tiledTextureFileName = L"assets\\ToyRobot_BaseColor_2106.xbtc";
#endif

    const std::wstring                          m_debugTextureFileName = L"ToyRobot_BaseColor.dds";
    TiledTexture                                m_tiledTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_debugTexture;

    // Memory pool for the tiles of the tiled texture. For this sample we limit the memory pool to only 1.5MB to
    // show how to render a 2.7MB tiled texture
    static constexpr int                        g_NumTilesInMemoryPool = 24;
    Microsoft::WRL::ComPtr<ID3D12Heap>          m_tileMemoryPool;
    std::queue<int>                             m_availableTilesInMemoryPool;       // Manage which slots in the heap is available
    D3D12_GPU_VIRTUAL_ADDRESS                   m_memoryPoolBaseAddress;
    int GetAvailableTileInMemoryPool();

    // DirectStorage
    Microsoft::WRL::ComPtr<IDStorageFactoryX>       m_dsFactory;
    Microsoft::WRL::ComPtr<IDStorageQueueX>         m_dsQueue;
    Microsoft::WRL::ComPtr<IDStorageStatusArrayX>   m_dsStatus;
    Microsoft::WRL::ComPtr<IDStorageFileX>          m_dsFile;

    // Streaming
    const int                           g_MaxNumStreamingRequests = 128;
    std::vector<StreamingRequest>       m_newStreamingRequests;
    std::vector<StreamingRequest>       m_enqueuedStreamingRequests;
    void GenerateStreamingRequest(UINT feedbackMapX, UINT feedbackMapY, BYTE encodedRequestedMip);
    void GenerateNewStreamingRequests();
    void ProcessNewStreamingRequests();
    void ProcessSubmittedStreamingRequests();

    // Age out tiles that haven't been seen for a while
    const double                        g_TileAgeOutInSeconds = 0.1;   // The time to wait until we age out a tile
    void AgeOutTiles();
    void UnmapTile(Tile* pTile, ID3D12CommandQueue* cmdQueue, ID3D12Resource* pTiledTexture);
    void ResetStreaming();

    // Helper function for a stating texture
    void CreateStagingBufferForTexture(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC* pTextureDesc, bool readback,
            Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation, D3D12_RANGE* pEntireRange);
};
