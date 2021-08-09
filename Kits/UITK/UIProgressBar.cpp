//--------------------------------------------------------------------------------------
// File: UIProgressBar.cpp
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
#include "UIProgressBar.h"
#include "UIMath.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*public:*/

/// note: the provided percentage will be clamped to be between
/// 0.0 and 1.0 (0% to 100%)

void UIProgressBar::SetProgressPercentage(float percentage)
{
    m_progressBarDataProperties.m_progressPercentage = UIMath::Clamp(percentage, 0.0f, 1.0f);
}

/*protected:*/

/*virtual*/ void UIProgressBar::Render()
{
    UIStyleManager& styleManager = m_uiManager.GetStyleManager();
    auto spriteStyle = styleManager.GetTypedById<UISpriteStyle>(m_elementDataProperties.styleId);

    auto parentRectangle = m_parent->GetPaddedRectInPixels();
    auto fullRectangle = spriteStyle->SubtractMarginFromRect(parentRectangle);
    auto progressRectangle = Rectangle(
        fullRectangle.x, fullRectangle.y,
        static_cast<long>(static_cast<float>(fullRectangle.width)* m_progressBarDataProperties.m_progressPercentage),
        fullRectangle.height);

    spriteStyle->RenderCroppedSimpleSprite(
        progressRectangle,
        Vector2(m_progressBarDataProperties.m_progressPercentage, 1.0f));
}

/*protected:*/

/*static*/ void UIProgressBarFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ ProgressBarDataProperties& progressBarDataProperties)
{
    progressBarDataProperties.m_progressPercentage =
        data->GetIfExists<float>(UITK_FIELD(progressPercentage), ProgressBarDataProperties::c_defaultProgressPercentage);
}

NAMESPACE_ATG_UITK_END
