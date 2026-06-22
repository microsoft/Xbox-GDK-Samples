//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX_XBOXONE
#include "GpuCounterSetDurango.h"
#endif

#include "GpuProperties.h"

#ifdef _GAMING_XBOX
#include "Util.h"

// GpuCounterType.h is a utility header which wraps a macro around the text name of each GPU component:
// E.g. DB, SPI, TCC, etc. We include this file in multiple places in order to define a class of
// template specializations, one per component.

// An enum with separate values for each of the [type]_PERFCOUNTER_SELECT enums.
// Allows us to distinguish between different counters with the same numeric values.
// E.g. CB_PERF_SEL_BUSY and DB_PERF_SEL_SC_DB_TILE_BUSY both have value 1, but they will
// have GpuCounterType 4 vs 5.
enum GpuCounterType : uint32_t
{
#define GPU_COUNTER_CODE(type) GpuCounterType##type,
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE

    GpuCounterTypeInvalid
};

typedef uint64_t CounterValue;

#else // defined(_GAMING_DESKTOP)

typedef double CounterValue;

#endif // #ifdef _GAMING_XBOX

typedef std::vector<CounterValue> CounterValueArray;

typedef CounterValue (*CounterAggregator)(CounterValueArray array);

inline CounterValue CounterValueArrayAdd(CounterValueArray array)
{
    CounterValue result = 0;
    for (auto value : array)
    {
        result += value;
    }
    return result;
}

inline CounterValue CounterValueArrayMax(CounterValueArray array)
{
    CounterValue result = 0;
    for (auto value : array)
    {
        result = std::max(result, value);
    }
    return result;
}

inline CounterValue CounterValueArrayAvg(CounterValueArray array)
{
    CounterValue result = 0;
    for (auto value : array)
    {
        result += value;
    }
    return result / array.size();
}

inline CounterValue CounterValueArrayStd(CounterValueArray array)
{
    auto Sq = [] (CounterValue c)->CounterValue { return c * c; };

    CounterValue sum = 0;
    CounterValue sumSq = 0;
    for (auto value : array)
    {
        sum += value;
        sumSq += Sq(value);
    }
    return CounterValue(sqrt(sumSq / array.size() - Sq(sum / array.size())));
}

#ifdef _GAMING_XBOX
// A class which records the type and id of a counter.
// Allows us to distinguish between different counters with the same numeric values.
// E.g. CB_PERF_SEL_BUSY and DB_PERF_SEL_SC_DB_TILE_BUSY both have value 1, but they will
// have GpuCounter {4,1} vs {5,1}.
class GpuCounter
{
public:
    enum ShaderMask : uint32_t
    {
        SHADER_MASK_VS = 1 << 0,
        SHADER_MASK_PS = 1 << 1,
        SHADER_MASK_GS = 1 << 2,
        SHADER_MASK_ES = 1 << 3,
        SHADER_MASK_HS = 1 << 4,
        SHADER_MASK_LS = 1 << 5,
        SHADER_MASK_CS = 1 << 6,

        SHADER_MASK_ALL = 0,
    };

private:
    ShaderMask      m_shaderMask    : 4;
    GpuCounterType  m_type          : 28;

    union
    {
        uint32_t    m_id;

#define GPU_COUNTER_CODE(type) GPUPerfCounters::type##_PERFCOUNTER_SELECT m_id##type : 32;
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
    };

public:
#define GPU_COUNTER_CODE(type) GpuCounter(GPUPerfCounters::type##_PERFCOUNTER_SELECT id, ShaderMask shaderMask = SHADER_MASK_ALL) : m_shaderMask(shaderMask), m_type(GpuCounterType##type), m_id##type(id) {}
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE

    const wchar_t* GetTypeName() const
    {
        switch (m_type)
        {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return L#type;
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
        default: return 0;
        }
    }

    size_t GetArrayCount() const
    {
#ifdef _GAMING_XBOX_SCARLETT
        if (IsScarlettClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_SET_DESC::type);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else
        {
            throw std::exception("Only Scarlett hardware types are currently supported");
        }
#else
        if (IsDurangoClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_SET_DURANGO_DESC::type);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else if (IsScorpioClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_SET_DESC::type);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else
        {
            throw std::exception("Only Durango and Scorpio hardware types are currently supported");
        }
#endif
    }

    size_t GetInstanceCount() const
    {
#ifdef _GAMING_XBOX_SCARLETT
        if (IsAnacondaClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_DATA::type[0]);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else if (IsLockhartClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_LOCKHART_DATA::type[0]);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else
        {
        throw std::exception("Only Scarlett hardware types are currently supported");
        }
#else
        if (IsDurangoClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_DURANGO_DATA::type[0]);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else if (IsScorpioClass())
        {
            switch (m_type)
            {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return _countof(D3D12XBOX_COUNTER_DATA::type[0]);
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
            default: return 0;
            }
        }
        else
        {
            throw std::exception("Only Durango and Scorpio hardware types are currently supported");
        }
#endif
    }

    UINT GetId(const D3D12XBOX_COUNTER_SET_DESC& counterSetDesc, size_t arrayIndex) const
    {
        switch (m_type)
        {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return counterSetDesc.type[arrayIndex];
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
        default: return 0;
        }
    }
    virtual void SetId(D3D12XBOX_COUNTER_SET_DESC& counterSetDesc, size_t arrayIndex, UINT id) const
    {
        switch (m_type)
        {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: counterSetDesc.type[arrayIndex] = id; return;
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
        default:;
        }
    }

    CounterValue GetValue(const D3D12XBOX_COUNTER_DATA& counterData, size_t arrayIndex, size_t instance) const
    {
        switch (m_type)
        {
#define GPU_COUNTER_CODE(type) case GpuCounterType##type: return counterData.type[arrayIndex][instance];
#include "GpuCounterType.h"
#undef GPU_COUNTER_CODE
        default: return 0;
        }
    }

    // Operators and constructors to support std::map
    GpuCounter(const GpuCounter& other) { *this = other;  }
    GpuCounter& operator=(const GpuCounter& other) { m_shaderMask = other.m_shaderMask;  m_type = other.m_type; m_id = other.m_id; return *this; }
    operator const uint64_t () const { return uint64_t(m_shaderMask) << 60 | uint64_t(m_type) << 32 | uint64_t(m_id);  }
};

typedef std::map<GpuCounter, CounterValueArray> GpuCounterTable;

#else // defined(_GAMING_DESKTOP)

typedef std::map<std::string, CounterValue> GpuCounterTable;

#endif // #ifdef _GAMING_XBOX

// Class which supports an arbitrary collection of GPU counters. We organize these counters
// into the minimal number of D3D12XBOX_COUNTER_SET_DESC needed to cover them all. Collection occurs
// in the minimal number of passes.
class GpuCounterSet
{
public:
    GpuCounterSet()
#ifdef _GAMING_DESKTOP
        : m_nvRawCounterConfig(nullptr)
        , m_nvRawCounterDomains()
        , m_nvCounterDataBuilder(nullptr)
        , m_nvConfigImage()
        , m_nvCounterDataPrefix()
        , m_nvCounterDataImage()
        , m_nvCounterDataScratch()
        , m_existingCounterNames()
        , m_requestedCounterNames()
        , m_nvSpgoThread()
        , m_nvSpgoThreadExited(0)
        , m_amdGpaCounterIndicesPerRequestedCounter()
        , m_amdGpaCounterSampleOffsetsPerRequestedCounter()
        , m_amdGpaCounterDataTypesPerRequestedCounter()
        , m_amdRequestedCounterIndexToGpaCountersIndicesStart()
        , m_amdGpaSampleData()
        , m_amdGpaSessionId(nullptr)
        , m_amdGpaCmdListId(nullptr)
        , m_numPasses(0)
        , m_isAddingCounters(0)
        , m_counterPassIndex(0)
#else
        : m_counterPassIndex(0)
#endif
    {
        Reset();
    }

    void Initialize()
    {
#ifdef _GAMING_DESKTOP
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // initialise RawCounterConfig
            NVPW_D3D12_RawCounterConfig_Create_Params rawCountersConfigCreate = { NVPW_D3D12_RawCounterConfig_Create_Params_STRUCT_SIZE };
            rawCountersConfigCreate.activityKind = NVPA_ACTIVITY_KIND_PROFILER;
            rawCountersConfigCreate.pChipName = gpuProps.m_nvChipName;
            checkNvPaStatus(NVPW_D3D12_RawCounterConfig_Create(&rawCountersConfigCreate));
            m_nvRawCounterConfig = rawCountersConfigCreate.pRawCounterConfig;

            // initialise CounterDataBuilder
            NVPW_CounterDataBuilder_Create_Params counterDataBuilderCreate = { NVPW_CounterDataBuilder_Create_Params_STRUCT_SIZE };
            counterDataBuilderCreate.pChipName = gpuProps.m_nvChipName;
            checkNvPaStatus(NVPW_CounterDataBuilder_Create(&counterDataBuilderCreate));
            m_nvCounterDataBuilder = counterDataBuilderCreate.pCounterDataBuilder;

            // query and remember existing clock status
            NVPW_Device_GetClockStatus_Params deviceGetClockStatus = { NVPW_Device_GetClockStatus_Params_STRUCT_SIZE };
            deviceGetClockStatus.deviceIndex = gpuProps.m_nvDeviceIndex;
            checkNvPaStatus(NVPW_Device_GetClockStatus(&deviceGetClockStatus));

            NVPW_Device_SetClockSetting_Params deviceSetClockSettings = { NVPW_Device_SetClockSetting_Params_STRUCT_SIZE };
            deviceSetClockSettings.deviceIndex = gpuProps.m_nvDeviceIndex;
            deviceSetClockSettings.clockSetting = NVPW_DEVICE_CLOCK_SETTING_LOCK_TO_RATED_TDP;
            checkNvPaStatus(NVPW_Device_SetClockSetting(&deviceSetClockSettings));
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            gpuProps.m_amdGpa->GpaCreateSession(gpuProps.m_amdGpaContextId, kGpaSessionSampleTypeDiscreteCounter, &m_amdGpaSessionId);
        }
#endif
    }

    void BeginAddingCounters()
    {
#ifdef _GAMING_DESKTOP
        assert(m_isAddingCounters == 0 && "Adding Counters has been already started or never ended");
        m_isAddingCounters = 1;

        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // Start a new PassGroup so that subsequent AddMetrics() calls will succeed.
            // This will not result in optimal scheduling, but it obeys the principle of least surprise.
            NVPW_RawCounterConfig_GetAllAvailableRawCounterDomains_Params rccGetAllDomains = { NVPW_RawCounterConfig_GetAllAvailableRawCounterDomains_Params_STRUCT_SIZE };
            rccGetAllDomains.pRawCounterConfig = m_nvRawCounterConfig;
            rccGetAllDomains.pAvailableDomains = nullptr;
            checkNvPaStatus(NVPW_RawCounterConfig_GetAllAvailableRawCounterDomains(&rccGetAllDomains));
            m_nvRawCounterDomains.resize(rccGetAllDomains.numAvailableDomains);
            rccGetAllDomains.pAvailableDomains = m_nvRawCounterDomains.data();
            checkNvPaStatus(NVPW_RawCounterConfig_GetAllAvailableRawCounterDomains(&rccGetAllDomains));

            NVPW_RawCounterConfig_BeginPassGroup_Params rccBeginPassGroup = { NVPW_RawCounterConfig_BeginPassGroup_Params_STRUCT_SIZE };
            rccBeginPassGroup.pRawCounterConfig = m_nvRawCounterConfig;
            rccBeginPassGroup.numDomains = m_nvRawCounterDomains.size();
            rccBeginPassGroup.pDomains = m_nvRawCounterDomains.data();
            checkNvPaStatus(NVPW_RawCounterConfig_BeginPassGroup(&rccBeginPassGroup));
        }
#endif
    }

    void EndAddingCounters()
    {
#ifdef _GAMING_DESKTOP
        assert(m_isAddingCounters == 1 && "Adding Counters has been already finished.");
        m_isAddingCounters = 0;

        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // convert metric names to metric requests
            NVPW_MetricsEvaluator_ConvertMetricNameToMetricEvalRequest_Params meCvtMetricNameToMetricEvalReq = { NVPW_MetricsEvaluator_ConvertMetricNameToMetricEvalRequest_Params_STRUCT_SIZE };
            meCvtMetricNameToMetricEvalReq.pMetricsEvaluator = gpuProps.m_nvMetricsEvaluator;
            meCvtMetricNameToMetricEvalReq.metricEvalRequestStructSize = NVPW_MetricEvalRequest_STRUCT_SIZE;

            NVPW_MetricEvalRequest request = {};

            m_nvCounterRequests.reserve(m_requestedCounterNames.size());
            m_existingCounterNames.reserve(m_requestedCounterNames.size());
            m_counterDatas.reserve(m_requestedCounterNames.size());
            for (size_t i = 0; i < m_requestedCounterNames.size(); ++i)
            {
                meCvtMetricNameToMetricEvalReq.pMetricName = m_requestedCounterNames[i];
                meCvtMetricNameToMetricEvalReq.pMetricEvalRequest = &request;
                if (NVPA_STATUS_SUCCESS == NVPW_MetricsEvaluator_ConvertMetricNameToMetricEvalRequest(&meCvtMetricNameToMetricEvalReq))
                {
                    m_existingCounterNames.push_back(m_requestedCounterNames[i]);
                    m_nvCounterRequests.push_back(request);
                }
                else if (IsDebuggerPresent())
                {
                    // NOTE: An attempt to add the counter m_requestedCounterNames[i] failed.
                    // This is likely to happen due to the following reasons:
                    //      1. Attempt to add non-existing counter name. In that case, simply continue, the returned value for this counter will be zero
                    //      2. Attempt to add a counter with incorrect rollup/submetric. In that case, double check the documentation what's the correct rollup/submetric
                    __debugbreak();
                }
            }
            m_requestedCounterNames.clear();
            m_counterDatas.resize(m_existingCounterNames.size());
#if 0
            // determine the number of raw dependencies and optional raw dependencies
            NVPW_MetricsEvaluator_GetMetricRawDependencies_Params meGetMetricRawDependencies = { NVPW_MetricsEvaluator_GetMetricRawDependencies_Params_STRUCT_SIZE };
            meGetMetricRawDependencies.pMetricsEvaluator = gpuProps.m_nvMetricsEvaluator;
            meGetMetricRawDependencies.pMetricEvalRequests = m_nvCounterRequests.data();
            meGetMetricRawDependencies.numMetricEvalRequests = m_nvCounterRequests.size();
            meGetMetricRawDependencies.metricEvalRequestStructSize = NVPW_MetricEvalRequest_STRUCT_SIZE;
            meGetMetricRawDependencies.metricEvalRequestStrideSize = sizeof(NVPW_MetricEvalRequest);
            checkNvPaStatus(NVPW_MetricsEvaluator_GetMetricRawDependencies(&meGetMetricRawDependencies));

            // pre-allocate required number of raw dependencies and optional raw dependencies
            std::vector<const char *> metricRawDependencies(meGetMetricRawDependencies.numRawDependencies);
            std::vector<const char *> metricRawDependenciesOpt(meGetMetricRawDependencies.numOptionalRawDependencies);

            // retrieve raw dependencies and optional raw dependencies
            meGetMetricRawDependencies.ppRawDependencies = metricRawDependencies.data();
            meGetMetricRawDependencies.ppOptionalRawDependencies = metricRawDependenciesOpt.data();
            checkNvPaStatus(NVPW_MetricsEvaluator_GetMetricRawDependencies(&meGetMetricRawDependencies));

            // create raw metric requests
            std::vector<NVPA_RawMetricRequest> rawMetricRequests(meGetMetricRawDependencies.numRawDependencies + meGetMetricRawDependencies.numOptionalRawDependencies);
            NVPA_RawMetricRequest rawMetricRequest = { NVPA_RAW_METRIC_REQUEST_STRUCT_SIZE };
            for (size_t i = 0; i < metricRawDependencies.size(); ++i)
            {
                rawMetricRequests[i] = rawMetricRequest;
                rawMetricRequests[i].pMetricName = metricRawDependencies[i];
                rawMetricRequests[i].keepInstances = true;
            }
            for (size_t j = 0; j < metricRawDependenciesOpt.size(); ++j)
            {
                rawMetricRequests[meGetMetricRawDependencies.numRawDependencies + j] = rawMetricRequest;
                rawMetricRequests[meGetMetricRawDependencies.numRawDependencies + j].pMetricName = metricRawDependenciesOpt[j];
                rawMetricRequests[meGetMetricRawDependencies.numRawDependencies + j].keepInstances = true;
            }

            // submit raw metric requests into CounterDataBuilder
            NVPW_CounterDataBuilder_AddMetrics_Params cdbAddMetric = { NVPW_CounterDataBuilder_AddMetrics_Params_STRUCT_SIZE };
            cdbAddMetric.pCounterDataBuilder = m_nvCounterDataBuilder;
            cdbAddMetric.pRawMetricRequests = rawMetricRequests.data();
            cdbAddMetric.numMetricRequests = rawMetricRequests.size();
            checkNvPaStatus(NVPW_CounterDataBuilder_AddMetrics(&cdbAddMetric));

            // submit raw metric requests into RawMetricConfig
            NVPW_RawMetricsConfig_AddMetrics_Params rccAddCounters = { NVPW_RawMetricsConfig_AddMetrics_Params_STRUCT_SIZE };
            rccAddCounters.pRawMetricsConfig = m_nvRawMetricsConfig;
            rccAddCounters.pRawMetricRequests = rawMetricRequests.data();
            rccAddCounters.numMetricRequests = rawMetricRequests.size();
            checkNvPaStatus(NVPW_RawMetricsConfig_AddMetrics(&rccAddCounters));
#else
            // determine the number of raw dependencies and optional raw dependencies
            NVPW_MetricsEvaluator_GetMetricRawDependencies_Params meGetMetricRawDependencies = { NVPW_MetricsEvaluator_GetMetricRawDependencies_Params_STRUCT_SIZE };
            meGetMetricRawDependencies.pMetricsEvaluator = gpuProps.m_nvMetricsEvaluator;
            meGetMetricRawDependencies.pMetricEvalRequests = m_nvCounterRequests.data();
            meGetMetricRawDependencies.numMetricEvalRequests = m_nvCounterRequests.size();
            meGetMetricRawDependencies.metricEvalRequestStructSize = NVPW_MetricEvalRequest_STRUCT_SIZE;
            meGetMetricRawDependencies.metricEvalRequestStrideSize = sizeof(NVPW_MetricEvalRequest);

            for (size_t i = 0; i < m_nvCounterRequests.size(); ++i)
            {
                meGetMetricRawDependencies.pMetricEvalRequests = &m_nvCounterRequests[i];
                meGetMetricRawDependencies.numMetricEvalRequests = 1;
                meGetMetricRawDependencies.ppRawDependencies = nullptr;
                meGetMetricRawDependencies.ppOptionalRawDependencies = nullptr;
                checkNvPaStatus(NVPW_MetricsEvaluator_GetMetricRawDependencies(&meGetMetricRawDependencies));

                std::vector<const char*> metricRawDependencies(meGetMetricRawDependencies.numRawDependencies);
                std::vector<const char*> metricRawDependenciesOpt(meGetMetricRawDependencies.numOptionalRawDependencies);

                // retrieve raw dependencies and optional raw dependencies
                meGetMetricRawDependencies.ppRawDependencies = metricRawDependencies.data();
                meGetMetricRawDependencies.ppOptionalRawDependencies = metricRawDependenciesOpt.data();
                checkNvPaStatus(NVPW_MetricsEvaluator_GetMetricRawDependencies(&meGetMetricRawDependencies));

                for (size_t j = 0; j < metricRawDependencies.size(); ++j)
                {
                    NVPW_RawCounterRequest rawCounterRequest = { 0 };
                    rawCounterRequest.pRawCounterName = metricRawDependencies[j];
                    rawCounterRequest.keepInstances = true;
                    rawCounterRequest.domain = NVPW_RAW_COUNTER_DOMAIN_INVALID;

                    NVPW_CounterDataBuilder_AddRawCounters_Params cdbAddRawCounters = { NVPW_CounterDataBuilder_AddRawCounters_Params_STRUCT_SIZE };
                    cdbAddRawCounters.pCounterDataBuilder = m_nvCounterDataBuilder;
                    cdbAddRawCounters.rawCounterRequestStructSize = NVPW_RAW_COUNTER_REQUEST_STRUCT_SIZE;
                    cdbAddRawCounters.numRawCounterRequests = 1;
                    cdbAddRawCounters.pRawCounterRequests = &rawCounterRequest;
                    checkNvPaStatus(NVPW_CounterDataBuilder_AddRawCounters(&cdbAddRawCounters));

                    // submit raw metric requests into RawMetricConfig
                    NVPW_RawCounterConfig_AddRawCounters_Params rccAddCounters = { NVPW_RawCounterConfig_AddRawCounters_Params_STRUCT_SIZE };
                    rccAddCounters.pRawCounterConfig = m_nvRawCounterConfig;
                    rccAddCounters.rawCounterRequestStructSize = NVPW_RAW_COUNTER_REQUEST_STRUCT_SIZE;
                    rccAddCounters.numRawCounterRequests = 1;
                    rccAddCounters.pRawCounterRequests = &rawCounterRequest;
                    checkNvPaStatus(NVPW_RawCounterConfig_AddRawCounters(&rccAddCounters));
                }

                for (size_t j = 0; j < metricRawDependenciesOpt.size(); ++j)
                {
                    NVPW_RawCounterRequest rawCounterRequest = { 0 };
                    rawCounterRequest.pRawCounterName = metricRawDependenciesOpt[j];
                    rawCounterRequest.keepInstances = true;
                    rawCounterRequest.domain = NVPW_RAW_COUNTER_DOMAIN_INVALID;

                    NVPW_CounterDataBuilder_AddRawCounters_Params cdbAddRawCounters = { NVPW_CounterDataBuilder_AddRawCounters_Params_STRUCT_SIZE };
                    cdbAddRawCounters.pCounterDataBuilder = m_nvCounterDataBuilder;
                    cdbAddRawCounters.rawCounterRequestStructSize = NVPW_RAW_COUNTER_REQUEST_STRUCT_SIZE;
                    cdbAddRawCounters.numRawCounterRequests = 1;
                    cdbAddRawCounters.pRawCounterRequests = &rawCounterRequest;
                    checkNvPaStatus(NVPW_CounterDataBuilder_AddRawCounters(&cdbAddRawCounters));

                    // submit raw metric requests into RawMetricConfig
                    NVPW_RawCounterConfig_AddRawCounters_Params rccAddCounters = { NVPW_RawCounterConfig_AddRawCounters_Params_STRUCT_SIZE };
                    rccAddCounters.pRawCounterConfig = m_nvRawCounterConfig;
                    rccAddCounters.numRawCounterRequests = 1;
                    rccAddCounters.pRawCounterRequests = &rawCounterRequest;
                    checkNvPaStatus(NVPW_RawCounterConfig_AddRawCounters(&rccAddCounters));
                }
            }
#endif

            NVPW_RawCounterConfig_EndPassGroup_Params rccEndPassGroupParam = { NVPW_RawCounterConfig_EndPassGroup_Params_STRUCT_SIZE };
            rccEndPassGroupParam.pRawCounterConfig = m_nvRawCounterConfig;
            rccEndPassGroupParam.numDomains = m_nvRawCounterDomains.size();
            rccEndPassGroupParam.pDomains = m_nvRawCounterDomains.data();
            checkNvPaStatus(NVPW_RawCounterConfig_EndPassGroup(&rccEndPassGroupParam));

            NVPW_RawCounterConfig_GenerateConfigImage_Params rccGenConfigImage = { NVPW_RawCounterConfig_GenerateConfigImage_Params_STRUCT_SIZE };
            rccGenConfigImage.pRawCounterConfig = m_nvRawCounterConfig;
            checkNvPaStatus(NVPW_RawCounterConfig_GenerateConfigImage(&rccGenConfigImage));

            // calculate size of the buffer required to store ConfigImage
            NVPW_RawCounterConfig_GetConfigImage_Params rccGetConfigImage = { NVPW_RawCounterConfig_GetConfigImage_Params_STRUCT_SIZE };
            rccGetConfigImage.pBuffer = nullptr;
            rccGetConfigImage.bytesAllocated = 0;
            rccGetConfigImage.pRawCounterConfig = m_nvRawCounterConfig;
            checkNvPaStatus(NVPW_RawCounterConfig_GetConfigImage(&rccGetConfigImage));

            //if (m_nvConfigImage.size() < rccGetConfigImage.bytesCopied)
            m_nvConfigImage.resize(rccGetConfigImage.bytesCopied);

            // fill in the buffer with the ConfigImage
            rccGetConfigImage.bytesAllocated = m_nvConfigImage.size();
            rccGetConfigImage.pBuffer = m_nvConfigImage.data();
            checkNvPaStatus(NVPW_RawCounterConfig_GetConfigImage(&rccGetConfigImage));

            // calculate size of the buffer required to store CounterDataPrefix
            NVPW_CounterDataBuilder_GetCounterDataPrefix_Params cdbGetCounterDataPrefix = { NVPW_CounterDataBuilder_GetCounterDataPrefix_Params_STRUCT_SIZE };
            cdbGetCounterDataPrefix.bytesAllocated = 0;
            cdbGetCounterDataPrefix.pBuffer = nullptr;
            cdbGetCounterDataPrefix.pCounterDataBuilder = m_nvCounterDataBuilder;
            checkNvPaStatus(NVPW_CounterDataBuilder_GetCounterDataPrefix(&cdbGetCounterDataPrefix));

            //if (m_nvCounterDataPrefix.size() < cdbGetCounterDataPrefix.bytesCopied)
            m_nvCounterDataPrefix.resize(cdbGetCounterDataPrefix.bytesCopied);

            // fill in the buffer with the CounterDataPrefix
            cdbGetCounterDataPrefix.bytesAllocated = m_nvCounterDataPrefix.size();
            cdbGetCounterDataPrefix.pBuffer = m_nvCounterDataPrefix.data();
            checkNvPaStatus(NVPW_CounterDataBuilder_GetCounterDataPrefix(&cdbGetCounterDataPrefix));

            // get num passes
            NVPW_Config_GetNumPasses_V2_Params configGetNumPasses = { NVPW_Config_GetNumPasses_V2_Params_STRUCT_SIZE };
            configGetNumPasses.pConfig = m_nvConfigImage.data();
            checkNvPaStatus(NVPW_Config_GetNumPasses_V2(&configGetNumPasses));
            m_numPasses = static_cast<uint32_t>(configGetNumPasses.numPasses);
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            const size_t numRequestedCounters = m_requestedCounterNames.size();

            m_amdGpaCounterIndicesPerRequestedCounter.reserve(2 * numRequestedCounters);

            std::regex isNonRegexString("[A-Z0-9_]*");
            for (size_t i = 0; i < numRequestedCounters; ++i)
            {
                m_amdRequestedCounterIndexToGpaCountersIndicesStart.push_back(
                    static_cast<uint32_t>(m_amdGpaCounterIndicesPerRequestedCounter.size())
                );
                /**
                 *  This is the fast path: we check if requested counter name is non-regex string, so we try to use it
                 *  as direct counter name and enable
                 */
                if (std::regex_match(m_requestedCounterNames[i], isNonRegexString))
                {
                    uint32_t index = 0;
                    // check if counter exists/supported
                    GpaStatus gpaStatus = gpuProps.m_amdGpa->GpaGetCounterIndex(m_amdGpaSessionId, m_requestedCounterNames[i], &index);

                    if (gpaStatus == kGpaStatusErrorCounterNotFound)
                    {
                        // do nothing, we only check this error explicitly because it's handled -- we simply don't enable the counter
                    }
                    else
                    {
                        checkAmdGpaStatus(gpaStatus);
                        checkAmdGpaStatus(gpuProps.m_amdGpa->GpaEnableCounter(m_amdGpaSessionId, index));
                        m_amdGpaCounterIndicesPerRequestedCounter.push_back(index);
                    }
                }
                else
                {
                    /**
                     *  Otherwise, if counter name is a regular expression -- iterate over all counters,
                     *  find matches and enable them
                     */
                    uint32_t counterCount = 0;
                    checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetNumCounters(m_amdGpaSessionId, &counterCount));

                    std::regex requestedCounterAsRegex(m_requestedCounterNames[i]);

                    for (uint32_t counterIndex = 0; counterIndex < counterCount; ++counterIndex)
                    {
                        const char *counterName = nullptr;
                        checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetCounterName(m_amdGpaSessionId, counterIndex, &counterName));

                        if (std::regex_match(counterName, requestedCounterAsRegex))
                        {
                            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaEnableCounter(m_amdGpaSessionId, counterIndex));
                            m_amdGpaCounterIndicesPerRequestedCounter.push_back(counterIndex);
                        }
                    }
                }
            }
            uint32_t numEnabledCounters = 0;
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetNumEnabledCounters(m_amdGpaSessionId, &numEnabledCounters));

            std::map<uint32_t, uint32_t> gpaCounterIndexToSampleOffset;

            // traverse enabled counters in the order they are going to be stored in the final sample to get their original indices and names
            for (uint32_t i = 0; i < numEnabledCounters; ++i)
            {
                uint32_t enabledIndex = ~0u;

                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetEnabledIndex(m_amdGpaSessionId, i, &enabledIndex));
                gpaCounterIndexToSampleOffset[enabledIndex] = i;
            }
            // convert counter indices to offsets within sample and data types to use during sample retrieval
            m_amdGpaCounterSampleOffsetsPerRequestedCounter.reserve(m_amdGpaCounterIndicesPerRequestedCounter.size());
            m_amdGpaCounterDataTypesPerRequestedCounter.reserve(m_amdGpaCounterIndicesPerRequestedCounter.size());

            for (size_t i = 0; i < m_amdGpaCounterIndicesPerRequestedCounter.size(); ++i)
            {
                GpaDataType counterDataType = kGpaDataTypeLast;
                uint32_t counterIndex = m_amdGpaCounterIndicesPerRequestedCounter[i];
                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetCounterDataType(m_amdGpaSessionId, counterIndex, &counterDataType));

                m_amdGpaCounterSampleOffsetsPerRequestedCounter.push_back(gpaCounterIndexToSampleOffset[counterIndex]);
                m_amdGpaCounterDataTypesPerRequestedCounter.push_back(counterDataType);
            }

            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetPassCount(m_amdGpaSessionId, &m_numPasses));
        }
#endif
    }

#ifdef _GAMING_XBOX
    template<typename t_counterType>
    void AddCounter(t_counterType id, GpuCounter::ShaderMask shaderMask = GpuCounter::SHADER_MASK_ALL)
    {
        GpuCounter counter(id, shaderMask);
        CounterTracker tracker = {};

        bool foundSlot = false;
        for (auto& counterPass : m_counterPasses)
        {
            if (counterPass.m_desc.SQ_SHADER_MASK == shaderMask)
            {
                for (tracker.m_arrayIndex = 0; tracker.m_arrayIndex < counter.GetArrayCount(); ++tracker.m_arrayIndex)
                {
                    auto existingId = counter.GetId(counterPass.m_desc, tracker.m_arrayIndex);
                    if (UINT(-1) == existingId)
                    {
                        counter.SetId(counterPass.m_desc, tracker.m_arrayIndex, id);
                        foundSlot = true;
                        break;
                    }
                    else if (UINT(id) == existingId)
                    {
                        foundSlot = true;
                        break;
                    }
                }
                if (foundSlot)
                {
                    break;
                }
            }

            ++tracker.m_counterPassIndex;
        }

        if (!foundSlot)
        {
            // A new pass is required
            tracker.m_arrayIndex = 0;
            tracker.m_counterPassIndex = m_counterPasses.size();
            m_counterPasses.emplace_back();
            auto& counterPass = m_counterPasses.back();
            memset(&counterPass.m_desc, 0xff, sizeof(counterPass.m_desc));      // Initialize to INVALID_INDEX
            counterPass.m_desc.Size = sizeof(counterPass.m_desc);               // Size of struct
            counterPass.m_desc.Version = D3D12XBOX_COUNTER_SET_DESC_VERSION;    // current version
            counterPass.m_desc.SQ_SHADER_MASK = shaderMask;                     // 0 means "all shader stages"
            counterPass.m_desc.MC_CITF_CID = 0xb3;                              // 0xb3 == MC_CID_DMIF_RD measures display output bandwidth
            counter.SetId(counterPass.m_desc, tracker.m_arrayIndex, id);
        }

        m_counterMap[counter] = tracker;
    }

#else // #ifdef _GAMING_DESKTOP

    void AddCounter(const char *counterName)
    {
        assert(m_isAddingCounters == 1 && "Adding Counters has been already finished.");

        m_requestedCounterNames.push_back(counterName);
    }
#endif // #ifdef _GAMING_XBOX

    bool Done()
    {
#ifdef _GAMING_XBOX
        return m_counterPasses.size() == m_counterPassIndex;
#else
        return m_numPasses == m_counterPassIndex;
#endif
    }

#ifdef _GAMING_DESKTOP
    static void SpgoThreadProc(GpuCounterSet *counterSet, ID3D12CommandQueue* commandQueue)
    {
        // Run continuously in the background, handling all BeginPass and EndPass GPU operations until EndSession().
        NVPW_D3D12_Queue_ServicePendingGpuOperations_Params serviceGpuOpsParams = { NVPW_D3D12_Queue_ServicePendingGpuOperations_Params_STRUCT_SIZE };
        serviceGpuOpsParams.pCommandQueue = commandQueue;
        serviceGpuOpsParams.numOperations = 0; // run until EndSession()
        serviceGpuOpsParams.timeout = INFINITE;
        checkNvPaStatus(NVPW_D3D12_Queue_ServicePendingGpuOperations(&serviceGpuOpsParams));

        counterSet->m_nvSpgoThreadExited = 1;
    }
#endif // #ifdef _GAMING_DESKTOP

    void StartCollect(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
    {
#ifdef _GAMING_XBOX
        (void)commandQueue;
        for (auto& counterPass : m_counterPasses)
        {
            auto& counterSetDesc = counterPass.m_desc;
            auto& counterSet = counterPass.m_set;
            auto& counterBuffer = counterPass.m_buffer;
            auto& counterSize = counterPass.m_size;

            DX::ThrowIfFailed(device->CreateCounterSetX(&counterSetDesc, IID_GRAPHICS_PPV_ARGS(counterSet.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(counterSet);

            counterSize = counterSet->GetCounterDataSizeX();

            auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            static_assert(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT >= sizeof(UINT64), "Counters require at least 8-byte alignment");
            auto descCounterBuffer = CD3DX12_RESOURCE_DESC::Buffer(2U * counterSize, D3D12_RESOURCE_FLAG_NONE, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descCounterBuffer,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(counterBuffer.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(counterBuffer);
        }
#else // #ifdef _GAMING_DESKTOP
        (void)device;
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            /** NOTE: we choose to set it to "1" because GpuBench runs each benchmark test one by one with a full GPU stop, so we don't need to support multiple ranges */
            static const uint32_t kMaxRangesPerPass = 1;

            /** NOTE: an arbitrary number long enough to keep most range names */
            static const uint32_t kAvgRangeNameLength = 128;

            NVPW_D3D12_Profiler_CalcTraceBufferSize_Params d3d12profCalcTraceBufferSize = { NVPW_D3D12_Profiler_CalcTraceBufferSize_Params_STRUCT_SIZE };

            d3d12profCalcTraceBufferSize.maxRangesPerPass = kMaxRangesPerPass;
            d3d12profCalcTraceBufferSize.avgRangeNameLength = kAvgRangeNameLength;
            checkNvPaStatus(NVPW_D3D12_Profiler_CalcTraceBufferSize(&d3d12profCalcTraceBufferSize));

            NVPW_D3D12_Profiler_Queue_BeginSession_Params d3d12profQueueBeginSession = { NVPW_D3D12_Profiler_Queue_BeginSession_Params_STRUCT_SIZE };
            d3d12profQueueBeginSession.pCommandQueue = commandQueue;

            /** NOTE: we choose "1" here because GpuBench executes benchmark test's command buffer ones, stops GPU and reads back the result, so the trace buffer can be reused */
            d3d12profQueueBeginSession.numTraceBuffers = m_numPasses;
            d3d12profQueueBeginSession.traceBufferSize = d3d12profCalcTraceBufferSize.traceBufferSize;
            d3d12profQueueBeginSession.maxRangesPerPass = d3d12profCalcTraceBufferSize.maxRangesPerPass;
            d3d12profQueueBeginSession.maxLaunchesPerPass = d3d12profCalcTraceBufferSize.maxRangesPerPass;
            NVPA_Status nvpaStatus = NVPW_D3D12_Profiler_Queue_BeginSession(&d3d12profQueueBeginSession);
            if (nvpaStatus)
            {
                if (nvpaStatus == NVPA_STATUS_INSUFFICIENT_PRIVILEGE)
                {
                    assert(!"Failed to start profiler session: profiling permissions not enabled.  Please follow these instructions: https://developer.nvidia.com/ERR_NVGPUCTRPERM\n");
                }
                else if (nvpaStatus == NVPA_STATUS_INSUFFICIENT_DRIVER_VERSION)
                {
                    assert(!"Failed to start profiler session: insufficient driver version.  Please install the latest NVIDIA driver from https://www.nvidia.com\n");
                }
                else if(nvpaStatus == NVPA_STATUS_RESOURCE_UNAVAILABLE)
                {
                    assert(!"Failed to start profiler session: resource conflict - only one profiler session can run at a time per GPU.\n");
                }
                else if(nvpaStatus == NVPA_STATUS_INVALID_OBJECT_STATE)
                {
                    assert(!"Failed to start profiler session: a profiler session already exists.\n");
                }
                else
                {
                    assert(!"Failed to start profiler session: unknown error.");
                }
                return;
            }
            // run thread doing data retrieval
            m_nvSpgoThreadExited = 0;
            m_nvSpgoThread = std::thread(SpgoThreadProc, this, commandQueue);

            // TODO: try to move counter buffers estimation and allocation to the `EndAddingCounters`,
            // otherwise we just keep calling this code for all tests in the benchmark and for all passes, but in reality the counters don't change,
            // so the data stays the same

            // setup the options required to determine the memory size required for `m_nvCounterDataImage` -- the buffer containing HW counters
            NVPW_D3D12_Profiler_CounterDataImageOptions d3d12profCounterDataImageOptions = { NVPW_D3D12_Profiler_CounterDataImageOptions_STRUCT_SIZE };
            d3d12profCounterDataImageOptions.pCounterDataPrefix     = m_nvCounterDataPrefix.data();
            d3d12profCounterDataImageOptions.counterDataPrefixSize  = m_nvCounterDataPrefix.size();
            d3d12profCounterDataImageOptions.maxNumRanges           = kMaxRangesPerPass;
            d3d12profCounterDataImageOptions.maxNumRangeTreeNodes   = kMaxRangesPerPass * 2;
            d3d12profCounterDataImageOptions.maxRangeNameLength     = kAvgRangeNameLength;

            // calculate the memory size
            NVPW_D3D12_Profiler_CounterDataImage_CalculateSize_Params d3d12profcdiCalculateSize = { NVPW_D3D12_Profiler_CounterDataImage_CalculateSize_Params_STRUCT_SIZE };
            d3d12profcdiCalculateSize.pOptions                      = &d3d12profCounterDataImageOptions;
            d3d12profcdiCalculateSize.counterDataImageOptionsSize   = NVPW_D3D12_Profiler_CounterDataImageOptions_STRUCT_SIZE;
            checkNvPaStatus(NVPW_D3D12_Profiler_CounterDataImage_CalculateSize(&d3d12profcdiCalculateSize));

            // resize the opaque memory blob to the required size
            //if (m_nvCounterDataImage.size() < d3d12profcdiCalculateSize.counterDataImageSize)
            m_nvCounterDataImage.resize(d3d12profcdiCalculateSize.counterDataImageSize);

            // intialize opaque memory blob
            NVPW_D3D12_Profiler_CounterDataImage_Initialize_Params d3d12profcdiInitialize = { NVPW_D3D12_Profiler_CounterDataImage_Initialize_Params_STRUCT_SIZE };
            d3d12profcdiInitialize.counterDataImageOptionsSize  = NVPW_D3D12_Profiler_CounterDataImageOptions_STRUCT_SIZE;
            d3d12profcdiInitialize.pOptions                     = &d3d12profCounterDataImageOptions;
            d3d12profcdiInitialize.counterDataImageSize         = m_nvCounterDataImage.size();
            d3d12profcdiInitialize.pCounterDataImage            = m_nvCounterDataImage.data();
            checkNvPaStatus(NVPW_D3D12_Profiler_CounterDataImage_Initialize(&d3d12profcdiInitialize));

            // calculate the memory size required for `m_nvCounterDataScratch` -- the buffer needed to decode HW counters
            NVPW_D3D12_Profiler_CounterDataImage_CalculateScratchBufferSize_Params d3d12profcdiCalculateScratchBufferSize = { NVPW_D3D12_Profiler_CounterDataImage_CalculateScratchBufferSize_Params_STRUCT_SIZE };
            d3d12profcdiCalculateScratchBufferSize.counterDataImageSize = m_nvCounterDataImage.size();
            d3d12profcdiCalculateScratchBufferSize.pCounterDataImage    = m_nvCounterDataImage.data();
            checkNvPaStatus(NVPW_D3D12_Profiler_CounterDataImage_CalculateScratchBufferSize(&d3d12profcdiCalculateScratchBufferSize));

            // if (m_nvCounterDataScratch.size() < d3d12profcdiCalculateScratchBufferSize.counterDataScratchBufferSize)
            m_nvCounterDataScratch.resize(d3d12profcdiCalculateScratchBufferSize.counterDataScratchBufferSize);

            NVPW_D3D12_Profiler_CounterDataImage_InitializeScratchBuffer_Params d3d12profcdiInitializeScratchBuffer = { NVPW_D3D12_Profiler_CounterDataImage_InitializeScratchBuffer_Params_STRUCT_SIZE };
            d3d12profcdiInitializeScratchBuffer.counterDataImageSize        = m_nvCounterDataImage.size();
            d3d12profcdiInitializeScratchBuffer.pCounterDataImage           = m_nvCounterDataImage.data();
            d3d12profcdiInitializeScratchBuffer.counterDataScratchBufferSize= m_nvCounterDataScratch.size();
            d3d12profcdiInitializeScratchBuffer.pCounterDataScratchBuffer   = m_nvCounterDataScratch.data();

            checkNvPaStatus(NVPW_D3D12_Profiler_CounterDataImage_InitializeScratchBuffer(&d3d12profcdiInitializeScratchBuffer));

            NVPW_D3D12_Profiler_Queue_SetConfig_Params d3d12profQueueSetConfig = { NVPW_D3D12_Profiler_Queue_SetConfig_Params_STRUCT_SIZE };
            d3d12profQueueSetConfig.pCommandQueue       = commandQueue;
            d3d12profQueueSetConfig.pConfig             = m_nvConfigImage.data();
            d3d12profQueueSetConfig.configSize          = m_nvConfigImage.size();
            d3d12profQueueSetConfig.minNestingLevel     = 1;
            d3d12profQueueSetConfig.numNestingLevels    = 1;
            d3d12profQueueSetConfig.passIndex           = 0;
            d3d12profQueueSetConfig.targetNestingLevel  = 1;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_SetConfig(&d3d12profQueueSetConfig));
        }
        else if (gpuProps.IsSupportedAmdGpu() && m_amdGpaCounterIndicesPerRequestedCounter.size() > 0)
        {
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaBeginSession(m_amdGpaSessionId));
        }
#endif // #ifdef _GAMING_XBOX
    }

    void StartPass(ID3D12GraphicsCommandList* commandList)
    {
#ifdef _GAMING_XBOX
        auto& counterSet = m_counterPasses[m_counterPassIndex].m_set;
        auto& counterBuffer = m_counterPasses[m_counterPassIndex].m_buffer;
        auto& counterSize = m_counterPasses[m_counterPassIndex].m_size;

        commandList->CommandListFunction()->StartCountersX(commandList, counterSet.Get());
        commandList->CommandListFunction()->SampleCountersX(commandList, counterBuffer.Get(), 0U * counterSize);

#else // #ifdef _GAMING_DESKTOP
        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedAmdGpu() && m_amdGpaCounterIndicesPerRequestedCounter.size() > 0)
        {
            //if (m_amdGpaCmdListId == nullptr)
                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaBeginCommandList(m_amdGpaSessionId, m_counterPassIndex, commandList, kGpaCommandListPrimary, &m_amdGpaCmdListId));
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaBeginSample(0, m_amdGpaCmdListId));
        }
#endif // #ifdef _GAMING_XBOX
    }

    void EndPass(ID3D12GraphicsCommandList* commandList)
    {
#ifdef _GAMING_XBOX
        auto& counterBuffer = m_counterPasses[m_counterPassIndex].m_buffer;
        auto& counterSize = m_counterPasses[m_counterPassIndex].m_size;

        commandList->CommandListFunction()->SampleCountersX(commandList, counterBuffer.Get(), 1U * counterSize);
        commandList->CommandListFunction()->StopCountersX(commandList);

        ++m_counterPassIndex;
#else // #ifdef _GAMING_DESKTOP
        (void)commandList;
        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedAmdGpu() && m_amdGpaCounterIndicesPerRequestedCounter.size() > 0)
        {
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaEndSample(m_amdGpaCmdListId));

            //if (m_counterPassIndex == m_numPasses - 1)
            {
                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaEndCommandList(m_amdGpaCmdListId));
                m_amdGpaCmdListId = nullptr;
            }
        }
#endif // #ifdef _GAMING_XBOX
    }

    void StartExecutePass(ID3D12CommandQueue* commandQueue)
    {
#ifdef _GAMING_DESKTOP

        const auto & gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // specify where counter collection pass starts
            NVPW_D3D12_Profiler_Queue_BeginPass_Params d3d12profqueueBeginPass = { NVPW_D3D12_Profiler_Queue_BeginPass_Params_STRUCT_SIZE };
            d3d12profqueueBeginPass.pCommandQueue = commandQueue;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_BeginPass(&d3d12profqueueBeginPass));

            // specify the single range we use within pass
            NVPW_D3D12_Profiler_Queue_PushRange_Params d3d12profqueuePushRange = {NVPW_D3D12_Profiler_Queue_PushRange_Params_STRUCT_SIZE};
            d3d12profqueuePushRange.pRangeName      = "<default>";
            d3d12profqueuePushRange.rangeNameLength = 0;
            d3d12profqueuePushRange.pCommandQueue   = commandQueue;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_PushRange(&d3d12profqueuePushRange));
        }
#else // #ifdef _GAMING_XBOX
        (void)commandQueue;
#endif // #ifdef _GAMING_DESKTOP
    }

    void EndExecutePass(ID3D12CommandQueue* commandQueue)
    {
#ifdef _GAMING_DESKTOP
        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // pop the only <default> range we have
            NVPW_D3D12_Profiler_Queue_PopRange_Params d3d12profqueuePopRange = { NVPW_D3D12_Profiler_Queue_PopRange_Params_STRUCT_SIZE };
            d3d12profqueuePopRange.pCommandQueue = commandQueue;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_PopRange(&d3d12profqueuePopRange));

            // specify where counter collection ends
            NVPW_D3D12_Profiler_Queue_EndPass_Params d3d12profqueueEndPass = { NVPW_D3D12_Profiler_Queue_EndPass_Params_STRUCT_SIZE };
            d3d12profqueueEndPass.pCommandQueue = commandQueue;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_EndPass(&d3d12profqueueEndPass));

            ++m_counterPassIndex;
        }
        else if (gpuProps.IsSupportedAmdGpu() && m_amdGpaCounterIndicesPerRequestedCounter.size() > 0)
        {
            for (;;)
            {
                // according to the docs we need to wait for each pass to finish with kGpaStatusOk before re-using resources
                GpaStatus status = gpuProps.m_amdGpa->GpaIsPassComplete(m_amdGpaSessionId, m_counterPassIndex);
                if (status == kGpaStatusOk)
                {
                    break;

                }
                else
                {
                    Sleep(10);
                }
            }
            ++m_counterPassIndex;
        }
#else // #ifdef _GAMING_XBOX
        (void)commandQueue;
#endif // #ifdef _GAMING_DESKTOP
    }

    void EndCollect(ID3D12Device* /*device*/, ID3D12CommandQueue *commandQueue)
    {
#ifdef _GAMING_DESKTOP
        const auto & gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // finalize SPGO thread
            NVPW_D3D12_Profiler_Queue_EndSession_Params d3d12profqueueEndSession = { NVPW_D3D12_Profiler_Queue_EndSession_Params_STRUCT_SIZE };
            d3d12profqueueEndSession.pCommandQueue  = commandQueue;
            d3d12profqueueEndSession.timeout        = INFINITE;
            checkNvPaStatus(NVPW_D3D12_Profiler_Queue_EndSession(&d3d12profqueueEndSession));

            m_nvSpgoThread.join();
            m_nvSpgoThreadExited = 0;
        }
#else // #ifdef _GAMING_XBOX
        (void)commandQueue;
#endif // #ifdef _GAMING_DESKTOP
        Reset();
    }

    void RetrieveCounters(GpuCounterTable& counterTable, ID3D12CommandQueue *commandQueue)
    {
        (void)commandQueue;
#ifdef _GAMING_XBOX
        for (auto& counterPass : m_counterPasses)
        {
            auto& counterSet = counterPass.m_set;
            auto& counterDataStart = counterPass.m_dataStart;
            auto& counterDataStop = counterPass.m_dataStop;
            auto& counterBuffer = counterPass.m_buffer;
            auto& counterSize = counterPass.m_size;

            // We have already performed a full wait before entering
            auto ThrowIfFailedOrNotDone = [] (HRESULT hr)->void
            {
                if (FAILED(hr) || S_FALSE == hr)
                {
                    throw std::exception("Failure to retrieve GPU counters");
                }
            };
            ThrowIfFailedOrNotDone(counterSet->RetrieveCounterDataX(counterBuffer.Get(), 0U * counterSize, &counterDataStart));
            ThrowIfFailedOrNotDone(counterSet->RetrieveCounterDataX(counterBuffer.Get(), 1U * counterSize, &counterDataStop));
        }

        counterTable.clear();
        for (auto& pair : m_counterMap)
        {
            counterTable[pair.first] = RetrieveCounter(pair.first, pair.second);
        }
#else // ifdef _GAMING_DESKTOP
        auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu() && m_existingCounterNames.size() > 0)
        {
            // decode counters collected in this pass
            NVPW_D3D12_Profiler_Queue_DecodeCounters_Params d3d12profQueueDecodeCounters = { NVPW_D3D12_Profiler_Queue_DecodeCounters_Params_STRUCT_SIZE };
            d3d12profQueueDecodeCounters.pCommandQueue                  = commandQueue;
            d3d12profQueueDecodeCounters.counterDataImageSize           = m_nvCounterDataImage.size();
            d3d12profQueueDecodeCounters.pCounterDataImage              = m_nvCounterDataImage.data();
            d3d12profQueueDecodeCounters.counterDataScratchBufferSize   = m_nvCounterDataScratch.size();
            d3d12profQueueDecodeCounters.pCounterDataScratchBuffer      = m_nvCounterDataScratch.data();
            while (!d3d12profQueueDecodeCounters.allPassesCollected)
            {
                checkNvPaStatus(NVPW_D3D12_Profiler_Queue_DecodeCounters(&d3d12profQueueDecodeCounters));
            }

            {

                NVPW_MetricsEvaluator_SetDeviceAttributes_Params meSetDeviceAttributes = { NVPW_MetricsEvaluator_SetDeviceAttributes_Params_STRUCT_SIZE };
                meSetDeviceAttributes.pMetricsEvaluator     = gpuProps.m_nvMetricsEvaluator;
                meSetDeviceAttributes.pCounterDataImage     = m_nvCounterDataImage.data();
                meSetDeviceAttributes.counterDataImageSize  = m_nvCounterDataImage.size();
                checkNvPaStatus(NVPW_MetricsEvaluator_SetDeviceAttributes(&meSetDeviceAttributes));

                NVPW_CounterData_GetNumRanges_Params cdGetNumRanges = { NVPW_CounterData_GetRangeDescriptions_Params_STRUCT_SIZE };
                cdGetNumRanges.pCounterDataImage = m_nvCounterDataImage.data();
                checkNvPaStatus(NVPW_CounterData_GetNumRanges(&cdGetNumRanges));

                assert(cdGetNumRanges.numRanges == 1);

                NVPW_MetricsEvaluator_EvaluateToGpuValues_Params meEvaluateToGpuValues = { NVPW_MetricsEvaluator_EvaluateToGpuValues_Params_STRUCT_SIZE };
                meEvaluateToGpuValues.pMetricsEvaluator             = gpuProps.m_nvMetricsEvaluator;
                meEvaluateToGpuValues.pMetricEvalRequests           = m_nvCounterRequests.data();
                meEvaluateToGpuValues.numMetricEvalRequests         = m_nvCounterRequests.size();
                meEvaluateToGpuValues.metricEvalRequestStructSize   = NVPW_MetricEvalRequest_STRUCT_SIZE;
                meEvaluateToGpuValues.metricEvalRequestStrideSize   = sizeof(NVPW_MetricEvalRequest);
                meEvaluateToGpuValues.pCounterDataImage             = m_nvCounterDataImage.data();
                meEvaluateToGpuValues.counterDataImageSize          = m_nvCounterDataImage.size();
                meEvaluateToGpuValues.rangeIndex                    = 0; // hardcoded because of benchmark structure
                meEvaluateToGpuValues.pMetricValues                 = m_counterDatas.data();
                checkNvPaStatus(NVPW_MetricsEvaluator_EvaluateToGpuValues(&meEvaluateToGpuValues));
                for (size_t i = 0; i < m_existingCounterNames.size(); ++i)
                {
                    counterTable[m_existingCounterNames[i]] = m_counterDatas[i];
                }
            }
        }
        else if (gpuProps.IsSupportedAmdGpu() && m_amdGpaCounterIndicesPerRequestedCounter.size() > 0)
        {
            // can use this function for polling
            for (;;)
            {
                GpaStatus gpaStatus = gpuProps.m_amdGpa->GpaEndSession(m_amdGpaSessionId);
                if (kGpaStatusOk != gpaStatus)
                {
                    Sleep(10);
                }
                else
                {
                    break;
                }
            }

            uint32_t sampleCount = 0;
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetSampleCount(m_amdGpaSessionId, &sampleCount));

            size_t sampleSize = 0;
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetSampleResultSize(m_amdGpaSessionId, 0, &sampleSize));
            m_amdGpaSampleData.resize(sampleSize);

            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetSampleResult(m_amdGpaSessionId, 0, sampleSize, m_amdGpaSampleData.data()));

            union F64OrU64
            {
                uint64_t asU64;
                double   asF64;
            };
            // as all data is is either double or uint64_t, choose uint64_t stride
            const F64OrU64 *counterData = reinterpret_cast<const F64OrU64*>(m_amdGpaSampleData.data());

            // resolve individual counters
            m_counterDatas.clear();
            m_counterDatas.reserve(m_amdGpaCounterSampleOffsetsPerRequestedCounter.size());

            for (size_t i = 0; i < m_amdGpaCounterSampleOffsetsPerRequestedCounter.size(); ++i)
            {
                uint32_t sampleOffset = m_amdGpaCounterSampleOffsetsPerRequestedCounter[i];
                double counterValue = (kGpaDataTypeUint64 == m_amdGpaCounterDataTypesPerRequestedCounter[i])
                                    ? static_cast<double>(counterData[sampleOffset].asU64)
                                    : counterData[sampleOffset].asF64;

                m_counterDatas.push_back(counterValue);
            }

            // accumulate multiple counters based on regexp (if they are specified through regexp)
            for (size_t i = 0; i < m_amdRequestedCounterIndexToGpaCountersIndicesStart.size(); ++i)
            {
                uint32_t counterStart = m_amdRequestedCounterIndexToGpaCountersIndicesStart[i];
                uint32_t counterEnd = i == m_amdRequestedCounterIndexToGpaCountersIndicesStart.size() - 1
                                    ? static_cast<uint32_t>(m_counterDatas.size())
                                    : m_amdRequestedCounterIndexToGpaCountersIndicesStart[i + 1];

                double counterValue = 0.0;
                for (uint32_t j = counterStart; j < counterEnd; ++j)
                {
                    counterValue += m_counterDatas[j];
                }
                counterTable[m_requestedCounterNames[i]] = counterValue;
            }
        }
#endif
    }

    // To see counter values broken down into each hardware instance.
    // E.g. TCP counters for Anaconda show each of the 52 individual TCP values.
    void DumpCounters()
    {
        OutputDebugString(L"\n");
        OutputDebugString(L"Dumping counter set:\n");
        OutputDebugString(L"--------------------------------------------------------------------\n");

#ifdef _GAMING_XBOX
        for (const auto& pair : m_counterMap)
        {
            auto counter = pair.first;
            auto tracker = pair.second;

            auto counterPassIndex = tracker.m_counterPassIndex;
            auto arrayIndex = tracker.m_arrayIndex;
            const auto& counterPass = m_counterPasses[counterPassIndex];
            auto& counterSetDesc = counterPass.m_desc;
            const auto& counterDataStart = counterPass.m_dataStart;
            const auto& counterDataStop = counterPass.m_dataStop;

            std::wostringstream typeName;
            typeName << L"Type: ";
            typeName << counter.GetTypeName();
            typeName << std::endl;
            OutputDebugString(typeName.str().c_str());

            std::wostringstream counterId;
            counterId << L"Id: ";
            counterId << counter.GetId(counterSetDesc, arrayIndex);
            counterId << std::endl;
            OutputDebugString(counterId.str().c_str());

            std::wostringstream counterValues;
            counterValues << L"Values: {";
            for (auto instance = 0U; instance < counter.GetInstanceCount(); ++instance)
            {
                auto value = counter.GetValue(counterDataStop, arrayIndex, instance)
                    - counter.GetValue(counterDataStart, arrayIndex, instance);
                counterValues << value;
                counterValues << L", ";
            }
            counterValues << L"}";
            counterValues << std::endl;
            counterValues << std::endl;
            OutputDebugString(counterValues.str().c_str());
        }
#endif

        OutputDebugString(L"--------------------------------------------------------------------\n");
        OutputDebugString(L"\n");
    }

    void Uninitialize()
    {
#ifdef _GAMING_XBOX
        m_counterPasses.clear();
        m_counterMap.clear();
#else
        const auto & gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            NVPW_RawCounterConfig_Destroy_Params rccDestroy = { NVPW_RawMetricsConfig_Destroy_Params_STRUCT_SIZE };
            rccDestroy.pRawCounterConfig = m_nvRawCounterConfig;
            checkNvPaStatus(NVPW_RawCounterConfig_Destroy(&rccDestroy));
            m_nvRawCounterConfig = nullptr;

            NVPW_CounterDataBuilder_Destroy_Params counterDataBuilderDestroy = { NVPW_CounterDataBuilder_Destroy_Params_STRUCT_SIZE };
            counterDataBuilderDestroy.pCounterDataBuilder = m_nvCounterDataBuilder;
            checkNvPaStatus(NVPW_CounterDataBuilder_Destroy(&counterDataBuilderDestroy));
            m_nvCounterDataBuilder = nullptr;
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaDeleteSession(m_amdGpaSessionId));
            m_amdGpaSessionId = nullptr;

            m_amdRequestedCounterIndexToGpaCountersIndicesStart.clear();
            m_amdGpaCounterIndicesPerRequestedCounter.clear();
            m_amdGpaCounterSampleOffsetsPerRequestedCounter.clear();
            m_amdGpaCounterDataTypesPerRequestedCounter.clear();
        }
        m_requestedCounterNames.clear();
        m_existingCounterNames.clear();
        m_counterDatas.clear();
        m_numPasses = 0;
#endif
    }

private:

#ifdef _GAMING_XBOX
    struct CounterTracker
    {
        size_t                                          m_counterPassIndex;
        size_t                                          m_arrayIndex;
    };

    struct CounterPass
    {
        D3D12XBOX_COUNTER_SET_DESC                          m_desc{};
        Microsoft::WRL::ComPtr<ID3D12XboxCounterSet>        m_set;

        // TODO: Replace m_buffer with a single buffer covering all passes

        Microsoft::WRL::ComPtr<ID3D12Resource>              m_buffer;
        SIZE_T                                              m_size{};
        D3D12XBOX_COUNTER_DATA                              m_dataStart{};
        D3D12XBOX_COUNTER_DATA                              m_dataStop{};
    };
#endif

    void Reset()
    {
        m_counterPassIndex = 0;
#ifdef _GAMING_XBOX

        for (auto& counterPass : m_counterPasses)
        {
            auto& counterSet = counterPass.m_set;
            auto& counterBuffer = counterPass.m_buffer;

            counterSet.Reset();
            counterBuffer.Reset();
        }
#else
        const auto & gpuProps = GpuProperties::Get();
        const size_t numExistingCounters = m_amdGpaCounterIndicesPerRequestedCounter.size();
        if (gpuProps.IsSupportedAmdGpu() && numExistingCounters > 0)
        {
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaResetSession(m_amdGpaSessionId));

            for (size_t i = 0; i < numExistingCounters; ++i)
            {
                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaEnableCounter(m_amdGpaSessionId, m_amdGpaCounterIndicesPerRequestedCounter[i]));
            }
            uint32_t numEnabledCounters = 0;
            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetNumEnabledCounters(m_amdGpaSessionId, &numEnabledCounters));

            std::map<uint32_t, uint32_t> gpaCounterIndexToSampleOffset;

            // traverse enabled counters in the order they are going to be stored in the final sample
            for (uint32_t i = 0; i < numEnabledCounters; ++i)
            {
                uint32_t enabledIndex = ~0u;

                checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetEnabledIndex(m_amdGpaSessionId, i, &enabledIndex));
                gpaCounterIndexToSampleOffset[enabledIndex] = i;
            }

#ifdef _DEBUG
            // validate the sample offsets didn't change after sessition reset and 
            for (size_t i = 0; i < numExistingCounters; ++i)
            {
                uint32_t counterIndex = m_amdGpaCounterIndicesPerRequestedCounter[i];

                assert(gpaCounterIndexToSampleOffset[counterIndex] == m_amdGpaCounterSampleOffsetsPerRequestedCounter[i]);
            }
#endif

            checkAmdGpaStatus(gpuProps.m_amdGpa->GpaGetPassCount(m_amdGpaSessionId, &m_numPasses));
        }
#endif
    }

#ifdef _GAMING_XBOX
    CounterValueArray RetrieveCounter(const GpuCounter& counter, const CounterTracker& tracker) const
    {
        auto counterPassIndex = tracker.m_counterPassIndex;
        auto arrayIndex = tracker.m_arrayIndex;
        const auto& counterPass = m_counterPasses[counterPassIndex];
        const auto& counterDataStart = counterPass.m_dataStart;
        const auto& counterDataStop = counterPass.m_dataStop;

        CounterValueArray array;

        for (auto instance = 0U; instance < counter.GetInstanceCount(); ++instance)
        {
            array.push_back(counter.GetValue(counterDataStop, arrayIndex, instance)
                - counter.GetValue(counterDataStart, arrayIndex, instance));
        }
        return array;
    }

    // The GPU only supports retrieval of a limited number of counters per hardware component at one time
    // For arbitrary collections of counters, we may need to perform multiple passes
    std::map<GpuCounter, CounterTracker>                m_counterMap;
    std::vector<CounterPass>                            m_counterPasses;
#else
    NVPW_RawCounterConfig *m_nvRawCounterConfig;

    std::vector<NVPW_RawCounterDomain> m_nvRawCounterDomains;

    NVPA_CounterDataBuilder *m_nvCounterDataBuilder;

    // `m_nvConfigImage` stores the configuration settings for the counters, informing the profiler which counters to collect and how to collect them.
    // These settings are typically applied to a profiling session through the SetConfig API.
    // Note that a profiling session only stores a pointer to the counter config image, so it must remain valid for the entire duration of the profiling session,
    // or until the next SetConfig call with a different counter config image.
    std::vector<uint8_t> m_nvConfigImage;

    // `m_nvCounterDataPrefix` stores the header information for counter data, which includes the counters present.
    // This header is used to initialize the actual counter data, where counter values are stored.
    // It can be safely destroyed if no further counter data initialization is needed.
    std::vector<uint8_t> m_nvCounterDataPrefix;

    // opaque buffer containing HW counter data; updated in DecodeCounters on each frame
    std::vector<uint8_t> m_nvCounterDataImage;

    // opaque buffer needed by DecodeCounters
    std::vector<uint8_t> m_nvCounterDataScratch;

    std::vector<const char *> m_existingCounterNames;

    std::vector<const char *> m_requestedCounterNames;

    std::vector<NVPW_MetricEvalRequest> m_nvCounterRequests;

    std::vector<double> m_counterDatas;

    std::thread m_nvSpgoThread;

    volatile uint32_t m_nvSpgoThreadExited;

    std::vector<uint32_t> m_amdGpaCounterIndicesPerRequestedCounter;
    std::vector<uint32_t> m_amdGpaCounterSampleOffsetsPerRequestedCounter;
    std::vector<GpaDataType> m_amdGpaCounterDataTypesPerRequestedCounter;

    std::vector<uint32_t> m_amdRequestedCounterIndexToGpaCountersIndicesStart;
    std::vector<uint8_t> m_amdGpaSampleData;

    GpaSessionId m_amdGpaSessionId;
    GpaCommandListId m_amdGpaCmdListId;

    uint32_t m_numPasses;

    uint32_t m_isAddingCounters : 1;

#endif
    // Temporary data, tracked between StartCollect/EndCollect
    uint32_t                                            m_counterPassIndex;
};
