//--------------------------------------------------------------------------------------
// PanelScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "PanelScreen.h"
#include "FrontPanelManager.h"
#include "StepTimer.h"

using namespace ATG;

PanelScreen::~PanelScreen()
{
    m_owner = nullptr;
}

void PanelScreen::Update(DX::StepTimer const &)
{
}

void PanelScreen::OnNeighborsChanged()
{
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

bool PanelScreen::OnButtonPressed(XFrontPanelButton whichButton)
{
    switch (whichButton)
    {
    default:
        break;

    case XFrontPanelButton::DPadLeft:
        if (m_leftNeighbor)
        {
            m_leftNeighbor->Resume(this);
            return true;
        }
        break;

    case XFrontPanelButton::DPadRight:
        if (m_rightNeighbor)
        {
            m_rightNeighbor->Resume(this);
            return true;
        }
        break;

    case XFrontPanelButton::DPadUp:
        if (m_upNeighbor)
        {
            m_upNeighbor->Resume(this);
            return true;
        }
        break;

    case XFrontPanelButton::DPadDown:
        if (m_downNeighbor)
        {
            m_downNeighbor->Resume(this);
            return true;
        }
        break;
    }
    return false;
}

void PanelScreen::Resume(const PanelScreen *)
{
    m_owner->Navigate(*this);
    RenderFrontPanel();
}

void PanelScreen::CreateDeviceDependentResources(DX::DeviceResources *)
{
}

void PanelScreen::CreateWindowSizeDependentResources(DX::DeviceResources *)
{
}

void PanelScreen::GPURender(DX::DeviceResources *)
{
}

void PanelScreen::SetLeftNeighbor(PanelScreen * leftNeighbor)
{
    m_leftNeighbor = leftNeighbor;
    OnNeighborsChanged();
}

void PanelScreen::SetRightNeighbor(PanelScreen * rightNeighbor)
{
    m_rightNeighbor = rightNeighbor;
    OnNeighborsChanged();
}

void PanelScreen::SetUpNeighbor(PanelScreen * upNeighbor)
{
    m_upNeighbor = upNeighbor;
    OnNeighborsChanged();
}

void PanelScreen::SetDownNeighbor(PanelScreen * downNeighbor)
{
    m_downNeighbor = downNeighbor;
    OnNeighborsChanged();
}
