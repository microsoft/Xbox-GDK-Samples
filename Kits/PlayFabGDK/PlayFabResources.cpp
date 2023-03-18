//--------------------------------------------------------------------------------------
// File: PlayFabResources.cpp
//
// Handles login to PlayFab services using Xbox authentication
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabResources.h"

/// This class when constructed takes in a PlayFab title ID and a valid Xbox Live user (XUser)
/// and does the token request required for authentication.
/// After initialization, the account can be signed into the PlayFab service with the XUser account.
/// Result is an authenticated state and IDs required for calling PlayFab APIs from the client.

using namespace ATG;

struct SharedState
{
    XUserHandle xuserHandle{ nullptr };
    XUserGetTokenAndSignatureData* xuserTokenData{ nullptr };
    std::vector<uint8_t> xuserTokenBuffer;
    PFServiceConfigHandle serviceConfigHandle{ nullptr };
    PFEntityHandle entityHandle{ nullptr };
    std::vector<char> entityHandleBuffer;
    PFEntityKey const* pEntityKey{};
};

SharedState g_playFabState;

void Log(_In_z_ _Printf_format_string_ const char* format, ...)
{
    va_list args;
    va_start(args, format);
    size_t bufCount = 1;
    std::vector<char> buf(bufCount + std::vsnprintf(NULL, 0, format, args));
    std::vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    std::string strBuffer = buf.data();
    strBuffer += "\r\n";

    OutputDebugStringA(strBuffer.c_str());
}

_Use_decl_annotations_
PlayFabResources::PlayFabResources(const char* titleId, XUserHandle xuser)
{
    Initialize(titleId, xuser);
}

PlayFabResources::~PlayFabResources()
{
    Cleanup();
}

void PlayFabResources::Initialize(const char* titleId, XUserHandle xuser)
{
    g_playFabState.xuserHandle = xuser;
    std::string endpoint = "https://" + std::string(titleId) + ".playfabapi.com";

    Log("Calling PFServicesInitialize");
    HRESULT hr = PFServicesInitialize(nullptr);

    if (SUCCEEDED(hr))
    {
        Log("Calling PFServiceConfigCreateHandle");
        hr = PFServiceConfigCreateHandle(endpoint.c_str(), titleId, &g_playFabState.serviceConfigHandle);

        if (SUCCEEDED(hr))
        {
            RequestXUserToken();
        }
        else
        {
            Log("Error calling PFServiceConfigCreateHandle : 0x%08X", hr);
        }
    }
    else
    {
        Log("Error calling PFServicesInitialize : 0x%08X", hr);
    }
}

void PlayFabResources::Cleanup()
{
    if (g_playFabState.entityHandle)
    {
        PFEntityCloseHandle(g_playFabState.entityHandle);
        g_playFabState.entityHandle = nullptr;
        Log("PFEntityCloseHandle");
    }

    if (g_playFabState.serviceConfigHandle)
    {
        PFServiceConfigCloseHandle(g_playFabState.serviceConfigHandle);
        Log("PFServiceConfigCloseHandle");

        XAsyncBlock async{};
        PFServicesUninitializeAsync(&async);
        XAsyncGetStatus(&async, true);
        Log("PFServicesUninitializeAsync");
    }
}

void PlayFabResources::RequestXUserToken()
{
    Log("Calling XUserGetTokenAndSignatureAsync");

    XUserGetTokenAndSignatureOptions options = XUserGetTokenAndSignatureOptions::ForceRefresh;

    XAsyncBlock async{};
    HRESULT hr = XUserGetTokenAndSignatureAsync(
        g_playFabState.xuserHandle,
        options,
        "GET",
        "https://playfabapi.com",
        0,
        nullptr,
        0,
        nullptr,
        &async);
    hr = XAsyncGetStatus(&async, true);

    if (SUCCEEDED(hr))
    {
        size_t bufferSize;
        hr = XUserGetTokenAndSignatureResultSize(&async, &bufferSize);
        g_playFabState.xuserTokenBuffer.resize(bufferSize);

        Log("Calling XUserGetTokenAndSignatureResult");
        hr = XUserGetTokenAndSignatureResult(
            &async,
            g_playFabState.xuserTokenBuffer.size(),
            g_playFabState.xuserTokenBuffer.data(),
            &g_playFabState.xuserTokenData,
            nullptr);

        if (FAILED(hr))
        {
            Log("Error calling XUserGetTokenAndSignatureResult : 0x%08X", hr);
        }
    }
    else
    {
        Log("Error calling XUserGetTokenAndSignatureAsync : 0x%08X", hr);
    }
}

void PlayFabResources::LoginToPlayFab()
{
    Log("Calling PFAuthenticationLoginWithXUserAsync");

    PFAuthenticationLoginWithXUserRequest request{};
    request.createAccount = true;
    request.user = g_playFabState.xuserHandle;

    XAsyncBlock async{};
    HRESULT hr = PFAuthenticationLoginWithXUserAsync(g_playFabState.serviceConfigHandle, &request, &async);
    hr = XAsyncGetStatus(&async, true);

    if (SUCCEEDED(hr))
    {
        std::vector<char> loginResultBuffer;
        PFAuthenticationLoginResult const* loginResult;
        size_t bufferSize;
        hr = PFAuthenticationLoginWithXUserGetResultSize(&async, &bufferSize);
        loginResultBuffer.resize(bufferSize);

        hr = PFAuthenticationLoginWithXUserGetResult(
            &async,
            &g_playFabState.entityHandle,
            loginResultBuffer.size(),
            loginResultBuffer.data(),
            &loginResult,
            nullptr);

        if (SUCCEEDED(hr))
        {
            size_t size{};
            hr = PFEntityGetEntityKeySize(g_playFabState.entityHandle, &size);
            g_playFabState.entityHandleBuffer.resize(size);

            Log("PFEntityGetEntityKey");
            hr = PFEntityGetEntityKey(
                g_playFabState.entityHandle,
                g_playFabState.entityHandleBuffer.size(),
                g_playFabState.entityHandleBuffer.data(),
                &g_playFabState.pEntityKey,
                nullptr);

            if (FAILED(hr))
            {
                Log("Error calling PFEntityGetEntityKey : 0x%08X", hr);
            }
        }
        else
        {
            Log("Error calling PFAuthenticationLoginWithXUserGetResult : 0x%08X", hr);
        }
    }
    else
    {
        Log("Error calling PFAuthenticationLoginWithXUserAsync : 0x%08X", hr);
    }
}

PFEntityHandle PlayFabResources::GetEntityHandle()
{
    return g_playFabState.entityHandle;
}

PFEntityKey const* PlayFabResources::GetPlayerEntityKey()
{
    return g_playFabState.pEntityKey;
}

const char* PlayFabResources::GetXboxToken()
{
    return g_playFabState.xuserTokenData->token;
}
