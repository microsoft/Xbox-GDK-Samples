//--------------------------------------------------------------------------------------
// AnimationPlayer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace glTF
{
    class Asset;
}

class AnimationPlayer
{
public:
    static constexpr uint32_t MaxJoints = 128;

    bool Initialize(const glTF::Asset& asset, uint32_t skinIndex = 0);
    void Update(float elapsedSeconds);
    bool SetAnimation(uint32_t animationIndex, float transitionDuration = 0.0f);
    uint32_t FindAnimation(std::string_view name) const noexcept;

    uint32_t GetAnimationCount() const noexcept
    {
        return static_cast<uint32_t>(m_clips.size());
    }

    uint32_t GetCurrentAnimationIndex() const noexcept
    {
        return m_currentAnimationIndex;
    }

    const std::string& GetAnimationName(uint32_t animationIndex) const;
    float GetAnimationDuration(uint32_t animationIndex) const;

    const std::vector<DirectX::XMFLOAT4X4>& GetJointPalette() const noexcept
    {
        return m_jointPalette;
    }

private:
    enum class Interpolation
    {
        Linear,
        Step,
    };

    enum class Path
    {
        Translation,
        Rotation,
        Scale,
    };

    struct Node
    {
        int32_t parentIndex = -1;
        bool hasMatrix = false;
        DirectX::XMFLOAT3 bindTranslation = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 bindRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 bindScale = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4X4 bindMatrix = {};
        DirectX::XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct Pose
    {
        DirectX::XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct Sampler
    {
        std::vector<float> times;
        std::vector<DirectX::XMFLOAT4> values;
        Interpolation interpolation = Interpolation::Linear;
    };

    struct Channel
    {
        uint32_t samplerIndex = 0;
        uint32_t targetNodeIndex = 0;
        Path path = Path::Translation;
    };

    struct Clip
    {
        std::string name;
        std::vector<Sampler> samplers;
        std::vector<Channel> channels;
        float startTime = 0.0f;
        float endTime = 0.0f;
    };

    DirectX::XMMATRIX EvaluateGlobalTransform(uint32_t nodeIndex);
    DirectX::XMFLOAT4 Sample(const Sampler& sampler, float time, Path path) const;
    void EvaluatePose(const Clip& clip, float time);
    void SampleClipPose(const Clip& clip, float time);
    void BuildJointPalette();

    std::vector<Node> m_nodes;
    std::vector<Clip> m_clips;
    std::vector<uint32_t> m_jointNodeIndices;

    std::vector<DirectX::XMFLOAT4X4> m_inverseBindMatrices;
    std::vector<DirectX::XMFLOAT4X4> m_localTransforms;
    std::vector<DirectX::XMFLOAT4X4> m_globalTransforms;

    std::vector<uint8_t> m_globalTransformValid;
    std::vector<DirectX::XMFLOAT4X4> m_jointPalette;
    std::vector<Pose> m_transitionSourcePose;

    uint32_t m_meshNodeIndex = 0;
    uint32_t m_currentAnimationIndex = 0;
    float m_playbackTime = 0.0f;
    float m_transitionDuration = 0.0f;
    float m_transitionTime = 0.0f;
};
