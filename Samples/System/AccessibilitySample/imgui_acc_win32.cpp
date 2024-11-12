#include "pch.h"
#include "imgui_acc_win32.h"

#pragma region Narrator
Narrator::Narrator() : managementThreadActive(false), pVoice(nullptr), isManagingCOMLibrary(false), isNarrationEnabled(true)
{
    // Initialize COM library if not already initialized
    HRESULT hr = CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
    if (!FAILED(hr))
    {
        isManagingCOMLibrary = true;
    }
    else if (hr != S_FALSE)
    {
        throw std::runtime_error("COM Library failed to initialize");
        return;
    }
    hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create instance");
        return;
    }
}

Narrator::~Narrator()
{
    // Signal the thread to stop
    managementThreadActive.store(false);

    // Ensure the thread is properly joined
    if (managementThread.joinable())
    {
        managementThread.join();
    }

    if (pVoice)
    {
        pVoice->Release();
        pVoice = nullptr;
    }

    if (isManagingCOMLibrary)
    {
        CoUninitialize();
    }
}

void Narrator::AddPendingNarration(const char* newValue)
{
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        currentWidgetValue = newValue;
    }

    if (!managementThreadActive.load())
    {
        // managementThread completed, so it should be safe to detach the thread.
        if (managementThread.joinable())
        {
            managementThread.detach();
        }

        // Spin up a new thread to manage narration
        if (currentWidgetValue != lastCompleteNarration)
        {
            managementThreadActive.store(true);
            managementThread = std::thread(&Narrator::NarrationManagementThread, this);
        }
    }
}

void Narrator::EnableNarration()
{
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        isNarrationEnabled = true;
    }
}

void Narrator::DisableNarration()
{
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        isNarrationEnabled = false;
    }
}

// A serial thread run asynchronously to manage the narration. Only one is active at at time.
void Narrator::NarrationManagementThread()
{
    while (currentWidgetValue != lastCompleteNarration)
    {
        {
            std::lock_guard<std::mutex> lock(narrationMutex);
            pendingNarration = currentWidgetValue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(Narrator::DELAYMS));

        narrationMutex.lock();
        if (isNarrationEnabled)
        {
            // If the currentWidgetValue was not changed while sleeping,
            // then we can start the narration.
            if (currentWidgetValue == pendingNarration)
            {
                wchar_t wNarration[MAXBUFFERSIZE];
                mbstowcs_s(nullptr, wNarration, MAXBUFFERSIZE, pendingNarration.c_str(), MAXBUFFERSIZE);
                narrationMutex.unlock();
                if (pVoice)
                {
                    // Stop any ongoing speech and start the new narration
                    // IspVoice has its own thread management, so we can just
                    // call Speak with the new text and it will interrupt the ongoing speech.
                    pVoice->Speak(wNarration, SPF_PURGEBEFORESPEAK | SPF_ASYNC | SPF_IS_NOT_XML, nullptr);
                    lastCompleteNarration = pendingNarration;
                }
            }
            else
            {
                narrationMutex.unlock();
            }
        }
        else
        {
            // Narration is disabled, so purge any active narration
            if (pVoice)
            {
                pVoice->Speak(L"", SPF_PURGEBEFORESPEAK | SPF_ASYNC | SPF_IS_NOT_XML, nullptr);
                lastCompleteNarration = pendingNarration;
            }
            narrationMutex.unlock();
        }
    }
    managementThreadActive.store(false); // No longer active after narration
}
#pragma endregion

#pragma region Base class
imgui_acc_win32::imgui_acc_win32():
    m_pVoice(nullptr),
    m_DPI(0),
    m_lastDPI(0),
    m_scaleFactor(1.0f),
    m_hWindow(nullptr),
    m_narrator(nullptr)
{
}

imgui_acc_win32::~imgui_acc_win32()
{
    if(m_narrator)
        delete m_narrator;
}

imgui_acc_win32* imgui_acc_win32::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new imgui_acc_win32();
    }

    return instance;
}
void imgui_acc_win32::CleanUp()
{
    if(instance)
        delete instance;
    instance = nullptr;
}

void imgui_acc_win32::Initialize(HWND window)
{
    m_hWindow = window;
    m_narrator = new Narrator();
    DetectHighContrastTheme();
    UpdateScaleFactor();
}

void imgui_acc_win32::NewFrame()
{
    UpdateScaleFactor();
    DetectHighContrastTheme();
}

#pragma endregion

#pragma region Imgui Widget Wrappers
bool imgui_acc_win32::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags, ImVec2 size)
{
    ImGui::SetNextWindowSize(size);

    if (!ImGui::Begin(name, p_open, flags | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        return false;

    // Enable horizontal scrolling
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && !ImGui::IsAnyItemActive())
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() - 10.0f);
        }
        if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() + 10.0f);
        }
    }

    return true;
}

void imgui_acc_win32::End()
{
    ImGui::End();
}

void imgui_acc_win32::Text(const char* text)
{
    // ImGui::Text is not interactable at all, and for the purposes of narration, not useable.
    // Selectable is used in its place instead.
    ImGui::Selectable(text);
    if (ImGui::IsItemFocused())
    {
        m_narrator->AddPendingNarration(text);
    }
}

bool imgui_acc_win32::Button(const char* label, const ImVec2&)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the size ImGui would normally use
    ImVec2 size =
    {
        ImGui::CalcTextSize(label, NULL, true).x + style.FramePadding.x * 2.0f,
        ImGui::CalcTextSize(label, NULL, true).y + style.FramePadding.y * 2.0f
    };

    // Enforce minimum size
    ImVec2 adjusted_size;
    adjusted_size.x = std::max(size.x, MINWIDGETSIZE.x);
    adjusted_size.y = std::max(size.y, MINWIDGETSIZE.y);

    bool pressed = ImGui::Button(label, adjusted_size);

    if (ImGui::IsItemFocused())
    {
        m_narrator->AddPendingNarration(label);
    }

    char buffer[MAXBUFFERSIZE] = {};
    if (ImGui::IsItemDeactivated())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %s\n", label, "pressed.");
        m_narrator->AddPendingNarration(buffer);
    }
    return pressed;
}

bool imgui_acc_win32::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    ImGuiStyle& style = ImGui::GetStyle();
    // Calculate the default size
    ImVec2 text_size = ImGui::CalcTextSize(buf, NULL, true);
    ImVec2 total_size;
    total_size.x = text_size.x + style.FramePadding.x * 2.0f;
    total_size.y = text_size.y + style.FramePadding.y * 2.0f;

    // Enforce minimum size
    ImVec2 size_diff;
    size_diff.x = std::max(MINWIDGETSIZE.x - total_size.x, 0.0f);
    size_diff.y = std::max(MINWIDGETSIZE.y - total_size.y, 0.0f);

    // Adjust frame padding if necessary
    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImVec2 padding = { 0,0 };
        padding.x = style.FramePadding.x + size_diff.x / 2.0f;
        padding.y = style.FramePadding.y + size_diff.y / 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool valueChanged = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);

    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImGui::PopStyleVar();
    }

    char buffer[MAXBUFFERSIZE] = {};
    if (ImGui::IsItemActive())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"Editing %s %s\n", label, buf);
        m_narrator->AddPendingNarration(buffer);
    }
    else if (ImGui::IsItemFocused())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %s\n", label, buf);
        m_narrator->AddPendingNarration(buffer);
    }

    return valueChanged;
}

bool imgui_acc_win32::Checkbox(const char* label, bool* v)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the total size of the checkbox and label
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    float square_sz = ImGui::GetFrameHeight(); // Size of the checkbox square
    ImVec2 total_size;
    total_size.x = square_sz + style.ItemInnerSpacing.x + label_size.x;
    total_size.y = std::max(square_sz, label_size.y);

    // Enforce minimum size
    ImVec2 size_diff;
    size_diff.x = std::max(MINWIDGETSIZE.x - total_size.x, 0.0f);
    size_diff.y = std::max(MINWIDGETSIZE.y - total_size.y, 0.0f);

    // Push style vars if adjustments are needed as imgui checkbox doesn't have a size parameter
    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImVec2 padding = { 0,0 };
        padding.x = style.FramePadding.x + size_diff.x / 2.0f;
        padding.y = style.FramePadding.y + size_diff.y / 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool pressed = ImGui::Checkbox(label, v);

    // Pop style vars if they were pushed
    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImGui::PopStyleVar();
    }

    char buffer[MAXBUFFERSIZE] = {};

    if (ImGui::IsItemFocused() || pressed)
    {
        if (*v)
            sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %s\n", label, "checked.");
        else
            sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %s\n", label, "unchecked.");

        m_narrator->AddPendingNarration(buffer);
    }
    return pressed;
}

bool imgui_acc_win32::SliderInt(const char* label, int* v, int v_min, int v_max, const char*, ImGuiSliderFlags flags)
{
    ImGui::BeginGroup();
    float slider_height = ImGui::GetFrameHeight();

    if (slider_height < MINWIDGETSIZE.y)
    {
        float padding_increase = (MINWIDGETSIZE.y - slider_height) / 2.0f;

        ImVec2 padding = ImGui::GetStyle().FramePadding;
        padding.y += padding_increase;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool result = ImGui::SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, "", flags | ImGuiSliderFlags_NoInput);

    char buffer[MAXBUFFERSIZE] = {};
    if (ImGui::IsItemActive())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"Editing %s %d\n", label, *v);
        m_narrator->AddPendingNarration(buffer);
    }
    else if (ImGui::IsItemFocused())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %d\n", label, *v);
        m_narrator->AddPendingNarration(buffer);
    }

    // Pop style var if it was pushed
    if (slider_height < MINWIDGETSIZE.y)
    {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    ImGui::Text(": %d", *v);
    ImGui::EndGroup();

    return result;
}

bool imgui_acc_win32::WindowHeader(const char* text)
{
    // Create a unique ID for the selectable item based on the text
    ImGui::PushID(text);

    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // Calculate the position to start the text so it is centered
    float textPosX = (windowSize.x - textSize.x) * 0.5f;

    // Force selection to start on left side of window
    bool isSelected = ImGui::Selectable("");

    // Check if this widget is focused and trigger narration
    if (ImGui::IsItemFocused())
    {
        char buffer[MAXBUFFERSIZE] = {};
        sprintf_s(buffer, MAXBUFFERSIZE, u8"%s window focused.\n", text);
        m_narrator->AddPendingNarration(buffer);
    }

    ImGui::SameLine();

    // Add text to the center of the line
    ImGui::SetCursorPosX(textPosX);
    ImGui::Text("%s", text);
    ImGui::Separator();
    ImGui::PopID();

    return isSelected;
}

void imgui_acc_win32::EnableNarration()
{
    m_narrator->EnableNarration();
}

void imgui_acc_win32::DisableNarration()
{
    m_narrator->DisableNarration();
}
#pragma endregion

#pragma region Accessibility
void imgui_acc_win32::ApplyHighContrastDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    //GPT attempt at Aquatic theme with slight modifications for accessiblity purposes
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);                // White text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);        // Grey text (disabled)
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);            // Dark background
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);             // Dark background
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);             // Very dark background
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);              // Grey border
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);        // No border shadow
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);            // Dark blue-grey background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);        // Medium Blue on MOUSE hover #263B50 for header
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);       // Slightly lighter blue-grey
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);             // Dark blue-grey background for title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.20f, 0.25f, 1.00f);       // Slightly lighter for active title
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);    // Same as window background for collapsed title
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);           // Dark blue-grey background for menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);        // Dark blue-grey background for scrollbar
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Lighter grey when active
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);            // White for check mark CHANGE THIS to WHITE
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);         // White slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);    // Lighter blue when active #8EE3F0
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);             // Dark blue-grey background for button #202020
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);      // Medium Blue on hover
    colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);         // Lighter blue when active
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);             // Dark blue-grey background for header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);        // Medium Blue on MOUSE hover #263B50 for header
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);        // Lighter blue when active
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);           // Grey separator
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);    // Lighter grey on hover
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.71f, 1.00f);     // Even lighter grey when active
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);          // Blue resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);   // Lighter blue on hover
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.67f, 1.00f, 1.00f);    // Even lighter blue when active
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);                // Dark blue-grey background for tab
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);          // White on hover
    colors[ImGuiCol_TabActive] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);           // Lighter blue for active tab
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);       // Dark blue-grey background for unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);  // Blue for active unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);           // Grey plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);    // Orange plot lines on hover
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);       // Grey plot histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);// Orange plot histogram on hover
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);      // Dark blue-grey background for table header
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);   // Grey for strong table border
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);    // Lighter grey for light table border
    colors[ImGuiCol_TableRowBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);          // Dark background for table row background (even rows)
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);      // Dark blue-grey background for table row background (odd rows)
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);    // Medium Blue background for selected text #263B50
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);      // Lighter blue for drag drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);        // White for navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // White for windowing highlight with transparency
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.25f, 0.29f, 0.20f);   // Dark blue-grey background with transparency for dim background
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.25f, 0.29f, 0.35f);    // Dark blue-grey background with transparency for modal dim background
}

void imgui_acc_win32::ApplyHighContrastLightTheme()
{
    //GPT attempt at Desert theme modified to match it better
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);                // Dark grey text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);        // Grey text (disabled)
    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);            // Light grey background
    colors[ImGuiCol_ChildBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);             // Light grey background
    colors[ImGuiCol_PopupBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background
    colors[ImGuiCol_Border] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);              // Dark brown border
    colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.50f);        // Dark grey shadow with transparency
    colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);      // Lighter shade on hover
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);       // Same as FrameBg

    colors[ImGuiCol_TitleBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background for title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);       // Slightly darker for active title
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);    // Same as window background for collapsed title
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);           // Light grey background for menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);         // Light background for scrollbar
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.30f, 0.10f, 1.00f); // Dark brown when active
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);           // Dark brown for check mark
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);          // Grey slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);    // Dark brown when active
    colors[ImGuiCol_Button] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);              // Light background for button
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);       // Lighter shade on hover
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);        // Dark brown when active
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);              // Light background for header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);       // Lighter shade on hover
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);        // Dark brown when active
    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);           // Grey separator
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter on hover
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);     // Dark brown when active
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);          // Grey resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);   // Slightly lighter on hover
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);    // Dark brown when active
    colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);                 // Light background for tab
    colors[ImGuiCol_TabHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);          // Lighter shade on hover
    colors[ImGuiCol_TabActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);           // Slightly darker for active tab
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);        // Light background for unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);  // Slightly darker for active unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);           // Grey plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter on hover
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey plot histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);       // Light background for table header
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);   // Grey for strong table border
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter for light table border

    colors[ImGuiCol_TableRowBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);         // Light grey for table row background (even rows)
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);      // Slightly darker for table row background (odd rows)
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);     // Light background for selected text
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);     // Slightly darker for drag drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);       // Dark brown for navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f); // Slightly darker for windowing highlight
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.75f, 0.50f);  // Light background with transparency for dim background
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.75f, 0.50f);   // Light background with transparency for modal dim background
}

void imgui_acc_win32::UpdateScaleFactor()
{
    // Get DPI 
    m_DPI = GetDpiForWindow(m_hWindow);
    if (m_DPI != m_lastDPI)
    {
        m_lastDPI = m_DPI;
        m_scaleFactor = m_DPI / 96.0f;
        AdjustForDisplayScaling();
    }
}

void imgui_acc_win32::AdjustForDisplayScaling()
{       
    ImGui::GetStyle().ScaleAllSizes(m_scaleFactor);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = m_scaleFactor;
}

void imgui_acc_win32::DetectHighContrastTheme()
{
    HIGHCONTRAST highContrast = {};
    highContrast.cbSize = sizeof(HIGHCONTRAST);

    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &highContrast, 0))
    {
        auto isColorLight = [](COLORREF color)
        {
            uint32_t r = color & 0x000000FF;
            uint32_t g = (color >> 8) & 0xFF;
            uint32_t b = (color >> 16) & 0xFF;

            return (((5 * g) + (2 * r) + b) > (8 * 128));
        };

        // Get system foreground color
        COLORREF foreground = GetSysColor(COLOR_WINDOWTEXT);
        bool isDarkMode = isColorLight(foreground);

        if (highContrast.dwFlags & HCF_HIGHCONTRASTON)
        {
            // High contrast is enabled
            if (isDarkMode)
            {
                ApplyHighContrastDarkTheme();
            }
            else
            {
                ApplyHighContrastLightTheme();
            }
        }
        else
        {
            ImGui::StyleColorsClassic();

            // Ensure minimum contrast in certain UI elements
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
        }
    }
    else
    {
        throw std::runtime_error("SystemParametersInfo failed to get SPI_GETHIGHCONTRAST parameter.");
    }
}

void imgui_acc_win32::Narrate(const wchar_t* text)
{
    m_pVoice->Speak(text, SPF_ASYNC, NULL);
}
