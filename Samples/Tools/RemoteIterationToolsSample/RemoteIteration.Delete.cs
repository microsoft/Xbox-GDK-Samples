using Microsoft.Gaming.WdRemoteApi;
using System.Text;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    public static partial class RemoteIteration
    {
        // Best-effort deletion has no per-item results; the API ignores status callbacks.
        public static Task<HRESULT> DeleteAsync(
            string remoteDevice,
            string remoteFolderPath,
            DeleteOptions? deleteOptions = null,
            DeleteSearchOptions? searchOptions = null,
            WdCancellationHandleWrapper? cancellationHandleWrapper = null)
        {
            return Task.Run(() =>
            {
                unsafe
                {
                    fixed (byte* aliasBytes = string.IsNullOrEmpty(deleteOptions?.CommonRootAlias) ? null : Encoding.UTF8.GetBytes(deleteOptions.CommonRootAlias + '\0'))
                    fixed (byte* includeBytes = string.IsNullOrEmpty(searchOptions?.IncludeFilePattern) ? null : Encoding.UTF8.GetBytes(searchOptions.IncludeFilePattern + '\0'))
                    fixed (byte* excludeBytes = string.IsNullOrEmpty(searchOptions?.ExcludeFilePattern) ? null : Encoding.UTF8.GetBytes(searchOptions.ExcludeFilePattern + '\0'))
                    fixed (byte* directoryBytes = string.IsNullOrEmpty(searchOptions?.ExcludeDirPattern) ? null : Encoding.UTF8.GetBytes(searchOptions.ExcludeDirPattern + '\0'))
                    {
                        var nativeOptions = new WdDeleteOptions
                        {
                            commonRootAlias = new PCSTR(aliasBytes),
                            deleteRootFolder = deleteOptions?.DeleteRootFolder == true ? (byte)1 : (byte)0
                        };
                        // Delete uses uint64 masks and different directory field names from copy.
                        var nativeSearchOptions = new WdDeleteSearchOptions
                        {
                            includeFilePattern = new PCSTR(includeBytes),
                            excludeFilePattern = new PCSTR(excludeBytes),
                            excludeDirPattern = new PCSTR(directoryBytes),
                            includeFileAttributes = searchOptions?.IncludeFileAttributes ?? 0,
                            excludeFileAttributes = searchOptions?.ExcludeFileAttributes ?? 0,
                            includeDirAttributes = searchOptions?.IncludeDirAttributes ?? 0,
                            excludeDirAttributes = searchOptions?.ExcludeDirAttributes ?? 0
                        };
                        // CsWin32's .NET 8 UTF-8 string overload does not append NUL.
                        return PInvoke.WdDeleteRemoteFiles(
                            remoteDevice + '\0',
                            remoteFolderPath + '\0',
                            nativeOptions,
                            nativeSearchOptions,
                            statusCallbacks: null,
                            cancellationHandle: cancellationHandleWrapper?.Handle);
                    }
                }
            });
        }

        // Stops the client wait only; deletion already started on the endpoint continues.
        public static HRESULT RequestDeleteStopWaiting(WdCancellationHandleWrapper cancellationHandleWrapper)
        {
            if (cancellationHandleWrapper.Handle.IsInvalid)
                throw new ArgumentException("Invalid cancellation handle.");

            return PInvoke.WdCancelRemoteDelete(cancellationHandleWrapper.Handle);
        }
    }
}
