using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    public sealed class MainMenu : MonoBehaviour
    {
        [SerializeField] private Button InGameStoreButton;
        [SerializeField] private Button ShowGameButton;
        [SerializeField] private Button PurchaseGameButton;
        [SerializeField] private Button ShowAddonsButton;
        [SerializeField] private Button ShowRateAndReviewButton;
        [SerializeField] private Button RedeemTokenButton;
        [SerializeField] private Button QueryCatalogButton;
        [SerializeField] private Button QueryCollectionsButton;
        [SerializeField] private Button QueryGameLicenseButton;
        [SerializeField] private Button QueryAddonLicensesButton;
        [SerializeField] private Button SignInButton;

        public static MainMenu Instance { get; private set; }

        private GameObject _previousSelected;

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
            _previousSelected = null;
        }

        /// <summary>
        /// Initializes the Main Menu. If returning to this menu from another location,
        /// it will set focus to the previously selected button.
        /// </summary>
        public void ShowMenu()
        {
            if (_previousSelected != null)
            {
                EventSystem.current.SetSelectedGameObject(_previousSelected);
            }
            else
            {
                EventSystem.current.SetSelectedGameObject(InGameStoreButton.gameObject);
                _previousSelected = InGameStoreButton.gameObject;
            }
        }

        // Start is called before the first frame update
        void Start()
        {
            XboxManager.Instance.UserSignInStarted += OnStoreChangeDetected;
            XboxManager.Instance.UserSignedIn += OnStoreChangeDetected;
            XboxManager.Instance.UserSignedOut += OnStoreChangeDetected;
            XStoreManager.Instance.StoreInitializationSucceeded += OnStoreChangeDetected;
            XStoreManager.Instance.StoreInitializationFailed += OnStoreInitializationFailed;

            InGameStoreButton.onClick.AddListener(() => ShowInGameStore());
            ShowGameButton.onClick.AddListener(() => ShowGamePage());
            PurchaseGameButton.onClick.AddListener(() => PurchaseGame());
            ShowAddonsButton.onClick.AddListener(() => ShowAddonsPage());
            ShowRateAndReviewButton.onClick.AddListener(() => ShowRateAndReviewPage());
            RedeemTokenButton.onClick.AddListener(() => ShowRedeemTokenPage());
            QueryCatalogButton.onClick.AddListener(() => QueryCatalog());
            QueryCollectionsButton.onClick.AddListener(() => QueryCollections());
            QueryGameLicenseButton.onClick.AddListener(() => QueryGameLicense());
            QueryAddonLicensesButton.onClick.AddListener(() => QueryAddonLicenses());

            InGameStoreButton.interactable = false;
            ShowGameButton.interactable = false;
            PurchaseGameButton.interactable = false;
            ShowAddonsButton.interactable = false;
            ShowRateAndReviewButton.interactable = false;
            RedeemTokenButton.interactable = false;
            QueryCatalogButton.interactable = false;
            QueryCollectionsButton.interactable = false;
            QueryGameLicenseButton.interactable = false;
            QueryAddonLicensesButton.interactable = false;
            SignInButton.interactable = false;

            ShowMenu();
        }

        private void Update()
        {
            GameObject selected = EventSystem.current.currentSelectedGameObject;
            if (selected != null && _previousSelected != selected && selected.activeInHierarchy)
            {
                _previousSelected = selected;
            }
        }

        /// <summary>
        /// Toggles button interactable state based on if the store is fully initialized.
        /// Whenever the active user is signed-out or a new user is signed-in, the store will be
        /// re-initialized and all buttons disabled. 
        /// When store initialization is complete, store-related butons are re-enabled.
        /// </summary>
        private void OnStoreChangeDetected()
        {
            bool interactable = XStoreManager.Instance.IsStoreReady;

            InGameStoreButton.interactable = interactable;
            ShowGameButton.interactable = interactable;
            PurchaseGameButton.interactable = interactable;
            ShowAddonsButton.interactable = interactable;
            ShowRateAndReviewButton.interactable = interactable;
            RedeemTokenButton.interactable = interactable;
            QueryCatalogButton.interactable = interactable;
            QueryCollectionsButton.interactable = interactable;
            QueryGameLicenseButton.interactable = interactable;
            QueryAddonLicensesButton.interactable = interactable;
            SignInButton.interactable = interactable;

            if (!interactable && gameObject.activeInHierarchy)
            {
                EventSystem.current.SetSelectedGameObject(InGameStoreButton.gameObject);
            }
        }

        /// <summary>
        /// In the rare case that the store fails to initialize,
        /// grant user access to system buttons.
        /// </summary>
        private void OnStoreInitializationFailed()
        {
            SignInButton.interactable = true;

            if (gameObject.activeInHierarchy)
            {
                EventSystem.current.SetSelectedGameObject(SignInButton.gameObject);
            }
        }

        private void ShowGamePage()
        {
            Logger.Instance.Log($"Calling ShowProductPageUI for {XStoreManager.Instance.BaseGame}", color: LogColor.Event);
            XStoreShowUI.ShowProductPageUI(XStoreManager.Instance.StoreContext, XStoreManager.Instance.BaseGame);
        }

        private void PurchaseGame()
        {
            Logger.Instance.Log($"Calling ShowPurchaseUI for {XStoreManager.Instance.BaseGame}", color: LogColor.Event);
            XStoreShowUI.ShowPurchaseUI(XStoreManager.Instance.StoreContext, XStoreManager.Instance.BaseGame);
        }

        private void ShowAddonsPage()
        {
            Logger.Instance.Log($"Calling ShowAssociatedProductsUI for {XStoreManager.Instance.BaseGame}", color: LogColor.Event);
            XStoreShowUI.ShowAssociatedProductsUI(XStoreManager.Instance.StoreContext, XStoreManager.Instance.BaseGame);
        }

        private void ShowRateAndReviewPage()
        {
            Logger.Instance.Log("Calling ShowRateAndReviewUI", color: LogColor.Event);
            XStoreShowUI.ShowRateAndReviewUI(XStoreManager.Instance.StoreContext);
        }

        private void ShowRedeemTokenPage()
        {
            Logger.Instance.Log("Calling ShowRedeemTokenUI", color: LogColor.Event);
            XStoreShowUI.ShowRedeemTokenUI(XStoreManager.Instance.StoreContext);
        }

        private void QueryCatalog()
        {
            Logger.Instance.Log("Calling QueryAssociatedProducts", color: LogColor.Event);
            XStoreManager.Instance.QueryAssociatedProducts(true);
        }

        private void QueryCollections()
        {
            Logger.Instance.Log("Calling QueryEntitledProducts", color: LogColor.Event);
            XStoreManager.Instance.QueryEntitledProducts(true);
        }

        private void ShowInGameStore()
        {
            _previousSelected = InGameStoreButton.gameObject;
            MenuManager.Instance.ShowProductListMenu();
        }

        private void QueryGameLicense()
        {
            Logger.Instance.Log("Calling QueryGameLicense", color: LogColor.Event);
            XStoreManager.Instance.QueryGameLicense(true);
        }

        private void QueryAddonLicenses()
        {
            Logger.Instance.Log("Calling QueryAddOnLicenses", color: LogColor.Event);
            XStoreLicensing.QueryAddOnLicenses(XStoreManager.Instance.StoreContext);
        }

    }
}
