#include "precomp.hpp"

#include "win32app.h"
#include "d3d12app.hpp"

struct TranslationTableEntry
{
    UINT32 vkey;
    GamepadButtons button;
    UINT32 StickButton;
};

static const TranslationTableEntry g_KeyTranslationTable[] =
{
    { VK_RETURN,      GamepadButtons::A,              0x00 },
    { 'B',            GamepadButtons::B,              0x00 },
    { 'X',            GamepadButtons::X,              0x00 },
    { 'Y',            GamepadButtons::Y,              0x00 },
    { VK_LEFT ,       GamepadButtons::DPadLeft,       0x00 },
    { VK_RIGHT,       GamepadButtons::DPadRight,      0x00 },
    { VK_UP   ,       GamepadButtons::DPadUp,         0x00 },
    { VK_DOWN ,       GamepadButtons::DPadDown,       0x00 },
    { VK_ESCAPE,      GamepadButtons::View,           0x00 },
    { VK_HOME,        GamepadButtons::Menu,           0x00 },
    { '9',            GamepadButtons::LeftShoulder,   0x00 },
    { '0',            GamepadButtons::RightShoulder,  0x00 },
    { 'A',            GamepadButtons::None,           0x01 },
    { 'S',            GamepadButtons::None,           0x04 },
    { 'W',            GamepadButtons::None,           0x08 },
    { 'D',            GamepadButtons::None,           0x02 },
    { VK_SHIFT,       GamepadButtons::RightShoulder,  0x00 },
};

void TranslateKey(D3D12App* pApp, UINT message, WPARAM wParam, LPARAM lParam)
{
    const bool IsKeyDown = (message == WM_KEYDOWN);
    for (UINT32 i = 0; i < ARRAYSIZE(g_KeyTranslationTable); ++i)
    {
        const TranslationTableEntry& Entry = g_KeyTranslationTable[i];
        if (wParam == Entry.vkey)
        {
            pApp->SetTranslatedButtons(IsKeyDown, (UINT32)Entry.button, Entry.StickButton);
            break;
        }
    }
}

void TranslateGamepad(RawGamepadReading* pReading, const _XINPUT_STATE* pXGamepad)
{
    pReading->LeftThumbstickX = (FLOAT)pXGamepad->Gamepad.sThumbLX / 32768.0f;
    pReading->LeftThumbstickY = (FLOAT)pXGamepad->Gamepad.sThumbLY / 32768.0f;
    pReading->RightThumbstickX = (FLOAT)pXGamepad->Gamepad.sThumbRX / 32768.0f;
    pReading->RightThumbstickY = (FLOAT)pXGamepad->Gamepad.sThumbRY / 32768.0f;
    pReading->LeftTrigger = (FLOAT)pXGamepad->Gamepad.bLeftTrigger / 255.0f;
    pReading->RightTrigger = (FLOAT)pXGamepad->Gamepad.bRightTrigger / 255.0f;
    UINT32 NewButtons = 0;
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { NewButtons |= (UINT32)GamepadButtons::DPadUp; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { NewButtons |= (UINT32)GamepadButtons::DPadDown; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { NewButtons |= (UINT32)GamepadButtons::DPadLeft; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { NewButtons |= (UINT32)GamepadButtons::DPadRight; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_A) { NewButtons |= (UINT32)GamepadButtons::A; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_B) { NewButtons |= (UINT32)GamepadButtons::B; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_X) { NewButtons |= (UINT32)GamepadButtons::X; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_Y) { NewButtons |= (UINT32)GamepadButtons::Y; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) { NewButtons |= (UINT32)GamepadButtons::LeftThumbstick; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) { NewButtons |= (UINT32)GamepadButtons::LeftShoulder; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) { NewButtons |= (UINT32)GamepadButtons::RightThumbstick; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) { NewButtons |= (UINT32)GamepadButtons::RightShoulder; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_START) { NewButtons |= (UINT32)GamepadButtons::Menu; }
    if (pXGamepad->Gamepad.wButtons & XINPUT_GAMEPAD_BACK) { NewButtons |= (UINT32)GamepadButtons::View; }
    pReading->Buttons = (GamepadButtons)NewButtons;
}

#if !defined(_GAMING_XBOX)

void TranslateMouse(D3D12App* pApp, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HCURSOR s_SavedCursor = NULL;
    static bool s_LeftMouseCaptured = false;
    static INT s_LastXPos = 0;
    static INT s_LastYPos = 0;

    switch (message)
    {
    case WM_LBUTTONDOWN:
    {
        HWND hCaptured = GetCapture();
        if (hCaptured == nullptr)
        {
            s_LeftMouseCaptured = true;
            SetCapture(hWnd);
            s_SavedCursor = GetCursor();
            SetCursor(NULL);
            s_LastXPos = (INT)LOWORD(lParam);
            s_LastYPos = (INT)HIWORD(lParam);
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (s_LeftMouseCaptured)
        {
            SetCursor(s_SavedCursor);
            s_SavedCursor = NULL;
            SetCapture(nullptr);
            s_LeftMouseCaptured = false;
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (s_LeftMouseCaptured)
        {
            const INT XPos = (INT)LOWORD(lParam);
            const INT YPos = (INT)HIWORD(lParam);
            pApp->OnMouseDelta(XPos - s_LastXPos, YPos - s_LastYPos);
            s_LastXPos = XPos;
            s_LastYPos = YPos;
        }
        break;
    }
}

#endif