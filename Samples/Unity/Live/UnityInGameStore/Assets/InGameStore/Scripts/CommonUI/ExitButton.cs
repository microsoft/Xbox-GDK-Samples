using UnityEngine;

namespace GdkSample_InGameStore
{
    public sealed class ExitButton : MonoBehaviour
    {
        // Called when the Exit button is clicked
        public void HandleExit()
        {
            Debug.LogWarning("Shutting down...");
            XStoreManager.Instance.CloseAllStoreHandles();
            Application.Quit();
        }
    }
}
