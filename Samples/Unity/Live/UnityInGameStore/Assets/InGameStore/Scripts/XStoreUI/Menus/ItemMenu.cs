using System;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// ItemMenu class for product-level controls. Available buttons are based on product type.
    /// </summary>
    public sealed class ItemMenu : MonoBehaviour
    {
        [SerializeField] private RawImage ProductImage;
        [SerializeField] private Text TitleText;
        [SerializeField] private Text ProductDetailsText;
        [SerializeField] private Button ShowProductPageButton;
        [SerializeField] private Button PurchaseButton;
        [SerializeField] private Button PreviewLicenseButton;
        [SerializeField] private Button AcquireLicenseButton;
        [SerializeField] private Button ConsumeButton;
        [SerializeField] private Button DownloadAndInstallButton;
        [SerializeField] private Button CloseButton;
        [SerializeField] private GameObject ConsoleLogPlaceholder;

        public static ItemMenu Instance { get; private set; }

        private string _storeId;
        private bool _isCatalogItem;
        private bool _isConsumable;
        private bool _isDurable;
        private bool _hasDigitalDownload;

        private InputAction _cancelAction;

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;

            _storeId = "";
            _isCatalogItem = false;
            _isConsumable = false;
            _isDurable = false;
            _hasDigitalDownload = false;
            _cancelAction = InputSystem.ListEnabledActions().FindLast(action => action.name == "Cancel");
        }

        /// <summary>
        /// Initializes the menu and determines which buttons should
        /// be interactable based on the type of add-on content displayed.
        /// </summary>
        /// <param name="storeId">StoreId of product to show.</param>
        public void ShowMenu(string storeId)
        {
            _storeId = storeId;
            _isCatalogItem = false;
            _isConsumable = false;
            _isDurable = false;
            _hasDigitalDownload = false;

            // Determine type of add-on content displayed.
            if (XStoreManager.Instance.CatalogProducts.ContainsKey(_storeId))
            {
                _isCatalogItem = true;
            }

            if (XStoreManager.Instance.AllProducts.ContainsKey(_storeId))
            {
                XStoreProductKind productKind = XStoreManager.Instance.AllProducts[_storeId].ProductKind;

                if (productKind == XStoreProductKind.Durable)
                {
                    _isDurable = true;
                    _hasDigitalDownload = XStoreManager.Instance.AllProducts[_storeId].HasDigitalDownload;
                }
                else if(productKind == XStoreProductKind.Consumable || productKind == XStoreProductKind.UnmanagedConsumable)
                {
                    _isConsumable = true;
                }
            }

            SetProductDetails();

            // Determine which product type button to display
            if(_isConsumable)
            {
                DownloadAndInstallButton.gameObject.SetActive(false);
                ConsumeButton.gameObject.SetActive(true);
            }
            else
            {
                DownloadAndInstallButton.gameObject.SetActive(true);
                ConsumeButton.gameObject.SetActive(false);
            }

            // Toggle interactable state based on content type.
            PurchaseButton.interactable = _isCatalogItem;
            ShowProductPageButton.interactable = _isCatalogItem;
            DownloadAndInstallButton.interactable = _hasDigitalDownload;
            ConsumeButton.interactable = !_isDurable;
            PreviewLicenseButton.interactable = _isDurable;
            AcquireLicenseButton.interactable = _isDurable;

            // Set focus to the first interactable button.
            if (_isCatalogItem)
            {
                EventSystem.current.SetSelectedGameObject(PurchaseButton.gameObject);
            }
            else if(_isConsumable)
            {
                EventSystem.current.SetSelectedGameObject(ConsumeButton.gameObject);
            }
            else if (_hasDigitalDownload)
            {
                EventSystem.current.SetSelectedGameObject(DownloadAndInstallButton.gameObject);
            }
            else if (_isDurable)
            {
                EventSystem.current.SetSelectedGameObject(PreviewLicenseButton.gameObject);
            }
            else
            {
                EventSystem.current.SetSelectedGameObject(CloseButton.gameObject);
            }
        }
        private void Start()
        {
            XboxManager.Instance.UserSignInStarted += OnStoreChangeDetected;
            XboxManager.Instance.UserSignedIn += OnStoreChangeDetected;
            XboxManager.Instance.UserSignedOut += OnStoreChangeDetected;
            ProductUIManager.Instance.UIProductsUpdated += () => SetProductDetails();
            XStoreLicenseManager.Instance.DurableLicenseAcquired += SetProductDetails;
            XStoreLicenseManager.Instance.DurableLicenseLost += SetProductDetails;
            XNetworkManager.Instance.NetworkConnectionLost += OnNetworkChangeDetected;

            PurchaseButton.onClick.AddListener(() => PurchaseProduct());
            ShowProductPageButton.onClick.AddListener(() => ShowProductPage());
            PreviewLicenseButton.onClick.AddListener(() => PreviewLicense());
            AcquireLicenseButton.onClick.AddListener(() => AcquireLicense());
            DownloadAndInstallButton.onClick.AddListener(() => DownloadAndInstall());
            ConsumeButton.onClick.AddListener(() => Consume());
            CloseButton.onClick.AddListener(() => Close());

            if (ConsoleLogPlaceholder != null)
            {
                Destroy(ConsoleLogPlaceholder);
            }
        }

        private void Update()
        {
            if (_cancelAction.WasReleasedThisFrame())
            {
                Close();
            }
        }

        /// <summary>
        /// Returns to the Main Menu.
        /// </summary>
        private void OnStoreChangeDetected()
        {
            if (gameObject.activeInHierarchy)
            {
                MenuManager.Instance.ShowMainMenu();
            }
        }

        /// <summary>
        /// Returns to the Product List Menu.
        /// </summary>
        private void Close()
        {
            if (gameObject.activeInHierarchy)
            {
                MenuManager.Instance.ShowProductListMenu();
            }
        }

        /// <summary>
        /// Returns to Main Menu when no network connection is available.
        /// </summary>
        private void OnNetworkChangeDetected()
        {
            if (gameObject.activeInHierarchy && !XNetworkManager.Instance.IsNetworkAvailable)
            {
                MenuManager.Instance.ShowMainMenu();
            }
        }

        /// <summary>
        /// Displays details for the selected product.
        /// </summary>
        private void SetProductDetails(string storeId = null)
        {
            if (!XStoreManager.Instance.IsStoreReady)
            { return; }

            ProductAttributes attributes = ProductUIManager.Instance.UIProducts[_storeId].GetComponentInChildren<ProductAttributes>(true);

            if (attributes != null)
            {
                ProductImage.texture = ProductUIManager.Instance.BoxArtImages[_storeId].GetComponent<RawImage>().texture;

                TitleText.text = attributes.Name.text;

                string detailsText = $"{attributes.StoreId}\n{attributes.ProductKind}\n{attributes.Ownership}\n{attributes.Quantity}";

                if (_isDurable && XStoreLicenseManager.Instance.LicensedDurables.ContainsKey(_storeId))
                {
                    detailsText = $"{detailsText}Licensed";
                }

                ProductDetailsText.text = detailsText;

                PurchaseButton.GetComponentInChildren<Text>().text = $"Purchase {attributes.Price.text}";
            }
        }

        private void PurchaseProduct()
        {
            Logger.Instance.Log($"Calling ShowPurchaseUI for {_storeId}", color: LogColor.Event);
            XStoreShowUI.ShowPurchaseUI(XStoreManager.Instance.StoreContext, _storeId);
        }

        private void ShowProductPage()
        {
            Logger.Instance.Log($"Calling ShowProductPageUI for {_storeId}", color: LogColor.Event);
            XStoreShowUI.ShowProductPageUI(XStoreManager.Instance.StoreContext, _storeId);
        }

        /// <summary>
        /// Calls XStoreReportConsumableFulfillmentAsync to remove quantity from the consumable entitlement in the Microsoft Store.
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xstore/functions/xstorereportconsumablefulfillmentasync
        /// Note: Calling consume from the client is not recommended, call collections.mp.microsoft.com/v8.0/collections/consume from a title service instead.
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/commerce/service-to-service/microsoft-store-apis/xstore-v8-consume
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/commerce/fundamentals/xstore-consumable-based-ecosystems
        /// </summary>
        private void Consume()
        {
            Debug.LogWarning("Client-based consume is for testing purposes only. Plan to consume from a title service instead.");
            Logger.Instance.Log($"Calling Consume for {_storeId}", color: LogColor.Event);
            
            XStoreQueries.QueryConsumableBalance(
                XStoreManager.Instance.StoreContext,
                _storeId,
                (int hr, uint balance) =>
                {
                    uint quantity = 1;

                    if (HR.SUCCEEDED(hr) && balance >= quantity)
                    {
                        Guid trackingId = Guid.NewGuid();

                        SDK.XStoreReportConsumableFulfillmentAsync(
                            XStoreManager.Instance.StoreContext,
                            _storeId,
                            quantity,
                            trackingId,
                            (int hr, XStoreConsumableResult result) =>
                            {
                                if (HR.SUCCEEDED(hr))
                                {
                                    Debug.Log($"Consumed {quantity} of {_storeId}");
                                    Logger.Instance.Log($"Remaining balance for {_storeId}: {result.Quantity}", LogColor.Success);

                                    // Update UI with new balance
                                    if (ProductUIManager.Instance.UIProducts.ContainsKey(_storeId))
                                    {
                                        ProductAttributes attributes = ProductUIManager.Instance.UIProducts[_storeId].GetComponentInChildren<ProductAttributes>();
                                        if(attributes != null)
                                        {
                                            attributes.Quantity = $"Quantity: {result.Quantity}";
                                        }
                                        ProductUIManager.Instance.UIProductsUpdated?.Invoke();
                                    }
                                }
                                else
                                {
                                    Debug.LogWarning($"Failed to consume {quantity} of {_storeId}, {nameof(SDK.XStoreReportConsumableFulfillmentAsync)} returned: 0x{{hr:X8)}}");
                                }
                            });
                    }
                    else if (balance < quantity)
                    {
                        Debug.LogWarning("Insufficient quantity available.");
                    }
                });
        }

        private void DownloadAndInstall()
        {
            Logger.Instance.Log($"Calling DownloadAndInstallPackage for {_storeId}", color: LogColor.Event);
            XStoreDownload.DownloadAndInstallPackage(XStoreManager.Instance.StoreContext, _storeId);

            // See XPackage* APIs for monitoring progress, enumerating packages, and mounting the package.
            // https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xpackage/xpackage_members
        }

        private void PreviewLicense()
        {
            // Calls XStoreCanAcquireLicenseForStoreId for DWOBs, and XStoreCanAcquireLicenseForPackageAsync for DWIBs.
            Logger.Instance.Log($"Calling PreviewLicense for {_storeId}", color: LogColor.Event);
            XStoreLicensing.PreviewLicense(XStoreManager.Instance.StoreContext, _storeId, _hasDigitalDownload);
        }

        /// <summary>
        /// Attempts to acquire a license for the durable product.
        /// </summary>
        private void AcquireLicense()
        {
            // Calls XStoreAcquireLicenseForDurablesAsync for DWOBs, and XStoreAcquireLicenseForPackageAsync for DWIBs.
            Logger.Instance.Log($"Calling AcquireLicense for {_storeId}", color: LogColor.Event);
            XStoreLicenseManager.Instance.AcquireDurableLicense(_storeId, _hasDigitalDownload);
        }
    }
}
