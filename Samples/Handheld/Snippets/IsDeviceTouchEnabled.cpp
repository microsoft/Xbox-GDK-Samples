#include <Windows.h>

bool IsDeviceTouchEnabled()
{
    // https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getsystemmetrics
    // SM_DIGITIZER determines if Tablet PC Input service is started
    // Return value is a bitmask of touch devices available
    return (GetSystemMetrics(SM_DIGITIZER) & NID_INTEGRATED_TOUCH) > 0;
}
