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

#pragma once
// Disable warning "switch of enum X is not explicitly handled by a case label" 
#pragma warning(disable:4061)
#include "../json/json.h"
#pragma warning(default:4061)

#include "GltfStructures.h"

using json = nlohmann::json;
namespace AMDTK
{
    struct GLTFCommonTransformed
    {
        std::vector<DirectX::XMMATRIX> worldSpaceMats;     // world space matrices of each node after processing the hierarchy
        std::map<tfNodeIdx, std::vector<DirectX::XMMATRIX>> worldSpaceSkeletonMats; // skinning matrices, following the m_jointsNodeIdx order
    };

    //
    // Structures holding the per frame constant buffer data. 
    //
    struct Light
    {
        DirectX::XMMATRIX      mLightViewProj;

        float         direction[3];
        float         range;

        float         color[3];
        float         intensity;

        float         position[3];
        float         innerConeCos;

        float         outerConeCos;
        uint32_t      type;
        float         depthBias;
        uint32_t      shadowMapIndex = 0xffffffffu;
    }; 

    struct per_frame
    {
        DirectX::XMMATRIX  mCameraViewProj;
        DirectX::XMMATRIX  mInverseCameraViewProj;
        DirectX::XMVECTOR  cameraPos;
        float     iblFactor;
        float     emmisiveFactor;

        float     lodBias;

        uint32_t  lightCount;
        Light     lights[80];

    };
    struct GltfCamera {
        DirectX::XMMATRIX            m_View;
        DirectX::XMVECTOR            m_eyePos;
        float               m_near, m_far;
        float               m_yaw = 0.0f;
        float               m_pitch = 0.0f;
    };

    //
    // GLTFFile, loading and storing of gltf state
    //
    class GLTFFile
    {
        friend class GLTFResources;
    public:
        
        bool Load(const std::wstring &filename);
        void Unload();

        // misc functions
        tfNodeIdx FindMeshSkinId(const tfNodeIdx meshId) const;
        size_t GetInverseBindMatricesBufferSizeByID(size_t id) const;
        void GetBufferDetails(unsigned accessor, tfAccessor *pAccessor) const;
        void GetAttributesAccessors(const json &gltfAttributes, std::vector<char*> *pStreamNames, std::vector<tfAccessor> *pAccessors) const;
        inline void GetXYZ(float *f, DirectX::XMVECTOR v)
        {
            f[0] = DirectX::XMVectorGetX(v);
            f[1] = DirectX::XMVectorGetY(v);
            f[2] = DirectX::XMVectorGetZ(v);
        }

        // transformation and animation functions
        void SetAnimationTime(uint32_t animationIndex, float time);
        void TransformScene(unsigned sceneIndex, DirectX::XMMATRIX world);
        per_frame* UpdatePerFrameLights();
        bool GetCamera(size_t cameraIdx, GltfCamera *pCam) const;
        size_t GetNumCameras();
        tfNodeIdx AddNode(const tfNode& node);
        tfNodeIdx AddLight(const tfNode& node, const tfLight& light);

        bool UpdateLightInstanceNode(const tfNodeIdx instance, const tfNode& node);

        inline const std::wstring& GetFilePath() { return m_path; };

        per_frame& GetCommonPerFrameData() { return m_perFrameData; }

        const json& GetJson() { return j3; };

        std::vector<tfNode>* GetNodes() { return &m_nodes; };
        std::vector<tfMesh>* GetMeshes() { return &m_meshes; };

        const GLTFCommonTransformed* GetCurrentFrameTransformedData()
        {
            return m_pCurrentFrameTransformedData;
        }

        const GLTFCommonTransformed* GetPreviousFrameTransformedData()
        {
            return m_pPreviousFrameTransformedData;
        }

        void ClearLights()
        {
            m_lights.clear();
            m_lightInstances.clear();
        }

        size_t NumLights() { return m_lights.size(); };
        size_t NumLightInstances() { return m_lightInstances.size(); };
    private:
        json j3;

        std::wstring m_path;
        std::vector<tfScene> m_scenes;
        std::vector<tfMesh> m_meshes;
        std::vector<tfSkins> m_skins;
        std::vector<tfLight> m_lights;
        std::vector<LightInstance> m_lightInstances;
        std::vector<tfCamera> m_cameras;

        std::vector<tfNode> m_nodes;

        std::vector<tfAnimation> m_animations;
        std::vector<char *> m_buffersData;

        const json *m_pAccessors;
        const json *m_pBufferViews;

        std::vector<DirectX::XMMATRIX> m_animatedMats;       // object space matrices of each node after being animated
         
        // we keep the data for the last 2 frames, this is for computing motion vectors
        GLTFCommonTransformed m_transformedData[2];
        GLTFCommonTransformed *m_pCurrentFrameTransformedData;
        GLTFCommonTransformed *m_pPreviousFrameTransformedData;

        per_frame m_perFrameData;

        void InitTransformedData(); //this is called after loading the data from the GLTF
        void TransformNodes(const tfNode *pRootNode, DirectX::XMMATRIX world, const std::vector<tfNodeIdx> *pNodes, GLTFCommonTransformed *pTransformed) const;
    };
}
