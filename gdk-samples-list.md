# Microsoft Game Development Kit samples list

## Categories

Following is a list of categories for the samples.

- [Microsoft Game Development Kit samples](#microsoft-game-development-kit-samples-list) 
  - [Categories](#categories)
    - [Audio](#audio)
    - [Graphics](#graphics)
    - [Handheld](#handheld)
    - [IntroGraphics](#intrographics)
    - [Live](#live)
    - [System](#system)
    - [Tools](#tools)
    - [Unity](#unity)
    - [xCloud](#xcloud)

<a id="audio"></a> 
 
### Audio 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__AdvancedSpatialSounds__](Samples/Audio/AdvancedSpatialSounds) | This sample demonstrates how use ISpatialAudioClient to playback both static and dynamic positional audio using Windows Sonic technologies in an Xbox title. | ✓ |  | 
| [__InGameChat__](Samples/Audio/InGameChat) | The InGameChat sample provides a working example of integrating the GameChat2 library into an Xbox title. It brings together the pieces needed to demonstrate in-title VOIP communications: GameChat, Multiplayer Sessions, and Peer Networking. | ✓ | ✓ | 
| [__SimpleCustomAPO__](Samples/Audio/SimpleCustomAPO) | This sample demonstrates how to play a wav file using XAudio2 on the Xbox using a custom xAPO effect. | ✓ |  | 
| [__SimplePlay3DSound__](Samples/Audio/SimplePlay3DSound) | This sample demonstrates how use XAudio2 and X3DAudio to playback positional audio on Xbox. | ✓ |  | 
| [__SimplePlaySound__](Samples/Audio/SimplePlaySound) | This sample demonstrates how to play a wav file using XAudio2 on the Xbox. | ✓ |  | 
| [__SimplePlaySoundStream__](Samples/Audio/SimplePlaySoundStream) | This sample demonstrates how to stream a wav file using XAudio2 on the Xbox. | ✓ |  | 
| [__SimpleSpatialPlaySound__](Samples/Audio/SimpleSpatialPlaySound) | This sample demonstrates how use ISpatialAudioClient to playback static audio with height channels using Windows Sonic technologies in an Xbox title. | ✓ |  | 
| [__SimpleWASAPICapture__](Samples/Audio/SimpleWASAPICapture) | This sample demonstrates how to capture audio using WASAPI on Xbox. | ✓ |  | 
| [__SimpleWASAPIPlaySound__](Samples/Audio/SimpleWASAPIPlaySound) | This sample demonstrates how to play setup and play a simple sound (sine tone) to a WASAPI render endpoint on Xbox. | ✓ |  | 
 
<a id="graphics"></a> 
 
### Graphics 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__AdvancedESRAM__](Samples/Graphics/AdvancedESRAM) | This sample demonstrates the use of advanced DirectX 12.x memory features on Xbox One to effectively alias memory for Direct3D resources. | ✓ |  | 
| [__AdvancedLighting__](Samples/Graphics/AdvancedLighting) | This sample demonstrates different techniques to improve performance of deferred rendering with many lights. | ✓ | ✓ | 
| [__AMD_FSR__](Samples/Graphics/AMD_FSR) | This sample demonstrates AMD FSR™ Upscaling 4 on the Microsoft GDK, integrated through the unified AMD FSR™ SDK. | ✓ | ✓ | 
| [__AMDFidelityFX_CACAO__](Samples/Graphics/AMDFidelityFX_CACAO) | This sample demonstrates AMD's FidelityFX Combined Adaptive Compute Ambient Occlusion (CACAO) algorithm. | ✓ | ✓ | 
| [__AMDFidelityFX_CAS__](Samples/Graphics/AMDFidelityFX_CAS) | This sample demonstrates AMD's FidelityFX Contrast Adaptive Sharpening (CAS) algorithm. | ✓ | ✓ | 
| [__AMDFidelityFX_FSR3FrameInterpolation__](Samples/Graphics/AMDFidelityFX_FSR3FrameInterpolation) | This sample demonstrates AMD's FidelityFX FSR3 algorithm. | ✓ | ✓ | 
| [__AMDFidelityFX_RTShadowDenoiser__](Samples/Graphics/AMDFidelityFX_RTShadowDenoiser) | This sample shows an implementation of the AMD's FidelityFX RT Shadow Denoiser applied against a basic implementation of DXR 1.0 Raytraced Shadows. | ✓ | ✓ | 
| [__AMDFidelityFX_SuperResolution__](Samples/Graphics/AMDFidelityFX_SuperResolution) | This sample demonstrates AMD's FidelityFX Super Resolution (FSR) algorithm. | ✓ | ✓ | 
| [__AMDFidelityFX_VariableShading__](Samples/Graphics/AMDFidelityFX_VariableShading) | This sample demonstrates AMD's FidelityFX Variable Shading algorithm. | ✓ | ✓ | 
| [__Antialiasing__](Samples/Graphics/Antialiasing) | This sample shows different antialiasing methods (SMAA, SMAA2x, and FXAA) on Xbox. | ✓ |  | 
| [__AutoRGB__](Samples/Graphics/AutoRGB) | This sample demonstrates how to extract a single, representative ambient color from a given scene and use it to light up lamps from connected devices (HID). Uses the LampArray api. | ✓ |  | 
| [__ComputeParticles__](Samples/Graphics/ComputeParticles) | This sample demonstrates how to use compute shaders and append buffers to perform a basic particle simulation and performantly render a silly number of particles. | ✓ |  | 
| [__DeferredParticles__](Samples/Graphics/DeferredParticles) | This DirectX 12 sample demonstrates a method for rendering lit smoke particles in either a forward or deferred fashion. | ✓ |  | 
| [__DirectStorageTextureShuffling__](Samples/Graphics/DirectStorageTextureShuffling) | This sample demonstrates how to use DirectStorage swizzle modes together with shuffling (deinterleaving) the data of BC1, BC3, BC4 and BC5 textures in specific ways to improve ZLib compression.The sample consists of an offline tool to prepare the texture data, and runtime code with compute shaders to unshuffle the texture data. | ✓ | ✓ | 
| [__DXRSimpleLighting__](Samples/Graphics/DXRSimpleLighting) | This sample demonstrates how diffuse lighting and animated cubes can be rendered with the DirectX Raytracing API on a Xbox Series X\|S device. | ✓ |  | 
| [__DXRTriangle__](Samples/Graphics/DXRTriangle/DXRTriangle) | This sample demonstrates the basic use of the DirectX Raytracing API on Xbox Series X\|S and PC. | ✓ | ✓ | 
| [__DynamicCubeMap__](Samples/Graphics/DynamicCubeMap) | This sample demonstrates how to render the scene to a cubemap at runtime and then sample from the cubemap. | ✓ |  | 
| [__DynamicLOD__](Samples/Graphics/DynamicLOD) | This sample demonstrates how to leverage amplification shaders to do per-instance frustum culling and mesh level-of-detail (LOD) selection entirely on the GPU for an arbitrary number of instances on Xbox Series X\|S and PC. | ✓ | ✓ | 
| [__ExecuteIndirect__](Samples/Graphics/ExecuteIndirect) | This sample demonstrates usage of DirectX 12's ExecuteIndirect API for asynchronously building rendering commands. | ✓ |  | 
| [__FastBlockCompress__](Samples/Graphics/FastBlockCompress) | This sample demonstrates how you can use DirectCompute to perform fast texture compression at run time to the BC1, BC3, and BC5 formats based on the classic Fast Block Compression algorithm on Xbox. | ✓ |  | 
| [__FramePacing__](Samples/Graphics/FramePacing) | This sample demonstrates how CPU work, GPU work, swap chain buffers, and Xbox frame scheduling interact to affect frame rate, latency, and presentation margin. | ✓ |  | 
| [__GeometricExpansion__](Samples/Graphics/GeometricExpansion) | This sample demonstrates the geometric expansion capabilities of mesh shaders for Xbox Series X\|S and PC. | ✓ | ✓ | 
| [__HDR10__](Samples/Graphics/HDR10) | Switch a UHD TV into HDR mode and render a HDR10 scene with values higher than 1.0f, which will be displayed as brighter than white on a UHD TV using DirectX 12 on Xbox. | ✓ |  | 
| [__HDRCalibration__](Samples/Graphics/HDRCalibration) | This sample switches an HDR TV to HDR mode and then presents the user with several calibration options to adjust the visible detail in brights and darks, the overall brightness and color saturation of the image. | ✓ |  | 
| [__HDRDisplayMapping__](Samples/Graphics/HDRDisplayMapping) | This sample shows that even when rendering a HDR scene on a HDR capable TV, some tone mapping is still needed, referred to as 'HDR display mapping'. | ✓ |  | 
| [__HDRPrecision__](Samples/Graphics/HDRPrecision) | This sample demonstrates how precision and GPU performance are affected when using different formats and color spaces while rendering HDR on Xbox Series X\|S. | ✓ |  | 
| [__HistogramCS__](Samples/Graphics/HistogramCS) | This sample demonstrates some performance considerations for Compute Shaders on Xbox. | ✓ |  | 
| [__HlslCompile__](Samples/Graphics/HlslCompile) | The sample compiles the same pixel shader in a number of different ways, to illustrate different options for PC-side asset build. It also shows how to precompile pipeline state offline with the ACO shader compiler for Xbox Series X\|S. | ✓ |  | 
| [__MeshletCull__](Samples/Graphics/MeshletCull) | This sample demonstrates how to leverage amplification shaders to cull meshlets against the camera using per-meshlet culling metadata on Xbox Series X\|S and PC. | ✓ | ✓ | 
| [__MeshletInstancing__](Samples/Graphics/MeshletInstancing) | Demonstrates GPU instancing for DirectX 12 Mesh Shaders. | ✓ | ✓ | 
| [__MP4Reader__](Samples/Graphics/MP4Reader) | This sample shows how the Media Foundation Source Reader can be used to read a MP4 file which contains an H264 or HEVC video stream, and decode it using hardware acceleration on Xbox. | ✓ |  | 
| [__PipelinedPostprocess__](Samples/Graphics/PipelinedPostprocess) | This sample demonstrates how to overlap the postprocess phase of one frame with the beginning of the next frame, potentially achieving some performance benefit from parallelization. | ✓ |  | 
| [__PointSprites__](Samples/Graphics/PointSprites) | Demonstrates ten methods of rendering point sprites in DirectX 12. | ✓ |  | 
| [__ShaderReflect__](Samples/Graphics/ShaderReflect) | This sample demonstrates how you can access shader reflection information and how it relates to the root signature. | ✓ |  | 
| [__Simple120Hz__](Samples/Graphics/Simple120Hz) | This sample demonstrates the basics of rendering at several refresh rates on Xbox: 30Hz, 40Hz, 60Hz, and 120Hz. | ✓ |  | 
| [__SimpleHDR__](Samples/Graphics/SimpleHDR) | This is a simple sample showing how to implement HDR on Xbox. | ✓ |  | 
| [__SimpleMeshlet__](Samples/Graphics/SimpleMeshlet) | This sample introduces the meshlet data structure and provides an example of rendering using meshlets on Xbox Series X\|S and PC. | ✓ | ✓ | 
| [__SimplePBR__](Samples/Graphics/SimplePBR) | This sample demonstrates physically based rendering (PBR) on Xbox using DirectX 12. | ✓ | ✓ | 
| [__SmokeSimulation__](Samples/Graphics/SmokeSimulation) | This sample demonstrates how to use Compute Shader 6.0 and 3D textures to implement basic 3D Navier-Stokes flow simulation. | ✓ |  | 
| [__TemporalAntialiasing__](Samples/Graphics/TemporalAntialiasing) | This sample demonstrates how to implement Temporal Antialiasing (TAA), plus a series of other techniques which help improve the algoritm stability and fix some of its inherent issues. | ✓ | ✓ | 
| [__VisibilityBuffer__](Samples/Graphics/VisibilityBuffer/VisibilityBuffer) | Demonstrates a visibility buffer (deferred) rendering technique, making use of Mesh Shaders and HLSL 6.6 Dynamic Resources. | ✓ | ✓ | 
| [__VisibilityBufferIndexReorder__](Samples/Graphics/VisibilityBuffer/VisibilityBufferIndexReorder) | This sample augments the VisibilityBuffer sample with a simple index buffer reordering to improve vertex shader performance on Xbox. | ✓ | ✓ | 
 
<a id="handheld"></a> 
 
### Handheld 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__HandheldBestPractices__](Samples/Handheld/HandheldBestPractices) | This sample demonstrates the best practices for running a title on handheld devices. |  | ✓ | 
 
<a id="intrographics"></a> 
 
### IntroGraphics 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__SimpleBezier__](Samples/IntroGraphics/SimpleBezier) | This sample demonstrates how to create hull and domain shaders to draw a tessellated Bezier surface representing a Mobius strip for DirectX 12 on Xbox. | ✓ |  | 
| [__SimpleCompute__](Samples/IntroGraphics/SimpleCompute) | SimpleCompute shows how to use DirectCompute (i.e. Direct3D Compute Shader) for DirectX 12 on Xbox. | ✓ |  | 
| [__SimpleDeviceAndSwapChain__](Samples/IntroGraphics/SimpleDeviceAndSwapChain) | This sample demonstrates how to create a Direct3D 12 device and swap chain for an GameCore on Xbox title. | ✓ |  | 
| [__SimpleDynamicResources__](Samples/IntroGraphics/SimpleDynamicResources) | Demonstrates how to use HLSL Dynamic Resources in HLSL Shader Model 6.6. This sample is functionally identical to SimpleTexture, except resources are accessed directly through the heap using ResourceDescriptorHeap[] and SamplerDescriptorHeap[] in HLSL. | ✓ | ✓ | 
| [__SimpleInstancing__](Samples/IntroGraphics/SimpleInstancing) | This sample demonstrates how to use instancing with the Direct3D 12 API on Xbox. | ✓ |  | 
| [__SimpleLighting__](Samples/IntroGraphics/SimpleLighting) | This sample demonstrates how to create a static Direct3D 12 vertex, index, and constant buffer to draw indexed geometry lit by using static and dynamic Lambertian lighting on Xbox. | ✓ |  | 
| [__SimpleMeshShader__](Samples/IntroGraphics/SimpleMeshShader) | This sample is a companion to the SimpleTriangle sample with the exception being that it uses Directx 12 Mesh Shaders. | ✓ | ✓ | 
| [__SimpleMSAA__](Samples/IntroGraphics/SimpleMSAA) | This sample implements an MSAA render target and depth/stencil buffer for a 3D scene using DirectX 12 on Xbox. | ✓ |  | 
| [__SimpleSamplerFeedback__](Samples/IntroGraphics/SimpleSamplerFeedback) | Simple example of using DirectX 12 Sampler Feedback. | ✓ |  | 
| [__SimpleTexture__](Samples/IntroGraphics/SimpleTexture) | This sample implements an MSAA render target and depth/stencil buffer for a 3D scene using DirectX 12 on Xbox. | ✓ |  | 
| [__SimpleTriangle__](Samples/IntroGraphics/SimpleTriangle) | This sample demonstrates how to create a static Direct3D 12 vertex buffer to render a triangle on screen on Xbox. | ✓ |  | 
| [__SimpleTriangleDesktop__](Samples/IntroGraphics/SimpleTriangleDesktop) | This sample demonstrates how to create a static Direct3D 12 vertex buffer to render a triangle on screen. |  | ✓ | 
 
<a id="live"></a> 
 
### Live 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__Achievements__](Samples/Live/Achievements) | This sample demonstrates using the Title-Managed Achievements C-API provided by the Microsoft Game Development Kit. | ✓ | ✓ | 
| [__DownloadableContent__](Samples/Live/DownloadableContent) | This sample demonstrates how to implement enumeration and loading of downloadable content for Xbox LIVE services. | ✓ | ✓ | 
| [__Fundamentals_Desktop__](Samples/Live/Fundamentals_Desktop) | This sample demonstrates signing into Xbox Live and making a license check to ensure that the game is owned by the currently signed in user on PC. |  | ✓ | 
| [__GameHub__](Samples/Live/GameHub) | This sample demonstrates how to implement Franchise Game Hub. | ✓ | ✓ | 
| [__InGameStore__](Samples/Live/InGameStore) | This sample demonstrates the client-based operations used in presenting and operating an in-game storefront. | ✓ | ✓ | 
| [__LeaderboardsEventBased__](Samples/Live/LeaderboardsEventBased) | This sample demonstrates the usage of Xbox Live Leaderboards with Events-Based stats. | ✓ | ✓ | 
| [__LeaderboardsTitleManaged__](Samples/Live/LeaderboardsTitleManaged) | The leaderboards sample demonstrates the usage of Xbox Live Leaderboards with Title-Managed stats. | ✓ | ✓ | 
| [__mDNS__](Samples/Live/mDNS) | This sample demonstrates using mDNS to register a game service and broadcasting it across your local network, as well as demonstrating network discovery and resolving on Xbox. | ✓ | ✓ | 
| [__MicrosoftStoreServicesClient__](Samples/Live/MicrosoftStoreServicesClient) | This sample works with the Microsoft.StoreServices Sample and demonstrates the client side call pattern to do service-to-service auth with the Store Services | ✓ | ✓ | 
| [__MsQuicEcho__](Samples/Live/MsQuicEcho) | This sample is to demonstrate how to implement MsQuic using a simple Echo Server as an example. |  | ✓ | 
| [__PlayFabGameSave__](Samples/Live/PlayFabGameSave) | This sample demonstrates how to use PlayFab Game Saves with the Microsoft Game Development Kit (GDK). | ✓ | ✓ | 
| [__PlayFabLeaderboards__](Samples/Live/PlayFabLeaderboards) | This sample displays the data of the leaderboard registered in PlayFab Leaderboards (v2). As an option, a tool is provided to create PlayFab Leaderboards and write any number of player entities and scores to them. | ✓ | ✓ | 
| [__PlayFabStore__](Samples/Live/PlayFabStore) | This sample demonstrates the client-based operations used in presenting and operating an in-game economy with PlayFab Economy v2. | ✓ | ✓ | 
| [__SimpleCrossGenMPSD__](Samples/Live/SimpleCrossGenMPSD/SimpleCrossGenMPSD) | This sample demonstrates how to use MPSD to implement sessions and matchmaking for both cross generation and single generation games. | ✓ | ✓ | 
| [__SimpleHttp__](Samples/Live/SimpleHttp/SimpleHttp) | This sample demonstrates using XCurl to make HTTP requests including adding the user token and signature to the headers for authenticated Xbox LIVE calls. | ✓ | ✓ | 
| [__SimpleMPA__](Samples/Live/SimpleMPA/SimpleMPA) | This sample demonstrates how to use the Multiplayer Activity Service for activities and invites. | ✓ | ✓ | 
| [__SimpleWebSockets__](Samples/Live/SimpleWebSockets/SimpleWebSockets) | This sample demonstrates using LibHttpClient to connect, send, and receive messages to/from a host via Web Sockets. | ✓ | ✓ | 
| [__SocialManager__](Samples/Live/SocialManager) | This sample demonstrates the Social Manager C-API provided by the Microsoft Gaming SDK (GDK). | ✓ | ✓ | 
| [__TitleStorage__](Samples/Live/TitleStorage) | This sample demonstrates Title Storage API provided by the Microsoft Game Development Kit (GDK). | ✓ | ✓ | 
 
<a id="system"></a> 
 
### System 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__AccessibilitySample__](Samples/System/AccessibilitySample) | This sample demonstrates how to implement cross-platform accessibility features for a title. | ✓ | ✓ | 
| [__AdvancedExceptionHandling__](Samples/System/AdvancedExceptionHandling) | Demonstrates various ways that C++ and SEH exceptions can occur in Microsoft GDK titles including advanced scenarios. | ✓ | ✓ | 
| [__AsynchronousProgramming__](Samples/System/AsynchronousProgramming) | This sample shows how to use XAsync, XTaskQueue, and XAsyncProvider to implement asynchronous programming and task handling in various ways. | ✓ | ✓ | 
| [__Collision__](Samples/System/Collision) | This sample demonstrates DirectXMath's collision types for simple bounding volume tests. | ✓ | ✓ | 
| [__CustomEventProvider__](Samples/System/CustomEventProvider) | This sample demonstrates how to use custom ETW event providers on Xbox. | ✓ |  | 
| [__DataBreakPoints__](Samples/System/DataBreakPoints) | This sample shows how to create hardware data breakpoints that are useful for detecting different types of memory access on Xbox. | ✓ | ✓ | 
| [__DeepLinking__](Samples/System/DeepLinking) | This sample shows how a GDK title can receive a protocol (URI) activation to 'deep link' the player directly into a specific area of the game. | ✓ |  | 
| [__FrontPanelDemo__](Samples/System/FrontPanelDemo) | FrontPanelDemo combines several samples into one executable and then ties together the functionality with a menu system all hosted entirely on the Xbox DevKit front panel. | ✓ |  | 
| [__FrontPanelDolphin__](Samples/System/FrontPanelDolphin) | FrontPanelDolphin demonstrates how to use the GPU to render to the Xbox DevKit FrontPanel. | ✓ |  | 
| [__FrontPanelGame__](Samples/System/FrontPanelGame) | FrontPanelGame is the classic 'snake game' implemented completely on the Xbox DevKit Front Panel. | ✓ |  | 
| [__FrontPanelLogo__](Samples/System/FrontPanelLogo) | This sample provides some starting code to help you render an image the Xbox Devkit front panel display using a standard image format. | ✓ |  | 
| [__FrontPanelText__](Samples/System/FrontPanelText) | The FrontPanelText sample demonstrates how to use the CPU to draw text on the Xbox Devkit Front Panel Display. | ✓ |  | 
| [__GameInputInterfacing__](Samples/System/GameInputInterfacing) | This sample demonstrates how to effectively interface and read inputs from a gamepad, arcade sticks, racing wheels, and more in the Microsoft GDK. | ✓ | ✓ | 
| [__GameInputSequential__](Samples/System/GameInputSequential) | This sample demonstrates how to read inputs sequentially from a gamepad using GameInput. | ✓ | ✓ | 
| [__Gamepad__](Samples/System/Gamepad) | This sample demonstrates gamepad input, motion sensor readings, vibration control, audio-driven haptics, and RGB lighting using GameInput on Xbox and Windows PC. | ✓ | ✓ | 
| [__GamepadKeyboardMouse__](Samples/System/GamepadKeyboardMouse) | This sample demonstrates how to read and process inputs from gamepad, mouse, and keyboard using GameInput. | ✓ | ✓ | 
| [__GameSaveCombo__](Samples/System/GameSaveCombo) | This sample demonstrates the use of the XGameSave APIs to save and load game save data on Xbox. | ✓ | ✓ | 
| [__GameSaveFilesCombo__](Samples/System/GameSaveFilesCombo) | This sample demonstrates the use of the XGameSaveFiles APIs to access folders on Xbox. | ✓ | ✓ | 
| [__IntelligentDelivery__](Samples/System/IntelligentDelivery) | This sample demonstrates Intelligent Delivery APIs. | ✓ | ✓ | 
| [__Lighting__](Samples/System/Lighting) | This sample shows how to use the LampArray API. | ✓ | ✓ | 
| [__LocalStorage__](Samples/System/LocalStorage) | This sample shows how to use different local storage locations in a title for both Xbox and PC platforms. | ✓ | ✓ | 
| [__ModernGamerTag__](Samples/System/ModernGamerTag) | This sample demonstrates the use of a Glyph cache that can be used to store and render strings to a screen on Xbox. | ✓ | ✓ | 
| [__MouseInput__](Samples/System/MouseInput) | This sample demonstrates how to implement mouse controls with GameInput. | ✓ |  | 
| [__MultiExecutablePackage__](Samples/System/MultiExecutablePackage) | This sample demonstrates how to manage multiple projects, dlls, and utility processes within a single solution. | ✓ | ✓ | 
| [__MultiPackage__](Samples/System/MultiPackage) | This sample demonstrates how to manage multiple packages that are capable of launching between each other. | ✓ | ✓ | 
| [__NLSAndLocalization__](Samples/System/NLSAndLocalization) | This sample demonstrates how to localize package manifest as well as in-title resources. It also demonstrate usage of the NLS APIs on Xbox. | ✓ | ✓ | 
| [__PIXCircularCaptureCombo__](Samples/System/PIXCircularCaptureCombo) | This sample shows how to use the PIX[Begin/End]Capture APIs to implement a circular buffer of captures. | ✓ | ✓ | 
| [__SimpleDirectStorageCombo__](Samples/System/SimpleDirectStorageCombo) | Demonstrates several ways to use DirectStorage on Xbox Series X\|S, Desktop, and Xbox One. DirectStorage on Xbox One is handled through an emulation layer | ✓ | ✓ | 
| [__SimpleExceptionHandling__](Samples/System/SimpleExceptionHandling) | Demonstrates various ways that C++ and SEH exceptions can occur in Microsoft GDK titles. | ✓ | ✓ | 
| [__SimpleFFBWheel__](Samples/System/SimpleFFBWheel) | This sample demonstrates how to use a force feedback steering wheel in the Microsoft GDK. | ✓ | ✓ | 
| [__SimpleFrontPanel__](Samples/System/SimpleFrontPanel) | The SimpleFrontPanel sample demonstrates the API covering the basic functionality that you will need to get started programming for the Xbox DevKit Front Panel. | ✓ |  | 
| [__SimpleMultiExePackage__](Samples/System/SimpleMultiExePackage) | This sample demonstrates how to manage multiple projects with executables within a single solution. | ✓ | ✓ | 
| [__SimplePLM__](Samples/System/SimplePLM) | This sample shows the behavior of the Process Lifetime Management (PLM) events on Xbox. | ✓ |  | 
| [__SimpleUserModel__](Samples/System/SimpleUserModel) | This sample shows how to use the new Simple User Model introduced with the Microsoft Game Development Kit (April 2021). | ✓ | ✓ | 
| [__SystemInfo__](Samples/System/SystemInfo) | This sample demonstrates a number of APIs for querying system information and hardware capabilities. | ✓ | ✓ | 
| [__UserManagement__](Samples/System/UserManagement) | This sample demonstrates user management with gamepad association for both single-user and multi-user scenarios on Xbox. | ✓ |  | 
 
<a id="tools"></a> 
 
### Tools 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__BWOIExample__](Samples/Tools/BWOIExample) | This example demonstrates a method for building MSBuild-based projects using the Gaming.*.x64 platforms without having the Microsoft GDK installed. | ✓ | ✓ | 
| [__CMakeExample__](Samples/Tools/CMakeExample) | This is an example of using the CMake cross-platform build system to build an executable with the Microsoft Game Development Kit via the Ninja generator. | ✓ | ✓ | 
| [__CMakeGDKExample__](Samples/Tools/CMakeGDKExample) | This is an example of using the CMake cross-platform build system to build an executable with the Microsoft Game Development Kit via the Visual Studio generator. | ✓ | ✓ | 
| [__CMakeXboxConsoleApp__](Samples/Tools/CMakeXboxConsoleApp) | This is an example of using the CMake cross-platform build system to produce a 'Win32 console' application that can be executed on the Xbox hardware using the Microsoft GDK. | ✓ |  | 
| [__DumpTool__](Samples/Tools/DumpTool) | DumpTool runs in the Xbox Game OS and generates a crash dump for another process that you specify by name as an argument to the tool. | ✓ |  | 
| [__MeshletConverter__](Samples/Tools/MeshletConverter) | The meshlet converter is a command line tool for use on PC. |  | ✓ | 
| [__PlayInputMacro__](Samples/Tools/PlayInputMacro) | This tool allows play back of recorded XBOM macros to a console from the command line. |  | ✓ | 
| [__RemoteIterationToolsSample__](Samples/Tools/RemoteIterationToolsSample) | A WPF sample application demonstrating how to use the Remote Iteration Client API |  | ✓ | 
| [__WPAProfiles__](Samples/Tools/WPAProfiles) | These are WPA profiles for the Windows Performance Analyzer tool. | ✓ | ✓ | 
| [__xbcompress__](Samples/Tools/xbcompress) | This samples demonstrates the Compression API introduced with Windows 8 which is supported for all Gaming.*.x64 platforms. | ✓ | ✓ | 
| [__xbdepends__](Samples/Tools/xbdepends) | This is a command-line for Windows 10 machines intended to help diagnose build and launch issues for GDKX titles. | ✓ | ✓ | 
| [__xbgamepad__](Samples/Tools/xbgamepad) | This testing tool forwards XINPUT gamepad input on PC to a Xbox console. |  | ✓ | 
| [__xtexconv__](Samples/Tools/xtexconv) | This sample is a PC side command-line tool which extends the standard TexConv command-line tool for texture conversion and preparation to support Xbox offline texture tiling for use with the CreatePlacedResourceX APIs in DirectX 12.X. | ✓ | ✓ | 
 
<a id="unity"></a> 
 
### Unity 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__HandheldHelper__](Samples/Unity/System/HandheldHelper/HandheldHelper) | Helper project for UnityHandheldBestPractices. |  | ✓ | 
| [__UnityAchievements__](Samples/Unity/Live/UnityAchievements) | This sample demonstrates using the Title-Managed Achievements C# API provided by the Unity GDK Wrapper. | ✓ | ✓ | 
| [__UnityHandheldBestPractices__](Samples/Unity/System/UnityHandheldBestPractices) | This sample demonstrates the best practices for running a title on handheld devices in Unity. |  | ✓ | 
| [__UnityInGameStore__](Samples/Unity/Live/UnityInGameStore) | This sample demonstrates the client-based operations used in presenting and operating an in-game storefront in Unity. | ✓ | ✓ | 
| [__UnityLeaderboardsTitleManaged__](Samples/Unity/Live/UnityLeaderboardsTitleManaged) | The leaderboards sample demonstrates the usage of Xbox Live Leaderboards with Title-Managed stats in Unity. | ✓ | ✓ | 
| [__UnitySimpleHttpCall__](Samples/Unity/Live/UnitySimpleHttpCall) | This sample demonstrates usage of Xbox Live Http Calls using Unity. |  | ✓ | 
| [__UnitySimpleMPSD__](Samples/Unity/Live/UnitySimpleMPSD) | This sample demonstrates how to use MPSD to implement sessions and matchmaking for Unity games. | ✓ | ✓ | 
| [__UnitySimplePLM__](Samples/Unity/System/UnitySimplePLM) | This sample shows behavior of the Process Lifetime Management (PLM) events on Xbox in Unity. | ✓ | ✓ | 
| [__UnitySimpleWebSockets__](Samples/Unity/Live/UnitySimpleWebSockets) | This sample demonstrates usage of Http Client Web Sockets using the Unity game engine. | ✓ | ✓ | 
| [__UnitySocialManager__](Samples/Unity/Live/UnitySocialManager) | This sample demonstrates usage of Xbox Live Social Manager using the Unity game engine. | ✓ | ✓ | 
 
<a id="xcloud"></a> 
 
### xCloud 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| [__CloudVariableReplacement__](Samples/xCloud/CloudVariableReplacement) | This sample demonstrates how to alter the state of the touch adaptation kit from within a game. | ✓ |  | 
| [__CustomResolution__](Samples/xCloud/CustomResolution) | This sample demonstrates how to change rendering resolution and set streaming resolution to match streaming client characteristics. | ✓ |  | 
| [__SimpleCloudAwareSample__](Samples/xCloud/SimpleCloudAwareSample) | This sample demonstrates how to detect game streaming clients, change onscreen control layouts, and read touch points. | ✓ |  | 
 
