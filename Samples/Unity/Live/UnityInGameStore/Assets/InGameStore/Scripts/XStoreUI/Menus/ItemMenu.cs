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
        /// Intializes the menu and determines which buttons should
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
            XStoreManager.Instance.DurableLicenseLost += SetProductDetails;
            ProductUIManager.Instance.UIProductsUpdated += SetProductDetails;

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
        /// Displays details for the selected product.
        /// </summary>
        private void SetProductDetails()
        {
            if (!XStoreManager.Instance.IsStoreReady)
            { return; }

            ProductAttributes attributes = ProductUIManager.Instance.UIProducts[_storeId].GetComponentInChildren<ProductAttributes>(true);

            if (attributes != null)
            {
                ProductImage.texture = ProductUIManager.Instance.BoxArtImages[_storeId].GetComponent<RawImage>().texture;

                TitleText.text = attributes.Name.text;

                string detailsText = $"{attributes.StoreId}\n{attributes.ProductKind}\n{attributes.Ownership}\n{attributes.Quantity}";

                if (_isDurable && XStoreManager.Instance.LicensedDurables.ContainsKey(_storeId))
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
        /// Calls XStoreReportConsumableFulfillmentAsync to remove quanity from the consumable entitlement in the Microsoft Store.
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xstore/functions/xstorereportconsumablefulfillmentasync
        /// Note: Calling consume from the client is not recommended, call collections.microsoft.com's consume API from a title service instead.
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
                        Debug.LogWarning("Insufficent quantity available.");
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
            Logger.Instance.Log($"Calling PreviewLicense for {_storeId}", color: LogColor.Event);
            XStoreLicensing.PreviewLicense(XStoreManager.Instance.StoreContext, _storeId, _hasDigitalDownload);
        }

        /// <summary>
        /// Acquires a license for the durable product. If the license is valid, it will register the license
        /// for LicenseLost events.
        /// </summary>
        private void AcquireLicense()
        {
            Logger.Instance.Log($"Calling AcquireLicense for {_storeId}", color: LogColor.Event);

            XStoreLicensing.AcquireLicense(
                XStoreManager.Instance.StoreContext,
                _storeId,
                _hasDigitalDownload,
                (int hr, string storeId, XStoreLicense license) =>
                {
                    if (HR.SUCCEEDED(hr) && license != null)
                    {
                        // Check to see if the license is valid.
                        bool isValid = SDK.XStoreIsLicenseValid(license);
                        string licenseState = $"License for {storeId} is valid: {isValid}";
                        if (isValid)
                        {
                            Logger.Instance.Log(licenseState, LogColor.Success);

                            if (!XStoreManager.Instance.LicensedDurables.ContainsKey(storeId))
                            {
                                Logger.Instance.Log($"Registering durable {storeId} for license lost events.", LogColor.System);

                                // Register the new license for LicenseLost events.
                                // Note: XStoreRegisterPackageLicenseLost works for durables with and without a package.
                                // https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreregisterpackagelicenselost
                                hr = SDK.XStoreRegisterPackageLicenseLost(
                                    license,
                                    (IntPtr context) =>
                                    {
                                        Logger.Instance.Log($"License lost for durable {storeId}.", LogColor.System);

                                        // Keep track of which license has been lost so we can unregister and close the handle.
                                        XStoreManager.Instance.LostLicensesQueue.Enqueue(storeId);
                                    },
                                    out PackageLicenseLostCallbackToken token);

                                if (HR.SUCCEEDED(hr))
                                {
                                    // Keep track of registered license and token.
                                    // This info is required for unregistering from LicenseLost events later.
                                    XStoreManager.Instance.LicensedDurables.Add(storeId, new Tuple<XStoreLicense, PackageLicenseLostCallbackToken>(license, token));

                                    // Update UI with new 'Licensed' state.
                                    SetProductDetails();
                                }
                                else
                                {
                                    SDK.XStoreCloseLicenseHandle(license);
                                    Debug.Log($"{nameof(SDK.XStoreRegisterPackageLicenseLost)} for {storeId} returned: 0x{hr:X8}");
                                }
                            }
                        }
                        else
                        {
                            Debug.LogWarning(licenseState);
                            SDK.XStoreCloseLicenseHandle(license);
                        }                        
                    }
                    else if (license != null)
                    {
                        SDK.XStoreCloseLicenseHandle(license);
                    }
                });
        }
    }
}
