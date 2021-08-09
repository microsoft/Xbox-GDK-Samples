//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Mesh.hlsli"

[ROOT_SIGNATURE_MESH]
[maxvertexcount(3)]
void main(triangle GSSceneIn input[3], inout TriangleStream<PSSceneIn> stream)
{
    PSSceneIn output[3];

    output[0].m_position = input[0].m_position;
    output[1].m_position = input[1].m_position;
    output[2].m_position = input[2].m_position;

    float3 faceNormal = normalize(cross(input[1].m_worldPos - input[0].m_worldPos, input[2].m_worldPos - input[1].m_worldPos));

    output[0].m_normal = faceNormal;
    output[1].m_normal = faceNormal;
    output[2].m_normal = faceNormal;

    stream.Append(output[0]);
    stream.Append(output[1]);
    stream.Append(output[2]);
}

