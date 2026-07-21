//--------------------------------------------------------------------------------------
// imgui_atg.h
//
// ATG ImGui extensions: style, DPI, fullscreen layout, resizable split panels,
// table helpers, and application log.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui_atg_device_context.h"
#include <cstdint>
#include <array>
#include <optional>
#ifndef _GAMING_XBOX
#include <winerror.h>
#else
#include <windef.h>
#endif

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

namespace ImGuiAtg
{
    // Style and DPI
    void LoadFont(float fontSize, const char* name = "segoeui.ttf");
    void SetAtgStyle(float baseFontSize = 20.0f, const char* fontName = "segoeui.ttf");
    void SetAtgStyleDark();
    void SetAtgStyleLight();
    void ToggleTheme();
    bool IsLightMode();
    void SetDpiScale(HWND hWnd);
    float GetCurrentScale();
    bool DidScaleChange();
    void SetClearColor(const std::array<float, 4>& color);
    const std::array<float, 4>& GetClearColor();

    // Returns value scaled by the current DPI factor.
    // Use for fixed widget sizes: ImGuiAtg::Scaled(180) instead of 180 * scale.
    inline float Scaled(float value) { return value * GetCurrentScale(); }

    // Full-screen layout -- creates a single borderless ImGui window filling the framebuffer.
    //
    // titleSafe controls the TV title-safe inset (a 5%-per-edge margin that protects
    // content from being cropped by TV overscan):
    //   std::nullopt (default) -- automatic: enabled on Xbox console builds (_GAMING_XBOX),
    //                             disabled on PC builds.
    //   true                   -- force the inset on, regardless of platform.
    //   false                  -- force the inset off, regardless of platform.
    void BeginFullscreenLayout(std::optional<bool> titleSafe = std::nullopt);
    void EndFullscreenLayout();

    // Standard sample footer -- a single-line strip with exit and theme toggle hints:
    //   [Alt]+[F4] / [LB]+[RB]+[View]+[Menu] Exit       [F2] / [LB]+[RB]+[Y] Toggle Light/Dark
    //
    // DrawFooter() renders the visual footer only. The matching keyboard/gamepad
    // shortcuts are processed by ImGuiAtg::HandleStandardInput(), which the sample
    // Main.cpp calls automatically once per frame.
    //
    // Use GetFooterHeight() to reserve space when laying out scrollable content above
    // the footer, e.g.:
    //   ImGui::BeginChild("##Content", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGuiAtg::GetFooterHeight()));
    //       // ... main content ...
    //   ImGui::EndChild();
    //   ImGuiAtg::DrawFooter();
    float GetFooterHeight();
    void DrawFooter();

    // Handles standard sample input -- the same shortcuts that DrawFooter() advertises:
    //
    //   Alt+F4 / LB+RB+View+Menu    -- exit the app (Alt+F4 is handled by Windows)
    //   F2     / LB+RB+Y            -- toggle light/dark theme
    //
    // Call once per frame from the main loop between ImGui::NewFrame() and Render
    // (the sample Main.cpp does this for you). Maintains its own previous-frame
    // gamepad state for edge detection. Keyboard shortcuts are suppressed while
    // ImGui is capturing keyboard input for a text widget. Gamepad handling uses
    // the v0 GameInput API (always available via the Windows SDK or the
    // Microsoft.GameInput NuGet package) so this works in any sample regardless of
    // which GameInput API version the sample itself targets. The kit creates its
    // own IGameInput instance on first use.
    void HandleStandardInput();

    // Splitter -- splits the current region into two resizable panels.
    //
    // Usage (horizontal -- top/bottom):
    //   ImGuiAtg::BeginSplitH("##split", 200.0f, 50.0f);
    //       // ... top content ...
    //   ImGuiAtg::SplitNext();
    //       // ... bottom content ...
    //   ImGuiAtg::EndSplit();
    //
    // Usage (vertical -- left/right):
    //   ImGuiAtg::BeginSplitV("##split", 300.0f, 50.0f);
    //       // ... left content ...
    //   ImGuiAtg::SplitNext();
    //       // ... right content ...
    //   ImGuiAtg::EndSplit();
    //
    // Parameters:
    //   id:          Unique string ID (state stored automatically via ImGui)
    //   defaultSize: Initial size of the second panel on first use
    //   minSize:     Minimum size for either panel
    void BeginSplitH(const char* id, float defaultSize = 200.0f, float minSize = 50.0f);
    void BeginSplitV(const char* id, float defaultSize = 200.0f, float minSize = 50.0f);
    void SplitNext();
    void EndSplit();

    // Controller string rendering -- mixed text and glyph font icons.
    //
    // Parses text for [XXX] tags and renders button glyphs inline with regular text.
    // Supported tags (case-insensitive):
    //   [A] [B] [X] [Y]                      -- Xbox face buttons
    //   [Cross] [Circle] [Square] [Triangle] -- Alternate face buttons
    //   [DPad] [DPadUp] [DPadDown]           -- Xbox D-Pad
    //   [DPadLeft] [DPadRight]
    //   [Up] [Down] [Left] [Right]           -- Generic D-Pad
    //   [LB] [RB] [LT] [RT]                  -- Shoulders / triggers (Xbox)
    //   [L1] [R1] [L2] [R2]                  -- Shoulders / triggers (alternate)
    //   [LThumb] [RThumb]                    -- Analog sticks
    //   [LSB] [RSB] [L3] [R3]                -- Stick clicks
    //   [Menu] [View] [Nexus] [Guide]        -- System buttons (Xbox)
    //   [Start] [Select] [Back]              -- System buttons (generic)
    //   [Options] [Home]                     -- System buttons (alternate)
    //
    // Keyboard tags:
    //   [Esc] [Tab] [Enter] [Backspace]      -- Common keys
    //   [Shift] [Ctrl] [Alt] [Super]
    //   [Space] [Caps] [Fn]
    //   [F1] .. [F12]                        -- Function keys
    //   [Ins] [Del] [KbHome] [End]           -- Editing / navigation
    //   [PgUp] [PgDn]
    //   [ArrowUp] [ArrowDown]                -- Arrow keys (distinct from D-Pad)
    //   [ArrowLeft] [ArrowRight]
    //   [PrtSc] [ScrLk] [Pause] [NumLk]      -- Lock / system keys
    //
    void ControllerText(const char* text);

    // --- Controller glyph codepoints ---
    //
    // Strongly-typed enum of Unicode codepoints for use with Glyph().
    // Example: ImGuiAtg::Glyph(ControllerGlyph::BtnA);
    //
    enum class ControllerGlyph : uint32_t
    {
        // Face buttons (Xbox)
        BtnA            = 0x21D3,
        BtnB            = 0x21D2,
        BtnX            = 0x21D0,
        BtnY            = 0x21D1,

        // Face buttons (alternate)
        BtnCross        = 0x21E3,
        BtnCircle       = 0x21E2,
        BtnSquare       = 0x21E0,
        BtnTriangle     = 0x21E1,
        BtnC            = 0x21EB,
        BtnZ            = 0x21EC,

        // D-Pad (Xbox-style)
        DPad            = 0x2284,
        DPadUp          = 0x227B,
        DPadDown        = 0x227D,
        DPadLeft        = 0x227A,
        DPadRight       = 0x227C,

        // D-Pad (generic)
        Up              = 0x219F,
        Down            = 0x21A1,
        Left            = 0x219E,
        Right           = 0x21A0,

        // Shoulders / triggers (Xbox-style)
        LB              = 0x2198,
        RB              = 0x2199,
        LT              = 0x2196,
        RT              = 0x2197,

        // Shoulders / triggers (alternate)
        L1              = 0x21B0,
        R1              = 0x21B1,
        L2              = 0x21B2,
        R2              = 0x21B3,

        // Analog sticks
        LThumb          = 0x21CB,
        RThumb          = 0x21CC,
        LStickClick     = 0x21EF,
        RStickClick     = 0x21F0,

        // System buttons
        Menu            = 0x21FB,
        View            = 0x21FA,
        Nexus           = 0xE001,
        Start           = 0x21F8,
        Select          = 0x21F7,
        Back            = 0x23CE,
        Options         = 0x21E5,
        Home            = 0x21F9,

        // Keyboard -- modifiers and named keys
        KbEsc           = 0x242F,
        KbTab           = 0x242B,
        KbEnter         = 0x242E,
        KbBackspace     = 0x242D,
        KbShift         = 0x2429,
        KbCtrl          = 0x2427,
        KbAlt           = 0x2428,
        KbSuper         = 0x242A,
        KbSpace         = 0x243A,
        KbCaps          = 0x242C,
        KbFn            = 0x2426,
        KbInsert        = 0x2434,
        KbDelete        = 0x2437,
        KbHome          = 0x2435,
        KbEnd           = 0x2438,
        KbPageUp        = 0x2436,
        KbPageDown      = 0x2439,
        KbPrintScreen   = 0x2430,
        KbScrollLock    = 0x2431,
        KbPause         = 0x2432,
        KbNumLock       = 0x2433,

        // Keyboard -- arrow keys
        KbArrowUp       = 0x23F6,
        KbArrowDown     = 0x23F7,
        KbArrowLeft     = 0x23F4,
        KbArrowRight    = 0x23F5,

        // Keyboard -- function keys are sequential: KbF1 + (n-1) for F1..F12.
        // [F1]..[F12] tags are also recognized by ControllerText and computed at lookup.
        KbF1            = 0x2460,
    };

    // Render a single controller glyph icon.
    // glyph: a ControllerGlyph enum value
    // size: glyph size in pixels (0 = default 1.33x current font size)
    // fallback: text to display if glyph font not loaded (nullptr = render nothing)
    void Glyph(ControllerGlyph glyph, float size = 0.0f, const char* fallback = nullptr);

    // Returns the internally loaded glyph font, or nullptr if not found.
    ImFont* GetGlyphFont();

    // --- GameInput Integration (available when GameInput.h is included before this header) ---
    // Maps a GameInputLabel enum value to a ControllerGlyph.
    // Returns static_cast<ControllerGlyph>(0) if no matching glyph exists.
    ControllerGlyph GameInputLabelToGlyph(GameInputLabel label);

    // Render a single controller glyph from a GameInputLabel.
    // size: glyph size in pixels (0 = default 1.33x current font size)
    // fallback: text to display if glyph not found (nullptr = empty)
    void Glyph(GameInputLabel label, float size = 0.0f, const char* fallback = nullptr);

    // Table helpers
    template<typename... Args>
    inline void DrawNameValueTableHRESULT(const char* name, HRESULT hr, const char* fmt, const Args&... args)
    {
        ImGui::TableNextColumn(); ImGui::Text(name);
        ImGui::TableNextColumn();
        if(SUCCEEDED(hr))
        {
            ImGui::Text(fmt, args...);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error %08X", args...);
        }
    }

    template<typename... Args>
    inline void DrawNameValueTable(const char* name, const char* fmt, const Args&... args)
    {
        ImGui::TableNextColumn(); ImGui::Text(name);
        ImGui::TableNextColumn(); ImGui::Text(fmt, args...);
    }

    inline void DrawNameBoolValueTable(const char* name, int32_t value)
    {
        ImGui::TableNextColumn(); ImGui::Text(name);
        ImGui::TableNextColumn(); ImGui::Text(value ? "TRUE" : "FALSE");
    }

    template<typename... Args>
    inline void DrawNameValueTableColored(const char* name, const ImVec4& valueColor, const char* fmt, const Args&... args)
    {
        ImGui::TableNextColumn(); ImGui::Text(name);
        ImGui::TableNextColumn(); ImGui::TextColored(valueColor, fmt, args...);
    }

    // --- Log ---
    //
    // ImGui-style application log with support for multiple named instances.
    // Each log is identified by a string ID. The default (empty) ID provides
    // backward compatibility with the single-log API.
    //
    // Default log:
    //   ImGuiAtg::Log(fmt, ...)        -- Add message (also OutputDebugString)
    //   ImGuiAtg::DrawLog(title)       -- Draw as floating window
    //   ImGuiAtg::DrawLogPanel()       -- Draw as embedded panel
    //   ImGuiAtg::ClearLog()           -- Clear
    //   ImGuiAtg::IsLogCollapsed()     -- Header collapsed?
    //
    // Named logs (multiple instances):
    //   ImGuiAtg::LogTo(id, fmt, ...)
    //   ImGuiAtg::DrawNamedLog(title, id)
    //   ImGuiAtg::DrawNamedLogPanel(id)
    //   ImGuiAtg::ClearNamedLog(id)
    //   ImGuiAtg::IsNamedLogCollapsed(id)

    void Log(const char* fmt, ...);
    void DrawLog(const char* title, float filterWidth = -200.0f, bool wrap = false);
    void DrawLogPanel(float height = 0.0f, bool wrap = false);
    void ClearLog();
    bool IsLogCollapsed();

    void LogTo(const char* id, const char* fmt, ...);
    void DrawNamedLog(const char* title, const char* id, float filterWidth = -200.0f, bool wrap = false);
    void DrawNamedLogPanel(const char* id, float height = 0.0f, bool wrap = false);
    void ClearNamedLog(const char* id);
    bool IsNamedLogCollapsed(const char* id);
} // namespace ImGuiAtg

//--------------------------------------------------------------------------------------
// Convenience macros (always use the default log)
//--------------------------------------------------------------------------------------

#define LOG(f, ...) ImGuiAtg::Log(f, __VA_ARGS__)

#define LOG_IF_FAILED(f) \
{ \
    HRESULT _hr = f; \
    if(FAILED(_hr)) { ImGuiAtg::Log("%08X - "#f"\n", _hr); } \
}

#define LOG_AND_RETURN_IF_FAILED(f) \
{ \
    HRESULT _hr = f; \
    ImGuiAtg::Log("%08X - "#f"\n", _hr); \
    if(FAILED(_hr)) return; \
}

#define LOG_IF_FAILED_AND_RETURN(f) \
{ \
    HRESULT _hr = f; \
    if(FAILED(_hr)) { ImGuiAtg::Log("%08X - "#f"\n", _hr); return; } \
}

#define LOG_AND_CONTINUE(f) \
{ \
    HRESULT _hr = f; \
    ImGuiAtg::Log("%08X - "#f"\n", _hr); \
}
