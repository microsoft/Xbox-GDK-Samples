//--------------------------------------------------------------------------------------
// GameHub.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "UITK.h"
#include "FileDownloader.h"

using namespace ATG::UITK;

#define RETURN_BUSY(V) { if (V) { return; } else { V = true; }}

/*
     "9P0P0MRKG6DC", // GameHubRelatedGame DWIB (have MPC relation) - Durable_DWIB_01
     "9P6W73J185BB", // GameHubRelatedGame DWOB (have MPC relation) - Durable_dlc01
     "9P0ZBQ3PLQ6Q", // GameHubRequiredGame DWIB (not have MPC relation) - GameHub_RequiredGame_DWIB
     "9NRX4QL8V3DP", // GameHubRequiredGame DWOB (not have MPC relation) - Durable_dlc01
*/


struct UIProductImage
{
    UIProductImage() = default;
    UIProductImage(XStoreImage& image) :
        width{ image.width },
        height{ image.height },
        uri{ image.uri },
        tag{ image.imagePurposeTag }
    {
    };

    uint32_t width;
    uint32_t height;
    std::string uri;
    std::string tag;
};

// Installed package
struct PackageInfo
{
    PackageInfo() = default;

    PackageInfo(const XPackageDetails* package) :
        packageIdentifier{ package->packageIdentifier },
        displayName{ package->displayName },
        storeId{ package->storeId },
        titleId{ package->titleId },
        licensable{ false },
        hasUpdate{ false },
        isGame{ false },
        button{ nullptr }
    {
    };

    std::string packageIdentifier;
    std::string displayName;
    std::string storeId;
    std::string titleId;
    bool licensable; // from XStoreCanAcquireLicenseResult
    bool hasUpdate; // from XStorePackageUpdate
    bool isGame;

    std::shared_ptr<UIButton> button;
};

// Product on Store
struct ProductInfo
{
    ProductInfo() = default;

    ProductInfo(const XStoreProduct* product) :
        kind{ product->productKind },
        storeId{ product->storeId },
        parentStoreId{ "" },
        title{ product->title },
        hasDigitalDownload{ product->hasDigitalDownload },
        isInUserCollection{ product->isInUserCollection },
        licensable{ false },
        isBusy{ false },
        button{ nullptr }
    {
    };

    XStoreProductKind kind;
    std::string storeId;
    std::string parentStoreId;
    std::string title;
    bool hasDigitalDownload;
    bool isInUserCollection;
    bool licensable; // from XStoreCanAcquireLicenseResult 
    bool isBusy;

    std::shared_ptr<UIButton> button;
    std::vector<UIProductImage> images;
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

    // Packages
    void EnumerateInstalledPackages();
    void CheckInstalledPackagesForUpdates();
    void EnumerateStoreProducts();
    void EnumerateDurables(std::string storeId);
    void EnumerateUpdatedPackages();
    void PurchaseStoreProduct(ProductInfo& product);
    void DownloadUpdatedPackage(PackageInfo& package, bool install);
    void DownloadStorePackage(ProductInfo& product);
    void StartInstallMonitor(const char* identity, ProductInfo& product);
    void CheckForUpdates(std::vector<const char*>& packageIds);
    void CheckLicense(PackageInfo& package);
    void CheckLicense(ProductInfo& product);
    void UninstallPackage(PackageInfo& package);
    ProductInfo* FindProduct(std::string storeId);

    // Persistent local storage
    void WritePLS();

    // UI
    void InitializeUI();
    void RefreshGameProductUI();
    void RefreshDurableProductUI();

    std::shared_ptr<UIConsoleWindow> GetConsole() const
    {
        return m_consoleWindow;
    }

    static ATG::FileDownloader& GetFileDownloader() { return s_fileDownloader; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    XStoreContextHandle                         m_storeContext;
    XNetworkingConnectivityLevelHint            m_networkingConnectivityLevel;

    // Task
    XTaskQueueHandle                            m_asyncQueue;
    XTaskQueueRegistrationToken                 m_packageInstallToken;

    //

    std::map<std::string, PackageInfo>          m_packageList;
    std::map<std::string, ProductInfo>          m_gameList;
    std::map<std::string, ProductInfo>          m_durableList;

    std::string                                 m_selectedStoreId;

    // UI
    std::shared_ptr<UIElement>                  m_gamePanel;
    std::shared_ptr<UIElement>                  m_durablePanel;
    std::shared_ptr<UIConsoleWindow>            m_consoleWindow;
    std::shared_ptr<UIElement>                  m_menuPanel;

    static ATG::FileDownloader                  s_fileDownloader;
    bool                                        m_isStoreEnumerating;
    std::list<std::string>                      m_queryDurableList;

    void UpdateMenuButtonUI();
    void UpdatePackageStatusUI(ProductInfo& product);
    void UpdateDownloadProgressUI(std::shared_ptr<UIButton> button, UINT64 installedBytes, UINT64 totalBytes);
    void DownloadProductImage(const ProductInfo& product, const char* tag, std::shared_ptr<UIImage> imageElement);

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };
};
