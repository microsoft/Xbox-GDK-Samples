//--------------------------------------------------------------------------------------
// AccessibilitySample.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "ImGuiAcc.h"

class Sample
{
public:
    Sample() = default;
    ~Sample();

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void Initialize(HWND window);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef _GAMING_XBOX
    void Suspend(ImGuiAtg::DeviceContext* dc);
    void Resume(ImGuiAtg::DeviceContext* dc);
#endif

private:
    ImGuiAcc*   m_imguiAcc = nullptr;
};
