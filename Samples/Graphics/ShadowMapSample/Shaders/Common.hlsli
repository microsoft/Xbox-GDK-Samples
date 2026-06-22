//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//-----------------------
// Root Signature (TEMP EXPERIMENT)
//-----------------------
#define ROOT_SIG    " RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
                    "            | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
                    "            | DENY_HULL_SHADER_ROOT_ACCESS)," \
                    " DescriptorTable (" \
                    "   SRV(t0, numDescriptors  = 1)," \
                    "   visibility=SHADER_VISIBILITY_PIXEL)," \
                    " DescriptorTable (" \
                    "   SRV(t1, numDescriptors  = 2)," \
                    "   visibility=SHADER_VISIBILITY_PIXEL)," \
                    " CBV(b0, space = 0)," \
                    " CBV(b1, space = 0)," \
                    " CBV(b2, space = 0)," \
                    " CBV(b3, space = 0)," \
                    " StaticSampler(s0, " \
                    "   visibility=SHADER_VISIBILITY_PIXEL)," \
                    " StaticSampler(s1," \
                    "   visibility=SHADER_VISIBILITY_PIXEL," \
                    "   borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK)" \


//------------------------------------------------------
// Common structures/functions for the shaders
//------------------------------------------------------

float Square( float f ) { return f * f; }

float Nop( float fIn ) { return fIn; }
float2 Nop( float2 fIn ) { return fIn; }
float3 Nop( float3 fIn ) { return fIn; }
float4 Nop( float4 fIn ) { return fIn; }


#define SUPPORT_NORMAL_MAP

//-------------------------------------
// Constant buffers
//-------------------------------------
struct CBTransformStruct
{
    float4x4 g_mWorld;
};

struct CBSceneConstStruct
{
    float4x4 g_mView;
    float4x4 g_mProj;
};

static const uint g_iNumLights = 1;
struct CBLightStruct
{
    struct LightData
    {
        float4x4    m_mLightViewProj;
        float4      m_vLightWorldDir;
        float4      m_vLightColor;
        float       m_fSpecularPower;
        float       m_fDepthBias;
        float       m_padding1;
        float       m_padding2;
    } g_LightData[g_iNumLights];
    float4  g_vAmbientColor;
    float4  g_vEye;
};

struct CBRemapStruct
{
    float g_fFarNearRatio;  // needed for re-linearizing depth
};


//-----------------------------------------
// Shader import and export types
//-----------------------------------------
struct VertexMesh
{
    float4 position     : POSITION0;
    float3 normal       : NORMAL0;
    float3 color        : COLOR0;
    float2 texcoord     : TEXCOORD0;
#ifdef SUPPORT_NORMAL_MAP
    float3 tangent      : TANGENT0;
    float3 binormal     : BINORMAL0;
#endif
};

struct InterpolantsMesh
{
    float4 position     : SV_POSITION0;
    float3 normal       : NORMAL0;
    float3 color        : COLOR0;
    float2 texcoord     : TEXCOORD0;
#ifdef SUPPORT_NORMAL_MAP
    float3 tangent      : TANGENT0;
    float3 binormal     : BINORMAL0;
#endif
    float3 posworld	    : TEXCOORD1;
};

// Pixel shader output
struct Pixel
{
    float4 color        : SV_TARGET0;
};

// These two are for passing a single point, for the FSQ pass
struct VertexPoint
{
    float4 position : POSITION0;
};

struct InterpolantsPoint
{
    float4 position : SV_Position;
};

// Geometry shader output (also for FSQ pass)
struct InterpolantsTexcoord
{
    float4 position : SV_POSITION0;
    float2 texcoord : TEXCOORD0;
};
