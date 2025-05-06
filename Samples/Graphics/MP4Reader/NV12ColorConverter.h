//--------------------------------------------------------------------------------------
// NV12ColorConverter.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class CXboxNV12ToRGBConverter
{
public:
    HRESULT Initialize(_In_ ID3D12Device* pDevice, _In_ _In_opt_ IMFAttributes* pProperty);
    void Release() { delete this; }

    static HRESULT CreateInstance(_In_ ID3D12Device* pDevice,  _In_opt_ IMFAttributes* pProperty, _Outptr_ void** ppvObject);       

    HRESULT RenderDecodedSampleToResource(
        _Inout_ ID3D12GraphicsCommandList* pCmdList,
        _Inout_ ID3D12CommandQueue* pCmdQueue,
        _In_ IMFSample* pSample,
        _In_ uint32_t videoWidth,
        _In_ uint32_t videoHeight,
        _Out_ ID3D12Resource* pOutputResource);

    ID3D12CommandQueue* GetVideoProcessCommandQueue() const { return m_pVpCommandQueue.Get(); }

public:
    Microsoft::WRL::ComPtr<ID3D12Device> m_spDevice;

protected:    
    Microsoft::WRL::ComPtr<ID3D12VideoProcessCommandList> m_pVpCommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pVpCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pVpCommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12VideoProcessor> m_pVideoProcessor;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_pVpFence;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pTargetResource;

    uint64_t m_vpFenceValue = 1;
};

STDAPI CreateDxvaSampleRendererX(_In_ ID3D12Device* pDevice, _In_opt_ IMFAttributes* pAttribute, _Outptr_ CXboxNV12ToRGBConverter** pObject);
