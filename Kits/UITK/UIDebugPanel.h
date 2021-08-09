//--------------------------------------------------------------------------------------
// File: UIDebugPanel.h
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

NAMESPACE_ATG_UITK_BEGIN

class UIDebugPanel : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIDebugPanel, DebugPanel);

public:

    UIDebugPanel(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
    }

    struct DebugPanelDataProperties
    {
        const Vector2 c_defaultGridSize = Vector2(5.f, 5.f);

        DebugPanelDataProperties() : m_gridSize(c_defaultGridSize) {}
        Vector2 m_gridSize;
    };

    void Update(float) override {}
    void Render() override;
    void PostRender() override {}
    void HandleStyleIdChanged() override;

private:
    DebugPanelDataProperties m_debugPanelDataProperties;

    std::shared_ptr<class UIBasicStyle> m_basicStyle;

    
};

class UIDebugPanelFactory : public UIElementFactory<UIDebugPanel>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ UIDebugPanel::DebugPanelDataProperties&);

protected:
    /*virtual*/ UIDebugPanel* Create(UIManager& manager, ID id, UIDataPtr data);
};

NAMESPACE_ATG_UITK_END
