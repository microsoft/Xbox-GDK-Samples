#pragma once
#include "SimpleMath.h"

#define NAMESPACE_ATG_UITK_UIMATH_BEGIN namespace ATG { namespace UITK { namespace UIMath {
#define NAMESPACE_ATG_UITK_UIMATH_END } } }

NAMESPACE_ATG_UITK_UIMATH_BEGIN

using Rectangle = DirectX::SimpleMath::Rectangle;
using Vector2 = DirectX::SimpleMath::Vector2;
using Offsets = RECT;

Rectangle GetSubRectangle(const Rectangle& rectangle, const Vector2& minUV, const Vector2& maxUV);
Rectangle GetScaledRectangle(const Rectangle& rectangle, float scale);

struct RectPt
{
    long x;
    long y;
};

struct RectangleExt : public Rectangle
{
    const Rectangle& rect_;

    RectangleExt(const Rectangle& rect) : rect_(rect) {}
    RectangleExt(const RectangleExt& ext) : rect_(ext.rect_) {}
    RectangleExt(RectangleExt&& ext) noexcept : rect_(ext.rect_) { }

    RectangleExt() = delete;
    RectangleExt& operator=(const RectangleExt& ext) = delete;
    RectangleExt& operator=(const Rectangle& rect) = delete;

    RectPt GetCenter();
    RectPt GetLeftTop();
    RectPt GetRightTop();
    RectPt GetLeftBottom();
    RectPt GetRightBottom();
    RectPt GetLeftEdgeCenter();
    RectPt GetRightEdgeCenter();
    RectPt GetTopEdgeCenter();
    RectPt GetBottomEdgeCenter();
};

template <typename T>
inline T Clamp(T val, T min, T max)
{
	const T result = (val < min) ? min : val;
	return (result > max) ? max : result;
}

inline float NormalizedClamp(float val)
{
	return Clamp<float>(val, 0.0f, 1.0f);
}

inline double NormalizedClamp(double val)
{
	return Clamp<double>(val, 0.0, 1.0);
}

inline Rectangle GetScaledRectangle(const Rectangle& rectangle, float scale)
{
    return Rectangle(
        static_cast<long>(rectangle.x * scale),
        static_cast<long>(rectangle.y * scale),
        static_cast<long>(rectangle.width * scale),
        static_cast<long>(rectangle.height * scale));
}

inline Rectangle SubtractOffsetsFromRect(const Rectangle& source, const Rectangle& offsets)
{
    return Rectangle(
        source.x + offsets.x,
        source.y + offsets.y,
        source.width - offsets.width,
        source.height - offsets.height
    );
}

inline Rectangle SubtractOffsetsFromRect(const Rectangle& source, const Offsets& offsets)
{
    return Rectangle(
        source.x + offsets.left,
        source.y + offsets.top,
        source.width - (offsets.left + offsets.right),
        source.height - (offsets.top + offsets.bottom)
    );
}

inline Rectangle AddOffsetsFromRect(const Rectangle& source, const Rectangle& offsets)
{
    return Rectangle(
        source.x - offsets.x,
        source.y - offsets.y,
        source.width + offsets.width,
        source.height + offsets.height
    );
}

inline Rectangle AddOffsetsFromRect(const Rectangle& source, const Offsets& offsets)
{
    return Rectangle(
        source.x - offsets.left,
        source.y - offsets.top,
        source.width + (offsets.left + offsets.right),
        source.height + (offsets.top + offsets.bottom)
    );
}

inline float RoundToNextMultiple(float val, float multiple)
{
    return ((val + multiple - 1) / multiple) * multiple;
}

NAMESPACE_ATG_UITK_UIMATH_END

