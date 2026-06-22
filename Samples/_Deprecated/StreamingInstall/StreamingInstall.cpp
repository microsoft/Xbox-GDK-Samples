//--------------------------------------------------------------------------------------
// StreamingInstall.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StreamingInstall.h"

#include "ATGColors.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_SampleTitle = L"Streaming Install and Intelligent Delivery Sample";
    const wchar_t* g_OnDemandToAdd = L"\n\n\'A to install\'\0";
    const wchar_t* g_OnDemandToDel = L"\n\n\'X to uninstall\'\0";

    const uint32_t g_numChunks = 8;			// The number of chunks to stream (excluding the launch chunk which is streamed before the sample can be launched)
    const uint32_t g_numRows = 2;           // Number of rows of chunk items displayed
    const uint32_t g_numCols = 4;           // Number of chunk items displayed in a row
    const uint32_t g_OnDemandChunkNum = 7;  // Chunk number of the on demand sample

    const XMVECTORF32 g_ChunkNotInstallingColor = ATG::Colors::LightGrey;

    // Chunk mappings - this version of the sample has moved to an explicit array of chunk IDs to separate the logic of chunk ID queries from layout

                                                      //Col 0                           Col 1                            Col 2                            Col 3
    const uint32_t g_chunkIds[g_numRows][g_numCols] = { {1001,                            1002,                            1003,                            1004},
                                                      {1005,                            1006,                            1007,                            1008} };

    XMFLOAT3 g_chunkColor[g_numChunks] =            { XMFLOAT3(1.00f, 0.71f, 0.00f),   XMFLOAT3(0.77f, 0.55f, 0.01f),   XMFLOAT3(1.00f, 0.56f, 0.00f),   XMFLOAT3(0.64f, 0.35f, 0.01f),
                                                      XMFLOAT3(0.84f, 0.29f, 0.00f),   XMFLOAT3(0.53f, 0.18f, 0.01f),   XMFLOAT3(0.83f, 0.08f, 0.00f),   XMFLOAT3(0.52f, 0.05f, 0.01f) };

#ifdef _GAMING_XBOX_SCARLETT
    const wchar_t* g_chunkNames[g_numChunks] =      { L"Stock",                        L"Custom",                       L"Lockhart",                     L"Anaconda",
                                                      L"En",                           L"Fr",                           L"Es",                           L"OnDemand\nMapEditor" };
#else
    const wchar_t* g_chunkNames[g_numChunks] =      { L"Stock",                        L"Custom",                       L"Durango",                      L"Scorpio",
                                                      L"En",                           L"Fr",                           L"Es",                           L"OnDemand\nMapEditor" };
#endif

    const char* g_chunkSpecifiers[g_numChunks] =    { nullptr,                          nullptr,                         nullptr,                         nullptr,
                                                      "en",                             "fr",                            "es",                            "MapEditor" };
}

Sample::Sample() noexcept(false) :
    m_highlightedChunk(0),
    m_highlightedUIChunk(nullptr),
    m_chunkMode(false),
    m_userDefaultLocaleString(nullptr),
    m_packageIdentifier{},
    m_packageIdentifierValid(false),
    m_pimHandleOverall(nullptr),
    m_pimHandleOverallActive(false),
    m_taskQueue(nullptr),
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue)
    );

    m_packageIdentifierValid = 
        SUCCEEDED(XPackageGetCurrentProcessPackageIdentifier(ARRAYSIZE(m_packageIdentifier), m_packageIdentifier));

    for (UINT i = 0; i < g_numChunks; i++)
    {
        UINT curChunkId = GetChunkInstallIdFromIndex(i);
        std::unique_ptr<UIChunk> curUIChunk = std::make_unique<UIChunk>(m_packageIdentifier, curChunkId);
        curUIChunk->m_chunkColor = g_chunkColor[i];
        curUIChunk->m_chunkName = g_chunkNames[i];
        curUIChunk->m_chunkSpecifier = g_chunkSpecifiers[i];
        m_UIChunks.insert(std::make_pair(curChunkId, std::move(curUIChunk)));
    }

    m_highlightedUIChunk = m_UIChunks[GetChunkInstallIdFromIndex(0)].get();

    // Get the locale name for the system using XPackageGetUserLocale(). This will return the locale selected through the 
    // settings app only if that locale has been added to the Resources section of the application's MicrosoftGame.config.
    // In case the resource is absent from the config, this API will return the first locale in the Resource Language.
    char localeName[LOCALE_NAME_MAX_LENGTH] = {};
    if(SUCCEEDED(XPackageGetUserLocale(ARRAYSIZE(localeName), localeName)))
    {
        const char* languageMessage = "Language setting from XPackageGetUserLocale API";
        size_t count = LOCALE_NAME_MAX_LENGTH + strlen(languageMessage);
        m_userDefaultLocaleString = new char[count];
        sprintf_s(m_userDefaultLocaleString, count, "%s: %s", languageMessage, localeName);
    }
    else
    {
        const char* languageMessage = "Language setting from XPackageGetUserLocale API failed!\nCheck readme.docx for instructions to generate a package and use streaming-install. Streaming-install must be used for this sample.";
        size_t count = LOCALE_NAME_MAX_LENGTH + strlen(languageMessage);
        m_userDefaultLocaleString = new char[count];
        sprintf_s(m_userDefaultLocaleString, count, "%s", languageMessage);
    }
}

Sample::~Sample()
{
    if(m_pimHandleOverallActive)
    {
        XPackageCloseInstallationMonitorHandle(m_pimHandleOverall);
    }
    if(m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    if (!XPackageIsPackagedProcess())
    {
        // This sample requires a packaged rather than loose  deployment.
        // See the readme.docx.
        throw std::exception("Title is not packaged");
    }

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    if(!m_packageIdentifierValid)
    {
        return;
    }

    for (unsigned i = 0; i < g_numChunks; ++i)
    {
        WatchChunk(i);
    }

    SetupTransferPackageProgress();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0))
    { }

    auto pad = m_gamePad->GetState(0);

    // If the chunk is in use, the only allowed action is exit.
    if (m_chunkMode)
    {
        if(m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_chunkMode = false;
        }
    }
    else
    {
        if(m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_highlightedChunk % g_numCols == 0)
            {
                m_highlightedChunk += (g_numCols - 1);
            }
            else
            {
                m_highlightedChunk -= 1;
            }
        }

        if(m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_highlightedChunk % g_numCols == (g_numCols - 1))
            {
                m_highlightedChunk -= (g_numCols - 1);
            }
            else
            {
                m_highlightedChunk += 1;
            }
        }

        if( m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED ||
            m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED )
        {
            if (m_highlightedChunk / g_numCols == 0)
            {
                m_highlightedChunk += g_numCols;
            }
            else
            {
                m_highlightedChunk -= g_numCols;
            }
        }

        m_highlightedUIChunk = m_UIChunks[GetChunkInstallIdFromIndex(m_highlightedChunk)].get();

        if( m_packageIdentifierValid && 
            (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED ||
            m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED) )
        {
            if (m_highlightedUIChunk->m_chunkSpecifier != nullptr)
            {
                // Setup selector for chunk to modify
                XPackageChunkSelector selector;
                if (m_highlightedChunk == g_OnDemandChunkNum)
                {
                    selector.type = XPackageChunkSelectorType::Tag;
                    selector.tag = m_highlightedUIChunk->m_chunkSpecifier;
                }
                else
                {
                    selector.type = XPackageChunkSelectorType::Language;
                    selector.language = m_highlightedUIChunk->m_chunkSpecifier;
                }

                // Remove installed chunk if x is pressed
                if( m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED && m_highlightedUIChunk->IsInstalled() )
                {
                    DX::ThrowIfFailed(XPackageUninstallChunks(m_packageIdentifier, 1, &selector));

                    // Check if uninstall worked as the last language cannot be removed
                    XPackageChunkAvailability availability;
                    DX::ThrowIfFailed(XPackageFindChunkAvailability(m_packageIdentifier, 1, &selector, &availability));
                    m_highlightedUIChunk->UpdatePackageAvailability(availability);
                    if(!m_highlightedUIChunk->IsInstalled())
                    {
                        m_highlightedUIChunk->SetProgressState(UIChunk::ProgressState::NotStarted);
                        m_highlightedUIChunk->SetProgress(0.0f);
                    }
                }

                // Install an available chunk if a is pressed
                // A notification will be shown to the user to accept/deny
                if( m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED && m_highlightedUIChunk->IsAvailable() )
                {
                    auto asyncBlock = new XAsyncBlock{};
                    asyncBlock->queue = m_taskQueue;
                    asyncBlock->context = this;
                    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
                    {
                        auto pSample = reinterpret_cast<Sample*>(asyncBlock->context);
                        XPackageInstallationMonitorHandle pimHandle;
                        HRESULT result = XPackageInstallChunksResult(asyncBlock, &pimHandle);

                        // XPackageInstallChunks fails if canceled by user, so don't throw on failure
                        if (SUCCEEDED(result))
                        {
                            // Register a callback to monitor the updates. Update callbacks will also notify when install completes.
                            XTaskQueueRegistrationToken callbackToken;
                            DX::ThrowIfFailed(XPackageRegisterInstallationProgressChanged(pimHandle, pSample->m_highlightedUIChunk,
                                [](void* context, XPackageInstallationMonitorHandle pimHandle)
                                {
                                    UIChunk* m_highlightedUIChunk = static_cast<UIChunk*>(context);

                                    XPackageInstallationProgress progress;
                                    XPackageGetInstallationProgress(pimHandle, &progress);

                                    if (!progress.completed)
                                    {
                                        m_highlightedUIChunk->SetProgressState(UIChunk::ProgressState::Active);
                                        m_highlightedUIChunk->SetProgress(static_cast<float>(static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
                                        m_highlightedUIChunk->UpdatePackageAvailability(XPackageChunkAvailability::Pending);
                                    }
                                    else
                                    {
                                        m_highlightedUIChunk->SetProgressState(UIChunk::ProgressState::Completed);
                                        m_highlightedUIChunk->SetProgress(1.0f);
                                        m_highlightedUIChunk->UpdatePackageAvailability(XPackageChunkAvailability::Ready);

                                        XPackageCloseInstallationMonitorHandle(pimHandle);
                                    }
                                }, &callbackToken));

                            //Re-initialize the overall package transfer watching code so that the progress bar reflects the additional data coming down
                            //(Including if the package had been shown as completely installed before)
                            pSample->SetupTransferPackageProgress();
                        }

                        delete asyncBlock;
                    };

                    // XPackageInstallChunks() blocks on user interaction, so use the async version to avoid the program stalling while waiting.
                    HRESULT hr = XPackageInstallChunksAsync(
                        m_packageIdentifier,
                        1,
                        &selector,
                        1000,
                        false,
                        asyncBlock);
                    if (FAILED(hr))
                    {
                        delete asyncBlock;
                    }
                }
            }

            if( m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED )
            {
                // Enter chunk mode only if the chunk has been installed, otherwise move the chunk first in priority list so we can access it sooner.
                if (m_highlightedUIChunk->IsInstalled())
                {
                    m_chunkMode = true;
                }
                else
                {
                    // Make chunk first in download order
                    XPackageChunkSelector selector;
                    selector.type = XPackageChunkSelectorType::Chunk;
                    selector.chunkId = GetChunkInstallIdFromIndex(m_highlightedChunk);
                    DX::ThrowIfFailed(XPackageChangeChunkInstallOrder(m_packageIdentifier, 1, &selector));
                }
            }

        }
    }

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Size of output window and safe area
    auto size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    // The height of a scroll bar. Serves as the base unit to position most UI elements.
    const float scrollBarHeight = static_cast<float>(safe.bottom - safe.top) / 32.0f;

    // 2D screen coordinate management helpr class
    ScreenCoordinates screenCoords(safe, scrollBarHeight, g_numRows, g_numCols);

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_drawEffect->Apply(commandList);
    m_drawBatch->Begin(commandList);

    // Render the overall progress bar at the bottom of the screen.
    m_overallProgress.SetViewport(screenCoords.GetViewportForOverallProgressBar());
    m_overallProgress.Render(*m_drawBatch.get());

    if (m_chunkMode)
    {
        D3D12_VIEWPORT viewport = screenCoords.GetViewportForChunkMode();
        DX::DrawQuadFromViewport(*m_drawBatch.get(), viewport, XMLoadFloat3(&m_highlightedUIChunk->m_chunkColor));
    }
    else
    {
        // Display thumbnails and progress bars for each chunk
        for (unsigned i = 0; i < g_numChunks; ++i)
        {
            UIChunk* curChunk = m_UIChunks[GetChunkInstallIdFromIndex(i)].get();
            D3D12_VIEWPORT thumbnailViewport = screenCoords.GetViewportForChunkThumbnail(i / g_numCols, i % g_numCols);

            if (i == m_highlightedChunk)
            {
                // Draw a highlight around the currently selected chunk
                D3D12_VIEWPORT highlightViewport = screenCoords.GetViewportForChunkThumbnailHighlight(i / g_numCols, i % g_numCols);

                DX::DrawQuadFromViewport(*m_drawBatch.get(), highlightViewport, ATG::Colors::White);
            }

            if (curChunk->IsInstalled() ||
                curChunk->IsPending())
            {
                DX::DrawQuadFromViewport(*m_drawBatch.get(), thumbnailViewport, XMLoadFloat3(&curChunk->m_chunkColor));
            }
            else
            {
                DX::DrawQuadFromViewport(*m_drawBatch.get(), thumbnailViewport, g_ChunkNotInstallingColor);
            }

            curChunk->SetViewport(screenCoords.GetViewportForChunkProgressBar(i / g_numCols, i % g_numCols));
            curChunk->Render(*m_drawBatch.get());
        }
    }
    m_drawBatch->End();

    // Fonts
    m_sprites->Begin(commandList);

    // Render this sample's title
    m_font->DrawString(m_sprites.get(), g_SampleTitle, XMFLOAT2(float(safe.left), float(safe.top)), ATG::Colors::White);
    m_font->DrawString(m_sprites.get(), m_userDefaultLocaleString, XMFLOAT2(float(safe.left), float(safe.top) + 32.f), 
        m_packageIdentifierValid ? ATG::Colors::White : Colors::Red);
    m_font->DrawString(m_sprites.get(), L"Overall package install progress", XMFLOAT2(float(safe.left), float(safe.bottom - scrollBarHeight)), ATG::Colors::White);

    if (m_chunkMode)
    {
        D3D12_VIEWPORT viewport = screenCoords.GetViewportForChunkMode();
        DX::DrawCenteredText(m_font.get(), m_sprites.get(), L"This chunk is fully installed\n\nPress 'B' to exit to main menu",
            XMFLOAT2(viewport.TopLeftX + (viewport.Width / 2), viewport.TopLeftY + (viewport.Height / 2)), ATG::Colors::White);
    }
    else
    {
        // Display thumbnails and progress bars for each chunk
        for (unsigned i = 0; i < g_numChunks; ++i)
        {
            UIChunk* curChunk = m_UIChunks[GetChunkInstallIdFromIndex(i)].get();
            D3D12_VIEWPORT nameViewport = screenCoords.GetViewportForChunkName(i / g_numCols, i % g_numCols);

            if (m_highlightedChunk >= 4 && i == m_highlightedChunk)
            {
                DX::DrawCenteredText(m_font.get(), m_sprites.get(), (m_highlightedUIChunk->IsInstalled()) ? g_OnDemandToDel : g_OnDemandToAdd,
                    XMFLOAT2(nameViewport.TopLeftX + (nameViewport.Width / 2), nameViewport.TopLeftY + (nameViewport.Height / 2)), ATG::Colors::White);
            }
            DX::DrawCenteredText(m_font.get(), m_sprites.get(), curChunk->m_chunkName,
                XMFLOAT2(nameViewport.TopLeftX + (nameViewport.Width / 2), nameViewport.TopLeftY + (nameViewport.Height / 2)), ATG::Colors::White);
        }
    }

    m_sprites->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}
#pragma endregion

void Sample::SetupTransferPackageProgress()
{
    if(m_pimHandleOverallActive)
    {
        return;
    }

    DX::ThrowIfFailed(XPackageCreateInstallationMonitor(
        m_packageIdentifier,
        0,
        nullptr,
        1000,
        m_taskQueue,
        &m_pimHandleOverall
        ));
    m_pimHandleOverallActive = true;

    // Register a callback to monitor the overall installation state.  In GXDK, the progress-changed event
    // will also notify if installation completes.
    // Alternatively, you can manually check progress on your own by calling XPackageGetInstallationProgress()
    // whenever you want after creating the installation monitor.
    XTaskQueueRegistrationToken callbackToken;
    DX::ThrowIfFailed(XPackageRegisterInstallationProgressChanged(m_pimHandleOverall, this, 
        [](void* context, XPackageInstallationMonitorHandle pimHandleOverall)
    {
        Sample* pThis = static_cast<Sample*>(context);

        XPackageInstallationProgress progress;
        XPackageGetInstallationProgress(pimHandleOverall, &progress);

        if(!progress.completed)
        {
            pThis->m_overallProgress.SetProgressState(ProgressTracker::ProgressState::Active);
            pThis->m_overallProgress.SetProgress(static_cast<float>(static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
        }
        else
        {
            pThis->m_overallProgress.SetProgressState(ProgressTracker::ProgressState::Completed);
            pThis->m_overallProgress.SetProgress(1.0f);

            //It's possible to get future updates after completed is true. This can happen if chunks are later added by the game. Because of this the monitor handle is kept active
            //XPackageCloseInstallationMonitorHandle(pimHandleOverall);
            //pThis->m_pimHandleOverallActive = false;
        }
    }, &callbackToken));
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandQueue = m_deviceResources->GetCommandQueue();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_overallProgress.Initialize();
    for (std::map<uint32_t, std::unique_ptr<UIChunk>>::iterator it = m_UIChunks.begin(); it != m_UIChunks.end(); ++it)
    {
        (it->second->Initialize());
    }

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    {
        SpriteBatchPipelineStateDescription spritePsoDesc(rtState, &CommonStates::AlphaBlend);
        ResourceUploadBatch upload(device);
        upload.Begin();

        m_sprites = std::make_unique<SpriteBatch>(device, upload, spritePsoDesc);
        m_font = std::make_unique<DirectX::SpriteFont>(device, upload, L"SegoeUI_18.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
            m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

        upload.End(commandQueue);
    }

    // Set up drawing effect and batch
    EffectPipelineStateDescription effectPsoDesc(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, CommonStates::CullNone, rtState);
    m_drawEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, effectPsoDesc);
    m_drawBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_sprites->SetViewport(vp);

    // Scaled input coordinates for effect
    SimpleMath::Matrix proj = SimpleMath::Matrix::CreateOrthographicOffCenter(0.f, float(vp.Width), float(vp.Height), 0.f, 0.f, 1.f);

    m_drawEffect->SetProjection(proj);
}
#pragma endregion

UINT Sample::GetChunkInstallIdFromIndex(UINT highlightedChunk)
{
    return g_chunkIds[highlightedChunk / g_numCols][highlightedChunk % g_numCols];
}

void Sample::WatchChunk(UINT UIChunkIndex)
{
    // Setup which chunk to watch
    XPackageChunkSelector selector;
    selector.type = XPackageChunkSelectorType::Chunk;
    selector.chunkId = GetChunkInstallIdFromIndex(UIChunkIndex); 

    // Check and update availability
    XPackageChunkAvailability availability;
    DX::ThrowIfFailed(XPackageFindChunkAvailability(m_packageIdentifier, 1, &selector, &availability));
    UIChunk* curUIChunk = m_UIChunks[selector.chunkId].get();
    curUIChunk->UpdatePackageAvailability(availability);

    // If already installed, then just set as so
    if(curUIChunk->IsInstalled())
    {
        curUIChunk->SetProgress(1.0f);
        curUIChunk->SetProgressState(UIChunk::ProgressState::Completed);
    }

    // If pending, then setup install progress watching
    if(curUIChunk->IsPending())
    {
        // Create installation monitor
        XPackageInstallationMonitorHandle pimHandle;
        DX::ThrowIfFailed(XPackageCreateInstallationMonitor(
            m_packageIdentifier,
            1,
            &selector,
            1000,
            m_taskQueue,
            &pimHandle
        ));

        // Register a callback to monitor the updates.  Update callbacks will also notify when install completes.
        XTaskQueueRegistrationToken callbackToken;
        DX::ThrowIfFailed(XPackageRegisterInstallationProgressChanged(pimHandle, curUIChunk, 
            [](void* context, XPackageInstallationMonitorHandle pimHandle)
        {
            UIChunk* curUIChunk = static_cast<UIChunk*>(context);

            XPackageInstallationProgress progress;
            XPackageGetInstallationProgress(pimHandle, &progress);

            if(!progress.completed)
            {
                curUIChunk->SetProgressState(UIChunk::ProgressState::Active);
                curUIChunk->SetProgress(static_cast<float>(static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
                curUIChunk->UpdatePackageAvailability(XPackageChunkAvailability::Pending);
            }
            else
            {
                curUIChunk->SetProgressState(UIChunk::ProgressState::Completed);
                curUIChunk->SetProgress(1.0f);
                curUIChunk->UpdatePackageAvailability(XPackageChunkAvailability::Ready);

                XPackageCloseInstallationMonitorHandle(pimHandle);
            }
        }, &callbackToken));
    }
}
