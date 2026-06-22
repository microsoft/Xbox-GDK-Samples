//--------------------------------------------------------------------------------------
// Timeline.h
//
// Helper code for rendering statistics as a timeline graph on the screen
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PrimitiveBatch.h"
#include "Utility.h"

namespace
{
    using namespace DirectX::SimpleMath;

    enum TIMELINE
    {
        TIMELINE_PIXEL_COUNT,
        TIMELINE_FRAME_TIME_FLIP,
        TIMELINE_VSYNC_MARGIN,

        TIMELINE_COUNT
    };

    static const wchar_t* c_timelineNames[] =
    {
        L"Pixel count (%)",
        L"Frame time (ms)",
        L"Vsync margin (ms)",
    };
    static_assert(_countof(c_timelineNames) == TIMELINE_COUNT, "Mismatch between enum and reflected names");

    constexpr XMVECTORF32 c_lineColor = {{{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }}};
    constexpr XMVECTORF32 c_labelColor = {{{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }}};

    struct Timeline
    {
        static constexpr uint32_t c_numDataPoints = 256;
        static constexpr float c_lengthInMs = 4000.0f;

        void Update(const FrameStatistics* frameStats)
        {
            m_data.push_back(NewDataPoint(frameStats));
        }

        void Render(
            ID3D12GraphicsCommandList* commandList,
            PrimitiveBatch<VertexPositionColor>* primBatch,
            BasicEffect* lineEffect,
            SpriteBatch* spriteBatch,
            SpriteFont* font,
            const D3D12_VIEWPORT* screenViewport,
            const D3D12_VIEWPORT* timelineViewport);

        float ScaleX(float val)
        {
            return Saturate(((val - 0.0f) / (1.0f - 0.0f) * 2.0f) - 1.0f, -0.999f, 0.999f);
        }

        float ScaleY(float val)
        {
            return Saturate(((val - m_minY) / (m_maxY - m_minY) * 2.0f) - 1.0f, -0.999f, 0.999f);
        }

        const wchar_t* m_name=nullptr;

        float m_minY=0.0f;
        float m_maxY=0.0f;
        float m_tickInterval=0.0f;

        struct DataPoint
        {
            float m_timeInMs;
            float m_value;
        };
        typedef DataPoint(*FnDataPoint)(const FrameStatistics* frameStats);
        FnDataPoint NewDataPoint=nullptr;

        Ring< DataPoint, c_numDataPoints > m_data;
    };


    void Timeline::Render(
        ID3D12GraphicsCommandList* commandList,
        PrimitiveBatch<VertexPositionColor>* primBatch,
        BasicEffect* lineEffect, SpriteBatch* spriteBatch,
        SpriteFont* font,
        const D3D12_VIEWPORT* screenViewport,
        const D3D12_VIEWPORT* timelineViewport)
    {
        wchar_t textBuffer[128] = {};
        _snwprintf_s(textBuffer, _TRUNCATE, L"Timeline: %ls", m_name);
        ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, textBuffer);

        commandList->RSSetViewports(1, screenViewport);

        // Where is the timeline element itself (excluding text), within its viewport
        float timelineStartX = 0.0f;
        float timelineStopX = timelineViewport->Width - 48.0f;
        float timelineStartY = 15.0f;
        float timelineStopY = timelineViewport->Height - 20.0f;    // account for label

        // Text -- positions based on full-screen viewport
        float incX = 10.0f;
        float incY = 10.0f;
        float textX = timelineViewport->TopLeftX;
        float textY = timelineViewport->TopLeftY + timelineStopY;
        XMFLOAT2 textPos(textX, textY);

        spriteBatch->Begin(commandList);

        font->DrawString(spriteBatch, m_name, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

        textPos.x = timelineViewport->TopLeftX + timelineStopX + incX;
        textPos.y = timelineViewport->TopLeftY;
        _snwprintf_s(textBuffer, _TRUNCATE, L"%2.2f", m_maxY);
        font->DrawString(spriteBatch, textBuffer, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

        textPos.y = timelineViewport->TopLeftY + timelineStopY - incY;
        _snwprintf_s(textBuffer, _TRUNCATE, L"%2.2f", m_minY);
        font->DrawString(spriteBatch, textBuffer, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

        spriteBatch->End();

        // Graph
        D3D12_VIEWPORT viewportGraph = 
        {
            timelineViewport->TopLeftX + timelineStartX,
            timelineViewport->TopLeftY + timelineStartY,
            timelineStopX - timelineStartX,
            timelineStopY - timelineStartY,
            0.0f, 
            1.0f, 
        };
        commandList->RSSetViewports(1, &viewportGraph);

        lineEffect->Apply(commandList);
        primBatch->Begin(commandList);

        // Frame
        float leftX = ScaleX( 0.0f );
        float rightX = ScaleX( 1.0f );
        float topY = ScaleY( m_maxY );
        float bottomY = ScaleY( m_minY );
        float zeroY = ScaleY( 0.0f );
        Vector3 topLeft    ( leftX,  topY,    0.0f );
        Vector3 topRight   ( rightX, topY,    0.0f );
        Vector3 bottomLeft ( leftX,  bottomY, 0.0f );
        Vector3 bottomRight( rightX, bottomY, 0.0f );
        Vector3 zeroLeft   ( leftX,  zeroY,   0.0f );
        Vector3 zeroRight  ( rightX, zeroY,   0.0f );
        primBatch->DrawLine(VertexPositionColor(topLeft, c_labelColor), VertexPositionColor(bottomLeft, c_labelColor));
        primBatch->DrawLine(VertexPositionColor(zeroLeft, c_labelColor), VertexPositionColor(zeroRight, c_labelColor));
        primBatch->DrawLine(VertexPositionColor(bottomRight, c_labelColor), VertexPositionColor(topRight, c_labelColor));
        float tickSizeX = 0.02f;
        for( float tick = m_minY; tick <= m_maxY; tick += m_tickInterval )
        {
            float tickY = ScaleY( tick );
            float tickLeftStartX = ScaleX( 0.0f );
            float tickLeftStopX = ScaleX( 0.0f + tickSizeX );
            Vector3 tickLeftStart( tickLeftStartX, tickY, 0.0f );
            Vector3 tickLeftStop ( tickLeftStopX,  tickY, 0.0f );
            primBatch->DrawLine(VertexPositionColor(tickLeftStart, c_labelColor), VertexPositionColor(tickLeftStop, c_labelColor));
            float tickRightStartX = ScaleX( 1.0f );
            float tickRightStopX = ScaleX( 1.0f - tickSizeX );
            Vector3 tickRightStart( tickRightStartX, tickY, 0.0f );
            Vector3 tickRightStop ( tickRightStopX,  tickY, 0.0f );
            primBatch->DrawLine(VertexPositionColor(tickRightStart, c_labelColor), VertexPositionColor(tickRightStop, c_labelColor));
        }

        // Data
        if( !m_data.empty() )
        {
            VertexPositionColor vertices[ c_numDataPoints ] = {};
            VertexPositionColor* vertex = vertices;
            float nowInMs = m_data.back().m_timeInMs;
            for( auto it = m_data.cend(); it != m_data.cbegin(); --it )
            {
                auto itPrev = it - 1;

                float timeBeforeNowInMS = nowInMs - itPrev->m_timeInMs;
                float dataX = ScaleX( ( c_lengthInMs - timeBeforeNowInMS ) / c_lengthInMs );
                float dataY = ScaleY( itPrev->m_value );

                *vertex = VertexPositionColor(Vector3(dataX, dataY, 0.0f), c_lineColor);

                ++vertex;
                if( vertex == vertices + _countof( vertices ) )
                {
                    break;
                }
            }

            primBatch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, vertices, c_numDataPoints);
        }

        primBatch->End();
    };
}
