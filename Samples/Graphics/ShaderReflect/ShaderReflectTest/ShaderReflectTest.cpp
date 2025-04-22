//------------------------------------------------------------------------------
// ShaderReflectTest - a simple driver for running the ShaderReflect tool
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This project includes a number of test shaders that get compiled into shader
// objects during build. At runtime the tool invokes the ShaderReflect tool
// with each of the shader objects.
//------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include <iterator>

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Determine executable directory
    wchar_t baseDir[MAX_PATH];
    {
        if (!GetModuleFileName(nullptr, baseDir, _countof(baseDir)))
        {
            fwprintf(stderr, L"ERROR: Failed to get executable path\n");
            return 1;
        }
        wchar_t* lastPathSeparator = wcsrchr(baseDir, '\\');
        if (!lastPathSeparator)
        {
            fwprintf(stderr, L"ERROR: Missing path separator\n");
            return 1;
        }
        // Terminate module path at last separator
        lastPathSeparator[1] = '\0';
    }

    const wchar_t* shaderObjects[] =
    {
        L"DeferredLightingVS.cso",
        L"DeferredLightingPS.cso",
        L"ParticleDeferredPS.cso",
        L"ParticleForwardPS.cso",
        L"ParticleVS.cso",
        L"SceneVS.cso",
        L"ScenePS.cso"
    };

    // Invoke ShaderReflect tool with each shader object
    wchar_t cmdLine[1024];
    for (size_t i = 0; i < std::size(shaderObjects); i++)
    {
        cmdLine[0] = '\0';
        wcscat_s(cmdLine, baseDir);
        wcscat_s(cmdLine, L"ShaderReflect.exe ");
        wcscat_s(cmdLine, baseDir);
        wcscat_s(cmdLine, shaderObjects[i]);

        // Start the child process
        STARTUPINFO si = { sizeof(si), {} };
        PROCESS_INFORMATION pi = {};
        if (!CreateProcess(
            nullptr,        // Application name
            cmdLine,        // Command line
            nullptr,        // Process handle not inheritable
            nullptr,        // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            nullptr,        // Use parent's environment block
            nullptr,        // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi))           // Pointer to PROCESS_INFORMATION structure
        {
            fwprintf(stderr, L"ERROR: Failed to spawn tool process (0x%u)\n", GetLastError());
            return 1;
        }

        // Wait until child process exits
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return 0;
}
