using System;
using System.Linq;
using System.Threading;

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        private class CPUSampleResult
        {
            public int Core { get; set; }
            public long Timestamp { get; set; }
        }

        // Query 10 sampled functions
        public void GetSampledFunction(bool bUseSymbolCache, bool bShowSource, long procThreadId, CancellationToken cancelToken)
        {
            if (bUseSymbolCache && functionTable == null)
            {
                DeserializeSymbolData();
            }

            string sql = @"
                SELECT   Core,
                         Timestamp
                FROM     CPUSample
                WHERE    ProcThreadId = @procThreadId
                AND      Timestamp >= @startTimestamp
                ORDER BY Timestamp ASC
                LIMIT 10
            ";

            var results = Query<CPUSampleResult>(sql, new { procThreadId = procThreadId, startTimestamp = captureFacts.captureFirstReliableTime });

            if (results.Count() == 0)
            {
                debugOutput.Text("No sampled function events found for thread " + ProcThreadIdToThreadId(procThreadId).ToString());
                return;
            }

            foreach (var result in results)
            {
                if (cancelToken.IsCancellationRequested)
                {
                    break;
                }

                debugOutput.Text("- Core " + result.Core.ToString() +
                    " Timestamp: " + NanosecondsToSeconds(result.Timestamp).ToString() + "s");

                GetStackEvents(bUseSymbolCache, bShowSource, procThreadId, result.Timestamp);
            }
        }
    }
}
