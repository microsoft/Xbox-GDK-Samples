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
    printf("CPUTool process started.\nThe first command-line parameter (if specified) is interpreted as an amount of seconds to sleep.\n");

    // Get current working directory
    const size_t cwdBufferSize = MAX_PATH + 2;
    char cwdBuffer[cwdBufferSize] = {};
    GetCurrentDirectoryA(cwdBufferSize, cwdBuffer);

    printf("The current working directory is \"%s\"\n", cwdBuffer);
    printf("The commandline is [%s]\n", GetCommandLineA());

    fprintf(stderr, "This is a test print to stderr.\n");

    fflush(stdout);
    if (argc > 1)
    {
        int sleepSeconds = atoi(argv[1]);
        for (int index = 0; index < sleepSeconds; ++index)
        {
            Sleep(1000);

            printf("Slept for %d second(s).\n", index + 1);
            fflush(stdout);
        }
    }

    printf("Process finished. Return value should be 99.\n");

    return 99;
}
