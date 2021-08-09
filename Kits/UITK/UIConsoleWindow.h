//--------------------------------------------------------------------------------------
// File: UIConsoleWindow.h
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
#include "UISliderElement.h"

NAMESPACE_ATG_UITK_BEGIN

//! @anchor ConsoleWindow
//! @class ConsoleWindowDataProperties UIConsoleWindow.h "UITK/UIConsoleWindow.h"
//! @brief Console window UI element instance properties specific to text output console windows.
struct ConsoleWindowDataProperties
{
    ConsoleWindowDataProperties() : maxConsoleLines(c_defaultMaxConsoleLines)
    {

    }

    // add your data properties and defaults here...

    // the maximum number of console lines that we will display
    //! @subpage maxConsoleLines "maxConsoleLines JSON property"
    uint32_t maxConsoleLines;

    // the ID of the panel sub element used to display the lines of console text
    //! @subpage viewportPanelSubElementId "viewportPanelSubElementId JSON property"
    ID viewportPanelSubElementId;

    // the ID of the slider sub element used to vertically scroll the console
    //! @subpage verticalSliderSubElementId "verticalSliderSubElementId JSON property"
    ID verticalSliderSubElementId;

    //! @private
    /// defaults for properties if not specified in data

    static constexpr uint32_t c_defaultMaxConsoleLines = uint32_t(50);
};

//! @private
/// UIConsoleWindows utilize 1 slider in conunction with begin parented to
/// 1 other containing panel sub element (which provideds the clipping viewport)
/// to accomplish allowing the user to scroll through a list of buffered
/// lines of text up to some maximum amount.  The panel sub element needs to be
/// an ancestor as this element's rectangle is implicitly the panel's padded rect.
/// UIConsoleWindow will be show-able & enable-able like all UIElements, but also
/// can support highlighted-ness and focused-ness for its sub elements.
class UIConsoleWindow : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIConsoleWindow, ConsoleWindow)

public:
    virtual ~UIConsoleWindow() = default;

    // add your type-specific public APIs here...

    void AppendLineOfText(const std::string& lineOfText);
    void ClearAllLines();

protected:
    ConsoleWindowDataProperties m_consoleWindowDataProperties;

    // add your internal member variables here...

    std::shared_ptr<class UISlider>   m_cachedVerticalSlider;
    std::shared_ptr<class UIPanel>    m_cachedViewportPanel;

    uint32_t                    m_startingDisplayOffset;
    std::vector<std::string>    m_linesOfText;

protected:
    UIConsoleWindow(UIManager& uiManager, ID id) : UIElement(uiManager, id), m_startingDisplayOffset(0)
    {
        // add your construction logic here...
    }

    void PostLoad() override
    {
        WireUpElements();
    }

    bool HandleInputEvent(const InputEvent& inputEvent) override;
    void Update(float elapsedTimeInS) override;
    void Render() override;
    void PostRender() override;

    void WireUpElements();
};

//! @private
/// A factory for creating UIConsoleWindows from data to be managed by
/// the provided UIManager.
class UIConsoleWindowFactory : public UIElementFactory<UIConsoleWindow>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ ConsoleWindowDataProperties&);

protected:
    UIConsoleWindow* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newConsoleWindow = UIElementFactory<UIConsoleWindow>::Create(manager, id, data);
        UIConsoleWindowFactory::DeserializeDataProperties(
            data,
            newConsoleWindow->m_consoleWindowDataProperties);
        return newConsoleWindow;
    }
};

NAMESPACE_ATG_UITK_END
