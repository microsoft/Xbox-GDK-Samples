using UnityEngine;

namespace GdkSample_LeaderboardsTM
{
    public sealed class SignInButton : MonoBehaviour
    {
        public void HandleSignIn()
        {
            XboxManager.Instance.SignIn(true);
        }
    }
}
