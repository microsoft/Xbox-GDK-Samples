// Shared root signature + vertex-to-pixel structure for the magnifier overlay pass. Mirrors the
// AMD FidelityFX_FSR reference sample's magnifier: a full-screen triangle sampling the final
// pre-magnifier color image, drawing a zoomed-in circular inset (plus connecting border lines) so
// devs can visually compare fine detail without needing a separate capture/zoom tool.
#define MagnifierRootSignature \
    "RootFlags(DENY_HULL_SHADER_ROOT_ACCESS | " \
    "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
    "DENY_GEOMETRY_SHADER_ROOT_ACCESS)," \
    "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
    "filter = FILTER_MIN_MAG_MIP_POINT," \
    "addressU = TEXTURE_ADDRESS_BORDER," \
    "addressV = TEXTURE_ADDRESS_BORDER," \
    "addressW = TEXTURE_ADDRESS_BORDER," \
    "borderColor = STATIC_BORDER_COLOR_OPAQUE_BLACK," \
    "visibility = SHADER_VISIBILITY_PIXEL)"

struct FullScreenOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};
