//--------------------------------------------------------------------------------------
// GamepadKeyboardMouse.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "TextConsole.h"

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#endif

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:
    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    struct CursorInput
    {
        bool CursorInputSwitch;
        float CursorInputAxisX;
        float CursorInputAxisY;
        int CursorInputRotation;
        int CursorInputMouseX;
        int CursorInputMouseY;
    };

    struct CursorState
    {
        int xPos;
        int yPos;
        int degreeRotation;
        bool isRelativeMode;
        bool cursorVisible;

        CursorState(int x, int y) {
            xPos = x; yPos = y; degreeRotation = 0; isRelativeMode = true; cursorVisible = true;
        };
    };

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Callbacks
    static void CALLBACK DeviceCallback(
        _In_ GameInputCallbackToken callbackToken,
        _In_ void* context,
        _In_ IGameInputDevice* device,
        _In_ uint64_t timestamp,
        _In_ GameInputDeviceStatus currentStatus,
        _In_ GameInputDeviceStatus previousStatus) noexcept;

    // Cursor Logic
    void ProcessInput();
    float DegreesToRad(int degrees) { return (float)degrees * (3.1415f / 180.f); };

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    int             m_screenLocation_x = 0;
    int             m_screenLocation_y = 0;

private:
    void Update();
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    Microsoft::WRL::ComPtr<IGameInput>          m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_prevReading;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    GameInputCallbackToken                      deviceCallbackToken;

    CursorState     m_cursorState;
    CursorInput     m_cursorInput;
    CursorInput     m_lastCursorInput;

    //Input states
    std::wstring    m_buttonString;
    std::wstring    m_keyboardString;
    std::wstring    m_mouseGiString;
    std::wstring    m_mouseMsgString;
    float           m_leftTrigger;
    float           m_rightTrigger;
    float           m_leftStickX;
    float           m_leftStickY;
    float           m_rightStickX;
    float           m_rightStickY;

    static constexpr int moveSpeed = 6;

    static constexpr int minX = 700;
    static constexpr int maxX = 1800;
    static constexpr int minY = 225;
    static constexpr int maxY = 895;
    static constexpr int cursorSize = 80;

    // List for virtual key name display
    static constexpr wchar_t const* const c_vkeyNames[256] =
    {
        L"VK_NONE", L"VK_LBUTTON", L"VK_RBUTTON", L"VK_CANCEL", L"VK_MBUTTON", L"VK_XBUTTON1", L"VK_XBUTTON2", L"VK_NEXUS", L"VK_BACK",
        L"VK_TAB", L"(reserved VK)", L"(reserved VK)", L"VK_CLEAR", L"VK_RETURN", L"(unassigned VK)", L"(unassigned VK)", L"VK_SHIFT",
        L"VK_CONTROL", L"VK_MENU", L"VK_PAUSE", L"VK_CAPITAL", L"VK_KANA", L"VK_IME_ON", L"VK_JUNJA", L"VK_FINAL", L"VK_HANJA",
        L"VK_IME_OFF", L"VK_ESCAPE", L"VK_CONVERT", L"VK_NONCONVERT", L"VK_ACCEPT", L"VK_MODECHANGE", L"VK_SPACE", L"VK_PRIOR",
        L"VK_NEXT", L"VK_END", L"VK_HOME", L"VK_LEFT", L"VK_UP", L"VK_RIGHT", L"VK_DOWN", L"VK_SELECT", L"VK_PRINT", L"VK_EXECUTE",
        L"VK_SNAPSHOT", L"VK_INSERT", L"VK_DELETE", L"VK_HELP", L"VK_0", L"VK_1", L"VK_2", L"VK_3", L"VK_4", L"VK_5", L"VK_6", L"VK_7",
        L"VK_8", L"VK_9", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)",
        L"(unassigned VK)", L"(unassigned VK)", L"VK_A", L"VK_B", L"VK_C", L"VK_D", L"VK_E", L"VK_F", L"VK_G", L"VK_H", L"VK_I", L"VK_J",
        L"VK_K", L"VK_L", L"VK_M", L"VK_N", L"VK_O", L"VK_P", L"VK_Q", L"VK_R", L"VK_S", L"VK_T", L"VK_U", L"VK_V", L"VK_W", L"VK_X", L"VK_Y",
        L"VK_Z", L"VK_LWIN", L"VK_RWIN", L"VK_APPS", L"(reserved VK)", L"VK_SLEEP", L"VK_NUMPAD0", L"VK_NUMPAD1", L"VK_NUMPAD2",
        L"VK_NUMPAD3", L"VK_NUMPAD4", L"VK_NUMPAD5", L"VK_NUMPAD6", L"VK_NUMPAD7", L"VK_NUMPAD8", L"VK_NUMPAD9", L"VK_MULTIPLY",
        L"VK_ADD", L"VK_SEPARATOR", L"VK_SUBTRACT", L"VK_DECIMAL", L"VK_DIVIDE", L"VK_F1", L"VK_F2", L"VK_F3", L"VK_F4", L"VK_F5",
        L"VK_F6", L"VK_F7", L"VK_F8", L"VK_F9", L"VK_F10", L"VK_F11", L"VK_F12", L"VK_F13", L"VK_F14", L"VK_F15", L"VK_F16", L"VK_F17",
        L"VK_F18", L"VK_F19", L"VK_F20", L"VK_F21", L"VK_F22", L"VK_F23", L"VK_F24", L"VK_NAVIGATION_VIEW", L"VK_NAVIGATION_MENU",
        L"VK_NAVIGATION_UP", L"VK_NAVIGATION_DOWN", L"VK_NAVIGATION_LEFT", L"VK_NAVIGATION_RIGHT", L"VK_NAVIGATION_ACCEPT",
        L"VK_NAVIGATION_CANCEL", L"VK_NUMLOCK", L"VK_SCROLL", L"VK_OEM_NEC_EQUAL", L"VK_OEM_FJ_MASSHOU", L"VK_OEM_FJ_TOUROKU",
        L"VK_OEM_FJ_LOYA", L"VK_OEM_FJ_ROYA", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)",
        L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"(unassigned VK)", L"VK_LSHIFT",
        L"VK_RSHIFT", L"VK_LCONTROL", L"VK_RCONTROL", L"VK_LMENU", L"VK_RMENU", L"VK_BROWSER_BACK", L"VK_BROWSER_FORWARD",
        L"VK_BROWSER_REFRESH", L"VK_BROWSER_STOP", L"VK_BROWSER_SEARCH", L"VK_BROWSER_FAVORITES", L"VK_BROWSER_HOME",
        L"VK_VOLUME_MUTE", L"VK_VOLUME_DOWN", L"VK_VOLUME_UP", L"VK_MEDIA_NEXT_TRACK", L"VK_MEDIA_PREV_TRACK", L"VK_MEDIA_STOP",
        L"VK_MEDIA_PLAY_PAUSE", L"VK_LAUNCH_MAIL", L"VK_LAUNCH_MEDIA_SELECT", L"VK_LAUNCH_APP1", L"VK_LAUNCH_APP2", L"(reserved VK)",
        L"(reserved VK)", L"VK_OEM_1", L"VK_OEM_PLUS", L"VK_OEM_COMMA", L"VK_OEM_MINUS", L"VK_OEM_PERIOD", L"VK_OEM_2", L"VK_OEM_3",
        L"(reserved VK)", L"(reserved VK)", L"VK_GAMEPAD_A", L"VK_GAMEPAD_B", L"VK_GAMEPAD_X", L"VK_GAMEPAD_Y",
        L"VK_GAMEPAD_RIGHT_SHOULDER", L"VK_GAMEPAD_LEFT_SHOULDER", L"VK_GAMEPAD_LEFT_TRIGGER", L"VK_GAMEPAD_RIGHT_TRIGGER",
        L"VK_GAMEPAD_DPAD_UP", L"VK_GAMEPAD_DPAD_DOWN", L"VK_GAMEPAD_DPAD_LEFT", L"VK_GAMEPAD_DPAD_RIGHT", L"VK_GAMEPAD_MENU",
        L"VK_GAMEPAD_VIEW", L"VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON", L"VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON",
        L"VK_GAMEPAD_LEFT_THUMBSTICK_UP", L"VK_GAMEPAD_LEFT_THUMBSTICK_DOWN", L"VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT",
        L"VK_GAMEPAD_LEFT_THUMBSTICK_LEFT", L"VK_GAMEPAD_RIGHT_THUMBSTICK_UP", L"VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN",
        L"VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT", L"VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT", L"VK_OEM_4", L"VK_OEM_5", L"VK_OEM_6",
        L"VK_OEM_7", L"VK_OEM_8", L"(reserved VK)", L"VK_OEM_AX", L"VK_OEM_102", L"VK_ICO_HELP", L"VK_ICO_00", L"VK_PROCESSKEY",
        L"VK_ICO_CLEAR", L"VK_PACKET", L"(unassigned VK)", L"VK_OEM_RESET", L"VK_OEM_JUMP", L"VK_OEM_PA1", L"VK_OEM_PA2",
        L"VK_OEM_PA3", L"VK_OEM_WSCTRL", L"VK_OEM_CUSEL", L"VK_OEM_ATTN", L"VK_OEM_FINISH", L"VK_OEM_COPY", L"VK_OEM_AUTO",
        L"VK_OEM_ENLW", L"VK_OEM_BACKTAB", L"VK_ATTN", L"VK_CRSEL", L"VK_EXSEL", L"VK_EREOF", L"VK_PLAY", L"VK_ZOOM", L"VK_NONAME",
        L"VK_PA1", L"VK_OEM_CLEAR", L"VK_UNKNOWN"
    };

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_cursor;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_pointer;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_logo;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    std::unique_ptr<DX::TextConsoleImage>       m_logConsole;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors : size_t
    {
        PrintFont,
        TextFont,
        ControllerFont,
        ConsoleFont,
        ConsoleBackground,
        Background,
        Cursor,
        Pointer,
        Logo,
        Count,
    };
};
