//--------------------------------------------------------------------------------------
// UIChunk.h
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "ScreenCoordinates.h"

class UIChunk
{
public:
    enum class ProgressState { NotStarted = 0, Active, Completed };
    HRESULT Initialize();

    UIChunk(const char* packageId, uint32_t chunkId);
    ~UIChunk();

    void SetProgressState(ProgressState state) { m_state = state; }
    void SetProgress(float progress);

    ProgressState GetProgressState() const { return m_state; }
    float GetProgress() const { return m_progress; }

    void UpdatePackageAvailability(const XPackageChunkAvailability& updatedAvailability) { m_availability = updatedAvailability; }

    bool IsInstalled() const;
    bool IsAvailable() const;
    bool IsPending() const;

    void SetViewport(_In_ const D3D12_VIEWPORT& viewport) { memcpy(&m_viewport, &viewport, sizeof(m_viewport)); }
    void Render(_In_ DirectX::PrimitiveBatch<DirectX::VertexPositionColor>& pPrimitiveBatch);

    DirectX::XMFLOAT3   m_chunkColor;     
    const WCHAR*        m_chunkName;        // Friendly name for each chunk
    const char*         m_chunkSpecifier;   // Specifier name for the chunk
    uint32_t            m_chunkId;          // Chunk Id
    const char*         m_packageId;        // Identifier of the package

private:

    XPackageChunkAvailability           m_availability;

    D3D12_VIEWPORT                      m_viewport;
    ProgressState                       m_state;
    float                               m_progress;
};
