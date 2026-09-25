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

        private void BrowseRetrieveDestination_Click(object sender, RoutedEventArgs e)
        {
            if (_activeOperation != null)
                return;

            using var dialog = new System.Windows.Forms.FolderBrowserDialog();
            dialog.Description = "Select local destination for retrieved artifacts";
            dialog.UseDescriptionForTitle = true;
            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                RetrieveDestinationPathTextBox.Text = dialog.SelectedPath;
            }
        }

        private async void Retrieve_Click(object sender, RoutedEventArgs e)
        {
            if (_activeOperation != null)
                return;

            string device = DeviceIpTextBox.Text.Trim();
            string remoteSource = RetrieveSourcePathTextBox.Text.Trim();
            string localDestination = RetrieveDestinationPathTextBox.Text.Trim();
            string? commonRootAlias = string.IsNullOrWhiteSpace(RetrieveCommonRootAliasTextBox.Text)
                ? null
                : RetrieveCommonRootAliasTextBox.Text.Trim();

            if (string.IsNullOrWhiteSpace(device) ||
                string.IsNullOrWhiteSpace(remoteSource) ||
                string.IsNullOrWhiteSpace(localDestination))
            {
                System.Windows.MessageBox.Show(
                    Messages.RetrieveRequiredInputsMessage,
                    Messages.RetrieveInvalidInputsTitle,
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                return;
            }

            if (!System.IO.Directory.Exists(localDestination))
            {
                System.Windows.MessageBox.Show(
                    string.Format(Messages.RetrieveDestinationNotFoundFormat, localDestination),
                    Messages.RetrieveInvalidInputsTitle,
                    MessageBoxButton.OK,
                    MessageBoxImage.Warning);
                return;
            }

            var operation = new OperationState("Retrieve", device)
            {
                SourcePath = remoteSource,
                DestinationPath = localDestination,
                CopyDirection = WdCopyDirection.CopyFrom,
                CommonRootAlias = commonRootAlias
            };
            if (!BeginOperation(operation))
                return;

            try
            {
                RetrieveProgressBar.IsIndeterminate = true;
                RetrieveProgressStatusText.Text = Messages.RetrieveProgressActive;
                string commonRootDisplayName = operation.CommonRootAlias ?? Messages.DefaultCommonRootDisplayName;
                AppendOutput(string.Format(
                    Messages.RetrieveStartingFormat,
                    operation.Device,
                    operation.SourcePath,
                    operation.DestinationPath,
                    commonRootDisplayName));

                WdCopyStatusCallbacks callbacks = operation.CreateCopyCallbacks();
                operation.CopyActive = true;
                SetCopyCancelEnabled(operation, true);
                _progressTimer.Start();

                HRESULT hr = await RemoteIteration.CopyAsync(
                    remoteDevice: operation.Device,
                    sourcePath: operation.SourcePath,
                    destinationPath: operation.DestinationPath,
                    copyDirection: operation.CopyDirection,
                    commonRootAlias: operation.CommonRootAlias,
                    cancellationHandleWrapper: operation.CancellationHandle,
                    statusCallbacks: callbacks);

                operation.CopyActive = false;
                SetCopyCancelEnabled(operation, false);
                _progressTimer.Stop();
                RetrieveProgressBar.IsIndeterminate = false;
                AppendOutput(string.Format(Messages.RetrieveCopyReturnedFormat, hr.Value));
                SetCopyStatus(operation, Messages.RetrieveDisplayingRemainingOutput);
                await DrainRemainingCopyUpdatesAsync(operation);
                if (hr.Failed)
                {
                    SetCopyStatus(operation, Messages.RetrieveCopyFailedStatus);
                    AppendOutput(string.Format(Messages.RetrieveCopyFailedFormat, hr.Value));
                    if (operation.CancellationRequested)
                        AppendOutput(Messages.CopyFailureCancellationNote);
                    return;
                }
                if (operation.CancellationRequested)
                {
                    SetCopyStatus(operation, Messages.RetrieveCancellationUnconfirmedStatus);
                    AppendOutput(Messages.RetrieveCancellationUnconfirmedMessage);
                    return;
                }
                if (operation.CallbackFailure != null)
                {
                    SetCopyStatus(operation, Messages.RetrieveCallbackFailureStatus);
                    AppendOutput(Messages.RetrieveCallbackFailureMessage);
                    return;
                }

                SetCopyStatus(operation, Messages.RetrieveCompletedStatus);
                AppendOutput(Messages.RetrieveCompletedMessage);
            }
            catch (Exception ex)
            {
                operation.CopyActive = false;
                SetCopyCancelEnabled(operation, false);
                _progressTimer.Stop();
                RetrieveProgressBar.IsIndeterminate = false;
                await DrainRemainingCopyUpdatesAsync(operation);
                ShowOperationFailure(operation, ex);
                SetCopyStatus(operation, Messages.RetrieveOperationFailed);
            }
            finally
            {
                operation.CopyActive = false;
                RetrieveProgressBar.IsIndeterminate = false;
                EndOperation(operation);
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
                CancelRetrieveButton.IsEnabled = false;
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
            RetrieveSourcePathTextBox.IsEnabled = enabled;
            RetrieveCommonRootAliasTextBox.IsEnabled = enabled;
            RetrieveDestinationPathTextBox.IsEnabled = enabled;
            RetrieveBrowseButton.IsEnabled = enabled;
            RetrieveButton.IsEnabled = enabled;
        }

        private void SetCopyCancelEnabled(OperationState operation, bool enabled)
        {
            if (operation.CopyDirection == WdCopyDirection.CopyFrom)
                CancelRetrieveButton.IsEnabled = enabled;
            else
                CancelButton.IsEnabled = enabled;
        }

        private void SetCopyStatus(OperationState operation, string status)
        {
            if (operation.CopyDirection == WdCopyDirection.CopyFrom)
                RetrieveProgressStatusText.Text = status;
            else
                ProgressStatusText.Text = status;
        }

        private void Window_Closing(object? sender, CancelEventArgs e)
        {
            if (_activeOperation == null)
                return;

            e.Cancel = true;
            AppendOutput(Messages.ActiveOperationClosingGuidance);
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

                // CopyFrom summary totals describe files completed so far, not the full job.
                // Keep Retrieve indeterminate and use its per-file callback messages instead.
                if (operation.CopyDirection == WdCopyDirection.CopyTo &&
                    operation.LatestSummary is WdCopyOperationSummary summary)
                {
                    double percentage = summary.totalByteCount == 0
                        ? 0
                        : Math.Min((double)summary.bytesTransferredCount / summary.totalByteCount * 100, 100);
                    DeploymentProgressBar.Value = percentage;
                    ProgressPercentText.Text = $"{percentage:F1}%";
                    if (!operation.CancellationRequested)
                    {
                        string progressState = operation.CopyActive
                            ? Messages.CopyProgressWaitingForResult
                            : Messages.CopyProgressDisplayingRemainingOutput;
                        ProgressStatusText.Text = string.Format(
                            Messages.CopyProgressStatusFormat,
                            summary.bytesTransferredCount,
                            summary.totalByteCount,
                            summary.filesCompletedCount,
                            summary.totalFileCount,
                            progressState);
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
            string.Format(Messages.CallbackFailureFormat, ex.Message, ex.HResult);

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
                System.Windows.MessageBox.Show(Messages.DeployRequiredInputsMessage);
                return;
            }

            var operation = new OperationState("Deployment", ip)
            {
                SourcePath = folder,
                DestinationPath = destination,
                CopyDirection = WdCopyDirection.CopyTo,
                SearchOptions = new CopySearchOptions
                {
                    IncludeFilePattern = IncludeFilePatternTextBox.Text.Trim(),
                    ExcludeFilePattern = ExcludeFilePatternTextBox.Text.Trim(),
                    ExcludeDirPattern = ExcludeDirPatternTextBox.Text.Trim(),
                    IncludeFileAttributes = IncludeFileAttributesSelector.Mask,
                    ExcludeFileAttributes = ExcludeFileAttributesSelector.Mask,
                    IncludeDirectoryAttributes = IncludeDirectoryAttributesSelector.Mask,
                    ExcludeDirectoryAttributes = ExcludeDirectoryAttributesSelector.Mask
                }
            };
            if (!BeginOperation(operation))
                return;

            try
            {
                DeploymentProgressBar.Value = 0;
                ProgressPercentText.Text = "0%";
                ProgressStatusText.Text = "Copying; waiting for progress.";
                AppendOutput(string.Format(
                    Messages.DeployStartingFormat,
                    operation.Device,
                    operation.DestinationPath,
                    operation.SourcePath));
                WdCopyStatusCallbacks callbacks = operation.CreateCopyCallbacks();
                operation.CopyActive = true;
                SetCopyCancelEnabled(operation, true);
                _progressTimer.Start();

                HRESULT hr = await RemoteIteration.CopyAsync(
                    remoteDevice: operation.Device,
                    sourcePath: operation.SourcePath,
                    destinationPath: operation.DestinationPath,
                    copyDirection: operation.CopyDirection,
                    commonRootAlias: operation.CommonRootAlias,
                    cancellationHandleWrapper: operation.CancellationHandle,
                    searchOptions: operation.SearchOptions,
                    statusCallbacks: callbacks);

                operation.CopyActive = false;
                SetCopyCancelEnabled(operation, false);
                _progressTimer.Stop();
                // Callbacks only enqueue owned data. Drain before setting the authoritative result.
                AppendOutput($"\nCopy returned HRESULT 0x{hr.Value:X8}.\n");
                ProgressStatusText.Text = "Copy returned; displaying remaining output.";
                await DrainRemainingCopyUpdatesAsync(operation);
                if (hr.Failed)
                {
                    ProgressStatusText.Text = "Deployment failed during copy.";
                    AppendOutput(string.Format(Messages.DeployCopyFailedFormat, hr.Value));
                    if (operation.CancellationRequested)
                        AppendOutput(Messages.CopyFailureCancellationNote);
                    AppendOutput(Messages.RemotePathValidationSkippedMessage);
                    return;
                }
                if (operation.CancellationRequested)
                {
                    ProgressStatusText.Text = "Cancellation requested; deployment completeness not confirmed.";
                    AppendOutput(Messages.DeployCancellationUnconfirmedMessage);
                    return;
                }
                if (operation.CallbackFailure != null)
                {
                    ProgressStatusText.Text = "Copy returned success, but callback processing failed.";
                    AppendOutput(Messages.DeployCallbackFailureMessage);
                    return;
                }

                ProgressStatusText.Text = "Copy finished. Validating remote path.";
                AppendOutput(Messages.RemotePathValidationStartingMessage);
                hr = await RemoteIteration.RegisterRemoteXboxGameAsync(operation.Device, operation.DestinationPath);
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
                SetCopyCancelEnabled(operation, false);
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
            SetCopyCancelEnabled(operation, false);
            SetCopyStatus(operation, Messages.CopyCancellationRequestedStatus);
            AppendOutput(Messages.CopyCancellationRequestedOutput);
            try
            {
                // The signal is nonblocking. Running it on the UI thread prevents the awaited
                // copy continuation from disposing its handle while cancellation uses it.
                HRESULT hr = RemoteIteration.RequestCopyCancellation(operation.CancellationHandle);
                AppendOutput(string.Format(Messages.CopyCancellationSignalReturnedFormat, hr.Value));
                if (hr.Failed)
                {
                    SetCopyStatus(operation, Messages.CopyCancellationSignalFailedStatus);
                    AppendOutput(Messages.CopyCancellationSignalFailedOutput);
                }
            }
            catch (Exception ex)
            {
                AppendOutput(string.Format(
                    Messages.CopyCancellationSignalExceptionFormat,
                    ex.Message,
                    ex.HResult));
                SetCopyStatus(operation, Messages.CopyCancellationSignalFailedStatus);
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
                    : string.Format(Messages.LaunchSucceededFormat, process.ProcessId, process.ThreadId));
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
            string cancellationNote = operation.CancellationRequested
                ? Messages.OperationFailureCancellationNote
                : "";
            string deleteNote = operation.StopWaitingRequested
                ? Messages.OperationFailureDeleteMayContinueNote
                : "";
            AppendOutput(string.Format(
                Messages.OperationFailureFormat,
                operation.Name,
                ex.Message,
                ex.HResult,
                cancellationNote,
                deleteNote));
        }

        private void AppendOutput(string text)
        {
            OutputTextBox.AppendText(text);
            OutputTextBox.ScrollToEnd();
        }
    }
}
