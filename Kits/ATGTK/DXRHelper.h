//--------------------------------------------------------------------------------------
// DXRHelper.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

#pragma warning(push)
#pragma warning(disable : 4316 4324) // We don't care that ShaderRecord (or derived types) are potentially unaligned or padded. They won't be in the Upload Heap

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
#pragma warning(pop)
