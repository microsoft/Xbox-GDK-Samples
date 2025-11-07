using UnityEngine;
using UnityEngine.UI;
using System.Text;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// ProductDetails class for displaying product details (name, price, images, etc) in the UI.
    /// </summary>
    public sealed class ProductDetails : MonoBehaviour
    {
        [SerializeField] private Text NameText;
        [SerializeField] private Text SummaryText;
        [SerializeField] private Text DescriptionText;
        [SerializeField] private Text QuantityText;
        [SerializeField] private Text BundleTitleText;
        [SerializeField] private Text BundleDetailsText;
        [SerializeField] private RawImage PosterImage;

        // Start is called before the first frame update
        void Start()
        {
            ProductListMenu.Instance.ProductSelectionChanged += OnSelectionChanged;
            OnSelectionChanged();
        }

        /// <summary>
        /// Handles selection change events coming from the Product List Menu.
        /// Keeps product details in-sync with which product has focus in the product list.
        /// </summary>
        private void OnSelectionChanged()
        {
            NameText.text = "";
            SummaryText.text = "";
            DescriptionText.text = "";
            QuantityText.text = "";
            BundleTitleText.gameObject.SetActive(false);
            BundleDetailsText.text = "";

            string storeId = ProductListMenu.Instance.SelectedProduct;

            if (!string.IsNullOrEmpty(storeId))
            {
                SetProductDetails(storeId);
            }
        }

        /// <summary>
        /// Sets product details (summary, description, poster image, etc.) for display in UI.
        /// </summary>
        /// <param name="storeId"></param>
        public void SetProductDetails(string storeId)
        {
            ProductAttributes attributes = ProductUIManager.Instance.UIProducts[storeId].GetComponentInChildren<ProductAttributes>(true);
            if (attributes == null)
            {
                return;
            }

            PosterImage.texture = ProductUIManager.Instance.PosterImages[storeId].GetComponent<RawImage>().texture;

            NameText.text = attributes.Name.text;
            SummaryText.text = $"{attributes.StoreId}\n{attributes.ProductKind}\n{attributes.Ownership}";
            DescriptionText.text = attributes.Description;
            QuantityText.text = attributes.Quantity;

            if (attributes.BundledSkus.Length > 0)
            {
                BundleTitleText.gameObject.SetActive(true);
                StringBuilder bundleDetails = new();

                foreach (string sku in attributes.BundledSkus)
                {
                    bundleDetails.Append($"{sku}\n");
                    if (ProductUIManager.Instance.UIProducts.ContainsKey(sku))
                    {
                        bundleDetails.Append($"\t{ProductUIManager.Instance.UIProducts[sku].GetComponentInChildren<ProductAttributes>().Name.text}\n");
                    }
                }

                bundleDetails.Remove(bundleDetails.Length - 1, 1);
                BundleDetailsText.text = bundleDetails.ToString();
            }
        }
    }
}
