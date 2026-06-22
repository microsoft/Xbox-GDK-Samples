  ![](./media/image1.png)

#   FutexCombo Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

The traditional use for a spin-lock is to provide thread synchronization
when the lock is usually held for a small amount of time. With the
proper spin count the thread waiting on the lock can stay in user-mode
and acquire the lock in the minimum amount of time. However sometimes
the lock cannot be acquired within the spin time so the waiting thread
should allow other threads to utilize the CPU so overall work can
continue.

Many implementations of a spin-lock use
[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)
to give time to other ready threads on the core. However, the meaning
and use case for these functions is diametrically opposed to the meaning
and use case for a spin-lock. A spin-lock is used when a thread has
important work to do and needs to quickly acquire a lock so that it can
continue running. The
[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)
functions mean the thread has nothing else to do, so give the rest of
it's quantum to any other thread including a lower priority thread.
Depending on the current state of threads the title could easily see up
to a 30ms stall on a higher priority job threads leading to frame
stalls.

This sample provides an implementation of a spin-lock that uses
[WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle).
This allows the spinning thread to still give time to other ready
threads but also allows it to continue execution as soon as the lock can
be acquired. This leads to a much smoother execution time between
threads with the removal of the spikes caused by the use of
[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread).
The overall effect is more consistent work being done that matches the
planned priority between threads.

# Using the sample

The sample will continuously run various thread setups using different
spin-lock implementations. In each configuration it will calculate the
amount of work being performed on a set of foreground threads as well as
a set of background threads. On the console you can cycle between the
screens using the A button on the controller. On the Desktop the screens
will auto-cycle every 5 seconds.

# Implementation notes

Three different spin-lock implementations are being measured.

-   Slowtex -- An implementation that uses
    [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)

-   Futex -- An implementation that uses
    [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)

-   Nulltex -- An implementation that immediately calls
    [Sleep](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)
    with a zero timeout

    -   This is to show a worst-case scenario where the spin always
        fails

A total of eight threads are created with four foreground and four
background threads. There are two affinity configurations. The first is
that the threads are allowed to freely float across cores. The second is
that the threads are locked to a single core. A single foreground and a
single background thread will be locked to the same core.

The high priority threads sit in a loop counting the number of
operations they can perform. Every so often they will attempt to acquire
the spin-lock, hold it for some length of time, and then release it. The
background threads sit in the same loop counting the number of
operations; however, with the default variables they will not attempt to
acquire the spin-lock.

Three different contention levels are measured, high, medium, and low.
These control how often to acquire the spin-lock and how long to hold
the lock.

The spin time used by the locks as well as the time each thread holds
the lock can be controlled by adjusting the control variables defined at
the top of FutexTest.cpp.

# Results

This is an example of the data gathered from an Xbox Series X running at
3.8GHz with SMT disabled.

Futex spin-lock

| Contention  |  Priority  |  Affinity Locked  |  Affinity Floating        |
|----------------|-----------------|-----------------|-----------------|
| High           |  Foreground      |  1,695,453       |  1,705,016       |
|                |  Background      |  15,646          |  10,662          |
| Medium         |  Foreground      |  1,831,193       |  1,767,261       |
|                |  Background      |  2,352           |  3,566           |
| Low            |  Foreground      |  2,106,397       |  1,900,651       |
|                |  Background      |  538             |  2,086           |

Slowtex spin-lock

| Contention  |  Priority  |  Affinity Locked  |  Affinity Floating        |
|----------------|-----------------|-----------------|-----------------|
| High           |  Foreground      |  671,615         |  743,589         |
|                |  Background      |  1,191,006       |  494,248         |
| Medium         |  Foreground      |  929,646         |  992,124         |
|                |  Background      |  972,009         |  399,833         |
| Low            |  Foreground      |  1,248,495       |  1,173,585       |
|                |  Background      |  718,960         |  369,296         |

Nulltex spin-lock

| Contention  |  Priority  |  Affinity Locked  |  Affinity Floating        |
|----------------|-----------------|-----------------|-----------------|
| High           |  Foreground      |  18,782          |  428,358         |
|                |  Background      |  1,919,796       |  419,653         |
| Medium         |  Foreground      |  32,644          |  160,260         |
|                |  Background      |  1,880,733       |  988,944         |
| Low            |  Foreground      |  50,038          |  333,363         |
|                |  Background      |  1,887,807       |  1,132,100       |

The key numbers to look at here is the amount of work being done on the
Foreground thread, this represents the work that needs to be done for
the frame to continue processing. The Foreground thread when using a
spin-lock object implemented with
[WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)
consistently performs more work at all contention levels. This leads to
a more consistent frame rate as the critical work is completed faster.
The reason is because the threads are allowed to fully preempt based on
their priority.

The spin-lock object implemented using
[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)
only performs reasonably when threads are allowed to float between
cores. However even in that case due to the
[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)
behavior of giving the rest of its quantum to another thread and not
allowing the spinning thread to switch cores there is still wasted time.
This wasted time is given to the Background threads causing the work on
the Foreground thread to take longer to complete leading to more stalls
waiting for critical work to complete.

# Update history

Initial release August 2022

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
