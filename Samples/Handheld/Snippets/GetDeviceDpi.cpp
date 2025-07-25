#include <ShellScalingApi.h>

// GetDpiForMonitor
#pragma comment(lib, "shcore.lib")

HRESULT GetDeviceDpi(unsigned int* x, unsigned int* y)
{
    // https://learn.microsoft.com/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor
    // https://learn.microsoft.com/windows/win32/hidpi/wm-dpichanged
    // NOTE: Don't forget to handle WM_DPICHANGED messages in WndProc handler for
    //       PROCESS_PER_MONITOR_DPI_AWARE applications or DPI_AWARENESS_PER_MONITOR_AWARE threads

    return GetDpiForMonitor(MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY), MDT_EFFECTIVE_DPI, x, y);
}
