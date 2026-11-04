//--------------------------------------------------------------------------------------
// MyAcoCompiler.cpp
//
// A minimal offline build tool for the ACO Xbox Series X|S shader compiler.
//
// Graphics shaders compiled with ACO run on the console only through a pipeline state
// that was serialized offline. Creating a PSO at runtime with CreateGraphicsPipelineState
// does not use ACO code. A title's build pipeline therefore needs two steps, each
// implemented here as a command:
//
//   compile  Compile HLSL with xdxcompiler_xs.dll (IXdxcCompiler3). The output DXIL
//            container carries the ACO compiled code. Symbols are saved to a PDB file.
//
//            MyAcoCompiler.exe compile <target> <input.hlsl> <output.cso> <output.pdb>
//
//   pso      Combine ACO vertex and pixel shaders into a graphics pipeline state with
//            aco_xbox.dll and serialize it to disk. The title loads it with
//            ID3D12Device::DeserializeGraphicsPipelineStateX (see AcoPipelineState.cpp).
//
//            MyAcoCompiler.exe pso <vertexShader.cso> <pixelShader.cso> <outputBasePath>
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <Windows.h>
#include <wrl/client.h>

// Xbox Series X|S headers for PC tools, from the GDK's xbox\include\devpctools folders.
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#include <xdxcapi.h>    // IXdxcCompiler3, IXdxcResult (xdxcompiler_xs.dll)
#include <aco_xbox.h>   // ACO offline pipeline state serialization (aco_xbox.dll)

#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
    //----------------------------------------------------------------------------------
    // Helpers
    //----------------------------------------------------------------------------------

    void ThrowIfFailed(HRESULT hr, const char* operation)
    {
        if (FAILED(hr))
        {
            char message[256] = {};
            sprintf_s(message, "%s failed with HRESULT 0x%08X", operation, static_cast<unsigned int>(hr));
            throw std::runtime_error(message);
        }
    }

    std::vector<uint8_t> ReadBinaryFile(const wchar_t* path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            char message[MAX_PATH + 64] = {};
            sprintf_s(message, "Unable to open %ls", path);
            throw std::runtime_error(message);
        }

        std::vector<uint8_t> data(static_cast<size_t>(file.tellg()));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return data;
    }

    void WriteBinaryFile(const std::wstring& path, const void* data, size_t size)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!file)
        {
            char message[MAX_PATH + 64] = {};
            sprintf_s(message, "Unable to write %ls", path.c_str());
            throw std::runtime_error(message);
        }

        wprintf(L"  %6zu bytes -> %ls\n", size, path.c_str());
    }

    void WriteBlob(const std::wstring& path, ID3DBlob* blob)
    {
        // Every component is written, even if empty, so the title can load a fixed set of files.
        WriteBinaryFile(path, blob ? blob->GetBufferPointer() : nullptr, blob ? blob->GetBufferSize() : 0);
    }

    //----------------------------------------------------------------------------------
    // Step 1: compile HLSL with ACO
    //----------------------------------------------------------------------------------

    void CompileShader(const wchar_t* target, const wchar_t* inputPath, const wchar_t* outputPath, const wchar_t* pdbPath)
    {
        wprintf(L"Compiling %ls (%ls) with ACO\n", inputPath, target);

        // xdxcompiler_xs.dll is the DXC front end for Xbox Series X|S. Its compiler object implements
        // IXdxcCompiler3, which extends the standard IDxcCompiler3 with ACO specific options.
        ComPtr<IXdxcCompiler3> compiler;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf())), "DxcCreateInstance(CLSID_DxcCompiler)");

        const aco_compile_options acoOptions = ACO_COMPILE_OPTIONS_DEFAULT;
        ThrowIfFailed(compiler->SetCompileOptions(&acoOptions), "IXdxcCompiler3::SetCompileOptions");

        ComPtr<IDxcUtils> utils;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf())), "DxcCreateInstance(CLSID_DxcUtils)");

        ComPtr<IDxcBlobEncoding> source;
        ThrowIfFailed(utils->LoadFile(inputPath, nullptr, source.GetAddressOf()), "IDxcUtils::LoadFile");

        const DxcBuffer sourceBuffer = { source->GetBufferPointer(), source->GetBufferSize(), DXC_CP_ACP };

        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf()), "IDxcUtils::CreateDefaultIncludeHandler");

        // These are the same arguments you would pass to xdxc.exe on the command line.
        const wchar_t* arguments[] =
        {
            inputPath,                  // Source file name, recorded in the PDB
            L"-E", L"main",
            L"-T", target,

            // Generate ACO code only. By default the compiler generates code for both ACO and the
            // previous NewBE compiler.
            L"-noprecompile",

            // Fail the compile if ACO code can't be generated, rather than producing a shader without it.
            L"-requireacoprecompile",

            // Generate full symbols, strip them from the shader binary, and name the PDB file. The
            // name is recorded in the binary so PIX can find the PDB automatically.
            L"-Zi",
            L"-Qstrip_debug",
            L"-Fd", pdbPath,

            // Reflection data isn't needed at runtime.
            L"-Qstrip_reflect",
        };

        ComPtr<IDxcResult> result;
        ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments, static_cast<UINT32>(std::size(arguments)),
            includeHandler.Get(), IID_PPV_ARGS(result.GetAddressOf())), "IXdxcCompiler3::Compile");

        // Print warnings and errors in the standard compiler format, so they show up in the build output.
        ComPtr<IDxcBlobUtf8> messages;
        if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(messages.GetAddressOf()), nullptr))
            && messages && messages->GetStringLength() > 0)
        {
            printf("%s\n", messages->GetStringPointer());
        }

        HRESULT status = E_FAIL;
        ThrowIfFailed(result->GetStatus(&status), "IDxcResult::GetStatus");
        ThrowIfFailed(status, "Shader compilation");

        // IXdxcResult::GetCookPayload returns the ACO code embedded in the shader. Graphics shaders
        // use the 'ACGS' part, compute shaders the 'ACCC' part. -requireacoprecompile already
        // guarantees it is present; it's queried here to show where the ACO output lives.
        ComPtr<IXdxcResult> xboxResult;
        ThrowIfFailed(result.As(&xboxResult), "QueryInterface(IXdxcResult)");

        ComPtr<IDxcBlob> acoPayload;
        uint32_t acoFourCC = 0;
        ThrowIfFailed(xboxResult->GetCookPayload(acoPayload.GetAddressOf(), &acoFourCC), "IXdxcResult::GetCookPayload");
        wprintf(L"  ACO payload '%.4hs', %zu bytes\n", reinterpret_cast<const char*>(&acoFourCC), acoPayload->GetBufferSize());

        // The PDB contains the HLSL source and DXIL debug information. It doesn't include
        // debug information for ACO generated ISA, so PIX can't correlate ACO instructions with
        // HLSL source lines.
        ComPtr<IDxcBlob> pdb;
        ThrowIfFailed(result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdb.GetAddressOf()), nullptr), "IDxcResult::GetOutput(DXC_OUT_PDB)");
        WriteBinaryFile(pdbPath, pdb->GetBufferPointer(), pdb->GetBufferSize());

        ComPtr<IDxcBlob> shader;
        ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shader.GetAddressOf()), nullptr), "IDxcResult::GetOutput(DXC_OUT_OBJECT)");
        WriteBinaryFile(outputPath, shader->GetBufferPointer(), shader->GetBufferSize());
    }

    //----------------------------------------------------------------------------------
    // Step 2: serialize a graphics pipeline state with ACO
    //----------------------------------------------------------------------------------

    // AcoOfflineInit/AcoOfflineShutdown bracket all use of the aco_xbox.dll offline API.
    class AcoOfflineSession
    {
    public:
        AcoOfflineSession()
        {
            ThrowIfFailed(AcoOfflineInit(), "AcoOfflineInit");
        }

        ~AcoOfflineSession()
        {
            AcoOfflineShutdown();
        }

        AcoOfflineSession(const AcoOfflineSession&) = delete;
        AcoOfflineSession& operator=(const AcoOfflineSession&) = delete;
    };

    // Owns the blobs returned by AcoOfflineSerializePipelineStateXEx.
    struct SerializedPipelineState : D3D12XBOX_SERIALIZE_GRAPHICS_PIPELINE_STATE
    {
        SerializedPipelineState() noexcept : D3D12XBOX_SERIALIZE_GRAPHICS_PIPELINE_STATE{} {}

        ~SerializedPipelineState()
        {
            ID3DBlob* blobs[] =
            {
                pPacket, pMetaData,
                PS.pGpuInstructions, PS.pMetaData, VS.pGpuInstructions, VS.pMetaData,
                GS.pGpuInstructions, GS.pMetaData, ES.pGpuInstructions, ES.pMetaData,
                HS.pGpuInstructions, HS.pMetaData, LS.pGpuInstructions, LS.pMetaData,
                FS.pGpuInstructions, FS.pMetaData,
            };

            for (auto blob : blobs)
            {
                if (blob)
                {
                    blob->Release();
                }
            }
        }

        SerializedPipelineState(const SerializedPipelineState&) = delete;
        SerializedPipelineState& operator=(const SerializedPipelineState&) = delete;
    };

    // Only shaders compiled with ACO can be serialized by aco_xbox.dll.
    void RequireAcoGraphicsShader(const std::vector<uint8_t>& shader, const wchar_t* path)
    {
        const void* part = nullptr;
        uint32_t partSize = 0;
        if (AcoOfflineFindCookPartInDxil(shader.data(), static_cast<uint32_t>(shader.size()), ACO_FOURCC_ACGS, &part, &partSize) < 0)
        {
            char message[MAX_PATH + 128] = {};
            sprintf_s(message, "%ls has no ACO graphics code. Compile it with \"MyAcoCompiler compile\" or xdxc.exe.", path);
            throw std::runtime_error(message);
        }
    }

    void SerializePipelineState(const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, const std::wstring& outputBasePath)
    {
        wprintf(L"Serializing ACO pipeline state for %ls + %ls\n", vertexShaderPath, pixelShaderPath);

        const AcoOfflineSession acoSession;

        const auto vertexShader = ReadBinaryFile(vertexShaderPath);
        const auto pixelShader = ReadBinaryFile(pixelShaderPath);
        RequireAcoGraphicsShader(vertexShader, vertexShaderPath);
        RequireAcoGraphicsShader(pixelShader, pixelShaderPath);

        // The serializer takes the root signature as a version 1.1 description. It must match the
        // root signature the title uses with this PSO: here, the shaders' HLSL attribute
        //     [RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
        // A production pipeline would generate both from a single source.
        D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // Describe the pipeline state. This must match the state the title would otherwise pass to
        // CreateGraphicsPipelineState (see HlslCompile.cpp). A production pipeline would read these
        // descriptions from data rather than hard-coding them.
        static const D3D12_INPUT_ELEMENT_DESC c_inputLayout[] =
        {
            // DirectX::VertexPositionColor::InputLayout
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // The render target formats the sample passes to DX::DeviceResources.
        constexpr DXGI_FORMAT c_backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        constexpr DXGI_FORMAT c_depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          inputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    primitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS                    vs;
            CD3DX12_PIPELINE_STATE_STREAM_PS                    ps;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            rasterizerState;
            CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            blendState;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL         depthStencilState;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  dsvFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
            CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK           sampleMask;
            CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           sampleDesc;
        } stream;

        stream.inputLayout = D3D12_INPUT_LAYOUT_DESC{ c_inputLayout, static_cast<UINT>(std::size(c_inputLayout)) };
        stream.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        stream.vs = D3D12_SHADER_BYTECODE{ vertexShader.data(), vertexShader.size() };
        stream.ps = D3D12_SHADER_BYTECODE{ pixelShader.data(), pixelShader.size() };
        stream.rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        stream.blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

        CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;
        stream.depthStencilState = depthStencilDesc;
        stream.dsvFormat = c_depthBufferFormat;

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = c_backBufferFormat;
        stream.rtvFormats = rtvFormats;

        stream.sampleMask = UINT_MAX;
        stream.sampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };

        const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(stream), &stream };

        // Compile the pipeline state with ACO and serialize it. The result is a set of blobs: the
        // PSO packet and its metadata, plus the GPU instructions and metadata of each shader stage.
        const aco_compile_options acoOptions = ACO_COMPILE_OPTIONS_DEFAULT;
        SerializedPipelineState serialized;
        ThrowIfFailed(AcoOfflineSerializePipelineStateXEx(&streamDesc, &rootSignatureDesc, &serialized,
            nullptr, "HlslCompile_Aco", &acoOptions), "AcoOfflineSerializePipelineStateXEx");

        // This tool and the title's loader handle the vertex + pixel shader case only. Other
        // pipelines also produce geometry, hull, or domain shader stages (GS/ES/HS/LS).
        // ACO doesn't produce a separate fetch shader (FS).
        if (!serialized.pPacket || !serialized.VS.pGpuInstructions || !serialized.PS.pGpuInstructions
            || serialized.GS.pGpuInstructions || serialized.ES.pGpuInstructions || serialized.HS.pGpuInstructions
            || serialized.LS.pGpuInstructions || serialized.FS.pGpuInstructions)
        {
            throw std::runtime_error("Unexpected set of serialized pipeline state components");
        }

        // Each component is saved to its own file, named so AcoPipelineState.cpp can find them.
        WriteBlob(outputBasePath + L".packet.bin", serialized.pPacket);
        WriteBlob(outputBasePath + L".packet.meta.bin", serialized.pMetaData);
        WriteBlob(outputBasePath + L".vs.bin", serialized.VS.pGpuInstructions);
        WriteBlob(outputBasePath + L".vs.meta.bin", serialized.VS.pMetaData);
        WriteBlob(outputBasePath + L".ps.bin", serialized.PS.pGpuInstructions);
        WriteBlob(outputBasePath + L".ps.meta.bin", serialized.PS.pMetaData);
    }

    void PrintUsage()
    {
        wprintf(L"Usage:\n"
            L"  MyAcoCompiler.exe compile <target> <input.hlsl> <output.cso> <output.pdb>\n"
            L"  MyAcoCompiler.exe pso <vertexShader.cso> <pixelShader.cso> <outputBasePath>\n");
    }
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc == 6 && wcscmp(argv[1], L"compile") == 0)
        {
            CompileShader(argv[2], argv[3], argv[4], argv[5]);
            return 0;
        }

        if (argc == 5 && wcscmp(argv[1], L"pso") == 0)
        {
            SerializePipelineState(argv[2], argv[3], argv[4]);
            return 0;
        }

        PrintUsage();
    }
    catch (const std::exception& e)
    {
        // The "error" prefix makes MSBuild report this as a build error.
        printf("MyAcoCompiler : error : %s\n", e.what());
    }

    return 1;
}
