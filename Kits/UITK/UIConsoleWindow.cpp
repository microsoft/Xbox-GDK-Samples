//--------------------------------------------------------------------------------------
// File: UIConsoleWindow.cpp
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

#include "UIConsoleWindow.h"
#include "UIPanel.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*public:*/

void UIConsoleWindow::AppendLineOfText(const std::string& lineOfText)
{
    if (m_linesOfText.size() < m_consoleWindowDataProperties.maxConsoleLines)
    {
        if (m_cachedVerticalSlider)
        {
            m_cachedVerticalSlider->ModifySliderRange(-float(m_linesOfText.size()), 0.0f, uint32_t(m_linesOfText.size()));
            m_cachedVerticalSlider->SetVisible(true);
        }
    }

    // add the line to the set to render (applying the set TextStyle) and
    // also adjust the associated slider's range & thumb positioning.

    m_linesOfText.insert(m_linesOfText.begin(), lineOfText);
}

void UIConsoleWindow::ClearAllLines()
{
    // remove all of the lines of text and reset the associated slider.

    m_linesOfText.clear();
    m_startingDisplayOffset = 0;
    if (m_cachedVerticalSlider)
    {
        m_cachedVerticalSlider->ModifySliderRange(-1.0f, 0.0f, 1);
        m_cachedVerticalSlider->SetVisible(false);
    }
}

/*protected:*/

/*virtual*/ bool UIConsoleWindow::HandleInputEvent(const InputEvent& /*inputEvent*/) 
{
    // We have no need to handle input events on our own.  
    // We rely on sub-element input handling instead.
    return false;
}

/*virtual*/ void UIConsoleWindow::Update(float /*elapsedTimeInS*/) 
{
    // no frame-by-frame logic needed.
}

/*virtual*/ void UIConsoleWindow::PostRender()
{
    // no post render logic needed
}

/*virtual*/ void UIConsoleWindow::Render() 
{
    // render our lines of text (which vertically fit within the panel's
    // padded rectangle) with the first line being based on the slider's
    // current value.

    if (!m_cachedViewportPanel)
    {
        return;
    }

    UIStyleManager& styleManager = m_uiManager.GetStyleManager();
    auto textStyle = styleManager.GetTypedById<UITextStyle>(m_elementDataProperties.styleId);

    auto viewportRect = m_cachedViewportPanel->GetScreenRectInPixels();

    auto scissorStackIndex = styleManager.GetStyleRenderer().IntersectScissorRectangle(viewportRect);
    RECT viewportPaddedRect = m_cachedViewportPanel->GetPaddedRectInPixels();

    auto lineHeight = textStyle->GetScaledLineHeight();

    for (size_t consoleLineIndex = m_startingDisplayOffset; consoleLineIndex < m_linesOfText.size(); ++consoleLineIndex)
    {
        const auto& consoleLine = m_linesOfText[consoleLineIndex];

        auto renderedLine = consoleLineIndex - m_startingDisplayOffset;

        RECT consoleLineRect{
            viewportPaddedRect.left,
            viewportPaddedRect.bottom - (LONG(renderedLine + 1) * LONG(lineHeight)),
            viewportPaddedRect.right,
            viewportPaddedRect.bottom - (LONG(renderedLine) * LONG(lineHeight))
        };

        if (consoleLineRect.bottom <= viewportPaddedRect.top)
        {
            break;
        }

        // do this for all strings that are within the panel's padded rectangle
        // going upward from the bottom
        textStyle->RenderText(
            Rectangle(consoleLineRect),
            consoleLine,
            HorizontalTextWrapping::Overflow,
            VerticalTextTruncation::Overflow);
    }

    styleManager.GetStyleRenderer().PopScissorRectangle(scissorStackIndex);
}

void UIConsoleWindow::WireUpElements()
{
    if (!m_cachedVerticalSlider)
    {
        m_cachedVerticalSlider = GetTypedSubElementById<UISlider>(
            m_consoleWindowDataProperties.verticalSliderSubElementId);
        m_cachedVerticalSlider->CurrentValueState().AddListener([this](UISlider*) {
            // adjust the starting offset of the displayed console window strings?
            m_startingDisplayOffset = uint32_t(-int(m_cachedVerticalSlider->CurrentValueState().Get()));
        });

        // initialize the slider to have no console lines reflected
        // in its range.
        m_cachedVerticalSlider->ModifySliderRange(-1.0f, 0.0f, 1);
        m_cachedVerticalSlider->SetVisible(false);
    }

    if (!m_cachedViewportPanel)
    {
        m_cachedViewportPanel = GetTypedSubElementById<UIPanel>(
            m_consoleWindowDataProperties.viewportPanelSubElementId);

        if (m_cachedVerticalSlider)
        {
            // set height of slider to match viewport
            m_cachedVerticalSlider->SetHeight(m_cachedViewportPanel->GetSizeInRefUnits().y);

            // move to right edge of viewport
            auto sliderWidth = m_cachedVerticalSlider->GetSizeInRefUnits().x;
            m_cachedVerticalSlider->SetRelativePositionInRefUnits(
                Vector2(m_cachedViewportPanel->GetSizeInRefUnits().x - sliderWidth,
                    m_cachedViewportPanel->GetRelativePositionInRefUnits().y));
        }
    }
}

/*protected:*/

/*static*/ void UIConsoleWindowFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ ConsoleWindowDataProperties& properties)
{
    data->GetTo<uint32_t>(UITK_FIELD(maxConsoleLines), properties.maxConsoleLines);

    properties.verticalSliderSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(verticalSliderSubElementId), ID::Default);
    properties.viewportPanelSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(viewportPanelSubElementId), ID::Default);
}

NAMESPACE_ATG_UITK_END
