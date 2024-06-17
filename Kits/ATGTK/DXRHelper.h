//--------------------------------------------------------------------------------------
// DXRHelper.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

#pragma warning(push)
#pragma warning(disable : 4062 4316 4324) // We don't care that ShaderRecord (or derived types) are potentially unaligned or padded. They won't be in the Upload Heap

#include <iomanip>
#include <sstream>
#include <string>

__declspec(align(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT))
struct ShaderRecord
{
    ShaderRecord() : shaderIdentifier{}
    {
    }

    void Initialize(ID3D12StateObjectProperties* props, LPCWSTR exportName)
    {
        const void* identifier = props->GetShaderIdentifier(exportName);
        memcpy_s(shaderIdentifier, sizeof(shaderIdentifier), identifier, sizeof(shaderIdentifier));
    }

    char shaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};


template<class T, int NumRayGen, int NumMissShader, int NumHitGroup>
class ShaderBindingTable
{
    // Strictly we could get away with each record being D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT (32) bytes instead of 64.
    // But I've chosen to pack all shader tables into a single buffer and rather than pad out parts of the binding table that
    // have an odd number of records (e.g. 3 ray gen shader records of 32 bytes each == 96 bytes), I'll just enforce 64 bytes per record
    // to ensure the first record in every table is aligned to 64 bytes.
    static_assert(sizeof(T) % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 0, "Shader record must be a multiple of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT bytes");

public:
    ShaderBindingTable<T, NumRayGen, NumMissShader, NumHitGroup>()
    {
        m_rayGenRecords = new T[NumRayGen];
        m_missShaderRecords = new T[NumMissShader];
        m_hitGroupRecords = new T[NumHitGroup];
    }

    ~ShaderBindingTable()
    {
        delete[] m_rayGenRecords;
        delete[] m_missShaderRecords;
        delete[] m_hitGroupRecords;
    }

    ShaderBindingTable(ShaderBindingTable&&) = default;
    ShaderBindingTable& operator= (ShaderBindingTable&&) = default;

    ShaderBindingTable(ShaderBindingTable const&) = delete;
    ShaderBindingTable& operator=(ShaderBindingTable const&) = delete;

    void SetRayGenRecord(int index, T& record)
    {
        assert(index < NumRayGen);
        m_rayGenRecords[index] = record;
    }

    void SetMissShaderRecord(int index, T& record)
    {
        assert(index < NumMissShader);
        m_missShaderRecords[index] = record;
    }

    void SetHitGroupRecord(int index, T& record)
    {
        assert(index < NumHitGroup);
        m_hitGroupRecords[index] = record;
    }

    void Commit()
    {
        m_shaderBindingTableBuffer = DirectX::GraphicsMemory::Get().Allocate(
            sizeof(T) * (NumRayGen + NumMissShader + NumHitGroup), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        auto dest = static_cast<T*>(m_shaderBindingTableBuffer.Memory());
        memcpy_s(dest, sizeof(T) * NumRayGen, m_rayGenRecords, sizeof(T) * NumRayGen);
        dest += NumRayGen;
        memcpy_s(dest, sizeof(T) * NumMissShader, m_missShaderRecords, sizeof(T) * NumMissShader);
        dest += NumMissShader;
        memcpy_s(dest, sizeof(T) * NumHitGroup, m_hitGroupRecords, sizeof(T) * NumHitGroup);
    }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetRayGenerationRecord(int rayGenRecordIndex)
    {
        assert(rayGenRecordIndex < NumRayGen);

        D3D12_GPU_VIRTUAL_ADDRESS_RANGE record;
        record.StartAddress = m_shaderBindingTableBuffer.GpuAddress() + rayGenRecordIndex * sizeof(T);
        record.SizeInBytes = sizeof(T);
        return record;
    }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissShaderTable()
    {
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE record;
        record.StartAddress = m_shaderBindingTableBuffer.GpuAddress() + GetFirstMissShaderOffsetInRecords() * sizeof(T);
        record.SizeInBytes = sizeof(T) * NumMissShader;
        record.StrideInBytes = sizeof(T);
        return record;
    }

    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupShaderTable()
    {
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE record;
        record.StartAddress = m_shaderBindingTableBuffer.GpuAddress() + GetFirstHitGroupOffsetInRecords() * sizeof(T);
        record.SizeInBytes = sizeof(T) * NumHitGroup;
        record.StrideInBytes = sizeof(T);
        return record;
    }

    void Reset()
    {
        m_shaderBindingTableBuffer.Reset();
    }

private:
    UINT64 GetFirstMissShaderOffsetInRecords()
    {
        return NumRayGen;
    }

    UINT64 GetFirstHitGroupOffsetInRecords()
    {
        return NumRayGen + NumMissShader;
    }

    UINT64 GetShaderBindingTableSizeInBytes()
    {
        return (NumRayGen + NumMissShader + NumHitGroup) * sizeof(T);
    }

    DirectX::GraphicsResource m_shaderBindingTableBuffer;

    T* m_rayGenRecords;
    T* m_missShaderRecords;
    T* m_hitGroupRecords;
};

// Pretty-print a state object tree.
inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* pDesc)
{
    std::wstringstream wstr;
    wstr << L"\n";
    wstr << L"--------------------------------------------------------------------\n";
    wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(pDesc) << L": ";
    if (pDesc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
    if (pDesc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

    auto ExportTree = [](uint32_t depth, uint32_t numExports, const D3D12_EXPORT_DESC* exports)
        {
            std::wostringstream woss;
            for (uint32_t i = 0; i < numExports; i++)
            {
                woss << L"|";
                if (depth > 0)
                {
                    for (uint32_t j = 0; j < 2 * depth - 1; j++) woss << L" ";
                }
                woss << L" [" << i << L"]: ";
                if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
                woss << exports[i].Name << L"\n";
            }
            return woss.str();
        };

    for (uint32_t i = 0; i < pDesc->NumSubobjects; i++)
    {
        wstr << L"| [" << i << L"]: ";
        switch (pDesc->pSubobjects[i].Type)
        {
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            wstr << L"Global Root Signature 0x" << pDesc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
            wstr << L"Local Root Signature 0x" << pDesc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
            wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const uint32_t*>(pDesc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
            break;
#ifdef _GAMING_XBOX_SCARLETT
        case D3D12XBOX_STATE_SUBOBJECT_TYPE_RAYTRACING_OPTIONS:
            break;
#endif
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
        {
            wstr << L"DXIL Library 0x";
            auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(pDesc->pSubobjects[i].pDesc);
            wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
            wstr << ExportTree(1, lib->NumExports, lib->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
        {
            wstr << L"Existing Library 0x";
            auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(pDesc->pSubobjects[i].pDesc);
            wstr << collection->pExistingCollection << L"\n";
            wstr << ExportTree(1, collection->NumExports, collection->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"Subobject to Exports Association (Subobject [";
            auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(pDesc->pSubobjects[i].pDesc);
            uint32_t index = static_cast<uint32_t>(association->pSubobjectToAssociate - pDesc->pSubobjects);
            wstr << index << L"])\n";
            for (size_t j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"DXIL Subobjects to Exports Association (";
            auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(pDesc->pSubobjects[i].pDesc);
            wstr << association->SubobjectToAssociate << L")\n";
            for (size_t j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
        {
            wstr << L"Raytracing Shader Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(pDesc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
            wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
        {
            wstr << L"Raytracing Pipeline Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(pDesc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
        {
            wstr << L"Hit Group (";
            auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(pDesc->pSubobjects[i].pDesc);
            wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
            wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG:
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1:
        case D3D12_STATE_SUBOBJECT_TYPE_MAX_VALID:
            break;
        }
        wstr << L"|--------------------------------------------------------------------\n";
    }
    wstr << L"\n";
    OutputDebugStringW(wstr.str().c_str());
}

#pragma warning(pop)
