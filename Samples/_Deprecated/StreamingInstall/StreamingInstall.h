//--------------------------------------------------------------------------------------
// StreamingInstall.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "ProgressTracker.h"
#include "UIChunk.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    void SetupTransferPackageProgress();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    UINT GetChunkInstallIdFromIndex(UINT highlightedChunk);
    void WatchChunk(UINT UIChunkIndex);

    ProgressTracker                                                         m_overallProgress;          // Tracks overall streaming progress of the whole sample package
    std::map<UINT, std::unique_ptr<UIChunk>>                                m_UIChunks;                 // Chunks that make up the UI.
    UINT                                                                    m_highlightedChunk;         // Chunk currently highlighted
    UIChunk*                                                                m_highlightedUIChunk;       // Highlighted UIChunk object
    bool                                                                    m_chunkMode;                // Indicates if a chunk is currently active (being displayed on top of the menu)
    char*                                                                   m_userDefaultLocaleString;  // Displays the language the app is being told to use by the system settings

    char                                                                    m_packageIdentifier[XPACKAGE_IDENTIFIER_MAX_LENGTH];
    bool                                                                    m_packageIdentifierValid;
    XPackageInstallationMonitorHandle                                       m_pimHandleOverall;
    bool                                                                    m_pimHandleOverallActive;
    XTaskQueueHandle                                                        m_taskQueue;

    std::unique_ptr<DirectX::SpriteFont>		                            m_font;
    std::unique_ptr<DirectX::SpriteBatch>		                            m_sprites;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                                    m_deviceResources;

    // Rendering loop timer.
    uint64_t                                                                m_frame;
    DX::StepTimer                                                           m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                                       m_gamePad;
    DirectX::GamePad::ButtonStateTracker                                    m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                                m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>                                m_resourceDescriptors;
    std::unique_ptr<DirectX::BasicEffect>                                   m_drawEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_drawBatch;

    enum Descriptors
    {
        Font,
        Count
    };
};
