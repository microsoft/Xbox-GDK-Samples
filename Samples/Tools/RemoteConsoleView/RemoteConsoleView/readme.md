# RemoteConsoleView Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample tool shows how to consume the VideoStreamingControl from a
WPF app, to show console content remotely on a PC, in a similar way to
the Xbox One Manager.

When you run the sample, enter the name (or IP address) of a console,
click the Connect button, and the console display should render.

The first time you run the app you will likely get a Firewall prompt:
the app needs network access to get to the remote console.

If you wish to use this control in your own WPF projects, do the
following:

-   Ensure your project builds 64-bit (the control is only available in
    64-bit version)

-   Add a reference to `C:\Program Files (x86)\Microsoft
    GDK\bin\Microsoft.Xbox.Tools.RemoteVideo.dll` (which comes with the
    GDK installation)

-   Add this file to your Project: `C:\Program Files
    (x86)\Microsoft GDK\bin\xtfremotevideo.dll` and ensure *Copy
    to Output Directory* is set to *Copy if newer*. This file must exist
    next to the exe for the control to work.

-   Add a XAML reference to the control, see the sample for details.

## VideoStreamingControl

The control is pretty straight-forward to use. Set the *Source* property
to the name of the console (or ip address), and it will start as soon as
connected (*AutoPlay* defaults to True). It has a *Stop* method, and you
can control volume via the *Volume* and *IsMuted* properties. If errors
occur, the *Status* and *StatusMessage* properties can be bound to. Note
that this control uses an *HWndHost* for rendering, which means that you
cannot render other WPF controls over the top of it (due to the airspace
issue).

# Update history

Initial release March 2020.
