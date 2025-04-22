//--------------------------------------------------------------------------------------
// utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

constexpr uint32_t GPASS_RT_COUNT = 3;
constexpr DXGI_FORMAT GPASS_RT_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;

// IMPORTANT - this needs to match with the declaration in shared.h
#define NUM_INSTANCES       64

// Color space for the histogram
enum ColorSpace
{
    YCC = 0,
    RGB,
    COLOR_SPACE_NUM
};

#pragma region SDR Textures
static constexpr int c_NumImages = 8;
static wchar_t const* m_sdrTextureFiles[] =
{
    L"GOW4_1.DDS",
    L"GOW4_2.DDS",
    L"GOW4_4.DDS",
    L"Halo_1.DDS",
    L"Halo_2.DDS",
    L"Halo_4.DDS",
    L"Halo_3.DDS",
    L"GOW4_3.DDS",
};
static_assert(std::size(m_sdrTextureFiles) == c_NumImages &&
    L"Number of screenshots needs to match with size of array of paths.");
#pragma endregion

// Offsets into the descriptor pile SRV for each descriptor
enum SRV_HEAP_OFFSETS
{
    TEXT_FONT_OFFSET = 0,
    CONTROLLER_FONT_OFFSET,
    INTERMEDIATE_TARGET_SRV_0,
    INTERMEDIATE_TARGET_SRV_1,
    INTERMEDIATE_TARGET_UAV_0,
    INTERMEDIATE_TARGET_UAV_1,
    HISTOGRAM_UAV_0,
    HISTOGRAM_UAV_1,
    HISTOGRAM_SRV_0,
    HISTOGRAM_SRV_1,
    HISTOGRAM_READBACK_0,
    HISTOGRAM_READBACK_1,
    BG_TEXTURE_0,
    BG_TEXTURE_FINAL = BG_TEXTURE_0 + c_NumImages - 1,
    SCENE_TEXTURE_OFFSET,
};
static_assert(c_NumImages == (BG_TEXTURE_FINAL - BG_TEXTURE_0 + 1) &&
    L"Number of descriptor handles does not match number of textures.");

// Which method to use for picking color from histograms
enum COLOR_SELECTION_METHODS
{
    AVERAGE,
    TOP_X_BUCKETS,
    BUCKETS_X_AVG,
    COLOR_SELECTION_METHODS_COUNT,
};
static std::wstring wstrp_ColorSelectionTechniqueName[] =
{
    L"AVG",
    L"TOP_X_BUCKETS",
    L"BUCKETS_X_AVG",
};
static_assert(std::size(wstrp_ColorSelectionTechniqueName) == COLOR_SELECTION_METHODS_COUNT && "Sized do not match.");

static std::wstring wstrp_HistogramName[] =
{
    L"Red",
    L"Green",
    L"Blue",
    L"Luma",
    L"Cb",
    L"Cr",
};

struct SingleColor
{
    float r;
    float g;
    float b;
    float a;
};

// Histogram pass CB (already padded, does not need a wrapping pad structure)
struct AmbientConstants
{
    float r;
    float g;
    float b;
    float intensity;
};

struct VizConstantBuffer
{
    uint32_t NumBins;
    float Scale;
    uint32_t offset;
    float pad2;
};

// Histogram pass CB (already padded, does not need a wrapping pad structure)
struct CSConstantBuffer
{
    uint32_t width;
    uint32_t height;
    uint32_t pad[2];
};

// Describes each of the for the timer
enum GPU_TIMER_PASSES
{
    GPU_TIMER_GENERATE_HISTOGRAM,
    GPU_TIMER_TOTAL,
    GPU_TIMER_NUM
};

enum CPU_TIMER_PASSES
{
    CPU_TIMER_READBACK_AND_GEN_AMBIENT_COLOR,
    CPU_TIMER_FRAME_TIME,
    CPU_TIMER_NUM
};

struct lightPositionsCB
{
    DirectX::SimpleMath::Vector4    lightPositions[NUM_INSTANCES];
};
static_assert(sizeof(lightPositionsCB) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0
    && L"Bad allignment for constant buffer.");

// Struct representing scene constants, like camera related matrices.
// USed on multiple passes (ambient pass, light volumes, and tiled).
struct SceneConstantsStruct
{
    DirectX::SimpleMath::Matrix     matViewProj;
    DirectX::SimpleMath::Matrix     matView;
    DirectX::SimpleMath::Matrix     matInvProj;
    DirectX::SimpleMath::Vector3    cameraPos;
    uint32_t                        sceneLightCount;
    float                           lightRadius;
    uint32_t                        tileCorrectScreenWidth;
    uint32_t                        tileCorrectScreenHeight;
};
struct SceneConstantsStructPadded
{
    SceneConstantsStruct            data;
    uint8_t                         pad[1];
};
static_assert(sizeof(SceneConstantsStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0
    && L"Bad allignment for constant buffer.");

// Per object data, like model matrix.
// Used on Geometry pass.
struct PerObjectStruct
{
    DirectX::SimpleMath::Matrix worldMat;
    DirectX::SimpleMath::Matrix worldMatRotation;
};

class MovingAvg
{
public:
    MovingAvg(uint32_t capacity = 10, float alpha = 0.6f) :
        m_alpha(alpha),
        m_capacity(capacity),
        m_totalCount(0)
    {
    }

    ~MovingAvg()
    {
    }

    void AddValue(DirectX::SimpleMath::Vector3 const& newVal)
    {
        // For avg calculations
        m_totalSum += newVal;
        m_totalCount++;

        m_windowedSum += newVal;
        m_values.push_front(newVal);

        if (m_values.size() > m_capacity)
        {
            m_windowedSum -= m_values.back();
            m_values.pop_back();
        }

        assert(m_values.size() <= m_capacity);
    }

    DirectX::SimpleMath::Vector3 GetSMA()
    {
        return m_windowedSum / static_cast<float>(m_values.size());
    }

    DirectX::SimpleMath::Vector3 GetEMA(DirectX::SimpleMath::Vector3 const& newVal)
    {
        auto ema = m_alpha * (newVal) + (1.0f - m_alpha) * (m_prevEMA);
        m_prevEMA = ema;

        AddValue(newVal);

        return ema;
    }

    DirectX::SimpleMath::Vector3 GetAvg()
    {
        return m_totalSum / static_cast<float>(m_totalCount);
    }

    void UpdateAlpha(float newVal)
    {
        assert(newVal >= 0.0f && newVal <= 1.0f);
        m_alpha = newVal;
    }

private:
    std::list<DirectX::SimpleMath::Vector3> m_values;
    float m_alpha;

    uint32_t m_capacity;
    uint32_t m_totalCount;

    DirectX::SimpleMath::Vector3 m_totalSum;
    DirectX::SimpleMath::Vector3 m_windowedSum;
    DirectX::SimpleMath::Vector3 m_prevEMA;
};
