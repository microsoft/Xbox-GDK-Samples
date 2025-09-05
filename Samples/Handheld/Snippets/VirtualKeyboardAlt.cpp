// Using the CppWinRT wrappers for the virtual keyboard APIs requires Windows 8 or higher and compiling with exceptions enabled (/EHsc).
// While the entire project does not need to be compiled this way, or one can #pragma disable the warnings that occur when exceptions
// are disabled, neither solution is idea.  Additionally, some titles require compatibility with down-leven versions of Windows where
// WinRT and this API may not be avaialable.

// The following alternative snippet will compile all the way down to Windows 7 and dynamically load/create what is needed
// on current versions of Windows.  Where WinRT and the Gamepad keyboard are not available, this code will silently fail

// disable wanrings
#pragma warning(disable:4191)
#pragma warning(disable:4265)
#pragma warning(disable:4365)

#include <roapi.h>
#include <wrl.h>
#include <windows.ui.viewmanagement.core.h>
#include <functional>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::ViewManagement::Core;
using namespace ABI::Windows::Foundation::Collections;
using namespace Microsoft::WRL;

static EventRegistrationToken g_primaryViewShowingToken, g_primaryViewHidingToken;
static ComPtr<ICoreInputView>  g_coreInputView;
static ComPtr<ICoreInputView3> g_coreInputView3;
static ComPtr<ICoreInputView4> g_coreInputView4;

HRESULT InitializeCoreInputViews()
{
    // if we've already created the views, then exit
    if(g_coreInputView.Get() != nullptr && g_coreInputView3.Get() != nullptr && g_coreInputView4.Get() != nullptr)
    {
        return S_OK;
    }

    // the GetModuleHandle/GetProcAddress pattern used here avoids calling a function on down-level Windows where the function does not exist
    // if the title supports Windows 8 or greater, these functions can be used directly, if preferred
    typedef HRESULT (WINAPI* PFN_RoInitialize)(RO_INIT_TYPE initType);
    typedef HRESULT (WINAPI* PFN_WindowsCreateStringReference)(PCWSTR sourceString, UINT32 length, HSTRING_HEADER *hstringHeader, HSTRING *hstring);
    typedef HRESULT (WINAPI* PFN_RoGetActivationFactory)(HSTRING activatableClassId, REFIID iid, void** factory);

    // get a reference to combase.dll
    HMODULE hModule = GetModuleHandleA("combase.dll");
    if(!hModule)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // get references to the functions we need
    PFN_RoInitialize _RoInitialize = (PFN_RoInitialize)GetProcAddress(hModule, "RoInitialize");
    if(!_RoInitialize)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PFN_WindowsCreateStringReference _WindowsCreateStringReference = (PFN_WindowsCreateStringReference)GetProcAddress(hModule, "WindowsCreateStringReference");
    if(!_WindowsCreateStringReference)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PFN_RoGetActivationFactory _RoGetActivationFactory = (PFN_RoGetActivationFactory)GetProcAddress(hModule, "RoGetActivationFactory");
    if(!_RoGetActivationFactory)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // initialize WinRT on this thread
    HRESULT hr = _RoInitialize(RO_INIT_MULTITHREADED);
    if(FAILED(hr))
    {
        return hr;
    }

    HSTRING_HEADER header{};
    HSTRING hstr{};

    // create an HSTRING containing the CoreInputView class name
    hr = _WindowsCreateStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView, static_cast<UINT32>(wcslen(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView)), &header, &hstr);
    if(FAILED(hr))
    {
        return hr;
    }

    // get the activation factory for the CoreInputView class
    ComPtr<ICoreInputViewStatics> coreInputViewStatics;
    hr = _RoGetActivationFactory(hstr, IID_PPV_ARGS(&coreInputViewStatics));
    if(FAILED(hr))
    {
        return hr;
    }

    // get the CoreInputView for the current view
    hr = coreInputViewStatics->GetForCurrentView(&g_coreInputView);
    if(FAILED(hr))
    {
        return hr;
    }

    // get CoreInputView3/4 references for usage by the functions below
    hr = g_coreInputView.As(&g_coreInputView3);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = g_coreInputView.As(&g_coreInputView4);
    if(FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

bool ShowVirtualKeyboard()
{
    boolean result = false;

    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return false;
    }

    hr = g_coreInputView3->TryShowWithKind(CoreInputViewKind_Gamepad, &result);
    if(FAILED(hr))
    {
        return false;
    }

    return result;
}

bool HideVirtualKeyboard()
{
    boolean result = false;

    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return false;
    }

    hr = g_coreInputView3->TryHide(&result);
    if(FAILED(hr))
    {
        return false;
    }

    return result;
}

// determine if a virtual keyboard is displayed on top of the title window based on occlusion values
bool IsVirtualKeyboardOverlayed()
{
    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return false;
    }

    // https://learn.microsoft.com/uwp/api/windows.ui.viewmanagement.core.coreinputview.getcoreinputviewocclusions?view=winrt-26100
    ComPtr<IVectorView<CoreInputViewOcclusion*>> occlusions;
    hr = g_coreInputView->GetCoreInputViewOcclusions(&occlusions);
    unsigned int size = 0;
    hr = occlusions->get_Size(&size);
    if(FAILED(hr))
    {
        return false;
    }

    return size > 0;
}

// register an event to be called when a VK is shown
HRESULT RegisterKeyboardShowingEvent(std::function<void()> callback)
{
    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return hr;
    }

    hr = g_coreInputView4->add_PrimaryViewShowing(
        Callback<ITypedEventHandler<CoreInputView*, CoreInputViewShowingEventArgs*>>(
            [=](ICoreInputView*, ICoreInputViewShowingEventArgs*)
            {
                // Event Handler here...
                callback();
                return S_OK;
            }).Get(), &g_primaryViewShowingToken);

    return hr;
}

// register an event to be called when a VK is hidden
HRESULT RegisterKeyboardHidingEvent(std::function<void()> callback)
{
    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return hr;
    }

    hr = g_coreInputView4->add_PrimaryViewHiding(
        Callback<ITypedEventHandler<CoreInputView*, CoreInputViewHidingEventArgs*>>(
            [=](ICoreInputView*, ICoreInputViewHidingEventArgs*)
            {
                // Event Handler here...
                callback();
                return S_OK;
            }).Get(), &g_primaryViewHidingToken);

    return hr;
}

// unregister events
void UnregisterKeyboardShowingEvent()
{
    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return;
    }

    hr = g_coreInputView4->remove_PrimaryViewShowing(g_primaryViewShowingToken);
    g_primaryViewShowingToken.value = 0;
}

void UnregisterKeyboardHidingEvent()
{
    HRESULT hr = InitializeCoreInputViews();
    if(FAILED(hr))
    {
        return;
    }

    hr = g_coreInputView4->remove_PrimaryViewHiding(g_primaryViewHidingToken);
    g_primaryViewHidingToken.value = 0;
}
