//-----------------------------------------------------------------------------
// PsoDesc.h
//
// This header is shared between the AdvancedPSO and PSOGen projects. It has
// common settings for offline PSO creation - normally these would be driven
// by data shared between title and tool.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace ATG
{
    inline DXGI_FORMAT GetRenderTargetFormat()
    {
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    }

    inline DXGI_FORMAT GetDepthFormat() 
    {
        return DXGI_FORMAT_D32_FLOAT;
    }

    // Shared function to generate a PSO desc for both PSOGen tool and AdvancedPSO viewer.
    inline void GetSharedPSODescription(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
    {
        // Input Layout
        static const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        static const D3D12_DEPTH_STENCIL_DESC DepthDefault =
        {
            TRUE, // DepthEnable
            D3D12_DEPTH_WRITE_MASK_ALL, // DepthWriteMask
            D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
            FALSE, // StencilEnable
            D3D12_DEFAULT_STENCIL_READ_MASK, // StencilReadMask
            D3D12_DEFAULT_STENCIL_READ_MASK, // StencilWriteMask
            {
                D3D12_STENCIL_OP_KEEP, // StencilFailOp
                D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP, // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
            }, // FrontFace,
            {
                D3D12_STENCIL_OP_KEEP, // StencilFailOp
                D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP, // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
            } // BackFace
        };

        // Describe the base graphics pipeline state object (PSO). 
        psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = DepthDefault;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = GetRenderTargetFormat();
        psoDesc.DSVFormat = GetDepthFormat();
        psoDesc.SampleDesc.Count = 1;
    }

}
