using Microsoft.Gaming.WdRemoteApi;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private WdCancellationHandleWrapper? _cancellationHandleWrapper;
        
        private static ulong _totalBytesToDeploy = 0;

        private ProcThreadId _remoteProcessId;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void BrowseFolder_Click(object sender, RoutedEventArgs e)
        {
            using var dialog = new System.Windows.Forms.FolderBrowserDialog();
            dialog.Description = "Select folder to deploy";
            dialog.UseDescriptionForTitle = true;
            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                FolderPathTextBox.Text = dialog.SelectedPath;
            }
        }

        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        public unsafe static HRESULT FileProgressUpdate(
            nuint fileProgressCount,
            WdCopyFileProgressInfo* fileUpdates,
            WdCopyOperationSummary* summary,
            void* context)
        {
            try
            {
                if (fileUpdates == null || fileProgressCount == 0)
                {
                    return new HRESULT(0);
                }

                _totalBytesToDeploy = summary->totalByteCount;

                var fileProgressLines = new List<string>();
                ulong deployedBytes = summary->bytesTransferredCount;

                for (nuint i = 0; i < fileProgressCount; i++)
                {
                    WdCopyFileProgressInfo* info = &fileUpdates[i];
                    string? filePath = info->relativeFilePath.ToString();

                    if (info->fileSize > 0)
                    {
                        double pct = (double)info->bytesTransferred / info->fileSize * 100;
                        if (info->bytesTransferred == info->fileSize)
                        {
                            fileProgressLines.Add($"[Completed] {filePath}\n");
                        }
                        else
                        {
                            fileProgressLines.Add($"[Deploy] {filePath} ({info->bytesTransferred:N0} / {info->fileSize:N0} bytes - {pct:F1}%)\n");
                        }
                    }
                    else
                    {
                        fileProgressLines.Add($"[Completed] {filePath}\n");
                    }
                }

                ulong totalFiles = summary->totalFileCount;
                ulong totalBytes = summary->totalByteCount;
                ulong filesCompleted = summary->filesCompletedCount;

                System.Windows.Application.Current?.Dispatcher.InvokeAsync(() =>
                {
                    var mainWindow = System.Windows.Application.Current?.MainWindow as MainWindow;
                    if (mainWindow != null)
                    {
                        foreach (var line in fileProgressLines)
                        {
                            mainWindow.AppendOutput(line);
                        }

                        if (totalBytes > 0)
                        {
                            double overallProgress = Math.Min((double)deployedBytes / totalBytes * 100, 100.0);
                            mainWindow.DeploymentProgressBar.Value = overallProgress;
                            mainWindow.ProgressPercentText.Text = $"{overallProgress:F1}%";

                            if (filesCompleted >= totalFiles)
                            {
                                mainWindow.ProgressStatusText.Text = "Deployment completed.";
                            }
                            else
                            {
                                mainWindow.ProgressStatusText.Text = $"Deployed {deployedBytes:N0} / {totalBytes:N0} bytes ({filesCompleted:N0} / {totalFiles:N0} files)";
                            }
                        }
                    }
                }, System.Windows.Threading.DispatcherPriority.Background);

                return new HRESULT(0);
            }
            catch
            {
                return new HRESULT(-1);
            }
        }

        private async void Deploy_Click(object sender, RoutedEventArgs e)
        {
            string ip = DeviceIpTextBox.Text.Trim();
            string folder = FolderPathTextBox.Text.Trim();
            string destination = DestinationPathTextBox.Text.Trim();

            if (string.IsNullOrWhiteSpace(ip))
            {
                System.Windows.MessageBox.Show("Please enter a device name or IP Address.");
                return;
            }
            if (string.IsNullOrWhiteSpace(folder))
            {
                System.Windows.MessageBox.Show("Select a deployment folder.");
                return;
            }
            if (string.IsNullOrWhiteSpace(destination))
            {
                System.Windows.MessageBox.Show("Enter destination folder name.");
                return;
            }

            _cancellationHandleWrapper = new WdCancellationHandleWrapper();

            
            // Disable deploy button during deployment
            var deployButton = sender as System.Windows.Controls.Button;
            if (deployButton != null)
            {
                deployButton.IsEnabled = false;
            }

            OutputTextBox.Clear();
            
            // Reset progress tracking
            _totalBytesToDeploy = 0;
            
            AppendOutput($"> Deploying to {ip}:{destination} ...\n\n");
            AppendOutput($"Source: {folder}\n");
            AppendOutput($"Destination: {destination}\n\n");

            string includeFilePattern = IncludeFilePatternTextBox.Text.Trim();
            string excludeFilePattern = ExcludeFilePatternTextBox.Text.Trim();
            string excludeDirPattern = ExcludeDirPatternTextBox.Text.Trim();
            string includeFileAttribText = IncludeFileAttributesTextBox.Text.Trim();
            string excludeFileAttribText = ExcludeFileAttributesTextBox.Text.Trim();

            try
            {
                // Build search options from advanced options UI
                CopySearchOptions searchOptions = new CopySearchOptions();

                if (!string.IsNullOrEmpty(includeFilePattern))
                    searchOptions.IncludeFilePattern = includeFilePattern;
                if (!string.IsNullOrEmpty(excludeFilePattern))
                    searchOptions.ExcludeFilePattern = excludeFilePattern;
                if (!string.IsNullOrEmpty(excludeDirPattern))
                    searchOptions.ExcludeDirPattern = excludeDirPattern;
                if (!string.IsNullOrEmpty(includeFileAttribText))
                {
                    if (ulong.TryParse(includeFileAttribText, out ulong includeAttribs))
                        searchOptions.IncludeFileAttributes = includeAttribs;
                    else
                    {
                        System.Windows.MessageBox.Show("Include File Attributes must be a valid number.");
                        return;
                    }
                }
                if (!string.IsNullOrEmpty(excludeFileAttribText))
                {
                    if (ulong.TryParse(excludeFileAttribText, out ulong excludeAttribs))
                        searchOptions.ExcludeFileAttributes = excludeAttribs;
                    else
                    {
                        System.Windows.MessageBox.Show("Exclude File Attributes must be a valid number.");
                        return;
                    }
                }

                WdCopyStatusCallbacks copyStatusCallbacks = new WdCopyStatusCallbacks();
                
                unsafe
                {
                    copyStatusCallbacks.copyFilesStatusCallback = &FileProgressUpdate;
                }
                copyStatusCallbacks.refreshRateMs = 500;

                HRESULT hr = await RemoteIteration.CopyAsync(
                    remoteDevice: ip,
                    localSourcePath: folder,
                    remoteDestPath: destination,
                    cancellationHandleWrapper: _cancellationHandleWrapper,
                    searchOptions: searchOptions,
                    statusCallbacks: copyStatusCallbacks);

                if (hr.Failed)
                {
                    AppendOutput($"\n✗ Deployment failed during copy: HRESULT 0x{hr.Value:X8}\n");
                    System.Windows.MessageBox.Show($"Deployment failed during copy with error code: 0x{hr.Value:X8}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                DeploymentProgressBar.Value = 100;
                ProgressPercentText.Text = "100%";
                ProgressStatusText.Text = "Deployment completed.";

                hr = await RemoteIteration.RegisterRemoteXboxGameAsync(ip, destination);

                if (hr.Failed)
                {
                    AppendOutput($"\n✗ Deployment failed during registration: HRESULT 0x{hr.Value:X8}\n");
                    System.Windows.MessageBox.Show($"Deployment failed during registration with error code: 0x{hr.Value:X8}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
            }
            catch (RemoteIterationException ex)
            {
                AppendOutput($"\n✗ Deployment failed: {ex.Message} (HRESULT: 0x{ex.HResult:X8})\n");
                System.Windows.MessageBox.Show($"Deployment failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);

                ProgressStatusText.Text = "Deployment failed.";
            }
            catch (Exception ex)
            {
                AppendOutput($"\n✗ Deployment failed: {ex.Message}\n");
                System.Windows.MessageBox.Show($"Deployment failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                
                ProgressStatusText.Text = "Deployment failed.";
            }
            finally
            {
                if (deployButton != null)
                {
                    deployButton.IsEnabled = true;
                }

                _cancellationHandleWrapper?.Dispose();
                _cancellationHandleWrapper = null;
            }
        }

        private async void Cancel_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if(_cancellationHandleWrapper == null)
                {
                    AppendOutput("\n⚠ No deployment to cancel.\n");
                    return;
                }

                await RemoteIteration.CancelDeployAsync(_cancellationHandleWrapper);
                
                OutputTextBox.Clear();
                AppendOutput($"Deploy cancelled \n\n");
            }
            catch (RemoteIterationException ex)
            {
                AppendOutput($"\n✗ Cancel failed: {ex.Message} (HRESULT: 0x{ex.HResult:X8})\n");
                System.Windows.MessageBox.Show($"Cancel failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            catch (Exception ex)
            {
                AppendOutput($"\n✗ Cancel failed: {ex.Message}\n");
                System.Windows.MessageBox.Show($"Cancel failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        
        private async void Launch_Click(object sender, RoutedEventArgs e)
        {
            await LaunchProcessAsync(launchOptions: null);
        }

        private async void LaunchSuspended_Click(object sender, RoutedEventArgs e)
        {
            var launchOptions = new WdLaunchOptions
            {
                launchMode = WdLaunchMode.Suspended
            };

            await LaunchProcessAsync(launchOptions);
        }

        private async Task LaunchProcessAsync(WdLaunchOptions? launchOptions)
        {
            string ip = DeviceIpTextBox.Text.Trim();
            string executable = ExecutablePathTextBox.Text.Trim();

            if (string.IsNullOrWhiteSpace(ip))
            {
                System.Windows.MessageBox.Show("Please enter a device name or IP Address.");
                return;
            }
            if (string.IsNullOrWhiteSpace(executable))
            {
                System.Windows.MessageBox.Show("Please enter an executable path.");
                return;
            }

            OutputTextBox.Clear();

            try
            {
                (HRESULT hr, ProcThreadId procThreadId) = await RemoteIteration.LaunchRemoteGameAsync(
                    remoteDevice: ip, remotePath: executable, launchOptions: launchOptions);

                if (hr.Failed)
                {
                    AppendOutput($"\n✗ Launch failed: HRESULT 0x{hr.Value:X8}\n");
                    System.Windows.MessageBox.Show($"Launch failed with error code: 0x{hr.Value:X8}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                _remoteProcessId = procThreadId;

                AppendOutput($"> Launching application on {ip} ...\n\n");
                AppendOutput($"Path: {executable}\n");
                AppendOutput($"\nDone. (Process ID: {_remoteProcessId.ProcessId}, Thread ID: {_remoteProcessId.ThreadId})\n");
            }
            catch (Exception ex)
            {
                AppendOutput($"\n✗ Launch failed: {ex.Message}\n");
                System.Windows.MessageBox.Show($"Launch failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private async void Resume_Click(object sender, RoutedEventArgs e)
        {
            string ip = DeviceIpTextBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(ip))
            {
                System.Windows.MessageBox.Show("Please enter a device name or IP Address.");
                return;
            }
            OutputTextBox.Clear();
            
            try
            {
                HRESULT hr = await RemoteIteration.ResumeGameAsync(ip);

                if (hr.Failed)
                {
                    AppendOutput($"{ip} failed to resume remote game with error 0x{hr.Value:X8}.");
                }
                else
                {
                    AppendOutput($"{ip} resumed remote game {_remoteProcessId.ProcessId} successfully.");
                }
            }
            catch (RemoteIterationException ex)
            {
                AppendOutput($"\n✗ Resume failed: {ex.Message} (HRESULT: 0x{ex.HResult:X8})\n");
                System.Windows.MessageBox.Show($"Resume failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            catch (Exception ex)
            {
                AppendOutput($"\n✗ Resume failed: {ex.Message}\n");
                System.Windows.MessageBox.Show($"Resume failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private async void Terminate_Click(object sender, RoutedEventArgs e)
        {
            string ip = DeviceIpTextBox.Text.Trim();

            if (string.IsNullOrWhiteSpace(ip))
            {
                System.Windows.MessageBox.Show("Please enter a device name or IP Address.");
                return;
            }

            OutputTextBox.Clear();
            
            try
            {
                HRESULT hr = await RemoteIteration.TerminateRemoteGameAsync(ip);

                if (hr.Failed)
                {
                    AppendOutput($"{ip} failed to terminate remote game with error 0x{hr.Value:X8}.");
                }
                else
                {
                    AppendOutput($"{ip} terminated remote game successfully.");
                }
            }
            catch (RemoteIterationException ex)
            {
                AppendOutput($"\n✗ Terminate failed: {ex.Message} (HRESULT: 0x{ex.HResult:X8})\n");
                System.Windows.MessageBox.Show($"Terminate failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            catch (Exception ex)
            {
                AppendOutput($"\n✗ Terminate failed: {ex.Message}\n");
                System.Windows.MessageBox.Show($"Terminate failed: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            
        }

        private void AppendOutput(string text)
        {
            OutputTextBox.AppendText(text);
            OutputTextBox.ScrollToEnd();
        }
    }
}