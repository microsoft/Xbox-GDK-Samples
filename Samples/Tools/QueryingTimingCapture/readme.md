  ![](./media/image1.png)

#   Queryig Timing Capture Sample

This sample is compatible with the Microsoft GDK (October 2024)

# Description

This sample demonstrates how to query PIX timing capture by SQL.

![](./media/image2.png)

# Building the sample

Set the active solution platform to `x64`. This sample use PixStorage.dll which is an extension and support only x64 platform.

# Running the sample

This sample supports both console mode and GUI mode. If you pass the args, it outputs to the console.

## Console

```
QueryTimingCapture.exe <pixfile> [GetSamples] [GetContextSwitch] [LongestEvent] [DelayedThread] [InfoSummary] [RunAllSampleQueries] [/threadId <n>] [/useSymbolCache] [/eventName <name>] [/addBookmark]
```

- GetSamples needs /threadId. /useSymbolCache and /showSource are option.
- GetContextSwitch needs /threadId. /useSymbolCache and /showSource are option.
- LongestEvent needs /eventName. /addBookmark is option.

## GUI

First, you need to open PIX timing capture file (*.xpix or *.wpix).

If it's success to open the file, this sample will load some basic info and thread list from the capture. Then you can select tabs to run the querying sampple.

## GetSampledFunction

Get CPU sample callstacks from the thread you select. (first 10 samples)

If you check 'Use symbol cache', the sample will load all symbol/source information and cache. You can check 'Show source info' for display source information.

## GetContextSwitch

Get context switches information from the thread you select. (first 10 context switches) This includes 'Ready From', 'Ready To', 'Ready by' callstacks.

If you check 'Use symbol cache', the sample will load all symbol/source information and cache. You can check 'Show source info' for display source information.

## LongestEvent

Query 5 events in order of longest duration. and add a bookmark for longest one in PIX timing capture.

## DelayedThread

Query 10 delayed threads by looking for context switches where the thread was ready but not scheduled immediately.

## PerfReviewInfoSummary

Quick check for getting ATG systems engagement. It includes PDB GUID info.

## Free Input SQL

You can query SQL directly. There are some templates in SampleQueries folder.

# Implementation notes

## Thread

This sample load all thread information when opening the capture file. This includes ProcThreadId which is combination of process ID and thread ID.

## Callstack

The call stack information is in StackEvents table. This table has StackEventData which is a array of timestamp and stackId. PIX extenstion provides sqlite function FindStackId to extract it. This function returns stackId from the timestamp and threadId. You can get actual function address from Stack table using the stackId. The function address is stored in Addresses as an array.

CPU sample and context switch have the call stack. You can query it by the timestamp as a key.

You can use FunctionInformation / SourceLine tables to get the function name and source code information. Keep mind the function address = Images.PELoadAddress + FunctionInformation.Offset.

Symbol strings are decorated, the sample undecorates it by UnDecorateSymbolName API.

![](./media/image_callstacks.png)

## Context switch

Context switch information is in ContextSwitchRange table. Information such as timestamps, priority, and 'Ready to', etc are stored as arrays in ContextSwitches. Extracting this information requires a complex process, but it is easier to obtain information by using a virtual table named ContextSwitch.

To know reading thread, you need to query another ReadyThread table.

![](./media/image_contextSwitch.png)

## PIX Event

The PIX event information is in PixEventInfo table. If you want to know the Duration / Execution time, you need to query PixCPUExecutionTimes.

![](./media/image_event.png)

## Virtual tables

Using virtual tables in complex joins is heavy, so It is necessary to devise a way to access only the required number of times. (ContextSwitch, ReadyThread, PixCpuExecution, PixCpuExecutionTimes)

## Capture facts

There are some usefull information in CaptureFacts table. This sample uses CaptureFirstReliableTime, CaptureStopTime, ProcessorCount, etc...

## Bookmark

PIX capture can have some bookmarks. These are stored in Bookmarks table. When adding a bookmark, it is necessary to set the location information for each target lane. For example, If you want to add a bookmark into CpuThreadLane, the location parameter is the threadId.

# Known issues

# Update history

**Initial Release:** April 2025

**Update:** April 2026. 

- Fixed an issue where queries were duplicated when multiple threads had events with the same name in LongestEvent.
- Optimized some SQL strings and cleaned up.
- Added usage to pre-cache a virtual table in LongestEvent. 
- Added DelayedThread tab.
- Added PDB info in summary tab.
