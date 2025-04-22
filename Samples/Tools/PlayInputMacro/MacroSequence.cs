//-----------------------------------------------------------------------------
// MacroSequence.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Xml;

namespace PlayInputMacro
{
    public sealed class MacroSequence : IDisposable
    {
        #region private variables

        // If we increment this version number, we need to address the load
        // method to ensure it is backwards compatible with earlier versions.
        private static readonly string CurrentVersion = "1.0";

        private Dictionary<GamepadInput, ActionList> _actionLists = null;

        // We keep an open handle to the backing file to avoid deletions.
        private FileStream _openHandle = null;

        #endregion       

        #region Constructors

        internal MacroSequence()
        {
            Reset();
        }

        internal MacroSequence(string filename)
        {
            Reset();

            FileName = filename;

            LoadMacroFromFile();
        }

        #endregion

        #region Accessors

        private double _currentTime = 0;
        public double CurrentTime
        {
            get => _currentTime;
            set
            {
                _currentTime = value;

                if (SequenceLength < _currentTime)
                {
                    SequenceLength = _currentTime;
                }
            }
        }

        public bool IsLooping { get; set; } = false;
        public double SequenceLength { get; set; } = 0;
        public bool IsAtSequenceEnd => _currentTime >= SequenceLength;
        public string FileName { get; set; } = null;

        #endregion

        #region Public Methods

        internal void MoveToBeginning()
        {
            _currentTime = 0;
        }

        internal bool GetNextActions(double millisecondsPassed, ref GamepadState state)
        {
            if (_currentTime >= SequenceLength)
            {
                if (IsLooping)
                {
                    MoveToBeginning();
                }
                return false;
            }

            bool foundChanges = false;
            double startTime = _currentTime;

            _currentTime += millisecondsPassed;

            foreach (ActionList actions in _actionLists.Values)
            {
                foundChanges = actions.ApplyChangesInRange(startTime, _currentTime, ref state) || foundChanges;
            }

            return foundChanges;
        }
        #endregion

        #region Private methods

        private void Reset()
        {
            SequenceLength = 0;
            _currentTime = 0;

            // Create all of our action lists
            _actionLists = new Dictionary<GamepadInput, ActionList>
            {
                { GamepadInput.AButton,         new ActionList(GamepadInput.AButton) },
                { GamepadInput.BButton,         new ActionList(GamepadInput.BButton) },
                { GamepadInput.XButton,         new ActionList(GamepadInput.XButton) },
                { GamepadInput.YButton,         new ActionList(GamepadInput.YButton) },
                { GamepadInput.DPadDown,        new ActionList(GamepadInput.DPadDown) },
                { GamepadInput.DPadLeft,        new ActionList(GamepadInput.DPadLeft) },
                { GamepadInput.DPadRight,       new ActionList(GamepadInput.DPadRight) },
                { GamepadInput.DPadUp,          new ActionList(GamepadInput.DPadUp) },
                { GamepadInput.MenuButton,      new ActionList(GamepadInput.MenuButton) },
                { GamepadInput.ViewButton,      new ActionList(GamepadInput.ViewButton) },
                { GamepadInput.HomeButton,      new ActionList(GamepadInput.HomeButton) },
                { GamepadInput.LeftShoulder,    new ActionList(GamepadInput.LeftShoulder) },
                { GamepadInput.RightShoulder,   new ActionList(GamepadInput.RightShoulder) },
                { GamepadInput.LeftThumbPress,  new ActionList(GamepadInput.LeftThumbPress) },
                { GamepadInput.RightThumbPress, new ActionList(GamepadInput.RightThumbPress) },
                { GamepadInput.LeftThumbX,      new ActionList(GamepadInput.LeftThumbX) },
                { GamepadInput.LeftThumbY,      new ActionList(GamepadInput.LeftThumbY) },
                { GamepadInput.RightThumbX,     new ActionList(GamepadInput.RightThumbX) },
                { GamepadInput.RightThumbY,     new ActionList(GamepadInput.RightThumbY) },
                { GamepadInput.LeftTrigger,     new ActionList(GamepadInput.LeftTrigger) },
                { GamepadInput.RightTrigger,    new ActionList(GamepadInput.RightTrigger) }
            };
        }

        private void LoadMacroFromFile()
        {
            if (_openHandle != null)
            {
                _openHandle.Dispose();
                _openHandle = null;
            }

            using (XmlTextReader reader = new(FileName))
            {
                reader.DtdProcessing = DtdProcessing.Ignore;
                bool foundCollection = false;

                while (reader.Read())
                {
                    // Parse the document until we find the ControllerInputCollection element.
                    if (reader.Name.Equals("ControllerInputCollection", StringComparison.OrdinalIgnoreCase))
                    {
                        foundCollection = true;
                    }
                    else if (foundCollection && reader.NodeType == XmlNodeType.Element)
                    {
                        if (reader.Name.Equals("MacroInfo", StringComparison.OrdinalIgnoreCase))
                        {
                            SequenceLength = double.Parse(reader.GetAttribute("MacroLength"), CultureInfo.InvariantCulture);

                            string version = reader.GetAttribute("Version");

                            if (!version.Equals(CurrentVersion))
                            {
                                throw new XmlException("Unable to parse a macro file with this version: " + version);
                            }
                        }
                        else
                        {
                            GamepadInput currentButtonType = GamepadInput.None;
                            foreach (GamepadInput buttonType in _actionLists.Keys)
                            {
                                if (reader.Name.Equals(buttonType.ToString(), StringComparison.OrdinalIgnoreCase))
                                {
                                    currentButtonType = buttonType;
                                }
                            }

                            if (currentButtonType == GamepadInput.None)
                            {
                                throw new XmlException("Unexpected Node reached on line '" + reader.LineNumber + "' with value '" + reader.Name + "'.");
                            }

                            try
                            {
                                switch (currentButtonType)
                                {
                                    case GamepadInput.LeftThumbX:
                                    case GamepadInput.LeftThumbY:
                                    case GamepadInput.RightThumbX:
                                    case GamepadInput.RightThumbY:
                                    case GamepadInput.LeftTrigger:
                                    case GamepadInput.RightTrigger:
                                        AnalogAction analogAction = AnalogAction.Deserialize(currentButtonType, reader);
                                        _actionLists[currentButtonType].AddAction(analogAction);
                                        break;
                                    default:
                                        ButtonPressAction buttonPressAction = ButtonPressAction.Deserialize(currentButtonType, reader);
                                        _actionLists[currentButtonType].AddAction(buttonPressAction);
                                        break;
                                }
                            }
                            catch (FormatException)
                            {
                                throw new XmlException("Unable to parse value on line '" + reader.LineNumber + "'.");
                            }
                            catch (ArgumentNullException)
                            {
                                throw new XmlException("Failed to find expected attributes on line '" + reader.LineNumber + "'.");
                            }
                        }
                    }
                }

                if (!foundCollection)
                {
                    throw new XmlException("Failed to find root ControllerInputCollection element.");
                }

                reader.Close();
            }

            // Open a shared handle to prevent file deletions.
            _openHandle = File.Open(FileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        }

        #endregion

        #region IDisposable

        public void Dispose()
        {
            GC.SuppressFinalize(this);
            if (_openHandle != null)
            {
                _openHandle.Dispose();
                _openHandle = null;
            }
        }

        #endregion
    }
}
