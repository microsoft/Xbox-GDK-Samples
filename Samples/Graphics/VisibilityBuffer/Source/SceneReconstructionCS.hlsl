//--------------------------------------------------------------------------------------
// SceneReconstructionCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "common.hlsli"

// Enable dynamic indexing into descriptor heap with CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED.
// Enable dynamic indexing into sampler heap with SAMPLER_HEAP_DIRECTLY_INDEXED.
// This enables use of the ResourceDescriptorHeap[] and SamplerDescriptorHeap[] keywords in HLSL SM 6.6,
// which allows "bindless" access of resources and samplers directly from the descriptor heaps,
// without needing them to be bound in the root signature.
#define MainRS \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED)," \
    "RootConstants(num32BitConstants=21, b0)"

ConstantBuffer<ConstantElement> Constants : register(b0);

#ifdef __XBOX_ONE
float XBQuadReadAcrossX(float A)
{
    return __XB_LaneSwizzle(A, 32945);// 1000 0000 1011 0001
}

float XBQuadReadAcrossY(float A)
{
    return __XB_LaneSwizzle(A, 32846);// 1000 0000 0100 1110
}

float XBQuadReadAcrossDiagonal(float A)
{
   return __XB_LaneSwizzle(A, 32795);// 1000 0000 0001 1011
}
#endif

[numthreads(8, 8, 1)]
[RootSignature(MainRS)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Get handles to the input and output buffers using dynamic access.
    // ResourceDescriptorHeap[] is a new keyword in HLSL SM 6.6 allowing dynamic access to resources through the descriptor heap.
    Texture2D<uint> Visibility = ResourceDescriptorHeap[Descriptors::VisibilityBuffer];
    RWTexture2D<float4> OutputTexture = ResourceDescriptorHeap[Descriptors::OutputUAV + Constants.uavIndex];
    
    uint2 viDims;
    Visibility.GetDimensions(viDims.x, viDims.y);

    float2 pixelLocation = DTid.xy + float2(0.5f, 0.5f);

    // Compute NDC coordinates of this screen position.
    float nearPlaneDist = 0.1f;
    float4 projectedPositionScreen = float4((pixelLocation.x / (float) viDims.x) * 2 - 1, -(pixelLocation.y / (float) viDims.y) * 2 + 1, 0, 1) * nearPlaneDist;

    // Unproject the screen coordinates to get world-space positions on the near-plane.
    float4 unprojectedPositionScreen = mul(Constants.inverseViewProjection, projectedPositionScreen);

    // Compute camera ray direction for this pixel.
    float3 rayDir = normalize((unprojectedPositionScreen).xyz);

    // Get camera ray directions for neighboring pixels, used to compute derivatives.
    // Need to get these before any threads early-out.
#ifdef __XBOX_ONE
    float3 neighborRayDirX = float3(XBQuadReadAcrossX(rayDir.x), XBQuadReadAcrossX(rayDir.y), XBQuadReadAcrossX(rayDir.z));
    float3 neighborRayDirY = float3(XBQuadReadAcrossY(rayDir.x), XBQuadReadAcrossY(rayDir.y), XBQuadReadAcrossY(rayDir.z));
#else
    float3 neighborRayDirX = QuadReadAcrossX(rayDir);
    float3 neighborRayDirY = QuadReadAcrossY(rayDir);
#endif

    uint visibility = Visibility.Load(int3(DTid));

    // No object covering this pixel, draw clear colour.
    if (visibility == 0)
    {
        OutputTexture[DTid.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // Parse object and primitive IDs out of 32-bit uint. objectID == 0 is reserved to signify no object, so adjust down value.
    uint objectID = ((visibility >> 20) & 0xfff) - 1;
    uint primitiveID = visibility & 0xfffff;

    // Use dynamic resource access to get a handle to the array of ObjectInfo describing objects in the scene.
    StructuredBuffer<ObjectInfo> ObjectInfoArray = ResourceDescriptorHeap[Descriptors::ObjectInfoBuffer];

    // Get the object at this pixel out of the info array, must be nonuniform as a wave could cover multiple objects.
    ObjectInfo object = ObjectInfoArray[objectID];

    // Use dynamic resource access to get handles to the vertex and index buffers.
    StructuredBuffer<IndexElement> IndexBuffer   = ResourceDescriptorHeap[NonUniformResourceIndex(Descriptors::IndexBuffer + object.indexBufferID)];
    StructuredBuffer<VertexElement> VertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(Descriptors::VertexBuffer + object.vertexBufferID)];

    // Get the three indices that make up the triangle at this pixel.
    IndexElement index0 = IndexBuffer[primitiveID * 3];
    IndexElement index1 = IndexBuffer[primitiveID * 3 + 1];
    IndexElement index2 = IndexBuffer[primitiveID * 3 + 2];

    // Get the (object-space) vertices of the triangle.
    VertexElement vertex0 = VertexBuffer[index0];
    VertexElement vertex1 = VertexBuffer[index1];
    VertexElement vertex2 = VertexBuffer[index2];

    // Transform the vertices to world-space.
    float4 transformedVertex0 = mul(object.modelTransform, float4(vertex0.position, 1.0f));
    float4 transformedVertex1 = mul(object.modelTransform, float4(vertex1.position, 1.0f));
    float4 transformedVertex2 = mul(object.modelTransform, float4(vertex2.position, 1.0f));

    // Get the world-space edge vectors of the triangle.
    float4 edge1 = transformedVertex1 - transformedVertex0;
    float4 edge2 = transformedVertex2 - transformedVertex0;

    // Reconstruct the (flat-shaded) triangle normal.
    float3 triNormal = cross(edge2.xyz, edge1.xyz);

    // Determine intersection of camera ray with triangle in world-space.
    float hitT;
    RayPlaneIntersection(hitT, Constants.cameraPosition, rayDir, transformedVertex0.xyz, triNormal);

    float3 hitPoint = Constants.cameraPosition + rayDir * hitT;

    // Reconstruct barycentrics of hit point in triangle.
    float3 baryCentrics = GetBarycentricsFromPlanePoint(hitPoint, transformedVertex0.xyz, transformedVertex1.xyz, transformedVertex2.xyz);

    // Interpolate UV values using barycentrics.
    float2 uvs = baryCentrics.x * vertex0.uv.xy + baryCentrics.y * vertex1.uv.xy + baryCentrics.z * vertex2.uv.xy;
    
    float2 dUVdx;
    float2 dUVdy;

    // If every thread hit the same triangle, then normal quad derivatives will give the correct result.
    if (WaveActiveAllEqual(visibility) && WaveActiveCountBits(true) == WaveGetLaneCount())
    {
        dUVdx = ddx(uvs);
        dUVdy = ddy(uvs);
    }
    else // Otherwise need to account for situations where neighboring pixels hit different triangles or objects, and reconstruct derivatives manually.
    {
        // Compute hitpoints for neighboring camera rays, against same triangle.
        float hitTX;
        RayPlaneIntersection(hitTX, Constants.cameraPosition, neighborRayDirX, transformedVertex0.xyz, triNormal);

        float hitTY;
        RayPlaneIntersection(hitTY, Constants.cameraPosition, neighborRayDirY, transformedVertex0.xyz, triNormal);

        float3 hitPointX = Constants.cameraPosition + neighborRayDirX * hitTX;
        float3 hitPointY = Constants.cameraPosition + neighborRayDirY * hitTY;

        // Determine (potentially extrapolated) UVs at neighboring camera rays, if they were to have hit the same triangle (or in the same plane as the triangle).
        float3 baryCentricsX = GetBarycentricsFromPlanePoint(hitPointX, transformedVertex0.xyz, transformedVertex1.xyz, transformedVertex2.xyz);
        float3 baryCentricsY = GetBarycentricsFromPlanePoint(hitPointY, transformedVertex0.xyz, transformedVertex1.xyz, transformedVertex2.xyz);

        float2 uvsX = baryCentricsX.x * vertex0.uv.xy + baryCentricsX.y * vertex1.uv.xy + baryCentricsX.z * vertex2.uv.xy;
        float2 uvsY = baryCentricsY.x * vertex0.uv.xy + baryCentricsY.y * vertex1.uv.xy + baryCentricsY.z * vertex2.uv.xy;

        // Compute screen space derivatives of UVs.
        dUVdx = uvsX - uvs;
        dUVdy = uvsY - uvs;
    }
    
    // Get handles to the texture and sampler for this object using dynamic access. Must be nonuniform as wave could cover multiple objects.
    Texture2D<float4> txDiffuse = ResourceDescriptorHeap[NonUniformResourceIndex(Descriptors::DragonTexture + object.textureIDs[0])];

    // SamplerDescriptorHeap[] is a new keyword in HLSL SM 6.6 allowing dynamic access to samplers through the descriptor heap.
    SamplerState sampleState = SamplerDescriptorHeap[NonUniformResourceIndex(Samplers::LinearSampler + object.materialID)];
            
    // Sample texture using reconstructed UVs and derivatives.
    float4 texColour = txDiffuse.SampleGrad(sampleState, uvs, dUVdx, dUVdx);

    for (int i = 0; i < 15; i++)
    {
        texColour.a += txDiffuse.SampleGrad(sampleState, uvs + float2(i/15.0, i/15.0), dUVdx, dUVdx).r;
    }

    // MeshletID is passed through primitiveID field in Mesh Shader.
    if (Constants.drawOverlay == OverlayModes::PrimitiveID || Constants.drawOverlay == OverlayModes::MeshletID)
    {
        uint bottom3Bits = (primitiveID & 0x7);
        uint middle3Bits = (primitiveID & 0x38);
        uint top3Bits    = (primitiveID & 0x1c0);
        
        texColour.r =  bottom3Bits / 7.0f;
        texColour.g =  middle3Bits / 7.0f;
        texColour.b =  top3Bits / 7.0f;
    }
    else if (Constants.drawOverlay == OverlayModes::ObjectID)
    {
        uint bottom3Bits = (objectID & 0x3);
        uint middle3Bits = (objectID & 0xc);
        uint top3Bits    = (objectID & 0x30);
        
        texColour.r =  bottom3Bits / 3.0f;
        texColour.g =  middle3Bits / 3.0f;
        texColour.b =  top3Bits / 3.0f;
    }

    OutputTexture[DTid.xy] = float4(texColour);
}
