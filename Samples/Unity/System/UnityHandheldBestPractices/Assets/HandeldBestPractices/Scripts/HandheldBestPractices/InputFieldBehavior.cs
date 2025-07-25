using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using TMPro;

namespace WindowsSample_HandheldBestPractices
{
    public class InputFieldBehavior : MonoBehaviour, IPointerClickHandler
    {
        private TMP_InputField inputField;
        private HandheldBestPracticesInputManager action;

        private void Awake()
        {
            action = new HandheldBestPracticesInputManager();
            inputField = GetComponent<TMP_InputField>();
        }

        private void Start()
        {
            action.UI.Cancel.performed += CancelInput;
            action.UI.Navigate.performed += context =>
            {
                if (Mathf.Abs(context.ReadValue<Vector2>().y) > 0)
                {
                    CancelInput(context);
                }
            };
            action.UI.Submit.performed += AcceptInput;
        }

        private void AcceptInput(InputAction.CallbackContext context)
        {
            if (EventSystem.current.currentSelectedGameObject == inputField.gameObject)
            {
                inputField.ActivateInputField();
                HandheldBestPracticesManager.ShowKeyboard();
            }
        }

        private void CancelInput(InputAction.CallbackContext context)
        {
            if (EventSystem.current.currentSelectedGameObject == inputField.gameObject)
            {
                inputField.DeactivateInputField();
                HandheldBestPracticesManager.HideKeyboard();
            }
        }

        public void OnPointerClick(PointerEventData eventData)
        {
            AcceptInput(default);
        }

        private void OnEnable()
        {
            action.Enable();
        }

        private void OnDisable()
        {
            action.Disable();
        }
    }
}