using Microsoft.Gaming.WdRemoteApi;
using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    internal sealed class OperationState : IDisposable
    {
        private GCHandle _callbackContext;
        private Exception? _callbackFailure;
        private CopyProgressUpdate? _latestProgressUpdate;
        private CopyProgressUpdate? _pendingProgressUpdate;
        private int _nextFileMessage;

        public OperationState(string name, string device, string remotePath = "")
        {
            Name = name;
            Device = device;
            RemotePath = remotePath;
        }

        public string Name { get; }
        public string Device { get; }
        public string RemotePath { get; }
        public string SourcePath { get; init; } = "";
        public string DestinationPath { get; init; } = "";
        public WdCopyDirection CopyDirection { get; init; } = WdCopyDirection.CopyTo;
        public string? CommonRootAlias { get; init; }
        public CopySearchOptions? SearchOptions { get; init; }
        public DeleteOptions? DeleteOptions { get; init; }
        public DeleteSearchOptions? DeleteSearchOptions { get; init; }
        public WdLaunchOptions? LaunchOptions { get; init; }
        public WdCancellationHandleWrapper? CancellationHandle { get; private set; }
        public bool CopyActive { get; set; }
        public bool CancellationRequested { get; set; }
        public bool DeleteActive { get; set; }
        public bool StopWaitingRequested { get; set; }
        public bool CallbackFailureReported { get; set; }
        public Exception? CallbackFailure => Volatile.Read(ref _callbackFailure);
        public ConcurrentQueue<CopyProgressUpdate> ProgressUpdates { get; } = new();
        public ConcurrentQueue<string> Diagnostics { get; } = new();
        public WdCopyOperationSummary? LatestSummary => Volatile.Read(ref _latestProgressUpdate)?.Summary;
        public bool HasPendingOutput => _pendingProgressUpdate != null || !ProgressUpdates.IsEmpty || !Diagnostics.IsEmpty;

        // Only the UI thread consumes batches. The budget limits dispatch work, not retained output.
        public string TakeOutputBatch(int maximumMessages)
        {
            if (maximumMessages < 1)
                throw new ArgumentOutOfRangeException(nameof(maximumMessages));

            var output = new StringBuilder();
            while (maximumMessages > 0)
            {
                if (Diagnostics.TryDequeue(out string? diagnostic))
                {
                    output.Append(diagnostic);
                    maximumMessages--;
                    continue;
                }

                if (_pendingProgressUpdate == null && !ProgressUpdates.TryDequeue(out _pendingProgressUpdate))
                    break;

                List<string> messages = _pendingProgressUpdate.FileMessages;
                while (maximumMessages > 0 && _nextFileMessage < messages.Count)
                {
                    output.Append(messages[_nextFileMessage++]);
                    maximumMessages--;
                }

                if (_nextFileMessage == messages.Count)
                {
                    // Summary-only notifications must also consume budget to bound each batch.
                    if (messages.Count == 0)
                        maximumMessages--;
                    _pendingProgressUpdate = null;
                    _nextFileMessage = 0;
                }
            }
            return output.ToString();
        }

        public void CreateCancellationHandle()
        {
            if (CancellationHandle != null)
                throw new InvalidOperationException("This operation already owns a cancellation handle.");

            CancellationHandle = new WdCancellationHandleWrapper();
        }

        public unsafe WdCopyStatusCallbacks CreateCopyCallbacks()
        {
            CreateCancellationHandle();
            _callbackContext = GCHandle.Alloc(this);
            return new WdCopyStatusCallbacks
            {
                copyFilesStatusCallback = &FileProgressUpdate,
                copyErrorCallback = &CopyDiagnostic,
                refreshRateMs = 500, // Desired update interval, not a guaranteed cadence.
                context = (void*)GCHandle.ToIntPtr(_callbackContext)
            };
        }

        public void RecordCallbackFailure(Exception exception)
        {
            // Preserve the first failure without allocating or invoking UI code on a native thread.
            Interlocked.CompareExchange(ref _callbackFailure, exception, null);
        }

        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        private static unsafe HRESULT FileProgressUpdate(
            nuint fileProgressCount,
            WdCopyFileProgressInfo* fileUpdates,
            WdCopyOperationSummary* summary,
            void* context)
        {
            // This context is owned until the blocking copy call and all its callbacks return.
            OperationState? operation = null;
            try
            {
                operation = (OperationState)GCHandle.FromIntPtr((IntPtr)context).Target!;
                if (summary == null || (fileProgressCount > 0 && fileUpdates == null))
                {
                    throw new InvalidOperationException("Copy progress callback supplied missing progress data.");
                }

                var lines = new List<string>();
                string copyAction = operation.CopyDirection == WdCopyDirection.CopyFrom ? "Retrieve" : "Deploy";
                for (nuint i = 0; i < fileProgressCount; i++)
                {
                    WdCopyFileProgressInfo info = fileUpdates[i];
                    string? filePath = Marshal.PtrToStringUTF8((IntPtr)info.relativeFilePath.Value);
                    if (info.bytesTransferred == info.fileSize)
                    {
                        lines.Add($"[Completed] {filePath}\n");
                    }
                    else
                    {
                        lines.Add($"[{copyAction}] {filePath} ({info.bytesTransferred:N0} / {info.fileSize:N0} bytes)\n");
                    }
                }

                // Copy the strings and scalar counters; no native pointer survives the callback.
                // A summary without per-file updates is still a valid progress notification.
                var update = new CopyProgressUpdate(lines, *summary);
                operation.ProgressUpdates.Enqueue(update);
                Volatile.Write(ref operation._latestProgressUpdate, update);
                return new HRESULT(0);
            }
            catch (Exception ex)
            {
                operation?.RecordCallbackFailure(ex);
                // A failed callback HRESULT does not stop the copy. The UI also reports this failure.
                return new HRESULT(ex.HResult);
            }
        }

        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvStdcall) })]
        private static unsafe HRESULT CopyDiagnostic(
            WdCopyErrorSeverity severity, PCSTR message, HRESULT error, void* context)
        {
            OperationState? operation = null;
            try
            {
                operation = (OperationState)GCHandle.FromIntPtr((IntPtr)context).Target!;
                operation.Diagnostics.Enqueue($"[{severity}] {Marshal.PtrToStringUTF8((IntPtr)message.Value)} (HRESULT 0x{error.Value:X8})\n");
                return new HRESULT(0);
            }
            catch (Exception ex)
            {
                operation?.RecordCallbackFailure(ex);
                return new HRESULT(ex.HResult);
            }
        }

        public void Dispose()
        {
            // Called on the UI thread only after awaiting native work. Cancellation is signalled
            // synchronously on that same thread, so it cannot race this resource cleanup.
            if (_callbackContext.IsAllocated)
            {
                _callbackContext.Free();
            }
            CancellationHandle?.Dispose();
        }
    }

    internal sealed record CopyProgressUpdate(List<string> FileMessages, WdCopyOperationSummary Summary);
}
