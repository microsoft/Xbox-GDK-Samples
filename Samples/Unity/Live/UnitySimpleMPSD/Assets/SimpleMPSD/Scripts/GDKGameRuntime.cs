using System;
using System.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.XGamingRuntime;
using UnityEngine;

namespace GdkSample_SimpleMPSD
{
    public sealed class GDKGameRuntime : MonoBehaviour
    {
        public static event Action GameRuntimeInitialized;
        public static string GameConfigSandbox { get; private set; } = "XDKS.1";

        // Documented as: "Specifies the SCID to be used for Save Game Storage."
        public static string GameConfigScid { get; private set; } = "00000000-0000-0000-0000-0000FFFFFFFF";

        // Documented as: "...a default value of 'FFFFFFFF' for this element. This allows for early iteration of your
        //   title without having to immediately acquire the Id from Partner Center. It is strongly recommended to change
        //   this Id as soon as you get your title building to avoid failures when attempting to do API calls."
        public static string GameConfigTitleId { get; private set; } = "FFFFFFFF";

        public static bool Initialized { get; private set; }
        public static GDKGameRuntime Instance { get; private set; }

        private static GameObject DispatchGDKGameObject;

        public static bool TryInitialize()
        {
            if (Instance == null)
            {
                Debug.Log("GDK XGameRuntime Library not initialized.");
                // Instantiate the GDKGameRuntime object that will call Awake.
                new GameObject("GDKGamingRuntimeManager").AddComponent<GDKGameRuntime>();
                if (!Initialized)
                {
                    return false;
                }
            }

            Debug.Log($"GDK XGameRuntime Library initialized: {Initialized}");
            Debug.Log($"GDK Xbox Live API SCID: {GameConfigScid}");
            Debug.Log($"GDK TitleId: {GameConfigTitleId}");
            Debug.Log($"GDK Sandbox: {GameConfigSandbox}");
            
            return Initialized;
        }

        private static bool InitializeOrDestroyInstance(GDKGameRuntime newInstance)
        {
            if (Instance != newInstance && Instance != null)
            {
                Debug.LogWarning($"An instance of {newInstance.GetType().Name} already exist. Destroying {newInstance}...");
                Destroy(newInstance.gameObject);
                return false;
            }

            Instance = newInstance;
            GameConfigTitleId = GdkPlatformSettings.gameConfigTitleId;
            GameConfigScid = GdkPlatformSettings.gameConfigScid;
            DontDestroyOnLoad(Instance.gameObject);
            return InitializeRuntime();
        }

        private static bool InitializeRuntime(bool forceInitialization = false)
        {
            if (HR.FAILED(InitializeGamingRuntime(forceInitialization)) ||
                !InitializeXboxLive(GameConfigScid))
            {
                Initialized = false;
                return false;
            }

            // Not necessary but handy to know when debugging
            int hResult = SDK.XGameGetXboxTitleId(out var titleId);
            if (HR.FAILED(hResult))
            {
                Debug.Log($"FAILED: Could not get TitleID! hResult: 0x{hResult:x} ({HR.NameOf(hResult)})");
            }

            if (titleId.ToString("X").ToLower().Equals(GameConfigTitleId.ToLower()) == false)
            {
                Debug.LogWarning($"WARNING! Expected Title Id: {GameConfigTitleId} got: {titleId:X}");
            }

            GameConfigTitleId = titleId.ToString("X");

            hResult = SDK.XSystemGetXboxLiveSandboxId(out var sandboxId);
            if (HR.FAILED(hResult))
            {
                Debug.Log($"FAILED: Could not get SandboxID! HResult: 0x{hResult:x} ({HR.NameOf(hResult)})");
            }

            if (sandboxId.Equals(GameConfigSandbox) == false)
            {
                Debug.LogWarning($"WARNING! Expected sandbox Id: {GameConfigSandbox} got: {sandboxId}");
            }

            GameConfigSandbox = sandboxId;

            Debug.Log($"GDK Initialized, titleId: {GameConfigTitleId}, sandboxId: {GameConfigSandbox}");

            // Done!
            Initialized = true;
            GameRuntimeInitialized?.Invoke();
            return true;
        }

        /// <summary>
        /// Initializes the main Runtime Library.
        /// Creates the Async Dispatch thread which will handle all calls to work.
        /// </summary>
        private static int InitializeGamingRuntime(bool forceInitialization = false)
        {
            // We do not want stack traces for all log statements. (Exceptions logged
            // with Debug.LogException will still have stack traces though.):
            //Application.SetStackTraceLogType(LogType.Log, StackTraceLogType.None);

            Debug.Log("Initializing XGame Runtime Library.");

            if (Initialized && forceInitialization == false)
            {
                Debug.Log("Gaming Runtime already initialized.");
                return 0;
            }

            int hResult = SDK.XGameRuntimeInitialize();
            if (HR.FAILED(hResult))
            {
                Debug.Log($"FAILED: Initialize XGameRuntime, HResult: 0x{hResult:X} ({HR.NameOf(hResult)})");
                return hResult;
            }

            StartAsyncDispatchCoroutine();

            return 0;
        }

        /// <summary>
        /// Initializes the Xbox Live Basic API this is required for all Xbox Live API calls.
        /// </summary>
        /// <returns>The HResult value of initializing Xbox Live</returns>
        private static bool InitializeXboxLive(string scid)
        {
            Debug.Log($"Initializing Xbox Live API (SCID: {scid}).");

            int hResult = SDK.XBL.XblInitialize(scid);
            if (HR.FAILED(hResult) && hResult != HR.E_XBL_ALREADY_INITIALIZED)
            {
                Debug.Log($"FAILED: Initialize Xbox Live, HResult: 0x{hResult:X}, {HR.NameOf(hResult)}");
                return false;
            }

            return true;
        }

        /// <summary>
        /// This allows the native code space to create asynchronous blocks and work in parallel to common calls.
        /// </summary>
        private static void StartAsyncDispatchCoroutine()
        {
            // We need to execute SDK.XTaskQueueDispatch(0) to pump all GDK events.
            if (DispatchGDKGameObject == null)
            {
                int hResult = SDK.CreateDefaultTaskQueue();
                if (HR.FAILED(hResult))
                {
                    Debug.Log($"FAILED: XTaskQueueCreate, HResult: 0x{hResult:X}");
                    return;
                }

                Instance.StartCoroutine(DispatchGDKTaskQueue());
            }

            // You also could execute SDK.XTaskQueueDispatch(0) in the main thread
            // (i.e. update method) but it can create stuttering, since some GDK actions
            //  can block the thread.

            // void Update() {
            //     SDK.XTaskQueueDispatch(0);
            // }

            // Another option is to make an actual thread as well with a Thread.Sleep(32),
            // but keep in mind that threads can cause crashes and other problems when accessing
            // Unity functions, since Unity is not thread safe.

            // Thread dispatchJob = new Thread(DispatchGDKTaskQueue) { Name = "GDK Task Queue Dispatch" };
            // dispatchJob.Start();
        }

        private void Awake()
        {
            InitializeOrDestroyInstance(this);
        }

        /// <summary>
        /// OnApplicationQuit/OnDestroy needs to handle cleanup especially if we have
        /// initialized the services from within the editor player
        /// </summary>
        private void OnApplicationQuit()
        {
            SDK.CloseDefaultXTaskQueue();
            Debug.Log("Uninitializing Xbox Live API.");
            SDK.XBL.XblCleanup(null);
            Debug.Log("Uninitializing XGame Runtime Library.");
            SDK.XGameRuntimeUninitialize();

            Initialized = false;
            Instance = null;
        }

        /// <summary>
        /// The actual Dispatch Task Queue - This executes all GDK Asynchronous block work natively
        /// </summary>
        private static IEnumerator DispatchGDKTaskQueue()
        {
            while (true)
            {
                SDK.XTaskQueueDispatch(0);
                yield return null;
            }
        }
    }
}