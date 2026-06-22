//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once


// Old Durango declaration for D3D12XBOX_COUNTER_SET_DESC and D3D12XBOX_COUNTER_DURANGO_DATA. 
// We need these declarations in case we are running Scorpio-aware code on Durango hardware. 
// Durango cannot retrieve as many simultaneous counters as Scorpio can.
// Durango also won't fill in as many instances of each counter as Scorpio can
typedef struct D3D12XBOX_COUNTER_SET_DURANGO_DESC
{
    UINT Size;
    UINT Version;

    UINT MC_CITF_CID;
    UINT SQ_SHADER_MASK;

    UINT GRBM[2];
    UINT SRBM[2];
    UINT CPF[2];
    UINT CPG[2];
    UINT CPC[2];
    UINT CB[4];
    UINT DB[4];
    UINT SU[4];
    UINT SC[8];
    UINT SX[4];
    UINT SPI[4];
    UINT SQ[16];
    UINT TA[2];
    UINT TD[2];
    UINT TCP[4];
    UINT TCC[4];
    UINT TCA[4];
    UINT GDS[4];
    UINT VGT[4];
    UINT IA[4];
    UINT WD[4];
    UINT MC_MCB_L1TLB[4];
    UINT MC_HV_MCB_L1TLB[4];
    UINT MC_MCD_L1TLB[4];
    UINT MC_HV_MCD_L1TLB[4];
    UINT MC_L2TLB[2];
    UINT MC_HV_L2TLB[2];
    UINT MC_ARB[4];
    UINT MC_CITF[4];
    UINT MC_HUB[4];
    UINT GRN[4];
} D3D12XBOX_COUNTER_SET_DURANGO_DESC;

typedef struct D3D12XBOX_COUNTER_DURANGO_DATA
{
    UINT64 GRBM[2][1];
    UINT64 SRBM[2][1];
    UINT64 CPF[2][1];
    UINT64 CPG[2][1];
    UINT64 CPC[2][1];
    UINT64 CB[4][4];
    UINT64 DB[4][4];
    UINT64 SU[4][2];
    UINT64 SC[8][2];
    UINT64 SX[4][2];
    UINT64 SPI[4][2];
    UINT64 SQ[16][2];
    UINT64 TA[2][12];
    UINT64 TD[2][12];
    UINT64 TCP[4][12];
    UINT64 TCC[4][4];
    UINT64 TCA[4][2];
    UINT64 GDS[4][1];
    UINT64 VGT[4][2];
    UINT64 IA[4][1];
    UINT64 WD[4][1];
    UINT64 MC_MCB_L1TLB[4][1];
    UINT64 MC_HV_MCB_L1TLB[4][1];
    UINT64 MC_MCD_L1TLB[4][2];
    UINT64 MC_HV_MCD_L1TLB[4][2];
    UINT64 MC_L2TLB[2][1];
    UINT64 MC_HV_L2TLB[2][1];
    UINT64 MC_ARB[4][2];
    UINT64 MC_CITF[4][2];
    UINT64 MC_HUB[4][1];
    UINT64 GRN[4][1];
} D3D12XBOX_COUNTER_DURANGO_DATA;

typedef struct D3D12XBOX_COUNTER_MASK_DURANGO
{
    UINT64 GRBM;
    UINT64 SRBM;
    UINT64 CPF;
    UINT64 CPG;
    UINT64 CPC;
    UINT64 CB;
    UINT64 DB;
    UINT64 SU;
    UINT64 SC;
    UINT64 SX;
    UINT64 SPI;
    UINT64 SQ;
    UINT64 TA;
    UINT64 TD;
    UINT64 TCP;
    UINT64 TCC;
    UINT64 TCA;
    UINT64 GDS;
    UINT64 VGT;
    UINT64 IA;
    UINT64 WD;
    UINT64 MC_MCB_L1TLB;
    UINT64 MC_HV_MCB_L1TLB;
    UINT64 MC_MCD_L1TLB;
    UINT64 MC_HV_MCD_L1TLB;
    UINT64 MC_L2TLB;
    UINT64 MC_HV_L2TLB;
    UINT64 MC_ARB;
    UINT64 MC_CITF;
    UINT64 MC_HUB;
    UINT64 GRN;
} D3D12XBOX_COUNTER_MASK_DURANGO;

// These bitmasks answer the question: Which of the Scorpio instances correspond to the Durango instances?
// For instance, Durango's 4 CBs are Scorpio CBs #0,1,4,5, so the CB mask value is 0x00000000'00000033.
static const D3D12XBOX_COUNTER_MASK_DURANGO g_counterMaskDurango =
{
                            // Durango                          // Scorpio
    0x00000000'00000001,    // UINT64 GRBM[2][1];               // UINT64 GRBM[2][1];
    0x00000000'00000001,    // UINT64 SRBM[2][1];               // UINT64 SRBM[2][1];
    0x00000000'00000001,    // UINT64 CPF[2][1];                // UINT64 CPF[2][1];
    0x00000000'00000001,    // UINT64 CPG[2][1];                // UINT64 CPG[2][1];
    0x00000000'00000001,    // UINT64 CPC[2][1];                // UINT64 CPC[2][1];
    0x00000000'00000033,    // UINT64 CB[4][4];                 // UINT64 CB[4][8];
    0x00000000'00000033,    // UINT64 DB[4][4];                 // UINT64 DB[4][8];
    0x00000000'00000003,    // UINT64 SU[4][2];                 // UINT64 SU[4][4];
    0x00000000'00000003,    // UINT64 SC[8][2];                 // UINT64 SC[8][4];
    0x00000000'00000003,    // UINT64 SX[4][2];                 // UINT64 SX[8][4];
    0x00000000'00000003,    // UINT64 SPI[4][2];                // UINT64 SPI[4][4];
    0x00000000'00000003,    // UINT64 SQ[16][2];                // UINT64 SQ[16][4];
    0x00000000'00333333,    // UINT64 TA[2][12];                // UINT64 TA[5][40];
    0x00000000'00333333,    // UINT64 TD[2][12];                // UINT64 TD[2][40];
    0x00000000'00333333,    // UINT64 TCP[4][12];               // UINT64 TCP[8][40];
    0x00000000'00000033,    // UINT64 TCC[4][4];                // UINT64 TCC[4][8];
    0x00000000'00000003,    // UINT64 TCA[4][2];                // UINT64 TCA[4][2];
    0x00000000'00000001,    // UINT64 GDS[4][1];                // UINT64 GDS[4][1];
    0x00000000'00000003,    // UINT64 VGT[4][2];                // UINT64 VGT[4][4];
    0x00000000'00000001,    // UINT64 IA[4][1];                 // UINT64 IA[4][2];
    0x00000000'00000001,    // UINT64 WD[4][1];                 // UINT64 WD[4][1];
    0x00000000'00000001,    // UINT64 MC_MCB_L1TLB[4][1];       // UINT64 MC_MCB_L1TLB[4][1];
    0x00000000'00000001,    // UINT64 MC_HV_MCB_L1TLB[4][1];    // UINT64 MC_HV_MCB_L1TLB[4][1];
    0x00000000'00000003,    // UINT64 MC_MCD_L1TLB[4][2];       // UINT64 MC_MCD_L1TLB[4][2];
    0x00000000'00000003,    // UINT64 MC_HV_MCD_L1TLB[4][2];    // UINT64 MC_HV_MCD_L1TLB[4][2];
    0x00000000'00000001,    // UINT64 MC_L2TLB[2][1];           // UINT64 MC_L2TLB[2][1];
    0x00000000'00000001,    // UINT64 MC_HV_L2TLB[2][1];        // UINT64 MC_HV_L2TLB[2][1];
    0x00000000'00000003,    // UINT64 MC_ARB[4][2];             // UINT64 MC_ARB[4][6];
    0x00000000'00000003,    // UINT64 MC_CITF[4][2];            // UINT64 MC_CITF[4][4];
    0x00000000'00000001,    // UINT64 MC_HUB[4][1];             // UINT64 MC_HUB[4][1];
    0x00000000'00000001,    // UINT64 GRN[4][1];                // UINT64 GRN[4][1];
};
