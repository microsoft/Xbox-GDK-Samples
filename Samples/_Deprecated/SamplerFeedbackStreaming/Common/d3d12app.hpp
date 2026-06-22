//--------------------------------------------------------------------------------------
// D3D12App.hpp
//
// A simple D3D12 modern app framework for dev samples.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DevtestPlatform.h"

#include "util.hpp"
#include "UISprite12.h"
#include "D3D12Util.h"

#if UWP_BUILD
#include "uwpapp.h"
#endif

#if defined(_GAMING_XBOX)
typedef D3D12XBOX_FRAME_PIPELINE_TOKEN FRAME_PIPELINE_TOKEN;
#else
typedef UINT64 FRAME_PIPELINE_TOKEN;
#endif

struct ThinWinInit
{
    WCHAR strWindowTitle[128];
    UINT Width;
    UINT Height;
    UINT XPos;
    UINT YPos;
};

struct D3DInitParameters
{
    D3D_DRIVER_TYPE DriverType;
    D3D_FEATURE_LEVEL FeatureLevel;
    DXGI_FORMAT BackBufferFormat;
    DXGI_FORMAT DepthBufferFormat;
    UINT EngineCount;
    UINT MsaaSamples;
    UINT MsaaQuality;
    BOOL CreateUISprite;
#if defined(_GAMING_XBOX)
    D3D12XBOX_CREATE_DEVICE_PARAMETERS CreateDeviceParameters;
    BOOL Multiplane;
    BOOL HDR;
#else
    BOOL CreateDebugDevice;
#endif
    BOOL Fullscreen;
    UINT SwapChainBufferCount;
    UINT MaxRecordingCommandLists;
    UINT RenderTargetHeapSize;
    UINT DepthStencilHeapSize;
    CD3DX12_DESCRIPTOR_RANGE DescRange[16];
    CD3DX12_ROOT_PARAMETER RTSlot[16];
    UINT RootSignatureSlotCount;
    SIZE_T UploadHeapSizeBytes;
    CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[8];
    UINT StaticSamplerCount;
    UINT SwapChainFlags;
};

class SuspendSemaphore
{
private:
    HANDLE m_hSemaphore;
    UINT32 m_EngineCount;

public:
    SuspendSemaphore()
        : m_hSemaphore( NULL ),
        m_EngineCount( 0 )
    { }

    HRESULT Initialize( UINT32 EngineCount = 4 )
    {
        m_EngineCount = EngineCount;
        m_hSemaphore = CreateSemaphoreEx( nullptr, m_EngineCount, m_EngineCount, nullptr, 0, SEMAPHORE_ALL_ACCESS );
        if (m_hSemaphore == NULL)
        {
            return GetLastError();
        }
        return S_OK;
    }

    void Terminate()
    {
        CloseHandle( m_hSemaphore );
        m_hSemaphore = NULL;
        m_EngineCount = 0;
    }

    bool TryEnterDma() { return TryEnter(); }
    void EnterDma() { Enter(1); }
    void LeaveDma() { Leave(1); }
    bool TryEnterCompute() { return TryEnter(); }
    void EnterCompute() { Enter(1); }
    void LeaveCompute() { Leave(1); }

protected:
    friend class D3D12App;
    void Suspend() { Enter(m_EngineCount); }
    void Resume() { Leave(m_EngineCount); }

private:
    bool TryEnter()
    {
        DWORD Result = WaitForSingleObject( m_hSemaphore, 0 );
        return (Result == WAIT_OBJECT_0);
    }
    void Enter( UINT32 Count )
    {
        assert( Count <= m_EngineCount );
        for (UINT32 i = 0; i < Count; ++i)
        {
            WaitForSingleObject( m_hSemaphore, INFINITE );
        }
    }
    void Leave( UINT32 Count )
    {
        assert( Count <= m_EngineCount );
        ReleaseSemaphore( m_hSemaphore, Count, nullptr );
    }
};

// A basic game implementation that creates a D3D12 device and
// provides a game loop
class D3D12App
{
protected:
    D3D12App();

public:

    // Initialization and management

#if UWP_BUILD
    friend ref class ApplicationView;
    void WindowCreated(Windows::UI::Core::CoreWindow^ window = nullptr);
#else
    void WindowCreated();
#endif

    void SuspendRendering();
    void ResumeRendering();
    void WindowClosing();
    virtual void ParseCommandLine(const wchar_t* lpCmdLine);
    HRESULT FrameworkInitialize();
    void WindowResize(FLOAT Width, FLOAT Height)
    {
        m_WindowBoundsWidth = Width;
        m_WindowBoundsHeight = Height;
        m_NeedsResize = true;
    }

    // Basic game loop
    bool Tick();

    void OnMouseDelta(LONG x, LONG y)
    {
        InterlockedAdd(&m_MouseDeltaX, x);
        InterlockedAdd(&m_MouseDeltaY, y);
    }

public:
    HRESULT CreateWithoutAppModel(const char* pAppName);
    HRESULT RunWithoutAppModel();

    void SetTranslatedButtons(bool ButtonsDown, UINT32 KeyButtons, UINT32 StickButtons)
    {
        if (ButtonsDown)
        {
            m_TranslatedKeyButtons |= KeyButtons;
            m_TranslatedStickButtons |= StickButtons;
        }
        else
        {
            m_TranslatedKeyButtons &= ~KeyButtons;
            m_TranslatedStickButtons &= ~StickButtons;
        }
    }

public:
    virtual VOID PreWindowInit( ThinWinInit* pInit ) = 0;
    virtual HRESULT PreD3DInitialize( D3DInitParameters* pD3DInitParams ) = 0;
    virtual HRESULT Initialize() = 0;
    virtual HRESULT Terminate() = 0;
    virtual HRESULT Update( BOOL Resized ) = 0;
    virtual HRESULT Render(FRAME_PIPELINE_TOKEN FrameToken) = 0;

protected:
    virtual void ParseTokenizedCommandLine(int argc, const wchar_t** argv);
    void CloseApp();

    void WaitForDebuggerAttach();
    bool IsButtonPressed( GamepadButtons Button ) const { return (m_PressedButtons & (UINT32)Button) != 0; }
    bool IsButtonDown( GamepadButtons Button ) const;

    UINT32 GetCurrentBackBufferIndex() const { return m_FrameIndex % m_InitParams.SwapChainBufferCount; }
    ID3D12Resource* GetCurrentBackBufferTexture(const UINT plane = 0) const { return m_pRenderTargetTexture[plane][GetCurrentBackBufferIndex()]; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRenderTargetView(const UINT plane = 0) const { return m_RenderTargetView[plane][GetCurrentBackBufferIndex()]; }
    ID3D12GraphicsCommandList* CreateCommandList( bool FrameScopedUsage = false, ID3D12PipelineState* pPSO = nullptr );
    ID3D12GraphicsCommandList* CreateBundle();

    void PrepareBackBufferForRendering(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBufferTexture);
    void PrepareBackBufferForPresent(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBufferTexture);

    HRESULT InitializeD3D();
    void TerminateD3D();
    HRESULT CreateSwapChainAndTargets();
    void SetFullscreenMode(bool Fullscreen);
    bool IsFullscreenMode();
#if !defined(_GAMING_XBOX)
    IUnknown* SelectAdapter();
#endif
    FRAME_PIPELINE_TOKEN Throttle();

    void BlockUntilIdle()
    {
        if (m_CpuFence > 0)
        {
            UINT64 BlockFence = m_CpuFence - 1;
            while (m_pGpuFence->GetCompletedValue() < BlockFence)
            {
                Sleep(0);
            }
        }
    }

    HRESULT PerformPixGpuCapture(const WCHAR* strFileName = nullptr);

#if defined(_XBOX_ONE) && defined(_TITLE)
    bool IsScorpio() const { return m_HardwareConfig.HardwareVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X; }
#endif

protected:
    // Core Application state
#if UWP_BUILD
    Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;
#endif
    FLOAT                                           m_WindowBoundsWidth;
    FLOAT                                           m_WindowBoundsHeight;
    bool                                            m_NeedsResize;

#if DEVTEST_USE_HWND
    static HWND s_hWnd;
    static HANDLE s_plmSuspendComplete;
    static HANDLE s_plmSignalResume;

    HRESULT CreateWin32Window();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

protected:
    // Direct3D Objects
    D3D_FEATURE_LEVEL           m_FeatureLevel;
    ID3D12Device*               m_pd3dDevice;
    ID3D12CommandQueue*         m_pd3dCmdQueue;
    ID3D12CommandAllocator*     m_pCmdAllocator;
    ID3D12CommandAllocator*     m_pFrameCmdAllocator;
    ID3D12CommandAllocator*     m_pFrameCmdAllocatorPool[16];
    UINT64                      m_FrameCmdAllocatorFenceValues[ARRAYSIZE(m_pFrameCmdAllocatorPool)];
    ID3D12Fence*                m_pGpuFence;
    UINT64                      m_CpuFence;
    HANDLE                      m_hFenceWaitEvent;
    DescriptorHeapWrapper       m_RTHeap;
    DescriptorHeapWrapper       m_DSHeap;
    ID3D12RootSignature*        m_pDefaultRootSignature;
    CpuGpuHeap                  m_UploadHeap;

#if !defined(_GAMING_XBOX)
    DXGI_SWAP_CHAIN_DESC1       m_SwapChainDesc;
    IDXGISwapChain1*            m_pSwapChain;
#endif
    ID3D12Resource*             m_pRenderTargetTexture[2][3];
    D3D12_CPU_DESCRIPTOR_HANDLE m_RenderTargetView[2][3];

    D3D12_VIEWPORT              m_Viewport;
    D3D12_RECT                  m_ScissorRect;
    FLOAT                       m_AspectRatio;

    ID3D12Resource*             m_pDepthStencilTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DepthStencilView;

    bool                        m_WaitForDebuggerAttach;
    UINT32                      m_FrameIndex;
    FrameTimer                  m_Timer;
    UINT64                      m_GpuTimestampFrequency;
    bool                        m_TitleRunning;

    ThinWinInit                 m_Init;
    D3DInitParameters           m_InitParams;

    UINT                        m_FrameCmdAllocatorPoolCount;
    UINT                        m_FrameCmdAllocatorPoolIndex;

    UISprite12                  m_Sprite;

    RawGamepadReading           m_LastReading;
    UINT32                      m_PressedButtons;
    FLOAT                       m_ThumbLeftX;
    FLOAT                       m_ThumbLeftY;
    FLOAT                       m_ThumbRightX;
    FLOAT                       m_ThumbRightY;
    UINT32                      m_TranslatedKeyButtons;
    UINT32                      m_TranslatedStickButtons;
    bool                        m_EmulateLeftStickFromKeyboard;
    bool                        m_InvertMouseY;
    volatile LONG               m_MouseDeltaX;
    volatile LONG               m_MouseDeltaY;
#if DEVTEST_USE_GDK_INPUT
    IGameInput*                 m_pGameInput;
#endif

    DeferredReleaseQueue        m_DeferredReleaseQueue;

    D3D12_FEATURE_DATA_D3D12_OPTIONS m_FeatureOptions;
    DXGI_ADAPTER_DESC1          m_SelectedAdapterDesc;

    bool                        m_RenderingPaused;
    bool                        m_ShutdownRequested;
#if defined(_GAMING_XBOX)
    // PLM state
    SuspendSemaphore            m_SuspendSemaphore;
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION m_HardwareConfig;
#endif
    bool                        m_screenshot = false;
    bool                        m_pixCapture = false;
    unsigned                    m_frameCountdown = UINT_MAX;
    const char*                 m_pAppName = "Default";
};

#if XGS_BUILD || PC_BUILD
#define MODERN_MAIN(AppClassName) \
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR lpCmdLine, int nCmdShow) \
{ \
    AppClassName* pApp = new AppClassName(); \
    pApp->ParseCommandLine(lpCmdLine); \
    pApp->CreateWithoutAppModel(#AppClassName); \
    pApp->RunWithoutAppModel(); \
    return 0; \
}
#else
#define MODERN_MAIN(AppClassName) \
    [Platform::MTAThread] \
    int main(Platform::Array<Platform::String^>^) \
{ \
    auto applicationViewSource = ref new ApplicationViewSource(); \
    applicationViewSource->SetApp( new AppClassName() ); \
    CoreApplication::Run(applicationViewSource); \
    return 0; \
}
#endif

