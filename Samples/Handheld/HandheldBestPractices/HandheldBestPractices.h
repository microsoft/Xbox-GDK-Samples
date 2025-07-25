//--------------------------------------------------------------------------------------
// HandheldBestPractices.h
//
// Header for sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define LOG(f, ...) \
{ \
    g_appLog->AddLog(f, __VA_ARGS__); \
}

#define LOG_IF_FAILED(f) \
{ \
    HRESULT _hr = f; \
    if(FAILED(_hr)) { g_appLog->AddLog("%08X - "#f"\n", _hr); } \
}

#define LOG_AND_RETURN_IF_FAILED(f) \
{ \
    HRESULT _hr = f; \
    g_appLog->AddLog("%08X - "#f"\n", _hr); \
    if(FAILED(_hr)) return; \
}

#define LOG_IF_FAILED_AND_RETURN(f) \
{ \
    HRESULT _hr = f; \
    if(FAILED(_hr)) { g_appLog->AddLog("%08X - "#f"\n", _hr); return; } \
}

#define LOG_AND_CONTNUE(f) \
{ \
    HRESULT _hr = f; \
    g_appLog->AddLog("%08X - "#f"\n", _hr); \
}

constexpr ImU32 COLOR_ERROR = IM_COL32(255, 0, 0, 255);

template<typename... Args>
static void DrawNameValueTableHRESULT(const char* name, HRESULT hr, const char* fmt, const Args&... args)
{
    ImGui::TableNextColumn(); ImGui::Text("%s", name);
    ImGui::TableNextColumn();
    if(SUCCEEDED(hr))
    {
        ImGui::Text(fmt, args...);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ERROR);
        ImGui::Text("Error %08X", hr);
        ImGui::PopStyleColor();
    }
}

template<typename... Args>
static void DrawNameValueTable(const char* name, const char* fmt, const Args&... args)
{
    ImGui::TableNextColumn(); ImGui::Text("%s", name);
    ImGui::TableNextColumn(); ImGui::Text(fmt, args...);
}

template<typename... Args>
static void DrawNameBoolValueTable(const char* name, int32_t value)
{
    ImGui::TableNextColumn(); ImGui::Text("%s", name);
    ImGui::TableNextColumn(); ImGui::Text("%s", value ? "TRUE" : "FALSE");
}

void Sample_Initialize(HWND hWnd);
void Sample_Update();
void Sample_Draw();
void Sample_Shutdown();
LRESULT Sample_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void LoadFont();
static void SetUIScale(float scale);
