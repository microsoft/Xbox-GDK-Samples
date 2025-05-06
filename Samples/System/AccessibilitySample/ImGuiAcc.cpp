// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ImGuiAcc.h"

#pragma region Base class

const static int c_maxbuffersize = 256;
const static ImVec2 c_minwidgetsize{ 30.0f, 30.0f };

ImGuiAcc::ImGuiAcc()
    : m_scaleFactor(1.0f)
{
    m_DPI = 0;
    m_lastDPI = 0;
    m_hWindow = 0;

#ifdef _GAMING_XBOX
    m_keyboardManager = new VirtualKeyboardManager();
#endif
    m_narrator = new Narrator();
}

ImGuiAcc::~ImGuiAcc()
{
#ifdef _GAMING_XBOX
    if (m_keyboardManager)
    {
        delete m_keyboardManager;
    }
#endif

    if (m_narrator)
    {
        delete m_narrator;
    }
}

ImGuiAcc* ImGuiAcc::GetInstance()
{
    if (m_instance == nullptr)
    {
        m_instance = new ImGuiAcc();
    }
    return m_instance;
}

void ImGuiAcc::NewFrame()
{
#ifdef _GAMING_DESKTOP
    UpdateScaleFactor();
#endif
    DetectHighContrastTheme();
}
#pragma endregion

#pragma region Imgui Widget Wrappers
bool ImGuiAcc::Begin(const char* name, bool* open, ImGuiWindowFlags flags, ImVec2 size)
{
    ImGui::SetNextWindowSize(size);

    if (!ImGui::Begin(name, open, flags |
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize))
    {
        return false;
    }

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

        float scrollSpeed = 10.0f;
        float deadzone = 0.1f;

        // Read the right thumbstick analog value directly
        float rx = ImGui::GetIO().KeysData[ImGuiKey_GamepadRStickRight].AnalogValue -
            ImGui::GetIO().KeysData[ImGuiKey_GamepadRStickLeft].AnalogValue;

        if (fabsf(rx) > deadzone)
        {
            ImGui::SetScrollX(ImGui::GetScrollX() + rx * scrollSpeed);
        }
    }
    return true;
}

void ImGuiAcc::Text(const char* text, ...)
{
    char buffer[c_maxbuffersize]{};

    // Handle variable arguments and format the string
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);

    ImGui::Selectable(buffer);
    if (ImGui::IsItemFocused())
    {
        m_narrator->AddNarration(buffer);
    }
}

bool ImGuiAcc::Button(const char* label)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the size ImGui would normally use
    ImVec2 size
    {
        ImGui::CalcTextSize(label, nullptr, true).x + style.FramePadding.x * 2.0f,
        ImGui::CalcTextSize(label, nullptr, true).y + style.FramePadding.y * 2.0f
    };

    // Enforce minimum size
    ImVec2 adjustedSize
    {
        std::max(size.x, c_minwidgetsize.x),
        std::max(size.y, c_minwidgetsize.y)
    };

    bool pressed = ImGui::Button(label, adjustedSize);

    if (ImGui::IsItemFocused())
    {
        m_narrator->AddNarration(label);
    }

    if (ImGui::IsItemDeactivated())
    {
        char buffer[c_maxbuffersize]{};
        snprintf(buffer, c_maxbuffersize, "%s %s\n", label, "pressed.");
        m_narrator->AddNarration(buffer);
    }
    return pressed;
}

bool ImGuiAcc::InputText(const char* label, char* buf, size_t bufSize, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData, GameInputKind activeGameInputKind)
{
    // Currently unused for desktop builds.
    UNREFERENCED_PARAMETER(activeGameInputKind);

    bool valueChanged = false;

    // Xbox specific virtual keyboard code
    // TODO: External callbacks are not supported at the moment on console as virtual keyboard support is taking the callback slot.
    ImGuiInputTextCallback keyboardCallback = nullptr;
    struct ImGuiInputTextContext
    {
        void* userData;
        ImGuiAcc* accessibilityContext;
    };

#ifdef _GAMING_XBOX
    // If physical keyboard is not connected, use virtual keyboard on console
    if ((activeGameInputKind & GameInputKind::GameInputKindKeyboard) == 0)
    {
        // Callback is called every frame when the input text field is active
        keyboardCallback = [](ImGuiInputTextCallbackData* data) -> int
        {
            ImGuiInputTextContext* context = static_cast<ImGuiInputTextContext*>(data->UserData);
            ImGuiAcc* accessibilityContext = context->accessibilityContext;

            if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
            {
                if (accessibilityContext->m_keyboardManager->IsKeyboardActive())
                {
                    uint32_t cursorPosition;
                    XGameUiTextEntryChangeTypeFlags changeType;

                    char updatedBuffer[c_maxbuffersize]{};
                    XGameUiTextEntryGetState(
                        accessibilityContext->m_keyboardManager->keyboardHandle,
                        &changeType,
                        &cursorPosition,
                        nullptr,
                        nullptr,
                        c_maxbuffersize,
                        updatedBuffer);

                    // Copy the new text into the ImGui input field buffer
                    if (static_cast<uint32_t>(XGameUiTextEntryChangeTypeFlags::TextChanged) & static_cast<uint32_t>(changeType))
                    {
                        // Narrate the latest text
                        accessibilityContext->m_narrator->AddNarration(updatedBuffer);
                        data->DeleteChars(0, data->BufTextLen);
                        data->InsertChars(0, updatedBuffer);
                    }
                }
                else
                {
                    accessibilityContext->m_keyboardManager->ActivateKeyboard(data->Buf);
                }
            }
            return 0;
        };
    }
#endif

    ImGuiStyle& style = ImGui::GetStyle();
    // Calculate the default size
    ImVec2 testSize = ImGui::CalcTextSize(buf, nullptr, true);
    ImVec2 totalSize =
    {
        testSize.x + style.FramePadding.x * 2.0f,
        testSize.y + style.FramePadding.y * 2.0f
    };

    // Enforce minimum size
    ImVec2 sizeDiff =
    {
        std::max(c_minwidgetsize.x - totalSize.x, 0.0f),
        std::max(c_minwidgetsize.y - totalSize.y, 0.0f)
    };

    // Adjust frame padding if necessary
    bool needsStyleModification = (sizeDiff.x > 0.0f || sizeDiff.y > 0.0f);
    if (needsStyleModification)
    {
        ImVec2 padding =
        {
            style.FramePadding.x + sizeDiff.x / 2.0f,
            style.FramePadding.y + sizeDiff.y / 2.0f
        };
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    if (keyboardCallback)
    {
        // Set up the context for the Virtual keyboard callback for console
        ImGuiInputTextContext context;
        context.userData = userData;
        context.accessibilityContext = this;

        valueChanged = ImGui::InputText(label, buf, bufSize, flags | ImGuiInputTextFlags_CallbackAlways, keyboardCallback, &context);
    }
    else
    {
        valueChanged = ImGui::InputText(label, buf, bufSize, flags, callback, userData);
    }

    if (needsStyleModification)
    {
        ImGui::PopStyleVar();
    }

    char buffer[c_maxbuffersize]{};
    if (ImGui::IsItemActive())
    {
        snprintf(buffer, c_maxbuffersize, "Editing %s %s\n", label, buf);
        m_narrator->AddNarration(buffer);
    }
    else if (ImGui::IsItemFocused())
    {
        snprintf(buffer, c_maxbuffersize, "%s %s\n", label, buf);
        m_narrator->AddNarration(buffer);
    }

#ifdef _GAMING_XBOX
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        // Check if deactivation was due to ImGuiKey_GamepadBack. If so, force a strcpy of the latest
        // virtual keyboard text into the ImGui buffer.
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight))
        {
            uint32_t cursorPosition;
            XGameUiTextEntryChangeTypeFlags changeType;
            char updatedBuffer[c_maxbuffersize]{};
            XGameUiTextEntryGetState(
                m_keyboardManager->keyboardHandle,
                &changeType,
                &cursorPosition,
                nullptr,
                nullptr,
                c_maxbuffersize,
                updatedBuffer);
            strcpy_s(buf, c_maxbuffersize, updatedBuffer);
        }
        m_keyboardManager->DeactivateKeyboard();
    }
    else if (ImGui::IsItemDeactivated())
    {
        m_keyboardManager->DeactivateKeyboard();
    }

#endif
    return valueChanged;
}

bool ImGuiAcc::Checkbox(const char* label, bool* isChecked)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the total size of the checkbox and label
    ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
    float squareSize = ImGui::GetFrameHeight(); // Size of the checkbox square
    ImVec2 totalSize =
    {
        squareSize + style.ItemInnerSpacing.x + labelSize.x,
        std::max(squareSize, labelSize.y)
    };

    // Enforce minimum size
    ImVec2 sizeDiff =
    {
        std::max(c_minwidgetsize.x - totalSize.x, 0.0f),
        std::max(c_minwidgetsize.y - totalSize.y, 0.0f)
    };

    // Push style vars if adjustments are needed as imgui checkbox doesn't have a size parameter
    bool needsStyleModification = (sizeDiff.x > 0.0f || sizeDiff.y > 0.0f);
    if (needsStyleModification)
    {
        ImVec2 padding =
        {
            style.FramePadding.x + sizeDiff.x / 2.0f,
            style.FramePadding.y + sizeDiff.y / 2.0f
        };
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool pressed = ImGui::Checkbox(label, isChecked);

    // Pop style vars if they were pushed
    if (needsStyleModification)
    {
        ImGui::PopStyleVar();
    }

    if (ImGui::IsItemFocused() || pressed)
    {
        char buffer[c_maxbuffersize]{};
        if (*isChecked)
        {
            snprintf(buffer, c_maxbuffersize, "%s %s\n", label, "checked.");
            m_narrator->EnableNarration();
        }
        else
        {
            m_narrator->DisableNarration();
        }

        m_narrator->AddNarration(buffer);
    }
    return pressed;
}

bool ImGuiAcc::SliderInt(const char* label, int* value, int valueMin, int valueMax, const char*, ImGuiSliderFlags flags)
{
    ImGui::BeginGroup();
    float sliderHeight = ImGui::GetFrameHeight();

    if (sliderHeight < c_minwidgetsize.y)
    {
        float paddingInc = (c_minwidgetsize.y - sliderHeight) / 2.0f;

        ImVec2 padding =
        {
            ImGui::GetStyle().FramePadding.x,
            ImGui::GetStyle().FramePadding.y + paddingInc
        };
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool result = ImGui::SliderScalar(label, ImGuiDataType_S32, value, &valueMin, &valueMax, "", flags | ImGuiSliderFlags_NoInput);

    char buffer[c_maxbuffersize]{};
    if (ImGui::IsItemActive())
    {
        snprintf(buffer, c_maxbuffersize, "Editing %s %d\n", label, *value);
        m_narrator->AddNarration(buffer);
    }
    else if (ImGui::IsItemFocused())
    {
        snprintf(buffer, c_maxbuffersize, "%s %d\n", label, *value);
        m_narrator->AddNarration(buffer);
    }

    // Pop style var if it was pushed
    if (sliderHeight < c_minwidgetsize.y)
    {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    ImGui::Text(": %d", *value);
    ImGui::EndGroup();

    return result;
}

bool ImGuiAcc::WindowHeader(const char* text)
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
        char buffer[c_maxbuffersize]{};
        snprintf(buffer, c_maxbuffersize, "%s window focused.\n", text);
        m_narrator->AddNarration(buffer);
    }

    ImGui::SameLine();

    // Add text to the center of the line
    ImGui::SetCursorPosX(textPosX);
    ImGui::Text("%s", text);
    ImGui::Separator();
    ImGui::PopID();

    return isSelected;
}
#pragma endregion

#pragma region Accessibility
void ImGuiAcc::ApplyHighContrastDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

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

void ImGuiAcc::ApplyHighContrastLightTheme()
{
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

#ifdef _GAMING_DESKTOP
void ImGuiAcc::UpdateScaleFactor()
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
#endif

void ImGuiAcc::AdjustForDisplayScaling()
{
    ImGui::GetStyle().ScaleAllSizes(m_scaleFactor);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = m_scaleFactor;
}

void ImGuiAcc::DetectHighContrastTheme()
{
#ifdef _GAMING_XBOX
    XHighContrastMode highContrastMode;
    DX::ThrowIfFailed(XHighContrastGetMode(&highContrastMode));

    if (highContrastMode == XHighContrastMode::Dark)
    {
        ApplyHighContrastDarkTheme();
    }
    else if (highContrastMode == XHighContrastMode::Light)
    {
        ApplyHighContrastLightTheme();
    }
    else
    {
        ImGui::StyleColorsClassic();

        // Required for 4.5:1 contrast ratio
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
    }
#else
    HIGHCONTRAST highContrast{};
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
#endif
}
#pragma endregion

#ifdef _GAMING_XBOX
#pragma region Virtual Keyboard
VirtualKeyboardManager::VirtualKeyboardManager()
{
    keyboardHandle = nullptr;
}

VirtualKeyboardManager::~VirtualKeyboardManager()
{
    if (keyboardHandle != nullptr)
    {
        XGameUiTextEntryClose(keyboardHandle);
    }
}

bool VirtualKeyboardManager::IsKeyboardActive()
{
    return keyboardHandle != nullptr;
}

void VirtualKeyboardManager::ActivateKeyboard(const char* buffer)
{
    XGameUiTextEntryOptions options =
    {
        XGameUiTextEntryInputScope::Default,
        XGameUiTextEntryPositionHint::Bottom,
        XGameUiTextEntryVisibilityFlags::Default
    };

    // The XGameUiTextEntry has a maximum buffer size of c_maxbuffersize,
    // so the ActivateKeyboard buffer will never exceed this size.
    XGameUiTextEntryOpen(&options,
        c_maxbuffersize,
        buffer,
        static_cast<uint32_t>(strlen(buffer)),
        &keyboardHandle);
}

void VirtualKeyboardManager::DeactivateKeyboard()
{
    XGameUiTextEntryClose(keyboardHandle);
    keyboardHandle = nullptr;
}
#pragma endregion
#endif
