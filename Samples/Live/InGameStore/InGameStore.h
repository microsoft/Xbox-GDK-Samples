//--------------------------------------------------------------------------------------
// InGameStore.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "LiveInfoHUD.h"
#include "LiveResources.h"
#include "SampleGUI.h"
#include "StringUtil.h"
#include "FileDownloader.h"


#include "UIManager.h"
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif

namespace
{
    const std::map<XStoreProductKind, std::string> c_kindMap =
    {
        { XStoreProductKind::Game, "Game"},
        { XStoreProductKind::Durable, "Durable" },
        { XStoreProductKind::Consumable, "Consumable" },
        { XStoreProductKind::UnmanagedConsumable, "Consumable" },
        { XStoreProductKind::Pass, "Pass" },
        { XStoreProductKind::None, "None" }
    };

    const std::map<std::string, XStoreProductKind> c_assocTypeMap =
    {
        { "ALL", XStoreProductKind::Game | XStoreProductKind::Durable | XStoreProductKind::Consumable },
        { "DLC", XStoreProductKind::Durable},
        { "DURABLES", XStoreProductKind::Durable},
        { "CONSUMABLES", XStoreProductKind::Consumable }
    };

    std::string ProductKindToString(XStoreProductKind& kind) 
    {
        return c_kindMap.at(kind);
    }
}

struct UIProductPrice
{
    float basePrice;
    float price;
    float recurrencePrice;
    std::string currencyCode;
    std::string formattedBasePrice;
    std::string formattedPrice;
    std::string formattedRecurrencePrice;
    bool isOnSale;
    time_t saleEndDate;
};

struct UIProductAvailability
{
    std::string availabilityId;
    UIProductPrice price;
    time_t endDate;
};

struct UIProductSku
{
    std::string skuId;
    std::string title;
    std::string description;
    UIProductPrice price;
    bool isTrial;

    std::vector<std::string> bundledSkus;
    std::vector<UIProductAvailability> availabilities;

    bool isInUserCollection;
    uint32_t quantity;
};

struct UIProductImage
{
    uint32_t width;
    uint32_t height;
    std::string uri;
    std::string tag;
};

struct UIProductDetails
{
    std::string storeId;
    std::string title;
    std::string description;
    std::string language;
    std::string inAppOfferToken;
    std::string linkUri;
    XStoreProductKind productKind;
    UIProductPrice price;
    bool hasDigitalDownload;
    bool isInUserCollection;
    uint32_t aggregateQuantity;
    uint32_t keywordsCount;
    std::vector<std::string> keywords;
    uint32_t skusCount;
    std::vector<UIProductSku> skus;
    uint32_t imagesCount;
    std::vector<UIProductImage> images;
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnConstrained();
    void OnUnconstrained();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    // Core XStore scenarios
    void InitStore();

    void QueryGameLicense();
    void QueryAddonLicenses();

    void QueryLicenseToken();

    void QueryGameProduct();
    void QueryNextPage(XAsyncBlock *async);
    void QueryCatalog();
    void QueryCollections();

    void ShowAssociatedProducts();
    void ShowProductPage(const char* storeId);

    void MakePurchase(const char* storeId);
    void Download(const char* storeId);
    void PreviewLicense(const char* storeId);
    void AcquireLicense(const char* storeId);

    // Internal product management
    void AddOrUpdateProductToCatalog(XStoreProductKind kind, UIProductDetails& product);
    void UpdateProductList();

    // UI Methods
    std::shared_ptr<ATG::UITK::UIConsoleWindow> GetConsole() const
    {
        return (m_console) ?
            m_console->GetTypedSubElementById<ATG::UITK::UIConsoleWindow>(ATG::UITK::ID("ConsoleWindow")) :
            nullptr;
    }

    std::shared_ptr<ATG::UITK::UIPanel> GetPopup() const
    {
        return m_uiManager.FindTypedById<ATG::UITK::UIPanel>(ATG::UITK::ID("OkPopup"));
    }

    void DownloadProductImage(const UIProductDetails& product, const char* tag, std::shared_ptr<ATG::UITK::UIImage> image);

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();                                                   

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // UIStyleManager::D3DResourcesProvider interface methods

    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }

    // Marketplace Object Storage                                   
    XStoreContextHandle                                             m_xStoreContext;
                                                                    
    // Device resources.                                            
    std::unique_ptr<DX::DeviceResources>                            m_deviceResources;

    // Rendering loop timer.                                        
    uint64_t m_frame;                                               
    DX::StepTimer m_timer;                                          
                                                                    
    XTaskQueueHandle m_asyncQueue;                                  
    XTaskQueueHandle m_imageQueue;

    // Input device.                                                
    std::unique_ptr<DirectX::GamePad>                               m_gamePad;
    std::unique_ptr<DirectX::GamePad::ButtonStateTracker>           m_gamePadButtons;
    std::unique_ptr<DirectX::Keyboard>                              m_keyboard;
    std::unique_ptr<DirectX::Keyboard::KeyboardStateTracker>        m_keyboardButtons;
    std::unique_ptr<DirectX::Mouse>                                 m_mouse;
    std::unique_ptr<DirectX::Mouse::ButtonStateTracker>             m_mouseButtons;

    // DirectXTK objects.                                           
    std::unique_ptr<DirectX::GraphicsMemory>                        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>                        m_resourceDescriptors;
                                                                    
    // Xbox Live objects.                                           
    std::shared_ptr<ATG::LiveResources>                             m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>                               m_liveInfoHUD;

    // UITK members
    ATG::UITK::UIManager                                            m_uiManager;
    ATG::UITK::UIInputState                                         m_uiInputState;
    std::shared_ptr<ATG::UITK::UIStackPanel>                        m_itemMenu;
    bool                                                            m_closeMenu;
    std::shared_ptr<ATG::UITK::UIPanel>                             m_console;
    bool                                                            m_showConsole;

    // UI methods
    void SetupUI();
    void ShowItemMenu(UIProductDetails item, long);
    void ShowGlobalMenu();
    void CloseMenu(bool doUpdate = false);

    std::mutex                                                      m_catalogLock;

    std::string                                                     m_baseGameStoreId;
    std::string                                                     m_selectedStoreId;

    // Master listing of catalog items
    std::map<XStoreProductKind, std::unordered_set<std::string>>    m_catalog;
    
    // Mapping of product ids to item details
    std::map<std::string, UIProductDetails>                         m_catalogDetails;
    
    enum Descriptors
    {
        Font,
        Reserve,
        Count = 64,
    };

    void SetBaseGameStoreId(char* storeId)
    {
        // Truncate any sku or availability id
        m_baseGameStoreId.assign(storeId, 12);
    }

public:
    static ATG::FileDownloader& GetFileDownloader() { return s_fileDownloader; }

private:
    static ATG::FileDownloader                                      s_fileDownloader;
};
