#pragma comment(lib, "onecore.lib")

// This method returns the diagonal screen size in inches for the integrated display
// as provided by the device driver/EDID.  If the device does not have an integrated
// display, this function will error, for example, on a desktop with attached monitor

HRESULT GetDeviceScreenDiagonalSizeInInches(double* size)
{
    if(!size)
    {
        return E_INVALIDARG;
    }

    *size = 0;

    // https://learn.microsoft.com/windows/win32/api/sysinfoapi/nf-sysinfoapi-getintegrateddisplaysize
    // uses the display driver as the source
    HRESULT hr = GetIntegratedDisplaySize(size);
    return hr;
}
