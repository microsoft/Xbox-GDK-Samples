using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System.Runtime.InteropServices;
using UnityEditor.SceneManagement;
using System;

[InitializeOnLoad]
public class ATGTelemetry
{
    delegate uint InitDelegate();
    delegate uint SendEventDelegate(string s);

    static ATGTelemetry()
    {
#if UNITY_EDITOR_WIN
        if (EditorApplication.timeSinceStartup < 10)
        {
            var lib = ATGTelemetryDLL.LoadLibrary(Application.dataPath + "/ATGTelemetry/ATGTelemetry.dll");

            if (lib == IntPtr.Zero)
            {
                return;
            }

            var initTelemetry = ATGTelemetryDLL.GetProcAddress(lib, "InitializeTelemetry");
            if (initTelemetry == IntPtr.Zero)
            {
                return;
            }
            ((InitDelegate)Marshal.GetDelegateForFunctionPointer(initTelemetry,typeof(InitDelegate)))();
            
            var sampleLoadedEvent = ATGTelemetryDLL.GetProcAddress(lib, "EventWriteSampleLoaded");
            if( sampleLoadedEvent == IntPtr.Zero)
            {
                return;
            }
            string[] s = Application.dataPath.Split('/');
            string projectName = s[s.Length - 2];
            ((SendEventDelegate)Marshal.GetDelegateForFunctionPointer(sampleLoadedEvent, typeof(SendEventDelegate)))(projectName);

            ATGTelemetryDLL.FreeLibrary(lib);
#endif
        }
    }
}

public static class ATGTelemetryDLL
{
    [DllImport("kernel32.dll")]
    public static extern IntPtr LoadLibrary(string dllToLoad);

    [DllImport("kernel32.dll")]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr hModule);
}
