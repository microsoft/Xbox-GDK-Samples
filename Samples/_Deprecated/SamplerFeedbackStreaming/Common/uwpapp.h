//--------------------------------------------------------------------------------------
// uwpapp.h
//
// Application view framework for devtests running on the Universal Windows platform.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class D3D12App;

// Application - implements the required functionality for a application
ref class ApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:

    ApplicationView();

    // IFrameworkView Methods
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

internal:
    void SetD3D12App(D3D12App* pApp) { m_pApp = pApp; }

protected:

    // Event Handlers
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
    void OnResuming(Platform::Object^ sender, Platform::Object^ args);

    void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
    void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

    void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
    void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

#if !defined(_XBOX_ONE)
    void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    void OnMouseMoved(Windows::Devices::Input::MouseDevice^ sender, Windows::Devices::Input::MouseEventArgs^ args);
    void OnPointerEvent(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
#endif

private:

    D3D12App* m_pApp;
    bool m_windowClosed;
    bool m_windowVisible;
    bool m_leftMouseDown;
#if !defined(_XBOX_ONE)
    Windows::UI::Core::CoreCursor^ m_SavedCursor;
#endif
};

// ApplicationSource - responsible for creating the Application instance 
// and passing it back to the system
ref class ApplicationViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
private:
    D3D12App* m_pApp;

public:
    ApplicationViewSource()
        : m_pApp(nullptr)
    { }
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();

internal:
    void SetApp(D3D12App* pApp) { m_pApp = pApp; }
};

#if !defined(_XBOX_ONE)
void DpiAdjustWindowBounds(FLOAT& BoundsWidth, FLOAT& BoundsHeight);
#endif
