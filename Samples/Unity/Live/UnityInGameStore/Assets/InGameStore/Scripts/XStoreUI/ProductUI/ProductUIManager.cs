using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;

using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// ProductUIManager class for syncs product details and images in the UI with data returned from service.
    /// </summary>
    public sealed class ProductUIManager : MonoBehaviour
    {
        [SerializeField] private GameObject ProductButtonPrefab;
        [SerializeField] private GameObject PosterImagePrefab;
        [SerializeField] private GameObject BoxArtImagePrefab;
        [SerializeField] private ScrollRect ProductScrollRect;

        public static ProductUIManager Instance { get; private set; }

        public int OwnedProductsCount { get; private set; }

        public Action UIProductsUpdated;

        public Dictionary<string, GameObject> UIProducts;
        public Dictionary<string, GameObject> PosterImages;
        public Dictionary<string, GameObject> BoxArtImages;

        private bool _initialized = false;

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
            OwnedProductsCount = 0;
        }

        // Start is called before the first frame update
        private void Start()
        {
            if (!_initialized)
            {
                // Clear test products from scene.
                ProductAttributes[] testProducts = ProductScrollRect.content.GetComponentsInChildren<ProductAttributes>(true);
                foreach (ProductAttributes product in testProducts)
                {
                    Destroy(product.gameObject);
                }

                _initialized = true;
            }

            UIProducts = new Dictionary<string, GameObject>();
            PosterImages = new Dictionary<string, GameObject>();
            BoxArtImages = new Dictionary<string, GameObject>();

            XStoreManager.Instance.ProductsUpdated += OnProductsUpdated;
        }

        /// <summary>
        /// Handles product updates that occur whenever queries are made to
        /// catalog or collections.
        /// </summary>
        public void OnProductsUpdated()
        {
            OwnedProductsCount = 0;

            if (XStoreManager.Instance.AllProducts.Count > 0)
            {
                foreach (KeyValuePair<string, XStoreProduct> product in XStoreManager.Instance.AllProducts)
                {
                    UpdateUIProduct(product.Value);
                }

                UIProductsUpdated?.Invoke();
            }
        }

        /// <summary>
        /// Updates details of an existing product in the UIProduct list.
        /// Modifies attributes (price, ownership, quantity) that can change during a user's session.
        /// </summary>
        /// <param name="product">The product to update.</param>
        private void UpdateUIProduct(XStoreProduct product)
        {
            if (!UIProducts.ContainsKey(product.StoreId))
            {
                AddUIProduct(product);
            }
            else
            {
                ProductAttributes attributes = UIProducts[product.StoreId].GetComponentInChildren<ProductAttributes>();
                if(attributes == null)
                {
                    return;
                }

                // Update product price.
                string price = product.Price.FormattedPrice;

                if ((product.Price.CurrencyCode == "CNY" || product.Price.CurrencyCode == "JPY") && price.Contains('\uffe5'))
                {
                    // Fullwidth Yen is not supported by the Sample's font, replace with standard Yen.
                    attributes.Price.text = $"{price.Replace('\uffe5', '\u00a5')}";
                }
                else
                {
                    attributes.Price.text = price;
                }

                // Update ownership details.
                if (product.IsInUserCollection)
                {
                    OwnedProductsCount++;
                    attributes.OwnershipIcon.gameObject.SetActive(true);
                    attributes.Ownership = "Owned";

                    if (product.ProductKind == XStoreProductKind.Game && product.StoreId == XStoreManager.Instance.BaseGame)
                    {
                        if (XStoreManager.Instance.IsTrialGame() && XStoreManager.Instance.GameLicense.IsTrialOwnedByThisUser)
                        {
                            attributes.Ownership = "Owned (Trial)";
                        }
                    }
                }
                else if (XStoreManager.Instance.CatalogProducts.ContainsKey(product.StoreId))
                {
                    attributes.Ownership = "Available for purchase";
                    attributes.OwnershipIcon.gameObject.SetActive(false);
                }
                else
                {
                    attributes.Ownership = "Not available";
                    attributes.OwnershipIcon.gameObject.SetActive(false);
                }

                // Update consumable quantity count.
                if (product.ProductKind == XStoreProductKind.Consumable || product.ProductKind == XStoreProductKind.UnmanagedConsumable)
                {
                    uint quantity = 0;

                    foreach (XStoreSku sku in product.Skus)
                    {
                        quantity += sku.CollectionData.Quantity;
                    }

                    attributes.Quantity = $"Quantity: {quantity}";
                }
            }
        }

        /// <summary>
        /// Adds a new product to the UIProducts list and downloads associated images.
        /// Note: The UIProduct list is preserved between user changes so images are cached.
        /// </summary>
        /// <param name="product"></param>
        private void AddUIProduct(XStoreProduct product)
        {
            if (UIProducts.ContainsKey(product.StoreId))
            {
                UpdateUIProduct(product);
            }
            else
            {
                GameObject productButton = Instantiate(ProductButtonPrefab);
                ProductAttributes attributes = productButton.GetComponentInChildren<ProductAttributes>(true);

                // Set general product details.
                attributes.StoreId = product.StoreId;
                attributes.Name.text = product.Title;
                attributes.Price.text = product.Price.FormattedPrice;
                attributes.Description = product.Description;
                attributes.ProductKind = product.ProductKind.ToString();

                if(product.ProductKind == XStoreProductKind.Durable && product.HasDigitalDownload)
                {
                    attributes.ProductKind = $"{product.ProductKind} (Package)";
                }

                // Set bundle details.
                List<string> bundledSkus = new();
                foreach (XStoreSku sku in product.Skus)
                {
                    if (sku.BundledSkus.Length > 0)
                    {
                        foreach (string bundledSku in sku.BundledSkus)
                        {
                            bundledSkus.Add(bundledSku);
                        }
                    }
                }
                attributes.BundledSkus = bundledSkus.ToArray();

                if(bundledSkus.Count > 0)
                {
                    attributes.ProductKind = $"{product.ProductKind} (Bundle)";
                }

                // Retrieve product images.
                GameObject posterImage = Instantiate(PosterImagePrefab);
                posterImage.SetActive(false);
                PosterImages.Add(product.StoreId, posterImage);

                GameObject boxArtImage = Instantiate(BoxArtImagePrefab);
                boxArtImage.SetActive(false);
                BoxArtImages.Add(product.StoreId, boxArtImage);

                foreach (XStoreImage image in product.Images)
                {
                    string imageUri = $"https:{image.Uri}";

                    // Get large image to display on the ProductListMenu.
                    if (image.ImagePurposeTag == "Poster")
                    {
                        RawImage poster = PosterImages[product.StoreId].GetComponent<RawImage>();
                        StartCoroutine(DownloadProductImage(imageUri, poster));
                    }

                    // Get small image to display on the product button.
                    if (image.ImagePurposeTag == "Logo")
                    {
                        StartCoroutine(DownloadProductImage(imageUri, attributes.ProductIcon));
                    }

                    // Get medium image to display on the ItemMenu.
                    if (image.ImagePurposeTag == "BoxArt")
                    {
                        RawImage boxArt = BoxArtImages[product.StoreId].GetComponent<RawImage>();
                        StartCoroutine(DownloadProductImage(imageUri, boxArt));
                    }
                }

                // Add button to the UI products list.
                productButton.transform.SetParent(ProductScrollRect.content.gameObject.transform, false);
                productButton.GetComponentInChildren<Button>().onClick.AddListener(() => MenuManager.Instance.ShowItemMenu(product.StoreId));
                productButton.SetActive(false);
                UIProducts.Add(product.StoreId, productButton);

                UpdateUIProduct(product);
            }
        }

        /// <summary>
        /// Downloads images from the web.
        /// </summary>
        /// <param name="imageUri">Uri of the XStoreImage to download.</param>
        /// <param name="resultImage">RawImage that can be displayed in UI.</param>
        /// <returns></returns>
        private IEnumerator DownloadProductImage(string imageUri, RawImage resultImage)
        {
            UnityWebRequest www = UnityWebRequestTexture.GetTexture(imageUri);
            yield return www.SendWebRequest();

            if (www.result != UnityWebRequest.Result.Success)
            {
                Debug.Log($"Failed to retrieve image: {www.error}");
            }
            else
            {
                resultImage.texture = DownloadHandlerTexture.GetContent(www);
            }
        }

    }
}
