//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

__declspec(selectany) D3D12XBOX_GPU_HARDWARE_CONFIGURATION g_gpuHardwareConfiguration;

inline bool IsDurangoClass()
{
    return D3D12XBOX_HARDWARE_VERSION_XBOX_ONE == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S == g_gpuHardwareConfiguration.HardwareVersion;
}

inline bool IsScorpioClass()
{
    return D3D12XBOX_HARDWARE_VERSION_SCORPIO == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion;
}

inline bool IsScarlettClass()
{
#ifdef _GAMING_XBOX_SCARLETT
    return D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion;
#else
    return false;
#endif
}

inline bool IsAnacondaClass()
{
#ifdef _GAMING_XBOX_SCARLETT
    return D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion;
#else
    return false;
#endif
}

inline bool IsLockhartClass()
{
#ifdef _GAMING_XBOX_SCARLETT
    return D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART == g_gpuHardwareConfiguration.HardwareVersion;
#else
    return false;
#endif
}

struct GpuProperties
{
    // Geometry related
    uint64_t m_numIa;
    uint64_t m_numVgtPerIa;

    // Compute related
    uint64_t m_numThreadsPerSimd;
    uint64_t m_numSimdPerCu;
    uint64_t m_numCuPerSe;
    uint64_t m_numSe;

    // RB related
    uint64_t m_numCb;
    uint64_t m_colorCacheSizeInBytes;
    uint64_t m_numDb;
    uint64_t m_numPixelPerClockPerDb;

    // Memory related
    uint64_t m_bytesPerMemTransGddr;
    uint64_t m_memTransPerSecondGddr;
    uint64_t m_nClkPerSecondNorthbridge;
    uint64_t m_bytesPerNClkOnionRead;
    uint64_t m_bytesPerNClkOnionWrite;
    uint64_t m_bytesPerSClkEsram;
    uint64_t m_bytesPerSClkDma;

    static const GpuProperties& Get() 
    { 
        if (g_gpuHardwareConfiguration.HardwareVersion < _countof(_allGpuProperties))
        {
            return _allGpuProperties[g_gpuHardwareConfiguration.HardwareVersion];
        }
        else
        {
            throw std::exception("GPU hardware version out of range!");
        }
    }

private:
#ifdef _GAMING_XBOX_SCARLETT
    static const GpuProperties _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT + 1];
#else
    static const GpuProperties _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT + 1];
#endif
};

__declspec(selectany) const GpuProperties GpuProperties::_allGpuProperties[] =
{
    // D3D12XBOX_HARDWARE_VERSION_XBOX_ONE
    {
        1,                                              // uint64_t m_numIa;
        2,                                              // uint64_t m_numVgtPerIa;

        16,                                             // uint64_t m_numThreadsPerSimd;
        4,                                              // uint64_t m_numSimdPerCu;
        6,                                              // uint64_t m_numCuPerSe;
        2,                                              // uint64_t m_numSe;

        4,                                              // uint64_t m_numCb;
        16 * 1024,                                      // uint64_t m_colorCacheSizeInBytes;
        4,                                              // uint64_t m_numDb;
        4,                                              // uint64_t m_numPixelPerClockPerDb;

        32,                                             // uint64_t m_bytesPerMemTransGddr;
        2133000000,                                     // uint64_t m_memTransPerSecondGddr;
        1250000000,                                     // uint64_t m_nClkPerSecondNorthbridge;
        24,                                             // uint64_t bytesPerNClkOnionRead; --- may be a heuristic estimate
        12,                                             // uint64_t m_bytesPerNClkOnionWrite; --- may be a heuristic estimate

        // ESRAM supports 128 bytes per clock with 32 byte requests.
        // In short bursts and with larger requests, and with read and write interleaved,
        // it's theoretically possible to get a 1 7/8 multiplier.
        // The conditions for obtaining this rate in a sustained way are not
        // produced by any actual hardware usage.
        // However, there are cases where a multiplier significantly above 1.0 
        // are observed.
        128,                                            // uint64_t m_bytesPerSClkEsram;
        64,                                             // uint64_t m_bytesPerSClkDma;
    },
    // D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S
    _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_XBOX_ONE],
    // D3D12XBOX_HARDWARE_VERSION_SCORPIO
    {
        2,                                              // uint64_t m_numIa;
        2,                                              // uint64_t m_numVgtPerIa;

        16,                                             // uint64_t m_numThreadsPerSimd;
        4,                                              // uint64_t m_numSimdPerCu;
        10,                                             // uint64_t m_numCuPerSe;
        4,                                              // uint64_t m_numSe;

        8,                                              // uint64_t m_numCb;
        16 * 1024,                                      // uint64_t m_colorCacheSizeInBytes;
        8,                                              // uint64_t m_numDb;
        4,                                              // uint64_t m_numPixelPerClockPerDb;

        48,                                             // uint64_t m_bytesPerMemTransGddr;
        6800000000,                                     // uint64_t m_memTransPerSecondGddr;
        1700000000,                                     // uint64_t m_nClkPerSecondNorthbridge;
        24,                                             // uint64_t bytesPerNClkOnionRead; --- may be a heuristic estimate
        12,                                             // uint64_t m_bytesPerNClkOnionWrite; --- may be a heuristic estimate
        0,                                              // uint64_t m_bytesPerSClkEsram;
        64,                                             // uint64_t m_bytesPerSClkDma;
    },
    // D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT
#if SCORPIO_DEVKIT_44CU_MODE                            // This mode is not queryable from the title, and is include here only for clarity
    {
        2,                                              // uint64_t m_numIa;
        2,                                              // uint64_t m_numVgtPerIa;

        16,                                             // uint64_t m_numThreadsPerSimd;
        4,                                              // uint64_t m_numSimdPerCu;
        11,                                             // uint64_t m_numCuPerSe;
        4,                                              // uint64_t m_numSe;

        8,                                              // uint64_t m_numCb;
        16 * 1024,                                      // uint64_t m_colorCacheSizeInBytes;
        8,                                              // uint64_t m_numDb;
        4,                                              // uint64_t m_numPixelPerClockPerDb;

        48,                                             // uint64_t m_bytesPerMemTransGddr;
        6800000000,                                     // uint64_t m_memTransPerSecondGddr;
        1700000000,                                     // uint64_t m_nClkPerSecondNorthbridge;
        24,                                             // uint64_t bytesPerNClkOnionRead; --- may be a heuristic estimate
        12,                                             // uint64_t m_bytesPerNClkOnionWrite; --- may be a heuristic estimate
        0,                                              // uint64_t m_bytesPerSClkEsram;
        64,                                             // uint64_t m_bytesPerSClkDma;
    },
#else
    _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_SCORPIO],
#endif
#ifdef _GAMING_XBOX_SCARLETT
    // D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART
    {
        1,                                              // uint64_t m_numIa;
        2,                                              // uint64_t m_numVgtPerIa;

        32,                                             // uint64_t m_numThreadsPerSimd;
        2,                                              // uint64_t m_numSimdPerCu;
        20,                                             // uint64_t m_numCuPerSe;
        1,                                              // uint64_t m_numSe;

        4,                                              // uint64_t m_numCb;
        32 * 1024,                                      // uint64_t m_colorCacheSizeInBytes;
        4,                                              // uint64_t m_numDb;
        4,                                              // uint64_t m_numPixelPerClockPerDb;

        48,                                             // uint64_t m_bytesPerMemTransGddr;
        6800000000,                                     // uint64_t m_memTransPerSecondGddr;
        1700000000,                                     // uint64_t m_nClkPerSecondNorthbridge;
        24,                                             // uint64_t bytesPerNClkOnionRead; --- may be a heuristic estimate
        12,                                             // uint64_t m_bytesPerNClkOnionWrite; --- may be a heuristic estimate
        0,                                              // uint64_t m_bytesPerSClkEsram;
        64,                                             // uint64_t m_bytesPerSClkDma;
    },
    // D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA
    {
        2,                                              // uint64_t m_numIa;
        2,                                              // uint64_t m_numVgtPerIa;

        32,                                             // uint64_t m_numThreadsPerSimd;
        2,                                              // uint64_t m_numSimdPerCu;
        26,                                             // uint64_t m_numCuPerSe;
        2,                                              // uint64_t m_numSe;

        8,                                              // uint64_t m_numCb;
        32 * 1024,                                      // uint64_t m_colorCacheSizeInBytes;
        8,                                              // uint64_t m_numDb;
        4,                                              // uint64_t m_numPixelPerClockPerDb;

        48,                                             // uint64_t m_bytesPerMemTransGddr;
        6800000000,                                     // uint64_t m_memTransPerSecondGddr;
        1700000000,                                     // uint64_t m_nClkPerSecondNorthbridge;
        24,                                             // uint64_t bytesPerNClkOnionRead; --- may be a heuristic estimate
        12,                                             // uint64_t m_bytesPerNClkOnionWrite; --- may be a heuristic estimate
        0,                                              // uint64_t m_bytesPerSClkEsram;
        64,                                             // uint64_t m_bytesPerSClkDma;
    },
    // D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT
    _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA],
#endif
};
