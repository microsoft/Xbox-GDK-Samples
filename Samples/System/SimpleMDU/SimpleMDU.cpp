//--------------------------------------------------------------------------------------
// SimpleMDU.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleMDU.h"

#include "ATGColors.h"
#include "FindMedia.h"

#include "CompressedDataStream.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;


HRESULT HandleInMemoryDataChangeRequestAsync(XAsyncBlock* asyncBlock)
{
    Sample* sample = reinterpret_cast<Sample*>(asyncBlock->context);
    sample->HandleInMemoryDataChangeRequest();
    return S_OK;
}


void Sample::HandleInMemoryDataChangeRequest()
{
    DataStream* targetOriginDataStream = DataStream::Fetch(c_uniqueDataSize, c_randLimits[m_selectedDataSet]);

    if (targetOriginDataStream)
    {
        m_inMemoryQueue.ActiveOriginData = targetOriginDataStream;
    }
    else
    {
        m_inMemoryQueue.ActiveOriginData = DataStream::Generate(c_uniqueDataSize, c_randLimits[m_selectedDataSet]);
    }

    CompressedDataStream* targetCompressedStream = CompressedDataStream::FetchMemoryBacked(m_inMemoryQueue.ActiveOriginData, c_dataSize, c_blockSizeOptions[m_selectedInMemoryBlockSize], c_inMemoryAlignments[m_selectedDataSetAlignment]);

    if (targetCompressedStream)
    {
        m_inMemoryQueue.ActiveCompressedStream = targetCompressedStream;
    }
    else
    {
        // first, check if we should cull some of the data streams, depending on available memory
        XMEM_WORKING_SET_STATISTICS stats = {};
        XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TITLE, &stats);

        size_t availableMem = stats.GameLimit - stats.GameUsed;

        while (availableMem < 2 * c_dataSize)
        {
            CompressedDataStream::ReleaseLRU();
            XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TITLE, &stats);
            availableMem = stats.GameLimit - stats.GameUsed;
        }

        m_inMemoryQueue.ActiveCompressedStream = CompressedDataStream::GenerateInMemory(m_inMemoryQueue.ActiveOriginData, c_dataSize, c_blockSizeOptions[m_selectedInMemoryBlockSize], c_inMemoryAlignments[m_selectedDataSetAlignment]);
    }

    m_inMemoryQueue.TaskStatus = TaskStatus::Idle;
}


HRESULT HandleFromStorageDataChangeRequestAsync(XAsyncBlock* asyncBlock)
{
    Sample* sample = reinterpret_cast<Sample*>(asyncBlock->context);
    sample->HandleFromStorageDataChangeRequest();
    return S_OK;
}


void Sample::HandleFromStorageDataChangeRequest()
{
    // cache these, since we don't block new controller input on current tasks
    float selectedRandSet = c_randLimits[m_selectedDataSet];
    uint32_t selectedBlockSize = c_blockSizeOptions[m_selectedInMemoryBlockSize];
    uint16_t selectedAlignment = c_inMemoryAlignments[m_selectedDataSetAlignment];

    CompressedDataStream* targetStream = CompressedDataStream::HydrateFromStorage(selectedRandSet, c_dataSize, selectedBlockSize, selectedAlignment);

    if (!targetStream)
    {
        m_fromStorageQueue.TaskStatus = TaskStatus::GeneratingNewDataSet;

        // just wait for the InMemory queue handlers to populate this
        targetStream = CompressedDataStream::FetchMemoryBacked(selectedRandSet, c_dataSize, selectedBlockSize, selectedAlignment);
        while (targetStream == nullptr)
        {
            Sleep(16);
            targetStream = CompressedDataStream::FetchMemoryBacked(selectedRandSet, c_dataSize, selectedBlockSize, selectedAlignment);
        }

        m_fromStorageQueue.TaskStatus = TaskStatus::StoringData;
        targetStream->PersistToStorage();
    }

    m_fromStorageQueue.ActiveCompressedStream = targetStream;
    m_fromStorageQueue.TaskStatus = TaskStatus::Idle;
}


Sample::Sample() noexcept(false) :
    m_selectedDataSet(5),           //  ~2:1 copmression ratio
    m_selectedInMemoryBlockSize(2), //  64KiB
    m_selectedDataSetAlignment(0),  //  16 byte aligned to make persisting to\from storage easier
    m_inMemoryQueue(c_uniqueDataSize, c_dataSize, c_randLimits[m_selectedDataSet], c_inMemoryAlignments[m_selectedDataSetAlignment], c_blockSizeOptions[m_selectedInMemoryBlockSize]),
    m_fromStorageQueue(c_uniqueDataSize, c_dataSize, c_randLimits[m_selectedDataSet], c_inMemoryAlignments[m_selectedDataSetAlignment], c_blockSizeOptions[m_selectedInMemoryBlockSize]),
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    if (timer.GetFrameCount() == 3)
    {
        m_inMemoryQueue.StartWorker();
        m_fromStorageQueue.StartWorker();
    }

    // merged input isn't typical for most titles but makes tools and samples easier to use across local and
    // remote scenarios where button presses might come through controller emulation (XboxOneManager etc...)
    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);    
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_inMemoryQueue.TaskStatus == TaskStatus::Idle && m_fromStorageQueue.TaskStatus == TaskStatus::Idle)
        {
            // DPad up\down is to change the compression ratio of the data
            uint8_t dataSet = m_selectedDataSet;
            if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED && dataSet > 0)
            {
                dataSet--;
            }
            else if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED && dataSet < _countof(c_randLimits) - 1)
            {
                dataSet++;
            }

            // DPad left\right is to change the chunk size
            uint8_t blockSize = m_selectedInMemoryBlockSize;
            if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED && blockSize > 0)
            {
                blockSize--;
            }
            else if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED && blockSize < _countof(c_blockSizeOptions)-1)
            {
                blockSize++;
            }

            // Shoulder buttons for alignment (there is a slight effect for from-storage unaligned streams
            uint8_t alignment = m_selectedDataSetAlignment;
            if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED && alignment > 0)
            {
                alignment--;
            }
            else if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED && alignment < _countof(c_inMemoryAlignments) - 1)
            {
                alignment++;
            }

            if (dataSet != m_selectedDataSet || blockSize != m_selectedInMemoryBlockSize || alignment != m_selectedDataSetAlignment)
            {
                // request to change the chunk size
                m_selectedDataSet = dataSet;
                m_selectedInMemoryBlockSize = blockSize;
                m_selectedDataSetAlignment = alignment;

                // first, the in-memory queue...

                DataStream* originData = DataStream::Fetch(c_uniqueDataSize, c_randLimits[m_selectedDataSet]);
                CompressedDataStream* memoryStream = CompressedDataStream::FetchMemoryBacked(originData, c_dataSize, c_blockSizeOptions[blockSize], c_inMemoryAlignments[m_selectedDataSetAlignment]);

                if (originData && memoryStream)
                {
                    // if we already have that configuration, switch right now
                    m_inMemoryQueue.ActiveOriginData = originData;
                    m_inMemoryQueue.ActiveCompressedStream = memoryStream;
                }
                else
                {
                    XAsyncBlock* ab = new XAsyncBlock();
                    ab->context = this;
                    ab->callback = [](XAsyncBlock* block) { delete block; };

                    m_inMemoryQueue.TaskStatus = originData ? TaskStatus::CompressingData : TaskStatus::GeneratingNewDataSet;
                    XAsyncRun(ab, reinterpret_cast<XAsyncWork*>(&HandleInMemoryDataChangeRequestAsync));
                }

                // then the from-storage queue
                CompressedDataStream* storageStream = CompressedDataStream::FetchStorageBacked(c_randLimits[m_selectedDataSet], c_dataSize, c_blockSizeOptions[blockSize], c_inMemoryAlignments[m_selectedDataSetAlignment]);
                if (storageStream)
                {
                    m_fromStorageQueue.ActiveCompressedStream = storageStream;
                }
                else
                {
                    XAsyncBlock* ab = new XAsyncBlock();
                    ab->context = this;
                    ab->callback = [](XAsyncBlock* block) { delete block; };

                    m_fromStorageQueue.TaskStatus = TaskStatus::GeneratingNewDataSet;
                    XAsyncRun(ab, reinterpret_cast<XAsyncWork*>(&HandleFromStorageDataChangeRequestAsync));
                }

            }
        }
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            if (QueueStatus::Running == m_inMemoryQueue.QueueStatus)
            {
                m_inMemoryQueue.QueueStatus = QueueStatus::Paused;
            }
            else if (QueueStatus::Paused == m_inMemoryQueue.QueueStatus)
            {
                m_inMemoryQueue.QueueStatus = QueueStatus::Running;
            }
        }
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            if (QueueStatus::Running == m_fromStorageQueue.QueueStatus)
            {
                m_fromStorageQueue.QueueStatus = QueueStatus::Paused;
            }
            else if (QueueStatus::Paused == m_fromStorageQueue.QueueStatus)
            {
                m_fromStorageQueue.QueueStatus = QueueStatus::Running;
            }
        }

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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    float tabSize = XMVectorGetX(m_regularFont->MeasureString(L"XXXX"));

    if (m_timer.GetFrameCount() > 3)
    {
        CompressedDataStream* currentInMemoryStream = m_inMemoryQueue.ActiveCompressedStream;
        float compressionRatio = 0.0f;

        m_largeFont->DrawString(m_spriteBatch.get(), L"SimpleMDU Sample", pos);
        pos.y += (m_largeFont->GetLineSpacing() * 2);
        pos.x += tabSize;

        switch (m_inMemoryQueue.QueueStatus)
        {
        case QueueStatus::PreInit:
            DisplayLineF(pos, L"In-memory Queue: waiting for initial data");
            break;
        case QueueStatus::Running:
            compressionRatio = float(c_dataSize) / float(currentInMemoryStream->CompressedSize);
            DisplayLineF(pos, L"In-memory Queue: Running (%0.f%% random, %1.2f:1 compression, %dKiB chunks, %d aligned)",
                100.0f*currentInMemoryStream->DataRandPercent,
                compressionRatio,
                currentInMemoryStream->ChunkSize/Ki,
                currentInMemoryStream->Alignment);
            break;
        case QueueStatus::Paused:
            DisplayLineF(pos, L"In-memory Queue: Paused");
            break;
        case QueueStatus::Stopped:
            DisplayLineF(pos, L"In-memory Queue: Terminating");
            break;
        }
        pos.x += tabSize;

        DisplayLineF(pos, L"MDU in: %1.1f MB/s", m_inMemoryQueue.MduInMbps.load());
        DisplayLineF(pos, L"MDU out: %1.1f MB/s", m_inMemoryQueue.MduOutMbps.load());

        switch (m_inMemoryQueue.TaskStatus)
        {
        case TaskStatus::GeneratingNewDataSet:
            DisplayLineF(pos, DirectX::Colors::OrangeRed, L"Please wait, generating data with %0.f%% random", 100.0f*c_randLimits[m_selectedDataSet]);
            break;
        case TaskStatus::CompressingData:
            DisplayLineF(pos, DirectX::Colors::OrangeRed, L"Please wait, compressing in %dKiB chunks, alignment %d", c_blockSizeOptions[m_selectedInMemoryBlockSize] / Ki, c_inMemoryAlignments[m_selectedDataSetAlignment]);
            break;
        case TaskStatus::StoringData:
            DisplayLine(pos, DirectX::Colors::OrangeRed, L"Please wait, persisting data stream for read back");
            break;
        case TaskStatus::InitialState:
        case TaskStatus::Idle:
        default:
            DisplayLine(pos, L"");
            break;
        }

        pos.x -= tabSize;
        DisplayLine(pos, L"");

        CompressedDataStream* currentFromStorageStream = m_fromStorageQueue.ActiveCompressedStream;
        compressionRatio = 0.0f;

        switch (m_fromStorageQueue.QueueStatus)
        {
        case QueueStatus::PreInit:
            DisplayLineF(pos, L"From-storage Queue: waiting for initial data");
            break;
        case QueueStatus::Running:
            compressionRatio = float(c_dataSize) / float(currentFromStorageStream->CompressedSize);
            DisplayLineF(pos, L"From-storage Queue: Running (%0.f%% random, %1.2f:1 compression, %dKiB chunks, %d aligned)",
                100.0f*currentFromStorageStream->DataRandPercent,
                compressionRatio,
                currentFromStorageStream->ChunkSize / Ki,
                currentFromStorageStream->Alignment);
            break;
        case QueueStatus::Paused:
            DisplayLineF(pos, L"From-storage Queue: Paused");
            break;
        case QueueStatus::Stopped:
            DisplayLineF(pos, L"From-storage Queue: Terminating");
            break;
        }
        pos.x += tabSize;

        DisplayLineF(pos, L"MDU in: %1.1f MB/s", m_fromStorageQueue.MduInMbps.load());
        DisplayLineF(pos, L"MDU out: %1.1f MB/s", m_fromStorageQueue.MduOutMbps.load());

        switch (m_fromStorageQueue.TaskStatus)
        {
        case TaskStatus::GeneratingNewDataSet:
            DisplayLineF(pos, DirectX::Colors::OrangeRed, L"Please wait, waiting on data generation function");
            break;
        case TaskStatus::CompressingData:
            DisplayLineF(pos, DirectX::Colors::OrangeRed, L"Please wait, waiting on data generation function");
            break;
        case TaskStatus::StoringData:
            DisplayLine(pos, DirectX::Colors::OrangeRed, L"Please wait, persisting data stream for read back");
            break;
        case TaskStatus::InitialState:
        case TaskStatus::Idle:
        default:
            DisplayLine(pos, L"");
            break;
        }

        pos.x -= tabSize;
        DisplayLine(pos, L"");

        DisplayLineF(pos, L"Total MDU throughput:");
        pos.x += tabSize;
        DisplayLineF(pos, L"MDU in: %1.1f MB/s", m_fromStorageQueue.MduInMbps.load() + m_inMemoryQueue.MduInMbps.load());
        DisplayLineF(pos, L"MDU out: %1.1f MB/s", m_fromStorageQueue.MduOutMbps.load() + m_inMemoryQueue.MduOutMbps.load());
        pos.x -= tabSize;
        DisplayLine(pos, L"");


        XMEM_WORKING_SET_STATISTICS stats = {};
        XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TITLE, &stats);

        DisplayLineF(pos, L"Memory Used: %zu / %zu MiB", stats.GameUsed / (1024 * 1024), stats.GameLimit / (1024 * 1024));
        DisplayLine(pos, L"");

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[DPad] Left\\Right to adjust chunk size", pos, DirectX::Colors::Aqua);
        pos.y += m_regularFont->GetLineSpacing();

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[DPad] Up\\Down to adjust compression ratio", pos, DirectX::Colors::Aqua);
        pos.y += m_regularFont->GetLineSpacing();

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[LB][RB] to adjust the alignment of each compressed chunk", pos, DirectX::Colors::Aqua);
        pos.y += m_regularFont->GetLineSpacing();

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[Y] to enable\\disable in-memory decompression", pos, DirectX::Colors::Aqua);
        pos.y += m_regularFont->GetLineSpacing();

        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[B] to enable\\disable from-storage decompression", pos, DirectX::Colors::Aqua);
        pos.y += m_regularFont->GetLineSpacing();
    }

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}


// Helper for output debug tracing
void Sample::DisplayLineF(XMFLOAT2& pos, _In_z_ _Printf_format_string_ const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);

    wchar_t buff[1024] = {};
    vswprintf_s(buff, format, args);
    DisplayLine(pos, DirectX::Colors::White, buff);
    va_end(args);
}

void Sample::DisplayLineF(XMFLOAT2& pos, FXMVECTOR color, _In_z_ _Printf_format_string_ const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);

    wchar_t buff[1024] = {};
    vswprintf_s(buff, format, args);
    DisplayLine(pos, color, buff);
    va_end(args);
}

void Sample::DisplayLine(XMFLOAT2& pos, const wchar_t *text)
{
    DisplayLine(pos, DirectX::Colors::White, text);
}

void Sample::DisplayLine(XMFLOAT2& pos, FXMVECTOR color, const wchar_t *text)
{
    m_regularFont->DrawString(m_spriteBatch.get(), text, pos, color);
    pos.y += m_regularFont->GetLineSpacing();
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

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    wchar_t strFilePath[MAX_PATH] = {};

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            strFilePath,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
}
#pragma endregion
