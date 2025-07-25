using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace WindowsSample_HandheldBestPractices
{
    public struct LogColor
    {
        public static readonly Color Default = new Color(0.75f, 0.75f, 0.75f, 1f);
        public static readonly Color Warning = new Color(1f, 0.75f, 0f, 1f);
        public static readonly Color Error = new Color(1f, 0.15f, 0f, 1f);
        public static readonly Color Success = new Color(0.5f, 1f, 0f, 1f);
        public static readonly Color System = new Color(0.95f, 0.95f, 0.95f, 1f);
        public static readonly Color Event = new Color(0f, 1f, 1f, 1f);
        public static readonly Color Initialization = new Color(1f, 0f, 1f, 1f);
    }

    public sealed class Logger : MonoBehaviour
    {
        [SerializeField] private ScrollRect _logScrollRect;
        [SerializeField] private GameObject _logPrefab;

        public static Logger Instance { get; private set; }

        private List<LogEntry> _logs = new List<LogEntry>();

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }
            Instance = this;

            gameObject.SetActive(true);

            Log("Application Started.");
        }

        private void OnEnable()
        {
            Application.logMessageReceived += HandleLogMessageReceived;
        }

        private void OnDisable()
        {
            Application.logMessageReceived -= HandleLogMessageReceived;
        }

        private void OnDestroy()
        {
            ClearLogs();
        }

        public void Log(string message, Color? color = null, bool sendToDebug = true)
        {
            GameObject prefab = Instantiate(_logPrefab, _logScrollRect.content.gameObject.transform);
            var consoleLog = prefab.GetComponent<LogEntry>();
            consoleLog.SetMessage(message, color ?? LogColor.Default);

            string prevMessage = _logs.Count > 0 ? _logs[_logs.Count - 1].Text : string.Empty;
            if (prevMessage.Equals(consoleLog.Text))
            {
                Destroy(consoleLog.gameObject);
            }
            else
            {
                _logs.Add(consoleLog);

                // Scroll to bottom
                Canvas.ForceUpdateCanvases();
                consoleLog.gameObject.GetComponent<ContentSizeFitter>().SetLayoutVertical();
                _logScrollRect.content.GetComponent<VerticalLayoutGroup>().CalculateLayoutInputVertical();
                _logScrollRect.content.GetComponent<ContentSizeFitter>().SetLayoutVertical();
                _logScrollRect.verticalNormalizedPosition = 0;

                if (sendToDebug)
                {
                    Debug.Log(message);
                }
            }
        }

        public void ClearLogs()
        {
            while (_logs.Count > 0)
            {
                LogEntry target = _logs[0];
                _logs.RemoveAt(0);
                Destroy(target.gameObject);
            }
        }

        private void HandleLogMessageReceived(string logString, string stackTrace, LogType type)
        {
            string newString = logString;

            Color color = LogColor.Default;

            if (type == LogType.Warning)
            {
                color = LogColor.Warning;
            }
            else if (type == LogType.Error || type == LogType.Exception)
            {
                color = LogColor.Error;
            }

            if (type == LogType.Exception)
            {
                newString = stackTrace;
            }

            Log(newString, color, false);
        }
    }
}
