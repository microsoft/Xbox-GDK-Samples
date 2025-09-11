// RtlGetDeviceFamilyInfoEnum requires linking against ntdll.lib
// This API is available in Windows 10+
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
