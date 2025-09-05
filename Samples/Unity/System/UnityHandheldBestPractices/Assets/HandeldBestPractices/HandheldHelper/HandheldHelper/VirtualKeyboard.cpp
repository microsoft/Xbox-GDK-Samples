#include "pch.h"

// Virtual keyboard WinRT items are in the ViewManagement namespace
#include <winrt/windows.ui.viewmanagement.core.h>
using namespace winrt::Windows::UI::ViewManagement::Core;

extern "C" __declspec(dllexport)
bool ShowVirtualKeyboard()
{
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.tryshow
    // CoreInputViewKind::Gamepad enum requires headers from 26100.3624 Windows SDK or greater
    // Otherwise, use static_cast<CoreInputViewKind>(7) for earlier SDKs
    return CoreInputView::GetForCurrentView().TryShow(CoreInputViewKind::Gamepad);
}

extern "C" __declspec(dllexport)
bool HideVirtualKeyboard()
{
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.tryhide
    return CoreInputView::GetForCurrentView().TryHide();
}