//--------------------------------------------------------------------------------------
// Utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define BACKBUFFER_COUNT        3U

// Handles for textures (color pass) and fonts
enum SRV_PILE_HANDLES
{
    SRV_HANDLE_HUD_FONT = 0,
    SRV_HANDLE_CONTROLLER_FONT,
    SRV_HANDLE_FXAA_IN_TEXTURE,
    UAV_HANDLE_FXAA_OUT_TEXTURE_RT0,
    UAV_HANDLE_FXAA_OUT_TEXTURE_RT1,
    UAV_HANDLE_FXAA_OUT_TEXTURE_RT2,
    SRV_HANDLE_COUNT
};
static_assert(UAV_HANDLE_FXAA_OUT_TEXTURE_RT2 - UAV_HANDLE_FXAA_OUT_TEXTURE_RT0 + 1U == BACKBUFFER_COUNT && L"Number of targets does not match with backbuffer count.");

// Pix colors
const uint32_t PixColorPerFrame[3] =
{
    PIX_COLOR(255, 255, 0),
    PIX_COLOR(255, 0, 255),
    PIX_COLOR(0, 255, 255),
};

enum GPU_TIMER_PASSES
{
    GPU_TIMER_GEOMETRY_PASSES = 0,
    GPU_TIMER_POST_PROCESS_PASSES,
    GPU_TIMER_PASSES_COUNT
};

// Per Scene Constants
struct SceneConstants
{
    DirectX::SimpleMath::Matrix m_mvp;
};
static_assert(sizeof(SceneConstants) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 && L"Misaligned constant buffer!");

struct FXAAConstantBuffer
{
    DirectX::SimpleMath::Vector2    fxaaPixelSize;
    float                           width;
    float                           height;
};
static_assert(sizeof(FXAAConstantBuffer) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 && L"Misaligned constant buffer!");

// Frame statistics tracked to monitor frame rate, latency, etc.
struct FrameStatistics
{
    D3D12XBOX_FRAME_EVENT_STATISTICS origin;
    D3D12XBOX_RENDER_STATISTICS render;
    D3D12XBOX_PRESENT_STATISTICS present;
};

class MovingAvg
{
public:
    MovingAvg() : m_window{}, m_maHead(0U)
    {}

    double GetWindowsAvg()
    {
        double sum = 0.0;
        size_t count = std::size(m_window);
        for (size_t i = 0; i < count; ++i)
        {
            sum += m_window[i];
        }
        return sum / count;
    }

    void AddValue(double value)
    {
        m_window[m_maHead] = value;
        m_maHead = (m_maHead + 1) % std::size(m_window);
    }

    void clear()
    {
        ZeroMemory(m_window, std::size(m_window));
        m_maHead = 0U;
    }

private:
    double m_window[30];
    uint32_t m_maHead;
};
