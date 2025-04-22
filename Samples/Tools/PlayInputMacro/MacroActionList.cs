//-----------------------------------------------------------------------------
// MacroActionList.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Xml;

namespace PlayInputMacro
{
    public abstract class BaseMacroAction
    {
        public double StartTime { get; set; }

        public GamepadInput ButtonType { get; set; } = GamepadInput.None;
    }

    public class ButtonPressAction : BaseMacroAction
    {
        public double EndTime { get; set; }

        public ButtonPressAction(GamepadInput type, double startTime)
        {
            base.ButtonType = type;
            base.StartTime = startTime;
        }

        /// <summary>
        /// Reads an XML representation of an ButtonPressAction object converting it
        /// to a new ButtonPressAction object.
        /// </summary>
        /// <param name="type">GamepadButtonType indicating the type of this action</param>
        /// <param name="reader">XmlTextReader with the cursor just past the start Action element</param>
        /// <returns>ButtonPressAction</returns>
        internal static ButtonPressAction Deserialize(GamepadInput type, XmlTextReader reader)
        {
            string startTime = reader.GetAttribute("StartTime");
            string endTime = reader.GetAttribute("EndTime");

            ButtonPressAction buttonPress = new(type, double.Parse(startTime, CultureInfo.InvariantCulture))
            {
                EndTime = double.Parse(endTime, CultureInfo.InvariantCulture)
            };

            return buttonPress;
        }
    }

    public class AnalogAction : BaseMacroAction
    {
        public AnalogAction(GamepadInput type, double currentTime, short value)
        {
            base.ButtonType = type;
            base.StartTime = currentTime;
            Value = value;
        }

        public short Value { get; set; }

        /// <summary>
        /// Reads an XML representation of an AnalogAction object converting it
        /// to a new AnalogAction object.
        /// </summary>
        /// <param name="type">GamepadButtonType indicating the type of this action</param>
        /// <param name="reader">XmlTextReader with the cursor just past the start Action element</param>
        /// <returns>AnalogAction</returns>
        internal static AnalogAction Deserialize(GamepadInput type, XmlTextReader reader)
        {
            string time = reader.GetAttribute("Time");
            string value = reader.GetAttribute("Value");

            return new AnalogAction(type, double.Parse(time, CultureInfo.InvariantCulture), short.Parse(value, CultureInfo.InvariantCulture));            
        }
    }

    public class ActionList
    {
        // An ActionList tracks the actions for only a single type of button
        // (one of the button presses or one of the analog actions).
        private readonly GamepadInput _buttonType = GamepadInput.None;

        private ObservableCollection<BaseMacroAction> _actions = null;

        public ActionList(GamepadInput buttonType)
        {
            _buttonType = buttonType;
            _actions = [];
        }

        // This does a binary search to find the insertion index.  The actions
        // list is sorted (enforced by AddAction being the only place actions
        // are inserted into the list).
        int FindActionIndexForTime(double time)
        {
            int startIndex = 0;
            int endIndex = _actions.Count - 1;

            // continue searching while our endIndex is larger than our start
            // index.
            while (endIndex >= startIndex)
            {
                // Calculate an index in the middle
                int middleIndex = startIndex + ((endIndex - startIndex) / 2);

                if (_actions[middleIndex].StartTime == time)
                {
                    // action with this timestamp found at middleIndex
                    return middleIndex;
                }
                // determine which half this action belongs in
                else if (_actions[middleIndex].StartTime < time)
                {
                    // change startIndex index to search upper half
                    startIndex = middleIndex + 1;
                }
                else
                {
                    // change endIndex to search lower half
                    endIndex = middleIndex - 1;
                }
            }

            // endIndex is now less than startIndex, so startIndex contains the
            // entry right before our search time.
            return startIndex;
        }

        internal IEnumerable<AnalogAction> GetAnalogActionsInRange(double startTime, double endTime)
        {
            List<AnalogAction> actions = [];

            if (_actions.Count == 0 || _actions.First() is not AnalogAction)
            {
                return actions;
            }

            int firstIndex = FindActionIndexForTime(startTime);
            int lastIndex = FindActionIndexForTime(endTime) + 1;

            // Include an extra action to the left as a lead-in to the
            // actions in our range.
            if (firstIndex > 0)
            {
                --firstIndex;
            }

            for (int i = firstIndex; i < lastIndex && i < _actions.Count; ++i)
            {
                actions.Add(_actions[i] as AnalogAction);
            }

            return actions;
        }

        public IEnumerable<ButtonPressAction> GetButtonPressActionsInRange(double startTime, double endTime)
        {
            List<ButtonPressAction> actions = [];

            if (_actions.Count == 0 || _actions.First() is not ButtonPressAction)
            {
                return actions;
            }

            for (int i = 0; i < _actions.Count; ++i)
            {
                ButtonPressAction buttonPress = _actions[i] as ButtonPressAction;

                // This button ends at or before this start time, and starts before 
                // this end time.
                if (buttonPress.EndTime >= startTime &&
                    buttonPress.StartTime <= endTime)
                {
                    actions.Add(buttonPress);
                }
            }

            return actions;
        }

        internal bool ApplyChangesInRange(double startTime, double endTime, ref GamepadState state)
        {
            bool hasChanges = false;
            
            IEnumerable<ButtonPressAction> buttonActions = GetButtonPressActionsInRange(startTime, endTime);
            IEnumerable<AnalogAction> analogActions = GetAnalogActionsInRange(startTime, endTime);

            foreach (var buttonPress in buttonActions)
            {
                // Case 1: This is a new button press.
                if (buttonPress.StartTime >= startTime &&
                    buttonPress.StartTime <= endTime)
                {
                    hasChanges = true;
                    state.Buttons |= GamepadState.GetButtonValue(_buttonType);
                }
                // Case 2: This is a newly released button press.
                else if (buttonPress.EndTime >= startTime &&
                    buttonPress.EndTime <= endTime)
                {
                    hasChanges = true;
                }
                // Case 3: Nothing new, but button is held down this whole range.
                else if (buttonPress.StartTime < startTime &&
                    buttonPress.EndTime > endTime)
                {
                    state.Buttons |= GamepadState.GetButtonValue(_buttonType);
                }
                // Case 4: Do nothing (button doesn't overlap with this time period).
            }

            foreach (var analogAction in analogActions)
            {
                if (analogAction.StartTime >= startTime &&
                    analogAction.StartTime <= endTime)
                {
                    hasChanges = true;

                    switch (_buttonType)
                    {
                        case GamepadInput.LeftTrigger:
                            state.LeftTrigger = (byte)analogAction.Value;
                            break;
                        case GamepadInput.RightTrigger:
                            state.RightTrigger = (byte)analogAction.Value;
                            break;
                        case GamepadInput.LeftThumbX:
                            state.LeftThumbX = analogAction.Value;
                            break;
                        case GamepadInput.LeftThumbY:
                            state.LeftThumbY = analogAction.Value;
                            break;
                        case GamepadInput.RightThumbX:
                            state.RightThumbX = analogAction.Value;
                            break;
                        case GamepadInput.RightThumbY:
                            state.RightThumbY = analogAction.Value;
                            break;
                    }
                }
            }            

            return hasChanges;
        }

        internal void AddAction(BaseMacroAction action)
        {
            bool needsAdding = true;

            // This may need to merge with other button presses instead of just getting added.
            if (action is ButtonPressAction buttonPress)
            {
                bool couldContainOverlap = true;
                while (couldContainOverlap)
                {
                    couldContainOverlap = false;
                    for (int i = 0; i < _actions.Count; ++i)
                    {
                        // We start before this action, but end after or when it starts,
                        // Or we start after this action but start before or when it ends.
                        if (_actions[i] is ButtonPressAction priorAction &&
                            priorAction != buttonPress &&
                                ((priorAction.StartTime <= buttonPress.StartTime &&
                                    priorAction.EndTime >= buttonPress.StartTime) ||
                                (priorAction.StartTime >= buttonPress.StartTime &&
                                    priorAction.StartTime <= buttonPress.EndTime)))
                        {
                            // Combine these two actions
                            priorAction.StartTime = Math.Min(priorAction.StartTime, buttonPress.StartTime);
                            priorAction.EndTime = Math.Max(priorAction.EndTime, buttonPress.EndTime);

                            // Remove the now redundant action
                            if (_actions.Contains(buttonPress))
                            {
                                _actions.Remove(buttonPress);
                            }

                            // We don't need to add this button, but our newly modified button
                            // press could potentially overlap with another press so we need
                            // to start over searching for overlap.
                            needsAdding = false;
                            buttonPress = priorAction;
                            couldContainOverlap = true;
                            break;
                        }
                    }
                }
            }

            if (needsAdding)
            {
                // Optimization for empty action lists or adding to the end of
                // the action list
                if (_actions.Count == 0 || _actions.Last().StartTime < action.StartTime)
                {
                    needsAdding = false;
                    _actions.Add(action);
                }

                // Insert if this fits before some other action
                if (needsAdding)
                {
                    int insertionIndex = FindActionIndexForTime(action.StartTime);
                    _actions.Insert(insertionIndex, action);
                }
            }
        }
    }
}
