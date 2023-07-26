//--------------------------------------------------------------------------------------
// CPUTool.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// If including Windows headers, be sure to set WINAPI_FAMILY_GAMES.
// This restricts API availability to only those that will run on the Xbox title partition successfully.
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
    printf("CPUTool process started.\nThe first command-line parameter (if specified) is interpreted\nas an amount of seconds to sleep.\n");

    // Get current working directory
    const size_t cwdBufferSize = MAX_PATH + 2;
    char cwdBuffer[cwdBufferSize] = {};
    GetCurrentDirectoryA(cwdBufferSize, cwdBuffer);

    printf("The commandline is [%s]\n", GetCommandLineA());
    printf("[%s]: The current working directory is \"%s\"\n", GetCommandLineA(), cwdBuffer);

    fprintf(stderr, "[%s]: This is a test print to stderr.\n", GetCommandLineA());

    fflush(stdout);
    if (argc > 1)
    {
        int sleepSeconds = atoi(argv[1]);
        for (int index = 0; index < sleepSeconds; ++index)
        {
            Sleep(1000);

            printf("[%s]: Slept for %d second(s).\n", GetCommandLineA(), index + 1);
            fflush(stdout);
        }
    }

    printf("[%s]: Process finished. Return value should be 99.\n", GetCommandLineA());

    return 99;
}
