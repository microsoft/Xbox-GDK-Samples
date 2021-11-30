//------------------------------------------------------------------------------------
// VisibilityPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "common.hlsli"

struct Interpolants
{
    float4 position    : SV_Position;
    uint primitiveID   : SV_PrimitiveID;
};

struct Pixel
{
    uint visibilityInfo    : SV_Target;
};

ConstantBuffer<ConstantsVis> constantInfo: register(b0);

[RootSignature(MainRSVis)]
Pixel main( Interpolants In )
{
    Pixel Out;

    uint objectID12Bit = (constantInfo.objectIndex + 1) & 0xfff;
    uint primitiveID20Bit = In.primitiveID & 0xfffff;
    
    Out.visibilityInfo = (objectID12Bit << 20) | primitiveID20Bit;
    
    return Out;
}
