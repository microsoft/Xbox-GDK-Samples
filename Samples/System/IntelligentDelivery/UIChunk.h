//--------------------------------------------------------------------------------------
// UIChunk.h
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "UIWidgets.h"

class UIChunk
{
public:
    UIChunk(
        const char* packageId,
        uint16_t chunkId,
        const char* name,
        const char* specifier,
        XPackageChunkSelectorType type,
        std::shared_ptr<ATG::UITK::UIButton> button
        );

    ~UIChunk();

    bool IsInstalled() const;
    bool IsPending() const;
    bool IsInstallable() const;
    bool IsRemovable() const;

    const char*               m_packageId;   // Identifier of the package
    uint16_t                  m_id;          // Chunk Id
    const char*               m_name;        // Chunk name
    const char*               m_specifier;   // Specifier name for the chunk
    XPackageChunkSelectorType m_type;        // Chunk Type

    std::shared_ptr<ATG::UITK::UIButton> m_button;

    void EstimateDownloadSize();

    uint16_t GetId() const { return m_id; };
    const char* GetName() const { return m_name; }

    XPackageChunkSelector GetSelector() const;

    void UpdateStatus();

    std::shared_ptr<ATG::UITK::UIButton> GetButton()
    {
        return m_button;
    }

    void SetLabel(const char* text)
    {
        m_button->GetTypedSubElementById<ATG::UITK::UIStaticText>(ATG::UITK::ID("boxlabel"))->SetDisplayText(text);
    }

    void SetStatus(const char* text)
    {
        m_button->GetTypedSubElementById<ATG::UITK::UIPanel>(ATG::UITK::ID("boxstatuspanel"))->
            GetTypedSubElementById<ATG::UITK::UIStaticText>(ATG::UITK::ID("boxstatus"))->SetDisplayText(text);
    }

    void ShowStatus(bool show)
    {
        auto panel = m_button->GetTypedSubElementById<ATG::UITK::UIPanel>(ATG::UITK::ID("boxstatuspanel"));
        auto status = panel->GetTypedSubElementById<ATG::UITK::UIStaticText>(ATG::UITK::ID("boxstatus"))->GetDisplayText();

        if (status.length() > 0)
        {
            panel->SetVisible(show);
        }
    }

    void SetProgress(float f)
    {
        m_button->GetTypedSubElementById<ATG::UITK::UIPanel>(ATG::UITK::ID("boxprogresspanel"))->
            GetTypedSubElementById<ATG::UITK::UIProgressBar>(ATG::UITK::ID("boxprogress"))->SetProgressPercentage(f);
    }

private:

    uint64_t                  m_downloadSize;
};
