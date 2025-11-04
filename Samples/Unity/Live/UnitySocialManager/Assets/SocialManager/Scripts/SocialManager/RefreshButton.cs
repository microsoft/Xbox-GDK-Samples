using UnityEngine;

namespace GdkSample_SocialManager
{
    public sealed class RefreshButton : MonoBehaviour
    {
        public void Refresh()
        {
            XboxSocialManager.Instance.Refresh();
        }
    }
}
