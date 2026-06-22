//--------------------------------------------------------------------------------------
// Utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "RayTracingHlslCompat.h"

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(uint32_t) + 1)

enum DESCRIPTOR_HEAP_INDICES
{
    DESCRIPTOR_INDEX_INDEX_BUFFER = 0,
    DESCRIPTOR_INDEX_VERTEX_BUFFER,
    DESCRIPTOR_INDEX_DXR_OUTPUT,
};

enum GPU_TIMER
{
    TIMER_RAYTRACING_PASS = 0,
    TIMER_COPY_PASS,
    TIMER_HUD_PASS,
    TIMER_COUNT
};

enum FONT_TYPE
{
    TEXT_FONT_OFFSET = 0,
    CONTROLLER_FONT_OFFSET,
    FONT_COUNT
};

namespace GeometryType
{
    enum Enum
    {
        Triangle = 0,
        AABB,       // Procedural geometry with an application provided AABB.
        Count
    };
}

struct AccelerationStructureBuffers
{
    Microsoft::WRL::ComPtr<ID3D12Resource>  scratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>  accelerationStructure;
    Microsoft::WRL::ComPtr<ID3D12Resource>  instanceDesc;    // Used only for top-level AS
    uint64_t                                ResultDataMaxSizeInBytes{0};
};

// Bottom-level acceleration structures (BottomLevelASType).
// This sample uses two BottomLevelASType, one for AABB and one for Triangle geometry.
// Mixing of geometry types within a BLAS is not supported.
namespace BottomLevelASType = GeometryType;

namespace IntersectionShaderType
{
    enum Enum
    {
        AnalyticPrimitive = 0,
        VolumetricPrimitive,
        SignedDistancePrimitive,
        Count
    };

    inline uint32_t PerPrimitiveTypeCount(Enum type)
    {
        switch (type)
        {
        case AnalyticPrimitive: return AnalyticPrimitive::Count;
        case VolumetricPrimitive: return VolumetricPrimitive::Count;
        case SignedDistancePrimitive: return SignedDistancePrimitive::Count;
        case Count:
            assert(false);
            break;
        }
        return 0;
    }

    static const uint32_t MaxPerPrimitiveTypeCount = std::max(static_cast<uint32_t>(AnalyticPrimitive::Count),
        std::max(static_cast<uint32_t>(VolumetricPrimitive::Count), static_cast<uint32_t>(SignedDistancePrimitive::Count)));

    static const uint32_t TotalPrimitiveCount = AnalyticPrimitive::Count + VolumetricPrimitive::Count + SignedDistancePrimitive::Count;
}

namespace InlineRootSignature
{
    enum RootParams
    {
        AccelerationStructure = 0,
        ShaderTable,
        SceneConstant,
        AABBattributeBuffer,
        VertexBuffers,
        OutputView,
        Count
    };
}

namespace GlobalRootSignature
{
    namespace Slot
    {
        enum Enum
        {
            AccelerationStructure = 0,
            SceneConstant,
            AABBattributeBuffer,
            VertexBuffers,
            OutputView,
            Count
        };
    }
}

namespace LocalRootSignature
{
    namespace Type
    {
        enum Enum
        {
            Triangle = 0,
            AABB,
            Count
        };
    }
}
namespace LocalRootSignature
{
    namespace Triangle
    {
        namespace Slot
        {
            enum Enum
            {
                MaterialConstant = 0,
                Count
            };
        }
        struct RootArguments
        {
            PrimitiveConstantBuffer materialCb;
        };
    }
}
namespace LocalRootSignature
{
    namespace AABB
    {
        namespace Slot
        {
            enum Enum
            {
                MaterialConstant = 0,
                GeometryIndex,
                Count
            };
        }
        struct RootArguments
        {
            PrimitiveConstantBuffer materialCb;
            PrimitiveInstanceConstantBuffer aabbCB;
        };
    }
}
namespace LocalRootSignature
{
    inline uint32_t MaxRootArgumentsSize()
    {
        return static_cast<uint32_t>(std::max(sizeof(Triangle::RootArguments), sizeof(AABB::RootArguments)));
    }
}
