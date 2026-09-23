using System.Windows;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    public partial class MainWindow
    {
        private async void Delete_Click(object sender, RoutedEventArgs e)
        {
            if (_activeOperation != null)
                return;

            string device = DeviceIpTextBox.Text.Trim();
            string path = DeleteRemotePathTextBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(device) || string.IsNullOrWhiteSpace(path))
            {
                System.Windows.MessageBox.Show("Enter a device and a remote path.", "Delete inputs required");
                return;
            }

            var options = new DeleteOptions
            {
                CommonRootAlias = DeleteCommonRootAliasTextBox.Text.Trim(),
                DeleteRootFolder = DeleteRootFolderCheckBox.IsChecked == true
            };
            var filters = new DeleteSearchOptions
            {
                IncludeFilePattern = DeleteIncludeFilePatternTextBox.Text.Trim(),
                ExcludeFilePattern = DeleteExcludeFilePatternTextBox.Text.Trim(),
                ExcludeDirPattern = DeleteExcludeDirPatternTextBox.Text.Trim(),
                IncludeFileAttributes = DeleteIncludeFileAttributesSelector.Mask,
                ExcludeFileAttributes = DeleteExcludeFileAttributesSelector.Mask,
                IncludeDirAttributes = DeleteIncludeDirAttributesSelector.Mask,
                ExcludeDirAttributes = DeleteExcludeDirAttributesSelector.Mask
            };
            string confirmation = DescribeDeleteRequest(device, path, options, filters);
            if (System.Windows.MessageBox.Show(this, confirmation + "\n\nDelete matching content?",
                "Confirm remote deletion", MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No) != MessageBoxResult.Yes)
                return;

            var operation = new OperationState("Delete", device, path)
            {
                DeleteOptions = options,
                DeleteSearchOptions = filters
            };
            if (!BeginOperation(operation))
                return;

            try
            {
                AppendOutput("> Remote delete request\n" + confirmation + "\n\n");
                operation.CreateCancellationHandle();
                operation.DeleteActive = true;
                StopWaitingDeleteButton.IsEnabled = true;
                DeleteProgressBar.IsIndeterminate = true;
                DeleteStatusText.Text = "Deleting...";
                HRESULT hr = await RemoteIteration.DeleteAsync(
                    operation.Device, operation.RemotePath, operation.DeleteOptions, operation.DeleteSearchOptions, operation.CancellationHandle);

                operation.DeleteActive = false;
                StopWaitingDeleteButton.IsEnabled = false;
                DeleteStatusText.Text = DescribeDeleteResult(hr, operation.StopWaitingRequested);
                AppendOutput($"\nDelete returned HRESULT 0x{hr.Value:X8}. {DeleteStatusText.Text}\n");
            }
            catch (Exception ex)
            {
                ShowOperationFailure(operation, ex);
                DeleteStatusText.Text = DescribeDeleteResult(new HRESULT(ex.HResult), operation.StopWaitingRequested);
            }
            finally
            {
                operation.DeleteActive = false;
                DeleteProgressBar.IsIndeterminate = false;
                EndOperation(operation);
            }
        }

        private static string DescribeDeleteRequest(string device, string path, DeleteOptions options, DeleteSearchOptions filters)
        {
            var activeFilters = new List<string>();
            if (!string.IsNullOrEmpty(filters.IncludeFilePattern))
                activeFilters.Add($"Include files: {filters.IncludeFilePattern}");
            if (!string.IsNullOrEmpty(filters.ExcludeFilePattern))
                activeFilters.Add($"Exclude files: {filters.ExcludeFilePattern}");
            if (!string.IsNullOrEmpty(filters.ExcludeDirPattern))
                activeFilters.Add($"Exclude directories: {filters.ExcludeDirPattern}");
            if (filters.IncludeFileAttributes != 0)
                activeFilters.Add($"Include file attributes: {filters.IncludeFileAttributes}");
            if (filters.ExcludeFileAttributes != 0)
                activeFilters.Add($"Exclude file attributes: {filters.ExcludeFileAttributes}");
            if (filters.IncludeDirAttributes != 0)
                activeFilters.Add($"Include directory attributes: {filters.IncludeDirAttributes}");
            if (filters.ExcludeDirAttributes != 0)
                activeFilters.Add($"Exclude directory attributes: {filters.ExcludeDirAttributes}");

            return $"Device: {device}\nPath: {path}\n" +
                $"Common Root alias: {(string.IsNullOrEmpty(options.CommonRootAlias) ? "(default)" : options.CommonRootAlias)}\n" +
                $"Remove root folder: {(options.DeleteRootFolder ? "If empty" : "No")}\n" +
                (activeFilters.Count == 0 ? "Filters: none" : "Folder filters:\n" + string.Join("\n", activeFilters));
        }

        private static string DescribeDeleteResult(HRESULT hr, bool stopWaitingRequested)
        {
            if (hr.Failed)
                return "Delete request failed." + (stopWaitingRequested ? " Remote deletion may continue." : "");

            // Normal completion and an observed stop-wait signal can both return S_OK.
            return stopWaitingRequested
                ? "Stopped waiting. Remote deletion may continue."
                : "Delete request completed; some items may remain.";
        }

        private void StopWaitingDelete_Click(object sender, RoutedEventArgs e)
        {
            OperationState? operation = _activeOperation;
            if (operation == null || !operation.DeleteActive || operation.StopWaitingRequested || operation.CancellationHandle == null)
                return;

            operation.StopWaitingRequested = true;
            StopWaitingDeleteButton.IsEnabled = false;
            DeleteStatusText.Text = "Stopping wait...";
            try
            {
                // The nonblocking UI-thread signal cannot race the awaited call's handle disposal.
                HRESULT hr = RemoteIteration.RequestDeleteStopWaiting(operation.CancellationHandle);
                AppendOutput($"Stop-wait signal returned HRESULT 0x{hr.Value:X8}.\n");
                if (hr.Failed)
                {
                    DeleteStatusText.Text = "Stop-wait signal failed; the client may still be waiting.";
                    AppendOutput(DeleteStatusText.Text + "\n");
                }
            }
            catch (Exception ex)
            {
                AppendOutput($"Stop-wait signal failed: {ex.Message} (HRESULT 0x{ex.HResult:X8}).\n");
                DeleteStatusText.Text = "Stop-wait signal failed; the client may still be waiting.";
            }
        }
    }
}
