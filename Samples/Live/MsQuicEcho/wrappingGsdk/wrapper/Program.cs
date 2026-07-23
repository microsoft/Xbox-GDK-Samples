using System;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.Playfab.Gaming.GSDK.CSharp;
using System.IO;
using System.Reflection;
using System.Linq;
using System.Runtime.InteropServices;

namespace wrapper
{
    class Program
    {
        private static Process gameProcess;
        private static IDictionary<string, string> activeConfig;
        private const string portName = "gameport";
        private const string commandExe = "cmd";
        private static int _listeningPort;


        static void Main(string[] args)
        {
            if (args.Length <= 1 || args[0] != "-g")
            {
                Console.WriteLine("Usage: wrapper.exe -g fakegame.exe args...");
                return;
            }

            string gameserverExe = args[1];

            // check here for the full guide on integrating with the GSDK 
            // https://docs.microsoft.com/en-us/gaming/playfab/features/multiplayer/servers/integrating-game-servers-with-gsdk

            LogMessage("Wrapper sample for Azure PlayFab Multiplayer Servers");
            
            LogMessage("Attempting to register GSDK callbacks");
            RegisterGSDKCallbacksAndStartGSDK();
            LogMessage("GSDK callback registration completed");
            
            LogMessage("Attempting to start game process");
            InitiateAndWaitForGameProcess(gameserverExe, args.Skip(2));
            LogMessage("Game process has exited");
        }

        // starts main game process and wait for it to complete
        public static void InitiateAndWaitForGameProcess(string gameserverExe, IEnumerable<string> args)
        {
            // Here we're starting the script that initiates the game process

            var gameServerConnectionInfo = GameserverSDK.GetGameServerConnectionInfo();
            var portInfo = gameServerConnectionInfo.GamePortsConfiguration.Where(x=>x.Name == portName);

            // When Wrapper is running in a container, Port Information (Port Name, Port Number, and Protocol) is already set as build configuration.
            // For example, if you already set port number as 80 in container build configuration, activeConfig will return 80 as port number.
            // But if Wrapper is running as a process, port will be mapped internally by MPS, so different number will be dynamically assigned. 
            if(portInfo.Count() > 0)
            {
                LogMessage($"{portName} was found in GSDK Config Settings.");
                _listeningPort = portInfo.Single().ServerListeningPort;
            }
            else
            {
                LogMessage($"Cannot find {portName} in GSDK Config Settings. Please make sure the LocalMultiplayerAgent is running " +
                             $"and that the MultiplayerSettings.json file includes correct {portName} as a GamePort Name.");
                return;
            }

            // Check if there is any process already using the port (_listeningPort). This will only work for Windows.
            if (CheckIfPortIsUsed(gameserverExe)) { return; };

            // We pass port number as a 3rd argument when we start fakegame.exe 
            // Port number is grabbed via GSDK and will be passed to fake game as a listening port.
            gameProcess = StartProcess(gameserverExe, string.Join(' ', args.Append(_listeningPort.ToString())));
            // as part of wrapping the main game server executable,
            // we create event handlers to process the output from the game (standard output/standard error)
            // based on this output, we will activate the server and process connected players
            gameProcess.OutputDataReceived += DataReceived;
            gameProcess.ErrorDataReceived += DataReceived;
            // start reading output (stdout/stderr) from the game
            gameProcess.BeginOutputReadLine();
            gameProcess.BeginErrorReadLine();

            // Call this when your game is done initializing and players can connect
            // Note: This is a blocking call, and will return when this game server is either allocated or terminated
            if(GameserverSDK.ReadyForPlayers())
            {
                // After allocation, we can grab the session cookie from the config
                activeConfig = GameserverSDK.getConfigSettings();

                var connectedPlayers = new List<ConnectedPlayer>();
                // initial players includes the list of the players that are allowed to connect to the game
                // they might or might not end up connecting
                // in this sample we're nevertheless adding them to the list 
                foreach (var player in GameserverSDK.GetInitialPlayers())
                {
                    connectedPlayers.Add(new ConnectedPlayer(player));
                }
                GameserverSDK.UpdateConnectedPlayers(connectedPlayers);
                
                if (activeConfig.TryGetValue(GameserverSDK.SessionCookieKey, out string sessionCookie))
                {
                    LogMessage($"The session cookie from the allocation call is: {sessionCookie}");
                }

                // if you are using matchmaking, this value will be set
                if (activeConfig.TryGetValue("PF_MATCH_QUEUE_NAME", out string matchQueueName))
                {
                    LogMessage($"The queue name from matchmaking is: {matchQueueName}");
                }
            }
            else
            {
                // No allocation happened, the server is getting terminated (likely because there are too many already in standing by)
                LogMessage("Server is getting terminated.");
                gameProcess?.Kill(); // we still need to call WaitForExit https://docs.microsoft.com/en-us/dotnet/api/system.diagnostics.process.kill?view=netcore-3.1#remarks
            }

            // wait till it exits or crashes
            gameProcess.WaitForExit();
        }

        // GSDK event handlers - we're setting them on the startup of the app
        public static void RegisterGSDKCallbacksAndStartGSDK()
        {
            // OnShutDown will be called when developer calls the ShutdDownMultiplayerServer API 
            // https://docs.microsoft.com/en-us/rest/api/playfab/multiplayer/multiplayerserver/shutdownmultiplayerserver?view=playfab-rest
            GameserverSDK.RegisterShutdownCallback(OnShutdown);
            // This callback will be called on every heartbeat to check if your game is healthy. So it should return quickly
            GameserverSDK.RegisterHealthCallback(IsHealthy);
            // this callback will be called to notify us that Azure will perform maintenance on the VM
            // you can see more details about Azure VM maintenance here https://docs.microsoft.com/en-gb/azure/virtual-machines/maintenance-and-updates?toc=/azure/virtual-machines/windows/toc.json&bc=/azure/virtual-machines/windows/breadcrumb/toc.json
            GameserverSDK.RegisterMaintenanceCallback(OnMaintenanceScheduled);
            
            // Call this while your game is initializing; it will start sending a heartbeat to our agent 
            // since our game server will transition to a standingBy state, we should call this when we're confident that our game server will not crash
            // here we're calling it just before we start our game server process - normally we would call it inside our game code
            GameserverSDK.Start();
        }

        public static Process StartProcess(string exeName, string args)
        {
            var process = new Process()
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = $"{exeName}",
                    Arguments = args,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                }
            };
            
            process.Start();
            return process;
        }

        // runs when we received data (stdout/stderr) from our game server process
        public static void DataReceived(object sender, DataReceivedEventArgs e)
        {
            LogMessage(e.Data); // used for debug purposes only - you can use `docker logs <container_id> to see the stdout logs
        }

        static void OnShutdown()
        {
            LogMessage("Shutting down...");
            gameProcess?.Kill();
            Environment.Exit(0);
        }

        static bool IsHealthy()
        {
            // returns whether this game server process is healthy
            // here we're doing a simple check if our game wrapper is still alive
            return gameProcess != null;
        }

        static void OnMaintenanceScheduled(DateTimeOffset time)
        {
            LogMessage($"Maintenance Scheduled at: {time}");
        }

        private static void LogMessage(string message)
        {
            Console.WriteLine(message);
            // This will add your log line to the GSDK log file, alongside other information logged by the GSDK
            GameserverSDK.LogMessage(message);
        }

        private static string ExecuteCommandAndCaptureOutput(string commandName, string arguments)
        {
            Process process = StartProcess(commandName, arguments);
            string commandOut = process.StandardOutput.ReadToEnd();

            try
            {
                process.WaitForExit(TimeSpan.FromSeconds(2).Milliseconds);
            }
            catch (Exception exp)
            {
                Console.WriteLine(exp.ToString());
            }
            return commandOut;
        }

        public static Process GetProcessPortInUse()
        {
            Process process = null;

            // GetProcessPortInUse() works only for Windows as it executes netstat command. 
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                // execute netstat command for the given port
                string commandArgument = string.Format("/c netstat -an -o -p tcp|findstr \":{0}.*LISTENING\"", _listeningPort);

                string commandOut = ExecuteCommandAndCaptureOutput(commandExe, commandArgument);
                if (string.IsNullOrEmpty(commandOut))
                {
                    // port is not in use
                    return null;
                }

                // stringTokens example: [Protocol, host:port, Foreign Address, LISTENING, PID]
                var stringTokens = commandOut.Split(default(Char[]), StringSplitOptions.RemoveEmptyEntries);

                // split host:port
                var hostPortTokens = stringTokens[1].Split(new char[] { ':' });
                int portFromHostPortToken = 0;

                // check if the format of host:port is not valid
                if (hostPortTokens.Length < 2 || !int.TryParse(hostPortTokens[1], out portFromHostPortToken))
                {
                    return null;
                }

                if (portFromHostPortToken != _listeningPort)
                {
                    return null;
                }

                // get processID from the token
                int processID = 0;
                if (!int.TryParse(stringTokens[4].Trim(), out processID))
                {
                    return null;
                }

                process = Process.GetProcessById(processID);
            }

            return process;
        }

        public static bool CheckIfPortIsUsed(string gameserverExe)
        {
            Process process = GetProcessPortInUse();

            if (process != null)
            {
                string fakeGameProcessName = gameserverExe.Split(".")[0];
                if (process.ProcessName == fakeGameProcessName)
                {
                    Console.WriteLine($"fake game already exists. Port:{_listeningPort} is already in use.");
                }
                else
                {
                    Console.WriteLine($"Port:{_listeningPort} is already in use. Please set the correct Port number");
                }

                return true;
            }

            return false;
        }
    }
}
