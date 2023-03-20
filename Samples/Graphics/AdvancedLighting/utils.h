//--------------------------------------------------------------------------------------
// utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

constexpr uint32_t GPASS_RT_COUNT = 3;
constexpr DXGI_FORMAT GPASS_RT_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;

// Offsets into the descriptor pile SRV for each descriptor
enum SRV_HEAP_OFFSETS
{
    TEXT_FONT_OFFSET = 0,
    CONTROLLER_FONT_OFFSET,
    SCENE_TEXTURE_OFFSET
};

// Describes each of the for the timer
enum GPU_TIMER_PASSES
{
    GPU_TIMER_GEOMETRY_PASS = 0,
    GPU_TIMER_AMBIENT,
    GPU_TIMER_LIGHT_VOLUMES,
    GPU_TIMER_TILED_PASS,
    GPU_TIMER_CLUSTERED_PASS,
    GPU_TIMER_UAV_COPY_TO_RT,
    GPU_TIMER_NUM
};

enum LIGHT_TECHNIQUE
{
    LIGHT_VOLUMES = 0,
    TILED,
    CLUSTERED,
    NUM_TECHNIQUES
};
static wchar_t const* g_wstrLightTechniquesNames[] =
{
    L"Light Volumes",
    L"Tiled",
    L"Clustered",
};
static_assert(std::size(g_wstrLightTechniquesNames) == LIGHT_TECHNIQUE::NUM_TECHNIQUES);


// For tiled and clustered compute passes - These two have to match the defines in shared.hlsli.
constexpr uint32_t GROUP_WIDTH = 8;
constexpr uint32_t CLUSTERS_Z_COUNT = 16;

struct ZClusterRanges
{
    DirectX::SimpleMath::Vector4 range; // x=min, y=max
};

// Struct used to pass a one time calculation to the clustered pass
struct zClusterRangesCB
{
    ZClusterRanges                  zClusterRanges[CLUSTERS_Z_COUNT];
};
struct zClusterRangesCBPadded
{
    zClusterRangesCB                data;
};
static_assert(sizeof(zClusterRangesCBPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0
    && L"Bad allignment for constant buffer.");

// Struct representing scene constants, like camera related matrices.
// USed on multiple passes (ambient pass, light volumes, and tiled).
struct SceneConstantsStruct
{
    DirectX::SimpleMath::Matrix     matViewProj;
    DirectX::SimpleMath::Matrix     matView;
    DirectX::SimpleMath::Matrix     matInvProj;
    DirectX::SimpleMath::Vector3    cameraPos;
    uint32_t                        sceneLightCount;
    float                           lightRadius;
    uint32_t                        tileCorrectScreenWidth;
    uint32_t                        tileCorrectScreenHeight;
    float                           zMinView;
    float                           zMaxView;
};
struct SceneConstantsStructPadded
{
    SceneConstantsStruct            data;
#ifdef _GAMING_DESKTOP
    uint8_t                         pad[28];
#else
    uint8_t                         pad[12];
#endif
};
static_assert(sizeof(SceneConstantsStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0
    && L"Bad allignment for constant buffer.");


// Per object data, like model matrix. Used on Geometry pass.
struct PerObjectStruct
{
    DirectX::SimpleMath::Matrix worldMat;
    DirectX::SimpleMath::Matrix worldMatRotation;
};
struct PerObjectStructPadded
{
    PerObjectStruct  data;
#ifdef _GAMING_DESKTOP
    uint8_t             pad[128];
#endif
};
static_assert(sizeof(PerObjectStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0
    && L"Bad allignment for constant buffer.");


// CBuffer used by the compute shader when updating particles position
struct UpdateConstantsCB
{
    DirectX::SimpleMath::Matrix     g_View;
    DirectX::SimpleMath::Vector4    g_ViewFrustum[6];
    DirectX::SimpleMath::Vector4    g_ParticleData; // x = movement step, y = radii
};
struct UpdateConstantsCBPadded
{
    UpdateConstantsCB   data;
#ifdef _GAMING_DESKTOP
    uint8_t             pad[80];
#endif
};
static_assert(sizeof(UpdateConstantsCBPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 && "Padded CB does not align properly.");

// CB used by the light rendered via Execute Indirect
struct LightParticlesSceneConsts
{
    DirectX::SimpleMath::Matrix     g_ViewProj;
};
struct LightParticlesSceneConstsPadded
{
    LightParticlesSceneConsts     data;
#ifdef _GAMING_DESKTOP
    uint8_t         pad[192];
#endif
};
static_assert(sizeof(LightParticlesSceneConstsPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 && "Padded CB does not align properly.");

class Icosahedron
{
private:
    struct particleVertexLayout
    {
        float position[3];
        float color[3];
    };

    static const uint32_t m_vertexCount = 12;
    static const uint32_t m_indexCount = 60;

    particleVertexLayout m_vertices[m_vertexCount];
    uint16_t m_indices[m_indexCount];

public:
    void initialize(float m_meshRadius = 1.0f)
    {
        const float scale = m_meshRadius * 1.0f / sqrtf((5.0f + sqrtf(5.0f)) / 2.0f);
        const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
        const float phiScale = phi * scale;

        particleVertexLayout verts[] =
        {
            { {-scale, -phiScale, 0.0f }, {1.0f, 0.0f, 0.0f}},   // Top     Left
            { {-scale,  phiScale, 0.0f }, {0.0f, 1.0f, 0.0f}},   // Top     Right
            { { scale, -phiScale, 0.0f }, {0.0f, 0.0f, 1.0f}},   // Bottom  Right
            { { scale,  phiScale, 0.0f }, {1.0f, 0.0f, 1.0f}},   // Bottom  Left
            { { 0.0f, -scale, -phiScale }, {1.0f, 0.0f, 0.0f}},  // Top     Left
            { { 0.0f, -scale,  phiScale }, {0.0f, 1.0f, 0.0f}},  // Bottom  Right
            { { 0.0f,  scale, -phiScale }, {0.0f, 1.0f, 0.0f}},  // Bottom  Right
            { { 0.0f,  scale,  phiScale }, {0.0f, 1.0f, 0.0f}},  // Bottom  Right
            { {-phiScale, 0.0f, -scale }, {0.0f, 1.0f, 0.0f}},   // Bottom  Right
            { { phiScale, 0.0f, -scale }, {0.0f, 1.0f, 0.0f}},   // Bottom  Right
            { {-phiScale, 0.0f,  scale }, {0.0f, 1.0f, 0.0f}},   // Bottom  Right
            { { phiScale, 0.0f,  scale }, {0.0f, 1.0f, 0.0f}}    // Bottom  Right
        };
        memcpy(m_vertices, verts, sizeof(verts));

        uint16_t inds[] =
        {
              0,  4,  2,
              0,  2,  5,
              0,  5, 10,
              0, 10,  8,
              0,  8,  4,
              3,  1,  7,
              3,  7, 11,
              3, 11,  9,
              3,  9,  6,
              3,  6,  1,
              4,  9,  2,
              2,  9, 11,
              5,  2, 11,
              5, 11,  7,
              5,  7, 10,
             10,  7,  1,
             10,  1,  8,
              8,  1,  6,
              8,  6,  4,
              4,  6,  9
        };
        memcpy(m_indices, inds, sizeof(inds));
    }

    uint32_t GetIndexCount() const noexcept { return m_indexCount; }
    DXGI_FORMAT GetIndexFormat() const noexcept { return DXGI_FORMAT_R16_UINT; }
    uint32_t GetVertexCount() const noexcept { return m_vertexCount; }
    uint32_t GetVertexStride() const noexcept { return sizeof(particleVertexLayout); }
    uint32_t GetVertexSizeBytes() const noexcept { return sizeof(m_vertices); }
    uint32_t GetIndicesSizeBytes() const noexcept { return sizeof(m_indices); }
    particleVertexLayout const* GetVertices() const noexcept { return m_vertices; }
    uint16_t const* GetIndices() const noexcept { return m_indices; }
};
