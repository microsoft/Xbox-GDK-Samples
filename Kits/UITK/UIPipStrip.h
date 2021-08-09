//--------------------------------------------------------------------------------------
// File: UIPipStrip.h
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
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

enum class PipStripOrientation
{
    LeftToRight = 0,
    TopToBottom = 1,
};

//! @anchor PipStrip
//! @class PipStripDataProperties UIPipStrip.h "UITK/UIPipStrip.h"
//! @brief Pip strip UI element instance properties specific to strips of pips.
struct PipStripDataProperties
{
    PipStripDataProperties() :
        m_numberOfPips(c_defaultInitialNumberOfPips),
        m_pipSize(c_defaultPipSize),
        m_pipSpacing(c_defaultPipSpacing),
        m_stripOrientation(c_defaultPipStripOrientation)
    {
    }

    // the sprite style to render an inactive pip
    //! @subpage inactivePipStyleId "inactivePipStyleId JSON property"
    ID m_inactivePipStyleId;

    // the sprite style to render an active pip
    //! @subpage activePipStyleId "activePipStyleId JSON property"
    ID m_activePipStyleId;

    // the initial number of pips to support
    //! @subpage numberOfPips "numberOfPips JSON property"
    uint32_t m_numberOfPips;

    // the size of a single pip
    //! @subpage pipSize "pipSize JSON property"
    Vector2 m_pipSize;

    // the amount of space between pips
    //! @subpage pipSpacing "pipSpacing JSON property"
    float m_pipSpacing;

    // the orientation of the pip strip
    //! @subpage stripOrientation "stripOrientation JSON property"
    PipStripOrientation m_stripOrientation;

    //! @privatesection
    /// defaults for properties if not specified in data

    static constexpr uint32_t c_defaultInitialNumberOfPips = 0;
    static constexpr Vector2 c_defaultPipSize = { 20, 20 };
    static constexpr float c_defaultPipSpacing = 0.0f;
    static constexpr PipStripOrientation c_defaultPipStripOrientation = PipStripOrientation::LeftToRight;
};

//! @private
/// UIPipStrips are a purely visual element which display in a strip (that can
/// be either vertical [top to bottom], horizontal [left to right]) with one of
/// the pips considered "active," and the other pips considered inactive.
class UIPipStrip : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIPipStrip, PipStrip)

public:
    virtual ~UIPipStrip() = default;

    // add your type-specific public APIs here...

    void SetPipCount(uint32_t pipCount)
    {
        m_pipStripDataProperties.m_numberOfPips = pipCount;

        float totalSpacing = m_pipStripDataProperties.m_pipSpacing * (int(pipCount) - 1);

        Vector2 sizeOfAllPips;
        switch (m_pipStripDataProperties.m_stripOrientation)
        {
        case PipStripOrientation::LeftToRight:
            sizeOfAllPips = DirectX::SimpleMath::Vector2(
                m_pipStripDataProperties.m_pipSize.x * pipCount + totalSpacing,
                m_pipStripDataProperties.m_pipSize.y );
            break;
        case PipStripOrientation::TopToBottom:
            sizeOfAllPips = DirectX::SimpleMath::Vector2(
                m_pipStripDataProperties.m_pipSize.x,
                m_pipStripDataProperties.m_pipSize.y * pipCount + totalSpacing);
            break;
        default:
            throw std::exception("Unhandled pip strip orientation in method UIPipStrip::SetPipCount()");
        }

        UIElement::SetRelativeSizeInRefUnits(sizeOfAllPips);

        SetActivePipIndex(m_activePipIndex);
    }

    void SetActivePipIndex(uint32_t activePipIndex)
    {
        m_activePipIndex = activePipIndex > m_pipStripDataProperties.m_numberOfPips ? 0 : activePipIndex;
    }

protected:
    PipStripDataProperties m_pipStripDataProperties;

    uint32_t m_activePipIndex;

protected:
    UIPipStrip(UIManager& uiManager, ID id) : UIElement(uiManager, id),
        m_activePipIndex(0)
    {
        // add your construction logic here...
    }

    bool HandleInputEvent(const InputEvent& /*inputEvent*/) override { return false; }
    void Update(float /*elapsedTimeInS*/) override {}
    void Render() override;
};

//! @private
class UIPipStripFactory : public UIElementFactory<UIPipStrip>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ PipStripDataProperties&);

protected:
    UIPipStrip* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newPipStrip = UIElementFactory<UIPipStrip>::Create(manager, id, data);
        UIPipStripFactory::DeserializeDataProperties(
            data,
            newPipStrip->m_pipStripDataProperties);
        return newPipStrip;
    }
};

NAMESPACE_ATG_UITK_END
