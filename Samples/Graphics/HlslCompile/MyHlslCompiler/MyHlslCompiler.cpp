//--------------------------------------------------------------------------------------
// MyHlslCompiler.cpp
//
// A minimal offline build tool that compiles HLSL with the Xbox shader compiler DLL
// (dxcompiler_x.dll for Xbox One, dxcompiler_xs.dll for Xbox Series X|S), and shows the
// options for storing shader symbols (PDBs).
//
//   MyHlslCompiler.exe dxc <flags> <input.hlsl> <output.cso> [output.pdb]
//
//   flags  0  Embed symbols in the shader binary.
//          1  Strip symbols from the shader binary (CompileFlag_StripPdbs).
//          3  Strip symbols and save them to a PDB file (CompileFlag_StripPdbs | CompileFlag_SavePdbs).
//             If output.pdb is omitted, the compiler names the PDB after the shader hash and
//             it is written next to output.cso.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <gxdk.h>

#include <Windows.h>
#include <wrl/client.h>

// Xbox shader compiler headers and import libraries for PC tools.
#ifdef _TOOL_XBOX_SCARLETT
#include <dxcapi_xs.h>
#pragma comment(lib, "dxcompiler_xs.lib")
#else
#include <dxcapi_x.h>
#pragma comment(lib, "dxcompiler_x.lib")
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
    enum CompileFlags : uint32_t
    {
        CompileFlag_StripPdbs = 0x1,
        CompileFlag_SavePdbs = 0x2,
    };

    void ThrowIfFailed(HRESULT hr, const char* operation)
    {
        if (FAILED(hr))
        {
            char message[256] = {};
            sprintf_s(message, "%s failed with HRESULT 0x%08X", operation, static_cast<unsigned int>(hr));
            throw std::runtime_error(message);
        }
    }

    void WriteBlobToFile(const std::wstring& path, IDxcBlob* blob)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(static_cast<const char*>(blob->GetBufferPointer()), static_cast<std::streamsize>(blob->GetBufferSize()));
        if (!file)
        {
            char message[MAX_PATH + 64] = {};
            sprintf_s(message, "Unable to write %ls", path.c_str());
            throw std::runtime_error(message);
        }

        wprintf(L"  %6zu bytes -> %ls\n", blob->GetBufferSize(), path.c_str());
    }

    void CompileShader(uint32_t flags, const wchar_t* inputPath, const wchar_t* outputPath, const wchar_t* pdbPath)
    {
        // Whether to strip symbols from the shader binary, rather than embed them.
        const bool stripPdbs = (flags & CompileFlag_StripPdbs) != 0;

        // Whether to save the symbols to a PDB file. Only stripped symbols can be saved.
        const bool savePdbs = (flags & CompileFlag_SavePdbs) != 0;

        if (savePdbs && !stripPdbs)
        {
            throw std::invalid_argument("CompileFlag_SavePdbs requires CompileFlag_StripPdbs");
        }

        wprintf(L"Compiling %ls\n", inputPath);

        // The -Fd argument is either a full PDB file name, or a folder with a trailing '\'. For a
        // folder, the compiler chooses a PDB file name based on the shader hash.
        std::wstring pdbOutput;
        if (savePdbs)
        {
            if (pdbPath)
            {
                pdbOutput = pdbPath;
            }
            else
            {
                const std::wstring output(outputPath);
                const size_t folderEnd = output.find_last_of(L"\\/");
                pdbOutput = (folderEnd == std::wstring::npos) ? L".\\" : output.substr(0, folderEnd + 1);
            }
        }

        ComPtr<IDxcUtils> utils;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf())), "DxcCreateInstance(CLSID_DxcUtils)");

        ComPtr<IDxcBlobEncoding> source;
        ThrowIfFailed(utils->LoadFile(inputPath, nullptr, source.GetAddressOf()), "IDxcUtils::LoadFile");

        const DxcBuffer sourceBuffer = { source->GetBufferPointer(), source->GetBufferSize(), DXC_CP_ACP };

        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf()), "IDxcUtils::CreateDefaultIncludeHandler");

        // These are the same arguments you would pass to dxc.exe on the command line.
        std::vector<const wchar_t*> arguments =
        {
            inputPath,                  // Source file name, recorded in the PDB
            L"-E", L"main",
            L"-T", L"ps_6_0",
        };

        if (stripPdbs)
        {
            // Generate a small PDB containing the source and compile options, and leave the
            // symbols out of the shader binary.
            arguments.push_back(L"-Zs");
        }
        else
        {
            // Generate full symbols and embed them in the shader binary.
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Qembed_debug");
        }

        if (savePdbs)
        {
            // The compiler records the PDB name in the shader binary so PIX can find it. It returns
            // the PDB contents, and the tool writes the file itself (see below).
            arguments.push_back(L"-Fd");
            arguments.push_back(pdbOutput.c_str());
        }

        // Reflection data isn't needed at runtime.
        arguments.push_back(L"-Qstrip_reflect");

        // Remove the DXIL from the final binary. Only the Xbox precompiled code is needed at runtime.
        arguments.push_back(L"-D");
        arguments.push_back(L"__XBOX_STRIP_DXIL=1");

        ComPtr<IDxcCompiler3> compiler;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf())), "DxcCreateInstance(CLSID_DxcCompiler)");

        ComPtr<IDxcResult> result;
        ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()),
            includeHandler.Get(), IID_PPV_ARGS(result.GetAddressOf())), "IDxcCompiler3::Compile");

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

        if (savePdbs)
        {
            // The compiler also returns the PDB file name it recorded in the shader binary.
            ComPtr<IDxcBlob> pdb;
            ComPtr<IDxcBlobUtf16> pdbName;
            ThrowIfFailed(result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdb.GetAddressOf()), pdbName.GetAddressOf()), "IDxcResult::GetOutput(DXC_OUT_PDB)");

            if (!pdbPath)
            {
                // The binary contains only the PDB file name, not the folder. In PIX, the user
                // must add the folder to the PDB search paths.
                if (!pdbName || pdbName->GetStringLength() == 0)
                {
                    throw std::runtime_error("The compiler didn't generate a PDB file name");
                }

                pdbOutput += pdbName->GetStringPointer();
            }
            // Otherwise the binary contains the fully qualified PDB path, and PIX finds the PDB
            // automatically as long as it isn't moved.

            WriteBlobToFile(pdbOutput, pdb.Get());
        }

        ComPtr<IDxcBlob> shader;
        ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shader.GetAddressOf()), nullptr), "IDxcResult::GetOutput(DXC_OUT_OBJECT)");
        WriteBlobToFile(outputPath, shader.Get());
    }

    void PrintUsage()
    {
        wprintf(L"Usage: MyHlslCompiler.exe dxc <flags> <input.hlsl> <output.cso> [output.pdb]\n"
            L"  flags: 0 = embed symbols, 1 = strip symbols, 3 = strip symbols and save them to a PDB file\n");
    }
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
    try
    {
        // The first argument names the compiler. Only dxc is supported; the legacy fxc compiler
        // is no longer supported for Xbox.
        if ((argc == 5 || argc == 6) && wcscmp(argv[1], L"dxc") == 0)
        {
            const auto flags = static_cast<uint32_t>(wcstoul(argv[2], nullptr, 10));
            CompileShader(flags, argv[3], argv[4], (argc == 6) ? argv[5] : nullptr);
            return 0;
        }

        PrintUsage();
    }
    catch (const std::exception& e)
    {
        // The "error" prefix makes MSBuild report this as a build error.
        printf("MyHlslCompiler : error : %s\n", e.what());
    }

    return 1;
}
