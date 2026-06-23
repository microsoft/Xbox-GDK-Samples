//--------------------------------------------------------------------------------------
// HandheldBestPractices.h
//
// Header for sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class Sample
{
public:
    void Initialize(HWND hWnd);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
};
