using System.Linq;
using System.Threading;

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        public enum WaitReason : byte
        {
            Executive,
            FreePage,
            PageIn,
            PoolAllocation,
            DelayExecution,
            Suspended,
            UserRequest,
            WrExecutive,
            WrFreePage,
            WrPageIn,
            WrPoolAllocation,
            WrDelayExecution,
            WrSuspended,
            WrUserRequest,
            WrEventPair,
            WrQueue,
            WrLpcReceive,
            WrLpcReply,
            WrVirtualMemory,
            WrPageOut,
            WrRendezvous,
            WrKeyedEvent,
            WrTerminated,
            WrProcessInSwap,
            WrCpuRateControl,
            WrCalloutStack,
            WrKernel,
            WrResource,
            WrPushLock,
            WrMutex,
            WrQuantumEnd,
            WrDispatchInt,
            WrPreempted,
            WrYieldExecution,
            WrFastMutex,
            WrGuardedMutex,
            WrRundown,
            WrAlertByThreadId,
            WrDeferredPreempt,
            WrPhysicalFault,
            WrIoRing,
            MaximumWaitReason
        }

        private class ContextSwitchData
        {
            public long rowid { get; set; }
            public long Timestamp { get; set; }
            public int Core { get; set; }
            public long FromProcThreadId { get; set; }
            public int FromThreadPriority { get; set; }
            public long ToProcThreadId { get; set; }
            public int ToThreadPriority { get; set; }
            public long ReadyProcThreadId { get; set; }
            public long ReadyTimestamp { get; set; }
            public int FromThreadWaitReason { get; set; }

            public long FromStackId { get; set; }
            public long ToStackId { get; set; }
        }

        private class DelayedThreadCandidate
        {
            public long rowid { get; set; }
            public long Timestamp { get; set; }
            public int Core { get; set; }
            public long FromProcThreadId { get; set; }
            public int FromThreadPriority { get; set; }
            public long ToProcThreadId { get; set; }
            public int ToThreadPriority { get; set; }
            public long ReadyThreadId { get; set; }
            public int FromThreadWaitReason { get; set; }
            public long ReadyingThreadRowId { get; set; }
            public long Delay { get; set; }
            public long ReadyTimestamp { get; set; }
        }

        // Query 10 context switches
        public void GetContextSwitch(bool bUseSymbolCache, bool bShowSource, long procThreadId, CancellationToken cancelToken)
        {
            if (bUseSymbolCache && functionTable == null)
            {
                DeserializeSymbolData();
            }

            /*
                A context switch has a "from" thread (switched out) and a "to" thread (switched in).
                Both threads' call stacks can be resolved using FindStackId, but with an important caveat:

                  - The "to" thread's stack is available at the context switch timestamp itself.
                  - The "from" thread's stack is NOT available at the switch-out timestamp.  Instead,
                    FindStackId returns the stack at the thread's next switch-in (when it resumes execution).
                    This is how the underlying StackEvents data is structured.             

                To find the "from" thread's stack, the query locates the next context switch where the "from" thread
                is switched back in, then calls FindStackId at that timestamp.
            */

            string sql = @"
                WITH cs AS (
                    SELECT
                        ROW_NUMBER() OVER (ORDER BY Timestamp) AS r,
                        Core,
                        Timestamp,
                        FromProcThreadId,
                        ToProcThreadId,
                        ReadyThreadId,
                        FromThreadPriority,
                        ToThreadPriority,
                        FromThreadWaitReason
                    FROM
                        ContextSwitch
                    WHERE
                        Timestamp >= @startTimestamp
                        AND (
                            FromProcThreadId = @procThreadId
                            OR ToProcThreadId = @procThreadId
                        )
                    ORDER BY
                        Timestamp
                    LIMIT 10
                )
                SELECT
                    cs1.FromProcThreadId AS FromProcThreadId,
                    cs1.ToProcThreadId AS ToProcThreadId,
                    rtt.ProcThreadId AS ReadyProcThreadId,
                    rt.Timestamp AS ReadyTimestamp,
                    cs1.Timestamp AS Timestamp,
                    cs1.FromThreadPriority AS FromThreadPriority,
                    cs1.ToThreadPriority AS ToThreadPriority,
                    cs1.FromThreadWaitReason AS FromThreadWaitReason,
                    cs1.Core AS Core,
                    cs1.Timestamp - cs_prev.Timestamp AS Duration,
                    FindStackId(
                        cs1.FromProcThreadId & 0xFFFFFFFF,
                        cs_prev.Timestamp
                    ) AS FromStackId,
                    FindStackId(
                        cs1.ToProcThreadId & 0xFFFFFFFF,
                        cs1.Timestamp
                    ) AS ToStackId,
                    FindStackId(
                        rtt.ProcThreadId & 0xFFFFFFFF,
                        rt.Timestamp
                    ) AS ReadyStackId,
                    CASE cs1.FromProcThreadId
                        WHEN @procThreadId THEN 'SwitchedOut'
                        ELSE 'SwitchedIn'
                    END AS ContextSwitchType
                FROM
                    cs cs1
                    LEFT OUTER JOIN cs cs2
                        ON cs1.r = cs2.r - 1
                    LEFT OUTER JOIN cs cs_prev
                        ON cs1.r = cs_prev.r + 1
                    LEFT JOIN ReadyThread rt
                        ON cs1.ReadyThreadId = rt.Id
                    LEFT JOIN Threads rtt
                        ON rt.ReadyingThreadRowId = rtt.Id
            ";
            var matchingResults = Query<ContextSwitchData>(sql, 
                new { procThreadId, startTimestamp = captureFacts.captureFirstReliableTime }).ToList();

            foreach (var result in matchingResults)
            {
                if (cancelToken.IsCancellationRequested)
                {
                    break;
                }

                debugOutput.Text("---- Timestamp: " + NanosecondsToSeconds(result.Timestamp).ToString() + "s Core: " + result.Core);

                debugOutput.Text("- From " + threadTable.First(x => x.ProcThreadId == result.FromProcThreadId).ThreadName +
                    "(" + ProcThreadIdToThreadId(result.FromProcThreadId) + ") priority: " + result.FromThreadPriority);

                if (bUseSymbolCache)
                {
                    GetStackFromCache(bShowSource, result.FromStackId);
                }
                else
                {
                    GetStack(bShowSource, result.FromStackId);
                }

                debugOutput.Text("- To " + threadTable.First(x => x.ProcThreadId == result.ToProcThreadId).ThreadName +
                    "(" + ProcThreadIdToThreadId(result.ToProcThreadId) + ") priority: " + result.ToThreadPriority);

                if (bUseSymbolCache)
                {
                    GetStackFromCache(bShowSource, result.ToStackId);
                }
                else
                {
                    GetStack(bShowSource, result.ToStackId);
                }

                debugOutput.Text("- Readied by " + threadTable.First(x => x.ProcThreadId == result.ReadyProcThreadId).ThreadName +
                    "(" + ProcThreadIdToThreadId(result.ReadyProcThreadId) + ") reason: " + WaitReasonToString(result.FromThreadWaitReason) +
                    " Timestamp: " + NanosecondsToSeconds(result.ReadyTimestamp).ToString() + "s");

                GetStackEvents(bUseSymbolCache, bShowSource, result.ReadyProcThreadId, result.ReadyTimestamp);
            }
        }

        // Query 10 delayed threads by looking for context switches where the thread was ready but not scheduled immediately
        public void DelayedThread(bool bUseSymbolCache, bool bShowSource, CancellationToken cancelToken)
        {
            if (bUseSymbolCache && functionTable == null)
            {
                DeserializeSymbolData();
            }

            /*
                Query the context switch which has long delay excludeing PIX threads.
                Compare the timestamp of the ContextSwitch with the timestamp of the ReadyThread and sort them.
            */

            string sql = @"
                SELECT
                    cs.rowid,
                    cs.Timestamp,
                    cs.Core,
                    cs.FromProcThreadId,
                    cs.FromThreadPriority,
                    cs.ToProcThreadId,
                    cs.ToThreadPriority,
                    cs.ReadyThreadId,
                    cs.FromThreadWaitReason,
                    rt.Timestamp AS ReadyTimestamp,
                    rt.ReadyingThreadRowId,
                    (cs.Timestamp - rt.Timestamp) AS Delay
                FROM
                    ContextSwitch cs
                    JOIN ReadyThread rt ON cs.ReadyThreadId = rt.Id
                WHERE
                    cs.Timestamp > @startTimestamp
                ORDER BY
                    Delay DESC
                LIMIT 10
            ";
            var topResults = Query<DelayedThreadCandidate>(sql,
                new { startTimestamp = captureFacts.captureFirstReliableTime }).ToList();

            foreach (var result in topResults)
            {
                if (cancelToken.IsCancellationRequested)
                {
                    break;
                }

                long toStackId = GetStackIdFromDatabase(result.ToProcThreadId, result.Timestamp);

                debugOutput.Text("Delay " + NanosecondsToSeconds(result.Delay).ToString() + "s");
                debugOutput.Text("---- Timestamp: " + NanosecondsToSeconds(result.Timestamp).ToString() + "s Core: " + result.Core);

                debugOutput.Text("- To " + threadTable.First(x => x.ProcThreadId == result.ToProcThreadId).ThreadName +
                    "(" + ProcThreadIdToThreadId(result.ToProcThreadId) + ") priority: " + result.ToThreadPriority);

                if (bUseSymbolCache)
                {
                    GetStackFromCache(bShowSource, toStackId);
                }
                else
                {
                    GetStack(bShowSource, toStackId);
                }

                long readiedByThreadId = threadTable.First(x => x.Id == result.ReadyingThreadRowId).ProcThreadId;
                long readiedTimeStamp = result.ReadyTimestamp;

                debugOutput.Text("- Readied by " + threadTable.First(x => x.ProcThreadId == readiedByThreadId).ThreadName +
                    "(" + ProcThreadIdToThreadId(readiedByThreadId) + ") reason: " + WaitReasonToString(result.FromThreadWaitReason));

                GetStackEvents(bUseSymbolCache, bShowSource, readiedByThreadId, readiedTimeStamp);
            }
        }

        string WaitReasonToString(int waitReason)
        {
            if (waitReason < 0 || waitReason >= (int)WaitReason.MaximumWaitReason)
            {
                return "Unknown";
            }
            return ((WaitReason)waitReason).ToString();
        }
    }
}
