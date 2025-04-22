// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "pch.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#include "GLTFFile.h"
#include "GltfHelpers.h"
#include <fstream>
#include <codecvt>

using namespace DirectX;
using namespace DirectX::SimpleMath;



#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <memory>
#include <string>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#define S_ISREG(e) (((e) & _S_IFMT) == _S_IFREG)
#define S_ISDIR(e) (((e) & _S_IFMT) == _S_IFDIR)
#else
#include <unistd.h>
#define _open open
#define _close close
#define _fstat64 fstat64
#define _stat64 stat64
#define _read read
#define _O_RDONLY O_RDONLY
#define _O_NOINHERIT 0
#define _O_BINARY 0
#define _S_IREAD S_IRUSR|S_IRGRP|S_IROTH
#endif


template
<typename T>
struct FileDataTemplate
{
    FileDataTemplate() = default;
    FileDataTemplate(char* d, T l) :datum(d), len(l) {}
    char* datum{ nullptr };
    T len{ 0 };
};

typedef struct FileDataTemplate<uint64_t> FileData64;

FileData64 ReadFileAll64(const wchar_t* fileName)
{
#if defined(_WIN32) || defined(_WIN64)
    int file = -1;
    //file = _sopen(fileName, _O_RDONLY | _O_NOINHERIT | _O_BINARY | _O_SEQUENTIAL, _SH_DENYNO, _S_IREAD);
    
    (void)_wsopen_s(&file, fileName, _O_RDONLY | _O_NOINHERIT | _O_BINARY | _O_SEQUENTIAL, _SH_DENYNO, _S_IREAD);
#else
    auto file = _wopen(fileName, _O_RDONLY | _O_NOINHERIT | _O_BINARY, _S_IREAD);
#endif

    if (file == -1)
    {
        perror("unable to open file");
        return FileData64{};
    }

    struct _stat64 fileStatus;
    if (_fstat64(file, &fileStatus) != 0)
    {
        perror("unable to get size");
        (void)_close(file);
        return FileData64{};
    }

    if (!S_ISREG(fileStatus.st_mode))
    {
        perror("not a regular file");
        (void)_close(file);
        return FileData64{};
    }

    const uint64_t fileLengthBytes = static_cast<uint64_t>(std::max<int64_t>(fileStatus.st_size, 0));
    char* pFileBufferBase = (char*)malloc(fileLengthBytes);

    if (pFileBufferBase == nullptr)
    {
        perror("out of memory");
        (void)_close(file);
        return FileData64{};
    }

    char* pFileBuffer = pFileBufferBase;
    for (uint64_t bytesLeft = fileLengthBytes; bytesLeft > 0;)
    {
        uint32_t bytesRequested = static_cast<uint32_t>(std::min<uint64_t>(bytesLeft, UINT32_MAX));
        auto bytesReceivedOrStatus = _read(file, pFileBuffer, bytesRequested);
        // Break into 4GB chunks.
        if (bytesReceivedOrStatus > 0)
        {
            bytesLeft -= bytesReceivedOrStatus;
            pFileBuffer += bytesReceivedOrStatus;
        }
        else if (bytesReceivedOrStatus == 0)
        {
            // eof... don't do this again.
            break;
        }
        else if (bytesReceivedOrStatus < 0)
        {
            perror("error while reading file");
            break;
        }
    }
    (void)_close(file);
    return FileData64{ pFileBufferBase, fileLengthBytes };
}


namespace AMDTK
{
    bool GLTFFile::Load(const std::wstring &filename)
    {
        m_path = filename.substr(0, filename.find_last_of(L"\\") + 1);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

        std::ifstream f(filename);
        if (!f)
        {
            printf("The file %ws cannot be found\n", filename.c_str());
            return false;
        }

        m_perFrameData = {};

        f >> j3;

        if (j3.find("buffers") != j3.end())
        {
            const json &buffers = j3["buffers"];
            m_buffersData.reserve(buffers.size());

            for (const auto& buffer : buffers)
            { 
                // filenames need converted
                const std::wstring& name {
                    converter.from_bytes(static_cast<const std::string&>(buffer["uri"]) )
                };
                FileData64 data{ ReadFileAll64((m_path + name).c_str()) };
                m_buffersData.push_back(data.datum);
            }
        }

        // Load Meshes
        //
        m_pAccessors = &j3["accessors"];
        m_pBufferViews = &j3["bufferViews"];
        const json &meshes = j3["meshes"];
        m_meshes.resize(meshes.size());
        for (size_t i = 0; i < meshes.size(); i++)
        {
            tfMesh *tfmesh = &m_meshes[i];
            auto &primitives = meshes[i]["primitives"];
            tfmesh->m_pPrimitives.resize(primitives.size());
            for (size_t p = 0; p < primitives.size(); p++)
            {
                tfPrimitives *pPrimitive = &tfmesh->m_pPrimitives[p];

                size_t positionId = primitives[p]["attributes"]["POSITION"];
                const json &accessor = m_pAccessors->at(positionId);

                XMVECTOR max = GetVector(GetElementJsonArray(accessor, "max", { 0.0, 0.0, 0.0, 0.0 }));
                XMVECTOR min = GetVector(GetElementJsonArray(accessor, "min", { 0.0, 0.0, 0.0, 0.0 }));

                pPrimitive->m_center = XMVectorMultiply(XMVectorAdd(min, max), XMVectorReplicate(0.5f));

                pPrimitive->m_radius = XMVectorSubtract(max, pPrimitive->m_center);

                pPrimitive->m_center = XMVectorSetW(pPrimitive->m_center, 1.0f); //set the W to 1 since this is a position not a direction
            }
        }

        // Load lights
        //
        if (j3.find("extensions") != j3.end())
        {
            const json &extensions = j3["extensions"];
            if (extensions.find("KHR_lights_punctual") != extensions.end())
            {
                const json &KHR_lights_punctual = extensions["KHR_lights_punctual"];
                if (KHR_lights_punctual.find("lights") != KHR_lights_punctual.end())
                {
                    const json &lights = KHR_lights_punctual["lights"];
                    m_lights.resize(lights.size());
                    for (size_t i = 0; i < lights.size(); i++)
                    {
                        json::object_t light = lights[i];
                        m_lights[i].m_color = GetElementVector(light, "color", XMVectorSet(1, 1, 1, 0));
                        m_lights[i].m_range = GetElementFloat(light, "range", 105);
                        m_lights[i].m_intensity = GetElementFloat(light, "intensity", 1);
                        m_lights[i].m_innerConeAngle = GetElementFloat(light, "spot/innerConeAngle", 0);
                        m_lights[i].m_outerConeAngle = GetElementFloat(light, "spot/outerConeAngle", XM_PIDIV4);

                        std::string name = GetElementString(light, "type", "");
                        if (name == "spot")
                            m_lights[i].m_type = tfLight::LIGHT_SPOTLIGHT;
                        else if (name == "point")
                            m_lights[i].m_type = tfLight::LIGHT_POINTLIGHT;
                        else if (name == "directional")
                            m_lights[i].m_type = tfLight::LIGHT_DIRECTIONAL;
                    }
                }
            }
        }

        // Load cameras
        //
        if (j3.find("cameras") != j3.end())
        {
            const json &cameras = j3["cameras"];
            m_cameras.resize(cameras.size());
            for (size_t i = 0; i < cameras.size(); i++)
            {
                const json &camera = cameras[i];
                tfCamera *tfcamera = &m_cameras[i];

                tfcamera->yfov = GetElementFloat(camera, "perspective/yfov", 0.1f);
                tfcamera->znear = GetElementFloat(camera, "perspective/znear", 0.1f);
                tfcamera->zfar = GetElementFloat(camera, "perspective/zfar", 100.0f);
                tfcamera->m_nodeIndex = tfNodeInvalidIndex;
            }
        }

        // Load nodes
        //
        if (j3.find("nodes") != j3.end())
        {
            const json &nodes = j3["nodes"];
            m_nodes.resize(nodes.size());
            for (size_t i = 0; i < nodes.size(); i++)
            {
                tfNode *tfnode = &m_nodes[i];

                // Read node data
                //
                json::object_t node = nodes[i];

                if (node.find("children") != node.end())
                {
                    for (size_t c = 0; c < node["children"].size(); c++)
                    {
                        tfNodeIdx nodeID = node["children"][c];
                        tfnode->m_children.push_back(nodeID);
                    }
                }

                tfnode->meshIndex = GetElementUnsignedInt(node, "mesh", tfNodeInvalidIndex);
                tfnode->skinIndex = GetElementUnsignedInt(node, "skin", tfNodeInvalidIndex);

                tfNodeIdx cameraIdx = GetElementUnsignedInt(node, "camera", tfNodeInvalidIndex);
                if ( cameraIdx < m_cameras.size() && cameraIdx < tfNodeInvalidIndex)
                    m_cameras[(size_t)cameraIdx].m_nodeIndex = (tfNodeIdx)i; 

                tfNodeIdx lightIdx = GetElementUnsignedInt(node, "extensions/KHR_lights_punctual/light", tfNodeInvalidIndex);
                if (lightIdx < tfNodeInvalidIndex)
                {
                    m_lightInstances.push_back({ lightIdx, i > tfNodeInvalidIndex ? tfNodeInvalidIndex : (tfNodeIdx)i });
                }

                tfnode->m_tranform.m_translation = GetElementVector(node, "translation", XMVectorSet(0, 0, 0, 0));
                tfnode->m_tranform.m_scale = GetElementVector(node, "scale", XMVectorSet(1, 1, 1, 0));

                if (node.find("name") != node.end())
                    tfnode->m_name = GetElementString(node, "name", "unnamed");

                if (node.find("rotation") != node.end())
                    tfnode->m_tranform.m_rotation = XMMatrixRotationQuaternion(GetVector(node["rotation"].get<json::array_t>()));
                else if (node.find("matrix") != node.end())
                    tfnode->m_tranform.m_rotation = GetMatrix(node["matrix"].get<json::array_t>());
                else
                    tfnode->m_tranform.m_rotation = XMMatrixIdentity();
            }
        }

        // Load scenes
        //
        if (j3.find("scenes") != j3.end())
        {
            const json &scenes = j3["scenes"];
            m_scenes.resize(scenes.size());
            for (size_t i = 0; i < scenes.size(); i++)
            {
                auto scene = scenes[i];
                for (size_t n = 0; n < scene["nodes"].size(); n++)
                {
                    tfNodeIdx nodeId = scene["nodes"][n];
                    m_scenes[i].m_nodes.push_back(nodeId);
                }
            }
        }

        // Load skins
        //
        if (j3.find("skins") != j3.end())
        {
            const json &skins = j3["skins"];
            m_skins.resize(skins.size());
            for (uint32_t i = 0; i < skins.size(); i++)
            {
                GetBufferDetails(skins[i]["inverseBindMatrices"].get<unsigned>(), &m_skins[i].m_InverseBindMatrices);

                if (skins[i].find("skeleton") != skins[i].end())
                    m_skins[i].m_pSkeleton = &m_nodes[skins[i]["skeleton"]];

                const json &joints = skins[i]["joints"];
                for (uint32_t n = 0; n < joints.size(); n++)
                {
                    m_skins[i].m_jointsNodeIdx.push_back(joints[n]);
                }

            }
        }

        // Load animations
        //
        if (j3.find("animations") != j3.end())
        {
            const json &animations = j3["animations"];
            m_animations.resize(animations.size());
            for (size_t i = 0; i < animations.size(); i++)
            {
                const json &channels = animations[i]["channels"];
                const json &samplers = animations[i]["samplers"];

                tfAnimation *tfanim = &m_animations[i];
                for (size_t c = 0; c < channels.size(); c++)
                {
                    json::object_t channel = channels[c];
                    unsigned node = GetElementUnsignedInt(channel, "target/node", 0u);

                    tfChannel *tfchannel;

                    auto ch = tfanim->m_channels.find(node);
                    if (ch == tfanim->m_channels.end())
                    {
                        tfchannel = &tfanim->m_channels[node];
                    }
                    else
                    {
                        tfchannel = &ch->second;
                    }

                    tfSampler *tfsmp = new tfSampler;

                    // Get time line
                    //
                    json::size_type sampler = channel["sampler"];
                    GetBufferDetails(samplers[sampler]["input"], &tfsmp->m_time);
                    assert(tfsmp->m_time.m_stride == 4);

                    tfanim->m_duration = std::max<float>(tfanim->m_duration, *(float*)tfsmp->m_time.Get(tfsmp->m_time.m_count - 1));

                    // Get value line
                    //
                    GetBufferDetails(samplers[sampler]["output"], &tfsmp->m_value);

                    // Index appropriately
                    // 
                    std::string targetPath = GetElementString(channel, "target/path", std::string());
                    if (targetPath == "translation")
                    {
                        tfchannel->m_pTranslation = tfsmp;
                        assert(tfsmp->m_value.m_stride == 3 * 4);
                        assert(tfsmp->m_value.m_dimension == 3);
                    }
                    else if (targetPath == "rotation")
                    {
                        tfchannel->m_pRotation = tfsmp;
                        assert(tfsmp->m_value.m_stride == 4 * 4);
                        assert(tfsmp->m_value.m_dimension == 4);
                    }
                    else if (targetPath == "scale")
                    {
                        tfchannel->m_pScale = tfsmp;
                        assert(tfsmp->m_value.m_stride == 3 * 4);
                        assert(tfsmp->m_value.m_dimension == 3);
                    }
                }
            }
        }

        InitTransformedData();

        return true;
    }

    void GLTFFile::Unload()
    {
        for (const auto& buffer: m_buffersData)
        {
            free(buffer);
        }
        m_buffersData.clear();

        m_animations.clear();
        m_nodes.clear();
        m_scenes.clear();

        j3.clear();
    }

    //
    // Animates the matrices (they are still in object space)
    //
    void GLTFFile::SetAnimationTime(uint32_t animationIndex, float time)
    {
        if (animationIndex < m_animations.size())
        {
            tfAnimation *anim = &m_animations[animationIndex];

            //loop animation
            time = fmod(time, anim->m_duration);

            for (auto it = anim->m_channels.begin(); it != anim->m_channels.end(); it++)
            {
                Transform *pSourceTrans = &m_nodes[it->first].m_tranform;
                Transform animated;

                float frac, *pCurr, *pNext;

                // Animate translation
                //
                if (it->second.m_pTranslation != NULL)
                {
                    it->second.m_pTranslation->SampleLinear(time, &frac, &pCurr, &pNext);
                    animated.m_translation = XMVectorAdd(
                        XMVectorMultiply(XMVectorReplicate(1.0f - frac), XMVectorSet(pCurr[0], pCurr[1], pCurr[2], 0))
                        ,
                        XMVectorMultiply(XMVectorReplicate(frac), XMVectorSet(pNext[0], pNext[1], pNext[2], 0))
                    );

                }
                else
                {
                    animated.m_translation = pSourceTrans->m_translation;
                }

                // Animate rotation
                //
                if (it->second.m_pRotation != NULL)
                {
                    it->second.m_pRotation->SampleLinear(time, &frac, &pCurr, &pNext);
                    animated.m_rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(XMVectorSet(pCurr[0], pCurr[1], pCurr[2], pCurr[3]), XMVectorSet(pNext[0], pNext[1], pNext[2], pNext[3]), frac));
                }
                else
                {
                    animated.m_rotation = pSourceTrans->m_rotation;
                }

                // Animate scale
                //
                if (it->second.m_pScale != NULL)
                {
                    it->second.m_pScale->SampleLinear(time, &frac, &pCurr, &pNext);
                    animated.m_scale = XMVectorAdd(
                        XMVectorMultiply(XMVectorReplicate(1.0f - frac), XMVectorSet(pCurr[0], pCurr[1], pCurr[2], 0)),
                        XMVectorMultiply(XMVectorReplicate(frac), XMVectorSet(pNext[0], pNext[1], pNext[2], 0))
                    );
                }
                else
                {
                    animated.m_scale = pSourceTrans->m_scale;
                }

                m_animatedMats[it->first] = animated.GetWorldMat();
            }
        }
    }

    void GLTFFile::GetBufferDetails(unsigned accessor, tfAccessor *pAccessor) const
    {
        const json &inAccessor = m_pAccessors->at(accessor);

        unsigned bufferViewIdx = inAccessor.value("bufferView", 0u); // not required.
        const json &bufferView = m_pBufferViews->at(bufferViewIdx);

        unsigned bufferIdx = bufferView.value("buffer", 0u);
        assert(bufferIdx >= 0);

        char *buffer = m_buffersData[bufferIdx];

        unsigned offset = bufferView.value("byteOffset", 0u);

        unsigned byteLength = bufferView["byteLength"];

        unsigned byteOffset = inAccessor.value("byteOffset", 0u);
        offset += byteOffset;
        byteLength -= byteOffset;

        pAccessor->m_data = &buffer[offset];
        pAccessor->m_dimension = GetDimensions(inAccessor["type"]);
        pAccessor->m_type = GetFormatSize(inAccessor["componentType"]);
        pAccessor->m_stride = pAccessor->m_dimension * pAccessor->m_type;
        pAccessor->m_count = inAccessor["count"];
    }

    void GLTFFile::GetAttributesAccessors(const json &gltfAttributes, std::vector<char*> *pStreamNames, std::vector<tfAccessor> *pAccessors) const
    {
        for (size_t s = 0; s < pStreamNames->size(); s++)
        {
            auto attr = gltfAttributes.find(pStreamNames->at(s));
            if (attr != gltfAttributes.end())
            {
                tfAccessor accessor;
                GetBufferDetails(attr.value(), &accessor);
                pAccessors->push_back(accessor);
            }
        }
    }

    //
    // Given a mesh find the skin it belongs to
    //
    tfNodeIdx GLTFFile::FindMeshSkinId(const tfNodeIdx meshId) const
    {
        for (const auto& node: m_nodes)
        {
            if (node.meshIndex == meshId)
            {
                return node.skinIndex;
            }
        }

        return tfNodeInvalidIndex;
    }

    //
    // given a skinId return the size of the skeleton matrices (vulkan needs this to compute the offsets into the uniform buffers)
    //
    size_t GLTFFile::GetInverseBindMatricesBufferSizeByID(size_t id) const
    {
        if (id >= m_skins.size())
            return 0u;

        return m_skins[id].m_InverseBindMatrices.m_count * (4 * 4 * sizeof(float));
    }

    //
    // Transforms a node hierarchy recursively 
    //
    void GLTFFile::TransformNodes(const tfNode *pRootNode, XMMATRIX world, const std::vector<tfNodeIdx> *pNodes, GLTFCommonTransformed *pTransformed) const
    {
        pTransformed->worldSpaceMats.resize(m_nodes.size());

        for (size_t n = 0; n < pNodes->size(); n++)
        {
            tfNodeIdx nodeIdx = pNodes->at(n);

            XMMATRIX m = m_animatedMats[nodeIdx] * world;
            pTransformed->worldSpaceMats[nodeIdx] = m;

            TransformNodes(pRootNode, m, &m_nodes[nodeIdx].m_children, pTransformed);
        }
    }

    //
    // Initializes the GLTFCommonTransformed structure 
    //
    void GLTFFile::InitTransformedData()
    {
        // we need to init 2 frames, the current and the previous one, this is needed for the MVs
        for (unsigned frame = 0; frame < 2; frame++)
        {
            auto& transformData{ m_transformedData[frame] };
            // initializes matrix buffers to have the same dimension as the nodes
            transformData.worldSpaceMats.resize(m_nodes.size());

            // same thing for the skinning matrices but using the size of the InverseBindMatrices
            for (tfNodeIdx i = 0; i < m_skins.size(); i++)
            {
                const auto& skin{ m_skins[i] };
                transformData.worldSpaceSkeletonMats[i].resize(skin.m_InverseBindMatrices.m_count);
            }
        }

        m_pCurrentFrameTransformedData = &m_transformedData[0];
        m_pPreviousFrameTransformedData = &m_transformedData[1];

        // sets the animated data to the default values of the nodes
        // later on these values can be updated by the SetAnimationTime function
        m_animatedMats.resize(m_nodes.size());
        for (tfNodeIdx i = 0; i < m_nodes.size(); i++)
        {
            m_animatedMats[i] = m_nodes[i].m_tranform.GetWorldMat();
        }
    }

    //
    // Takes the animated matrices and processes the hierarchy, also computes the skinning matrix buffers. 
    //
    void GLTFFile::TransformScene(unsigned sceneIndex, XMMATRIX world)
    {
        //swap transformation buffers, we need to keep the last frame data around so we can compute the motion vectors
        GLTFCommonTransformed *pTmp = m_pCurrentFrameTransformedData;
        m_pCurrentFrameTransformedData = m_pPreviousFrameTransformedData;
        m_pPreviousFrameTransformedData = pTmp;

        // transform all the nodes of the scene
        //           
        std::vector<unsigned> sceneNodes = { m_scenes[sceneIndex].m_nodes };
        TransformNodes(m_nodes.data(), world, &sceneNodes, m_pCurrentFrameTransformedData);

        //process skeletons, takes the skinning matrices from the scene and puts them into a buffer that the vertex shader will consume
        //
        for (uint32_t i = 0; i < m_skins.size(); i++)
        {
            tfSkins &skin = m_skins[i];

            //pick the matrices that affect the skin and multiply by the inverse of the bind         
            const XMMATRIX *pM = (const XMMATRIX *)skin.m_InverseBindMatrices.m_data;
            std::vector<XMMATRIX> &skinningMats = m_pCurrentFrameTransformedData->worldSpaceSkeletonMats[i];

            for (size_t j = 0; j < skin.m_InverseBindMatrices.m_count; j++)
            {
                skinningMats[j] = XMMatrixMultiply(pM[j], m_pCurrentFrameTransformedData->worldSpaceMats[skin.m_jointsNodeIdx[j]]);
            }
        }
    }


    size_t GLTFFile::GetNumCameras()
    {
        return  m_cameras.size();
    }

    bool GLTFFile::GetCamera(size_t cameraIdx, GltfCamera *pCam) const
    {
        if (cameraIdx >= m_cameras.size())
        {
            return false;
        }

        DirectX::XMMATRIX *pMats = m_pCurrentFrameTransformedData->worldSpaceMats.data();

        const DirectX::XMMATRIX& cameraMatrix = pMats[m_cameras[cameraIdx].m_nodeIndex];


        if (pCam != nullptr)
        {
            pCam->m_eyePos = cameraMatrix.r[3];
            pCam->m_View = DirectX::XMMatrixInverse(nullptr, cameraMatrix);

            DirectX::XMFLOAT3 zBasis;
            DirectX::XMStoreFloat3(&zBasis, cameraMatrix.r[2]);

            pCam->m_yaw = atan2f(zBasis.x, zBasis.z);
            float fLen = sqrtf(zBasis.z * zBasis.z + zBasis.x * zBasis.x);
            pCam->m_pitch = atan2f(zBasis.y, fLen);

            pCam->m_near = m_cameras[cameraIdx].znear;
            pCam->m_far = m_cameras[cameraIdx].zfar;

            return true;
        }
        return false;
    }

    per_frame *GLTFFile::UpdatePerFrameLights()
    {
        XMMATRIX *pMats = m_pCurrentFrameTransformedData->worldSpaceMats.data();

        // Process lights
        m_perFrameData.lightCount = (uint32_t)m_lightInstances.size();
 
        {
            for (size_t i = 0; i < m_lightInstances.size(); i++)
            {
                Light* pSL = &m_perFrameData.lights[i];

                // get light data and node trans
                const tfLight &lightData = m_lights[m_lightInstances[i].m_lightId];
                XMMATRIX lightMat = pMats[m_lightInstances[i].m_nodeIndex];

                XMMATRIX lightView = XMMatrixInverse(nullptr, lightMat);
                if (lightData.m_type == tfLight::LIGHT_SPOTLIGHT)
                    pSL->mLightViewProj = lightView * XMMatrixPerspectiveFovRH(lightData.m_outerConeAngle * 2.0f, 1.0f, 0.10f, 100.0f);
                else if (lightData.m_type == tfLight::LIGHT_DIRECTIONAL)
                    pSL->mLightViewProj = lightView * XMMatrixOrthographicRH(30.0f, 30.0f, 0.1f, 100.0f);

                GetXYZ(pSL->direction, XMVector4Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMMatrixTranspose(lightView)));
                GetXYZ(pSL->color, lightData.m_color);
                pSL->range = lightData.m_range;
                pSL->intensity = lightData.m_intensity;
                GetXYZ(pSL->position, lightMat.r[3]);
                pSL->outerConeCos = cosf(lightData.m_outerConeAngle);
                pSL->innerConeCos = cosf(lightData.m_innerConeAngle);
                pSL->type = lightData.m_type;
                pSL->depthBias = 0.0008f;
            }
        }
        return &m_perFrameData;
    }


    tfNodeIdx GLTFFile::AddNode(const tfNode& node)
    {
        m_nodes.push_back(node);
        auto idx{ static_cast<tfNodeIdx>(m_nodes.size() - 1) };
        m_scenes[0].m_nodes.push_back(idx);
        m_animatedMats.push_back(node.m_tranform.GetWorldMat());
        return idx;
    }

    tfNodeIdx GLTFFile::AddLight(const tfNode& node, const tfLight& light)
    {
        tfNodeIdx nodeID{ AddNode(node) };
        m_lights.push_back(light);
        auto lightInstanceID{ static_cast<tfNodeIdx>(m_lights.size() - 1) };
        m_lightInstances.push_back({ lightInstanceID, nodeID });
        return lightInstanceID;
    }

    // Needs a call to UpdatePerFrameLights() after this is used
    bool GLTFFile::UpdateLightInstanceNode(const tfNodeIdx instance, const tfNode& node)
    {
        if ( instance < m_lightInstances.size())
        {
            m_nodes[m_lightInstances[instance].m_nodeIndex] = node;
            m_animatedMats[m_lightInstances[instance].m_nodeIndex] = node.m_tranform.GetWorldMat();
            return true;
        }
        return false;
    }
}
