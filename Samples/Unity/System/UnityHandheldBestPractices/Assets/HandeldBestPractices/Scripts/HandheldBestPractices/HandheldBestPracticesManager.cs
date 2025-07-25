// TODO: Make note on acquiring this namespace: https://www.nuget.org/packages/Microsoft.Management.Infrastructure
using System;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Text;
using TMPro;
using Unity.Profiling;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Utilities;

namespace WindowsSample_HandheldBestPractices
{
    public sealed class HandheldBestPracticesManager : MonoBehaviour
    {
        [DllImport("HandheldHelper")] static extern uint GetPageFaultCount();
        [DllImport("HandheldHelper")] static extern UIntPtr GetWorkingSetSize();
        [DllImport("HandheldHelper")] static extern double GetScreenSize();
        [DllImport("HandheldHelper")] static extern bool IsDeviceHandheld();
        [DllImport("HandheldHelper")] static extern bool IsBluetoothEnabled();
        [DllImport("HandheldHelper")] static extern bool ShowVirtualKeyboard();
        [DllImport("HandheldHelper")] static extern bool HideVirtualKeyboard();

        NetworkInterfaceType[] ACCEPTED_NETWORK_INTERFACE_TYPES = {
            NetworkInterfaceType.Ethernet,
            NetworkInterfaceType.Wireless80211
        };
        string[] IGNORE_MOUSE_CONTROLS = {
            "position",
            "x",
            "y"
        };
        const uint ONE_MEGABYTE = 1024 * 1024;
        const float ORIGINAL_FONT_SIZE = 18f;
        const float MIN_PIXEL_HEIGHT = 9f; // 9 pixels on a 720p display
        const float RECOMMENDED_MIN_PIXEL_HEIGHT = 12f; // 12 pixels on a 720p display

        public static HandheldBestPracticesManager Instance { get; private set; }
        public static bool ShowingKeyboard { get; private set; } = false;

        [Header("Set In Inspector")]
        [SerializeField]
        private TMP_Text deviceInfo;
        [Header("Set In Inspector")]
        [SerializeField]
        private TMP_Text supportedResolutions;
        [Header("Set In Inspector")]
        [SerializeField]
        private TMP_Text networkAdapters;
        [Header("Set In Inspector")]
        [SerializeField]
        private TMP_Text[] scalableTextboxes;
        [Header("Set In Inspector")]
        [SerializeField]
        private TMP_Text textScalingInfo;
        public enum TextScalingMode
        {
            original = 0,
            minScaling = 1,
            recommendedMinScaling = 2
        }

        private ProfilerRecorder systemMemoryRecorder;

        private enum InputDeviceType
        {
            unknown = 0,
            gamepad = 1,
            keyboard = 2,
            mouse = 3
        }
        private InputDeviceType lastInputDeviceType = 0;

        // Device Info
        private long totalMemory;
        private long availableMemory;
        private ulong workingSetSize;
        private ulong pageFaults;

        // Network Info
        private NetworkInterface[] activeNetworkAdapters;
        private NetworkInterface[] inactiveNetworkAdapters;

        void OnEnable()
        {
            systemMemoryRecorder = ProfilerRecorder.StartNew(ProfilerCategory.Memory, "System Used Memory");
        }

        void OnDisable()
        {
            systemMemoryRecorder.Dispose();
        }
        void OnDestroy()
        {
            AudioSettings.OnAudioConfigurationChanged -= OnAudioConfigurationChanged;
        }

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }
            Instance = this;
        }

        void Start()
        {
            AudioSettings.OnAudioConfigurationChanged += OnAudioConfigurationChanged;
            UpdateMemoryInfo();
            UpdateNetworkInfo();
            UpdateTextScaling(TextScalingMode.original);
        }

        void Update()
        {
            // Input device detection
            if (Gamepad.current != null && Gamepad.current.allControls.Any(c => c.IsPressed()))
            {
                lastInputDeviceType = InputDeviceType.gamepad;
            }
            else if(Keyboard.current != null && Keyboard.current.allControls.Any(c => c.IsPressed()))
            {
                lastInputDeviceType = InputDeviceType.keyboard;
            }
            else if(Mouse.current != null && Mouse.current.allControls.Any(c => !IGNORE_MOUSE_CONTROLS.Contains(c.name) && c.IsPressed()))
            {
                lastInputDeviceType = InputDeviceType.mouse;
            }

            deviceInfo.text = GetInfo();
            supportedResolutions.text = GetSupportedResolutionsString();
            networkAdapters.text = GetNetworkAdaptersString();
        }

        public static void ShowKeyboard()
        {
            bool b = ShowVirtualKeyboard();
            ShowingKeyboard = b;
            Debug.Log($"Show virtual keyboard: {b}");
        }

        public static void HideKeyboard()
        {
            bool b = HideVirtualKeyboard();
            ShowingKeyboard = !b;
            Debug.Log($"Hide virtual keyboard: {b}");
        }

        private string GetInfo()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("<b>Device Info</b>");
            string[] deviceModelExtract = SystemInfo.deviceModel.Split('(', ')');
            sb.AppendLine($"  Manufacturer: {(deviceModelExtract.Length > 1 ? deviceModelExtract[1].Trim() : "N/A")}");
            sb.AppendLine($"  Product Name: {(deviceModelExtract.Length > 0 ? deviceModelExtract[0].Trim() : "N/A")}");
            sb.AppendLine($"  Total / Available Memory: {totalMemory} MB / {availableMemory} MB");
            sb.AppendLine($"  Working Set Size: {workingSetSize} MB");
            sb.AppendLine($"  Page Faults: {pageFaults}");
            sb.AppendLine($"  IsHandheld: {IsDeviceHandheld()}");
            // Eventually switch to this, once the editor supports detects this natively
            //sb.AppendLine($"  IsHandheld: {SystemInfo.deviceType == DeviceType.Handheld}");
            sb.AppendLine($"  IsPowered: {SystemInfo.batteryStatus == BatteryStatus.Charging || SystemInfo.batteryStatus == BatteryStatus.Full || SystemInfo.batteryStatus == BatteryStatus.NotCharging}");
            sb.AppendLine($"  IsTouchEnabled: {Touchscreen.current != null}");
            sb.AppendLine($"  IsBluetoothEnabled: {IsBluetoothEnabled()}");

            sb.AppendLine("<b>CPU Info</b>");
            sb.AppendLine($"  CPU Name: {SystemInfo.processorType}");
            sb.AppendLine($"  Logical Cores: {SystemInfo.processorCount}");

            sb.AppendLine("<b>GPU Info</b>");
            sb.AppendLine($"  GPU Name: {SystemInfo.graphicsDeviceName}");
            sb.AppendLine($"  VEN DEV: {SystemInfo.graphicsDeviceVendorID:X} / {SystemInfo.graphicsDeviceID:X}");
            sb.AppendLine($"  Dedicated VRAM: {SystemInfo.graphicsMemorySize} MB");
            sb.AppendLine($"  Wave Size: {SystemInfo.computeSubGroupSize}");

            sb.AppendLine("<b>Current Display Info</b>");
            sb.AppendLine($"  Resolution: {Screen.currentResolution.width} x {Screen.currentResolution.height}");
            sb.AppendLine($"  Vertical Refresh: {Screen.mainWindowDisplayInfo.refreshRate.value:N2}Hz");
            sb.AppendLine($"  Effective DPI: {Screen.dpi}x{Screen.dpi}, scale {(Screen.dpi / 96):N2}x");

            sb.AppendLine("<b>Integrated Display Info</b>");
            sb.AppendLine($"  Screen Size: {GetScreenSize():N2} inches");
            HDROutputSettings[] displays = HDROutputSettings.displays;
            sb.AppendLine($"  HDR Capable: {(SystemInfo.hdrDisplaySupportFlags & HDRDisplaySupportFlags.Supported) == 0}");
            sb.AppendLine($"  HDR Enabled: {(displays.Length > 0 ? displays[0].active : false)}");

            sb.AppendLine("<b>Input Info</b>");
            sb.AppendLine($"  LThumbstick X/Y: {(Gamepad.current == null ? "no gamepad connected" : $"{Gamepad.current.leftStick.x.value}, {Gamepad.current.leftStick.y.value}")}");
            sb.AppendLine($"  Key Pressed: {(Keyboard.current == null ? "no keyboard connected" : Keyboard.current.allKeys.FirstOrDefault(k => k.isPressed))}");
            sb.AppendLine($"  Mouse X/Y: {(Mouse.current == null ? "no mouse connected" : $"{Mouse.current.position.x.value}, {Screen.height - Mouse.current.position.y.value}")}");
            sb.AppendLine($"  Active Input: {lastInputDeviceType.ToString()}");

            sb.AppendLine("<b>Network Info</b>");
            sb.AppendLine($"  Connectivity: {(Application.internetReachability == NetworkReachability.NotReachable ? "No Internet" : "Internet")}");
            sb.AppendLine("<b>Audio Info</b>");
            AudioConfiguration audioConfig = AudioSettings.GetConfiguration();
            sb.AppendLine($"  Sample Rate: {audioConfig.sampleRate}Hz");
            sb.AppendLine($"  Speaker Mode: {audioConfig.speakerMode}");
            string spatialPluginName = AudioSettings.GetSpatializerPluginName();
            sb.AppendLine($"  Spatializer Audio Plugin: {(spatialPluginName == string.Empty ? "None" : spatialPluginName)}");

            return sb.ToString();
        }

        private string GetSupportedResolutionsString()
        {
            StringBuilder sb = new StringBuilder();
            foreach (var res in Screen.resolutions)
            {
                sb.AppendLine($"  {res.width}x{res.height} @ {res.refreshRate}Hz");
            }
            return sb.ToString();
        }

        private string GetNetworkAdaptersString()
        {
            StringBuilder sb = new StringBuilder();
            foreach (NetworkInterface active in activeNetworkAdapters)
                sb.AppendLine($"  <color=\"green\">{active.Description} | {active.Name}</color>");
            foreach (NetworkInterface inactive in inactiveNetworkAdapters)
                sb.AppendLine($"  <color=\"grey\">{inactive.Description} | {inactive.Name}</color>");
            return sb.ToString();
        }

        public void UpdateMemoryInfo()
        {
            totalMemory = SystemInfo.systemMemorySize;
            availableMemory = totalMemory - systemMemoryRecorder.LastValue / ONE_MEGABYTE;
            workingSetSize = GetWorkingSetSize().ToUInt64() / ONE_MEGABYTE;
            pageFaults = GetPageFaultCount();
            Debug.Log($"Updated memory info");
        }

        public void UpdateNetworkInfo()
        {
            var n = NetworkInterface.GetAllNetworkInterfaces();
            activeNetworkAdapters = NetworkInterface.GetAllNetworkInterfaces().Where(
                i => ACCEPTED_NETWORK_INTERFACE_TYPES.Contains(i.NetworkInterfaceType)
                && i.OperationalStatus == OperationalStatus.Up).ToArray();
            inactiveNetworkAdapters = NetworkInterface.GetAllNetworkInterfaces().Where(
                i => ACCEPTED_NETWORK_INTERFACE_TYPES.Contains(i.NetworkInterfaceType)
                && i.OperationalStatus != OperationalStatus.Up).ToArray();
            Debug.Log($"Updated network adapter info");
        }

        public void UpdateTextScaling(TextScalingMode textScalingMode)
        {
            float pixelHeight;
            float fontSizeInPoints;
            if (textScalingMode == TextScalingMode.original)
            {
                pixelHeight = (ORIGINAL_FONT_SIZE / 72f) * Screen.dpi;
                fontSizeInPoints = ORIGINAL_FONT_SIZE;
            }
            else {
                if (textScalingMode == TextScalingMode.minScaling)
                {
                    pixelHeight = MIN_PIXEL_HEIGHT;
                }
                else
                {
                    pixelHeight = RECOMMENDED_MIN_PIXEL_HEIGHT;
                }
                fontSizeInPoints = pixelHeight / Screen.dpi * 72f; // 72 points per inch
            }
            foreach (TMP_Text textbox in scalableTextboxes)
            {
                textbox.fontSize = fontSizeInPoints;
            }
            textScalingInfo.text = $"{pixelHeight} pixels (at 720p), {fontSizeInPoints/72f:N2} inches, {fontSizeInPoints} points";

            Debug.Log($"Updated text scaling mode to {textScalingMode}");
        }

        void OnAudioConfigurationChanged(bool deviceWasChanged)
        {
            Debug.Log(deviceWasChanged ? "Audio device was changed" : "Reset was called on audio device");
        }
    }
}