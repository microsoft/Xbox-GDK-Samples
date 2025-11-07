using UnityEngine;
using UnityEngine.UI;

namespace GdkSample_SocialManager
{
    public sealed class SystemInfo : MonoBehaviour
    {
        [SerializeField] private Text _sandboxId;
        [SerializeField] private Text _titleId;
        [SerializeField] private Text _scid;

        // Start is called before the first frame update
        void Start()
        {
            GDKGameRuntime.GameRuntimeInitialized += HandleGameRuntimeInitialized;

            if (GDKGameRuntime.Initialized)
            {
                HandleGameRuntimeInitialized();
            }
        }

        private void HandleGameRuntimeInitialized()
        {
            _sandboxId.text = string.Format($"Sandbox ID: {GDKGameRuntime.GameConfigSandbox}");
            _titleId.text = string.Format($"Title ID: 0x{GDKGameRuntime.GameConfigTitleId}");
            _scid.text = string.Format($"SCID: {GDKGameRuntime.GameConfigScid}");
        }
    }
}
