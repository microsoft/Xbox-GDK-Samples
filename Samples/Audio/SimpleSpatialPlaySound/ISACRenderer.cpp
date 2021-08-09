//--------------------------------------------------------------------------------------
//
// ISACRenderer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "pch.h"
#include "ISACRenderer.h"

using Microsoft::WRL::ComPtr;

//
//  ISACRenderer()
//
ISACRenderer::ISACRenderer() :
    m_bufferCompletionEvent(nullptr),
    m_ISACrenderstate(RenderState::Inactive)
{

}

//
//  ~ISACRenderer()
//
ISACRenderer::~ISACRenderer()
{
}

//
//  InitializeAudioDevice()
//
//  Activates the default audio renderer.  This needs to be called from the main UI thread.
//
HRESULT ISACRenderer::InitializeAudioDevice()
{
    m_SpatialAudioClient.Reset();
    m_SpatialAudioStream.Reset();

    // Create a device enumerator
    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr))
    {
        return hr;
    }

    // Get the default renderer
    ComPtr<IMMDevice> device;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr))
    {
        return hr;
    }

    // Activate the endpoint
    hr = device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, (void**)&m_SpatialAudioClient);
    if (FAILED(hr))
    {
        return hr;
    }

    // Check the available rendering formats 
    ComPtr<IAudioFormatEnumerator> audioObjectFormatEnumerator;
    hr = m_SpatialAudioClient->GetSupportedAudioObjectFormatEnumerator(&audioObjectFormatEnumerator);

    // WavFileIO is helper class to read WAV file
    WAVEFORMATEX* objectFormat = nullptr;
    UINT32 audioObjectFormatCount;
    hr = audioObjectFormatEnumerator->GetCount(&audioObjectFormatCount); // There is at least one format that the API accept
    if (audioObjectFormatCount == 0)
    {
        return E_FAIL;
    }
         
    // Select the most favorable format, first one
    hr = audioObjectFormatEnumerator->GetFormat(0, &objectFormat);

    // Create the event that will be used to signal the client for more data
    m_bufferCompletionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // Define the object mask for the Spatial bed - in this case 7.1.4
    AudioObjectType objectmask =
        AudioObjectType_FrontLeft |
        AudioObjectType_FrontRight |
        AudioObjectType_FrontCenter |
        AudioObjectType_LowFrequency |
        AudioObjectType_SideLeft |
        AudioObjectType_SideRight |
        AudioObjectType_BackLeft |
        AudioObjectType_BackRight |
        AudioObjectType_TopFrontLeft |
        AudioObjectType_TopFrontRight |
        AudioObjectType_TopBackLeft |
        AudioObjectType_TopBackRight;

    // Set the activation parameters for the Spatial stream
    SpatialAudioObjectRenderStreamActivationParams  activationparams =
    {
        objectFormat,
        objectmask,
        0,							//zero dynamic objects needed
        0,							//zero dynamic objects needed
        AudioCategory_GameEffects,
        m_bufferCompletionEvent,
        nullptr
    };

    PROPVARIANT activateParams;
    PropVariantInit(&activateParams);
    activateParams.vt = VT_BLOB;
    activateParams.blob.cbSize = sizeof(activationparams);
    activateParams.blob.pBlobData = reinterpret_cast<BYTE*>(&activationparams);

    // Activate the spatial stream
    hr = m_SpatialAudioClient->ActivateSpatialAudioStream(&activateParams, __uuidof(m_SpatialAudioStream), &m_SpatialAudioStream);
    if (FAILED(hr))
    {
        return hr;
    }

    // Start streaming / rendering  
    hr = m_SpatialAudioStream->Start();
    if (FAILED(hr))
    {
        return hr;
    }

    m_ISACrenderstate = RenderState::Active;
    return hr;
}


