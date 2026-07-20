using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
using Unity.Microsoft.GDK.Discovery;

[InitializeOnLoad]
public static class AutoImportSampleDependencies
{
    // During project load, automatically imports PlayFab dependencies from the provided package
    // and updates PlayFab Dlls to match latest GDK vesion installed on the PC.
    static AutoImportSampleDependencies()
    {
        SampleSetup.AutoImportPlayFabPackages();
    }
}

public class SampleSetup
{
   static readonly string playFabSdkPackage = "Assets/SDKs/playfab-party_rumble-sample.unitypackage";
   static readonly string playFabPartyVersionFile = "Assets/PlayFabPartySDK/Source/Scripts/PartyUnitySDK/PlayFabPartyVersion.cs";
   static readonly string gdkPlayFabPartyLibraryPath_240600 = "GRDK/ExtensionLibraries/PlayFab.Party.Cpp/Redist/CommonConfiguration/neutral";
   static readonly string gdkPlayFabPartyXboxLiveLibraryPath_240600 = "GRDK/ExtensionLibraries/PlayFab.PartyXboxLive.Cpp/Redist/CommonConfiguration/neutral";
   static readonly string gdkPlayFabPartyLibraryPath_241000 = "GRDK/ExtensionLibraries/PlayFab.Party.Cpp/Redist/x64";
   static readonly string gdkPlayFabPartyXboxLiveLibraryPath_241000 = "GRDK/ExtensionLibraries/PlayFab.PartyXboxLive.Cpp/Redist/x64";
   static readonly string targetPlayFabPartyDllPath = "Assets/PlayFabPartySDK/Source/DLLs/GameCore";

   static SampleSetup()
   {
       AssetDatabase.importPackageStarted += OnImportPackageStarted;
       AssetDatabase.importPackageCompleted += OnImportPackageCompleted;
       AssetDatabase.importPackageFailed += OnImportPackageFailed;
       AssetDatabase.importPackageCancelled += OnImportPackageCancelled;
   }

   [MenuItem("SampleSetup/Import PlayFab Packages")]
   private static void ReimportPlayFabPackages()
   {
       ImportPlayFabPackages(true);
   }

   public static void AutoImportPlayFabPackages()
   {
       if (!File.Exists(playFabPartyVersionFile))
       {
           ImportPlayFabPackages(false);
       }
   }

   private static void ImportPlayFabPackages(bool interactive)
   {
       string packagePath = playFabSdkPackage;

       if (File.Exists(packagePath))
       {
           AssetDatabase.ImportPackage(packagePath, interactive);
       }
       else
       {
           Debug.Log($"Failed to find {packagePath}");
       }
   }

   [MenuItem("SampleSetup/Update PlayFab GDK Dlls")]
   private static void UpdatePlayFabDlls()
   {
       int gdkLatestVersion = 0;
       string gdkPath = string.Empty;
       var gdks = GdkEnumerator.DiscoveredGdks;

       if (gdks.Any())
       {
           foreach (var gdk in gdks)
           {
               if (gdk.Edition > gdkLatestVersion)
               {
                   gdkLatestVersion = gdk.Edition;
                   gdkPath = gdk.Path;
               }
           }
       }
       else
       {
           Debug.LogWarning("No installed GDKs were found.");
           return;
       }

       // If we have a GDK path, import PlayFab libraries from latest GDK install
       if (!string.IsNullOrEmpty(gdkPath))
       {
           // Copy Party.dll from GDK install to 'GameCore' folder and rename to PartyWin.dll
           string sourcePath = $"{gdkPath}{gdkLatestVersion}/{gdkPlayFabPartyLibraryPath_240600}";
           if (gdkLatestVersion >= 241000)
           {
               sourcePath = $"{gdkPath}{gdkLatestVersion}/{gdkPlayFabPartyLibraryPath_241000}";
           }
           string sourceName = "Party";
           string targetPath = $"{targetPlayFabPartyDllPath}";
           string targetName = "PartyWin";

           CopyDll(sourcePath, sourceName, targetPath, targetName);

           // Copy PartyXboxLive.dll from GDK install to 'GameCore' folder
           sourcePath = $"{gdkPath}{gdkLatestVersion}/{gdkPlayFabPartyXboxLiveLibraryPath_240600}";
           if (gdkLatestVersion >= 241000)
           {
               sourcePath = $"{gdkPath}{gdkLatestVersion}/{gdkPlayFabPartyXboxLiveLibraryPath_241000}";
           }
           sourceName = "PartyXboxLive";
           targetPath = $"{targetPlayFabPartyDllPath}";
           targetName = "PartyXboxLive";

           CopyDll(sourcePath, sourceName, targetPath,targetName);

           AssetDatabase.Refresh();
       }
   }

   private static void CopyDll(string sourcePath, string sourceName, string targetPath, string targetName)
   {
       // Copy Party.dll from GDK install to 'GameCore' folder in project and rename to PartyWin.dll
       string dllSource = $"{sourcePath}/{sourceName}.dll";
       string pdbSource = $"{sourcePath}/{sourceName}.pdb";
       string dllTarget = $"{targetPath}/{targetName}.dll";
       string pdbTarget = $"{targetPath}/{targetName}.pdb";

       if (File.Exists(dllSource))
       {
           Debug.Log($"Copying {dllSource} to {dllTarget}");
           File.Copy(dllSource, dllTarget, true);

           Debug.Log($"Copying {pdbSource} to {pdbTarget}");
           File.Copy(pdbSource, pdbTarget, true);
       }
       else
       {
           Debug.LogWarning($"Failed to find {sourceName}.dll at {dllSource}. GDK location may have changed.");
       }
   }

   private static void OnImportPackageCancelled(string packageName)
   {
       Debug.Log($"Cancelled the import of package: {packageName}");
   }

   private static void OnImportPackageCompleted(string packagename)
   {
       Debug.Log($"Imported package: {packagename}");

       // Update PlayFab Dlls to match latest GDK version on PC
       UpdatePlayFabDlls();
   }

   private static void OnImportPackageFailed(string packagename, string errormessage)
   {
       Debug.Log($"Failed importing package: {packagename} with error: {errormessage}");
   }

   private static void OnImportPackageStarted(string packagename)
   {
       Debug.Log($"Started importing package: {packagename}");
   }
}
