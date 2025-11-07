using UnityEngine;

namespace GdkSample_Achievements
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void HandleSignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
