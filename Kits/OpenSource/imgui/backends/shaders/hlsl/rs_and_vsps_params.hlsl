// Desktop builds require the Windows SDK version 10.0.22000.0 or later; controlled by the $(WindowsTargetPlatformVersion) setting.

// The sampler filter is baked into the (emplaced) root signature. Xbox requires shaders to be
// precompiled against a root signature, and the PSO's root signature must match that emplaced one
// or the shader is recompiled at runtime. To support both bilinear (default) and point/nearest
// sampling without a mismatch, the shaders are compiled twice: once as-is (linear) and once with
// POINT_SAMPLER defined (point). Each variant emplaces a matching root signature.
#ifdef POINT_SAMPLER
#define IMGUI_SAMPLER_FILTER "   filter          = FILTER_MIN_MAG_MIP_POINT,             "
#else
#define IMGUI_SAMPLER_FILTER "   filter          = FILTER_MIN_MAG_MIP_LINEAR,            "
#endif

#define ROOT_SIGNATURE                                              \
    "RootFlags(                                                 "   \
    "   ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |                    "   \
    "   DENY_HULL_SHADER_ROOT_ACCESS |                          "   \
    "   DENY_DOMAIN_SHADER_ROOT_ACCESS |                        "   \
    "   DENY_GEOMETRY_SHADER_ROOT_ACCESS),                      "   \
    "RootConstants(b0,                                          "   \
    "   num32BitConstants   = 16,                               "   \
    "   visibility          = SHADER_VISIBILITY_VERTEX),        "   \
    "DescriptorTable(                                           "   \
    "   SRV(t0,                                                 "   \
    "       numDescriptors  = 1,                                "   \
    "       space           = 0),                               "   \
    "   visibility          = SHADER_VISIBILITY_PIXEL),         "   \
    "StaticSampler(s0,                                          "   \
    IMGUI_SAMPLER_FILTER                                            \
    "   addressU        = TEXTURE_ADDRESS_WRAP,                 "   \
    "   addressV        = TEXTURE_ADDRESS_WRAP,                 "   \
    "   addressW        = TEXTURE_ADDRESS_WRAP,                 "   \
    "   mipLODBias      = 0.0f,                                 "   \
    "   maxAnisotropy   = 0,                                    "   \
    "   comparisonFunc  = COMPARISON_ALWAYS,                    "   \
    "   borderColor     = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"   \
    "   minLOD          = 0.0f,                                 "   \
    "   maxLOD          = 0.0f,                                 "   \
    "   space           = 0,                                    "   \
    "   visibility      = SHADER_VISIBILITY_PIXEL               "   \
    ")"

#define imgui_impl_dx12_rs ROOT_SIGNATURE

struct VSPS_PARAMS
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
