// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "imgui.h"
#include "Narrator.h"

#include <GameInput.h>
#include <sapi.h>
#include "XAccessibility.h"
#include "XSpeechSynthesizer.h"
#include <xaudio2.h>
#include <XAsync.h>
#include <any>

#include <mutex>

#include <sstream>

#include <unordered_map>

#include <XGameUI.h>

// Doesnt support resizeable buffer. It would require syncing resize operations with imgui.
// TODO: Move into a seperate file for better file management
class VirtualKeyboardManager
{
public:
    VirtualKeyboardManager();
    ~VirtualKeyboardManager();
    bool IsKeyboardActive();
    void ActivateKeyboard(const char* buffer);
    void DeactivateKeyboard();

    XGameUiTextEntryHandle keyboardHandle;
};

class ImGuiAcc
{
public:
    // Desktop specific functions
    void SetWindow(HWND window)
    {
        m_hWindow = window;
    }

    void NewFrame();
    void UpdateScaleFactor();
    void AdjustForDisplayScaling();
    void Text(const char* text, ...);
    bool Begin(const char* name, bool* open = NULL, ImGuiWindowFlags flags = 0, ImVec2 size = ImVec2(0, 0));
    void End()
    {
        ImGui::End();
    }

    bool Button(const char* label);
    bool InputText(const char* label, char* buf, size_t bufSize, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* userData = NULL, GameInputKind activeGameInputKind = (GameInputKind)0);
    bool Checkbox(const char* label, bool* isChecked);

    // Format is unused as slider value is place OUTSIDE of the widget
    bool SliderInt(const char* label, int* value, int valueMin, int valueMax, const char* format = "", ImGuiSliderFlags flags = 0);

    // TODO: Delete these when acc files merged
    void EnableNarration() {}
    void DisableNarration() {}

    // Custom widget for this sample, used to handle narrating window switches
    bool WindowHeader(const char* text);

    static ImGuiAcc* GetInstance();

private:
    inline static ImGuiAcc* m_instance;

    ImGuiAcc();
    ~ImGuiAcc();
    void DetectHighContrastTheme();
    void ApplyHighContrastDarkTheme();
    void ApplyHighContrastLightTheme();

    float                                           m_scaleFactor;
    Narrator*                                       m_narrator;


    // Desktop fields
    HWND                                            m_hWindow;
    UINT                                            m_DPI;
    UINT                                            m_lastDPI;

#ifdef _GAMING_XBOX
    VirtualKeyboardManager*                         m_keyboardManager;
#endif
};
