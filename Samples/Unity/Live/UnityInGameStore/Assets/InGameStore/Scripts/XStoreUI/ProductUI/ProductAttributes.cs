using UnityEngine;
using UnityEngine.UI;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// ProductAttributes class for storing product attributes (name, price, images, etc).
    /// Note: Product attribtes might contain unicode symbols. Be sure to use a unicode-supported font for display in UI.
    /// </summary>
    public sealed class ProductAttributes : MonoBehaviour
    {
        public Text Name;
        public Text Price;
        public RawImage ProductIcon;
        public RawImage OwnershipIcon;
        public string StoreId;
        public string ProductKind;
        public string Quantity;
        public string Ownership;
        public string Description;
        public string[] BundledSkus;
    }
}
