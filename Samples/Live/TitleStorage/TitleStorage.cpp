//--------------------------------------------------------------------------------------
// TitleStorage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TitleStorage.h"

#include "ATGColors.h"
#include "StringUtil.h"

#include "UIConstants.h"

extern void ExitSample() noexcept;

using namespace DirectX;

namespace
{
    const wchar_t* BlobTypeStrings[] = {
        L"Unknown",
        L"Binary",
        L"JSON",
        L"Config"
    };

    template <typename functor>
    void StringWidenAndUse(std::string_view toConvert, functor fun)
    {
        assert(toConvert.size() <= 2048);
        wchar_t buffer[2048] = {};
        int result = MultiByteToWideChar(CP_UTF8, 0, toConvert.data(), -1, buffer, _countof(buffer));
        if (!result)
        {
            throw std::exception("MultiByteToWideChar");
        }
        fun(buffer);
    }

    const char* scid = "00000000-0000-0000-0000-0000610194be"; // This SCID is for TitleStorage Sample
    constexpr uint32_t preferredUploadBlockSize = (1024 * 256); // default value
}

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_mainAsyncQueue(nullptr),
    m_selectedStorageType(XblTitleStorageType::GlobalStorage)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_mainAsyncQueue)
    );

    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_shared<ATG::LiveResources>(m_mainAsyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Title Storage Sample");

    ATG::UIConfig uiconfig;

    wcscpy_s(uiconfig.largeFontName, L"Assets/SegoeUI_36.spritefont");
    wcscpy_s(uiconfig.largeItalicFontName, L"Assets/SegoeUI_36_Italic.spritefont");
    wcscpy_s(uiconfig.largeBoldFontName, L"Assets/SegoeUI_36_Bold.spritefont");

    wcscpy_s(uiconfig.midFontName, L"Assets/SegoeUI_22.spritefont");
    wcscpy_s(uiconfig.midItalicFontName, L"Assets/SegoeUI_22_Italic.spritefont");
    wcscpy_s(uiconfig.midBoldFontName, L"Assets/SegoeUI_22_Bold.spritefont");

    wcscpy_s(uiconfig.smallFontName, L"Assets/SegoeUI_18.spritefont");
    wcscpy_s(uiconfig.smallItalicFontName, L"Assets/SegoeUI_18_Italic.spritefont");
    wcscpy_s(uiconfig.smallBoldFontName, L"Assets/SegoeUI_18_Bold.spritefont");

    wcscpy_s(uiconfig.largeLegendName, L"Assets/XboxOneControllerLegend.spritefont");
    wcscpy_s(uiconfig.smallLegendName, L"Assets/XboxOneControllerLegendSmall.spritefont");

    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
    m_log = std::make_unique<DX::TextConsoleImage>();
    m_display = std::make_unique<DX::TextConsoleImage>();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
    if (m_mainAsyncQueue)
    {
        XTaskQueueCloseHandle(m_mainAsyncQueue);
        m_mainAsyncQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
#ifdef _GAMING_DESKTOP
    m_mouse->SetWindow(window);
#endif

    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets\\");

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_mainAsyncQueue);
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();
        // Default to show Global Storage
        if (user != nullptr)
        {
            ChangeStorageType(XblTitleStorageType::GlobalStorage);
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_mainAsyncQueue);
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Close();
    });

    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {

        }
    });

// Before we can make an Xbox Live call we need to ensure that the Game OS has intialized the network stack
// For sample purposes we block user interaction with the sample.  A game should wait for the network to be
// initialized before the main menu appears.  For samples, we will wait at the end of initialization.
    while (!m_liveResources->IsNetworkAvailable())
    {
        SwitchToThread();
    }

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();

    SetupUI();
}

void Sample::SetupUI()
{
    using namespace ATG;

    ListViewConfig config = { 10, 3100, 100, c_sampleUIPanel };
    m_listView = std::make_unique<ListView<MetadataBlobDetail, TitleStorageViewRow>>(m_ui, config);
    m_listView->ClearAllRows();
    m_listView->SetSelectedCallback([this](IPanel*, IControl* control)
    {
        auto storageMetadata = reinterpret_cast<MetadataBlobDetail*>(control->GetUser());
        ShowFileOptions(storageMetadata, storageMetadata->storageType != XblTitleStorageType::GlobalStorage);

    });

    // File Option to download selected file
    m_ui->FindControl<Button>(c_fileOptionsPopUpPanel, 8001)->SetCallback([this](IPanel*, IControl* control)
    {
        auto storageMetadata = reinterpret_cast<MetadataBlobDetail*>(control->GetUser());
        DownloadBlob(storageMetadata);
    });

    // File Option to delete selected file
    m_ui->FindControl<Button>(c_fileOptionsPopUpPanel, 8002)->SetCallback([this](IPanel*, IControl* control)
    {
        auto storageMetadata = reinterpret_cast<MetadataBlobDetail*>(control->GetUser());
        DeleteBlob(storageMetadata);
    });

    // Upload button selected
    m_ui->FindControl<Button>(c_sampleUIPanel, 2206)->SetCallback([this](IPanel*, IControl*)
    {
        bool canUploadBinary = XblTitleStorageType::GlobalStorage != m_selectedStorageType;
        if (canUploadBinary)
        {
            m_ui->FindPanel<IPanel>(c_uploadPopUpPanel)->Show();
        }
    });

    // Upload the JSON blob
    m_ui->FindControl<Button>(c_uploadPopUpPanel, 9001)->SetCallback([this](IPanel*, IControl*)
    {
        UploadBlob(XblTitleStorageBlobType::Json, (char*)"./blob.json", (char*)"{\n\"isThisJson\": 1,\n\"monstersKilled\": 10,\n\"playerClass\": \"warrior\"\n}");
    });

    // Upload the binary blob
    m_ui->FindControl<Button>(c_uploadPopUpPanel, 9002)->SetCallback([this](IPanel*, IControl*)
    {
        UploadBlob(XblTitleStorageBlobType::Binary, (char*)"./blob.dat", (char*)"This is a Test.");
    });

    // Storage Location options
    m_ui->FindControl<CheckBox>(c_sampleUIPanel, c_globalStorageCheckBox)->SetCallback([this](IPanel* parent, IControl*)
    {
        dynamic_cast<CheckBox*>(parent->Find(c_globalStorageCheckBox))->SetChecked(true);
        dynamic_cast<CheckBox*>(parent->Find(c_jsonStorageCheckBox))->SetChecked(false);
        dynamic_cast<CheckBox*>(parent->Find(c_trustedStorageCheckBox))->SetChecked(false);

        ChangeStorageType(XblTitleStorageType::GlobalStorage);
    });
    m_ui->FindControl<CheckBox>(c_sampleUIPanel, c_jsonStorageCheckBox)->SetCallback([this](IPanel* parent, IControl*)
    {
        dynamic_cast<CheckBox*>(parent->Find(c_globalStorageCheckBox))->SetChecked(false);
        dynamic_cast<CheckBox*>(parent->Find(c_jsonStorageCheckBox))->SetChecked(true);
        dynamic_cast<CheckBox*>(parent->Find(c_trustedStorageCheckBox))->SetChecked(false);

        ChangeStorageType(XblTitleStorageType::Universal);
    });
    m_ui->FindControl<CheckBox>(c_sampleUIPanel, c_trustedStorageCheckBox)->SetCallback([this](IPanel* parent, IControl*)
    {
        dynamic_cast<CheckBox*>(parent->Find(c_globalStorageCheckBox))->SetChecked(false);
        dynamic_cast<CheckBox*>(parent->Find(c_jsonStorageCheckBox))->SetChecked(false);
        dynamic_cast<CheckBox*>(parent->Find(c_trustedStorageCheckBox))->SetChecked(true);

        ChangeStorageType(XblTitleStorageType::TrustedPlatformStorage);
    });
}

// Pulls data out of the blob metadata for display
void TitleStorageViewRow::Update(const MetadataBlobDetail* item)
{
    // Extract information from the blob metadata for display
    StringWidenAndUse(item->blobPath, [this](const wchar_t* converted)
    {
        m_blobPath->SetText(converted);
    });
    m_blobType->SetText(BlobTypeStrings[static_cast<int>(item->blobType)]);

    StringWidenAndUse(item->displayName, [this](const wchar_t* converted)
    {
        m_displayName->SetText(converted);
    });
    StringWidenAndUse(std::to_string(item->length), [this](const wchar_t* converted)
    {
        m_length->SetText(converted);
    });
    StringWidenAndUse(std::to_string(item->xboxUserId), [this](const wchar_t* converted)
    {
        m_XUID->SetText(converted);
    });

    metadataBlob = *item;
}
// Download a blob of the selected storage.
void Sample::DownloadBlob(MetadataBlobDetail* blob)
{
    struct DownloadContext
    {
        Sample* sample;
        std::unique_ptr<std::vector<uint8_t>> contextBuffer;
    };

    auto ctx = new DownloadContext;
    ctx->sample = this;
    ctx->contextBuffer = std::make_unique<std::vector<uint8_t>>(blob->length);

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(ctx);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto context = reinterpret_cast<DownloadContext*>(asyncBlock->context);
        auto& [sample, downloadBlobBuffer] = *reinterpret_cast<DownloadContext*>(asyncBlock->context);

        XblTitleStorageBlobMetadata blobMetadata;
        HRESULT hr = XblTitleStorageDownloadBlobResult(asyncBlock, &blobMetadata);

        if (SUCCEEDED(hr))
        {
            std::wstring downloadstr;
            downloadstr.assign(downloadBlobBuffer->begin(), downloadBlobBuffer->end());
            context->sample->ShowNoticePopUp(L"Download Result", downloadstr.c_str());
        }
        else
        {
            context->sample->ShowNoticePopUp(L"Download Error", (wchar_t*)L"Failed to download the data.");
        }
        delete reinterpret_cast<DownloadContext*>(asyncBlock->context);
        delete asyncBlock;
    };

    XblTitleStorageBlobMetadata blobMetadata{};
    strcpy_s(blobMetadata.serviceConfigurationId, blob->serviceConfigurationId.c_str());
    strcpy_s(blobMetadata.blobPath, blob->blobPath.c_str());
    blobMetadata.storageType = blob->storageType;
    blobMetadata.blobType = blob->blobType;
    blobMetadata.xboxUserId = GetXuid();


    HRESULT hr = XblTitleStorageDownloadBlobAsync(
        GetXblContext(),
        blobMetadata,
        ctx->contextBuffer->data(),
        blob->length,
        XblTitleStorageETagMatchCondition::NotUsed,
        "", // optional
        0, // optional
        asyncBlock
    );
    if (FAILED(hr))
    {
        delete ctx;
        delete asyncBlock;
    }
}

// Delete a blob of the selected storage.
void Sample::DeleteBlob(MetadataBlobDetail* blob)
{
    struct Deleteontext
    {
        Sample* sample;
        XblTitleStorageType storagetype;
    };
    auto ctx = new Deleteontext;
    ctx->sample = this;
    ctx->storagetype = blob->storageType;

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(ctx);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto context = reinterpret_cast<Deleteontext*>(asyncBlock->context);
        HRESULT hr = XAsyncGetStatus(asyncBlock, false);

        if (SUCCEEDED(hr))
        {
            context->sample->ChangeStorageType(context->storagetype);
        }
        delete reinterpret_cast<Deleteontext*>(asyncBlock->context);
        delete asyncBlock;
    };

    XblTitleStorageBlobMetadata blobMetadata{};

    strcpy_s(blobMetadata.serviceConfigurationId, blob->serviceConfigurationId.c_str());
    strcpy_s(blobMetadata.blobPath, blob->blobPath.c_str());
    blobMetadata.storageType = blob->storageType;
    blobMetadata.blobType = blob->blobType;
    blobMetadata.xboxUserId = GetXuid();

    HRESULT hr = XblTitleStorageDeleteBlobAsync(
        GetXblContext(),
        blobMetadata,
        false,
        asyncBlock
    );
    if (FAILED(hr))
    {
        delete ctx;
        delete asyncBlock;
    }

}
// Uploads a blob to a specific path in the selected storage type.
void Sample::UploadBlob(XblTitleStorageBlobType blobType, char* blobpath, char* blobdata)
{
    struct UploadBlobContext
    {
        Sample* sample;
        std::unique_ptr<std::vector<uint8_t>> blob;
    };
    auto ctx = new UploadBlobContext();
    ctx->sample = this;
    ctx->blob = std::make_unique<std::vector<uint8_t>>(*blobdata);

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(ctx);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto& [sample, blob] = *reinterpret_cast<UploadBlobContext*>(asyncBlock->context);

        XblTitleStorageBlobMetadata blobMetadata{};
        HRESULT hr = XblTitleStorageUploadBlobResult(asyncBlock, &blobMetadata);

        if (SUCCEEDED(hr))
        {
            sample->ChangeStorageType(blobMetadata.storageType);
        }
        delete reinterpret_cast<UploadBlobContext*>(asyncBlock->context);
        delete asyncBlock;

    };

    XblTitleStorageBlobMetadata blobMetadata{};
    strcpy_s(blobMetadata.displayName, "storage sample");
    strcpy_s(blobMetadata.serviceConfigurationId, scid);
    strcpy_s(blobMetadata.blobPath, blobpath);
    blobMetadata.storageType = m_selectedStorageType;
    blobMetadata.blobType = blobType;

    HRESULT hr = XblTitleStorageUploadBlobAsync(
        GetXblContext(),
        blobMetadata,
        (uint8_t*)blobdata,
        std::strlen(blobdata),
        XblTitleStorageETagMatchCondition::NotUsed,
        preferredUploadBlockSize,
        asyncBlock
    );

    if (FAILED(hr))
    {
        delete ctx;
        delete asyncBlock;
    }
}

void Sample::ShowNoticePopUp(const wchar_t* title, const wchar_t* message)
{
    std::wstring titleCapture = title, messageCapture = message;
    m_ui->FindControl<ATG::TextLabel>(c_noticePopUpPanel, 7001)->SetText(titleCapture.c_str());
    m_ui->FindControl<ATG::TextLabel>(c_noticePopUpPanel, 7002)->SetText(messageCapture.c_str());
    m_ui->FindPanel<ATG::IPanel>(c_noticePopUpPanel)->Show();
}

void Sample::ShowFileOptions(void* blob, bool canDelete)
{
    m_ui->FindControl<ATG::Button>(c_fileOptionsPopUpPanel, 8001)->SetUser(blob);

    // If the storage location is Global Storage, then we cannot delete the file.
    // Disable the delete option.
    auto deleteBtn = m_ui->FindControl<ATG::Button>(c_fileOptionsPopUpPanel, 8002);
    deleteBtn->SetEnabled(canDelete);
    deleteBtn->SetUser(blob);
    m_ui->FindPanel<ATG::IPanel>(c_fileOptionsPopUpPanel)->Show();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (!m_ui->Update(elapsedTime, pad))
        {
            if (pad.IsViewPressed())
            {
                ExitSample();
            }
            if (pad.IsYPressed())
            {
                std::wstring noticeMessage = L"Downloaded Blob Contents:\n";
                ShowNoticePopUp(L"Error", noticeMessage.c_str());
            }

            if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
            {
                if (!m_liveResources->IsUserSignedIn())
                {
                    m_liveResources->SignInSilently();
                }
                else
                {
                    m_liveResources->SignInWithUI();
                }
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            m_liveResources->SignInWithUI();
        }
    }

    // Process any completed tasks
    while (XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0))
    {
    }

    m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);
    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);
    m_ui->Render(commandList);

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
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
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
    width = 1980;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto const device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    const RECT fullscreen = m_deviceResources->GetOutputSize();

    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    m_ui->SetWindow(fullscreen);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion


#pragma region ListViewRow

void Sample::ListViewUpdate(const XblTitleStorageBlobMetadata metadata[], int itemsize)
{
    std::vector<MetadataBlobDetail> blobdetail;

    MetadataBlobDetail listdata;

    for (int i = 0; i < itemsize; ++i)
    {
        listdata.blobPath = metadata[i].blobPath;
        listdata.blobType = metadata[i].blobType;
        listdata.storageType = metadata[i].storageType;
        listdata.displayName = metadata[i].displayName;
        listdata.eTag = metadata[i].eTag;
        listdata.clientTimestamp = metadata[i].clientTimestamp;
        listdata.length = metadata[i].length;
        listdata.serviceConfigurationId = metadata[i].serviceConfigurationId;
        listdata.xboxUserId = metadata[i].xboxUserId;
        blobdetail.emplace_back(listdata);

        m_listView->UpdateRows(blobdetail);
    }

}

void Sample::ChangeStorageType(XblTitleStorageType storage_type)
{
    m_selectedStorageType = storage_type;

    struct ChangeStorageTypeContext
    {
        Sample* sample;
    };
    auto ctx = new ChangeStorageTypeContext();
    ctx->sample = this;

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(ctx);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto context = reinterpret_cast<ChangeStorageTypeContext*>(asyncBlock->context);

        size_t usedBytes;
        size_t quotaBytes;
        HRESULT hr = XblTitleStorageGetQuotaResult(asyncBlock, &usedBytes, &quotaBytes);

        if (SUCCEEDED(hr))
        {
            context->sample->m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_usedBytesLabel)->SetText(std::to_wstring(usedBytes).c_str());
            context->sample->m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_quotaBytesLabel)->SetText(std::to_wstring(quotaBytes).c_str());
        }
        delete reinterpret_cast<ChangeStorageTypeContext*>(asyncBlock->context);
        delete asyncBlock;
    };

    HRESULT hr = XblTitleStorageGetQuotaAsync(
        GetXblContext(),
        scid,
        storage_type,
        asyncBlock
    );
    if (FAILED(hr))
    {
        delete ctx;
        delete asyncBlock;
    }

    struct GetBlobContext
    {
        Sample* sample;
    };
    auto ctx_getblob = new GetBlobContext();
    ctx_getblob->sample = this;

    auto asyncBlockGetBlob = new XAsyncBlock{};
    asyncBlockGetBlob->queue = m_mainAsyncQueue;
    asyncBlockGetBlob->context = reinterpret_cast<void*>(ctx_getblob);
    asyncBlockGetBlob->callback = [](XAsyncBlock* asyncBlockGetBlob)
    {
        auto context = reinterpret_cast<GetBlobContext*>(asyncBlockGetBlob->context);

        XblTitleStorageBlobMetadataResultHandle blobMetadataResultHandle;
        HRESULT hr = XblTitleStorageGetBlobMetadataResult(asyncBlockGetBlob, &blobMetadataResultHandle);

        if (SUCCEEDED(hr))
        {
            if (blobMetadataResultHandle != nullptr)
            {
                XblTitleStorageBlobMetadataResultCloseHandle(blobMetadataResultHandle);
            }

            const XblTitleStorageBlobMetadata* items;
            size_t itemsSize;

            HRESULT hr_items = XblTitleStorageBlobMetadataResultGetItems(blobMetadataResultHandle, &items, &itemsSize);
            if (SUCCEEDED(hr_items))
            {
                context->sample->ListViewUpdate(&items[0], (int)itemsSize);
            }
        }
        else {
            context->sample->m_listView->ClearAllRows();
        }
        delete reinterpret_cast<GetBlobContext*>(asyncBlockGetBlob->context);
        delete asyncBlockGetBlob;
    };

    HRESULT brob_hr = XblTitleStorageGetBlobMetadataAsync(
        GetXblContext(),
        scid,
        storage_type,
        "./",
        GetXuid(),
        0,//option
        0,//option
        asyncBlockGetBlob
    );

    if (FAILED(brob_hr))
    {
        delete ctx_getblob;
        delete asyncBlockGetBlob;
    }

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, 2206)->SetEnabled(storage_type != XblTitleStorageType::GlobalStorage);

}

void TitleStorageViewRow::Show()
{
    m_selectBtn->SetVisible(true);
    m_selectBtn->SetEnabled(true);
    m_blobPath->SetVisible(true);
    m_blobType->SetVisible(true);
    m_displayName->SetVisible(true);
    m_length->SetVisible(true);
    m_XUID->SetVisible(true);
}

void TitleStorageViewRow::Hide()
{
    m_selectBtn->SetVisible(false);
    m_selectBtn->SetEnabled(false);
    m_blobPath->SetVisible(false);
    m_blobType->SetVisible(false);
    m_displayName->SetVisible(false);
    m_length->SetVisible(false);
    m_XUID->SetVisible(false);
}

void TitleStorageViewRow::SetControls(ATG::IPanel *parent, uint32_t rowStart)
{
    m_selectBtn = dynamic_cast<ATG::Button *>(parent->Find(rowStart));
    if (m_selectBtn)
    {
        m_selectBtn->SetUser(reinterpret_cast<void*>(this));
    }
    m_selectBtn = dynamic_cast<ATG::Button *>(parent->Find(rowStart));
    m_blobPath = dynamic_cast<ATG::TextLabel *>(parent->Find(rowStart + 1));
    m_blobType = dynamic_cast<ATG::TextLabel *>(parent->Find(rowStart + 2));
    m_displayName = dynamic_cast<ATG::TextLabel *>(parent->Find(rowStart + 3));
    m_length = dynamic_cast<ATG::TextLabel *>(parent->Find(rowStart + 4));
    m_XUID = dynamic_cast<ATG::TextLabel *>(parent->Find(rowStart + 5));
}

void TitleStorageViewRow::SetSelectedCallback(ATG::IControl::callback_t callback)
{
    m_selectBtn->SetCallback(callback);
}

#pragma endregion
