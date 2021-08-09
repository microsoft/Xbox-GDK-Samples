//--------------------------------------------------------------------------------------
// IResourceUploader.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <d3d12.h>
#include <inttypes.h>

// Example interface which performs functionality for managing upload heaps and copies to D3D12 resources.
class IResourceUploader
{
public:
    // This function is expected to:
    // - Allocate memory from an upload heap
    // - Local copy from 'data' to the allocation
    // - Schedule and execute a copy operation on a command list
    // - Use a fence to manage the lifetime of the upload heap resource
    virtual void Upload(ID3D12Resource* dest, void* data, uint32_t byteSize) = 0;

    // Helper function to transition copied resources
    virtual void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) = 0;
};
