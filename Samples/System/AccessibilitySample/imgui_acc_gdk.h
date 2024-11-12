#pragma once
#include "imgui.h"
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

// Used to manage narration state for GDK
class Narrator
{
public:
    Narrator(XTaskQueueHandle taskqueue);
    ~Narrator();
    void AddPendingNarration(const char* newValue);

    // Once a pending narration occurs, a thread is spun up to manage
    // narration. Once there is no more new content to narrate, the thread is stopped.
    void NarrationManagementThread();

    void EnableNarration();
    void DisableNarration();
private:
    std::string pendingNarration;
    std::string lastCompleteNarration;
    std::string activeNarration;
    std::string currentWidgetValue;

    std::mutex narrationMutex;
    std::atomic<bool> managementThreadActive;

    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasterVoice;
    IXAudio2SourceVoice* pSourceVoice;

    WAVEFORMATEX waveFormat;

    bool isManagingCOMLibrary;
    bool keepNarrationThreadAlive;
    bool isNarrationEnabled;
    const static int DELAYMS = 100;
};

// Doesnt support resizeable buffer. It would require syncing resize operations with imgui.
class VirtualKeyboardManager
{
public:
    VirtualKeyboardManager();
    ~VirtualKeyboardManager();
    bool IsKeyboardActive();
    void ActivateKeyboard(const char* buffer);
    void DeactivateKeyboard();

    XGameUiTextEntryHandle keyboardHandle;
private:
};

const static int MAXBUFFERSIZE = 256;
const static ImVec2 MINWIDGETSIZE = { 30.0f, 30.0f };

class imgui_acc_gdk
{
public:
    // Get the instance of imgui_acc_gdk
    static imgui_acc_gdk* GetInstance();

    // Clean up the instance of imgui_acc_gdk
    static void CleanUp();

    // Initialize the instance of imgui_acc_gdk
    HRESULT Initialize();

    void NewFrame();
    void AdjustForDisplayScaling();
    void Text(const char* text, ...);
    bool Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0, ImVec2 size = ImVec2(0, 0));
    void End();

    // size_arg is unused as the size will be automatically calculated depending on MINWIDGETSIZE
    bool Button(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
    bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, GameInputKind activeGameInputKind = (GameInputKind)0);
    bool Checkbox(const char* label, bool* v);

    // format is unused as slider value is place OUTSIDE of the widget
    bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "", ImGuiSliderFlags flags = 0);

    void EnableNarration();
    void DisableNarration();
    // Custom widget for this sample, used to handle narrating window switches
    bool WindowHeader(const char* text);

    inline static imgui_acc_gdk* instance;
private:
    imgui_acc_gdk();
    ~imgui_acc_gdk();
    void DetectHighContrastTheme();
    void ApplyHighContrastDarkTheme();
    void ApplyHighContrastLightTheme();

    float                                           m_scaleFactor;
    Narrator*                                       m_narrator;
    VirtualKeyboardManager*                         m_keyboardManager;
    XTaskQueueHandle                                m_taskQueue;
};
