//--------------------------------------------------------------------------------------
// ProgressTracker.cpp
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ProgressTracker.h"

//--------------------------------------------------------------------------------------
// Name: Default contructor
// Desc: Set initial internal values to defaults
//--------------------------------------------------------------------------------------
ProgressTracker::ProgressTracker()
    : m_progress(0.f),    
    m_viewport{},
    m_state(ProgressState::NotStarted)
{
}


//--------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes internal variables
//--------------------------------------------------------------------------------------
HRESULT ProgressTracker::Initialize()
{
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SetProgress
// Desc: Lets user set progress as a percentage. 1.0f corresponds to 100%
//--------------------------------------------------------------------------------------
void ProgressTracker::SetProgress(float progress)
{
    m_progress = progress;
}

//--------------------------------------------------------------------------------------
// Name: Render
// Desc: Draws a progress bar to the viewport
//--------------------------------------------------------------------------------------
void ProgressTracker::Render(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>& primitiveBatch)
{
    DirectX::XMVECTOR color = DirectX::XMVectorSet(0.6f, 0.6f, 0.6f, 0.0f);
    D3D12_VIEWPORT viewport = m_viewport;

    if (m_state == ProgressState::Active)
    {
        viewport.Width *= m_progress;

        color = DirectX::XMVectorSet(1.0f, 0.82f, 0.0f, 0.0f);
    }
    else if (m_state == ProgressState::Completed)
    {
        color = DirectX::XMVectorSet(0.33f, 0.73f, 0.25f, 0.0f);
    }

    DX::DrawQuadFromViewport(primitiveBatch, viewport, color);
}
