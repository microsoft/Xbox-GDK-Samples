// AudioDeviceManager.cpp

#define INITGUID
#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <comdef.h>
#include <vector>
#include <iostream>
#include <functiondiscoverykeys_devpkey.h>

struct DeviceInfo
{
    LPWSTR deviceId;
    LPWSTR friendlyName;
    UINT formFactorId;
	LPWSTR formFactorName;
    bool isDefault;
};

// global list of all known audio devices
std::vector<DeviceInfo> g_audioDevices;

// audio device enumerator
static IMMDeviceEnumerator* g_enumerator = nullptr;

// This enumeration is used to identify the type of audio endpoint device.
// and can be used to determine the friendly names, form factor, and
// other properties of audio devices.
HRESULT EnumerateAudioDevices()
{
    if(!g_enumerator)
    {
        return E_FAIL;
    }

    IMMDeviceCollection* pCollection = nullptr;
    IMMDevice* defaultDevice = nullptr;
    LPWSTR defaultDeviceId = nullptr;

    // get the default audio output endpoint
    HRESULT hr = g_enumerator->GetDefaultAudioEndpoint(eRender, ERole::eConsole, &defaultDevice);
    if (FAILED(hr))
    {
        g_enumerator->Release();
        return hr;
    }

    // get the ID for the default endpoint
    hr = defaultDevice->GetId(&defaultDeviceId);
    if (FAILED(hr))
    {
        g_enumerator->Release();
        return hr;
    }

    // no longer need the defaultDevice object
    defaultDevice->Release();

	// Enumerate the devices: eRender for playback devices, eCapture for recording devices, eAll for both
    hr = g_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr))
    {
        g_enumerator->Release();
        CoTaskMemFree(defaultDeviceId);
        return hr;
    }

	g_audioDevices.clear();

	// Gather information about each device: deviceID, friendly name, and form factor
    UINT count = 0;
    pCollection->GetCount(&count);
    for (UINT i = 0; i < count; ++i)
    {
        IMMDevice* pDevice = nullptr;
        if (SUCCEEDED(pCollection->Item(i, &pDevice)))
        {
            IPropertyStore* pProps = nullptr;
            if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pProps)))
            {
                DeviceInfo thisDevice{};

				//Get the device ID
                LPWSTR deviceId;
                pDevice->GetId(&deviceId);
                thisDevice.deviceId = deviceId;

                // if this matches the default device ID retrieved earlier, set isDefault
                thisDevice.isDefault = wcscmp(defaultDeviceId, deviceId) == 0;

				//Other optional properties can be retrieved using the property store: FriendlyName, FormFactor, etc.
                PROPVARIANT varName;
                PropVariantInit(&varName);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName)))
                {
                    thisDevice.friendlyName = varName.pwszVal;
                }

                hr = pProps->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
                if (hr == S_OK && varName.uintVal)
                {
					thisDevice.formFactorId = varName.uintVal;
                    UINT formfactor = varName.uiVal;
                    switch (formfactor)
                    {
                        case EndpointFormFactor::Headphones:
                        {
						    thisDevice.formFactorName = const_cast<LPWSTR>(L"Headphones");
                            break;
                        }
                        case EndpointFormFactor::Headset:
                        {
                            thisDevice.formFactorName = const_cast<LPWSTR>(L"Headset");
                            break;
                        }
                        case EndpointFormFactor::Speakers:
                        {
                            thisDevice.formFactorName = const_cast<LPWSTR>(L"Speakers");
                            break;
                        }
                        case EndpointFormFactor::DigitalAudioDisplayDevice:
                        {
                            thisDevice.formFactorName = const_cast<LPWSTR>(L"Digital Audio Display Device");    
                            break;
                        }
                        default:
                        {
                            thisDevice.formFactorName = const_cast<LPWSTR>(L"Other");
                            break;
                        }
                    }
                }
				g_audioDevices.push_back(thisDevice);
                pProps->Release();
            }
            pDevice->Release();
        }
    }

    if(pCollection)
    {
        pCollection->Release();
    }

    if(defaultDevice)
    {
        CoTaskMemFree(defaultDeviceId);
    }

    return hr;
}

// IMMNotificationClient implementation
// These callbacks will be fired as the audio state of the machine changes
class AudioDeviceNotificationClient : public IMMNotificationClient
{
    LONG _refCount;

public:
    AudioDeviceNotificationClient() : _refCount(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&_refCount); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&_refCount);
        if (0 == ulRef) delete this;
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) override
    {
        if (IID_IUnknown == riid || __uuidof(IMMNotificationClient) == riid)
        {
            *ppvInterface = (IMMNotificationClient*)this;
            AddRef();
            return S_OK;
        }
        *ppvInterface = nullptr;
        return E_NOINTERFACE;
    }

    // update the master list of audio devices if the default changes
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
        if (flow == eRender && role == eConsole)
        {
            EnumerateAudioDevices();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }
};

AudioDeviceNotificationClient* g_audioNotificationClient;

// Call this function once (e.g., after window creation) to start monitoring
HRESULT StartAudioDeviceMonitoring()
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return hr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&g_enumerator);
    if (FAILED(hr)) return hr;

    g_audioNotificationClient = new AudioDeviceNotificationClient();

    hr = g_enumerator->RegisterEndpointNotificationCallback(g_audioNotificationClient);
    if (FAILED(hr)) return hr;

    // Enumerate current devices
    hr = EnumerateAudioDevices();
    if (FAILED(hr)) return hr;

    return S_OK;
}

// Call this function to stop monitoring and clean up notification callbacks
HRESULT StopAudioDeviceMonitoring()
{
    if(g_enumerator)
    {
        g_enumerator->UnregisterEndpointNotificationCallback(g_audioNotificationClient);
        g_enumerator->Release();
        g_enumerator = nullptr;
    }

    if(g_audioNotificationClient)
    {
        g_audioNotificationClient->Release();
    }

    return S_OK;
}

// Retrieve the default audio IMMDevice object
HRESULT GetDefaultAudioOutputDevice(IMMDevice* immDevice)
{
    if(!immDevice)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return hr;

    IMMDeviceEnumerator* pEnumerator = nullptr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return hr;

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, ERole::eConsole, &immDevice);
    if (FAILED(hr))
    {
        pEnumerator->Release();
        return hr;
    }

    pEnumerator->Release();
    CoUninitialize();

    return S_OK;
}
