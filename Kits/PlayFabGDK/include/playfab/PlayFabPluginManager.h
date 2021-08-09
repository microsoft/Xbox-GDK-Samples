#pragma once

#include <playfab/PlayFabCallRequestContainer.h>
#include <playfab/PlayFabError.h>
#include <mutex>
#include <unordered_map>

namespace PlayFab
{
    /// <summary>
    /// The enumeration of supported plugin contracts.
    /// </summary>
    enum class PlayFabPluginContract
    {
        PlayFab_Serializer,
        PlayFab_Transport
    };

    /// <summary>
    /// Interface of any PlayFab SDK plugin.
    /// </summary>
    class IPlayFabPlugin
    {
    };

    /// <summary>
    /// Interface of any transport SDK plugin.
    /// All functions execute asynchronously
    /// </summary>
    class IPlayFabHttpPlugin : public IPlayFabPlugin
    {
    public:
        /// <summary>
        /// starts the process of making a post request.
        /// A user is expected to supply their own CallRequestContainerBase
        /// </summary>
        virtual void MakePostRequest(std::unique_ptr<CallRequestContainerBase> requestContainer) = 0;

        /// <summary>
        /// updates the process of making post requests.
        /// This method can be used when plugin is not using a working thread and instead should execute
        /// its long-running operations via polling using this method.
        /// Returns number of currently pending post requests.
        /// </summary>
        virtual size_t Update() = 0;
    };

    /// <summary>
    /// Interface of any data serializer SDK plugin.
    /// </summary>
    class IPlayFabSerializerPlugin : public IPlayFabPlugin
    {
    };

    /// <summary>
    /// The PlayFab plugin manager.
    /// It can be used either as an instance or a singleton.
    /// </summary>
    class PlayFabPluginManager
    {
    public:
        static PlayFabPluginManager& GetInstance(); // The singleton instance of plugin manager (created on demand)
        PlayFabPluginManager();

        // Prevent copy/move construction
        PlayFabPluginManager(const PlayFabPluginManager&) = delete;
        PlayFabPluginManager(PlayFabPluginManager&&) = delete;

        // Prevent copy/move assignment operations
        PlayFabPluginManager& operator=(const PlayFabPluginManager&) = delete;
        PlayFabPluginManager& operator=(PlayFabPluginManager&&) = delete;

        static const std::string defaultInstanceName;

        // Gets a plugin.
        // If a plugin with specified contract and optional instance name does not exist, it will create a new one.
        template <typename T>
        static std::shared_ptr<T> GetPlugin(const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName)
        {
            return std::static_pointer_cast<T>(GetInstance().GetPluginInternal(contract, instanceName));
        }

        // Sets a custom plugin.
        // If a plugin with specified contract and optional instance name already exists, it will be replaced with specified instance.
        static void SetPlugin(const std::shared_ptr<IPlayFabPlugin>& plugin, const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName);

        // Gets a plugin.
        // If a plugin with specified contract and optional instance name does not exist, it will create a new one.
        template <typename T>
        std::shared_ptr<T> GetPluginInstance(const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName)
        {
            return std::static_pointer_cast<T>(GetPluginInternal(contract, instanceName));
        }

        // Sets a custom plugin.
        // If a plugin with specified contract and optional instance name already exists, it will be replaced with specified instance.
        void SetPluginInstance(const std::shared_ptr<IPlayFabPlugin>& plugin, const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName);

        // Sets a custom exception handler for any possible background thread exceptions
        void SetExceptionHandler(ExceptionCallback exceptionHandler);

        // Called when an exception occured on a worker thread
        void HandleException(const std::exception);

    private:
    struct PluginEntry
        {
            const PlayFabPluginContract contract;
            const std::string name;
            std::shared_ptr<IPlayFabPlugin> plugin;
        };

        std::shared_ptr<IPlayFabPlugin> GetPluginInternal(const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName);
        void SetPluginInternal(const std::shared_ptr<IPlayFabPlugin>& plugin, const PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName);
        PluginEntry& FindOrCreatePluginEntry(PlayFabPluginContract contract, const std::string& instanceName = defaultInstanceName);

        std::shared_ptr<IPlayFabPlugin> CreatePlayFabSerializerPlugin();
        std::shared_ptr<IPlayFabPlugin> CreatePlayFabTransportPlugin();

    private:
        std::mutex pluginsMutex;
        std::vector<PluginEntry> plugins;
        std::mutex userExceptionCallbackMutex;
        ExceptionCallback userExceptionCallback;
    };
}
