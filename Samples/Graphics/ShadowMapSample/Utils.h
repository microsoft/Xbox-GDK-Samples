//--------------------------------------------------------------------------------------
// Utils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Offsets into the descriptor pile SRV for each descriptor
enum SRV_HEAP_OFFSETS
{
    FONT_OFFSET,
    SHADOW_MAP_OFFSET,
    VARIANCE_PASS1_OFFSET,
    VARIANCE_PASS2_OFFSET,
    EXPONENTIAL_PASS1_OFFSET,
    EXPONENTIAL_PASS2_OFFSET,
    EXP_VAR_PASS1_OFFSET,
    EXP_VAR_PASS2_OFFSET,
    FIXED_PORTION_OFFSET
};

// Describes each of the passes
enum RENDER_PASSES
{
    SHADOW_MAP_PASS,
    RESOLVE_PASS,
    FILTERING_PASS,
    SHADING_PASS,
    RENDER_PASSES_COUNT
};

static const wchar_t* g_wstrRenderPassNames[RENDER_PASSES_COUNT] =
{
    L"Shadow Map",
    L"Resolve",
    L"Filtering",
    L"Shading"
};
static_assert(RENDER_PASSES_COUNT == std::size(g_wstrRenderPassNames), "Mismatch between enum and reflected names");


// Offsets into the descriptor pile SRV for each descriptor
enum ROOT_PARAM_INDICES
{
    SHADOW_TEX_TABLE = 0,
    DIFFUSE_TEX_TABLE,
    OBJ_TRANSFORM_CBV,
    LIGHT_TRANSFORM_CBV,
    REMAPPING_CBV,
    SCENE_CONSTANTS_CBV,
    ROOT_PARAMS_COUNT
};

// Constant Buffers
struct CBSceneConstStruct
{
    DirectX::XMFLOAT4X4 g_mView;
    DirectX::XMFLOAT4X4 g_mProj;
};
struct CBSceneConstStructPadded
{
    CBSceneConstStruct  data;
    uint8_t             padding[128];
};
static_assert(sizeof(CBSceneConstStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "Error, constant buffer not properly aligned.");

struct CBTransformStruct
{
    DirectX::XMFLOAT4X4 g_mWorld;
};
struct CBTransformStructPadded
{
    CBTransformStruct   data;
    uint8_t             padding[192];
};
static_assert(sizeof(CBTransformStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "Error, constant buffer not properly aligned.");

constexpr uint32_t g_iNumLights = 1;
struct CBLightStruct
{
    struct LightData
    {
        DirectX::XMFLOAT4X4  m_mLightViewProj;
        DirectX::XMFLOAT4	m_vLightWorldDir;
        DirectX::XMFLOAT4	m_vLightColor;
        float       m_fSpecularPower;
        float       m_fDepthBias;
        float       m_padding1;
        float       m_padding2;
    } g_LightData[g_iNumLights];
    DirectX::XMFLOAT4        g_vAmbientColor;
    DirectX::XMFLOAT4        g_vEye;
};
struct CBLightStructPadded
{
    CBLightStruct   data;
    uint8_t         padding[112];
};
static_assert(sizeof(CBLightStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "Error, constant buffer not properly aligned.");

struct CBRemapStruct
{
    float g_fFarNearRatio;  // needed for re-linearizing depth
};
struct CBRemapStructPadded
{
    CBRemapStruct   data;
    uint8_t         padding[252];
};
static_assert(sizeof(CBRemapStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "Error, constant buffer not properly aligned.");

// Utility functions
template< typename t_A, typename t_B >
static inline t_A NextMultiple(const t_A& a, const t_B& b)
{
	const t_A a_mod_b = a % b;
	return a_mod_b ? (a + b - a_mod_b) : a;
}

template< typename t_Type >
t_Type Lerp(const t_Type& a, const t_Type& b, FLOAT f)
{
	return (1.0f - f) * a + f * b;
}

extern wchar_t const * const g_SampleTitle;
extern wchar_t const * const g_SampleDescription;

#pragma region SHADOW FILTERING TECHNIQUE
// What type of data are we storing to represent depth from the light perspective
enum DEPTH_REMAPPING : uint32_t  // typed enum
{
	DEPTH_REMAPPING_VARIANCE,
	DEPTH_REMAPPING_EXPONENTIAL,
	DEPTH_REMAPPING_EXPONENTIAL_VARIANCE,
	DEPTH_REMAPPING_STANDARD,
	DEPTH_REMAPPING_COUNT
};
static const wchar_t* const g_wstrDepthRemappingNames[] =
{
	L"Variance",
	L"Exponential",
	L"Exponential variance",
    L"No Remapping"
};
static_assert(DEPTH_REMAPPING_COUNT == std::size(g_wstrDepthRemappingNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region PCF SAMPLE FUNCTION
enum PCF_MODE : uint32_t
{
	PCF_MODE_NONE,
    PCF_MODE_SAMPLECMP_STEP1,
    PCF_MODE_SAMPLECMP_STEP2,
	PCF_MODE_GATHERCMP,
	PCF_MODE_COUNT
};

static const wchar_t* const g_wstrPCFModeNames[] =
{
	L"None",
	L"SampleCmp step by 1",
	L"SampleCmp step by 2",
	L"GatherCmp",
};
static_assert(DEPTH_REMAPPING_COUNT == std::size(g_wstrDepthRemappingNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region SHADOW MAP DIMENSIONS
enum SHADOW_MAP_DIMS : uint32_t
{
	SHADOW_MAP_DIMS_256x256,
	SHADOW_MAP_DIMS_512x512,
	SHADOW_MAP_DIMS_1024x1024,
	SHADOW_MAP_DIMS_2048x2048,
	SHADOW_MAP_DIMS_4096x4096,
	SHADOW_MAP_DIMS_COUNT
};
static const uint32_t g_iShadowMapDims[] =
{
	256,
	512,
	1024,
	2048,
	4096,
};
static_assert(SHADOW_MAP_DIMS_COUNT == std::size(g_iShadowMapDims), "Mismatch between enum and reflected names");
static const wchar_t* const g_wstrShadowMapDimsNames[] =
{
	L"256x256",
	L"512x512",
	L"1024x1024",
	L"2048x2048",
	L"4096x4096",
};
static_assert(SHADOW_MAP_DIMS_COUNT == std::size(g_wstrShadowMapDimsNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region KERNEL SIZE
// How much softness in the shadows, as measured by the size of the filter kernel
enum BLUR_KERNEL : uint32_t
{
	BLUR_KERNEL_1x1,
	BLUR_KERNEL_2x2,
	BLUR_KERNEL_4x4,
	BLUR_KERNEL_6x6,
	BLUR_KERNEL_8x8,
	BLUR_KERNEL_COUNT
};
static const uint32_t g_iBlurKernelSize[] =
{
	1,
	2,
	4,
	6,
	8,
};
static_assert(BLUR_KERNEL_COUNT == std::size(g_iBlurKernelSize), "Mismatch between enum and reflected names");
static const wchar_t* const g_wstrBlurKernelNames[] =
{
	L"1x1",
	L"2x2",
	L"4x4",
	L"6x6",
	L"8x8",
};
static_assert(BLUR_KERNEL_COUNT == std::size(g_wstrBlurKernelNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region BLUR TYPE
// Filtering used on shadow map
enum BLUR_TYPE : uint32_t
{
	BLUR_TYPE_GAUSSIAN,
	BLUR_TYPE_SQUARE,
	BLUR_TYPE_COUNT
};
static const wchar_t* const g_wstrBlurTypeNames[] =
{
	L"Constant",
	L"Gaussian",
};
static_assert(BLUR_TYPE_COUNT == std::size(g_wstrBlurTypeNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region FILTERING MODE ON SHADOW MAP
// Filtering used on shadow map
enum FILTER_MODE : uint32_t
{
	FILTER_MODE_POINT,
	FILTER_MODE_BILINEAR,
	FILTER_MODE_TRILINEAR,
	FILTER_MODE_ANISO,
	FILTER_MODE_COUNT
};

static D3D12_FILTER g_dwFilterModes[] =
{
	D3D12_FILTER_MIN_MAG_MIP_POINT,
	D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D12_FILTER_MIN_MAG_MIP_LINEAR,
	D3D12_FILTER_ANISOTROPIC,
};
static_assert(FILTER_MODE_COUNT == std::size(g_dwFilterModes), "Mismatch between enum and reflected names");
static D3D12_FILTER g_dwCompFilterModes[] =
{
	D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
	D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
	D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
	D3D12_FILTER_COMPARISON_ANISOTROPIC,
};
static_assert(FILTER_MODE_COUNT == std::size(g_dwFilterModes), "Mismatch between enum and reflected names");
static const wchar_t* const g_wstrFilterModeNames[] =
{
	L"Point",
	L"Bilinear",
	L"Trilinear",
	L"Aniso",
};
static_assert(FILTER_MODE_COUNT == std::size(g_wstrFilterModeNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region MSAA
// Multisampling mode used to render the shadow map (only makes sense for pre-filterable techniques)
enum MSAA_LEVEL : uint32_t
{
	MSAA_LEVEL_NONE,
	MSAA_LEVEL_2x,
	MSAA_LEVEL_4x,
	MSAA_LEVEL_8x,
	MSAA_LEVEL_COUNT
};
static const uint32_t g_iMSAALevelNumSamples[] =
{
	1,
	2,
	4,
	8,
};
static_assert(MSAA_LEVEL_COUNT == std::size(g_iMSAALevelNumSamples), "Mismatch between enum and reflected names");
static const wchar_t* const g_wstrMSAALevelNames[] =
{
	L"None",
	L"2x",
	L"4x",
	L"8x",
};
static_assert(MSAA_LEVEL_COUNT == std::size(g_wstrMSAALevelNames), "Mismatch between enum and reflected names");
enum MSAA_ON_OFF : uint32_t
{
    MSAA_ON,
    MSAA_OFF,
    MSAA_ON_OFF_COUNT
};
#pragma endregion

#pragma region CULLING
enum SHADOW_CULL : uint32_t
{
	SHADOW_CULL_NONE,
	SHADOW_CULL_FRONT,
	SHADOW_CULL_BACK,
	SHADOW_CULL_COUNT
};
static const wchar_t* const g_wstrShadowCullNames[] =
{
	L"Draw both faces",
	L"Draw back faces",
	L"Draw front faces",
};
static_assert(SHADOW_CULL_COUNT == std::size(g_wstrShadowCullNames), "Mismatch between enum and reflected names");
#pragma endregion

#pragma region SHADOW COMPRESSION
enum SHADOW_COMPRESSION : uint32_t
{
	SHADOW_COMPRESSED,
	SHADOW_UNCOMPRESSED,
#if defined(_XBOX_ONE) && defined(_TITLE) 
	SHADOW_UNCOMPRESSED_DEPTH_BOUNDS,
#endif
	SHADOW_COMPRESSION_COUNT
};
static const wchar_t* const g_wstrShadowCompressionNames[] =
{
	L"Compressed",
	L"Uncompressed",
#if defined(_XBOX_ONE) && defined(_TITLE) 
	L"Uncompressed (DB clear)",
#endif
};
static_assert(SHADOW_COMPRESSION_COUNT == std::size(g_wstrShadowCompressionNames), "Mismatch between enum and reflected names");
#pragma endregion


// Input layout for passes that draw geometry
namespace
{
    D3D12_INPUT_ELEMENT_DESC gElemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescNormal = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescColor = { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTexcoord = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescTangent = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gElemDescBinormal = { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC gInputElemArr[] = { gElemDescPosition , gElemDescNormal, gElemDescColor, gElemDescTexcoord, gElemDescTangent, gElemDescBinormal };
};


struct DirectionalLightInfo
{
    DirectX::SimpleMath::Matrix  dirLightView;
    DirectX::SimpleMath::Matrix  dirLightProj;
    DirectX::SimpleMath::Vector4 lightPosition;
    DirectX::SimpleMath::Vector4 lightForward;
    float   lightViewportWidth  = 0;
    float   lightViewportHeight = 0;
    float   lightNear           = 0;
    float   lightFar            = 0;

    void UpdateMatrices();

    void SetForwardVector(DirectX::SimpleMath::Vector4 forward);
};


struct VertexMesh
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    uint32_t color;
	DirectX::XMFLOAT2 texcoord;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 binormal;
};
typedef UINT32 IndexMesh;


/// <summary>
/// Utility class for holding all shader names and blobs
/// </summary>
class ShaderInfo
{
public:
    std::vector<uint8_t> VS_BLOB;
    std::vector<uint8_t> VSNull_BLOB;
    std::vector<uint8_t> GS_BLOB;
    std::vector<uint8_t> PS_Unlit_BLOB;
    std::vector<uint8_t> PS_LightSrc_BLOB;

    std::vector<uint8_t> Resolve_BLOBS[DEPTH_REMAPPING_COUNT - 1];
    std::vector<uint8_t> FilterHrz_BLOBS[MSAA_ON_OFF_COUNT][DEPTH_REMAPPING_COUNT - 1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    std::vector<uint8_t> FilterVrt_BLOBS[DEPTH_REMAPPING_COUNT - 1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    std::vector<uint8_t> ShadingPCFSampleCmpStep1_BLOBS[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    std::vector<uint8_t> ShadingPCFSampleCmpStep2_BLOBS[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    std::vector<uint8_t> ShadingPCFGatherCmp_BLOBS[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    std::vector<uint8_t> ShadingNoPCF_BLOBS[DEPTH_REMAPPING_COUNT];

    ShaderInfo()
    {
        this->Initialize();
    }

    virtual ~ShaderInfo() {}

    ShaderInfo(ShaderInfo const& rhs) = delete;
    ShaderInfo& operator=(ShaderInfo const& rhs) = delete;

    ShaderInfo(ShaderInfo && rhs) = default;
    ShaderInfo& operator=(ShaderInfo && rhs) = default;

    void Initialize();

private:
    // Standalone shader names
    wchar_t const* PSUnlitName = L"PSUnlit.cso";
    wchar_t const* PSLightSrcName = L"PSLightSrc.cso";
    wchar_t const* GShaderName = L"GSFullScreenTriangle.cso";
    wchar_t const* VShaderName = L"VSMesh.cso";
    wchar_t const* VShaderNullName = L"VSNull.cso";

    // Shader names grouped in arrays
    wchar_t const* PSNamesResolve[DEPTH_REMAPPING_COUNT - 1];
    wchar_t const* DirectionalBlurHrzPSNames[2][DEPTH_REMAPPING_COUNT - 1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    wchar_t const* DirectionalBlurVrtPSNames[DEPTH_REMAPPING_COUNT - 1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    wchar_t const* ShadingPCFGatherCmp[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    wchar_t const* ShadingPCFSampleCmpStep1[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    wchar_t const* ShadingPCFSampleCmpStep2[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    wchar_t const* ShadingNonPCF[DEPTH_REMAPPING_COUNT];

    void SetupShaderNames();
    void LoadShaderBlobs();
};
