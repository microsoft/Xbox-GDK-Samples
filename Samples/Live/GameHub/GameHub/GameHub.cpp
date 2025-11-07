//--------------------------------------------------------------------------------------
// GameHub.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameHub.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

ATG::FileDownloader Sample::s_fileDownloader;

namespace
{
    Sample* s_sample;

    template <size_t bufferSize = 2048>
    void debugPrint(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize] = "";

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);

        if (s_sample)
        {
            auto console = s_sample->GetConsole();
            if (console)
            {
                console->AppendLineOfText(buffer);
            }
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_storeContext(nullptr),
    m_asyncQueue(nullptr),
    m_packageInstallToken{},
    m_isStoreEnumerating{ false }
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("GameHub");

    //  Register for when a package is installed
    DX::ThrowIfFailed(
    XPackageRegisterPackageInstalled(m_asyncQueue, this, [](void* context, const XPackageDetails* package)
    {
        auto pThis = reinterpret_cast<Sample*>(context);
        debugPrint("Package Installed event received: %s\n", package->displayName);
        pThis->EnumerateInstalledPackages();

        ProductInfo* product = pThis->FindProduct(package->storeId);
        if (product)
        {
            pThis->UpdatePackageStatusUI(*product);
            pThis->UpdateMenuButtonUI();
        }

    }, & m_packageInstallToken)
    );
}

Sample::~Sample()
{
    if (m_storeContext)
    {
        XStoreCloseContextHandle(m_storeContext);
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XPackageUnregisterPackageInstalled(m_packageInstallToken, true);

    if (m_asyncQueue)
    {
        XTaskQueueCloseHandle(m_asyncQueue);
        m_asyncQueue = nullptr;
    }
}

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

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());

        if (!XUserIsStoreUser(user))
        {
            debugPrint("Current user doesn't match store user.\n");
        }

        if (m_storeContext)
        {
            XStoreCloseContextHandle(m_storeContext);
        }

        XNetworkingConnectivityHint hint{};
        XNetworkingGetConnectivityHint(&hint);

        m_networkingConnectivityLevel = hint.connectivityLevel;

        HRESULT hr  = XStoreCreateContext(user, &m_storeContext);

        if (SUCCEEDED(hr))
        {
            EnumerateInstalledPackages();
            CheckInstalledPackagesForUpdates();
            EnumerateStoreProducts(); // This calls RefreshUI
        }
        else
        {
            debugPrint("XStoreCreateContext failed : 0x%x\n", hr);
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());

        XStoreCloseContextHandle(m_storeContext);
        m_storeContext = nullptr;
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

    InitializeUI();

    s_sample = this;

    WritePLS();
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

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

#ifdef USING_GAMEINPUT
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }
    const auto& keys = m_inputState.GetKeyboardKeys();

    if (m_gamePadButtons.view == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
    }

    if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Tab))
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

    if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::A))
    {
        if (m_selectedStoreId != "")
        {
            ProductInfo* product = FindProduct(m_selectedStoreId);

            if (m_packageList.find(product->storeId) != m_packageList.end())
            {
                auto& package = m_packageList[product->storeId];

                if (package.hasUpdate)
                {
                    // Download and install ?
                    DownloadUpdatedPackage(package, false);
                }
                else
                {
                    if (package.isGame)
                    {
                        std::string uri = "ms-xbl-" + package.titleId + "://";
                        debugPrint("Launching %s %s\n", package.displayName.c_str(), uri.c_str());

                        XLaunchUri(m_liveResources->GetUser(), uri.c_str());
#ifdef _GAMING_DESKTOP
                        ExitSample();
#endif
                    }
                }
            }
            else
            {
                if (product->isInUserCollection)
                {
                    if (product->hasDigitalDownload)
                    {
                        DownloadStorePackage(*product);
                    }
                }
                else
                {
                    PurchaseStoreProduct(*product);
                }
            }
        }
    }

    if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::B))
    {
        if (m_selectedStoreId != "")
        {
            if (m_packageList.find(m_selectedStoreId) != m_packageList.end())
            {
                auto& package = m_packageList[m_selectedStoreId];

                std::vector<const char*> packageIds = { package.packageIdentifier.c_str() };
                CheckForUpdates(packageIds);
            }
        }
    }

    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::X))
    {
        EnumerateUpdatedPackages();
    }

    if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Y))
    {
        if (m_selectedStoreId != "")
        {
            ProductInfo* product = FindProduct(m_selectedStoreId);

            CheckLicense(*product);

            if (m_packageList.find(product->storeId) != m_packageList.end())
            {
                auto& package = m_packageList[product->storeId];

                CheckLicense(package);
            }
        }
    }

    if (m_gamePadButtons.leftStick == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::L))
    {
        if (m_selectedStoreId != "")
        {
            ProductInfo* product = FindProduct(m_selectedStoreId);

            if (m_packageList.find(product->storeId) != m_packageList.end())
            {
                auto& package = m_packageList[product->storeId];

                UninstallPackage(package);
            }
        }
    }

    if (m_gamePadButtons.rightStick == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::R))
    {
        debugPrint("Refreshing.\n");

        m_uiManager.ClearChildren(m_gamePanel);
        m_uiManager.ClearChildren(m_durablePanel);
        m_durableList.clear();
        m_queryDurableList.clear();

        EnumerateInstalledPackages();
        CheckInstalledPackagesForUpdates();
        EnumerateStoreProducts(); // This calls RefreshUI
    }

    // Query DLC sequentially.
    if (!m_isStoreEnumerating && m_queryDurableList.size() > 0)
    {
        std::string durableStoreId = m_queryDurableList.front();

        EnumerateDurables(durableStoreId); // This calls RefreshUI

        m_queryDurableList.pop_front();
    }

    // Process any completed tasks
    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0))
    {
    }

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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_liveResources->Refresh();
    m_gamePadButtons.Reset();
    m_inputState.Reset();
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
    width = 1920;
    height = 1080;
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
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto const size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region Products
void Sample::EnumerateInstalledPackages()
{
    m_packageList.clear();

    HRESULT hr = XPackageEnumeratePackages(XPackageKind::Game, XPackageEnumerationScope::ThisPublisher,
        this, [](void*, const XPackageDetails* details) -> bool
    {
        debugPrint("[XPackageEnumeratePackages][Game][ThisPublisher] %s %s %s\n",
        details->packageIdentifier, details->storeId, details->displayName);
        return true;
    });

    if (FAILED(hr))
    {
        debugPrint("XPackageEnumeratePackages [Game][ThisPublisher] failed : 0x%08X\n", hr);
    }

    hr = XPackageEnumeratePackages(XPackageKind::Content, XPackageEnumerationScope::ThisPublisher,
        this, [](void*, const XPackageDetails* details) -> bool
    {
        debugPrint("[XPackageEnumeratePackages][Content][ThisPublisher] %s %s %s\n",
        details->packageIdentifier, details->storeId, details->displayName);
        return true;
    });

    if (FAILED(hr))
    {
        debugPrint("XPackageEnumeratePackages [Content][ThisPublisher] failed : 0x%08X\n", hr);
    }

    hr = XPackageEnumeratePackages(XPackageKind::Game, XPackageEnumerationScope::ThisAndRelated,
        this, [](void* context, const XPackageDetails* details) -> bool
    {
        Sample* pThis = reinterpret_cast<Sample*>(context);

        debugPrint("[XPackageEnumeratePackages][Game][ThisAndRelated] %s %s %s\n",
            details->packageIdentifier, details->storeId, details->displayName);

        PackageInfo package(details);
        package.isGame = true;
        pThis->m_packageList.insert(std::make_pair(details->storeId, package));

        return true;
    });

    if (FAILED(hr))
    {
        debugPrint("XPackageEnumeratePackages [Game][ThisAndRelated] failed : 0x%08X\n", hr);
    }

    hr = XPackageEnumeratePackages(XPackageKind::Content, XPackageEnumerationScope::ThisAndRelated,
        this, [](void* context, const XPackageDetails* details) -> bool
    {
        Sample* pThis = reinterpret_cast<Sample*>(context);

        debugPrint("[XPackageEnumeratePackages][Content][ThisAndRelated] %s %s %s\n",
            details->packageIdentifier, details->storeId, details->displayName);

        PackageInfo package(details);
        pThis->m_packageList.insert(std::make_pair(details->storeId, package));

        return true;
    });

    if (FAILED(hr))
    {
        debugPrint("XPackageEnumeratePackages [Content][ThisAndRelated] failed : 0x%08X\n", hr);
    }
}

void Sample::CheckInstalledPackagesForUpdates()
{
    std::vector<const char*> packageIds;
    for (auto& pair : m_packageList)
    {
        auto& package = pair.second;
        packageIds.push_back(package.packageIdentifier.c_str());
    }

    CheckForUpdates(packageIds);
}

void Sample::EnumerateStoreProducts()
{
    RETURN_BUSY(m_isStoreEnumerating);

    m_gameList.clear();

    // When offline, we use dummy product to display UI.
    if (m_networkingConnectivityLevel != XNetworkingConnectivityLevelHint::InternetAccess)
    {
        for (auto& pair : m_packageList)
        {
            auto& package = pair.second;

            if (package.storeId == "9P6W7ZRLM4Q3") // GameHub own
                continue;

            ProductInfo product;
            product.storeId = package.storeId;
            product.title = package.displayName;
            product.hasDigitalDownload = true;

            if (package.isGame)
            {
                product.kind = XStoreProductKind::Game;
                m_gameList.insert_or_assign(product.storeId, product);
            }
            else
            {
                product.kind = XStoreProductKind::Durable;
                product.parentStoreId = ""; // No way to know
                m_durableList.insert_or_assign(product.storeId, product);
            }
        }

        m_isStoreEnumerating = false;
        RefreshGameProductUI();
        return;
    }

    auto async = new XAsyncBlock();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        Sample* pThis = reinterpret_cast<Sample*>(async->context);

        XStoreProductQueryHandle queryHandle = nullptr;

        HRESULT hr = XStoreQueryAssociatedProductsResult(
            async,
            &queryHandle);

        if (FAILED(hr))
        {
            debugPrint("XStoreQueryAssociatedProductsResult failed : 0x%08X\n", hr);
            pThis->m_isStoreEnumerating = false;
            delete async;
            return;
        }

        hr = XStoreEnumerateProductsQuery(
            queryHandle,
            async->context,
            [](const XStoreProduct* product, void* context) -> bool
        {
            Sample* pThis = reinterpret_cast<Sample*>(context);

            ProductInfo productInfo(product);

            debugPrint("[XStoreQueryAssociatedProducts/XStoreEnumerateProductsQuery] %s %s\n",
                product->storeId, product->title);

            for (uint32_t i = 0; i < product->imagesCount; i++)
            {
                UIProductImage image(product->images[i]);

                productInfo.images.push_back(image);
            }

            if (product->productKind == XStoreProductKind::Game)
            {
                pThis->m_gameList.insert_or_assign(productInfo.storeId, productInfo);
            }

            return true;
        }
        );

        if (FAILED(hr))
        {
            debugPrint("XStoreEnumerateProductsQuery failed : 0x%08X\n", hr);
            XStoreCloseProductsQueryHandle(queryHandle);
            pThis->m_isStoreEnumerating = false;
            delete async;
            return;
        }

        // To enum durable after enum game.
        for (auto& pair : pThis->m_gameList)
        {
            auto& product = pair.second;
            pThis->m_queryDurableList.push_back(product.storeId);
        }

        XStoreCloseProductsQueryHandle(queryHandle);
        delete async;

        pThis->RefreshGameProductUI();

        pThis->m_isStoreEnumerating = false;
    };

    HRESULT hr = XStoreQueryAssociatedProductsAsync(
        m_storeContext,
        XStoreProductKind::Game,
        25,
        async);

    if (FAILED(hr))
    {
        debugPrint("XStoreQueryAssociatedProductsAsync failed : 0x%08X\n", hr);
        m_isStoreEnumerating = false;
        delete async;
    }
}

void Sample::EnumerateDurables(std::string storeId)
{
    RETURN_BUSY(m_isStoreEnumerating);

    struct Context
    {
        Sample* pThis;
        std::string storeId;
    };

    Context* context = new Context{ this, storeId };

    auto async = new XAsyncBlock();
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, storeId] = *reinterpret_cast<Context*>(async->context);

        XStoreProductQueryHandle queryHandle = nullptr;

        HRESULT hr = XStoreQueryAssociatedProductsForStoreIdResult(
            async,
            &queryHandle);

        if (FAILED(hr))
        {
            debugPrint("XStoreQueryAssociatedProductsForStoreIdResult failed : 0x%08X\n", hr);
            pThis->m_isStoreEnumerating = false;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        hr = XStoreEnumerateProductsQuery(
            queryHandle,
            async->context,
            [](const XStoreProduct* product, void* context) -> bool
        {
            auto& [pThis, storeId] = *reinterpret_cast<Context*>(context);

            ProductInfo productInfo(product);

            debugPrint("[XStoreQueryAssociatedProductsForStoreId/XStoreEnumerateProductsQuery] %s %s\n",
                product->storeId, product->title);

            for (uint32_t i = 0; i < product->imagesCount; i++)
            {
                UIProductImage image(product->images[i]);

                productInfo.images.push_back(image);
            }

            productInfo.parentStoreId = storeId;

            pThis->m_durableList.insert_or_assign(productInfo.storeId, productInfo);

            return true;
        }
        );

        if (FAILED(hr))
        {
            debugPrint("XStoreEnumerateProductsQuery failed : 0x%08X\n", hr);
            XStoreCloseProductsQueryHandle(queryHandle);
            pThis->m_isStoreEnumerating = false;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        XStoreCloseProductsQueryHandle(queryHandle);
        pThis->m_isStoreEnumerating = false;
        pThis->RefreshDurableProductUI();
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreQueryAssociatedProductsForStoreIdAsync(
        m_storeContext,
        storeId.c_str(),
        XStoreProductKind::Durable,
        25,
        async);

    if (FAILED(hr))
    {
        debugPrint("XStoreQueryAssociatedProductsForStoreIdAsync failed : 0x%08X\n", hr);
        m_isStoreEnumerating = false;
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }
}

void Sample::EnumerateUpdatedPackages()
{
    RETURN_BUSY(m_isStoreEnumerating);

    auto async = new XAsyncBlock();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        Sample* pThis = reinterpret_cast<Sample*>(async->context);
        uint32_t count = 0;

        HRESULT hr = XStoreQueryGameAndDlcPackageUpdatesResultCount(async, &count);

        if (count > 0)
        {
            XStorePackageUpdate* updates = new XStorePackageUpdate[count];
            hr = XStoreQueryGameAndDlcPackageUpdatesResult(
                async,
                count,
                updates);

            if (FAILED(hr))
            {
                delete[] updates;
                debugPrint("XStoreQueryGameAndDlcPackageUpdatesResult failed: 0x%x\n", hr);
                pThis->m_isStoreEnumerating = false;
                return;
            }

            for (uint32_t index = 0; index < count; index++)
            {
                debugPrint("[XStoreQueryGameAndDlcPackageUpdatesAsync] %s %s\n",
                    updates[index].packageIdentifier,
                    updates[index].isMandatory ? "mandatory" : "");

                for (auto& pair : pThis->m_packageList)
                {
                    auto& package = pair.second;

                    if (package.packageIdentifier == updates[index].packageIdentifier)
                    {
                        debugPrint("Found update : %s %s\n", updates[index].packageIdentifier, updates[index].isMandatory ? "Mandatory" : "");

                        package.hasUpdate = true;
                        auto product = pThis->FindProduct(package.storeId);
                        pThis->UpdatePackageStatusUI(*product);
                        break;
                    }
                }
            }

            delete[] updates;
        }
        else
        {
            debugPrint("[XStoreQueryGameAndDlcPackageUpdatesAsync] No updates found..\n");
        }

        pThis->UpdateMenuButtonUI();
        pThis->m_isStoreEnumerating = false;
    };

    HRESULT hr = XStoreQueryGameAndDlcPackageUpdatesAsync(m_storeContext, async);

    if (FAILED(hr))
    {
        debugPrint("XStoreQueryGameAndDlcPackageUpdatesAsync failed : 0x%08X\n", hr);
        m_isStoreEnumerating = false;
    }
}


void Sample::PurchaseStoreProduct(ProductInfo& product)
{
    RETURN_BUSY(product.isBusy);

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context* context = new Context{ this, product };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, product] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);
        product.isBusy = false;

        if (FAILED(hr))
        {
            debugPrint("Failed the purchase : %s 0x%x\n", product.storeId.c_str(), hr);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        product.isInUserCollection = true;
        pThis->UpdatePackageStatusUI(product);
        pThis->UpdateMenuButtonUI();

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_storeContext,
        product.storeId.c_str(),
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async);

    if (FAILED(hr))
    {
        debugPrint("Failed to purchase : %s 0x%x\n", product.storeId.c_str(), hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
        return;
    }
}

void Sample::DownloadUpdatedPackage(PackageInfo& package, bool install)
{
    HRESULT hr = S_OK;

    struct Context
    {
        Sample* pThis;
        PackageInfo& package;
        bool install;
    };

    Context* context = new Context{ this, package, install };

    const char* packageIdentifiers[] = {
        package.packageIdentifier.c_str()
    };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, package, install] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = S_OK;

        if (install)
        {
            debugPrint("[XStoreDownloadAndInstallPackageUpdates] packageIdentifier : %s %s\n",
                package.packageIdentifier.c_str(), package.displayName.c_str());

            hr = XStoreDownloadAndInstallPackageUpdatesResult(async);

            if (FAILED(hr))
            {
                debugPrint("XStoreDownloadAndInstallPackageUpdatesResult failed : 0x%08X\n", hr);
                delete reinterpret_cast<Context*>(async->context);
                delete async;
            }
        }
        else
        {
            debugPrint("[XStoreDownloadPackageUpdates] packageIdentifier : %s %s\n",
                package.packageIdentifier.c_str(), package.displayName.c_str());

            hr = XStoreDownloadPackageUpdatesResult(async);

            if (FAILED(hr))
            {
                debugPrint("XStoreDownloadPackageUpdatesResult failed : 0x%08X\n", hr);
                delete reinterpret_cast<Context*>(async->context);
                delete async;
            }
        }

        auto product = pThis->FindProduct(package.storeId);
        pThis->UpdateDownloadProgressUI(product->button, 0, 100);

        // Start monitor
        pThis->StartInstallMonitor(package.packageIdentifier.c_str(), *product);

    };

    if (install)
    {
        hr = XStoreDownloadAndInstallPackageUpdatesAsync(m_storeContext, packageIdentifiers, 1, async);

        if (FAILED(hr))
        {
            debugPrint("XStoreDownloadAndInstallPackageUpdatesAsync failed : 0x%08X\n", hr);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
        }
    }
    else
    {
        hr = XStoreDownloadPackageUpdatesAsync(m_storeContext, packageIdentifiers, 1, async);

        if (FAILED(hr))
        {
            debugPrint("XStoreDownloadPackageUpdatesAsync failed : 0x%08X\n", hr);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
        }
    }
}

void Sample::DownloadStorePackage(ProductInfo& product)
{
    RETURN_BUSY(product.isBusy);

    const char* storeids[] = {
        product.storeId.c_str()
    };

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context* context = new Context{ this, product };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, product] = *reinterpret_cast<Context*>(async->context);

        uint32_t count;
        HRESULT hr = XStoreDownloadAndInstallPackagesResultCount(async, &count);

        if (FAILED(hr))
        {
            debugPrint("XStoreDownloadAndInstallPackagesResultCount failed : 0x%x\n", hr);
            product.isBusy = false;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        if (count == 0)
        {
            debugPrint("No downloads queued\n");
            product.isBusy = false;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        auto packageIdentifiers = new char[count][XPACKAGE_IDENTIFIER_MAX_LENGTH];

        hr = XStoreDownloadAndInstallPackagesResult(
            async,
            count,
            &packageIdentifiers[0]);

        if (FAILED(hr))
        {
            debugPrint("XStoreDownloadAndInstallPackagesResult failed : 0x%x\n", hr);
            product.isBusy = false;
            delete[] packageIdentifiers;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        debugPrint("[XStoreDownloadAndInstallPackages] packageIdentifier : %s %s\n", packageIdentifiers[0], product.title.c_str());

        pThis->UpdateDownloadProgressUI(product.button, 0, 100);

        // Start monitor
        pThis->StartInstallMonitor(packageIdentifiers[0], product);

        delete[] packageIdentifiers;
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreDownloadAndInstallPackagesAsync(m_storeContext, storeids, 1, async);

    if (FAILED(hr))
    {
        debugPrint("XStoreDownloadAndInstallPackagesAsync: 0x%x\n", hr);
        product.isBusy = false;
        delete reinterpret_cast<Context*>(context);
        delete async;
        return;
    }
}

void Sample::StartInstallMonitor(const char* identity, ProductInfo& product)
{
    XSystemAllowFullDownloadBandwidth(true);

    XPackageInstallationMonitorHandle monitor;

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    HRESULT hr = XPackageCreateInstallationMonitor(
        identity,      // Identity to be monitored
        0,             // Number of selectors
        nullptr,       // Selectors
        1000,          // Resolution of the monitor, in milliseconds
        m_asyncQueue,  // Queue where updates are performed
        &monitor);

    if (FAILED(hr))
    {
        debugPrint("Failed to create installation monitor : 0x%x\n", hr);
        product.isBusy = false;
        XSystemAllowFullDownloadBandwidth(false);
        return;
    }

    Context* context = new Context{ this, product };

    XTaskQueueRegistrationToken token;

    hr = XPackageRegisterInstallationProgressChanged(
        monitor,
        context,
        [](void* context, XPackageInstallationMonitorHandle monitor) -> void
    {
        auto& [pThis, product] = *reinterpret_cast<Context*>(context);

        XPackageInstallationProgress progress;
        XPackageGetInstallationProgress(monitor, &progress);

        if (progress.totalBytes != UINT64_MAX)
        {
            pThis->UpdateDownloadProgressUI(product.button, progress.installedBytes, progress.totalBytes);
        }

        if (progress.completed)
        {
            product.isBusy = false;
            pThis->UpdateDownloadProgressUI(product.button, 0, 0);

            debugPrint("-- install completed.\n");

            XPackageCloseInstallationMonitorHandle(monitor);
            delete reinterpret_cast<Context*>(context);
            XSystemAllowFullDownloadBandwidth(false);
        }

    }, &token);

    if (FAILED(hr))
    {
        product.isBusy = false;
        XPackageCloseInstallationMonitorHandle(monitor);
        delete reinterpret_cast<Context*>(context);
        XSystemAllowFullDownloadBandwidth(false);
    }
}

void Sample::CheckForUpdates(std::vector<const char*>& packageIds)
{
    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = reinterpret_cast<Sample*>(async->context);

        uint32_t count;
        HRESULT hr = XStoreQueryPackageUpdatesResultCount(async, &count);

        if (FAILED(hr))
        {
            debugPrint("XStoreQueryPackageUpdatesResultCount failed : 0x%08X\n", hr);
            delete async;
            return;
        }

        if (count == 0)
        {
            debugPrint("[XStoreQueryPackageUpdatesAsync] No updates found.\n");
            delete async;
            return;
        }

        XStorePackageUpdate* updates = new XStorePackageUpdate[count];

        hr = XStoreQueryPackageUpdatesResult(async, count, updates);

        if (FAILED(hr))
        {
            debugPrint("XStoreQueryPackageUpdatesResult failed : 0x%08X\n", hr);
            delete [] updates;
            delete async;
            return;
        }

        for (uint32_t index = 0; index < count; index++)
        {
            debugPrint("[XStoreQueryPackageUpdatesResult] %s\n", updates[index].packageIdentifier);

            for (auto& pair : pThis->m_packageList)
            {
                auto& package = pair.second;

                if (package.packageIdentifier == updates[index].packageIdentifier)
                {
                    debugPrint("Found update : %s\n", updates[index].packageIdentifier);

                    package.hasUpdate = true;
                    auto product = pThis->FindProduct(package.storeId);
                    if (product)
                    {
                        pThis->UpdatePackageStatusUI(*product);
                    }
                    break;
                }
            }
        }

        pThis->UpdateMenuButtonUI();

        delete[] updates;
        delete async;
    };

    HRESULT hr = XStoreQueryPackageUpdatesAsync(m_storeContext, packageIds.data(), packageIds.size(), async);

    if (FAILED(hr))
    {
        debugPrint("XStoreQueryPackageUpdatesAsync failed : 0x%08X\n", hr);
        delete async;
    }
}

void Sample::CheckLicense(PackageInfo& package)
{
    struct Context
    {
        Sample* pThis;
        PackageInfo& package;
    };

    Context* context = new Context{ this, package };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, package] = *reinterpret_cast<Context*>(async->context);
        XStoreCanAcquireLicenseResult result;

        HRESULT hr = XStoreCanAcquireLicenseForPackageResult(async, &result);

        if (SUCCEEDED(hr))
        {
            if (result.status == XStoreCanLicenseStatus::Licensable)
            {
                package.licensable = true;
            }
            else
            {
                package.licensable = false;
            }
        }
        else
        {
            package.licensable = false;
        }

        debugPrint("[XStoreCanAcquireLicenseForPackage] package licensable : %s\n", package.licensable ? "true" : "false");

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreCanAcquireLicenseForPackageAsync(m_storeContext, package.packageIdentifier.c_str(), async);

    if (FAILED(hr))
    {
        debugPrint("XStoreCanAcquireLicenseForPackageAsync failed : 0x%08X\n", hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }
}

void Sample::CheckLicense(ProductInfo& product)
{
    RETURN_BUSY(product.isBusy);

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context* context = new Context{ this, product };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, product] = *reinterpret_cast<Context*>(async->context);
        XStoreCanAcquireLicenseResult result;

        HRESULT hr = XStoreCanAcquireLicenseForStoreIdResult(async, &result);

        if (SUCCEEDED(hr))
        {
            if (result.status == XStoreCanLicenseStatus::Licensable)
            {
                product.licensable = true;
            }
            else
            {
                product.licensable = false;
            }
        }
        else
        {
            product.licensable = false;
        }

        debugPrint("[XStoreCanAcquireLicenseForStoreId] product licensable : %s\n", product.licensable ? "true" : "false");
        product.isBusy = false;

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreCanAcquireLicenseForStoreIdAsync(m_storeContext, product.storeId.c_str(), async);

    if (FAILED(hr))
    {
        debugPrint("XStoreCanAcquireLicenseForStoreIdAsync failed : 0x%08X\n", hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }
}

void Sample::UninstallPackage(PackageInfo& package)
{
    if (XPackageUninstallPackage(package.packageIdentifier.c_str()))
    {
        debugPrint("[XPackageUninstallPackage] success %s %s\n", package.packageIdentifier.c_str(), package.displayName.c_str());
        auto product = FindProduct(package.storeId);

        m_packageList.erase(package.storeId);

        UpdatePackageStatusUI(*product);
        UpdateMenuButtonUI();
    }
    else
    {
        debugPrint("XPackageUninstallPackage failed\n");
    }
}

ProductInfo* Sample::FindProduct(std::string storeId)
{
    if (m_gameList.find(storeId) != m_gameList.end())
    {
        return &m_gameList[storeId];
    }
    if (m_durableList.find(storeId) != m_durableList.end())
    {
        return &m_durableList[storeId];
    }

    return nullptr;
}
#pragma endregion

#pragma region UI Methods
void Sample::InitializeUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_gamePanel = layout->GetChildById(ID("GamePanel"));
    m_durablePanel = layout->GetChildById(ID("DurablePanel"));
    m_consoleWindow = layout->GetChildById(ID("OutputConsoleWindowOuterPanel"))->GetTypedChildById<UIConsoleWindow>(ID("OutputConsoleWindow"));
    m_menuPanel = layout->GetChildById(ID("MenuPanel"));
}

void Sample::RefreshGameProductUI()
{
    m_uiManager.ClearChildren(m_gamePanel);

    if (m_gameList.size() == 0)
    {
        m_uiManager.FindById(ID("EnumeratingGameText"))->SetVisible(true);
    }
    else
    {
        m_uiManager.FindById(ID("EnumeratingGameText"))->SetVisible(false);
    }

    for (auto& pair : m_gameList)
    {
        auto& product = pair.second;

        if (!product.button)
        {
            product.button = CastPtr<UIButton>(m_uiManager.InstantiatePrefab("Assets/ProductPrefab.json"));
        }
        m_gamePanel->AddChild(product.button);

        product.button->GetTypedSubElementById<UIStaticText>(ID("ProductText"))->SetDisplayText(product.title);
        product.button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->SetDisplayText(product.storeId);

        UpdatePackageStatusUI(product);

        DownloadProductImage(product, "Logo", product.button->GetTypedSubElementById<UIImage>(ID("ProductLogo")));

        product.button->ButtonState().AddListenerWhen(UIButton::State::Focused, [this](UIButton* button)
        {
            m_selectedStoreId = button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->GetDisplayText();
            UpdateMenuButtonUI();

            RefreshDurableProductUI();
        });

        product.button->ButtonState().AddListenerWhen(UIButton::State::Normal, [this](UIButton* button)
        {
            if (m_selectedStoreId == button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->GetDisplayText())
            {
                m_selectedStoreId = "";
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[DPad] Scroll debug text");
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckLicenseText"))->SetVisible(false);
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetVisible(false);
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("UninstallText"))->SetVisible(false);
            }
        });
    }

    // Set focus first one.
    if (m_gameList.size() > 0)
    {
        auto& product = m_gameList.begin()->second;
        m_uiManager.SetFocus(product.button);
    }
}

void Sample::RefreshDurableProductUI()
{
    m_uiManager.ClearChildren(m_durablePanel);

    if (m_durableList.size() == 0)
    {
        m_uiManager.FindById(ID("EnumeratingDLCText"))->SetVisible(true);
    }
    else
    {
        m_uiManager.FindById(ID("EnumeratingDLCText"))->SetVisible(false);
    }

    for (auto& pair : m_durableList)
    {
        auto& product = pair.second;
        if (!product.button)
        {
            product.button = CastPtr<UIButton>(m_uiManager.InstantiatePrefab("Assets/ProductPrefab.json"));
        }
    }

    // Sort by display name
    std::vector<std::pair<std::string, ProductInfo>> sortedList;
    {
        std::copy(m_durableList.begin(),
            m_durableList.end(),
            std::back_inserter<std::vector<std::pair<std::string, ProductInfo>>>(sortedList));

        std::sort(sortedList.begin(), sortedList.end(),
            [](const std::pair<std::string, ProductInfo>& l, const std::pair<std::string, ProductInfo>& r)
        {
            if (l.second.title != r.second.title) {
                return l.second.title < r.second.title;
            }

            return l.first < r.first;
        });
    }

    // When offline, we use dummy product to display UI.
    bool bOnline = (m_networkingConnectivityLevel == XNetworkingConnectivityLevelHint::InternetAccess);

    for (auto& pair : sortedList)
    {
        auto& product = pair.second;

        // Filter other game durable.
        if (product.parentStoreId != m_selectedStoreId && bOnline)
            continue;

        m_durablePanel->AddChild(product.button);

        product.button->GetTypedSubElementById<UIStaticText>(ID("ProductText"))->SetDisplayText(product.title);
        product.button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->SetDisplayText(product.storeId);

        UpdatePackageStatusUI(product);

        DownloadProductImage(product, "Logo", product.button->GetTypedSubElementById<UIImage>(ID("ProductLogo")));

        product.button->ButtonState().AddListenerWhen(UIButton::State::Focused, [this](UIButton* button)
        {
            m_selectedStoreId = button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->GetDisplayText();
            UpdateMenuButtonUI();
        });

        product.button->ButtonState().AddListenerWhen(UIButton::State::Normal, [this](UIButton* button)
        {
            if (m_selectedStoreId == button->GetTypedSubElementById<UIStaticText>(ID("StoreID"))->GetDisplayText())
            {
                m_selectedStoreId = "";
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[DPad] Scroll debug text");
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckLicenseText"))->SetVisible(false);
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetVisible(false);
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("UninstallText"))->SetVisible(false);
            }
        });
    }
}

void Sample::UpdateMenuButtonUI()
{
    // Installed
    if (m_packageList.find(m_selectedStoreId) != m_packageList.end())
    {
        auto& package = m_packageList[m_selectedStoreId];

        m_menuPanel->GetTypedChildById<UIStaticText>(ID("UninstallText"))->SetVisible(true);

        if (package.hasUpdate)
        {
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[A] Update");
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(true);
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetVisible(false);
        }
        else
        {
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetDisplayText("[B] Check selected for update");
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetVisible(true);

            if (package.isGame)
            {
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[A] Launch");
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(true);
            }
            else
            {
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(false);
            }
        }

        m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckLicenseText"))->SetVisible(true);
    }
    else
    {
        ProductInfo* product = FindProduct(m_selectedStoreId);

        m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckUpdateText"))->SetVisible(false);
        m_menuPanel->GetTypedChildById<UIStaticText>(ID("UninstallText"))->SetVisible(false);

        if (product && product->isInUserCollection)
        {
            if (product->hasDigitalDownload)
            {
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[A] Install");
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(true);
            }
            else
            {
                m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(false);
            }
        }
        else
        {
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetDisplayText("[A] Purchase");
            m_menuPanel->GetTypedChildById<UIStaticText>(ID("InstallLaunchText"))->SetVisible(true);
        }

        m_menuPanel->GetTypedChildById<UIStaticText>(ID("CheckLicenseText"))->SetVisible(false);
    }
}

void Sample::UpdatePackageStatusUI(ProductInfo& product)
{
    // installed
    if (m_packageList.find(product.storeId) != m_packageList.end())
    {
        auto& package = m_packageList[product.storeId];

        if (package.hasUpdate)
        {
            product.button->GetTypedSubElementById<UIPanel>(ID("NotPurchasedPanel"))->SetVisible(false);
            product.button->GetTypedSubElementById<UIPanel>(ID("NotInstalledPanel"))->SetVisible(false);
            product.button->GetTypedSubElementById<UIPanel>(ID("UpdateAvailablePanel"))->SetVisible(true);
        }
        else
        {
            product.button->GetTypedSubElementById<UIPanel>(ID("NotPurchasedPanel"))->SetVisible(false);
            product.button->GetTypedSubElementById<UIPanel>(ID("NotInstalledPanel"))->SetVisible(false);
            product.button->GetTypedSubElementById<UIPanel>(ID("UpdateAvailablePanel"))->SetVisible(false);
        }
    }
    else
    {
        // purchased
        if (product.isInUserCollection)
        {
            if (product.hasDigitalDownload)
            {
                // not installed
                product.button->GetTypedSubElementById<UIPanel>(ID("NotPurchasedPanel"))->SetVisible(false);
                product.button->GetTypedSubElementById<UIPanel>(ID("NotInstalledPanel"))->SetVisible(true);
                product.button->GetTypedSubElementById<UIPanel>(ID("UpdateAvailablePanel"))->SetVisible(false);
            }
            else
            {
                // DWOB
                product.button->GetTypedSubElementById<UIPanel>(ID("NotPurchasedPanel"))->SetVisible(false);
                product.button->GetTypedSubElementById<UIPanel>(ID("NotInstalledPanel"))->SetVisible(false);
                product.button->GetTypedSubElementById<UIPanel>(ID("UpdateAvailablePanel"))->SetVisible(false);
            }
        }
        else
        {
            // not purchased
            product.button->GetTypedSubElementById<UIPanel>(ID("NotPurchasedPanel"))->SetVisible(true);
            product.button->GetTypedSubElementById<UIPanel>(ID("NotInstalledPanel"))->SetVisible(false);
            product.button->GetTypedSubElementById<UIPanel>(ID("UpdateAvailablePanel"))->SetVisible(false);
        }
    }
}

void Sample::UpdateDownloadProgressUI(std::shared_ptr<UIButton> button, UINT64 installedBytes, UINT64 totalBytes)
{
    if (installedBytes == 0 && totalBytes == 0)
    {
        button->GetTypedSubElementById<UIPanel>(ID("ProgressPanel"))->SetVisible(false);
    }
    else
    {
        button->GetTypedSubElementById<UIPanel>(ID("ProgressPanel"))->SetVisible(true);

        button->GetTypedSubElementById<UIPanel>(ID("ProgressPanel"))->
            GetTypedSubElementById<UIProgressBar>(ID("ProgressBox"))->
            SetProgressPercentage(static_cast<float>
                (static_cast<double>(installedBytes) / static_cast<double>(totalBytes)));

        char temp[256];
        sprintf_s(temp, "Installing... %lld %%", installedBytes * 100 / totalBytes);
        button->GetTypedSubElementById<UIPanel>(ID("ProgressPanel"))->
            GetTypedSubElementById<UIStaticText>(ID("ProgressText"))->
            SetDisplayText(UIDisplayString(temp));
    }
}

void Sample::DownloadProductImage(const ProductInfo& product, const char* tag, std::shared_ptr<UIImage> imageElement)
{
    // This downloads the image with the provided tag
    // If optional imageElement is passed in, apply image data when downloaded
    for (auto& image : product.images)
    {
        if (image.tag == tag)
        {
            auto async = new XAsyncBlock{};
            async->queue = m_asyncQueue;
            async->context = imageElement.get();
            async->callback = [](XAsyncBlock* async)
            {
                UIImage* imageElement = static_cast<UIImage*>(async->context);

                FileHandle file;
                HRESULT hr = Sample::GetFileDownloader().DownloadFileAsyncResult(async, &file);

                if (FAILED(hr))
                {
                    debugPrint("Error calling DownloadFileAsyncResult: 0x%x\n", hr);
                }
                else
                {
                    if (imageElement)
                    {
                        imageElement->UseTextureData(file->data(), file->size());
                    }
                }

                delete async;
            };

            // image uris just start with //
            std::string uri = "https:" + image.uri;

            GetFileDownloader().DownloadFileAsync(uri, product.storeId + tag, async);
            break;
        }
    }
}
#pragma endregion

void Sample::WritePLS()
{
    HRESULT hr = S_OK;

    size_t pathSize = 0;
    hr = XPersistentLocalStorageGetPathSize(&pathSize);

    if (FAILED(hr))
    {
        debugPrint("XPersistentLocalStorageGetPathSize failed : 0x%08X\n", hr);
        return;
    }

    char* path = new char[pathSize];
    size_t pathUsed = 0;
    hr = XPersistentLocalStorageGetPath(pathSize, path, &pathUsed);

    if (FAILED(hr))
    {
        debugPrint("XPersistentLocalStorageGetPath failed : 0x%08X\n", hr);
        delete[] path;
        return;
    }

#ifdef _GAMING_DESKTOP
    // Create the directory for PLS.
    CreateDirectoryA(path, NULL);
#endif

    std::filesystem::path filePath(path);
    filePath /= "shared.txt";

    HANDLE hFile = CreateFileA(filePath.generic_string<char>().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        debugPrint("Failed to create file in PLS.\n");
        delete[] path;
        return;
    }

    char sharedData[100] = "This text is written by GameHub at ";

    struct tm ct;
    time_t now = time(0);
    localtime_s(&ct, &now);

    char dt[30] = "";
    sprintf_s(dt, "[%u/%02u/%02u %02u:%02u:%02u]", 1900 + ct.tm_year, 1 + ct.tm_mon, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec);
    
    strcat_s(sharedData, dt);

    DWORD writeSize = 0;
    if (WriteFile(hFile, (void*)sharedData, sizeof(sharedData), &writeSize, NULL) == 0)
    {
        debugPrint("Failed to write to PLS : 0x%08X\n", GetLastError());
    }

    CloseHandle(hFile);

    delete[] path;

    debugPrint("Completed writing shared data into PLS.\n");
}
