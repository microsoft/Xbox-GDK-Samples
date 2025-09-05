#include "pch.h"
#include <windows.h>
#include <bluetoothapis.h> // NOTE: Bluetooth API header must be included after windows.h
#include <Psapi.h>

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