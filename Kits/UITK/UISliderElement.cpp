//--------------------------------------------------------------------------------------
// File: UISliderElement.cpp
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"

#include "UISliderElement.h"
#include "UIKeywords.h"

NAMESPACE_ATG_UITK_BEGIN

/*protected:*/

UIDisplayString UISlider::GetCurrentValueAsText() const
{
    return std::to_string(GetCurrentValue());
}

/*protected:*/

/*virtual*/ bool UISlider::HandleGlobalInputState(const UIInputState& inputState)
{
    auto leftPressed = inputState.AnyDPLIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Left);
    auto rightPressed = inputState.AnyDPRIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Right);
    auto upPressed = inputState.AnyDPUIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Up);
    auto downPressed = inputState.AnyDPDIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Down);

    float currentValue = std::round(GetCurrentValue());

    if ((m_sliderDataProperties.sliderOrientation == SliderOrientation::TopToBottom && upPressed) ||
        (m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight && leftPressed))
    {
        currentValue -= 1.f;
        SetSliderValue(currentValue);
        return true;
    }

    if ((m_sliderDataProperties.sliderOrientation == SliderOrientation::TopToBottom && downPressed) ||
        (m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight && rightPressed))
    {
        currentValue += 1.f;
        SetSliderValue(currentValue);
        return true;
    }

    return false;
}

/*virtual*/ bool UISlider::HandleInputEvent(const InputEvent& inputEvent)
{
    if (inputEvent.m_inputEvent != InputEvent::InputStateChange)
    {
        return false;
    }

    if (m_mouseCaptured && inputEvent.m_inputState.GetMouseButtons().leftButton !=
        Mouse::ButtonStateTracker::ButtonState::HELD)
    {
        m_mouseCaptured = false;
    }

    int dx = 0;
    int dy = 0;
    int stickScrollSpeed = 5;

    if (m_cachedThumbButton)
    {
        auto isHorizontal = m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight;
        auto isVertical = !isHorizontal;

        // TODO: listen to the joystick on the gamepad
        if (isVertical)
        {
            if (inputEvent.m_inputState.AnyLTStickDown())
            {
                dy += stickScrollSpeed;
            }
            else if (inputEvent.m_inputState.AnyLTStickUp())
            {
                dy -= stickScrollSpeed;
            }
        }
        else
        {
            if (inputEvent.m_inputState.AnyLTStickLeft())
            {
                dx -= stickScrollSpeed;
            }
            else if (inputEvent.m_inputState.AnyLTStickRright())
            {
                dx += stickScrollSpeed;
            }
        }
        
        // TODO: listen to the +/- keys on the keyboard
        
        // detect mouse movement and adjust the thumb button
        // and the current value state accordingly
        if (m_cachedThumbButton->ButtonState().Get() == UIButton::State::Down || m_mouseCaptured)
        {
            m_mouseCaptured = true;

            if (isHorizontal)
            {
                // support horizontal sliders
                dx = inputEvent.m_inputState.GetMouseState().x -
                    inputEvent.m_inputState.GetPrevMouseState().x;
            }
            else
            {
                // support vertical sliders
                dy = inputEvent.m_inputState.GetMouseState().y -
                    inputEvent.m_inputState.GetPrevMouseState().y;
            }
        }
    }

    if (dx != 0)
    {
        MoveThumb(float(dx), 0.0f);
    }

    if (dy != 0)
    {
        MoveThumb(0.0f, float(dy));
    }

    if (dx != 0 || dy != 0)
    {
        ClampThumb();
        m_currentValueState.Set(CalculateThumbValue(), this);
    }

    return (dx != 0 || dy != 0);
}

/*virtual*/ void UISlider::Update(float /*elapsedTimeInS*/)
{
    // we do not need to update anything specific to the slider for now...
}

/*virtual*/ void UISlider::Render()
{
    // we do not need to render anything specific to the slider for now...
}

void UISlider::WireUpElements()
{
    m_currentValueState.ClearTo(m_sliderDataProperties.initialValue);

    if (!m_cachedBackground)
    {
        m_cachedBackground = GetSubElementById(
            m_sliderDataProperties.backgroundSubElementId);
    }

    if (!m_cachedThumbButton)
    {
        m_cachedThumbButton = GetTypedSubElementById<UIButton>(
            m_sliderDataProperties.thumbButtonSubElementId);
        MoveThumbToValue(m_currentValueState.Get());
    }
}

void UISlider::MoveThumbToValue(float value)
{
    // position the thumb according to the initial value
    // coming from data within our rectangle.

    if (m_cachedThumbButton)
    {
        auto parentPaddedRect = m_cachedThumbButton->GetParent()->GetPaddedRectInRefUnits();
        auto thumbMarginedRect = m_cachedThumbButton->GetMarginedRectInRefUnits();

        if (m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight)
        {
            auto movementSpan = parentPaddedRect.width - thumbMarginedRect.width;
            auto t = m_sliderDataProperties.CalculateInterpolant(value);
            auto x = t * float(movementSpan);
            auto y = m_cachedThumbButton->GetRelativePositionInRefUnits().y;
            m_cachedThumbButton->SetRelativePositionInRefUnits(Vector2(x, y));
        }
        else
        {
            // support vertically oriented sliders too!
            auto movementSpan = parentPaddedRect.height - thumbMarginedRect.height;
            auto t = m_sliderDataProperties.CalculateInterpolant(value);
            auto y = t * float(movementSpan);
            auto x = m_cachedThumbButton->GetRelativePositionInRefUnits().x;
            m_cachedThumbButton->SetRelativePositionInRefUnits(Vector2(x, y));
        }
    }
}

void UISlider::MoveThumb(float dxInPixels, float dyInPixels)
{
    if (m_cachedThumbButton)
    {
        auto scaledDX = float(dxInPixels) * m_uiManager.GetPixelsToRefUnitsScale();
        auto scaledDY = float(dyInPixels) * m_uiManager.GetPixelsToRefUnitsScale();
        auto thumbPosition = m_cachedThumbButton->GetRelativePositionInRefUnits();
        auto newThumbPosition = Vector2(thumbPosition.x + scaledDX, thumbPosition.y + scaledDY);
        m_cachedThumbButton->SetRelativePositionInRefUnits(newThumbPosition);
    }
}

void UISlider::ClampThumb()
{
    // make sure the thumb + margin fits within its parent - padding
    if (m_cachedThumbButton)
    {
        auto parentPaddedRect = m_cachedThumbButton->GetParent()->GetPaddedRectInPixels();
        auto thumbMarginedRect = m_cachedThumbButton->GetMarginedRectInPixels();

        if (m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight)
        {
            int dx = 0;
            if (thumbMarginedRect.x < parentPaddedRect.x)
            {
                dx = parentPaddedRect.x - thumbMarginedRect.x;
            }
            else if (thumbMarginedRect.x + thumbMarginedRect.width > parentPaddedRect.x + parentPaddedRect.width)
            {
                dx = parentPaddedRect.x + parentPaddedRect.width - thumbMarginedRect.x - thumbMarginedRect.width;
            }
            MoveThumb(float(dx), 0.0f);
        }
        else
        {
            // support vertical orientation sliders too!
            int dy = 0;
            if (thumbMarginedRect.y < parentPaddedRect.y)
            {
                dy = parentPaddedRect.y - thumbMarginedRect.y;
            }
            else if (thumbMarginedRect.y + thumbMarginedRect.height > parentPaddedRect.y + parentPaddedRect.height)
            {
                dy = parentPaddedRect.y + parentPaddedRect.height - thumbMarginedRect.y - thumbMarginedRect.height;
            }
            MoveThumb(0.0f, float(dy));
        }
    }
}

float UISlider::CalculateThumbValue()
{
    // calculate the "t" interpolation value based on the thumb
    // position within it movable position
    // then, interpolate the actual value within the range
    if (m_cachedThumbButton)
    {
        auto parentPaddedRect = m_cachedThumbButton->GetParent()->GetPaddedRectInRefUnits();
        auto thumbMarginedRect = m_cachedThumbButton->GetMarginedRectInRefUnits();

        if (m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight)
        {
            auto movementSpan = parentPaddedRect.width - thumbMarginedRect.width;
            auto movementOffset = thumbMarginedRect.x - parentPaddedRect.x;
            auto newValue = m_sliderDataProperties.CalculateValue(movementOffset / float(movementSpan));
            return newValue;
        }
        else // vertically oriented sliders
        {
            auto movementSpan = parentPaddedRect.height - thumbMarginedRect.height;
            auto movementOffset = thumbMarginedRect.y - parentPaddedRect.y;
            auto newValue = m_sliderDataProperties.CalculateValue(movementOffset / float(movementSpan));
            return newValue;
        }
    }

    return m_sliderDataProperties.initialValue;
}

void UISlider::SetWidth(float width)
{
    assert(m_sliderDataProperties.sliderOrientation == SliderOrientation::LeftToRight);

    SetRelativeSizeInRefUnits(Vector2(width, GetRelativeSizeInRefUnits().y));

    if (m_sliderDataProperties.backgroundSubElementId)
    {
        auto backgroundElement = GetSubElementById(m_sliderDataProperties.backgroundSubElementId);
        backgroundElement->SetRelativeSizeInRefUnits(Vector2(width, backgroundElement->GetRelativeSizeInRefUnits().y));
    }
}

void UISlider::SetHeight(float height)
{
    assert(m_sliderDataProperties.sliderOrientation == SliderOrientation::TopToBottom);

    SetRelativeSizeInRefUnits(Vector2(GetRelativeSizeInRefUnits().x, height));

    if (m_sliderDataProperties.backgroundSubElementId)
    {
        auto backgroundElement = GetSubElementById(m_sliderDataProperties.backgroundSubElementId);
        backgroundElement->SetRelativeSizeInRefUnits(Vector2(backgroundElement->GetRelativeSizeInRefUnits().x, height));
    }
}

ENUM_LOOKUP_TABLE(SliderOrientation,
    ID_ENUM_PAIR(UITK_VALUE(leftToRight), SliderOrientation::LeftToRight),
    ID_ENUM_PAIR(UITK_VALUE(topToBottom), SliderOrientation::TopToBottom)
)

ENUM_LOOKUP_TABLE(SliderType,
    ID_ENUM_PAIR(UITK_VALUE(continuous), SliderType::Continuous),
    ID_ENUM_PAIR(UITK_VALUE(discrete), SliderType::Discrete)
)

/*static*/ void UISliderFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ SliderDataProperties& sliderDataProperties)
{
    ID sliderOrientationID;
    if (data->GetTo(UITK_FIELD(sliderOrientation), sliderOrientationID))
    {
        UIEnumLookup(sliderOrientationID, sliderDataProperties.sliderOrientation);
    }

    sliderDataProperties.thumbButtonSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(thumbButtonSubElementId), ID::Default);
    sliderDataProperties.backgroundSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(backgroundSubElementId), ID::Default);

    data->GetTo(UITK_FIELD(valueRange), sliderDataProperties.valueRange);
    data->GetTo(UITK_FIELD(initialValue), sliderDataProperties.initialValue);
    data->GetTo(UITK_FIELD(numDiscreteSteps), sliderDataProperties.numDiscreteSteps);

    ID sliderTypeID;
    if (data->GetTo(UITK_FIELD(sliderType), sliderTypeID))
    {
        UIEnumLookup(sliderTypeID, sliderDataProperties.sliderType);
    }
}

NAMESPACE_ATG_UITK_END
