//--------------------------------------------------------------------------------------
// InGameStore_Desktop.h
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
#include "UITwist.h"
#include "ListView.h"
#include "UIConstants.h"
#include "SampleGUI.h"

namespace
{
    const std::wstring c_productTypeStrings[] = {
        L"All",
        L"Game",
        L"Durable",
        L"Consumable",
        L"Pass"
    };
}

struct UIProductPrice
{
    float basePrice;
    float price;
    float recurrencePrice;
    std::wstring currencyCode;
    std::wstring formattedBasePrice;
    std::wstring formattedPrice;
    std::wstring formattedRecurrencePrice;
    bool isOnSale;
    time_t saleEndDate;
};

struct UIProductAvailability
{
    std::wstring availabilityId;
    UIProductPrice price;
    time_t endDate;
};

struct UIProductSku
{
    std::wstring skuId;
    std::wstring title;
    std::wstring description;
    UIProductPrice price;
    bool isTrial;
    bool isInUserCollection;
    bool isSubscription;
    XStoreSubscriptionInfo subscriptionInfo;
    uint32_t bundledSkusCount;
    std::vector<std::wstring> bundledSkus;
    uint32_t availabilitiesCount;
    std::vector<UIProductAvailability> availabilities;
    XStoreCollectionData collectionData;


    //  Unused variables from the XStoreProduct we don't need 
    //  for this sample's UI
    //_Field_z_ const char* language;
    //_Field_z_ const char* inAppOfferToken;
    //_Field_z_ char* linkUri;
    //uint32_t keywordsCount;
    //_Field_z_ const char** keywords;
    //uint32_t imagesCount;
    //XStoreImage* images;
    //uint32_t videosCount;
    //XStoreVideo* videos;
};

struct UIProductDetails
{
    std::wstring storeId;
    std::wstring title;
    std::wstring description;
    std::wstring language;
    std::wstring inAppOfferToken;
    std::wstring linkUri;
    XStoreProductKind productKind;
    UIProductPrice price;
    bool hasDigitalDownload;
    bool isInUserCollection;
    uint32_t keywordsCount;
    std::vector<std::wstring> keywords;
    uint32_t skusCount;
    std::vector<UIProductSku> skus;
    /*uint32_t imagesCount;
    XStoreImage* images;
    uint32_t videosCount;
    XStoreVideo* videos;*/
};

class Sample;

class ProductListViewRow
{
public:
    void Show();
    void Hide();
    void SetControls(ATG::IPanel *parent, int rowStart);
    void Update(UIProductDetails item);
    void SetSelectedCallback(ATG::IControl::callback_t callback);
	void SetFocusedCallback(ATG::IControl::callback_t callback);

    static std::weak_ptr<ATG::UIManager> s_ui;
    //static std::weak_ptr<ImageManager>   s_images;
    static Sample*                       s_sample;
private:
    ATG::Button *m_selectBtn;
    ATG::Image  *m_productImage;
    ATG::Image  *m_productOwned;
    ATG::Image  *m_productLicensed;
    std::wstring m_productId;
};

enum UIUpdateState
{
    ProductList = 1,
    ProductDetails = 2
};


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
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
    void GetDefaultSize( int& width, int& height ) const;

    // Sample Functions
    bool ProductIsAvailable(const std::wstring& productId) { return m_catalogDetails.find(productId) != m_catalogDetails.end(); }
    bool ProductIsOwned(const std::wstring &productId) { return m_userEntitlements.find(productId) != m_userEntitlements.end(); }
    bool ProductIsLicensed(const std::wstring& productId) { return m_addOnLicenses.find(productId) != m_addOnLicenses.end(); }
    void CompleteCatalogItemsRefresh(const std::wstring& type);
    UIProductDetails CopyToUIProduct(const XStoreProduct* Product);
    UIProductSku CopyToUISku(XStoreSku Sku);
    UIProductAvailability CopyToUIAvailability(XStoreAvailability Availability);
    UIProductPrice CopyToUIPrice(XStorePrice Price);
    std::wstring ProductKindToString(XStoreProductKind kind);
    void QueryProducts();
    void QueryCollections();
    void MakePurchase();
    void QueryGameLicense();
	void QueryLicenseToken();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Marketplace Object Storage
    XStoreContextHandle m_xStoreContext;


    void TriggerUpdateDetails();
    XStoreProductKind StringToProductKind(const std::wstring& type);

    // UI Methods
    void UpdateProductList();
    void UpdateListView(size_t currentPage, size_t totalProducts);
    void UpdateDetailsView();
    void SetupUI();
    void ShowPopup(const wchar_t* format, ...);

    // UI Objects
    std::shared_ptr<ATG::UIManager>         m_ui;
    std::unique_ptr<UITwist>                m_twist;
    ATG::IControl*                          m_currentButton;
    std::atomic_int                         m_uiUpdate;
    size_t                                  m_listViewPage;
    bool									m_pauseKbInput;

    std::unique_ptr<ListView<UIProductDetails, ProductListViewRow>> m_listView;

    std::mutex                              m_catalogLock;
    std::mutex                              m_detailsLock;
    std::mutex                              m_entitlementsLock;
    std::mutex                              m_bundleLock;

    std::wstring							m_storeId;

    // Master listing of catalog items
    std::map<XStoreProductKind, std::unordered_set<std::wstring>> m_catalog;

    // Mapping of product ids to item details
    std::map<std::wstring, UIProductDetails>  m_catalogDetails;
    std::wstring                              m_currentProduct;

    std::map<std::wstring, UIProductDetails>  m_userEntitlements;
    std::map<std::wstring, UIProductDetails>  m_addOnLicenses;

    XTaskQueueHandle                            m_asyncQueue;

    // --- Live Info HUD Start ---
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
    // --- Live Info HUD End ---

    // --- Live Resources Start ---
    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    // --- Live Resources End ---
    
    // --- Live Info HUD Start ---
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    // --- Live Info HUD End ---

    //tmp for debugging
    std::string m_ExePath;

    enum Descriptors
    {
        Reserve,
        Count = 64,
    };
};
