// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

namespace ATG
{
    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float ConvertDipsToPixels(float dips, float dpi)
    {
        constexpr float dipsPerInch = 96.0f;
        return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
    }

    inline float GetScaleFactorForWindow(const RECT& viewportBounds)
    {
        constexpr float referenceWidth = 1920.f;
        constexpr float referenceHeight = 1080.f;
        constexpr float referenceAspectRatio = referenceWidth / referenceHeight;
        float viewportWidth = float(viewportBounds.right);
        float viewportHeight = float(viewportBounds.bottom);
        float aspectRatio = viewportWidth / viewportHeight;
        float scale = 1.f;

        if (aspectRatio <= referenceAspectRatio)
        {
            scale = viewportWidth / referenceWidth;
        }
        else // aspectRatio > referenceAspectRatio
        {
            scale = viewportHeight / referenceHeight;
        }

        return scale;
    }

    inline DirectX::XMMATRIX GetScaleMatrixForWindow(const RECT& viewportBounds)
    {
        float scale = GetScaleFactorForWindow(viewportBounds);
        return DirectX::XMMatrixScaling(scale, scale, 1.f);
    }

    inline DirectX::XMINT2 ConvertWindowPixelToLocalCoord(const RECT& viewportBounds, const DirectX::XMINT2& pixel)
    {
        float scale = 1.f / GetScaleFactorForWindow(viewportBounds);
        return DirectX::XMINT2(lround(float(pixel.x) * scale), lround(float(pixel.y) * scale));
    }

    inline bool IsPointInsideRectangle(const DirectX::XMINT2& point, const RECT& rectangle)
    {
        return (point.x >= rectangle.left
             && point.x <= rectangle.right
             && point.y >= rectangle.top
             && point.y <= rectangle.bottom);
    }

    inline bool IsPointInsideCircle(const DirectX::XMFLOAT2& point, const DirectX::XMFLOAT2& center, float radius)
    {
        using namespace DirectX;
        XMVECTOR x1 = XMLoadFloat2(&point);
        XMVECTOR x2 = XMLoadFloat2(&center);
        XMVECTOR V = XMVectorSubtract(x2, x1);
        XMVECTOR X = XMVector2LengthSq(V);
        float distanceSq = XMVectorGetX(X);
        float radiusSq = radius * radius;
        return distanceSq <= radiusSq;
    }
}
