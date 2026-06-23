//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):   James Stanard
//

#pragma once

#ifndef _WIN32
#define _WIN32
#endif
#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter
#include "Json.h"
#pragma warning(pop)

#include <string>

namespace glTF
{
    using json = nlohmann::json;
    typedef std::shared_ptr<std::vector<uint8_t>> ByteArray;

    struct BufferView
    {
        uint32_t buffer;
        uint32_t byteLength;
        uint32_t byteOffset;
        uint16_t byteStride;
        bool     elementArrayBuffer;
    };

    struct Accessor
    {
        enum // componentType
        {
            kByte,
            kUnsignedByte,
            kShort,
            kUnsignedShort,
            kSignedInt, // won't happen
            kUnsignedInt,
            kFloat
        };

        enum // type
        {
            kScalar,
            kVec2,
            kVec3,
            kVec4,
            kMat2,
            kMat3,
            kMat4
        };

        unsigned char* dataPtr;
        uint32_t stride;
        uint32_t count; // number of elements
        uint16_t componentType;
        uint16_t type;
    };

    struct Image
    {
        std::string path; // UTF8
    };

    struct Sampler
    {
        D3D12_FILTER filter;
        D3D12_TEXTURE_ADDRESS_MODE wrapS;
        D3D12_TEXTURE_ADDRESS_MODE wrapT;
    };

    struct Texture
    {
        Image* source;
        int32_t samplerIndex;
        uint32_t index;
    };

    struct Material
    {
        // PBR properties
        float baseColorFactor[4]; // default=[1,1,1,1]
        float metallicFactor; // default=1
        float roughnessFactor; // default=1

        union
        {
            uint32_t flags;
            struct
            {
                uint32_t baseColorUV : 1;
                uint32_t metallicRoughnessUV : 1;
                uint32_t occlusionUV : 1;
                uint32_t emissiveUV : 1;
                uint32_t normalUV : 1;
                uint32_t twoSided : 1;
                uint32_t alphaTest : 1;
                uint32_t alphaBlend : 1;
                uint32_t _pad : 8;
                uint32_t alphaCutoff : 16; // FP16
            } properties;
        };
        float emissiveFactor[3]; // default=[0,0,0]
        float normalTextureScale; // default=1
        enum : uint32_t { kBaseColor, kMetallicRoughness, kOcclusion, kEmissive, kNormal, kNumTextures };
        Texture* textures[kNumTextures];
        uint32_t index;

    };

    struct Primitive
    {
        enum eAttribType { kPosition, kNormal, kTangent, kTexcoord0, kTexcoord1, kColor0, kJoints0, kWeights0, kNumAttribs };
        Accessor* attributes[kNumAttribs];
        Accessor* indices;
        int32_t materialIndex;
        uint16_t attribMask;
        uint16_t mode; // D3D_PRIMITIVE_TOPOLOGY
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
        __declspec(align(16)) float minPos[3];
        __declspec(align(16)) float maxPos[3];
#pragma warning(pop)
        uint32_t minIndex;
        uint32_t maxIndex;
    };

    struct Mesh
    {
        std::vector<Primitive> primitives;
        int32_t skin = -1;
    };

    struct Camera
    {
        enum eType { kPerspective, kOrthographic } type = kPerspective;
        std::string name;
        union
        {
            struct
            {
                float aspectRatio;
                float yfov;
            } perspective;
            struct
            {
                float xmag;
                float ymag;
            } orthographic;
        };
        float znear = 0.1f;
        float zfar = 1000.0f;

        // Explicit default constructor to initialize union members
        Camera() : type(kPerspective), znear(0.1f), zfar(1000.0f)
        {
            perspective.aspectRatio = 1.0f;
            perspective.yfov = 1.0f;
        }
    };

    struct Node
    {
        struct
        {
            bool pointsToCamera : 1;
            bool hasMatrix : 1;
            bool skeletonRoot : 1;
        } properties = {};

        Camera* camera = nullptr;

        std::vector<Node*> children;

#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
        union
        {
            __declspec(align(16)) float matrix[16];
            struct
            {
                __declspec(align(16)) float scale[3];
                __declspec(align(16)) float rotation[4];
                __declspec(align(16)) float translation[3];
            } srt;
        };
#pragma warning(pop)

        int32_t meshIdx = -1; // Assists with mapping scene nodes to flat arrays

        // Constructor to initialize union and other members
        Node() : properties{}, camera(nullptr), meshIdx(-1)
        {
            // Initialize matrix to identity
            for (int i = 0; i < 16; ++i)
                matrix[i] = 0.0f;
            matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
        }
    };

    struct Skin
    {
        Accessor* inverseBindMatrices = nullptr;  // An array of IBMs that match the order of joint nodes
        Node* skeleton = nullptr; // root node (if non-null, this node should be in the list of joints too)
        std::vector<Node*> joints;
    };

    struct Scene
    {
        std::vector<Node*> nodes;
    };

    struct AnimSampler
    {
        Accessor* m_input;  // key frame time stamps
        Accessor* m_output; // key frame values
        enum eInterpolation { kLinear, kStep, kCatmullRomSpline, kCubicSpline };
        eInterpolation m_interpolation;
    };

    struct AnimChannel
    {
        AnimSampler* m_sampler;
        Node* m_target;
        enum ePath { kTranslation, kRotation, kScale, kWeights };
        ePath m_path;
    };

    struct Animation
    {
        std::vector<AnimChannel> m_channels;
        std::vector<AnimSampler> m_samplers;
    };

    class Asset
    {
    public:
        Asset() : m_scene(nullptr) {}
        Asset(const std::wstring& filepath, const wchar_t* const* searchFolders = nullptr) : m_scene(nullptr) { Parse(filepath, searchFolders); }
        ~Asset() { m_meshes.clear(); }

        void Parse(const std::wstring& filepath, const wchar_t* const* searchFolders);

        Scene* m_scene;
        std::wstring m_basePath;
        std::vector<Scene> m_scenes;
        std::vector<Node> m_nodes;
        std::vector<Camera> m_cameras;
        std::vector<Mesh> m_meshes;
        std::vector<Image> m_images;
        std::vector<Sampler> m_samplers;
        std::vector<Texture> m_textures;
        std::vector<Accessor> m_accessors;
        std::vector<Skin> m_skins;
        std::vector<Material> m_materials;
        std::vector<ByteArray> m_buffers;
        std::vector<BufferView> m_bufferViews;
        std::vector<Animation> m_animations;

    private:
        void ProcessBuffers(json& buffers, ByteArray chunk1bin, const wchar_t* const* searchFolders);
        void ProcessBufferViews(json& bufferViews);
        void ProcessAccessors(json& accessors);
        void ProcessMaterials(json& materials);
        void ProcessTextures(json& textures);
        void ProcessSamplers(json& samplers);
        void ProcessImages(json& images);
        void ProcessSkins(json& skins);
        void ProcessMeshes(json& meshes, json& accessors);
        void ProcessNodes(json& nodes);
        void ProcessAnimations(json& nodes);
        void ProcessCameras(json& cameras);
        void ProcessScenes(json& scenes);
        void FindAttribute(Primitive& prim, json& attributes, Primitive::eAttribType type, const std::string& name);
        uint32_t ReadTextureInfo(json& info_json, glTF::Texture*& info);
    };

} // namespace glTF
