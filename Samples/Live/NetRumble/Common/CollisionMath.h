//--------------------------------------------------------------------------------------
// CollisionMath.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct CircleLineCollisionResult
{
	bool Collision;
	DirectX::SimpleMath::Vector2 Point;
	DirectX::SimpleMath::Vector2 Normal;
	float Distance;
};

namespace CollisionMath
{
	using namespace DirectX;

	/// <summary>
	/// Determines the point of intersection between two line segments,
	/// as defined by four points.
	/// </summary>
	/// <param name="a">The first point on the first line segment.</param>
	/// <param name="b">The second point on the first line segment.</param>
	/// <param name="c">The first point on the second line segment.</param>
	/// <param name="d">The second point on the second line segment.</param>
	/// <param name="point">The output value with the interesection, if any.</param>
	/// <remarks>The output parameter "point" is only valid
	/// when the return value is true.</remarks>
	/// <returns>True if intersecting, false otherwise.</returns>
	bool LineLineIntersect(const SimpleMath::Vector2 &a, const SimpleMath::Vector2 &b, const SimpleMath::Vector2 &c, const SimpleMath::Vector2 &d, SimpleMath::Vector2 &point)
	{
		point = SimpleMath::Vector2::Zero;

		double r, s;
		double denominator = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);

		// If the denominator in above is zero, AB & CD are colinear
		if (denominator == 0)
		{
			return false;
		}

		double numeratorR = (a.y - c.y) * (d.x - c.x) - (a.x - c.x) * (d.y - c.y);
		r = numeratorR / denominator;

		double numeratorS = (a.y - c.y) * (b.x - a.x) - (a.x - c.x) * (b.y - a.y);
		s = numeratorS / denominator;

		// non-intersecting
		if (r < 0 || r > 1 || s < 0 || s > 1)
		{
			return false;
		}

		// find intersection point
		point = a + (static_cast<float>(r) * (b - a));

		return true;
	}

	/// <summary>
	/// Determine if two circles intersect or contain each other.
	/// </summary>
	/// <param name="center1">The center of the first circle.</param>
	/// <param name="radius1">The radius of the first circle.</param>
	/// <param name="center2">The center of the second circle.</param>
	/// <param name="radius2">The radius of the second circle.</param>
	/// <returns>True if the circles intersect or contain one another.</returns>
	bool CircleCircleIntersect(SimpleMath::Vector2 center1, float radius1, SimpleMath::Vector2 center2, float radius2)
	{
		SimpleMath::Vector2 line = center2 - center1;
		// we use LengthSquared to avoid a costly square-root call
		float lengthSquared = line.LengthSquared();
		return (lengthSquared <= (radius1 + radius2) * (radius1 + radius2));
	}

	/// <summary>
	/// Determines if a circle and rectangle intersect, and if so, how they do.
	/// </summary>
	/// <param name="center">The center of the circle.</param>
	/// <param name="radius">The radius of the circle.</param>
	/// <param name="rectangle">The rectangle.</param>
	/// <param name="result">The result data for the collision.</param>
	/// <returns>True if a collision occurs, provided for convenience.</returns>
	bool CircleRectangleCollide(const SimpleMath::Vector2 &center, float radius, RECT rectangle, CircleLineCollisionResult& result)
	{
		auto point = center;
		if (point.x < rectangle.left) point.x = static_cast<float>(rectangle.left);
		if (point.x > rectangle.right) point.x = static_cast<float>(rectangle.right);

		if (point.y < rectangle.top) point.y = static_cast<float>(rectangle.top);
		if (point.y > rectangle.bottom) point.y = static_cast<float>(rectangle.bottom);

		SimpleMath::Vector2 direction = center - point;
		float distance = direction.Length();

		if (distance < radius)
		{
			result.Collision = true;
			result.Distance = radius - distance;
			result.Normal = XMVectorScale(direction, 1.0f / distance);
			result.Point = point;
		}
		else
		{
			result.Collision = false;
		}

		return result.Collision;
	}

	/// <summary>
	/// Determines if a circle and line segment intersect, and if so, how they do.
	/// </summary>
	/// <param name="center">The center of the circle.</param>
	/// <param name="radius">The radius of the circle.</param>
	/// <param name="lineStart">The first point on the line segment.</param>
	/// <param name="lineEnd">The second point on the line segment.</param>
	/// <param name="result">The result data for the collision.</param>
	/// <returns>True if a collision occurs, provided for convenience.</returns>
	bool CircleLineCollide(const SimpleMath::Vector2 &center, float radius, const SimpleMath::Vector2 &lineStart, const SimpleMath::Vector2 &lineEnd, CircleLineCollisionResult& result)
	{
		SimpleMath::Vector2 AC = center - lineStart;
		SimpleMath::Vector2 AB = lineEnd - lineStart;
		float ab2 = AB.LengthSquared();
		if (ab2 <= 0.0f)
		{
			return false;
		}
		float acab = AC.Dot(AB);
		float t = acab / ab2;

		if (t < 0.0f)
			t = 0.0f;
		else if (t > 1.0f)
			t = 1.0f;

		result.Point = lineStart + t * AB;
		result.Normal = center - result.Point;

		float h2 = result.Normal.LengthSquared();
		float r2 = radius * radius;

		if ((h2 > 0) && (h2 <= r2))
		{
			h2 = sqrt(h2);
			result.Normal /= h2;
			result.Distance = radius - (center - result.Point).Length();
			result.Collision = true;
		}
		else
		{
			result.Collision = false;
		}

		return result.Collision;
	}
}