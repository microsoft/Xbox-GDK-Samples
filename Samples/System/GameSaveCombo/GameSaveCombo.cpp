//--------------------------------------------------------------------------------------
// GameSaveCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameSaveCombo.h"
#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    const char* c_scid = "00000000-0000-0000-0000-00007E750470";
    const char* c_worldDataBlobName = "WorldState";
    const char* c_characterStatDataBlobName = "CharacterStats";
    const char* c_containerName = "SampleContainerName";

    const char* c_xPosition = "xPosition";
    const char* c_yPosition = "yPosition";
    const char* c_isDayTime = "isDayTime";

    const char* c_name = "name";
    const char* c_level = "level";
    const char* c_combatClass = "combatClass";
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_userHandle(nullptr),
    m_providerHandle(nullptr),
    m_userAddInProgress(false),
    m_isDataDisplayable(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    srand(static_cast<unsigned int>(time(0)));
    m_gamerPic = {0};
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);

    XUserCloseHandle(m_userHandle);
    XGameSaveCloseProvider(m_providerHandle);
    XGameSaveCloseContainer(m_containerHandle);
}

#pragma region XGameSave

HRESULT Sample::GetProviderHandle()
{
    // Setup async block and function
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = static_cast<Sample*>(asyncBlock->context);

        XGameSaveProviderHandle providerHandle = nullptr;
        HRESULT hr = XGameSaveInitializeProviderResult(asyncBlock, &providerHandle);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_providerHandle)
            {
                // If switching out the provider handle, close the current handle.
                XGameSaveCloseProvider(pThis->m_providerHandle);
            }
            pThis->m_providerHandle = providerHandle;
            pThis->Log("Successfully obtained Provider Handle");
            pThis->InitializeData();
        }
        else
        {
            pThis->LogFailedHR("XGameSaveInitializeProviderResult", hr);
        }

        delete asyncBlock;
    };

    // Obtain the provider handle for the user. This handle is required in order for a user
    // to manipulate XGameSave data with a specific title(scid).
    HRESULT hr = XGameSaveInitializeProviderAsync(m_userHandle, c_scid, false, asyncBlock);
    if (FAILED(hr))
    {
        // Failed, so be sure to clean async block
        delete asyncBlock;
        LogFailedHR("XGameSaveInitializeProviderAsync", hr);
    }
    return hr;
}

HRESULT Sample::InitializeData()
{
    Log("Enumerating all containers.");

    struct ContainerInfoContext
    {
        uint8_t containerCount;
        Sample* pThis;
    };

    ContainerInfoContext context;
    context.pThis = this;
    context.containerCount = 0;

    // Iterate through all containers that match the passed in container name.
    HRESULT hr = XGameSaveEnumerateContainerInfoByName(m_providerHandle, c_containerName, &context,
        [](_In_ const XGameSaveContainerInfo*, _In_ void* context) -> bool
        {
            ContainerInfoContext* ctx = static_cast<ContainerInfoContext*>(context);
            ctx->containerCount++;
            return true;
        }
    );

    if (FAILED(hr))
    {
        LogFailedHR("XGameSaveEnumerateContainerInfoByName", hr);
    }
    else
    {
        if (context.containerCount == 0)
        {
            Log("No containers found!");
        }
        else
        {
            Log("Container found! Downloading blobs.");
            CreateContainer(true);
        }
    }

    return hr;
}

// Create a container handle under a specific title. The container
// may already exist, and if so, we just load the data.
// If it doesn't then we create a new container with new blob data.
HRESULT Sample::CreateContainer(bool containerExists)
{
    HRESULT hr = XGameSaveCreateContainer(m_providerHandle, c_containerName, &m_containerHandle);

    XGameSaveUpdateHandle updateHandle = nullptr;

    if (containerExists)
    {
        if (SUCCEEDED(hr))
        {
            hr = LoadBlobsFromDisk();
        }
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            // Create an update handle. All changes to this container will happen through
            // this update handle.
            hr = XGameSaveCreateUpdate(m_containerHandle, c_containerName, &updateHandle);
        }
        if (SUCCEEDED(hr))
        {
            hr = GenerateBlobData(updateHandle);
        }
        if (SUCCEEDED(hr))
        {
            // Write all of our changes in our update context to the container.
            hr = XGameSaveSubmitUpdate(updateHandle);
        }
        if (SUCCEEDED(hr))
        {
            XGameSaveCloseUpdate(updateHandle);
        }
    }
    if (SUCCEEDED(hr))
    {
        Log("Successfully created/loaded a container with blob data");
    }

    return hr;
}

HRESULT Sample::GenerateBlobData(XGameSaveUpdateHandle updateHandle)
{
    HRESULT hr = S_OK;
    json characterStatData;
    json worldData;

    characterStatData[c_name] = "Rodney";
    characterStatData[c_level] = rand() % 100 + 1;
    characterStatData[c_combatClass] = "Mage";

    worldData[c_xPosition] = rand() % 1000 - 500;
    worldData[c_yPosition] = rand() % 1000 - 500;
    worldData[c_isDayTime] = false;

    std::string characterStatDataStr = characterStatData.dump();
    std::string worldDataStr = worldData.dump();

    const uint8_t* serializedCharacterStatData = reinterpret_cast<const uint8_t*>(characterStatDataStr.c_str());
    size_t characterStatDataSize = sizeof(uint8_t) * (characterStatDataStr.size() + 1);

    const uint8_t* serializedWorldData = reinterpret_cast<const uint8_t*>(worldDataStr.c_str());
    size_t worldDataSize = sizeof(uint8_t) * (worldDataStr.size() + 1);

    // Queue an write update to the c_characterStatDataBlobName blob in our update context.
    hr = XGameSaveSubmitBlobWrite(updateHandle, c_characterStatDataBlobName, serializedCharacterStatData, characterStatDataSize);
    if (SUCCEEDED(hr))
    {
        // Queue an write update to the c_worldDataBlobName blob in our update context.
        hr = XGameSaveSubmitBlobWrite(updateHandle, c_worldDataBlobName, serializedWorldData, worldDataSize);
    }
    if (SUCCEEDED(hr))
    {
        // UI updates
        m_currentWorldData.xPosition = worldData[c_xPosition];
        m_currentWorldData.yPosition = worldData[c_yPosition];
        m_currentWorldData.isDayTime = worldData[c_isDayTime];

        m_currentCharacterData.name = characterStatData[c_name];
        m_currentCharacterData.level = characterStatData[c_level];
        m_currentCharacterData.combatClass = characterStatData[c_combatClass];
        SetDataDisplayable(true);
    }
    return hr;
}

HRESULT Sample::LoadBlobsFromDisk()
{
    SetDataDisplayable(false);

    const char* blobNames[] = {
        c_worldDataBlobName,
        c_characterStatDataBlobName
    };

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;

    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = reinterpret_cast<Sample*>(asyncBlock->context);
        uint32_t blobCount = 0;
        size_t allocatedSize = 0;
        XAsyncGetResultSize(asyncBlock, &allocatedSize);

        uint8_t* blobBuf = new uint8_t[allocatedSize];
        XGameSaveBlob* blobData = reinterpret_cast<XGameSaveBlob*>(blobBuf);

        HRESULT hr = XGameSaveReadBlobDataResult(asyncBlock, allocatedSize, blobData, &blobCount);
        if (SUCCEEDED(hr))
        {
            pThis->ParseBlobData(blobData, blobCount);
        }
        delete [] blobBuf;
        delete asyncBlock;
    };

    HRESULT hr = XGameSaveReadBlobDataAsync(m_containerHandle, blobNames, 2, asyncBlock);

    if (FAILED(hr))
    {
        delete asyncBlock;
    }
    return hr;
}

HRESULT Sample::DeleteContainer()
{
    HRESULT hr = S_FALSE;
    hr = XGameSaveDeleteContainer(m_providerHandle, c_containerName);
    if (SUCCEEDED(hr))
    {
        Log("Deleted Container!");
        SetDataDisplayable(false);
    }
    return hr;
}

#pragma endregion

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    UIElementPtr layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/sample_layout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_loadButton = layout->GetTypedChildById<UIButton>(ID("Load_Button"));
    m_addContainerButton = layout->GetTypedChildById<UIButton>(ID("Add_Container_Button"));
    m_deleteContainerButton = layout->GetTypedChildById<UIButton>(ID("Delete_Container_Button"));
    m_switchUserButton = layout->GetTypedChildById<UIButton>(ID("Switch_User_Button"));
    m_consoleWindow = layout->GetTypedChildById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_containerLabel = layout->GetTypedChildById<UIStaticText>(ID("Container_Label"));
    m_userInfoPanel = layout->GetTypedChildById<UIStackPanel>(ID("Sample_Title_Panel"))->
        GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"));
    m_gamertagText = m_userInfoPanel->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));
    m_gamerpicImage = m_userInfoPanel->GetTypedChildById<UIImage>(ID("Gamerpic"));

    m_characterDataNameLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("Character_Data_Name"));
    m_characterDataLevelLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("Character_Data_Level"));
    m_characterDataCombatClassLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("Character_Data_CombatClass"));
    m_worldDataIsDayTimeLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("World_Data_IsDayTime"));
    m_worldDataXPositionLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("World_Data_XPosition"));
    m_worldDataYPositionLabel = layout->GetTypedChildById<UIPanel>(ID("Input_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("World_Data_YPosition"));

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("Output_Console_Window"));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue));

    RegisterUIEventHandlers();

    DX::ThrowIfFailed(GetUserHandle(XUserAddOptions::AddDefaultUserAllowingUI));
}

// Register the event handlers to be used for UI elements.
void Sample::RegisterUIEventHandlers()
{
    if (m_loadButton)
    {
        m_loadButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->InitializeData());
            });
    }
    if (m_addContainerButton)
    {
        m_addContainerButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->CreateContainer(false));
            });
    }
    if (m_deleteContainerButton)
    {
        m_deleteContainerButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->DeleteContainer());
            });
    }
    if (m_switchUserButton)
    {
        m_switchUserButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->GetUserHandle(XUserAddOptions::None));
            });
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Push logs to UI
    if(m_logQueue.size() > 0)
    {
        std::lock_guard<std::mutex> scopeLock(m_logMutex);
        for (size_t i = 0; i < m_logQueue.size(); ++i)
        {
            m_consoleWindow->AppendLineOfText(m_logQueue[i]);
        }
        m_logQueue.clear();
    }
    if (m_gamerPic.size != 0)
    {
        m_gamerpicImage->UseTextureData(m_gamerPic.data.get(), m_gamerPic.size);

        m_gamerPic.size = 0;
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

    UpdateView();

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
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
    m_uiManager.Render();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

void Sample::OnSuspending()
{
    XGameSaveCloseProvider(m_providerHandle);
    XGameSaveCloseContainer(m_containerHandle);
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_inputState.Reset();
    GetProviderHandle();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion

#pragma region User Management

HRESULT Sample::GetUserHandle(XUserAddOptions userAddOption)
{
    if (m_userAddInProgress)
    {
        return S_FALSE;
    }

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = static_cast<Sample*>(asyncBlock->context);

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_userHandle)
            {
                XUserCloseHandle(pThis->m_userHandle);
            }
            pThis->m_userHandle = newUser;
            pThis->Log("Successfully obtained User Handle");

            DX::ThrowIfFailed(pThis->GetProviderHandle());
            pThis->UpdateUserUIData();
        }
        else
        {
            pThis->LogFailedHR("XUserAddResult", hr);
        }

        pThis->m_userAddInProgress = false;
        delete asyncBlock;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock);
    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
    }
    else
    {
        LogFailedHR("XUserAddAsync", hr);
        delete asyncBlock;
    }

    return hr;
}

HRESULT Sample::UpdateUserUIData()
{
    //Reset UI
    SetDataDisplayable(false);

    char gamertagBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1] = {};
    size_t gamertagSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::UniqueModern, sizeof(gamertagBuffer), gamertagBuffer, &gamertagSize));
    m_gamertagText->SetDisplayText(gamertagBuffer);

    struct AsyncBlockContext
    {
        GamerPicBytes* gamerPicBytes;
        Sample* pThis;
    };
    AsyncBlockContext* ctx = new AsyncBlockContext();
    ctx->gamerPicBytes = &m_gamerPic;
    ctx->pThis = this;

    // Setup gamerpic request
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = ctx;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        AsyncBlockContext* ctx = static_cast<AsyncBlockContext*>(asyncBlock->context);
        Sample* pThis = ctx->pThis;
        // Get buffer size
        size_t bufferSize = 0;
        DX::ThrowIfFailed(XUserGetGamerPictureResultSize(asyncBlock, &bufferSize));

        size_t bufferUsed = 0;

        ctx->gamerPicBytes->size = bufferSize;
        ctx->gamerPicBytes->data.reset(new uint8_t[bufferSize]);

        DX::ThrowIfFailed(XUserGetGamerPictureResult(asyncBlock, ctx->gamerPicBytes->size, ctx->gamerPicBytes->data.get(), &bufferUsed));

        // Set in UI
        pThis->Log("Got the gamer pic.");
        delete ctx;
        delete asyncBlock;
    };

    // Request gamerpic
    HRESULT hr = XUserGetGamerPictureAsync(m_userHandle, XUserGamerPictureSize::Medium, asyncBlock);
    if (FAILED(hr))
    {
        LogFailedHR("XUserGetGamerPictureAsync", hr);
    }
    return hr;
}
#pragma endregion

#pragma region UI

void Sample::UpdateView()
{
    char buffer[256] = {};
    if (m_isDataDisplayable)
    {
        m_containerLabel->SetDisplayText(c_containerName);
        sprintf_s(buffer, 256, u8"%s: %s", c_name, m_currentCharacterData.name.c_str());
        m_characterDataNameLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: %d", c_level, m_currentCharacterData.level);
        m_characterDataLevelLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: %s", c_combatClass, m_currentCharacterData.combatClass.c_str());
        m_characterDataCombatClassLabel->SetDisplayText(buffer);

        sprintf_s(buffer, 256, u8"%s: %s", c_isDayTime, m_currentWorldData.isDayTime ? "true" : "false");
        m_worldDataIsDayTimeLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: %d", c_xPosition, m_currentWorldData.xPosition);
        m_worldDataXPositionLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: %d", c_yPosition, m_currentWorldData.yPosition);
        m_worldDataYPositionLabel->SetDisplayText(buffer);
    }
    else
    {
        sprintf_s(buffer, 256, u8"%s: ", c_name);
        m_characterDataNameLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: ", c_level);
        m_characterDataLevelLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: ", c_combatClass);
        m_characterDataCombatClassLabel->SetDisplayText(buffer);

        sprintf_s(buffer, 256, u8"%s: ", c_isDayTime);
        m_worldDataIsDayTimeLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s:", c_xPosition);
        m_worldDataXPositionLabel->SetDisplayText(buffer);
        sprintf_s(buffer, 256, u8"%s: ", c_yPosition);
        m_worldDataYPositionLabel->SetDisplayText(buffer);
    }
}

void Sample::SetDataDisplayable(bool value)
{
    if (value != m_isDataDisplayable)
    {
        m_isDataDisplayable = value;
    }
}

#pragma endregion

void Sample::ParseBlobData(XGameSaveBlob* blobData, uint32_t blobCount)
{
    for (uint32_t i = 0; i < blobCount; i++)
    {
        json data = nlohmann::json::parse(reinterpret_cast<const char*>(blobData[i].data));
        if (strcmp(blobData[i].info.name, c_worldDataBlobName) == 0)
        {
            m_currentWorldData.xPosition = data[c_xPosition];
            m_currentWorldData.yPosition = data[c_yPosition];
            m_currentWorldData.isDayTime = data[c_isDayTime];
        }
        else
        {
            m_currentCharacterData.name = data[c_name];
            m_currentCharacterData.level = data[c_level];
            m_currentCharacterData.combatClass = data[c_combatClass];
        }
    }
    // UI 
    SetDataDisplayable(true);
}

void Sample::LogFailedHR(const char* functionName, HRESULT hr )
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s failed with hr=%08X", functionName, hr);
    Log(buffer);
}

void Sample::Log(const char* text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        m_logQueue.push_back(text);
    }

    OutputDebugStringA(text);
    OutputDebugStringA(u8"\n");
}
