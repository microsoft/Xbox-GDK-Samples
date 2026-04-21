//--------------------------------------------------------------------------------------
// DownloadableContent.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"

#include "UIInputState.h"
#include "UIWidgets.h"

// files needed to initialize the renderer
#include "UIStyleRendererD3D.h"

#include "TrackedPackage.h"

using namespace ATG::UITK;

class Sample;

extern std::unique_ptr<Sample> g_sample;

// for enumerating packages.
struct PackageInfo
{
    PackageInfo() = default;

    PackageInfo(const XPackageDetails* package) :
        packageIdentifier{ package->packageIdentifier },
        displayName{ package->displayName },
        storeId{ package->storeId },
        button{ nullptr }
    {
    };

    std::string                 packageIdentifier;
    std::string                 displayName;
    std::string                 storeId;

    std::shared_ptr<UIButton>   button;
};

// for querying products from the store.
struct ProductInfo
{
    ProductInfo() : isInUserCollection{ false }, isBusy{ false } {};

    ProductInfo(const XStoreProduct* product) :
        storeId{ product->storeId },
        title{ product->title },
        isInUserCollection{ product->isInUserCollection },
        isBusy{ false },
        button{ nullptr }
    {
    };

    std::string                 storeId;
    std::string                 title;
    bool                        isInUserCollection;
    bool                        isBusy;

    std::shared_ptr<UIButton>   button;
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public D3DResourcesProvider
{
public:
    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    // Store Methods
    void CreateStoreContext();
    void RefreshStoreProducts();
    void PurchaseStoreProduct(ProductInfo& product);
    void DownloadStorePackage(ProductInfo& product);
    void StartInstallMonitor(const char* identity, ProductInfo& product);

    // Package Methods
    void RefreshInstalledPackages();
    void MountPackage(TrackedPackage& package);
    void UnmountTrackedPackages();

    // UI Methods
    std::shared_ptr<ATG::UITK::UIConsoleWindow> GetConsole() const
    {
        return (m_console) ?
            m_console->GetTypedSubElementById<ATG::UITK::UIConsoleWindow>(ATG::UITK::ID("ConsoleWindow")) :
            nullptr;
    }

    std::shared_ptr<ATG::UITK::UIStaticText> GetErrorMessageUI() const
    {
        return (m_errorMessage) ? m_errorMessage : nullptr;
    }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void InitializeUI();
    void ResetStoreButton(std::shared_ptr<UIButton> button);
    void UpdatePackageStatus(std::shared_ptr<UIButton> button, bool isMounted, bool isLicensed);
    void UpdateDownloadProgress(std::shared_ptr<UIButton> button, UINT64 installedBytes, UINT64 totalBytes);
    void SetBackgroundImage(const char* filename);
    void ExecuteDLLFunction(const char* filename);

    // UI Event Handlers for DLC buttons
    void OnDlcButtonPressed(UIButton* button);
    void OnDlcButtonFocused(UIButton* button);
    void OnDlcButtonHovered(UIButton* button);

    // UI Event Handlers for Store buttons
    void OnStoreButtonPressed(UIButton* button);
    void OnStoreButtonFocused(UIButton* button);
    void OnStoreButtonHovered(UIButton* button);

    // UIStyleManager::D3DResourcesProvider interface methods
    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // UITK members
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_uiInputState;
    std::shared_ptr<ATG::UITK::UIVerticalStack> m_uiProductList;
    std::shared_ptr<ATG::UITK::UIVerticalStack> m_uiPackageList;
    std::shared_ptr<ATG::UITK::UITwistMenu>     m_twistMenu;
    std::shared_ptr<ATG::UITK::UIImage>         m_backgroundImage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_errorMessage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_legendText;
    std::shared_ptr<ATG::UITK::UIPanel>         m_console;
    bool                                        m_showConsole;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    XStoreContextHandle                         m_storeContext;

    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    XTaskQueueHandle                            m_asyncQueue;
    XTaskQueueRegistrationToken                 m_packageInstallToken;

    std::map<std::string, PackageInfo>          m_packageList;
    std::map<std::string, TrackedPackage>       m_trackedPackageList;
    std::map<std::string, ProductInfo>          m_productList;

    std::atomic<bool>                           m_isStoreEnumerating;
    std::string                                 m_currentPackageStoreId;

    enum Descriptors
    {
        Texture,
        Count = 32,
    };
};

namespace
{
    template <size_t bufferSize = 2048>
    void debugPrint(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        char buffer[bufferSize] = "";

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);

        if (g_sample)
        {
            auto console = g_sample->GetConsole();
            if (console)
            {
                console->AppendLineOfText(buffer);
            }
        }
    }

    template <size_t bufferSize = 2048>
    void ErrorMessage(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        char buffer[bufferSize];

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        if (g_sample)
        {
            auto errorUI = g_sample->GetErrorMessageUI();
            if (errorUI)
            {
                errorUI->SetDisplayText(UIDisplayString(buffer));
            }
        }

        debugPrint(buffer);
    }
}

