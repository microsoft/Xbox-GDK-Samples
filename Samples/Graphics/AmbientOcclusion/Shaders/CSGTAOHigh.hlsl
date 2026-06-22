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

#include "CSGTAO.hlsli"

// Engine-specific entry point for the second pass
[RootSignature(GTAO_RootSig)]
[numthreads(XE_GTAO_NUMTHREADS_X_MAIN, XE_GTAO_NUMTHREADS_Y_MAIN, 1)]
void main(const uint2 pixCoord : SV_DispatchThreadID)
{
    XeGTAO_MainPass(pixCoord, 3, 3);
}
