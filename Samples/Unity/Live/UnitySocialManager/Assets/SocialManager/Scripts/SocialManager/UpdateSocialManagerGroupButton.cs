using UnityEngine;

namespace GdkSample_SocialManager
{
    // Sets the social manager filter to the assigned social group.
    public sealed class UpdateSocialManagerGroupButton : MonoBehaviour
    {
        [SerializeField] XboxSocialManager.SocialGroupType m_SocialGroup;

        public void UpdateSocialGroup()
        {
            XboxSocialManager.Instance.UpdateSocialGroup(m_SocialGroup);
        }
    }
}
