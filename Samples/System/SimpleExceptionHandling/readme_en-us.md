  ![](./media/image1.png)

#   SimpleExceptionHandling Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample shows several different ways handle exceptions that can
occur in your title.

-   Unhandled Exception Filter -- Demonstrates how to use an [Unhandled
    Exception
    Filter](https://docs.microsoft.com/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter)
    to catch and process general exceptions your title.

-   Structured Exceptions -- Demonstrates how to use the [Structured
    Exception
    Handling](https://docs.microsoft.com/cpp/cpp/structured-exception-handling-c-cpp)
    system.

-   Vectored Exception Handler -- Demonstrates how to use the [Vectored
    Exception
    Handling](https://docs.microsoft.com/windows/win32/debug/vectored-exception-handling)
    system.

-   C++ Language Exceptions -- Demonstrates how to use the exception
    system built into the [C++
    Language](https://docs.microsoft.com/cpp/cpp/try-throw-and-catch-statements-cpp).

-   Recommended pattern -- Demonstrates a recommended pattern that uses
    a combination of the other systems.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using the Desktop, set the active solution platform to `x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press the corresponding button on the controller for each demonstration.
The display will show the order of operations that happen in the code
when an exception is raised.

Note: The Unhandled Exception Filter example will behave differently if
there is a debugger attached, there are additional details in the
comments.

# Implementation notes

All the examples are contained in the Examples folder. They are heavily
documented with details on each system and how they work.

# Update history

Initial release April 2021

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
