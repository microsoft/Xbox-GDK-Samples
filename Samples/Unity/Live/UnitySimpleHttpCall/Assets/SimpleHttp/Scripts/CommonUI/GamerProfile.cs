using UnityEngine;
using UnityEngine.UI;

namespace GdkSample_SimpleHttp
{
    public sealed class GamerProfile : MonoBehaviour
    {
        [SerializeField] private RawImage _gamerPicture;
        [SerializeField] private Text _gamerTag;

        // Start is called before the first frame update
        void Start()
        {
            XboxManager.Instance.GamerTagRetrieved += HandleGamerTagRetrieved;
            XboxManager.Instance.GamerPictureRetrieved += HandleGamerPictureRetrieved;
        }

        private void HandleGamerTagRetrieved()
        {
            if (string.IsNullOrEmpty(XboxManager.Instance.GamerTag))
            {
                _gamerTag.text = "No account signed in";
            }
            else
            {
                _gamerTag.text = XboxManager.Instance.GamerTag;
            }
        }

        private void HandleGamerPictureRetrieved()
        {
            _gamerPicture.texture = XboxManager.Instance.GamerPicture;
        }
    }
}
