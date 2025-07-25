using UnityEngine;

namespace WindowsSample_HandheldBestPractices
{
    public sealed class RefreshMemoryButton : MonoBehaviour
    {
        public void Refresh()
        {
            HandheldBestPracticesManager.Instance.UpdateMemoryInfo();
        }
    }
}
