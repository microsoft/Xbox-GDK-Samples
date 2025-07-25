// disable C4365 for cppwinrt happiness
#pragma warning(disable:4265)
#pragma warning(disable:4365)

// Virtual keyboard WinRT items are in the ViewManagement namespace
#include <winrt/windows.ui.viewmanagement.core.h>

// Events and collections used in Show/Hide callbacks
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt::Windows::UI::ViewManagement::Core;

bool ShowVirtualKeyboard()
{
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.tryshow
    // CoreInputViewKind::Gamepad enum requires headers from 26100.3624 Windows SDK or greater
    // Otherwise, use static_cast<CoreInputViewKind>(7) for earlier SDKs
    return CoreInputView::GetForCurrentView().TryShow(CoreInputViewKind::Gamepad);
}

void HideVirtualKeyboard()
{
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.tryhide
    CoreInputView::GetForCurrentView().TryHide();
}

bool IsVirtualKeyboardOverlayed()
{
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.getcoreinputviewocclusions
    auto occlusions = CoreInputView::GetForCurrentView().GetCoreInputViewOcclusions();
    return (occlusions.Size() > 0);
}

static winrt::event_token g_primaryViewShowingToken, g_primaryViewHidingToken;

void RegisterKeyboardShowingEvent()
{
    // Unregister this token in shutodwn with
    // CoreInputView::GetForCurrentView().PrimaryViewShowing(g_primaryViewShowingToken);
    g_primaryViewShowingToken = CoreInputView::GetForCurrentView().PrimaryViewShowing(
        [](CoreInputView const& /*sender*/, CoreInputViewShowingEventArgs const& /*args*/)
        {
            // Event Handler here...
        }
    );
}

void RegisterKeyboardHidingEvent()
{
    // Unregister this token in shutodwn with
    // CoreInputView::GetForCurrentView().PrimaryViewHiding(g_primaryViewHidingToken);
    g_primaryViewHidingToken = CoreInputView::GetForCurrentView().PrimaryViewHiding(
        [](CoreInputView const& /*sender*/, CoreInputViewHidingEventArgs const& /*args*/)
        {
            // Event Handler here...
        }
    );
}

// These event handlers can be used for callbacks when a virtual keyboard
// is shown or hidden:
//
// https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewshowing
// CoreInputView::GetForCurrentView().PrimaryViewShowing
// https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewhiding
// CoreInputView::GetForCurrentView().PrimaryViewHiding

bool Test_VirtualKeyboard()
{
    try
    {
        ShowVirtualKeyboard();
        Sleep(1000);
        HideVirtualKeyboard();
    }
    catch (const winrt::hresult_error& ex)
    {
        std::wstring msg = ex.message().c_str();
        OutputDebugString(msg.c_str());
        return false;
    }

    return true;
}
