//------------------------------------------------------------------------------
// ShaderReflect - a command line shader reflection tool
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This sample demonstrates how you can access shader reflection information and
// how it relates to the root signature. The tool can be invoked on the command
// line using this syntax:
//
// ShaderReflect.exe <shader obj file> [root signature obj file]
//
// Shader obj (and root signature) files can be produced by the Scarlett shader
// compiler (dxc.exe). If you don't specify an explicit root signature file, the
// tool will use the embedded root signature in the shader obj file.
// 
// The tool will print the following information:
// - Shader input and output parameters
// - Shader resource bindings (and their relation to root signature parameters)
// - Constant and Texture buffer structures
//------------------------------------------------------------------------------

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

#include <Windows.h>

#include <wrl/client.h>

// CRT includes:
#include <cassert>
#include <cstdio>

#include <exception>
#include <iterator>
#include <locale>
#include <set>
#include <stdexcept>
#include <vector>

// GXDK toolkit includes:
#include <gxdk.h>

#if _GXDK_VER < 0x55F00C58 /* GDK Edition 220300 */
#error This sample requires the March 2022 GDK or later
#endif

#if USE_SCARLETT
#include <d3d12_xs.h>
#include <d3d12shader_xs.h>			// This include contains the shader reflection interface
#include <dxcapi_xs.h>              // This include contains the shader compiler API for accessing the shader reflection interface
#elif USE_XBOXONE
#include <d3d12_x.h>
#include <d3d12shader_x.h>			// This include contains the shader reflection interface
#include <dxcapi_x.h>               // This include contains the shader compiler API for accessing the shader reflection interface
#else
#error Currently no support for Windows Desktop
#endif

// ATGTK includes:
#include <ReadData.h>

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            fprintf(stderr, "**ERROR** Fatal Error with HRESULT of %08X\n", static_cast<unsigned int>(hr));
            __debugbreak();
            throw com_exception(hr);
        }
    }
} // End DX namespace

namespace
{
    const wchar_t* GetErrorDesc(HRESULT hr)
    {
        static wchar_t desc[1024] = {};

        LPWSTR errorText = nullptr;

        DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, static_cast<DWORD>(hr),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorText), 0, nullptr);

        *desc = 0;

        if (result > 0 && errorText)
        {
            swprintf_s(desc, L": %ls", errorText);

            size_t len = wcslen(desc);
            if (len >= 2)
            {
                desc[len - 2] = 0;
                desc[len - 1] = 0;
            }

            if (errorText)
                LocalFree(errorText);
        }

        return desc;
    }

    //------------------------------------------------------------------------------
    // ROOT SIGNATURE REFLECTION
    //------------------------------------------------------------------------------
    D3D12_SHADER_VISIBILITY ConvertShaderVersionTypeToShaderVisibility(D3D12_SHADER_VERSION_TYPE versionType)
    {
        switch (versionType)
        {
        case D3D12_SHVER_PIXEL_SHADER:      return D3D12_SHADER_VISIBILITY_PIXEL;
        case D3D12_SHVER_VERTEX_SHADER:     return D3D12_SHADER_VISIBILITY_VERTEX;
        case D3D12_SHVER_GEOMETRY_SHADER:   return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case D3D12_SHVER_HULL_SHADER:       return D3D12_SHADER_VISIBILITY_HULL;
        case D3D12_SHVER_DOMAIN_SHADER:     return D3D12_SHADER_VISIBILITY_DOMAIN;
        case D3D12_SHVER_COMPUTE_SHADER:    return D3D12_SHADER_VISIBILITY_ALL;
        default:
            throw std::invalid_argument("Illegal D3D12_SHADER_VERSION_TYPE");
        }
    }

    struct RootSignatureBinding
    {
        enum class BindingKind
        {
            StaticSampler,
            RootParameter
        };

        struct StaticSamplerBind
        {
            UINT                        Index;      // Index in root signature
        };

        struct RootParameterBind
        {
            D3D12_ROOT_PARAMETER_TYPE   Type;
            UINT                        Index;      // Index in root signature

            // This is only valid when RootType is D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
            UINT                        DescriptorOffset;           // Index of first descriptor in table
        };

        BindingKind     Kind;
        union
        {
            StaticSamplerBind   StaticSampler;
            RootParameterBind   RootParameter;
        };
    };

    const char* GetRootParameterTypeString(D3D12_ROOT_PARAMETER_TYPE rootParamType)
    {
        switch (rootParamType)
        {
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:    return "DESCRIPTOR_TABLE";
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:     return "ROOT_CONSTANTS";
        case D3D12_ROOT_PARAMETER_TYPE_CBV:                 return "ROOT_CBV";
        case D3D12_ROOT_PARAMETER_TYPE_SRV:                 return "ROOT_SRV";
        case D3D12_ROOT_PARAMETER_TYPE_UAV:                 return "ROOT_UAV";
        default:
            throw std::invalid_argument("Illegal D3D12_ROOT_PARAMETER_TYPE");
        }
    }

    const char* GetRootSignatureBindingString(char* strBuf, size_t strBufSize, const RootSignatureBinding& binding)
    {
        switch (binding.Kind)
        {
        case RootSignatureBinding::BindingKind::StaticSampler:
        {
            sprintf_s(strBuf, strBufSize, "ss#%u STATIC_SAMPLER",
                binding.StaticSampler.Index);
            break;
        }
        case RootSignatureBinding::BindingKind::RootParameter:
        {
            const auto& rootParam = binding.RootParameter;
            if (rootParam.Type != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                // Inline root parameter binding
                sprintf_s(strBuf, strBufSize, "rp#%u %s",
                    rootParam.Index,
                    GetRootParameterTypeString(rootParam.Type));
            }
            else
            {
                // Descriptor table binding
                sprintf_s(strBuf, strBufSize, "rp#%u %s (offset: %u)",
                    rootParam.Index,
                    GetRootParameterTypeString(rootParam.Type),
                    rootParam.DescriptorOffset);
            }
            break;
        }
        }

        return strBuf;
    }

    RootSignatureBinding GetRootSignatureBinding(
        const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc,
        D3D12_SHADER_VERSION_TYPE shaderType,
        const D3D12_SHADER_INPUT_BIND_DESC& inputBindDesc)
    {
        D3D12_SHADER_VISIBILITY wantedShaderVisibility = ConvertShaderVersionTypeToShaderVisibility(shaderType);

        bool wantsUnboundedRange = inputBindDesc.BindCount == 0;        // Shader resource is an unbounded array

        // Determine possible root parameter and range types for the resource
        D3D12_ROOT_PARAMETER_TYPE wantedRootParameterType = {};
        D3D12_DESCRIPTOR_RANGE_TYPE wantedRangeType = {};
        switch (inputBindDesc.Type)
        {
        case D3D_SIT_CBUFFER:
        {
            wantedRootParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS is also valid - see special handling below
            wantedRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            break;
        }
        case D3D_SIT_TBUFFER:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            wantedRootParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            wantedRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            break;
        }
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        case D3D_SIT_UAV_FEEDBACKTEXTURE:
        {
            wantedRootParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            wantedRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            break;
        }
        case D3D_SIT_SAMPLER:
        {
            wantedRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            break;
        }

        default:
            throw std::invalid_argument("Illegal D3D_SHADER_INPUT_TYPE");
        }

        if (wantedRangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
        {
            // Check static samplers
            for (unsigned int samplerIndex = 0; samplerIndex < rootSignatureDesc.NumStaticSamplers; samplerIndex++)
            {
                const D3D12_STATIC_SAMPLER_DESC& sampler = rootSignatureDesc.pStaticSamplers[samplerIndex];

                // Check shader visibility
                if ((sampler.ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL) &&
                    (sampler.ShaderVisibility != wantedShaderVisibility))
                {
                    continue;
                }

                // Check bind point, range and space
                if ((sampler.ShaderRegister == inputBindDesc.BindPoint) &&
                    (inputBindDesc.BindCount == 1) &&
                    (sampler.RegisterSpace == inputBindDesc.Space))
                {
                    RootSignatureBinding rv = {};
                    rv.Kind                 = RootSignatureBinding::BindingKind::StaticSampler;
                    rv.StaticSampler.Index  = samplerIndex;
                    return rv;
                }
            }

        }

        // This function does a linear search through the root signature to find
        // a root paramter or descriptor table range that contains the input bind
        // desc.
        for (unsigned int rootIndex = 0; rootIndex < rootSignatureDesc.NumParameters; rootIndex++)
        {
            const D3D12_ROOT_PARAMETER1& rootParam = rootSignatureDesc.pParameters[rootIndex];

            // Check shader visibility
            if ((rootParam.ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL) &&
                (rootParam.ShaderVisibility != wantedShaderVisibility))
            {
                continue;
            }

            if (rootParam.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                // Inline root parameter
                bool paramBindTypeMatch = false;
                UINT paramBindPoint = {};
                UINT paramBindSpace = {};
                switch (rootParam.ParameterType)
                {
                case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                {
                    // Root constants always show up as cbuffer in the shader
                    paramBindTypeMatch = wantedRootParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV;
                    paramBindPoint = rootParam.Constants.ShaderRegister;
                    paramBindSpace = rootParam.Constants.RegisterSpace;
                    break;
                }
                case D3D12_ROOT_PARAMETER_TYPE_CBV:
                case D3D12_ROOT_PARAMETER_TYPE_SRV:
                case D3D12_ROOT_PARAMETER_TYPE_UAV:
                {
                    paramBindTypeMatch = wantedRootParameterType == rootParam.ParameterType;
                    paramBindPoint = rootParam.Descriptor.ShaderRegister;
                    paramBindSpace = rootParam.Descriptor.RegisterSpace;
                    break;
                }
                default:
                    throw std::invalid_argument("Illegal D3D12_ROOT_PARAMETER_TYPE");
                }

                // Check parameter type, bind point, range and space
                if (paramBindTypeMatch &&
                    (paramBindPoint == inputBindDesc.BindPoint) &&
                    (inputBindDesc.BindCount == 1) &&
                    (paramBindSpace == inputBindDesc.Space))
                {
                    RootSignatureBinding rv = {};
                    rv.Kind                 = RootSignatureBinding::BindingKind::RootParameter;
                    rv.RootParameter.Type   = rootParam.ParameterType;
                    rv.RootParameter.Index  = rootIndex;
                    return rv;
                }
            }
            else
            {
                // Descriptor table - search through all ranges
                UINT rangeOffset = 0;
                for (UINT rangeIndex = 0; rangeIndex < rootParam.DescriptorTable.NumDescriptorRanges; rangeIndex++)
                {
                    const D3D12_DESCRIPTOR_RANGE1& range = rootParam.DescriptorTable.pDescriptorRanges[rangeIndex];

                    // D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND is a magic value that allows the
                    // offset to be automatically calculated based on the previous range's
                    // offset and count
                    if (range.OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
                    {
                        rangeOffset = range.OffsetInDescriptorsFromTableStart;
                    }

                    // Check range type and bind point, range and space
                    bool rangeUnbounded = range.NumDescriptors == UINT_MAX;                         // Range is an unbounded array
                    UINT lastRangeRegister = range.BaseShaderRegister + range.NumDescriptors - 1;
                    UINT lastInputRegister = inputBindDesc.BindPoint + inputBindDesc.BindCount - 1;
                    if ((range.RangeType == wantedRangeType) &&
                        (range.BaseShaderRegister <= inputBindDesc.BindPoint) &&
                        (rangeUnbounded || (!wantsUnboundedRange && (lastRangeRegister >= lastInputRegister))) &&
                        (range.RegisterSpace == inputBindDesc.Space))
                    {
                        RootSignatureBinding rv = {};
                        rv.Kind                 = RootSignatureBinding::BindingKind::RootParameter;
                        rv.RootParameter.Type   = rootParam.ParameterType;
                        rv.RootParameter.Index  = rootIndex;
                        rv.RootParameter.DescriptorOffset     = rangeOffset + (inputBindDesc.BindPoint - range.BaseShaderRegister);
                        return rv;
                    }
                    else
                    {
                        rangeOffset += range.NumDescriptors;
                    }
                }
            }
        }

        throw std::invalid_argument("No compatible root parameter found - root signature is incompatible with shader object");
    }

    //------------------------------------------------------------------------------
    // SHADER REFLECTION
    //------------------------------------------------------------------------------
    const char* GetShaderVersionTypeString(D3D12_SHADER_VERSION_TYPE versionType)
    {
        switch (versionType)
        {
        case D3D12_SHVER_PIXEL_SHADER:      return "PIXEL_SHADER";
        case D3D12_SHVER_VERTEX_SHADER:     return "VERTEX_SHADER";
        case D3D12_SHVER_GEOMETRY_SHADER:   return "GEOMETRY_SHADER";
        case D3D12_SHVER_HULL_SHADER:       return "HULL_SHADER";
        case D3D12_SHVER_DOMAIN_SHADER:     return "DOMAIN_SHADER";
        case D3D12_SHVER_COMPUTE_SHADER:    return "COMPUTE_SHADER";
        default:
            throw std::invalid_argument("Illegal D3D12_SHADER_VERSION_TYPE");
        }
    }

    const char* GetRegisterComponentTypeString(D3D_REGISTER_COMPONENT_TYPE componentType)
    {
        switch (componentType)
        {
        case D3D_REGISTER_COMPONENT_UNKNOWN:        return "UNKNOWN";
        case D3D_REGISTER_COMPONENT_UINT32:         return "UINT32";
        case D3D_REGISTER_COMPONENT_SINT32:         return "SINT32";
        case D3D_REGISTER_COMPONENT_FLOAT32:        return "FLOAT32";
        default:
            throw std::invalid_argument("Illegal D3D_REGISTER_COMPONENT_TYPE");
        }
    }

    const char* GetShaderInputTypeString(D3D_SHADER_INPUT_TYPE inputType)
    {
        switch (inputType)
        {
        case D3D_SIT_CBUFFER:                       return "CBUFFER";
        case D3D_SIT_TBUFFER:                       return "TBUFFER";
        case D3D_SIT_TEXTURE:                       return "TEXTURE";
        case D3D_SIT_SAMPLER:                       return "SAMPLER";
        case D3D_SIT_UAV_RWTYPED:                   return "UAV_RWTYPED";
        case D3D_SIT_STRUCTURED:                    return "STRUCTURED";
        case D3D_SIT_UAV_RWSTRUCTURED:              return "UAV_RWSTRUCTURED";
        case D3D_SIT_BYTEADDRESS:                   return "BYTEADDRESS";
        case D3D_SIT_UAV_RWBYTEADDRESS:             return "UAV_RWBYTEADDRESS";
        case D3D_SIT_UAV_APPEND_STRUCTURED:         return "UAV_APPEND_STRUCTURED";
        case D3D_SIT_UAV_CONSUME_STRUCTURED:        return "UAV_CONSUME_STRUCTURED";
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: return "UAV_RWSTRUCTURED_WITH_COUNTER";
        case D3D_SIT_RTACCELERATIONSTRUCTURE:       return "RTACCELERATIONSTRUCTURE";
        case D3D_SIT_UAV_FEEDBACKTEXTURE:           return "UAV_FEEDBACKTEXTURE";
        default:
            throw std::invalid_argument("Illegal D3D_SHADER_INPUT_TYPE");
        }
    }

    char GetShaderInputTypeBindChar(D3D_SHADER_INPUT_TYPE inputType)
    {
        switch (inputType)
        {
        case D3D_SIT_CBUFFER:                       return 'b';
        case D3D_SIT_TBUFFER:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_RTACCELERATIONSTRUCTURE:       return 't';
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        case D3D_SIT_UAV_FEEDBACKTEXTURE:           return 'u';
        case D3D_SIT_SAMPLER:                       return 's';
        default:
            throw std::invalid_argument("Illegal D3D_SHADER_INPUT_TYPE");
        }
    }

    const char* GetResourceReturnTypeString(D3D_RESOURCE_RETURN_TYPE returnType)
    {
        switch (returnType)
        {
        case D3D_RETURN_TYPE_UNORM:     return "UNORM";
        case D3D_RETURN_TYPE_SNORM:     return "SNORM";
        case D3D_RETURN_TYPE_SINT:      return "SINT";
        case D3D_RETURN_TYPE_UINT:      return "UINT";
        case D3D_RETURN_TYPE_FLOAT:     return "FLOAT";
        case D3D_RETURN_TYPE_MIXED:     return "MIXED";
        case D3D_RETURN_TYPE_DOUBLE:    return "DOUBLE";
        case D3D_RETURN_TYPE_CONTINUED: return "CONTINUED";
        default:
            throw std::invalid_argument("Illegal D3D_RESOURCE_RETURN_TYPE");
        }
    }

    const char* GetShaderVariableClassString(D3D_SHADER_VARIABLE_CLASS varClass)
    {
        switch (varClass)
        {
        case D3D_SVC_SCALAR:            return "SCALAR";
        case D3D_SVC_VECTOR:            return "VECTOR";
        case D3D_SVC_MATRIX_ROWS:       return "MATRIX_ROWS";
        case D3D_SVC_MATRIX_COLUMNS:    return "MATRIX_COLUMNS";
        case D3D_SVC_STRUCT:            return "STRUCT";
        //case D3D_SVC_OBJECT:            return "OBJECT";                  // These types are not supported for constant and texture buffers
        //case D3D_SVC_INTERFACE_CLASS:   return "INTERFACE_CLASS";
        //case D3D_SVC_INTERFACE_POINTER: return "INTERFACE_POINTER";
        default:
            throw std::invalid_argument("Illegal D3D_SHADER_VARIABLE_CLASS for constant/ texture buffer");
        }
    }

    const char* GetConstantBufferTypeString(D3D_CBUFFER_TYPE cbufferType)
    {
        switch (cbufferType)
        {
        case D3D_CT_CBUFFER:            return "CBUFFER";
        case D3D_CT_TBUFFER:            return "TBUFFER";
        case D3D_CT_INTERFACE_POINTERS: return "INTERFACE_POINTERS";
        case D3D_CT_RESOURCE_BIND_INFO: return "RESOURCE_BIND_INFO";
        default:
            throw std::invalid_argument("Illegal D3D_CBUFFER_TYPE");
        }
    }

    // Recursively reflect struct types
    void ReflectStructType(std::set<ID3D12ShaderReflectionType*>& processed, ID3D12ShaderReflectionType* structType)
    {
        if (processed.find(structType) != processed.end())
        {
            return;
        }
        processed.insert(structType);

        D3D12_SHADER_TYPE_DESC typeDesc;
        DX::ThrowIfFailed(structType->GetDesc(&typeDesc));

        printf("\nstruct %llu: %s\n\n", processed.size() - 1, typeDesc.Name);

        // Print all member information
        printf(
            "Var# | Name             | Type class     | Type name                        | Columns  | Rows     | Elements | Offset\n"
            "-----|------------------|----------------|----------------------------------|----------|----------|----------|--------\n");

        std::vector<ID3D12ShaderReflectionType*> memberStructs;
        for (unsigned int i = 0; i < typeDesc.Members; i++)
        {
            ID3D12ShaderReflectionType* memberType = structType->GetMemberTypeByIndex(i);
            D3D12_SHADER_TYPE_DESC memberTypeDesc;
            DX::ThrowIfFailed(memberType->GetDesc(&memberTypeDesc));
            printf("%4u | %-16.16s | %-14.14s | %-32.32s | %8u | %8u | %8u | %6u\n",
                i,
                structType->GetMemberTypeName(i),
                GetShaderVariableClassString(memberTypeDesc.Class),
                memberTypeDesc.Name ? memberTypeDesc.Name : "",
                memberTypeDesc.Columns,
                memberTypeDesc.Rows,
                memberTypeDesc.Elements,
                memberTypeDesc.Offset);

            if (memberTypeDesc.Class == D3D_SVC_STRUCT)
            {
                memberStructs.push_back(memberType);
            }
        }

        // Dump all member structures
        for (auto memberStruct : memberStructs)
        {
            ReflectStructType(processed, memberStruct);
        }
    }

    // Prints shader reflection information to Debug Output window
    void ReflectShader(
        const wchar_t* shaderObjFilename,
        const wchar_t* rootSignatureFilename,
        ID3D12ShaderReflection* shaderReflection,
        const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc)
    {
        // Dump reflection information to debug output
        D3D12_SHADER_DESC shaderDesc = {};
        DX::ThrowIfFailed(shaderReflection->GetDesc(&shaderDesc));
        D3D12_SHADER_VERSION_TYPE shaderType = static_cast<D3D12_SHADER_VERSION_TYPE>(D3D12_SHVER_GET_TYPE(shaderDesc.Version));

        printf(
            "\n========================================================\n"
            "Shader object file:  %ls\n"
            "Root signature file: %ls\n"
            "\n"
            "Shader type: %s %u_%u\n"
            "========================================================\n",
            shaderObjFilename,
            rootSignatureFilename ? rootSignatureFilename : L"<embedded in shader object file>",
            GetShaderVersionTypeString(shaderType),
            D3D12_SHVER_GET_MAJOR(shaderDesc.Version), D3D12_SHVER_GET_MINOR(shaderDesc.Version));

        // Input params
        if (shaderDesc.InputParameters)
        {
            printf(
                "\n----------------\n"
                "Input parameters\n"
                "----------------\n\n"
                "Param# | Semantic name    | Semantic# | Register# | SysVal | CompType | Mask | RWMask\n"
                "-------|------------------|-----------|-----------|--------|----------|------|--------\n");
            for (unsigned int i = 0; i < shaderDesc.InputParameters; i++)
            {
                D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
                DX::ThrowIfFailed(shaderReflection->GetInputParameterDesc(i, &paramDesc));

                printf("%6u | %-16.16s | %9u | %9u | %-6s | %-8s |  0x%1x |    0x%1x\n",
                    i,
                    paramDesc.SemanticName,
                    paramDesc.SemanticIndex,
                    paramDesc.Register,
                    paramDesc.SystemValueType != D3D_NAME_UNDEFINED ? "yes" : "no",
                    GetRegisterComponentTypeString(paramDesc.ComponentType),
                    paramDesc.Mask,
                    paramDesc.ReadWriteMask);
            }
        }

        // Output params
        if (shaderDesc.OutputParameters)
        {
            printf(
                "\n-----------------\n"
                "Output parameters\n"
                "-----------------\n\n"
                "Param# | Semantic name    | Semantic# | Register# | SysVal | CompType | Mask | RWMask\n"
                "-------|------------------|-----------|-----------|--------|----------|------|--------\n");
            for (unsigned int i = 0; i < shaderDesc.OutputParameters; i++)
            {
                D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
                DX::ThrowIfFailed(shaderReflection->GetOutputParameterDesc(i, &paramDesc));

                printf("%6u | %-16.16s | %9u | %9u | %-6s | %-8s |  0x%1x |    0x%1x\n",
                    i,
                    paramDesc.SemanticName,
                    paramDesc.SemanticIndex,
                    paramDesc.Register,
                    paramDesc.SystemValueType != D3D_NAME_UNDEFINED ? "true" : "false",
                    GetRegisterComponentTypeString(paramDesc.ComponentType),
                    paramDesc.Mask,
                    paramDesc.ReadWriteMask);
            }
        }

        // Resources
        if (shaderDesc.BoundResources)
        {
            char bindCountBuf[64];
            char rootSignatureBuf[64];

            printf(
                "\n-----------------\n"
                "Resource bindings\n"
                "-----------------\n\n"
                "Param# | Resource name    | Type                          | BindPoint | BindCount | Flags | ReturnType | Root signature binding\n"
                "-------|------------------|-------------------------------|-----------|-----------|-------|------------|------------------------------------------\n");
            for (unsigned int i = 0; i < shaderDesc.BoundResources; i++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                DX::ThrowIfFailed(shaderReflection->GetResourceBindingDesc(i, &bindDesc));

                if (bindDesc.BindCount == 0)
                {
                    // Unbounded range
                    sprintf_s(bindCountBuf, "%s", "unbounded");
                }
                else
                {
                    // Bounded range
                    sprintf_s(bindCountBuf, "%9u", bindDesc.BindCount);
                }

                // Find the physical binding of the resource in the root signature
                RootSignatureBinding rootBinding = GetRootSignatureBinding(rootSignatureDesc, shaderType, bindDesc);
                printf("%6u | %-16.16s | %-29.29s | %c%-4u,s%-2u | %-9.9s |  0x%02x | %-10.10s | %-40.40s\n",
                    i,
                    bindDesc.Name,
                    GetShaderInputTypeString(bindDesc.Type),
                    GetShaderInputTypeBindChar(bindDesc.Type), bindDesc.BindPoint, bindDesc.Space,
                    bindCountBuf,
                    bindDesc.uFlags,            // See D3D_SHADER_INPUT_FLAGS
                    bindDesc.Type == D3D_SIT_TEXTURE ? GetResourceReturnTypeString(bindDesc.ReturnType) : "N/A",
                    GetRootSignatureBindingString(rootSignatureBuf, std::size(rootSignatureBuf), rootBinding));              
            }
        }

        // Constant buffers
        if (shaderDesc.ConstantBuffers)
        {
            printf(
                "\n----------------\n"
                "Constant buffers\n"
                "----------------\n");
            std::vector<ID3D12ShaderReflectionType*> rootStructs;
            for (unsigned int i = 0; i < shaderDesc.ConstantBuffers; i++)
            {
                // Note: ID3D12ShaderReflectionConstantBuffer is not a COM interface
                ID3D12ShaderReflectionConstantBuffer* cbReflection = shaderReflection->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC cbDesc;
                DX::ThrowIfFailed(cbReflection->GetDesc(&cbDesc));
                printf("\n%s #%u: %s (size: %u bytes, flags: 0x%1x)\n\n",
                    GetConstantBufferTypeString(cbDesc.Type),
                    i,
                    cbDesc.Name,
                    cbDesc.Size,
                    cbDesc.uFlags);

                printf(
                    "Var# | Name             | Offset | Size   | Type class     | Type name                        | Columns  | Rows     | Elements | Offset \n"
                    "-----|------------------|--------|--------|----------------|----------------------------------|----------|----------|----------|--------\n");
                for (unsigned int j = 0; j < cbDesc.Variables; j++)
                {
                    // Neither ID3D12ShaderReflectionVariable nor ID3D12ShaderReflectionType are COM interfaces
                    ID3D12ShaderReflectionVariable* varReflection = cbReflection->GetVariableByIndex(j);
                    D3D12_SHADER_VARIABLE_DESC varDesc;
                    DX::ThrowIfFailed(varReflection->GetDesc(&varDesc));

                    ID3D12ShaderReflectionType* typeReflection = varReflection->GetType();
                    D3D12_SHADER_TYPE_DESC typeDesc;
                    DX::ThrowIfFailed(typeReflection->GetDesc(&typeDesc));

                    printf("%4u | %-16.16s | %6u | %6u | %-14.14s | %-32.32s | %8u | %8u | %8u | %6u\n",
                        j,
                        varDesc.Name,
                        varDesc.StartOffset,
                        varDesc.Size,
                        GetShaderVariableClassString(typeDesc.Class),
                        typeDesc.Name ? typeDesc.Name : "",
                        typeDesc.Columns,
                        typeDesc.Rows,
                        typeDesc.Elements,
                        typeDesc.Offset);

                    if (typeDesc.Class == D3D_SVC_STRUCT)
                    {
                        rootStructs.push_back(typeReflection);
                    }
                }
            }

            // Dump all referenced structures
            std::set<ID3D12ShaderReflectionType*> processedStructs;
            for (auto structType : rootStructs)
            {
                ReflectStructType(processedStructs, structType);
            }
        }
    }

    void PrintUsage()
    {
        printf(
            "Shader Reflection Tool for Scarlett\n"
            "\n"
            "Usage: ShaderReflect.exe <shader obj file> [root signature obj file]\n"
            "\n"
            "Input shader and root signature object files are produced by the Scarlett shader\n"
            "compiler (use dxc.exe /? to get more information).\n"
        );
    }
} // End unnamed namespace

//------------------------------------------------------------------------------
// MAIN
//------------------------------------------------------------------------------
int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Set locale for output since GetErrorDesc can get localized strings.
    std::locale::global(std::locale(""));

    // Parameter parsing
    if ((argc < 2) || (argc > 3))
    {
        PrintUsage();
        return 2;
    }

    const wchar_t* shaderObjFilename = argv[1];
    const wchar_t* rootSignatureFilename = argc == 3 ? argv[2] : nullptr;

    // Load shader object file
    std::vector<uint8_t> shaderObj;
    try
    {
        shaderObj = DX::ReadData(shaderObjFilename);
    }
    catch (...)
    {
        fprintf(stderr, "ERROR: Failed to read shader object file: %ls\n", shaderObjFilename);
        return 1;
    }

    // Note: the use of dxcompiler and the d3d12 UMD below requires loading
    // toolkit DLLs (located in %GXDKLatest%\bin\Scarlett). These DLLs are
    // deployed to the build output directory using a custom build step.

    // Create DxcUtils helper
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        fprintf(stderr, "ERROR: Failed to create DXC Utilities (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
        return 1;
    }

    // Acquire shader reflection interface
    DxcBuffer shaderObjBuffer = { shaderObj.data(), shaderObj.size(), 0 };
    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection;
    hr = dxcUtils->CreateReflection(&shaderObjBuffer, IID_PPV_ARGS(shaderReflection.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        fprintf(stderr, "ERROR: Failed to reflect shader object (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
        return 1;
    }

    // Create UMD12 device - this is required for the root signature deserializer
    Microsoft::WRL::ComPtr<ID3D12Device3> umdDevice;
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_GRAPHICS_PPV_ARGS(umdDevice.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        fprintf(stderr, "ERROR: Failed to create Xbox D3D12 PC device (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
        return 1;
    }

    // Deserialize root signature
    Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> rootSignatureDeserializer;
    if (rootSignatureFilename)
    {
        // Deserialize root signature from separate obj file
        std::vector<uint8_t> rootSignatureObj;
        try
        {
            shaderObj = DX::ReadData(rootSignatureFilename);
        }
        catch (...)
        {
            fprintf(stderr, "ERROR: Failed to read root signature object file: %ls\n", rootSignatureFilename);
            return 1;
        }

        hr = D3D12CreateVersionedRootSignatureDeserializer(
            rootSignatureObj.data(), rootSignatureObj.size(),
            IID_GRAPHICS_PPV_ARGS(rootSignatureDeserializer.ReleaseAndGetAddressOf()));
    }
    else
    {
        // Deserialize the root signature embedded in the shader object. This will fail if you use the
        // -Qstrip_rootsignature option when compiling the shader.
        hr = D3D12CreateVersionedRootSignatureDeserializer(
            shaderObj.data(), shaderObj.size(),
            IID_GRAPHICS_PPV_ARGS(rootSignatureDeserializer.ReleaseAndGetAddressOf()));
    }
    if (FAILED(hr))
    {
        fprintf(stderr, "ERROR: Failed to deserialize root signature (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
        return 1;
    }
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootSignatureDesc = nullptr;
    hr = rootSignatureDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureDesc);
    if (FAILED(hr))
    {
        fprintf(stderr, "ERROR: Failed to get root signature 1.1 (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
        return 1;
    }

    // Print reflection information
    ReflectShader(
        shaderObjFilename,
        rootSignatureFilename,
        shaderReflection.Get(),
        rootSignatureDesc->Desc_1_1);

    return 0;
}
