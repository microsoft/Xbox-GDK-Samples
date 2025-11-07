using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

namespace GdkSample_InGameStore
{
    // Handles OnPointerEnter for product buttons in the ProductListMenu.
    public class ProductButton : MonoBehaviour, IPointerEnterHandler
    {
        private InputDevice _lastDevice;

        void Start()
        {
            // Track last device used to change behavior based on input source.
            InputSystem.onActionChange += (obj, change) =>
            {
                if (change == InputActionChange.ActionPerformed)
                {
                    _lastDevice = ((InputAction)obj).activeControl.device;
                }
            };
        }

        public void OnPointerEnter(PointerEventData eventData)
        {
            if (_lastDevice != null && _lastDevice is Mouse 
                && EventSystem.current.currentSelectedGameObject != gameObject)
            {
                EventSystem.current.SetSelectedGameObject(gameObject);
            }
        }
    }
}
