#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

// Retrieves GPU information for the GPU with the highest performance, should more than one GPU be connected to the device

HRESULT GetGPUInfo(std::wstring& name, UINT* vendorId, UINT* deviceId, UINT* revision, size_t* dedicatedMemorySize, size_t* sharedMemorySize, UINT* waveLaneCountMin, UINT* waveLaneCountMax, UINT* totalLaneCount)
{
    if(!vendorId || !deviceId || !revision || !dedicatedMemorySize || !sharedMemorySize || !waveLaneCountMin || !waveLaneCountMax || !totalLaneCount)
    {
        return E_INVALIDARG;
    }

    *vendorId = 0;
    *deviceId = 0;
    *revision = 0;
    *dedicatedMemorySize = 0;
    *sharedMemorySize = 0;
    *waveLaneCountMin = 0;
    *waveLaneCountMax = 0;
    *totalLaneCount   = 0;

    ComPtr<IDXGIFactory6> dxgiFactory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device> device;

    // Create the DXGI factory
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf()));
    if(FAILED(hr))
    {
        return hr;
    }

    // when ordered by performance, the 0th element should be the best choice for rendering
    hr = dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
    if(FAILED(hr))
    {
        return hr;
    }

    // Get the description struct for this GPU
    DXGI_ADAPTER_DESC1 desc;
    hr = adapter->GetDesc1(&desc);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.ReleaseAndGetAddressOf()));
    if(FAILED(hr))
    {
        return hr;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
    hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));
    if(FAILED(hr))
    {
        return hr;
    }
 
    // return the name and VRAM size
    name = desc.Description;
    *dedicatedMemorySize = desc.DedicatedVideoMemory;
    *sharedMemorySize =  desc.SharedSystemMemory;
    *vendorId = desc.VendorId;
    *deviceId = desc.DeviceId;
    *revision = desc.Revision;
    *waveLaneCountMin = options1.WaveLaneCountMin;
    *waveLaneCountMax = options1.WaveLaneCountMax;
    *totalLaneCount = options1.TotalLaneCount;

    return S_OK;
}
