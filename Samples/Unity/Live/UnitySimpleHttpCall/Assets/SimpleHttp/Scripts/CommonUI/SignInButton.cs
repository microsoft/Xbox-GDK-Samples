using UnityEngine;

namespace GdkSample_SimpleHttp
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void HandleSignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
