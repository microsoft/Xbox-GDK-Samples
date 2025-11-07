using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;
using Unity.XGamingRuntime;

namespace GdkSample_SocialManager
{
    // UI controller for the 5 social manager buttons.
    public sealed class DisplaySocialManagerUserButtonManager : MonoBehaviour
    {
        private const int C_USER_SOCIAL_BUTTON_COUNT = 5;

        [SerializeField] private Text m_SocialGroupsTitle;
        [SerializeField] private DisplaySocialManagerUserButton[] m_SocialManagerUserButtons = new DisplaySocialManagerUserButton[C_USER_SOCIAL_BUTTON_COUNT];

        private void OnValidate()
        {
            Assert.AreEqual(m_SocialManagerUserButtons.Length, C_USER_SOCIAL_BUTTON_COUNT, $"{nameof(m_SocialManagerUserButtons)} does not have exactly {C_USER_SOCIAL_BUTTON_COUNT} entries.");;
            for (int i = 0; i < C_USER_SOCIAL_BUTTON_COUNT; i++)
            {
                Assert.IsNotNull(m_SocialManagerUserButtons[i], $"{nameof(m_SocialManagerUserButtons)} contains {null} at index {i}");
            }
        }

        private void Start()
        {
            XboxSocialManager.Instance.UserListRefreshed += OnSocialManagerUserListRefreshed;
        }

        private void OnSocialManagerUserListRefreshed(XblSocialManagerUser[] users)
        {
            // Update title
            m_SocialGroupsTitle.text = XboxSocialManager.Instance.CurrentSocialGroup.ToString();

            // Update user buttons
            for (int i = 0; i < C_USER_SOCIAL_BUTTON_COUNT; i++)
            {
                m_SocialManagerUserButtons[i].Clear();
            }
            for (int i = 0; i < users.Length; i++)
            {
                if (i == C_USER_SOCIAL_BUTTON_COUNT)
                {
                    break;
                }
                m_SocialManagerUserButtons[i].SetSocialManagerUser(users[i]);
            }
        }
    }
}
