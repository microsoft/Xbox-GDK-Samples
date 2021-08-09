//--------------------------------------------------------------------------------------
// SimpleFrontPanel.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleFrontPanel.h"

#include "ATGColors.h"

#include "FileHelpers.h"
#include "OSHelpers.h"

namespace
{
    // For more information, see DirectX Tool Kit's dds.h
    const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

#define DDS_LUMINANCE                   0x00020000  // DDPF_LUMINANCE
#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_SURFACE_FLAGS_TEXTURE       0x00001000 // DDSCAPS_TEXTURE

#pragma pack(push, 1)
    struct DDS_PIXELFORMAT
    {
        uint32_t    size;
        uint32_t    flags;
        uint32_t    fourCC;
        uint32_t    RGBBitCount;
        uint32_t    RBitMask;
        uint32_t    GBitMask;
        uint32_t    BBitMask;
        uint32_t    ABitMask;
    };

    struct DDS_HEADER
    {
        uint32_t        size;
        uint32_t        flags;
        uint32_t        height;
        uint32_t        width;
        uint32_t        pitchOrLinearSize;
        uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
        uint32_t        mipMapCount;
        uint32_t        reserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t        caps;
        uint32_t        caps2;
        uint32_t        caps3;
        uint32_t        caps4;
        uint32_t        reserved2;
    };
#pragma pack(pop)

    const DDS_PIXELFORMAT DDSPF_L8 = { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0,  8, 0xff, 0x00, 0x00, 0x00 };
}

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);

    m_available = XFrontPanelIsSupported();
    if (m_available)
    {
        // Get the screen width and height and allocate a panel buffer
        DX::ThrowIfFailed(XFrontPanelGetScreenDimensions(&m_screenHeight, &m_screenWidth));
        m_panelBuffer = std::unique_ptr<uint8_t>(new uint8_t[m_screenWidth * m_screenHeight]);

        // Fill the panel buffer with a checkerboard pattern
        CheckerboardFillPanelBuffer();

        DX::ThrowIfFailed(XFrontPanelSetLightStates(XFrontPanelLight::None));
    }
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

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if(m_available)
    {
        XFrontPanelButton previousButtons = m_rememberedButtons;
        DX::ThrowIfFailed(XFrontPanelGetButtonStates(&m_rememberedButtons));
        XFrontPanelButton pressedButtons = XFrontPanelButton((previousButtons ^ m_rememberedButtons) & m_rememberedButtons);

        // Use DPAD left and right to toggle between checkerboard and gradient
        if ((pressedButtons & XFrontPanelButton::DPadLeft) != XFrontPanelButton::None ||
            (pressedButtons & XFrontPanelButton::DPadRight) != XFrontPanelButton::None)
        {
            m_checkerboard = !m_checkerboard;
            if (m_checkerboard)
            {
                CheckerboardFillPanelBuffer();
            }
            else
            {
                GradientFillPanelBuffer();
            }
        }

        if ((pressedButtons & XFrontPanelButton::DPadUp) != XFrontPanelButton::None)
        {
            BrightenPanelBuffer();
        }

        if ((pressedButtons & XFrontPanelButton::DPadDown) != XFrontPanelButton::None)
        {
            DimPanelBuffer();
        }

        XFrontPanelButton lightButtons[5] = {XFrontPanelButton::Button1, XFrontPanelButton::Button2, 
            XFrontPanelButton::Button3, XFrontPanelButton::Button4, XFrontPanelButton::Button5};
        for (unsigned index = 0; index < 5; ++index)
        {
            if ((pressedButtons & lightButtons[index]) != XFrontPanelButton::None)
            {
                ToggleButtonLight(lightButtons[index]);
            }
        }

        if ((pressedButtons & XFrontPanelButton::DPadSelect) != XFrontPanelButton::None)
        {
            CaptureFrontPanelScreen(L"D:\\FrontPanelScreen.dds");
#ifdef _DEBUG
            OutputDebugStringA("Screenshot of front panel display written to development drive.\n");
#endif
        }

        PresentFrontPanel();
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

    auto output = m_deviceResources->GetOutputSize();

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);
    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), output);
    m_batch->End();

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

    // No clears needed as sample only draws a sprite background

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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload,
            m_available ? L"FrontPanelPresent.png" : L"NoFrontPanel.png",
            m_background.ReleaseAndGetAddressOf())
    );

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}
#pragma endregion

void Sample::CheckerboardFillPanelBuffer()
{
    uint8_t *data = m_panelBuffer.get();
    for (unsigned i = 0; i < m_screenWidth; ++i)
    {
        for (unsigned j = 0; j < m_screenHeight; ++j)
        {
            uint8_t color = ((i / 16 & 1) ^ (j / 16 & 1)) * 0xFF;
            data[i + j * m_screenWidth] = color;
        }
    }
    m_dirty = true;
    m_checkerboard = true;
}

void Sample::GradientFillPanelBuffer()
{
    uint8_t *data = m_panelBuffer.get();
    uint8_t colorBand = uint8_t(m_screenWidth / 16);
    for (unsigned i = 0; i < m_screenWidth; ++i)
    {
        for (unsigned j = 0; j < m_screenHeight; ++j)
        {
            uint8_t color = uint8_t((i / colorBand) << 4);
            data[i + j * m_screenWidth] = color;
        }
    }
    m_dirty = true;
    m_checkerboard = false;
}

void Sample::DimPanelBuffer()
{
    uint8_t *data = m_panelBuffer.get();
    for (unsigned i = 0; i < m_screenWidth; ++i)
    {
        for (unsigned j = 0; j < m_screenHeight; ++j)
        {
            uint8_t color = data[i + j * m_screenWidth];
            if (color >= 0x10)
            {
                color -= 0x10;
                data[i + j * m_screenWidth] = color;
            }
        }
    }
    m_dirty = true;
}

void Sample::BrightenPanelBuffer()
{
    uint8_t *data = m_panelBuffer.get();
    for (unsigned i = 0; i < m_screenWidth; ++i)
    {
        for (unsigned j = 0; j < m_screenHeight; ++j)
        {
            uint8_t color = data[i + j * m_screenWidth];
            if (color < 0xF0)
            {
                color += 0x10;
                data[i + j * m_screenWidth] = color;
            }
        }
    }
    m_dirty = true;
}

void Sample::ToggleButtonLight(XFrontPanelButton whichButton)
{
    XFrontPanelLight lights = XFrontPanelLight::None;
    DX::ThrowIfFailed(XFrontPanelGetLightStates(&lights));
    lights = lights ^ static_cast<XFrontPanelLight>(whichButton);
    DX::ThrowIfFailed(XFrontPanelSetLightStates(lights));
}

void Sample::PresentFrontPanel()
{
    // It is only necessary to present to the front panel when pixels have changed.
    if (m_dirty)
    {
        DX::ThrowIfFailed(XFrontPanelPresentBuffer(m_screenWidth * m_screenHeight, m_panelBuffer.get()));
        m_dirty = false;
    }
}

void Sample::CaptureFrontPanelScreen(const wchar_t * fileName)
{
    if (!fileName)
    {
        throw std::invalid_argument("Invalid filename");
    }

    // Create file
    DX::ScopedHandle hFile(DX::safe_handle(CreateFile2(fileName, GENERIC_WRITE | DELETE, 0, CREATE_ALWAYS, nullptr)));

    if (!hFile)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    DX::auto_delete_file delonfail(hFile.get());

    // Setup header
    const size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER);
    uint8_t fileHeader[HEADER_SIZE] = {};

    *reinterpret_cast<uint32_t*>(&fileHeader[0]) = DDS_MAGIC;

    auto header = reinterpret_cast<DDS_HEADER*>(&fileHeader[0] + sizeof(uint32_t));
    header->size = sizeof(DDS_HEADER);
    header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    header->height = m_screenHeight;
    header->width = m_screenWidth;
    header->mipMapCount = 1;
    header->caps = DDS_SURFACE_FLAGS_TEXTURE;
    memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_L8, sizeof(DDS_PIXELFORMAT));

    UINT rowPitch = m_screenWidth;
    UINT slicePitch = m_screenWidth * m_screenHeight;

    header->flags |= DDS_HEADER_FLAGS_PITCH;
    header->pitchOrLinearSize = static_cast<uint32_t>(rowPitch);

    // Write header & pixels
    DWORD bytesWritten = 0;
    if (!WriteFile(hFile.get(), fileHeader, static_cast<DWORD>(HEADER_SIZE), &bytesWritten, nullptr))
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    if (bytesWritten != HEADER_SIZE)
    {
        throw std::exception("WriteFile");
    }

    if (!WriteFile(hFile.get(), m_panelBuffer.get(), static_cast<DWORD>(slicePitch), &bytesWritten, nullptr))
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    if (bytesWritten != slicePitch)
    {
        throw std::exception("WriteFile");
    }

    delonfail.clear();
}
