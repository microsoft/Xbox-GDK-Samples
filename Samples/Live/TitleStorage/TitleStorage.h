//--------------------------------------------------------------------------------------
// TitleStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "TextConsole.h"

#include "ListView.h"
#include "UIConstants.h"
#include "LiveInfoHUD.h"

struct MetadataBlobDetail
{
    MetadataBlobDetail() = default;

    MetadataBlobDetail(const XblTitleStorageBlobMetadata *blobmetadata) :
        blobPath{ blobmetadata->blobPath },
        blobType{ blobmetadata->blobType },
        storageType{ blobmetadata->storageType },
        displayName{ blobmetadata->displayName },
        eTag{ blobmetadata->eTag },
        clientTimestamp{ blobmetadata->clientTimestamp },
        length{ blobmetadata->length },
        serviceConfigurationId{ blobmetadata->serviceConfigurationId },
        xboxUserId{}
    {
    }

    std::string blobPath;
    XblTitleStorageBlobType blobType;
    XblTitleStorageType storageType;
    std::string displayName;
    std::string eTag;
    time_t clientTimestamp;
    size_t length;
    std::string serviceConfigurationId;
    uint64_t xboxUserId;
};

class Sample;

class TitleStorageViewRow
{
public:
    void Show();
    void Hide();
    void SetControls(ATG::IPanel *parent, int rowStart);
    void Update(const MetadataBlobDetail *item);
    void SetSelectedCallback(ATG::IControl::callback_t callback);

    MetadataBlobDetail metadataBlob;

    static std::weak_ptr<ATG::UIManager> s_ui;
    static Sample*                       s_sample;

private:
    ATG::Button    *m_selectBtn;
    ATG::TextLabel *m_blobPath;
    ATG::TextLabel *m_blobType;
    ATG::TextLabel *m_displayName;
    ATG::TextLabel *m_length;
    ATG::TextLabel *m_XUID;
};


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    //TitleStorage
    void ChangeStorageType(XblTitleStorageType);
    void DeleteBlob(MetadataBlobDetail *blob);

    void UploadBlob(XblTitleStorageBlobType blobtype, char *blobPath, char *blobdata);
    void DownloadBlob(MetadataBlobDetail *blob);

    void ShowNoticePopUp(const wchar_t *noticeType, const wchar_t *message);
    void ShowFileOptions(void *blob, bool canDelete);
    void ListViewUpdate(const XblTitleStorageBlobMetadata metadata[], int itemsize);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;

    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    
    enum Descriptors
    {
        Reserve,
        Count = 32,
    };

    void SetupUI();
    using AsyncCallBackFunc = std::function<void(XAsyncBlock*)>;

    XblContextHandle GetXblContext() const { return m_liveResources->GetLiveContext(); }
    uint64_t GetXuid() const { return m_liveResources->GetXuid(); }

    XTaskQueueHandle							m_mainAsyncQueue;
    XblTitleStorageType                         m_selectedStorageType;

    // UI Objects
    std::shared_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_log;
    std::unique_ptr<DX::TextConsoleImage>       m_display;

    std::unique_ptr<ListView<MetadataBlobDetail, TitleStorageViewRow>> m_listView;
    std::vector<MetadataBlobDetail > m_currentBlobList;
};

