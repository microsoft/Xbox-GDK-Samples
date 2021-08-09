//--------------------------------------------------------------------------------------
// UIChunk.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UIChunk.h"

UIChunk::UIChunk(
    const char* packageId,
    uint16_t chunkId,
    const char* name,
    const char* specifier,
    XPackageChunkSelectorType type,
    std::shared_ptr<ATG::UITK::UIButton> button):
    m_packageId(packageId),
    m_id(chunkId),
    m_name(name),
    m_specifier(specifier),
    m_type(type),
    m_button(button),
    m_downloadSize(0)
{
    SetLabel(name);
    EstimateDownloadSize();
}

UIChunk::~UIChunk()
{
}

XPackageChunkSelector UIChunk::GetSelector() const
{
    XPackageChunkSelector selector;

    selector.type = m_type;

    switch (selector.type)
    {
        case XPackageChunkSelectorType::Feature:    selector.feature = m_specifier;     break;
        case XPackageChunkSelectorType::Tag:        selector.tag = m_specifier;         break;
        case XPackageChunkSelectorType::Language:   selector.language = m_specifier;    break;
        case XPackageChunkSelectorType::Chunk:
        default:
        {
            selector.type = XPackageChunkSelectorType::Chunk;
            selector.chunkId = m_id;
            break;
        }
    }

    return selector;
}

bool UIChunk::IsInstalled() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector = GetSelector();
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);
    return availability == XPackageChunkAvailability::Ready;
}

bool UIChunk::IsPending() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector = GetSelector();
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);
    return availability == XPackageChunkAvailability::Pending;
}

bool UIChunk::IsInstallable() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector = GetSelector();
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);

    // only features or language chunks can be installed
    return availability == XPackageChunkAvailability::Installable &&
        (m_type != XPackageChunkSelectorType::Chunk);
}

bool UIChunk::IsRemovable() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector = GetSelector();
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);

    // only features or language chunks can be removed
    return availability == XPackageChunkAvailability::Ready &&
        (m_type != XPackageChunkSelectorType::Chunk);
}

void UIChunk::EstimateDownloadSize()
{
    uint64_t downloadSize = 0;
    bool wouldPresentUserConfirmation = false;

    auto selector = GetSelector();

    if (m_type == XPackageChunkSelectorType::Tag ||
        m_type == XPackageChunkSelectorType::Language ||
        m_type == XPackageChunkSelectorType::Feature)
    {
        // XPackageEstimateDownloadSize only takes selector of type language, tag, or feature
        // Note: try to not call this while an install is in progress--takes longer while it pauses the install
        XPackageEstimateDownloadSize(m_packageId, 1, &selector, &downloadSize, &wouldPresentUserConfirmation);

        // convert to MiB
        m_downloadSize = downloadSize >> 20;
    }
}

void UIChunk::UpdateStatus()
{
    if (IsInstallable())
    {
        if (m_downloadSize > 0)
        {
            char text[64] = {};
            snprintf(text, ARRAYSIZE(text), "%llu MB", m_downloadSize);
            SetStatus(text);
        }
        else
        {
            SetStatus("");
        }
    }
    else
    {
        SetStatus(
            IsInstalled() ? "INSTALLED" :
            IsPending() ? "PENDING" : "");
    }
}
