//-----------------------------------------------------------------------------
// GamepadState.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;

namespace PlayInputMacro
{
    /// <summary>
    /// This gives us a single enum to describe all possible value types
    /// on the controller (binary and analog).
    /// </summary>
    public enum GamepadInput
    {
        None,

        AButton,
        BButton,
        XButton,
        YButton,

        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,

        LeftShoulder,
        RightShoulder,

        LeftThumbPress,
        RightThumbPress,

        HomeButton,
        ViewButton,
        MenuButton,

        LeftTrigger,
        RightTrigger,

        LeftThumbX,
        LeftThumbY,

        RightThumbX,
        RightThumbY,

        ShareButton,
    }

    public struct GamepadState
    {
        internal static ushort MaxTriggerValue = 1023;

        #region Accessors

        internal XInputButtonValues Buttons { get; set; }
        internal byte LeftTrigger { get; set; }
        internal byte RightTrigger { get; set; }
        internal short LeftThumbX { get; set; }
        internal short LeftThumbY { get; set; }
        internal short RightThumbX { get; set; }
        internal short RightThumbY { get; set; }

        #endregion

        #region Methods

        /// <summary>
        /// Checks if a button is pressed
        /// </summary>
        /// <param name="button">Button to check</param>
        /// <returns>Whether button was pressed</returns>
        internal readonly bool IsButtonPressed(XInputButtonValues button) => (Buttons & button) == button;

        /// <summary>
        /// Converts the internal XInput button mask format
        /// to an XtfButtonMask format for use by a console.
        /// </summary>
        /// <returns>XtfInput Button Mask</returns>
        internal readonly ConsoleButtonValues GetXtfInputButtonMask()
        {
            ConsoleButtonValues returnButtonMask = 0;

            if (IsButtonPressed(XInputButtonValues.DPadUp))
            {
                returnButtonMask |= ConsoleButtonValues.DPadUp;
            }
            if (IsButtonPressed(XInputButtonValues.DPadDown))
            {
                returnButtonMask |= ConsoleButtonValues.DPadDown;
            }
            if (IsButtonPressed(XInputButtonValues.DPadLeft))
            {
                returnButtonMask |= ConsoleButtonValues.DPadLeft;
            }
            if (IsButtonPressed(XInputButtonValues.DPadRight))
            {
                returnButtonMask |= ConsoleButtonValues.DPadRight;
            }
            if (IsButtonPressed(XInputButtonValues.Menu))
            {
                returnButtonMask |= ConsoleButtonValues.Menu;
            }
            if (IsButtonPressed(XInputButtonValues.View))
            {
                returnButtonMask |= ConsoleButtonValues.View;
            }
            if (IsButtonPressed(XInputButtonValues.LeftThumb))
            {
                returnButtonMask |= ConsoleButtonValues.LeftThumb;
            }
            if (IsButtonPressed(XInputButtonValues.RightThumb))
            {
                returnButtonMask |= ConsoleButtonValues.RightThumb;
            }
            if (IsButtonPressed(XInputButtonValues.LeftShoulder))
            {
                returnButtonMask |= ConsoleButtonValues.LeftShoulder;
            }
            if (IsButtonPressed(XInputButtonValues.RightShoulder))
            {
                returnButtonMask |= ConsoleButtonValues.RightShoulder;
            }
            if (IsButtonPressed(XInputButtonValues.Home))
            {
                returnButtonMask |= ConsoleButtonValues.Home;
            }
            if (IsButtonPressed(XInputButtonValues.AButton))
            {
                returnButtonMask |= ConsoleButtonValues.AButton;
            }
            if (IsButtonPressed(XInputButtonValues.BButton))
            {
                returnButtonMask |= ConsoleButtonValues.BButton;
            }
            if (IsButtonPressed(XInputButtonValues.XButton))
            {
                returnButtonMask |= ConsoleButtonValues.XButton;
            }
            if (IsButtonPressed(XInputButtonValues.YButton))
            {
                returnButtonMask |= ConsoleButtonValues.YButton;
            }

            return returnButtonMask;
        }

        /// <summary>
        /// Converts the internal XInput button mask format
        /// to an XtfMoreButtonMask format for use by a console.
        /// </summary>
        /// <returns>XtfInput More Button Mask</returns>
        internal readonly uint GetXtfMoreButtonMask()
        {
            uint returnButtonMask = 0;

            if (IsButtonPressed(XInputButtonValues.ShareButton))
            {
                returnButtonMask |= (uint)ConsoleMoreButtonValues.ShareButton;
            }

            return returnButtonMask;
        }

        // Convert a GamepadButtonType to an XInputButtonValue
        internal static XInputButtonValues GetButtonValue(GamepadInput type)
        {
            switch (type)
            {
                case GamepadInput.AButton:
                    return XInputButtonValues.AButton;
                case GamepadInput.BButton:
                    return XInputButtonValues.BButton;
                case GamepadInput.XButton:
                    return XInputButtonValues.XButton;
                case GamepadInput.YButton:
                    return XInputButtonValues.YButton;
                case GamepadInput.DPadUp:
                    return XInputButtonValues.DPadUp;
                case GamepadInput.DPadDown:
                    return XInputButtonValues.DPadDown;
                case GamepadInput.DPadLeft:
                    return XInputButtonValues.DPadLeft;
                case GamepadInput.DPadRight:
                    return XInputButtonValues.DPadRight;
                case GamepadInput.HomeButton:
                    return XInputButtonValues.Home;
                case GamepadInput.ViewButton:
                    return XInputButtonValues.View;
                case GamepadInput.MenuButton:
                    return XInputButtonValues.Menu;
                case GamepadInput.LeftShoulder:
                    return XInputButtonValues.LeftShoulder;
                case GamepadInput.RightShoulder:
                    return XInputButtonValues.RightShoulder;
                case GamepadInput.LeftThumbPress:
                    return XInputButtonValues.LeftThumb;
                case GamepadInput.RightThumbPress:
                    return XInputButtonValues.RightThumb;
                case GamepadInput.ShareButton:
                    return XInputButtonValues.ShareButton;
                default:
                    return XInputButtonValues.None;
            }
        }

        #endregion

        #region Button Value Mask used by the PC (XInput)

        /// <summary>
        /// Possible flags for buttons mask
        /// These must match XINPUT_GAMEPAD_xx defined in xinput.h
        /// </summary>
        [Flags]
        public enum XInputButtonValues
        {
            /// <summary>
            /// No buttons
            /// </summary>
            None = 0,

            /// <summary>
            /// DPad up
            /// </summary>
            DPadUp = 0x0001,

            /// <summary>
            /// DPad down
            /// </summary>
            DPadDown = 0x0002,

            /// <summary>
            /// DPad left
            /// </summary>
            DPadLeft = 0x0004,

            /// <summary>
            /// DPad right
            /// </summary>
            DPadRight = 0x0008,

            /// <summary>
            /// Start or Menu
            /// </summary>
            Menu = 0x0010,

            /// <summary>
            /// Back or View
            /// </summary>
            View = 0x0020,

            /// <summary>
            /// Left thumb press
            /// </summary>
            LeftThumb = 0x0040,

            /// <summary>
            /// Right thumb press
            /// </summary>
            RightThumb = 0x0080,

            /// <summary>
            /// Left shoulder
            /// </summary>
            LeftShoulder = 0x0100,

            /// <summary>
            /// Right shoulder
            /// </summary>
            RightShoulder = 0x0200,

            /// <summary>
            /// Home Button
            /// </summary>
            Home = 0x0400,

            /// <summary>
            /// A button
            /// </summary>
            AButton = 0x1000,

            /// <summary>
            /// B button
            /// </summary>
            BButton = 0x2000,

            /// <summary>
            /// X button
            /// </summary>
            XButton = 0x4000,

            /// <summary>
            /// Y button
            /// </summary>
            YButton = 0x8000,

            /// <summary>
            /// All values after this button are not supported by XInput
            /// </summary>
            MoreButtonsMask = 0x7FFF0000,

            /// <summary>
            /// Share button (not supported by XInput)
            /// </summary>
            ShareButton = 0x10000,
        }        

        #endregion

        #region Button Value Mask used by the Console (XtfInput)

        /// <summary>
        /// These are very analogous to the button mask values used by XInput
        /// but XtfInput uses a different order for the buttons.
        /// Note: Share button is NOT here
        /// This must match GAMEPAD_BUTTONS in xtfinput.idl
        /// </summary>
        [Flags]
        internal enum ConsoleButtonValues : ushort
        {
            RightThumb = 0x8000,
            LeftThumb = 0x4000,
            RightShoulder = 0x2000,
            LeftShoulder = 0x1000,
            DPadRight = 0x800,
            DPadLeft = 0x400,
            DPadDown = 0x200,
            DPadUp = 0x100,
            YButton = 0x80,
            XButton = 0x40,
            BButton = 0x20,
            AButton = 0x10,
            View = 0x8,
            Menu = 0x4,
            Home = 0x2,
            Enroll = 0x1, // Unused as XInput doesn't provide this
        }

        /// <summary>
        /// This is used by XtfInput for the additional, non-XInput buttons
        /// This must match GAMEPAD_MORE_BUTTONS in xtfinput.idl
        /// </summary>
        [Flags]
        internal enum ConsoleMoreButtonValues : uint
        {
            ShareButton = 1,
        }

        #endregion

        #region Overridden Equals

        public override bool Equals(object obj)
        {
            if (obj is not GamepadState)
            {
                return false;
            }

            GamepadState state = (GamepadState)obj;

            if (state.Buttons != Buttons ||
                state.LeftThumbX != LeftThumbX ||
                state.LeftThumbY != LeftThumbY ||
                state.RightThumbX != RightThumbX ||
                state.RightThumbY != RightThumbY ||
                state.LeftTrigger != LeftTrigger ||
                state.RightTrigger != RightTrigger)
            {
                return false;
            }

            return true;
        }

        // If we override the Equals method, we also have to override
        // the GetHashCode method to avoid our compiler firing an error.
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
        public static bool operator ==(GamepadState left, GamepadState right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(GamepadState left, GamepadState right)
        {
            return !(left == right);
        }

        #endregion
    }
}
