#pragma once

class D3D12App;
struct RawGamepadReading;
struct _XINPUT_STATE;

void TranslateKey(D3D12App* pApp, UINT message, WPARAM wParam, LPARAM lParam);
void TranslateGamepad(RawGamepadReading* pReading, const _XINPUT_STATE* pXGamepad);
void TranslateMouse(D3D12App* pApp, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
