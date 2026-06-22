//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"


void ZoomCopy(uint2 DTid, RWTexture2D<float3> uav)
{
    uint zoomFactor = rootConstantCB1;

    uint2 size;
    PostProcessSRV.GetDimensions(size.x, size.y);
    uint2 corner = (size / 2) - ((size / 2) >> zoomFactor);
    uint2 srcCoord = corner + (DTid >> zoomFactor);

    uav[DTid] = PostProcessSRV[srcCoord].xyz;
}
