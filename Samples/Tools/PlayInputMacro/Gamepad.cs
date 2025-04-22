//-----------------------------------------------------------------------------
// Gamepad.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma warning disable CA2002

using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using System.Threading;

namespace PlayInputMacro
{
    /// <summary>
    /// This class reflects a gamepad's connection to a specific console (NOT a gamepad device as you might expect from the name)
    /// At creation time it is bound to a specific console (or none), and that cannot change
    /// </summary>
    internal class Gamepad : IDisposable
    {
        public const int MaxVirtualControllers = 4;

        #region Private members
        private IXtfInputClient _xtfInputClient;
        private readonly GamepadState[] _lastSentState;
        private bool _isConnectedToConsole;
        private readonly ulong[] _controllerIds;
        private readonly string _address;
        private IntPtr _ppvObj;
        #endregion

        #region Constructor

        public Gamepad(string address)
        {
            _address = address;
            _lastSentState = new GamepadState[MaxVirtualControllers];
            _controllerIds = new ulong[MaxVirtualControllers];
            _isConnectedToConsole = false;
            _xtfInputClient = null;
        }

        #endregion

        #region Destructor

        ~Gamepad()
        {
            Dispose(false);
        }

        #endregion

        #region Public Accessors

        internal bool ConnectedToConsole
        {
            get { return _isConnectedToConsole; }
        }

        #endregion


        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void Diagnostic(string name, string value, [CallerFilePath] string file = "", [CallerMemberName] string function = "", [CallerLineNumber] int line = 0)
        {
            Console.WriteLine("{0}: {1}", name, value);
        }

        #region Public Methods

        // Given a controller state, merge it into the "final" state, last-one-wins for each component
        internal void ResetConsoleState()
        {
            if (!_isConnectedToConsole)
            {
                // Nothing to do here if we have never connected
                Diagnostic("ResetConsoleStateSkipped", _address);
                return;
            }

            Diagnostic("ResetConsoleState", _address);

            lock (this)
            {
                Dispose(true);
            }
        }

        public void SendStateToConsole(GamepadState stateToSend)
        {
            if (!_isConnectedToConsole)
            {
                throw new COMException("Controller is not connected to console.");
            }

            SendStateToConsole(0, stateToSend);
        }

        private void SendStateToConsole(int which, GamepadState stateToSend)
        {
            if (stateToSend.Equals(_lastSentState[which]))
            {
                return;
            }

            GAMEPAD_REPORT_EX report = new GAMEPAD_REPORT_EX();

            // Buttons
            if (stateToSend.Buttons != GamepadState.XInputButtonValues.None)
            {
                report.Buttons = (ushort)stateToSend.GetXtfInputButtonMask();
                report.MoreButtons = stateToSend.GetXtfMoreButtonMask();
            }

            // Triggers -- XInput stores these as bytes, they should actually be
            // ushorts, but the max value is actually 1023 (2.5 bytes), so we need
            // to scale the values here.
            report.LeftTrigger = (ushort)(((double)stateToSend.LeftTrigger / byte.MaxValue) * GamepadState.MaxTriggerValue);
            report.RightTrigger = (ushort)(((double)stateToSend.RightTrigger / byte.MaxValue) * GamepadState.MaxTriggerValue);

            // Thumbsticks
            report.LeftThumbstickX = stateToSend.LeftThumbX;
            report.LeftThumbstickY = stateToSend.LeftThumbY;

            report.RightThumbstickX = stateToSend.RightThumbX;
            report.RightThumbstickY = stateToSend.RightThumbY;

            lock (this)
            {
                if (_xtfInputClient != null)
                {
                    _xtfInputClient.SendGamepadReportEx(_controllerIds[which], report);
                    _lastSentState[which] = stateToSend;
                }
            }
        }

        public void ConnectToConsole()
        {
            // Acquire the lock for this entire method so the client
            // can't attempt to send data while we are connecting.
            lock (this)
            {
                // There is no console associated with this gamepad
                if (_address == null)
                {
                    return;
                }

                if (_xtfInputClient != null)
                {
                    // Dispose our prior connection before creating a new one
                    Dispose(true);
                }

                Guid riid = typeof(IXtfInputClient).GUID;
                if (NativeMethods.XtfCreateInputClient(_address, ref riid, out _ppvObj) < 0)
                {
                    throw new COMException("Unable to connect to " + _address);
                }

                ComWrappers cw = new StrategyBasedComWrappers();
                _xtfInputClient = (IXtfInputClient)cw.GetOrCreateObjectForComInstance(_ppvObj, CreateObjectFlags.None);
                AddVirtualControllerToConsole();
            }
        }

        /// <summary>
        /// lock needs to be taken by caller
        /// </summary>
        private void AddVirtualControllerToConsole()
        {
            try
            {
                int index = 0;
                Diagnostic("AddVirtualController", index.ToString());
                Debug.Assert(_controllerIds[index] == 0);
                _xtfInputClient.ConnectGamepad(out _controllerIds[index]);
                _isConnectedToConsole = true;
            }
            catch (Exception exception)
            {
                Diagnostic("AddVirtualController.Failed", exception.Message);
                throw;
            }

            // According to XDK docs, you should wait 2 seconds after calling ConnectGamepad
            // before allowing calls to SendGamepadReport to ensure the console has finished
            // registering the new controller.
            Thread.Sleep(2000);
        }
        #endregion

        #region IDisposable

        public void Dispose()
        {
            lock (this)
            {
                Dispose(true);
            }

            //
            // Avoid cleaning up twice.  Calling Dispose(true)
            // implies that managed objects will be cleaned up as well
            // as unmanaged objects so the garbage collector will
            // have no work to do when finalizing this object.
            //
            GC.SuppressFinalize(this);
        }

        public static void ReleaseComObject<T>(ref T obj) where T : class
        {
            if(obj != null)
            {
                Marshal.ReleaseComObject(obj);
                obj = null;
            }
        }

        private void Dispose(bool disposing)
        {
            //
            // Since the garbage collector's Finalize() runs on
            // a background thread, managed objects are not safe
            // to reference.  Only clean up managed objects if this
            // is being explicitly disposed.
            //
            // if (disposing)
            // {
            // }

            if (_xtfInputClient != null)
            {
                try
                {
                    if (_isConnectedToConsole)
                    {
                        _xtfInputClient.DisconnectAllGamepads();
                    }
                }
                catch (Exception e)
                {
                    // Do nothing.  This Disconnect call can fail if the console
                    // gets rebooted while the controller is connected.
                    Debug.WriteLine(e.GetType() + ": " + e.Message);
                }

                try
                {
                    // free unmanaged objects
                    Marshal.Release(_ppvObj);
                    _xtfInputClient = null;
                }
                catch (Exception e)
                {
                    // Do nothing. We'll make this best effort to avoid leaking memory.
                    Debug.WriteLine(e.GetType() + ": " + e.Message);
                }
            }

            for (int i = 0; i < _controllerIds.Length; i++)
            {
                _controllerIds[i] = 0;
            }
            _isConnectedToConsole = false;
        }

        #endregion
    }
}
