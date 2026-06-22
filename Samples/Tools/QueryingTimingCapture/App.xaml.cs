using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;

namespace QueryingTimingCapture
{
    public partial class App : Application
    {
        [DllImport("Kernel32.dll")]
        public static extern bool AttachConsole(int processId);
        [DllImport("kernel32.dll")]
        public static extern bool FreeConsole();

        private void Application_Startup(object sender, StartupEventArgs e)
        {
            if (e.Args.Length != 0)
            {
                AttachConsole(-1);

                RunOnCommandline(e.Args);

                FreeConsole();
                Shutdown(0);
            }
            else
            {
                MainWindow w = new MainWindow();
                w.Show();
            }
        }

        public void RunOnCommandline(string[] args)
        {
            DebugOutput debugOutput = new DebugOutput();
            PixStorageWrap pixStorageWrap = new PixStorageWrap(debugOutput);

            string pixPath = "";
            bool bGetSamples = false;
            bool bGetContextSwitch = false;
            bool bLongestEvent = false;
            bool bDelayedThread = false;
            bool bInfoSummary = false;
            bool bRunAllSampleQueries = false;
            long threadID = 0;
            string eventName = "";
            bool bUseSymbolCache = false;
            bool bShowSource = false;
            bool bAddBookmark = false;

            // first parameter is always the path to the pix file (excluded /? param)
            pixPath = args[0];

            if (pixPath == "/?" || pixPath == "/h")
            {
                debugOutput.Text("usage: QuryingTimingCapture.exe c:\\temp\\test.xpix GetSamples GetContextSwitch /threadId 100 /useCache");
                debugOutput.Text("");
                debugOutput.Text("Supported actions: GetSamples /threadId /useCache /showSource");
                debugOutput.Text("                   GetContextSwitch /threadId /useCache /showSource");
                debugOutput.Text("                   LongestEvent /eventName /addBookmark");
                debugOutput.Text("                   DelayedThread /useCache /showSource");
                debugOutput.Text("                   InfoSummary");
                debugOutput.Text("                   RunAllSampleQueries");
                return;
            }

            if (pixStorageWrap.Open(pixPath))
            {
                pixStorageWrap.GetThreadList();

                // parse
                for (int i = 1; i < args.Length; i++)
                {
                    string arg = args[i];

                    switch (arg)
                    {
                        case "GetSamples":
                            bGetSamples = true;
                            break;
                        case "GetContextSwitch":
                            bGetContextSwitch = true;
                            break;
                        case "LongestEvent":
                            bLongestEvent = true;
                            break;
                        case "DelayedThread":
                            bDelayedThread = true;
                            break;
                        case "InfoSummary":
                            bInfoSummary = true;
                            break;
                        case "RunAllSampleQueries":
                            bRunAllSampleQueries = true;
                            break;
                        case "/threadId":
                            if (i + 1 < args.Length)
                            {
                                if (long.TryParse(args[i + 1], out threadID))
                                {
                                }
                                else
                                {
                                    debugOutput.Text("invalid thread id");
                                }
                            }
                            break;
                        case "/eventName":
                            if (i + 1 < args.Length)
                            {
                                eventName = args[i + 1];
                            }
                            break;
                        case "/useCache":
                            bUseSymbolCache = true;
                            break;
                        case "/showSource":
                            bShowSource = true;
                            break;
                        case "/addBookmark":
                            bAddBookmark = true;
                            break;
                        default:
                            break;
                    }
                }

                long procThreadID = (pixStorageWrap.captureFacts.targetProcessId << 32) | threadID;

                if (bUseSymbolCache)
                {
                    debugOutput.Text("Deserialize symbol.");
                    pixStorageWrap.DeserializeSymbolData();
                }

                debugOutput.Text("");

                CancellationTokenSource cancelTokenSource = new CancellationTokenSource();

                if (bGetSamples)
                {
                    pixStorageWrap.GetSampledFunction(bUseSymbolCache, bShowSource, procThreadID, cancelTokenSource.Token);
                }
                if (bGetContextSwitch)
                {
                    pixStorageWrap.GetContextSwitch(bUseSymbolCache, bShowSource, procThreadID, cancelTokenSource.Token);
                }
                if (bLongestEvent)
                {
                    pixStorageWrap.LongestEvent(eventName, bAddBookmark, cancelTokenSource.Token);
                }
                if (bDelayedThread)
                {
                    pixStorageWrap.DelayedThread(bUseSymbolCache, bShowSource, cancelTokenSource.Token);
                }
                if (bInfoSummary)
                {
                    pixStorageWrap.InformationSummaryForPerformanceReview();
                }
                if (bRunAllSampleQueries)
                {
                    try
                    {
                        string[] allSampleQueryFilePath = Directory.GetFiles(System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SampleQueries"), "*.sql");
                        if (allSampleQueryFilePath.Length == 0)
                        {
                            debugOutput.Text("Sample queries not found");
                        }
                        else
                        {
                            foreach (string sampleQueryFilePath in allSampleQueryFilePath)
                            {
                                using (StreamReader streamReader = new StreamReader(sampleQueryFilePath))
                                {
                                    string queryName = System.IO.Path.GetFileName(sampleQueryFilePath);
                                    string query = streamReader.ReadToEnd();
                                    debugOutput.Text(queryName);
                                    pixStorageWrap.GetFreeSQLInputResult(query);
                                }
                            }
                        }
                    }
                    catch (Exception e)
                    {
                        _ = e;
                        debugOutput.Text(" SampleQueries folder is not found.");
                    }
                }
            }
        }
    }
}
