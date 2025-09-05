#include <winreg.h>

bool GetDeviceOEMInfo(std::wstring& manufacturer, std::wstring& productName, std::wstring& systemFamily, std::wstring& baseboardProductName)
{
    constexpr DWORD BufferSize = sizeof(wchar_t) * 256;

    wchar_t temp[256]{};
    DWORD dataSize = BufferSize;

    LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if(status != ERROR_SUCCESS)
    {
        return false;
    }

    manufacturer = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if(status != ERROR_SUCCESS)
    {
        return false;
    }

    productName = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemFamily", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if(status != ERROR_SUCCESS)
    {
        return false;
    }

    systemFamily = temp;

    dataSize = BufferSize;
    status = RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    if(status != ERROR_SUCCESS)
    {
        return false;
    }

    baseboardProductName = temp;

    return true;
}

bool Test_GetDeviceOEMInfo()
{
    std::wstring manufacturer, productName, systemFamily;
    GetDeviceOEMInfo(manufacturer, productName, systemFamily);

    return true;
}
