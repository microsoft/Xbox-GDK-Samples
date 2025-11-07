#include "pch.h"
#include <windows.h>
#include <bluetoothapis.h> // NOTE: Bluetooth API header must be included after windows.h
#include <Psapi.h>
#include <string>
#include <combaseapi.h> // For CoTaskMemAlloc/CoTaskMemFree

// RtlGetDeviceFamilyInfoEnum requires linking against ntdll.lib
// This API is available in Windows 10+
#pragma comment(lib, "ntdll")
#pragma comment(lib, "Bthprops.lib")
// GetIntegratedDisplaySize requires linking against onecore.lib
#pragma comment(lib, "onecore.lib")

// If an older Windows SDK is being used, define the HANDHELD constant
#ifndef DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD   
#define DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD    0x0000002E
#endif

bool GetDeviceOEMInfo_Internal(std::wstring& manufacturer, std::wstring& productName, std::wstring& systemFamily, std::wstring& baseboardProductName)
{
    constexpr DWORD BufferSize = sizeof(wchar_t) * 256;

    wchar_t temp[256]{};
    DWORD dataSize = BufferSize;

    LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    manufacturer = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    productName = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemFamily", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    systemFamily = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    baseboardProductName = temp;

    return true;
}


HRESULT GetProcessMemory(DWORD* pageFaultCount, size_t* workingSetSize)
{
    if (!pageFaultCount || !workingSetSize)
    {
        return E_INVALIDARG;
    }

    *pageFaultCount = 0;
    *workingSetSize = 0;

    // https://learn.microsoft.com/windows/win32/api/psapi/ns-psapi-process_memory_counters
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);

    // https://learn.microsoft.com/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, counters.cb))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pageFaultCount = counters.PageFaultCount;
    *workingSetSize = counters.WorkingSetSize;

    return S_OK;
}

extern "C" __declspec(dllexport)
DWORD GetPageFaultCount()
{
    DWORD pageFaultCount = 0;
    size_t workingSetSize = 0;
    LOG_IF_FAILED(GetProcessMemory(&pageFaultCount, &workingSetSize));
    return pageFaultCount;
}

extern "C" __declspec(dllexport)
size_t GetWorkingSetSize()
{
    DWORD pageFaultCount = 0;
    size_t workingSetSize = 0;
    LOG_IF_FAILED(GetProcessMemory(&pageFaultCount, &workingSetSize));
    return workingSetSize;
}

extern "C" __declspec(dllexport)
double GetScreenSize()
{
    double sizeInInches = 0;
    LOG_IF_FAILED(GetIntegratedDisplaySize(&sizeInInches));
    return sizeInInches;
}

extern "C" __declspec(dllexport)
bool IsDeviceHandheld()
{
    DWORD deviceForm = 0;
    RtlGetDeviceFamilyInfoEnum(nullptr, nullptr, &deviceForm);
    return (deviceForm == DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD);
}

extern "C" __declspec(dllexport)
bool IsBluetoothEnabled()
{
    // https://learn.microsoft.com/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothisconnectable
    return BluetoothIsConnectable(nullptr);
}

// return individual strings that C# can marshal automatically
extern "C" __declspec(dllexport)
wchar_t* GetDeviceManufacturer()
{
    std::wstring mfg, prod, family, baseboard;
    if (!GetDeviceOEMInfo_Internal(mfg, prod, family, baseboard))
        return nullptr;
    
    size_t len = (mfg.length() + 1) * sizeof(wchar_t);
    wchar_t* result = static_cast<wchar_t*>(CoTaskMemAlloc(len));
    if (result)
        wcscpy_s(result, mfg.length() + 1, mfg.c_str());
    return result;
}

extern "C" __declspec(dllexport)
wchar_t* GetDeviceProductName()
{
    std::wstring mfg, prod, family, baseboard;
    if (!GetDeviceOEMInfo_Internal(mfg, prod, family, baseboard))
        return nullptr;
    
    size_t len = (prod.length() + 1) * sizeof(wchar_t);
    wchar_t* result = static_cast<wchar_t*>(CoTaskMemAlloc(len));
    if (result)
        wcscpy_s(result, prod.length() + 1, prod.c_str());
    return result;
}

extern "C" __declspec(dllexport)
wchar_t* GetDeviceSystemFamily()
{
    std::wstring mfg, prod, family, baseboard;
    if (!GetDeviceOEMInfo_Internal(mfg, prod, family, baseboard))
        return nullptr;
    
    size_t len = (family.length() + 1) * sizeof(wchar_t);
    wchar_t* result = static_cast<wchar_t*>(CoTaskMemAlloc(len));
    if (result)
        wcscpy_s(result, family.length() + 1, family.c_str());
    return result;
}

extern "C" __declspec(dllexport)
wchar_t* GetDeviceBaseboardProductName()
{
    std::wstring mfg, prod, family, baseboard;
    if (!GetDeviceOEMInfo_Internal(mfg, prod, family, baseboard))
        return nullptr;
    
    size_t len = (baseboard.length() + 1) * sizeof(wchar_t);
    wchar_t* result = static_cast<wchar_t*>(CoTaskMemAlloc(len));
    if (result)
        wcscpy_s(result, baseboard.length() + 1, baseboard.c_str());
    return result;
}
