//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX
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

#else // defined(_GAMING_DESKTOP)

// Nvidia "Nsight Perf SDK" headers
#include "nvperf_d3d12_host.h"
#include "nvperf_d3d12_target.h"

// define helper macro to automatically check calls for errors
#define checkNvPaStatus(call)                                                                           \
    do                                                                                                  \
    {                                                                                                   \
        NVPA_Status status = call;                                                                      \
        if (NVPA_STATUS_SUCCESS != status)                                                              \
        {                                                                                               \
            const char* pStatusStr = "";                                                                \
            const char* pCommentStr = "";                                                               \
            NVPW_NVPAStatusToString(status, &pStatusStr, &pCommentStr);                                 \
            char buffer[256];                                                                           \
            snprintf(buffer, _countof(buffer), "NVPerf Failed: '%s' - '%s'\n", pStatusStr, pCommentStr);\
            OutputDebugStringA(buffer);                                                                 \
            __debugbreak();                                                                             \
        }                                                                                               \
    }                                                                                                   \
    while (0)

// AMD GPU Performance API headers
#pragma warning(push)
#pragma warning(disable : 4191)


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif

#include <gpu_performance_api/gpu_perf_api_interface_loader.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#pragma warning(pop)

// AMD GPU Performance API expects the below variables to be declared
#ifdef __cplusplus
__declspec(selectany) GpaApiManager *GpaApiManager::gpa_api_manager_ = nullptr;
#endif
__declspec(selectany) GpaFuncTableInfo  *gpa_function_table_info = nullptr;

// We add one more global function to translate error codes to string messages
__declspec(selectany) GpaGetStatusAsStrPtrType gpa_get_status_as_str = nullptr;

#define checkAmdGpaStatus(call)                                                     \
    do                                                                              \
    {                                                                               \
        GpaStatus status = call;                                                    \
        if (kGpaStatusOk != status)                                                 \
        {                                                                           \
            const char *string = gpa_get_status_as_str(status);                     \
            char buffer[256];                                                       \
            snprintf(buffer, _countof(buffer), "AMD GPA Failed: '%s'\n", string);   \
            OutputDebugStringA(buffer);                                             \
            __debugbreak();                                                         \
        }                                                                           \
    }                                                                               \
    while(0)


#endif // #ifdef _GAMING_XBOX

struct GpuProperties
{
#ifdef _GAMING_XBOX
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
#else // #ifdef _GAMING_DESKTOP
    uint32_t m_isGpuVendorAmd       : 1;
    uint32_t m_isGpuVendorIntel     : 1;
    uint32_t m_isGpuVendorNvidia    : 1;

    // Nvidia Nsight PerfSDK bits that are always initialised
    uint32_t                m_nvDeviceIndex;
    const char             *m_nvChipName;
    NVPW_MetricsEvaluator  *m_nvMetricsEvaluator;
    std::vector<uint8_t>    m_nvMetricsEvaluatorScratch;

    // AMD GPU PerfAPI bits that are always initialised
    GpaFunctionTable   *m_amdGpa;
    GpaContextId        m_amdGpaContextId;

    static bool IsSupportedNvidiaGpu()
    {
        /** the initialisation path only setup m_nvChipName pointer variable only when we can collect counters from this GPU */
        return _dynamicGpuProperties.m_nvChipName != nullptr;
    }

    static bool IsSupportedAmdGpu()
    {
        /** the initialisation path setup m_amdGpa pointer only when we can collect counters from this GPU */
        return _dynamicGpuProperties.m_amdGpa != nullptr;
    }

    static void Initialize(ID3D12Device *device, UINT vendorId)
    {
        auto & gpuProps = _dynamicGpuProperties;

        gpuProps.m_isGpuVendorAmd = vendorId == 0x1002;
        gpuProps.m_isGpuVendorIntel = vendorId == 0x8086;
        gpuProps.m_isGpuVendorNvidia = vendorId == 0x10de;

        gpuProps.m_nvDeviceIndex        = 0;
        gpuProps.m_nvChipName           = nullptr;
        gpuProps.m_nvMetricsEvaluator   = nullptr;

        gpuProps.m_amdGpa           = nullptr;
        gpuProps.m_amdGpaContextId  = nullptr;

#ifdef ENABLE_GPU_COUNTERS
        if (gpuProps.m_isGpuVendorNvidia)
        {
            // initialize "host"
            NVPW_InitializeHost_Params initializeHostParams = { NVPW_InitializeHost_Params_STRUCT_SIZE };
            checkNvPaStatus(NVPW_InitializeHost(&initializeHostParams));

            // initialize "target"
            NVPW_InitializeTarget_Params initializeTargetParams = { NVPW_InitializeTarget_Params_STRUCT_SIZE };
            checkNvPaStatus(NVPW_InitializeTarget(&initializeTargetParams));

            NVPW_D3D12_LoadDriver_Params loadDriverParams = { NVPW_D3D12_LoadDriver_Params_STRUCT_SIZE };
            checkNvPaStatus(NVPW_D3D12_LoadDriver(&loadDriverParams));

            // consider if we need multi-GPU support?
            NVPW_D3D12_Device_GetDeviceIndex_Params getDeviceIndexParams = { NVPW_D3D12_Device_GetDeviceIndex_Params_STRUCT_SIZE };
            getDeviceIndexParams.pDevice    = device;
            getDeviceIndexParams.sliIndex   = 0;
            checkNvPaStatus(NVPW_D3D12_Device_GetDeviceIndex(&getDeviceIndexParams));
            gpuProps.m_nvDeviceIndex = static_cast<uint32_t>(getDeviceIndexParams.deviceIndex);

            NVPW_D3D12_Profiler_IsGpuSupported_Params isGpuSupportedParams = { NVPW_D3D12_Profiler_IsGpuSupported_Params_STRUCT_SIZE };
            isGpuSupportedParams.deviceIndex = gpuProps.m_nvDeviceIndex;
            checkNvPaStatus(NVPW_D3D12_Profiler_IsGpuSupported(&isGpuSupportedParams));
            if (isGpuSupportedParams.isSupported)
            {
                // retrieve Chip Name
                NVPW_Device_GetNames_Params getNamesParams = { NVPW_Device_GetNames_Params_STRUCT_SIZE };
                getNamesParams.deviceIndex = getDeviceIndexParams.deviceIndex;
                checkNvPaStatus(NVPW_Device_GetNames(&getNamesParams));
                gpuProps.m_nvChipName = getNamesParams.pChipName;

                // determine Scratch Buffer size for Metric Evaluator and allocate it
                NVPW_D3D12_MetricsEvaluator_CalculateScratchBufferSize_Params meCalcScratchBufferSize = { NVPW_D3D12_MetricsEvaluator_CalculateScratchBufferSize_Params_STRUCT_SIZE };
                meCalcScratchBufferSize.pChipName = gpuProps.m_nvChipName;
                checkNvPaStatus(NVPW_D3D12_MetricsEvaluator_CalculateScratchBufferSize(&meCalcScratchBufferSize));

                gpuProps.m_nvMetricsEvaluatorScratch.resize(meCalcScratchBufferSize.scratchBufferSize);

                // initialise Metric Evaluator
                NVPW_D3D12_MetricsEvaluator_Initialize_Params meInitialize = { NVPW_D3D12_MetricsEvaluator_Initialize_Params_STRUCT_SIZE };
                meInitialize.pScratchBuffer     = gpuProps.m_nvMetricsEvaluatorScratch.data();
                meInitialize.scratchBufferSize  = gpuProps.m_nvMetricsEvaluatorScratch.size();
                meInitialize.pChipName          = gpuProps.m_nvChipName;
                checkNvPaStatus(NVPW_D3D12_MetricsEvaluator_Initialize(&meInitialize));
                gpuProps.m_nvMetricsEvaluator   = meInitialize.pMetricsEvaluator;
            }
        }
        else if (gpuProps.m_isGpuVendorAmd)
        {
            if (kGpaStatusOk == GpaApiManager::Instance()->LoadApi(kGpaApiDirectx12))
            {
                gpuProps.m_amdGpa = GpaApiManager::Instance()->GetFunctionTable(kGpaApiDirectx12);
                if (nullptr != gpuProps.m_amdGpa)
                {
                    if (kGpaStatusOk == gpuProps.m_amdGpa->GpaInitialize(kGpaInitializeDefaultBit))
                    {
                        gpa_get_status_as_str = gpuProps.m_amdGpa->GpaGetStatusAsStr;
                        checkAmdGpaStatus(gpuProps.m_amdGpa->GpaOpenContext(reinterpret_cast<void *>(device), kGpaOpenContextEnableHardwareCountersBit, &gpuProps.m_amdGpaContextId));
                    }
                    else
                    {
                        
                        gpuProps.m_amdGpa = nullptr;
                    }
                }
            }
        }
#endif // #ifdef ENABLE_GPU_COUNTERS
    }
#endif // #ifdef _GAMING_XBOX

    static const GpuProperties& Get()
    {
#ifdef _GAMING_XBOX
        if (g_gpuHardwareConfiguration.HardwareVersion < _countof(_allGpuProperties))
        {
            return _allGpuProperties[g_gpuHardwareConfiguration.HardwareVersion];
        }
        else
        {
            throw std::exception("GPU hardware version out of range!");
        }
#else
        return _dynamicGpuProperties;
#endif
    }

private:
#if defined(_GAMING_XBOX_SCARLETT)
    static const GpuProperties _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT + 1];
#elif defined(_GAMING_XBOX_XBOXONE)
    static const GpuProperties _allGpuProperties[D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT + 1];
#else // defined(_GAMING_DESKTOP)

    // On PC GPU properties are evaluated after ID3D12Device creation
    static GpuProperties _dynamicGpuProperties;
#endif
};

#ifdef _GAMING_XBOX
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

#else // ifdef _GAMING_DESKTOP

__declspec(selectany) GpuProperties GpuProperties::_dynamicGpuProperties = {};

#endif // #ifdef _GAMING_XBOX
