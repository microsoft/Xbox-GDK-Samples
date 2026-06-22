//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

[ROOT_SIGNATURE]
float4 main(
    float4 position                     : SV_POSITION,
    uint isFrontFace                    : SV_IsFrontFace,
    uint coverage                       : SV_Coverage
) : SV_Target
{
    float dummy = 0.0f;

// In PIX, you can check SPI_PS_INPUT_ENA for the number of populated VGPRs
// Each input parameter is only intended for increasing the number of VGPR initialization

#ifdef __XBOX_SCARLETT
                                // PERSP_CENTER_ENA is default barycentric parameter for interpolation
    #if VGPR_NUM == 2           // PERSP_CENTER_ENA (2 VGPRs)
    #elif VGPR_NUM == 3         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA
    dummy += position.x;        
    #elif VGPR_NUM == 4         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA
    dummy += position.x;
    dummy += position.y;
    #elif VGPR_NUM == 5         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, FRONT_FACE_ENA
    dummy += position.x;
    dummy += position.y;
    dummy += isFrontFace;
    #elif VGPR_NUM == 6         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, ANCILLARY_ENA, SAMPLE_COVERAGE_ENA
    dummy += position.x;
    dummy += position.y;
    dummy += coverage;
    #elif VGPR_NUM == 7         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, FRONT_FACE_ENA, LINEAR_CENTER_ENA (2 VGPRs)
    dummy += position.x;
    dummy += position.y;
    dummy += isFrontFace;    
    dummy += __XB_GetBarycentricCoords_Linear_Center().x;
    dummy += __XB_GetBarycentricCoords_Noperspective_Center().x;
    #elif VGPR_NUM == 8         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, ANCILLARY_ENA, SAMPLE_COVERAGE_ENA, LINEAR_CENTER_ENA (2 VGPRs)
    dummy += position.x;
    dummy += position.y;
    dummy += coverage;
    dummy += __XB_GetBarycentricCoords_Linear_Center().x;
    dummy += __XB_GetBarycentricCoords_Noperspective_Center().x;
    #elif VGPR_NUM == 9         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, FRONT_FACE_ENA, ANCILLARY_ENA, SAMPLE_COVERAGE_ENA, LINEAR_CENTER_ENA (2 VGPRs)
    dummy += position.x;
    dummy += position.y;
    dummy += isFrontFace;
    dummy += coverage;
    dummy += __XB_GetBarycentricCoords_Linear_Center().x;
    dummy += __XB_GetBarycentricCoords_Noperspective_Center().x;        
    #endif
#else
                                // PERSP_CENTER_ENA is default barycentric parameter for interpolation
    #if VGPR_NUM == 2           // PERSP_CENTER_ENA (2 VGPRs)                                
    #elif VGPR_NUM == 3         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA
    dummy += position.x;
    #elif VGPR_NUM == 4         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA
    dummy += position.x;
    dummy += position.y;
    #elif VGPR_NUM == 5         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, FRONT_FACE_ENA
    dummy += position.x;
    dummy += position.y;
    dummy += isFrontFace;
    #elif VGPR_NUM == 6         // PERSP_CENTER_ENA (2 VGPRs), POS_X_FLOAT_ENA, POS_Y_FLOAT_ENA, FRONT_FACE_ENA, SAMPLE_COVERAGE_ENA
    dummy += position.x;
    dummy += position.y;
    dummy += isFrontFace;
    dummy += coverage;
    #endif
#endif

    return float4(dummy, 1.0f, 1.0f, 1.0f);
}
