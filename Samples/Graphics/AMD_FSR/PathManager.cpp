//--------------------------------------------------------------------------------------
// PathManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PathManager.h"

#include "Json.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

using namespace DirectX;

namespace
{
    XMFLOAT3 Add(const XMFLOAT3& left, const XMFLOAT3& right)
    {
        return XMFLOAT3(left.x + right.x, left.y + right.y, left.z + right.z);
    }

    XMFLOAT3 Subtract(const XMFLOAT3& left, const XMFLOAT3& right)
    {
        return XMFLOAT3(left.x - right.x, left.y - right.y, left.z - right.z);
    }

    XMFLOAT3 Scale(const XMFLOAT3& value, float scale)
    {
        return XMFLOAT3(value.x * scale, value.y * scale, value.z * scale);
    }

    float Length(const XMFLOAT3& value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }
}

bool PathManager::Initialize(const wchar_t* path, Settings settings)
{
    if (!path || settings.speed <= 0.0f
        || settings.acceleration <= 0.0f
        || settings.scale <= 0.0f)
    {
        return false;
    }

    std::ifstream stream{ std::filesystem::path(path) };
    if (!stream)
    {
        throw std::runtime_error("Failed to open the character path JSON");
    }

    json document;
    stream >> document;
    if (!document.contains("checkpoints") || !document["checkpoints"].is_array())
    {
        throw std::runtime_error("Character path JSON must contain a checkpoints array");
    }

    m_checkpoints.clear();
    for (const auto& value : document["checkpoints"])
    {
        if (!value.contains("x") || !value.contains("y") || !value.contains("z")
            || !value.contains("animation"))
        {
            throw std::runtime_error("Each character checkpoint requires x, y, z, and animation");
        }

        Checkpoint checkpoint = {};
        checkpoint.position = XMFLOAT3(
            value.at("x").get<float>(),
            value.at("y").get<float>(),
            value.at("z").get<float>());

        const auto& animation = value.at("animation");
        if (animation.is_number_unsigned() || animation.is_number_integer())
        {
            const int64_t index = animation.get<int64_t>();
            if (index < 0 || index > UINT32_MAX)
            {
                throw std::runtime_error("Checkpoint animation index is out of range");
            }
            checkpoint.animation.isIndex = true;
            checkpoint.animation.index = static_cast<uint32_t>(index);
        }
        else if (animation.is_string())
        {
            checkpoint.animation.name = animation.get<std::string>();
            if (checkpoint.animation.name.empty())
            {
                throw std::runtime_error("Checkpoint animation name cannot be empty");
            }
        }
        else
        {
            throw std::runtime_error("Checkpoint animation must be an integer or string");
        }
        m_checkpoints.push_back(std::move(checkpoint));
    }

    if (m_checkpoints.size() < 4)
    {
        throw std::runtime_error("Character path requires at least four checkpoints");
    }

    settings.samplesPerSegment = std::max<uint32_t>(settings.samplesPerSegment, 8);
    m_settings = std::move(settings);
    BuildArcLengthTables();

    m_segmentIndex = 0;
    m_segmentDistance = 0.0f;
    m_currentSpeed = 0.0f;
    m_checkpointTimeRemaining = 0.0f;
    m_needsCheckpointAnimation = true;
    m_movementStarted = false;
    m_position = m_checkpoints.front().position;
    m_facing = EvaluateTangent(0, 0.0f);
    UpdateTransform(m_facing);
    return true;
}

void PathManager::Update(float elapsedSeconds)
{
    float remainingTime = std::max(elapsedSeconds, 0.0f);
    while (remainingTime > 0.0f)
    {
        const float timeStep = std::min(remainingTime, 1.0f / 30.0f);
        remainingTime -= timeStep;

        if (m_needsCheckpointAnimation)
        {
            return;
        }

        if (m_checkpointTimeRemaining > 0.0f)
        {
            m_checkpointTimeRemaining =
                std::max(0.0f, m_checkpointTimeRemaining - timeStep);
            if (m_checkpointTimeRemaining <= 0.0f)
            {
                m_movementStarted = true;
            }
            continue;
        }

        const auto& segment = m_segments[m_segmentIndex];
        const float distanceRemaining = std::max(0.0f, segment.length - m_segmentDistance);
        if (distanceRemaining <= 0.0001f)
        {
            ArriveAtNextWaypoint();
            continue;
        }

        const float stoppingSpeed = std::sqrt(2.0f * m_settings.acceleration * distanceRemaining);
        const float targetSpeed = std::min(m_settings.speed, stoppingSpeed);
        const float previousSpeed = m_currentSpeed;
        if (m_currentSpeed < targetSpeed)
        {
            m_currentSpeed = std::min(
                targetSpeed,
                m_currentSpeed + m_settings.acceleration * timeStep);
        }
        else
        {
            m_currentSpeed = std::max(
                targetSpeed,
                m_currentSpeed - m_settings.acceleration * timeStep);
        }

        const float distanceStep = 0.5f * (previousSpeed + m_currentSpeed) * timeStep;
        if (distanceStep >= distanceRemaining)
        {
            ArriveAtNextWaypoint();
            continue;
        }

        m_segmentDistance += distanceStep;
        const float t = DistanceToParameter(m_segmentIndex, m_segmentDistance);
        m_position = EvaluatePosition(m_segmentIndex, t);
        const XMFLOAT3 tangent = EvaluateTangent(m_segmentIndex, t);
        if (Length(tangent) > 0.0001f)
        {
            m_facing = tangent;
        }
        UpdateTransform(m_facing);
    }
}

void PathManager::SetCheckpointAnimationDuration(float duration)
{
    if (!m_needsCheckpointAnimation)
    {
        throw std::logic_error("Checkpoint animation duration was already assigned");
    }

    m_checkpointTimeRemaining = std::max(duration, 0.0f);
    m_needsCheckpointAnimation = false;
}

const PathManager::AnimationSelector& PathManager::GetCurrentCheckpointAnimation() const
{
    return m_checkpoints[m_segmentIndex].animation;
}

XMFLOAT3 PathManager::GetWaypoint(int32_t index) const
{
    const int32_t count = static_cast<int32_t>(m_checkpoints.size());
    index %= count;
    if (index < 0)
    {
        index += count;
    }
    return m_checkpoints[static_cast<size_t>(index)].position;
}

XMFLOAT3 PathManager::EvaluatePosition(uint32_t segmentIndex, float t) const
{
    const XMFLOAT3 p0 = GetWaypoint(static_cast<int32_t>(segmentIndex) - 1);
    const XMFLOAT3 p1 = GetWaypoint(static_cast<int32_t>(segmentIndex));
    const XMFLOAT3 p2 = GetWaypoint(static_cast<int32_t>(segmentIndex) + 1);
    const XMFLOAT3 p3 = GetWaypoint(static_cast<int32_t>(segmentIndex) + 2);
    const float t2 = t * t;
    const float t3 = t2 * t;

    return Scale(
        Add(
            Add(
                Scale(p1, 2.0f),
                Scale(Subtract(p2, p0), t)),
            Add(
                Scale(
                    Add(
                        Add(Scale(p0, 2.0f), Scale(p1, -5.0f)),
                        Add(Scale(p2, 4.0f), Scale(p3, -1.0f))),
                    t2),
                Scale(
                    Add(
                        Add(Scale(p0, -1.0f), Scale(p1, 3.0f)),
                        Add(Scale(p2, -3.0f), p3)),
                    t3))),
        0.5f);
}

XMFLOAT3 PathManager::EvaluateTangent(uint32_t segmentIndex, float t) const
{
    const XMFLOAT3 p0 = GetWaypoint(static_cast<int32_t>(segmentIndex) - 1);
    const XMFLOAT3 p1 = GetWaypoint(static_cast<int32_t>(segmentIndex));
    const XMFLOAT3 p2 = GetWaypoint(static_cast<int32_t>(segmentIndex) + 1);
    const XMFLOAT3 p3 = GetWaypoint(static_cast<int32_t>(segmentIndex) + 2);

    return Scale(
        Add(
            Subtract(p2, p0),
            Add(
                Scale(
                    Add(
                        Add(Scale(p0, 2.0f), Scale(p1, -5.0f)),
                        Add(Scale(p2, 4.0f), Scale(p3, -1.0f))),
                    2.0f * t),
                Scale(
                    Add(
                        Add(Scale(p0, -1.0f), Scale(p1, 3.0f)),
                        Add(Scale(p2, -3.0f), p3)),
                    3.0f * t * t))),
        0.5f);
}

float PathManager::DistanceToParameter(uint32_t segmentIndex, float distance) const
{
    const auto& cumulativeLengths = m_segments[segmentIndex].cumulativeLengths;
    const auto upper = std::lower_bound(
        cumulativeLengths.begin(),
        cumulativeLengths.end(),
        distance);
    if (upper == cumulativeLengths.begin())
    {
        return 0.0f;
    }
    if (upper == cumulativeLengths.end())
    {
        return 1.0f;
    }

    const size_t upperIndex = static_cast<size_t>(upper - cumulativeLengths.begin());
    const size_t lowerIndex = upperIndex - 1;
    const float intervalLength = cumulativeLengths[upperIndex] - cumulativeLengths[lowerIndex];
    const float fraction = intervalLength > 0.0f
        ? (distance - cumulativeLengths[lowerIndex]) / intervalLength
        : 0.0f;
    return (static_cast<float>(lowerIndex) + fraction)
        / static_cast<float>(m_settings.samplesPerSegment);
}

void PathManager::BuildArcLengthTables()
{
    m_segments.clear();
    m_segments.resize(m_checkpoints.size());
    for (uint32_t segmentIndex = 0; segmentIndex < m_segments.size(); ++segmentIndex)
    {
        auto& segment = m_segments[segmentIndex];
        segment.cumulativeLengths.resize(
            static_cast<size_t>(m_settings.samplesPerSegment) + 1);
        segment.cumulativeLengths[0] = 0.0f;

        XMFLOAT3 previous = EvaluatePosition(segmentIndex, 0.0f);
        for (uint32_t sampleIndex = 1; sampleIndex <= m_settings.samplesPerSegment; ++sampleIndex)
        {
            const float t = static_cast<float>(sampleIndex)
                / static_cast<float>(m_settings.samplesPerSegment);
            const XMFLOAT3 current = EvaluatePosition(segmentIndex, t);
            segment.cumulativeLengths[sampleIndex] =
                segment.cumulativeLengths[sampleIndex - 1] + Length(Subtract(current, previous));
            previous = current;
        }
        segment.length = segment.cumulativeLengths.back();
    }
}

void PathManager::UpdateTransform(const XMFLOAT3& tangent)
{
    const float yaw = std::atan2(tangent.x, tangent.z) + m_settings.headingOffset;
    XMStoreFloat4x4(
        &m_worldTransform,
        XMMatrixScaling(m_settings.scale, m_settings.scale, m_settings.scale)
        * XMMatrixRotationY(yaw)
        * XMMatrixTranslation(m_position.x, m_position.y, m_position.z));
}

void PathManager::ArriveAtNextWaypoint()
{
    m_segmentIndex = (m_segmentIndex + 1) % static_cast<uint32_t>(m_checkpoints.size());
    m_segmentDistance = 0.0f;
    m_currentSpeed = 0.0f;
    m_checkpointTimeRemaining = 0.0f;
    m_needsCheckpointAnimation = true;
    m_position = m_checkpoints[m_segmentIndex].position;
    UpdateTransform(m_facing);
}
