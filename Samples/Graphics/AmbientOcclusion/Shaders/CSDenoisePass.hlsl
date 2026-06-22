///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         (see XeGTAO.h)                                                                            (='.'=)
// Details:         https://github.com/GameTechDev/XeGTAO                                                     (")_(")
//
// Version history: see XeGTAO.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CSDenoisePass.hlsli"


// Engine-specific entry point for the third pass
[RootSignature(GTAO_RootSig)]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y_WAVE32, 1)]
void main(const uint2 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixCoordBase = dispatchThreadID * uint2(2, 2);    // we're computing 4 pixels at a time (performance optimization)
    XeGTAO_Denoise(pixCoordBase, false);
}
