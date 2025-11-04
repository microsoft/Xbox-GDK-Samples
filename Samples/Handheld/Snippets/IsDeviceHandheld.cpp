// RtlGetDeviceFamilyInfoEnum requires linking against ntdll.lib
// This API is available in Windows 10+ .
//
// For a down-level compilable version, see the IsDeviceHandheldDownlevel function below

#pragma comment(lib, "ntdll")

// If an older Windows SDK is being used, define the HANDHELD constant
#ifndef DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD   
#define DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD    0x0000002E
#endif 

bool IsDeviceHandheld()
{
    DWORD deviceForm = 0;
    RtlGetDeviceFamilyInfoEnum(nullptr, nullptr, &deviceForm);
    return (deviceForm == DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD);
}

// Down-level compilable verson of IsDeviceHandheld
// Use if you are compiling against an older Windows SDK or targetting a version of Windows < Windows 10

// disable cast warning
#pragma warning(disable:4191)

typedef VOID (WINAPI* PFN_RtlGetDeviceFamilyInfoEnum)(ULONGLONG *pullUAPInfo, DWORD *pulDeviceFamily, DWORD *pulDeviceForm);

bool IsDeviceHandheldDownlevel()
{
    // get a reference to ntdll.dll
    HMODULE hModule = GetModuleHandleA("ntdll.dll");
    if(!hModule)
    {
        return false;
    }

    // get reference to RtlGetDeviceFamilyInfoEnum
    PFN_RtlGetDeviceFamilyInfoEnum _RtlGetDeviceFamilyInfoEnum = (PFN_RtlGetDeviceFamilyInfoEnum)GetProcAddress(hModule, "RtlGetDeviceFamilyInfoEnum");
    if(!_RtlGetDeviceFamilyInfoEnum)
    {
        return false;
    }

    // call the function via the loaded function pointer
    DWORD deviceForm = 0;
    _RtlGetDeviceFamilyInfoEnum(nullptr, nullptr, &deviceForm);
    return (deviceForm == DEVICEFAMILYDEVICEFORM_GAMING_HANDHELD);
}
