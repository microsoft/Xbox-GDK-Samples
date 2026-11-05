#include "MagnifierCommon.hlsli"

// Ported from AMD's FidelityFX_FSR reference sample's MagnifierPS.hlsl. Draws a zoomed-in
// circular inset of the area around MousePos, connected to the original area by two tangent
// border lines, so devs can visually inspect fine detail (e.g. upscale reconstruction quality)
// without needing a separate capture/zoom tool.
Texture2D    SourceColor : register(t0);
SamplerState PointBorderSampler : register(s0);

cbuffer MagnifierConstants : register(b0)
{
    uint  ImageWidth;
    uint  ImageHeight;
    int   MousePosX;
    int   MousePosY;

    float4 BorderColorRGBA; // linear RGBA

    float MagnificationAmount;   // [1-...]
    float MagnifierScreenRadius; // [0-1]
    int   MagnifierOffsetX;      // in pixels
    int   MagnifierOffsetY;      // in pixels
};

[RootSignature(MagnifierRootSignature)]
float4 main(FullScreenOutput input) : SV_Target
{
    const float2 uv = input.TexCoord;
    float4 outColor = SourceColor.SampleLevel(PointBorderSampler, uv, 0);

    const float2 ImageSize = float2(ImageWidth, ImageHeight);
    const float AspectRatio = ImageSize.x / ImageSize.y;
    const float AspectRatioInv = ImageSize.y / ImageSize.x;
    const float2 MagnifierAreaCenter = float2(MousePosX, MousePosY);
    const float2 MagnifierOnScreenLocation = float2(MousePosX + MagnifierOffsetX, MousePosY + MagnifierOffsetY);

    const float2 uvMagnifiedArea = float2(MagnifierAreaCenter / ImageSize);
    const float2 uvMagnifierOnScreen = float2(MagnifierOnScreenLocation / ImageSize);

    const float MagnifiedAreaR = MagnifierScreenRadius / MagnificationAmount;
    const float MagnifierOnScreenR = MagnifierScreenRadius;

    float2 uvRArea = uv - uvMagnifiedArea;
    if (ImageSize.x > ImageSize.y)
        uvRArea.x *= AspectRatio;
    else
        uvRArea.y *= AspectRatioInv;

    float2 uvROnScreen = uv - uvMagnifierOnScreen;
    if (ImageSize.x > ImageSize.y)
        uvROnScreen.x *= AspectRatio;
    else
        uvROnScreen.y *= AspectRatioInv;

    const float BORDER_THICKNESS_OFFSET = 0.001f;

    const bool bAreWeInMagnifiedAreaRegion = length(uvRArea) < MagnifiedAreaR;
    const bool bAreWeInMagnifiedAreaBorderRegion = length(uvRArea) < (MagnifiedAreaR + BORDER_THICKNESS_OFFSET);

    const bool bAreWeInMagnifierOnScreenRegion = length(uvROnScreen) < MagnifierOnScreenR;
    const bool bAreWeInMagnifierOnScreenBorderRegion = length(uvROnScreen) < (MagnifierOnScreenR + BORDER_THICKNESS_OFFSET);

    if (bAreWeInMagnifiedAreaRegion)
    {
        return outColor;
    }

    if (bAreWeInMagnifierOnScreenBorderRegion || bAreWeInMagnifiedAreaBorderRegion)
    {
        outColor.rgb = BorderColorRGBA.rgb;
    }

    if (bAreWeInMagnifierOnScreenRegion)
    {
        float2 sampleUVOffsetFromCenter = uv - uvMagnifierOnScreen;
        sampleUVOffsetFromCenter /= MagnificationAmount;
        const float2 magnifierUV = uvMagnifiedArea + sampleUVOffsetFromCenter;
        outColor = SourceColor.SampleLevel(PointBorderSampler, magnifierUV, 0);
        return outColor;
    }

    // Draw the two lines tangent to both circles connecting the magnified area to its on-screen
    // inset, so the inset reads as "zooming out of" the highlighted region.
    float2 currentToCenter = uv - uvMagnifiedArea;
    float2 centerToCenterLine = uvMagnifierOnScreen - uvMagnifiedArea;

    if (ImageSize.x > ImageSize.y)
    {
        currentToCenter.x *= AspectRatio;
        centerToCenterLine.x *= AspectRatio;
    }
    else
    {
        currentToCenter.y *= AspectRatioInv;
        centerToCenterLine.y *= AspectRatioInv;
    }

    float centerToCenterLength = length(centerToCenterLine);
    centerToCenterLine /= centerToCenterLength;

    float distanceOnCenterline = dot(currentToCenter, centerToCenterLine);
    float2 projOnCenterline = distanceOnCenterline * centerToCenterLine;
    float2 perpToCenterline = currentToCenter - projOnCenterline;

    float alphaFactor = (MagnifierOnScreenR - MagnifiedAreaR) / centerToCenterLength;
    float centerToCenterPlusAlphaLength = centerToCenterLength - alphaFactor * (MagnifierOnScreenR - MagnifiedAreaR);
    distanceOnCenterline += alphaFactor * MagnifiedAreaR;

    if (distanceOnCenterline > 0 && distanceOnCenterline < centerToCenterPlusAlphaLength)
    {
        float t = distanceOnCenterline / centerToCenterPlusAlphaLength;
        float radiusAtPoint = ((1 - t) * MagnifiedAreaR + t * MagnifierOnScreenR) * sqrt(1 - alphaFactor * alphaFactor);

        if (abs(length(perpToCenterline) - radiusAtPoint) < BORDER_THICKNESS_OFFSET)
        {
            outColor.rgb = BorderColorRGBA.rgb;
        }
    }

    return outColor;
}
