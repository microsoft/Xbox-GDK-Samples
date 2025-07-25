#include <Windows.h>

// This method will get the current resolution and refresh for the display on which the calling thread is running

bool GetDeviceScreenResolutionAndRefresh(DWORD* xres, DWORD* yres, DWORD* refresh)
{
    if(!xres || !yres || !refresh)
    {
        return false;
    }

    *xres = 0;
    *yres = 0;
    *refresh = 0;

    DEVMODE dm{};
    dm.dmSize = sizeof(dm);

    // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumdisplaysettingsa
    // Gets current settings using ENUM_CURRET_SETTINGS
    if(EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm))
    {
        *xres = dm.dmPelsWidth;
        *yres = dm.dmPelsHeight;
        *refresh = dm.dmDisplayFrequency;
        return true;
    }

    return false;
}

struct ResolutionInfo
{
    DWORD Width;
    DWORD Height;
    DWORD Refresh;

    // operator overloads for std::find, std::sort below
    bool operator==(const ResolutionInfo& ri) const
    {
        return (Width == ri.Width) && (Height == ri.Height) && (Refresh == ri.Refresh);
    }

    bool operator<(const ResolutionInfo& ri) const
    {
        return Width < ri.Width;
    }
};

std::vector<ResolutionInfo> GetAllScreenResolutions()
{
    std::vector<ResolutionInfo> modes;

    DEVMODE dm{};
    dm.dmSize = sizeof(dm);

    // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumdisplaysettingsa
    // Enumerate all display modes
    for(DWORD i = 0; ; i++)
    {
        if(!EnumDisplaySettings(nullptr, i, &dm))
        {
            break;
        }
        else
        {
            ResolutionInfo ri = { dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency };

            // only add if we haven't seen this combo before
            if(std::find(modes.begin(), modes.end(), ri) == modes.end())
            {
                modes.push_back(ri);
            }
        }
    }

    // sort based on width
    std::sort(modes.begin(), modes.end());
    return modes;
}
