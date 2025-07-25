#include <WinSock2.h>
#include <iphlpapi.h>

#include <string>
#include <vector>

// Libraries needed for network connectivity
#pragma comment(lib, "iphlpapi.lib")

enum class ConnectionType
{
    Ethernet,
    WiFi,
    Unknown
};

struct NetworkAdapterInfo
{
    DWORD index;
    std::wstring name;
    std::wstring description;
    bool operStatus;
    ConnectionType type;
    NL_NETWORK_CONNECTIVITY_LEVEL_HINT connectionLevel;
};

// This function will enumerate all network adapters on the system and return their information, specifically only returing the connected Ethernet and WiFi adapters.
std::vector<NetworkAdapterInfo> ListNetworkAdapters()
{
    std::vector<NetworkAdapterInfo> adapters;

    // Get the list of adapters. Use the standard pattern of first calling with a null buffer to get the size.
    ULONG bufferSize = 0;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &bufferSize);

    if (result != ERROR_BUFFER_OVERFLOW)
    {
        return adapters;
    }

    std::vector<BYTE> buffer(bufferSize);
    IP_ADAPTER_ADDRESSES* adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterAddresses, &bufferSize);
    if (result != NO_ERROR) {
        return adapters;
    }

    for (IP_ADAPTER_ADDRESSES* pCurrAddresses = adapterAddresses; pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next) {

        // Filter out disconnected, and non-Ethernet/WiFi adapters
        if (pCurrAddresses->OperStatus != IfOperStatusUp || (pCurrAddresses->IfType != IF_TYPE_ETHERNET_CSMACD && pCurrAddresses->IfType != IF_TYPE_IEEE80211)) {
            continue;
        }

        NetworkAdapterInfo adapter;
        adapter.index = pCurrAddresses->IfIndex;
        adapter.name = pCurrAddresses->FriendlyName;
        adapter.description = pCurrAddresses->Description;

        // Mark the adapter as ethernet or WiFi based on its type
        if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
        {
            adapter.type = ConnectionType::Ethernet;
        }
        else if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
        {
            adapter.type = ConnectionType::WiFi;
        }
        else
        {
            adapter.type = ConnectionType::Unknown;
        }

        // Get the connectivity hint for the adapter
        // https://learn.microsoft.com/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhintforinterface
        NL_NETWORK_CONNECTIVITY_HINT hint;
        result = GetNetworkConnectivityHintForInterface(adapter.index, &hint);

        if (result == ERROR_SUCCESS)
        {
            adapter.connectionLevel = hint.ConnectivityLevel;
        }
        else
        {
            adapter.connectionLevel = NetworkConnectivityLevelHintUnknown;
        }

        adapters.push_back(adapter);
    }

    return adapters;
}


