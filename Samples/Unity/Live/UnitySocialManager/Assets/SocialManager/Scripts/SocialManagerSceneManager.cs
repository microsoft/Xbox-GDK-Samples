using System.Collections.Generic;
using Unity.XGamingRuntime;
using UnityEngine;
using UnityEngine.UI;

namespace GdkSample_SocialManager
{
    public sealed class SocialManagerSceneManager : MonoBehaviour
    {
        [SerializeField] private List<Button> _sampleButtons;

        // Start is called before the first frame update
        private void Start()
        {
            XboxManager.Instance.UserSignedIn += HandleUserSignedIn;
            XboxManager.Instance.UserSignedOut += HandleUserSignedOut;

            SetButtonInteractableState(false);

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        /// <summary>
        /// Called after an Xbox Live user has signed into the game
        /// </summary>
        private void HandleUserSignedIn(XUserHandle userHandle)
        {
            SetButtonInteractableState(true);
        }

        /// <summary>
        /// Called when the active Xbox Live user signs out of the system
        /// </summary>
        private void HandleUserSignedOut()
        {
            SetButtonInteractableState(false);
        }

        private void SetButtonInteractableState(bool isInteractable)
        {
            foreach (Button button in _sampleButtons)
            {
                if (button != null)
                {
                    button.interactable = isInteractable;
                }
            }
        }
    }
} // namespace GdkSample_SocialManager
