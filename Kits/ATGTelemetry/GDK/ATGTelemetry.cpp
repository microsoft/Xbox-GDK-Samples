//--------------------------------------------------------------------------------------
// File: ATGTelemetry.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "ATGTelemetry.h"

#include <XGame.h>
#include <XSystem.h>

#include "httpClient/httpClient.h"
#include "StringUtil.h"
#include "Json.h"
#include <sstream>

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#include <fstream>
#include "Shlwapi.h"
#include "Shlobj.h"
#endif

#ifdef _GAMING_XBOX
#pragma warning(disable : 4365)
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#endif

namespace {
    using AsyncUniquePtr = std::unique_ptr<XAsyncBlock>;
    AsyncUniquePtr CreateAsync() { return AsyncUniquePtr(new XAsyncBlock{}); }

    const char* LOGIN_URL = "https://3436.playfabapi.com/Client/LoginWithCustomID";
    const char* EVENT_URL = "https://3436.playfabapi.com/Event/WriteTelemetryEvents";

    struct TelemetryData
    {
        std::string osVersion;
        std::string hostingOsVersion;
        std::string family;
        std::string form;
        std::string clientId;
        std::string sandbox;
        std::string titleId;
        std::string exeName;
        std::string sessionId;

        json ToJson() const
        {
            return json
            {
                {"osVersion", osVersion },
                {"hostingOsVersion", hostingOsVersion },
                {"family", family },
                {"form", form },
                {"clientId", clientId },
                {"sandboxId", sandbox },
                {"titleId", titleId },
                {"exeName", exeName },
                {"sessionId", sessionId }
            };
        }
    };
}

using namespace ATG;

class ATGTelemetry
{
public:
    ATGTelemetry() :
        m_bHCInitialized{ false },
        m_authToken{},
        m_data{}
    {
    }

    ~ATGTelemetry()
    {
        if (m_bHCInitialized)
        {
            HCCleanup();
        }
    }

    void SendTelemetry()
    {
        static bool sent = false;

        if (!sent)
        {
            // Ensure the HTTP library is initialized
            if (FAILED(HCInitialize(nullptr)))
            {
                return;
            }
            m_bHCInitialized = true;

            CollectBaseTelemetry();
            UploadTelemetry();
            sent = true;
        }
    }

private:
    std::string NewGuid()
    {
        GUID id = {};
        char buf[64] = {};

        std::ignore = CoCreateGuid(&id);

        sprintf_s(buf, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
            id.Data1, id.Data2, id.Data3,
            id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
            id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);

        return std::string(buf);
    }

    std::string GetTelemetryId()
    {
        const int BUFSIZE = 64;
        char buf[BUFSIZE] = {};

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_GAMES)
        size_t ignoreField;
        XSystemGetConsoleId(XSystemConsoleIdBytes, buf, &ignoreField);

        std::string clientId = buf;
        clientId.erase(std::remove(clientId.begin(), clientId.end(), '.'), clientId.end());
        return clientId;
#else
        wchar_t* folderPath;
        std::string telemetryId;

        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_NO_PACKAGE_REDIRECTION, nullptr, &folderPath)))
        {
            // Path information
            std::wstring path(folderPath);
            path += L"\\Microsoft\\ATGSamples\\";
            std::wstring fullPath = path + L"telemetry.txt";

            bool createFile = CreateDirectoryW(path.c_str(), nullptr);

            if (!createFile)
            {
                DWORD error = GetLastError();

                // Sanity checking - Directory failed to create but that should mean it already exists
                if (error != ERROR_ALREADY_EXISTS)
                {
                    return NewGuid();
                }
            }

            int attempts = 0;
            while (attempts++ < 2)
            {
                if (createFile)
                {
                    std::ofstream telFile;
                    telFile.open(fullPath, std::ios::out | std::ios::trunc);
                    telemetryId = NewGuid();

                    telFile << telemetryId;
                    telFile.close();
                    break;
                }
                else
                {
                    std::ifstream telFile;
                    telFile.open(fullPath);
                    if (telFile.good())
                    {
                        telFile.get(buf, BUFSIZE);
                        telFile.close();
                        telemetryId = buf;

                        if (telemetryId.size() >= 32)
                        {
                            break;
                        }
                    }

                    createFile = true;
                }
            }
        }

        return telemetryId;
#endif
    }

    // Collect the base telemetry for the system
    void CollectBaseTelemetry()
    {
        constexpr int BUFSIZE = 256;
        size_t ignore;
        char buf[BUFSIZE] = {};

        // OS and client information
        {
            auto info = XSystemGetAnalyticsInfo();

            m_data.osVersion = XVersionToString(info.osVersion);
            m_data.hostingOsVersion = XVersionToString(info.hostingOsVersion);
            m_data.family = info.family;
            m_data.form = info.form;

            m_data.clientId = GetTelemetryId();

            XSystemGetXboxLiveSandboxId(XSystemXboxLiveSandboxIdMaxBytes, buf, &ignore);
            m_data.sandbox = buf;
        }

        // Sample information
        {
            uint32_t titleId = 0;
            XGameGetXboxTitleId(&titleId);
            m_data.titleId = ToHexString(titleId);

            wchar_t exePath[MAX_PATH + 1] = {};

            if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
            {
                m_data.exeName = "Unknown";
            }
            else
            {
                auto path = std::wstring(exePath);
                auto off = path.find_last_of(L"\\");
                auto exeOnly = path.substr(off + 1);
                m_data.exeName = DX::WideToUtf8(exeOnly);
            }
        }

        // This run information
        m_data.sessionId = NewGuid();
    }

    void UploadTelemetry()
    {
        AsyncUniquePtr async = CreateAsync();
        AuthPlayfab();

        std::string payload = CreateEventPayload();

        auto httpCall = CreateEventRequest(m_authToken, payload);
        async->context = httpCall;

        if (SUCCEEDED(HCHttpCallPerformAsync(httpCall, async.get())))
        {
            XAsyncGetStatus(async.get(), true);
        }

        HCHttpCallCloseHandle(httpCall);
    }

    void AuthPlayfab()
    {
        struct AuthContext
        {
            ATGTelemetry* _this;
            HCCallHandle httpCallHandle;
        };

        AsyncUniquePtr async = CreateAsync();
        async->callback = [](XAsyncBlock* async)
        {
            auto ctx = static_cast<AuthContext*>(async->context);
            uint32_t status;

            HCCallHandle httpCall = ctx->httpCallHandle;

            HCHttpCallResponseGetStatusCode(httpCall, &status);
            if (status < 200 || status >= 300) { return; }

            const char* responseString;
            HCHttpCallResponseGetResponseString(httpCall, &responseString);

            json respJs = json::parse(responseString);
            ctx->_this->m_authToken = respJs["data"]["EntityToken"]["EntityToken"].get<std::string>();
        };

        HCCallHandle httpCall = CreateAuthRequest();
        auto ctx = new AuthContext{ this, httpCall };
        async->context = ctx;

        if (SUCCEEDED(HCHttpCallPerformAsync(httpCall, async.get())))
        {
            XAsyncGetStatus(async.get(), true);
        }

        HCHttpCallCloseHandle(httpCall);
    }

private:
#pragma region Utility Methods
    HCCallHandle CreateAuthRequest()
    {
        HCCallHandle httpCall;
        json payload
        {
            { "CreateAccount", true },
            { "CustomId", m_data.clientId },
            { "TitleId", "3436" }
        };

        HCHttpCallCreate(&httpCall);
        HCHttpCallRequestSetUrl(httpCall, "POST", LOGIN_URL);
        HCHttpCallRequestSetRequestBodyString(httpCall, payload.dump().c_str());
        HCHttpCallRequestSetHeader(httpCall, "Content-Type", "application/json", true);

        return httpCall;
    }

    std::string CreateEventPayload()
    {
        json evJS
        {
            {
                "Events", json::array({
                {
                    { "EventNamespace", "com.playfab.events.samples" },
                    { "Name", "launch" },
                    { "Payload", m_data.ToJson() }
                }})
            }
        };

        return evJS.dump();
    }

    static HCCallHandle CreateEventRequest(const std::string& authToken, const std::string& payload)
    {
        HCCallHandle httpCall;

        HCHttpCallCreate(&httpCall);
        HCHttpCallRequestSetUrl(httpCall, "POST", EVENT_URL);
        HCHttpCallRequestSetRequestBodyString(httpCall, payload.c_str());
        HCHttpCallRequestSetHeader(httpCall, "Content-Type", "application/json", true);
        HCHttpCallRequestSetHeader(httpCall, "X-EntityToken", authToken.c_str(), false);

        return httpCall;
    }

    static std::string XVersionToString(XVersion version)
    {
        std::stringstream ss;
        ss << version.major << "." << version.minor << "." << version.build << "." << version.revision;
        return ss.str();
    }

    static std::string ToHexString(uint32_t value)
    {
        std::stringstream ss;
        ss << std::hex << value;
        return ss.str();
    }
#pragma endregion

private:
    bool m_bHCInitialized;
    std::string m_authToken;
    TelemetryData m_data;
};

void ATG::SendLaunchTelemetry()
{
#ifndef ATG_DISABLE_TELEMETRY
    AsyncUniquePtr async = CreateAsync();
    async->callback = [](XAsyncBlock* ab)
    {
        delete ab;
    };

    if (SUCCEEDED(XAsyncRun(async.get(), [](XAsyncBlock*) -> HRESULT
        {
            ATGTelemetry atgTelemetry;
            atgTelemetry.SendTelemetry();

            return S_OK;
        })))
    {
        async.release();
    }
#endif
}

