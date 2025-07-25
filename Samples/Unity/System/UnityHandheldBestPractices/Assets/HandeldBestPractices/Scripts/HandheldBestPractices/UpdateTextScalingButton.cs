using UnityEngine;

namespace WindowsSample_HandheldBestPractices
{
    public sealed class UpdateTextScalingButton : MonoBehaviour
    {
        [Header("Set in Inspector")]
        public HandheldBestPracticesManager.TextScalingMode TextScalingValue;
        public void UpdateTextScaling()
        {
            HandheldBestPracticesManager.Instance.UpdateTextScaling(TextScalingValue);
        }
    }
}
