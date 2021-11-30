//--------------------------------------------------------------------------------------
// Collision.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Collision.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "DebugDraw.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_cameraSpacing = 50.f;

    //--------------------------------------------------------------------------------------
    // Returns the color based on the collision result and the group number.
    // Frustum tests (group 0) return 0, 1, or 2 for outside, partially inside, and fully inside;
    // all other tests return 0 or 1 for no collision or collision.
    //--------------------------------------------------------------------------------------
    inline XMVECTOR GetCollisionColor(ContainmentType collision, int groupnumber)
    {
        // special case: a value of 1 for groups 1 and higher needs to register as a full collision
        if (groupnumber >= 3 && collision > 0)
            collision = CONTAINS;

        switch (collision)
        {
        case DISJOINT:      return ATG::Colors::Green;
        case INTERSECTS:    return ATG::Colors::Orange;
        case CONTAINS:
        default:            return ATG::Colors::White;
        }
    }

    const wchar_t* g_SampleTitle = L"Collision sample";
    const wchar_t* g_SampleDescription = L"This sample demonstrates DirectXMath's collision types";
    const ATG::HelpButtonAssignment g_HelpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Toggle help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::RIGHT_STICK,         L"Orbit X/Y" },
        { ATG::HelpID::RIGHT_STICK_CLICK,   L"Reset view" },
        { ATG::HelpID::DPAD_LEFT,           L"Ray" },
        { ATG::HelpID::DPAD_RIGHT,          L"Axis-aligned box" },
        { ATG::HelpID::DPAD_UP,             L"Frustum" },
        { ATG::HelpID::DPAD_DOWN,           L"Oriented box" },
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_ctrlConnected(false),
    m_showHelp(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    m_camera.SetRadius(25.f);
    m_camera.SetSensitivity(5.f, 1.f, 10.f, .25f);
    m_camera.SetProjectionParameters(XM_PI / 4.f, 0.1f, 1000.f);
    m_camera.SetFlags(DX::OrbitCamera::c_FlagsDisableTranslation
        | DX::OrbitCamera::c_FlagsDisableRollZ
        | DX::OrbitCamera::c_FlagsArrowKeysOrbit
        | DX::OrbitCamera::c_FlagsDisableRadiusControl
        | DX::OrbitCamera::c_FlagsDisableSensitivityControl);

    m_help = std::make_unique<ATG::Help>(g_SampleTitle, g_SampleDescription, g_HelpButtons, _countof(g_HelpButtons));
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    InitializeObjects();

    SetViewForGroup(0);

    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
#ifdef _GAMING_DESKTOP
    m_mouse->SetWindow(window);
#endif

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
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

    // Update position of collision objects.
    Animate(timer.GetTotalSeconds());

    // Compute collisions.
    Collide();

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_ctrlConnected = true;
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showHelp = !m_showHelp;
        }
        else if (m_showHelp && m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showHelp = false;
        }
        else if (!m_showHelp)
        {
            m_camera.Update(elapsedTime, pad);

            if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            {
                SetViewForGroup(0);
            }
            else if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
            {
                SetViewForGroup(1);
            }
            else if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
            {
                SetViewForGroup(2);
            }
            else if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
            {
                SetViewForGroup(3);
            }
        }
    }
    else
    {
        m_ctrlConnected = false;
        m_gamePadButtons.Reset();

        if (!m_showHelp)
            m_camera.Update(elapsedTime, *m_mouse, *m_keyboard);
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    // Keyboard input handling for controller help menu.
    if (m_keyboardButtons.IsKeyPressed(Keyboard::F1))
    {
        m_showHelp = !m_showHelp;
    }
    else if (m_showHelp && kb.Escape)
    {
        m_showHelp = false;
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::Escape))
    {
        ExitSample();
    }
    else if (!m_showHelp)
    {
        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::D1))
        {
            SetViewForGroup(0);
        }
        else if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::D2))
        {
            SetViewForGroup(1);
        }
        else if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::D3))
        {
            SetViewForGroup(2);
        }
        else if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::D4))
        {
            SetViewForGroup(3);
        }
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

    if (m_showHelp)
    {
        // Draw help screen.
        m_help->Render(commandList);
    }
    else
    {
        ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        m_effect->SetView(m_camera.GetView());
        m_effect->SetProjection(m_camera.GetProjection());
        m_effect->Apply(commandList);

        m_batch->Begin(commandList);

        // Draw ground planes
        for (size_t i = 0; i < c_groupCount; ++i)
        {
            static const XMVECTORF32 s_xAxis = { 20.f, 0.f, 0.f, 0.f };
            static const XMVECTORF32 s_yAxis = { 0.f, 0.f, 20.f, 0.f };

            static const XMVECTORF32 s_Offset = { 0.f, 10.f, 0.f, 0.f };
            XMVECTOR origin = XMVectorSubtract(m_cameraOrigins[i], s_Offset);

            DX::DrawGrid(m_batch.get(), s_xAxis, s_yAxis, origin, 20, 20, ATG::Colors::OffWhite);
        }

        // Draw primary collision objects in white
        DX::Draw(m_batch.get(), m_primaryFrustum, ATG::Colors::Blue);
        DX::Draw(m_batch.get(), m_primaryAABox, ATG::Colors::Blue);
        DX::Draw(m_batch.get(), m_primaryOrientedBox, ATG::Colors::Blue);

        {
            XMVECTOR direction = XMVectorScale(m_primaryRay.direction, 10.0f);
            DX::DrawRay(m_batch.get(), m_primaryRay.origin, direction, false, ATG::Colors::LightGrey);
            DX::DrawRay(m_batch.get(), m_primaryRay.origin, direction, false, ATG::Colors::White);
        }

        // Draw secondary collision objects in colors based on collision results
        for (int i = 0; i < static_cast<int>(c_groupCount); ++i)
        {
            const CollisionSphere& sphere = m_secondarySpheres[i];
            XMVECTOR c = GetCollisionColor(sphere.collision, i);
            DX::Draw(m_batch.get(), sphere.sphere, c);

            const CollisionBox& obox = m_secondaryOrientedBoxes[i];
            c = GetCollisionColor(obox.collision, i);
            DX::Draw(m_batch.get(), obox.obox, c);

            const CollisionAABox& aabox = m_secondaryAABoxes[i];
            c = GetCollisionColor(aabox.collision, i);
            DX::Draw(m_batch.get(), aabox.aabox, c);

            const CollisionTriangle& tri = m_secondaryTriangles[i];
            c = GetCollisionColor(tri.collision, i);
            DX::DrawTriangle(m_batch.get(), tri.pointa, tri.pointb, tri.pointc, c);
        }

        // Draw results of ray-object intersection, if there was a hit this frame
        if (m_rayHitResultBox.collision != DISJOINT)
            DX::Draw(m_batch.get(), m_rayHitResultBox.aabox, ATG::Colors::Orange);

        m_batch->End();

        auto rect = m_deviceResources->GetOutputSize();

        auto safeRect = Viewport::ComputeTitleSafeArea(UINT(rect.right), UINT(rect.bottom));

        m_sprites->Begin(commandList);

        m_font->DrawString(m_sprites.get(), m_name.c_str(), XMFLOAT2(float(safeRect.left), float(safeRect.top)), ATG::Colors::White);

        const wchar_t* legend = (m_ctrlConnected) ?
            L"[View] Exit   [Menu] Help"
            : L"Esc - Exit   F1 - Help";

        DX::DrawControllerString(m_sprites.get(),
            m_font.get(), m_ctrlFont.get(),
            legend,
            XMFLOAT2(float(safeRect.left),
                float(safeRect.bottom) - m_font->GetLineSpacing()),
            ATG::Colors::LightGrey);

        m_sprites->End();
    }

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
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

        m_effect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    m_states = std::make_unique<CommonStates>(device);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    ResourceUploadBatch upload(device);

    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(rtState);
        m_sprites = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    wchar_t buff[MAX_PATH] = {};
    DX::FindMediaFile(buff, MAX_PATH, L"SegoeUI_18.spritefont");
    m_font = std::make_unique<SpriteFont>(device, upload, buff,
        m_resourceDescriptors->GetCpuHandle(Descriptors::MyFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::MyFont));

    DX::FindMediaFile(buff, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload, buff,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    // Set help device context.
    m_help->RestoreDevice(device, upload, rtState);

    auto finish = upload.End(m_deviceResources->GetCommandQueue());

    m_deviceResources->WaitForGpu();

    finish.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_sprites->SetViewport(vp);

    auto output = m_deviceResources->GetOutputSize();
    m_camera.SetWindow(output.right - output.left, output.bottom - output.top);

    // Set help rendering size.
    m_help->SetWindow(output);
}

void Sample::OnDeviceLost()
{
    m_states.reset();
    m_effect.reset();
    m_batch.reset();
    m_font.reset();
    m_ctrlFont.reset();
    m_sprites.reset();
    m_resourceDescriptors.reset();

    m_help->ReleaseDevice();

    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::InitializeObjects()
{
    const XMVECTOR c_zero = XMVectorZero();

    // Set up the primary frustum object from a D3D projection matrix
    // NOTE: This can also be done on your camera's projection matrix.  The projection
    // matrix built here is somewhat contrived so it renders well.
    XMMATRIX xmProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.77778f, 0.5f, 10.0f);
    BoundingFrustum::CreateFromMatrix(m_primaryFrustum, xmProj);
    m_primaryFrustum.Origin.z = -7.0f;
    m_cameraOrigins[0] = c_zero;

    // Set up the primary axis aligned box
    m_primaryAABox.Center = XMFLOAT3(c_cameraSpacing, 0, 0);
    m_primaryAABox.Extents = XMFLOAT3(5, 5, 5);
    m_cameraOrigins[1] = Vector3(c_cameraSpacing, 0, 0);

    // Set up the primary oriented box with some rotation
    m_primaryOrientedBox.Center = XMFLOAT3(-c_cameraSpacing, 0, 0);
    m_primaryOrientedBox.Extents = XMFLOAT3(5, 5, 5);
    XMStoreFloat4(&m_primaryOrientedBox.Orientation, XMQuaternionRotationRollPitchYaw(XM_PIDIV4, XM_PIDIV4, 0));
    m_cameraOrigins[2] = Vector3(-c_cameraSpacing, 0, 0);

    // Set up the primary ray
    m_primaryRay.origin = Vector3(0, 0, c_cameraSpacing);
    m_primaryRay.direction = Vector3::UnitZ;
    m_cameraOrigins[3] = Vector3(0, 0, c_cameraSpacing);

    // Initialize all of the secondary objects with default values
    for (size_t i = 0; i < c_groupCount; i++)
    {
        m_secondarySpheres[i].sphere.Radius = 1.0f;
        m_secondarySpheres[i].sphere.Center = XMFLOAT3(0, 0, 0);
        m_secondarySpheres[i].collision = DISJOINT;

        m_secondaryOrientedBoxes[i].obox.Center = XMFLOAT3(0, 0, 0);
        m_secondaryOrientedBoxes[i].obox.Extents = XMFLOAT3(0.5f, 0.5f, 0.5f);
        m_secondaryOrientedBoxes[i].obox.Orientation = XMFLOAT4(0, 0, 0, 1);
        m_secondaryOrientedBoxes[i].collision = DISJOINT;

        m_secondaryAABoxes[i].aabox.Center = XMFLOAT3(0, 0, 0);
        m_secondaryAABoxes[i].aabox.Extents = XMFLOAT3(0.5f, 0.5f, 0.5f);
        m_secondaryAABoxes[i].collision = DISJOINT;

        m_secondaryTriangles[i].pointa = c_zero;
        m_secondaryTriangles[i].pointb = c_zero;
        m_secondaryTriangles[i].pointc = c_zero;
        m_secondaryTriangles[i].collision = DISJOINT;
    }

    // Set up ray hit result box
    m_rayHitResultBox.aabox.Center = XMFLOAT3(0, 0, 0);
    m_rayHitResultBox.aabox.Extents = XMFLOAT3(0.05f, 0.05f, 0.05f);
}

void Sample::Animate(double fTime)
{
    float t = static_cast<float>(fTime * 0.2);

    const float camera0OriginX = XMVectorGetX(m_cameraOrigins[0]);
    const float camera1OriginX = XMVectorGetX(m_cameraOrigins[1]);
    const float camera2OriginX = XMVectorGetX(m_cameraOrigins[2]);
    const float camera3OriginX = XMVectorGetX(m_cameraOrigins[3]);
    const float camera3OriginZ = XMVectorGetZ(m_cameraOrigins[3]);

    // animate sphere 0 around the frustum
    m_secondarySpheres[0].sphere.Center.x = 10.f * sinf(3.f * t);
    m_secondarySpheres[0].sphere.Center.y = 7.f * cosf(5.f * t);

    // animate oriented box 0 around the frustum
    m_secondaryOrientedBoxes[0].obox.Center.x = 8.f * sinf(3.5f * t);
    m_secondaryOrientedBoxes[0].obox.Center.y = 5.f * cosf(5.1f * t);
    XMStoreFloat4(&(m_secondaryOrientedBoxes[0].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 1.4f, t * 0.2f, t));

    // animate aligned box 0 around the frustum
    m_secondaryAABoxes[0].aabox.Center.x = 10.f * sinf(2.1f * t);
    m_secondaryAABoxes[0].aabox.Center.y = 7.f * cosf(3.8f * t);

    // animate sphere 1 around the aligned box
    m_secondarySpheres[1].sphere.Center.x = 8.f * sinf(2.9f * t) + camera1OriginX;
    m_secondarySpheres[1].sphere.Center.y = 8.f * cosf(4.6f * t);
    m_secondarySpheres[1].sphere.Center.z = 8.f * cosf(1.6f * t);

    // animate oriented box 1 around the aligned box
    m_secondaryOrientedBoxes[1].obox.Center.x = 8.f * sinf(3.2f * t) + camera1OriginX;
    m_secondaryOrientedBoxes[1].obox.Center.y = 8.f * cosf(2.1f * t);
    m_secondaryOrientedBoxes[1].obox.Center.z = 8.f * sinf(1.6f * t);
    XMStoreFloat4(&(m_secondaryOrientedBoxes[1].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 0.7f, t * 1.3f, t));

    // animate aligned box 1 around the aligned box
    m_secondaryAABoxes[1].aabox.Center.x = 8.f * sinf(1.1f * t) + camera1OriginX;
    m_secondaryAABoxes[1].aabox.Center.y = 8.f * cosf(5.8f * t);
    m_secondaryAABoxes[1].aabox.Center.z = 8.f * cosf(3.0f * t);

    // animate sphere 2 around the oriented box
    m_secondarySpheres[2].sphere.Center.x = 8.f * sinf(2.2f * t) + camera2OriginX;
    m_secondarySpheres[2].sphere.Center.y = 8.f * cosf(4.3f * t);
    m_secondarySpheres[2].sphere.Center.z = 8.f * cosf(1.8f * t);

    // animate oriented box 2 around the oriented box
    m_secondaryOrientedBoxes[2].obox.Center.x = 8.f * sinf(3.7f * t) + camera2OriginX;
    m_secondaryOrientedBoxes[2].obox.Center.y = 8.f * cosf(2.5f * t);
    m_secondaryOrientedBoxes[2].obox.Center.z = 8.f * sinf(1.1f * t);
    XMStoreFloat4(&(m_secondaryOrientedBoxes[2].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 0.9f, t * 1.8f, t));

    // animate aligned box 2 around the oriented box
    m_secondaryAABoxes[2].aabox.Center.x = 8.f * sinf(1.3f * t) + camera2OriginX;
    m_secondaryAABoxes[2].aabox.Center.y = 8.f * cosf(5.2f * t);
    m_secondaryAABoxes[2].aabox.Center.z = 8.f * cosf(3.5f * t);

    // triangle points in local space - equilateral triangle with radius of 2
    static const XMVECTORF32 TrianglePointA = { 0.f, 2.f, 0.f, 0.f };
    static const XMVECTORF32 TrianglePointB = { 1.732f, -1.f, 0.f, 0.f };
    static const XMVECTORF32 TrianglePointC = { -1.732f, -1.f, 0.f, 0.f };

    // animate triangle 0 around the frustum
    XMMATRIX TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
    XMMATRIX Translation = XMMatrixTranslation(5.f * sinf(5.3f * t) + camera0OriginX, 5.f * cosf(2.3f * t), 5.f * sinf(3.4f * t));
    TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
    m_secondaryTriangles[0].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
    m_secondaryTriangles[0].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
    m_secondaryTriangles[0].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

    // animate triangle 1 around the aligned box
    TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
    Translation = XMMatrixTranslation(8.f * sinf(5.3f * t) + camera1OriginX, 8.f * cosf(2.3f * t), 8.f * sinf(3.4f * t));
    TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
    m_secondaryTriangles[1].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
    m_secondaryTriangles[1].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
    m_secondaryTriangles[1].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

    // animate triangle 2 around the oriented box
    TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
    Translation = XMMatrixTranslation(8.f * sinf(5.3f * t) + camera2OriginX, 8.f * cosf(2.3f * t), 8.f * sinf(3.4f * t));
    TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
    m_secondaryTriangles[2].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
    m_secondaryTriangles[2].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
    m_secondaryTriangles[2].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);

    // animate primary ray (this is the only animated primary object)
    m_primaryRay.direction = Vector3(sinf(t * 3.f), 0.f, cosf(t * 3.f));

    // animate sphere 3 around the ray
    m_secondarySpheres[3].sphere.Center = XMFLOAT3(camera3OriginX - 3.f, 0.5f * sinf(t * 5.f), camera3OriginZ);

    // animate aligned box 3 around the ray
    m_secondaryAABoxes[3].aabox.Center = XMFLOAT3(camera3OriginX + 3.f, 0.5f * sinf(t * 4.f), camera3OriginZ);

    // animate oriented box 3 around the ray
    m_secondaryOrientedBoxes[3].obox.Center = XMFLOAT3(camera3OriginX, 0.5f * sinf(t * 4.5f), camera3OriginZ + 3.f);
    XMStoreFloat4(&(m_secondaryOrientedBoxes[3].obox.Orientation), XMQuaternionRotationRollPitchYaw(t * 0.9f, t * 1.8f, t));

    // animate triangle 3 around the ray
    TriangleCoords = XMMatrixRotationRollPitchYaw(t * 1.4f, t * 2.5f, t);
    Translation = XMMatrixTranslation(camera3OriginX, 0.5f * cosf(4.3f * t), camera3OriginZ - 3.f);
    TriangleCoords = XMMatrixMultiply(TriangleCoords, Translation);
    m_secondaryTriangles[3].pointa = XMVector3Transform(TrianglePointA, TriangleCoords);
    m_secondaryTriangles[3].pointb = XMVector3Transform(TrianglePointB, TriangleCoords);
    m_secondaryTriangles[3].pointc = XMVector3Transform(TrianglePointC, TriangleCoords);
}

void Sample::Collide()
{
    // test collisions between objects and frustum
    m_secondarySpheres[0].collision = m_primaryFrustum.Contains(m_secondarySpheres[0].sphere);
    m_secondaryOrientedBoxes[0].collision = m_primaryFrustum.Contains(m_secondaryOrientedBoxes[0].obox);
    m_secondaryAABoxes[0].collision = m_primaryFrustum.Contains(m_secondaryAABoxes[0].aabox);
    m_secondaryTriangles[0].collision = m_primaryFrustum.Contains(m_secondaryTriangles[0].pointa,
        m_secondaryTriangles[0].pointb,
        m_secondaryTriangles[0].pointc);

    // test collisions between objects and aligned box
    m_secondarySpheres[1].collision = m_primaryAABox.Contains(m_secondarySpheres[1].sphere);
    m_secondaryOrientedBoxes[1].collision = m_primaryAABox.Contains(m_secondaryOrientedBoxes[1].obox);
    m_secondaryAABoxes[1].collision = m_primaryAABox.Contains(m_secondaryAABoxes[1].aabox);
    m_secondaryTriangles[1].collision = m_primaryAABox.Contains(m_secondaryTriangles[1].pointa,
        m_secondaryTriangles[1].pointb,
        m_secondaryTriangles[1].pointc);

    // test collisions between objects and oriented box
    m_secondarySpheres[2].collision = m_primaryOrientedBox.Contains(m_secondarySpheres[2].sphere);
    m_secondaryOrientedBoxes[2].collision = m_primaryOrientedBox.Contains(m_secondaryOrientedBoxes[2].obox);
    m_secondaryAABoxes[2].collision = m_primaryOrientedBox.Contains(m_secondaryAABoxes[2].aabox);
    m_secondaryTriangles[2].collision = m_primaryOrientedBox.Contains(m_secondaryTriangles[2].pointa,
        m_secondaryTriangles[2].pointb,
        m_secondaryTriangles[2].pointc);

    // test collisions between objects and ray
    float fDistance = -1.0f;

    float fDist;
    if (m_secondarySpheres[3].sphere.Intersects(m_primaryRay.origin, m_primaryRay.direction, fDist))
    {
        fDistance = fDist;
        m_secondarySpheres[3].collision = INTERSECTS;
    }
    else
        m_secondarySpheres[3].collision = DISJOINT;

    if (m_secondaryOrientedBoxes[3].obox.Intersects(m_primaryRay.origin, m_primaryRay.direction, fDist))
    {
        fDistance = fDist;
        m_secondaryOrientedBoxes[3].collision = INTERSECTS;
    }
    else
        m_secondaryOrientedBoxes[3].collision = DISJOINT;

    if (m_secondaryAABoxes[3].aabox.Intersects(m_primaryRay.origin, m_primaryRay.direction, fDist))
    {
        fDistance = fDist;
        m_secondaryAABoxes[3].collision = INTERSECTS;
    }
    else
        m_secondaryAABoxes[3].collision = DISJOINT;

    if (TriangleTests::Intersects(m_primaryRay.origin, m_primaryRay.direction,
        m_secondaryTriangles[3].pointa,
        m_secondaryTriangles[3].pointb,
        m_secondaryTriangles[3].pointc,
        fDist))
    {
        fDistance = fDist;
        m_secondaryTriangles[3].collision = INTERSECTS;
    }
    else
        m_secondaryTriangles[3].collision = DISJOINT;

    // If one of the ray intersection tests was successful, fDistance will be positive.
    // If so, compute the intersection location and store it in g_RayHitResultBox.
    if (fDistance > 0)
    {
        // The primary ray's direction is assumed to be normalized.
        XMVECTOR HitLocation = XMVectorMultiplyAdd(m_primaryRay.direction, XMVectorReplicate(fDistance),
            m_primaryRay.origin);
        XMStoreFloat3(&m_rayHitResultBox.aabox.Center, HitLocation);
        m_rayHitResultBox.collision = INTERSECTS;
    }
    else
    {
        m_rayHitResultBox.collision = DISJOINT;
    }
}

//--------------------------------------------------------------------------------------
// Sets the camera to view a particular group of objects
//--------------------------------------------------------------------------------------
void Sample::SetViewForGroup(int group)
{
    assert(static_cast<size_t>(group) < c_groupCount);

    m_camera.SetFocus(m_cameraOrigins[group]);
    m_camera.SetRotation(XMQuaternionRotationRollPitchYaw(-XM_PI / 4.f, 0.f, 0.f));

    switch (group)
    {
    default: m_name = L"Frustum"; break;
    case 1: m_name = L"Axis-aligned box"; break;
    case 2: m_name = L"Oriented box"; break;
    case 3: m_name = L"Ray"; break;
    }
}
