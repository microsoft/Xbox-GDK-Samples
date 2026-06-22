//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX_XBOXONE
#include "GpuCounterSetDurango.h"
#endif
#include "GpuProperties.h"
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

// Class which supports an arbitrary collection of GPU counters. We organize these counters
// into the minimal number of D3D12XBOX_COUNTER_SET_DESC needed to cover them all. Collection occurs
// in the minimal number of passes.
class GpuCounterSet
{
public:
    GpuCounterSet()
    {
        Reset();
    }

    void Initialize()
    {
    }

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

    bool Done() const 
    { 
        return m_counterPasses.size() == m_counterPassIndex;  
    }

    void StartCollect(ID3D12Device* device) 
    {
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
    }

    void StartPass(ID3D12GraphicsCommandList* commandList)
    {
        auto& counterSet = m_counterPasses[m_counterPassIndex].m_set;
        auto& counterBuffer = m_counterPasses[m_counterPassIndex].m_buffer;
        auto& counterSize = m_counterPasses[m_counterPassIndex].m_size;

        commandList->CommandListFunction()->StartCountersX(commandList, counterSet.Get());
        commandList->CommandListFunction()->SampleCountersX(commandList, counterBuffer.Get(), 0U * counterSize);
    }

    void EndPass(ID3D12GraphicsCommandList* commandList) 
    {
        auto& counterBuffer = m_counterPasses[m_counterPassIndex].m_buffer;
        auto& counterSize = m_counterPasses[m_counterPassIndex].m_size;

        commandList->CommandListFunction()->SampleCountersX(commandList, counterBuffer.Get(), 1U * counterSize);
        commandList->CommandListFunction()->StopCountersX(commandList);

        ++m_counterPassIndex;
    }

    void EndCollect(ID3D12Device* /*device*/) 
    {
        Reset();
    }

    void RetrieveCounters(GpuCounterTable& counterTable)
    {
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
    }

    // To see counter values broken down into each hardware instance.
    // E.g. TCP counters for Anaconda show each of the 52 individual TCP values.
    void DumpCounters()
    {
        OutputDebugString(L"\n");
        OutputDebugString(L"Dumping counter set:\n");
        OutputDebugString(L"--------------------------------------------------------------------\n");

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

        OutputDebugString(L"--------------------------------------------------------------------\n");
        OutputDebugString(L"\n");
    }

    void Uninitialize()
    {
        m_counterPasses.clear();
        m_counterMap.clear();
    }

private:
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

    void Reset() 
    {
        m_counterPassIndex = 0;

        for (auto& counterPass : m_counterPasses)
        {
            auto& counterSet = counterPass.m_set;
            auto& counterBuffer = counterPass.m_buffer;

            counterSet.Reset();
            counterBuffer.Reset();
        }
    }

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

    // Temporary data, tracked between StartCollect/EndCollect
    uint32_t                                            m_counterPassIndex;
};
