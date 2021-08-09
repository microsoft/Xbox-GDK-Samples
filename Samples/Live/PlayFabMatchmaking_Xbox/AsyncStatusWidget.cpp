#include "pch.h"
#include "AsyncStatusWidget.h"

using namespace ATG::UITK;

namespace
{
    constexpr float c_DotsPerSecond = 2.0f;
    constexpr int c_TotalDotCount = 4;
    constexpr const char* c_DotStrings[c_TotalDotCount] =
    {
        u8".", u8"..", u8"...", u8"...."
    };
}

void AsyncOpWidget::Show(std::string description)
{
    m_opDescription = description;
    Show();
}

void AsyncOpWidget::Show()
{
    if (!m_panel)
    {
        Initialize();
    }

    m_uiManager.Detach(m_panel);
    m_uiManager.AttachTo(m_panel, m_uiManager.GetRootElement());

    SetAsyncOpDescription(m_opDescription);

    m_panel->SetVisible(true);
}

void AsyncOpWidget::Hide()
{
    if (m_panel)
    {
        m_panel->SetVisible(false);
    }
}

void AsyncOpWidget::Update(float elapsedFrameTimeInS)
{
    m_totalElapsedTime += elapsedFrameTimeInS;
    auto currentDots = m_totalElapsedTime * c_DotsPerSecond;
    m_currentDotCount = static_cast<int>(currentDots) % c_TotalDotCount;
    m_opProgressDots->SetDisplayText(c_DotStrings[m_currentDotCount]);
}

void AsyncOpWidget::SetAsyncOpDescription(std::string description)
{
    m_opDescription = description;

    if (m_panel)
    {
        m_opDescriptionText->SetDisplayText(m_opDescription);
        m_currentDotCount = 0;
        m_opProgressDots->SetDisplayText(c_DotStrings[m_currentDotCount]);
    }
}

/*private:*/ void AsyncOpWidget::Initialize()
{
    m_panel = m_uiManager.InstantiatePrefab(m_prefabAssetPath);
    m_opDescriptionText = m_panel->GetTypedSubElementById<UIStaticText>(ID("Op_Description_Text"));
    m_opProgressDots = m_panel->GetTypedSubElementById<UIStaticText>(ID("Op_Progress_Dots"));

    if (m_opDescription.empty())
    {
        m_opDescription = m_opDescriptionText->GetDisplayText();
    }
}
