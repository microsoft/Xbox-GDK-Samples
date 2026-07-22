//--------------------------------------------------------------------------------------
// imgui_atg.cpp
//
// ATG ImGui extensions: style, DPI, fullscreen layout, and resizable split panels.
// See imgui_atg.h for usage documentation.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "imgui_atg.h"
#include "imgui_internal.h"
#ifndef _GAMING_XBOX
#include <ShellScalingApi.h>
#pragma comment(lib, "shcore.lib") // GetDpiForMonitor
#endif
#include <map>
#include <string>
#include <unordered_map>

namespace ImGuiAtg
{
    //--------------------------------------------------------------------------------------
    // Style / Font helpers
    //--------------------------------------------------------------------------------------

    static float s_currentScale = 1.0f;
    static bool  s_scaleChanged = false;
    static bool  s_lightMode = false;
    static std::array<float, 4> s_clearColor = { 0.45f, 0.55f, 0.60f, 1.0f };

    float GetCurrentScale()
    {
        return s_currentScale;
    }

    bool DidScaleChange()
    {
        if(s_scaleChanged)
        {
            s_scaleChanged = false;
            return true;
        }
        return false;
    }

    void SetClearColor(const std::array<float, 4>& color)
    {
        s_clearColor = color;
    }

    const std::array<float, 4>& GetClearColor()
    {
        return s_clearColor;
    }

    static ImFont* s_glyphFont = nullptr;

    // Returns the directory containing the running executable, with a trailing separator.
    static std::filesystem::path GetExecutableDir()
    {
        char buf[MAX_PATH]{};
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
            return {};
        return std::filesystem::path(buf).parent_path();
    }

    static void LoadGlyphFont(float fontSize)
    {
        // Look for promptfont.ttf relative to working directory first (Xbox, local dev),
        // then next to the executable (Desktop installs via Directory.Build.props copy)
        static const char* fontFile = "promptfont.ttf";

        std::filesystem::path candidates[] =
        {
            std::filesystem::path("Media") / fontFile,       // CWD-relative
            GetExecutableDir() / "Media" / fontFile,         // exe-relative
            GetExecutableDir() / fontFile,                   // exe-sxs
        };

        for (const auto& fontPath : candidates)
        {
            if (!fontPath.empty() && std::filesystem::exists(fontPath))
            {
                s_glyphFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), fontSize);
                return;
            }
        }
        s_glyphFont = nullptr;
    }

    ImFont* GetGlyphFont()
    {
        return s_glyphFont;
    }

    void LoadFont(float fontSize, const char* name)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        // Try to find the font file:
        // 1. As-given (absolute path or relative to CWD)
        // 2. In the system fonts folder (desktop only)
        char fontpath[MAX_PATH]{};
        bool found = false;

        // Check as-given first
        if (std::filesystem::exists(name))
        {
            strncpy_s(fontpath, name, MAX_PATH);
            found = true;
        }

#ifndef _GAMING_XBOX
        // Check system fonts folder (not available on Xbox)
        if (!found)
        {
            char windir[MAX_PATH]{};
            if (GetWindowsDirectoryA(windir, MAX_PATH))
            {
                sprintf_s(fontpath, "%s\\fonts\\%s", windir, name);
                found = std::filesystem::exists(fontpath);
            }
        }
#endif

        // Load the font if found, otherwise fall back to ImGui default
        if (found)
        {
            io.Fonts->AddFontFromFileTTF(fontpath, fontSize);
        }
        else
        {
            ImFontConfig ifc{};
            ifc.SizePixels = fontSize;
            io.Fonts->AddFontDefaultVector(&ifc);
        }
    }

    //--------------------------------------------------------------------------------------
    // Theme -- dark and light color schemes
    //--------------------------------------------------------------------------------------

    static void ApplyCommonStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding    = 2.0f;
        style.FrameRounding     = 2.0f;
        style.GrabRounding      = 2.0f;
        style.TabRounding       = 2.0f;
    }

    void SetDpiScale(HWND hWnd)
    {
#ifdef _GAMING_XBOX
        UNREFERENCED_PARAMETER(hWnd);
#else
        UINT dpiX = 96, dpiY = 96;
        GetDpiForMonitor(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        float scale = dpiX / 96.0f;

        // Save current colors before resetting
        ImGuiStyle& currentStyle = ImGui::GetStyle();
        ImVec4 savedColors[ImGuiCol_COUNT];
        memcpy(savedColors, currentStyle.Colors, sizeof(savedColors));
   
        // Reset to defaults, scale, restore colors
        currentStyle = ImGuiStyle();
        currentStyle.ScaleAllSizes(scale);
        currentStyle.FontScaleDpi = scale;
        memcpy(currentStyle.Colors, savedColors, sizeof(savedColors));

        // Restore style tweaks (ScaleAllSizes resets to defaults first)
        ApplyCommonStyle();

        // Update internal state
        s_currentScale = scale;
        s_scaleChanged = true;
#endif // !_GAMING_XBOX
    }

    void SetAtgStyle(float baseFontSize, const char* fontName)
    {
        LoadFont(baseFontSize, fontName);
        LoadGlyphFont(baseFontSize);
        SetAtgStyleDark();
    }

    void SetAtgStyleDark()
    {
        ImGui::StyleColorsDark();
        ApplyCommonStyle();
        s_lightMode = false;

        ImVec4 bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        s_clearColor = { bg.x, bg.y, bg.z, 1.0f };
    }

    void SetAtgStyleLight()
    {
        ImGui::StyleColorsLight();
        ApplyCommonStyle();
        s_lightMode = true;

        ImVec4 bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        s_clearColor = { bg.x, bg.y, bg.z, 1.0f };
    }

    bool IsLightMode()
    {
        return s_lightMode;
    }

    void ToggleTheme()
    {
        if (s_lightMode)
            SetAtgStyleDark();
        else
            SetAtgStyleLight();
    }

    //--------------------------------------------------------------------------------------
    // Fullscreen layout -- a single borderless window filling the framebuffer.
    //--------------------------------------------------------------------------------------

    void BeginFullscreenLayout(std::optional<bool> titleSafe)
    {
        // Resolve the title-safe choice: explicit caller value wins, otherwise default
        // to "on" for console builds and "off" for PC.
#ifdef _GAMING_XBOX
        const bool applyTitleSafe = titleSafe.value_or(true);
#else
        const bool applyTitleSafe = titleSafe.value_or(false);
#endif

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImVec2 pos(0.0f, 0.0f);
        if (applyTitleSafe)
        {
            // Standard TV title-safe area: 5% inset on each edge (per axis).
            const float insetX = displaySize.x * 0.05f;
            const float insetY = displaySize.y * 0.05f;
            pos = ImVec2(insetX, insetY);
            displaySize = ImVec2(displaySize.x - 2.0f * insetX, displaySize.y - 2.0f * insetY);
        }
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(displaySize);
        ImGui::Begin("##FullscreenLayout", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus);
    }

    void EndFullscreenLayout()
    {
        ImGui::End();
    }

    //--------------------------------------------------------------------------------------
    // Standard sample footer -- single-line strip with exit and theme toggle hints.
    //--------------------------------------------------------------------------------------

    float GetFooterHeight()
    {
        // Footer = glyph row (fontSize * 1.5) + the separator line above it.
        // DrawFooter() collapses ItemSpacing.y to 0 so no inter-item vertical gap
        // is accounted for here. Add a few pixels of buffer so the separator line
        // and any ImGui-internal padding around it never push the text out.
        return ImGui::GetFontSize() * 1.5f + ImGuiAtg::Scaled(5.0f);
    }

    void DrawFooter()
    {
        // Collapse vertical spacing so the footer takes the minimum height possible
        const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 0.0f));

        ImGui::Separator();

        // Exit hint -- click to quit. ControllerText wraps its output in a Group,
        // so the IsItem* queries here apply to the entire glyph+text run.
        ControllerText("[Alt]+[F4] / [LB]+[RB]+[View]+[Menu] Exit");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                PostQuitMessage(0);
        }

        ImGui::SameLine(0, Scaled(80.0f));

        // Theme toggle hint -- click to flip light/dark.
        ControllerText("[F2] / [LB]+[RB]+[Y] Toggle Light/Dark");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                ToggleTheme();
        }

        ImGui::PopStyleVar();
    }

    void HandleStandardInput()
    {
        // Theme toggle. Exit via Alt+F4 is handled by Windows (DefWindowProc), so we
        // only wire up F2 here. Suppressed while ImGui is capturing keyboard input
        // for a text widget so typing in a text box doesn't flip the theme.
        if (!ImGui::GetIO().WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
                ToggleTheme();
        }

        using Microsoft::WRL::ComPtr;
        static ComPtr<IGameInput> s_gameInput;
        static GameInputGamepadState s_prevGamepadState{};

        if (!s_gameInput)
        {
            if (FAILED(::GameInputCreate(&s_gameInput)))
                return;
        }

        ComPtr<::IGameInputReading> reading;
        if (FAILED(s_gameInput->GetCurrentReading(::GameInputKindGamepad, nullptr, &reading)))
            return;

        ::GameInputGamepadState curr{};
        if (!reading->GetGamepadState(&curr))
            return;

        const auto buttons = curr.buttons;
        const auto prevButtons = s_prevGamepadState.buttons;

        // Exit combo: LB + RB + View + Menu
        if ((buttons & ::GameInputGamepadLeftShoulder) &&
            (buttons & ::GameInputGamepadRightShoulder) &&
            (buttons & ::GameInputGamepadMenu) &&
            (buttons & ::GameInputGamepadView))
        {
            PostQuitMessage(0);
        }

        // Theme toggle combo: LB + RB + Y (edge-triggered to fire once per press)
        const bool themeCombo = (buttons & ::GameInputGamepadLeftShoulder) &&
                                (buttons & ::GameInputGamepadRightShoulder) &&
                                (buttons & ::GameInputGamepadY);
        const bool wasThemeCombo = (prevButtons & ::GameInputGamepadLeftShoulder) &&
                                   (prevButtons & ::GameInputGamepadRightShoulder) &&
                                   (prevButtons & ::GameInputGamepadY);
        if (themeCombo && !wasThemeCombo)
            ToggleTheme();

        s_prevGamepadState = curr;
    }

    //--------------------------------------------------------------------------------------
    // Splitter -- Begin/Next/End pattern for resizable split panels
    //--------------------------------------------------------------------------------------

    static float* s_splitSize = nullptr;
    static float  s_splitMinSize = 50.0f;
    static float  s_splitMaxSize = 0.0f;
    static bool   s_splitHorizontal = true;
    static bool   s_splitCollapsed = false;
    static constexpr float s_splitThickness = 4.0f;
    static constexpr float s_collapsedSize = 40.0f;

    static void BeginSplitImpl(const char* id, bool horizontal, float defaultSize, float minSize)
    {
        s_splitHorizontal = horizontal;
        s_splitMinSize = minSize;

        ImGuiID imId = ImGui::GetID(id);
        s_splitSize = ImGui::GetStateStorage()->GetFloatRef(imId, defaultSize);

        float availWidth = ImGui::GetContentRegionAvail().x;
        float availHeight = ImGui::GetContentRegionAvail().y;
        float totalSpace = horizontal ? availHeight : availWidth;
        s_splitMaxSize = totalSpace - s_splitThickness - minSize;

        // Check if the second panel's content is collapsed (e.g., log header folded).
        // Uses previous frame's state -- one frame latency is imperceptible.
        s_splitCollapsed = IsLogCollapsed();
        float secondSize = s_splitCollapsed ? s_collapsedSize : *s_splitSize;

        // Clamp
        if (*s_splitSize < minSize) *s_splitSize = minSize;
        if (*s_splitSize > s_splitMaxSize) *s_splitSize = s_splitMaxSize;

        float firstSize = totalSpace - secondSize - s_splitThickness;
        if (firstSize < minSize) firstSize = minSize;

        if (horizontal)
            ImGui::BeginChild("##SplitFirst", ImVec2(0, firstSize), ImGuiChildFlags_None);
        else
            ImGui::BeginChild("##SplitFirst", ImVec2(firstSize, 0), ImGuiChildFlags_None);
    }

    void BeginSplitH(const char* id, float defaultSize, float minSize)
    {
        BeginSplitImpl(id, true, defaultSize, minSize);
    }

    void BeginSplitV(const char* id, float defaultSize, float minSize)
    {
        BeginSplitImpl(id, false, defaultSize, minSize);
    }

    void SplitNext()
    {
        ImGui::EndChild(); // end first panel

        // Draggable splitter bar
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabActive));

        if (!s_splitHorizontal)
            ImGui::SameLine();

        ImGui::Button("##SplitBar", s_splitHorizontal
            ? ImVec2(-1, s_splitThickness)
            : ImVec2(s_splitThickness, -1));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemActive() && s_splitSize && !s_splitCollapsed)
        {
            float delta = s_splitHorizontal ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
            *s_splitSize -= delta;
            if (*s_splitSize < s_splitMinSize) *s_splitSize = s_splitMinSize;
            if (*s_splitSize > s_splitMaxSize) *s_splitSize = s_splitMaxSize;
        }

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            ImGui::SetMouseCursor(s_splitHorizontal ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);

        if (!s_splitHorizontal)
            ImGui::SameLine();

        // Begin second panel (fills remaining space)
        ImGui::BeginChild("##SplitSecond", ImVec2(0, 0), ImGuiChildFlags_None);
    }

    void EndSplit()
    {
        ImGui::EndChild(); // end second panel
    }

    //--------------------------------------------------------------------------------------
    // Controller string -- mixed text and glyph font rendering
    //--------------------------------------------------------------------------------------

    // Encode a Unicode codepoint (BMP) to a 4-byte UTF-8 buffer. Returns length written.
    static int EncodeUtf8(uint32_t cp, char* buf)
    {
        if (cp < 0x80)
        {
            buf[0] = static_cast<char>(cp);
            return 1;
        }
        if (cp < 0x800)
        {
            buf[0] = static_cast<char>(0xC0 | (cp >> 6));
            buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
            return 2;
        }
        buf[0] = static_cast<char>(0xE0 | (cp >> 12));
        buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
        return 3;
    }

    // Case-insensitive comparator for glyph tag lookup
    struct CaseInsensitiveLess
    {
        bool operator()(const std::string& a, const std::string& b) const
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };

    // Tag-to-glyph mapping for controller icons (used by ControllerText).
    static const std::map<std::string, ControllerGlyph, CaseInsensitiveLess> s_controllerGlyphs =
    {
        // Xbox face buttons
        { "A",         ControllerGlyph::BtnA },
        { "B",         ControllerGlyph::BtnB },
        { "X",         ControllerGlyph::BtnX },
        { "Y",         ControllerGlyph::BtnY },

        // Alternate face buttons
        { "Cross",     ControllerGlyph::BtnCross },
        { "Circle",    ControllerGlyph::BtnCircle },
        { "Square",    ControllerGlyph::BtnSquare },
        { "Triangle",  ControllerGlyph::BtnTriangle },

        // Xbox D-Pad
        { "DPad",      ControllerGlyph::DPad },
        { "DPadUp",    ControllerGlyph::DPadUp },
        { "DPadDown",  ControllerGlyph::DPadDown },
        { "DPadLeft",  ControllerGlyph::DPadLeft },
        { "DPadRight", ControllerGlyph::DPadRight },

        // Generic D-Pad
        { "Up",        ControllerGlyph::Up },
        { "Down",      ControllerGlyph::Down },
        { "Left",      ControllerGlyph::Left },
        { "Right",     ControllerGlyph::Right },

        // Shoulders and triggers (Xbox naming)
        { "LB",        ControllerGlyph::LB },
        { "RB",        ControllerGlyph::RB },
        { "LT",        ControllerGlyph::LT },
        { "RT",        ControllerGlyph::RT },

        // Shoulders and triggers (alternate naming)
        { "L1",        ControllerGlyph::L1 },
        { "R1",        ControllerGlyph::R1 },
        { "L2",        ControllerGlyph::L2 },
        { "R2",        ControllerGlyph::R2 },

        // Analog sticks
        { "LThumb",    ControllerGlyph::LThumb },
        { "RThumb",    ControllerGlyph::RThumb },
        { "LSB",       ControllerGlyph::LStickClick },
        { "RSB",       ControllerGlyph::RStickClick },
        { "L3",        ControllerGlyph::LStickClick },
        { "R3",        ControllerGlyph::RStickClick },

        // System buttons (Xbox)
        { "Menu",      ControllerGlyph::Menu },
        { "View",      ControllerGlyph::View },
        { "Nexus",     ControllerGlyph::Nexus },
        { "Guide",     ControllerGlyph::Nexus },

        // System buttons (generic)
        { "Start",     ControllerGlyph::Start },
        { "Select",    ControllerGlyph::Select },
        { "Back",      ControllerGlyph::Back },
        { "Options",   ControllerGlyph::Options },
        { "Home",      ControllerGlyph::Home },

        // Keyboard -- modifiers / named keys
        { "Esc",         ControllerGlyph::KbEsc },
        { "Escape",      ControllerGlyph::KbEsc },
        { "Tab",         ControllerGlyph::KbTab },
        { "Enter",       ControllerGlyph::KbEnter },
        { "Return",      ControllerGlyph::KbEnter },
        { "Backspace",   ControllerGlyph::KbBackspace },
        { "Shift",       ControllerGlyph::KbShift },
        { "Ctrl",        ControllerGlyph::KbCtrl },
        { "Control",     ControllerGlyph::KbCtrl },
        { "Alt",         ControllerGlyph::KbAlt },
        { "Super",       ControllerGlyph::KbSuper },
        { "Win",         ControllerGlyph::KbSuper },
        { "Cmd",         ControllerGlyph::KbSuper },
        { "Space",       ControllerGlyph::KbSpace },
        { "Caps",        ControllerGlyph::KbCaps },
        { "CapsLock",    ControllerGlyph::KbCaps },
        { "Fn",          ControllerGlyph::KbFn },
        { "Ins",         ControllerGlyph::KbInsert },
        { "Insert",      ControllerGlyph::KbInsert },
        { "Del",         ControllerGlyph::KbDelete },
        { "Delete",      ControllerGlyph::KbDelete },
        { "KbHome",      ControllerGlyph::KbHome },
        { "End",         ControllerGlyph::KbEnd },
        { "PgUp",        ControllerGlyph::KbPageUp },
        { "PageUp",      ControllerGlyph::KbPageUp },
        { "PgDn",        ControllerGlyph::KbPageDown },
        { "PageDown",    ControllerGlyph::KbPageDown },
        { "PrtSc",       ControllerGlyph::KbPrintScreen },
        { "PrintScreen", ControllerGlyph::KbPrintScreen },
        { "ScrLk",       ControllerGlyph::KbScrollLock },
        { "ScrollLock",  ControllerGlyph::KbScrollLock },
        { "Pause",       ControllerGlyph::KbPause },
        { "Break",       ControllerGlyph::KbPause },
        { "NumLk",       ControllerGlyph::KbNumLock },
        { "NumLock",     ControllerGlyph::KbNumLock },

        // Keyboard arrow keys (Arrow-prefixed to avoid clashing with generic D-Pad Up/Down/Left/Right)
        { "ArrowUp",     ControllerGlyph::KbArrowUp },
        { "ArrowDown",   ControllerGlyph::KbArrowDown },
        { "ArrowLeft",   ControllerGlyph::KbArrowLeft },
        { "ArrowRight",  ControllerGlyph::KbArrowRight },
    };

    // Look up a tag string and return the ControllerGlyph, or (ControllerGlyph)0 if not found.
    // [F1]..[F12] are recognized algorithmically rather than via 12 map entries.
    static ControllerGlyph LookupControllerGlyph(const char* tagStart, size_t tagLen)
    {
        auto it = s_controllerGlyphs.find(std::string(tagStart, tagLen));
        if (it != s_controllerGlyphs.end())
            return it->second;

        // [F1]..[F12]: codepoint = KbF1 + (n-1).
        if ((tagLen == 2 || tagLen == 3) && (tagStart[0] == 'F' || tagStart[0] == 'f'))
        {
            int n = 0;
            for (size_t i = 1; i < tagLen; ++i)
            {
                if (tagStart[i] < '0' || tagStart[i] > '9')
                    return static_cast<ControllerGlyph>(0);
                n = n * 10 + (tagStart[i] - '0');
            }
            if (n >= 1 && n <= 12)
                return static_cast<ControllerGlyph>(
                    static_cast<uint32_t>(ControllerGlyph::KbF1) + static_cast<uint32_t>(n - 1));
        }

        return static_cast<ControllerGlyph>(0);
    }

    void ControllerText(const char* text)
    {
        if (!text || !*text)
            return;

        ImFont* glyphFont = s_glyphFont;

        // PushFont takes an *unscaled* base size; ImGui applies FontScaleDpi
        // internally. Compute both: base for PushFont, final for metric probes.
        const ImGuiStyle& style = ImGui::GetStyle();
        const float scale = (style.FontScaleDpi > 0.0f) ? style.FontScaleDpi : 1.0f;
        const float textSizeBase   = style.FontSizeBase;
        const float glyphSizeBase  = textSizeBase * 1.5f;
        const float textSizeFinal  = ImGui::GetFontSize();      // == textSizeBase * scale
        const float glyphSizeFinal = glyphSizeBase * scale;

        // Align by visual centers using actual rendered glyph extents (ImFontGlyph
        // Y0/Y1). PromptFont is an icon font with no meaningful baseline -- box or
        // baseline alignment ends up off by a few pixels. Probing the real visual
        // box for one glyph from each font gives DPI-correct alignment.
        // Group the whole run so SameLine after this call gets a clean cursor.
        ImGui::BeginGroup();

        // Probe 'M' for text and [A] for the glyph font
        float textVisualMid = textSizeFinal * 0.5f;
        if (ImFontBaked* textBaked = ImGui::GetFontBaked())
        {
            if (ImFontGlyph* g = textBaked->FindGlyph('M'))
            {
                textVisualMid = (g->Y0 + g->Y1) * 0.5f;
            }
        }

        float glyphVisualMid = glyphSizeFinal * 0.5f;
        if (glyphFont)
        {
            if (ImFontBaked* glyphBaked = glyphFont->GetFontBaked(glyphSizeFinal))
            {
                if (ImFontGlyph* g = glyphBaked->FindGlyph(static_cast<ImWchar>(ControllerGlyph::BtnA)))
                {
                    glyphVisualMid = (g->Y0 + g->Y1) * 0.5f;
                }
            }
        }

        const float rowY = ImGui::GetCursorPosY();
        const float textY = rowY + (glyphVisualMid - textVisualMid);

        bool first = true;
        const char* p = text;

        while (*p)
        {
            const char* bracket = p;
            while (*bracket && *bracket != '[')
                ++bracket;

            if (bracket > p)
            {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                ImGui::SetCursorPosY(textY);
                ImGui::TextUnformatted(p, bracket);
                first = false;
            }

            if (!*bracket)
                break;

            const char* closeBracket = bracket + 1;
            while (*closeBracket && *closeBracket != ']')
                ++closeBracket;

            if (!*closeBracket)
            {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                ImGui::SetCursorPosY(textY);
                ImGui::TextUnformatted(bracket);
                break;
            }

            const char* tagStart = bracket + 1;
            size_t tagLen = static_cast<size_t>(closeBracket - tagStart);
            ControllerGlyph glyph = glyphFont
                ? LookupControllerGlyph(tagStart, tagLen)
                : static_cast<ControllerGlyph>(0);

            if (static_cast<uint32_t>(glyph) != 0)
            {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                ImGui::SetCursorPosY(rowY);
                char utf8[4];
                int len = EncodeUtf8(static_cast<uint32_t>(glyph), utf8);
                ImGui::PushFont(glyphFont, glyphSizeBase);  // base size; ImGui scales by FontScaleDpi
                ImGui::TextUnformatted(utf8, utf8 + len);
                ImGui::PopFont();
                first = false;
            }
            else
            {
                if (!first) ImGui::SameLine(0.0f, 0.0f);
                ImGui::SetCursorPosY(textY);
                ImGui::TextUnformatted(bracket, closeBracket + 1);
                first = false;
            }

            p = closeBracket + 1;
        }

        ImGui::EndGroup();
    }

    // Shared glyph rendering -- used by Glyph() overloads.
    // Renders a single glyph at the given size, or fallback text if glyph font unavailable.
    static void RenderGlyph(ControllerGlyph glyph, float size, const char* fallback)
    {
        ImFont* glyphFont = s_glyphFont;
        if (static_cast<uint32_t>(glyph) != 0 && glyphFont)
        {
            char utf8[4];
            int len = EncodeUtf8(static_cast<uint32_t>(glyph), utf8);
            ImGui::PushFont(glyphFont, size);
            ImGui::TextUnformatted(utf8, utf8 + len);
            ImGui::PopFont();
        }
        else if (fallback)
        {
            // Vertically center fallback text within the glyph-sized row
            float textHeight = ImGui::GetFontSize();
            float rowHeight = size * GetCurrentScale();
            float offsetY = (rowHeight - textHeight) * 0.5f;
            if (offsetY > 0.0f)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
            ImGui::TextUnformatted(fallback);
        }
    }

    void Glyph(ControllerGlyph glyph, float size, const char* fallback)
    {
        if (size <= 0.0f)
            size = ImGui::GetFontSize() * 1.33f;

        RenderGlyph(glyph, size, fallback);
    }

    void Glyph(GameInputLabel label, float size, const char* fallback)
    {
        if (size <= 0.0f)
            size = ImGui::GetFontSize() * 1.33f;

        ControllerGlyph glyph = GameInputLabelToGlyph(label);
        RenderGlyph(glyph, size, fallback);
    }

    //--------------------------------------------------------------------------------------
    // GameInput label-to-glyph mapping (compiled only when GameInput.h is included)
    //--------------------------------------------------------------------------------------
    ControllerGlyph GameInputLabelToGlyph(GameInputLabel label)
    {
        static const std::map<GameInputLabel, ControllerGlyph> glyphs =
        {
            // Xbox face buttons
            { GameInputLabelXboxA,                  ControllerGlyph::BtnA },
            { GameInputLabelXboxB,                  ControllerGlyph::BtnB },
            { GameInputLabelXboxX,                  ControllerGlyph::BtnX },
            { GameInputLabelXboxY,                  ControllerGlyph::BtnY },

            // Xbox D-Pad
            { GameInputLabelXboxDPadUp,             ControllerGlyph::DPadUp },
            { GameInputLabelXboxDPadDown,           ControllerGlyph::DPadDown },
            { GameInputLabelXboxDPadLeft,           ControllerGlyph::DPadLeft },
            { GameInputLabelXboxDPadRight,          ControllerGlyph::DPadRight },

            // Xbox shoulders/triggers/sticks
            { GameInputLabelXboxLeftShoulder,       ControllerGlyph::LB },
            { GameInputLabelXboxRightShoulder,      ControllerGlyph::RB },
            { GameInputLabelXboxLeftTrigger,        ControllerGlyph::LT },
            { GameInputLabelXboxRightTrigger,       ControllerGlyph::RT },
            { GameInputLabelXboxLeftStickButton,    ControllerGlyph::LStickClick },
            { GameInputLabelXboxRightStickButton,   ControllerGlyph::RStickClick },

            // Xbox system buttons
            { GameInputLabelXboxMenu,               ControllerGlyph::Menu },
            { GameInputLabelXboxView,               ControllerGlyph::View },
            { GameInputLabelXboxStart,              ControllerGlyph::Menu },      // alias
            { GameInputLabelXboxBack,               ControllerGlyph::View },      // alias
            { GameInputLabelXboxGuide,              ControllerGlyph::Nexus },

            // Alternate face buttons (Icon* labels)
            { GameInputLabelIconCross,              ControllerGlyph::BtnCross },
            { GameInputLabelIconCircle,             ControllerGlyph::BtnCircle },
            { GameInputLabelIconSquare,             ControllerGlyph::BtnSquare },
            { GameInputLabelIconTriangle,           ControllerGlyph::BtnTriangle },

            // Generic D-Pad labels
            { GameInputLabelUp,                     ControllerGlyph::Up },
            { GameInputLabelDown,                   ControllerGlyph::Down },
            { GameInputLabelLeft,                   ControllerGlyph::Left },
            { GameInputLabelRight,                  ControllerGlyph::Right },

            // Letter labels -- matching button glyphs where available
            { GameInputLabelLetterA,                ControllerGlyph::BtnA },
            { GameInputLabelLetterB,                ControllerGlyph::BtnB },
            { GameInputLabelLetterC,                ControllerGlyph::BtnC },
            { GameInputLabelLetterX,                ControllerGlyph::BtnX },
            { GameInputLabelLetterY,                ControllerGlyph::BtnY },
            { GameInputLabelLetterZ,                ControllerGlyph::BtnZ },

            // Generic shoulder/trigger labels
            { GameInputLabelLB,                     ControllerGlyph::LB },
            { GameInputLabelRB,                     ControllerGlyph::RB },
            { GameInputLabelLT,                     ControllerGlyph::LT },
            { GameInputLabelRT,                     ControllerGlyph::RT },
            { GameInputLabelLSB,                    ControllerGlyph::LStickClick },
            { GameInputLabelRSB,                    ControllerGlyph::RStickClick },
            { GameInputLabelL1,                     ControllerGlyph::L1 },
            { GameInputLabelR1,                     ControllerGlyph::R1 },
            { GameInputLabelL2,                     ControllerGlyph::L2 },
            { GameInputLabelR2,                     ControllerGlyph::R2 },
            { GameInputLabelL3,                     ControllerGlyph::LStickClick },
            { GameInputLabelR3,                     ControllerGlyph::RStickClick },

            // Generic system buttons
            { GameInputLabelMenu,                   ControllerGlyph::Menu },
            { GameInputLabelView,                   ControllerGlyph::View },
            { GameInputLabelStart,                  ControllerGlyph::Start },
            { GameInputLabelSelect,                 ControllerGlyph::Select },
            { GameInputLabelBack,                   ControllerGlyph::Back },
            { GameInputLabelOptions,                ControllerGlyph::Options },
            { GameInputLabelHome,                   ControllerGlyph::Home },
        };

        auto it = glyphs.find(label);
        return (it != glyphs.end()) ? it->second : static_cast<ControllerGlyph>(0);
    }

    //--------------------------------------------------------------------------------------
    // Log -- application log widget with multiple named instances
    //--------------------------------------------------------------------------------------

    struct LogState
    {
        ImGuiTextBuffer buffer;
        ImGuiTextFilter filter;
        ImVector<int>   lineOffsets;
        bool            autoScroll = true;
        bool            collapsed = false;

        LogState() { lineOffsets.push_back(0); }
    };

    static std::unordered_map<std::string, LogState>& GetLogMap()
    {
        static std::unordered_map<std::string, LogState> s_logs;
        return s_logs;
    }

    static LogState& GetState(const char* id = "")
    {
        return GetLogMap()[id ? id : ""];
    }

    static void LogImpl(LogState& s, const char* text)
    {
        OutputDebugStringA(text);
        int old_size = s.buffer.size();
        s.buffer.append(text, 0);
        for (int new_size = s.buffer.size(); old_size < new_size; old_size++)
            if (s.buffer[old_size] == '\n')
                s.lineOffsets.push_back(old_size + 1);
    }

    static void ClearImpl(LogState& s)
    {
        s.buffer.clear();
        s.lineOffsets.clear();
        s.lineOffsets.push_back(0);
    }

    static void DrawLogContent(LogState& s, const char* headerLabel, float height, bool wrap)
    {
        ImGui::PushID(&s);

        if (!ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
        {
            s.collapsed = true;
            ImGui::PopID();
            return;
        }
        s.collapsed = false;

        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        s.filter.Draw("##Filter", -200.0f);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &s.autoScroll);

        ImGui::Separator();

        // When wrapping, the horizontal scrollbar is unwanted -- long lines fold
        // onto the next row instead of scrolling sideways.
        ImGuiWindowFlags scrollFlags = wrap ? ImGuiWindowFlags_None : ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("##scrolling", ImVec2(0, height > 0 ? height - ImGui::GetCursorPosY() : 0),
            ImGuiChildFlags_None, scrollFlags);

        if (clear)
            ClearImpl(s);
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        if (wrap)
            ImGui::PushTextWrapPos(0.0f);   // Wrap at the right edge of the content region.

        const char* buf = s.buffer.begin();
        const char* buf_end = s.buffer.end();
        if (s.filter.IsActive())
        {
            for (int line_no = 0; line_no < s.lineOffsets.Size; line_no++)
            {
                const char* line_start = buf + s.lineOffsets[line_no];
                const char* line_end = (line_no + 1 < s.lineOffsets.Size) ? (buf + s.lineOffsets[line_no + 1] - 1) : buf_end;
                if (s.filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else if (wrap)
        {
            // A wrapped line can occupy several rows, so the fixed-row-height
            // ImGuiListClipper cannot be used; draw every line directly. This
            // forgoes virtualization, which is fine for typical sample logs.
            for (int line_no = 0; line_no < s.lineOffsets.Size; line_no++)
            {
                const char* line_start = buf + s.lineOffsets[line_no];
                const char* line_end = (line_no + 1 < s.lineOffsets.Size) ? (buf + s.lineOffsets[line_no + 1] - 1) : buf_end;
                ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(s.lineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + s.lineOffsets[line_no];
                    const char* line_end = (line_no + 1 < s.lineOffsets.Size) ? (buf + s.lineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }

        if (wrap)
            ImGui::PopTextWrapPos();
        ImGui::PopStyleVar();

        if (s.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::PopID();
    }

    // --- Default log API ---

    void Log(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        LogImpl(GetState(), buf);
    }

    void DrawLog(const char* title, float /*filterWidth*/, bool wrap)
    {
        ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse);
        DrawLogContent(GetState(), "Log", 0, wrap);
        ImGui::End();
    }

    void DrawLogPanel(float height, bool wrap)
    {
        DrawLogContent(GetState(), "Log", height, wrap);
    }

    void ClearLog()
    {
        ClearImpl(GetState());
    }

    bool IsLogCollapsed()
    {
        return GetState().collapsed;
    }

    // --- Named log API ---

    void LogTo(const char* id, const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        LogImpl(GetState(id), buf);
    }

    void DrawNamedLog(const char* title, const char* id, float /*filterWidth*/, bool wrap)
    {
        ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse);
        DrawLogContent(GetState(id), id, 0, wrap);
        ImGui::End();
    }

    void DrawNamedLogPanel(const char* id, float height, bool wrap)
    {
        DrawLogContent(GetState(id), id, height, wrap);
    }

    void ClearNamedLog(const char* id)
    {
        ClearImpl(GetState(id));
    }

    bool IsNamedLogCollapsed(const char* id)
    {
        return GetState(id).collapsed;
    }
} // namespace ImGuiAtg
