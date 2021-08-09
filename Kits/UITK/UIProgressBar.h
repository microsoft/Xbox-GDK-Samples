//--------------------------------------------------------------------------------------
// File: UIProgressBar.h
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

//! @anchor ProgressBar
//! @class ProgressBarDataProperties UIProgressBar.h "UITK/UIProgressBar.h"
//! @brief Progress bar UI element instance properties specific to progress bars used
//! for showing visual progress.
struct ProgressBarDataProperties
{
    ProgressBarDataProperties() :
        m_progressPercentage(c_defaultProgressPercentage)
    {
    }

    //! @subpage progressPercentage "progressPercentage JSON property"
    float m_progressPercentage;

    //! @privatesection
    /// defaults for properties if not specified in data

    static constexpr float      c_defaultProgressPercentage = 0.0f;
};

//! @private
/// A UIProgressBar element is a simple element that renders a textured
/// rectangle that fits within the parent's padded rectangle (including the
/// progress bar's margin).  It is expecting only the outer UV extentsfrom its
/// sprite style and will interpolate the UVs from empty to full extents
/// as the quad is rendered according to the progress percentage.  It is an
/// output-only element that will not handle user input.
class UIProgressBar : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIProgressBar, ProgressBar)

public:
    virtual ~UIProgressBar() = default;

    /// note: the provided percentage will be clamped to be between
    /// 0.0 and 1.0 (0% to 100%)
    void SetProgressPercentage(float percentage);

    /// returns a progress percentage that ranges from 0.0 to 1.0
    float GetProgressPercentage() const
    {
        return m_progressBarDataProperties.m_progressPercentage;
    }

protected:
    ProgressBarDataProperties m_progressBarDataProperties;

protected:
    UIProgressBar(UIManager& uiManager, ID id) : UIElement(uiManager, id) {}

    void Render() override;
};

//! @private
/// A factory for creating UIProgressBars from data to be managed by
/// the provided UIManager.
class UIProgressBarFactory : public UIElementFactory<UIProgressBar>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ ProgressBarDataProperties&);

protected:
    /*virtual*/ UIProgressBar* Create(UIManager& manager, ID id, UIDataPtr data)
    {
        auto newProgressBar = UIElementFactory<UIProgressBar>::Create(manager, id, data);
        UIProgressBarFactory::DeserializeDataProperties(
            data,
            newProgressBar->m_progressBarDataProperties);
        return newProgressBar;
    }
};

NAMESPACE_ATG_UITK_END
