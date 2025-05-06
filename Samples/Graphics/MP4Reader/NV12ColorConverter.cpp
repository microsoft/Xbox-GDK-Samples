//--------------------------------------------------------------------------------------
// NV12ColorConverter.cpp
// 
// Color convertion classes for NV12->RGB and RGB->NV12.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NV12ColorConverter.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

const UINT g_MaxVideoWidth = 3840;
const UINT g_MaxVideoHeight = 2160;

/////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
STDAPI CreateDxvaSampleRendererX(ID3D12Device* pDevice, IMFAttributes* pAttribute, CXboxNV12ToRGBConverter** pObject)
{
    return CXboxNV12ToRGBConverter::CreateInstance(pDevice, pAttribute, (void**)pObject);
}


/////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
HRESULT CXboxNV12ToRGBConverter::CreateInstance(ID3D12Device* pDevice, IMFAttributes* pAttribute, void** ppvObject)
{
    if (!pDevice || !ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;

    auto spVP = new (std::nothrow) CXboxNV12ToRGBConverter();
    if (!spVP)
        return E_OUTOFMEMORY;

    HRESULT hr = spVP->Initialize(pDevice, pAttribute);
    if (FAILED(hr))
        return hr;

    *ppvObject = spVP;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
HRESULT CXboxNV12ToRGBConverter::Initialize(ID3D12Device* pDevice, IMFAttributes* pAttribute)
{
    UNREFERENCED_PARAMETER(pAttribute);
    if (!pDevice)
        return E_INVALIDARG;

    m_spDevice = ComPtr<ID3D12Device>(pDevice);

    HRESULT hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, IID_GRAPHICS_PPV_ARGS(m_pVpCommandAllocator.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    D3D12_COMMAND_QUEUE_DESC descQueue{};
    descQueue.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
    descQueue.Priority = 0;
    descQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = pDevice->CreateCommandQueue(&descQueue, IID_GRAPHICS_PPV_ARGS(m_pVpCommandQueue.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, m_pVpCommandAllocator.Get(), nullptr, __uuidof(ID3D12CommandList), reinterpret_cast<void**>(m_pVpCommandList.GetAddressOf()));
    if (FAILED(hr))
        return hr;
		
    hr = m_pVpCommandList->Close();
    if (FAILED(hr))
        return hr;

    // The new Video Processor for Microsoft GDK with Xbox extensions
    ComPtr<ID3D12VideoDevice> pVideoDevice;
    hr = pDevice->QueryInterface(IID_GRAPHICS_PPV_ARGS(pVideoDevice.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC inputStreamDesc{};
    inputStreamDesc.Format = DXGI_FORMAT_NV12;
    inputStreamDesc.ColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
    inputStreamDesc.SourceSizeRange = D3D12_VIDEO_SIZE_RANGE{ g_MaxVideoWidth, g_MaxVideoHeight, 1, 1 };
    inputStreamDesc.DestinationSizeRange = D3D12_VIDEO_SIZE_RANGE{ g_MaxVideoWidth, g_MaxVideoHeight, 1, 1 };

    D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC outputStreamDesc{};
    outputStreamDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    outputStreamDesc.ColorSpace = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709;

    hr = pVideoDevice->CreateVideoProcessor(0, &outputStreamDesc, 1, &inputStreamDesc, IID_GRAPHICS_PPV_ARGS(m_pVideoProcessor.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    hr = pDevice->CreateFence(++m_vpFenceValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_pVpFence.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
HRESULT CXboxNV12ToRGBConverter::RenderDecodedSampleToResource(
    ID3D12GraphicsCommandList* pCmdList,
    ID3D12CommandQueue* pCmdQueue,
    IMFSample* pSample,
    uint32_t videoWidth,
    uint32_t videoHeight,
    ID3D12Resource* pOutputResource)
{
    if (!pCmdList || !pCmdQueue || !pSample || !pOutputResource || videoWidth == 0 || videoHeight == 0)
    {
        return E_INVALIDARG;
    }

    DWORD bufferCount = 0;
    HRESULT hr = pSample->GetBufferCount(&bufferCount);
    if (FAILED(hr))
        return hr;
    assert(bufferCount == 1);

    ComPtr<IMFMediaBuffer> spBuffer;
    hr = pSample->GetBufferByIndex(0, spBuffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    ComPtr<IMFDXGIBuffer> spDXGIBuffer;
    hr = spBuffer.Get()->QueryInterface(spDXGIBuffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    ComPtr<ID3D12Resource> spResourceTexture;
    hr = spDXGIBuffer->GetResource(IID_GRAPHICS_PPV_ARGS(spResourceTexture.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    UINT32 uiIndexSrc;
    hr = spDXGIBuffer->GetSubresourceIndex(&uiIndexSrc);
    if (FAILED(hr))
        return hr;

    const auto targetResourceDesc = pOutputResource->GetDesc();

    // Create a temporary target for the video processor since DX12 does not allow writing directly to the swapchain
    // unless the command queue has ownership
    if (m_pTargetResource.Get() == nullptr)
    {
        const D3D12_HEAP_PROPERTIES D3D12heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_STATES tmpState = D3D12_RESOURCE_STATE_COPY_SOURCE| D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        D3D12_HEAP_FLAGS outputHeapFlags = D3D12_HEAP_FLAG_NONE;

        hr = m_spDevice->CreateCommittedResource(
            &D3D12heapProperties,
            outputHeapFlags,
            &targetResourceDesc,
            tmpState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_pTargetResource.ReleaseAndGetAddressOf()));

        if (FAILED(hr))
            return hr;
    }

    D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS inputArgs {
        {
            {
                spResourceTexture.Get(),
                uiIndexSrc,
                D3D12_VIDEO_PROCESS_REFERENCE_SET{}
            },
            {
                nullptr,
                0,
                D3D12_VIDEO_PROCESS_REFERENCE_SET{}
            },
        },
        D3D12_VIDEO_PROCESS_TRANSFORM { { 0 ,0, (LONG)videoWidth, (LONG)videoHeight},
                                        { 0 ,0, (LONG)targetResourceDesc.Width, (LONG)targetResourceDesc.Height } },
                                        D3D12_VIDEO_PROCESS_INPUT_STREAM_FLAG_NONE,
                                        D3D12_VIDEO_PROCESS_INPUT_STREAM_RATE {},
                                        {}, // Filter Levels
                                        D3D12_VIDEO_PROCESS_ALPHA_BLENDING {}
    };

    D3D12_VIDEO_PROCESS_OUTPUT_STREAM_ARGUMENTS outputArgs {
        {
            m_pTargetResource.Get(),
            0, // Output subresource index
        },
        CD3DX12_RECT(0, 0, (LONG)targetResourceDesc.Width, (LONG)targetResourceDesc.Height),
    };

    CD3DX12_RESOURCE_BARRIER preVpResourceBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(spResourceTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ, uiIndexSrc),
        CD3DX12_RESOURCE_BARRIER::Transition(m_pTargetResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE),
    };

    CD3DX12_RESOURCE_BARRIER postVpResourceBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(spResourceTexture.Get(), D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ, D3D12_RESOURCE_STATE_COMMON, uiIndexSrc),
        CD3DX12_RESOURCE_BARRIER::Transition(m_pTargetResource.Get(),  D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    // Wait on any pending work if needed since we only have one allocator
    if (m_pVpFence->GetCompletedValue() < m_vpFenceValue)
    {
        HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        m_pVpFence->SetEventOnCompletion(m_vpFenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }

    m_pVpCommandAllocator->Reset();
    m_pVpCommandList->Reset(m_pVpCommandAllocator.Get());

    m_pVpCommandList->ResourceBarrier(ARRAYSIZE(preVpResourceBarriers), preVpResourceBarriers);
    m_pVpCommandList->ProcessFrames(m_pVideoProcessor.Get(), &outputArgs, 1, &inputArgs);
    m_pVpCommandList->ResourceBarrier(ARRAYSIZE(postVpResourceBarriers), postVpResourceBarriers);

    m_pVpCommandList->Close();
    m_pVpCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)m_pVpCommandList.GetAddressOf());

    // Signal the fence when its complete
    const auto fenceValue = ++m_vpFenceValue;
    m_pVpCommandQueue->Signal(m_pVpFence.Get(), fenceValue);

    // Ask the graphics queue to wait till the VP operation is complete before copying to the output
    pCmdQueue->Wait(m_pVpFence.Get(), fenceValue);
    D3D12_RESOURCE_BARRIER resBarrier = {};

    resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resBarrier.Transition.pResource = pOutputResource;
    resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    
    pCmdList->ResourceBarrier(1, &resBarrier);
    pCmdList->CopyResource(pOutputResource, m_pTargetResource.Get());

    resBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    pCmdList->ResourceBarrier(1, &resBarrier);

    return S_OK;
}
