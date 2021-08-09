//--------------------------------------------------------------------------------------
// File: UIStackPanel.cpp
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
#include "UIStackPanel.h"
#include "UIKeywords.h"

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_DEBUG(UIStackPanel);

/*virtual*/ bool UIStackPanel::HandleGlobalInputState(const UIInputState& inputState)
{
    auto leftPressed = inputState.AnyDPLIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Left);
    auto rightPressed = inputState.AnyDPRIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Right);
    auto upPressed = inputState.AnyDPUIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Up);
    auto downPressed = inputState.AnyDPDIsState(GamePad::ButtonStateTracker::PRESSED) ||
        inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Down);

    auto prevPressed = IsVertical(m_stackPanelDataProperties.stackingOrientation) ? upPressed : leftPressed;
    auto nextPressed = IsVertical(m_stackPanelDataProperties.stackingOrientation) ? downPressed : rightPressed;


    bool isMouse = false;
    if (IsVertical(m_stackPanelDataProperties.stackingOrientation))
    {
        static int s_scrollWheelValue = 0;
        if (inputState.GetMouseState().scrollWheelValue < s_scrollWheelValue)
        {
            isMouse = true;
            nextPressed = true;
        }
        else if (inputState.GetMouseState().scrollWheelValue > s_scrollWheelValue)
        {
            isMouse = true;
            prevPressed = true;
        }

        s_scrollWheelValue = inputState.GetMouseState().scrollWheelValue;
    }

    return (m_stackPanelDataProperties.maxVisibleItems > 0) ?
        (prevPressed ? ShiftPrevious(isMouse) :
            nextPressed ? ShiftNext(isMouse) :
            false) :
        false;
}


/*virtual*/ bool UIStackPanel::HandleInputEvent(const InputEvent& inputEvent)
{
    if (inputEvent.m_inputEvent != InputEvent::InputStateChange)
    {
        return false;
    }

    return false;
}

void UIStackPanel::Update(float)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStackPanel_Update");
    PIXSetMarker(PIX_COLOR_DEFAULT, L"StackPanel: %hs", GetID().AsCStr());

    Vector2 stackOffsetSign = Vector2::Zero;
    Vector2 stackOffsetPosition = Vector2::Zero;

    Anchor positioningAndSizingAnchor{};

    switch (GetOrientation())
    {
    case StackingOrientation::Down:
        positioningAndSizingAnchor.Horizontal = m_stackPanelDataProperties.horizontalAlignmentAnchor;
        positioningAndSizingAnchor.Vertical = VerticalAnchor::Top;
        stackOffsetSign = DirectX::SimpleMath::Vector2(0.0f, 1.0f);
        break;
    case StackingOrientation::Up:
        positioningAndSizingAnchor.Horizontal = m_stackPanelDataProperties.horizontalAlignmentAnchor;
        positioningAndSizingAnchor.Vertical = VerticalAnchor::Bottom;
        stackOffsetSign = DirectX::SimpleMath::Vector2(0.0f, -1.0f);
        break;
    case StackingOrientation::Left:
        positioningAndSizingAnchor.Vertical = m_stackPanelDataProperties.verticalAlignmentAnchor;
        positioningAndSizingAnchor.Horizontal = HorizontalAnchor::Right;
        stackOffsetSign = DirectX::SimpleMath::Vector2(-1.0f, 0.0f);
        break;
    case StackingOrientation::Right:
        positioningAndSizingAnchor.Vertical = m_stackPanelDataProperties.verticalAlignmentAnchor;
        positioningAndSizingAnchor.Horizontal = HorizontalAnchor::Left;
        stackOffsetSign = DirectX::SimpleMath::Vector2(1.0f, 0.0f);
        break;
    default:
        throw std::exception("Invalid options");
    }

    auto numChildren = uint32_t(m_children.size());
    if (numChildren != m_numChildren && numChildren > 0)
    {
        // numChildren changed, readjust slider if necessary
        if (m_cachedSlider)
        {
            // 0 1 2 3 4 5 6 7 8 9 numChildren 10 maxVisibleItems 4
            // 0 1 2 3 > 6 7 8 9 maxValue = 6
            // range [0-6]
            // 10 - 4 + 1 = 7 steps
            uint32_t maxVisible = m_stackPanelDataProperties.maxVisibleItems;
            uint32_t maxValue = uint32_t(numChildren) - maxVisible;

            m_cachedSlider->ModifySliderRange(0.f, float(maxValue), maxValue + 1);
            m_cachedSlider->SetSliderValue(0.f);

            // resize slider to match number of visible children
            if (IsVertical(m_stackPanelDataProperties.stackingOrientation))
            {
                // assumes all children are same height
                auto childHeight = m_children.at(0)->GetRelativeSizeInRefUnits().y;
                auto newHeight = childHeight * maxVisible + m_stackPanelDataProperties.elementPadding * (maxVisible - 1);
                m_cachedSlider->SetHeight(newHeight);
            }
            else
            {
                // assumes all children are same width
                auto childWidth = m_children.at(0)->GetRelativeSizeInRefUnits().x;
                auto newWidth = childWidth * maxVisible + m_stackPanelDataProperties.elementPadding * (maxVisible - 1);
                m_cachedSlider->SetWidth(newWidth);
            }
        }
        
        m_numChildren = numChildren;
    }

    uint32_t maxVisible = m_stackPanelDataProperties.maxVisibleItems;

    for (uint32_t i = 0; i < numChildren; ++i)
    {
        auto& child = m_children.at(i);

        if(maxVisible > 0)
        {
            child->SetVisible(i >= m_startIndex && i <= GetEndIndex());
        }

        if (!child->IsVisible())
        {
            continue;
        }

        if (!IsStackPanelId(child->GetClassID()))
        {
            child->SetPositioningAnchor(positioningAndSizingAnchor);
            child->SetSizingAnchor(positioningAndSizingAnchor);
        }
        child->SetRelativePositionInRefUnits(stackOffsetPosition);
        auto size = child->GetRelativeSizeInRefUnits();
        auto offset = stackOffsetSign * m_stackPanelDataProperties.elementPadding;

        stackOffsetPosition += (offset + (stackOffsetSign * size));
    }

    if (m_cachedSlider)
    {
        m_cachedSlider->SetVisible(numChildren > maxVisible);
    }

    if (m_updateFocus != c_noUpdate)
    {
        m_uiManager.SetFocus(m_children.at(m_updateFocus));
        m_updateFocus = c_noUpdate;
    }

    InvalidateRectangleCaches();
}

void UIStackPanel::Reset()
{
    m_startIndex = 0;

    for (auto& child : m_children)
    {
        child->SetVisible(false);
    }

    m_children.clear();
    m_numChildren = 0;
}

void UIStackPanel::WireUpElements()
{
    if (!m_cachedSlider)
    {
        m_cachedSlider = GetTypedSubElementById<UISlider>(m_stackPanelDataProperties.sliderSubElementId);
    }

    if (m_cachedSlider)
    {
        m_cachedSlider->CurrentValueState().AddListener([this](UISlider*)
        {
            m_startIndex = uint32_t(std::round(m_cachedSlider->GetCurrentValue()));
        });
    }
}

bool UIStackPanel::IsFocusedAtBeginning()
{
    return m_children.at(m_startIndex)->IsFocused();
}

bool UIStackPanel::IsFocusedAtEnd()
{
    auto& child = m_children;
    auto maxVisible = m_stackPanelDataProperties.maxVisibleItems;

    if (maxVisible > 0 && child.size() > maxVisible)
    {
        return child.at(GetEndIndex())->IsFocused();
    }
    else
    {
        return child.at(child.size() - 1)->IsFocused();
    }
}

bool UIStackPanel::HasMorePreviousElements()
{
    return m_startIndex > 0;
}

bool UIStackPanel::HasMoreNextElements()
{
    // example: size 10 maxVisibleItems 4
    // 0 1 2 3  start 0 + 4 = 4 < 10 true
    // 2 3 4 5  start 2 + 4 = 6 < 10 true
    // 5 6 7 8  start 5 + 4 = 9 < 10 true
    // 6 7 8 9  start 6 + 4 = 10 < 10 false
    return (GetEndIndex() + 1) < m_children.size();
}

bool UIStackPanel::ShiftPrevious(bool isMouse)
{
    auto focusedAtBeginning = IsFocusedAtBeginning();
    auto focusedAtEnd = IsFocusedAtEnd();
    if ((isMouse || focusedAtBeginning) && HasMorePreviousElements())
    {
        m_startIndex -= 1;
        auto endIndex = GetEndIndex();

        UILOG_DEBUG("ShiftPrevious [%d-%d] (%d)", m_startIndex, endIndex, m_children.size());
        if (focusedAtBeginning)
        {
            m_updateFocus = m_startIndex;
        }
        else if (focusedAtEnd)
        {
            m_updateFocus = endIndex;
        }

        if (m_cachedSlider && m_cachedSlider->IsVisible())
        {
            m_cachedSlider->SetSliderValue(float(m_startIndex));
        }

        return true;
    }

    return false;
}

bool UIStackPanel::ShiftNext(bool isMouse)
{
    auto focusedAtBeginning = IsFocusedAtBeginning();
    auto focusedAtEnd = IsFocusedAtEnd();
    if ((isMouse || focusedAtEnd) && HasMoreNextElements())
    {
        m_startIndex += 1;
        auto endIndex = GetEndIndex();
        UILOG_DEBUG("ShiftNext [%d-%d] (%d)", m_startIndex, endIndex, m_children.size());
        if (focusedAtBeginning)
        {
            m_updateFocus = m_startIndex;
        }
        else if (focusedAtEnd)
        {
            m_updateFocus = endIndex;
        }

        if (m_cachedSlider && m_cachedSlider->IsVisible())
        {
            m_cachedSlider->SetSliderValue(float(m_startIndex));
        }

        return true;
    }

    return false;
}

ENUM_LOOKUP_TABLE(StackingOrientation,
    ID_ENUM_PAIR(UITK_VALUE(down), StackingOrientation::Down),
    ID_ENUM_PAIR(UITK_VALUE(up), StackingOrientation::Up),
    ID_ENUM_PAIR(UITK_VALUE(left), StackingOrientation::Left),
    ID_ENUM_PAIR(UITK_VALUE(right), StackingOrientation::Right)
)

/*static*/ void UIStackPanelFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ StackPanelDataProperties& stackDataProperties)
{
    ID classId;
    data->GetTo(UITK_FIELD(classId), classId);

    // the stacking direction of the stack
    ID stackingOrientationID;
    if (data->GetTo(UITK_FIELD(stackingOrientation), stackingOrientationID))
    {
        UIEnumLookup(stackingOrientationID, stackDataProperties.stackingOrientation);
    }
    else
    {
        // provide defaults for the various stacking classes
        if (classId == UIStackPanel::VerticalStackPanelClassID())
        {
            stackDataProperties.stackingOrientation = StackPanelDataProperties::c_defaultVerticalStackingOrientation;
        }
        else
        {
            stackDataProperties.stackingOrientation = StackPanelDataProperties::c_defaultHorizontalStackingOrientation;
        }
    }

    // Get the class id to determine the alignment strategy
    if (classId == UIStackPanel::VerticalStackPanelClassID())
    {
        assert(UIStackPanel::IsVertical(stackDataProperties.stackingOrientation));
        data->GetTo(UITK_FIELD(stackElementAlignment), stackDataProperties.verticalAlignmentAnchor);
    }
    else
    {
        assert(UIStackPanel::IsHorizontal(stackDataProperties.stackingOrientation));
        data->GetTo(UITK_FIELD(stackElementAlignment), stackDataProperties.horizontalAlignmentAnchor);
    }

    // the spacing between stacked elements
    data->GetTo(UITK_FIELD(stackElementPadding), stackDataProperties.elementPadding);

    // number of visible elements
    data->GetTo(UITK_FIELD(maxVisibleItems), stackDataProperties.maxVisibleItems);

    // optional slider that shows when visible < num elements
    stackDataProperties.sliderSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(sliderSubElementId), ID::Default);
}

NAMESPACE_ATG_UITK_END
