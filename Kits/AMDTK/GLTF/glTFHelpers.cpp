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
#include "GltfHelpers.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

unsigned GetFormatSize(unsigned id)
{
    switch (id)
    {
        case 5120u: return 1u; //(BYTE)
        case 5121u: return 1u; //(UNSIGNED_BYTE)1
        case 5122u: return 2u; //(SHORT)2
        case 5123u: return 2u; //(UNSIGNED_SHORT)2
        case 5124u: return 4u; //(SIGNED_INT)4
        case 5125u: return 4u; //(UNSIGNED_INT)4
        case 5126u: return 4u; //(FLOAT)
    }

    assert(!"unsupported format type.");
    return 0u;
}

unsigned GetDimensions(const std::string &str)
{
    if (str == "SCALAR")    return  1u;
    else if (str == "VEC2") return  2u;
    else if (str == "VEC3") return  3u;
    else if (str == "VEC4") return  4u;
    else if (str == "MAT2") return  2u*2u;
    else if (str == "MAT3") return  3u*3u;
    else if (str == "MAT4") return  4u*4u;
    else return  0;
}

void SplitGltfAttribute(std::string attribute, std::string *semanticName, uint32_t *semanticIndex)
{
    *semanticIndex = 0;

    if (isdigit(attribute.back()))
    {
        *semanticIndex = static_cast<uint32_t>(attribute.back() - '0');

        attribute.pop_back();
        if (attribute.back() == '_')
            attribute.pop_back();
    }

    *semanticName = attribute;
}

XMVECTOR GetVector(const json::array_t &accessor)
{
    if (accessor.size() == 4)
    {
        return XMVectorSet(accessor[0], accessor[1], accessor[2], accessor[3]);
    }

    return XMVectorSet(accessor[0], accessor[1], accessor[2], 0);
}

XMMATRIX GetMatrix(const json::array_t &accessor)
{
    return XMMatrixSet(accessor[0], accessor[1], accessor[2], accessor[3],
        accessor[4], accessor[5], accessor[6], accessor[7],
        accessor[8], accessor[9], accessor[10], accessor[11],
        accessor[12], accessor[13], accessor[14], accessor[15]);
}

template <class type>
type GetElement(const json::object_t *pRoot, const char *path, type pDefault)
{
    const char *p = path;
    char token[128]{ 0 };
    while (true)
    {
        for (; *p != '/' && *p != 0 && *p != '['; p++);
        ptrdiff_t len = p - path;
        if( len > 0 && len <= 128 )
        {
            memcpy(token, path, (size_t)len);
            token[len] = 0;
        }

        auto it = pRoot->find(token);
        if (it == pRoot->end())
            return pDefault;

        if (*p == '[')
        {
            p++;
            int32_t ival = atoi(p);
            size_t i = ival >= 0 ? static_cast<size_t>(ival): 0;
            for (; *p != 0 && *p != ']'; p++);
            pRoot = it->second.at(i).get_ptr<const json::object_t *>();
            p++;
        }
        else
        {
            if (it->second.is_object())
                pRoot = it->second.get_ptr<const json::object_t *>();
            else
            {
                return it->second.get<type>();
            }
        }
        p++;
        path = p;
    }

    return pDefault;
}

std::string GetElementString(const json::object_t &root, const char *path, std::string pDefault)
{
    return GetElement<std::string>(&root, path, pDefault);
}

bool GetElementBoolean(const json::object_t &root, const char *path, bool pDefault)
{
    return GetElement<bool>(&root, path, pDefault);
}

float GetElementFloat(const json::object_t &root, const char *path, float pDefault)
{
    return GetElement<float>(&root, path, pDefault);
}

int GetElementInt(const json::object_t &root, const char *path, int pDefault)
{
    return GetElement<int>(&root, path, pDefault);
}

unsigned int GetElementUnsignedInt(const json::object_t& root, const char* path, unsigned int pDefault)
{
    return GetElement<unsigned int>(&root, path, pDefault);
}

json::array_t GetElementJsonArray(const json::object_t &root, const char *path, json::array_t pDefault)
{
    return GetElement<json::array_t>(&root, path, pDefault);
}

XMVECTOR GetElementVector(json::object_t &root, const char *path, XMVECTOR pDefault)
{
    if (root.find(path) != root.end() && !root[path].is_null())
    {
        return GetVector(root[path].get<json::array_t>());
    }
    else
        return pDefault;
}


namespace AMDTK
{
    DXGI_FORMAT GetFormat(const std::string &str, int id)
    {
        if (str == "SCALAR")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC2")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8G8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC3")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_UNKNOWN; //(BYTE)
            case 5121: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_UNKNOWN; //(SHORT)2
            case 5123: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC4")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8G8B8A8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16B16A16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32A32_FLOAT; //(FLOAT)
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    void CreateSamplerForPBR(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 4;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    };

    void CreateSamplerForBrdfLut(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    };

    void CreateSamplerForShadowMap(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    };

    void CreateSamplerForShadowBuffer(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
}

