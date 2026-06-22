using Dapper;
using Microsoft.Data.Sqlite;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;

// This project must be compiled as x64.

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        private uint minSupportedSchemaVersion = 0x15;

        private string connectionString = null;
        private SqliteConnection sqlConnect = null;
        private DebugOutput debugOutput = null;
        public CaptureFacts captureFacts = null;

        private class StackResult
        {
            public long Id { get; set; }
            public byte[] Addresses { get; set; }
            public int NumFrames { get; set; }
        }

        private class ThreadQueryResult
        {
            public long Id { get; set; }
            public long ProcThreadId { get; set; }
            public string ThreadName { get; set; }
            public long PixEventCount { get; set; }
            public long ContextSwitchCount { get; set; }
            public long SampleCount { get; set; }
            public int ThreadPriority { get; set; }
        }

        public PixStorageWrap(DebugOutput debugOutput)
        {
            this.debugOutput = debugOutput;
        }

        public bool Open(string pixFile)
        {
            if (sqlConnect != null)
            {
                Close();
            }

            var builder = new SqliteConnectionStringBuilder()
            {
                DataSource = pixFile,
                Mode = SqliteOpenMode.ReadWrite
            };

            connectionString = builder.ToString();

            string extensionPath = Environment.GetEnvironmentVariable("GameDK");
            if (extensionPath != null)
            {
                extensionPath += "bin\\pixstorage.dll";
            }
            else
            {
                debugOutput.Text("GameDK environment variable is not set.");
                return false;
            }

            if (File.Exists(extensionPath) == false)
            {
                debugOutput.Text("pixstorage.dll is not found.");
                return false;
            }

            try
            {
                sqlConnect = new SqliteConnection(connectionString);
                sqlConnect.EnableExtensions(true);
                sqlConnect.LoadExtension(extensionPath, "sqlite3_batchexpand_init");
                sqlConnect.Open();
            }
            catch (Exception e)
            {
                _ = e;
                debugOutput.Text(pixFile + " is not a valid Timing Capture file.");
                return false;
            }

            captureFacts = new CaptureFacts(sqlConnect);

            debugOutput.Text("Opening file version : " + captureFacts.engineCaptureVersion + ".");

            debugOutput.Text("Complete loading CaptureFacts.");

            if (!CheckVersion(pixFile))
            {
                return false;
            }

            LoadImageInfo();

            return true;
        }

        public IEnumerable<Type> Query<Type>(string sql, object param = null)
        {
            try
            {
                return sqlConnect.Query<Type>(sql, param);
            }
            catch (Exception e)
            {
                debugOutput.Text("Exception occured: " + e.Message);
                Error(-1);
                return null;
            }
        }

        public Type QuerySingleOrDefault<Type>(string sql, object param = null)
        {
            try
            {
                return sqlConnect.QuerySingleOrDefault<Type>(sql, param);
            }
            catch (Exception e)
            {
                debugOutput.Text("Exception occured: " + e.Message);
                Error(-1);
                return default(Type);
            }
        }

        public int Execute<Type>(string sql, Type param)
        {
            try
            {
                return sqlConnect.Execute(sql, param);
            }
            catch (Exception e)
            {
                debugOutput.Text("Exception occured: " + e.Message);
                Error(-1);
                return -1;
            }
        }

        public void Close()
        {
            sqlConnect.Close();
        }

        public void Error(int exitCode)
        {
            var commandline = Environment.GetCommandLineArgs();

            // GUI mode
            if (commandline.Length == 1)
            {
                MessageBox.Show("Error");
            }

            Environment.Exit(exitCode);
        }

        public string ResolveSymbol(ulong address)
        {
            string sql = @"
                SELECT  ss.Value as FunctionName
                FROM    SymbolStrings ss
                JOIN    FunctionInformation fi
                ON      ss.Id = fi.DecoratedNameId
                JOIN    Images i
                ON      fi.ModuleId = i.ModuleId
                WHERE   @address BETWEEN i.PELoadAddress + fi.Offset
                AND     i.PELoadAddress + fi.Offset + fi.Size - 1
                LIMIT 1
            ";

            var result = QuerySingleOrDefault<string>(sql, new { address = address });

            if (result != null)
            {
                if (result[0] == '?')
                {
                    return UnDecorateSymbol(result);
                }
                return result;
            }

            return string.Empty;
        }

        private (long module, ulong offset) GetModuleAndOffset(ulong address)
        {
            var result = imageOffsets.FirstOrDefault(io => io.PELoadAddress <= address && io.PEEndAddress > address) ?? new ImageInfo();
            return (result.ModuleId, address - result.PELoadAddress);
        }

        public SourceInfo GetSourceInfo(ulong address)
        {
            string sql = @"
                SELECT  ss.Value as SourceFile,
                        sl.LineStart,
                        sl.ColumnStart
                FROM    SourceLine sl
                JOIN    SymbolStrings ss
                ON      ss.Id = sf.SourceFileNameId
                JOIN    SourceFile sf
                ON      sl.SourceFileId = sf.Id
                WHERE   sl.ModuleId = @moduleId
                  AND   sl.Offset = (SELECT MAX(Offset) FROM SourceLine WHERE ModuleId = @moduleId AND Offset <= @offset)
                LIMIT 1
            ";

            var module = GetModuleAndOffset(address);

            var result = QuerySingleOrDefault<SourceInfo>(sql, new { moduleId = module.module, offset = module.offset });

            return result;
        }

        private long GetStackIdFromDatabase(long procThreadId, long timestamp)
        {
            string sql = $"SELECT FindStackId(({procThreadId} & 0xFFFFFFFF), {timestamp}) AS StackId";
            return QuerySingleOrDefault<long>(sql);
        }

        public void GetStackEvents(bool bUseSymbolCache, bool bShowSource, long procThreadId, long timeStamp)
        {
            if (procThreadId == 0) return;

            var stackId = GetStackIdFromDatabase(procThreadId, timeStamp);

            if (stackId != 0)
            {
                if (bUseSymbolCache)
                {
                    GetStackFromCache(bShowSource, stackId);
                }
                else
                {
                    GetStack(bShowSource, stackId);
                }
            }
        }

        public void GetStackFromCache(bool bShowSource, long stackId)
        {
            string sql = @"
                SELECT  Id,
                        Addresses,
                        NumFrames
                FROM    Stacks
                WHERE   Id = @id
            ";

            var result = QuerySingleOrDefault<StackResult>(sql, new { id = stackId });

            if (result != null)
            {
                BinaryReader br = new BinaryReader(new MemoryStream(result.Addresses));

                // Read from binary
                for (int j = 0; j < result.NumFrames; j++)
                {
                    ulong address = br.ReadUInt64();

                    var functions = functionTable.FindAll(x => x.Address <= address && address < x.Address + x.Size);

                    if (functions.Count == 0)
                    {
                        debugOutput.Text("  " + address.ToString("X16") + " " + "** couldn't resolve the symbol.");
                    }
                    else if (functions.Count > 1)
                    {
                        debugOutput.Text("  " + address.ToString("X16") + " " + "** multiple symbols found.");
                    }
                    else if (bShowSource)
                    {
                        // source info
                        var module = GetModuleAndOffset(functions[0].Address);
                        var sources = sourceTable[module.module].FindAll(x => (ulong)x.Offset == module.offset);
                        if (sources.Count == 0)
                        {
                            debugOutput.Text("  " + address.ToString("X16") + " " + functions[0].Name + " ** couldn't find the source.");
                        }
                        else if (sources.Count > 1)
                        {
                            debugOutput.Text("  " + address.ToString("X16") + " " + functions[0].Name + " **  multiple sources found.");
                        }
                        else
                        {
                            debugOutput.Text("  " + address.ToString("X16") + " " + functions[0].Name +
                                " [" + sources[0].SourceFile + ":" + sources[0].LineStart.ToString() + "]");
                        }
                    }
                    else
                    {
                        debugOutput.Text("  " + address.ToString("X16") + " " + functions[0].Name);
                    }
                }
            }
        }

        public void GetStack(bool bShowSource, long stackId)
        {
            string sql = @"
                SELECT  Id,
                        Addresses,
                        NumFrames
                FROM    Stacks
                WHERE   Id = @id
            ";

            var result = QuerySingleOrDefault<StackResult>(sql, new { id = stackId });

            if (result != null)
            {
                BinaryReader br = new BinaryReader(new MemoryStream(result.Addresses));

                // Read from binary
                for (int j = 0; j < result.NumFrames; j++)
                {
                    ulong address = br.ReadUInt64();
                    string functionName = ResolveSymbol(address);

                    if (String.IsNullOrEmpty(functionName))
                    {
                        debugOutput.Text("  " + address.ToString("X16") + " " + "** couldn't resolve the symbol.");
                    }
                    else if (bShowSource)
                    {
                        SourceInfo sourceInfo = GetSourceInfo(address);

                        if (sourceInfo != null)
                        {
                            debugOutput.Text("  " + address.ToString("X16") + " " + functionName +
                                " [" + sourceInfo.SourceFile + ":" + sourceInfo.LineStart.ToString() + "]");
                        }
                    }
                    else
                    {
                        debugOutput.Text("  " + address.ToString("X16") + " " + functionName);
                    }
                }
            }
            else
            {
                debugOutput.Text("** no stack information.");
            }
        }

        public void GetThreadList()
        {
            string sql = @"
                SELECT  t.Id as Id,
                        t.ProcThreadId as ProcThreadId,
                        st.Value as ThreadName,
                        t.PixEventCount as PixEventCount,
                        t.ContextSwitchCount as ContextSwitchCount,
                        t.SampleCount  as SampleCount,                    
                        tp.BasePriority as ThreadPriority
                FROM    Threads t    
                JOIN    Strings st
                ON      t.ThreadNameId = st.Id
                JOIN    ThreadPriorities tp
                ON      t.Id = tp.ThreadRowId
            ";

            var results = Query<ThreadQueryResult>(sql);
            threadTable = new List<ThreadInfo>(results.Count());

            foreach (var result in results)
            {
                long processId = ProcThreadIdToProcessId(result.ProcThreadId);
                bool isTitleThread = (processId == captureFacts.targetProcessId);

                threadTable.Add(new ThreadInfo()
                {
                    Id = result.Id,
                    IsTitleThread = isTitleThread,
                    ProcThreadId = result.ProcThreadId,
                    ThreadName = result.ThreadName,
                    PixEventCount = result.PixEventCount,
                    ContextSwitchCount = result.ContextSwitchCount,
                    SampleCount = result.SampleCount,
                    ThreadPriority = result.ThreadPriority
                });
            }

            debugOutput.Text("Complete loading thread list.");
        }

        public void StoreThreadList(ComboBox cb)
        {
            cb.Items.Clear();

            foreach (var thread in threadTable)
            {
                if (thread.IsTitleThread)
                {
                    cb.Items.Add(thread.ProcThreadId.ToString() + " (" + ProcThreadIdToThreadId(thread.ProcThreadId).ToString() + ") " + thread.ThreadName);
                }
            }
        }

        [DllImport("dbghelp.dll", CharSet = CharSet.Ansi)]
        private static extern uint UnDecorateSymbolName(string name, StringBuilder outputString, uint maxStringLength, uint flags);

        public string UnDecorateSymbol(string decoratedName)
        {
            // decoratedName should be like "?FunctionName@@YAXH@Z"

            StringBuilder undecoratedName = new StringBuilder(256);
            uint result = UnDecorateSymbolName(decoratedName, undecoratedName, (uint)undecoratedName.Capacity, 0x1000 /* UNDNAME_NAME_ONLY */);

            if (result > 0)
            {
                return undecoratedName.ToString();
            }
            else
            {
                return string.Empty;
            }
        }

        [DllImport("PixStorage.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        internal static extern int P3SQueryStorageVersion(
            string captureFilePath,
            out UInt32 currentStorageVersion,
            out UInt32 fileVersion,
            [MarshalAs(UnmanagedType.Bool)] out bool canMigrate
        );

        public bool CheckVersion(string pixFile)
        {
            if (sqlConnect == null)
            {
                return false;
            }

            uint neededVersion = 0;
            uint fileVersion = 0;
            bool canMigrate = false;

            if (P3SQueryStorageVersion(pixFile, out neededVersion, out fileVersion, out canMigrate) != 0)
            {
                return false;
            }

            if (fileVersion < minSupportedSchemaVersion)
            {
                debugOutput.Text("Previous timing capture. Unsupported.");
                return false;
            }

            if (neededVersion != fileVersion)
            {
                if (neededVersion > fileVersion)
                {
                    if (canMigrate)
                    {
                        debugOutput.Text("Previous timing capture. Please migrate the file via PIX.");
                    }
                    else
                    {
                        debugOutput.Text("Previous timing capture. Unsupported.");
                    }
                }
                else if (neededVersion < fileVersion)
                {
                    debugOutput.Text("Newer timing capture. Please update PixStorage.dll.");
                }

                return false;
            }

            return true;
        }


        double NanosecondsToSeconds(long ns)
        {
            return ns / 1000000000.0;
        }

        long ProcThreadIdToThreadId(long procThreadId)
        {
            return procThreadId & 0xffffffff; // ProcThreadId = (pid << 32) | tid;            
        }

        long ProcThreadIdToProcessId(long procThreadId)
        {
            return procThreadId >> 32;
        }
    }

    public class CaptureFacts
    {
        private class CaptureFactResult
        {
            public int Id { get; set; }
            public long Value { get; set; }
        }

        public long captureFirstReliableTime;
        public long targetProcessId;
        public long processorCount;
        public long captureStopTime;
        public long captureVirtualAllocEvents;
        public long captureHeapAllocEvents;
        public long captureXMemAllocEvents;
        public long captureCustomAllocEvents;
        public long captureFileIoEvents;
        public string engineCaptureVersion;

        private enum CaptureFactId
        {
            CaptureStartTime = 1,
            CaptureFirstReliableTime = 2,
            CaptureLastEventTime = 3,
            TargetProcessId = 4,
            ProcessorCount = 5,
            CapturePlatform = 6,

            CaptureStopTime = 24,

            EngineCaptureVersionId = 66,

            CaptureCpuSamples = 100,
            CaptureContextSwitches = 101,
            CaptureReadyThreadEvents = 102,
            CaptureGpuTimingEvents = 103,
            CaptureGpuSchedulerEvents = 104,
            CaptureGpuMemoryEvents = 105,
            CaptureVirtualAllocEvents = 106,
            CaptureHeapAllocEvents = 107,
            CaptureXMemAllocEvents = 108,
            CaptureCustomAllocEvents = 109,

            CaptureFileIoEvents = 111,
        };

        public CaptureFacts(SqliteConnection sqlConnect)
        {
            string captureDurationSql = @"
                SELECT  *
                FROM    CaptureFacts
            ";

            var captureDurationResults = sqlConnect.Query<CaptureFactResult>(captureDurationSql);
            Dictionary<CaptureFactId, long> facts = captureDurationResults.ToDictionary(row => (CaptureFactId)row.Id, row => row.Value);

            // It's possible there are no facts in previous capture.
            facts.TryGetValue(CaptureFactId.CaptureFirstReliableTime, out captureFirstReliableTime);
            facts.TryGetValue(CaptureFactId.TargetProcessId, out targetProcessId);
            facts.TryGetValue(CaptureFactId.ProcessorCount, out processorCount);
            facts.TryGetValue(CaptureFactId.CaptureStopTime, out captureStopTime);
            facts.TryGetValue(CaptureFactId.CaptureVirtualAllocEvents, out captureVirtualAllocEvents);
            facts.TryGetValue(CaptureFactId.CaptureHeapAllocEvents, out captureHeapAllocEvents);
            facts.TryGetValue(CaptureFactId.CaptureXMemAllocEvents, out captureXMemAllocEvents);
            facts.TryGetValue(CaptureFactId.CaptureCustomAllocEvents, out captureCustomAllocEvents);
            facts.TryGetValue(CaptureFactId.CaptureFileIoEvents, out captureFileIoEvents);

            long versionId = 0;
            if (facts.TryGetValue(CaptureFactId.EngineCaptureVersionId, out versionId))
            {
                string sql = @"
                    SELECT Value
                    FROM   Strings
                    WHERE  Id = @versionString
                ";

                var result = sqlConnect.QuerySingleOrDefault<string>(sql, new { versionString = versionId });

                if (!string.IsNullOrEmpty(result))
                {
                    engineCaptureVersion = result;
                }
            }
            else
            {
                engineCaptureVersion = "unknown";
            }
        }
    }
}

