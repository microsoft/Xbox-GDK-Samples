#pragma once

#define SET_NAME_TO_SELF(a) (a)->SetName(L#a) 
#define SET_FENCE_NAME_TO_SELF(device, a) (device)->RegisterCustomFenceLocationX((a), L#a) 
#define UNSET_FENCE_NAME(device, h) (device)->UnregisterCustomFenceLocationX(h) 

#define _CONCATENATE(a,b) (a##b) 
#define CONCATENATE(a,b) _CONCATENATE(a,b) 
#define SCOPED_ERROR_FILTER(device, id) ScopedErrorFilter CONCATENATE(scopedErrorFilter,__LINE__)((device), (id))

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

std::vector<uint8_t> LoadBGRAImage(const wchar_t* filename, uint32_t& width, uint32_t& height);

inline bool IsDurangoClass(ID3D12Device* device)
{
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION gpuHardwareConfiguration = {};
    device->GetGpuHardwareConfigurationX(&gpuHardwareConfiguration);

    return D3D12XBOX_HARDWARE_VERSION_XBOX_ONE == gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S == gpuHardwareConfiguration.HardwareVersion;
}

inline bool IsScorpioClass(ID3D12Device* device)
{
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION gpuHardwareConfiguration = {};
    device->GetGpuHardwareConfigurationX(&gpuHardwareConfiguration);

    return D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X == gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT == gpuHardwareConfiguration.HardwareVersion;
}
