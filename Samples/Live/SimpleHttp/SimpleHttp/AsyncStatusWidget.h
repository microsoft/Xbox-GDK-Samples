//-----------------------------------------------------------------------------
// AsyncStatusWidget.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include "UITK.h"

class AsyncOpWidget
{
public:
    AsyncOpWidget(ATG::UITK::UIManager& uiManager, std::string prefabAssetPath) :
        m_totalElapsedTime(0.0f),
        m_uiManager(uiManager),
        m_prefabAssetPath(prefabAssetPath)
    {
    }

    ~AsyncOpWidget() = default;

    void Show(const std::string& description);
    void Show();
    void Hide();
    void Update(float elapsedFrameTimeInS);
    void SetAsyncOpDescription(const std::string& description);

private:
    void Initialize();

private:
    int m_currentDotCount = 0;
    float m_totalElapsedTime = 0.0f;
    ATG::UITK::UIManager& m_uiManager;
    std::string m_prefabAssetPath;
    std::string m_opDescription;
    ATG::UITK::UIElementPtr m_panel;
    std::shared_ptr<ATG::UITK::UIStaticText> m_opDescriptionText;
    std::shared_ptr<ATG::UITK::UIStaticText> m_opProgressDots;
};
