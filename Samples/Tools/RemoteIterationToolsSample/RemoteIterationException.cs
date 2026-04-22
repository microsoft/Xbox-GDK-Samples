namespace RemoteIterationToolsSample
{
    internal class RemoteIterationException : Exception
    {
        public RemoteIterationException()
        {
        }

        public RemoteIterationException(string? message) : base(message)
        {
        }

        public RemoteIterationException(string? message, int hr)
            : base(FormatMessage(message ?? string.Empty, hr))
        {
            HResult = hr;
        }

        public RemoteIterationException(string? message, Exception? innerException) : base(message, innerException)
        {
        }

        private static string FormatMessage(string message, int hr)
        {
            string? friendly = HResultHelper.GetFriendlyMessage(hr);
            return friendly != null
                ? $"{message}: {friendly}"
                : message;
        }
    }
}