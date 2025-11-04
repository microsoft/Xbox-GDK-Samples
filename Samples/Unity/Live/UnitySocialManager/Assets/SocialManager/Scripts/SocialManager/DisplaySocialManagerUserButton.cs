using System;
using UnityEngine;
using UnityEngine.UI;
using Unity.XGamingRuntime;

using XBL = Unity.XGamingRuntime.SDK.XBL;

namespace GdkSample_SocialManager
{
    // UI button to display an Xbox user's profile card.
    public sealed class DisplaySocialManagerUserButton : MonoBehaviour
    {
        public const Int32 E_ABORT = unchecked((Int32)0x80004004);
        public void Clear()
        {
            m_TextGamerTag.text = "";
            m_TextConnectionStatus.text = "";
            CleanupTexture();
            m_Gamerpic.enabled = false;

            m_xuid = 0;
            m_isSet = false;
        }

        public void DisplayProfileCard()
        {
            Debug.Log($"{nameof(DisplaySocialManagerUserButton)}.{nameof(DisplaySocialManagerUserButton.DisplayProfileCard)}()");

            if (m_isSet)
            {
                Debug.Log($"Opening profile card for {m_TextGamerTag.text}");

                // Display the targetted user's Xbox profile card
                SDK.XGameUiShowPlayerProfileCardAsync(
                    XboxManager.Instance.UserHandle,   // local Xbox user handle
                    m_xuid,                             // Xbox user id to look up
                    (int hr) =>
                    {
                        Debug.Log($"{nameof(SDK)}.{nameof(SDK.XGameUiShowPlayerProfileCardAsync)}.Callback()");

                        if (HR.FAILED(hr) && hr != E_ABORT)
                        {
                            Debug.LogError($"{nameof(SDK)}.{nameof(SDK.XGameUiShowPlayerProfileCardAsync)} failed - 0x{hr:X8}.");
                        }
                    });
            }
        }

        public void SetSocialManagerUser(XblSocialManagerUser user)
        {
            m_xuid = user.XboxUserId;
            m_TextGamerTag.text = user.Gamertag;
            StartCoroutine(XboxSocialManager.Instance.GetGamerpic(user, OnProfilePicBytesReceived));

            // Retrieve the social presence of the user
            var titleUInt = uint.Parse(GDKGameRuntime.GameConfigTitleId, System.Globalization.NumberStyles.HexNumber);
            if (XBL.XblSocialManagerPresenceRecordIsUserPlayingTitle(user.PresenceRecord, titleUInt))
            {
                m_TextConnectionStatus.text = "In Title";
            }
            else
            {
                m_TextConnectionStatus.text = Enum.GetName(typeof(XblPresenceUserState), user.PresenceRecord.UserState);
            }

            m_isSet = true;
        }

        private void OnProfilePicBytesReceived(string context, byte[] pngImageBytes)
        {
            CleanupTexture();

            m_texture = new Texture2D(2, 2);
            m_texture.LoadImage(pngImageBytes);

            m_Gamerpic.texture = m_texture;
            m_Gamerpic.enabled = true;
        }

        [SerializeField] private Text m_TextGamerTag;
        [SerializeField] private Text m_TextConnectionStatus;
        [SerializeField] private RawImage m_Gamerpic;
        private Texture2D m_texture;

        private ulong m_xuid;
        private bool m_isSet;

        private void Start()
        {
            Clear();
        }

        private void CleanupTexture()
        {
            if (m_texture != null)
            {
                Destroy(m_texture);
                m_texture = null;
            }
        }

        private void OnDestroy()
        {
            CleanupTexture();
        }
    }
}
