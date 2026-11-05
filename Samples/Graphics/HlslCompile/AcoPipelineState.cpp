//--------------------------------------------------------------------------------------
// AcoPipelineState.cpp
//
// Graphics shaders compiled with ACO run only through a pipeline state that was serialized
// offline (see MyAcoCompiler.cpp). This loads the serialized components written by
// MyAcoCompiler.exe "pso" and recreates the pipeline state with DeserializeGraphicsPipelineStateX.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AcoPipelineState.h"

#include "ReadData.h"

#include <xmem.h>

namespace
{
    // GPU shader instructions must be in memory the GPU can execute shaders from. The same
    // attributes are passed to XMemAlloc and XMemFree.
    const uint64_t c_shaderMemoryAttributes = MAKE_XALLOC_ATTRIBUTES(
        eXALLOCAllocatorId_GameMin,
        0,
        XALLOC_MEMTYPE_GRAPHICS_SHADER_CACHEABLE_NONCOHERENT,
        XALLOC_PAGESIZE_64KB,
        XALLOC_ALIGNMENT_ANY,
        0);
}

void AcoPipelineState::ShaderMemoryDeleter::operator()(void* memory) const noexcept
{
    XMemFree(memory, c_shaderMemoryAttributes);
}

AcoPipelineState::ShaderMemory AcoPipelineState::LoadShaderInstructions(const std::wstring& path, size_t& size)
{
    auto const instructions = DX::ReadData(path.c_str());

    ShaderMemory memory(XMemAlloc(instructions.size(), c_shaderMemoryAttributes));
    if (!memory)
    {
        throw std::bad_alloc();
    }

    memcpy(memory.get(), instructions.data(), instructions.size());
    size = instructions.size();
    return memory;
}

AcoPipelineState::AcoPipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, const wchar_t* basePath)
{
    const std::wstring base(basePath);

    // The PSO packet and all metadata are only read during deserialization, so ordinary heap
    // memory that's freed when this function returns is fine.
    auto packet = DX::ReadData((base + L".packet.bin").c_str());
    auto packetMetaData = DX::ReadData((base + L".packet.meta.bin").c_str());
    auto vertexShaderMetaData = DX::ReadData((base + L".vs.meta.bin").c_str());
    auto pixelShaderMetaData = DX::ReadData((base + L".ps.meta.bin").c_str());

    // The shader instructions are executed in place by the GPU, so they need graphics memory that
    // outlives the pipeline state.
    m_vertexShaderInstructions = LoadShaderInstructions(base + L".vs.bin", m_vertexShaderSize);
    m_pixelShaderInstructions = LoadShaderInstructions(base + L".ps.bin", m_pixelShaderSize);

    // The root signature must match the one the pipeline state was serialized with. ACO doesn't
    // produce a separate fetch shader (FS), and this pipeline has no GS/ES/HS/LS stages.
    D3D12XBOX_DESERIALIZE_GRAPHICS_PIPELINE_STATE desc = {};
    desc.pRootSignature = rootSignature;
    desc.pPacket = packet.data();
    desc.PacketSize = packet.size();
    desc.pMetaData = packetMetaData.data();
    desc.MetaDataSize = packetMetaData.size();
    desc.VS.pGpuInstructions = m_vertexShaderInstructions.get();
    desc.VS.GpuInstructionsSize = m_vertexShaderSize;
    desc.VS.pMetaData = vertexShaderMetaData.data();
    desc.VS.MetaDataSize = vertexShaderMetaData.size();
    desc.PS.pGpuInstructions = m_pixelShaderInstructions.get();
    desc.PS.GpuInstructionsSize = m_pixelShaderSize;
    desc.PS.pMetaData = pixelShaderMetaData.data();
    desc.PS.MetaDataSize = pixelShaderMetaData.size();

    DX::ThrowIfFailed(device->DeserializeGraphicsPipelineStateX(&desc,
        IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
}
