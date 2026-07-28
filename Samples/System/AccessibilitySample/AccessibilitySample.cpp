//--------------------------------------------------------------------------------------
// AccessibilitySample.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AccessibilitySample.h"
#include "GameInputManager.h"

extern void ExitSample() noexcept;

Sample::~Sample()
{
    Shutdown();
}

void Sample::Initialize(HWND window)
{
    UNREFERENCED_PARAMETER(window);

    m_imguiAcc = ImGuiAcc::GetInstance();

#ifdef _GAMING_DESKTOP
    m_imguiAcc->SetWindow(window);
#endif

    GameInputManager::Init();
}

void Sample::Update()
{
}

void Sample::Draw()
{
    m_imguiAcc->NewFrame();
    m_imguiAcc->Begin("Example UI", 0, 0, ImVec2(700, 300));
    m_imguiAcc->WindowHeader("Example UI");
    m_imguiAcc->Button("Button 1");
    m_imguiAcc->Button("Button 2");
    static char inputText[256] = "";
    m_imguiAcc->InputText("Input Text", inputText, IM_ARRAYSIZE(inputText), GameInputManager::GetActiveGameInputKind());
    static int sliderValue = 5;
    m_imguiAcc->SliderInt("Slider", &sliderValue, 0, 10);
    m_imguiAcc->End();

    // Set the initial coordinates for the legend window
    static bool initialPlacement = false;
    if (!initialPlacement)
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 bottomLeftCorner(windowPos.x, windowPos.y + 320);
        ImGui::SetNextWindowPos(ImVec2(bottomLeftCorner.x, bottomLeftCorner.y));
        initialPlacement = true;
    }

    m_imguiAcc->Begin("Legend", 0, 0, ImVec2(700, 500));
    m_imguiAcc->WindowHeader("Legend");
    m_imguiAcc->Text("Switch Window: CTRL + TAB");
    m_imguiAcc->Text("Next Widget: TAB, DOWN ARROW");
    m_imguiAcc->Text("Previous Widget: SHIFT + TAB, UP ARROW");
    m_imguiAcc->Text("Activate Widget: ENTER");
    m_imguiAcc->Text("Un-focus Widget: ESC");
    m_imguiAcc->Text("Horizontal Scroll: LEFT ARROW, RIGHT ARROW");
    static bool enableNarration = true;
    m_imguiAcc->Checkbox(": Enable Narration ", &enableNarration);
    ImGui::Separator();
    ImGui::Text("");
    m_imguiAcc->Text("Theme control is automatically pulled from Windows settings.");
    m_imguiAcc->Text("Text scaling is automatically pulled from Windows settings.");
    m_imguiAcc->Text("Narration is automatic. Focusing on a widget will narrate that widget.");

    if (enableNarration)
    {
        m_imguiAcc->EnableNarration();
    }
    else
    {
        m_imguiAcc->DisableNarration();
    }
    m_imguiAcc->End();
}

void Sample::Shutdown()
{
    GameInputManager::Shutdown();
}

void Sample::Activated()
{
}

void Sample::Deactivated()
{
}

LRESULT Sample::WndProcHandler(HWND /*hWnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return 0;
}

#ifdef _GAMING_XBOX
void Sample::Suspend(ImGuiAtg::DeviceContext* dc)
{
    dc->Suspend();
}

void Sample::Resume(ImGuiAtg::DeviceContext* dc)
{
    dc->Resume();
}
#endif
