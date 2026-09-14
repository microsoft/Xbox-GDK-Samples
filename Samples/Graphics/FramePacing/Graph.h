//--------------------------------------------------------------------------------------
// Graph.h
//
// Helper code for rendering statistics as graphs on the screen
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DirectXColors.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "Utility.h"

namespace FramePacingUtils
{
    inline const DirectX::XMVECTORF32 c_intervalColors[] =
    {
        DirectX::Colors::LightGreen,
        DirectX::Colors::Yellow,
        DirectX::Colors::MediumSlateBlue,
        DirectX::Colors::Orange,
    };

    enum : uint32_t
    {
        LINE_GRAPH_FRAME_TIME_FLIP,
        LINE_GRAPH_FRAME_LATENCY,
        LINE_GRAPH_LATENCY_MARGIN,

        LINE_GRAPH_COUNT
    };

    inline constexpr const wchar_t* c_lineGraphNames[] =
    {
        L"Frame time (ms)",
        L"Ideal latency (ms)",
        L"Latency margin (ms)",
    };
    static_assert(_countof(c_lineGraphNames) == LINE_GRAPH_COUNT, "Mismatch between enum and reflected names");

    inline constexpr const char* c_lineGraphLayoutNames[] =
    {
        "frame_time_panel",
        "latency_panel",
        "margin_panel",
    };
    static_assert(_countof(c_lineGraphLayoutNames) == LINE_GRAPH_COUNT, "Mismatch between enum and reflected names");

    enum : uint32_t
    {
        INTERVAL_GRAPH_CPU_UPDATE,
        INTERVAL_GRAPH_CPU_RENDER,
        INTERVAL_GRAPH_GPU_GRAPHICS,
        INTERVAL_GRAPH_GPU_COMPUTE,
        INTERVAL_GRAPH_FLIP,

        INTERVAL_GRAPH_COUNT
    };

    inline constexpr const wchar_t* c_intervalGraphNames[] =
    {
        L"CPU update",
        L"CPU render",
        L"GPU graphics",
        L"GPU compute",
        L"Flip",
    };
    static_assert(_countof(c_intervalGraphNames) == INTERVAL_GRAPH_COUNT, "Mismatch between enum and reflected names");

    inline constexpr const char* c_intervalGraphLayoutNames[] =
    {
        "cpu_update_panel",
        "cpu_render_panel",
        "gpu_graphics_panel",
        "gpu_compute_panel",
        "flip_panel",
    };
    static_assert(_countof(c_intervalGraphLayoutNames) == INTERVAL_GRAPH_COUNT, "Mismatch between enum and reflected names");
template<typename DataType, typename InputType>
class Graph
{
protected:
    static constexpr uint32_t c_numData = 256;
    static constexpr float c_lengthInMs = 4000.0f;

public:
    Graph() noexcept
        : m_name(nullptr)
        , m_minX(0.0f)
        , m_maxX(0.0f)
        , m_minY(0.0f)
        , m_maxY(0.0f)
        , m_tickInterval(0.0f)
        , m_createData(nullptr)
    {}

    virtual ~Graph() = default;

    void Update(const InputType* input)
    {
        m_data.push_back(m_createData(input));
    }

    virtual void Render(
        ID3D12GraphicsCommandList* commandList,
        DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* primBatch,
        DirectX::BasicEffect* basicEffect,
        DirectX::SpriteBatch* spriteBatch,
        DirectX::SpriteFont* font,
        const D3D12_VIEWPORT* screenViewport,
        const D3D12_VIEWPORT* graphViewport) = 0;

    float ScaleX(float val)
    {
        return Saturate(((val - m_minX) / (m_maxX - m_minX) * 2.0f) - 1.0f, -0.999f, 0.999f);
    }

    float ScaleY(float val)
    {
        return Saturate(((val - m_minY) / (m_maxY - m_minY) * 2.0f) - 1.0f, -0.999f, 0.999f);
    }

    void ZoomX(float zoom, float offset)
    {
        m_minX = 1.0f - zoom + offset;
        m_maxX = 1.0f + offset;
    }

    const wchar_t* m_name;

    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;
    float m_tickInterval;

    using DataFactory = DataType(*)(const InputType* input);
    DataFactory m_createData;

    Ring< DataType, c_numData > m_data;
};

struct DataPoint
{
    double m_flipTimeInMs;
    float m_value;
};
class LineGraph : public Graph<DataPoint, FrameStatistics>
{
public:
    ~LineGraph() override = default;

    void Render(
        ID3D12GraphicsCommandList* commandList,
        DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* primBatch,
        DirectX::BasicEffect* basicEffect,
        DirectX::SpriteBatch* spriteBatch,
        DirectX::SpriteFont* font,
        const D3D12_VIEWPORT* screenViewport,
        const D3D12_VIEWPORT* graphViewport) override;
};

struct DataInterval
{
    double m_flipTimeInMs;
    double m_startInMs;
    double m_stopInMs;

    uint32_t m_frameBuffer;
};
class IntervalGraph : public Graph<DataInterval, FrameTimestamps>
{
public:
    ~IntervalGraph() override = default;

    void Render(
        ID3D12GraphicsCommandList* commandList,
        DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* primBatch,
        DirectX::BasicEffect* basicEffect,
        DirectX::SpriteBatch* spriteBatch,
        DirectX::SpriteFont* font,
        const D3D12_VIEWPORT* screenViewport,
        const D3D12_VIEWPORT* graphViewport) override;
};
} // namespace FramePacingUtils
