# How to integrate the UITK into a Sample

This "how-to" guide will walk through the basics of standing up the __UITK__ (which stands for User Interface Tool Kit) into an
ATG sample -- VS 2017 or VS 2019 -- solution (referred to from here on simply as "the Sample"), and will cover the following topics:

- Bringing in the UITK shared project into the Sample solution, and referencing it.

- Implementing the basic UI manager boilerplate logic (which would be well served as a part of the ATG template at some point in the future).

- Adding a UI layout to the project as an asset, and loading it within the Sample.

- Finding UI controls by their ID, and attaching appropriate event handlers to those controls to do stuff.

## Dependencies

The __UITK__ project is implemented as a VS shared items project compatible with both VS 2017 & VS 2019.  It can be coded against when referenced into the Sample project, but will fail to compile if these dependencies are not present:

- The __DirectXTK12__ (for DirectXTK12_2022 for VS 2022) project -- typically brought in through the sample template.  The UITK depends on the DescriptorPile, SpriteBatch, SpriteFont, Rectangle, DirectXHelpers, etc.

- The __ATGTK__ shared items project.  The UITK depends on Texture, StringUtil, Json, etc.

Make sure that the dependencies are integrated in beforehand.  The UITK is compatible with Desktop & XboxOne build targets.  Scarlett is untested, but should also be a compatible target.

Also, __very important__, make sure that your sample project is set to use __C++17__ as the language standard as the UITK library code makes use of features from that version.

## Bring the UITK into the Sample
### _Legacy instructions for samples not built from templates_
---
**Important:** If you have started your sample using the **ATG Sample Template** for either VS 2017 or VS 2019, and made sure to check the "**Include the UI Tool Kit**" option in the template UI, then a large portion of the following steps can be _completely ignored_.  In this situation, you may safely skip ahead to the [Adding a UI layout](#adding-a-ui-layout) section.

---
Before doing anything, make sure that the Sample builds and runs.  That way, we can be certain that any build related issues can be pin-pointed to __UITK__ integration issues.

### Step 1: Add the UITK shared items project & ATGTK shared source code

Select the solution name in the _Solution Explorer_, and add an existing project to it like so:

![add-project-to-solution](images/add-project-to-solution.png)

... then pick the ```UITK.vcxitems``` file from within the __*gx_dev/Kits/UITK*__ folder like so:

![open-the-uitk-project](images/open-the-uitk-project.png)

There should now be a project within the _Solution Explorer_ window that looks like:

![solution-explorer-with-uitk](images/solution-explorer-with-uitk.png)

Finally, from within the __*gx_dev/Kits/ATGTK*__ folder, locate the **Texture.cpp/.h** files and the **StringUtil.cpp/.h** files, and make sure they are also added and compiled into the sample project as the UITK depends upon these two classes and bits of their functionality.

### Step 2: Make the Sample depend on the UITK & ATGTK

Now add the __UITK__ project as a reference to the Sample by selecting it as a shared project from within the _Add Reference_ picker:

![add-the-shared-project](images/add-the-shared-project.png)

Likewise should also be true for the __ATGTK__.  Select it as a shared project dependency in the _References_ section of the sample project.

### Step 3: Get the Sample to compile again

Make sure that the include headers listed within the Sample's ```pch.h``` have the ```std::``` namespace and __DirectXTK__ headers:

![includes-in-pch](images/includes-in-pch.png)

```cpp
#define ...

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <assert.h>
#include <stdio.h>

// DirectXTK
#include "CommonStates.h"
#include "DescriptorHeap.h" 
#include "DirectXHelpers.h"
#include "GamePad.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "RenderTargetState.h"
#include "ResourceUploadBatch.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

#ifdef ...

```

Take note of the ```<functional>``` and ```"CommonStates.h"``` header files that are listed there (which may not be automatically added with the ATG template).

At this point, attempt to build the solution again -- which of course will trigger compilation of the __UITK__ source -- and it should succeed.

## Implement basic UI manager boilerplate

In the absence of ATG template integration and application, using the __UITK__ in a sample first requires performing some initialization (and other boilerplate chores).  __*Ultimately, these steps can and will be automated (through ATG sample templating) and made unnecessary.*__

### Step 1: Modify the Sample header to include UITK objects

A typical ATG template sample starts with a header that contains members like the following:

![template-sample-header-members](images/template-sample-header-members.png)

... we first __remove__ the members for specific input devices:

```cpp
// Input devices.
std::unique_ptr<DirectX::GamePad>       m_gamePad;
std::unique_ptr<DirectX::Keyboard>      m_keyboard;
std::unique_ptr<DirectX::Mouse>         m_mouse;

DirectX::GamePad::ButtonStateTracker    m_gamePadButtons;
DirectX::Keyboard::KeyboardStateTracker m_keyboardButtons;
```

... replace those members with an ```ATG::UITK::UIInputState``` and a ```ATG::UITK::UIManager``` by first including the __UITK__ headers for those classes, and then declaring them like so:

```cpp
// UITK members
ATG::UITK::UIManager          m_uiManager;
ATG::UITK::UIInputState       m_uiInputState;
```

Notice that we have the line:

```cpp
using namespace ATG::UITK;
```

after the ```#include``` directives so the compiler recognizes the new UITK types.

Attemping to build the Sample at this point will yield a slew of compile errors for code that is attemping to use members that we just ripped out.

### Step 2: Remove code that no longer compiles

Since we no longer maintain a direct reference to specific input devices, we need to remove their usages from the ```Sample::Initialize()``` method:

![old-initialize](images/old-initialize.png)

... and also from the ```Sample::Update()``` method:

![old-update](images/old-update.png)

... and finally from the ```Sample::OnResuming()``` event handler method:

![old-onresuming](images/old-onresuming.png)

If we then include UIElements.h (which includes some needed UI factory definitions) like so in our Sample header file:

```cpp
#include "UIWidgets.h"
```

We can now build again, albeit with a completely non-functional sample.

### Step 3: Add in UITK boilerplate code

First we augment the ```Sample::Initialize()``` method to include initialization of our input state and UI layout:

```cpp
// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // perform UITK initialization
    m_uiInputState.SetWindow(window);
    InitializeUI();
}

void Sample::InitializeUI()
{
  // TODO: implement me when I have a UI layout...
}
```

... now we can also build back up our ```Sample::Update()``` method to do meaningful things again (like updating input state and the UI):

```cpp
// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your sample logic here.
    elapsedTime;

    // update our UI input state and managed layout

    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    // check if the "view" button was pressed and exit the sample if so

    auto buttons = m_uiInputState.GetGamePadButtons(0);

    if (buttons.view == GamePad::ButtonStateTracker::PRESSED)
    {
        ExitSample();
    }

    // ... also exit if the Sample user presses the [ESC] key

    auto keys = m_uiInputState.GetKeyboardKeys();

    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
    }

    PIXEndEvent();
}
```

... and since we will want our UI layout to be rendered to the screen, we can choose when to render it within the ```Sample::Render()``` method like so:

```cpp
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    commandList;

    // RENDER THE UI SCENE
    m_uiManager.Render();

    PIXEndEvent(commandList);

    // Show the new frame.
    // (code removed for brevity)
}
```

.. the final step for the __cpp__ source file for the Sample is to set up its internal renderer, designating the Sample instance as the provider of the D3D _resources_ that it needs:

```cpp
#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // create the style renderer for the UI manager to use for rendering the UI scene styles
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}
```

In order for us to be able to use the D3D style renderer for the __UITK__, we need to be sure to include its header in the Sample header file:

```cpp
#include "UIManager.h"
#include "UIInputState.h"
#include "UIWidgets.h"

// files needed to initialize the renderer
#include "UIStyleRendererD3D.h"

using namespace ATG::UITK;
```

In order for the D3D style renderer to be given the resources it needs to perform its rendering chores, the renderer needs to be given an interface which provides those resources.  Since we pass the Sample instance itself in the ```Sample::CreateDeviceDependentResources()``` method above, we need to augment the Sample class to be a ```D3DResourcesProvider```:

```cpp
class Sample final : public DX::IDeviceNotify, public D3DResourcesProvider
```

and include the public inlined methods:

```cpp
    // UIStyleManager::D3DResourcesProvider interface methods

    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }
```

Now we should have a building and working Sample again with the __UITK__ successfully integrated into the Sample's code.  The next sections will give guidance on actually making use of the __UITK__.

## Adding a UI layout

The data-driven JSON-based UI layout system employed by the UITK contains many features and a decent amount of complexity to it in order to be flexible enough and support as many scenarios as we foresee the need to support in the years to come.  We will not delve into all of those advanced topics in this document.  Instead this section will provide the basic steps to getting a barebones UI set up that covers some simple and common use cases.

Topics that will be covered:

1. __Creating a simple JSON UI layout file.__<br/>This section will cover the structure of a layout file and some simple layout schema.

1. __Adding the layout to the solution workspace.__<br/>This section will cover how to include the layout file into the Sample's solution project and cover what build settings to set the layout file to in the VS IDE to make it visible to the Sample at runtime.

1. __Getting your layout to display in the sample.__<br/>We wrap this up by loading the layout and ensuring that we get some UI on screen that is responsive to input.

1. __Adding textures & fonts to support the layout.__<br/>This section will cover basic textures in styles and how to include and reference them.

1. __Extending the layout with buttons and a console window.__<br/>This section will build on the layout by adding interactive controls and a means of displaying feedback.

### Creating a simple JSON UI layout file

Before starting to manually code JSON, at the very least it is _highly_ recommended to use a text editor taht has the following features:

- JSON syntax color highlighting
- Smart bracing and indenting of JSON blocks
- Syntax checking
- JSON pretty printing

VS Code provides these features through extensions, as do other text editing environments.

#### A layout with a panel

```json
{
  "layout": {
    "classId": "Panel",
    "id": "Control_Panel",
    "positioningAnchor": [ "right", "top" ],
    "position": [ -100, 100 ],
    "sizingAnchor": [ "right", "top" ],
    "size": [ 400, 800 ],
    "style": {
      "classId": "SpriteStyle",
      "id": "Basic_Panel_Style",
      "colorUsage": "override",
      "color": [ 192, 192, 192, 1.0 ]
    }
  }
}
```

All UI layout files have precisely __one__ ```layout``` element at the root level of the file, and the contents of this JSON node specify what the layout's root UI element is going to be.  A brief description of these properties follows:

| JSON property key | description | default value |
|:---:|---|:---|
|```classId```|The UI class to create for the given properties.|__(none)__ _required_|
|```id```|The ID to use for the element for referencing it.|__(none)__ _required_|
|```positioningAnchor```|The parent's location from which to relatively position its anchor point.|__"top", "left"__|
|```position```|The relative position* of the anchor point -- _positive coordinates are rightward and downward._|__0, 0__|
|```sizingAnchor```|The location of the element from which to size itself.|__"top", "left"__|
|```size```|The size* of the element to render on screen.|__0, 0__|
|```style```|The properties** which define how the element is rendered.|__(none)__ _required_|

__* note:__ positioning and sizing units are in a _reference resolution_ that is HD (or 1920x1080).  final screen pixels are calculated from the application of a global scale which seeks to "best fit" the UI layout on screen so that at least __one__ dimension (either horizontal or vertical) is completely visible.

__** note:__ if a style already exists through a previously defined ```style``` block that was given an ```id``` property, an alternative is to reference that style instead using a ```styleId``` property key like so:

```json
{
  "layout": {
    "classId": "Panel",
    "id": "Another_Panel",
    "styleId": "Basic_Panel_Style"
  }
}
```

#### Important notes regarding JSON keys and values

The JSON __keys__ like ```id```, ```sizingAnchor```, and so forth, are _case sensitive_ and must match the schema defined for those properties.  The JSON __values__ on the other hand are _case insensitive_ and are ultimately converted to lowercase.  In addition, the UITK code ultimately defines the property keys, and what the default values for the properties actually are.

### Adding the layout to the solution workspace

The UI layout file that was created in some JSON friendly text editor will be included an asset file with the rest of the Sample project.  The convention for samples has been to place sample related assets into an __*/Assets*__ subfolder both on the file system and in the VS IDE like so:

![layout-asset-file](images/layout-asset-file.png)

... and on disk:

![layout-file-folder](images/layout-file-folder.png)

Within the properties for the new asset, we have to direct the IDE to copy the file when performing the build:

![layout-copy-file](images/layout-copy-file.png)

... to the /Assets/Layouts/ subdirectory under the build target's deployment root:

![copy-file-destination](images/copy-file-destination.png)

At this point, when a build occurs, one of the logging statements you should see would be the copy file operation to the matching destination:

![copy-file-output](images/copy-file-output.png)

### Getting your layout to display in the sample.

In order to be able to load, display, and update your UI layout file, the first step is to make sure that loaded files are relative to the build target root folder for the app.  For the __XboxOne__ & __Scarlett__, nothing further is required to achieve this as the app's mounted file locations are taken care of by the OS, and the current working directory is mapped to the root directory.

For the __Desktop__ build target, the following lines applied somewhere near the beginning of execution will also ensure that the current working directory points to the executable's root directory:

```cpp
#ifndef _GAMING_XBOX
    // what this chunk of code does is: on Desktop, we establish the correct current working
    // directory from which the sample executes in order for debugging from within
    // VS to work properly when running as a registered store app.
    char dir[_MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, dir, _MAX_PATH) > 0)
    {
        std::string exePath = dir;
        exePath = exePath.substr(0, exePath.find_last_of("\\"));
        std::ignore = SetCurrentDirectoryA(exePath.c_str());
    }
#endif//_GAMING_XBOX
```

_... this would be well served to either go into a utility method, or be a part of the ATG sample template itself at some point in the future._

Having the necessary working directory now set, we can load the layout file using the ```UIManager``` like so:

```cpp
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/sample_layout.json");
```

In the code line above, we are choosing the have the root element (found within the ```layout``` JSON key) be attached as a child of the UI scene root, or screen.

Make sure that the following two lines are present within the ```Sample::Update()``` method:

```cpp
    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);
```

Also make sure that the following line is present within the ```Sample::Render()``` method:

```cpp
    m_uiManager.Render();
```

Building and running the Sample (by pressing the 'F5' key within the VS IDE) shield yield a screen that looks like the following:

![layout-first-render](images/layout-first-render.png)

That non-descript grey rectangle on the right side of the screen is our untextured panel that serves as the root element of the layout we just loaded.

### Adding textures & fonts to support the layout.

Currently, the default font expected to be present by the __UITK__ for static text rendering, is the **SegoeUI** font at __18__ point size with normal, _italic_, and __bold__ style and weight placed within the __*/Assets/Fonts*__ subdirectory.  Text style related properties can override this font to whatever.  If we add this font+size (and its variant) ```.spritefont```s our Sample project would look like:

![texture-and-font-assets](images/texture-and-font-assets.png)

... a `roundy-border.png` texture was also added to demonstrate giving the panel that created a more interesting border:

![roundy-border](example_assets/textures/roundy-border.png)

As an alternative to designating every single new asset file added as a __CopyFile__ type, and setting the destination directory to __*/Assets/(Type-Folder)/$(filename)$(extension)*__, instead a post-build step can be added to ```xcopy``` all asset files:

![xcopy-asset-files](images/xcopy-asset-files.png)

If we go ahead and add the texture to the panel in the ```sample-layout.json``` file:

```json
{
  "layout": {
    "classId": "Panel",
    "id": "Control_Panel",
    "positioningAnchor": [ "right", "top" ],
    "position": [ -100, 100 ],
    "sizingAnchor": [ "right", "top" ],
    "size": [ 400, 800 ],
    "style": {
      "classId": "SpriteStyle",
      "id": "Basic_Panel_Style",
      "colorUsage": "override",
      "color": [ 192, 192, 192, 1.0 ],
      "texture": "Assets/Textures/roundy-border.png"
    }
  }
}
```

... building and running gives us a stretched textured panel rather than the simple colored rectangle:

![stretched-textured-panel](images/stretched-textured-panel.png)

To get a more pixel perfect rounded rectangle, we can indicate to our sprite style to render the sprite as a sliced sprite with slicing UV coordinates, rather than render it as just a plain single quad:

```json
{
  "layout": {
    "classId": "Panel",
    "id": "Control_Panel",
    "positioningAnchor": [ "right", "top" ],
    "position": [ -100, 100 ],
    "sizingAnchor": [ "right", "top" ],
    "size": [ 400, 800 ],
    "style": {
      "classId": "SpriteStyle",
      "id": "Basic_Panel_Style",
      "colorUsage": "override",
      "color": [ 192, 192, 192, 1.0 ],
      "texture": "Assets/Textures/roundy-border.png",
      "spriteType": "nineSliced",
      "innerUVExtents": [ 0.1, 0.1, 0.9, 0.9 ]
    }
  }
}
```

The ```spriteType``` (which defaults to __simple__) is changed to __nineSliced__, and some ```innerUVExtents``` are provided to determine the 9 sliced regions which map to the rendered corners, edges, and center for a cleaner rendered panel that will now look like (when run after saving the layout changes):

![sliced-textured-panel](images/sliced-textured-panel.png)

## Making the UI actually do something

At this point we have a panel rendering and nothing else.  Since the Sample needs to be able to take action based on input, and respond with feedback to the user of the Sample.

### Extending the layout with buttons

The UITK supports the following notions to make somewhat sophisticated-ish sample UIs possible:

- Allow any UI element to be parent of child UI elements
- Allow the composition of more complicated UI controls through the support of lesser, more primitive controls.

This section will demonstrate the application of both of these features with the addition of some UI elements to the layout.  The existing panel will house 3 buttons within it as well as a static text title.

#### Separating out the styles

Since the layout file will rapidly fill up with very verbose JSON rather quickly, one thing that can be done is to define the styles in a separate block from the layout, and reference those styles by their ID (as mentioned previously).  

Since we are going to have a ```Panel```, some ```Button```s, and some ```StaticText``` elements, we will make styles, to support these elements, into a separate JSON block first that looks like the following:

```json
{
  "styles": [
    {
      "classId": "SpriteStyle",
      "id": "Basic_Panel_Style",
      "colorUsage": "override",
      "color": [ 192, 192, 192, 1.0 ],
      "texture": "Assets/Textures/roundy-border.png",
      "spriteType": "nineSliced",
      "innerUVExtents": [ 0.1, 0.1, 0.9, 0.9 ],
      "padding": [ 0, 40, 0, 0 ]
    },
    {
      "classId": "TextStyle",
      "id": "Basic_Label_Style",
      "colorUsage": "override",
      "color": [ 0, 0, 0, 1.0 ],
      "verticalAlign": "middle"
    },
    {
      "classId": "TextStyle",
      "id": "Basic_Title_Style",
      "colorUsage": "override",
      "color": [ 0, 0, 128, 1.0 ],
      "verticalAlign": "middle",
      "horizontalAlign": "center"
    },
    {
      "classId": "SpriteStyle",
      "id": "Basic_Button_Style",
      "inheritsFromId": "Basic_Panel_Style",
      "colorUsage": "setExisting",
      "color": [ 255, 255, 255, 1.0 ],
      "padding": [ 20, 0, 0, 0 ]
    },
    {
      "classId": "SpriteStyle",
      "id": "Focused_Button_Style",
      "inheritsFromId": "Basic_Button_Style",
      "color": [ 0, 255, 255, 1.0 ]
    },
    {
      "classId": "SpriteStyle",
      "id": "Disabled_Button_Style",
      "inheritsFromId": "Basic_Button_Style",
      "color": [ 128, 128, 128, 1.0 ]
    },
    {
      "classId": "SpriteStyle",
      "id": "Pressed_Button_Style",
      "inheritsFromId": "Basic_Button_Style",
      "color": [ 0, 192, 192, 1.0 ]
    },
    {
      "classId": "SpriteStyle",
      "id": "Hovered_Button_Style",
      "inheritsFromId": "Basic_Button_Style",
      "color": [ 128, 255, 128, 1.0 ]
    }
  ],
  "layout": {
    ...
  }
}
```

... when have a style inherit from previously defined styles, only properties that are to be overridden in the extended need be defined, and the remaining properties defer to ancestor styles on up the chain.

In the block above, 5 __button__ sprite styles were defined for the 5 button visual states: _normal_, _focused_, _disabled_, _pressed_, and _hovered_.  Also, we declared 2 __static text__ text styles, one for the button labels, and one for the panel's title.  The __panel__'s sprite style -- which was previously embedded with the panel element properties -- was moved to a defined style.

#### Adding the Buttons and Text

Here are the changes we will make to the layout to have 3 buttons added as ```child element```s to the existing panel:

```json
{
  "styles": [
    ...
  ],
  "layout": {
    "classId": "Panel",
    "id": "Control_Panel",
    "positioningAnchor": [ "right", "top" ],
    "position": [ -100, 100 ],
    "sizingAnchor": [ "right", "top" ],
    "size": [ 400, 800 ],
    "styleId": "Basic_Panel_Style",
    "childElements": [
      {
        "classId": "StaticText",
        "id": "Panel_Title_Label",
        "positioningAnchor": [ "center", "top" ],
        "sizingAnchor": [ "center", "top" ],
        "text": "Panel Title",
        "styleId": "Basic_Title_Style"
      },
      {
        "classId": "Button",
        "id": "Hello_World_Button",
        "positioningAnchor": [ "center", "top" ],
        "position": [ 0, 100 ],
        "sizingAnchor": [ "center", "top" ],
        "size": [ 300, 150 ],
        "styleId": "Basic_Button_Style",
        "focusable": true,
        "disabledStyleId": "Disabled_Button_Style",
        "focusedStyleId": "Focused_Button_Style",
        "hoveredStyleId": "Hovered_Button_Style",
        "pressedStyleId": "Pressed_Button_Style",
        "childElements": [
          {
            "classId": "StaticText",
            "id": "Hello_World_Label",
            "positioningAnchor": [ "left", "middle" ],
            "sizingAnchor": [ "left", "middle" ],
            "text": "Hello World",
            "styleId": "Basic_Label_Style",
            "size": [400, 100]
          }
        ]
      },
      {
        "classId": "Button",
        "id": "Clear_Button",
        "positioningAnchor": [ "center", "top" ],
        "position": [ 0, 300 ],
        "sizingAnchor": [ "center", "top" ],
        "size": [ 300, 150 ],
        "styleId": "Basic_Button_Style",
        "focusable": true,
        "disabledStyleId": "Disabled_Button_Style",
        "focusedStyleId": "Focused_Button_Style",
        "hoveredStyleId": "Hovered_Button_Style",
        "pressedStyleId": "Pressed_Button_Style",
        "childElements": [
          {
            "classId": "StaticText",
            "id": "Clear_Label",
            "positioningAnchor": [ "left", "middle" ],
            "sizingAnchor": [ "left", "middle" ],
            "text": "Clear",
            "styleId": "Basic_Label_Style",
            "size": [400, 100]
          }
        ]
      },
      {
        "classId": "Button",
        "id": "Toggle_Button",
        "positioningAnchor": [ "center", "top" ],
        "position": [ 0, 500 ],
        "sizingAnchor": [ "center", "top" ],
        "size": [ 300, 150 ],
        "styleId": "Basic_Button_Style",
        "focusable": true,
        "disabledStyleId": "Disabled_Button_Style",
        "focusedStyleId": "Focused_Button_Style",
        "hoveredStyleId": "Hovered_Button_Style",
        "pressedStyleId": "Pressed_Button_Style",
        "childElements": [
          {
            "classId": "StaticText",
            "id": "Toggle_Label",
            "positioningAnchor": [ "left", "middle" ],
            "sizingAnchor": [ "left", "middle" ],
            "text": "Toggle",
            "styleId": "Basic_Label_Style",
            "size": [400, 100]
          }
        ]
      }
    ]
  }
}
```

The panel element is still there as the root element, but now there is a static text element serving as the title, as well as 3 button elements -- each of which has a static text element to serve as the button's label.  All of the elements are referencing their styles through the ```styleId``` property.

Upon executing the Sample once again, after the new layout data has been added and saved to the sample layout JSON asset file, the screen will look like:

![final-layout-rendering](images/final-layout-rendering.png)

The reason that the "Hello World" button has a cyan color over it is because that is the color defined for the _focused_ state for buttons, and the __UITK__ chose to have that button be the starting focused element.  Elements are made __focusable__ by means of the ```focusable``` element property, which for the buttons was set to ```true```.

### Attaching event handlers to UI controls

It is time to have the Sample do something with this UI layout.  As it stands, when executing the Sample, the gamepad, keyboard, and mouse can modify the current focused UI element, as well as press the buttons.  Without any event handling though, those button clicks will do nothing.

We introduce a new method in the Sample (after the ```::Initialize()``` method):

```cpp
    // Initialization and management
    void Initialize(HWND window, int width, int height);
    void RegisterUIEventHandlers();
```
just to make it clear where we are registering callbacks on the UI element events that we want to take action on.  For the 3 buttons that we defined in the layout JSON, we add handlers (called __listeners__) to those elements' states, which trigger when a particular state (```Pressed```) happens:

```cpp
void Sample::RegisterUIEventHandlers()
{
    auto helloWorldButton = m_uiManager.FindTypedById<UIButton>(ID("Hello_World_Button"));
    if (helloWorldButton)
    {
        helloWorldButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, 
            [this](UIButton*) {
                this->m_uiManager.FindTypedById<UIStaticText>(ID("Panel_Title_Label"))->
                    SetDisplayText(u8"Hello World!");
            });
    }

    auto clearButton = m_uiManager.FindTypedById<UIButton>(ID("Clear_Button"));
    if (clearButton)
    {
        clearButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*) {
                this->m_uiManager.FindTypedById<UIStaticText>(ID("Panel_Title_Label"))->
                    SetDisplayText(u8"[Cleared]");
            });
    }

    auto toggleButton = m_uiManager.FindTypedById<UIButton>(ID("Toggle_Button"));
    if (toggleButton)
    {
        toggleButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [clearButton](UIButton*) {
                clearButton->SetEnabled(!clearButton->GetEnabled());
            });
    }
}
```

All of the display text used by the __UITK__ is expected to be __UTF8__ encoded.  The identifiers (using the ```id``` property key) are used in code by virtue of the ```ID``` strongly declared type.  

In the 3 event handlers registered in the above code, we listen for __when__ the buttons enter into the _pressed_ state and then provide the lamda that we wish to execute.  The ```AddListener*()``` methods return a __Handle__ that can be used to unregister the lambda at a future point in time.


## Using GlyphCache for Unicode rendering 

GlyphCache class in ATGTK supports Unicode rendering. By using the GlyphCache class, UITK can render Unicode characters in a layout. GlyphCache is already integrated into UITK, but by default it is disabled.
If you want to render Unicode characters in UITK, follow the steps below to enable GlyphCache in UITK.

![glyphcache_unicode](images/glyphcache_unicode.png)


### Step 1: Add GlyphCache and its dependencies

From within the __*gx_dev/Kits/ATGTK/UnicodeRendering*__ folder, locate the **GlyphCache.cpp/.h** files and the **ATGColors.h** files, and add these files into the project. Make sure that GlyphCache.h is in include paths. GlyphCache also requires the StringUtil files, but as you set up UITK, the files are already in the project.

GlyphCache depends on two open-source libraries, FreeType2 and HarfBuzz. The two open-source libraries are in the __*gx_dev/Kits/OpenSource*__ folder. Include all the headers and libraries into the project. Also, make sure that each header is in include paths. It is best to setup filters matching the library hierarchies in the _Solution Explorer_. 

FreeType2 would look like: 

![glyphcache_freetype](images/glyphcache_freetype.png)

and HarfBuzz would look like: 

![glyphcache_harfbuzz](images/glyphcache_harfbuzz.png)

Ensure to update the lib properties to only be included for the proper build configurations. Release libs should be used for Profile and Release, and Debug libs should be used for Debug:

![glyphcache_libraries](images/glyphcache_libraries.png)

You can also choose to use Release libs for Debug for better performance.

### Step 2: Add Noto Fonts

Currently, only Noto Fonts are supported in UITK with GlyphCache. From within the __*gx_dev/Media/Fonts/OFL*__ folder, include all the Noto font files into the project. Do not copy the font files into the project as a local copy.

![glyphcache_fonts](images/glyphcache_fonts.png)

Then, mark the font files as **Yes** in Content property and select **Copy file** as Item Type in their properties:

![glyphcache_fonts_property_type](images/glyphcache_fonts_property_type.png)

Here, in Copy File tab of the property, specify **$(OutDir)Assets\Fonts** in Destination Directories. With this, the font file will be copied into the __*Loose\Assets\Fonts*__ during the build. For using GlyphCache in UITK, the font files must be in the folder.

![glyphcache_fonts_property_directory](images/glyphcache_fonts_property_directory.png)

For legend, sprite font will be still used. If XboxOneControllerLegendSmall.spritefont is not in the project, ensure to include the sprite font to the project in the same way. Also, if you are planning to use GlyphCache partially in a layout, which is recommended for performance, keep including other sprite fonts, too.

### Step 3: Enable GlyphCache in UITK with a macro

Finally, add ```#define UITK_ENABLE_FREETYPE``` in the precompiled header. This will enable GlyphCache feature in UITK.

### Using Unicode rendering for specific TextStyle

When GlyphCache is enabled in UITK, you can specify which TextStyle to use GlyphCache rendering in json layout files with ```fontType``` property.
When ```"fontType": "freetype"``` is specified in TextStyle, the TextStyle will be rendered with GlyphCache. If ```"fontType": "sprite"```  is specified or no ```fontType``` is specified, the TextStyle will be rendered with sprite font. You can still specify legend sprite font in ```legendFont``` while FreeType is being specified as fontType. If legendFont isn't specified, it will fallback to XboxOneControllerLegendSmall.

```json
{
  "classId": "TextStyle",
  "id": "Unicode_Label_Style",
  "colorUsage": "override",
  "color": [ 0, 0, 0, 1.0 ],
  "fontType": "freetype",
  "size": 64,
  "legendFont": "Assets/Fonts/XboxOneControllerLegend",
  "verticalAlign": "middle"
}
```
Due to performance impact, currently it is recommended that GlyphCache should be used in some part of UIs, not in entire UIs.

There are some limitations when using GlyphCache in UITK:
- ```font``` and ```weight``` property in TextStyle will be ignored, and regular Noto fonts will be used in every case.
- Legends are rendered with sprite fonts.
- OpenSource libraries for GlyphCache give LNK4099 error. This can be supressed via ```/ignore:4099```.

