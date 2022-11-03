//--------------------------------------------------------------------------------------
// ResolutionSet.h
//
// Manages the resources for multiple resolutions, aliased over the same memory.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Contains the resources necessary to render to the aliased buffers for a particular resolution.
struct ResolutionData
{
    D3D12_CPU_DESCRIPTOR_HANDLE             rtvDescriptor;
    D3D12_CPU_DESCRIPTOR_HANDLE             dsvDescriptor;
    Microsoft::WRL::ComPtr<ID3D12Resource>  renderTarget;
    Microsoft::WRL::ComPtr<ID3D12Resource>  depthStencil;
};

// Manages a set of ResolutionData objects, each tracking resources for a different resolution,
// aliased over a shared memory allocation.
class ResolutionSet
{
public:
    ResolutionSet()
        : m_device(nullptr)
    {};

    ResolutionData InitializeMaxResolution(
        ID3D12Device* device,
        uint32_t maxWidth,
        uint32_t maxHeight,
        uint32_t maxResolutions,
        DXGI_FORMAT colorFormat,
        DXGI_FORMAT depthFormat);

    ResolutionData GetOrCreateResourcesForResolution(
        uint32_t width,
        uint32_t height);

private:
    ResolutionData CreateResourcesForResolution(
        uint32_t width,
        uint32_t height,
        bool isMaxResolution
    );

    typedef std::pair<uint32_t, uint32_t>               Resolution;
    struct ResolutionHash
    {
        size_t operator() (const Resolution& resolution) const
        {
            // Simple hash that shouldn't collide for any reasonable resolutions
            return resolution.first * 8192ull + resolution.second;
        }
    };
    // Stores a set of ResolutionData objects, identified by width x height pairs
    std::unordered_map<Resolution, ResolutionData, ResolutionHash> m_rezMap;

    ID3D12Device*                                       m_device;
    DXGI_FORMAT                                         m_colorFormat;
    DXGI_FORMAT                                         m_depthFormat;
    D3D12_GPU_VIRTUAL_ADDRESS                           m_rtAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS                           m_dsAddress = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
    int32_t                                             m_nextDescriptorIndex = 0;
};
