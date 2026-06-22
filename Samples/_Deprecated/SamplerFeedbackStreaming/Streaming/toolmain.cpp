//--------------------------------------------------------------------------------------
// toolmain.cpp
//
// Main app class for the Streaming devtest content creation tool.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <conio.h>
#include <random>
#include <d3d12.h>
#include "JobQueue.h"
#include "TextureGen.h"
#include "TextureFiles.h"
#include "ToolDefines.h"

UINT32 g_Seed = 0;

static void* AllocateThreadData(UINT32 ThreadIndex)
{
    JobQueueThreadData* pThreadData = new JobQueueThreadData();
    pThreadData->ThreadIndex = ThreadIndex;
    pThreadData->hThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    pThreadData->RNG.SetSeed(ThreadIndex * g_Seed);
    return pThreadData;
}

static void FreeThreadData(UINT32 ThreadIndex, void* pThreadData)
{
    JobQueueThreadData* pJQTD = (JobQueueThreadData*)pThreadData;
    CloseHandle(pJQTD->hThreadEvent);
    delete pJQTD;
}

UINT32 g_StreamingTextureCount = 64;
UINT32 g_TextureDimension = 8192;
WCHAR g_strTexturePath[MAX_PATH] = L".\\";

void PrintUsage()
{
    printf_s("Usage: ContentTool <texture count> <texture size> [output path]\n"
             "   texture count is a number between 1 and 1024, inclusive\n"
             "   texture size is a power of 2 number between 1024 and 16384, inclusive\n"
             "   output path (optional) is where the texture data files are written, defaults to current directory\n\n"
             "Note that this tool will consume a lot of memory when generating textures with large (> 4096) sizes.\n");
}

bool ProcessCommandLine(int argc, char** argv)
{
    if (argc < 3)
    {
        PrintUsage();
        return false;
    }

    g_StreamingTextureCount = atoi(argv[1]);
    if (g_StreamingTextureCount == 0 || g_StreamingTextureCount > 1024)
    {
        printf_s("Error: Invalid texture count\n\n");
        PrintUsage();
        return false;
    }

    g_TextureDimension = atoi(argv[2]);
    const bool IsPow2 = ((g_TextureDimension & (g_TextureDimension - 1)) == 0);
    if (g_TextureDimension > 16384 || g_TextureDimension < 1024 || !IsPow2)
    {
        printf_s("Error: Invalid texture dimension\n\n");
        PrintUsage();
        return false;
    }

    if (argc >= 4)
    {
        MultiByteToWideChar(CP_ACP, 0, argv[3], (UINT32)strlen(argv[3]) + 1, g_strTexturePath, ARRAYSIZE(g_strTexturePath));
        wcscat_s(g_strTexturePath, L"\\");
    }

    return true;
}

int main(int argc, char** argv)
{
    bool Success = ProcessCommandLine(argc, argv);
    if (!Success)
    {
        return 1;
    }

    g_Seed = 12345;

    SYSTEM_INFO sysinfo = {};
    GetSystemInfo(&sysinfo);

    JobQueueDesc QueueDesc = {};
    QueueDesc.ThreadCount = sysinfo.dwNumberOfProcessors;
    //QueueDesc.ThreadCount = 1;
    QueueDesc.pJobThreadDataInitFunction = AllocateThreadData;
    QueueDesc.pJobThreadDataFreeFunction = FreeThreadData;
    g_JobQueue.Initialize(&QueueDesc);

    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
    {
        return 1;
    }

    ID3D12Device* m_pd3dDevice = nullptr;

    hr = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_12_0,
        __uuidof(ID3D12Device),
        (void**)&m_pd3dDevice);

    if (FAILED(hr))
    {
        return 1;
    }

    SetTextureBasePath(g_strTexturePath);

    printf_s("Creating %u textures of %u x %u dimensions with %u threads to %S.\n", g_StreamingTextureCount, g_TextureDimension, g_TextureDimension, QueueDesc.ThreadCount, g_strTexturePath);
    printf_s("Press ESC to cancel.\n");

    bool IsGeneratingTextures = GenerateTextures(g_StreamingTextureCount, g_TextureDimension, m_pd3dDevice);

    UINT32 LastTextureCount = -1;
    while (IsGeneratingTextures)
    {
        UINT32 Count = 0;
        UINT32 Total = 0;
        UINT64 Bytes = 0;
        IsGeneratingTextures = !IsTextureGenerationComplete(&Count, &Total, &Bytes);
        if (Count != LastTextureCount)
        {
            printf_s("Generating textures... %u of %u complete (%I64u bytes)\n", Count, Total, Bytes);
            LastTextureCount = Count;
        }
        if (_kbhit() != 0)
        {
            UINT32 Keystroke = _getch();
            if (Keystroke == 27)
            {
                printf_s("Terminating texture generation.\n");
                break;
            }
        }
        Sleep(100);
    }

    m_pd3dDevice->Release();

    if (!IsGeneratingTextures)
    {
        printf_s("Texture generation complete.\n");
    }

    return 0;
}
