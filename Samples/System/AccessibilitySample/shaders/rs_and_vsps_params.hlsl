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
    "   filter          = FILTER_MIN_MAG_MIP_LINEAR,            "   \
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
