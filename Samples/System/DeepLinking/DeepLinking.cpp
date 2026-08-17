//--------------------------------------------------------------------------------------
// DeepLinking.cpp
//
// Sample implementation
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "imgui.h"
#include "imgui/imgui_atg.h"
#include "imgui/imgui_atg_device_context.h"
#include "DeepLinking.h"
#include <XGameActivation.h>

namespace
{
    void DrawHeader(ImVec4 color, const char* text)
    {
        ImGui::PushFont(nullptr, 30.0f);

        // Set cursor to the middle (minus half the text width) and draw the text
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) * 0.5f);
        ImGui::TextColored(color, text);

        ImGui::PopFont();
    }
}

void Sample::Initialize(HWND /*hWnd*/)
{
    HRESULT hr = XGameActivationRegisterForEvent(nullptr, this, [](void* context, const XGameActivationInfo* activationInfo)
    {
        // only handle Protocol Activations in this sample
        if (activationInfo->type != XGameActivationType::Protocol)
        {
            LOG("Received activation that was not a protocol activation, ignoring.\n");
            return;
        }

        Sample* sample = static_cast<Sample*>(context);

        // Deep Link activations are started as:
        //   ms-windows-store://launch?productid=PRODID&path=PATH
        //     ...but arrive to the title as:
        //   ms-xbl-TITLEID://PATH/

        // Additional querystring params must be urlencoded on the source side, but arrive decoded on the title side:
        // Ensure that the querystring is appended to the path as "%2F%3Fkey1%3Dvalue1%26key2%3Dvalue2" on the source side
        // This will arrive as "/?key1=value1&key2=value2" on the title side.
        //   ms-windows-store://launch?productId=PRODID&path=store%2F%3Foffer%3D1234%26key%3Dvalue"
        //      ...but arrives to the title as:
        //   ms-xbl-TITLEID://store/?offer=1234&key=value
        // %26 &
        // %2F /
        // %3F ?
        // %3D =

        std::string uri = activationInfo->protocolUri;

        // find the start of the path/verb portion of the uri
        auto pos = uri.find("://");
        if (pos == std::string::npos)
        {
            LOG("Could not parse protocol URI: %s\n", uri.c_str());
            return;
        }
        pos += 3;

        // split the remainder into "path" and "querystring" on the first '?'
        std::string rest = uri.substr(pos);
        std::string path;
        std::string querystring;

        auto queryPos = rest.find('?');
        if (queryPos == std::string::npos)
        {
            path = rest;
        }
        else
        {
            path = rest.substr(0, queryPos);
            querystring = rest.substr(queryPos + 1);
        }

        // strip a trailing '/' from the path (the platform appends one when there is no querystring)
        if (!path.empty() && path.back() == '/')
        {
            path.pop_back();
        }

        // walk the querystring, splitting "key1=value1&key2=value2" into a map
        std::map<std::string, std::string> queryStrings;
        for (size_t start = 0; start < querystring.size(); )
        {
            size_t amp = querystring.find('&', start);
            if (amp == std::string::npos)
            {
                amp = querystring.size();
            }

            std::string pair = querystring.substr(start, amp - start);
            auto eq = pair.find('=');
            if (eq != std::string::npos)
            {
                queryStrings[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
            else if (!pair.empty())
            {
                queryStrings[pair] = "";
            }

            start = amp + 1;
        }

        // tell the sample to navigate to that path
        sample->NavigateTo(uri, path, queryStrings);

    }, &m_activationToken);

    LOG("XGameActivationRegisterForEvent: 0x%08X\n", hr);
}

void Sample::NavigateTo(const std::string& uri, const std::string& path, const std::map<std::string, std::string>& queryStrings)
{
    // cache the uri/path/querystrings for display later
    m_uri = uri;
    m_path = path;
    m_queryStrings = queryStrings;

    // look up the path in our map to determine which mode to enter
    auto it = s_pathToGameMode.find(path);
    m_mode = (it != s_pathToGameMode.end()) ? it->second : GameMode::None;
    LOG("Navigating to path: '%s' (mode %d), %zu querystring param(s)\n", path.c_str(), static_cast<int>(m_mode), queryStrings.size());
}

void Sample::Update()
{
}

void Sample::Draw()
{
    ImGuiAtg::BeginFullscreenLayout();

    ImGui::CollapsingHeader("ATG DeepLinking Sample", ImGuiTreeNodeFlags_Leaf);

    // Reserve space for the standard sample footer below the split
    float footerH = ImGuiAtg::GetFooterHeight();
    ImGui::BeginChild("##SplitArea", ImVec2(0, ImGui::GetContentRegionAvail().y - footerH));

    // Content on left, log on right, with draggable splitter
    ImGuiAtg::BeginSplitH("##LogSplit", 300.0f);
        ImGui::BeginChild("##Content", ImVec2(0, ImGui::GetContentRegionAvail().y));

            switch(m_mode)
            {
                case GameMode::None:
                    DrawHeader(ImVec4(0, 1, 0, 1), "TITLE SCREEN");
                    ImGui::Separator();
                    ImGui::Text("This is the title screen.");
                    break;
                case GameMode::Lobby:
                    DrawHeader(ImVec4(0, 1, 1, 1), "LOBBY");
                    ImGui::Separator();
                    ImGui::Text("This is the lobby screen.");
                    break;
                case GameMode::Campaign:
                    DrawHeader(ImVec4(1, 0, 0, 1), "CAMPAIGN");
                    ImGui::Separator();
                    ImGui::Text("This is campaign/single player mode screen.");
                    break;
                case GameMode::Shop:
                    DrawHeader(ImVec4(1, 0, 1, 1), "SHOP");
                    ImGui::Separator();
                    ImGui::Text("This is the in-game shop screen.");

                    // example of pulling specific values out of the parsed querystring
                    auto idIt = m_queryStrings.find("id");
                    if (idIt != m_queryStrings.end())
                    {
                        ImGui::Text("Showing item id: %s", idIt->second.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("(no 'id' querystring parameter was provided)");
                    }

                    auto qtyIt = m_queryStrings.find("qty");
                    if (qtyIt != m_queryStrings.end())
                    {
                        ImGui::Text("Quantity: %s", qtyIt->second.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("(no 'qty' querystring parameter was provided)");
                    }
                    break;
            }

            ImGui::Dummy(ImVec2(0, 100));

            ImGui::CollapsingHeader("Debug Info", ImGuiTreeNodeFlags_Leaf);

            ImGui::Text("Full URI:");
            ImGui::SameLine();
            if (m_uri.empty())
            {
                ImGui::TextDisabled("(none)");
            }
            else
            {
                ImGui::TextWrapped("%s", m_uri.c_str());
            }

            ImGui::Text("Parsed Path:");
            ImGui::SameLine();
            if (m_path.empty())
            {
                ImGui::TextDisabled("(none)");
            }
            else
            {
                ImGui::TextWrapped("%s", m_path.c_str());
            }

            ImGui::Text("Query String:");
            if (m_queryStrings.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(none)");
            }
            else
            {
                ImGui::Indent();
                for (const auto& kv : m_queryStrings)
                {
                    ImGui::Text("%s = %s", kv.first.c_str(), kv.second.c_str());
                }
                ImGui::Unindent();
            }

        ImGui::EndChild();

        ImGuiAtg::SplitNext();
            ImGuiAtg::DrawLogPanel();
        ImGuiAtg::EndSplit();

    ImGui::EndChild();

    ImGuiAtg::DrawFooter();

    ImGuiAtg::EndFullscreenLayout();
}

void Sample::Shutdown()
{
    // Wait (true) to ensure no in-flight activation callbacks can still reference `this` during/after shutdown.
    XGameActivationUnregisterForEvent(m_activationToken, true);
}

void Sample::Activated()
{
}

void Sample::Deactivated()
{
}

LRESULT Sample::WndProcHandler(HWND /*hWnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return 0;
}

#ifdef _GAMING_XBOX
void Sample::Suspend(ImGuiAtg::DeviceContext* dc)
{
    dc->Suspend();
}

void Sample::Resume(ImGuiAtg::DeviceContext* dc)
{
    dc->Resume();
}
#endif
