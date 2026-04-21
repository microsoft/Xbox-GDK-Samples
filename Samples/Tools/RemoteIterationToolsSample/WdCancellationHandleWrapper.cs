using System;
using Microsoft.Gaming.WdRemoteApi;
using Windows.Win32.Foundation;

namespace RemoteIterationToolsSample
{
    /// <summary>
    /// Safe wrapper class for a WdCancellationHandle, managing its lifecycle via IDisposable.
    /// </summary>
    public class WdCancellationHandleWrapper : IDisposable
    {
        private readonly WdCloseCancellationHandleSafeHandle? _handle;
        private bool _disposed;

        public WdCancellationHandleWrapper()
        {
            HRESULT hr = PInvoke.WdCreateCancellationHandle(out _handle);
            if (hr.Failed)
            {
                throw new RemoteIterationException("Failed to create copy cancellation handle", hr);
            }
        }

        public WdCloseCancellationHandleSafeHandle Handle
        {
            get
            {
                ObjectDisposedException.ThrowIf(_disposed, this);
                if (_handle is null)
                {
                    throw new InvalidOperationException("Handle is not initialized.");
                }
                return _handle;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    _handle?.Dispose();
                }

                _disposed = true;
            }
        }
    }
}
