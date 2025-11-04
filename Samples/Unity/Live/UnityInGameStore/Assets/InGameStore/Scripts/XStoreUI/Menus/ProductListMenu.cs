using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// ProductListMenu class for displaying a list of catalog products available to the user.
    /// </summary>
    public sealed class ProductListMenu : MonoBehaviour
    {
        [SerializeField] private ScrollRect ProductScrollRect;
        [SerializeField] private ProductDetails ProductDetails;
        [SerializeField] private Button CloseButton;
        [SerializeField] private Text TotalCountText;
        [SerializeField] private Text OwnedCountText;

        public static ProductListMenu Instance { get; private set; }
        public string SelectedProduct { get; private set; }

        public event Action ProductSelectionChanged;

        private InputAction _cancelAction;

        // GameObject initialization.
        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
            SelectedProduct = "";
            _cancelAction = InputSystem.ListEnabledActions().FindLast(action => action.name == "Cancel");
        }

        /// <summary>
        /// Prepares Menu for user interaction.
        /// </summary>
        public void ShowMenu()
        {
            if (string.IsNullOrEmpty(SelectedProduct))
            {
                if (ProductUIManager.Instance.UIProducts.Count == 0)
                {
                    // This shouldn't be hit, but we can query for products again if something went wrong
                    // during store initialization.
                    XStoreManager.Instance.QueryAssociatedProducts();
                    EventSystem.current.SetSelectedGameObject(CloseButton.gameObject);
                }
                else
                {
                    // Intialize the ProductList with products available to current user.
                    OnProductsUpdated();
                }
            }
            else
            {
                // Returning to the menu after previous intialization.
                // Set focus to the previously selected item.
                EventSystem.current.SetSelectedGameObject(ProductUIManager.Instance.UIProducts[SelectedProduct]);
                ProductSelectionChanged?.Invoke();
            }
        }

        // Start is called before the first frame update.
        private void Start()
        {
            // Set event handlers.
            XboxManager.Instance.UserSignInStarted += Close;
            XboxManager.Instance.UserSignedIn += Close;
            XboxManager.Instance.UserSignedOut += Close;
            XStoreManager.Instance.StoreInitializationSucceeded += OnProductsUpdated;
            ProductUIManager.Instance.UIProductsUpdated += OnProductsUpdated;

            CloseButton.onClick.AddListener(() => Close());
        }

        private void Update()
        {
            // Track product selection changes to keep product details in-sync with user selection.
            if (EventSystem.current.currentSelectedGameObject != null)
            {
                ProductAttributes attributes = EventSystem.current.currentSelectedGameObject.GetComponentInChildren<ProductAttributes>();

                if (attributes != null)
                {
                    string selectedItem = attributes.StoreId;

                    if (SelectedProduct != selectedItem)
                    {
                        SelectedProduct = selectedItem;
                        ProductSelectionChanged?.Invoke();
                    }
                }
            }

            if (_cancelAction.WasReleasedThisFrame())
            {
                Close();
            }
        }

        /// <summary>
        /// Changes visiblity of UI products to only show items available to the current user.
        /// The 'UIProducts' list maintained by ProductUIManager persists between user changes to avoid
        /// re-creating product buttons and re-downloading product images. We use the 'AllProducts' list,
        /// created by XStoreManager in context of the current user, to determine which items should be visible.
        /// </summary>
        private void OnProductsUpdated()
        {
            if (!XStoreManager.Instance.IsStoreReady)
            { return; }

            int activeProducts = 0;
            int ownedProducts = ProductUIManager.Instance.OwnedProductsCount;
            string firstVisibleProduct = "";

            // UIProducts contains all products retrieved across all users that have signed-in during the game session.
            // If there was a user change, then some of these products might not be relevant to the current player.

            foreach (KeyValuePair<string, GameObject> product in ProductUIManager.Instance.UIProducts)
            {
                // AllProducts contains products that are only relevant to the current user. If the product
                // is in this list, then it should be visible to the player.

                if (XStoreManager.Instance.AllProducts.ContainsKey(product.Key))
                {
                    product.Value.SetActive(true);
                    activeProducts++;

                    // Determine if the base game offer should be visible to the player.
                    if (product.Key == XStoreManager.Instance.BaseGame && !XStoreManager.Instance.ShowBaseGameOffer())
                    {
                        // User has access to the full license, so don't show the upsell offer.
                        product.Value.SetActive(false);
                        activeProducts--;

                        if (ownedProducts > 0)
                        {
                            ownedProducts--;
                        }
                    }

                    // Keep track of the first visible item in case the previous selected item
                    // is no longer valid.
                    if (string.IsNullOrEmpty(firstVisibleProduct) && product.Value.activeSelf)
                    {
                        firstVisibleProduct = product.Key;
                    }

                    // If there wasn't a previous item selected, then mark this item as the selected item.
                    if (string.IsNullOrEmpty(SelectedProduct) && product.Value.activeSelf)
                    {
                        SelectedProduct = product.Key;
                    }
                }
                else
                {
                    // Hide any products that should not be visible to the current user.
                    product.Value.SetActive(false);
                }
            }

            // Determine which product in the list should have focus.
            if (SelectedProduct == "" || !ProductUIManager.Instance.UIProducts[SelectedProduct].activeSelf)
            {
                SelectedProduct = firstVisibleProduct;
            }

            // If the menu is visible, set product details and focus to the SelectedProduct.
            if (gameObject.activeInHierarchy)
            {
                ProductDetails.SetProductDetails(SelectedProduct);
                EventSystem.current.SetSelectedGameObject(ProductUIManager.Instance.UIProducts[SelectedProduct]);
            }

            // Update total counts to match visible products in the list.
            TotalCountText.text = "Total: " + activeProducts;
            OwnedCountText.text = "Owned: " + ownedProducts;
        }

        /// <summary>
        /// Returns user to the Main Menu.
        /// </summary>
        private void Close()
        {
            if (gameObject.activeInHierarchy)
            {
                MenuManager.Instance.ShowMainMenu();
            }
        }
    }
}
