//--------------------------------------------------------------------------------------
// Graph.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Graph.h"

using namespace DirectX;
using namespace FramePacingUtils;

namespace
{
    using namespace DirectX::SimpleMath;

    const XMVECTORF32 c_lineColor = { {{ 0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f }} };
    const XMVECTORF32 c_labelColor = { {{ 0.5490196078f, 0.7764705882f, 0.7803921568f, 1.0f }} };
    const XMVECTORF32 c_rangeColor = DirectX::Colors::Tan;
};


void LineGraph::Render(
    ID3D12GraphicsCommandList* commandList,
    PrimitiveBatch<VertexPositionColor>* primBatch,
    BasicEffect* basicEffect,
    SpriteBatch* spriteBatch,
    SpriteFont* font,
    const D3D12_VIEWPORT* screenViewport,
    const D3D12_VIEWPORT* graphViewport)
{
    wchar_t textBuffer[128] = {};
    _snwprintf_s(textBuffer, _TRUNCATE, L"Graph: %ls", m_name);
    ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, textBuffer);

    commandList->RSSetViewports(1, screenViewport);

    // Where is the graph element itself (excluding text), within its viewport
    float graphStartX = 15.0f;
    float graphStopX = graphViewport->Width - 70.0f;
    float graphStartY = 15.0f;
    float graphStopY = graphViewport->Height - 25.0f;    // account for label

    // Text -- positions based on full-screen viewport
    float incX = 10.0f;
    float incY = 10.0f;
    float textX = graphViewport->TopLeftX;
    float textY = graphViewport->TopLeftY + graphStopY;
    XMFLOAT2 textPos(textX, textY);

    spriteBatch->Begin(commandList);

    font->DrawString(spriteBatch, m_name, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

    textPos.x = graphViewport->TopLeftX + graphStopX + incX;
    textPos.y = graphViewport->TopLeftY;
    _snwprintf_s(textBuffer, _TRUNCATE, L"%2.2f", m_maxY);
    font->DrawString(spriteBatch, textBuffer, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

    textPos.y = graphViewport->TopLeftY + graphStopY - incY;
    _snwprintf_s(textBuffer, _TRUNCATE, L"%2.2f", m_minY);
    font->DrawString(spriteBatch, textBuffer, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

    spriteBatch->End();

    // Graph
    D3D12_VIEWPORT viewportGraph =
    {
        graphViewport->TopLeftX + graphStartX,
        graphViewport->TopLeftY + graphStartY,
        graphStopX - graphStartX,
        graphStopY - graphStartY,
        0.0f,
        1.0f,
    };
    commandList->RSSetViewports(1, &viewportGraph);

    basicEffect->Apply(commandList);
    primBatch->Begin(commandList);

    // Frame
    float leftX = ScaleX(0.0f);
    float rightX = ScaleX(1.0f);
    float topY = ScaleY(m_maxY);
    float bottomY = ScaleY(m_minY);
    float zeroY = ScaleY(0.0f);
    Vector3 topLeft(leftX, topY, 0.0f);
    Vector3 topRight(rightX, topY, 0.0f);
    Vector3 bottomLeft(leftX, bottomY, 0.0f);
    Vector3 bottomRight(rightX, bottomY, 0.0f);
    Vector3 zeroLeft(leftX, zeroY, 0.0f);
    Vector3 zeroRight(rightX, zeroY, 0.0f);
    primBatch->DrawLine(VertexPositionColor(topLeft, c_labelColor), VertexPositionColor(bottomLeft, c_labelColor));
    primBatch->DrawLine(VertexPositionColor(zeroLeft, c_labelColor), VertexPositionColor(zeroRight, c_labelColor));
    primBatch->DrawLine(VertexPositionColor(bottomRight, c_labelColor), VertexPositionColor(topRight, c_labelColor));
    float tickSizeX = 0.02f;
    for (float tick = m_minY; tick <= m_maxY; tick += m_tickInterval)
    {
        float tickY = ScaleY(tick);
        float tickLeftStartX = ScaleX(0.0f);
        float tickLeftStopX = ScaleX(0.0f) + tickSizeX;
        Vector3 tickLeftStart(tickLeftStartX, tickY, 0.0f);
        Vector3 tickLeftStop(tickLeftStopX, tickY, 0.0f);
        primBatch->DrawLine(VertexPositionColor(tickLeftStart, c_labelColor), VertexPositionColor(tickLeftStop, c_labelColor));
        float tickRightStartX = ScaleX(1.0f);
        float tickRightStopX = ScaleX(1.0f) - tickSizeX;
        Vector3 tickRightStart(tickRightStartX, tickY, 0.0f);
        Vector3 tickRightStop(tickRightStopX, tickY, 0.0f);
        primBatch->DrawLine(VertexPositionColor(tickRightStart, c_labelColor), VertexPositionColor(tickRightStop, c_labelColor));
    }

    // Data
    if (!m_data.empty())
    {
        VertexPositionColor vertices[c_numData] = {};
        VertexPositionColor* vertex = vertices;
        double nowInMs = m_data.back().m_flipTimeInMs;
        for (auto it = m_data.cend(); it != m_data.cbegin(); --it)
        {
            auto itPrev = it - 1;

            float timeBeforeNowInMS = static_cast<float>(nowInMs - itPrev->m_flipTimeInMs);
            float dataX = ScaleX((c_lengthInMs - timeBeforeNowInMS) / c_lengthInMs);
            float dataY = ScaleY(itPrev->m_value);

            *vertex = VertexPositionColor(Vector3(dataX, dataY, 0.0f), c_lineColor);

            ++vertex;
            if (vertex == vertices + _countof(vertices))
            {
                break;
            }
        }

        primBatch->Draw(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, vertices, static_cast<size_t>(vertex - vertices));
    }

    primBatch->End();
}

void IntervalGraph::Render(
    ID3D12GraphicsCommandList* commandList,
    PrimitiveBatch<VertexPositionColor>* primBatch,
    BasicEffect* basicEffect,
    SpriteBatch* spriteBatch,
    SpriteFont* font,
    const D3D12_VIEWPORT* screenViewport,
    const D3D12_VIEWPORT* graphViewport)
{
    wchar_t textBuffer[128] = {};
    _snwprintf_s(textBuffer, _TRUNCATE, L"Graph: %ls", m_name);
    ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, textBuffer);

    commandList->RSSetViewports(1, screenViewport);

    // Where is the graph element itself (excluding text), within its viewport
    float graphStartX = 15.0f;
    float graphStopX = graphViewport->Width - 15.0f;
    float graphStartY = 15.0f;
    float graphStopY = graphViewport->Height - 25.0f;    // account for label

    // Text -- positions based on full-screen viewport
    float textX = graphViewport->TopLeftX;
    float textY = graphViewport->TopLeftY + graphStopY;
    XMFLOAT2 textPos(textX, textY);

    spriteBatch->Begin(commandList);

    font->DrawString(spriteBatch, m_name, textPos, c_labelColor, 0.0f, { 0.0f, 0.0f }, 0.8f);

    spriteBatch->End();

    D3D12_VIEWPORT viewportGraph =
    {
        graphViewport->TopLeftX + graphStartX,
        graphViewport->TopLeftY + graphStartY,
        graphStopX - graphStartX,
        graphStopY - graphStartY,
        0.0f,
        1.0f,
    };
    commandList->RSSetViewports(1, &viewportGraph);

    basicEffect->Apply(commandList);
    primBatch->Begin(commandList);

    // Data
    if (!m_data.empty())
    {
        std::vector<VertexPositionColor> vertices(c_numData * 4);
        VertexPositionColor* vertex = vertices.data();

        // Align the times for all interval graphs by starting at flip time
        double nowInMs = m_data.back().m_flipTimeInMs;   
        for (auto it = m_data.cend()-1; it != m_data.cbegin(); --it)
        {
            // float is good enough for relative times
            float startBeforeNowInMS = static_cast<float>(nowInMs - it->m_startInMs);
            float stopBeforeNowInMS = static_cast<float>(nowInMs - it->m_stopInMs);

            float leftX = ScaleX((c_lengthInMs - startBeforeNowInMS) / c_lengthInMs);
            float rightX = ScaleX((c_lengthInMs - stopBeforeNowInMS) / c_lengthInMs);
            float topY = ScaleY(0.0f);
            float bottomY = ScaleY(1.0f);

            Vector3 topLeft(leftX, topY, 0.0f);
            Vector3 topRight(rightX, topY, 0.0f);
            Vector3 bottomLeft(leftX, bottomY, 0.0f);
            Vector3 bottomRight(rightX, bottomY, 0.0f);
            auto colorIndex = it->m_frameBuffer % _countof(c_intervalColors);
            *vertex++ = VertexPositionColor(topLeft, c_intervalColors[colorIndex]);
            *vertex++ = VertexPositionColor(bottomLeft, c_intervalColors[colorIndex]);
            *vertex++ = VertexPositionColor(bottomRight, c_intervalColors[colorIndex]);
            *vertex++ = VertexPositionColor(topRight, c_intervalColors[colorIndex]);

            if (vertex == vertices.data() + vertices.size())
            {
                break;
            }
        }

        primBatch->Draw(D3D_PRIMITIVE_TOPOLOGY_QUADLIST, vertices.data(), static_cast<size_t>(vertex - vertices.data()));
    }

    // Range finder
    {
        // Frame
        float leftX = ScaleX(0.0f);
        float rightX = ScaleX(1.0f);
        float topY = ScaleY(0.0f);
        float bottomY = ScaleY(1.0f);
        Vector3 topLeft(leftX, topY, 0.0f);
        Vector3 topRight(rightX, topY, 0.0f);
        Vector3 bottomLeft(leftX, bottomY, 0.0f);
        Vector3 bottomRight(rightX, bottomY, 0.0f);
        primBatch->DrawLine(VertexPositionColor(topLeft, c_rangeColor), VertexPositionColor(bottomLeft, c_rangeColor));
        primBatch->DrawLine(VertexPositionColor(bottomLeft, c_rangeColor), VertexPositionColor(bottomRight, c_rangeColor));
        primBatch->DrawLine(VertexPositionColor(bottomRight, c_rangeColor), VertexPositionColor(topRight, c_rangeColor));
        primBatch->DrawLine(VertexPositionColor(topRight, c_rangeColor), VertexPositionColor(topLeft, c_rangeColor));
    }

    primBatch->End();
}
