//--------------------------------------------------------------------------------------
// AnimationPlayer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AnimationPlayer.h"

#include "gltf/gltf.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

using namespace DirectX;

namespace
{
    size_t ComponentCount(uint16_t accessorType)
    {
        switch (accessorType)
        {
        case glTF::Accessor::kScalar: return 1;
        case glTF::Accessor::kVec2: return 2;
        case glTF::Accessor::kVec3: return 3;
        case glTF::Accessor::kVec4: return 4;
        case glTF::Accessor::kMat4: return 16;
        default:
            throw std::exception("Unsupported animation accessor type");
        }
    }

    const float* AccessorElement(const glTF::Accessor& accessor, uint32_t index)
    {
        if (accessor.componentType != glTF::Accessor::kFloat)
        {
            throw std::exception("Animation accessors must contain floats");
        }

        const size_t packedStride = ComponentCount(accessor.type) * sizeof(float);
        const size_t stride = accessor.stride ? accessor.stride : packedStride;
        return reinterpret_cast<const float*>(accessor.dataPtr + static_cast<size_t>(index) * stride);
    }

    uint32_t NodeIndex(const glTF::Asset& asset, const glTF::Node* node)
    {
        const auto index = node - asset.m_nodes.data();
        if (index < 0 || static_cast<size_t>(index) >= asset.m_nodes.size())
        {
            throw std::exception("Animation node does not belong to the glTF asset");
        }
        return static_cast<uint32_t>(index);
    }
}

bool AnimationPlayer::Initialize(const glTF::Asset& asset, uint32_t skinIndex)
{
    // Invalid params passed for this gltf asset
    if (asset.m_animations.empty() || skinIndex >= asset.m_skins.size())
    {
        return false;
    }

    const auto& skin = asset.m_skins[skinIndex];

    // Fail if no samplers, channels, or joints have invalid sizes
    if (skin.joints.empty() || skin.joints.size() > MaxJoints)
    {
        return false;
    }

    // m_nodes are obtained from the nodes list in the GLTF asset object
    // This is copying the entire list of Nodes, not just the ones used in the animation
    m_nodes.clear();
    m_nodes.resize(asset.m_nodes.size());
    for (size_t nodeIndex = 0; nodeIndex < asset.m_nodes.size(); ++nodeIndex)
    {
        const auto& source = asset.m_nodes[nodeIndex];
        auto& destination = m_nodes[nodeIndex];
        destination.hasMatrix = source.properties.hasMatrix;

        if (destination.hasMatrix)
        {
            std::memcpy(&destination.bindMatrix, source.matrix, sizeof(destination.bindMatrix));
        }
        else
        {
            const void* srcTr = reinterpret_cast<const void*>(&source.srt.translation[0]);
            memcpy(reinterpret_cast<void*>(&destination.bindTranslation), srcTr, sizeof(DirectX::XMFLOAT3));

            const void* srcRot = reinterpret_cast<const void*>(&source.srt.rotation[0]);
            memcpy(reinterpret_cast<void*>(&destination.bindRotation), srcRot, sizeof(DirectX::XMFLOAT4));

            const void* srcSc = reinterpret_cast<const void*>(&source.srt.scale[0]);
            memcpy(reinterpret_cast<void*>(&destination.bindScale), srcSc, sizeof(DirectX::XMFLOAT3));

        }

        destination.translation = destination.bindTranslation;
        destination.rotation = destination.bindRotation;
        destination.scale = destination.bindScale;

        for (const auto* child : source.children)
        {
            m_nodes[NodeIndex(asset, child)].parentIndex = static_cast<int32_t>(nodeIndex);
        }
    }

    // Not entirely clear what this is doing and what meshIdx is used for
    m_meshNodeIndex = UINT32_MAX;
    for (size_t nodeIndex = 0; nodeIndex < asset.m_nodes.size(); ++nodeIndex)
    {
        const int32_t meshIndex = asset.m_nodes[nodeIndex].meshIdx;
        if (meshIndex >= 0
            && static_cast<size_t>(meshIndex) < asset.m_meshes.size()
            && asset.m_meshes[static_cast<size_t>(meshIndex)].skin == static_cast<int32_t>(skinIndex))
        {
            m_meshNodeIndex = static_cast<uint32_t>(nodeIndex);
            break;
        }
    }

    if (m_meshNodeIndex == UINT32_MAX)
    {
        return false;
    }

    m_jointNodeIndices.clear();
    m_jointNodeIndices.reserve(skin.joints.size());
    for (const auto* joint : skin.joints)
    {
        m_jointNodeIndices.push_back(NodeIndex(asset, joint));
    }

    m_inverseBindMatrices.resize(skin.joints.size());
    if (skin.inverseBindMatrices)
    {
        if (skin.inverseBindMatrices->type != glTF::Accessor::kMat4 ||
            skin.inverseBindMatrices->count != skin.joints.size())
        {
            throw std::exception("Skin inverse-bind matrices do not match the joint count");
        }

        for (uint32_t jointIndex = 0; jointIndex < skin.inverseBindMatrices->count; ++jointIndex)
        {
            std::memcpy(&m_inverseBindMatrices[jointIndex],
                AccessorElement(*skin.inverseBindMatrices, jointIndex),
                sizeof(XMFLOAT4X4));
        }
    }
    else
    {
        for (auto& inverseBind : m_inverseBindMatrices)
        {
            XMStoreFloat4x4(&inverseBind, XMMatrixIdentity());
        }
    }

    m_clips.clear();
    m_clips.reserve(asset.m_animations.size());
    for (const auto& animation : asset.m_animations)
    {
        if (animation.m_samplers.empty() || animation.m_channels.empty())
        {
            throw std::exception("Animation contains no samplers or channels");
        }

        Clip clip = {};
        clip.name = animation.m_name;
        clip.samplers.resize(animation.m_samplers.size());
        clip.startTime = std::numeric_limits<float>::max();
        clip.endTime = std::numeric_limits<float>::lowest();
        for (size_t samplerIndex = 0; samplerIndex < animation.m_samplers.size(); ++samplerIndex)
        {
            const auto& source = animation.m_samplers[samplerIndex];
            auto& destination = clip.samplers[samplerIndex];

            if (!source.m_input || !source.m_output || source.m_input->type != glTF::Accessor::kScalar)
            {
                throw std::exception("Invalid animation sampler");
            }

            if (source.m_interpolation != glTF::AnimSampler::kLinear && source.m_interpolation != glTF::AnimSampler::kStep)
            {
                throw std::exception("This animation path supports LINEAR and STEP interpolation");
            }

            destination.interpolation = source.m_interpolation == glTF::AnimSampler::kStep ? Interpolation::Step : Interpolation::Linear;
            destination.times.resize(source.m_input->count);
            for (uint32_t keyIndex = 0; keyIndex < source.m_input->count; ++keyIndex)
            {
                destination.times[keyIndex] = AccessorElement(*source.m_input, keyIndex)[0];
            }

            const size_t outputComponents = ComponentCount(source.m_output->type);
            if (outputComponents < 3 || outputComponents > 4  || source.m_output->count != source.m_input->count)
            {
                throw std::exception("Unexpected animation sampler output");
            }

            destination.values.resize(source.m_output->count);
            for (uint32_t keyIndex = 0; keyIndex < source.m_output->count; ++keyIndex)
            {
                const float* value = AccessorElement(*source.m_output, keyIndex);
                destination.values[keyIndex] = XMFLOAT4(
                    value[0],
                    value[1],
                    value[2],
                    outputComponents == 4 ? value[3] : 0.0f);
            }

            if (!destination.times.empty())
            {
                clip.startTime = std::min(clip.startTime, destination.times.front());
                clip.endTime = std::max(clip.endTime, destination.times.back());
            }
        }

        clip.channels.reserve(animation.m_channels.size());
        for (const auto& source : animation.m_channels)
        {
            if (!source.m_sampler || !source.m_target || source.m_path == glTF::AnimChannel::kWeights)
            {
                continue;
            }

            const auto samplerIndex = source.m_sampler - animation.m_samplers.data();
            if (samplerIndex < 0 || static_cast<size_t>(samplerIndex) >= animation.m_samplers.size())
            {
                throw std::exception("Animation channel references an invalid sampler");
            }

            Channel channel = {};
            channel.samplerIndex = static_cast<uint32_t>(samplerIndex);
            channel.targetNodeIndex = NodeIndex(asset, source.m_target);
            if (m_nodes[channel.targetNodeIndex].hasMatrix)
            {
                throw std::exception("Animation channels cannot target matrix-based nodes");
            }

            switch (source.m_path)
            {
            case glTF::AnimChannel::kTranslation: channel.path = Path::Translation; break;
            case glTF::AnimChannel::kRotation: channel.path = Path::Rotation; break;
            case glTF::AnimChannel::kScale: channel.path = Path::Scale; break;
            case glTF::AnimChannel::kWeights: continue;
            default: continue;
            }
            clip.channels.push_back(channel);
        }

        if (clip.startTime > clip.endTime)
        {
            throw std::exception("Animation has no keyframe times");
        }
        m_clips.push_back(std::move(clip));
    }

    m_localTransforms.resize(m_nodes.size());
    m_globalTransforms.resize(m_nodes.size());
    m_globalTransformValid.resize(m_nodes.size());
    m_jointPalette.resize(m_jointNodeIndices.size());
    m_transitionSourcePose.clear();
    m_currentAnimationIndex = 0;
    m_playbackTime = 0.0f;
    m_transitionDuration = 0.0f;
    m_transitionTime = 0.0f;
    EvaluatePose(m_clips.front(), m_clips.front().startTime);
    return true;
}

void AnimationPlayer::Update(float elapsedSeconds)
{
    if (m_jointPalette.empty() || m_clips.empty())
    {
        return;
    }

    const auto& clip = m_clips[m_currentAnimationIndex];
    m_playbackTime += elapsedSeconds;
    const float duration = clip.endTime - clip.startTime;
    const float sampleTime = duration > 0.0f
        ? clip.startTime + std::fmod(m_playbackTime, duration)
        : clip.startTime;
    SampleClipPose(clip, sampleTime);

    if (m_transitionDuration > 0.0f)
    {
        m_transitionTime = std::min(
            m_transitionTime + std::max(elapsedSeconds, 0.0f),
            m_transitionDuration);
        const float transition = m_transitionTime / m_transitionDuration;
        for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex)
        {
            auto& node = m_nodes[nodeIndex];
            if (node.hasMatrix)
            {
                continue;
            }

            const auto& source = m_transitionSourcePose[nodeIndex];
            XMVECTOR target = XMLoadFloat3(&node.translation);
            XMStoreFloat3(
                &node.translation,
                XMVectorLerp(XMLoadFloat3(&source.translation), target, transition));

            target = XMLoadFloat4(&node.rotation);
            XMStoreFloat4(
                &node.rotation,
                XMQuaternionNormalize(XMQuaternionSlerp(
                    XMQuaternionNormalize(XMLoadFloat4(&source.rotation)),
                    XMQuaternionNormalize(target),
                    transition)));

            target = XMLoadFloat3(&node.scale);
            XMStoreFloat3(
                &node.scale,
                XMVectorLerp(XMLoadFloat3(&source.scale), target, transition));
        }

        if (m_transitionTime >= m_transitionDuration)
        {
            m_transitionSourcePose.clear();
            m_transitionDuration = 0.0f;
            m_transitionTime = 0.0f;
        }
    }

    BuildJointPalette();
}

bool AnimationPlayer::SetAnimation(uint32_t animationIndex, float transitionDuration)
{
    if (animationIndex >= m_clips.size())
    {
        return false;
    }

    transitionDuration = std::max(transitionDuration, 0.0f);
    if (transitionDuration > 0.0f && !m_nodes.empty())
    {
        m_transitionSourcePose.resize(m_nodes.size());
        for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex)
        {
            const auto& node = m_nodes[nodeIndex];
            auto& source = m_transitionSourcePose[nodeIndex];
            source.translation = node.translation;
            source.rotation = node.rotation;
            source.scale = node.scale;
        }
        m_transitionDuration = transitionDuration;
        m_transitionTime = 0.0f;
    }
    else
    {
        m_transitionSourcePose.clear();
        m_transitionDuration = 0.0f;
        m_transitionTime = 0.0f;
    }

    m_currentAnimationIndex = animationIndex;
    m_playbackTime = 0.0f;
    if (m_transitionDuration <= 0.0f)
    {
        EvaluatePose(m_clips[animationIndex], m_clips[animationIndex].startTime);
    }
    return true;
}

uint32_t AnimationPlayer::FindAnimation(std::string_view name) const noexcept
{
    for (size_t animationIndex = 0; animationIndex < m_clips.size(); ++animationIndex)
    {
        if (m_clips[animationIndex].name == name)
        {
            return static_cast<uint32_t>(animationIndex);
        }
    }
    return UINT32_MAX;
}

const std::string& AnimationPlayer::GetAnimationName(uint32_t animationIndex) const
{
    if (animationIndex >= m_clips.size())
    {
        throw std::out_of_range("Animation index is out of range");
    }
    return m_clips[animationIndex].name;
}

float AnimationPlayer::GetAnimationDuration(uint32_t animationIndex) const
{
    if (animationIndex >= m_clips.size())
    {
        throw std::out_of_range("Animation index is out of range");
    }
    return m_clips[animationIndex].endTime - m_clips[animationIndex].startTime;
}

XMFLOAT4 AnimationPlayer::Sample(const Sampler& sampler, float time, Path path) const
{
    if (sampler.times.empty())
    {
        return path == Path::Rotation
            ? XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
            : XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const auto upper = std::upper_bound(sampler.times.begin(), sampler.times.end(), time);
    if (upper == sampler.times.begin())
    {
        return sampler.values.front();
    }
    if (upper == sampler.times.end())
    {
        return sampler.values.back();
    }

    const size_t upperIndex = static_cast<size_t>(upper - sampler.times.begin());
    const size_t lowerIndex = upperIndex - 1;
    if (sampler.interpolation == Interpolation::Step)
    {
        return sampler.values[lowerIndex];
    }

    const float keyDuration = sampler.times[upperIndex] - sampler.times[lowerIndex];
    const float interpolation = keyDuration > 0.0f
        ? (time - sampler.times[lowerIndex]) / keyDuration
        : 0.0f;

    const XMVECTOR lowerValue = XMLoadFloat4(&sampler.values[lowerIndex]);
    const XMVECTOR upperValue = XMLoadFloat4(&sampler.values[upperIndex]);
    XMFLOAT4 result = {};
    if (path == Path::Rotation)
    {
        XMStoreFloat4(
            &result,
            XMQuaternionNormalize(XMQuaternionSlerp(
                XMQuaternionNormalize(lowerValue),
                XMQuaternionNormalize(upperValue),
                interpolation)));
    }
    else
    {
        XMStoreFloat4(&result, XMVectorLerp(lowerValue, upperValue, interpolation));
    }
    return result;
}

void AnimationPlayer::EvaluatePose(const Clip& clip, float time)
{
    SampleClipPose(clip, time);
    BuildJointPalette();
}

void AnimationPlayer::SampleClipPose(const Clip& clip, float time)
{
    for (auto& node : m_nodes)
    {
        node.translation = node.bindTranslation;
        node.rotation = node.bindRotation;
        node.scale = node.bindScale;
    }

    for (const auto& channel : clip.channels)
    {
        auto& node = m_nodes[channel.targetNodeIndex];
        const XMFLOAT4 value = Sample(clip.samplers[channel.samplerIndex], time, channel.path);
        switch (channel.path)
        {
        case Path::Translation:
            node.translation = XMFLOAT3(value.x, value.y, value.z);
            break;
        case Path::Rotation:
            node.rotation = value;
            break;
        case Path::Scale:
            node.scale = XMFLOAT3(value.x, value.y, value.z);
            break;
        }
    }
}

void AnimationPlayer::BuildJointPalette()
{
    for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex)
    {
        const auto& node = m_nodes[nodeIndex];
        XMMATRIX local = XMLoadFloat4x4(&node.bindMatrix);
        if (!node.hasMatrix)
        {
            local =
                XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z)
                * XMMatrixRotationQuaternion(XMLoadFloat4(&node.rotation))
                * XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
        }
        XMStoreFloat4x4(&m_localTransforms[nodeIndex], local);
    }

    std::fill(m_globalTransformValid.begin(), m_globalTransformValid.end(), uint8_t(0));
    for (size_t jointIndex = 0; jointIndex < m_jointNodeIndices.size(); ++jointIndex)
    {
        const XMMATRIX inverseBind = XMLoadFloat4x4(&m_inverseBindMatrices[jointIndex]);
        const XMMATRIX jointGlobal = EvaluateGlobalTransform(m_jointNodeIndices[jointIndex]);
        XMStoreFloat4x4(
            &m_jointPalette[jointIndex],
            inverseBind * jointGlobal);
    }
}

XMMATRIX AnimationPlayer::EvaluateGlobalTransform(uint32_t nodeIndex)
{
    if (m_globalTransformValid[nodeIndex])
    {
        return XMLoadFloat4x4(&m_globalTransforms[nodeIndex]);
    }

    XMMATRIX global = XMLoadFloat4x4(&m_localTransforms[nodeIndex]);
    const int32_t parentIndex = m_nodes[nodeIndex].parentIndex;
    if (parentIndex >= 0)
    {
        global *= EvaluateGlobalTransform(static_cast<uint32_t>(parentIndex));
    }

    XMStoreFloat4x4(&m_globalTransforms[nodeIndex], global);
    m_globalTransformValid[nodeIndex] = 1;
    return global;
}
