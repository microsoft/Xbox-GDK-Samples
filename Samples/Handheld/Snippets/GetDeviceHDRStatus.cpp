#include <windows.h>

// This method will get the HDR status for the device's integrated display
// To get this information for an external display, look at the
// path.TargetInfo.outputTechnology for types that are not INTERNAL

HRESULT GetDeviceHDRStatus(bool* available, bool* enabled)
{
    if(!available || !enabled)
    {
        return E_INVALIDARG;
    }

    *available = false;
    *enabled = false;

    UINT32 pathCount = 0, modeCount = 0;

    // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getdisplayconfigbuffersizes
    LONG err = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
    if (err != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(static_cast<unsigned long>(err));
    }

    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);

    // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-querydisplayconfig
    err = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
    if (err != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(static_cast<unsigned long>(err));
    }

    for (const auto& path : paths)
    {
        // retrieve HDR status for the integrated display ONLY
        if(path.targetInfo.outputTechnology & DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL)
        {
            DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2 colorInfo = {};
            colorInfo.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2;
            colorInfo.header.size = sizeof(DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2);
            colorInfo.header.adapterId = path.targetInfo.adapterId;
            colorInfo.header.id = path.targetInfo.id;

            // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-displayconfiggetdeviceinfo
            err = DisplayConfigGetDeviceInfo(&colorInfo.header);
            if (err != ERROR_SUCCESS)
            {
                return HRESULT_FROM_WIN32(static_cast<unsigned long>(err));
            }

            *available = colorInfo.highDynamicRangeSupported;
            *enabled = colorInfo.highDynamicRangeUserEnabled;

            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}
