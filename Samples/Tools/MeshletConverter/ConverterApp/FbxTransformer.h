//--------------------------------------------------------------------------------------
// FbxTransformer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>

namespace fbxsdk
{
    class FbxScene;
}

namespace ATG
{
    // The FbxTransformer performs a global scaling & optional Z coordinate flip according to command line parameters.
    class FbxTransformer
    {
    public:
        FbxTransformer(float unitScale = 1.0f, bool flipZ = true)
            : m_unitScale(unitScale)
            , m_flipZ(flipZ)
            , m_maxConversion(false)
        { }

        void  Initialize(fbxsdk::FbxScene* pScene);

        void  TransformMatrix(DirectX::XMFLOAT4X4* pDestMatrix, const DirectX::XMFLOAT4X4* pSrcMatrix) const;
        void  TransformPosition(DirectX::XMFLOAT3* pDestPosition, const DirectX::XMFLOAT3* pSrcPosition) const;
        void  TransformDirection(DirectX::XMFLOAT3* pDestDirection, const DirectX::XMFLOAT3* pSrcDirection) const;
        float TransformLength(float fInputLength) const;

        void  SetUnitScale(const float fScale) { m_unitScale = fScale; }
        void  SetZFlip(const bool bFlip) { m_flipZ = bFlip; }

    private:
        float m_unitScale;
        bool  m_flipZ;
        bool  m_maxConversion; // Whether the scene was authorted in Autodesk Max - determined by the scene axis system.
    };
}
