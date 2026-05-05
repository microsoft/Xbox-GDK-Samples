#pragma once

inline bool IsDeveloperModeEnabled()
{
#ifdef _GAMING_XBOX
    XSystemDeviceType deviceType = XSystemGetDeviceType();
    return deviceType == XSystemDeviceType::XboxOneXDevkit
        || deviceType == XSystemDeviceType::XboxOneDevkit
        || deviceType == XSystemDeviceType::XboxScarlettDevkit
        || deviceType == XSystemDeviceType::XboxScarlettTestkit;
#else
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD value = 0;
        DWORD size = sizeof(value);
        bool enabled = (RegQueryValueExW(hKey, L"AllowDevelopmentWithoutDevLicense", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) && (value != 0);
        RegCloseKey(hKey);
        return enabled;
    }
    return false;
#endif
}