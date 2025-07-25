#include <Windows.h>
#include <sysinfoapi.h>

HRESULT GetMemoryInfo(size_t* totalMemory, size_t* availableMemory)
{
    if(!totalMemory || !availableMemory)
    {
        return E_INVALIDARG;
    }

    *totalMemory = 0;
    *availableMemory = 0;

    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);

    // https://learn.microsoft.com/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex
    if(!GlobalMemoryStatusEx(&memoryStatus))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *totalMemory = memoryStatus.ullTotalPhys;
    *availableMemory = memoryStatus.ullAvailPhys;

    return S_OK;
}

#include <Psapi.h>

HRESULT GetProcessMemory(DWORD* pageFaultCount, size_t* workingSetSize)
{
    if(!pageFaultCount || !workingSetSize)
    {
        return E_INVALIDARG;
    }

    *pageFaultCount = 0;
    *workingSetSize = 0;

    // https://learn.microsoft.com/windows/win32/api/psapi/ns-psapi-process_memory_counters
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);

    // https://learn.microsoft.com/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
    if(!GetProcessMemoryInfo(GetCurrentProcess(), &counters, counters.cb))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pageFaultCount = counters.PageFaultCount;
    *workingSetSize = counters.WorkingSetSize;

    return S_OK;
}
