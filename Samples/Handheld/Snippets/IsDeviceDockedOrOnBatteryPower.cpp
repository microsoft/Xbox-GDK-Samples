#include <Windows.h>
#include <iostream>

SYSTEM_POWER_STATUS g_sysPowerStatus;

void ViewDevicePowerStatus()
{
	BOOL bResult = GetSystemPowerStatus(&g_sysPowerStatus);

	if (bResult == 0)
	{
		DWORD dwError = GetLastError();
		std::cout << "GetSystemPowerStatus failed with error: " << dwError << std::endl;
	}
	else
	{
		// A full list of the fields in the SYSTEM_POWER_STATUS structure can be found here:
		// https://docs.microsoft.com/windows/win32/api/winbase/ns-winbase-system_power_status

		// Output some key pieces of info as an example
		std::cout << "ACLineStatus: " << (g_sysPowerStatus.ACLineStatus==1?"Plugged In":"Unplugged") << std::endl;

		if (g_sysPowerStatus.BatteryFlag == 128)
			std::cout << "No system battery detected" << std::endl;
		else
			std::cout << "BatteryFlag: " << g_sysPowerStatus.BatteryFlag << std::endl;

		if (g_sysPowerStatus.BatteryLifePercent == 255)
			std::cout << "BatteryLifePercent: Status Unknown" << std::endl;
		else
			std::cout << "BatteryLifePercent: " << g_sysPowerStatus.BatteryLifePercent << std::endl;

		std::cout << "SystemStatusFlag: Battery saver is " << (g_sysPowerStatus.SystemStatusFlag == 1?"On":"Off") << std::endl;

		if (g_sysPowerStatus.BatteryLifeTime == -1)
			std::cout << "BatteryLifeTime: Unknown - Device is either plugged in or this data is not available" << std::endl;
		else
			std::cout << "BatteryLifeTime: " << g_sysPowerStatus.BatteryLifeTime << " Seconds remaining" << std::endl;

		if (g_sysPowerStatus.BatteryFullLifeTime == -1)
			std::cout << "BatteryFullLifeTime: Unknown - Device is either plugged in or this data is not available" << std::endl;
		else
			std::cout << "BatteryFullLifeTime: " << g_sysPowerStatus.BatteryFullLifeTime << " Seconds in total" << std::endl;
	}
}

bool IsDevicePowered()
{
    // https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-getsystempowerstatus
    // https://learn.microsoft.com/windows/win32/api/winbase/ns-winbase-system_power_status

    SYSTEM_POWER_STATUS sps{};

    if(GetSystemPowerStatus(&sps))
    {
        return (sps.ACLineStatus == 1);
    }
    else
    {
        return false;
    }
}
