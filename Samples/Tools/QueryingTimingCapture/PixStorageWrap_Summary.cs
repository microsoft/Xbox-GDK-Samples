using Dapper;
using System;
using System.Collections.Generic;

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        private class PDBInfoResult
        {
            public string PDBPath { get; set; }
            public byte[] PDBGuid { get; set; }
            public long PDBAge { get; set; }
        }

        public void InformationSummaryForPerformanceReview()
        {
            string warningMessage = "";
            string errorMessage = "";

            try
            {
                debugOutput.Text("-------\nSUMMARY\n-------");

                debugOutput.Text("Capture Duration: " + NanosecondsToSeconds(captureFacts.captureStopTime - captureFacts.captureFirstReliableTime).ToString("F3") + "s");
                if (NanosecondsToSeconds(captureFacts.captureStopTime - captureFacts.captureFirstReliableTime) < 5)
                {
                    warningMessage += "Capture duration should be long enough for performance check, for example, more than 5s\n";
                }

                // Check CPU samples are captured
                long cpuSamplesSampleCount = 0;
                foreach (var thread in threadTable)
                {
                    if (thread.IsTitleThread == true)
                    {
                        cpuSamplesSampleCount += thread.SampleCount;
                    }
                }

                if (cpuSamplesSampleCount != 0)
                {
                    debugOutput.Text("Cpu Samples: Captured");
                }
                else
                {
                    debugOutput.Text("Cpu Samples: Not captured");
                    warningMessage += "Cpu samples are not captured\n";
                }

                // Check whether each thread has its name (and it is not just a number)
                long threadNameCount = 0;
                long titleThreadCount = 0;
                foreach (var thread in threadTable)
                {
                    if (thread.IsTitleThread == true)
                    {
                        titleThreadCount++;
                        if (!String.IsNullOrWhiteSpace(thread.ThreadName) && !decimal.TryParse(thread.ThreadName, out _))
                        {
                            threadNameCount++;
                        }
                    }
                }

                if (threadNameCount > 0)
                {
                    debugOutput.Text("Thread Name: " + threadNameCount + "/" + titleThreadCount + " threads have names");
                }
                else
                {
                    debugOutput.Text("Thread Name: No thread has its name");
                    warningMessage += "None of threads has its name\n";
                }

                // Check there is enough user-defined PIX events (PixBeginEvent)
                long apiEventsCount = 0;
                foreach (var thread in threadTable)
                {
                    if (thread.IsTitleThread == true)
                    {
                        apiEventsCount += thread.PixEventCount;
                    }
                }

                if (apiEventsCount != 0)
                {
                    debugOutput.Text("User-defined PIX Events: Defined");
                }
                else
                {
                    debugOutput.Text("User-defined PIX Events: Not defined");
                    warningMessage += "There is no user-defined PIX events\n";
                }

                // Check what data is included in the capture
                debugOutput.Text("VirtualAllocEvents: " + (captureFacts.captureVirtualAllocEvents != 0 ? "enable" : "disable"));
                debugOutput.Text("HeapAllocEvents: " + (captureFacts.captureHeapAllocEvents != 0 ? "enable" : "disable"));
                debugOutput.Text("XMemAllocEvents: " + (captureFacts.captureXMemAllocEvents != 0 ? "enable" : "disable"));
                debugOutput.Text("CustomAllocEvents: " + (captureFacts.captureCustomAllocEvents != 0 ? "enable" : "disable"));
                debugOutput.Text("FileIoEvents: " + (captureFacts.captureFileIoEvents != 0 ? "enable" : "disable"));

                // Display PDB GUID

                debugOutput.Text("-------\nPDB info\n-------");

                string sqlPDB = @"
                    SELECT s.Value AS PDBPath, m.PDBGuid, m.PDBAge
                    FROM   Modules m
                    JOIN   Strings s
                    ON     s.Id = m.PDBPathId;
                ";

                var results = Query<PDBInfoResult>(sqlPDB);

                foreach (var result in results)
                {
                    string pdbPath = result.PDBPath;
                    Guid pdbGuid = new Guid(result.PDBGuid);
                    long pdbAge = result.PDBAge;

                    debugOutput.Text(pdbPath + " : " + pdbGuid.ToString() + " age " + pdbAge.ToString());
                }
            }
            catch (Exception e)
            {
                _ = e;
                errorMessage += "the file is not a valid Timing Capture file.";
            }

            if (!String.IsNullOrEmpty(errorMessage))
            {
                debugOutput.Text("-------\nERROR\n-------");
                debugOutput.Text(errorMessage);
            }
            if (!String.IsNullOrEmpty(warningMessage))
            {
                debugOutput.Text("-------\nWARNING\n-------");
                debugOutput.Text(warningMessage);
            }
        }

        public void GetFreeSQLInputResult(string sql)
        {
            debugOutput.Text(">" + sql);

            // Using dynamic object for free SQL input.
            IEnumerable<dynamic> results = null;
            try
            {
                results = sqlConnect.Query(sql);
            }
            catch (Exception e)
            {
                debugOutput.Text("Exception occured: " + e.Message);
            }

            if (results != null)
            {
                debugOutput.Table(results);
            }
        }
    }
}
