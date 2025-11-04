using UnityEngine;

namespace GdkSample_SocialManager
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void SignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
