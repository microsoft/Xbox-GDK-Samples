// disable wanrings
#pragma warning(disable:4191)
#pragma warning(disable:4265)
#pragma warning(disable:4365)

// required lib for WinRT
#pragma comment(lib, "runtimeobject.lib")

#include <functional>

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
    return (occlusions.Size() > 0); // if there is any occlusion, the keyboard must be displayed
}

static winrt::event_token g_primaryViewShowingToken, g_primaryViewHidingToken;

void RegisterKeyboardShowingEvent(std::function<void()> callback)
{
    // Unregister this token in shutodwn with
    // CoreInputView::GetForCurrentView().PrimaryViewShowing(g_primaryViewShowingToken);
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewshowing
    g_primaryViewShowingToken = CoreInputView::GetForCurrentView().PrimaryViewShowing(
        [=](CoreInputView const& /*sender*/, CoreInputViewShowingEventArgs const& /*args*/)
        {
            // Event Handler here...
            callback();
        }
    );
}

void RegisterKeyboardHidingEvent(std::function<void()> callback)
{
    // Unregister this token in shutodwn with
    // CoreInputView::GetForCurrentView().PrimaryViewHiding(g_primaryViewHidingToken);
    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.primaryviewhiding
    g_primaryViewHidingToken = CoreInputView::GetForCurrentView().PrimaryViewHiding(
        [=](CoreInputView const& /*sender*/, CoreInputViewHidingEventArgs const& /*args*/)
        {
            // Event Handler here...
            callback();
        }
    );
}

void UnregisterKeyboardShowingEvent()
{
    if(g_primaryViewShowingToken.value)
    {
        CoreInputView::GetForCurrentView().PrimaryViewShowing(g_primaryViewShowingToken);
        g_primaryViewShowingToken.value = 0;
    }
}

void UnregisterKeyboardHidingEvent()
{
    if(g_primaryViewHidingToken.value)
    {
        CoreInputView::GetForCurrentView().PrimaryViewShowing(g_primaryViewHidingToken);
        g_primaryViewHidingToken.value = 0;
    }
}
