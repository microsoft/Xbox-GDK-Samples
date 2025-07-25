using UnityEngine;

namespace WindowsSample_HandheldBestPractices
{
    public sealed class RefreshNetworkButton : MonoBehaviour
    {
        public void Refresh()
        {
            HandheldBestPracticesManager.Instance.UpdateNetworkInfo();
        }
    }
}
