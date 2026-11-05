//--------------------------------------------------------------------------------------
// PathManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <vector>

class PathManager
{
public:
    struct AnimationSelector
    {
        bool isIndex = false;
        uint32_t index = 0;
        std::string name;
    };

    struct Settings
    {
        float scale = 1.0f;
        float speed = 1.0f;
        float acceleration = 1.0f;
        float headingOffset = 0.0f;
        uint32_t samplesPerSegment = 64;
    };

    bool Initialize(const wchar_t* path, Settings settings);
    void Update(float elapsedSeconds);
    void SetCheckpointAnimationDuration(float duration);

    bool NeedsCheckpointAnimation() const noexcept
    {
        return m_needsCheckpointAnimation;
    }

    bool ConsumeMovementStarted() noexcept
    {
        const bool started = m_movementStarted;
        m_movementStarted = false;
        return started;
    }

    bool IsMoving() const noexcept
    {
        return !m_needsCheckpointAnimation && m_checkpointTimeRemaining <= 0.0f;
    }

    const AnimationSelector& GetCurrentCheckpointAnimation() const;

    DirectX::XMMATRIX GetWorldTransform() const noexcept
    {
        return DirectX::XMLoadFloat4x4(&m_worldTransform);
    }

    DirectX::XMFLOAT3 GetPosition() const noexcept
    {
        return m_position;
    }

    float GetCurrentSpeed() const noexcept
    {
        return m_currentSpeed;
    }

private:
    struct Checkpoint
    {
        DirectX::XMFLOAT3 position = {};
        AnimationSelector animation;
    };

    struct Segment
    {
        std::vector<float> cumulativeLengths;
        float length = 0.0f;
    };

    DirectX::XMFLOAT3 GetWaypoint(int32_t index) const;
    DirectX::XMFLOAT3 EvaluatePosition(uint32_t segmentIndex, float t) const;
    DirectX::XMFLOAT3 EvaluateTangent(uint32_t segmentIndex, float t) const;
    float DistanceToParameter(uint32_t segmentIndex, float distance) const;
    void BuildArcLengthTables();
    void UpdateTransform(const DirectX::XMFLOAT3& tangent);
    void ArriveAtNextWaypoint();

    Settings m_settings;
    std::vector<Checkpoint> m_checkpoints;
    std::vector<Segment> m_segments;
    DirectX::XMFLOAT4X4 m_worldTransform = {};
    DirectX::XMFLOAT3 m_position = {};
    DirectX::XMFLOAT3 m_facing = { 0.0f, 0.0f, 1.0f };
    uint32_t m_segmentIndex = 0;
    float m_segmentDistance = 0.0f;
    float m_currentSpeed = 0.0f;
    float m_checkpointTimeRemaining = 0.0f;
    bool m_needsCheckpointAnimation = true;
    bool m_movementStarted = false;
};
