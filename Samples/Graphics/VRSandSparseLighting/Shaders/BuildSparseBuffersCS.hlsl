//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"
#include "BuildSparseBuffersCommon.hlsli"


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]                   // each 8x8 group processes 16x16 pixels
void BuildSparseBuffersCS(uint2 Gid : SV_GroupID, uint threadIndex : SV_GroupIndex, uint2 GTid : SV_GroupThreadID)
{
    bool sparseBuffersTileSize2x2 = rootConstantCB1 ? true : false;

    // normal deferred lighting wastes processing at the edge of the screen if the render target is not
    // a multiple of the Gid.xy size, we simply have threads reading and writing harmlessly outside of the
    // surface. Here we can do better, and reduce the lighting count for pixels off render target
    // if all tiles are full rate we light every pixel and don't bother with doing an examination work
    // except that if the Gid.xy clips the render target, we may still be able to reduce the light count
    bool tileClipsRenderTarget = any((Gid.xy * 16 + 16) > renderTargetDims.xy);

    uint2 tileLocalCoord = GTid.xy * 2;
    uint2 pixelCoord = Gid.xy * 16 + tileLocalCoord;

    // each thread examines 2x2 pixels
    float depth00 = LinearDepthSRV[pixelCoord + uint2(0, 0)];
    float depth01 = LinearDepthSRV[pixelCoord + uint2(0, 1)];
    float depth10 = LinearDepthSRV[pixelCoord + uint2(1, 0)];
    float depth11 = LinearDepthSRV[pixelCoord + uint2(1, 1)];

    uint shadingRate;

    if (sparseBuffersTileSize2x2)
    {
        // shading rate per 2x2
        shadingRate = ShadingRateImage2x2_SRV[pixelCoord / 2];
    }
    else
    {
        // shading rate per 8x8
        shadingRate = ShadingRateImage8x8_SRV[pixelCoord / 8];
    }
    // Note in this demo, when depth is decompressed a negative value is used to denote clear, whereas you might want to check against 0.0 for reverseZ, or 1.0 for forwardZ
    uint clearCode;
    clearCode  = depth00 < 0.0 ? CLEAR_CODE_00 : 0;
    clearCode |= depth10 < 0.0 ? CLEAR_CODE_10 : 0;
    clearCode |= depth01 < 0.0 ? CLEAR_CODE_01 : 0;
    clearCode |= depth11 < 0.0 ? CLEAR_CODE_11 : 0;

    bool clearAll = WaveActiveCountBits(clearCode == CLEAR_CODE_ALL) == 64;
    bool highRateAll = WaveActiveCountBits(shadingRate == D3D12_SHADING_RATE_1X1) == 64;

    // build sparse lighting data
    uint pixelsToLightCount;

    [branch]
    if (clearAll || (highRateAll && (!tileClipsRenderTarget)))
    {
        // these are special cases where we don't need to output any tileLocalCoordinates
        // uses as R8, so write 255 instead of 256
        pixelsToLightCount = clearAll ? 0 : 255;
    }
    else
    {
        uint writeCode = BUILD_WRITE_CODE_LIGHT_ALL4_PIXELS;
        bool removeClearPixels = true;

        // reduced rate?
        if (shadingRate != D3D12_SHADING_RATE_1X1)
        {
            /*
                Work out which samples are identical using SV_Coverage output by PS

                Quad coverage only needs 4 bits, but here I'm using an 8bit channel
                unorm, so twice as much storage as needed.... (but no bitmask operations)

                GatherAlpha might be interest here, except you have to calculate the uv
                and this shader is VALU limited, not TEX limited

                Some points of interest. Firstly, coverage data already respects shading
                rate, if we had 1x1 rate, every coverage value would be zero. Secondly because
                I am only checking for equality, no need to unpack as an integer.

                Reading off the render target will produce a value of 0
            */ 
            float coverage00 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(0, 0)].w;
            float coverage01 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(0, 1)].w;
            float coverage10 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(1, 0)].w;
            float coverage11 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(1, 1)].w;

            uint shadingRateX = shadingRate & D3D12_SHADING_RATE_2X1;
            uint shadingRateY = shadingRate & D3D12_SHADING_RATE_1X2;

            bool equalHoriz0 = (coverage00 == coverage10);
            bool equalHoriz1 = (coverage01 == coverage11);
            bool equalVert0  = (coverage00 == coverage01);
            bool equalVert1  = (coverage10 == coverage11);

            writeCode = BuildWriteCode(removeClearPixels, clearCode, shadingRateX, shadingRateY, equalHoriz0, equalHoriz1, equalVert0, equalVert1);
        }
        if(removeClearPixels)
        {
            writeCode = RemoveClearPixels(clearCode, writeCode);
        }
        if (tileClipsRenderTarget)              // scalar branch
        {
            writeCode = RemoveOffRTPixels(writeCode, pixelCoord);
        }
        // all threads have to execute this, since we have cross lane intrinsics involved
        pixelsToLightCount = WriteSparseCoordinates(writeCode, Gid.xy, tileLocalCoord,
            Build21BitDepth(depth00),
            Build21BitDepth(depth01),
            Build21BitDepth(depth10),
            Build21BitDepth(depth11));
    }
    if (threadIndex == 0)
    {
        WriteSparseLightingCount(Gid.xy, pixelsToLightCount);
    }
}
