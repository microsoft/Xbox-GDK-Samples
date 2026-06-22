//--------------------------------------------------------------------------------------
// UI.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UI.h"
#include "StringUtil.h"

using namespace ATG;
using namespace UITK;

UI::UI()
    : m_state(UIState::Browser)
    , m_browserElementCount(0)
    , m_browserStartOffset(0)
    , m_logMaxBufferHistory(200)
    , m_logRenderIndex(0)
    , m_logAutoScroll(true)
{
    DX::ThrowIfFailed(XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue));
}

UI::~UI()
{
    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 5000);
    XTaskQueueCloseHandle(m_taskQueue);
}

#pragma warning(push)
#pragma warning(disable : 26444) // Ignore static analysis warning that AddChildFromPrefab isn't using the return value
void UI::Initialize(D3DResourcesProvider& resourcesProvider, int sizeX, int sizeY, bool jumpToRuntimeScreen, Callbacks callbacks)
{
    // Store callbacks
    m_callbacks = callbacks;

    // Initialize UITK Manager
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(resourcesProvider, sizeX, sizeY);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
    m_uiManager.SetWindowSize(sizeX, sizeY);

    m_launcherCache = std::make_unique<LauncherCache>("d:\\DevkitToolLauncherParameterCache.json");
    m_launcherCache->Load();
    
    m_browserLayout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/BrowserLayout.json");
    m_launcherLayout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/LauncherLayout.json");
    m_runtimeLayout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/RuntimeLayout.json");
    
    m_uiManager.AttachTo(m_browserLayout, m_uiManager.GetRootElement());
    m_uiManager.AttachTo(m_launcherLayout, m_uiManager.GetRootElement());
    m_uiManager.AttachTo(m_runtimeLayout, m_uiManager.GetRootElement());
    
    // Browser Setup
    m_browserVerticalStack = m_uiManager.FindTypedById<UIStackPanel>(ID("BrowserVerticalStack"));
    const Vector2 verticalStackSize = m_browserVerticalStack->GetSizeInRefUnits();
    m_browserVerticalStack->AddChildFromPrefab("Assets/Layouts/BrowserItemPrefab.json");
    auto firstBrowserChild = m_browserVerticalStack->GetChildByIndex(0);
    firstBrowserChild->SetStyleId(ID("focused_browser_item_style"));
    const int browserItemHeight = int(firstBrowserChild->GetSizeInRefUnits().y + 0.5f);
    const int numBrowserItems = int(verticalStackSize.y + 0.5f) / browserItemHeight;
    for (int index = 1; index < numBrowserItems; ++index)
    {
        m_browserVerticalStack->AddChildFromPrefab("Assets/Layouts/BrowserItemPrefab.json");
    }
    m_browserElementCount = static_cast<int>(m_browserVerticalStack->GetChildCount());
    
    // Launcher Setup
    auto exePrefab = m_uiManager.FindById(ID("LaunchSettingsExeTextEntry"));
    m_launcherExeEntryPanel = exePrefab->GetSubElementById(ID("LaunchTextEntryBorderPanel"));
    m_launcherExeEntryText = m_launcherExeEntryPanel->GetTypedSubElementById<UIStaticText>(ID("LaunchTextEntryStaticText"));
    auto clPrefab = m_uiManager.FindById(ID("LaunchSettingsCommandLineTextEntry"));
    m_launcherCommandLineEntryPanel = clPrefab->GetSubElementById(ID("LaunchTextEntryBorderPanel"));
    m_launcherCommandLineEntryText = m_launcherCommandLineEntryPanel->GetTypedSubElementById<UIStaticText>(ID("LaunchTextEntryStaticText"));
    auto workingDirPrefab = m_uiManager.FindById(ID("LaunchSettingsWorkingDirectoryTextEntry"));
    m_launcherWorkingDirEntryPanel = workingDirPrefab->GetSubElementById(ID("LaunchTextEntryBorderPanel"));
    m_launcherWorkingDirEntryText = m_launcherWorkingDirEntryPanel->GetTypedSubElementById<UIStaticText>(ID("LaunchTextEntryStaticText"));
    auto launchButtonsPanel = m_uiManager.FindById(ID("LaunchButtonsLayoutPanel"));
    m_launcherLaunchCPUOnlyButton = launchButtonsPanel->GetSubElementById(ID("LaunchButtonCPU"));
    m_launcherLaunchCPUAndGPUButton = launchButtonsPanel->GetSubElementById(ID("LaunchButtonGPU"));
    m_launcherCancelButton = launchButtonsPanel->GetSubElementById(ID("CancelButton"));
    
    // Runtime Setup
    m_runtimeExeCommandlineText = m_uiManager.FindTypedById<UIStaticText>(ID("RuntimeExeText"));
    m_runtimeWorkingDirText = m_uiManager.FindTypedById<UIStaticText>(ID("RuntimeWorkingDirectoryText"));
    m_runtimeButtonPromptText = m_uiManager.FindTypedById<UIStaticText>(ID("RuntimeButtonPromptText"));
    auto runtimeLogVerticalStack = m_uiManager.FindById(ID("RuntimeLogVerticalStack"));
    const Vector2 runtimeLogVerticalStackSize = runtimeLogVerticalStack->GetSizeInRefUnits();
    runtimeLogVerticalStack->AddChildFromPrefab("Assets/Layouts/RuntimeLogEntryPrefab.json");
    auto firstRuntimeChild = runtimeLogVerticalStack->GetChildByIndex(0);
    const int runtimeItemHeight = int(firstRuntimeChild->GetSizeInRefUnits().y + 0.5f);
    const int numRuntimeItems = int(runtimeLogVerticalStackSize.y + 0.5f) / runtimeItemHeight;
    for (int index = 1; index < numRuntimeItems; ++index)
    {
        runtimeLogVerticalStack->AddChildFromPrefab("Assets/Layouts/RuntimeLogEntryPrefab.json");
    }
    m_runtimeLogTexts.reserve(static_cast<size_t>(numRuntimeItems));
    for (int index = 0; index < numRuntimeItems; ++index)
    {
        auto childElement = runtimeLogVerticalStack->GetChildByIndex(size_t(index));
        m_runtimeLogTexts.push_back(childElement->GetTypedSubElementById<UIStaticText>(ID("RuntimeLogEntryText")));
    }

    // Pick startup screen
    if (jumpToRuntimeScreen)
    {
        SwitchTo(UIState::Runtime);
        Runtime_Initialize(true);
    }
    else
    {
        SwitchTo(UIState::Browser);
    }

    // Always initialize browser screen to set it up
    Browser_Initialize();
}
#pragma warning(pop)

void UI::Update(float elapsedSeconds)
{
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0);

    switch (m_state)
    {
    case UIState::Browser:
        Browser_Update();
        break;
    case UIState::Launcher:
        Launcher_Update();
        break;
    case UIState::Runtime:
        Runtime_Update();
        break;
    default:
        break;
    }

    // Using blank input state as this class manually handles input events via input commands
    m_uiManager.Update(elapsedSeconds, m_inputState);
}

void UI::Render()
{
    m_uiManager.Render();
}

void UI::HandleInputCommand(UIInputCommand command)
{
    switch (m_state)
    {
    case UIState::Browser:
        Browser_HandleInputCommand(command);
        break;

    case UIState::Launcher:
        Launcher_HandleInputCommand(command);
        break;

    case UIState::Runtime:
        Runtime_HandleInputCommand(command);
        break;

    default:
        break;
    }
}

void UI::Log(const char* text)
{
    std::string textToUse = text;
    if (textToUse.back() == u8'\r')
    {
        textToUse.pop_back();
    }

    OutputDebugStringA(textToUse.c_str());
    OutputDebugStringA("\n");

    if (m_state == UIState::Runtime)
    {
        Runtime_Log(text);
    }
}

void UI::OnSuspending()
{
}

void UI::OnResuming()
{
    m_inputState.Reset();
}

void UI::SwitchTo(UIState state)
{
    const bool showBrowser = (state == UIState::Browser);
    const bool showLauncher = (state == UIState::Launcher);
    const bool showRuntime = (state == UIState::Runtime);
    
    m_state = state;
    m_browserLayout->SetVisible(showBrowser);
    m_browserLayout->SetEnabled(showBrowser);
    m_launcherLayout->SetVisible(showLauncher);
    m_launcherLayout->SetEnabled(showLauncher);
    m_runtimeLayout->SetVisible(showRuntime);
    m_runtimeLayout->SetEnabled(showRuntime);
}

void UI::UITK_NavUp()
{
    auto upElement = m_uiManager.GetUpFocusableElement();
    if (upElement)
    {
        m_uiManager.SetFocus(upElement);
    }
}

void UI::UITK_NavDown()
{
    auto downElement = m_uiManager.GetDownFocusableElement();
    if (downElement)
    {
        m_uiManager.SetFocus(downElement);
    }
}

void UI::UITK_NavLeft()
{
    auto leftElement = m_uiManager.GetLeftFocusableElement();
    if (leftElement)
    {
        m_uiManager.SetFocus(leftElement);
    }
}

void UI::UITK_NavRight()
{
    auto rightElement = m_uiManager.GetRightFocusableElement();
    if (rightElement)
    {
        m_uiManager.SetFocus(rightElement);
    }
}

void UI::Browser_Initialize()
{
    // Setup the browser to scan the "d" drive which is SystemScratch when referenced by the Xbox title
    m_browser = std::make_unique<DirectoryBrowser>("d", ".exe");

    // Expand the root level
    m_browserIter = m_browser->GetIteratorToFirst();
    if (m_browserIter.IsValid())
    {
        assert(m_browserIter.IsDirectory());
        m_browserIter.SetDirectoryExpanded(true);
        m_browserStartOffset = 0;
    }

    // Try to find a directory called "DevkitToolLauncherExampleTools", expand it, and select it to start.
    // The "DeployExampleTools.bat" script creates CPUTool and GPUTool folders at
    // "d:\DevkitToolLauncherExampleTools\[CPU/GPU]Tool"
    const int maxIndex = static_cast<int>(m_browser->GetNumIteratedElements()) - 1;
    while (m_browserIter.GetIndex() != maxIndex)
    {
        if (m_browserIter.IsDirectory() &&
            m_browserIter.GetDepth() == 1 &&
            DX::ToLower(m_browserIter.GetName()).compare("devkittoollauncherexampletools") == 0)
        {
            m_browserIter.SetDirectoryExpanded(true);
            break;
        }
        Browser_NavDown();
    }
    if (m_browserIter.GetIndex() == maxIndex)
    {
        m_browserIter = m_browser->GetIteratorToFirst();
    }

    Browser_UpdateUIData();
}

void UI::Browser_Update()
{
}

void UI::Browser_HandleInputCommand(UIInputCommand command)
{
    switch (command)
    {
    case UIInputCommand::NavUp:
        Browser_NavUp();
        break;
    
    case UIInputCommand::NavDown:
        Browser_NavDown();
        break;
    
    case UIInputCommand::NavLeft:
        Browser_NavLeft();
        break;
    
    case UIInputCommand::NavRight:
        Browser_NavRight();
        break;
    
    case UIInputCommand::RefreshBrowser:
        Browser_Refresh();
        break;
    
    case UIInputCommand::Select:
        Browser_Select();
        break;
    
    case UIInputCommand::NavPageUp:
    case UIInputCommand::NavPageDown:
    case UIInputCommand::ToggleAutoScroll:
    case UIInputCommand::Back:
    default:
        break;
    }
}

void UI::Browser_UpdateUIData()
{
    DirectoryBrowser::Iterator iter = m_browser->GetIteratorToFirst();
    iter += m_browserStartOffset;
    for (size_t index = 0; index < static_cast<size_t>(m_browserElementCount); ++index)
    {
        auto uiElement = m_browserVerticalStack->GetChildByIndex(index);
    
        if (iter.IsValid())
        {
            // Update visibility and selection
            uiElement->SetVisible(true);
            uiElement->SetEnabled(true);
            if (m_browserIter.GetIndex() == iter.GetIndex())
            {
                uiElement->SetStyleId(ID("focused_browser_item_style"));
            }
            else
            {
                uiElement->SetStyleId(ID("unfocused_browser_item_style"));
            }
    
            // Update data
            float spacerAmount = 40.0f * static_cast<float>(iter.GetDepth());
            std::string expanderText;
            if (iter.IsDirectory())
            {
                if (iter.GetDirectory()->GetExpanded())
                {
                    expanderText = "-";
                }
                else
                {
                    expanderText = "+";
                }
            }
            auto horizontalStackElement = uiElement->GetSubElementById(ID("BrowserItemHorizontalStack"));
            auto spacerElement = horizontalStackElement->GetSubElementById(ID("BrowserItemSpacerPanel"));
            auto currentSpacerSize = spacerElement->GetRelativeSizeInRefUnits();
            spacerElement->SetRelativeSizeInRefUnits(Vector2(spacerAmount, currentSpacerSize.y));
            horizontalStackElement->GetTypedSubElementById<UIStaticText>(ID("BrowserExpanderText"))->SetDisplayText(expanderText);
            horizontalStackElement->GetTypedSubElementById<UIStaticText>(ID("BrowserEntryText"))->SetDisplayText(iter.GetName());
    
            ++iter;
        }
        else
        {
            uiElement->SetVisible(false);
            uiElement->SetEnabled(false);
        }
    }
}

void UI::Browser_NavUp()
{
    if (!m_browserIter.IsValid())
    {
        return;
    }

    if (m_browserIter.GetIndex() == 0)
    {
        return;
    }

    --m_browserIter;
    if (m_browserStartOffset > m_browserIter.GetIndex())
    {
        m_browserStartOffset = m_browserIter.GetIndex();
    }
    Browser_UpdateUIData();
}

void UI::Browser_NavDown()
{
    if (!m_browserIter.IsValid())
    {
        return;
    }

    int maxIndex = static_cast<int>(m_browser->GetNumIteratedElements()) - 1;
    if (m_browserIter.GetIndex() == maxIndex)
    {
        return;
    }

    ++m_browserIter;
    if (m_browserStartOffset <= (m_browserIter.GetIndex() - m_browserElementCount))
    {
        m_browserStartOffset = (m_browserIter.GetIndex() - m_browserElementCount) + 1;
    }
    Browser_UpdateUIData();
}

void UI::Browser_NavLeft()
{
    if (m_browserIter.IsValid() && m_browserIter.IsDirectory())
    {
        if (m_browserIter.GetDirectory()->GetExpanded())
        {
            m_browserIter.SetDirectoryExpanded(false);
            Browser_UpdateUIData();
        }
        else
        {
            std::shared_ptr<Directory> parent = m_browserIter.GetDirectory()->GetParent().lock();
            if (parent)
            {
                while (true)
                {
                    Browser_NavUp();
                    if (m_browserIter.IsDirectory() && m_browserIter.GetDirectory() == parent)
                    {
                        break;
                    }

                    if (m_browserIter.GetIndex() == 0)
                    {
                        break;
                    }
                }
            }
        }
    }
}

void UI::Browser_NavRight()
{
    if (m_browserIter.IsValid() && m_browserIter.IsDirectory())
    {
        if (m_browserIter.GetDirectory()->GetExpanded())
        {
            if (m_browserIter.GetDirectory()->HasChildren())
            {
                Browser_NavDown();
            }
        }
        else
        {
            m_browserIter.SetDirectoryExpanded(true);
            Browser_UpdateUIData();
        }
    }
}

void UI::Browser_Select()
{
    if (!m_browserIter.IsValid())
    {
        return;
    }

    if(m_browserIter.IsFile())
    {
        if (Launcher_Initialize(m_browserIter.GetFile()))
        {
            SwitchTo(UIState::Launcher);
        }
    }
    else
    {
        m_browserIter.SetDirectoryExpanded(!m_browserIter.GetDirectoryExpanded());
        Browser_UpdateUIData();
    }
}

void UI::Browser_Refresh()
{
    Browser_Initialize();
}

bool UI::Launcher_Initialize(std::shared_ptr<File> file)
{
    if (!file)
    {
        return false;
    }
    
    // Update data
    m_exePath = file->GetFullPath();
    m_exeCommandLine = m_launcherCache->GetValue(m_exePath, "commandLine");
    m_exeWorkingDir = m_launcherCache->GetValue(m_exePath, "workingDir");
    if (m_exeWorkingDir.empty())
    {
        m_exeWorkingDir = std::filesystem::path(m_exePath).parent_path().string();
    }
    
    // Update UI
    m_launcherExeEntryText->SetDisplayText(m_exePath);
    m_launcherCommandLineEntryText->SetDisplayText(m_exeCommandLine);
    m_launcherWorkingDirEntryText->SetDisplayText(m_exeWorkingDir);
    
    return true;
}

void UI::Launcher_Update()
{
    // Update text block selections every frame. Is there an event for when UI focus changes?
    if (m_launcherExeEntryPanel->IsFocused())
    {
        m_launcherExeEntryPanel->SetStyleId(ID("focused_button_style"));
    }
    else
    {
        m_launcherExeEntryPanel->SetStyleId(ID("basic_button_style"));
    }
    if (m_launcherCommandLineEntryPanel->IsFocused())
    {
        m_launcherCommandLineEntryPanel->SetStyleId(ID("focused_button_style"));
    }
    else
    {
        m_launcherCommandLineEntryPanel->SetStyleId(ID("basic_button_style"));
    }
    if (m_launcherWorkingDirEntryPanel->IsFocused())
    {
        m_launcherWorkingDirEntryPanel->SetStyleId(ID("focused_button_style"));
    }
    else
    {
        m_launcherWorkingDirEntryPanel->SetStyleId(ID("basic_button_style"));
    }
}

void UI::Launcher_HandleInputCommand(UIInputCommand command)
{
    switch (command)
    {
    case UIInputCommand::Select:
        if (m_launcherLaunchCPUOnlyButton->IsFocused())
        {
            Launcher_Launch(false);
        }
        else if (m_launcherLaunchCPUAndGPUButton->IsFocused())
        {
            Launcher_Launch(true);
        }
        else if (m_launcherCancelButton->IsFocused())
        {
            Launcher_Cancel();
        }
        else
        {
            Launcher_SpawnVirtualKeyboard();
        }
        break;

    case UIInputCommand::Back:
        m_uiManager.SetFocus(m_launcherCancelButton);
        break;

    case UIInputCommand::NavUp:
        UITK_NavUp();
        break;

    case UIInputCommand::NavDown:
        UITK_NavDown();
        break;

    case UIInputCommand::NavLeft:
        UITK_NavLeft();
        break;

    case UIInputCommand::NavRight:
        UITK_NavRight();
        break;

    case UIInputCommand::NavPageUp:
    case UIInputCommand::NavPageDown:
    case UIInputCommand::RefreshBrowser:
    case UIInputCommand::ToggleAutoScroll:
    default:
        break;
    }
}

void UI::Launcher_Launch(bool gpu)
{
    m_launcherCache->SetValue(m_exePath, "commandLine", m_exeCommandLine);
    m_launcherCache->SetValue(m_exePath, "workingDir", m_exeWorkingDir);
    m_launcherCache->Save();

    Runtime_Initialize();
    SwitchTo(UIState::Runtime);

    // Send proc start request
    std::string commandLine = "\"";
    commandLine += m_exePath;
    commandLine += "\" ";
    commandLine += m_exeCommandLine;
    m_callbacks.m_spawnToolProcessCallback(commandLine, m_exeWorkingDir.c_str(), gpu);
}

void UI::Launcher_Cancel()
{
    SwitchTo(UIState::Browser);
}

void UI::Launcher_SpawnVirtualKeyboard()
{
    std::shared_ptr<ATG::UITK::UIStaticText> textEntryToUse;
    std::string titleText;
    std::string descriptionText;
    std::string defaultText;
    bool forCommandLine = false;
    bool forWorkingDir = false;

    if (m_launcherExeEntryPanel->IsFocused())
    {
        textEntryToUse = m_launcherExeEntryText;
        titleText = "Enter Exe";
        descriptionText = "Enter the full path of the executable to launch in the title partition";
        defaultText = m_launcherExeEntryText->GetDisplayText();
    }
    else if (m_launcherCommandLineEntryPanel->IsFocused())
    {
        textEntryToUse = m_launcherCommandLineEntryText;
        titleText = "Enter Command Line";
        descriptionText = "Enter the command line to use for the executable launch";
        defaultText = m_launcherCommandLineEntryText->GetDisplayText();
        forCommandLine = true;
    }
    else if (m_launcherWorkingDirEntryPanel->IsFocused())
    {
        textEntryToUse = m_launcherWorkingDirEntryText;
        titleText = "Enter Working Directory";
        descriptionText = "Enter the working directory to set for the executable launch";
        defaultText = m_launcherWorkingDirEntryText->GetDisplayText();
        forWorkingDir = true;
    }

    if (textEntryToUse)
    {
        struct ExtraData
        {
            std::shared_ptr<ATG::UITK::UIStaticText> textEntry;
            std::string* strToUpdate;
            std::string exePath;
            ATG::LauncherCache* launcherCache;
            bool forCommandLine;
            bool forWorkingDir;
        };
        auto extraData = new ExtraData{};
        extraData->textEntry = textEntryToUse;
        extraData->launcherCache = m_launcherCache.get();
        extraData->forCommandLine = forCommandLine;
        extraData->forWorkingDir = forWorkingDir;
        extraData->strToUpdate = forCommandLine ? &m_exeCommandLine : forWorkingDir ? &m_exeWorkingDir : &m_exePath;
        extraData->exePath = m_exePath;

        auto asyncBlock = new XAsyncBlock{};
        asyncBlock->queue = m_taskQueue;
        asyncBlock->context = extraData;
        asyncBlock->callback =
            [](XAsyncBlock* async)
        {
            auto extraData = static_cast<ExtraData*>(async->context);

            uint32_t resultSize = 0;
            HRESULT result = XGameUiShowTextEntryResultSize(async, &resultSize);
            if (SUCCEEDED(result))
            {
                if (resultSize == 0)
                {
                    (*extraData->strToUpdate) = "";
                    extraData->textEntry->SetDisplayText(*extraData->strToUpdate);

                    if (extraData->forCommandLine)
                    {
                        extraData->launcherCache->SetValue(extraData->exePath, "commandLine", (*extraData->strToUpdate));
                        extraData->launcherCache->Save();
                    }
                    else if (extraData->forWorkingDir)
                    {
                        extraData->launcherCache->SetValue(extraData->exePath, "workingDir", (*extraData->strToUpdate));
                        extraData->launcherCache->Save();
                    }
                }
                else
                {
                    char* buf = new char[static_cast<size_t>(resultSize) + 2];

                    uint32_t written = 0;
                    result = XGameUiShowTextEntryResult(async, resultSize + 1, buf, &written);
                    if (SUCCEEDED(result))
                    {
                        buf[written] = 0;

                        (*extraData->strToUpdate) = buf;
                        extraData->textEntry->SetDisplayText(*extraData->strToUpdate);

                        if (extraData->forCommandLine)
                        {
                            extraData->launcherCache->SetValue(extraData->exePath, "commandLine", (*extraData->strToUpdate));
                            extraData->launcherCache->Save();
                        }
                        else if (extraData->forWorkingDir)
                        {
                            extraData->launcherCache->SetValue(extraData->exePath, "workingDir", (*extraData->strToUpdate));
                            extraData->launcherCache->Save();
                        }
                    }

                    delete[] buf;
                }
            }

            delete async;
            delete extraData;
        };

        XGameUiShowTextEntryAsync(asyncBlock, titleText.c_str(), descriptionText.c_str(), defaultText.c_str(),
            XGameUiTextEntryInputScope::Url, MAX_PATH);
    }
}

void UI::Runtime_Initialize(bool getDisplayDataFromCallback)
{
    Runtime_ClearLog();
    
    if (getDisplayDataFromCallback)
    {
        m_runtimeExeCommandlineText->SetDisplayText(m_callbacks.m_getCommandLineProcessCallback());
    
        std::string workingDir = "Working Dir: \"";
        workingDir += m_callbacks.m_getCommandLineWorkingDirCallback();
        workingDir += "\"";
        m_runtimeWorkingDirText->SetDisplayText(workingDir);
    }
    else
    {
        std::string commandLine = "\"";
        commandLine += m_exePath;
        commandLine += "\" ";
        commandLine += m_exeCommandLine;
        m_runtimeExeCommandlineText->SetDisplayText(commandLine);

        std::string workingDir = "Working Dir: \"";
        workingDir += m_exeWorkingDir;
        workingDir += "\"";
        m_runtimeWorkingDirText->SetDisplayText(workingDir);
    }
}

void UI::Runtime_Update()
{
    Runtime_UpdateButtonPromptUIData();
}

void UI::Runtime_HandleInputCommand(UIInputCommand command)
{
    switch (command)
    {
    case UIInputCommand::NavUp:
        Runtime_ScrollUp();
        break;
    
    case UIInputCommand::NavDown:
        Runtime_ScrollDown();
        break;
    
    case UIInputCommand::NavPageUp:
        Runtime_ScrollPageUp();
        break;
    
    case UIInputCommand::NavPageDown:
        Runtime_ScrollPageDown();
        break;
    
    case UIInputCommand::ToggleAutoScroll:
        Runtime_ToggleAutoScroll();
        break;
    
    case UIInputCommand::Back:
        if (!m_callbacks.m_isToolProcessActiveCallback())
        {
            Runtime_ReturnToBrowser();
        }
        break;
    
    case UIInputCommand::NavLeft:
    case UIInputCommand::NavRight:
    case UIInputCommand::RefreshBrowser:
    case UIInputCommand::Select:
    default:
        break;
    }
}

static bool FindInStringCaseInsensitive(const std::string& stringToSearch, const std::string& stringToSearchFor)
{
    auto iter = std::search(
        stringToSearch.begin(), stringToSearch.end(),
        stringToSearchFor.begin(), stringToSearchFor.end(),
        [](char first, char second)
        {
            return std::tolower(first) == std::tolower(second);
        }
    );
    return iter != stringToSearch.end();
}

static ID GetStyleIdToUse(const std::string& text)
{
    if (FindInStringCaseInsensitive(text, "fail") ||
        FindInStringCaseInsensitive(text, "error"))
    {
        return ID("entry_text_style_error");
    }
    else if (FindInStringCaseInsensitive(text, "warn") ||
        FindInStringCaseInsensitive(text, "[stderr]"))
    {
        return ID("entry_text_style_warning");
    }
    else if (FindInStringCaseInsensitive(text, "success") ||
        FindInStringCaseInsensitive(text, "succeed") ||
        FindInStringCaseInsensitive(text, "finish") ||
        FindInStringCaseInsensitive(text, "complete"))
    {
        return ID("entry_text_style_success");
    }
    else
    {
        return ID("entry_text_style");
    }
}

void UI::Runtime_UpdateUIData()
{
    for (size_t index = 0; index < m_runtimeLogTexts.size(); ++index)
    {
        size_t logIndex = m_logRenderIndex + index;
        if (logIndex < m_log.size())
        {
            m_runtimeLogTexts[index]->SetVisible(true);
            m_runtimeLogTexts[index]->SetEnabled(true);
            m_runtimeLogTexts[index]->SetStyleId(GetStyleIdToUse(m_log[logIndex]));
            m_runtimeLogTexts[index]->SetDisplayText(m_log[logIndex]);
        }
        else
        {
            m_runtimeLogTexts[index]->SetVisible(false);
            m_runtimeLogTexts[index]->SetEnabled(false);
        }
    }

    Runtime_UpdateButtonPromptUIData();
}

void UI::Runtime_UpdateButtonPromptUIData()
{
    std::string buildingText = "[DPad]Up/[DPad]Down: Scroll Log   [LB]/[RB]: Page Scroll Log";

    if (m_logAutoScroll)
    {
        buildingText += "   [Y]: Toggle Auto-Scroll (Currently Enabled)";
    }
    else
    {
        buildingText += "   [Y]: Toggle Auto-Scroll (Currently Disabled)";
    }

    if (m_callbacks.m_isToolProcessActiveCallback())
    {
        buildingText += "   Hold [B]: Terminate Process";
    }
    else
    {
        buildingText += "   [B]: Return to Browser";
    }

    m_runtimeButtonPromptText->SetDisplayText(buildingText);
}

void UI::Runtime_ScrollUp()
{
    if (m_logRenderIndex > 0)
    {
        --m_logRenderIndex;
    }

    Runtime_UpdateUIData();
}

void UI::Runtime_ScrollDown()
{
    if ((m_log.size() > m_runtimeLogTexts.size()) &&
        (m_logRenderIndex < (m_log.size() - m_runtimeLogTexts.size())))
    {
        ++m_logRenderIndex;
    }

    Runtime_UpdateUIData();
}

void UI::Runtime_ScrollPageUp()
{
    if (m_logRenderIndex > m_runtimeLogTexts.size())
    {
        m_logRenderIndex -= m_runtimeLogTexts.size();
    }
    else
    {
        m_logRenderIndex = 0;
    }

    Runtime_UpdateUIData();
}

void UI::Runtime_ScrollPageDown()
{
    if (m_log.size() <= m_runtimeLogTexts.size())
    {
        m_logRenderIndex = 0;
        return;
    }

    size_t maxAllowableIndex = m_log.size() - m_runtimeLogTexts.size();
    if (maxAllowableIndex - m_logRenderIndex >= m_runtimeLogTexts.size())
    {
        m_logRenderIndex += m_runtimeLogTexts.size();
    }
    else
    {
        m_logRenderIndex = maxAllowableIndex;
    }

    Runtime_UpdateUIData();
}

void UI::Runtime_ToggleAutoScroll()
{
    m_logAutoScroll = !m_logAutoScroll;

    Runtime_UpdateButtonPromptUIData();
}

void UI::Runtime_Log(const char* text)
{
    // Store log entry
    // Split by new lines for our own log
    std::stringstream stringStream(text);
    std::string foundLine;
    while (std::getline(stringStream, foundLine, u8'\n'))
    {
        m_log.push_back(foundLine);
    }
    
    // Auto-scroll
    if (m_logAutoScroll)
    {
        if (m_log.size() < m_runtimeLogTexts.size())
        {
            m_logRenderIndex = 0;
        }
        else
        {
            m_logRenderIndex = m_log.size() - m_runtimeLogTexts.size();
        }
    }
    
    // To save memory, remove elements when logging too much
    if (m_log.size() > m_logMaxBufferHistory)
    {
        const size_t numElementsToErase = m_log.size() - m_logMaxBufferHistory;
        m_log.erase(m_log.begin(), m_log.begin() + static_cast<const int64_t>(numElementsToErase));
        if (m_logRenderIndex < numElementsToErase)
        {
            m_logRenderIndex = 0;
        }
        else
        {
            m_logRenderIndex -= numElementsToErase;
        }
    }
    
    Runtime_UpdateUIData();
}

void UI::Runtime_ClearLog()
{
    m_log.clear();
    m_logRenderIndex = 0;

    Runtime_UpdateUIData();
}

void UI::Runtime_ReturnToBrowser()
{
    SwitchTo(UIState::Browser);
}
