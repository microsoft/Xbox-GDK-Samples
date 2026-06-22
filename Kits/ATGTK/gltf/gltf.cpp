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
// Author:  James Stanard
//

#include "pch.h"
#include "gltf.h"

#include "FindMedia.h"
#include "ReadData.h"
#include "StringUtil.h"

#include <filesystem>

using namespace glTF;

namespace
{
    void ReadFloats(json& list, float flt_array[])
    {
        uint32_t i = 0;
        for (auto& flt : list)
            flt_array[i++] = flt;
    }

    constexpr inline uint32_t floatToHalf(float f)
    {
        const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56)); // 2^-112
        union { float f; uint32_t u; } x = { 0 };
        x.f = std::min(std::max(f, 0.0f), 1.0f) * kF32toF16;
        return x.u >> 13;
    }

    uint16_t TypeToEnum(const char type[])
    {
        if (strncmp(type, "VEC", 3) == 0)
            return static_cast<uint16_t>(Accessor::kVec2 + type[3] - '2');
        else if (strncmp(type, "MAT", 3) == 0)
            return static_cast<uint16_t>(Accessor::kMat2 + type[3] - '2');
        else
            return Accessor::kScalar;
    }

    inline void ThrowIfFalse(bool cond, const char* message)
    {
        if (!cond)
        {
            throw std::exception(message);
        }
    }
}

void glTF::Asset::ProcessNodes(json& nodes)
{
    m_nodes.resize(nodes.size());

    uint32_t nodeIdx = 0;

    for (json::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        glTF::Node& node = m_nodes[nodeIdx++];
        json& thisNode = it.value();

        node.meshIdx = -1;

        if (thisNode.find("camera") != thisNode.end())
        {
            node.camera = &m_cameras[thisNode.at("camera")];
            node.properties.pointsToCamera = true;
        }
        else if (thisNode.find("mesh") != thisNode.end())
        {
            node.meshIdx = thisNode.at("mesh");
        }

        if (thisNode.find("skin") != thisNode.end())
        {
            ThrowIfFalse(node.meshIdx != -1, "node.meshIdx is invalid");
            m_meshes[static_cast<uint32_t>(node.meshIdx)].skin = thisNode.at("skin");
        }

        if (thisNode.find("children") != thisNode.end())
        {
            json& children = thisNode["children"];
            node.children.reserve(children.size());
            for (auto& child : children)
                node.children.push_back(&m_nodes[child]);
        }

        if (thisNode.find("matrix") != thisNode.end())
        {
            // TODO:  Should check for negative determinant to reverse triangle winding
            ReadFloats(thisNode["matrix"], node.matrix);
            node.properties.hasMatrix = true;
        }
        else
        {
            // TODO:  Should check scale for 1 or 3 negative values to reverse triangle winding
            json::iterator scale = thisNode.find("scale");
            if (scale != thisNode.end())
            {
                ReadFloats(scale.value(), node.srt.scale);
            }
            else
            {
                node.srt.scale[0] = 1.0f;
                node.srt.scale[1] = 1.0f;
                node.srt.scale[2] = 1.0f;
            }

            json::iterator rotation = thisNode.find("rotation");
            if (rotation != thisNode.end())
            {
                ReadFloats(rotation.value(), node.srt.rotation);
            }
            else
            {
                node.srt.rotation[0] = 0.0f;
                node.srt.rotation[1] = 0.0f;
                node.srt.rotation[2] = 0.0f;
                node.srt.rotation[3] = 1.0f;
            }

            json::iterator translation = thisNode.find("translation");
            if (translation != thisNode.end())
            {
                ReadFloats(translation.value(), node.srt.translation);
            }
            else
            {
                node.srt.translation[0] = 0.0f;
                node.srt.translation[1] = 0.0f;
                node.srt.translation[2] = 0.0f;
            }
        }
    }
}

void glTF::Asset::ProcessScenes(json& scenes)
{
    m_scenes.reserve(scenes.size());

    for (json::iterator it = scenes.begin(); it != scenes.end(); ++it)
    {
        glTF::Scene scene;
        json& thisScene = it.value();

        if (thisScene.find("nodes") != thisScene.end())
        {
            json& nodes = thisScene["nodes"];
            scene.nodes.reserve(nodes.size());
            for (auto& node : nodes)
                scene.nodes.push_back(&m_nodes[node]);
        }

        m_scenes.push_back(scene);
    }
}

void glTF::Asset::ProcessCameras(json& cameras)
{
    m_cameras.reserve(cameras.size());

    for (json::iterator it = cameras.begin(); it != cameras.end(); ++it)
    {
        glTF::Camera camera;
        json& thisCamera = it.value();

        auto nameIt = thisCamera.find("name");
        if (nameIt != thisCamera.end())
        {
            camera.name = *nameIt;
        }

        if (thisCamera["type"] == "perspective")
        {
            json& perspective = thisCamera["perspective"];
            camera.type = Camera::kPerspective;
            camera.perspective.aspectRatio = 0.0f;
            if (perspective.find("aspectRatio") != perspective.end())
                camera.perspective.aspectRatio = perspective.at("aspectRatio");
            camera.perspective.yfov = perspective["yfov"];
            camera.znear = perspective["znear"];
            camera.zfar = 0.0f;
            if (perspective.find("zfar") != perspective.end())
                camera.zfar = perspective.at("zfar");
        }
        else
        {
            camera.type = Camera::kOrthographic;
            json& orthographic = thisCamera["orthographic"];
            camera.orthographic.xmag = orthographic["xmag"];
            camera.orthographic.ymag = orthographic["ymag"];
            camera.znear = orthographic["znear"];
            camera.zfar = orthographic["zfar"];
            ThrowIfFalse(camera.zfar > camera.znear, "zfar must be greater than znear");
        }

        m_cameras.push_back(camera);
    }
}

void glTF::Asset::ProcessAccessors(json& accessors)
{
    m_accessors.reserve(accessors.size());

    for (json::iterator it = accessors.begin(); it != accessors.end(); ++it)
    {
        glTF::Accessor accessor;
        json& thisAccessor = it.value();

        glTF::BufferView& bufferView = m_bufferViews[thisAccessor.at("bufferView")];
        accessor.dataPtr = m_buffers[bufferView.buffer]->data() + bufferView.byteOffset;
        accessor.stride = bufferView.byteStride;
        if (thisAccessor.find("byteOffset") != thisAccessor.end())
            accessor.dataPtr += thisAccessor.at("byteOffset");
        accessor.count = thisAccessor.at("count");
        accessor.componentType = thisAccessor.at("componentType").get<uint16_t>() - 5120U;

        char type[8];
        strcpy_s(type, thisAccessor.at("type").get<std::string>().c_str());

        accessor.type = TypeToEnum(type);

        m_accessors.push_back(accessor);
    }
}

void glTF::Asset::FindAttribute(Primitive& prim, json& attributes, Primitive::eAttribType type, const std::string& name)
{
    json::iterator attrib = attributes.find(name);
    if (attrib != attributes.end())
    {
        prim.attribMask |= 1 << type;
        prim.attributes[type] = &m_accessors[attrib.value()];
    }
    else
    {
        prim.attributes[type] = nullptr;
    }
}

void glTF::Asset::ProcessMeshes(json& meshes, json& accessors)
{
    m_meshes.resize(meshes.size());

    uint32_t curMesh = 0;
    for (json::iterator meshIt = meshes.begin(); meshIt != meshes.end(); ++meshIt, ++curMesh)
    {
        json& thisMesh = meshIt.value();
        json& primitives = thisMesh.at("primitives");

        m_meshes[curMesh].primitives.resize(primitives.size());
        m_meshes[curMesh].skin = -1;

        uint32_t curSubMesh = 0;
        for (json::iterator primIt = primitives.begin(); primIt != primitives.end(); ++primIt, ++curSubMesh)
        {
            glTF::Primitive& prim = m_meshes[curMesh].primitives[curSubMesh];
            json& thisPrim = primIt.value();

            prim.attribMask = 0;
            json& attributes = thisPrim.at("attributes");

            FindAttribute(prim, attributes, Primitive::kPosition, "POSITION");
            FindAttribute(prim, attributes, Primitive::kNormal, "NORMAL");
            FindAttribute(prim, attributes, Primitive::kTangent, "TANGENT");
            FindAttribute(prim, attributes, Primitive::kTexcoord0, "TEXCOORD_0");
            FindAttribute(prim, attributes, Primitive::kTexcoord1, "TEXCOORD_1");
            FindAttribute(prim, attributes, Primitive::kColor0, "COLOR_0");
            FindAttribute(prim, attributes, Primitive::kJoints0, "JOINTS_0");
            FindAttribute(prim, attributes, Primitive::kWeights0, "WEIGHTS_0");

            // Read position AABB
            json& positionAccessor = accessors[attributes.at("POSITION").get<uint32_t>()];
            ReadFloats(positionAccessor.at("min"), prim.minPos);
            ReadFloats(positionAccessor.at("max"), prim.maxPos);

            prim.mode = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            prim.indices = nullptr;
            prim.materialIndex = -1;
            prim.minIndex = 0;
            prim.maxIndex = 0;
            prim.mode = 4;

            if (thisPrim.find("mode") != thisPrim.end())
                prim.mode = thisPrim.at("mode");

            if (thisPrim.find("indices") != thisPrim.end())
            {
                uint32_t accessorIndex = thisPrim.at("indices");
                json& indicesAccessor = accessors[accessorIndex];
                prim.indices = &m_accessors[accessorIndex];
                if (indicesAccessor.find("max") != indicesAccessor.end())
                    prim.maxIndex = indicesAccessor.at("max")[0];
                if (indicesAccessor.find("min") != indicesAccessor.end())
                    prim.minIndex = indicesAccessor.at("min")[0];
            }

            if (thisPrim.find("material") != thisPrim.end())
                prim.materialIndex = thisPrim.at("material");

            // TODO:  Add morph targets
            //if (thisPrim.find("targets") != thisPrim.end())
        }
    }
}

void glTF::Asset::ProcessSkins(json& skins)
{
    uint32_t skinIdx = 0;

    for (json::iterator it = skins.begin(); it != skins.end(); ++it)
    {
        glTF::Skin& skin = m_skins[skinIdx++];

        json& thisSkin = it.value();

        skin.inverseBindMatrices = nullptr;
        skin.skeleton = nullptr;

        if (thisSkin.find("inverseBindMatrices") != thisSkin.end())
            skin.inverseBindMatrices = &m_accessors[thisSkin.at("inverseBindMatrices")];

        if (thisSkin.find("skeleton") != thisSkin.end())
        {
            skin.skeleton = &m_nodes[thisSkin.at("skeleton")];
            skin.skeleton->properties.skeletonRoot = true;
        }

        json& joints = thisSkin.at("joints");
        skin.joints.reserve(joints.size());
        for (auto& joint : joints)
            skin.joints.push_back(&m_nodes[joint]);
    }
}

uint32_t glTF::Asset::ReadTextureInfo(json& info_json, glTF::Texture*& info)
{
    info = nullptr;

    if (info_json.find("index") != info_json.end())
        info = &m_textures[info_json.at("index")];

    if (info_json.find("texCoord") != info_json.end())
        return info_json.at("texCoord");
    else
        return 0;
}

void glTF::Asset::ProcessMaterials(json& materials)
{
    m_materials.reserve(materials.size());

    uint32_t materialIdx = 0;

    for (json::iterator it = materials.begin(); it != materials.end(); ++it)
    {
        glTF::Material material;
        json& thisMaterial = it.value();

        material.index = materialIdx++;
        material.flags = 0;
        material.properties.alphaCutoff = floatToHalf(0.5f);
        material.normalTextureScale = 1.0f;

        if (thisMaterial.find("alphaMode") != thisMaterial.end())
        {
            std::string alphaMode = thisMaterial.at("alphaMode");
            if (alphaMode == "BLEND")
                material.properties.alphaBlend = true;
            else if (alphaMode == "MASK")
                material.properties.alphaTest = true;
        }

        if (thisMaterial.find("alphaCutoff") != thisMaterial.end())
        {
            material.properties.alphaCutoff = floatToHalf(thisMaterial.at("alphaCutoff"));
        }

        if (thisMaterial.find("pbrMetallicRoughness") != thisMaterial.end())
        {
            json& metallicRoughness = thisMaterial.at("pbrMetallicRoughness");

            material.baseColorFactor[0] = 1.0f;
            material.baseColorFactor[1] = 1.0f;
            material.baseColorFactor[2] = 1.0f;
            material.baseColorFactor[3] = 1.0f;
            material.metallicFactor = 1.0f;
            material.roughnessFactor = 1.0f;

            for (uint32_t i = 0; i < Material::kNumTextures; ++i)
                material.textures[i] = nullptr;

            if (metallicRoughness.find("baseColorFactor") != metallicRoughness.end())
                ReadFloats(metallicRoughness.at("baseColorFactor"), material.baseColorFactor);

            if (metallicRoughness.find("metallicFactor") != metallicRoughness.end())
                material.metallicFactor = metallicRoughness.at("metallicFactor");

            if (metallicRoughness.find("roughnessFactor") != metallicRoughness.end())
                material.roughnessFactor = metallicRoughness.at("roughnessFactor");

            if (metallicRoughness.find("baseColorTexture") != metallicRoughness.end())
            {
                material.properties.baseColorUV = ReadTextureInfo(
                    metallicRoughness.at("baseColorTexture"),
                    material.textures[Material::kBaseColor]);
            }

            if (metallicRoughness.find("metallicRoughnessTexture") != metallicRoughness.end())
            {
                material.properties.metallicRoughnessUV = ReadTextureInfo(
                    metallicRoughness.at("metallicRoughnessTexture"),
                    material.textures[Material::kMetallicRoughness]);
            }
        }

        material.emissiveFactor[0] = 0.0f;
        material.emissiveFactor[1] = 0.0f;
        material.emissiveFactor[2] = 0.0f;
        material.normalTextureScale = 1.0f;

        if (thisMaterial.find("doubleSided") != thisMaterial.end())
            material.properties.twoSided = thisMaterial.at("doubleSided");

        if (thisMaterial.find("normalTextureScale") != thisMaterial.end())
            material.normalTextureScale = thisMaterial.at("normalTextureScale");

        if (thisMaterial.find("emissiveFactor") != thisMaterial.end())
            ReadFloats(thisMaterial.at("emissiveFactor"), material.emissiveFactor);

        if (thisMaterial.find("occlusionTexture") != thisMaterial.end())
        {
            material.properties.occlusionUV = ReadTextureInfo(
                thisMaterial.at("occlusionTexture"),
                material.textures[Material::kOcclusion]);
        }

        if (thisMaterial.find("emissiveTexture") != thisMaterial.end())
        {
            material.properties.emissiveUV = ReadTextureInfo(
                thisMaterial.at("emissiveTexture"),
                material.textures[Material::kEmissive]);
        }

        if (thisMaterial.find("normalTexture") != thisMaterial.end())
        {
            material.properties.normalUV = ReadTextureInfo(
                thisMaterial.at("normalTexture"),
                material.textures[Material::kNormal]);
        }

        m_materials.push_back(material);
    }
}

void glTF::Asset::ProcessBuffers(json& buffers, ByteArray chunk1bin, const wchar_t* const* searchFolders)
{
    m_buffers.reserve(buffers.size());

    for (json::iterator it = buffers.begin(); it != buffers.end(); ++it)
    {
        json& thisBuffer = it.value();

        if (thisBuffer.find("uri") != thisBuffer.end())
        {
            const std::string& uri = thisBuffer.at("uri");
            std::wstring filepath = m_basePath + std::wstring(uri.begin(), uri.end());
            wchar_t strFilePath[MAX_PATH] = {};
            DX::FindMediaFile(strFilePath, MAX_PATH, filepath.c_str(), searchFolders);
            ByteArray ba = std::make_shared<std::vector<uint8_t>>(DX::ReadData(strFilePath));
            ThrowIfFalse(ba->size() > 0, "Missing bin file");
            m_buffers.push_back(ba);
        }
        else
        {
            ThrowIfFalse(it == buffers.begin(), "Only the 1st buffer allowed to be internal");
            ThrowIfFalse(chunk1bin->size() > 0, "GLB chunk1 missing data or not a GLB file");
            m_buffers.push_back(chunk1bin);
        }
    }
}

void glTF::Asset::ProcessBufferViews(json& bufferViews)
{
    m_bufferViews.reserve(bufferViews.size());

    for (json::iterator it = bufferViews.begin(); it != bufferViews.end(); ++it)
    {
        glTF::BufferView bufferView;
        json& thisBufferView = it.value();

        bufferView.buffer = thisBufferView.at("buffer");
        bufferView.byteLength = thisBufferView.at("byteLength");
        bufferView.byteOffset = 0;
        bufferView.byteStride = 0;
        bufferView.elementArrayBuffer = false;

        if (thisBufferView.find("byteOffset") != thisBufferView.end())
            bufferView.byteOffset = thisBufferView.at("byteOffset");

        if (thisBufferView.find("byteStride") != thisBufferView.end())
            bufferView.byteStride = thisBufferView.at("byteStride");

        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_bufferview_target
        // 34962 = ARRAY_BUFFER;  34963 = ELEMENT_ARRAY_BUFFER
        if (thisBufferView.find("target") != thisBufferView.end() && thisBufferView.at("target") == 34963)
            bufferView.elementArrayBuffer = true;

        m_bufferViews.push_back(bufferView);
    }
}

void glTF::Asset::ProcessImages(json& images)
{
    m_images.resize(images.size());

    uint32_t imageIdx = 0;

    for (json::iterator it = images.begin(); it != images.end(); ++it)
    {
        json& thisImage = it.value();
        if (thisImage.find("uri") != thisImage.end())
        {
            m_images[imageIdx++].path = thisImage.at("uri").get<std::string>();
        }
        else if (thisImage.find("bufferView") != thisImage.end())
        {
            char buffer[256];
            sprintf_s(buffer, 256, "GLB image at buffer view %d with mime type %s\n", thisImage.at("bufferView").get<uint32_t>(), thisImage.at("mimeType").get<std::string>().c_str());
            OutputDebugStringA(buffer);
        }
        else
        {
            ThrowIfFalse(false, "image not found");
        }
    }
}

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_sampler_wraps
D3D12_TEXTURE_ADDRESS_MODE GLtoD3DTextureAddressMode(int32_t glWrapMode)
{
    switch (glWrapMode)
    {
    default: OutputDebugStringA("Unexpected sampler wrap mode");
        [[fallthrough]];
    case 33071: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case 33648: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case 10497: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_sampler_minfilter
D3D12_FILTER GLtoD3DTextureFilterMode( int32_t magFilter, int32_t minFilter )
{
    bool linearMag = magFilter == 9729;
    switch (minFilter)
    {
    case 9728: //nearest
    case 9984: return linearMag ? D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;//nearest_mipmap_nearest
    case 9729: //linear
    case 9987: return linearMag ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;//linear_mipmap_linear
    case 9985: return linearMag ? D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;//linear_mipmap_nearest
    case 9986: return linearMag ? D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;//nearest_mipmap_linear
    default: return linearMag ? D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;
    }
}

void glTF::Asset::ProcessSamplers(json& samplers)
{
    m_samplers.resize(samplers.size());

    uint32_t samplerIdx = 0;

    for (json::iterator it = samplers.begin(); it != samplers.end(); ++it)
    {
        json& thisSampler = it.value();

        glTF::Sampler& sampler = m_samplers[samplerIdx++];
        sampler.filter = D3D12_FILTER_ANISOTROPIC;
        sampler.wrapS = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.wrapT = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        int32_t magFilter = 9729;
        int32_t minFilter = 9987;
        if (thisSampler.find("magFilter") != thisSampler.end())
            magFilter = thisSampler.at("magFilter");
        if (thisSampler.find("minFilter") != thisSampler.end())
            minFilter = thisSampler.at("minFilter");
        sampler.filter = GLtoD3DTextureFilterMode(magFilter, minFilter);

        // But these could matter for correctness.  Though, where is border mode?
        if (thisSampler.find("wrapS") != thisSampler.end())
            sampler.wrapS = GLtoD3DTextureAddressMode(thisSampler.at("wrapS"));
        if (thisSampler.find("wrapT") != thisSampler.end())
            sampler.wrapT = GLtoD3DTextureAddressMode(thisSampler.at("wrapT"));
    }
}

void glTF::Asset::ProcessTextures(json& textures)
{
    m_textures.resize(textures.size());

    uint32_t texIdx = 0;

    for (json::iterator it = textures.begin(); it != textures.end(); ++it)
    {
        glTF::Texture& texture = m_textures[texIdx++];
        json& thisTexture = it.value();

        texture.source = nullptr;
        texture.samplerIndex = -1;
        texture.index = texIdx;

        if (thisTexture.find("source") != thisTexture.end())
            texture.source = &m_images[thisTexture.at("source")];

        if (thisTexture.find("sampler") != thisTexture.end())
            texture.samplerIndex = thisTexture.at("sampler");
    }
}

void glTF::Asset::ProcessAnimations(json& animations)
{
    m_animations.resize(animations.size());
    uint32_t animIdx = 0;

    // Process all animations
    for (json::iterator it = animations.begin(); it != animations.end(); ++it)
    {
        json& thisAnimation = it.value();
        glTF::Animation& animation = m_animations[animIdx++];

        // Process this animation's samplers
        json& samplers = thisAnimation.at("samplers");
        animation.m_samplers.resize(samplers.size());
        uint32_t samplerIdx = 0;

        for (json::iterator it2 = samplers.begin(); it2 != samplers.end(); ++it2)
        {
            json& thisSampler = it2.value();
            glTF::AnimSampler& sampler = animation.m_samplers[samplerIdx++];
            sampler.m_input = &m_accessors[thisSampler.at("input")];
            sampler.m_output = &m_accessors[thisSampler.at("output")];
            sampler.m_interpolation = AnimSampler::kLinear;
            if (thisSampler.find("interpolation") != thisSampler.end())
            {
                const std::string& interpolation = thisSampler.at("interpolation");
                if (interpolation == "LINEAR")
                    sampler.m_interpolation = AnimSampler::kLinear;
                else if (interpolation == "STEP")
                    sampler.m_interpolation = AnimSampler::kStep;
                else if (interpolation == "CATMULLROMSPLINE")
                    sampler.m_interpolation = AnimSampler::kCatmullRomSpline;
                else if (interpolation == "CUBICSPLINE")
                    sampler.m_interpolation = AnimSampler::kCubicSpline;
            }
        }

        // Process this animation's channels
        json& channels = thisAnimation.at("channels");
        animation.m_channels.resize(channels.size());
        uint32_t channelIdx = 0;

        for (json::iterator it2 = channels.begin(); it2 != channels.end(); ++it2)
        {
            json& thisChannel = it2.value();
            glTF::AnimChannel& channel = animation.m_channels[channelIdx++];
            channel.m_sampler = &animation.m_samplers[thisChannel.at("sampler")];
            json& thisTarget = thisChannel.at("target");
            channel.m_target = &m_nodes[thisTarget.at("node")];
            const std::string& path = thisTarget.at("path");
            if (path == "translation")
                channel.m_path = AnimChannel::kTranslation;
            else if (path == "rotation")
                channel.m_path = AnimChannel::kRotation;
            else if (path == "scale")
                channel.m_path = AnimChannel::kScale;
            else if (path == "weights")
                channel.m_path = AnimChannel::kWeights;
        }
    }
}

void glTF::Asset::Parse(const std::wstring& filepath, const wchar_t* const* searchFolders)
{
    // TODO:  add GLB support by extracting JSON section and BIN sections
    //https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#glb-file-format-specification

    ByteArray gltfFile;
    ByteArray chunk1Bin;

    std::wstring fileExt = DX::ToLower(std::filesystem::path(filepath).extension());

    if (fileExt == L".glb")
    {
        std::ifstream glbFile(filepath, std::ios::in | std::ios::binary);
        struct GLBHeader
        {
            char magic[4];
            uint32_t version;
            uint32_t length;
        } header;
        glbFile.read((char*)&header, sizeof(GLBHeader));
        if (strncmp(header.magic, "glTF", 4) != 0)
        {
            OutputDebugStringA("Error:  Invalid glTF binary format\n");
            return;
        }
        if (header.version != 2)
        {
            OutputDebugStringA("Error:  Only glTF 2.0 is supported\n");
            return;
        }

        uint32_t chunk0Length;
        char chunk0Type[4];
        glbFile.read((char*)&chunk0Length, 4);
        glbFile.read((char*)&chunk0Type, 4);
        if (strncmp(chunk0Type, "JSON", 4) != 0)
        {
            OutputDebugStringA("Error: Expected chunk0 to contain JSON\n");
            return;
        }
        gltfFile = std::make_shared<std::vector<uint8_t>>(chunk0Length + 1);
        glbFile.read((char*)gltfFile->data(), chunk0Length);
        (*gltfFile)[chunk0Length] = '\0';

        uint32_t chunk1Length;
        char chunk1Type[4];
        glbFile.read((char*)&chunk1Length, 4);
        glbFile.read((char*)&chunk1Type, 4);
        if (strncmp(chunk1Type, "BIN", 3) != 0)
        {
            OutputDebugStringA("Error: Expected chunk1 to contain BIN\n");
            return;
        }

        chunk1Bin = std::make_shared<std::vector<uint8_t>>(chunk1Length);
        glbFile.read((char*)chunk1Bin->data(), chunk1Length);
    }
    else
    {
        ThrowIfFalse(fileExt == L".gltf", "incorrect file extension");

        // Null terminate the string (just in case)
        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, filepath.c_str(), searchFolders);
        gltfFile = std::make_shared<std::vector<uint8_t>>(DX::ReadData(strFilePath));
        if (gltfFile->size() == 0)
        {
            wchar_t errorMessage[512] = {};
            swprintf_s(errorMessage, L"empty file: %s\n", filepath.c_str());
            OutputDebugStringW(errorMessage);
            return;
        }

        gltfFile->push_back('\0');
        chunk1Bin = std::make_shared<std::vector<uint8_t>>(0);
    }

    json root = json::parse((const char*)gltfFile->data());
    if (!root.is_object())
    {
        OutputDebugStringA("Invalid glTF file");
        return;
    }

    // Strip off file name to get root path to other related files
    m_basePath = std::filesystem::path(filepath).parent_path();

    // Parse all state

    if (root.find("buffers") != root.end())
        ProcessBuffers(root.at("buffers"), chunk1Bin, searchFolders);
    if (root.find("bufferViews") != root.end())
        ProcessBufferViews(root.at("bufferViews"));
    if (root.find("accessors") != root.end())
        ProcessAccessors(root.at("accessors"));
    if (root.find("images") != root.end())
        ProcessImages(root.at("images"));
    if (root.find("samplers") != root.end())
        ProcessSamplers(root.at("samplers"));
    if (root.find("textures") != root.end())
        ProcessTextures(root.at("textures"));
    if (root.find("materials") != root.end())
        ProcessMaterials(root.at("materials"));
    if (root.find("meshes") != root.end())
        ProcessMeshes(root.at("meshes"), root.at("accessors"));
    if (root.find("cameras") != root.end())
        ProcessCameras(root.at("cameras"));
    if (root.find("skins") != root.end())
        m_skins.resize(root.at("skins").size());
    if (root.find("nodes") != root.end())
        ProcessNodes(root.at("nodes"));
    if (root.find("skins") != root.end())
        ProcessSkins(root.at("skins"));
    if (root.find("scenes") != root.end())
        ProcessScenes(root.at("scenes"));
    if (root.find("animations") != root.end())
        ProcessAnimations(root.at("animations"));
    if (root.find("scene") != root.end())
        m_scene = &m_scenes[root.at("scene")];
}
