//--------------------------------------------------------------------------------------
// Util.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "EQAAConstants.h"

using namespace DirectX;

constexpr uint32_t c_maxLogFragments = 3;    // 8xMSAA

// The available scenes (add more here, and then add initialization data in Sample::Initialize)
enum ALIASING_SCENE
{
	ALIASING_SCENE_GOTHIC_WINDOW,
	ALIASING_SCENE_CACTUS,
	ALIASING_SCENE_CONTROLLER,
	ALIASING_SCENE_WHEEL_OF_FORTUNE,

	ALIASING_SCENE_COUNT
};

// The different closeup views in the zoom pane
enum VIEW_MODE
{
	VIEW_MODE_RESOLVED,
	VIEW_MODE_NEAREST_SAMPLE,
	VIEW_MODE_SINGLE_SAMPLE,
	VIEW_MODE_ALL_SAMPLES,

	VIEW_MODE_COUNT
};
static const wchar_t* const g_viewModeNames[] =
{
	L"Resolved",
	L"Nearest sample",
	L"Single sample",
	L"All samples",
};
static_assert(VIEW_MODE_COUNT == _countof(g_viewModeNames), "Mismatch between enum and reflected names");

// The thumbsticks and triggers have different functions based on this option
enum CONTROL_MODE
{
	CONTROL_MODE_ZOOM,
	CONTROL_MODE_CAMERA,
	CONTROL_MODE_LIGHT,

	CONTROL_MODE_COUNT
};
static const wchar_t* const g_controlModeNames[] =
{
	L"Zoom",
	L"Camera",
	L"Light",
};
static_assert(CONTROL_MODE_COUNT == _countof(g_controlModeNames), "Mismatch between enum and reflected names");

// Utility functions
template< typename t_A, typename t_B >
constexpr t_A NextMultipleConstexpr(const t_A& a, const t_B& b)
{
	const t_A a_mod_b = a % b;
	return a_mod_b ? (a + b - a_mod_b) : a;
}

// Helper structs/classes

// General world-view-proj transform and associated data
struct Transforms
{
	void Set(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj)
	{
		m_world = world;
		m_view = view;
		m_proj = proj;

		m_worldViewProj = m_world * m_view * m_proj;
		m_viewInverse = XMMatrixInverse(nullptr, m_view);
		m_eyeWorldPos = XMVector3TransformCoord(XMVectorZero(), m_viewInverse);
	}

	XMMATRIX                m_world;
	XMMATRIX                m_view;
	XMMATRIX                m_viewInverse;
	XMMATRIX                m_proj;
	XMMATRIX                m_worldViewProj;
	XMVECTOR                m_eyeWorldPos;
};

// The current camera, one per scene
struct CameraSettings
{
	CameraSettings(
		XMVECTOR cameraPosition = XMVectorZero(),
		XMVECTOR cameraDirection = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		XMVECTOR cameraRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
		XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
		float translationSpeed = 1.0f,
		float rotationSpeed = 1.0f,
		float nearPlane = 0.05f,
		float farPlane = 100.0f
	)
		: m_cameraPosition(cameraPosition)
		, m_cameraDirection(cameraDirection)
		, m_cameraRight(cameraRight)
		, m_cameraUp(cameraUp)
		, m_translationSpeed(translationSpeed)
		, m_rotationSpeed(rotationSpeed)
		, m_nearPlane(nearPlane)
		, m_farPlane(farPlane)
	{
	}

	XMVECTOR                    m_cameraPosition;
	XMVECTOR                    m_cameraDirection;
	XMVECTOR                    m_cameraRight;
	XMVECTOR                    m_cameraUp;
	float                       m_translationSpeed;
	float                       m_rotationSpeed;
	float                       m_nearPlane;
	float                       m_farPlane;

	static CameraSettings       m_default;
};
__declspec(selectany) CameraSettings CameraSettings::m_default;

// The current zoom settings, one per scene
struct ZoomSettings
{
	ZoomSettings(
		float zoom = 0.02f,
		float offsetX = 0.0f,
		float offsetY = 0.0f,
		float zoomSpeed = 1.0f
	)
		: m_zoom(zoom)
		, m_offsetX(offsetX)
		, m_offsetY(offsetY)
		, m_zoomSpeed(zoomSpeed)
	{
	}

	float                       m_zoom;
	float                       m_offsetX;
	float                       m_offsetY;
	float                       m_zoomSpeed;

	static ZoomSettings         m_default;
};
__declspec(selectany) ZoomSettings ZoomSettings::m_default;

// The current light settings, one per light (currently two lights per scene)
struct LightSettings
{
	LightSettings(XMVECTOR dir = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
		XMVECTOR color = XMVectorSet(0.8f, 0.8f, 0.4f, 1.0f),
		float specularPower = 16.0f)
		: m_worldDir(dir)
		, m_color(color)
		, m_specularPower(specularPower)
	{
	}

	XMVECTOR                    m_worldDir;
	XMVECTOR                    m_color;
	float                       m_specularPower;

	static LightSettings        m_default;
	static LightSettings        m_defaultSecondary;
};
__declspec(selectany) LightSettings LightSettings::m_default;
__declspec(selectany) LightSettings LightSettings::m_defaultSecondary(
	XMVectorSet(-0.707f, 0.707f, 0.0f, 0.0f),    // XMVECTOR                    m_vWorldDir;
	XMVectorSet(0.2f, 0.3f, 0.6f, 1.0f),         // XMVECTOR                    m_vColor;
	32.0f                                        // float                       m_fSpecularPower;
);

// Data which varies per scene (e.g. mesh, camera and lighting)
struct Scene
{
	static constexpr uint32_t   c_numLights = 2;

	// Light indicator pane
	static Transforms           m_transformsIndicator;

	Scene(
		const wchar_t* name = L"",
		const wchar_t* meshName = L"",
		const wchar_t* folderName = L"",
		bool useNormalMap = false
	)
		: m_mame(name)
		, m_meshName(meshName)
		, m_folderName(folderName)
		, m_useNormalMap(useNormalMap)
	{
		m_cameraSettings = CameraSettings::m_default;   // automatic
		m_lightSettings[0] = LightSettings::m_default;   // automatic
		m_lightSettings[1] = LightSettings::m_defaultSecondary;
	}

	void SetInitialCamera(const CameraSettings& cameraSettings)
	{
		m_originalCameraSettings = m_cameraSettings = cameraSettings;
	}

	void SetInitialZoom(const ZoomSettings& zoomSettings)
	{
		m_originalZoomSettings = m_zoomSettings = zoomSettings;
	}

	void SetInitialLight(const LightSettings& lightSettings, uint32_t light)
	{
		m_originalLightSettings[light] = m_lightSettings[light] = lightSettings;
	}

	const wchar_t*                m_mame;
	const wchar_t*                m_meshName;
	const wchar_t*                m_folderName;

	bool                        m_useNormalMap;

	CameraSettings              m_originalCameraSettings;
	CameraSettings              m_cameraSettings;

	ZoomSettings                m_originalZoomSettings;
	ZoomSettings                m_zoomSettings;

	LightSettings               m_originalLightSettings[c_numLights];
	LightSettings               m_lightSettings[c_numLights];
};
__declspec(selectany) Transforms Scene::m_transformsIndicator;
__declspec(selectany) Scene g_scene[ALIASING_SCENE_COUNT];

//-------------------------------------------------------------------------------------------------------------
// Name: InitializeScenes()
// Desc: Set up camera, lighting, zoom paramters
//-------------------------------------------------------------------------------------------------------------
inline void InitializeScenes()
{
	// Scene initialization
	g_scene[ALIASING_SCENE_WHEEL_OF_FORTUNE] = Scene(
		L"Wheel of fortune",                                // const wchar_t*                m_name;
		L"",                                                // const wchar_t*                m_meshName;
		L"",                                                // const wchar_t*                m_folderName;
		false                                               // bool                        m_useNormalMap;
	);

	g_scene[ALIASING_SCENE_WHEEL_OF_FORTUNE].SetInitialCamera(CameraSettings(
		XMVectorSet(0.0f, 0.0f, 2.0f, 1.0f),            // XMVECTOR                    m_vCameraPosition;
		XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f),            // XMVECTOR                    m_vCameraDirection;
		XMVectorSet(-1.0f, 0.0f, 0.0f, 1.0f)             // XMVECTOR                    m_vCameraRight;
	));

	g_scene[ALIASING_SCENE_WHEEL_OF_FORTUNE].SetInitialZoom(ZoomSettings(
		0.016f,                                             // float                       m_fZoom;
		0.078f,                                             // float                       m_fOffsetX;
		0.0f                                                // float                       m_fOffsetY;
	));

	g_scene[ALIASING_SCENE_WHEEL_OF_FORTUNE].SetInitialLight(LightSettings::m_default, 0);
	g_scene[ALIASING_SCENE_WHEEL_OF_FORTUNE].SetInitialLight(LightSettings::m_defaultSecondary, 1);

	g_scene[ALIASING_SCENE_GOTHIC_WINDOW] = Scene(
		L"Gothic window",                                   // const wchar_t*                m_name;
		L"gothic_window",                                   // const wchar_t*                m_meshName;
		L"GothicWindow",                                    // const wchar_t*                m_folderName;
		true                                                // BOOL                        m_useNormalMap;
	);

	g_scene[ALIASING_SCENE_GOTHIC_WINDOW].SetInitialCamera(CameraSettings(
		XMVectorSet(8.82f, 38.97f, -0.36f, 1.0f),        // XMVECTOR                    m_vCameraPosition;
		XMVectorSet(-0.41f, -0.91f, -0.09f, 0.0f),        // XMVECTOR                    m_vCameraDirection;
		XMVectorSet(-0.04f, -0.08f, 0.99f, 0.0f),        // XMVECTOR                    m_vCameraRight;
		XMVectorSet(-0.91f, 0.42f, -0.01f, 0.0f)         // XMVECTOR                    m_vCameraUp;
	));

	g_scene[ALIASING_SCENE_GOTHIC_WINDOW].SetInitialZoom(ZoomSettings(
		0.012f,                                             // float                       m_fZoom;
		0.044f,                                             // float                       m_fOffsetX;
		0.0f                                                // float                       m_fOffsetY;
	));

	g_scene[ALIASING_SCENE_GOTHIC_WINDOW].SetInitialLight(LightSettings(
		XMVectorSet(-0.83f, -0.02f, 0.56f, 0.0f),        // XMVECTOR                    m_vWorldDir;
		XMVectorSet(0.55f, 0.55f, 0.27f, 0.0f)         // XMVECTOR                    m_vColor;
	),
		0);
	g_scene[ALIASING_SCENE_GOTHIC_WINDOW].SetInitialLight(LightSettings(
		XMVectorSet(0.46f, 0.34f, -0.82f, 0.0f),        // XMVECTOR                    m_vWorldDir;
		XMVectorSet(0.20f, 0.30f, 0.60f, 0.0f)         // XMVECTOR                    m_vColor;
	),
		1);

	g_scene[ALIASING_SCENE_CACTUS] = Scene(
		L"Cactus",                                          // const wchar_t*                m_name;
		L"cactus",                                          // const wchar_t*                m_meshName;
		L"Cactus",                                          // const wchar_t*                m_folderName;
		true                                                // bool                        m_useNormalMap;
	);

	g_scene[ALIASING_SCENE_CACTUS].SetInitialCamera(CameraSettings(
		XMVectorSet(9.21f, 44.54f, -167.f, 1.0f),        // XMVECTOR                    m_vCameraPosition;
		XMVectorSet(-0.24f, 0.00f, 0.97f, 0.0f),        // XMVECTOR                    m_vCameraDirection;
		XMVectorSet(0.97f, 0.00f, 0.24f, 0.0f),        // XMVECTOR                    m_vCameraRight;
		XMVectorSet(0.00f, 1.00f, 0.00f, 0.0f),        // XMVECTOR                    m_vCameraUp;
		100.0f,                                             // float                       m_fTranslationSpeed;
		CameraSettings::m_default.m_rotationSpeed,         // float                       m_fRotationSpeed;
		5.0f,                                               // float                       m_fNearPlane;
		10000.0f                                            // float                       m_fFarPlane;
	));

	g_scene[ALIASING_SCENE_CACTUS].SetInitialZoom(ZoomSettings(
		ZoomSettings::m_default.m_zoom,                  // float                       m_fZoom;
		-0.044f,                                            // float                       m_fOffsetX;
		0.290f                                              // float                       m_fOffsetY;
	));

	g_scene[ALIASING_SCENE_CACTUS].SetInitialLight(LightSettings::m_default, 0);
	g_scene[ALIASING_SCENE_CACTUS].SetInitialLight(LightSettings::m_defaultSecondary, 1);

	g_scene[ALIASING_SCENE_CONTROLLER] = Scene(
		L"Controller",                                      // const wchar_t*                m_name;
		L"controller",                                      // const wchar_t*                m_meshName;
		L"Controller",                                      // const wchar_t*                m_folderName;
		false                                               // bool                        m_useNormalMap;
	);

	g_scene[ALIASING_SCENE_CONTROLLER].SetInitialCamera(CameraSettings(
		XMVectorSet(0.00f, 0.00f, -5.00f, 1.0f)         // XMVECTOR                    m_vCameraPosition;
	));

	g_scene[ALIASING_SCENE_CONTROLLER].SetInitialZoom(ZoomSettings(
		0.014f,                                             // float                       m_fZoom;
		0.053f,                                             // float                       m_fOffsetX;
		-0.021f                                             // float                       m_fOffsetY;
	));

	g_scene[ALIASING_SCENE_CONTROLLER].SetInitialLight(LightSettings::m_default, 0);
	g_scene[ALIASING_SCENE_CONTROLLER].SetInitialLight(LightSettings::m_defaultSecondary, 1);
}

// Constant buffers --- each one should match the associated declaration in the .hlsl files
struct ConstantBufferTransform
{
	static constexpr uint32_t c_slot = 0;
	XMMATRIX m_worldViewProj;
	XMMATRIX m_world;
};

struct ConstantBufferLight
{
	static constexpr uint32_t c_slot = 0;
	static constexpr uint32_t c_numLights = 2;
	XMVECTOR    m_ambientColor;
	XMVECTOR    m_eyeWorldPos;
	BOOL        m_useNormalMap;
	struct LightData
	{
		XMVECTOR m_lightDir;
		XMVECTOR m_lightColor;
		float    m_specularPower;
	} m_lightData[c_numLights];
};

struct ConstantBufferGrid
{
	static constexpr uint32_t c_slot = 2;
	XMVECTOR    m_gridColor;
	BOOL        m_horizontal;
};

struct ConstantBufferPointSprite
{
	static constexpr uint32_t c_slot = 2;
	uint32_t        m_selectedSample;
};

struct ConstantBufferPointSpriteTransform
{
	static constexpr uint32_t c_slot = 0;
	XMMATRIX    m_positionTransform;
	XMMATRIX    m_texcoordTransform;
};

struct ConstantBufferViewMode
{
	static constexpr uint32_t c_slot = 0;
	uint32_t m_selectedSample;
};

#if ENABLE_EQAA
struct ConstantBufferFMask
{
	static constexpr uint32_t c_slot = 1;
	BOOL COLOR_EXPANDED;
	uint32_t LOG_NUM_FRAGMENTS;
	uint32_t LOG_NUM_SAMPLES;
	uint32_t NUM_FRAGMENTS;
	uint32_t NUM_SAMPLES;
	uint32_t INDEX_BITS;
	uint32_t INDEX_MASK;
	uint32_t VALID_INDEX_MASK;
	uint32_t INVALID_INDEX_MASK;
	uint32_t QUALITY;
};

struct ConstantBufferEQAA
{
	static constexpr uint32_t c_slot = 3;
	EQAASamplePositionsStruct m_EQAASamplePositions;
};
#endif

// Helper functions
#if ENABLE_EQAA
//-------------------------------------------------------------------------------------------------------------
// Name: CalcFMaskParams()
// Desc: Calculate the params to pass to shaders.
//-------------------------------------------------------------------------------------------------------------
constexpr ConstantBufferFMask CalcFMaskParams(uint32_t logFragments, uint32_t quality, bool useNativeFMask = true)
{
	const uint32_t maxQuality = 16;   // Upper bound for the number of quality levels we will consider
	assert(quality < maxQuality);

	// Hard-coded table
	constexpr uint32_t logSamplesTable[c_maxLogFragments + 1][maxQuality] =
	{
		{ 0, 1, 2, 3, 4 },
		{ 1, 1, 2, 3, 4 },
		{ 2, 2, 2, 3, 4 },
		{ 3, 3, 3, 3, 4 },
	};

	uint32_t logSamples = logSamplesTable[logFragments][quality];

	// EQAA has "unknown" as an extra code
	uint32_t bitsPerSample = (logSamples == logFragments) ? logFragments : (logFragments + 1);
	bitsPerSample = (bitsPerSample == 3) ? 4 : bitsPerSample;  // hardware rounds up to power-of-two
	bitsPerSample = useNativeFMask ? 4 : bitsPerSample;
	assert(bitsPerSample <= 4);

	uint32_t indexMask = useNativeFMask ? 0xf : ((1 << bitsPerSample) - 1);
	uint32_t validIndexMask = (1 << logFragments) - 1;
	uint32_t invalidIndexMask = indexMask & ~validIndexMask;

	ConstantBufferFMask constantBufferFMask =
	{
		quality == 0,                  // BOOL COLOR_EXPANDED;
		logFragments,                  // uint32_t LOG_NUM_FRAGMENTS;
		logSamples,                    // uint32_t LOG_NUM_SAMPLES;
		1U << logFragments,            // uint32_t NUM_FRAGMENTS;
		1U << logSamples,              // uint32_t NUM_SAMPLES;
		bitsPerSample,                 // uint32_t INDEX_BITS;
		indexMask,                     // uint32_t INDEX_MASK;
		validIndexMask,                // uint32_t VALID_INDEX_MASK;
		invalidIndexMask,              // uint32_t INVALID_INDEX_MASK;
		quality,                       // uint32_t QUALITY
	};

	return constantBufferFMask;
}

//-------------------------------------------------------------------------------------------------------------
// Name: CalcFMaskFormatRaw()
// Desc: The FMask is a uint32_t (or two) with enough bits to code the number of color samples per pixel,
// plus possibly a code for "unknown"
//-------------------------------------------------------------------------------------------------------------
constexpr DXGI_FORMAT CalcFMaskRawFormat(uint32_t logFragments, uint32_t quality)
{
	ConstantBufferFMask constantBufferFMask = CalcFMaskParams(logFragments, quality, false);

	uint32_t bitsPerPixel = constantBufferFMask.INDEX_BITS * constantBufferFMask.NUM_SAMPLES;

	assert(bitsPerPixel > 0 && bitsPerPixel <= 64);

	if (bitsPerPixel <= 8)
	{
		return DXGI_FORMAT_R8_UINT;
	}
	else if (bitsPerPixel <= 16)
	{
		return DXGI_FORMAT_R16_UINT;
	}
	else if (bitsPerPixel <= 32)
	{
		return DXGI_FORMAT_R32_UINT;
	}
	else //if( bitsPerPixel <= 64 )
	{
		return DXGI_FORMAT_R32G32_UINT;
	}
}

//-------------------------------------------------------------------------------------------------------------
// Name: CalcFMaskFormatNative()
// Desc: Assign the corresponding native FMASK format
//-------------------------------------------------------------------------------------------------------------
#ifdef _GAMING_XBOX_SCARLETT

struct D3D12X_SRV_FORMAT
{
    D3D12XBOX_IMAGE_FORMAT ImageFormat;
    uint32_t Shader4ComponentMapping;
};
constexpr D3D12X_SRV_FORMAT CalcFMaskNativeFormat(uint32_t logFragments, uint32_t quality)
{
    constexpr uint32_t maxQuality = 16;   // Upper bound for the number of quality levels we will consider
    assert(quality < maxQuality);

    // Hard-coded table
    static_assert(0 == D3D12XBOX_IMAGE_FORMAT_INVALID, "Invalid assumption: 0 == D3D12XBOX_DATA_FORMAT_INVALID");
    const D3D12XBOX_IMAGE_FORMAT imageFormats[c_maxLogFragments + 1][maxQuality] =
    {
        {
            D3D12XBOX_IMAGE_FORMAT_INVALID,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S2_F1,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S4_F1,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S8_F1,
            D3D12XBOX_IMAGE_FORMAT_FMASK16_S16_F1,
        },
        {
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S2_F2,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S2_F2,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S4_F2,
            D3D12XBOX_IMAGE_FORMAT_FMASK16_S8_F2,
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S16_F2,
        },
        {
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S4_F4,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S4_F4,
            D3D12XBOX_IMAGE_FORMAT_FMASK8_S4_F4,
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S8_F4,
            D3D12XBOX_IMAGE_FORMAT_FMASK64_S16_F4,
        },
        {
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S8_F8,
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S8_F8,
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S8_F8,
            D3D12XBOX_IMAGE_FORMAT_FMASK32_S8_F8,
            D3D12XBOX_IMAGE_FORMAT_FMASK64_S16_F8,
        },
    };

    D3D12X_SRV_FORMAT Format = {};
    Format.ImageFormat = imageFormats[logFragments][quality];
    assert(D3D12XBOX_IMAGE_FORMAT_INVALID != Format.ImageFormat); // We didn't fall into one of the supported cases

    Format.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
        D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
        (quality == 4) ? D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1 : D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
        D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
        D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);

    return Format;
}

#else // #ifdef _GAMING_XBOX_SCARLETT

struct D3D12X_SRV_FORMAT
{
	D3D12XBOX_DATA_FORMAT DataFormat;
	D3D12XBOX_NUMBER_FORMAT NumberFormat;
	uint32_t Shader4ComponentMapping;
};
constexpr D3D12X_SRV_FORMAT CalcFMaskNativeFormat(uint32_t logFragments, uint32_t quality)
{
	constexpr uint32_t maxQuality = 16;   // Upper bound for the number of quality levels we will consider
	assert(quality < maxQuality);

	// Hard-coded table
	static_assert(0 == D3D12XBOX_DATA_FORMAT_INVALID, "Invalid assumption: 0 == D3D12XBOX_DATA_FORMAT_INVALID");
	const D3D12XBOX_DATA_FORMAT imageFormats[c_maxLogFragments + 1][maxQuality] =
	{
		{
			D3D12XBOX_DATA_FORMAT_INVALID,
			D3D12XBOX_DATA_FORMAT_FMASK8_S2_F1,
			D3D12XBOX_DATA_FORMAT_FMASK8_S4_F1,
			D3D12XBOX_DATA_FORMAT_FMASK8_S8_F1,
			D3D12XBOX_DATA_FORMAT_FMASK16_S16_F1,
		},
		{
			D3D12XBOX_DATA_FORMAT_FMASK8_S2_F2,
			D3D12XBOX_DATA_FORMAT_FMASK8_S2_F2,
			D3D12XBOX_DATA_FORMAT_FMASK8_S4_F2,
			D3D12XBOX_DATA_FORMAT_FMASK16_S8_F2,
			D3D12XBOX_DATA_FORMAT_FMASK32_S16_F2,
		},
		{
			D3D12XBOX_DATA_FORMAT_FMASK8_S4_F4,
			D3D12XBOX_DATA_FORMAT_FMASK8_S4_F4,
			D3D12XBOX_DATA_FORMAT_FMASK8_S4_F4,
			D3D12XBOX_DATA_FORMAT_FMASK32_S8_F4,
			D3D12XBOX_DATA_FORMAT_FMASK64_S16_F4,
		},
		{
			D3D12XBOX_DATA_FORMAT_FMASK32_S8_F8,
			D3D12XBOX_DATA_FORMAT_FMASK32_S8_F8,
			D3D12XBOX_DATA_FORMAT_FMASK32_S8_F8,
			D3D12XBOX_DATA_FORMAT_FMASK32_S8_F8,
			D3D12XBOX_DATA_FORMAT_FMASK64_S16_F8,
		},
	};

	D3D12X_SRV_FORMAT Format = {};
	Format.DataFormat = imageFormats[logFragments][quality];
	assert(D3D12XBOX_DATA_FORMAT_INVALID != Format.DataFormat); // We didn't fall into one of the supported cases
	Format.NumberFormat = D3D12XBOX_NUMBER_FORMAT_UINT;

	Format.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
		D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
		(quality == 4) ? D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1 : D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
		D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
		D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);

	return Format;
}

#endif // #ifdef _GAMING_XBOX_SCARLETT

#endif
