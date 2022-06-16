# Xbox-GDK-Samples

This repo contains game development samples written by the Microsoft Xbox Advanced Technology Group using the Microsoft Game Development Kit (GDK).

* [Kits](/Kits) contains support code used by the samples
* [Media](/Media) contains media files used by the samples
* [Samples](#Samples-by-category) contains samples
  * [Audio](#Audio)
  * [IntroGraphics](#Intro-Graphics)
  * [Graphics](#Graphics)
  * [Live](#Live)
  * [System](#System)
  * [Tools](#Tools)
  * [xCloud](#xCloud)

# Requirements

* Visual Studio 2017 ([15.9](https://walbourn.github.io/vs-2017-15-9-update/) update) or Visual Studio 2019
* Microsoft Game Development Kit (GDK)

## Privacy Statement

When compiling and running a sample, the file name of the sample executable will be sent to Microsoft to help track sample usage. To opt-out of this data collection, you can remove the block of code in ``Main.cpp`` labeled _Sample Usage Telemetry_.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/privacystatement/).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

## Other Samples

For more ATG samples, see [DirectML Samples](https://github.com/microsoft/DirectML), [PlayFab-Samples](https://github.com/PlayFab/PlayFab-Samples), [Xbox-ATG-Samples](https://github.com/microsoft/Xbox-ATG-Samples), and [Xbox-LIVE-Samples](https://github.com/microsoft/xbox-live-samples).

## Samples by category


### Audio

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                   AdvancedSpatialSounds|            [Xbox](Samples/Audio/AdvancedSpatialSounds)|                                                       |                                                       |
|                              InGameChat|                       [Xbox](Samples/Audio/InGameChat)|                         [PC](Samples/Audio/InGameChat)|                                                       |
|                       SimplePlay3DSound|                [Xbox](Samples/Audio/SimplePlay3DSound)|                                                       |                                                       |
|                         SimplePlaySound|                  [Xbox](Samples/Audio/SimplePlaySound)|                                                       |                                                       |
|                   SimplePlaySoundStream|            [Xbox](Samples/Audio/SimplePlaySoundStream)|                                                       |                                                       |
|                  SimpleSpatialPlaySound|           [Xbox](Samples/Audio/SimpleSpatialPlaySound)|                                                       |                                                       |
|                     SimpleWASAPICapture|              [Xbox](Samples/Audio/SimpleWASAPICapture)|                                                       |                                                       |
|                   SimpleWASAPIPlaySound|            [Xbox](Samples/Audio/SimpleWASAPIPlaySound)|                                                       |                                                       |

### Graphics

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                           AdvancedESRAM|                 [Xbox](Samples/Graphics/AdvancedESRAM)|                                                       |                                                       |
|                            Antialiasing|                  [Xbox](Samples/Graphics/Antialiasing)|                                                       |                                                       |
|                        ComputeParticles|              [Xbox](Samples/Graphics/ComputeParticles)|                                                       |                                                       |
|                       DeferredParticles|             [Xbox](Samples/Graphics/DeferredParticles)|                                                       |                                                       |
|                         ExecuteIndirect|               [Xbox](Samples/Graphics/ExecuteIndirect)|                                                       |                                                       |
|                       FastBlockCompress|             [Xbox](Samples/Graphics/FastBlockCompress)|                                                       |                                                       |
|                                   HDR10|                         [Xbox](Samples/Graphics/HDR10)|                                                       |                                                       |
|                             HistogramCS|                   [Xbox](Samples/Graphics/HistogramCS)|                                                       |                                                       |
|                             HlslCompile|                   [Xbox](Samples/Graphics/HlslCompile)|                                                       |                   [Tool](Samples/Graphics/HlslCompile)|
|                       MeshletInstancing|             [Xbox](Samples/Graphics/MeshletInstancing)|               [PC](Samples/Graphics/MeshletInstancing)|                                                       |
|                            PointSprites|                  [Xbox](Samples/Graphics/PointSprites)|                                                       |                                                       |
|                               SimpleHDR|                     [Xbox](Samples/Graphics/SimpleHDR)|                                                       |                                                       |
|                               SimplePBR|                     [Xbox](Samples/Graphics/SimplePBR)|                       [PC](Samples/Graphics/SimplePBR)|                                                       |
|                         SmokeSimulation|               [Xbox](Samples/Graphics/SmokeSimulation)|                                                       |                                                       |
|                        VisibilityBuffer|[Xbox](Samples/Graphics/VisibilityBuffer/VisibilityBuffer)|[PC](Samples/Graphics/VisibilityBuffer/VisibilityBuffer)|                                                       |

### IntroGraphics

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                            SimpleBezier|             [Xbox](Samples/IntroGraphics/SimpleBezier)|                                                       |                                                       |
|                           SimpleCompute|            [Xbox](Samples/IntroGraphics/SimpleCompute)|                                                       |                                                       |
|                SimpleDeviceAndSwapChain| [Xbox](Samples/IntroGraphics/SimpleDeviceAndSwapChain)|                                                       |                                                       |
|                  SimpleDynamicResources|   [Xbox](Samples/IntroGraphics/SimpleDynamicResources)|     [PC](Samples/IntroGraphics/SimpleDynamicResources)|                                                       |
|                        SimpleInstancing|         [Xbox](Samples/IntroGraphics/SimpleInstancing)|                                                       |                                                       |
|                          SimpleLighting|           [Xbox](Samples/IntroGraphics/SimpleLighting)|                                                       |                                                       |
|                        SimpleMeshShader|         [Xbox](Samples/IntroGraphics/SimpleMeshShader)|           [PC](Samples/IntroGraphics/SimpleMeshShader)|                                                       |
|                              SimpleMSAA|               [Xbox](Samples/IntroGraphics/SimpleMSAA)|                                                       |                                                       |
|                   SimpleSamplerFeedback|    [Xbox](Samples/IntroGraphics/SimpleSamplerFeedback)|                                                       |                                                       |
|                           SimpleTexture|            [Xbox](Samples/IntroGraphics/SimpleTexture)|                                                       |                                                       |
|                          SimpleTriangle|           [Xbox](Samples/IntroGraphics/SimpleTriangle)|      [PC](Samples/IntroGraphics/SimpleTriangleDesktop)|                                                       |

### Live

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                            Achievements|                      [Xbox](Samples/Live/Achievements)|                        [PC](Samples/Live/Achievements)|                                                       |
|                     DownloadableContent|               [Xbox](Samples/Live/DownloadableContent)|                 [PC](Samples/Live/DownloadableContent)|                                                       |
|                            Fundamentals|                                                       |                [PC](Samples/Live/Fundamentals_Desktop)|                                                       |
|                             InGameStore|                       [Xbox](Samples/Live/InGameStore)|                         [PC](Samples/Live/InGameStore)|                                                       |
|                LeaderboardsEventManaged|          [Xbox](Samples/Live/LeaderboardsEventManaged)|            [PC](Samples/Live/LeaderboardsEventManaged)|                                                       |
|                LeaderboardsTitleManaged|          [Xbox](Samples/Live/LeaderboardsTitleManaged)|    [PC](Samples/Live/LeaderboardsTitleManaged_Desktop)|                                                       |
|                                    mDNS|                              [Xbox](Samples/Live/mDNS)|                        [PC](Samples/Live/mDNS_Desktop)|                                                       |
|            MicrosoftStoreServicesClient|      [Xbox](Samples/Live/MicrosoftStoreServicesClient)|        [PC](Samples/Live/MicrosoftStoreServicesClient)|                                                       |
|                      SimpleCrossGenMPSD|                [Xbox](Samples/Live/SimpleCrossGenMPSD)|                  [PC](Samples/Live/SimpleCrossGenMPSD)|                                                       |
|                              SimpleHttp|                        [Xbox](Samples/Live/SimpleHttp)|                          [PC](Samples/Live/SimpleHttp)|                                                       |
|                        SimpleWebSockets|                  [Xbox](Samples/Live/SimpleWebSockets)|                    [PC](Samples/Live/SimpleWebSockets)|                                                       |
|                           SocialManager|                     [Xbox](Samples/Live/SocialManager)|                       [PC](Samples/Live/SocialManager)|                                                       |
|                            TitleStorage|                      [Xbox](Samples/Live/TitleStorage)|                                                       |                                                       |

### System

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|               AdvancedExceptionHandling|       [Xbox](Samples/System/AdvancedExceptionHandling)|         [PC](Samples/System/AdvancedExceptionHandling)|                                                       |
|                       OutOfProcDumpTool|       [Xbox](Samples/System/AdvancedExceptionHandling)|                                                       |                                                       |
|                 AsynchronousProgramming|         [Xbox](Samples/System/AsynchronousProgramming)|           [PC](Samples/System/AsynchronousProgramming)|                                                       |
|                               Collision|                       [Xbox](Samples/System/Collision)|                         [PC](Samples/System/Collision)|                                                       |
|                     CustomEventProvider|             [Xbox](Samples/System/CustomEventProvider)|                                                       |                                                       |
|                         DataBreakPoints|                 [Xbox](Samples/System/DataBreakPoints)|                   [PC](Samples/System/DataBreakPoints)|                                                       |
|                          FrontPanelDemo|                  [Xbox](Samples/System/FrontPanelDemo)|                                                       |                                                       |
|                       FrontPanelDolphin|               [Xbox](Samples/System/FrontPanelDolphin)|                                                       |                                                       |
|                          FrontPanelGame|                  [Xbox](Samples/System/FrontPanelGame)|                                                       |                                                       |
|                          FrontPanelLogo|                  [Xbox](Samples/System/FrontPanelLogo)|                                                       |                                                       |
|                          FrontPanelText|                  [Xbox](Samples/System/FrontPanelText)|                                                       |                                                       |
|                    GameInputInterfacing|            [Xbox](Samples/System/GameInputInterfacing)|                                                       |                                                       |
|                     GameInputSequential|             [Xbox](Samples/System/GameInputSequential)|                                                       |                                                       |
|                                 Gamepad|                         [Xbox](Samples/System/Gamepad)|                                                       |                                                       |
|                        GamepadVibration|                [Xbox](Samples/System/GamepadVibration)|                                                       |                                                       |
|                                GameSave|                        [Xbox](Samples/System/GameSave)|                  [PC](Samples/System/GameSave_Desktop)|                                                       |
|                     IntelligentDelivery|             [Xbox](Samples/System/IntelligentDelivery)|               [PC](Samples/System/IntelligentDelivery)|                                                       |
|                            LocalStorage|                    [Xbox](Samples/System/LocalStorage)|                      [PC](Samples/System/LocalStorage)|                                                       |
|                              MouseInput|                      [Xbox](Samples/System/MouseInput)|                                                       |                                                       |
|                      NLSAndLocalization|              [Xbox](Samples/System/NLSAndLocalization)|                [PC](Samples/System/NLSAndLocalization)|                                                       |
|                     SimpleDirectStorage|             [Xbox](Samples/System/SimpleDirectStorage)|                                                       |                                                       |
|                SimpleDirectStorageCombo|        [Xbox](Samples/System/SimpleDirectStorageCombo)|          [PC](Samples/System/SimpleDirectStorageCombo)|                                                       |
|                 SimpleExceptionHandling|         [Xbox](Samples/System/SimpleExceptionHandling)|           [PC](Samples/System/SimpleExceptionHandling)|                                                       |
|                          SimpleFFBWheel|                  [Xbox](Samples/System/SimpleFFBWheel)|                                                       |                                                       |
|                        SimpleFrontPanel|                [Xbox](Samples/System/SimpleFrontPanel)|                                                       |                                                       |
|                               SimplePLM|                       [Xbox](Samples/System/SimplePLM)|                                                       |                                                       |
|                         SimpleUserModel|                 [Xbox](Samples/System/SimpleUserModel)|                                                       |                                                       |
|                              SystemInfo|                      [Xbox](Samples/System/SystemInfo)|                        [PC](Samples/System/SystemInfo)|                                                       |
|                          UserManagement|                  [Xbox](Samples/System/UserManagement)|                                                       |                                                       |

### Tools

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                             BWOIExample|                      [Xbox](Samples/Tools/BWOIExample)|                        [PC](Samples/Tools/BWOIExample)|                                                       |
|                          CacheTestCombo|                   [Xbox](Samples/Tools/CacheTestCombo)|                     [PC](Samples/Tools/CacheTestCombo)|                                                       |
|                                DumpTool|                         [Xbox](Samples/Tools/DumpTool)|                                                       |                                                       |
|                            ConverterApp|                                                       |                                                       |                 [Tool](Samples/Tools/MeshletConverter)|
|                                 Runtime|                                                       |                                                       |                 [Tool](Samples/Tools/MeshletConverter)|
|                    OSPrimitiveTestCombo|             [Xbox](Samples/Tools/OSPrimitiveTestCombo)|               [PC](Samples/Tools/OSPrimitiveTestCombo)|                                                       |
|                               xbgamepad|                                                       |                                                       |                        [Tool](Samples/Tools/xbgamepad)|
|                                xtexconv|                                                       |                                                       |                         [Tool](Samples/Tools/xtexconv)|

### xCloud

|                                    Path|                                                   Xbox|                                                     PC|                                                   Tool|
|----------------------------------------|                                                 ------|                                                 ------|                                                 ------|
|                  SimpleCloudAwareSample|          [Xbox](Samples/xCloud/SimpleCloudAwareSample)|                                                       |                                                       |
