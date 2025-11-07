using System;
using UnityEngine;
using UnityEngine.UI;

namespace GdkSample_Achievements
{
    public sealed class LogEntry : MonoBehaviour
    {
        [SerializeField] private Text _text;

        public int Height { get { return Mathf.RoundToInt(_text.rectTransform.rect.height); } }
        public string Text { get { return _text.text; } }

        public void SetMessage(string message, Color color)
        {
            DateTime time = DateTime.UtcNow;

            string hour = time.Hour < 10 ? $"0{time.Hour}" : $"{time.Hour}";
            string minute = time.Minute < 10 ? $"0{time.Minute}" : $"{time.Minute}";
            string second = time.Second < 10 ? $"0{time.Second}" : $"{time.Second}";

            _text.text = $"[UTC {hour}:{minute}:{second}] {message}";
            _text.color = color;
        }
    }
}
