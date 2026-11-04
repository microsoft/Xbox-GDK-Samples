//--------------------------------------------------------------------------------------
// AcoPipelineState.h
//
// Loads a graphics pipeline state that MyAcoCompiler compiled with ACO and serialized
// offline (Xbox Series X|S only).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class AcoPipelineState
{
public:
    // basePath is the path MyAcoCompiler.exe "pso" wrote the components to, without extension.
    AcoPipelineState(ID3D12Device* device, ID3D12RootSignature* rootSignature, const wchar_t* basePath);

    AcoPipelineState(AcoPipelineState const&) = delete;
    AcoPipelineState& operator= (AcoPipelineState const&) = delete;

    ID3D12PipelineState* Get() const noexcept { return m_pipelineState.Get(); }

    // Size of the ACO generated pixel shader instructions.
    size_t GetPixelShaderSize() const noexcept { return m_pixelShaderSize; }

private:
    // Shader instructions live in graphics memory allocated with XMemAlloc.
    struct ShaderMemoryDeleter
    {
        void operator()(void* memory) const noexcept;
    };
    using ShaderMemory = std::unique_ptr<void, ShaderMemoryDeleter>;

    static ShaderMemory LoadShaderInstructions(const std::wstring& path, size_t& size);

    // The shader instructions must stay allocated for as long as the pipeline state exists,
    // so they're declared before (and destroyed after) the pipeline state.
    ShaderMemory                                m_vertexShaderInstructions;
    ShaderMemory                                m_pixelShaderInstructions;
    size_t                                      m_vertexShaderSize = 0;
    size_t                                      m_pixelShaderSize = 0;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};
