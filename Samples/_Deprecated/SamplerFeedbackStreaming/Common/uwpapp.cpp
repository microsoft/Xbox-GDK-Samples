#include "precomp.hpp"
#include "d3d12app.hpp"

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
//--------------------------------------------------------------------------------------
// uwpapp.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using namespace Windows::Foundation::Collections;
#if !defined(_XBOX_ONE)
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Input;
#endif

#if !defined(_XBOX_ONE)
void DpiAdjustWindowBounds(FLOAT& BoundsWidth, FLOAT& BoundsHeight)
{
    DisplayInformation^ displayInformation = DisplayInformation::GetForCurrentView();
    FLOAT dpi = displayInformation->LogicalDpi;

    BoundsWidth = floorf(BoundsWidth * (dpi / 96.0f) + 0.5f);
    BoundsHeight = floorf(BoundsHeight * (dpi / 96.0f) + 0.5f);
}
#endif

ApplicationView::ApplicationView()
{
    m_pApp = nullptr;
    m_windowClosed = false;
    m_windowVisible = true;
    m_leftMouseDown = false;
}

// Called by the system.  Perform application initialization here,
// hooking application wide events, etc.
void ApplicationView::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ApplicationView::OnActivated);
    CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &ApplicationView::OnSuspending);
    CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &ApplicationView::OnResuming);
}

// Called when we are provided a window.
void ApplicationView::SetWindow(CoreWindow^ window)
{
    window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ApplicationView::OnWindowClosed);

    window->SizeChanged +=
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &ApplicationView::OnWindowSizeChanged);

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &ApplicationView::OnVisibilityChanged);

#if !defined(_XBOX_ONE)
    DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

    currentDisplayInformation->DpiChanged +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ApplicationView::OnDpiChanged);

    currentDisplayInformation->OrientationChanged +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ApplicationView::OnOrientationChanged);

    DisplayInformation::DisplayContentsInvalidated +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ApplicationView::OnDisplayContentsInvalidated);

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &ApplicationView::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &ApplicationView::OnKeyUp);

    MouseDevice^ mouse = MouseDevice::GetForCurrentView();
    mouse->MouseMoved += ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &ApplicationView::OnMouseMoved);

    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ApplicationView::OnPointerEvent);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ApplicationView::OnPointerEvent);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ApplicationView::OnPointerEvent);
#endif

    assert(m_pApp != nullptr);
    m_pApp->WindowCreated(window);
}

// The purpose of this method is to get the application entry point.
void ApplicationView::Load(Platform::String^ entryPoint)
{
}

// Called by the system after initialization is complete.  This
// implements the traditional game loop
void ApplicationView::Run()
{
    // Initialize sample
    HRESULT hr = m_pApp->FrameworkInitialize();
    DX::ThrowIfFailed(hr);

    hr = m_pApp->Initialize();
    DX::ThrowIfFailed(hr);

    CoreDispatcher^ dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

    while (!m_windowClosed)
    {
        dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

        m_pApp->Tick();
    }

    m_pApp->WindowClosing();
}

void ApplicationView::Uninitialize()
{
}

// Called when the application is activated.
void ApplicationView::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Launch)
    {
        ILaunchActivatedEventArgs^ launchArgs = (ILaunchActivatedEventArgs^)args;
        /*const wchar_t* strArguments = (const wchar_t*)launchArgs->Arguments->Data();
        m_pApp->ParseCommandLine(1, &strArguments);*/
    }
    CoreWindow::GetForCurrentThread()->Activate();
}

// Called when the application is suspending.
void ApplicationView::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // TODO: Save game progress using the ConnectedStroage API.
    m_pApp->SuspendRendering();
}

// Called when the application is resuming from suspended.
void ApplicationView::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // TODO: Handle changes in users and input devices.
    m_pApp->ResumeRendering();
}

void ApplicationView::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
    m_windowClosed = true;
}

void ApplicationView::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
    Windows::Foundation::Rect BoundsRect = sender->Bounds;

#if !defined(_XBOX_ONE)
    DpiAdjustWindowBounds(BoundsRect.Width, BoundsRect.Height);
#endif

    m_pApp->WindowResize(BoundsRect.Width, BoundsRect.Height);
}

void ApplicationView::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
    m_windowVisible = args->Visible;
}

struct TranslationTableEntry
{
    Windows::System::VirtualKey vkey;
    GamepadButtons button;
    UINT32 StickButton;
};

static const TranslationTableEntry g_KeyTranslationTable[] =
{
    { Windows::System::VirtualKey::Enter,       GamepadButtons::A,              0x00 },
    { Windows::System::VirtualKey::B,           GamepadButtons::B,              0x00 },
    { Windows::System::VirtualKey::X,           GamepadButtons::X,              0x00 },
    { Windows::System::VirtualKey::Y,           GamepadButtons::Y,              0x00 },
    { Windows::System::VirtualKey::Left ,       GamepadButtons::DPadLeft,       0x00 },
    { Windows::System::VirtualKey::Right,       GamepadButtons::DPadRight,      0x00 },
    { Windows::System::VirtualKey::Up   ,       GamepadButtons::DPadUp,         0x00 },
    { Windows::System::VirtualKey::Down ,       GamepadButtons::DPadDown,       0x00 },
    { Windows::System::VirtualKey::Escape,      GamepadButtons::View,           0x00 },
    { Windows::System::VirtualKey::Home,        GamepadButtons::Menu,           0x00 },
    { Windows::System::VirtualKey::Number9,     GamepadButtons::LeftShoulder,   0x00 },
    { Windows::System::VirtualKey::Number0,     GamepadButtons::RightShoulder,  0x00 },
    { Windows::System::VirtualKey::A,           GamepadButtons::None,           0x01 },
    { Windows::System::VirtualKey::S,           GamepadButtons::None,           0x04 },
    { Windows::System::VirtualKey::W,           GamepadButtons::None,           0x08 },
    { Windows::System::VirtualKey::D,           GamepadButtons::None,           0x02 },
    { Windows::System::VirtualKey::Shift,       GamepadButtons::RightShoulder,  0x00 },
};

void ApplicationView::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
    for (TranslationTableEntry Entry : g_KeyTranslationTable)
    {
        if (args->VirtualKey == Entry.vkey)
        {
            m_pApp->SetTranslatedButtons(true, (UINT32)Entry.button, Entry.StickButton);
            break;
        }
    }
}

void ApplicationView::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
    for (TranslationTableEntry Entry : g_KeyTranslationTable)
    {
        if (args->VirtualKey == Entry.vkey)
        {
            m_pApp->SetTranslatedButtons(false, (UINT32)Entry.button, Entry.StickButton);
            break;
        }
    }
}

#if !defined(_XBOX_ONE)
void ApplicationView::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
}

void ApplicationView::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
}

void ApplicationView::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
}

void ApplicationView::OnMouseMoved(Windows::Devices::Input::MouseDevice^ sender, Windows::Devices::Input::MouseEventArgs^ args)
{
    if (m_leftMouseDown)
    {
        MouseDelta delta = args->MouseDelta;
        m_pApp->OnMouseDelta(delta.X, delta.Y);
    }
}

void ApplicationView::OnPointerEvent(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
    PointerDevice^ dev = args->CurrentPoint->PointerDevice;
    if (dev->PointerDeviceType != PointerDeviceType::Mouse)
    {
        return;
    }

    PointerPointProperties^ ppp = args->CurrentPoint->Properties;
    const bool leftMouseDown = ppp->IsLeftButtonPressed;
    if (leftMouseDown && !m_leftMouseDown)
    {
        m_SavedCursor = sender->PointerCursor;
        sender->PointerCursor = nullptr;
    }
    else if (m_leftMouseDown && !leftMouseDown)
    {
        sender->PointerCursor = m_SavedCursor;
        m_SavedCursor = nullptr;
    }
    m_leftMouseDown = leftMouseDown;
}
#endif

// Implements a IFrameworkView factory.
IFrameworkView^ ApplicationViewSource::CreateView()
{
    auto appview = ref new ApplicationView();
    appview->SetD3D12App(m_pApp);
    return appview;
}
