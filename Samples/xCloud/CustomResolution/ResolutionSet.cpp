//--------------------------------------------------------------------------------------
// ResolutionSet.cpp
//
// Manages the resources for multiple resolutions, aliased over the same memory.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "pch.h"

#include "ResolutionSet.h"

// Create all the resources needed for a ResolutionSet, including the targets for the largest resolution.
// Returns the ResolutionData object for the largest resolution.
ResolutionData ResolutionSet::InitializeMaxResolution(
    ID3D12Device* device,
    uint32_t maxWidth,
    uint32_t maxHeight,
    uint32_t maxResolutions,
    DXGI_FORMAT colorFormat,
    DXGI_FORMAT depthFormat)
{
    if (m_device != nullptr)
    {
        DX::ThrowIfFailed(E_NOT_VALID_STATE);
    }

    if (device == nullptr)
    {
        DX::ThrowIfFailed(E_INVALIDARG);
    }

    m_device = device;
    m_colorFormat = colorFormat;
    m_depthFormat = depthFormat;

    // Heaps dedicated to this resolution set
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxResolutions;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    m_rtvDescriptorHeap->SetName(L"ResolutionSet RTV heap");

    if (m_depthFormat != DXGI_FORMAT_UNKNOWN)
    {
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));
        m_dsvDescriptorHeap->SetName(L"ResolutionSet DSV heap");
    }

    // The starting resources must be big enough for the largest possible resolution
    return CreateResourcesForResolution(maxWidth, maxHeight, true);
}

// Get the ResolutionData object for the given resolution. If it doesn't already exist, create it
// along with placed resources to render into.
ResolutionData ResolutionSet::GetOrCreateResourcesForResolution(uint32_t width, uint32_t height)
{
    if (m_device == nullptr)
    {
        DX::ThrowIfFailed(E_NOT_VALID_STATE);
    }

    auto iter = m_rezMap.find(std::make_pair(width, height));
    if (iter != m_rezMap.end())
    {
        return iter->second;
    }

    // The resolution does not already exist, so we need to create resources for it
    return CreateResourcesForResolution(width, height, false);
}

// Create the resources for a particular resolution. If it's the maximum, these will be new allocations,
// and otherwise they'll be aliased over the same memory as the max resolution.
ResolutionData ResolutionSet::CreateResourcesForResolution(
    uint32_t width,
    uint32_t height,
    bool isMaxResolution)
{
    ResolutionData newRezData = {};

    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_colorFormat,
        width,
        height,
        1,
        1
    );
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = m_colorFormat;

    if (isMaxResolution)
    {
        // Commit the actual memory for the resource, which will be aliased by the other resolutions
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &optimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(newRezData.renderTarget.ReleaseAndGetAddressOf())));

        m_rtAddress = newRezData.renderTarget->GetGPUVirtualAddress();
    }
    else
    {
        // Only one resolution is active at a time, so they all share the memory of the largest size
        DX::ThrowIfFailed(m_device->CreatePlacedResourceX(
            m_rtAddress,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &optimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(newRezData.renderTarget.ReleaseAndGetAddressOf())));
    }

    wchar_t name[64] = {};
    swprintf_s(name, L"Render target %u x %u", width, height);
    newRezData.renderTarget->SetName(name);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = m_colorFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    newRezData.rtvDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_nextDescriptorIndex,
        m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_device->CreateRenderTargetView(newRezData.renderTarget.Get(), &rtvDesc, newRezData.rtvDescriptor);

    if (m_depthFormat != DXGI_FORMAT_UNKNOWN)
    {
        resourceDesc.Format = m_depthFormat;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = m_depthFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        if (isMaxResolution)
        {
            // Commit the actual memory for the resource, which will be aliased by the other resolutions
            CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

            DX::ThrowIfFailed(m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(newRezData.depthStencil.ReleaseAndGetAddressOf())));

            // Save this for creating placed resources later
            m_dsAddress = newRezData.depthStencil->GetGPUVirtualAddress();
        }
        else
        {
            // Only one resolution is active at a time, so they all share the memory of the largest size
            DX::ThrowIfFailed(m_device->CreatePlacedResourceX(
                m_dsAddress,
                &resourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(newRezData.depthStencil.ReleaseAndGetAddressOf())
            ));
        }

        swprintf_s(name, L"Depth stencil %u x %u", width, height);
        newRezData.depthStencil->SetName(name);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_depthFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        newRezData.dsvDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_nextDescriptorIndex,
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
        m_device->CreateDepthStencilView(newRezData.depthStencil.Get(), &dsvDesc, newRezData.dsvDescriptor);
    }

    Resolution rez = std::make_pair(width, height);
    m_rezMap.insert(std::make_pair(rez, newRezData));

    m_nextDescriptorIndex++;

    return newRezData;
}
