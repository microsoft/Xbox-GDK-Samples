#pragma once

#include <playfab/PlayFabError.h>

namespace PlayFab
{
    class PlayFabApiSettings;
    class PlayFabAuthenticationContext;

    /// <summary>
    /// All settings and global variables for PlayFab
    /// </summary>
    class PlayFabSettings
    {
    public:
        static const std::string sdkVersion;
        static const std::string buildIdentifier;
        static const std::string versionString;

        // Control whether all callbacks are threaded or whether the user manually controls callback timing from their main-thread
        static bool threadedCallbacks;
        // Used to override the PlayFab endpoint url - Not typical
        static std::string productionEnvironmentURL;
        // Used to receive a callback for every failed PlayFab API call - Parallel to the individual error callbacks
        static ErrorCallback globalErrorHandler;

        // The pointers to these objects should be const as they should always be fixed, but the contents are still mutable
        static const std::shared_ptr<PlayFabApiSettings> staticSettings;
        static const std::shared_ptr<PlayFabAuthenticationContext> staticPlayer;

        static void ForgetAllCredentials();
    private:
        PlayFabSettings(); // Private constructor, static class should never have an instance
        PlayFabSettings(const PlayFabSettings& other); // Private copy-constructor, static class should never have an instance
    };
}
