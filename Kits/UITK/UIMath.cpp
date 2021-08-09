#include "pch.h"
#include "UIMath.h"

using namespace ATG::UITK;
using Rectangle = DirectX::SimpleMath::Rectangle;


NAMESPACE_ATG_UITK_UIMATH_BEGIN

Rectangle GetSubRectangle(
    const Rectangle& rectangle,
    const Vector2& minUV, const Vector2& maxUV)
{
    if (minUV == Vector2::Zero && maxUV == Vector2::One) { return rectangle; }
    PIXSetMarker(PIX_COLOR_DEFAULT, L"UIMath_GetSubRectangle_Computing");
    Vector2 clampedMinUV;
    minUV.Clamp(Vector2::Zero, Vector2::One, clampedMinUV);

    Vector2 clampedMaxUV;
    maxUV.Clamp(Vector2::Zero, Vector2::One, clampedMaxUV);

    long l = long(float(rectangle.width) * minUV.x);
    long r = long(float(rectangle.width) * maxUV.x);
    long t = long(float(rectangle.height) * minUV.y);
    long b = long(float(rectangle.height) * maxUV.y);

    return Rectangle(rectangle.x + l, rectangle.y + t, r - l, b - t);
}

RectPt RectangleExt::GetCenter()
{
    return RectPt{
        rect_.x + (rect_.width / 2),
        rect_.y + (rect_.height / 2)
    };
}

RectPt RectangleExt::GetLeftTop()
{
    return RectPt{ rect_.x, rect_.y };
}

RectPt RectangleExt::GetRightTop()
{
    return RectPt{ rect_.x + rect_.width, rect_.y };
}

RectPt RectangleExt::GetLeftBottom()
{
    return RectPt{ rect_.x, rect_.y + rect_.height };
}

RectPt RectangleExt::GetRightBottom()
{
    return RectPt{ rect_.x + rect_.width, rect_.y + rect_.height };
}

RectPt RectangleExt::GetLeftEdgeCenter()
{
    return RectPt{
        rect_.x,
        rect_.y + (rect_.height / 2)
    };
}

RectPt RectangleExt::GetRightEdgeCenter()
{
    return RectPt{
        rect_.x + rect_.width,
        rect_.y + (rect_.height / 2)
    };
}

RectPt RectangleExt::GetTopEdgeCenter()
{
    return RectPt{
        rect_.x + (rect_.width / 2),
        rect_.y
    };
}

RectPt RectangleExt::GetBottomEdgeCenter()
{
    return RectPt{
        rect_.x + (rect_.width / 2),
        rect_.y + rect_.height
    };
}

NAMESPACE_ATG_UITK_UIMATH_END
