//--------------------------------------------------------------------------------------
// AdvancedSpatialSounds.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedSpatialSounds.h"
#include "..\Audio\WAVFileReader.h"

#include "ATGColors.h"
#include "ControllerFont.h"

#define MAX_X   100
#define MIN_X   -100
#define MAX_Y   100
#define MIN_Y   -100
#define MAX_Z   100
#define MIN_Z   -100
#define MAX_VEL 3
#define MAX_RADIUS  90
#define MIN_RADIUS  10
#define CURVE_START 30
#define CURVE_END   170
#define DRAW_BACK_WALL .3f
#define DRAW_POINT_SCALE 40.f
#define DRAW_POINT_MIN_SCALE 2.f

namespace
{
    const LPCWSTR g_bedFileList[] = {
        L"assets\\Jungle_RainThunder_5.1_mixdown.wav",
        L"assets\\Jungle_RainThunder_SideSurroundL-R.wav",
        L"assets\\Jungle_RainThunder_TopFrontL-R.wav",
        L"assets\\Jungle_RainThunder_TopRearL-R.wav",
    };

    const LPCWSTR g_pointFileList[] = {
        L"assets\\SFX_height_birdHawk01.wav",
        L"assets\\SFX_height_birdLoon01.wav",
        L"assets\\SFX_moving_BirdFlap01.wav",
        L"assets\\SFX_moving_birdFlicker01.wav",
        L"assets\\SFX_moving_birdFlycatcher01.wav",
        L"assets\\SFX_moving_birdLark01.wav",
        L"assets\\SFX_moving_birdLoop01.wav",
        L"assets\\SFX_moving_birdLoop02.wav",
        L"assets\\SFX_moving_birdLoop03.wav",
        L"assets\\SFX_moving_birdLoop04.wav",
        L"assets\\SFX_moving_birdLoop05.wav",
        L"assets\\SFX_moving_birdLoop06.wav",
        L"assets\\SFX_moving_birdSparrow01.wav",
        L"assets\\SFX_moving_birdSparrow02.wav",
        L"assets\\SFX_moving_birdWarbler01.wav",
        L"assets\\SFX_moving_Fly01.wav",
        L"assets\\SFX_moving_Fly02.wav",
        L"assets\\SFX_stationary_cicada01.wav",
        L"assets\\SFX_stationary_grasshopper01.wav",
        L"assets\\SFX_stationary_grasshopper02.wav",
    };
}

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    VOID CALLBACK SpatialWorkCallback(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WORK)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		DX::ThrowIfFailed(hr);

        auto Sink = reinterpret_cast<Sample *>(Context);

        while (Sink->m_bThreadActive)
        {
			while (Sink->m_bPlayingSound && Sink->m_Renderer->IsActive())
			{
				// Wait for a signal from the audio-engine to start the next processing pass 
				if (WaitForSingleObject(Sink->m_Renderer->m_bufferCompletionEvent, 100) != WAIT_OBJECT_0)
				{
					//make a call to stream to see why we didn't get a signal after 100ms
					hr = Sink->m_Renderer->m_SpatialAudioStream->Reset();

					//if we have an error, tell the renderer to reset
					if (FAILED(hr))
					{
						Sink->m_Renderer->Reset();
					}
					continue;
				}

				UINT32 frameCount;
				UINT32 availableObjectCount;

				// Begin the process of sending object data and metadata 
				// Get the number of active object that can be used to send object-data 
				// Get the number of frame count that each buffer be filled with  
				hr = Sink->m_Renderer->m_SpatialAudioStream->BeginUpdatingAudioObjects(
					&availableObjectCount,
					&frameCount);
				//if we have an error, tell the renderer to reset
				if (FAILED(hr))
				{
					Sink->m_Renderer->Reset();
					continue;
				}

				Sink->m_availableObjects = static_cast<int>(availableObjectCount);

				for (int chan = 0; chan < MAX_CHANNELS; chan++)
				{
					//Activate the object if not yet done
					if (Sink->m_bedChannels[chan].object == nullptr)
					{
						// If this method called more than activeObjectCount times 
						// It will fail with this error HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) 
						hr = Sink->m_Renderer->m_SpatialAudioStream->ActivateSpatialAudioObject(
							Sink->m_bedChannels[chan].objType,
							&Sink->m_bedChannels[chan].object);
						if (FAILED(hr))
						{
							continue;
						}

					}

					//Get the object buffer
					BYTE* buffer = nullptr;
					UINT32 bytecount = 0;
					hr = Sink->m_bedChannels[chan].object->GetBuffer(&buffer, &bytecount);
					if (FAILED(hr))
					{
						continue;
					}

					Sink->m_bedChannels[chan].object->SetVolume(Sink->m_bedChannels[chan].volume);

					for (UINT32 i = 0; i < bytecount; i++)
					{
						UINT32 fileLoc = Sink->m_bedChannels[chan].curBufferLoc;
						if (chan < Sink->m_numChannels)
						{
							buffer[i] = Sink->m_bedChannels[chan].wavBuffer[fileLoc];
						}
						else
						{
							buffer[i] = 0;
						}

						Sink->m_bedChannels[chan].curBufferLoc++;
						if (Sink->m_bedChannels[chan].curBufferLoc == Sink->m_bedChannels[chan].bufferSize)
						{
							Sink->m_bedChannels[chan].curBufferLoc = 0;
						}


					}

				}
				//Update the point sounds
				bool deleteSound = false;

				//Lock to ensure that we are only updating spatial objects one at a time
				std::lock_guard<std::mutex> lock(Sink->objectUpdateLock);
				{
					for (std::vector<PointSound>::iterator it = Sink->m_pointSounds.begin(); it != Sink->m_pointSounds.end(); it++)
					{
						//Activate the object if not yet done
						if (it->object == nullptr)
						{
							// If this method called more than activeObjectCount times 
							// It will fail with this error HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) 
							hr = Sink->m_Renderer->m_SpatialAudioStream->ActivateSpatialAudioObject(
								AudioObjectType_Dynamic,
								&it->object);
							if (FAILED(hr))
							{
								continue;
							}
						}

						//Get the object buffer
						BYTE* buffer = nullptr;
						UINT32 bytecount;
						hr = it->object->GetBuffer(&buffer, &bytecount);
						if (FAILED(hr))
						{
							continue;
						}

						if (!it->isPlaying)
						{
							// Set end of stream for the last buffer  
							hr = it->object->SetEndOfStream(0);

							// Last block of data in the object, release the audio object, 
							// so the resources can be recycled and used for another object 
							it->object = nullptr;
							it->curBufferLoc = 0;

							deleteSound = true;
						}
						else
						{
							it->object->SetPosition(it->posX, it->posY, it->posZ);
							it->object->SetVolume(it->volume);

							for (UINT32 i = 0; i < frameCount * 4; i++)
							{
								UINT32 fileLoc = it->curBufferLoc;
								buffer[i] = it->wavBuffer[fileLoc];
								it->curBufferLoc++;
								if (it->curBufferLoc == it->bufferSize)
								{
									it->curBufferLoc = 0;
								}
							}
						}
					}

					if (deleteSound)
					{
						Sink->m_pointSounds.pop_back();
					}

					// Let the audio-engine know that the object data are available for processing now 
					hr = Sink->m_Renderer->m_SpatialAudioStream->EndUpdatingAudioObjects();
					if (FAILED(hr))
					{
						Sink->m_Renderer->Reset();
					}
				}
			} //end lock
        }
    }
}

Sample::Sample() noexcept(false) :
	m_Renderer(nullptr),
	m_bThreadActive(false),
    m_bPlayingSound(false),
	m_bedChannels{},
    m_numChannels(0),
    m_availableObjects(0),
    m_usedObjects(0),
    m_frame(0),
    m_fileLoaded(false),
	m_workThread(nullptr)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
	if (m_deviceResources)
	{
		m_deviceResources->WaitForGpu();
	}

	if (m_workThread)
	{
		//stop and shutdown spatial worker thread
		m_bThreadActive = false;
		m_bPlayingSound = false;
		WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
		CloseThreadpoolWork(m_workThread);
		m_workThread = nullptr;
	}
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    InitializeSpatialStream();

    srand((unsigned int)time(nullptr));

    m_boundingBox = BoundingBox(XMFLOAT3(), XMFLOAT3((MAX_X - MIN_X) / 2, (MAX_Y - MIN_Y) / 2, (MAX_Z - MIN_Z) / 2));

    for (UINT32 i = 0; i < MAX_CHANNELS; i++)
    {
        m_bedChannels[i].bufferSize = 0;
        m_bedChannels[i].curBufferLoc = 0;
        m_bedChannels[i].volume = 1.f;
        m_bedChannels[i].object = nullptr;
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    m_frame++;
}


//--------------------------------------------------------------------------------------
// Name: DrawRoom()
// Desc: Draws the room
//--------------------------------------------------------------------------------------
void XM_CALLCONV Sample::DrawRoom(ID3D12GraphicsCommandList* commandList, FXMVECTOR color)
{
	XMVECTORF32 points[4];
	points[0] = { -DRAW_BACK_WALL, -DRAW_BACK_WALL, 0.f };
	points[1] = { -DRAW_BACK_WALL, DRAW_BACK_WALL, 0.f };
	points[2] = { DRAW_BACK_WALL, DRAW_BACK_WALL, 0.f };
	points[3] = { DRAW_BACK_WALL, -DRAW_BACK_WALL, 0.f };

	XMVECTORF32 edgePoints[4];
	edgePoints[0] = { -1.f, -1.f, 0.f };
	edgePoints[1] = { -1.f, 1.f, 0.f };
	edgePoints[2] = { 1.f, 1.f, 0.f };
	edgePoints[3] = { 1.f, -1.f, 0.f };

	VertexPositionColor v[4];
	v[0] = VertexPositionColor(points[0], color);
	v[1] = VertexPositionColor(points[1], color);
	v[2] = VertexPositionColor(points[2], color);
	v[3] = VertexPositionColor(points[3], color);

	VertexPositionColor edge[4];
	edge[0] = VertexPositionColor(edgePoints[0], color);
	edge[1] = VertexPositionColor(edgePoints[1], color);
	edge[2] = VertexPositionColor(edgePoints[2], color);
	edge[3] = VertexPositionColor(edgePoints[3], color);

	m_batchEffectRoom->Apply(commandList);
	m_batch->Begin(commandList);

	m_batch->DrawLine(v[0], v[1]);
	m_batch->DrawLine(v[1], v[2]);
	m_batch->DrawLine(v[2], v[3]);
	m_batch->DrawLine(v[3], v[0]);

    for (size_t i = 0; i <= 4; ++i)
    {
        m_batch->DrawLine(v[i], edge[i]);
    }

    m_batch->End();
}


//--------------------------------------------------------------------------------------
// Name: DrawSound()
// Desc: Draws a sound point
//--------------------------------------------------------------------------------------
void XM_CALLCONV Sample::DrawSound(float x, float y, float z, FXMVECTOR color)
{
    //Z MIN = 0, MAX = 1
    x = (x - MIN_X) / (MAX_X - MIN_X);
    y = (y - MIN_Y) / (MAX_Y - MIN_Y);
    z = (z - MIN_Z) / (MAX_Z - MIN_Z);
    float scale = z * DRAW_POINT_SCALE + DRAW_POINT_MIN_SCALE;

    RECT drawspace = m_deviceResources->GetOutputSize();

    float backWallWidth = DRAW_BACK_WALL * drawspace.right;
    float backWallHeight = DRAW_BACK_WALL * drawspace.bottom;

    float lowBoundX = (drawspace.right - backWallWidth) * (1 - z) / 2;
    float lowBoundY = (drawspace.bottom - backWallHeight) * (1 - z) / 2;
    float highBoundX = drawspace.right - lowBoundX;
    float highBoundY = drawspace.bottom - lowBoundY;

    x = x * (highBoundX - lowBoundX) + lowBoundX;
    y = y * (highBoundY - lowBoundY) + lowBoundY;

    drawspace.bottom = (LONG)(y + scale);
    drawspace.top = (LONG)y;
    drawspace.left = (LONG)x;
    drawspace.right = (LONG)(x + scale);

	auto circleSize = GetTextureSize(m_circleTexture.Get());
	m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Sound), circleSize, drawspace,	color);
}


//--------------------------------------------------------------------------------------
// Name: DrawListener()
// Desc: Draws a sound point
//--------------------------------------------------------------------------------------
void XM_CALLCONV Sample::DrawListener(ID3D12GraphicsCommandList* commandList, FXMVECTOR color)
{
	float scale = .035f;

	XMVECTORF32 points[3];
	points[0] = { -scale, -scale, 0 };
	points[1] = { scale, -scale, 0 };
	points[2] = { 0, .014f, 0 };

	VertexPositionColor v[3];
	v[0] = VertexPositionColor(points[0], color);
	v[1] = VertexPositionColor(points[1], color);
	v[2] = VertexPositionColor(points[2], color);

	m_batchEffectListener->Apply(commandList);

	m_batch->Begin(commandList);
    m_batch->DrawTriangle(v[0], v[1], v[2]);

    m_batch->End();
}


// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    //Are we resetting the renderer?  This will happen if we get an invalid stream 
    //  which can happen when render mode changes or device changes
    if (m_Renderer->IsResetting())
    {
        // Initialize Default Audio Device
        m_Renderer->InitializeAudioDevice();

        //Reset all the Objects that were being used
        for (int chan = 0; chan < MAX_CHANNELS; chan++)
        {
            m_bedChannels[chan].object = nullptr;
        }
        for (std::vector<PointSound>::iterator it = m_pointSounds.begin(); it != m_pointSounds.end(); it++)
        {
            it->object = nullptr;
        }
    }
    else if (m_Renderer->IsActive() && m_pointSounds.size() > m_Renderer->GetMaxDynamicObjects())
    {
        //If we reactivated or available object changed and had more active objects than we do now, clear out those we cannot render
        while (m_pointSounds.size() > m_Renderer->GetMaxDynamicObjects())
        {
            m_pointSounds.pop_back();
            m_usedObjects--;
        }
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        if (m_gamePadButtons.a == m_gamePadButtons.RELEASED)
        {
            //Start spatial worker thread
            if (!m_bThreadActive)
            {
                //(re)load file to start again
                m_fileLoaded = LoadBed();

                //startup spatial thread
                m_bThreadActive = true;
                m_bPlayingSound = true;
                m_workThread = CreateThreadpoolWork(SpatialWorkCallback, this, nullptr);
                SubmitThreadpoolWork(m_workThread);
            }
            else
            {
                //stop and shutdown spatial worker thread
                m_bThreadActive = false;
                m_bPlayingSound = false;
                WaitForThreadpoolWorkCallbacks(m_workThread, FALSE);
                CloseThreadpoolWork(m_workThread);
				m_workThread = nullptr;
            }

        }
        if (m_gamePadButtons.dpadUp == m_gamePadButtons.RELEASED)
        {
            if (m_usedObjects < m_availableObjects && m_bThreadActive && m_bPlayingSound)
			{
                PointSound tempChannel;
                int randIndex = rand() % int(_countof(g_pointFileList));
                if (LoadPointFile(g_pointFileList[randIndex], &tempChannel))
                {
                    tempChannel.soundIndex = randIndex;
                    tempChannel.travelData.travelType = TravelType(rand() % 3);
                    tempChannel.travelData.boundingBox = m_boundingBox;
                    tempChannel.travelData.vel = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_VEL);
                    tempChannel.posX = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_X - MIN_X);
                    tempChannel.posY = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_Y - MIN_Y);
                    tempChannel.posZ = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_Z - MIN_Z);

                    if (tempChannel.travelData.travelType == Round)
                    {
                        tempChannel.travelData.radius = static_cast <float> (rand() % (MAX_RADIUS - MIN_RADIUS) + MIN_RADIUS);
                    }
                    else
                    {
                        float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_X - MIN_X);
                        float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_Y - MIN_Y);
                        float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX / MAX_Z - MIN_Z);

                        XMVECTOR temp = { x, y, z };
                        temp = XMVector3Normalize(temp);
                        XMStoreFloat3(&tempChannel.travelData.direction, temp);
                    }

                    tempChannel.isPlaying = true;

                    std::lock_guard<std::mutex> lock(objectUpdateLock);
                    m_pointSounds.emplace_back(tempChannel);

                    m_usedObjects++;
                }
            }

        }
        else if (m_gamePadButtons.dpadDown == m_gamePadButtons.RELEASED)
        {
            if (m_pointSounds.size() > 0)
            {
                m_pointSounds.back().isPlaying = false;
                m_usedObjects--;
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    //Update the point sounds
    for (std::vector<PointSound>::iterator it = m_pointSounds.begin(); it != m_pointSounds.end(); it++)
    {
        switch (it->travelData.travelType)
        {
        case Linear:
            LinearTravel(&(*it));
            break;
        case Bounce:
            BounceTravel(&(*it));
            break;
        case Round:
            RoundTravel(&(*it));
            break;
        }

        float distance = sqrtf(powf(it->posX, 2.0) + powf(it->posY, 2.0) + powf(it->posZ, 2.0));
        float volume = 1.f - ((distance - CURVE_START) / (CURVE_END - CURVE_START));

        if (volume < 0)
        {
            it->volume = 0;
        }
        else if (volume > 1.f)
        {
            it->volume = 1.f;
        }
        else
        {
            it->volume = volume;
        }
    }
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    float spacing = m_font->GetLineSpacing();

    m_font->DrawString(m_spriteBatch.get(), L"Advanced Spatial Playback", pos, ATG::Colors::White);
    pos.y += spacing * 1.5f;
    if (!m_Renderer->IsActive())
    {
        m_font->DrawString(m_spriteBatch.get(), L"Spatial Renderer Not Available", pos, ATG::Colors::White);
        pos.y += spacing * 2.f;
    }
    else
    {
        wchar_t str[256] = {};
        swprintf_s(str, L"State: %ls", ((m_bPlayingSound) ? L"Playing" : L"Stopped"));
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
        pos.y += spacing * 1.5f;

        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [A] button to start/stop playback", pos, ATG::Colors::White);
        pos.y += spacing;

        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [Dpad] UP/DOWN to add/remove a sound", pos, ATG::Colors::White);
        pos.y += spacing;

        DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Use [View] to exit", pos, ATG::Colors::White);
        pos.y += spacing * 2.f;

        swprintf_s(str, L"Available Dynamic Objects: %d", m_availableObjects - m_usedObjects);
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
        pos.y += spacing;

        swprintf_s(str, L"Used Dynamic Objects: %d", m_usedObjects);
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
        pos.y += spacing;

        swprintf_s(str, L"Total Objects: %d", m_usedObjects + MAX_CHANNELS);
        m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);
    }

    DrawRoom(commandList, Colors::Green);

    for (std::vector<PointSound>::iterator it = m_pointSounds.begin(); it != m_pointSounds.end(); it++)
    {
        if (it->posZ < 0)
        {
            DrawSound(it->posX, it->posY, it->posZ, Colors::Blue);
        }
    }

    DrawListener(commandList, Colors::Yellow);

    for (std::vector<PointSound>::iterator it = m_pointSounds.begin(); it != m_pointSounds.end(); it++)
    {
        if (it->posZ >= 0)
        {
            DrawSound(it->posX, it->posY, it->posZ, Colors::Blue);
        }
    }

    m_spriteBatch->End();


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
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
	commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Don't need to clear color as the sample draws a fullscreen image background

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
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
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

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

	{
		D3D12_RASTERIZER_DESC state = CommonStates::CullNone;
		state.MultisampleEnable = FALSE;
		EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, state, rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);
	}

	{
		EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, CommonStates::CullNone, rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		m_batchEffectRoom = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
	}

	{
		EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, CommonStates::CullNone, rtState);
		m_batchEffectListener = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
	}

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

	m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
		L"XboxOneControllerLegendSmall.spritefont",
		m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
		m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

	DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"callout_circle.dds", m_circleTexture.ReleaseAndGetAddressOf()));

	auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

	CreateShaderResourceView(device, m_circleTexture.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Sound));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion


HRESULT Sample::InitializeSpatialStream(void)
{
    HRESULT hr = S_OK;

    if (!m_Renderer)
    {
        // Create a new ISAC instance
        m_Renderer = new ISACRenderer();
        // Selects the Default Audio Device
        hr = m_Renderer->InitializeAudioDevice();
    }

    return hr;
}

bool Sample::LoadBed()
{
    for (UINT32 i = 0; i < MAX_CHANNELS; i++)
    {
        if (m_bedChannels[i].bufferSize)
        {
            delete[] m_bedChannels[i].wavBuffer;
        }
        m_bedChannels[i].bufferSize = 0;
        m_bedChannels[i].curBufferLoc = 0;
        m_bedChannels[i].volume = 1.f;
    }
    
    m_numChannels = 0;

    HRESULT hr = S_OK;
    std::unique_ptr<uint8_t[]> m_waveFile;
	DirectX::WAVData  WavData;
    LPCWSTR inFile;
    int channelCount = 0;

    for (size_t fileIndex = 0; fileIndex < _countof(g_bedFileList); fileIndex++)
    {
        inFile = g_bedFileList[fileIndex];

        hr = DirectX::LoadWAVAudioFromFileEx(inFile, m_waveFile, WavData);
        if (FAILED(hr))
        {
            return false;
        }

        if ((WavData.wfx->wFormatTag == 1 || WavData.wfx->wFormatTag == 65534) && WavData.wfx->nSamplesPerSec == 48000)
        {
            m_numChannels += WavData.wfx->nChannels;

            unsigned numSamples = WavData.audioBytes / (2u * WavData.wfx->nChannels);
            for (int i = 0; i < WavData.wfx->nChannels; i++)
            {
                m_bedChannels[channelCount + i].wavBuffer = new BYTE[numSamples * 4];
                m_bedChannels[channelCount + i].bufferSize = numSamples * 4;
            }

            float * tempnew;
            short * tempdata = (short *)WavData.startAudio;

            for (unsigned i = 0; i < numSamples; i++)
            {
                for (int j = 0; j < WavData.wfx->nChannels; j++)
                {
                    tempnew = (float *)m_bedChannels[channelCount + j].wavBuffer;
                    tempnew[i] = (float)tempdata[(i * WavData.wfx->nChannels) + j] / 32768;
                }
            }

            channelCount += WavData.wfx->nChannels;
        }
        else if ((WavData.wfx->wFormatTag == 3) && WavData.wfx->nSamplesPerSec == 48000)
        {
            m_numChannels += WavData.wfx->nChannels;

            unsigned numSamples = WavData.audioBytes / (4u * WavData.wfx->nChannels);
            for (int i = 0; i < WavData.wfx->nChannels; i++)
            {
                m_bedChannels[channelCount + i].wavBuffer = new BYTE[numSamples * 4];
                m_bedChannels[channelCount + i].bufferSize = numSamples * 4;
            }

            float * tempnew;
            short * tempdata = (short *)WavData.startAudio;

            for (unsigned i = 0; i < numSamples; i++)
            {
                for (int j = 0; j < WavData.wfx->nChannels; j++)
                {
                    tempnew = (float *)m_bedChannels[j].wavBuffer;
                    tempnew[i] = (float)tempdata[(i * WavData.wfx->nChannels) + j];
                }
            }

            channelCount += WavData.wfx->nChannels;
        }
        else
        {
            return false;
        }
    }

    m_bedChannels[0].objType = AudioObjectType_FrontLeft;
    m_bedChannels[1].objType = AudioObjectType_FrontRight;
    m_bedChannels[2].objType = AudioObjectType_FrontCenter;
    m_bedChannels[3].objType = AudioObjectType_LowFrequency;
    m_bedChannels[4].objType = AudioObjectType_BackLeft;
    m_bedChannels[5].objType = AudioObjectType_BackRight;
    m_bedChannels[6].objType = AudioObjectType_SideLeft;
    m_bedChannels[7].objType = AudioObjectType_SideRight;
    m_bedChannels[8].objType = AudioObjectType_TopFrontLeft;
    m_bedChannels[9].objType = AudioObjectType_TopFrontRight;
    m_bedChannels[10].objType = AudioObjectType_TopBackLeft;
    m_bedChannels[11].objType = AudioObjectType_TopBackRight;

    return true;
}

bool Sample::LoadPointFile(LPCWSTR inFile, PointSound* outChannel)
{
    std::unique_ptr<uint8_t[]> m_waveFile;
    DirectX::WAVData  WavData;

    outChannel->bufferSize = 0;
    outChannel->curBufferLoc = 0;
    outChannel->volume = 1.f;

    if (DirectX::LoadWAVAudioFromFileEx(inFile, m_waveFile, WavData))
    {
        return false;
    }

    if ((WavData.wfx->wFormatTag == 1 || WavData.wfx->wFormatTag == 65534) && WavData.wfx->nSamplesPerSec == 48000 && WavData.wfx->nChannels == 1)
    {
        unsigned numSamples = WavData.audioBytes / 2u;
        outChannel->wavBuffer = new BYTE[numSamples * 4];
        outChannel->bufferSize = numSamples * 4;

        float * tempnew;
        short * tempdata = (short *)WavData.startAudio;

        for (unsigned i = 0; i < numSamples; i++)
        {
            tempnew = (float *)outChannel->wavBuffer;
            tempnew[i] = (float)tempdata[i] / 32768;
        }
    }
    else if ((WavData.wfx->wFormatTag == 3) && WavData.wfx->nSamplesPerSec == 48000 && WavData.wfx->nChannels == 1)
    {
        unsigned numSamples = WavData.audioBytes / 4u;
        outChannel->wavBuffer = new BYTE[numSamples * 4];
        outChannel->bufferSize = numSamples * 4;

        float * tempnew;
        float * tempdata = (float *)WavData.startAudio;

        for (unsigned i = 0; i < numSamples; i++)
        {
            tempnew = (float *)outChannel->wavBuffer;
            tempnew[i] = (float)tempdata[i];
        }
    }
    else
    {
        return false;
    }

    return true;
}

void Sample::LinearTravel(PointSound* inSound)
{
    //Travel in one direction until hitting a wall, then reverse
    XMVECTOR staringPosition = { inSound->posX, inSound->posY, inSound->posZ };
    XMVECTOR direction = XMLoadFloat3(&inSound->travelData.direction);
    XMVECTOR newPoint = XMVectorAdd(XMVectorScale(direction, inSound->travelData.vel), staringPosition);

    if (inSound->travelData.boundingBox.Contains(newPoint) == DISJOINT)
    {
        //Find the intersection point
        float distance = 0;
        XMVECTOR newDirection = XMVectorNegate(direction);
        inSound->travelData.boundingBox.Intersects(newPoint, newDirection, distance);
        XMVECTOR intersectPoint = XMVectorMultiplyAdd(direction, XMVectorReplicate(inSound->travelData.vel - distance), staringPosition);

        //Bounce back the way we came
        newPoint = XMVectorAdd(XMVectorScale(newDirection, distance), intersectPoint);
        XMStoreFloat3(&inSound->travelData.direction, newDirection);
    }

    inSound->posX = XMVectorGetX(newPoint);
    inSound->posY = XMVectorGetY(newPoint);
    inSound->posZ = XMVectorGetZ(newPoint);
}

void Sample::BounceTravel(PointSound* inSound)
{
    //Travel in one direction until hitting a wall, then bounce
    XMVECTOR staringPosition = { inSound->posX, inSound->posY, inSound->posZ };
    XMVECTOR direction = XMLoadFloat3(&inSound->travelData.direction);
    XMVECTOR newPoint = XMVectorAdd(XMVectorScale(direction, inSound->travelData.vel), staringPosition);

    if (inSound->travelData.boundingBox.Contains(newPoint) == DISJOINT)
    {
        //Find the intersection point
        float distance = 0;
        XMVECTOR newDirection = XMVectorNegate(direction);
        inSound->travelData.boundingBox.Intersects(newPoint, newDirection, distance);
        XMVECTOR intersectPoint = XMVectorMultiplyAdd(direction, XMVectorReplicate(inSound->travelData.vel - distance), staringPosition);

        //Build the vector to multiply
        XMFLOAT3 tempPoint;
        XMStoreFloat3(&tempPoint, intersectPoint);

        float checkPointHigh = inSound->travelData.boundingBox.Center.x + inSound->travelData.boundingBox.Extents.x;
        float checkPointLow = inSound->travelData.boundingBox.Center.x - inSound->travelData.boundingBox.Extents.x;

        if (tempPoint.x == checkPointHigh || tempPoint.x == checkPointLow)
        {
            tempPoint.x = -1.f;
        }
        else
        {
            tempPoint.x = 1.f;
        }

        checkPointHigh = inSound->travelData.boundingBox.Center.x + inSound->travelData.boundingBox.Extents.x;
        checkPointLow = inSound->travelData.boundingBox.Center.x - inSound->travelData.boundingBox.Extents.x;

        if (tempPoint.y == checkPointHigh || tempPoint.y == checkPointLow)
        {
            tempPoint.y = -1.f;
        }
        else
        {
            tempPoint.y = 1.f;
        }

        checkPointHigh = inSound->travelData.boundingBox.Center.x + inSound->travelData.boundingBox.Extents.x;
        checkPointLow = inSound->travelData.boundingBox.Center.x - inSound->travelData.boundingBox.Extents.x;

        if (tempPoint.z == checkPointHigh || tempPoint.z == checkPointLow)
        {
            tempPoint.z = -1.f;
        }
        else
        {
            tempPoint.z = 1.f;
        }

        direction = XMVectorMultiply(direction, XMLoadFloat3(&tempPoint));
        newPoint = XMVectorAdd(XMVectorScale(direction, distance), intersectPoint);
        XMStoreFloat3(&inSound->travelData.direction, direction);
    }

    inSound->posX = XMVectorGetX(newPoint);
    inSound->posY = XMVectorGetY(newPoint);
    inSound->posZ = XMVectorGetZ(newPoint);
}

void Sample::RoundTravel(PointSound* inSound)
{
    //Travel in a cirle around the listener
    XMVECTOR start = { inSound->posX, inSound->posY, inSound->posZ };
    XMMATRIX transform = XMMatrixRotationZ(inSound->travelData.vel / inSound->travelData.radius);
    XMVECTOR newPoint = XMVector3Transform(start, transform);
    inSound->posX = XMVectorGetX(newPoint);
    inSound->posY = XMVectorGetY(newPoint);
    inSound->posZ = XMVectorGetZ(newPoint);
}
