// NOTE: Bluetooth API header must be included after windows.h
#include <windows.h>
#include <bluetoothapis.h>

#pragma comment(lib, "Bthprops.lib")

bool IsBluetoothEnabled()
{
    // https://learn.microsoft.com/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothisconnectable
	return BluetoothIsConnectable(nullptr);
}
