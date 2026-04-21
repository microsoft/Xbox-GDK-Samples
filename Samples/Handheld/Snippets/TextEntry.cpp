//--------------------------------------------------------------------------------------
// TextEntry.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Straight Win32 implementation of a text entry dialog, similar to the GDK's XGameUiShowTextEntryAsync API.
// This has no dependencies on the GDK, UWP, WinRT, or anything else external to the Windows SDK and OS.
// Supports a very simple light and dark theme.
//
// To use:
//   - add VirtualKeyboard.cpp (CppWinRT) or VirtualKeyboardAlt.cpp (downlevel compileable) to your project
//   - add TextEntry.cpp/.h to your project
//   - #include "TextEntry.h" where you will call this TextEntry dialog
//   - Call ATG::ShowTextEntry() with appropriate parameters
//   - In the callback function, use or copy the string the user entered (or handle cancellation)
//   - Optional: Adjust window and font parameters using the constants at the top of this file
//   - Optional: Depending on how input is handled by your main application, you may need or want
//               to disable input handling before the call to ShowTextEntry and re-enable in the callback.
//               Otherwise, gamepad input may be processed by both the dialog and your main app.
// 
// Example:
//   ATG::ShowTextEntry("A Simple Text Entry Dialog", "This is an area to enter some text.  Try it!", "Default Text", 24,
//                      [](void* userContext, bool confirmed, const char* resultTextBuffer, uint32_t resultTextBufferUsed)
//   {
//        if(confirmed)
//            LOG("Entered text: %s - Length: \n", resultTextBuffer, resultTextBufferUsed);
//        else
//            LOG("Text entry canceled\n");
//   }, nullptr);

#include <Windows.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <Xinput.h>
#include <memory>
#include <string>
#include "TextEntry.h"

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Xinput.lib")
#pragma comment(lib, "Dwmapi.lib")

namespace ATG
{
    // window size
    constexpr int kWindowWidth = 660;
    constexpr int kWindowHeight = 260;

    // window padding
    constexpr int kWindowMarginTop = 14;
    constexpr int kWindowMarginLeft = 30;

    // button size
    constexpr int kButtonWidth = 280;
    constexpr int kButtonHeight = 60;

    // font size
    constexpr wchar_t kFontName[] = L"Segoe UI";
    constexpr int kFontSize = 13;

    constexpr wchar_t kClassName[] = L"ATG_TextEntryDialog";
    constexpr uint32_t kMaxInputTextLength = 1024;
    constexpr UINT ID_DESC = 1000;
    constexpr UINT ID_EDIT = 1001;
    constexpr UINT_PTR kEditSubclassId = 1;
    constexpr UINT_PTR kButtonSubclassId = 2;
    constexpr UINT_PTR kGamepadTimerId = 3;

    // fwd decls
    static std::wstring Utf8ToWide(const char* utf8);
    static std::string WideToUtf8(const wchar_t* wide);
    LRESULT CALLBACK TextEntryWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    struct ThemeElements
    {
        HBRUSH bgBrush{};
        HBRUSH editBgBrush{};
        COLORREF textColor{};
        HFONT hFont{};
        bool light{};
    };

    struct TextEntryCtx
    {
        ThemeElements theme{};
        HWND hParent{};
        HWND hWnd{};
        HWND hEdit{};
        HWND hDesc{};
        HWND hOk{};
        HWND hCancel{};
        const char* descriptionText{};
        const char* defaultText{};
        TextEntryCallback* callback{};
        void* context{};
        uint32_t maxTextLength = kMaxInputTextLength;
    };

    // Modeless, non-blocking. Returns immediately.
    // The app's existing PeekMessage/Dispatch loop keeps pumping messages for both windows.
    bool ShowTextEntry(const char* titleText, const char* descriptionText, const char* defaultText, uint32_t maxTextLength, TextEntryCallback callback, void* context)
    {
        // Prevent multiple instances
        HWND existing = FindWindowW(kClassName, nullptr);
        if (existing)
        {
            SetForegroundWindow(existing);
            return true;
        }

        HWND hParent = GetForegroundWindow();
        if (!hParent)
        {
            hParent = GetActiveWindow();
        }
        if (!hParent)
        {
            return false;
        }

        // ensure we only register the windowclass once
        static ATOM s_atom{};
        if (!s_atom)
        {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = TextEntryWndProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.lpszClassName = kClassName;
            s_atom = RegisterClassExW(&wc);
        }

        if(maxTextLength == 0 || maxTextLength > kMaxInputTextLength)
        {
            maxTextLength = kMaxInputTextLength;
        }

        TextEntryCtx* ctx = new TextEntryCtx{};
        ctx->hParent = hParent;
        ctx->descriptionText = descriptionText;
        ctx->defaultText = defaultText;
        ctx->maxTextLength = maxTextLength;
        ctx->callback = callback;
        ctx->context = context;

        // center the window to the parent
        RECT rcParent{};
        GetWindowRect(hParent, &rcParent);
        const int x = rcParent.left + ((rcParent.right - rcParent.left) - kWindowWidth) / 2;
        const int y = rcParent.top + ((rcParent.bottom - rcParent.top) - kWindowHeight) / 2;

        HWND hWnd = CreateWindowExW(WS_EX_DLGMODALFRAME,
            kClassName,
            Utf8ToWide(titleText).c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            x, y, kWindowWidth, kWindowHeight,
            hParent, nullptr, GetModuleHandleW(nullptr), ctx);

        if (!hWnd)
        {
            delete ctx;
            return false;
        }

        SetForegroundWindow(hWnd);

        return true;
    }

    // UTF-8 <-> wide conversion helpers
    static std::wstring Utf8ToWide(const char* utf8)
    {
        if (!utf8) return {};

        // Determine required length
        int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, nullptr, 0);

        // Allocate and convert
        std::wstring wide;
        wide.resize(static_cast<size_t>(required));
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, wide.data(), required);
        return wide;
    }

    static std::string WideToUtf8(const wchar_t* wide)
    {
        if (!wide) return {};

        // Determine required length
        int required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, nullptr, 0, nullptr, nullptr);

        // Allocate and convert
        std::string utf8;
        utf8.resize(static_cast<size_t>(required));
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, utf8.data(), required, nullptr, nullptr);
        return utf8;
    }

    static void SetFonts(TextEntryCtx* ctx)
    {
        if(!ctx) return;

        SendMessageW(ctx->hDesc,   WM_SETFONT, reinterpret_cast<WPARAM>(ctx->theme.hFont), TRUE);
        SendMessageW(ctx->hEdit,   WM_SETFONT, reinterpret_cast<WPARAM>(ctx->theme.hFont), TRUE);
        SendMessageW(ctx->hOk,     WM_SETFONT, reinterpret_cast<WPARAM>(ctx->theme.hFont), TRUE);
        SendMessageW(ctx->hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(ctx->theme.hFont), TRUE);
    }

    static void DestroyTheme(ThemeElements& tb)
    {
        if (tb.bgBrush)     { DeleteObject(tb.bgBrush);     tb.bgBrush = nullptr; }
        if (tb.editBgBrush) { DeleteObject(tb.editBgBrush); tb.editBgBrush = nullptr; }
        if (tb.hFont)       { DeleteObject(tb.hFont);       tb.hFont = nullptr; }
    }

    static void ApplyTheme(TextEntryCtx* ctx)
    {
        if (!ctx) return;

        DestroyTheme(ctx->theme);

        // determine light/dark theme
        DWORD appsUseLightTheme = 1;
        DWORD cb = sizeof(appsUseLightTheme);
        RegGetValueW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD, nullptr, &appsUseLightTheme, &cb);

        ctx->theme.light = (appsUseLightTheme != 0);

        // create simple palette for light/dark
        const COLORREF wndBk  = ctx->theme.light ? RGB(255, 255, 255) : RGB(32, 32, 32);
        const COLORREF ctrlBk = ctx->theme.light ? RGB(255, 255, 255) : RGB(45, 45, 45);
        ctx->theme.textColor  = ctx->theme.light ? RGB(0, 0, 0) : RGB(240, 240, 240);
        ctx->theme.bgBrush     = CreateSolidBrush(wndBk);
        ctx->theme.editBgBrush = CreateSolidBrush(ctrlBk);

        // create a font that's scaled for the current DPI
        LOGFONTW lf{};
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
        lf.lfPitchAndFamily = FF_DONTCARE;
        wcscpy_s(lf.lfFaceName, kFontName);

        UINT dpiX = 96, dpiY = 96;
        GetDpiForMonitor(MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY), MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

        lf.lfHeight = -MulDiv(kFontSize, dpiY, 72);
        lf.lfWeight = FW_NORMAL;
        ctx->theme.hFont = CreateFontIndirectW(&lf);

        BOOL dark = !ctx->theme.light;
        DwmSetWindowAttribute(ctx->hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

        SetFonts(ctx);
    }

    void HandleGamepadInput(TextEntryCtx* ctx)
    {
        if (!ctx)
        {
            return;
        }

        static DWORD lastPacketNumber[XUSER_MAX_COUNT] = {};
        static WORD lastButtons[XUSER_MAX_COUNT] = {};
        static WORD lastStickDir[XUSER_MAX_COUNT] = {};

        const SHORT STICK_DEADZONE = 16384;

        for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
        {
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(XINPUT_STATE));

            DWORD dwResult = XInputGetState(i, &state);

            if (dwResult == ERROR_SUCCESS)
            {
                // Only process on button state change to avoid repeats
                if (state.dwPacketNumber == lastPacketNumber[i])
                {
                    continue;
                }

                // Convert left stick to dpad buttons
                WORD stickDir = 0;
                if (state.Gamepad.sThumbLY > STICK_DEADZONE)
                {
                    stickDir |= XINPUT_GAMEPAD_DPAD_UP;
                }
                else if (state.Gamepad.sThumbLY < -STICK_DEADZONE)
                {
                    stickDir |= XINPUT_GAMEPAD_DPAD_DOWN;
                }
                if (state.Gamepad.sThumbLX > STICK_DEADZONE)
                {
                    stickDir |= XINPUT_GAMEPAD_DPAD_RIGHT;
                }
                else if (state.Gamepad.sThumbLX < -STICK_DEADZONE)
                {
                    stickDir |= XINPUT_GAMEPAD_DPAD_LEFT;
                }

                // Combine real D-pad with stick directions
                WORD combinedDir = state.Gamepad.wButtons | stickDir;
                WORD lastCombinedDir = lastButtons[i] | lastStickDir[i];

                WORD pressed = state.Gamepad.wButtons & ~lastButtons[i];
                WORD pressedDir = combinedDir & ~lastCombinedDir;

                lastPacketNumber[i] = state.dwPacketNumber;
                lastButtons[i] = state.Gamepad.wButtons;
                lastStickDir[i] = stickDir;

                HWND hFocus = GetFocus();

                // A button activates the focused control or focuses edit
                if (pressed & XINPUT_GAMEPAD_A)
                {
                    if (hFocus == ctx->hOk)
                    {
                        PostMessageW(ctx->hWnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)ctx->hOk);
                    }
                    else if (hFocus == ctx->hCancel)
                    {
                        PostMessageW(ctx->hWnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)ctx->hCancel);
                    }
                    else
                    {
                        SetFocus(ctx->hEdit);
                    }
                }

                // B button cancels
                if (pressed & XINPUT_GAMEPAD_B)
                {
                    PostMessageW(ctx->hWnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)ctx->hCancel);
                }

                // Dpad / left stick navigation between controls
                if (pressedDir & XINPUT_GAMEPAD_DPAD_DOWN)
                {
                    // Edit -> OK or Cancel (based on last horizontal position)
                    if (hFocus == ctx->hEdit)
                    {
                        SetFocus(ctx->hOk);
                    }
                }

                if (pressedDir & XINPUT_GAMEPAD_DPAD_UP)
                {
                    // OK or Cancel -> Edit
                    if (hFocus == ctx->hOk || hFocus == ctx->hCancel)
                    {
                        SetFocus(ctx->hEdit);
                    }
                }

                if (pressedDir & XINPUT_GAMEPAD_DPAD_LEFT)
                {
                    // Cancel -> OK
                    if (hFocus == ctx->hCancel)
                    {
                        SetFocus(ctx->hOk);
                    }
                }

                if (pressedDir & XINPUT_GAMEPAD_DPAD_RIGHT)
                {
                    // OK -> Cancel
                    if (hFocus == ctx->hOk)
                    {
                        SetFocus(ctx->hCancel);
                    }
                }
            }
        }
    }

    // SubclassProc for the edit control (Enter/Escape -> OK/Cancel, tab navigation)
    static LRESULT CALLBACK EditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
    {
        HWND hWndParent = GetParent(hEdit);

        switch(msg)
        {
            case WM_GETDLGCODE:
            {
                // Allow Tab to move focus, but handle Enter/Escape ourselves
                return DLGC_WANTARROWS | DLGC_WANTCHARS;
            }

            case WM_KEYDOWN:
            {
                if (wParam == VK_RETURN)
                {
                    PostMessageW(hWndParent, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), reinterpret_cast<LPARAM>(hEdit));
                    return 0;
                }
                else if (wParam == VK_ESCAPE)
                {
                    PostMessageW(hWndParent, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), reinterpret_cast<LPARAM>(hEdit));
                    return 0;
                }
                else if (wParam == VK_TAB)
                {
                    BOOL bShift = GetKeyState(VK_SHIFT) & 0x8000;
                    HWND hNext = GetNextDlgTabItem(hWndParent, hEdit, bShift);
                    if (hNext)
                    {
                        SetFocus(hNext);
                    }
                    return 0;
                }
                break;
            }

            case WM_NCDESTROY:
            {
                RemoveWindowSubclass(hEdit, EditSubclassProc, kEditSubclassId);
                break;
            }
        }

        return DefSubclassProc(hEdit, msg, wParam, lParam);
    }

    // SubclassProc for buttons (Tab navigation)
    static LRESULT CALLBACK ButtonSubclassProc(HWND hBtn, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
    {
        HWND hWndParent = GetParent(hBtn);

        switch(msg)
        {
            case WM_KEYDOWN:
            {
                if (wParam == VK_TAB)
                {
                    BOOL bShift = GetKeyState(VK_SHIFT) & 0x8000;
                    HWND hNext = GetNextDlgTabItem(hWndParent, hBtn, bShift);
                    if (hNext)
                    {
                        SetFocus(hNext);
                    }
                    return 0;
                }
                break;
            }
            case WM_NCDESTROY:
            {
                RemoveWindowSubclass(hBtn, ButtonSubclassProc, kButtonSubclassId);
                break;
            }
        }

        return DefSubclassProc(hBtn, msg, wParam, lParam);
    }

    static void DrawFlatButton(TextEntryCtx* ctx, const DRAWITEMSTRUCT* dis)
    {
        if (!ctx || !dis) return;

        const bool isDisabled = (dis->itemState & ODS_DISABLED) != 0;
        const bool isSelected = (dis->itemState & ODS_SELECTED) != 0;
        const bool isFocus    = (dis->itemState & ODS_FOCUS) != 0;

        // Base colors
        COLORREF baseBk, downBk, border, text;

        if (isDisabled)
        {
            text   = ctx->theme.light ? RGB(160, 160, 160) : RGB(120, 120, 120);
            baseBk = ctx->theme.light ? RGB(250, 250, 250) : RGB(38, 38, 38);
            border = ctx->theme.light ? RGB(235, 235, 235) : RGB(70, 70, 70);
            downBk = baseBk;
        }
        else
        {
            text    = ctx->theme.textColor;
            baseBk  = ctx->theme.light ? RGB(245, 245, 245) : RGB(40, 40, 40);
            border  = ctx->theme.light ? RGB(210, 210, 210) : RGB(80, 80, 80);
            downBk  = ctx->theme.light ? RGB(220, 220, 220) : RGB(65, 65, 65);
        }

        COLORREF bk = isSelected ? downBk : baseBk;

        // Paint background
        HBRUSH hBk = CreateSolidBrush(bk);
        FillRect(dis->hDC, &dis->rcItem, hBk);
        DeleteObject(hBk);

        // Draw border
        HPEN hPen = CreatePen(PS_SOLID, 2, border);
        HGDIOBJ oldPen = SelectObject(dis->hDC, hPen);
        HGDIOBJ oldBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
        SelectObject(dis->hDC, oldBrush);
        SelectObject(dis->hDC, oldPen);
        DeleteObject(hPen);

        // Fetch button text
        wchar_t textBuf[64]{};
        GetWindowTextW(dis->hwndItem, textBuf, (int)_countof(textBuf));

        // Set font
        HFONT oldFont = (HFONT)SelectObject(dis->hDC, ctx->theme.hFont);
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, text);

        // Center text with padding
        const int padX = 6;
        const int padY = 2;

        RECT rcText = dis->rcItem;
        rcText.left += padX;
        rcText.right -= padX;
        rcText.top += padY;
        rcText.bottom -= padY;

        // Offset when pressed
        if (isSelected && !isDisabled)
        {
            OffsetRect(&rcText, 1, 1);
        }

        DrawTextW(dis->hDC, textBuf, -1, &rcText, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

        // Focus rectangle
        if (isFocus && !isDisabled)
        {
            RECT rcFocus = dis->rcItem;
            rcFocus.left += 3; rcFocus.top += 3;
            rcFocus.right -= 3; rcFocus.bottom -= 3;
            DrawFocusRect(dis->hDC, &rcFocus);
        }

        SelectObject(dis->hDC, oldFont);
    }

    LRESULT CALLBACK TextEntryWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        TextEntryCtx* ctx = reinterpret_cast<TextEntryCtx*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

        switch (msg)
        {
            case WM_CREATE:
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                ctx = reinterpret_cast<TextEntryCtx*>(cs->lpCreateParams);
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
                ctx->hWnd = hWnd;

                ApplyTheme(ctx);

                // Make parent effectively modal without blocking the thread
                EnableWindow(ctx->hParent, FALSE);

                RECT rc{};

                GetClientRect(hWnd, &rc);

                // Description static
                int descH = 64;
                ctx->hDesc = CreateWindowExW(0, L"STATIC", L"",
                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                    kWindowMarginLeft, kWindowMarginTop,
                    rc.right - rc.left - kWindowMarginLeft * 2, descH,
                    hWnd, (HMENU)(UINT_PTR)ID_DESC, GetModuleHandleW(nullptr), nullptr);
                SetWindowTextW(ctx->hDesc, Utf8ToWide(ctx->descriptionText).c_str());

                // Edit control
                int editTop = kWindowMarginTop + descH + 6;

                // Measure the font to determine appropriate edit control height
                int editH = 32;
                HDC hdc = GetDC(hWnd);
                if (hdc)
                {
                    HFONT hOld = (HFONT)SelectObject(hdc, ctx->theme.hFont);
                    TEXTMETRICW tm{};
                    if (GetTextMetricsW(hdc, &tm))
                    {
                        editH = tm.tmHeight + tm.tmExternalLeading + 4;
                        if (editH < 24)
                        {
                            editH = 24;
                        }
                    }
                    SelectObject(hdc, hOld);
                    ReleaseDC(hWnd, hdc);
                }

                ctx->hEdit = CreateWindowExW(0, L"EDIT", L"",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
                    kWindowMarginLeft, editTop, rc.right - rc.left - kWindowMarginLeft * 2, editH,
                    hWnd, (HMENU)(UINT_PTR)ID_EDIT, GetModuleHandleW(nullptr), nullptr);
                SendMessageW(ctx->hEdit, EM_LIMITTEXT, ctx->maxTextLength, 0);
                SendMessageW(ctx->hEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(6, 6));
                SetWindowTextW(ctx->hEdit, Utf8ToWide(ctx->defaultText).c_str());
                SendMessageW(ctx->hEdit, EM_SETSEL, 0, -1);
                SetWindowSubclass(ctx->hEdit, EditSubclassProc, kEditSubclassId, reinterpret_cast<DWORD_PTR>(ctx));

                // OK button (owner-draw flat)
                ctx->hOk = CreateWindowExW(0, L"BUTTON", L"OK",
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
                    kWindowMarginLeft, rc.bottom - kWindowMarginTop - kButtonHeight, kButtonWidth, kButtonHeight,
                    hWnd, (HMENU)(UINT_PTR)IDOK, GetModuleHandleW(nullptr), nullptr);
                SetWindowSubclass(ctx->hOk, ButtonSubclassProc, kButtonSubclassId, 0);

                // Cancel button (owner-draw flat)
                ctx->hCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
                    rc.right - kWindowMarginLeft - kButtonWidth, rc.bottom - kWindowMarginTop - kButtonHeight, kButtonWidth, kButtonHeight,
                    hWnd, (HMENU)(UINT_PTR)IDCANCEL, GetModuleHandleW(nullptr), nullptr);
                SetWindowSubclass(ctx->hCancel, ButtonSubclassProc, kButtonSubclassId, 0);

                // set fonts for all of the controls
                SetFonts(ctx);

                // set a ~60fps timer to poll gamepad input
                SetTimer(hWnd, kGamepadTimerId, 16, nullptr);

                // Focus edit and bring up VK
                SetFocus(ctx->hEdit);
                ShowVirtualKeyboard();
                return 0;
            }
            case WM_TIMER:
            {
                if (!ctx || wParam != kGamepadTimerId) break;
                HandleGamepadInput(ctx);
                return 0;
            }
            case WM_ERASEBKGND:
            {
                if (!ctx) break;
                RECT rc{};
                GetClientRect(hWnd, &rc);
                FillRect(reinterpret_cast<HDC>(wParam), &rc, ctx->theme.bgBrush);
                return 1;
            }
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLOREDIT:
            {
                if (!ctx) break;
                HDC hdc = reinterpret_cast<HDC>(wParam);
                SetTextColor(hdc, ctx->theme.textColor);
                SetBkMode(hdc, TRANSPARENT);
                return reinterpret_cast<LRESULT>(msg == WM_CTLCOLOREDIT ? ctx->theme.editBgBrush : ctx->theme.bgBrush);
            }
            case WM_DRAWITEM:
            {
                if (!ctx) break;
                const auto* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
                if (dis && (dis->CtlID == IDOK || dis->CtlID == IDCANCEL))
                {
                    DrawFlatButton(ctx, dis);
                    return TRUE;
                }
                break;
            }
            case WM_COMMAND:
            {
                const UINT id = LOWORD(wParam);
                const UINT notify = HIWORD(wParam);
                const HWND hCtl = (HWND)lParam;

                // Show the VK whenever the edit control regains focus
                if (id == ID_EDIT && notify == EN_SETFOCUS && hCtl == (ctx ? ctx->hEdit : nullptr))
                {
                    ShowVirtualKeyboard();
                    return 0;
                }

                if (id == IDOK)
                {
                    wchar_t s_tempBuffer[kMaxInputTextLength+1] = {};

                    HideVirtualKeyboard();
                    GetWindowTextW(ctx->hEdit, s_tempBuffer, _countof(s_tempBuffer));
                    if (ctx->callback)
                    {
                        ctx->callback(ctx->context, true, WideToUtf8(s_tempBuffer).c_str(), lstrlenW(s_tempBuffer)+1);
                    }
                    DestroyWindow(hWnd);
                    return 0;
                }

                if (id == IDCANCEL)
                {
                    if (ctx->callback)
                    {
                        ctx->callback( ctx->context, false, nullptr, 0);
                    }

                    HideVirtualKeyboard();
                    DestroyWindow(hWnd);
                    return 0;
                }
                break;
            }
            case WM_SETTINGCHANGE:
            case WM_THEMECHANGED:
            {
                if (ctx)
                {
                    ApplyTheme(ctx);
                    InvalidateRect(hWnd, nullptr, TRUE);
                }
                return 0;
            }
            case WM_CLOSE:
            {
                HideVirtualKeyboard();
                DestroyWindow(hWnd);
                return 0;
            }

            case WM_DESTROY:
            {
                if (ctx)
                {
                    // Re-enable parent and return focus when window is closed
                    EnableWindow(ctx->hParent, TRUE);
                    SetForegroundWindow(ctx->hParent);
                    if (ctx->hEdit)
                    {
                        RemoveWindowSubclass(ctx->hEdit, EditSubclassProc, kEditSubclassId);
                    }
                    DestroyTheme(ctx->theme);
                    delete ctx;
                }
                return 0;
            }
        }

        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}
