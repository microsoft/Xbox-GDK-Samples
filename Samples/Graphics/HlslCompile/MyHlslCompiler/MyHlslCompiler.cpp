//--------------------------------------------------------------------------------------
// MyD3DCompiler.cpp
//
// Defines the entry point for the console application.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <gxdk.h>

#include <assert.h>
#include <iomanip>
#include <regex>
#include <stdio.h>
#include <string>
#include <sstream>
#include <windows.h>

#ifdef _TOOL_XBOX
#ifdef _TOOL_XBOX_SCARLETT
#include <dxcapi_xs.h>
#pragma comment(lib, "dxcompiler_xs.lib")
#else
#include <dxcapi_x.h>
#pragma comment(lib, "dxcompiler_x.lib")
#endif
#endif

#include <wrl/client.h>

#include "Shlwapi.h"

using Microsoft::WRL::ComPtr;

namespace
{
    void PrintMessage(_In_z_ _Printf_format_string_ const char* fmt, ...)
    {
        char  buf[1024] = {};

        va_list ap;

        va_start(ap, fmt);

        vsprintf_s(buf, _countof(buf) - 1, fmt, ap);
        buf[_countof(buf) - 1] = 0;

        va_end(ap);

        OutputDebugStringA(buf);
        printf("%s", buf);
    }
}

template <typename str_t> HANDLE MyCreateFile(_In_ str_t pFileName,
	_In_ BOOL bOverwrite);
template <> HANDLE MyCreateFile<LPCSTR>(_In_ LPCSTR pFileName,
	_In_ BOOL bOverwrite)
{
	return CreateFileA(pFileName, GENERIC_WRITE, 0, NULL,
		bOverwrite ? CREATE_ALWAYS : CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL, NULL);
}
template <> HANDLE MyCreateFile<wchar_t*>(_In_ wchar_t* pFileName,
	_In_ BOOL bOverwrite)
{
	return CreateFileW(pFileName, GENERIC_WRITE, 0, NULL,
		bOverwrite ? CREATE_ALWAYS : CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL, NULL);
}

template <typename str_t>
HRESULT WINAPI WriteBlobToFile(_In_ IDxcBlob* pBlob,
	_In_ str_t pFileName,
	_In_ BOOL bOverwrite)
{

	HRESULT hr;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hFile = MyCreateFile(pFileName, bOverwrite);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	BYTE* pBytes = (BYTE*)pBlob->GetBufferPointer();
	SIZE_T uBytes = pBlob->GetBufferSize();

	while (uBytes > 0)
	{
		DWORD uReq;
		DWORD uDone;

#ifdef _WIN64
		if (uBytes > UINT_MAX)
		{
			uReq = UINT_MAX;
		}
		else
		{
			uReq = (DWORD)uBytes;
		}
#else
		uReq = uBytes;
#endif

		if (!WriteFile(hFile, pBytes, uReq, &uDone, NULL))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			if (FAILED(hr)) goto lExit;
		}
		if (uDone < uReq)
		{
			hr = E_FAIL;
			goto lExit;
		}

		pBytes += uDone;
		uBytes -= uDone;
	}

	hr = S_OK;

lExit:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}

	return hr;
}

int CompileDxc(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
	enum CompileFlags : uint32_t
	{
		CompileFlag_StripPdbs = 0x00000001,
		CompileFlag_SavePdbs = 0x00000002,
	};

	// Usage
	// argv[1] -- Flags
	// argv[2] -- Path to HLSL input
	// argv[3] -- Path to binary output
	// argv[4] -- Path to updb output (optional)
	if (argc < 4)
	{
		PrintMessage("Usage: compiler.exe <fxc|dxc> flags HlslFilename BinaryOutputFilename [ShaderPdbFileName]\n"
			"Shader PDB is always written even if the filename is not specified\n");
		return -1;
	}

	uint32_t flags = 0;
	swscanf_s(argv[1], L"%d", &flags);

	// Whether to leave PDBs in the binary, or strip them out.
	bool stripPdbs = flags & CompileFlag_StripPdbs;

	// Whether to save PDBs to disk.
	bool savePdbs = flags & CompileFlag_SavePdbs;

	// Whether the command line included an explicit symbol file name
	bool pdbNameSpecifiedByUser = argc > 4;

	ComPtr<IDxcUtils> utils;
	auto hr = DxcCreateInstance(CLSID_DxcUtils, 
        IID_PPV_ARGS(utils.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
		return hr;

    wchar_t pdbPath[MAX_PATH] = {};
    if (savePdbs)
    {
        if (pdbNameSpecifiedByUser)
        {
            wcscpy_s(pdbPath, argv[4]);
        }
        else
        {
            // Find the path for the shader binary output, and leave a trailing '\'
            wchar_t* fname = PathFindFileName(argv[3]);
            size_t pathCharCount = fname - argv[3];
            wcsncpy_s(pdbPath, argv[3], pathCharCount);
            pdbPath[pathCharCount] = L'\0';
        }
    }
    
    ComPtr<IDxcResult> result;

    try 
    {
        ComPtr<IDxcBlobEncoding> sourceEncoding;
        auto fileName = argv[2];
        hr = utils->LoadFile(fileName, nullptr, sourceEncoding.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;
        BOOL knownEncoding = FALSE;
        UINT32 encoding = 0U;
        sourceEncoding->GetEncoding(&knownEncoding, &encoding);
        DxcBuffer source =
        {
            sourceEncoding->GetBufferPointer(),
            sourceEncoding->GetBufferSize(),
            encoding, 
        };

        ComPtr<IDxcIncludeHandler> includeHandler;
        utils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;

        // Pass the same arguments as you would to the command line, as text
        std::vector<LPCWSTR> arguments;

        // entrypoint
        arguments.push_back(L"-E");
        arguments.push_back(L"main");

        // shader model
        arguments.push_back(L"-T");
        arguments.push_back(L"ps_6_0");

        // Generate symbols
	    if (stripPdbs)
        {
            arguments.push_back(L"-Zs");
        }
        else
        {
            arguments.push_back(L"-Zi");

            // Embed symbols
            arguments.push_back(L"-Qembed_debug");
        }

        // Output symbols (we still need to do the WriteBlobToFile ourselves
        arguments.push_back(L"-Fd");
        arguments.push_back(pdbPath);

        // Strip
        arguments.push_back(L"-Qstrip_reflect");
        //arguments.push_back(L"-Qstrip_rootsignature");    // Currently gives a validation error
        arguments.push_back(L"-D");
        arguments.push_back(L"__XBOX_STRIP_DXIL=1");

        ComPtr<IDxcCompiler> compiler;
        hr = DxcCreateInstance(CLSID_DxcCompiler, 
            IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            return hr;

        ComPtr<IDxcCompiler3> compiler3;
        compiler.Get()->QueryInterface(compiler3.GetAddressOf());
        if (FAILED(hr))
            return hr;

        hr = compiler3->Compile(&source,
            arguments.data(),
            (UINT32)arguments.size(),
            includeHandler.Get(),
            IID_PPV_ARGS(result.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            return hr;
    }
	catch (const std::bad_alloc &) {
		return E_OUTOFMEMORY;
	}

	result->GetStatus(&hr);
	if (FAILED(hr))
	{
        ComPtr<IDxcBlobEncoding> errorMsgs;
        auto otherHr = result->GetOutput(DXC_OUT_ERRORS, 
            IID_PPV_ARGS(errorMsgs.ReleaseAndGetAddressOf()), 
            nullptr);
		if (FAILED(otherHr))
			return otherHr;

		PrintMessage("Compile returned error (0x%x), message = %s\n",
			hr,
			errorMsgs->GetBufferPointer());
		return hr;
	}

    ComPtr<IDxcBlob> code;

    hr = result->GetOutput(DXC_OUT_OBJECT, 
        IID_PPV_ARGS(code.ReleaseAndGetAddressOf()), 
        nullptr);
	if (FAILED(hr))
		return hr;

	if (savePdbs)
	{
        ComPtr<IDxcBlob> pdbData;
        ComPtr<IDxcBlobUtf16> pdbPathFromCompiler;

        hr = result->GetOutput(DXC_OUT_PDB, 
            IID_PPV_ARGS(pdbData.ReleaseAndGetAddressOf()), 
            pdbPathFromCompiler.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;

        if (!stripPdbs)
		{
			PrintMessage("CompileFlag_SavePdbs requires CompileFlag_StripPdbs\n");
			return E_INVALIDARG;
		}

        if (pdbNameSpecifiedByUser)
        {
            // This way the binary contains the fully qualified pdb path.
            // PIX will pick the pdb up automatically if it's not moved.
        }
        else if (pdbPathFromCompiler->GetStringLength() > 0U)
        {
            // This way the binary contains only the pdb file name, not the folder.
            // PIX will need the user to set a pdb path, or resolve the pdb file manually.
            wcscat_s(pdbPath, pdbPathFromCompiler->GetStringPointer());
        }
        else
        {
            PrintMessage("Pdb output was requested, but pdbName neither passed in by user nor generated by compiler.\n");
            return E_FAIL;
        }

		// Now write the contents of pPDB to a file 
		hr = WriteBlobToFile(pdbData.Get(), pdbPath, TRUE);
		if (FAILED(hr))
		{
			PrintMessage("WriteBlobToFile returned error (0x%08X)\n", static_cast<unsigned int>(hr));
			return hr;
		}

		PrintMessage("PDB size %d is written here: %ls\n",
			pdbData->GetBufferSize(),
			pdbPath);
	}

	hr = WriteBlobToFile(code.Get(), argv[3], TRUE);
	if (FAILED(hr))
	{
		PrintMessage("WriteBlobToFile returned error (0x%08X)\n", static_cast<unsigned int>(hr));
		return hr;
	}

	PrintMessage("Binary size %d is written here: %ls\n",
		code->GetBufferSize(),
		argv[3]);

	return hr;
}

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
	if (argc < 2 || (0 != wcscmp(argv[1], L"fxc") && 0 != wcscmp(argv[1], L"dxc")))
	{
		PrintMessage("First argument was \"%ws\", must be either fxc or dxc\n", argv[1]);
		return -1;
	}

	wchar_t** argvPassOn = (wchar_t**)alloca((argc - 1) * sizeof(wchar_t*));
	argvPassOn[0] = argv[0];
	for (auto i = 2; i < argc; ++i)
	{
		argvPassOn[i-1] = argv[i];
	}

	if (0 != wcscmp(argv[1], L"dxc"))
	{
        PrintMessage("The old shader compiler fxc is no longer supported\n", argv[1]);
        return -1;
	}
	else
	{
		return CompileDxc(argc - 1, argvPassOn);
	}
}

