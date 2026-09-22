using Microsoft.Gaming.WdRemoteApi;
using System;
using System.Text;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    public static class RemoteIteration
    {
        /* The WdRemoteCopy API automates copying game builds from local environments to remote devices, efficiently
         * transferring assets and binaries. It reduces manual errors by managing file movement and provides blocking
         * operations that return an HRESULT for clear status and error tracking. 
         * Files are copied to C:\ProgramData\Microsoft GDK\gameroot by default. 
         * WdCopyStatusCallbacks provides multiple callbacks for copy progress information.
         */
        public static Task<HRESULT> CopyAsync(
            string remoteDevice,
            string localSourcePath,
            string remoteDestPath,
            WdCancellationHandleWrapper? cancellationHandleWrapper = null,
            WdCopyOptions? copyOptions = null,
            CopySearchOptions? searchOptions = null,
            WdCopyStatusCallbacks? statusCallbacks = null
            )
        {
            return Task.Run(() =>
            {
                HRESULT hr;
                unsafe
                {
                    fixed (byte* includeFilePatternBytes = string.IsNullOrEmpty(searchOptions?.IncludeFilePattern) ? null : Encoding.UTF8.GetBytes(searchOptions.IncludeFilePattern + '\0'))
                    fixed (byte* excludeFilePatternBytes = string.IsNullOrEmpty(searchOptions?.ExcludeFilePattern) ? null : Encoding.UTF8.GetBytes(searchOptions.ExcludeFilePattern + '\0'))
                    fixed (byte* excludeDirPatternBytes = string.IsNullOrEmpty(searchOptions?.ExcludeDirPattern) ? null : Encoding.UTF8.GetBytes(searchOptions.ExcludeDirPattern + '\0'))
                    {
                        WdCopySearchOptions nativeSearchOptions = default;
                        nativeSearchOptions.includeFilePattern = new PCSTR(includeFilePatternBytes);
                        nativeSearchOptions.excludeFilePattern = new PCSTR(excludeFilePatternBytes);
                        nativeSearchOptions.excludeDirPattern = new PCSTR(excludeDirPatternBytes);
                        nativeSearchOptions.includeFileAttributes = searchOptions?.IncludeFileAttributes ?? 0;
                        nativeSearchOptions.excludeFileAttributes = searchOptions?.ExcludeFileAttributes ?? 0;
                        nativeSearchOptions.includeDirectoryAttributes = searchOptions?.IncludeDirectoryAttributes ?? 0;
                        nativeSearchOptions.excludeDirectoryAttributes = searchOptions?.ExcludeDirectoryAttributes ?? 0;

                        // CsWin32's string overload uses UTF-8 on .NET 8, but does not append NUL.
                        hr = PInvoke.WdRemoteCopy(
                            remoteDevice + '\0',
                            localSourcePath + '\0',
                            remoteDestPath + '\0',
                            copyOptions,
                            nativeSearchOptions,
                            statusCallbacks,
                            cancellationHandleWrapper?.Handle);
                    }
                }
 
                return hr;
            });
        }

        /* WdRegisterRemoteXboxGame resolves and validates a remote folder path.
         * It does not perform platform game registration. */
        public static Task<HRESULT> RegisterRemoteXboxGameAsync(
            string remoteDevice,
            string remoteFolderPath,
            string? commonRootAlias = null)
        {
            return Task.Run(() =>
            {
                unsafe
                {
                    fixed (byte* deviceBytes = Encoding.UTF8.GetBytes(remoteDevice + '\0'))
                    fixed (byte* folderBytes = Encoding.UTF8.GetBytes(remoteFolderPath + '\0'))
                    fixed (byte* aliasBytes = string.IsNullOrEmpty(commonRootAlias) ? null : Encoding.UTF8.GetBytes(commonRootAlias + '\0'))
                    {
                        return PInvoke.WdRegisterRemoteXboxGame(
                            new PCSTR(deviceBytes), new PCSTR(folderBytes), new PCSTR(aliasBytes));
                    }
                }
            });
        }

        /* The WdTerminateRemoteGame API terminates the last game launch using WdLaunchRemoteGame. 
         * This operation is useful for ending applications that are unresponsive, misbehaving, or need to be shut down
         * as part of an automated workflow. By providing a reliable way to terminate games, the API helps maintain
         * system stability and enables precise control over remote execution environments. */
        public static Task<HRESULT> TerminateRemoteGameAsync(string remoteDevice)
        {
            return Task.Run(() =>
            {
                HRESULT hr = PInvoke.WdTerminateRemoteGame(remoteDevice + '\0');

                return hr;
            });
        }

        /* The launch operation initiates an executable on a remote device, allowing you to specify command-line
         * arguments, launch configurations, and output redirection as needed. Upon successful launch, the API provides
         * the process ID and thread ID of the newly created process. This information enables developers to monitor,
         * manage, or interact with the launched application programmatically, supporting workflows that require
         * precise control over remote execution. */
        public static Task<(HRESULT hr, ProcThreadId procThreadId)> LaunchRemoteGameAsync(
            string remoteDevice,
            string remotePath,
            string? args = null,
            WdLaunchOptions? launchOptions = null
            )
        {
            return Task.Run(() =>
            {
                unsafe
                {
                    fixed (byte* deviceBytes = Encoding.UTF8.GetBytes(remoteDevice + '\0'))
                    fixed (byte* pathBytes = Encoding.UTF8.GetBytes(remotePath + '\0'))
                    fixed (byte* argsBytes = string.IsNullOrEmpty(args) ? null : Encoding.UTF8.GetBytes(args + '\0'))
                    {
                        WdLaunchOptions nativeOptions = launchOptions.GetValueOrDefault();
                        uint processId = 0;
                        uint threadId = 0;
                        HRESULT hr = PInvoke.WdLaunchRemoteGame(
                            new PCSTR(deviceBytes),
                            new PCSTR(pathBytes),
                            new PCSTR(argsBytes),
                            launchOptions.HasValue ? &nativeOptions : null,
                            &processId,
                            &threadId);

                        return (hr, new ProcThreadId(processId, threadId));
                    }
                }
            });
        }

        /* Resume the last game launch suspended with WdLaunchRemoteGame. */
        public static Task<HRESULT> ResumeGameAsync(
            string remoteDevice
            )
        {
            return Task.Run(() =>
            {
                HRESULT hr = PInvoke.WdResumeRemoteGame(remoteDevice + '\0');
                return hr;
            });
        }


        /* Cancels an ongoing WdRemoteCopy operation. This is a non-blocking call that signals the
         * copy operation to stop as soon as possible. */
        public static HRESULT RequestCopyCancellation(WdCancellationHandleWrapper cancellationHandleWrapper)
        {
            if (cancellationHandleWrapper.Handle.IsInvalid)
            {
                throw new ArgumentException("Invalid cancellation handle.");
            }

            return PInvoke.WdCancelRemoteCopy(cancellationHandleWrapper.Handle);
        }
    }

    public record struct ProcThreadId(uint ProcessId, uint ThreadId);
}
