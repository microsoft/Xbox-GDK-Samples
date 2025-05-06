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

//
// This file holds all the structures/classes used to load a glTF model
//
typedef unsigned tfNodeIdx;
enum tfNodeInvalidIndex :tfNodeIdx { tfNodeInvalidIndex = UINT_MAX };

class tfAccessor
{
public:

    DirectX::XMVECTOR m_min;
    DirectX::XMVECTOR m_max;

    const void *m_data = NULL;
    unsigned m_count = 0; // not converted to unsigned yet.
    unsigned m_stride;
    unsigned m_dimension;
    unsigned m_type;
    const void *Get(unsigned i) const
    {
        if (i >= m_count || i >= tfNodeInvalidIndex)
        {
            assert(m_count > 0);
            i = m_count - 1;
        }

        return (const char*)m_data + m_stride*i;
    }

    unsigned FindClosestFloatIndex(float val) const
    {
        assert(m_count > 0);

        unsigned ini = 0;
        unsigned fin = m_count - 1;

        // relies on fin becoming 0xFFFFFFFF by subtracting from 0.
        // other functions rely on this returning -1... otherwise they might assert.
        while (ini < fin) 
        {
            unsigned mid = (ini + fin) / 2;
            float v = *(const float*)Get(mid);

            if (val < v)
                fin = mid - 1;
            else if (val > v)
                ini = mid + 1;
            else
                return mid;
        }

        return fin;
    }
};

struct tfPrimitives
{
    DirectX::XMVECTOR m_center;
    DirectX::XMVECTOR m_radius;
};

struct tfMesh
{
    std::vector<tfPrimitives> m_pPrimitives;
};

struct Transform
{
    DirectX::XMMATRIX    m_rotation = DirectX::XMMatrixIdentity();
    DirectX::XMVECTOR m_translation = DirectX::XMVectorSet(0, 0, 0, 0);
    DirectX::XMVECTOR       m_scale = DirectX::XMVectorSet(1, 1, 1, 0);

    void LookAt(DirectX::XMVECTOR source, DirectX::XMVECTOR target) { m_rotation = DirectX::XMMatrixInverse(NULL, DirectX::XMMatrixLookAtRH(source, target, DirectX::XMVectorSet(0, 1, 0, 0))); m_translation = m_rotation.r[3];  m_rotation.r[3] = DirectX::XMVectorSet(0, 0, 0, 1); }
    DirectX::XMMATRIX GetWorldMat() const { return DirectX::XMMatrixScalingFromVector(m_scale)  * m_rotation  * DirectX::XMMatrixTranslationFromVector(m_translation); }
};

struct tfNode
{
    std::vector<tfNodeIdx> m_children;

    unsigned skinIndex = tfNodeInvalidIndex;
    unsigned meshIndex = tfNodeInvalidIndex;
    unsigned channel = tfNodeInvalidIndex;
    bool bIsJoint = false;

    std::string m_name;

    Transform m_tranform;
};

struct NodeMatrixPostTransform
{
    tfNode *pN; DirectX::XMMATRIX m;
};

struct tfScene
{
    std::vector<tfNodeIdx> m_nodes;
};

struct tfSkins
{
    tfAccessor m_InverseBindMatrices;
    tfNode *m_pSkeleton = NULL;
    std::vector<tfNodeIdx> m_jointsNodeIdx;
};

class tfSampler
{
public:
    tfAccessor m_time;
    tfAccessor m_value;

    void SampleLinear(float time, float *frac, float **pCurr, float **pNext) const
    {
        unsigned curr_index = m_time.FindClosestFloatIndex(time);
        unsigned next_index = std::min(curr_index + 1, m_time.m_count - 1);

        if (curr_index == next_index)
        {
            *frac = 0;
            *pCurr = (float*)m_value.Get(curr_index);
            *pNext = (float*)m_value.Get(next_index);
            return;
        }

        float curr_time = *(float*)m_time.Get(curr_index);
        float next_time = *(float*)m_time.Get(next_index);

        *pCurr = (float*)m_value.Get(curr_index);
        *pNext = (float*)m_value.Get(next_index);
        *frac = (time - curr_time) / (next_time - curr_time);
        if (*frac < 0.0f) *frac = 0;
        assert(*frac >= 0 && *frac <= 1.0);
    }
};

class tfChannel
{
public:
    ~tfChannel()
    {
        delete m_pTranslation;
        delete m_pRotation;
        delete m_pScale;
    }

    tfSampler *m_pTranslation;
    tfSampler *m_pRotation;
    tfSampler *m_pScale;
};

struct tfAnimation
{
    float m_duration;
    std::map<unsigned, tfChannel> m_channels;
};

struct tfLight
{
    DirectX::XMVECTOR m_color;
    float m_range;
    float m_intensity = 0.0f;
    float m_innerConeAngle = 0.0f;
    float m_outerConeAngle = 0.0f;
    enum LightType { LIGHT_DIRECTIONAL, LIGHT_POINTLIGHT, LIGHT_SPOTLIGHT };

    LightType m_type = LIGHT_DIRECTIONAL;

};

struct LightInstance
{
    tfNodeIdx m_lightId = tfNodeInvalidIndex;
    tfNodeIdx m_nodeIndex = tfNodeInvalidIndex;
};

struct tfCamera
{
    enum LightType { CAMERA_PERSPECTIVE };
    float yfov, zfar, znear;

    tfNodeIdx m_nodeIndex = tfNodeInvalidIndex;
};
