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

using namespace ATG::UITK;

class Sample;

// XPackageDetails objects returned by XPackageEnumeratePackages don't stick around past the enumeration.
// This struct will keeps the strings around.
struct PackageDetails
{
    PackageDetails() = default;

    PackageDetails(const XPackageDetails* package) :
        packageIdentifier{ package->packageIdentifier },
        version{ package->version },
        kind{ package->kind },
        displayName{ package->displayName },
        description{ package->description },
        publisher{ package->publisher },
        storeId{ package->storeId },
        installing{ package->installing },
        isMounted{ false },
        isLicense { false },
        isBusy { false },
        button{ nullptr }
    {
    }

    std::string packageIdentifier;
    XVersion version;
    XPackageKind kind;
    std::string displayName;
    std::string description;
    std::string publisher;
    std::string storeId;
    bool installing;
    bool isMounted;
    bool isLicense;
    bool isBusy;
    std::shared_ptr<UIButton> button;
};

struct PackageEventContext
{
    Sample *pThis;
    std::string storeId;
};

struct PackageMountInfo
{
    XStoreLicenseHandle packageLicense;
    XTaskQueueRegistrationToken licenseLostEvent;
    XPackageMountHandle mountHandle;
    PackageEventContext *context;
};

struct StoreProductDetails
{
    StoreProductDetails() = default;

    StoreProductDetails(const XStoreProduct* package) :
        storeId{ package->storeId },
        title{ package->title },
        description{ package->description },
        language{ package->language },
        inAppOfferToken{ package->inAppOfferToken ? package->inAppOfferToken : "" },
        linkUri{ package->linkUri },
        productKind{ package->productKind },
        price{ package->price },
        hasDigitalDownload{ package->hasDigitalDownload },
        isInUserCollection{ package->isInUserCollection },
        keywordsCount{ package->keywordsCount },
        keywords{ package->keywords },
        skusCount{ package->skusCount },
        skus{ package->skus },
        imagesCount{ package->imagesCount },
        images{ package->images },
        videosCount{ package->videosCount },
        videos{ package->videos },
        isBusy{ false },
        button{ nullptr }
    {
    }

    std::string storeId;
    std::string title;
    std::string description;
    std::string language;
    std::string inAppOfferToken;
    std::string linkUri;
    XStoreProductKind productKind;
    XStorePrice price;
    bool hasDigitalDownload;
    bool isInUserCollection;
    uint32_t keywordsCount;
    const char** keywords;
    uint32_t skusCount;
    XStoreSku* skus;
    uint32_t imagesCount;
    XStoreImage* images;
    uint32_t videosCount;
    XStoreVideo* videos;
    bool isBusy;
    std::shared_ptr<UIButton> button;
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public D3DResourcesProvider
{
public:
    enum EnumFilterType
    {
        AllDownloadableContent,
        AllRelatedPackages,
        CurrentTitleOnly,
        RelatedTitlesOnly
    };

    enum EnumFocusArea
    {
        None,
        InstalledPackages,
        StoreProducts
    };

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
    void RefreshStoreProducts();
    void PurchaseStoreProduct(StoreProductDetails &package);
    void DownloadStorePackage(StoreProductDetails &package);
    StoreProductDetails* GetStoreProductDetail(const std::string &storeId);
    void StartInstallMonitor(const char* identity, StoreProductDetails &package);

    // DLC Methods
    void RefreshInstalledPackages();
    void MountSelectedPackage(PackageDetails &package);
    void UnmountSelectedPackage(PackageDetails &package);
    XTaskQueueRegistrationToken RegisterPackageEvents(XStoreLicenseHandle license, PackageEventContext *context);
    void UnregisterPackageEvents(XStoreLicenseHandle license, XTaskQueueRegistrationToken licenseLostEvent);
    void AddNewMountedPackage(std::string &storeId, XStoreLicenseHandle license, XTaskQueueRegistrationToken token, XPackageMountHandle mountHandle, PackageEventContext *context);
    PackageMountInfo* GetPackageMountInfo(const std::string &storeId);
    PackageDetails* GetPackageDetail(const std::string &storeId);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void InitializeUI();
    void ErrorMessage(std::string_view format, ...);

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
    std::shared_ptr<ATG::UITK::UIVerticalStack> m_storeList;
    std::shared_ptr<ATG::UITK::UIVerticalStack> m_dlcList;
    std::shared_ptr<ATG::UITK::UITwistMenu>     m_twistMenu;
    std::shared_ptr<ATG::UITK::UIImage>         m_backgroundImage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_errorMessage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_legendText;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    XStoreContextHandle                         m_storeContext;

    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    XTaskQueueHandle                            m_asyncQueue;
    XTaskQueueRegistrationToken                 m_packageInstallToken;

    std::map<std::string, PackageMountInfo>     m_mountedPackageList;
    EnumFilterType                              m_selectedEnumFilter;

    std::vector<PackageDetails>                 m_dlcDetailList;
    std::vector<StoreProductDetails>            m_storeDetailList;
    bool                                        m_isStoreEnumerating;

    std::string                                 m_lastSelectStoreId;
    EnumFocusArea                               m_needSetFocus;
    
    enum Descriptors
    {
        Texture,
        Count = 32,
    };
};
