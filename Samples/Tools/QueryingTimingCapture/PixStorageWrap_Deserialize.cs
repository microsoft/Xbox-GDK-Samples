using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace QueryingTimingCapture
{
    public partial class PixStorageWrap
    {
        public class ImageInfo
        {
            public long ModuleId { get; set; }
            public ulong PELoadAddress { get; set; }
            public ulong LoadSize { get; set; }

            public ulong PEEndAddress => PELoadAddress + LoadSize;
        }

        public class FunctionInfo
        {
            public ulong Address { get; set; }
            public ulong Size { get; set; }
            public long ModuleId { get; set; }
            public string Name { get; set; }
        }

        public class SourceInfo
        {
            public long ModuleId { get; set; }
            public string SourceFile { get; set; }
            public long Offset { get; set; }
            public long Length { get; set; }
            public long LineStart { get; set; }
            public long ColumnStart { get; set; }
        }

        public class ThreadInfo
        {
            public long Id { get; set; }
            public bool IsTitleThread { get; set; }
            public long ProcThreadId { get; set; }
            public string ThreadName { get; set; }
            public long PixEventCount { get; set; }
            public long ContextSwitchCount { get; set; }
            public long SampleCount { get; set; }
            public long ThreadPriority { get; set; }
        }

        private class SymbolStringResult
        {
            public long Id { get; set; }
            public string Value { get; set; }
        }

        private class FunctionInformationResult
        {
            public long Id { get; set; }
            public long ModuleId { get; set; }
            public long Offset { get; set; }
            public long Size { get; set; }
            public long DecoratedNameId { get; set; }
        }

        private class SourceFileResult
        {
            public long Id { get; set; }
            public long SourceFileNameId { get; set; }
        }

        private class SourceLineResult
        {
            public long Id { get; set; }
            public long ModuleId { get; set; }
            public long SourceFileId { get; set; }
            public long Offset { get; set; }
            public long Length { get; set; }
            public long LineStart { get; set; }
            public long ColumnStart { get; set; }
        }

        Dictionary<long, ulong> imageTable;
        List<ImageInfo> imageOffsets;
        Dictionary<long, List<SourceInfo>> sourceTable;
        List<FunctionInfo> functionTable;
        List<ThreadInfo> threadTable;

        public void LoadImageInfo()
        {
            // Get all Images

            string sqlImages = "SELECT ModuleId, PELoadAddress, LoadSize FROM Images WHERE ModuleId != 0 ORDER BY PELoadAddress";
            var resultImages = Query<ImageInfo>(sqlImages);

            imageTable = new Dictionary<long, ulong>(resultImages.Count() + 1);
            imageOffsets = new List<ImageInfo>(resultImages.Count() + 1);

            foreach (var result in resultImages)
            {
                imageTable[result.ModuleId] = result.PELoadAddress;
                imageOffsets.Add(result);
            }

            imageOffsets.Sort((x, y) => x.PELoadAddress.CompareTo(y.PELoadAddress));

            Debug.Assert(imageOffsets[1].PELoadAddress > imageOffsets[0].PELoadAddress);
            debugOutput.Text("Complete loading images.");
        }

        public void DeserializeSymbolData()
        {
            // Get all SymbolStrings

            string sqlSymbolStrings = "SELECT Id, Value FROM SymbolStrings";
            var resultSymbolStrings = Query<SymbolStringResult>(sqlSymbolStrings);

            var strings = new Dictionary<long, string>(resultSymbolStrings.Count());

            foreach (var result in resultSymbolStrings)
            {
                strings.Add(result.Id, result.Value);
            }

            debugOutput.Text("(1/4) Complete loading SymbolStrings.");

            var taskGetFunctions = Task.Run(() =>
            {
                // Get all FunctionInformation

                string sqlFunctions = "SELECT Id, ModuleId, Offset, Size, DecoratedNameId FROM FunctionInformation";
                var resultFunctions = Query<FunctionInformationResult>(sqlFunctions);

                functionTable = new List<FunctionInfo>(resultFunctions.Count());

                foreach (var result in resultFunctions)
                {
                    var image = imageOffsets.FirstOrDefault(x => x.ModuleId == result.ModuleId);
                    var functionName = strings[result.DecoratedNameId];

                    if (functionName[0] == '?')
                    {
                        functionName = UnDecorateSymbol(functionName);
                    }

                    functionTable.Add(new FunctionInfo()
                    {
                        Address = (ulong)result.Offset + image.PELoadAddress,
                        Size = (ulong)result.Size,
                        ModuleId = result.ModuleId,
                        Name = functionName
                    });
                }

                debugOutput.Text("(2/4) Complete loading FunctionInformation.");
            });

            var taskGetSources = Task.Run(() =>
            {
                // Get all SourceFile

                string sqlSourceFiles = "SELECT Id, SourceFileNameId FROM SourceFile";
                var resultSourceFiles = Query<SourceFileResult>(sqlSourceFiles);

                sourceTable = new Dictionary<long, List<SourceInfo>>(resultSourceFiles.Count());
                var sourceFiles = new Dictionary<long, string>(resultSourceFiles.Count());

                foreach (var result in resultSourceFiles)
                {
                    sourceFiles.Add(result.Id, strings[result.SourceFileNameId]);
                }

                debugOutput.Text("(3/4) Complete loading SourceFile.");

                // Get all SourceLine

                List<long> moduleList = imageTable.Select(x => x.Key).ToList();

                foreach (long moduleId in moduleList)
                {
                    string sqlSources = @"
                        SELECT  Id, 
                                ModuleId, 
                                SourceFileId, 
                                Offset, 
                                Length, 
                                LineStart, 
                                ColumnStart
                        FROM    SourceLine
                        WHERE   ModuleId = @moduleId
                    ";
                    var resultSources = Query<SourceLineResult>(sqlSources, new { moduleId = moduleId });

                    var sourceInfoList = new List<SourceInfo>();

                    foreach (var result in resultSources)
                    {
                        var sourceFile = sourceFiles[result.SourceFileId];

                        sourceInfoList.Add(new SourceInfo()
                        {
                            ModuleId = result.ModuleId,
                            SourceFile = sourceFile,
                            Offset = result.Offset,
                            Length = result.Length,
                            LineStart = result.LineStart,
                            ColumnStart = result.ColumnStart
                        });
                    }

                    sourceTable.Add(moduleId, sourceInfoList);
                }

                debugOutput.Text("(4/4) Complete loading SourceLine.");
            });

            Task.WaitAll(new[] { taskGetFunctions, taskGetSources });
        }
    }
}
