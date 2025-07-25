// NOTE: Bluetooth API header must be included after windows.h
#include <windows.h>
#include <bluetoothapis.h>
#include <vector>

#pragma comment(lib, "Bthprops.lib")

bool IsUsingBluetoothConnectedDevice(std::vector<BLUETOOTH_DEVICE_INFO>& outConnectedDevices)
{
	outConnectedDevices.clear();

	BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { 0 };
	searchParams.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
	searchParams.fReturnAuthenticated = FALSE;
	searchParams.fReturnRemembered = FALSE;
	searchParams.fReturnConnected = TRUE;
	searchParams.fIssueInquiry = FALSE;
	searchParams.cTimeoutMultiplier = 0;

	BLUETOOTH_DEVICE_INFO deviceInfo = { 0 };
	deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

    // https://learn.microsoft.com/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothfindfirstdevice
	HANDLE hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
	bool foundNextDevice = true;
	while (hFind != nullptr && foundNextDevice)
	{
        // store device info
		outConnectedDevices.push_back(deviceInfo);

		// find next device
        // https://learn.microsoft.com/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothfindnextdevice
		foundNextDevice = BluetoothFindNextDevice(hFind, &deviceInfo);
	}

    if(hFind != nullptr)
    {
        // https://learn.microsoft.com/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothfinddeviceclose
    	BluetoothFindDeviceClose(hFind);
    }

	return !outConnectedDevices.empty();
}
