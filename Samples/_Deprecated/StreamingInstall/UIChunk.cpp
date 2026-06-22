//--------------------------------------------------------------------------------------
// UIChunk.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UIChunk.h"

UIChunk::UIChunk(const char* packageId, uint32_t chunkId) :
    m_chunkColor{},
    m_chunkName(nullptr),
    m_chunkSpecifier(nullptr),
    m_chunkId(chunkId),
    m_packageId(packageId),
    m_availability(XPackageChunkAvailability::Unavailable),
    m_viewport{},
    m_state(ProgressState::NotStarted),
    m_progress(0.f)
{
}

UIChunk::~UIChunk()
{
}

//--------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes internal variables
//--------------------------------------------------------------------------------------
HRESULT UIChunk::Initialize()
{
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: SetProgress
// Desc: Lets user set progress as a percentage. 1.0f corresponds to 100%
//--------------------------------------------------------------------------------------
void UIChunk::SetProgress(float progress)
{
	m_progress = progress;
}

bool UIChunk::IsInstalled() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector;
    selector.type = XPackageChunkSelectorType::Chunk;
    selector.chunkId = m_chunkId;
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);
    return availability == XPackageChunkAvailability::Ready;
}

bool UIChunk::IsAvailable() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector;
    selector.type = XPackageChunkSelectorType::Chunk;
    selector.chunkId = m_chunkId;
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);
    return availability == XPackageChunkAvailability::Installable;
}

bool UIChunk::IsPending() const
{
    XPackageChunkAvailability availability;
    XPackageChunkSelector selector;
    selector.type = XPackageChunkSelectorType::Chunk;
    selector.chunkId = m_chunkId;
    XPackageFindChunkAvailability(m_packageId, 1, &selector, &availability);
    return availability == XPackageChunkAvailability::Pending;
}

//--------------------------------------------------------------------------------------
// Name: Render
// Desc: Draws a progress bar to the viewport
//--------------------------------------------------------------------------------------
void UIChunk::Render(_In_ DirectX::PrimitiveBatch<DirectX::VertexPositionColor>& primitiveBatch)
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
