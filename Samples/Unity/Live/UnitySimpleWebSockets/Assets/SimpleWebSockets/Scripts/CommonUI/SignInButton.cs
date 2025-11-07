using UnityEngine;

namespace GdkSample_SimpleWebSockets
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void HandleSignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
