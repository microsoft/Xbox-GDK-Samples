using Microsoft.Gaming.WdRemoteApi;
using System.ComponentModel;
using System.Windows;
using System.Windows.Threading;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    public partial class MainWindow : Window
    {
        private const int OutputBatchSize = 128;
        private OperationState? _activeOperation;
        private readonly DispatcherTimer _progressTimer;

        public MainWindow()
        {
            InitializeComponent();
            _progressTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
            _progressTimer.Tick += ProgressTimer_Tick;
        }

        private void BrowseFolder_Click(object sender, RoutedEventArgs e)
        {
            if (_activeOperation != null)
                return;

            using var dialog = new System.Windows.Forms.FolderBrowserDialog();
            dialog.Description = "Select folder to deploy";
            dialog.UseDescriptionForTitle = true;
            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                FolderPathTextBox.Text = dialog.SelectedPath;
            }
        }

        private bool BeginOperation(OperationState operation)
        {
            if (_activeOperation != null)
                return false;

            _activeOperation = operation;
            SetInputsEnabled(false);
            OutputTextBox.Clear();
            return true;
        }

        private void EndOperation(OperationState operation)
        {
            _progressTimer.Stop();
            operation.Dispose();
            if (ReferenceEquals(_activeOperation, operation))
            {
                _activeOperation = null;
                CancelButton.IsEnabled = false;
                StopWaitingDeleteButton.IsEnabled = false;
                SetInputsEnabled(true);
            }
        }

        private void SetInputsEnabled(bool enabled)
        {
            DeviceInputs.IsEnabled = enabled;
            SourceInputs.IsEnabled = enabled;
            DestinationInputs.IsEnabled = enabled;
            AdvancedOptions.IsEnabled = enabled;
            DeployButton.IsEnabled = enabled;
            ExecutableInputs.IsEnabled = enabled;
            GameActions.IsEnabled = enabled;
            DeleteInputs.IsEnabled = enabled;
            DeleteButton.IsEnabled = enabled;
        }

        private void Window_Closing(object? sender, CancelEventArgs e)
        {
            if (_activeOperation == null)
                return;

            e.Cancel = true;
            AppendOutput("\nWait for the active operation to return before closing. " +
                "If it appears stalled, choose Cancel for copy or Stop waiting for delete when available, then wait for the call to return. " +
                "If it remains unresponsive, end RemoteIterationToolsSample.exe in Task Manager as a last resort. " +
                "Force-closing does not undo completed transfers or deletions, or confirm that the remote operation stopped.\n");
        }

        private void ProgressTimer_Tick(object? sender, EventArgs e)
        {
            OperationState? operation = _activeOperation;
            if (operation == null || !operation.CopyActive)
                return;

            if (!DrainCopyUpdates(operation))
                _progressTimer.Stop();
        }

        private bool DrainCopyUpdates(OperationState operation)
        {
            if (!ReferenceEquals(_activeOperation, operation))
                return false;

            try
            {
                string output = operation.TakeOutputBatch(OutputBatchSize);
                Exception? callbackFailure = operation.CallbackFailure;
                if (callbackFailure != null && !operation.CallbackFailureReported)
                    output += FormatCallbackFailure(callbackFailure);
                if (output.Length > 0)
                    AppendOutput(output);
                if (callbackFailure != null)
                    operation.CallbackFailureReported = true;

                if (operation.LatestSummary is WdCopyOperationSummary summary)
                {
                    double percentage = summary.totalByteCount == 0
                        ? 0
                        : Math.Min((double)summary.bytesTransferredCount / summary.totalByteCount * 100, 100);
                    DeploymentProgressBar.Value = percentage;
                    ProgressPercentText.Text = $"{percentage:F1}%";
                    if (!operation.CancellationRequested)
                    {
                        ProgressStatusText.Text =
                            $"Copied {summary.bytesTransferredCount:N0} / {summary.totalByteCount:N0} bytes " +
                            $"({summary.filesCompletedCount:N0} / {summary.totalFileCount:N0} files); " +
                            (operation.CopyActive ? "waiting for copy result." : "displaying remaining copy output.");
                    }
                }
                return true;
            }
            catch (Exception ex)
            {
                // WPF processing errors need the same visible outcome as native callback errors.
                operation.RecordCallbackFailure(ex);
                ReportCallbackFailure(operation);
                return false;
            }
        }

        private async Task DrainRemainingCopyUpdatesAsync(OperationState operation)
        {
            while (ReferenceEquals(_activeOperation, operation))
            {
                if (!DrainCopyUpdates(operation) || !operation.HasPendingOutput)
                    return;

                // Let input and rendering run between batches, even after the native call returns.
                await System.Windows.Threading.Dispatcher.Yield(DispatcherPriority.Background);
            }
        }

        private static string FormatCallbackFailure(Exception ex) =>
            $"\n✗ Callback processing failed: {ex.Message} (HRESULT 0x{ex.HResult:X8}). " +
            "A callback failure does not stop native copy or replace its final HRESULT.\n";

        private void ReportCallbackFailure(OperationState operation)
        {
            if (operation.CallbackFailure is Exception ex && !operation.CallbackFailureReported)
            {
                operation.CallbackFailureReported = true;
                AppendOutput(FormatCallbackFailure(ex));
            }
        }

        private async void Deploy_Click(object sender, RoutedEventArgs e)
        {
            if (_activeOperation != null)
                return;

            string ip = DeviceIpTextBox.Text.Trim();
            string folder = FolderPathTextBox.Text.Trim();
            string destination = DestinationPathTextBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(ip) || string.IsNullOrWhiteSpace(folder) || string.IsNullOrWhiteSpace(destination))
            {
                System.Windows.MessageBox.Show("Enter a device, a local source folder, and a remote destination folder.");
                return;
            }

            var operation = new OperationState("Deployment", ip, destination)
            {
                LocalSourcePath = folder,
                SearchOptions = new CopySearchOptions
                {
                    IncludeFilePattern = IncludeFilePatternTextBox.Text.Trim(),
                    ExcludeFilePattern = ExcludeFilePatternTextBox.Text.Trim(),
                    ExcludeDirPattern = ExcludeDirPatternTextBox.Text.Trim(),
                    IncludeFileAttributes = IncludeFileAttributesSelector.Mask,
                    ExcludeFileAttributes = ExcludeFileAttributesSelector.Mask
                }
            };
            if (!BeginOperation(operation))
                return;

            try
            {
                DeploymentProgressBar.Value = 0;
                ProgressPercentText.Text = "0%";
                ProgressStatusText.Text = "Copying; waiting for progress.";
                AppendOutput($"> Deploying to {operation.Device}:{operation.RemotePath}\nSource: {operation.LocalSourcePath}\n\n");
                WdCopyStatusCallbacks callbacks = operation.CreateCopyCallbacks();
                operation.CopyActive = true;
                CancelButton.IsEnabled = true;
                _progressTimer.Start();

                HRESULT hr = await RemoteIteration.CopyAsync(
                    remoteDevice: operation.Device,
                    localSourcePath: operation.LocalSourcePath,
                    remoteDestPath: operation.RemotePath,
                    cancellationHandleWrapper: operation.CancellationHandle,
                    searchOptions: operation.SearchOptions,
                    statusCallbacks: callbacks);

                operation.CopyActive = false;
                CancelButton.IsEnabled = false;
                _progressTimer.Stop();
                // Callbacks only enqueue owned data. Drain before setting the authoritative result.
                AppendOutput($"\nCopy returned HRESULT 0x{hr.Value:X8}.\n");
                ProgressStatusText.Text = "Copy returned; displaying remaining output.";
                await DrainRemainingCopyUpdatesAsync(operation);
                if (hr.Failed)
                {
                    ProgressStatusText.Text = "Deployment failed during copy.";
                    AppendOutput($"✗ Copy failed: HRESULT 0x{hr.Value:X8}." +
                        (operation.CancellationRequested ? " Cancellation was also requested; this does not establish the failure's cause." : "") +
                        " Remote path validation skipped.\n");
                    return;
                }
                if (operation.CancellationRequested)
                {
                    ProgressStatusText.Text = "Cancellation requested; deployment completeness not confirmed.";
                    AppendOutput("Cancellation was requested. Copy returning success does not confirm a complete deployment. Remote path validation skipped.\n");
                    return;
                }
                if (operation.CallbackFailure != null)
                {
                    ProgressStatusText.Text = "Copy returned success, but callback processing failed.";
                    AppendOutput("Deployment status is not confirmed because callback processing failed. Remote path validation skipped.\n");
                    return;
                }

                ProgressStatusText.Text = "Copy finished. Validating remote path.";
                AppendOutput("Validating remote path with WdRegisterRemoteXboxGame (not platform game registration).\n");
                hr = await RemoteIteration.RegisterRemoteXboxGameAsync(operation.Device, operation.RemotePath);
                AppendOutput($"Remote path validation returned HRESULT 0x{hr.Value:X8}.\n");
                if (hr.Failed)
                {
                    ProgressStatusText.Text = "Copy finished; remote path validation failed.";
                    return;
                }

                DeploymentProgressBar.Value = 100;
                ProgressPercentText.Text = "100%";
                ProgressStatusText.Text = "Deployment completed; remote path validated.";
            }
            catch (Exception ex)
            {
                operation.CopyActive = false;
                CancelButton.IsEnabled = false;
                _progressTimer.Stop();
                await DrainRemainingCopyUpdatesAsync(operation);
                ShowOperationFailure(operation, ex);
                ProgressStatusText.Text = "Deployment failed.";
            }
            finally
            {
                operation.CopyActive = false;
                EndOperation(operation);
            }
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            OperationState? operation = _activeOperation;
            if (operation == null || !operation.CopyActive || operation.CancellationRequested || operation.CancellationHandle == null)
                return;

            operation.CancellationRequested = true;
            CancelButton.IsEnabled = false;
            ProgressStatusText.Text = "Cancellation requested; waiting for copy to return.";
            AppendOutput("\nCancellation requested; waiting for the copy result.\n");
            try
            {
                // The signal is nonblocking. Running it on the UI thread prevents the awaited
                // copy continuation from disposing its handle while cancellation uses it.
                HRESULT hr = RemoteIteration.RequestCopyCancellation(operation.CancellationHandle);
                AppendOutput($"Cancellation signal returned HRESULT 0x{hr.Value:X8}.\n");
                if (hr.Failed)
                {
                    ProgressStatusText.Text = "Cancellation signal failed; waiting for copy to return.";
                    AppendOutput("✗ Cancellation signal failed; copy may continue. Cancellation intent is retained.\n");
                }
            }
            catch (Exception ex)
            {
                AppendOutput($"✗ Cancellation signal failed: {ex.Message} (HRESULT 0x{ex.HResult:X8}). Copy may continue.\n");
                ProgressStatusText.Text = "Cancellation signal failed; waiting for copy to return.";
            }
        }

        private async void Launch_Click(object sender, RoutedEventArgs e)
        {
            await LaunchProcessAsync(null);
        }

        private async void LaunchSuspended_Click(object sender, RoutedEventArgs e)
        {
            await LaunchProcessAsync(new WdLaunchOptions { launchMode = WdLaunchMode.Suspended });
        }

        private async Task LaunchProcessAsync(WdLaunchOptions? launchOptions)
        {
            string ip = DeviceIpTextBox.Text.Trim();
            string executable = ExecutablePathTextBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(ip) || string.IsNullOrWhiteSpace(executable))
            {
                System.Windows.MessageBox.Show("Enter a device and a remote executable path.");
                return;
            }

            var operation = new OperationState("Launch", ip, executable) { LaunchOptions = launchOptions };
            if (!BeginOperation(operation))
                return;

            try
            {
                AppendOutput($"> Launching {operation.RemotePath} on {operation.Device}" +
                    (operation.LaunchOptions?.launchMode == WdLaunchMode.Suspended ? " (suspended)" : "") + " ...\n");
                (HRESULT hr, ProcThreadId process) = await RemoteIteration.LaunchRemoteGameAsync(
                    operation.Device, operation.RemotePath, launchOptions: operation.LaunchOptions);
                AppendOutput(hr.Failed
                    ? $"✗ Launch failed: HRESULT 0x{hr.Value:X8}.\n"
                    : $"Launch succeeded (Process ID: {process.ProcessId}, Thread ID: {process.ThreadId}).\n");
            }
            catch (Exception ex)
            {
                ShowOperationFailure(operation, ex);
            }
            finally
            {
                EndOperation(operation);
            }
        }

        private async void Resume_Click(object sender, RoutedEventArgs e)
        {
            await ControlGameAsync(resume: true);
        }

        private async void Terminate_Click(object sender, RoutedEventArgs e)
        {
            await ControlGameAsync(resume: false);
        }

        private async Task ControlGameAsync(bool resume)
        {
            string ip = DeviceIpTextBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(ip))
            {
                System.Windows.MessageBox.Show("Enter a device name or IP address.");
                return;
            }

            var operation = new OperationState(resume ? "Resume" : "Terminate", ip);
            if (!BeginOperation(operation))
                return;

            try
            {
                HRESULT hr = resume
                    ? await RemoteIteration.ResumeGameAsync(operation.Device)
                    : await RemoteIteration.TerminateRemoteGameAsync(operation.Device);
                AppendOutput($"{operation.Name} on {operation.Device} " +
                    (hr.Failed ? "failed" : "succeeded") + $" (HRESULT 0x{hr.Value:X8}).\n");
            }
            catch (Exception ex)
            {
                ShowOperationFailure(operation, ex);
            }
            finally
            {
                EndOperation(operation);
            }
        }

        private void ShowOperationFailure(OperationState operation, Exception ex)
        {
            AppendOutput($"\n✗ {operation.Name} failed: {ex.Message} (HRESULT 0x{ex.HResult:X8})." +
                (operation.CancellationRequested ? " Cancellation was also requested." : "") +
                (operation.StopWaitingRequested ? " Remote deletion may continue." : "") + "\n");
        }

        private void AppendOutput(string text)
        {
            OutputTextBox.AppendText(text);
            OutputTextBox.ScrollToEnd();
        }
    }
}
