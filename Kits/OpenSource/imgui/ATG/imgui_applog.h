//--------------------------------------------------------------------------------------
// imgui_applog.h
//
// ImGui log window
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "../imgui.h"

class AppLog
{
    private:
        ImGuiTextBuffer     m_buffer;
        ImGuiTextFilter     m_filter;
        ImVector<int>       m_lineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
        bool                m_autoScroll;  // Keep scrolling if already at the bottom.

    public:
        AppLog();
        void Clear();
        void AddLog(const char* fmt, ...);
        void Draw(const char* title);
};
