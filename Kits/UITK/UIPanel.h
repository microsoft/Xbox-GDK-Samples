//--------------------------------------------------------------------------------------
// File: UIPanel.h
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
#include "UIStyle.h"
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

//! @anchor Panel
//! @class PanelDataProperties UIPanel.h "UITK/UIPanel.h"
//! @brief Panel UI element instance properties specific to panels.
struct PanelDataProperties
{
    PanelDataProperties() :
        m_clipChildren(c_defaultClipChildren)
    {
    }

    //! @subpage clipChildren "clipChildren JSON property"
    bool m_clipChildren;

    //! @privatesection
    /// defaults for properties if not specified in data
    static constexpr bool       c_defaultClipChildren = false;
};

//! @private
/// UIPanels are intended to be simple containers of child elements within
/// them.  They may (or may not) clip their children to their bounding
/// screen rectangle, and may either have a full background, or simply have
/// a border.  UIPanels will be show-able & enable-able like all UIElements,
/// but also should be highlight-able & focus-able.
class UIPanel : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIPanel, Panel)

public:
    virtual ~UIPanel() = default;

protected:
    PanelDataProperties m_panelDataProperties;
    size_t m_scissorStackIndex;

protected:
    UIPanel(UIManager& uiManager, ID id) :
        UIElement(uiManager, id),
        m_panelDataProperties(),
        m_scissorStackIndex(0),
        m_spriteStyle(nullptr)
    {}

    void Render() override;
    void PostRender() override;
    void HandleStyleIdChanged() override;

    std::shared_ptr<class UISpriteStyle> m_spriteStyle;
};

//! @private
/// A factory for creating UIPanels from data to be managed by
/// the provided UIManager.
class UIPanelFactory : public UIElementFactory<UIPanel>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ PanelDataProperties&);

protected:
    /*virtual*/ UIPanel* Create(UIManager& manager, ID id, UIDataPtr data);
};

NAMESPACE_ATG_UITK_END
