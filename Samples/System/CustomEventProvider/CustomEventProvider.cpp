//--------------------------------------------------------------------------------------
// CustomEventProvider.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CustomEventProvider.h"
#include "EtwScopedEvent.h"

#include "ATGColors.h"

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Need to define the sequence number used to generate unique event IDs.
uint32_t EtwScopedEvent::g_Sequence = 0;

Sample::Sample() noexcept(false) :
   m_frame(0),
   m_stressThread(nullptr),
   m_threadCount(0), 
   m_quitStress(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

Sample::~Sample()
{
   m_quitStress.store(true);
   if (m_stressThread)
   {
      ::WaitForSingleObject(m_stressThread, INFINITE);
      ::CloseHandle(m_stressThread);
      m_stressThread = nullptr;
   }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    //  Register the CEP_Main event provider
    EventRegisterCEP_Main();

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    //  Lock us to Core 0
    SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1 << 0));

    //  Create a work thread and lock to Core 1
    m_stressThread = CreateThread(nullptr, 0, Sample::StressThreadEntry, this, 0, nullptr);
    DX::ThrowIfFailed((m_stressThread == nullptr) ? HRESULT_FROM_WIN32(GetLastError()) : S_OK);
    SetThreadAffinityMask(m_stressThread, DWORD_PTR(1 << 1));
    
    //  Spin until stress thread activates
    while (m_threadCount < 1)
    {
        Sleep(10);
    }
}

//--------------------------------------------------------------------------------------
// Name: StressThread
// Desc: Code we can run on another core to simulate doing some real work.
//--------------------------------------------------------------------------------------

DWORD WINAPI Sample::StressThreadEntry( LPVOID lpParam )
{
    SetThreadDescription(GetCurrentThread(), L"StressThread");

    assert( lpParam != nullptr );
    ( (Sample*)lpParam )->StressThread();
    return 0;
}

void Sample::StressThread()
{
    m_threadCount.fetch_add(1);

    while( !m_quitStress )
    {
        {
            ETWScopedEvent( L"StressSleep" );
            Sleep( 1 );
        }

        Load1();
    }

    m_threadCount.fetch_sub(1);
}

void Sample::Load1()
{
    ETWScopedEvent( L"Load1" );

    volatile uint64_t sum = 0ull;

    for ( uint64_t i = 0ull; i < 1000000ull; ++i )
    {
        sum = sum + 1ull;
    }

    Load2();
}

void Sample::Load2()
{
    ETWScopedEvent( L"Load2" );

    volatile uint64_t sum = 0ull;

    for ( uint64_t i = 0ull; i < 1000000ull; ++i )
    {
        sum = sum + 1ull;
    }
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

    ETWScopedEvent(L"Update");
    EventWriteMark(L"Sample::Update");

    Child1();

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
}
#pragma endregion

//--------------------------------------------------------------------------------------
// Name: Child1()
// Desc: Simulated CPU load.
//--------------------------------------------------------------------------------------

void Sample::Child1()
{
    ETWScopedEvent(L"Child1");

    Sleep(1);

    Child2();

    Sleep(2);
}


//--------------------------------------------------------------------------------------
// Name: Child2()
// Desc: Simulated nested CPU load.
//--------------------------------------------------------------------------------------

void Sample::Child2()
{
    ETWScopedEvent(L"Child2");

    Sleep(3);
}

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    ETWScopedEvent(L"Render");
    EventWriteMark(L"Sample::Render");

    Child1();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

	m_effect->Apply(commandList);

    m_batch->Begin(commandList);

    VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), Colors::Yellow);
    VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), Colors::Yellow);
    VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), Colors::Yellow);

    m_batch->DrawTriangle(v1, v2, v3);

    m_batch->End();

    PIXEndEvent(commandList);

    //  Synthesize some data
    auto syntheticData = static_cast<uint32_t>(rand());
    EventWriteBlockCulled(syntheticData);

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

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

	RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
		m_deviceResources->GetDepthBufferFormat());

	EffectPipelineStateDescription pd(
		&VertexPositionColor::InputLayout,
		CommonStates::Opaque,
		CommonStates::DepthNone,
		CommonStates::CullNone,
		rtState);

	m_effect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);

	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    
}
#pragma endregion
