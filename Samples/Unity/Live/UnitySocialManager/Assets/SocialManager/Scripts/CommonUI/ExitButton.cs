using UnityEngine;

namespace GdkSample_SocialManager
{
    public sealed class ExitButton : MonoBehaviour
    {
        // Called when the Exit button is clicked
        public void HandleExit()
        {
            Debug.LogWarning("Shutting down...");

            Application.Quit();
        }
    }
}
