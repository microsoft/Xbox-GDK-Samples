// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <FidelityFX/host/ffx_interface.h>
#include <FidelityFX/host/ffx_util.h>
#include <FidelityFX/host/ffx_assert.h>
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>

#include <ffx_shader_blobs.h>
#include <ffx_breadcrumbs_list.h>

#include <codecvt>  // convert string to wstring
#include <memoryapi.h> // for VirtualAlloc
#include <mutex>

// PIX instrumentation is only enabled if one of the preprocessor symbols USE_PIX, DBG, _DEBUG, PROFILE, or PROFILE_BUILD is defined.
// ref: https://devblogs.microsoft.com/pix/winpixeventruntime/
#if defined(PROFILE) || defined(_DEBUG)
#ifndef USE_PIX
#define USE_PIX  // Allow PIX linking when building for Debug or Profile builds
#endif           // #ifndef USE_PIX
#endif // #if defined(PROFILE) || defined(_DEBUG)

#ifdef __clang__
#pragma clang diagnostic ignored "-Wpointer-bool-conversion"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#include <pix3.h>
#include <xmem.h>

// DX12 prototypes for functions in the backend interface
FfxVersionNumber GetSDKVersionDX12(FfxInterface* backendInterface);
FfxErrorCode GetEffectGpuMemoryUsageDX12(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage);
FfxErrorCode CreateBackendContextDX12(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId);
FfxErrorCode GetDeviceCapabilitiesDX12(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode DestroyBackendContextDX12(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode CreateResourceDX12(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode DestroyResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode MapResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr);
FfxErrorCode UnmapResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode RegisterResourceDX12(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource GetResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode UnregisterResourcesDX12(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode RegisterStaticResourceDX12(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptorDX12(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode StageConstantBufferDataDX12(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer);
FfxErrorCode CreatePipelineDX12(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription*  desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode DestroyPipelineDX12(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode ScheduleGpuJobDX12(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode ExecuteGpuJobsDX12(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode BreadcrumbsAllocBlockDX12(FfxInterface* backendInterface, uint64_t blockBytes, FfxBreadcrumbsBlockData* blockData);
void BreadcrumbsFreeBlockDX12(FfxInterface* backendInterface, FfxBreadcrumbsBlockData* blockData);
void BreadcrumbsWriteDX12(FfxInterface* backendInterface, FfxCommandList commandList, uint32_t value, uint64_t gpuLocation, void* gpuBuffer, bool isBegin);
void BreadcrumbsPrintDeviceInfoDX12(FfxInterface* backendInterface, FfxAllocationCallbacks* allocs, bool extendedInfo, char** printBuffer, size_t* printSize);
void RegisterConstantBufferAllocatorDX12(FfxInterface* backendInterface, FfxConstantBufferAllocator fpConstantAllocator);

#define FFX_MAX_RESOURCE_IDENTIFIER_COUNT (128)
#define FFX_MAX_STATIC_DESCRIPTOR_COUNT   (65536)

// various application callbacks
static FfxConstantBufferAllocator   s_fpConstantAllocator = nullptr;
static FfxResourceAllocatorFunc     s_fpResourceAllocator = nullptr;
static FfxResourceDestroyFunc       s_fpResourceDestroyFunc = nullptr;

typedef struct BackendContext_DX12 {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        wchar_t                 resourceName[64] = {};
#endif
        ID3D12Resource*         resourcePtr;
        FfxResourceDescription  resourceDescription;
        FfxResourceStates       initialState;
        FfxResourceStates       currentState;
        uint32_t                srvDescIndex;
        uint32_t                uavDescIndex;
        uint32_t                uavDescCount;
    } Resource;

    uint32_t refCount;
    uint32_t maxEffectContexts;

    ID3D12Device*           device = nullptr;

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount;

    uint32_t                nextRtvDescriptor;
    ID3D12DescriptorHeap*   descHeapRtvCpu;

    ID3D12DescriptorHeap*   descHeapSrvCpu;
    ID3D12DescriptorHeap*   descHeapUavCpu;
    ID3D12DescriptorHeap*   descHeapUavGpu;

    uint32_t                descRingBufferSize;
    uint32_t                descRingBufferBase;
    ID3D12DescriptorHeap*   descRingBuffer;
    uint32_t                descBindlessBase;

    uint8_t*                pStagingRingBuffer;
    uint32_t                stagingRingBufferBase = 0;

    D3D12_RESOURCE_BARRIER  barriers[FFX_MAX_BARRIERS];
    uint32_t                barrierCount;

    typedef struct alignas(32) EffectContext {

        // Effect identifier -- used for various resource callbacks to application
        FfxEffect           effectId;

        // Resource allocation
        uint32_t            nextStaticResource;
        uint32_t            nextDynamicResource;

        // UAV offsets
        uint32_t            nextStaticUavDescriptor;
        uint32_t            nextDynamicUavDescriptor;

        // Bindless heap
        uint32_t            bindlessTextureSrvHeapStart;
        uint32_t            bindlessTextureSrvHeapSize;
        uint32_t            bindlessBufferSrvHeapStart;
        uint32_t            bindlessBufferSrvHeapSize;
        uint32_t            bindlessTextureUavHeapStart;
        uint32_t            bindlessTextureUavHeapSize;
        uint32_t            bindlessBufferUavHeapStart;
        uint32_t            bindlessBufferUavHeapSize;

        uint32_t bindlessBufferHeapStart;
        uint32_t bindlessBufferHeapEnd;

        // Usage
        bool                active;

        // VRAM usage
        FfxEffectMemoryUsage vramUsage;

    } EffectContext;

    // Resource holder
    Resource*                   pResources;
    EffectContext*              pEffectContexts;

    // Allocation defaults
    FfxConstantAllocation       FallbackConstantAllocator(void* data, FfxUInt64 dataSize);
    void*                       constantBufferMem;
    ID3D12Resource*             constantBufferResource;
    uint32_t                    constantBufferSize;
    uint32_t                    constantBufferOffset;
    std::mutex                  constantBufferMutex;

} BackendContext_DX12;

static uint32_t getFreeBindlessDescriptorBlock(BackendContext_DX12 *context, uint32_t size, uint32_t effectId)
{
    uint32_t base = context->descBindlessBase;

    for (uint32_t i = 0; i < context->maxEffectContexts; ++i) {
        BackendContext_DX12::EffectContext *effectContext = &context->pEffectContexts[i];
        if (i == effectId || !effectContext->active) {
            continue;
        }

        if (!(base >= effectContext->bindlessBufferHeapEnd || base + size <= effectContext->bindlessBufferHeapStart)) {
            base = effectContext->bindlessBufferHeapEnd;
            i = 0;
        }
    }

    FFX_ASSERT(base + size <= context->descBindlessBase + FFX_MAX_STATIC_DESCRIPTOR_COUNT);

    return base;
}

FFX_API size_t ffxGetScratchMemorySizeDX12(size_t maxContexts)
{
    uint32_t resourceArraySize          = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX12::Resource), sizeof(uint64_t));
    uint32_t contextArraySize           = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_DX12::EffectContext), sizeof(uint32_t));
    uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(maxContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
    uint32_t gpuJobDescArraySize        = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));

    return FFX_ALIGN_UP(sizeof(BackendContext_DX12) + resourceArraySize + contextArraySize + stagingRingBufferArraySize + gpuJobDescArraySize, sizeof(uint64_t));
}

// Create a FfxDevice from a ID3D12Device*
FfxDevice ffxGetDeviceDX12(ID3D12Device* dx12Device)
{
    FFX_ASSERT(NULL != dx12Device);
    return reinterpret_cast<FfxDevice>(dx12Device);
}

// populate interface with DX12 pointers.
FfxErrorCode ffxGetInterfaceDX12(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts) {


    FFX_RETURN_ON_ERROR(
        backendInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxGetScratchMemorySizeDX12(maxContexts),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetSDKVersion = GetSDKVersionDX12;
    backendInterface->fpGetEffectGpuMemoryUsage = GetEffectGpuMemoryUsageDX12;
    backendInterface->fpCreateBackendContext = CreateBackendContextDX12;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesDX12;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextDX12;
    backendInterface->fpCreateResource = CreateResourceDX12;
    backendInterface->fpDestroyResource = DestroyResourceDX12;
    backendInterface->fpMapResource = MapResourceDX12;
    backendInterface->fpUnmapResource = UnmapResourceDX12;
    backendInterface->fpGetResource = GetResourceDX12;
    backendInterface->fpRegisterResource = RegisterResourceDX12;
    backendInterface->fpUnregisterResources = UnregisterResourcesDX12;
    backendInterface->fpRegisterStaticResource = RegisterStaticResourceDX12;
    backendInterface->fpGetResourceDescription = GetResourceDescriptorDX12;
    backendInterface->fpStageConstantBufferDataFunc = StageConstantBufferDataDX12;
    backendInterface->fpCreatePipeline = CreatePipelineDX12;
    backendInterface->fpGetPermutationBlobByIndex = ffxGetPermutationBlobByIndex;
    backendInterface->fpDestroyPipeline = DestroyPipelineDX12;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobDX12;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsDX12;
    backendInterface->fpBreadcrumbsAllocBlock = BreadcrumbsAllocBlockDX12;
    backendInterface->fpBreadcrumbsFreeBlock = BreadcrumbsFreeBlockDX12;
    backendInterface->fpBreadcrumbsWrite = BreadcrumbsWriteDX12;
    backendInterface->fpBreadcrumbsPrintDeviceInfo = BreadcrumbsPrintDeviceInfoDX12;
#if defined(_GAMING_XBOX) && (defined(FFX_FI) || defined(FFX_ALL))
    backendInterface->fpSwapChainConfigureFrameGeneration = ffxSetFrameGenerationConfigToSwapchainDX12;
#else
    backendInterface->fpSwapChainConfigureFrameGeneration = 0;
#endif // defined(_GAMING_XBOX)
    backendInterface->fpRegisterConstantBufferAllocator = RegisterConstantBufferAllocatorDX12;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FFX_RETURN_ON_ERROR(
        !backendContext->refCount,
        FFX_ERROR_BACKEND_API_ERROR);

    // Clear everything out
    memset(backendContext, 0, sizeof(*backendContext));

    // Set the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    backendContext->maxEffectContexts = (uint32_t)maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListDX12(ID3D12CommandList* cmdList)
{
    FFX_ASSERT(NULL != cmdList);
    return reinterpret_cast<FfxCommandList>(cmdList);
}

FfxPipeline ffxGetPipelineDX12(ID3D12PipelineState* pipelineState)
{
    FFX_ASSERT(NULL != pipelineState);
    return reinterpret_cast<FfxPipeline>(pipelineState);
}

// register a DX12 resource to the backend
FfxResource ffxGetResourceDX12(const ID3D12Resource* dx12Resource,
    FfxResourceDescription                     ffxResDescription,
    const wchar_t* ffxResName,
    FfxResourceStates                          state /*=FFX_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxResource resource = {};
    resource.resource    = reinterpret_cast<void*>(const_cast<ID3D12Resource*>(dx12Resource));
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName) {
        wcscpy_s(resource.name, ffxResName);
    }
#else
    FFX_UNUSED(ffxResName);
#endif

    return resource;
}

void TIF(HRESULT result)
{
    if (FAILED(result)) {

        wchar_t errorMessage[256];
        memset(errorMessage, 0, 256);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, 255, NULL);
        char errA[256];
        size_t returnSize;
        wcstombs_s(&returnSize, errA, 255, errorMessage, 255);
#ifdef _DEBUG
        OutputDebugStringW(errorMessage);
#endif // _DEBUG
        throw 1;
    }
}

FfxConstantAllocation BackendContext_DX12::FallbackConstantAllocator(void* data, FfxUInt64 dataSize)
{
    FfxConstantAllocation allocation;
    std::lock_guard<std::mutex> cbLock{ constantBufferMutex };

    if (!constantBufferMem)
    {
        // create dynamic ring buffer for constant uploads
        constantBufferSize = FFX_ALIGN_UP(FFX_BUFFER_SIZE, 256) * maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES; // Size aligned to 256

        CD3DX12_RESOURCE_DESC constDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
        TIF(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
            &constDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_GDK_PPV_ARGS(&constantBufferResource)));
        constantBufferResource->SetName(L"FFX_DX12_DynamicRingBuffer");

        // map it
        TIF(constantBufferResource->Map(0, nullptr,
            (void**)&constantBufferMem));
        constantBufferOffset = 0;
    }

    FFX_ASSERT(constantBufferMem);

    uint32_t size = FFX_ALIGN_UP(dataSize, 256);

    // wrap as needed 
    if (constantBufferOffset + size >= constantBufferSize)
        constantBufferOffset = 0;

    void* pBuffer = (void*)((uint8_t*)(constantBufferMem)+constantBufferOffset);
    memcpy(pBuffer, data, (size_t)dataSize);

    D3D12_GPU_VIRTUAL_ADDRESS bufferViewDesc = constantBufferResource->GetGPUVirtualAddress() + constantBufferOffset;

    // update the offset
    constantBufferOffset += size;

    allocation.resource = FfxResource(); // Not needed for directx
    allocation.handle = FfxUInt64(bufferViewDesc);

    return allocation;
}

// fix up format in case resource passed for UAV cannot be mapped
static DXGI_FORMAT convertFormatUav(DXGI_FORMAT format)
{
    switch (format) 
    {
        // Handle Depth
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        // Handle color: assume FLOAT for 16 and 32 bit channels, else UNORM
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:
            return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:
            return DXGI_FORMAT_R8_UNORM;
        default:
            return format;
    }
}

// fix up format in case resource passed for SRV cannot be mapped
static DXGI_FORMAT convertFormatSrv(DXGI_FORMAT format)
{
    switch (format) 
    {
        // Handle Depth
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        // Handle color: assume FLOAT for 16 and 32 bit channels, else UNORM
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:
            return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:
            return DXGI_FORMAT_R8_UNORM;
        // Others can map as is
        default:
            return format;
    }
}

D3D12_RESOURCE_STATES ffxGetDX12StateFromResourceState(FfxResourceStates state)
{
    switch (state) 
    {

        case FFX_RESOURCE_STATE_GENERIC_READ:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case FFX_RESOURCE_STATE_UNORDERED_ACCESS:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case FFX_RESOURCE_STATE_COMPUTE_READ:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case FFX_RESOURCE_STATE_PIXEL_READ:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case FFX_RESOURCE_STATE_COPY_SRC:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case FFX_RESOURCE_STATE_COPY_DEST:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case FFX_RESOURCE_STATE_PRESENT:
            return D3D12_RESOURCE_STATE_PRESENT;
        case FFX_RESOURCE_STATE_COMMON:
            return D3D12_RESOURCE_STATE_COMMON;
        case FFX_RESOURCE_STATE_RENDER_TARGET:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        default:
            FFX_ASSERT_MESSAGE(false, "Resource state not yet supported");
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

DXGI_FORMAT ffxGetDX12FormatFromSurfaceFormat(FfxSurfaceFormat surfaceFormat)
{
    switch (surfaceFormat) 
    {

        case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case (FFX_SURFACE_FORMAT_R32G32B32A32_UINT):
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16B16A16_TYPELESS):
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case (FFX_SURFACE_FORMAT_R32G32B32_FLOAT):
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case (FFX_SURFACE_FORMAT_R32G32_TYPELESS):
            return DXGI_FORMAT_R32G32_TYPELESS;
        case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case (FFX_SURFACE_FORMAT_R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case(FFX_SURFACE_FORMAT_R10G10B10A2_TYPELESS):
            return DXGI_FORMAT_R10G10B10A2_TYPELESS;
        case(FFX_SURFACE_FORMAT_R10G10B10A2_UNORM):
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case (FFX_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case (FFX_SURFACE_FORMAT_B8G8R8A8_UNORM):
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case (FFX_SURFACE_FORMAT_B8G8R8A8_SRGB):
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_TYPELESS):
            return DXGI_FORMAT_R16_TYPELESS;
        case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case (FFX_SURFACE_FORMAT_R16G16_SINT):
            return DXGI_FORMAT_R16G16_SINT;
        case (FFX_SURFACE_FORMAT_R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case (FFX_SURFACE_FORMAT_R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case (FFX_SURFACE_FORMAT_R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case (FFX_SURFACE_FORMAT_R8_TYPELESS):
            return DXGI_FORMAT_R8_TYPELESS;
        case (FFX_SURFACE_FORMAT_R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case (FFX_SURFACE_FORMAT_R8_UINT):
            return DXGI_FORMAT_R8_UINT;
        case (FFX_SURFACE_FORMAT_R8G8_UINT):
            return DXGI_FORMAT_R8G8_UINT;
        case (FFX_SURFACE_FORMAT_R8G8_TYPELESS):
            return DXGI_FORMAT_R8G8_TYPELESS;
        case (FFX_SURFACE_FORMAT_R8G8_UNORM):
            return DXGI_FORMAT_R8G8_UNORM;
        case (FFX_SURFACE_FORMAT_R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
#if defined(_GAMING_XBOX)
		case (FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
			return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
#endif // #if defined(_GAMING_XBOX)
        case (FFX_SURFACE_FORMAT_UNKNOWN):
            return DXGI_FORMAT_UNKNOWN;

        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_RESOURCE_FLAGS ffxGetDX12ResourceFlags(FfxResourceUsage flags)
{
    D3D12_RESOURCE_FLAGS dx12ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET) dx12ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (flags & FFX_RESOURCE_USAGE_UAV) dx12ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return dx12ResourceFlags;
}

FfxSurfaceFormat ffxGetSurfaceFormatDX12(DXGI_FORMAT format)
{
    switch (format) 
    {

        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return FFX_SURFACE_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;

        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
            return FFX_SURFACE_FORMAT_R32G32_FLOAT;

        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return FFX_SURFACE_FORMAT_R32_FLOAT;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return FFX_SURFACE_FORMAT_R32_UINT;

        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return FFX_SURFACE_FORMAT_R8_UINT;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
        
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return FFX_SURFACE_FORMAT_R8G8B8A8_SNORM;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return FFX_SURFACE_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return FFX_SURFACE_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return FFX_SURFACE_FORMAT_B8G8R8A8_SRGB;

        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
            return FFX_SURFACE_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R16G16_UINT:
            return FFX_SURFACE_FORMAT_R16G16_UINT;
        case DXGI_FORMAT_R16G16_SINT:
            return FFX_SURFACE_FORMAT_R16G16_SINT;
        case DXGI_FORMAT_R32_UINT:
            return FFX_SURFACE_FORMAT_R32_UINT;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return FFX_SURFACE_FORMAT_R32_FLOAT;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UINT:
            return FFX_SURFACE_FORMAT_R8G8_UINT;
        case DXGI_FORMAT_R8G8_UNORM:
            return FFX_SURFACE_FORMAT_R8G8_UNORM;

        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
            return FFX_SURFACE_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R16_UINT:
            return FFX_SURFACE_FORMAT_R16_UINT;
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return FFX_SURFACE_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R16_SNORM:
            return FFX_SURFACE_FORMAT_R16_SNORM;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_A8_UNORM:
            return FFX_SURFACE_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R8_UINT:
            return FFX_SURFACE_FORMAT_R8_UINT;

#if defined(_GAMING_XBOX)
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
#endif // #if defined(_GAMING_XBOX)

        case DXGI_FORMAT_UNKNOWN:
            return FFX_SURFACE_FORMAT_UNKNOWN;
        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

bool IsDepthDX12(DXGI_FORMAT format)
{
    return (format == DXGI_FORMAT_D16_UNORM) || 
           (format == DXGI_FORMAT_D32_FLOAT) || 
           (format == DXGI_FORMAT_D24_UNORM_S8_UINT) ||
           (format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
}

FfxResourceDescription ffxGetResourceDescriptionDX12(const ID3D12Resource* pResource, FfxResourceUsage additionalUsages /*=FFX_RESOURCE_USAGE_READ_ONLY*/)
{
    FfxResourceDescription resourceDescription = {};

    // This is valid
    if (!pResource)
        return resourceDescription;

    if (pResource)
    {
        D3D12_RESOURCE_DESC desc = const_cast<ID3D12Resource*>(pResource)->GetDesc();
        
        if( desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            resourceDescription.flags  = FFX_RESOURCE_FLAGS_NONE;
            resourceDescription.usage  = FFX_RESOURCE_USAGE_UAV;
            resourceDescription.size  = (uint32_t)desc.Width;
            resourceDescription.stride = (uint32_t)desc.Height;
            resourceDescription.format = ffxGetSurfaceFormatDX12(desc.Format);

            // What should we initialize this to?? No case for this yet
            resourceDescription.depth    = 0;
            resourceDescription.mipCount = 0;

            // Set the type
            resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
        }
        else
        {
            // Set flags properly for resource registration
            resourceDescription.flags     = FFX_RESOURCE_FLAGS_NONE;
           
            // Check for depth use
            resourceDescription.usage     = IsDepthDX12(desc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
            
            // Unordered access use
            if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
                resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

            // Resource-specific supplemental use flags
            resourceDescription.usage    = (FfxResourceUsage)(resourceDescription.usage | additionalUsages);

            resourceDescription.width    = (uint32_t)desc.Width;
            resourceDescription.height   = (uint32_t)desc.Height;
            resourceDescription.depth    = desc.DepthOrArraySize;
            resourceDescription.mipCount = desc.MipLevels;
            resourceDescription.format   = ffxGetSurfaceFormatDX12(desc.Format);

            switch (desc.Dimension)
            {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                if (FFX_CONTAINS_FLAG(additionalUsages, FFX_RESOURCE_USAGE_ARRAYVIEW))
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                else if (desc.DepthOrArraySize == 1)
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                else if (desc.DepthOrArraySize == 6)
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE_CUBE;
                else
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
                break;
            default:
                FFX_ASSERT_MESSAGE(false, "FFXInterface: GDK: Unsupported texture dimension requested. Please implement.");
                break;
            }
        }
    }

    return resourceDescription;
}

ID3D12Resource* getDX12ResourcePtr(BackendContext_DX12* backendContext, int32_t resourceIndex)
{
    FFX_ASSERT(NULL != backendContext);
    return reinterpret_cast<ID3D12Resource*>(backendContext->pResources[resourceIndex].resourcePtr);
}

void beginMarkerDX12(BackendContext_DX12* backendContext, ID3D12GraphicsCommandList* pCmdList, const wchar_t* label)
{
    FFX_ASSERT(nullptr != backendContext);
    FFX_ASSERT(nullptr != pCmdList);

    PIXBeginEvent(pCmdList, PIX_COLOR_DEFAULT, label);
}

void endMarkerDX12(BackendContext_DX12* backendContext, ID3D12GraphicsCommandList* pCmdList)
{
    FFX_ASSERT(nullptr != backendContext);
    FFX_ASSERT(nullptr != pCmdList);

    PIXEndEvent(pCmdList);
}

void addBarrier(BackendContext_DX12* backendContext, FfxResourceInternal* resource, FfxResourceStates newState)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != resource);

    ID3D12Resource* dx12Resource = getDX12ResourcePtr(backendContext, resource->internalIndex);
    D3D12_RESOURCE_BARRIER* barrier = &backendContext->barriers[backendContext->barrierCount];

    FFX_ASSERT(backendContext->barrierCount < FFX_MAX_BARRIERS);

    FfxResourceStates* currentState = &backendContext->pResources[resource->internalIndex].currentState;

    if ((*currentState & newState) != newState) {

        *barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            dx12Resource,
            ffxGetDX12StateFromResourceState(*currentState),
            ffxGetDX12StateFromResourceState(newState));

        *currentState = newState;
        ++backendContext->barrierCount;

    }
    else if (newState == FFX_RESOURCE_STATE_UNORDERED_ACCESS) {

        *barrier = CD3DX12_RESOURCE_BARRIER::UAV(dx12Resource);
        ++backendContext->barrierCount;
    }
}

void flushBarriers(BackendContext_DX12* backendContext, ID3D12GraphicsCommandList* dx12CommandList)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != dx12CommandList);

    if (backendContext->barrierCount > 0) {

        dx12CommandList->ResourceBarrier(backendContext->barrierCount, backendContext->barriers);
        backendContext->barrierCount = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// DX12 back end implementation

FfxVersionNumber GetSDKVersionDX12(FfxInterface*)
{
    return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

uint64_t GetCurrentGpuMemoryUsageDX12(FfxInterface*)
{
    uint64_t      memoryUsage = 0;
    XMEM_WORKING_SET_STATISTICS memStats;
    if (XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TITLE, &memStats))
    {
        memoryUsage = memStats.GameUsed;
    }

    return memoryUsage;
}

FfxErrorCode GetEffectGpuMemoryUsageDX12(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != outVramUsage);

    BackendContext_DX12*                backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    BackendContext_DX12::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    *outVramUsage = effectContext.vramUsage;

    return FFX_OK;
}

// initialize the DX12 backend
FfxErrorCode CreateBackendContextDX12(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != backendInterface->device);

    HRESULT result = S_OK;
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendInterface->device);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!backendContext->refCount) {

        new (&backendContext->constantBufferMutex) std::mutex();

        if (dx12Device != NULL) {

            dx12Device->AddRef();
            backendContext->device = dx12Device;
        }

        // Map all of our pointers
        uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX12::Resource), sizeof(uint64_t));
        uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * sizeof(BackendContext_DX12::EffectContext), sizeof(uint32_t));

        uint8_t* pMem = (uint8_t*)((BackendContext_DX12*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resources
        backendContext->pResources = (BackendContext_DX12::Resource*)(pMem);
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Map the staging buffer
        backendContext->pStagingRingBuffer = (uint8_t*)(pMem);
        memset(backendContext->pStagingRingBuffer, 0, stagingRingBufferArraySize);
        pMem += stagingRingBufferArraySize;

        // Map the effect contexts
        backendContext->pEffectContexts = reinterpret_cast<BackendContext_DX12::EffectContext*>(pMem);
        memset(backendContext->pEffectContexts, 0, contextArraySize);

        // CPUVisible
        D3D12_DESCRIPTOR_HEAP_DESC descHeap;
        descHeap.NumDescriptors = FFX_MAX_RESOURCE_COUNT * backendContext->maxEffectContexts;
        descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        descHeap.NodeMask = 0;

        result = dx12Device->CreateDescriptorHeap(&descHeap, IID_GDK_PPV_ARGS(&backendContext->descHeapSrvCpu));
        FFX_ASSERT(result == S_OK);
        result = dx12Device->CreateDescriptorHeap(&descHeap, IID_GDK_PPV_ARGS(&backendContext->descHeapUavCpu));
        FFX_ASSERT(result == S_OK);

        // GPU
        descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        result = dx12Device->CreateDescriptorHeap(&descHeap, IID_GDK_PPV_ARGS(&backendContext->descHeapUavGpu));
        FFX_ASSERT(result == S_OK);

        // descriptor ring buffer
        descHeap.NumDescriptors            = FFX_RING_BUFFER_DESCRIPTOR_COUNT * backendContext->maxEffectContexts + FFX_MAX_STATIC_DESCRIPTOR_COUNT;
        backendContext->descRingBufferSize = descHeap.NumDescriptors;
        backendContext->descRingBufferBase = 0;
        result = dx12Device->CreateDescriptorHeap(&descHeap, IID_GDK_PPV_ARGS(&backendContext->descRingBuffer));
        FFX_ASSERT(result == S_OK);

        backendContext->descBindlessBase   = FFX_RING_BUFFER_DESCRIPTOR_COUNT * backendContext->maxEffectContexts;

	    // RTV descriptor heap to raster jobs
	    descHeap.NumDescriptors = 8;
	    descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	    descHeap.NodeMask = 0;
	    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        result = dx12Device->CreateDescriptorHeap(&descHeap, IID_GDK_PPV_ARGS(&backendContext->descHeapRtvCpu));
        FFX_ASSERT(result == S_OK);

        // initialize the bindless offset to *after* the ring-buffer 
        backendContext->descBindlessBase = FFX_RING_BUFFER_DESCRIPTOR_COUNT * backendContext->maxEffectContexts;
    }

    // Increment the ref count
    ++backendContext->refCount;

    // Get an available context id
    for (uint32_t i = 0; i < backendContext->maxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.effectId = effect;

            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT) + 1;
            effectContext.nextDynamicResource = (i * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
            effectContext.nextStaticUavDescriptor = (i * FFX_MAX_RESOURCE_COUNT);
            effectContext.nextDynamicUavDescriptor = (i * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;

            if (bindlessConfig)
            {
                uint32_t numDescriptors = bindlessConfig->maxTextureSrvs + bindlessConfig->maxBufferSrvs + bindlessConfig->maxTextureUavs + bindlessConfig->maxBufferUavs;

                uint32_t bindlessBase = getFreeBindlessDescriptorBlock(backendContext, numDescriptors, i);

                effectContext.bindlessBufferHeapStart = bindlessBase;
                effectContext.bindlessBufferHeapEnd = bindlessBase + numDescriptors;

                effectContext.bindlessTextureSrvHeapStart = bindlessBase;
                effectContext.bindlessTextureSrvHeapSize  = bindlessConfig->maxTextureSrvs;

                bindlessBase += bindlessConfig->maxTextureSrvs;

                effectContext.bindlessBufferSrvHeapStart = bindlessBase;
                effectContext.bindlessBufferSrvHeapSize  = bindlessConfig->maxBufferSrvs;

                bindlessBase += bindlessConfig->maxBufferSrvs;

                effectContext.bindlessTextureUavHeapStart = bindlessBase;
                effectContext.bindlessTextureUavHeapSize  = bindlessConfig->maxTextureUavs;

                bindlessBase += bindlessConfig->maxTextureUavs;

                effectContext.bindlessBufferUavHeapStart = bindlessBase;
                effectContext.bindlessBufferUavHeapSize  = bindlessConfig->maxBufferUavs;

                bindlessBase += bindlessConfig->maxBufferUavs;
            }
            else
            {
                effectContext.bindlessTextureSrvHeapStart = 0;
                effectContext.bindlessTextureSrvHeapSize  = 0;
                effectContext.bindlessBufferSrvHeapStart  = 0;
                effectContext.bindlessBufferSrvHeapSize   = 0;
                effectContext.bindlessTextureUavHeapStart = 0;
                effectContext.bindlessTextureUavHeapSize  = 0;
                effectContext.bindlessBufferUavHeapStart  = 0;
                effectContext.bindlessBufferUavHeapSize   = 0;
                effectContext.bindlessBufferHeapStart     = 0;
                effectContext.bindlessBufferHeapEnd       = 0;
            }   

            break;
        }
    }

    return FFX_OK;
}

// query device capabilities to select the optimal shader permutation
FfxErrorCode GetDeviceCapabilitiesDX12(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != backendInterface->device);
    FFX_ASSERT(NULL != deviceCapabilities);
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendInterface->device);

    // Check if we have shader model 6.6
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)))) {

        switch (shaderModel.HighestShaderModel) {

        case D3D_SHADER_MODEL_5_1:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
            break;

        case D3D_SHADER_MODEL_6_0:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
            break;

        case D3D_SHADER_MODEL_6_1:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_1;
            break;

        case D3D_SHADER_MODEL_6_2:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_2;
            break;

        case D3D_SHADER_MODEL_6_3:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_3;
            break;

        case D3D_SHADER_MODEL_6_4:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_4;
            break;

        case D3D_SHADER_MODEL_6_5:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_5;
            break;

        case D3D_SHADER_MODEL_6_6:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
            break;

        default:
            deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
            break;
        }
    }
    else {

        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
    }

    // check if we can force wave64 mode.
    D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12Options1 = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12Options1, sizeof(d3d12Options1)))) {

        const uint32_t waveLaneCountMin = d3d12Options1.WaveLaneCountMin;
        const uint32_t waveLaneCountMax = d3d12Options1.WaveLaneCountMax;
        deviceCapabilities->waveLaneCountMin = waveLaneCountMin;
        deviceCapabilities->waveLaneCountMax = waveLaneCountMax;
    }

    // check if we have 16bit floating point.
    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12Options = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12Options, sizeof(d3d12Options)))) {

        deviceCapabilities->fp16Supported = bool(d3d12Options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12Options4 = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12Options4, sizeof(d3d12Options4)))) {

        deviceCapabilities->fp16Supported &= bool(d3d12Options4.Native16BitShaderOpsSupported);
    }

    // check if we have raytracing support
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12Options5 = {};
    if (SUCCEEDED(dx12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12Options5, sizeof(d3d12Options5)))) {

        deviceCapabilities->raytracingSupported = (d3d12Options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
    }

    deviceCapabilities->deviceCoherentMemorySupported = false;
    deviceCapabilities->dedicatedAllocationSupported = true; // committed resources are always available
    deviceCapabilities->bufferMarkerSupported = false;
    deviceCapabilities->extendedSynchronizationSupported = false;
    deviceCapabilities->shaderStorageBufferArrayNonUniformIndexing = true;

    return FFX_OK;
}

// deinitialize the DX12 backend
FfxErrorCode DestroyBackendContextDX12(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    FFX_ASSERT(backendContext->refCount > 0);

    // Delete any resources allocated by this context
    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < (uint32_t)effectContext.nextStaticResource; ++currentStaticResourceIndex) {
        if (backendContext->pResources[currentStaticResourceIndex].resourcePtr) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: DX12: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { (int32_t)currentStaticResourceIndex };
            DestroyResourceDX12(backendInterface, internalResource, effectContextId);
        }
    }

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --backendContext->refCount;

    if (!backendContext->refCount) {

        // release constant buffer pool if it was allocated
        if (backendContext->constantBufferMem)
        {
            backendContext->constantBufferResource->Unmap(0, nullptr);
            backendContext->constantBufferResource->Release();
            backendContext->constantBufferMem = nullptr;
            backendContext->constantBufferOffset = 0;
            backendContext->constantBufferSize = 0;
        }

        backendContext->gpuJobCount             = 0;
        backendContext->barrierCount            = 0;

        // release heaps
	backendContext->descHeapRtvCpu->Release();
        backendContext->descHeapSrvCpu->Release();
        backendContext->descHeapUavCpu->Release();
        backendContext->descHeapUavGpu->Release();
        backendContext->descRingBuffer->Release();

        if (backendContext->device != NULL) {
            backendContext->device->Release();
            backendContext->device = NULL;
        }
    }

    return FFX_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxErrorCode CreateResourceDX12(
    FfxInterface* backendInterface,
    const FfxCreateResourceDescription* createResourceDescription,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outTexture
)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != createResourceDescription);
    FFX_ASSERT(NULL != outTexture);
    FFX_ASSERT_MESSAGE(createResourceDescription->initData.type != FFX_RESOURCE_INIT_DATA_TYPE_INVALID,
                       "InitData type cannot be FFX_RESOURCE_INIT_DATA_TYPE_INVALID. Please explicitly specify the resource initialization type.");


    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    ID3D12Device* dx12Device = backendContext->device;

    uint64_t vramBefore = GetCurrentGpuMemoryUsageDX12(backendInterface);

    FFX_ASSERT(NULL != dx12Device);

    D3D12_HEAP_PROPERTIES dx12HeapProperties = {};

    switch (createResourceDescription->heapType)
    {
    case FFX_HEAP_TYPE_DEFAULT:
        dx12HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        break;
    case FFX_HEAP_TYPE_UPLOAD:
        dx12HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        break;
    case FFX_HEAP_TYPE_READBACK:
        dx12HeapProperties.Type = D3D12_HEAP_TYPE_READBACK;
        break;
    default:
        dx12HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        break;
    }

    FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);

    outTexture->internalIndex = effectContext.nextStaticResource++;
    BackendContext_DX12::Resource* backendResource = &backendContext->pResources[outTexture->internalIndex];
    backendResource->resourceDescription = createResourceDescription->resourceDescription;

    const auto& initData = createResourceDescription->initData;

    D3D12_RESOURCE_DESC dx12ResourceDescription = {};
    dx12ResourceDescription.Format              = DXGI_FORMAT_UNKNOWN;
    dx12ResourceDescription.Width               = 1;
    dx12ResourceDescription.Height              = 1;
    dx12ResourceDescription.MipLevels           = 1;
    dx12ResourceDescription.DepthOrArraySize    = 1;
    dx12ResourceDescription.SampleDesc.Count    = 1;
    dx12ResourceDescription.Flags               = ffxGetDX12ResourceFlags(backendResource->resourceDescription.usage);

    switch (createResourceDescription->resourceDescription.type) {

    case FFX_RESOURCE_TYPE_BUFFER:
        dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
        dx12ResourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        break;

    case FFX_RESOURCE_TYPE_TEXTURE1D:
        dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
        dx12ResourceDescription.DepthOrArraySize = static_cast<uint16_t>(createResourceDescription->resourceDescription.depth);
        dx12ResourceDescription.MipLevels = static_cast<uint16_t>(createResourceDescription->resourceDescription.mipCount);
        break;

    case FFX_RESOURCE_TYPE_TEXTURE2D:
        dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
        dx12ResourceDescription.Height = createResourceDescription->resourceDescription.height;
        dx12ResourceDescription.DepthOrArraySize = static_cast<uint16_t>(createResourceDescription->resourceDescription.depth);
        dx12ResourceDescription.MipLevels = static_cast<uint16_t>(createResourceDescription->resourceDescription.mipCount);
        break;

    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
        dx12ResourceDescription.Format = ffxGetDX12FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx12ResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        dx12ResourceDescription.Width = createResourceDescription->resourceDescription.width;
        dx12ResourceDescription.Height = createResourceDescription->resourceDescription.height;
        dx12ResourceDescription.DepthOrArraySize = static_cast<uint16_t>(createResourceDescription->resourceDescription.depth);
        dx12ResourceDescription.MipLevels = static_cast<uint16_t>(createResourceDescription->resourceDescription.mipCount);
        break;

    default:
        break;
    }

    ID3D12Resource* dx12Resource = nullptr;
    if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD) {

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT dx12Footprint = {};

        UINT rowCount;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;

        dx12Device->GetCopyableFootprints(&dx12ResourceDescription, 0, 1, 0, &dx12Footprint, &rowCount, &rowSizeInBytes, &totalBytes);

        D3D12_HEAP_PROPERTIES dx12UploadHeapProperties = {};
        dx12UploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC dx12UploadBufferDescription = {};

        dx12UploadBufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dx12UploadBufferDescription.Width = totalBytes;
        dx12UploadBufferDescription.Height = 1;
        dx12UploadBufferDescription.DepthOrArraySize = 1;
        dx12UploadBufferDescription.MipLevels = 1;
        dx12UploadBufferDescription.Format = DXGI_FORMAT_UNKNOWN;
        dx12UploadBufferDescription.SampleDesc.Count = 1;
        dx12UploadBufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (s_fpResourceAllocator)
        {
            FfxErrorCode error = s_fpResourceAllocator(effectContext.effectId, D3D12_RESOURCE_STATE_GENERIC_READ, &dx12HeapProperties,
                                                        &dx12UploadBufferDescription, &createResourceDescription->resourceDescription, 
                                                        nullptr, &dx12Resource);
            FFX_ASSERT_MESSAGE(error == FFX_OK, "Resource allocation from application failed!");
        }
        else
        {
            TIF(dx12Device->CreateCommittedResource(&dx12HeapProperties,
                                            D3D12_HEAP_FLAG_NONE,
                                            &dx12UploadBufferDescription,
                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                            nullptr,
                                            IID_GDK_PPV_ARGS(&dx12Resource)));
        }
        backendResource->initialState = FFX_RESOURCE_STATE_GENERIC_READ;
        backendResource->currentState = FFX_RESOURCE_STATE_GENERIC_READ;

        D3D12_RANGE dx12EmptyRange = {};
        void* uploadBufferData = nullptr;
        TIF(dx12Resource->Map(0, &dx12EmptyRange, &uploadBufferData));

        const uint8_t* src = static_cast<uint8_t*>(initData.buffer);
        uint8_t* dst = static_cast<uint8_t*>(uploadBufferData);
        for (uint32_t currentRowIndex = 0; currentRowIndex < createResourceDescription->resourceDescription.height; ++currentRowIndex) {

            if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_BUFFER)
            {
                memcpy(dst, src, (size_t)rowSizeInBytes);
                src += rowSizeInBytes;
            }
            else if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_VALUE)
            {
                memset(dst, initData.value, (size_t)rowSizeInBytes);
            }
            dst += dx12Footprint.Footprint.RowPitch;
        }

        dx12Resource->Unmap(0, nullptr);
        dx12Resource->SetName(createResourceDescription->name);
        backendResource->resourcePtr = dx12Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif
        return FFX_OK;

    }
    else {

        const FfxResourceStates resourceStates =
            ((initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED) && (createResourceDescription->heapType != FFX_HEAP_TYPE_UPLOAD))
                ? FFX_RESOURCE_STATE_COPY_DEST
                : createResourceDescription->initialState;
        // Buffers ignore any input state and create in common (but issue a warning)
        const D3D12_RESOURCE_STATES dx12ResourceStates = ffxGetDX12StateFromResourceState(resourceStates);

        if (s_fpResourceAllocator)
        {
            FfxErrorCode error = s_fpResourceAllocator(effectContext.effectId, dx12ResourceStates, &dx12HeapProperties,
                                                        &dx12ResourceDescription, &createResourceDescription->resourceDescription,
                                                        nullptr, &dx12Resource);
            FFX_ASSERT_MESSAGE(error == FFX_OK, "Resource allocation from application failed!");
        }
        else
        {
#ifdef _GAMING_XBOX_SCARLETT
            D3D12XBOX_RESOURCE_DESC dx12ResourceDescriptionX = {};
            dx12ResourceDescriptionX.Dimension = dx12ResourceDescription.Dimension;
            dx12ResourceDescriptionX.Alignment = dx12ResourceDescription.Alignment;
            dx12ResourceDescriptionX.Width = dx12ResourceDescription.Width;
            dx12ResourceDescriptionX.Height = dx12ResourceDescription.Height;
            dx12ResourceDescriptionX.DepthOrArraySize = dx12ResourceDescription.DepthOrArraySize;
            dx12ResourceDescriptionX.MipLevels = dx12ResourceDescription.MipLevels;
            dx12ResourceDescriptionX.Format = dx12ResourceDescription.Format;
            dx12ResourceDescriptionX.SampleDesc.Count = dx12ResourceDescription.SampleDesc.Count;
            dx12ResourceDescriptionX.SampleDesc.Quality = dx12ResourceDescription.SampleDesc.Quality;
            dx12ResourceDescriptionX.Layout = dx12ResourceDescription.Layout;
            dx12ResourceDescriptionX.Flags = dx12ResourceDescription.Flags;
            dx12ResourceDescriptionX.DccOptions.CompressionLevel = D3D12XBOX_DCC_COMPRESSION_LEVEL_DEFAULT;
            dx12ResourceDescriptionX.DccOptions.ColorTransform = D3D12XBOX_DCC_COLOR_TRANSFORM_AUTO_TEXTURE_COMPATIBLE;

            if (backendResource->resourceDescription.usage & FFX_RESOURCE_USAGE_DCC_RENDERTARGET)
            {
                /**
                 *  We enforce RENDER_TARGET flag on select resources only to set swizzling mode to what RENDER_TARGET textures use
                 *  for more optimal access
                 *
                 *  Everything else is DCC-related and we want DCC to be enabled to save on memory traffic.
                 */
                dx12ResourceDescriptionX.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                                                | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                                                | D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC
                                                | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;

                dx12ResourceDescriptionX.DccOptions.CompressionLevel = D3D12XBOX_DCC_COMPRESSION_LEVEL_128B_TEXTURE_COMPATIBLE;
            }

            TIF(dx12Device->CreateCommittedResourceX(&dx12HeapProperties, D3D12_HEAP_FLAG_NONE, &dx12ResourceDescriptionX, dx12ResourceStates, nullptr, IID_GDK_PPV_ARGS(&dx12Resource)));
#else
            TIF(dx12Device->CreateCommittedResource(&dx12HeapProperties, D3D12_HEAP_FLAG_NONE, &dx12ResourceDescription, dx12ResourceStates, nullptr, IID_GDK_PPV_ARGS(&dx12Resource)));
#endif // _GAMING_XBOX_SCARLETT
        }

        backendResource->initialState = resourceStates;
        backendResource->currentState = resourceStates;

        dx12Resource->SetName(createResourceDescription->name);
        backendResource->resourcePtr = dx12Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif

        // Create SRVs and UAVs
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};
            D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
            D3D12_RESOURCE_DESC dx12Desc = dx12Resource->GetDesc();
            dx12UavDescription.Format = convertFormatUav(dx12Desc.Format);
            dx12SrvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            dx12SrvDescription.Format = convertFormatSrv(dx12Desc.Format);

            bool requestArrayView = FFX_CONTAINS_FLAG(createResourceDescription->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

            switch (dx12Desc.Dimension) {

            case D3D12_RESOURCE_DIMENSION_BUFFER:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                if (dx12Desc.DepthOrArraySize > 1 || requestArrayView)
                {
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    dx12UavDescription.Texture1DArray.ArraySize = dx12Desc.DepthOrArraySize;
                    dx12UavDescription.Texture1DArray.FirstArraySlice = 0;
                    dx12UavDescription.Texture1DArray.MipSlice = 0;

                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    dx12SrvDescription.Texture1DArray.ArraySize = dx12Desc.DepthOrArraySize;
                    dx12SrvDescription.Texture1DArray.FirstArraySlice = 0;
                    dx12SrvDescription.Texture1DArray.MipLevels = dx12Desc.MipLevels;
                    dx12SrvDescription.Texture1DArray.MostDetailedMip = 0;
                }
                else
                {
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                    dx12UavDescription.Texture1D.MipSlice = 0;

                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                    dx12SrvDescription.Texture1D.MipLevels = dx12Desc.MipLevels;
                    dx12SrvDescription.Texture1D.MostDetailedMip = 0;
                }
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            {
                if (dx12Desc.DepthOrArraySize > 1 || requestArrayView)
                {
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    dx12UavDescription.Texture2DArray.ArraySize = dx12Desc.DepthOrArraySize;
                    dx12UavDescription.Texture2DArray.FirstArraySlice = 0;
                    dx12UavDescription.Texture2DArray.MipSlice = 0;
                    dx12UavDescription.Texture2DArray.PlaneSlice = 0;

                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    dx12SrvDescription.Texture2DArray.ArraySize = dx12Desc.DepthOrArraySize;
                    dx12SrvDescription.Texture2DArray.FirstArraySlice = 0;
                    dx12SrvDescription.Texture2DArray.MipLevels = dx12Desc.MipLevels;
                    dx12SrvDescription.Texture2DArray.MostDetailedMip = 0;
                    dx12SrvDescription.Texture2DArray.PlaneSlice = 0;
                }
                else
                {
                    dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    dx12UavDescription.Texture2D.MipSlice = 0;
                    dx12UavDescription.Texture2D.PlaneSlice = 0;

                    dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    dx12SrvDescription.Texture2D.MipLevels = dx12Desc.MipLevels;
                    dx12SrvDescription.Texture2D.MostDetailedMip = 0;
                    dx12SrvDescription.Texture2D.PlaneSlice = 0;
                }
                break;
            }

            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                dx12SrvDescription.Texture3D.MipLevels = dx12Resource->GetDesc().MipLevels;
                dx12SrvDescription.Texture3D.MostDetailedMip = 0;
                break;

            default:
                break;
            }

            if (dx12Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

                dx12SrvDescription.Buffer.FirstElement        = 0;
                dx12SrvDescription.Buffer.StructureByteStride = backendResource->resourceDescription.stride;
                dx12SrvDescription.Buffer.NumElements         = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;
                D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle     = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += outTexture->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, dx12CpuHandle);

                // UAV
                if (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                    FFX_ASSERT(effectContext.nextStaticUavDescriptor + 1 < effectContext.nextDynamicUavDescriptor);
                    backendResource->uavDescCount = 1;
                    backendResource->uavDescIndex = effectContext.nextStaticUavDescriptor++;

                    dx12UavDescription.Buffer.FirstElement = 0;
                    dx12UavDescription.Buffer.StructureByteStride = backendResource->resourceDescription.stride;
                    dx12UavDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;
                    dx12UavDescription.Buffer.CounterOffsetInBytes = 0;

                    dx12CpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                    dx12CpuHandle.ptr += (backendResource->uavDescIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);

                    dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                    dx12CpuHandle.ptr += (backendResource->uavDescIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);

                    effectContext.nextStaticUavDescriptor++;
                }
            }
            else {
                // CPU readable
                D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += outTexture->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, dx12CpuHandle);

                // UAV
                if (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                    const int32_t uavDescriptorCount = (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? dx12Desc.MipLevels : 1;
                    FFX_ASSERT(effectContext.nextStaticUavDescriptor + uavDescriptorCount < effectContext.nextDynamicUavDescriptor);

                    backendResource->uavDescCount = uavDescriptorCount;
                    backendResource->uavDescIndex = effectContext.nextStaticUavDescriptor;

                    for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorCount; ++currentMipIndex) {

                        if (createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE3D)
                        {
                            dx12UavDescription.Texture3D.MipSlice    = currentMipIndex;
                            dx12UavDescription.Texture3D.FirstWSlice = currentMipIndex;
                            dx12UavDescription.Texture3D.WSize       = createResourceDescription->resourceDescription.depth;
                        }
                        else if (createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE2D)
                            dx12UavDescription.Texture2D.MipSlice = currentMipIndex;
                        else if (createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE1D)
                            dx12UavDescription.Texture1D.MipSlice = currentMipIndex;

                        dx12CpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                        dx12CpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);

                        dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                        dx12CpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);
                    }

                    effectContext.nextStaticUavDescriptor += uavDescriptorCount;
                }
            }
        }

        // create upload resource and upload job
        if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED) {

            FfxResourceInternal copySrc;
            FfxCreateResourceDescription uploadDescription = { *createResourceDescription };
            uploadDescription.heapType = FFX_HEAP_TYPE_UPLOAD;
            uploadDescription.resourceDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;
            uploadDescription.initialState = FFX_RESOURCE_STATE_GENERIC_READ;

            backendInterface->fpCreateResource(backendInterface, &uploadDescription, effectContextId, &copySrc);

            // setup the upload job
            FfxGpuJobDescription copyJob  = { FFX_GPU_JOB_COPY, L"Resource Initialization Copy" };
            copyJob.copyJobDescriptor.src = copySrc;
            copyJob.copyJobDescriptor.dst = *outTexture;
            copyJob.copyJobDescriptor.srcOffset = 0;
            copyJob.copyJobDescriptor.dstOffset = 0;
            copyJob.copyJobDescriptor.size      = 0;

            backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
        }
    }
    
    uint64_t vramAfter = GetCurrentGpuMemoryUsageDX12(backendInterface);
    uint64_t vramDelta = vramAfter - vramBefore;
    effectContext.vramUsage.totalUsageInBytes += vramDelta;
    if ((createResourceDescription->resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
    {
        effectContext.vramUsage.aliasableUsageInBytes += vramDelta;
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceDX12(
    FfxInterface* backendInterface,
    FfxResourceInternal resource,
    FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
	BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
	if ((resource.internalIndex >= int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT)) && (resource.internalIndex < int32_t(effectContext.nextStaticResource))) {
		ID3D12Resource* dx12Resource = getDX12ResourcePtr(backendContext, resource.internalIndex);

		if (dx12Resource) {

            uint64_t vramBefore = GetCurrentGpuMemoryUsageDX12(backendInterface);

            if( s_fpResourceDestroyFunc) {
                s_fpResourceDestroyFunc(effectContext.effectId, dx12Resource);
            }
            else {
                dx12Resource->Release();
            }

            // update effect memory usage
            uint64_t vramAfter = GetCurrentGpuMemoryUsageDX12(backendInterface);
            uint64_t vramDelta = vramBefore - vramAfter;
            effectContext.vramUsage.totalUsageInBytes -= vramDelta;
            if ((backendContext->pResources[resource.internalIndex].resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
            {
                effectContext.vramUsage.aliasableUsageInBytes -= vramDelta;
            }

			backendContext->pResources[resource.internalIndex].resourcePtr = nullptr;
		}
        
        return FFX_OK;
	}

	return FFX_ERROR_OUT_OF_RANGE;
}

DXGI_FORMAT patchDxgiFormatWithFfxUsage(DXGI_FORMAT dxResFmt, FfxSurfaceFormat ffxFmt)
{
    DXGI_FORMAT fromFfx = ffxGetDX12FormatFromSurfaceFormat(ffxFmt);
    DXGI_FORMAT fmt = dxResFmt;

    switch (fmt)
    {
    case DXGI_FORMAT_UNKNOWN:
        return fromFfx;

    // fixup RGBA8 with SRGB flag passed in the ffxSurfaceFormat
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return fromFfx;
    
    // fixup depth formats as ffxGetDX12FormatFromSurfaceFormat will result in wrong format
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_UNORM;

    default:
        break;
    }
    return fmt;
}

FfxErrorCode MapResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    if (FAILED(backendContext->pResources[resource.internalIndex].resourcePtr->Map(0, NULL, ptr)))
        return FFX_ERROR_BACKEND_API_ERROR;

    return FFX_OK;
}

FfxErrorCode UnmapResourceDX12(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    backendContext->pResources[resource.internalIndex].resourcePtr->Unmap(0, NULL);

    return FFX_OK;
}

FfxErrorCode RegisterResourceDX12(
    FfxInterface* backendInterface,
    const FfxResource* inFfxResource,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outFfxResourceInternal
)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)(backendInterface->scratchBuffer);
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource* dx12Resource = reinterpret_cast<ID3D12Resource*>(inFfxResource->resource);
    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    FfxResourceStates state = inFfxResource->state;

    if (dx12Resource == nullptr) {

        outFfxResourceInternal->internalIndex = 0; // Always maps to FFX_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    FFX_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outFfxResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    BackendContext_DX12::Resource* backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];
    backendResource->resourcePtr = dx12Resource;
    backendResource->initialState = state;
    backendResource->currentState = state;

#ifdef _DEBUG
    const wchar_t* name = inFfxResource->name;
    if (name) {
        wcscpy_s(backendResource->resourceName, name);
    }
#endif

    // create resource views
    if (dx12Resource) {

        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
        D3D12_RESOURCE_DESC dx12Desc = dx12Resource->GetDesc();

        // we still want to respect the format provided in the description for SRGB or TYPELESS resources
        DXGI_FORMAT descFormat = patchDxgiFormatWithFfxUsage(dx12Desc.Format, inFfxResource->description.format);

        dx12UavDescription.Format = convertFormatUav(descFormat);
        // Will support something other than this only where there is an actual need for it
        dx12SrvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dx12SrvDescription.Format = convertFormatSrv(descFormat);

        bool requestArrayView = FFX_CONTAINS_FLAG(inFfxResource->description.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (dx12Desc.Dimension) {

        case D3D12_RESOURCE_DIMENSION_BUFFER:
            dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
            backendResource->resourceDescription.size = inFfxResource->description.size;
            backendResource->resourceDescription.stride = inFfxResource->description.stride;
            backendResource->resourceDescription.alignment = 0;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (dx12Desc.DepthOrArraySize > 1 || requestArrayView)
            {
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                dx12UavDescription.Texture1DArray.ArraySize = dx12Desc.DepthOrArraySize;
                dx12UavDescription.Texture1DArray.FirstArraySlice = 0;
                dx12UavDescription.Texture1DArray.MipSlice = 0;

                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                dx12SrvDescription.Texture1DArray.ArraySize = dx12Desc.DepthOrArraySize;
                dx12SrvDescription.Texture1DArray.FirstArraySlice = 0;
                dx12SrvDescription.Texture1DArray.MipLevels = dx12Desc.MipLevels;
                dx12SrvDescription.Texture1DArray.MostDetailedMip = 0;
            }
            else
            {
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                dx12UavDescription.Texture1D.MipSlice = 0;

                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                dx12SrvDescription.Texture1D.MipLevels = dx12Desc.MipLevels;
                dx12SrvDescription.Texture1D.MostDetailedMip = 0;
            }

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        {
            if (dx12Desc.DepthOrArraySize > 1 || requestArrayView)
            {
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                dx12UavDescription.Texture2DArray.ArraySize = dx12Desc.DepthOrArraySize;
                dx12UavDescription.Texture2DArray.FirstArraySlice = 0;
                dx12UavDescription.Texture2DArray.MipSlice = 0;
                dx12UavDescription.Texture2DArray.PlaneSlice = 0;

                dx12SrvDescription.ViewDimension = inFfxResource->description.type == FFX_RESOURCE_TYPE_TEXTURE_CUBE ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                dx12SrvDescription.Texture2DArray.ArraySize = dx12Desc.DepthOrArraySize;
                dx12SrvDescription.Texture2DArray.FirstArraySlice = 0;
                dx12SrvDescription.Texture2DArray.MipLevels = dx12Desc.MipLevels;
                dx12SrvDescription.Texture2DArray.MostDetailedMip = 0;
                dx12SrvDescription.Texture2DArray.PlaneSlice = 0;
            }
            else
            {
                dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                dx12UavDescription.Texture2D.MipSlice = 0;
                dx12UavDescription.Texture2D.PlaneSlice = 0;

                dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                dx12SrvDescription.Texture2D.MipLevels = dx12Desc.MipLevels;
                dx12SrvDescription.Texture2D.MostDetailedMip = 0;
                dx12SrvDescription.Texture2D.PlaneSlice = 0;
            }

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.height = inFfxResource->description.height;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;
        }

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            dx12UavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            dx12UavDescription.Texture3D.FirstWSlice      = 0; // Bind all W slices
            dx12UavDescription.Texture3D.WSize            = dx12Desc.DepthOrArraySize;
            dx12SrvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            dx12SrvDescription.Texture3D.MipLevels = dx12Desc.MipLevels;
            dx12SrvDescription.Texture3D.MostDetailedMip = 0;

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.height = inFfxResource->description.height;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;

        default:
            break;
        }

        if (dx12Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {

            // UAV
            if (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                FFX_ASSERT(effectContext.nextDynamicUavDescriptor > effectContext.nextStaticUavDescriptor);
                backendResource->uavDescCount = 1;
                backendResource->uavDescIndex = effectContext.nextDynamicUavDescriptor--;

                dx12UavDescription.Format = DXGI_FORMAT_UNKNOWN;
                dx12UavDescription.Buffer.FirstElement = 0;
                dx12UavDescription.Buffer.StructureByteStride = backendResource->resourceDescription.stride;
                dx12UavDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;
                dx12UavDescription.Buffer.CounterOffsetInBytes = 0;

                D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += (backendResource->uavDescIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);

                dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += (backendResource->uavDescIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, dx12CpuHandle);
            }

            {
                dx12SrvDescription.Format = DXGI_FORMAT_UNKNOWN;
                dx12SrvDescription.Buffer.FirstElement        = 0;
                dx12SrvDescription.Buffer.StructureByteStride = backendResource->resourceDescription.stride;
                dx12SrvDescription.Buffer.NumElements         = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;
                D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle     = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                dx12CpuHandle.ptr += outFfxResourceInternal->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, dx12CpuHandle);
                backendResource->srvDescIndex = outFfxResourceInternal->internalIndex;
            }
        }
        else {

            // CPU readable
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
            cpuHandle.ptr += outFfxResourceInternal->internalIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, cpuHandle);
            backendResource->srvDescIndex = outFfxResourceInternal->internalIndex;

            // UAV
            if (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) {

                const int32_t uavDescriptorsCount = (dx12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) ? dx12Desc.MipLevels : 1;
                FFX_ASSERT(effectContext.nextDynamicUavDescriptor - uavDescriptorsCount + 1 > effectContext.nextStaticUavDescriptor);

                backendResource->uavDescCount = uavDescriptorsCount;
                backendResource->uavDescIndex = effectContext.nextDynamicUavDescriptor - uavDescriptorsCount + 1;

                for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorsCount; ++currentMipIndex) {

                    switch (dx12Desc.Dimension)
                    {
                    case D3D12_RESOURCE_DIMENSION_BUFFER:
                        break;

                    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                        // TextureXD<Y>.MipSlice values map to same mem
                        dx12UavDescription.Texture2D.MipSlice = currentMipIndex;
                        break;

                    default:
                        FFX_ASSERT_MESSAGE(false, "Invalid View Dimension");
                        break;
                    }

                    cpuHandle = backendContext->descHeapUavGpu->GetCPUDescriptorHandleForHeapStart();
                    cpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, cpuHandle);

                    cpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                    cpuHandle.ptr += (backendResource->uavDescIndex + currentMipIndex) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    dx12Device->CreateUnorderedAccessView(dx12Resource, 0, &dx12UavDescription, cpuHandle);
                }

                effectContext.nextDynamicUavDescriptor -= uavDescriptorsCount;
            }
        }
    }

    return FFX_OK;
}

FfxResource GetResourceDX12(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FfxResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxResource resource = {};
    resource.resource = resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].resourcePtr);
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        wcscpy_s(resource.name, backendContext->pResources[inResource.internalIndex].resourceName);
    }
#endif

    return resource;
}

// dispose dynamic resources: This should be called at the end of the frame
FfxErrorCode UnregisterResourcesDX12(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)(backendInterface->scratchBuffer);
    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    for (uint32_t resourceIndex = ++effectContext.nextDynamicResource; resourceIndex < (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT; ++resourceIndex)
    {
        FfxResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;

        BackendContext_DX12::Resource* backendResource = &backendContext->pResources[resourceIndex];
        addBarrier(backendContext, &internalResource, backendResource->initialState);
    }

    FFX_ASSERT(nullptr != commandList);
    ID3D12GraphicsCommandList* pCmdList = reinterpret_cast<ID3D12GraphicsCommandList*>(commandList);

    flushBarriers(backendContext, pCmdList);

    effectContext.nextDynamicResource      = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
    effectContext.nextDynamicUavDescriptor = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;

    return FFX_OK;
}

FfxErrorCode registerStaticTextureSrv(BackendContext_DX12* backendContext, const FfxResource* inResource, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_DX12::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];
    ID3D12Device*                       dx12Device     = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource*                     dx12Resource   = reinterpret_cast<ID3D12Resource*>(inResource->resource);

    if (effectContext.bindlessTextureSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (dx12Resource == nullptr)
        return FFX_OK;

    // create resource views
    if (dx12Resource)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
        dx12SrvDescription.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dx12SrvDescription.Format                          = convertFormatSrv(dx12Resource->GetDesc().Format);

        uint32_t depthArraySize = dx12Resource->GetDesc().DepthOrArraySize;

        switch (dx12Resource->GetDesc().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (depthArraySize > 1)
            {
                dx12SrvDescription.ViewDimension            = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                dx12SrvDescription.Texture1DArray.MipLevels = dx12Resource->GetDesc().MipLevels;
                dx12SrvDescription.Texture1DArray.ArraySize = depthArraySize;
            }
            else
            {
                dx12SrvDescription.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE1D;
                dx12SrvDescription.Texture1D.MipLevels = dx12Resource->GetDesc().MipLevels;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (depthArraySize > 1)
            {
                dx12SrvDescription.ViewDimension            = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                dx12SrvDescription.Texture2DArray.MipLevels = dx12Resource->GetDesc().MipLevels;
                dx12SrvDescription.Texture2DArray.ArraySize = depthArraySize;
            }
            else
            {
                dx12SrvDescription.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE2D;
                dx12SrvDescription.Texture2D.MipLevels = dx12Resource->GetDesc().MipLevels;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            dx12SrvDescription.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE3D;
            dx12SrvDescription.Texture3D.MipLevels = dx12Resource->GetDesc().MipLevels;
            break;

        default:
            // Only texture resources are allowed here.
            FFX_ASSERT(false);
            break;
        }

        // CPU readable
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descRingBuffer->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr +=
            (effectContext.bindlessTextureSrvHeapStart + index) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, cpuHandle);
    }

    return FFX_OK;
}

FfxErrorCode registerStaticBufferSrv(BackendContext_DX12* backendContext, const FfxResource* inResource, uint32_t offset, uint32_t size, uint32_t stride, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_DX12::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];
    ID3D12Device*                       dx12Device     = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource*                     dx12Resource   = reinterpret_cast<ID3D12Resource*>(inResource->resource);

    if (effectContext.bindlessBufferSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    constexpr uint32_t ShaderComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (dx12Resource == nullptr)
        return FFX_OK;

    // create resource views
    if (dx12Resource)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};
        dx12SrvDescription.Shader4ComponentMapping         = ShaderComponentMapping;

        switch (dx12Resource->GetDesc().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
        {
            FFX_ASSERT(stride > 0);

            const uint32_t actualSize = size > 0 ? size : uint32_t(dx12Resource->GetDesc().Width);

            dx12SrvDescription.Format                     = DXGI_FORMAT_UNKNOWN;
            dx12SrvDescription.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
            dx12SrvDescription.Buffer.FirstElement        = offset / stride;
            dx12SrvDescription.Buffer.NumElements         = actualSize / stride;
            dx12SrvDescription.Buffer.StructureByteStride = stride;
            dx12SrvDescription.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
            dx12SrvDescription.Shader4ComponentMapping    = ShaderComponentMapping;
            break;
        }
        default:
            // Only buffer resources are allowed here.
            FFX_ASSERT(false);
            break;
        }

        // CPU readable
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descRingBuffer->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr +=
            (effectContext.bindlessBufferSrvHeapStart + index) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        dx12Device->CreateShaderResourceView(dx12Resource, &dx12SrvDescription, cpuHandle);
    }

    return FFX_OK;
}

FfxErrorCode registerStaticTextureUav(BackendContext_DX12* backendContext, const FfxResource* inResource, uint32_t mip, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_DX12::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];
    ID3D12Device*                       dx12Device     = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource*                     dx12Resource   = reinterpret_cast<ID3D12Resource*>(inResource->resource);

    if (effectContext.bindlessTextureUavHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (dx12Resource == nullptr)
        return FFX_OK;

    // create resource views
    if (dx12Resource)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};
        dx12UavDescription.Format                           = convertFormatSrv(dx12Resource->GetDesc().Format);

        uint32_t depthArraySize = dx12Resource->GetDesc().DepthOrArraySize;

        switch (dx12Resource->GetDesc().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        {
            if (depthArraySize > 1)
            {
                dx12UavDescription.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                dx12UavDescription.Texture1DArray.ArraySize       = depthArraySize;
                dx12UavDescription.Texture1DArray.FirstArraySlice = 0;
                dx12UavDescription.Texture1DArray.MipSlice        = mip;
            }
            else
            {
                dx12UavDescription.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                dx12UavDescription.Texture1D.MipSlice = mip;
            }
            break;
        }
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        {
            if (depthArraySize > 1)
            {
                dx12UavDescription.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                dx12UavDescription.Texture2DArray.ArraySize       = depthArraySize;
                dx12UavDescription.Texture2DArray.FirstArraySlice = 0;
                dx12UavDescription.Texture2DArray.MipSlice        = mip;
                dx12UavDescription.Texture2DArray.PlaneSlice      = 0;
            }
            else
            {
                dx12UavDescription.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                dx12UavDescription.Texture2D.MipSlice   = mip;
                dx12UavDescription.Texture2D.PlaneSlice = 0;
            }
            break;
        }
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        {
            dx12UavDescription.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
            dx12UavDescription.Texture3D.FirstWSlice = 0;  // Bind all W slices
            dx12UavDescription.Texture3D.WSize       = depthArraySize;
            break;
        }
        default:
            // Only texture resources are allowed here.
            FFX_ASSERT(false);
            break;
        }

        // CPU readable
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descRingBuffer->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr +=
            (effectContext.bindlessTextureUavHeapStart + index) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        dx12Device->CreateUnorderedAccessView(dx12Resource, nullptr, &dx12UavDescription, cpuHandle);
    }

    return FFX_OK;
}

FfxErrorCode registerStaticBufferUav(BackendContext_DX12* backendContext, const FfxResource* inResource, uint32_t offset, uint32_t size, uint32_t stride, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_DX12::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];
    ID3D12Device*                       dx12Device     = reinterpret_cast<ID3D12Device*>(backendContext->device);
    ID3D12Resource*                     dx12Resource   = reinterpret_cast<ID3D12Resource*>(inResource->resource);

    if (effectContext.bindlessBufferUavHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (dx12Resource == nullptr)
        return FFX_OK;

    // create resource views
    if (dx12Resource)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};

        switch (dx12Resource->GetDesc().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
        {
            FFX_ASSERT(stride > 0);

            const uint32_t actualSize = size > 0 ? size : uint32_t(dx12Resource->GetDesc().Width);

            dx12UavDescription.Format                      = DXGI_FORMAT_UNKNOWN;
            dx12UavDescription.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
            dx12UavDescription.Buffer.FirstElement         = offset / stride;
            dx12UavDescription.Buffer.NumElements          = actualSize / stride;
            dx12UavDescription.Buffer.StructureByteStride  = stride;
            dx12UavDescription.Buffer.CounterOffsetInBytes = 0;
            dx12UavDescription.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;
            break;
        }
        default:
            // Only buffer resources are allowed here.
            FFX_ASSERT(false);
            break;
        }

        // CPU readable
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = backendContext->descRingBuffer->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr +=
            (effectContext.bindlessBufferUavHeapStart + index) * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        dx12Device->CreateUnorderedAccessView(dx12Resource, nullptr, &dx12UavDescription, cpuHandle);
    }

    return FFX_OK;
}

FfxErrorCode RegisterStaticResourceDX12(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != desc);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    switch (desc->descriptorType)
    {
    case FFX_DESCRIPTOR_TEXTURE_SRV:
        return registerStaticTextureSrv(backendContext, desc->resource, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_SRV:
        return registerStaticBufferSrv(backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_TEXTURE_UAV:
        return registerStaticTextureUav(backendContext, desc->resource, desc->textureUavMip, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_UAV:
        return registerStaticBufferUav(backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    default:
        return FFX_ERROR_INVALID_ARGUMENT;
    }
}

FfxResourceDescription GetResourceDescriptorDX12(
    FfxInterface* backendInterface,
    FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

FfxErrorCode StageConstantBufferDataDX12(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    if (data && constantBuffer)
    {
        if ((backendContext->stagingRingBufferBase + FFX_ALIGN_UP(size, 256)) >= FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE)
            backendContext->stagingRingBufferBase = 0;

        uint32_t* dstPtr = (uint32_t*)(backendContext->pStagingRingBuffer + backendContext->stagingRingBufferBase);

        memcpy(dstPtr, data, size);

        constantBuffer->data            = dstPtr;
        constantBuffer->num32BitEntries = size / sizeof(uint32_t);

        backendContext->stagingRingBufferBase += FFX_ALIGN_UP(size, 256);

        return FFX_OK;
    }
    else
        return FFX_ERROR_INVALID_POINTER;
}

D3D12_TEXTURE_ADDRESS_MODE FfxGetAddressModeDX12(const FfxAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFX_ADDRESS_MODE_WRAP:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case FFX_ADDRESS_MODE_MIRROR:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case FFX_ADDRESS_MODE_CLAMP:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case FFX_ADDRESS_MODE_BORDER:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case FFX_ADDRESS_MODE_MIRROR_ONCE:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    default:
        FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    }
}

FfxErrorCode CreatePipelineDX12(
    FfxInterface* backendInterface,
    FfxEffect effect,
    FfxPass pass,
    uint32_t permutationOptions,
    const FfxPipelineDescription* pipelineDescription,
    FfxUInt32                     effectContextId,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;
    ID3D12Device* dx12Device = backendContext->device;

    FfxShaderBlob shaderBlob = { };
    backendInterface->fpGetPermutationBlobByIndex(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE, permutationOptions, &shaderBlob);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    int32_t staticTextureSrvCount = 0;
    int32_t staticBufferSrvCount  = 0;
    int32_t staticTextureUavCount = 0;
    int32_t staticBufferUavCount  = 0;

    int32_t staticTextureSrvSpace = -1;
    int32_t staticBufferSrvSpace  = -1;
    int32_t staticTextureUavSpace = -1;
    int32_t staticBufferUavSpace  = -1;

    // set up root signature
    // easiest implementation: simply create one root signature per pipeline
    // should add some management later on to avoid unnecessarily re-binding the root signature
    {
        // Xbox targets use root signature from the HLSL, so get the RS from shader
        dx12Device->CreateRootSignature(0, shaderBlob.data, shaderBlob.size, IID_GDK_PPV_ARGS(reinterpret_cast<ID3D12RootSignature**>(&outPipeline->rootSignature)));
    }

    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(outPipeline->rootSignature);

    // Only set the command signature if this is setup as an indirect workload
    if (pipelineDescription->indirectWorkload)
    {
        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs = { D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH };
        D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
        commandSignatureDesc.pArgumentDescs = &argumentDescs;
        commandSignatureDesc.NumArgumentDescs = 1;
        commandSignatureDesc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);

        HRESULT result = dx12Device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_GDK_PPV_ARGS(reinterpret_cast<ID3D12CommandSignature**>(&outPipeline->cmdSignature)));
        if (FAILED(result)) {

            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    uint32_t flattenedSrvTextureCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        uint32_t slotIndex  = shaderBlob.boundSRVTextures[srvIndex];
        uint32_t spaceIndex = shaderBlob.boundSRVTextureSpaces[srvIndex];
        uint32_t bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticTextureSrvSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvTextureCount++;

            outPipeline->srvTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                                0,
                                shaderBlob.boundSRVTextureNames[srvIndex],
                                -1,
                                outPipeline->srvTextureBindings[bindingIndex].name,
                                int(std::size(outPipeline->srvTextureBindings[bindingIndex].name)));
        }
    }

    outPipeline->srvTextureCount = flattenedSrvTextureCount;
    FFX_ASSERT(outPipeline->srvTextureCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavTextureCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        uint32_t slotIndex  = shaderBlob.boundUAVTextures[uavIndex];
        uint32_t spaceIndex = shaderBlob.boundUAVTextureSpaces[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticTextureUavSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavTextureCount++;

            outPipeline->uavTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                                0,
                                shaderBlob.boundUAVTextureNames[uavIndex],
                                -1,
                                outPipeline->uavTextureBindings[bindingIndex].name,
                                int(std::size(outPipeline->uavTextureBindings[bindingIndex].name)));
        }
    }

    outPipeline->uavTextureCount = flattenedUavTextureCount;
    FFX_ASSERT(outPipeline->uavTextureCount < FFX_MAX_NUM_UAVS);

    uint32_t flattenedSrvBufferCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        uint32_t slotIndex  = shaderBlob.boundSRVBuffers[srvIndex];
        uint32_t spaceIndex = shaderBlob.boundSRVBufferSpaces[srvIndex];
        uint32_t bindCount  = shaderBlob.boundSRVBufferCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticBufferSrvSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvBufferCount++;

            outPipeline->srvBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                                0,
                                shaderBlob.boundSRVBufferNames[srvIndex],
                                -1,
                                outPipeline->srvBufferBindings[bindingIndex].name,
                                int(std::size(outPipeline->srvBufferBindings[bindingIndex].name)));
        }
    }

    outPipeline->srvBufferCount = flattenedSrvBufferCount;
    FFX_ASSERT(outPipeline->srvBufferCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavBufferCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        uint32_t slotIndex  = shaderBlob.boundUAVBuffers[uavIndex];
        uint32_t spaceIndex = shaderBlob.boundUAVBufferSpaces[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticBufferUavSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavBufferCount++;

            outPipeline->uavBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                                0,
                                shaderBlob.boundUAVBufferNames[uavIndex],
                                -1,
                                outPipeline->uavBufferBindings[bindingIndex].name,
                                int(std::size(outPipeline->uavBufferBindings[bindingIndex].name)));
        }
    }

    outPipeline->uavBufferCount = flattenedUavBufferCount;
    FFX_ASSERT(outPipeline->uavBufferCount < FFX_MAX_NUM_UAVS);

    for (uint32_t cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex  = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].arrayIndex = 1;
        MultiByteToWideChar(CP_UTF8,
                            0,
                            shaderBlob.boundConstantBufferNames[cbIndex],
                            -1,
                            outPipeline->constantBufferBindings[cbIndex].name,
                            int(std::size(outPipeline->constantBufferBindings[cbIndex].name)));
    }

    outPipeline->constCount = shaderBlob.cbvCount;
    FFX_ASSERT(outPipeline->constCount < FFX_MAX_NUM_CONST_BUFFERS);

    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    outPipeline->staticTextureSrvCount = staticTextureSrvCount;
    FFX_ASSERT(outPipeline->staticTextureSrvCount <= effectContext.bindlessTextureSrvHeapSize);

    outPipeline->staticBufferSrvCount = staticBufferSrvCount;
    FFX_ASSERT(outPipeline->staticBufferSrvCount <= effectContext.bindlessBufferSrvHeapSize);

    outPipeline->staticTextureUavCount = staticTextureUavCount;
    FFX_ASSERT(outPipeline->staticTextureUavCount <= effectContext.bindlessTextureUavHeapSize);

    outPipeline->staticBufferUavCount = staticBufferUavCount;
    FFX_ASSERT(outPipeline->staticBufferUavCount <= effectContext.bindlessBufferUavHeapSize);

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;
        
    // create the PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC dx12PipelineStateDescription = {};
    dx12PipelineStateDescription.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    dx12PipelineStateDescription.pRootSignature = dx12RootSignature;
    dx12PipelineStateDescription.CS.pShaderBytecode = shaderBlob.data;
    dx12PipelineStateDescription.CS.BytecodeLength = shaderBlob.size;

    if (FAILED(dx12Device->CreateComputePipelineState(&dx12PipelineStateDescription, IID_GDK_PPV_ARGS(reinterpret_cast<ID3D12PipelineState**>(&outPipeline->pipeline)))))
        return FFX_ERROR_BACKEND_API_ERROR;

    // Set the pipeline name
    reinterpret_cast<ID3D12PipelineState*>(outPipeline->pipeline)->SetName(pipelineDescription->name);
    wcscpy_s(outPipeline->name, pipelineDescription->name);

    // Set the pass id for this pipeline
    outPipeline->passId = pass;

    return FFX_OK;
}

FfxErrorCode DestroyPipelineDX12(
    FfxInterface* backendInterface,
    FfxPipelineState* pipeline,
    FfxUInt32)
{
    FFX_ASSERT(backendInterface != nullptr);
    if (!pipeline) {
        return FFX_OK;
    }

    // destroy Rootsignature
    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(pipeline->rootSignature);
    if (dx12RootSignature) {
        dx12RootSignature->Release();
    }
    pipeline->rootSignature = nullptr;

    // destroy CmdSignature
    ID3D12CommandSignature* dx12CmdSignature = reinterpret_cast<ID3D12CommandSignature*>(pipeline->cmdSignature);
    if (dx12CmdSignature) {
        dx12CmdSignature->Release();
    }
    pipeline->cmdSignature = nullptr;

    // destroy pipeline
    ID3D12PipelineState* dx12Pipeline = reinterpret_cast<ID3D12PipelineState*>(pipeline->pipeline);
    if (dx12Pipeline) {
        dx12Pipeline->Release();
    }
    pipeline->pipeline = nullptr;

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobDX12(FfxInterface* backendInterface, const FfxGpuJobDescription* job)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != job);

    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;
    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_DX12*       backendContext,
                                         FfxGpuJobDescription*      job,
                                         ID3D12GraphicsCommandList* dx12CommandList,
                                         FfxUInt32                  effectContextId)
{
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);
    BackendContext_DX12::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Set descriptor head for binding
    ID3D12DescriptorHeap* dx12DescriptorHeap = reinterpret_cast<ID3D12DescriptorHeap*>(backendContext->descRingBuffer);

    // set root signature
    ID3D12RootSignature* dx12RootSignature = reinterpret_cast<ID3D12RootSignature*>(job->computeJobDescriptor.pipeline.rootSignature);
    dx12CommandList->SetComputeRootSignature(dx12RootSignature);

    // set descriptor heap
    dx12CommandList->SetDescriptorHeaps(1, &dx12DescriptorHeap);

    uint32_t descriptorTableIndex = 0;

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    {
        // Set a baseline minimal value
        uint32_t maximumUavIndex = job->computeJobDescriptor.pipeline.uavTextureCount + job->computeJobDescriptor.pipeline.uavBufferCount;

        for (uint32_t uavTextureBinding = 0; uavTextureBinding < job->computeJobDescriptor.pipeline.uavTextureCount; uavTextureBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[uavTextureBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.uavTextureBindings[uavTextureBinding].arrayIndex;

            if (slotIndex > maximumUavIndex)
                maximumUavIndex = slotIndex;
        }

        for (uint32_t uavBufferBinding = 0; uavBufferBinding < job->computeJobDescriptor.pipeline.uavBufferCount; uavBufferBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[uavBufferBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.uavTextureBindings[uavBufferBinding].arrayIndex;

            if (slotIndex > maximumUavIndex)
                maximumUavIndex = slotIndex;
        }

        if (maximumUavIndex)
        {
            // check if this fits into the ringbuffer, loop if not fitting
            if (backendContext->descRingBufferBase + maximumUavIndex + 1 > FFX_RING_BUFFER_DESCRIPTOR_COUNT * backendContext->maxEffectContexts)
                backendContext->descRingBufferBase = 0;

            D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            gpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // Set Texture UAVs
            for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex) {

                addBarrier(backendContext, &job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].resource, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex];

                // source: UAV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].resource.internalIndex;

                // NOTE: Now determine the maximal accessible index of the lowest mip.
                //      That is needed because the actual number of mips stored in 'backendContext->pResources[resourceIndex].uavDescCount'
                //      is based on texture resolution (720p for example), so it could be == 11
                //
                uint32_t texWidth = backendContext->pResources[resourceIndex].resourceDescription.width;
                uint32_t texHeight = backendContext->pResources[resourceIndex].resourceDescription.height;
                const uint32_t actualMipCount = backendContext->pResources[resourceIndex].uavDescCount;
                const uint32_t bindingMipCount = static_cast<uint32_t>(std::floor(std::log2(FFX_MAXIMUM(texWidth, texHeight)))) + 1;
                const uint32_t minimalAccessibleMipCount = FFX_MINIMUM(actualMipCount, bindingMipCount);

                // initialise UAV index to starting UAV index
                const uint32_t uavIndex = backendContext->pResources[resourceIndex].uavDescIndex + FFX_MINIMUM(minimalAccessibleMipCount - 1, job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].mip);

                // where to bind it
                const uint32_t currentUavResourceIndex = binding.slotIndex + binding.arrayIndex;

                D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                srcHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                cpuView.ptr += currentUavResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                // Copy descriptor
                dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            // Set Buffer UAVs
            for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex) {
                
                // continue if this is a null resource.
                if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex == 0)
                    continue;

                addBarrier(backendContext, &job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex];

                // where to bind it
                const uint32_t currentUavResourceIndex = binding.slotIndex + binding.arrayIndex;

                D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                cpuView.ptr += currentUavResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].size > 0)
                {
                    // if size is non-zero create a dynamic descriptor directly on the GPU heap
                    ID3D12Resource* buffer = getDX12ResourcePtr(backendContext, job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex);
                    FFX_ASSERT(buffer != NULL);

                    D3D12_UNORDERED_ACCESS_VIEW_DESC dx12UavDescription = {};

                    bool     isStructured = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride > 0;
                    uint32_t stride       = isStructured ? job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride : sizeof(uint32_t);

                    dx12UavDescription.Format                      = isStructured ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS;
                    dx12UavDescription.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
                    dx12UavDescription.Buffer.FirstElement         = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].offset / stride;
                    dx12UavDescription.Buffer.NumElements          = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].size / stride;
                    dx12UavDescription.Buffer.StructureByteStride  = isStructured ? stride : 0;
                    dx12UavDescription.Buffer.CounterOffsetInBytes = 0;
                    dx12UavDescription.Buffer.Flags                = isStructured ? D3D12_BUFFER_UAV_FLAG_NONE : D3D12_BUFFER_UAV_FLAG_RAW;

                    dx12Device->CreateUnorderedAccessView(buffer, 0, &dx12UavDescription, cpuView);
                }
                else
                {
                    // if size is zero assume it is a static descriptor and copy it from the CPU heap
                    const uint32_t resourceIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex;
          
                    const uint32_t uavIndex  = backendContext->pResources[resourceIndex].uavDescIndex;

                    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
                    srcHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                    // Copy descriptor
                    dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }

            backendContext->descRingBufferBase += maximumUavIndex + 1;
            dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
        }
    }

    // bind texture & buffer SRVs
    {
        // Set a baseline minimal value
        uint32_t maximumSrvIndex = job->computeJobDescriptor.pipeline.srvTextureCount + job->computeJobDescriptor.pipeline.srvBufferCount;

        for (uint32_t srvTextureBinding = 0; srvTextureBinding < job->computeJobDescriptor.pipeline.srvTextureCount; srvTextureBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[srvTextureBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.srvTextureBindings[srvTextureBinding].arrayIndex;

            if (slotIndex > maximumSrvIndex)
                maximumSrvIndex = slotIndex;
        }

        for (uint32_t srvBufferBinding = 0; srvBufferBinding < job->computeJobDescriptor.pipeline.srvBufferCount; srvBufferBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[srvBufferBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.srvTextureBindings[srvBufferBinding].arrayIndex;

            if (slotIndex > maximumSrvIndex)
                maximumSrvIndex = slotIndex;
        }

        if (maximumSrvIndex)
        {
            // check if this fits into the ringbuffer, loop if not fitting
            if (backendContext->descRingBufferBase + maximumSrvIndex + 1 > FFX_RING_BUFFER_DESCRIPTOR_COUNT * backendContext->maxEffectContexts)
            {
                backendContext->descRingBufferBase = 0;
            }

            D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            gpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
            {
                if (job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex == 0)
                    break;

                addBarrier(backendContext, &job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource, FFX_RESOURCE_STATE_COMPUTE_READ);

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex];

                // source: SRV of resource to bind
                const uint32_t              resourceIndex = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex;
                D3D12_CPU_DESCRIPTOR_HANDLE srcHandle     = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                srcHandle.ptr += resourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                // Where to bind it
                uint32_t currentSrvResourceIndex = binding.slotIndex + binding.arrayIndex;

                D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                cpuView.ptr += currentSrvResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            // Set Buffer SRVs
            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
            {
                // continue if this is a null resource.
                if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex == 0)
                    continue;

                addBarrier(backendContext, &job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource, FFX_RESOURCE_STATE_COMPUTE_READ);

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex];

                // where to bind it
                const uint32_t currentSrvResourceIndex = binding.slotIndex + binding.arrayIndex;

                D3D12_CPU_DESCRIPTOR_HANDLE cpuView = dx12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                cpuView.ptr += backendContext->descRingBufferBase * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                cpuView.ptr += currentSrvResourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].size > 0)
                {
                    // if size is non-zero create a dynamic descriptor directly on the GPU heap
                    ID3D12Resource* buffer = getDX12ResourcePtr(backendContext, job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex);
                    FFX_ASSERT(buffer != NULL);

                    D3D12_SHADER_RESOURCE_VIEW_DESC dx12SrvDescription = {};

                    bool     isStructured = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride > 0;
                    uint32_t stride       = isStructured ? job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride : sizeof(uint32_t);

                    dx12SrvDescription.Format                     = isStructured ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS;
                    dx12SrvDescription.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                    dx12SrvDescription.Buffer.FirstElement        = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].offset / stride;
                    dx12SrvDescription.Buffer.NumElements         = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].size / stride;
                    dx12SrvDescription.Buffer.StructureByteStride = isStructured ? stride : 0;
                    dx12SrvDescription.Buffer.Flags               = isStructured ? D3D12_BUFFER_SRV_FLAG_NONE : D3D12_BUFFER_SRV_FLAG_RAW;
                    dx12SrvDescription.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                    dx12Device->CreateShaderResourceView(buffer, &dx12SrvDescription, cpuView);
                }
                else
                {
                    // source: SRV of buffer to bind
                    const uint32_t resourceIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex;

                    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = backendContext->descHeapSrvCpu->GetCPUDescriptorHandleForHeapStart();
                    srcHandle.ptr += resourceIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                    // Copy descriptor
                    dx12Device->CopyDescriptorsSimple(1, cpuView, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }

            backendContext->descRingBufferBase += maximumSrvIndex + 1;
            dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
        }
    }

    // bind static texture srv table
    if (job->computeJobDescriptor.pipeline.staticTextureSrvCount > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += effectContext.bindlessTextureSrvHeapStart * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    // bind static buffer srv table
    if (job->computeJobDescriptor.pipeline.staticBufferSrvCount > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += effectContext.bindlessBufferSrvHeapStart * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    // bind static texture uav table
    if (job->computeJobDescriptor.pipeline.staticTextureUavCount > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += effectContext.bindlessTextureUavHeapStart * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    // bind static buffer uav table
    if (job->computeJobDescriptor.pipeline.staticBufferUavCount > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuView = dx12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        gpuView.ptr += effectContext.bindlessBufferUavHeapStart * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        dx12CommandList->SetComputeRootDescriptorTable(descriptorTableIndex++, gpuView);
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFX_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    flushBarriers(backendContext, dx12CommandList);

    // bind pipeline
    ID3D12PipelineState* dx12PipelineStateObject = reinterpret_cast<ID3D12PipelineState*>(job->computeJobDescriptor.pipeline.pipeline);
    dx12CommandList->SetPipelineState(dx12PipelineStateObject);

    // copy data to constant buffer and bind
    {
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex) {

            // If we have a constant buffer allocator, use that, otherwise use the default backend allocator
            FfxConstantAllocation allocation;
            if (s_fpConstantAllocator)
            {
                allocation = s_fpConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
            }
            else
            {
                allocation = backendContext->FallbackConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
            }

            D3D12_GPU_VIRTUAL_ADDRESS bufferViewDesc = D3D12_GPU_VIRTUAL_ADDRESS(allocation.handle);
            dx12CommandList->SetComputeRootConstantBufferView(descriptorTableIndex + currentRootConstantIndex, bufferViewDesc);
        }
    }

    // Dispatch (or dispatch indirect)
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        const uint32_t resourceIndex = job->computeJobDescriptor.cmdArgument.internalIndex;
        ID3D12Resource* pResource = backendContext->pResources[resourceIndex].resourcePtr;

        dx12CommandList->ExecuteIndirect(reinterpret_cast<ID3D12CommandSignature*>(job->computeJobDescriptor.pipeline.cmdSignature), 1, pResource, job->computeJobDescriptor.cmdArgumentOffset, nullptr, 0);
    }
    else
    {
        dx12CommandList->Dispatch(job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_DX12* backendContext, FfxUInt32, FfxGpuJobDescription* job, ID3D12GraphicsCommandList* dx12CommandList)
{
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);

    ID3D12Resource* dx12ResourceSrc = getDX12ResourcePtr(backendContext, job->copyJobDescriptor.src.internalIndex);
    ID3D12Resource* dx12ResourceDst = getDX12ResourcePtr(backendContext, job->copyJobDescriptor.dst.internalIndex);
    D3D12_RESOURCE_DESC dx12ResourceDescriptionDst = dx12ResourceDst->GetDesc();
    D3D12_RESOURCE_DESC dx12ResourceDescriptionSrc = dx12ResourceSrc->GetDesc();

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, dx12CommandList);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT dx12Footprint = {};
    UINT rowCount;
    UINT64 rowSizeInBytes;
    UINT64 totalBytes;
    dx12Device->GetCopyableFootprints(&dx12ResourceDescriptionDst, 0, 1, 0, &dx12Footprint, &rowCount, &rowSizeInBytes, &totalBytes);

    if (dx12ResourceDescriptionDst.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        dx12CommandList->CopyBufferRegion(dx12ResourceDst,
                                          job->copyJobDescriptor.dstOffset,
                                          dx12ResourceSrc,
                                          job->copyJobDescriptor.srcOffset,
                                          job->copyJobDescriptor.size > 0 ? job->copyJobDescriptor.size : totalBytes);
    }
    else if (dx12ResourceDescriptionSrc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // TODO: account for source buffer offset
        D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
        dx12SourceLocation.pResource = dx12ResourceSrc;
        dx12SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dx12SourceLocation.PlacedFootprint = dx12Footprint;

        D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
        dx12DestinationLocation.pResource = dx12ResourceDst;
        dx12DestinationLocation.SubresourceIndex = 0;
        dx12DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        dx12CommandList->CopyTextureRegion(&dx12DestinationLocation, 0, 0, 0, &dx12SourceLocation, nullptr);
    }
    else
    {
        D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
        dx12SourceLocation.pResource = dx12ResourceSrc;
        dx12SourceLocation.SubresourceIndex = 0;
        dx12SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
        dx12DestinationLocation.pResource = dx12ResourceDst;
        dx12DestinationLocation.SubresourceIndex = 0;
        dx12DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        dx12CommandList->CopyTextureRegion(&dx12DestinationLocation, 0, 0, 0, &dx12SourceLocation, nullptr);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobBarrier(BackendContext_DX12* backendContext, FfxUInt32, FfxGpuJobDescription* job, ID3D12GraphicsCommandList* dx12CommandList)
{
    addBarrier(backendContext, &job->barrierDescriptor.resource, job->barrierDescriptor.newState);
    flushBarriers(backendContext, dx12CommandList);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_DX12* backendContext, FfxUInt32, FfxGpuJobDescription* job, ID3D12GraphicsCommandList* dx12CommandList)
{
    ID3D12Device* dx12Device = reinterpret_cast<ID3D12Device*>(backendContext->device);

    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_DX12::Resource ffxResource = backendContext->pResources[idx];
    ID3D12Resource* dx12Resource = reinterpret_cast<ID3D12Resource*>(ffxResource.resourcePtr);
    uint32_t uavIndex = ffxResource.uavDescIndex;

    D3D12_CPU_DESCRIPTOR_HANDLE dx12CpuHandle = backendContext->descHeapUavCpu->GetCPUDescriptorHandleForHeapStart();
    dx12CpuHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_GPU_DESCRIPTOR_HANDLE dx12GpuHandle = backendContext->descHeapUavGpu->GetGPUDescriptorHandleForHeapStart();
    dx12GpuHandle.ptr += uavIndex * dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    dx12CommandList->SetDescriptorHeaps(1, &backendContext->descHeapUavGpu);

    addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    flushBarriers(backendContext, dx12CommandList);

    // If the format is R16G16, force a float clear as UINT clear doesn't work all the time
    if (ffxResource.resourceDescription.format == FFX_SURFACE_FORMAT_R16G16_FLOAT)
    {
        dx12CommandList->ClearUnorderedAccessViewFloat(dx12GpuHandle, dx12CpuHandle, dx12Resource, job->clearJobDescriptor.color, 0, nullptr);
    }
    else
    {
        uint32_t clearColorAsUint[4];
        clearColorAsUint[0] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[0]);
        clearColorAsUint[1] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[1]);
        clearColorAsUint[2] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[2]);
        clearColorAsUint[3] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[3]);
        dx12CommandList->ClearUnorderedAccessViewUint(dx12GpuHandle, dx12CpuHandle, dx12Resource, clearColorAsUint, 0, nullptr);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobDiscard(BackendContext_DX12* backendContext, FfxUInt32, FfxGpuJobDescription* job, ID3D12GraphicsCommandList* dx12CommandList)
{
    uint32_t idx = job->discardJobDescriptor.target.internalIndex;
    BackendContext_DX12::Resource ffxResource = backendContext->pResources[idx];
    ID3D12Resource* dx12Resource = reinterpret_cast<ID3D12Resource*>(ffxResource.resourcePtr);

    addBarrier(backendContext, &job->discardJobDescriptor.target, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    flushBarriers(backendContext, dx12CommandList);

    dx12CommandList->DiscardResource( dx12Resource, nullptr);

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsDX12(
    FfxInterface* backendInterface, 
    FfxCommandList commandList, 
    FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX12* backendContext = (BackendContext_DX12*)backendInterface->scratchBuffer;

    FFX_ASSERT(nullptr != commandList);
    ID3D12GraphicsCommandList* dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList*>(commandList);

    FfxErrorCode errorCode = FFX_OK;

    // execute all GpuJobs
    for (uint32_t currentGpuJobIndex = 0; currentGpuJobIndex < backendContext->gpuJobCount; ++currentGpuJobIndex) {

        FfxGpuJobDescription* GpuJob = &backendContext->pGpuJobs[currentGpuJobIndex];        

        // If we have a label for the job, drop a marker for it
        if (GpuJob->jobLabel[0]) {
            beginMarkerDX12(backendContext, dx12CommandList, GpuJob->jobLabel);
        }

        switch (GpuJob->jobType) {

            case FFX_GPU_JOB_CLEAR_FLOAT:
                errorCode = executeGpuJobClearFloat(backendContext, effectContextId, GpuJob, dx12CommandList);
                break;

            case FFX_GPU_JOB_COPY:
                errorCode = executeGpuJobCopy(backendContext, effectContextId, GpuJob, dx12CommandList);
                break;

            case FFX_GPU_JOB_COMPUTE:
                errorCode = executeGpuJobCompute(backendContext, GpuJob, dx12CommandList, effectContextId);
                break;

            case FFX_GPU_JOB_BARRIER:
                errorCode = executeGpuJobBarrier(backendContext, effectContextId, GpuJob, dx12CommandList);
                break;

            case FFX_GPU_JOB_DISCARD:
                errorCode = executeGpuJobDiscard(backendContext, effectContextId, GpuJob, dx12CommandList);
                break;

            default:
                break;
        }

        if (GpuJob->jobLabel[0]) {
            endMarkerDX12(backendContext, dx12CommandList);
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}

FfxErrorCode BreadcrumbsAllocBlockDX12(
    FfxInterface*,
    uint64_t,
    FfxBreadcrumbsBlockData*)
{
    FFX_ASSERT_MESSAGE(false, "Breadcrumbs not supported on Xbox platforms");
    return FFX_OK;
}

void BreadcrumbsFreeBlockDX12(
    FfxInterface*,
    FfxBreadcrumbsBlockData*)
{
    FFX_ASSERT_MESSAGE(false, "Breadcrumbs not supported on Xbox platforms");
}

void BreadcrumbsWriteDX12(
    FfxInterface*,
    FfxCommandList,
    uint32_t,
    uint64_t,
    void*,
    bool)
{
    FFX_ASSERT_MESSAGE(false, "Breadcrumbs not supported on Xbox platforms");
}

void BreadcrumbsPrintDeviceInfoDX12(
    FfxInterface*,
    FfxAllocationCallbacks*,
    bool,
    char**,
    size_t*)
{
    FFX_ASSERT_MESSAGE(false, "Breadcrumbs not supported on Xbox platforms");
}

void RegisterConstantBufferAllocatorDX12(FfxInterface*, FfxConstantBufferAllocator fpConstantAllocator)
{
    s_fpConstantAllocator = fpConstantAllocator;
}

FfxCommandQueue ffxGetCommandQueueDX12(ID3D12CommandQueue* pCommandQueue)
{
    FFX_ASSERT(nullptr != pCommandQueue);
    return reinterpret_cast<FfxCommandQueue>(pCommandQueue);
}

void ffxRegisterResourceAllocatorX(FfxResourceAllocatorFunc fpResourceAllocator)
{
    s_fpResourceAllocator = fpResourceAllocator;
}

void ffxRegisterResourceDestructorX(FfxResourceDestroyFunc fpResourceDestroyFunc)

{
    s_fpResourceDestroyFunc = fpResourceDestroyFunc;
}

