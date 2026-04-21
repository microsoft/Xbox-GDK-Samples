namespace RemoteIterationToolsSample
{
    /// <summary>
    /// Maps known HRESULTs from the remote iteration APIs to user-friendly messages.
    /// </summary>
    internal static class HResultHelper
    {
        private const int E_OUTSIDEGAMEROOT = unchecked((int)0x8C114001);
        private const int E_PROCESSNOTFOUND = unchecked((int)0x8C114002);
        private const int E_INVALIDPROCESSID = unchecked((int)0x8C114003);
        private const int E_INVALIDPIN = unchecked((int)0x8C114004);
        private const int E_INVALIDSSHKEY = unchecked((int)0x8C114005);
        private const int E_PAIRINGTIMEOUT = unchecked((int)0x8C114006);
        private const int E_TOOMANYFAILURES = unchecked((int)0x8C114007);
        private const int E_CLIENTNOTAUTHORIZED = unchecked((int)0x8C114008);
        private const int E_SERVERNOTAUTHORIZED = unchecked((int)0x8C114009);
        private const int E_SSHKEYTOOLARGE = unchecked((int)0x8C11400A);
        private const int E_GAMENOTSUSPENDED = unchecked((int)0x8C11400B);
        private const int E_GAMENOTRUNNING = unchecked((int)0x8C11400C);
        private const int E_GAMEOVERSUSPENDED = unchecked((int)0x8C11400D);
        private const int E_GAMESTILLRUNNING = unchecked((int)0x8C11400E);
        private const int E_GAMEFILEPATHNOTEXIST = unchecked((int)0x8C114010);
        private const int E_SERVERTOOOLD = unchecked((int)0x8C114011);
        private const int E_NAMERESOLUTIONFAILED = unchecked((int)0x8C114012);
        private const int E_CONNECTIONERROR = unchecked((int)0x8C114014);
        private const int E_ENDPOINTUPGRADEREQUIRED = unchecked((int)0x8C11401A);
        private const int E_ENDPOINTUPGRADERECOMMENDED = unchecked((int)0x8C11401B);
        private const int E_CLIENTUPGRADERECOMMENDED = unchecked((int)0x8C11401C);

        private const int WINHTTP_CANNOT_CONNECT = unchecked((int)0x80072EFD);
        private const int WINHTTP_NAME_NOT_RESOLVED = unchecked((int)0x80072EE7);
        private const int WINHTTP_TIMEOUT = unchecked((int)0x80072EE2);
        private const int WINHTTP_CONNECTION_ERROR = unchecked((int)0x80072EFE);

        /// <summary>
        /// Returns a user-friendly message for a known HRESULT, or null if unrecognized.
        /// </summary>
        public static string? GetFriendlyMessage(int hr)
        {
            return hr switch
            {
                E_CONNECTIONERROR => "A connection error occurred while communicating with the remote device.",
                E_NAMERESOLUTIONFAILED => "Could not resolve the remote device name or address.",
                E_CLIENTNOTAUTHORIZED => "This PC is not authorized to connect to the remote device. Try re-pairing.",
                E_SERVERNOTAUTHORIZED => "The remote device could not be verified. Try re-pairing.",
                E_ENDPOINTUPGRADEREQUIRED => "The remote device's wdendpoint.exe must be upgraded before this operation can proceed.",
                E_ENDPOINTUPGRADERECOMMENDED => "The remote device's wdendpoint.exe should be upgraded to a newer version.",
                E_CLIENTUPGRADERECOMMENDED => "A newer version of wdremote.exe is available and recommended.",
                E_INVALIDPIN => "The device rejected the provided PIN, authorization denied.",
                E_INVALIDSSHKEY => "The device rejected the SSH key.",
                E_PAIRINGTIMEOUT => "Pairing timed out. Please try again.",
                E_TOOMANYFAILURES => "Too many failed pairing attempts. Please restart wdendpoint.exe on the remote device and try again.",
                E_SSHKEYTOOLARGE => "The provided SSH key file is too large.",
                E_SERVERTOOOLD => "wdendpoint.exe is too old and no longer compatible. Please update wdendpoint.exe on the remote device.",
                E_OUTSIDEGAMEROOT => "Path lies outside of the common root.",
                E_GAMEFILEPATHNOTEXIST => "The game file path does not exist on the remote device.",
                E_GAMESTILLRUNNING => "A game is already running on the remote device.",
                E_GAMENOTRUNNING => "No game is running on remote device.",
                E_GAMENOTSUSPENDED => "The game is not suspended.",
                E_GAMEOVERSUSPENDED => "The game was not resumed because there were more suspend counts than expected.",
                WINHTTP_CANNOT_CONNECT => "Could not connect to the remote device. Verify the remote device is reachable.",
                WINHTTP_NAME_NOT_RESOLVED => "Could not resolve the remote device name. Verify the device name is correct.",
                WINHTTP_TIMEOUT => "The connection to the remote device timed out.",
                WINHTTP_CONNECTION_ERROR => "The connection to the remote device was interrupted.",
                _ => null,
            };
        }
    }
}
