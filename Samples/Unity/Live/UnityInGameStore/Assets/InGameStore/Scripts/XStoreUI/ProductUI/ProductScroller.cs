using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.UI;

namespace GdkSample_InGameStore
{
    public class ProductScroller : MonoBehaviour
    {
        [SerializeField] private ScrollRect ProductScrollRect;

        private InputDevice _lastDevice;

        // Start is called before the first frame update
        void Start()
        {
            ProductListMenu.Instance.ProductSelectionChanged += OnSelectedProductChanged;

            // Track last device used to change behavior based on input source.
            InputSystem.onActionChange += (obj, change) =>
            {
                if (change == InputActionChange.ActionPerformed)
                {
                    _lastDevice = ((InputAction)obj).activeControl.device;
                }
            };
        }

        private void OnSelectedProductChanged()
        {
            if (_lastDevice != null && _lastDevice is Keyboard || _lastDevice is Gamepad)
            {
                UpdateVerticalScrollbarPosition();
            }
        }

        /// <summary>
        /// Updates the vertical scrollbar position to keep the selected product in view.
        /// </summary>
        private void UpdateVerticalScrollbarPosition()
        {
            ProductAttributes[] products = ProductScrollRect.content.GetComponentsInChildren<ProductAttributes>(true);
            if (products.Length == 0)
            {
                return;
            }

            int productIndex = 0;
            int activeCount = 0;
            foreach (ProductAttributes product in products)
            {
                if (product.gameObject.activeSelf)
                {
                    if (product.StoreId == ProductListMenu.Instance.SelectedProduct)
                    {
                        productIndex = activeCount;
                    }

                    activeCount++;
                }
            }

            if (productIndex == (activeCount - 1))
            {
                // Scroll to bottom of list
                ProductScrollRect.verticalNormalizedPosition = 0;
            }
            else
            {
                float scrollPosition = 1f - (float)productIndex / activeCount;

                if (scrollPosition < 0.45f)
                {
                    // Correct scroll position as we get closer to the bottom
                    float correction = 1f / activeCount;
                    scrollPosition -= correction;
                }

                ProductScrollRect.verticalNormalizedPosition = scrollPosition;
            }
        }
    }
}
