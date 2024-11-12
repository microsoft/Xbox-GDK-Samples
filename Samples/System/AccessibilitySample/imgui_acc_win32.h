#pragma once
#include "imgui.h"
#include <any>
#include <unordered_map>

#include <UIAutomation.h>
#include <sapi.h>
#include <sstream>
#include <mutex>
#include <algorithm>

#include <windows.h>
#include <thread>
#include <atomic>

// Used to manage narration state for Win32
class Narrator
{
public:
    Narrator();
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
    std::thread narrationThread;
    std::thread managementThread;
    std::atomic<bool> managementThreadActive;
    ISpVoice* pVoice;

    bool isManagingCOMLibrary;
    bool isNarrationEnabled;
    const static int DELAYMS = 100;
};

const static int MAXBUFFERSIZE = 256;
const inline static ImVec2 MINWIDGETSIZE = { 30.0f, 30.0f };

class imgui_acc_win32
{
public:
    static imgui_acc_win32* GetInstance();
    static void CleanUp();
    void Initialize(HWND window);
    void NewFrame();
    void UpdateScaleFactor();

    void EnableNarration();
    void DisableNarration();
    struct WidgetState
    {
        std::string name;
        std::any value;

        bool (*comparator)(const std::any&, const std::any&);
        std::string(*valueToString)(const std::any&);

        // Function to call the provided toString function
        std::string toString() const
        {
            std::ostringstream oss;
            oss << name << " " << valueToString(value);
            return oss.str();
        }

        // Function to call the provided comparator function
        bool compare(const std::any& otherValue) const
        {
            if (comparator)
            {
                return comparator(value, otherValue);
            }
            return false;
        }
    };

    void AdjustForDisplayScaling();

    void Text(const char* text);
    bool Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0, ImVec2 size = ImVec2(0, 0));
    void End();

    // size_arg is unused as the size will be automatically calculated depending on MINWIDGETSIZE
    bool Button(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
    bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    bool Checkbox(const char* label, bool* v);

    // format is unused as slider value is place OUTSIDE of the widget
    bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "", ImGuiSliderFlags flags = 0);

    // Custom widget for this sample, used to handle narrating window switches
    bool WindowHeader(const char* text);

    inline static imgui_acc_win32* instance;

private:
    imgui_acc_win32();
    ~imgui_acc_win32();
    void DetectHighContrastTheme();
    void ApplyHighContrastDarkTheme();
    void ApplyHighContrastLightTheme();

    void Narrate(const wchar_t* text);

    std::mutex narrationMutex;

    ISpVoice* m_pVoice;
    UINT m_DPI;
    UINT m_lastDPI;
    float m_scaleFactor;
    HWND m_hWindow;
    Narrator* m_narrator;
};

