//--------------------------------------------------------------------------------------
// MemoryPlaceholders.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryPlaceholders.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG;

using Microsoft::WRL::ComPtr;

void Sample::GetRandomAllocationParameters(bool graphics, size_t& size, uint32_t& allocationType, uint64_t& xmemFlags)
{
    static std::random_device randomDevice;
    static std::mt19937_64 randomEngine(randomDevice());
    static std::uniform_int_distribution<size_t> sizeDist(c_minAllocationSize, c_maxAllocationSize);
    static std::uniform_int_distribution<size_t> pageSizeDist(0, 2);

    allocationType = MEM_COMMIT;
    size = sizeDist(randomEngine);
    if (graphics)
    {
        xmemFlags = XMEM_GRAPHICS;
        if (pageSizeDist(randomEngine) == 2)
            allocationType += MEM_2MB_PAGES;
        else
            allocationType += MEM_64K_PAGES;
    }
    else
    {
        xmemFlags = XMEM_CPU;
        switch (pageSizeDist(randomEngine))
        {
        case 1:
            allocationType += MEM_64K_PAGES;
            break;
        case 2:
            allocationType += MEM_2MB_PAGES;
            break;
        }
    }
}

void Sample::AllocateFromTitleReservedRange()
{
    size_t size;
    uint32_t allocationType;
    uint64_t xmemFlags;
    GetRandomAllocationParameters(false, size, allocationType, xmemFlags);
    void* baseAddress = AllocateTitleReserved(nullptr, size, allocationType, xmemFlags, PAGE_READWRITE);
    assert(baseAddress);
    m_titleReservedRangeAllocations.push_back(baseAddress);
}

void Sample::ReleaseToTitleReservedRange()
{
    if (m_titleReservedRangeAllocations.size())
    {
        ReleaseTitleReserved(m_titleReservedRangeAllocations[0]);
        m_titleReservedRangeAllocations.erase(m_titleReservedRangeAllocations.begin());
    }
}

void Sample::AllocateFromPlaceholder(bool graphics)
{
    size_t size;
    uint32_t allocationType;
    uint64_t xmemFlags;
    GetRandomAllocationParameters(graphics, size, allocationType, xmemFlags);
    if (graphics)
    {
        void* baseAddress = m_gpuPlaceholder.Allocate(nullptr, size, allocationType, xmemFlags, PAGE_READWRITE | PAGE_GRAPHICS_READONLY);
        assert(baseAddress);
        m_gpuPlaceholderAllocations.push_back(baseAddress);
    }
    else
    {
        void* baseAddress = m_cpuPlaceholder.Allocate(nullptr, size, allocationType, xmemFlags, PAGE_READWRITE);
        assert(baseAddress);
        m_cpuPlaceholderAllocations.push_back(baseAddress);
    }
}

void Sample::ReleaseToPlaceholder(bool graphics)
{
    if (graphics)
    {
        if (m_gpuPlaceholderAllocations.size())
        {
            m_gpuPlaceholder.Release(m_gpuPlaceholderAllocations[0]);
            m_gpuPlaceholderAllocations.erase(m_gpuPlaceholderAllocations.begin());
        }
    }
    else
    {
        if (m_cpuPlaceholderAllocations.size())
        {
            m_cpuPlaceholder.Release(m_cpuPlaceholderAllocations[0]);
            m_cpuPlaceholderAllocations.erase(m_cpuPlaceholderAllocations.begin());
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_cpuPlaceholder.SetupRegion(false, nullptr, c_defaultCPUPlaceholderSize);
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

    m_gpuPlaceholder.SetupRegion(true, nullptr, c_defaultGPUPlaceholderSize);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsAPressed())
        {
            AllocateFromTitleReservedRange();
        }
        else if (pad.IsBPressed())
        {
            ReleaseToTitleReservedRange();
        }
        else if (pad.IsXPressed())
        {
            AllocateFromPlaceholder(false);
        }
        else if (pad.IsYPressed())
        {
            ReleaseToPlaceholder(false);
        }
        else if (pad.IsLeftShoulderPressed())
        {
            AllocateFromPlaceholder(true);
        }
        else if (pad.IsRightShoulderPressed())
        {
            ReleaseToPlaceholder(true);
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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        m_largeFont->DrawString(m_spriteBatch.get(), L"Memory Placeholders Sample", pos);
        pos.y += (m_largeFont->GetLineSpacing() * 2);

        m_regularFont->DrawString(m_spriteBatch.get(), L"   The 4TB-8TB memory range is reserved for CPU title allocations that want to guarantee a fixed address is available.", pos, ATG::Colors::OffWhite);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[A]", L"allocate in title reserved range", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[B]", L"release in title reserved range", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        {
            wchar_t buffer[128];
            swprintf(buffer, 128, L"       %llu allocation%s in title reserved memory", m_titleReservedRangeAllocations.size(), m_titleReservedRangeAllocations.size() == 1 ? L"" : L"s");
            m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
            pos.y += m_regularFont->GetLineSpacing() * 3.0f;
        }

        m_regularFont->DrawString(m_spriteBatch.get(), L"   Using a placeholder for allocations requires the region to be split or merged as blocks are allocated/released.", pos, ATG::Colors::OffWhite);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[X]", L"allocate from CPU placeholder", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[Y]", L"release back to CPU placeholder", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[LB]", L"allocate from Graphics placeholder", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        DrawStatusString(L"[RB]", L"release back to Graphics placeholder", pos);
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        {
            wchar_t buffer[128];
            swprintf(buffer, 128, L"       %llu allocation%s in CPU placeholder region", m_cpuPlaceholderAllocations.size(), m_cpuPlaceholderAllocations.size() == 1 ? L"" : L"s");
            m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
            swprintf(buffer, 128, L"       %llu allocation%s in Graphics placeholder region", m_gpuPlaceholderAllocations.size(), m_gpuPlaceholderAllocations.size() == 1 ? L"" : L"s");
            pos.y += m_regularFont->GetLineSpacing() * 1.1f;
            m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
        }
    }
    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DrawStatusString(const std::wstring& button, const std::wstring& testName, XMFLOAT2& pos)
{
    std::wstring outputString(L"     Press ");
    outputString += button;
    outputString += L" to ";
    outputString += testName;

    DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), outputString.c_str(), pos);
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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            L"ATGSampleBackground.dds",
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    try
    {
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"SegoeUI_18.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
    }
    catch (std::exception& /*e*/) {}

    try
    {
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"SegoeUI_24.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));
    }
    catch (std::exception& /*e*/) {}

    try
    {
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"XboxOneController.spritefont",
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
    }
    catch (std::exception& /*e*/) {}

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
