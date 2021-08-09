//--------------------------------------------------------------------------------------
// File: UISliderElement.h
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

#pragma once

#include "UIManager.h"
#include "UIElement.h"
#include "UIButton.h"

NAMESPACE_ATG_UITK_BEGIN

enum class SliderOrientation
{
    LeftToRight = 0,
    TopToBottom = 1,
};

enum class SliderType
{
    Continuous = 0,
    Discrete = 1
};

//! @anchor Slider
//! @class SliderDataProperties UISliderElement.h "UITK/UISliderElement.h"
//! @brief Slider UI element instance properties specific to value sliders.
struct SliderDataProperties
{
    SliderDataProperties() :
        sliderOrientation(c_defaultSliderOrientation),
        valueRange{ c_defaultMinValue, c_defaultMaxValue },
        initialValue(c_defaultInitialValue),
        sliderType(c_defaultSliderType),
        numDiscreteSteps(c_defaultNumSteps)
    {
    }

    //! @private
    float CalculateValue(float t) const
    {
        // support discrete values
        if (sliderType == SliderType::Discrete)
        {
            auto floatDiscreteSteps = float(numDiscreteSteps);
            t = float(int(0.5f + (t * floatDiscreteSteps))) / floatDiscreteSteps;
        }
        return valueRange.x + (t * (valueRange.y - valueRange.x));
    }

    //! @private
    float CalculateInterpolant(float value) const
    {
        if (value <= valueRange.x)
        {
            return 0.0f;
        }
        else if (value >= valueRange.y)
        {
            return 1.0f;
        }
        else
        {
            return (value - valueRange.x) / (valueRange.y - valueRange.x);
        }
    }

    // add your data properties and defaults here...

    //! @subpage sliderOrientation "sliderOrientation JSON property"
    SliderOrientation sliderOrientation;
    //! @subpage thumbButtonSubElementId "thumbButtonSubElementId JSON property"
    ID thumbButtonSubElementId;
    //! @subpage backgroundSubElementId "backgroundSubElementId JSON property"
    ID backgroundSubElementId;
    //! @subpage valueRange "valueRange JSON property"
    Vector2 valueRange;         // ordered as x=min, y=max
    //! @subpage initialValue "initialValue JSON property"
    float initialValue;         // clamped to be within the range
    //! @subpage sliderType "sliderType JSON property"
    SliderType sliderType;
    //! @subpage numDiscreteSteps "numDiscreteSteps JSON property"
    uint32_t numDiscreteSteps;  // 0 if continuous, else >= 1 if discrete

    //! @privatesection
    /// defaults for properties if not specified in data

    static constexpr auto c_defaultSliderOrientation = SliderOrientation::LeftToRight;
    static constexpr auto c_defaultMinValue = 0.0f;
    static constexpr auto c_defaultMaxValue = 1.0f;
    static constexpr auto c_defaultInitialValue = 0.5f;
    static constexpr auto c_defaultSliderType = SliderType::Continuous;
    static constexpr auto c_defaultNumSteps = uint32_t(0);
};

//! @private
/// UISliders utilize 1 button backgrounded with 1 other sub element to
/// accomplish allowing the user to change a value from within a range
/// of acceptable values.  The button sub element needs to be a descendant.
/// UISliders will be show-able & enable-able like all UIElements, but also
/// can support highlighted-ness and focused-ness for its sub elements.
class UISlider : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UISlider, Slider)

public:
    virtual ~UISlider() = default;

    // add your type-specific public APIs here...

    float GetCurrentValue() const
    {
        return m_currentValueState.Get();
    }

    UIDisplayString GetCurrentValueAsText() const;

    UIStateEvent<UISlider, float>& CurrentValueState()
    {
        return m_currentValueState;
    }

    const UIStateEvent<UISlider, float>& CurrentValueState() const
    {
        return m_currentValueState;
    }

    void ModifySliderRange(float minValue, float maxValue, uint32_t numDiscreteSteps)
    {
        m_sliderDataProperties.valueRange = Vector2(minValue, maxValue);
        m_sliderDataProperties.numDiscreteSteps = numDiscreteSteps;

        if (CurrentValueState().Get() < minValue)
        {
            MoveThumbToValue(minValue);
            CurrentValueState().Set(minValue, this);
        }
        if (CurrentValueState().Get() > maxValue)
        {
            MoveThumbToValue(maxValue);
            CurrentValueState().Set(maxValue, this);
        }
    }

    void SetSliderValue(float value)
    {
        float minValue = m_sliderDataProperties.valueRange.x;
        float maxValue = m_sliderDataProperties.valueRange.y;

        if (value < minValue)
        {
            CurrentValueState().Set(minValue, this);
        }
        else if (value > maxValue)
        {
            CurrentValueState().Set(maxValue, this);
        }
        else
        {
            CurrentValueState().Set(value, this);
        }

        MoveThumbToValue(value);
    }

    void SetWidth(float width);
    void SetHeight(float height);

protected:
    SliderDataProperties m_sliderDataProperties;

    // add your internal member variables here...

    bool m_mouseCaptured;

    std::shared_ptr<UIButton>   m_cachedThumbButton;
    UIElementPtr                m_cachedBackground;

    UIStateEvent<UISlider, float> m_currentValueState;

protected:
    UISlider(UIManager & uiManager, ID id) : UIElement(uiManager, id),
        m_mouseCaptured(false)
    {
        // add your construction logic here...
        CurrentValueState().ClearTo(m_sliderDataProperties.initialValue);
    }

    void PostLoad() override
    {
        WireUpElements();
    }

    bool HandleInputEvent(const InputEvent & /*inputEvent*/) override;
    bool HandleGlobalInputState(const UIInputState& /*inputState*/) override;

    void Update(float elapsedTimeInS) override;
    void Render() override;

    void WireUpElements();

    void MoveThumbToValue(float value);
    void MoveThumb(float dx, float dy);
    void ClampThumb();

    float CalculateThumbValue();
};

//! @private
/// A factory for creating UISliders from data to be managed by
/// the provided UIManager.
class UISliderFactory : public UIElementFactory<UISlider>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ SliderDataProperties&);

protected:
    UISlider* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newSlider = UIElementFactory<UISlider>::Create(manager, id, data);
        UISliderFactory::DeserializeDataProperties(
            data,
            newSlider->m_sliderDataProperties);
        return newSlider;
    }

};

NAMESPACE_ATG_UITK_END
