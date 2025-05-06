//-----------------------------------------------------------------------------
// Program.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace PlayInputMacro
{
    class Program
    {
        private const double InputPlayRate = (1.0 / 60.0) * 1000.0; // 60 fps, 16.667 ms
        private const double DefaultRetryDelayForSendStateFailures = 2000.0; // 2 seconds

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: PlayInputMacro.exe <ConsoleIP> <MacroFile>");
                return;
            }

            Gamepad gamepad = new(args[0]);

            double delay = 0.0;
            DateTime lastPoll = DateTime.Now;
            MacroSequence sequence;

            try
            {
                Console.WriteLine($"Loading macro from {args[1]}");
                sequence = new MacroSequence(args[1]);
            }
            catch(Exception e)
            {
                Console.WriteLine($"Failed to load macro file '{args[1]}': {e.Message}");
                return;
            }

            sequence.MoveToBeginning();

            Console.WriteLine("Starting macro playback");
            while (!sequence.IsAtSequenceEnd)
            {
                double timeDelta = (DateTime.Now - lastPoll).TotalMilliseconds;

                if (timeDelta < InputPlayRate)
                {
                    Thread.Sleep((int)(InputPlayRate - timeDelta));
                }

                if (delay > 0)
                {
                    delay -= Math.Max(timeDelta, InputPlayRate);
                }

                GamepadState state = new();

                if (!sequence.GetNextActions(Math.Max(timeDelta, InputPlayRate), ref state))
                {
                    // No changes to Gamepad Input, Prepare for next iteration
                    lastPoll = DateTime.Now;
                    continue;
                }

                if (delay <= 0)
                {
                    // Add a single immediate retry to catch console reboot cases
                    // (eg, if the console reboots out from under us, the next
                    // time we try to send input our virtual controller won't exist
                    // and we'll need to create a new one).
                    int retries = 1;
                    bool completedSend = false;

                    do
                    {
                        try
                        {
                            if (!gamepad.ConnectedToConsole)
                            {
                                gamepad.ConnectToConsole();
                            }

                            gamepad.SendStateToConsole(state);

                            completedSend = true;
                        }
                        catch (COMException ex)
                        {
                            Console.WriteLine(ex);
                            gamepad.ResetConsoleState();
                        }

                        --retries;

                    } while (!completedSend && retries >= 0);

                    if (!completedSend)
                    {
                        // Add a delay before attempting again if we fail to
                        // send this input to the console.
                        // To avoid attempting to connect to the console too
                        // frequently we will delay sending state for a bit.
                        delay = DefaultRetryDelayForSendStateFailures;
                    }
                }

                // Prepare for next iteration
                lastPoll = DateTime.Now;
            }

            Console.WriteLine("Macro playback complete");
        }
    }
}
