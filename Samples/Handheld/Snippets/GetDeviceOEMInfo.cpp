bool GetDeviceOEMInfo(std::wstring& manufacturer, std::wstring& productName, std::wstring& systemFamily)
{
    wchar_t temp[256];
    DWORD dataSize = sizeof(wchar_t) * 256;

    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    manufacturer = temp;

    dataSize = sizeof(wchar_t) * 256;
    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    productName = temp;

    dataSize = sizeof(wchar_t) * 256;
    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemFamily", RRF_RT_REG_SZ, nullptr, temp, &dataSize);
    systemFamily = temp;

    return true;
}

bool Test_GetDeviceOEMInfo()
{
    std::wstring manufacturer, productName, systemFamily;
    GetDeviceOEMInfo(manufacturer, productName, systemFamily);

    return true;
}
