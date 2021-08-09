//--------------------------------------------------------------------------------------
// FbxTransformer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "FbxTransformer.h"

#include <cstring>
#include <fbxsdk.h>

using namespace ATG;
using namespace DirectX;

void FbxTransformer::Initialize(FbxScene* pScene)
{
    auto SceneAxisSystem = pScene->GetGlobalSettings().GetAxisSystem();

    // convert scene to Maya Y up coordinate system
    FbxAxisSystem::MayaYUp.ConvertScene(pScene);

    int iUpAxisSign;
    auto UpVector = SceneAxisSystem.GetUpVector(iUpAxisSign);

    if (UpVector == FbxAxisSystem::eZAxis)
    {
        m_maxConversion = true;
    }
    else
    {
        m_maxConversion = false;
    }
}

void FbxTransformer::TransformMatrix(XMFLOAT4X4* pDestMatrix, const XMFLOAT4X4* pSrcMatrix) const
{
    XMFLOAT4X4 SrcMatrix;
    if (pSrcMatrix == pDestMatrix)
    {
        std::memcpy(&SrcMatrix, pSrcMatrix, sizeof(XMFLOAT4X4));
        pSrcMatrix = &SrcMatrix;
    }
    std::memcpy(pDestMatrix, pSrcMatrix, sizeof(XMFLOAT4X4));

    // What we're doing here is premultiplying by a left hand -> right hand matrix,
    // and then postmultiplying by a right hand -> left hand matrix.
    // The end result of those multiplications is that the third row and the third
    // column are negated (so element _33 is left alone).  So instead of actually
    // carrying out the multiplication, we just negate the 6 matrix elements.

    if (m_flipZ)
    {
        pDestMatrix->_13 = -pSrcMatrix->_13;
        pDestMatrix->_23 = -pSrcMatrix->_23;
        pDestMatrix->_43 = -pSrcMatrix->_43;

        pDestMatrix->_31 = -pSrcMatrix->_31;
        pDestMatrix->_32 = -pSrcMatrix->_32;
        pDestMatrix->_34 = -pSrcMatrix->_34;
    }

    // Apply the global unit scale to the translation components of the matrix.
    pDestMatrix->_41 *= m_unitScale;
    pDestMatrix->_42 *= m_unitScale;
    pDestMatrix->_43 *= m_unitScale;
}

void FbxTransformer::TransformPosition(XMFLOAT3* pDestPosition, const XMFLOAT3* pSrcPosition) const
{
    XMFLOAT3 SrcVector;
    if (pSrcPosition == pDestPosition)
    {
        SrcVector = *pSrcPosition;
        pSrcPosition = &SrcVector;
    }

    if (m_maxConversion)
    {
        pDestPosition->x = pSrcPosition->x * m_unitScale;
        pDestPosition->y = pSrcPosition->z * m_unitScale;
        pDestPosition->z = pSrcPosition->y * m_unitScale;
    }
    else
    {
        const float flipZ = m_flipZ ? -1.0f : 1.0f;

        pDestPosition->x = pSrcPosition->x * m_unitScale;
        pDestPosition->y = pSrcPosition->y * m_unitScale;
        pDestPosition->z = pSrcPosition->z * m_unitScale * flipZ;
    }
}

void FbxTransformer::TransformDirection(XMFLOAT3* pDestDirection, const XMFLOAT3* pSrcDirection) const
{
    XMFLOAT3 SrcVector;
    if (pSrcDirection == pDestDirection)
    {
        SrcVector = *pSrcDirection;
        pSrcDirection = &SrcVector;
    }

    if (m_maxConversion)
    {
        pDestDirection->x = pSrcDirection->x;
        pDestDirection->y = pSrcDirection->z;
        pDestDirection->z = pSrcDirection->y;
    }
    else
    {
        const float flipZ = m_flipZ ? -1.0f : 1.0f;

        pDestDirection->x = pSrcDirection->x;
        pDestDirection->y = pSrcDirection->y;
        pDestDirection->z = pSrcDirection->z * flipZ;
    }
}

float FbxTransformer::TransformLength(float fInputLength) const
{
    return fInputLength * m_unitScale;
}
