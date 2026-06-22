//--------------------------------------------------------------------------------------
// ProgressTracker.h
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "ScreenCoordinates.h"

//--------------------------------------------------------------------------------------
// Name: class ProgressTracker
// Desc: Tracks progress as a percentage that can be rendered as a progress bar.
//--------------------------------------------------------------------------------------
class ProgressTracker
{
public:

    enum class ProgressState { NotStarted = 0, Active, Completed };

    ProgressTracker();
    ~ProgressTracker() {}

    HRESULT Initialize();

    void SetProgressState( ProgressState state ) { m_state = state; }
    void SetProgress( float progress );

    ProgressState GetProgressState() const { return m_state; }
    float GetProgress() const {  return m_progress; }

    void SetViewport( _In_ const D3D12_VIEWPORT& viewport ) { memcpy( &m_viewport, &viewport, sizeof( m_viewport ) ); }
    void Render( _In_ DirectX::PrimitiveBatch<DirectX::VertexPositionColor>& pPrimitiveBatch);

private:

	float             m_progress;
    D3D12_VIEWPORT    m_viewport;
    ProgressState     m_state;
};