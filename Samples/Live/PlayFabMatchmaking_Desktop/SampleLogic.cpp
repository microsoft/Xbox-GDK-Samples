//--------------------------------------------------------------------------------------
// SampleLogic.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabMatchmaking_Desktop.h"

#include "playfab/PlayFabClientApi.h"
#include "playfab/PlayFabApiSettings.h"
#include "playfab/PlayFabSettings.h"
#include "playfab/PlayFabMultiplayerApi.h"
#include "playfab/QoS/PlayFabQoSApi.h"

using namespace PlayFab;
using namespace std::chrono;

namespace
{
    constexpr const char* c_PlayFabTitleId = "C8C15";
    constexpr const char* c_TokenProviderUrl = "https://playfabapi.com/";
    constexpr int c_SocketBufferLen = 8;
    constexpr int c_QosServerUdpPort = PlayFab::QoS::PORT;
    constexpr int c_MatchmakingTimeoutInSeconds = 30;
    constexpr const char* c_PlayFabSimpleMatchMakeQueueName = "SimpleLevel";
    constexpr const char* c_PlayFabRegionMatchMakeQueueName = "SimpleRegion";

    addrinfo* GetSockAddrByHostName(const char* serverUrl, int serverPort)
    {
        addrinfo* result = nullptr;
        sockaddr_in* sockaddr_ipv4 = nullptr;

        //--------------------------------
        // Setup the hints address info structure
        // which is passed to the getaddrinfo() function
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_UDP;

        char portAsString[c_SocketBufferLen] = {};
        _itoa_s<c_SocketBufferLen>(serverPort, portAsString, 10);
        auto serverPortName = portAsString;

        //--------------------------------
        // Call getaddrinfo(). If the call succeeds,
        // the result variable will hold a linked list
        // of addrinfo structures containing response
        // information
        auto retVal = getaddrinfo(serverUrl, serverPortName, &hints, &result);
        if (retVal != 0)
        {
            WSACleanup();
            return nullptr;
        }

        // Retrieve the IPV4 address 
        for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next)
        {
            switch (ptr->ai_family) {
            case AF_INET:
                //printf("AF_INET (IPv4)\n");
                sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
                result = ptr;
                break;
            case AF_UNSPEC:
            case AF_INET6:
            case AF_NETBIOS:
            default:
                break;
            }
        }

        if (nullptr == sockaddr_ipv4)
        {
            freeaddrinfo(result);
            return nullptr;
        }

        return result;
    }
}

void Sample::CheckForNetworkInitialization()
{
    Log("CheckForNetworkingInitialization() started.");
    m_asyncOpWidget->Show(u8"Checking for network ready");

    ZeroMemory(&m_connectivityHint, sizeof(m_connectivityHint));
    auto hr = XNetworkingGetConnectivityHint(&m_connectivityHint);

    if (FAILED(hr))
    {
        Log("CheckForNetworkingInitialization() failed.");
        Log(">> XNetworkingGetConnectivityHint() returned 0x%08x.", hr);
        return;
    }
    if (m_connectivityHint.networkInitialized)
    {
        Log(">> network is already initialized.");
        HandleNetworkInitializationComplete();
    }
    else
    {
        auto callback = [](
            _In_opt_ void* context,
            _In_ const XNetworkingConnectivityHint* connectivityHint)
        {
            Sample* sample = static_cast<Sample*>(context);
            sample->Log("CheckForNetworkingInitialization() callback issued...");

            if (connectivityHint->networkInitialized)
            {
                sample->Log(">> network is initialized.");
                sample->m_connectivityHint = *connectivityHint;
                XNetworkingUnregisterConnectivityHintChanged(
                    sample->m_taskQueueRegToken,
                    false);
                sample->m_taskQueueRegToken.token = 0;
                sample->HandleNetworkInitializationComplete();
            }
            else
            {
                sample->Log(">> network is NOT initialized yet.");
            }
        };

        m_taskQueueRegToken.token = 0;
        hr = XNetworkingRegisterConnectivityHintChanged(
            m_theTaskQueue,
            this,
            callback,
            &m_taskQueueRegToken);

        if (FAILED(hr))
        {
            Log("CheckForNetworkingInitialization() failed.");
            Log(">> XNetworkingRegisterConnectivityHintChanged() returned 0x%08x.", hr);
        }
    }
}

void Sample::LoginToXboxLive(bool silentAuth)
{
    Log("LoginToXboxLive() started.");
    m_asyncOpWidget->Show(u8"Logging into Xbox Live");

    m_liveResources->SetErrorHandler([this](HRESULT result)
        {
            Log(">>> Error HRESULT: %08x", result);
        });
    m_liveResources->SetUserChangedCallback([this](XUserHandle /*userHandle*/)
        {
            if (m_liveResources->IsUserSignedIn())
            {
                HandleXboxLiveLoginComplete();
            }
        });

    if (silentAuth)
    {
        m_liveResources->SetErrorHandler([this](HRESULT)
        {
            m_liveResources->SetErrorHandler(nullptr);
            m_liveResources->SignInWithUI();
        });
        m_liveResources->SignInSilently();
    }
    else
    {
        m_liveResources->SignInWithUI();
    }
}

void Sample::RequestLiveUserToken()
{
    Log("RequestLiveUserToken() started.");
    m_asyncOpWidget->Show(u8"Requesting token for user");

    auto tokenAsyncBlock = new XAsyncBlock
    {
        m_theTaskQueue/*XTaskQueue queue*/,
        this/*void* context*/
    };

    tokenAsyncBlock->callback = [](XAsyncBlock* ab)
    {
        Sample* sample = reinterpret_cast<Sample*>(ab->context);

        size_t bufferSize = 0;
        auto hr = XUserGetTokenAndSignatureResultSize(ab, &bufferSize);

        if (FAILED(hr))
        {
            sample->Log(">>> XUserGetTokenAndSignatureAsync() failure with hr = %08x", hr);
        }
        else
        {
            std::vector<uint8_t> buffer(bufferSize);

            XUserGetTokenAndSignatureData* userTokenData = nullptr;

            hr = XUserGetTokenAndSignatureResult(
                ab,
                bufferSize,
                buffer.data(),
                &userTokenData,
                nullptr/*bufferUsed*/);
            sample->m_userToken = userTokenData->token;

            if (FAILED(hr))
            {
                sample->Log(">>> XUserGetTokenAndSignatureAsync() failed: %08x", hr);
            }
            else
            {
                sample->Log(">>> XUserGetTokenAndSignatureAsync() success!!!");
                sample->HandleLiveUserTokenRequestComplete();
            }
        }

        delete ab;
    };

    XUserGetTokenAndSignatureOptions options = XUserGetTokenAndSignatureOptions::None;

    //if (forceRefresh)
    {
        options |= XUserGetTokenAndSignatureOptions::ForceRefresh;
    }

    auto hr = XUserGetTokenAndSignatureAsync(
        m_liveResources->GetUser(),
        options,
        "GET",
        c_TokenProviderUrl,
        0,
        nullptr,
        0,
        nullptr,
        tokenAsyncBlock);

    if (FAILED(hr))
    {
        Log(">>> Immediate failure with hr = %08x", hr);
        delete tokenAsyncBlock;
    }
}

void Sample::LoginToPlayFab()
{
    Log("LoginToPlayFab() started.");
    m_asyncOpWidget->Show(u8"Logging into PlayFab");

    PlayFabSettings::staticSettings->titleId = c_PlayFabTitleId;

    ClientModels::LoginWithXboxRequest loginRequest;
    loginRequest.CreateAccount = true;
    loginRequest.TitleId = c_PlayFabTitleId;
    loginRequest.XboxToken = m_userToken;

    PlayFabClientAPI::LoginWithXbox(
        loginRequest,
        [](ClientModels::LoginResult loginResult, void* customData)
        {
            Sample* sample = reinterpret_cast<Sample*>(customData);
            sample->Log("PlayFabXboxLogin::LoginPlayFab() success!!!");
            sample->m_playFabId = loginResult.PlayFabId;
            sample->m_entityId = loginResult.EntityToken->Entity->Id;
            sample->m_entityType = loginResult.EntityToken->Entity->Type;
            sample->m_entityToken = loginResult.EntityToken->EntityToken;
            sample->HandlePlayFabLoginComplete();

            ClientModels::UpdateUserTitleDisplayNameRequest request;
            request.DisplayName = sample->m_liveResources->GetGamertag();
            PlayFabClientAPI::UpdateUserTitleDisplayName(
                request,
                [](const ClientModels::UpdateUserTitleDisplayNameResult&, void*)
                {});
        },
        [](const PlayFabError& error, void* customData)
        {
            Sample* sample = reinterpret_cast<Sample*>(customData);
            sample->Log(">> LoginWithXbox() failed with error = %d", error.ErrorCode);
            sample->Log(">> %s", error.ErrorMessage.c_str());
        }, this);
}
int Sample::PingServerUrl(const char* serverUrl, int serverPort)
{
    Log("Getting ping time for %s:%d", serverUrl, serverPort);

    struct sockaddr_in si_other;
    SOCKET s;
    int slen = sizeof(si_other);
    char buf[c_SocketBufferLen];
    WSADATA wsa;

    // initialize winsock
    Log("Initialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        Log("Failed. Error Code : %d", WSAGetLastError());
        return -1;
    }

    // get the host by name
    Log("Getting host by name...");
    auto result = GetSockAddrByHostName(serverUrl, serverPort);
    if (result == nullptr)
    {
        Log("get host by name + port failed: %d", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    // create socket for read/write
    Log("Creating socket...");
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == static_cast<SOCKET>(SOCKET_ERROR))
    {
        Log("socket() failed with error code : %d", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    auto start = high_resolution_clock::now();

    // communication with write/read
    {
        memset(buf, 0xFF, c_SocketBufferLen);
        if (sendto(s, buf, c_SocketBufferLen, 0, result->ai_addr, slen) == SOCKET_ERROR)
        {
            Log("sendto() failed with error code : %d", WSAGetLastError());
        }
        else
        {
            Log(">> sendto() was a success.");
        }

        memset(buf, 0xFF, c_SocketBufferLen);
        if (recvfrom(s, buf, c_SocketBufferLen, 0, result->ai_addr, &slen) == SOCKET_ERROR)
        {
            Log("recvfrom() failed with error code : %d", WSAGetLastError());
        }
        else
        {
            Log(">> recvfrom() was a success.");
        }
    }

    auto end = high_resolution_clock::now();
    auto durationInMs = (duration_cast<milliseconds>(end - start)).count();

    Log(">> Total ping time: %d ms", durationInMs);

    freeaddrinfo(result);

    Log("Closing socket...");
    closesocket(s);

    Log("Cleaning up Winsock...");
    WSACleanup();

    return static_cast<int>(durationInMs);
}

void Sample::GetLatencyToRegions()
{
    static std::string westUsUrl;
    static std::string eastUsUrl;

    Log("Sample::GetLatencyToRegions() has started.");
    m_asyncOpWidget->Show(u8"Getting latencies to regions");

    m_getRegionLatenciesButton->SetEnabled(false);

    MultiplayerModels::ListQosServersForTitleRequest request;
    request.IncludeAllRegions = true;

    PlayFabMultiplayerAPI::ListQosServersForTitle(
        request,
        [](const MultiplayerModels::ListQosServersForTitleResponse& response, void* customData)
        {
            Sample* sample = reinterpret_cast<Sample*>(customData);
            sample->Log(">> Found %d QOS servers.", response.QosServers.size());
            for (auto server : response.QosServers)
            {
                sample->Log(">> Region: %s, URL: %s", server.Region.c_str(), server.ServerUrl.c_str());
                if ("WestUs" == server.Region)
                {
                    westUsUrl = server.ServerUrl.c_str();
                }
                else if ("EastUs" == server.Region)
                {
                    eastUsUrl = server.ServerUrl.c_str();
                }
            }

            sample->m_westUsRegionLatency = sample->PingServerUrl(westUsUrl.c_str(), c_QosServerUdpPort);
            sample->m_eastUsRegionLatency = sample->PingServerUrl(eastUsUrl.c_str(), c_QosServerUdpPort);
            sample->HandleRegionLatencyComplete();
        },
        [](const PlayFabError& error, void* customData)
        {
            Sample* sample = reinterpret_cast<Sample*>(customData);
            sample->Log(">> ListQosServersForTitle() failed with error = %d", error.ErrorCode);
            sample->Log(">> %s", error.ErrorMessage.c_str());
        }, this);
}

void Sample::DoPlayFabMatchMake(bool simple)
{
    Log("DoPlayFabMatchMake()");
    m_asyncOpWidget->Show(u8"Performing matchmaking");

    if (!simple && !HasRegionLatencies())
    {
        Log(">> Please get the latencies for the region data centers first.");
        return;
    }

    m_getRegionLatenciesButton->SetEnabled(false);
    m_findSimpleMatchButton->SetEnabled(false);
    m_findRegionMatchButton->SetEnabled(false);
    m_cancelMatchmakingButton->SetEnabled(true);

    m_matchmakingManager->BeginMatchmaking(
        simple ? c_PlayFabSimpleMatchMakeQueueName : c_PlayFabRegionMatchMakeQueueName,
        // level, east latency, west latency
        {
            static_cast<int>(m_matchLevelSlider->GetCurrentValue()),
            m_eastUsRegionLatency,
            m_westUsRegionLatency
        },
        // timeout in seconds
        c_MatchmakingTimeoutInSeconds,
        [this]()
        {
            if (MatchmakingStatus::MatchingFailed == m_matchmakingManager->MatchStatus())
            {
                Log("DoPlayFabMatchMake() matchmaking failed.");
            }

            if (MatchmakingStatus::MatchingCancelled == m_matchmakingManager->MatchStatus())
            {
                HandleMatchingCancelled();
            }

            if (MatchmakingStatus::MatchingComplete == m_matchmakingManager->MatchStatus())
            {
                HandlePlayFabMatchMakeComplete();
            }
        });
}

void Sample::CancelMatchmaking()
{
    m_cancelMatchmakingButton->SetEnabled(false);
    m_asyncOpWidget->Hide();
    m_asyncOpWidget->Show(u8"Cancelling matchmaking");

    m_matchmakingManager->CancelMatchmaking(
        [this]
        {
            HandleMatchingCancelled();
        });
}

// Asynchronous functionality completion handlers

void Sample::HandleNetworkInitializationComplete()
{
    m_asyncOpWidget->Hide();
    InitializeXboxLive();
}

void Sample::HandleXboxLiveLoginComplete()
{
    m_asyncOpWidget->Hide();
    RequestLiveUserToken();
    m_liveInfoHUD->SetUser(m_liveResources->GetUser(), m_theTaskQueue);
}

void Sample::HandleLiveUserTokenRequestComplete()
{
    m_asyncOpWidget->Hide();
    LoginToPlayFab();
}

void Sample::HandlePlayFabLoginComplete()
{
    m_asyncOpWidget->Hide();
    m_matchmakingManager->Initialize(m_playFabId.c_str(), m_entityId.c_str(), m_entityToken.c_str());
    m_findSimpleMatchButton->SetEnabled(true);
    m_getRegionLatenciesButton->SetEnabled(true);
}

void Sample::HandleRegionLatencyComplete()
{
    m_asyncOpWidget->Hide();
    m_getRegionLatenciesButton->SetEnabled(true);
    m_findRegionMatchButton->SetEnabled(!m_matchmakingManager->IsMatchmaking());
}

void Sample::HandlePlayFabMatchMakeComplete()
{
    m_asyncOpWidget->Hide();
    m_getRegionLatenciesButton->SetEnabled(true);
    m_findRegionMatchButton->SetEnabled(HasRegionLatencies());
    m_findSimpleMatchButton->SetEnabled(true);
    m_cancelMatchmakingButton->SetEnabled(false);
    m_matchmakingManager->GetMatchedPlayerProfile(
        m_matchmakingManager->IsGameHost() ? size_t(1) : size_t(0),
        [this](std::string displayName, int level)
        {
            Log("Other player...");
            Log(">>> Display Name: %s", displayName.c_str());
            Log(">>> Level: %d", level);
        });
}

void Sample::HandleMatchingCancelled()
{
    m_asyncOpWidget->Hide();
    m_getRegionLatenciesButton->SetEnabled(true);
    m_findRegionMatchButton->SetEnabled(HasRegionLatencies());
    m_findSimpleMatchButton->SetEnabled(true);
}
