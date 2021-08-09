//--------------------------------------------------------------------------------------
// File: UICheckBox.cpp
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
#include "UICheckBox.h"
#include "UIKeywords.h"
#include "UIStyleImpl.h"
#include "UIStyleManager.h"

NAMESPACE_ATG_UITK_BEGIN

bool UICheckBox::HandleInputEvent(const InputEvent&)
{
    return false;
}

void UICheckBox::Render()
{
    UIStyleManager& styleManager = m_uiManager.GetStyleManager();
    auto screenRect = GetScreenRectInPixels();

    auto spriteStyle = styleManager.GetTypedById<UISpriteStyle>(m_elementDataProperties.styleId);
    spriteStyle->RenderSprite(screenRect);

    if (m_checkboxState.Get() == State::Checked)
    {
        spriteStyle = styleManager.GetTypedById<UISpriteStyle>(m_checkboxDataProperties.checkedStyleId);
        spriteStyle->RenderSprite(screenRect);
    }
}

void UICheckBoxFactory::DeserializeDataProperties(_In_ UIDataPtr data, _Out_ UICheckBox::CheckBoxDataProperties& props)
{
    data->GetTo(UITK_FIELD(checked), props.checked);
}

NAMESPACE_ATG_UITK_END
