//--------------------------------------------------------------------------------------
// xbgamepad.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "XtfInput.h"
#include "XInputGetStateEx.h"

#include <cstdint>
#include <iostream>

const wchar_t *g_versionString = L"1.4.1.0";

namespace
{
    IXtfInputClient* g_inputClient = nullptr;
    uint64_t g_controllerID = 0;
}

void Cleanup()
{
    if (g_controllerID != 0)
    {
        g_inputClient->DisconnectGamepad(g_controllerID);
    }
}

void CleanupAndExit(int exitCode)
{
    Cleanup();
    exit(exitCode);
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_CLOSE_EVENT:
    {
        Cleanup();
        break;
    }
    case CTRL_BREAK_EVENT:
    case CTRL_C_EVENT:
    {
        CleanupAndExit(0);
    }
    }

    return TRUE;
}

uint16_t XinputButtonsToGamePadButtons(uint16_t wButtons)
{
    uint16_t gpb = 0;

    gpb |= (wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? RIGHT_THUMBSTICK : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? LEFT_THUMBSTICK : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? RIGHT_SHOULDER : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? LEFT_SHOULDER : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? DPAD_RIGHT : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? DPAD_LEFT : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? DPAD_DOWN : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_DPAD_UP) ? DPAD_UP : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_Y) ? Y : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_X) ? X : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_B) ? B : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_A) ? A : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_BACK) ? VIEW : 0;
    gpb |= (wButtons & XINPUT_GAMEPAD_START) ? MENU : 0;
    // Private interface returns guide button
    gpb |= (wButtons & 0x0400 /* XINPUT_GAMEPAD_GUIDE */) ? NEXUS : 0;
    return gpb;
}

GAMEPAD_REPORT XinputStateToGamePadReport(const XINPUT_GAMEPAD& xinput)
{
    GAMEPAD_REPORT gpr;

    gpr.Buttons = XinputButtonsToGamePadButtons(xinput.wButtons);
    gpr.LeftThumbstickX = (abs(xinput.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ? xinput.sThumbLX : 0;
    gpr.LeftThumbstickY = (abs(xinput.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ? xinput.sThumbLY : 0;
    gpr.RightThumbstickX = (abs(xinput.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ? xinput.sThumbRX : 0;
    gpr.RightThumbstickY = (abs(xinput.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) ? xinput.sThumbRY : 0;
    gpr.LeftTrigger = (xinput.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? xinput.bLeftTrigger : 0;
    gpr.RightTrigger = (xinput.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? xinput.bRightTrigger : 0;

    return gpr;
}

void GamePadReportStateString(GAMEPAD_REPORT gpr, wchar_t* output, size_t outputMaxChars)
{
    swprintf_s(output, outputMaxChars, L"Pad state: buttons (%04X), Left Thumb (%05d, %05d), Right Thumb (%05d, %05d), Left Trigger (%05d), Right Trigger (%05d)\n", gpr.Buttons, gpr.LeftThumbstickX, gpr.LeftThumbstickY, gpr.RightThumbstickX, gpr.RightThumbstickY, gpr.LeftTrigger, gpr.RightTrigger);
}

uint32_t GamePadUpdateLoop(int updateRate)
{
    GAMEPAD_REPORT lastGpr;
    memset(&lastGpr, 0, sizeof(GAMEPAD_REPORT));
    CoInitialize(0);

    OutputDebugString(L"GamePadUpdateThread created, wait before first poll...\n");

    Sleep(2000);

    OutputDebugString(L"... wait over, onward!\n");

    // Is update rate supplied?  If not, 100ms sleep = 30hz.
    int sleepTime = updateRate != 0 ? 1000 / updateRate : 33;

    while (1)
    {
        XINPUT_STATE xis = {};
        DWORD result = XInputGetStateEx(0, &xis);
        if (result == ERROR_SUCCESS)
        {
            GAMEPAD_REPORT gpr = XinputStateToGamePadReport(xis.Gamepad);
            //
            // If gamepad state is different, send it.
            //
            if (0 != memcmp(&lastGpr, &gpr, sizeof(GAMEPAD_REPORT)))
            {
                memcpy(&lastGpr, &gpr, sizeof(GAMEPAD_REPORT));

                HRESULT sendgprhr = g_inputClient->SendGamepadReport(g_controllerID, gpr);

                if (sendgprhr != S_OK)
                {
                    wprintf(L"(%08X) Error from SendGamepadReport\n", sendgprhr);
                }
            }
            //
        }

        Sleep(sleepTime);
    }

    return 0;
}

void SetXtfDllPath()
{
    wchar_t xtfdllpath[MAX_PATH] = {};

    // try GDK first..
    GetEnvironmentVariable(L"GameDK", xtfdllpath, MAX_PATH);

    // nothing installed?  Try local copy of dlls.
    if (0 == xtfdllpath[0])
    {
        GetCurrentDirectory(MAX_PATH, xtfdllpath);
    }
    else
    {
        wcscat_s(xtfdllpath, L"bin");
    }
    wprintf(L"Using xtf dlls from %s\n", xtfdllpath);

    SetDllDirectory(xtfdllpath);
}

void ShowUsage()
{
    wprintf(L"Usage:\n\n\txbgamepad /x:<devkit ipv4 address> [/r:<update rate in hz - default is 30>]\n\n\tThis program needs to be able to reach your devkit on TCP port 4211 and 4212.\n");
}

void ShowInfo()
{
    wprintf(L"xbgamepad version %ls\n", g_versionString);
}

int main(int argc, char **argv)
{
    ShowInfo();

    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    //
    // Need at least /x:
    //
    if (argc < 2)
    {
        ShowUsage();
        CleanupAndExit(1);
    }

    wchar_t argIPAddress[32] = {};

    int argUpdateRate = 0;

    for (int i = 0; i < argc; ++i)
    {
        if (strstr(argv[i], "/x") || strstr(argv[i], "/X"))
        {
            // ip address.
            if (argv[i][2] != ':' || strlen(argv[i]) > 20)
            {
                wprintf(L"Malformed /x parameter, usage example: /x:10.5.120.0\n");
                Cleanup();
                exit(1);
            }
            else
            {
                wchar_t *destptr = argIPAddress;
                char* src = argv[i] + 3;

                for (; *src != 0; src++, destptr++)
                {
                    *destptr = *src;
                }
                *destptr = 0;
            }
        }

        if (strstr(argv[i], "/r") || strstr(argv[i], "/R"))
        {
            bool rateParseError = false;

            if (argv[i][2] == ':')
            {
                int rate = atoi(argv[i] + 3);

                if (rate > 0)
                {
                    argUpdateRate = rate;
                }
                else
                {
                    rateParseError = true;
                }
            }
            else
            {
                rateParseError = true;
            }

            if (rateParseError)
            {
                wprintf(L"Malformed /r parameter, usage example: /r:15\n");
                CleanupAndExit(1);
            }
        }
    }

    if (0 == argIPAddress[0])
    {
        wprintf(L"Please supply ip address of devkit, example /x:10.5.120.0\n\n");
        ShowUsage();
        CleanupAndExit(1);
    }

    SetXtfDllPath();

    if (!SetupXInputGetStateEx())
    {
        wprintf(L"Failed initializing XInput1_4.dll\n");
        CleanupAndExit(3);
    }

    // Ok, let's go...
    wprintf(L"Connecting to %s...", argIPAddress);
    HRESULT hr = XtfCreateInputClient(argIPAddress, IID_PPV_ARGS(&g_inputClient));

    if (FAILED(hr))
    {
        wprintf(L"Connection (XtfCreateInputClient) failed with 0x%08X.\n", hr);
        CleanupAndExit(2);
    }
    else
    {
        hr = g_inputClient->ConnectGamepad(&g_controllerID);

        if (SUCCEEDED(hr))
        {
            wprintf(L"Connected.\n\nTips:\n\n\tDisable Xbox Game Bar's use of the Xbox button in Xbox Game Bar's settings/shortcuts section.\n\tDon't run Xbox One Manager on this PC to remote view this devkit - controls may double up.\n\n");
            GamePadUpdateLoop(argUpdateRate);
            Cleanup();
        }
        else
        {
            wprintf(L"Connection (ConnectGamepad) failed with 0x%08X..\n\n\tThis program needs to be able to reach your devkit on TCP ports 4211 and 4212.\n", hr);
            CleanupAndExit(2);
        }
    }
}
