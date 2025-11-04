using UnityEngine;

namespace GdkSample_SimplePLM
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void HandleSignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
