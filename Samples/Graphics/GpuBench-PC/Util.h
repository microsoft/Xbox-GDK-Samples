#pragma once

#define SET_NAME_TO_SELF(a) (a)->SetName(L#a) 

// There's no way to un-set these names, so we will eventually exhaust the fence name registry.
// To fix this, we'd need a scoped version like the ones below.
#define SET_FENCE_NAME_TO_SELF(device, address) (device)->RegisterCustomFenceLocationX((address), L#address) 

#define _CONCATENATE(a,b) (a##b) 
#define CONCATENATE(a,b) _CONCATENATE(a,b) 
#define SCOPED_DEBUG_FLAGS(device, setFlags, unsetFlags) ScopedDebugFlags CONCATENATE(scopedDebugFlags,__LINE__)((device), (setFlags), (unsetFlags))
#define SCOPED_ERROR_FILTER(device, id) ScopedErrorFilter CONCATENATE(scopedErrorFilter,__LINE__)((device), (id))

#ifdef _GAMING_XBOX
class ScopedDebugFlags
{
public:
    ScopedDebugFlags(ID3D12Device* _device, D3D12XBOX_DEBUG_FLAGS setFlags, D3D12XBOX_DEBUG_FLAGS unsetFlags)
        : device(_device)
    {
        m_oldFlags = device->GetDebugFlagsX();
        device->SetDebugFlagsX((m_oldFlags | setFlags) & ~unsetFlags);
    };

    ~ScopedDebugFlags()
    {
        device->SetDebugFlagsX(m_oldFlags);
    }

private:
    ID3D12Device* device;
    D3D12XBOX_DEBUG_FLAGS m_oldFlags;
};

class ScopedErrorFilter
{
public:
    ScopedErrorFilter(ID3D12Device* _device, UINT _id, D3D12XBOX_DEBUG_FILTER_FLAGS flags = D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_FAILURE)
        : device(_device)
        , id(_id)
    {
        device->SetDebugErrorFilterX(id, flags);
    };

    ~ScopedErrorFilter()
    {
        device->SetDebugErrorFilterX(id, D3D12XBOX_DEBUG_FILTER_FLAG_NONE);
    }

private:
    ID3D12Device* device;
    UINT id;
};


inline void NullDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t descriptorIncrementSize)
{
    // Xbox-specific: CPU address is found by masking out upper bits
    // Xbox-specific: A descriptor of 0 is valid
    auto address = descriptor.ptr;
    address &= 0x00000000ffffffff;  
    memset((void*)address, 0, descriptorIncrementSize);
}

#else // ifdef _GAMING_DESKTOP

inline void NullDescriptorTypedBufferUav(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device* device)
{
    // create valid (Typed) Buffer descriptor with arbitrary parameters but valid format
    D3D12_UNORDERED_ACCESS_VIEW_DESC bufferUavDesc = {};
    bufferUavDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferUavDesc.ViewDimension         = D3D12_UAV_DIMENSION_BUFFER;
    bufferUavDesc.Buffer.NumElements    = 1;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateUnorderedAccessView(nullptr, nullptr, &bufferUavDesc, descriptor);
}

inline void NullDescriptorRawBufferUav(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device* device)
{
    // create valid (Raw) Buffer descriptor with arbitrary parameters but valid flag/format
    D3D12_UNORDERED_ACCESS_VIEW_DESC bufferUavDesc = {};
    bufferUavDesc.Format                = DXGI_FORMAT_R32_TYPELESS;
    bufferUavDesc.ViewDimension         = D3D12_UAV_DIMENSION_BUFFER;
    bufferUavDesc.Buffer.NumElements    = 1;
    bufferUavDesc.Buffer.Flags          = D3D12_BUFFER_UAV_FLAG_RAW;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateUnorderedAccessView(nullptr, nullptr, &bufferUavDesc, descriptor);
}

inline void NullDescriptorStructuredBufferUav(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device *device)
{
    // create valid (Structured) Buffer descriptor with arbitrary parameters but valid stride
    D3D12_UNORDERED_ACCESS_VIEW_DESC bufferUavDesc = {};
    bufferUavDesc.Format                        = DXGI_FORMAT_UNKNOWN;
    bufferUavDesc.ViewDimension                 = D3D12_UAV_DIMENSION_BUFFER;
    bufferUavDesc.Buffer.NumElements            = 1;
    bufferUavDesc.Buffer.StructureByteStride    = 4;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateUnorderedAccessView(nullptr, nullptr, &bufferUavDesc, descriptor);
}

inline void NullDescriptorTypedBufferSrv(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device* device)
{
    // create valid (Typed) Buffer descriptor with arbitrary parameters but valid format
    D3D12_SHADER_RESOURCE_VIEW_DESC bufferSrvDesc = {};
    bufferSrvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferSrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_BUFFER;
    bufferSrvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    bufferSrvDesc.Buffer.NumElements        = 1;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateShaderResourceView(nullptr, &bufferSrvDesc, descriptor);
}

inline void NullDescriptorRawBufferSrv(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device* device)
{
    // create valid (Raw) Buffer descriptor with arbitrary parameters but valid flag/format
    D3D12_SHADER_RESOURCE_VIEW_DESC bufferSrvDesc = {};
    bufferSrvDesc.Format                    = DXGI_FORMAT_R32_TYPELESS;
    bufferSrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_BUFFER;
    bufferSrvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    bufferSrvDesc.Buffer.NumElements        = 1;
    bufferSrvDesc.Buffer.Flags              = D3D12_BUFFER_SRV_FLAG_RAW;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateShaderResourceView(nullptr, &bufferSrvDesc, descriptor);
}

inline void NullDescriptorBufferSrv(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device *device)
{
    // create valid (Structured) Buffer descriptor with arbitrary parameters but valid stride
    D3D12_SHADER_RESOURCE_VIEW_DESC bufferSrvDesc = {};
    bufferSrvDesc.Format                        = DXGI_FORMAT_UNKNOWN;
    bufferSrvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_BUFFER;
    bufferSrvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    bufferSrvDesc.Buffer.NumElements            = 1;
    bufferSrvDesc.Buffer.StructureByteStride    = 4;

    // we pass valid buffer descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateShaderResourceView(nullptr, &bufferSrvDesc, descriptor);
}

inline void NullDescriptorTex2dSrv(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, ID3D12Device *device)
{
    // create valid Tex2DSrv descriptor with arbitrary but valid color format
    D3D12_SHADER_RESOURCE_VIEW_DESC tex2dSrvDesc = {};
    tex2dSrvDesc.Format                     = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex2dSrvDesc.ViewDimension              = D3D12_SRV_DIMENSION_TEXTURE2D;
    tex2dSrvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tex2dSrvDesc.Texture2D.MipLevels        = static_cast<UINT>(-1);

    // we pass valid texture descriptor but 'nullptr' as ID3D12Resource to create null descriptor
    device->CreateShaderResourceView(nullptr, &tex2dSrvDesc, descriptor);
}

#endif // #ifdef _GAMING_XBOX

inline void GpuFullStop(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    static Microsoft::WRL::ComPtr<ID3D12Fence> fenceGpuFullStop;
    if (!fenceGpuFullStop)
    {
        device->CreateFence(0ULL, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fenceGpuFullStop.ReleaseAndGetAddressOf()));
        SET_NAME_TO_SELF(fenceGpuFullStop);
    }

    commandQueue->Signal(fenceGpuFullStop.Get(), 1ULL);

    auto originalTime = GetTickCount64();
    while (0ULL == fenceGpuFullStop->GetCompletedValue()) 
    {
        auto currentTime = GetTickCount64();
        auto waitTime = 5000ULL; // 5 seconds
        if (currentTime > originalTime + waitTime)
        {
            if (IsDebuggerPresent())
            {
#ifdef _GAMING_XBOX
                device->ReportGpuHangX(D3D12XBOX_REPORT_GPU_HANG_FLAG_DO_NOT_REBOOT);
#endif
                __debugbreak();
            }
            else
            {
#ifdef _GAMING_XBOX
                device->ReportGpuHangX(D3D12XBOX_REPORT_GPU_HANG_FLAG_NONE);
#endif
            }
        }
        else
        {
            auto sleepTime = 10U; // 10 ms
            Sleep(sleepTime);
        }
    }

    fenceGpuFullStop->Signal(0ULL);
}

#ifdef _GAMING_XBOX
inline void GpuCacheFlushAndFullBarrier(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    static Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (!fence)
    {
        device->CreateFence(0ULL, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.ReleaseAndGetAddressOf()));
        SET_NAME_TO_SELF(fence);
    }

    // We use SignalX as a convenient way to access the D3D12XBOX_FLUSH_* flags without needing a command list
    // Wait for preceding shader work, and do a synchronous flush of all caches
    commandQueue->SignalX(fence.Get(), 0ULL, D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK);
}
#endif // #ifdef _GAMING_XBOX
