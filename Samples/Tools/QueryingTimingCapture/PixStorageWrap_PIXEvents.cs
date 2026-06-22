using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        private class EventData
        {
            public long ThreadRowId { get; set; }
            public long BeginTimestamp { get; set; }
            public long EndTimestamp { get; set; }
            public long Duration { get; set; }
            public long Execution { get; set; }
        }

        private class CpuExecutionResult
        {
            public long EventId { get; set; }
            public int ThreadRowId { get; set; }
            public long BeginTimestamp { get; set; }
            public long EndTimestamp { get; set; }
            public long Duration { get; set; }
            public long Execution { get; set; }
        }

        private class ExecutionTimeResult
        {
            public long EventId { get; set; }
            public long BeginTimestamp { get; set; }
            public long EndTimestamp { get; set; }
            public long Duration { get; set; }
            public long Execution { get; set; }
        }

        private class BookmarkParameters
        {
            public int LocationEnum { get; set; }
            public long LocationId { get; set; }
            public long StartTimestamp { get; set; }
            public long EndTimestamp { get; set; }
            public long StartViewportTimestamp { get; set; }
            public long EndViewportTimestamp { get; set; }
            public string Author { get; set; }
            public string Comment { get; set; }
        }

        // find the longest event and add the boomark into the PIX file
        public void LongestEvent(string eventName, bool bAddBookmark, CancellationToken cancelToken)
        {
#if false
            // This is original SQL query which directly joins virtual tables (PixCpuExecution and PixCpuExecutionTime).
            // This takes slightly longer than the method of caching in memory.

            string sqlEvent = @"
                SELECT      pce.ThreadRowId, 
                            top_events.BeginTimestamp, 
                            top_events.EndTimestamp,
                            top_events.Duration,
                            top_events.Execution
                FROM        (
                    SELECT  times.EventId,
                            times.BeginTimestamp, 
                            times.EndTimestamp,
                            times.Duration,
                            times.Execution
                    FROM    PixEventInfo pei
                    JOIN    Strings s ON pei.NameId = s.Id
                    JOIN    PixCpuExecutionTimes times ON times.EventId = pei.Id
                    WHERE   s.Value = @eventName
                    AND     times.BeginTimestamp >= @captureStart
                    AND     times.EndTimestamp <= @captureStopTime
                    ORDER BY times.Duration DESC
                    LIMIT 5
                ) AS top_events
                JOIN        PixCpuExecution pce
                ON          pce.EventId = top_events.EventId
                AND         pce.BeginTimestamp = top_events.BeginTimestamp
                AND         pce.EndTimestamp = top_events.EndTimestamp
            ";
            var results = Query<EventData>(sqlEvent, new 
            {
                captureStart = captureFacts.captureFirstReliableTime,
                captureStopTime = captureFacts.captureStopTime,
                eventName = eventName
            }).ToList();

#else
            // Get matching event IDs
            string sqlEventIds = @"
                SELECT pei.Id
                FROM   PixEventInfo pei
                JOIN   Strings s ON pei.NameId = s.Id
                WHERE  s.Value = @eventName
            ";

            var eventIds = Query<long>(sqlEventIds, new { eventName = eventName })
                .ToList();

            if (!eventIds.Any())
            {
                debugOutput.Text($"No events found with name: {eventName}");
                return;
            }

            // Load all PixCpuExecution data for these events into memory
            string sqlCpuExecution = @"
                SELECT EventId, ThreadRowId, BeginTimestamp, EndTimestamp, 0 AS Duration, 0 AS Execution
                FROM   PixCpuExecution
                WHERE  EventId IN @eventIds
                AND    BeginTimestamp >= @captureStart
                AND    EndTimestamp <= @captureStopTime
            ";

            var cpuExecutions = Query<CpuExecutionResult>(sqlCpuExecution, new
            {
                eventIds = eventIds,
                captureStart = captureFacts.captureFirstReliableTime,
                captureStopTime = captureFacts.captureStopTime
            }).ToList();

            // Load all PixCpuExecutionTimes data for these events into memory
            string sqlExecutionTimes = @"
                SELECT EventId, BeginTimestamp, EndTimestamp, Duration, Execution
                FROM   PixCpuExecutionTimes
                WHERE  EventId IN @eventIds
            ";

            var executionTimes = Query<ExecutionTimeResult>(sqlExecutionTimes, new { eventIds = eventIds })
                .ToDictionary(x => (x.EventId, x.BeginTimestamp, x.EndTimestamp));

            // Join in memory and find longest events
            var results = cpuExecutions
                .Where(ce =>
                {
                    var key = (ce.EventId, ce.BeginTimestamp, ce.EndTimestamp);
                    return executionTimes.ContainsKey(key);
                })
                .Select(ce =>
                {
                    var key = (ce.EventId, ce.BeginTimestamp, ce.EndTimestamp);
                    var times = executionTimes[key];
                    ce.Duration = times.Duration;
                    ce.Execution = times.Execution;
                    return ce;
                })
                .OrderByDescending(x => x.Duration)
                .Take(5)
                .ToList();
#endif
            // Output results and add bookmark for the longest event

            debugOutput.Text("---- Longest " + eventName + " events:");

            bool isFirst = true;
            foreach (var result in results)
            {
                if (cancelToken.IsCancellationRequested)
                {
                    break;
                }

                var thread = threadTable.Find(x => x.Id == result.ThreadRowId);

                debugOutput.Text("  Thread " + ProcThreadIdToThreadId(thread.ProcThreadId).ToString() +
                    " Duration " + result.Duration.ToString() +
                    " Execution " + result.Execution.ToString() +
                    " BeginTimestamp " + NanosecondsToSeconds(result.BeginTimestamp).ToString() + "s" +
                    " EndTimestamp " + NanosecondsToSeconds(result.EndTimestamp).ToString() + "s");

                if (isFirst && bAddBookmark)
                {
                    // Insert a bookmark
                    AddBookmark(BookmarkLocationType.CpuThreadLane, thread.ProcThreadId, result.BeginTimestamp, result.EndTimestamp, "QueryingTimingCapture", "Longest event.");

                    isFirst = false;
                }
            }
        }

        public enum BookmarkLocationType
        {
            CpuCoreLane = 1,
            CpuThreadLane = 2,
            ApiQueueLane = 3,
            DSQueueLane = 4,
            ControllerLane = 5,
            Win32DeviceLane = 6,
            SchedulerLane = 7,
            CustomDataLane = 8,
            VideoFrameLane = 9,
            ThreadCriticalPathLane = 10,
            MemoryUsageLane = 11,
            MetricsViewLane = 12,
            FrameEventLane = 13,
            ResidenceOperationLane = 14,
            GpuTraceSqttLane = 15,
            GpuTraceSpmLane = 16,
            GpuTraceGraphicsContextsLane = 17
        };

        public void AddBookmark(BookmarkLocationType type, long location, long startTimeStamp, long endTimeStamp, string author, string comment)
        {
            string sql = @"
                INSERT INTO Bookmarks
                    (LocationEnum, LocationId, StartTimestamp, EndTimestamp, StartViewportTimestamp, EndViewportTimestamp, Author, Comment)
                VALUES (@LocationEnum, @LocationId, @StartTimestamp, @EndTimestamp, @StartViewportTimestamp, @EndViewportTimestamp, @Author, @Comment)
            ";

            var parameters = new BookmarkParameters
            {
                LocationEnum = (int)type,
                LocationId = location,
                StartTimestamp = startTimeStamp,
                EndTimestamp = endTimeStamp,
                StartViewportTimestamp = startTimeStamp >= 100 ? startTimeStamp - 100 : 0,
                EndViewportTimestamp = endTimeStamp < captureFacts.captureStopTime - 100 ? endTimeStamp + 100 : captureFacts.captureStopTime,
                Author = author,
                Comment = comment
            };

            Execute(sql, parameters);

            debugOutput.Text("Bookmark is added");
        }

        public List<string> GetPixEvents()
        {
            string sql = @"
                SELECT  s.Value
                FROM    Strings s
                JOIN    PixEventInfo pei
                ON      s.Id = pei.NameId
                AND     pei.Count >= 0
                LIMIT 256
            ";

            var results = Query<string>(sql);

            return results.ToList();
        }
    }
}
