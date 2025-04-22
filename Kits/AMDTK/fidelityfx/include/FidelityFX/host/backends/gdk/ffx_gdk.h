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

/// @defgroup GDKBackend GDK Backend
/// FidelityFX SDK native backend implementation for Microsoft GDK.
/// 
/// @ingroup Backends

/// @defgroup GDKFrameInterpolation GDK FrameInterpolation
/// FidelityFX SDK native frame interpolation implementation for GDK backend.
/// 
/// @ingroup GDKBackend

#pragma once

#if !defined(_GAMING_XBOX)
#pragma error "GDK backend is only meant for XBOX gaming targets"
#endif // !defined(_GAMING_XBOX)

#if defined(_GAMING_XBOX_SCARLETT)

#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#ifndef IID_GDK_PPV_ARGS
#define IID_GDK_PPV_ARGS(x) IID_GRAPHICS_PPV_ARGS(x)
#endif // IID_GDK_PPV_ARGS

#elif defined(_GAMING_XBOX_XBOXONE)

#include <d3d12_x.h>
#include <d3dx12_x.h>
#ifndef IID_GDK_PPV_ARGS
#define IID_GDK_PPV_ARGS(x) IID_GRAPHICS_PPV_ARGS(x)
#endif // IID_GDK_PPV_ARGS

#else

#pragma error "Unsupported XBOX gaming target"

#endif // #ifdef FFX_API_GDK_SCARLETT

#include <FidelityFX/host/ffx_interface.h>

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

    /// A structure encapsulating the data sent to the FfxPresentXFunc callback
    ///
    /// @ingroup GDKBackend
    typedef struct FfxPresentXParams {
        ID3D12CommandQueue*                 presentQueue;               ///< The <c><i>ID3D12CommandQueue</i></c> to call PresentX on
        D3D12XBOX_FRAME_PIPELINE_TOKEN      framePipelineTokenOriginal; ///< The <c><i>D3D12XBOX_FRAME_PIPELINE_TOKEN</i></c> for which ffxPresentX was called
        D3D12XBOX_FRAME_PIPELINE_TOKEN      framePipelineTokenToSubmit; ///< The <c><i>D3D12XBOX_FRAME_PIPELINE_TOKEN</i></c> which must be passed into PresentX

        ID3D12Resource *                    sceneBackBufferToPresent;   ///< The <c><i>ID3D12Resource</i></c> representing scene back buffer with optional UI that must be passed into PresentX
        ID3D12Resource *                    uiBackBufferToPresent;      ///< The optional <c><i>ID3D12Resource</i></c> that contains UI (in case if <c><i>sceneBackBufferToPresent</i></c> doesn't contain UI) that must be passed into PresentX

        bool                                interpolatedFrame;          ///< Whether this frame was generated or not
        void *                              presentContext;             ///< A pointer to be passed to the <c><i>FfxPresentXFunc</i></c> callback
    } FfxPresentXParams;

    /// PresentX callback function.
    ///
    /// The callback function to be used when <c><i>FrameInterpolationSwapChain</i></c> is ready to call PresentX().
    ///
    /// @param [in]  bufferIndex        Index of the application back buffer resource to present.
    ///
    /// @retval
    /// None.
    ///
    /// @ingroup GDKBackend
    typedef void (*FfxPresentXFunc)(FfxPresentXParams* ffxPresentXParams);

    /// Resource allocation callback function.
    ///
    /// The callback function to be used when creating committed resources for a specified effect
    /// 
    /// @param [in]  ffxEffect
    /// @param [in]  initialState
    /// @param [in]  pD3DDesc
    /// @param [in]  pFfxDesc
    /// @param [in]  pOptimizedClear
    /// @param [out] ppD3DResource
    /// 
    /// @returns
    /// FFX_OK on success, or relevant error code otherwise
    typedef FfxErrorCode (*FfxResourceAllocatorFunc)(FfxEffect ffxEffect, D3D12_RESOURCE_STATES initialState, const D3D12_HEAP_PROPERTIES* pHeapProps,
                                    const D3D12_RESOURCE_DESC* pD3DDesc, const FfxResourceDescription* pFfxDesc,
                                    const D3D12_CLEAR_VALUE* pOptimizedClear, ID3D12Resource** ppD3DResource);

    /// Resource destruction callback function.
    ///
    /// The callback function to be used when releasing a committed resource for a specified effect
    /// 
    /// @param [in]  ffxEffect          The effect this resource was allocated for
    /// @param [in]  pResource          The resource to release
    /// 
    /// @returns
    /// FFX_OK on success, or relevant error code otherwise
    typedef FfxErrorCode(*FfxResourceDestroyFunc)(FfxEffect ffxEffect, ID3D12Resource* pResource);


    typedef struct FfxExecuteGPUJobParams
    {
        ID3D12Resource*     pResource;          ///< The D3D resource which will take part in the GPU job.
        uint32_t            internalResource;   ///< Non-zero if the resource is an internal FidelityFX resource.
        FfxResourceStates   resourceState;      ///< Current resource state of the D3DResource (The resource MUST return from callback in the same state)
    }FfxExecuteGPUJobParams;
    
    /// Gpu job execution callback.
    ///
    /// The callback function invoked prior to setting up all resources for the gpu job identified
    /// 
    /// @param [in]  ffxEffect          The effect this job is part of
    /// @param [in]  ffxJobType         The type of job this is (i.e. COMPUTE, CLEAR, COPY, etc.)
    /// @param [in]  ffxEffectPass      The internal effect pass id (will be 0xffffffff for anything but COMPUTE job types)
    /// @param [in]  pParams            The list of FfxExecuteGPUJobParams (see above)
    /// @param [in]  numParams          The number of parameters in pParams.
    /// 
    /// @returns
    /// FFX_OK on success, or relevant error code otherwise
    typedef FfxErrorCode(*FfxExecuteGPUJobCallbackFunc)(FfxEffect ffxEffect, FfxGpuJobType ffxJobType, FfxPass ffxEffectPass,
                                                        FfxExecuteGPUJobParams* pParams, size_t numParams);

    /// Query how much memory is required for the DirectX 12 backend's scratch buffer.
    /// 
    /// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
    ///                                         (Note that some effects contain internal contexts which count towards this maximum)
    ///
    /// @returns
    /// The size (in bytes) of the required scratch memory buffer for the DX12 backend.
    /// @ingroup GDKBackend
    FFX_API size_t ffxGetScratchMemorySizeDX12(size_t maxContexts);

    /// Create a <c><i>FfxDevice</i></c> from a <c><i>ID3D12Device</i></c>.
    ///
    /// @param [in] device                      A pointer to the DirectX12 device.
    ///
    /// @returns
    /// An abstract FidelityFX device.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxDevice ffxGetDeviceDX12(ID3D12Device* device);

    /// Populate an interface with pointers for the DX12 backend.
    ///
    /// @param [out] backendInterface           A pointer to a <c><i>FfxInterface</i></c> structure to populate with pointers.
    /// @param [in] device                      A pointer to the DirectX12 device.
    /// @param [in] scratchBuffer               A pointer to a buffer of memory which can be used by the DirectX(R)12 backend.
    /// @param [in] scratchBufferSize           The size (in bytes) of the buffer pointed to by <c><i>scratchBuffer</i></c>.
    /// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
    ///                                         (Note that some effects contain internal contexts which count towards this maximum)
    ///
    /// @retval
    /// FFX_OK                                  The operation completed successfully.
    /// @retval
    /// FFX_ERROR_CODE_INVALID_POINTER          The <c><i>interface</i></c> pointer was <c><i>NULL</i></c>.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxErrorCode ffxGetInterfaceDX12(
        FfxInterface* backendInterface,
        FfxDevice device,
        void* scratchBuffer,
        size_t scratchBufferSize,
        size_t maxContexts);

    /// Create a <c><i>FfxCommandList</i></c> from a <c><i>ID3D12CommandList</i></c>.
    ///
    /// @param [in] cmdList                     A pointer to the DirectX12 command list.
    ///
    /// @returns
    /// An abstract FidelityFX command list.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxCommandList ffxGetCommandListDX12(ID3D12CommandList* cmdList);

    /// Create a <c><i>FfxPipeline</i></c> from a <c><i>ID3D12PipelineState</i></c>.
    ///
    /// @param [in] pipelineState               A pointer to the DirectX12 pipeline state.
    ///
    /// @returns
    /// An abstract FidelityFX pipeline.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxPipeline ffxGetPipelineDX12(ID3D12PipelineState* pipelineState);

    /// Fetch a <c><i>FfxResource</i></c> from a <c><i>GPUResource</i></c>.
    ///
    /// @param [in] dx12Resource                A pointer to the DX12 resource.
    /// @param [in] ffxResDescription           An <c><i>FfxResourceDescription</i></c> for the resource representation.
    /// @param [in] ffxResName                  (optional) A name string to identify the resource in debug mode.
    /// @param [in] state                       The state the resource is currently in.
    ///
    /// @returns
    /// An abstract FidelityFX resources.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxResource ffxGetResourceDX12(const ID3D12Resource* dx12Resource,
        FfxResourceDescription ffxResDescription,
        const wchar_t* ffxResName,
        FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ);


    /// Fetch a <c><i>FfxSurfaceFormat</i></c> from a DXGI_FORMAT.
    ///
    /// @param [in] format              The DXGI_FORMAT to convert to <c><i>FfxSurfaceFormat</i></c>.
    ///
    /// @returns
    /// An <c><i>FfxSurfaceFormat</i></c>.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxSurfaceFormat ffxGetSurfaceFormatDX12(DXGI_FORMAT format);

    /// Fetch a <c><i>FfxResourceDescription</i></c> from an existing ID3D12Resource.
    ///
    /// @param [in] pResource           The ID3D12Resource resource to create a <c><i>FfxResourceDescription</i></c> for.
    /// @param [in] additionalUsages    Optional <c><i>FfxResourceUsage</i></c> flags needed for select resource mapping.
    ///
    /// @returns
    /// An <c><i>FfxResourceDescription</i></c>.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxResourceDescription ffxGetResourceDescriptionDX12(const ID3D12Resource* pResource, FfxResourceUsage additionalUsages = FFX_RESOURCE_USAGE_READ_ONLY);

    /// Fetch a <c><i>FfxCommandQueue</i></c> from an existing ID3D12CommandQueue.
    ///
    /// @param [in] pCommandQueue       The ID3D12CommandQueue to create a <c><i>FfxCommandQueue</i></c> from.
    ///
    /// @returns
    /// An <c><i>FfxCommandQueue</i></c>.
    ///
    /// @ingroup GDKBackend
    FFX_API FfxCommandQueue ffxGetCommandQueueDX12(ID3D12CommandQueue* pCommandQueue);

    FFX_API void ffxRegisterResourceAllocatorX(FfxResourceAllocatorFunc fpResourceAllocator);
    FFX_API void ffxRegisterResourceDestructorX(FfxResourceDestroyFunc fpResourceDestroyFunc);

    /// Creates a <c><i>FfxSwapchain</i></c> from passed in parameters. On console, the FfxSwapChain struct represents an overloaded command queue.
    ///
    /// @param [in] gameQueue               The ID3D12CommandQueue (graphics) from the calling application.
    /// @param [in] computeQueue            The ID3D12CommandQueue (compute) from the calling application to use for async workloads (see FfxFrameGenerationConfig to enable compute workloads).
    /// @param [in] presentQueue            The ID3D12CommandQueue (compute) from the calling application to use for async presentation (see FfxFrameGenerationConfig to enable asyncPresent).
    /// @param [in] swapChainResourceDesc	The D3D12_RESOURCE_DESC describing the presentation buffer resources.
    /// @param [out] outGameSwapChain       The created <c><i>FfxSwapchain</i></c>.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    /// @retval
    /// FFX_ERROR_INVALID_ARGUMENT          One of the parameters is invalid.
    /// FFX_ERROR_OUT_OF_MEMORY             Insufficient memory available to allocate <c><i>FfxSwapchain</i></c> or underlying component.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxCreateFrameinterpolationSwapchainX(FfxCommandQueue gameQueue,
        FfxCommandQueue computeQueue,
        FfxCommandQueue presentQueue,
        FfxPresentXFunc gamePresentXCallback,
        void *gamePresentXCallbackContext,
        FfxSwapchain& outGameSwapChain);

    /// Release a <c><i>FfxSwapchain</i></c> when done with it.
    ///
    /// @param [in] gameSwapChain			The <c><i>FfxSwapchain</i></c> to release.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxDestroyFrameinterpolationSwapchainX(FfxSwapchain gameSwapChain);

    /// Will wait until the PresentX call on the real frame buffer has been called. This is
    /// to control the pacing in calls to WaitFrameEventX
    ///
    /// @param [in] gameSwapChain			The <c><i>FfxSwapchain</i></c> object to query for present completion.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxWaitForPresentX(FfxSwapchain gameSwapChain);

    /// Replacement for PresentX to be called at the end of the frame when frame interpolation is enabled.
    /// This will either call the application callback for presentX presentation directly or will schedule
    /// frame interpolation to occur and pass presentation calls off to an asynchronous thread.
    ///
    /// @param [in] gameSwapChain               The <c><i>FfxSwapchain</i></c> to use for presentation.
    /// @param [in] realBackBufferPlane         The <c><i>ID3D12Resource</i></c> representing the back buffer prepared by the title
    /// @param [in] interpolatedBackBufferPlane The <c><i>ID3D12Resource</i></c> representing the back buffer where interpolated data will be written
    ///
    /// @retval
    /// FFX_OK                                  The operation completed successfully.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxPresentX(FfxSwapchain gameSwapChain,
                                     D3D12XBOX_FRAME_PIPELINE_TOKEN realBackBufferToken,
                                     FfxResource realBackBufferPlane,
                                     FfxResource interpolatedBackBufferPlane,
                                     FfxResource uiBackBufferPlane);

    /// Registers a <c><i>FfxResource</i></c> to use for UI with the provided <c><i>FfxSwapchain</i></c>.
    ///
    /// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to to register the UI resource with.
    /// @param [in] uiResource              The <c><i>FfxResource</i></c> representing the UI resource.
    /// @param [in] flags                   A set of <c><i>FfxUiCompositionFlags</i></c>.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    /// @retval
    /// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxRegisterFrameinterpolationUiResourceDX12(FfxSwapchain gameSwapChain, FfxResource uiResource, uint32_t flags);

    /// Fetches a <c><i>FfxCommandList</i></c> from the <c><i>FfxSwapchain</i></c>.
    ///
    /// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to get a <c><i>FfxCommandList</i></c> from.
    /// @param [out] gameCommandlist        The <c><i>FfxCommandList</i></c> from the provided <c><i>FfxSwapchain</i></c>.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    /// @retval
    /// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxGetFrameinterpolationCommandlistDX12(FfxSwapchain gameSwapChain, FfxCommandList& gameCommandlist);

    /// Fetches a <c><i>FfxResource</i></c>  representing the backbuffer from the <c><i>FfxSwapchain</i></c>.
    ///
    /// @param [in] gameSwapChain           The <c><i>FfxSwapchain</i></c> to get a <c><i>FfxResource</i></c> backbuffer from.
    ///
    /// @returns
    /// An abstract FidelityFX resources for the swapchain backbuffer.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxResource ffxGetFrameinterpolationTextureDX12(FfxSwapchain gameSwapChain);

    /// Sets a <c><i>FfxFrameGenerationConfig</i></c> to the internal FrameInterpolationSwapChain (in the backend).
    ///
    /// @param [in] config                  The <c><i>FfxFrameGenerationConfig</i></c> to set.
    ///
    /// @retval
    /// FFX_OK                              The operation completed successfully.
    /// @retval
    /// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
    ///
    /// @ingroup GDKFrameInterpolation
    FFX_API FfxErrorCode ffxSetFrameGenerationConfigToSwapchainDX12(FfxFrameGenerationConfig const* config);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
