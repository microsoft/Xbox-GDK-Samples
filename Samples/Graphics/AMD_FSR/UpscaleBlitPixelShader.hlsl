#include "UpscaleBlitCommon.hlsli"

Texture2D<float4> SourceTexture : register(t0);
SamplerState LinearSampler : register(s0);

// Scales the full-screen triangle's 0..1 UV range down to the sub-rect of SourceTexture that is
// actually populated this frame: 1.0 when sourcing FSR's upsample output (always fully written,
// display-sized), or renderResolution / textureResolution when bypassing FSR and sourcing the
// offscreen scene-color target directly (allocated at max/display size, but only its top-left
// render-resolution sub-rect holds this frame's image).
cbuffer UVScaleConstants : register(b0)
{
    float2 UVScale;
};

[RootSignature(UpscaleBlitRootSignature)]
float4 main(FullScreenOutput input) : SV_Target
{
    return SourceTexture.Sample(LinearSampler, input.TexCoord * UVScale);
}
