//-----------------------------------------------------------------------------
// NativeMethods.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;

namespace PlayInputMacro
{
    internal static partial class NativeMethods
    {
        [LibraryImport(@"XtfInput.dll", EntryPoint = "XtfCreateInputClient", StringMarshalling = StringMarshalling.Utf16)]
        [UnmanagedCallConv(CallConvs = new Type[] { typeof(CallConvStdcall) })]
        public static partial int XtfCreateInputClient(string address, ref Guid riid, out IntPtr ppvObject);
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct GAMEPAD_REPORT
    {
        public ushort Buttons;
        public ushort LeftTrigger;
        public ushort RightTrigger;
        public short LeftThumbstickX;
        public short LeftThumbstickY;
        public short RightThumbstickX;
        public short RightThumbstickY;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct GAMEPAD_REPORT_EX
    {
        public ushort Buttons;
        public ushort LeftTrigger;
        public ushort RightTrigger;
        public short LeftThumbstickX;
        public short LeftThumbstickY;
        public short RightThumbstickX;
        public short RightThumbstickY;
        public uint MoreButtons;
    }

    [GeneratedComInterface]
    [Guid("167B5DB0-F32C-487C-AEFD-8AF3DA6284FA")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public partial interface IXtfInputClient
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void ConnectGamepad(out ulong pControllerId);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void DisconnectAllGamepads();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void DisconnectGamepad(ulong controllerId);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void SendGamepadReport(ulong controllerId, GAMEPAD_REPORT report);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void SendGamepadReportEx(ulong controllerId, GAMEPAD_REPORT_EX report);
    }
}
