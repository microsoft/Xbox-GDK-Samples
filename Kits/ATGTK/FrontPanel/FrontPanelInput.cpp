//--------------------------------------------------------------------------------------
// FrontPanelInput.cpp
//
// Microsoft GDK with Xbox extensions
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "FrontPanelInput.h"

using namespace ATG;

using Microsoft::WRL::ComPtr;

// --------------------------------------------------------------------------------
// FrontPanelInput::Impl definition
// --------------------------------------------------------------------------------
#pragma region FrontPanelInput::Impl definition
class FrontPanelInput::Impl
{
public:
    Impl(FrontPanelInput *owner)
        : mOwner(owner)
    {
        if (s_frontPanelInput)
        {
            throw std::exception("FrontPanelInput is a singleton");
        }

        s_frontPanelInput = this;
        mAvailable = XFrontPanelIsSupported();
    }

    FrontPanelInput *mOwner;
    static FrontPanelInput::Impl *s_frontPanelInput;
    bool mAvailable;

    void GetState(State &state)
    {
        XFrontPanelButton buttonReading = XFrontPanelButton::None;
        if (IsAvailable())
        {
            DX::ThrowIfFailed(XFrontPanelGetButtonStates(&buttonReading));
        }
        state.buttons.rawButtons = buttonReading;

        XFrontPanelLight lightReading = XFrontPanelLight::None;
        if (IsAvailable())
        {
            DX::ThrowIfFailed(XFrontPanelGetLightStates(&lightReading));
        }
        state.lights.rawLights = lightReading;

        state.buttons.button1      = (buttonReading & XFrontPanelButton::Button1)    != XFrontPanelButton::None;
        state.buttons.button2      = (buttonReading & XFrontPanelButton::Button2)    != XFrontPanelButton::None;
        state.buttons.button3      = (buttonReading & XFrontPanelButton::Button3)    != XFrontPanelButton::None;
        state.buttons.button4      = (buttonReading & XFrontPanelButton::Button4)    != XFrontPanelButton::None;
        state.buttons.button5      = (buttonReading & XFrontPanelButton::Button5)    != XFrontPanelButton::None;
        state.buttons.dpadLeft     = (buttonReading & XFrontPanelButton::DPadLeft)   != XFrontPanelButton::None;
        state.buttons.dpadRight    = (buttonReading & XFrontPanelButton::DPadRight)  != XFrontPanelButton::None;
        state.buttons.dpadUp       = (buttonReading & XFrontPanelButton::DPadUp)     != XFrontPanelButton::None;
        state.buttons.dpadDown     = (buttonReading & XFrontPanelButton::DPadDown)   != XFrontPanelButton::None;
        state.buttons.buttonSelect = (buttonReading & XFrontPanelButton::DPadSelect) != XFrontPanelButton::None;

        state.lights.light1 = (lightReading & XFrontPanelLight::Light1) != XFrontPanelLight::None;
        state.lights.light2 = (lightReading & XFrontPanelLight::Light2) != XFrontPanelLight::None;
        state.lights.light3 = (lightReading & XFrontPanelLight::Light3) != XFrontPanelLight::None;
        state.lights.light4 = (lightReading & XFrontPanelLight::Light4) != XFrontPanelLight::None;
        state.lights.light5 = (lightReading & XFrontPanelLight::Light5) != XFrontPanelLight::None;
    }

    void SetLightStates(const XFrontPanelLight &lights)
    {
        if (IsAvailable())
        {
            DX::ThrowIfFailed(XFrontPanelSetLightStates(lights));
        }
    }

    bool IsAvailable() const
    {
        return mAvailable;
    }
};

FrontPanelInput::Impl* FrontPanelInput::Impl::s_frontPanelInput = nullptr;
#pragma endregion

// --------------------------------------------------------------------------------
// FrontPanelInput methods
// --------------------------------------------------------------------------------
#pragma region  FrontPanelInput methods
// Public constructor.
FrontPanelInput::FrontPanelInput()
    : pImpl( new Impl(this))
{
}


FrontPanelInput::FrontPanelInput(FrontPanelInput&&) noexcept = default;
FrontPanelInput& FrontPanelInput::operator=(FrontPanelInput&&) noexcept = default;
FrontPanelInput::~FrontPanelInput() = default;


FrontPanelInput::State FrontPanelInput::GetState()
{
    State state;
    pImpl->GetState(state);
    return state;
}

void FrontPanelInput::SetLightStates(const XFrontPanelLight& lights)
{
    pImpl->SetLightStates(lights);
}

FrontPanelInput & FrontPanelInput::Get()
{
    if (!Impl::s_frontPanelInput || !Impl::s_frontPanelInput->mOwner)
        throw std::exception("FrontPanelInput is a singleton");

    return *Impl::s_frontPanelInput->mOwner;
}

bool FrontPanelInput::IsAvailable() const
{
    return pImpl->IsAvailable();
}
#pragma endregion

// --------------------------------------------------------------------------------
// FrontPanelInput::ButtonStateTracker methods
// --------------------------------------------------------------------------------
#pragma region FrontPanelInput::ButtonStateTracker methods

#define UPDATE_BUTTON_STATE(field) field = static_cast<ButtonState>( ( !!state.buttons.field ) | ( ( !!state.buttons.field ^ !!lastState.buttons.field ) << 1 ) );

void FrontPanelInput::ButtonStateTracker::Update(const State& state)
{
    buttonsChanged = (state.buttons.rawButtons ^ lastState.buttons.rawButtons) != XFrontPanelButton::None;

    UPDATE_BUTTON_STATE(button1);

    assert((!state.buttons.button1 && !lastState.buttons.button1) == (button1 == UP));
    assert((state.buttons.button1 && lastState.buttons.button1) == (button1 == HELD));
    assert((!state.buttons.button1 && lastState.buttons.button1) == (button1 == RELEASED));
    assert((state.buttons.button1 && !lastState.buttons.button1) == (button1 == PRESSED));

    UPDATE_BUTTON_STATE(button2);
    UPDATE_BUTTON_STATE(button3);
    UPDATE_BUTTON_STATE(button4);
    UPDATE_BUTTON_STATE(button5);
    UPDATE_BUTTON_STATE(dpadLeft);
    UPDATE_BUTTON_STATE(dpadRight);
    UPDATE_BUTTON_STATE(dpadUp);
    UPDATE_BUTTON_STATE(dpadDown);
    UPDATE_BUTTON_STATE(buttonSelect);

    lastState = state;
}

#undef UPDATE_BUTTON_STATE

void FrontPanelInput::ButtonStateTracker::Reset()
{
    memset(this, 0, sizeof(ButtonStateTracker));
}
#pragma endregion
