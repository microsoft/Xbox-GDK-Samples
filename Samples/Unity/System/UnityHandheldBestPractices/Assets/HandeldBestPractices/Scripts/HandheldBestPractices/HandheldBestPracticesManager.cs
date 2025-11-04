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
        public delegate void KeyboardCallback();

        [DllImport("HandheldHelper")] static extern IntPtr GetDeviceManufacturer();
        [DllImport("HandheldHelper")] static extern IntPtr GetDeviceProductName();
        [DllImport("HandheldHelper")] static extern IntPtr GetDeviceSystemFamily();
        [DllImport("HandheldHelper")] static extern IntPtr GetDeviceBaseboardProductName();
        [DllImport("HandheldHelper")] static extern uint GetPageFaultCount();
        [DllImport("HandheldHelper")] static extern UIntPtr GetWorkingSetSize();
        [DllImport("HandheldHelper")] static extern double GetScreenSize();
        [DllImport("HandheldHelper")] static extern bool IsDeviceHandheld();
        [DllImport("HandheldHelper")] static extern bool IsBluetoothEnabled();
        [DllImport("HandheldHelper")] static extern bool ShowVirtualKeyboard();
        [DllImport("HandheldHelper")] static extern bool HideVirtualKeyboard();
        [DllImport("HandheldHelper")] static extern bool IsVirtualKeyboardOverlayed();
        [DllImport("HandheldHelper")] static extern void RegisterKeyboardShowingEvent(KeyboardCallback callback);
        [DllImport("HandheldHelper")] static extern void RegisterKeyboardHidingEvent(KeyboardCallback callback);
        [DllImport("HandheldHelper")] static extern void UnregisterKeyboardShowingEvent();
        [DllImport("HandheldHelper")] static extern void UnregisterKeyboardHidingEvent();

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

        // Callback delegates - keep references to prevent garbage collection
        private static KeyboardCallback keyboardShowingCallback;
        private static KeyboardCallback keyboardHidingCallback;

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
            UnregisterKeyboardCallbacks();
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
            RefreshInfoText();

            UpdateTextScaling(TextScalingMode.original);

            RegisterKeyboardCallbacks();
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

            supportedResolutions.text = GetSupportedResolutionsString();
            networkAdapters.text = GetNetworkAdaptersString();
        }

        public static void ShowKeyboard()
        {
            bool b = ShowVirtualKeyboard();
            Debug.Log($"Virtual keyboard: {b}");
        }

        public static void HideKeyboard()
        {
            HideVirtualKeyboard();
        }

        // Register callbacks for virtual keyboard show/hide events
        private void RegisterKeyboardCallbacks()
        {
            try
            {
                keyboardShowingCallback = OnKeyboardShowing;
                keyboardHidingCallback = OnKeyboardHiding;

                // Register the callbacks with the DLL
                RegisterKeyboardShowingEvent(keyboardShowingCallback);
                RegisterKeyboardHidingEvent(keyboardHidingCallback);

                Debug.Log("Virtual keyboard callbacks registered successfully");
            }
            catch (System.Exception ex)
            {
                Debug.LogError($"Failed to register keyboard callbacks: {ex.Message}");
            }
        }

        // Unregister callbacks for virtual keyboard show/hide events
        private void UnregisterKeyboardCallbacks()
        {
            try
            {
                UnregisterKeyboardShowingEvent();
                UnregisterKeyboardHidingEvent();

                keyboardShowingCallback = null;
                keyboardHidingCallback = null;

                Debug.Log("Virtual keyboard callbacks unregistered successfully");
            }
            catch (System.Exception ex)
            {
                Debug.LogError($"Failed to unregister keyboard callbacks: {ex.Message}");
            }
        }

        // Callback method called when virtual keyboard is showing
        [AOT.MonoPInvokeCallback(typeof(KeyboardCallback))]
        private static void OnKeyboardShowing()
        {
            Debug.Log("VK Showing");
        }

        // Callback method called when virtual keyboard is hiding
        [AOT.MonoPInvokeCallback(typeof(KeyboardCallback))]
        private static void OnKeyboardHiding()
        {
            Debug.Log("VK Hiding");
        }

        // Check if virtual keyboard is currently overlayed (polling method)
        public static bool IsKeyboardOverlayed()
        {
            try
            {
                return IsVirtualKeyboardOverlayed();
            }
            catch (System.Exception ex)
            {
                Debug.LogError($"Failed to check keyboard overlay status: {ex.Message}");
                return false;
            }
        }

        private string GetInfoStr()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("<b>Device Info</b>");
            string[] deviceModelExtract = SystemInfo.deviceModel.Split('(', ')');

            // OEM Info
            sb.AppendLine($"  Manufacturer: {(deviceModelExtract.Length > 1 ? deviceModelExtract[1].Trim() : "N/A")}");
            sb.AppendLine($"  Product Name: {(deviceModelExtract.Length > 0 ? deviceModelExtract[0].Trim() : "N/A")}");
            IntPtr familyPtr = GetDeviceSystemFamily();
            IntPtr baseboardPtr = GetDeviceBaseboardProductName();
            string systemFamily = familyPtr != IntPtr.Zero ? Marshal.PtrToStringUni(familyPtr) : "N/A";
            string baseboardProductName = baseboardPtr != IntPtr.Zero ? Marshal.PtrToStringUni(baseboardPtr) : "N/A";
            sb.AppendLine($"  System Family: {systemFamily}");
            sb.AppendLine($"  Baseboard Product Name: {baseboardProductName}");

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
            sb.AppendLine($"  Key Pressed: {(Keyboard.current == null ? "no keyboard connected" : Keyboard.current.allKeys.FirstOrDefault(k => k == null ? false : k.isPressed))}");
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

        public void RefreshInfoText()
        {
            deviceInfo.text = GetInfoStr();
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
            float actualPixelHeight;

            if (textScalingMode == TextScalingMode.original)
            {
                pixelHeight = (ORIGINAL_FONT_SIZE / 72f) * Screen.dpi;
                fontSizeInPoints = ORIGINAL_FONT_SIZE;
                actualPixelHeight = pixelHeight;
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
                // Calculate actual pixel height at current resolution
                // The constants are based on 720p, so scale proportionally
                actualPixelHeight = pixelHeight * (Screen.currentResolution.height / 720f);
            }
            foreach (TMP_Text textbox in scalableTextboxes)
            {
                textbox.fontSize = fontSizeInPoints;
            }
            textScalingInfo.text = $"{actualPixelHeight:F1} pixels (at {Screen.currentResolution.height}p), {fontSizeInPoints/72f:N2} inches, {fontSizeInPoints} points";

            Debug.Log($"Updated text scaling mode to {textScalingMode}");
        }

        void OnAudioConfigurationChanged(bool deviceWasChanged)
        {
            Debug.Log(deviceWasChanged ? "Audio device was changed" : "Reset was called on audio device");
        }
    }
}