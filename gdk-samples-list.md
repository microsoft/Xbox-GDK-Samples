# Microsoft Game Development Kit samples list

## Categories

Following is a list of categories for the samples.

- [Microsoft Game Development Kit samples](#microsoft-game-development-kit-samples-list) 
  - [Categories](#categories)
    - [Audio](#audio)
    - [Graphics](#graphics)
    - [IntroGraphics](#intrographics)
    - [Live](#live)
    - [System](#system)
    - [Tools](#tools)
    - [xCloud](#xcloud)

<a id="audio"></a> 
 
### Audio 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __AdvancedSpatialSounds__ | This sample demonstrates how use ISpatialAudioClient to playback both static and dynamic positional audio using Windows Sonic technologies in an Xbox title. | ✓ |  | 
| __InGameChat__ | The InGameChat sample provides a working example of integrating the GameChat2 library into an Xbox title. It brings together the pieces needed to demonstrate in-title VOIP communications: GameChat, Multiplayer Sessions, and Peer Networking. | ✓ | ✓ | 
| __SimpleCustomAPO__ | This sample demonstrates how to play a wav file using XAudio2 on the Xbox using a custom xAPO effect. | ✓ |  | 
| __SimplePlay3DSound__ | This sample demonstrates how use XAudio2 and X3DAudio to playback positional audio on Xbox. | ✓ |  | 
| __SimplePlaySound__ | This sample demonstrates how to play a wav file using XAudio2 on the Xbox. | ✓ |  | 
| __SimplePlaySoundStream__ | This sample demonstrates how to stream a wav file using XAudio2 on the Xbox. | ✓ |  | 
| __SimpleSpatialPlaySound__ | This sample demonstrates how use ISpatialAudioClient to playback static audio with height channels using Windows Sonic technologies in an Xbox title. | ✓ |  | 
| __SimpleWASAPICapture__ | This sample demonstrates how to capture audio using WASAPI on Xbox. | ✓ |  | 
| __SimpleWASAPIPlaySound__ | This sample demonstrates how to play setup and play a simple sound (sine tone) to a WASAPI render endpoint on Xbox. | ✓ |  | 
 
<a id="graphics"></a> 
 
### Graphics 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __AdvancedESRAM__ | This sample demonstrates the use of advanced DirectX 12.x memory features on Xbox One to effectively alias memory for Direct3D resources. | ✓ |  | 
| __AdvancedLighting__ | This sample demonstrates different techniques to improve performance of deferred rendering with many lights. | ✓ | ✓ | 
| __AMDFidelityFX_CACAO__ | This sample demonstrates AMD's FidelityFX Combined Adaptive Compute Ambient Occlusion (CACAO) algorithm. | ✓ | ✓ | 
| __AMDFidelityFX_CAS__ | This sample demonstrates AMD's FidelityFX Contrast Adaptive Sharpening (CAS) algorithm. | ✓ | ✓ | 
| __AMDFidelityFX_FSR3FrameInterpolation__ | This sample demonstrates AMD's FidelityFX FSR3 algorithm. | ✓ | ✓ | 
| __AMDFidelityFX_RTShadowDenoiser__ | This sample shows an implementation of the AMD's FidelityFX RT Shadow Denoiser applied against a basic implementation of DXR 1.0 Raytraced Shadows. | ✓ | ✓ | 
| __AMDFidelityFX_SuperResolution__ | This sample demonstrates AMD's FidelityFX Super Resolution (FSR) algorithm. | ✓ | ✓ | 
| __AMDFidelityFX_VariableShading__ | This sample demonstrates AMD's FidelityFX Variable Shading algorithm. | ✓ | ✓ | 
| __Antialiasing__ | This sample shows different antialiasing methods (SMAA, SMAA2x, and FXAA) on Xbox. | ✓ |  | 
| __AutoRGB__ | This sample demonstrates how to extract a single, representative ambient color from a given scene and use it to light up lamps from connected devices (HID). Uses the LampArray api. | ✓ |  | 
| __ComputeParticles__ | This sample demonstrates how to use compute shaders and append buffers to perform a basic particle simulation and performantly render a silly number of particles. | ✓ |  | 
| __DeferredParticles__ | This DirectX 12 sample demonstrates a method for rendering lit smoke particles in either a forward or deferred fashion. | ✓ |  | 
| __DirectStorageTextureShuffling__ | This sample demonstrates how to use DirectStorage swizzle modes together with shuffling (deinterleaving) the data of BC1, BC3, BC4 and BC5 textures in specific ways to improve ZLib compression.The sample consists of an offline tool to prepare the texture data, and runtime code with compute shaders to unshuffle the texture data. | ✓ | ✓ | 
| __DXRSimpleLighting__ | This sample demonstrates how diffuse lighting and animated cubes can be rendered with the DirectX Raytracing API on a Xbox Series X\|S device. | ✓ |  | 
| __DXRTriangle__ | This sample demonstrates the basic use of the DirectX Raytracing API on Xbox Series X\|S and PC. | ✓ | ✓ | 
| __DynamicCubeMap__ | This sample demonstrates how to render the scene to a cubemap at runtime and then sample from the cubemap. | ✓ |  | 
| __DynamicLOD__ | This sample demonstrates how to leverage amplification shaders to do per-instance frustum culling and mesh level-of-detail (LOD) selection entirely on the GPU for an arbitrary number of instances on Xbox Series X\|S and PC. | ✓ | ✓ | 
| __ExecuteIndirect__ | This sample demonstrates usage of DirectX 12's ExecuteIndirect API for asynchronously building rendering commands. | ✓ |  | 
| __FastBlockCompress__ | This sample demonstrates how you can use DirectCompute to perform fast texture compression at run time to the BC1, BC3, and BC5 formats based on the classic Fast Block Compression algorithm on Xbox. | ✓ |  | 
| __GeometricExpansion__ | This sample demonstrates the geometric expansion capabilities of mesh shaders for Xbox Series X\|S and PC. | ✓ | ✓ | 
| __HDR10__ | Switch a UHD TV into HDR mode and render a HDR10 scene with values higher than 1.0f, which will be displayed as brighter than white on a UHD TV using DirectX 12 on Xbox. | ✓ |  | 
| __HDRCalibration__ | This sample switches an HDR TV to HDR mode and then presents the user with several calibration options to adjust the visible detail in brights and darks, the overall brightness and color saturation of the image. | ✓ |  | 
| __HDRDisplayMapping__ | This sample shows that even when rendering a HDR scene on a HDR capable TV, some tone mapping is still needed, referred to as 'HDR display mapping'. | ✓ |  | 
| __HDRPrecision__ | This sample demonstrates how precision and GPU performance are affected when using different formats and color spaces while rendering HDR on Xbox Series X\|S. | ✓ |  | 
| __HistogramCS__ | This sample demonstrates some performance considerations for Compute Shaders on Xbox. | ✓ |  | 
| __HlslCompile__ | The sample compiles the same pixel shader in a number of different ways, to illustrate different options for PC-side asset build. | ✓ |  | 
| __MeshletCull__ | This sample demonstrates how to leverage amplification shaders to cull meshlets against the camera using per-meshlet culling metadata on Xbox Series X\|S and PC. | ✓ | ✓ | 
| __MeshletInstancing__ | Demonstrates GPU instancing for DirectX 12 Mesh Shaders. | ✓ | ✓ | 
| __MP4Reader__ | This sample shows how the Media Foundation Source Reader can be used to read a MP4 file which contains an H264 or HEVC video stream, and decode it using hardware acceleration on Xbox. | ✓ |  | 
| __PipelinedPostprocess__ | This sample demonstrates how to overlap the postprocess phase of one frame with the beginning of the next frame, potentially achieving some performance benefit from parallelization. | ✓ |  | 
| __PointSprites__ | Demonstrates ten methods of rendering point sprites in DirectX 12. | ✓ |  | 
| __ShaderReflect__ | This sample demonstrates how you can access shader reflection information and how it relates to the root signature. | ✓ |  | 
| __Simple120Hz__ | This sample demonstrates the basics of rendering at several refresh rates on Xbox: 30Hz, 40Hz, 60Hz, and 120Hz. | ✓ |  | 
| __SimpleHDR__ | This is a simple sample showing how to implement HDR on Xbox. | ✓ |  | 
| __SimpleMeshlet__ | This sample introduces the meshlet data structure and provides an example of rendering using meshlets on Xbox Series X\|S and PC. | ✓ | ✓ | 
| __SimplePBR__ | This sample demonstrates physically based rendering (PBR) on Xbox using DirectX 12. | ✓ | ✓ | 
| __SmokeSimulation__ | This sample demonstrates how to use Compute Shader 6.0 and 3D textures to implement basic 3D Navier-Stokes flow simulation. | ✓ |  | 
| __TemporalAntialiasing__ | This sample demonstrates how to implement Temporal Antialiasing (TAA), plus a series of other techniques which help improve the algoritm stability and fix some of its inherent issues. | ✓ | ✓ | 
| __VisibilityBuffer__ | Demonstrates a visibility buffer (deferred) rendering technique, making use of Mesh Shaders and HLSL 6.6 Dynamic Resources. | ✓ | ✓ | 
 
<a id="intrographics"></a> 
 
### IntroGraphics 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __SimpleBezier__ | This sample demonstrates how to create hull and domain shaders to draw a tessellated Bezier surface representing a Mobius strip for DirectX 12 on Xbox. | ✓ |  | 
| __SimpleCompute__ | SimpleCompute shows how to use DirectCompute (i.e. Direct3D Compute Shader) for DirectX 12 on Xbox. | ✓ |  | 
| __SimpleDeviceAndSwapChain__ | This sample demonstrates how to create a Direct3D 12 device and swap chain for an GameCore on Xbox title. | ✓ |  | 
| __SimpleDynamicResources__ | Demonstrates how to use HLSL Dynamic Resources in HLSL Shader Model 6.6. This sample is functionally identical to SimpleTexture, except resources are accessed directly through the heap using ResourceDescriptorHeap[] and SamplerDescriptorHeap[] in HLSL. | ✓ | ✓ | 
| __SimpleInstancing__ | This sample demonstrates how to use instancing with the Direct3D 12 API on Xbox. | ✓ |  | 
| __SimpleLighting__ | This sample demonstrates how to create a static Direct3D 12 vertex, index, and constant buffer to draw indexed geometry lit by using static and dynamic Lambertian lighting on Xbox. | ✓ |  | 
| __SimpleMeshShader__ | This sample is a companion to the SimpleTriangle sample with the exception being that it uses Directx 12 Mesh Shaders. | ✓ | ✓ | 
| __SimpleMSAA__ | This sample implements an MSAA render target and depth/stencil buffer for a 3D scene using DirectX 12 on Xbox. | ✓ |  | 
| __SimpleSamplerFeedback__ | Simple example of using DirectX 12 Sampler Feedback. | ✓ |  | 
| __SimpleTexture__ | This sample implements an MSAA render target and depth/stencil buffer for a 3D scene using DirectX 12 on Xbox. | ✓ |  | 
| __SimpleTriangle__ | This sample demonstrates how to create a static Direct3D 12 vertex buffer to render a triangle on screen on Xbox. | ✓ |  | 
| __SimpleTriangleDesktop__ | This sample demonstrates how to create a static Direct3D 12 vertex buffer to render a triangle on screen. |  | ✓ | 
 
<a id="live"></a> 
 
### Live 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __Achievements__ | This sample demonstrates using the Title-Managed Achievements C-API provided by the Microsoft Game Development Kit. | ✓ | ✓ | 
| __DownloadableContent__ | This sample demonstrates how to implement enumeration and loading of downloadable content for Xbox LIVE services. | ✓ | ✓ | 
| __Fundamentals_Desktop__ | This sample demonstrates signing into Xbox Live and making a license check to ensure that the game is owned by the currently signed in user on PC. |  | ✓ | 
| __GameHub__ | This sample demonstrates how to implement Franchise Game Hub. | ✓ | ✓ | 
| __InGameStore__ | This sample demonstrates the client-based operations used in presenting and operating an in-game storefront. | ✓ | ✓ | 
| __LeaderboardsEventBased__ | This sample demonstrates the usage of Xbox Live Leaderboards with Events-Based stats. | ✓ | ✓ | 
| __LeaderboardsTitleManaged__ | The leaderboards sample demonstrates the usage of Xbox Live Leaderboards with Title-Managed stats. | ✓ | ✓ | 
| __mDNS__ | This sample demonstrates using mDNS to register a game service and broadcasting it across your local network, as well as demonstrating network discovery and resolving on Xbox. | ✓ | ✓ | 
| __MicrosoftStoreServicesClient__ | This sample works with the Microsoft.StoreServices Sample and demonstrates the client side call pattern to do service-to-service auth with the Store Services | ✓ | ✓ | 
| __PlayFabLeaderboards__ | This sample displays the data of the leaderboard registered in PlayFab Leaderboards (v2). As an option, a tool is provided to create PlayFab Leaderboards and write any number of player entities and scores to them. | ✓ | ✓ | 
| __PlayFabStore__ | This sample demonstrates the client-based operations used in presenting and operating an in-game economy with PlayFab Economy v2. | ✓ | ✓ | 
| __SimpleCrossGenMPSD__ | This sample demonstrates how to use MPSD to implement sessions and matchmaking for both cross generation and single generation games. | ✓ | ✓ | 
| __SimpleHttp__ | This sample demonstrates using XCurl to make HTTP requests including adding the user token and signature to the headers for authenticated Xbox LIVE calls. | ✓ | ✓ | 
| __SimpleMPA__ | This sample demonstrates how to use the Multiplayer Activity Service for activities and invites. | ✓ | ✓ | 
| __SimpleWebSockets__ | This sample demonstrates using LibHttpClient to connect, send, and receive messages to/from a host via Web Sockets. | ✓ | ✓ | 
| __SocialManager__ | This sample demonstrates the Social Manager C-API provided by the Microsoft Gaming SDK (GDK). | ✓ | ✓ | 
| __TitleStorage__ | This sample demonstrates Title Storage API provided by the Microsoft Game Development Kit (GDK). | ✓ | ✓ | 
 
<a id="system"></a> 
 
### System 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __AccessibilitySample__ | This sample demonstrates how to implement cross-platform accessibility features for a title. | ✓ | ✓ | 
| __AdvancedExceptionHandling__ | Demonstrates various ways that C++ and SEH exceptions can occur in Microsoft GDK titles including advanced scenarios. | ✓ | ✓ | 
| __AsynchronousProgramming__ | This sample shows how to use XAsync, XTaskQueue, and XAsyncProvider to implement asynchronous programming and task handling in various ways. | ✓ | ✓ | 
| __Collision__ | This sample demonstrates DirectXMath's collision types for simple bounding volume tests. | ✓ | ✓ | 
| __CustomEventProvider__ | This sample demonstrates how to use custom ETW event providers on Xbox. | ✓ |  | 
| __DataBreakPoints__ | This sample shows how to create hardware data breakpoints that are useful for detecting different types of memory access on Xbox. | ✓ | ✓ | 
| __FrontPanelDemo__ | FrontPanelDemo combines several samples into one executable and then ties together the functionality with a menu system all hosted entirely on the Xbox DevKit front panel. | ✓ |  | 
| __FrontPanelDolphin__ | FrontPanelDolphin demonstrates how to use the GPU to render to the Xbox DevKit FrontPanel. | ✓ |  | 
| __FrontPanelGame__ | FrontPanelGame is the classic 'snake game' implemented completely on the Xbox DevKit Front Panel. | ✓ |  | 
| __FrontPanelLogo__ | This sample provides some starting code to help you render an image the Xbox Devkit front panel display using a standard image format. | ✓ |  | 
| __FrontPanelText__ | The FrontPanelText sample demonstrates how to use the CPU to draw text on the Xbox Devkit Front Panel Display. | ✓ |  | 
| __GameInputInterfacing__ | This sample demonstrates how to effectively interface and read inputs from a gamepad, arcade sticks, racing wheels, and more in the Microsoft GDK. | ✓ | ✓ | 
| __GameInputSequential__ | This sample demonstrates how to read inputs sequentially from a gamepad using GameInput. | ✓ | ✓ | 
| __Gamepad__ | This sample demonstrates how to read inputs from a gamepad on Xbox. | ✓ | ✓ | 
| __GamepadKeyboardMouse__ | This sample demonstrates how to read and process inputs from gamepad, mouse, and keyboard using GameInput. | ✓ | ✓ | 
| __GamepadVibration__ | This sample demonstrates how to use vibration with a gamepad on an Xbox. | ✓ | ✓ | 
| __GameSaveCombo__ | This sample demonstrates the use of the XGameSave APIs to save and load game save data on Xbox. | ✓ | ✓ | 
| __GameSaveFilesCombo__ | This sample demonstrates the use of the XGameSaveFiles APIs to access folders on Xbox. | ✓ | ✓ | 
| __IntelligentDelivery__ | This sample demonstrates Intelligent Delivery APIs. | ✓ | ✓ | 
| __Lighting__ | This sample shows how to use the LampArray API. | ✓ | ✓ | 
| __LocalStorage__ | This sample shows how to use different local storage locations in a title for both Xbox and PC platforms. | ✓ | ✓ | 
| __ModernGamertag__ | This sample demonstrates the use of a Glyph cache that can be used to store and render strings to a screen on Xbox. | ✓ | ✓ | 
| __MouseInput__ | This sample demonstrates how to implement mouse controls with GameInput. | ✓ |  | 
| __MultiExecutablePackage__ | This sample demonstrates how to manage multiple projects, dlls, and utility processes within a single solution. | ✓ | ✓ | 
| __MultiPackage__ | This sample demonstrates how to manage multiple packages that are capable of launching between each other. | ✓ | ✓ | 
| __NLSAndLocalization__ | This sample demonstrates how to localize package manifest as well as in-title resources. It also demonstrate usage of the NLS APIs on Xbox. | ✓ | ✓ | 
| __PIXCircularCaptureCombo__ | This sample shows how to use the PIX[Begin/End]Capture APIs to implement a circular buffer of captures. | ✓ | ✓ | 
| __SimpleDirectStorageCombo__ | Demonstrates several ways to use DirectStorage on Xbox Series X\|S, Desktop, and Xbox One. DirectStorage on Xbox One is handled through an emulation layer | ✓ | ✓ | 
| __SimpleExceptionHandling__ | Demonstrates various ways that C++ and SEH exceptions can occur in Microsoft GDK titles. | ✓ | ✓ | 
| __SimpleFFBWheel__ | This sample demonstrates how to use a force feedback steering wheel in the Microsoft GDK. | ✓ | ✓ | 
| __SimpleFrontPanel__ | The SimpleFrontPanel sample demonstrates the API covering the basic functionality that you will need to get started programming for the Xbox DevKit Front Panel. | ✓ |  | 
| __SimpleMultiExePackage__ | This sample demonstrates how to manage multiple projects with executables within a single solution. | ✓ | ✓ | 
| __SimplePLM__ | This sample shows the behavior of the Process Lifetime Management (PLM) events on Xbox. | ✓ |  | 
| __SimpleUserModel__ | This sample shows how to use the new Simple User Model introduced with the Microsoft Game Development Kit (April 2021). | ✓ | ✓ | 
| __SystemInfo__ | This sample demonstrates a number of APIs for querying system information and hardware capabilities. | ✓ | ✓ | 
| __UserManagement__ | This sample demonstrates user management with gamepad association for both single-user and multi-user scenarios on Xbox. | ✓ |  | 
 
<a id="tools"></a> 
 
### Tools 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __BWOIExample__ | This example demonstrates a method for building MSBuild-based projects using the Gaming.*.x64 platforms without having the Microsoft GDK installed. | ✓ | ✓ | 
| __CacheTestCombo__ | This sample works with the Cross-Core Memory Cost entry in the Microsoft GDK documentation. | ✓ | ✓ | 
| __CMakeExample__ | This is an example of using the CMake cross-platform build system to build an executable with the Microsoft Game Development Kit via the Ninja generator. | ✓ | ✓ | 
| __CMakeGDKExample__ | This is an example of using the CMake cross-platform build system to build an executable with the Microsoft Game Development Kit via the Visual Studio generator. | ✓ | ✓ | 
| __CMakeXboxConsoleApp__ | This is an example of using the CMake cross-platform build system to produce a 'Win32 console' application that can be executed on the Xbox hardware using the Microsoft GDK. | ✓ |  | 
| __DumpTool__ | DumpTool runs in the Xbox Game OS and generates a crash dump for another process that you specify by name as an argument to the tool. | ✓ |  | 
| __MeshletConverter__ | The meshlet converter is a command line tool for use on PC. |  | ✓ | 
| __OSPrimitiveTestCombo__ | This sample works with the Costs for Synchronization Primitives entry in the Microsoft GDK documentation. | ✓ | ✓ | 
| __PlayInputMacro__ | This tool allows play back of recorded XBOM macros to a console from the command line. |  | ✓ | 
| __WPAProfiles__ | These are WPA profiles for the Windows Performance Analyzer tool. | ✓ | ✓ | 
| __xbcompress__ | This samples demonstrates the Compression API introduced with Windows 8 which is supported for all Gaming.*.x64 platforms. | ✓ | ✓ | 
| __xbdepends__ | This is a command-line for Windows 10 machines intended to help diagnose build and launch issues for GDKX titles. | ✓ | ✓ | 
| __xbgamepad__ | This testing tool forwards XINPUT gamepad input on PC to a Xbox console. |  | ✓ | 
| __xtexconv__ | This sample is a PC side command-line tool which extends the standard TexConv command-line tool for texture conversion and preparation to support Xbox offline texture tiling for use with the CreatePlacedResourceX APIs in DirectX 12.X. | ✓ | ✓ | 
 
<a id="xcloud"></a> 
 
### xCloud 
 
| Sample | Description | Console | PC | 
| ------ | ----------- | ------- | -- | 
| __CloudVariableReplacement__ | This sample demonstrates how to alter the state of the touch adaptation kit from within a game. | ✓ |  | 
| __CustomResolution__ | This sample demonstrates how to change rendering resolution and set streaming resolution to match streaming client characteristics. | ✓ |  | 
| __SimpleCloudAwareSample__ | This sample demonstrates how to detect game streaming clients, change onscreen control layouts, and read touch points. | ✓ |  | 
 
