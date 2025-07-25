#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

// Libraries needed for network connectivity
#pragma comment(lib, "iphlpapi.lib")

// Note: This API is the analogue of XNetworkingRegisterNetworkConnectivityHintChange with one large difference: XNetworking allows to you control
//       where the callback is executed through XTaskQueue. This API will call the callback on the Windows Threadpool so beware of what operations
//       you do in the callback.

HANDLE g_hNetworkConnectivityChangeHandle = NULL;

void NetworkConnectivityHintChangeCallback(
    PVOID pContext,
    NL_NETWORK_CONNECTIVITY_HINT hNetworkConnectivityHint
);

void InitializeNetworkConnectivityNotification()
{
    // Register for network connectivity hint change notifications as the OS becomes aware of them
    // https://learn.microsoft.com/windows/win32/api/netioapi/nf-netioapi-notifynetworkconnectivityhintchange
    DWORD result = NotifyNetworkConnectivityHintChange(
        NetworkConnectivityHintChangeCallback, // Callback function
        NULL,                                  // Caller provided context
        TRUE,                                  // Can optionally be notified of the current state through the callback
        &g_hNetworkConnectivityChangeHandle    // Handle to be filled
    );

    if (result != ERROR_SUCCESS)
    {
        // Handle error
    }

    // There is a synchronous call that can query the connectivity hint:
    // https://learn.microsoft.com/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhint
    NL_NETWORK_CONNECTIVITY_HINT hint;
    result = GetNetworkConnectivityHint(&hint);

    if (result != ERROR_SUCCESS)
    {
        // Handle error
    }
}

void UnregisterNetworkConnectivityNotification()
{
    CancelMibChangeNotify2(g_hNetworkConnectivityChangeHandle);
    g_hNetworkConnectivityChangeHandle = NULL;
}

void NetworkConnectivityHintChangeCallback(
    PVOID,
    NL_NETWORK_CONNECTIVITY_HINT hint
)
{
    // This is the main field to check in the connectivity hint structure. It's representative of Windows best understading
    // of the state of the device's network connectivity. Even if you have InternetAccess you should not assume that means
    // all services are available.
    hint.ConnectivityLevel;
    // NetworkConnectivityLevelHintUnknown,
    // NetworkConnectivityLevelHintNone,
    // NetworkConnectivityLevelHintLocalAccess,
    // NetworkConnectivityLevelHintInternetAccess,
    // NetworkConnectivityLevelHintConstrainedInternetAccess,
    // NetworkConnectivityLevelHintHidden,
}


