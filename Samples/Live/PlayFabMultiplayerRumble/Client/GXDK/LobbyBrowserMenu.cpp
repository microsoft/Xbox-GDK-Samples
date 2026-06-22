//--------------------------------------------------------------------------------------
// LobbyBrowserMenu.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"
#include "LobbyBrowserMenu.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

LobbyBrowserMenu::LobbyBrowserMenu() :
    MenuScreen(),
    m_isSearching(false),
    m_pageNumber(0),
    m_pageCount(0),
    m_numberOfResults(0)
{
    auto onlineManager = Managers::Get<OnlineManager>();
    onlineManager->SetLobbySearchCallback(
        [this](uint32_t size, const PFLobbySearchResult* results)
        {
            m_searchResults.clear();

            for (size_t i = 0; i < size; i++)
            {
                auto result = results[i];

                LobbySearchResult searchResult{};
                searchResult.Name = std::string("Lobby ") + std::to_string(i + 1);
                searchResult.ConnectionString = std::string(result.connectionString);
                searchResult.CurrentMemberCount = result.currentMemberCount;
                searchResult.MaxMemberCount = result.maxMemberCount;

                m_searchResults.push_back(searchResult);
            }

            m_pageNumber = 0;
            m_pageCount = static_cast<uint32_t>(std::floor(size / static_cast<float>(MAX_RESULTS_PER_PAGE)));
            m_numberOfResults = size;

            Refresh();

            m_isSearching = false;
        });

    m_transitionOnTime = 1.0;
    m_transitionOffTime = 1.0;

    Refresh();
}

LobbyBrowserMenu::~LobbyBrowserMenu()
{
    auto onlineManager = Managers::Get<OnlineManager>();
    onlineManager->SetLobbySearchCallback(nullptr);
}

void LobbyBrowserMenu::Draw(float totalTime, float elapsedTime)
{
    auto renderContext = Managers::Get<RenderManager>()->GetRenderContext();

    auto contentManager = Managers::Get<ContentManager>();

    auto spriteFont = contentManager->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
    float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
    float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

    const auto searchingColor = Colors::Yellow;
    float scale = 0.5f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

    renderContext->Begin();

    std::string numberOfResults = std::to_string(m_numberOfResults);
    std::string searchMessage = m_isSearching ? "Searching for lobbies..." : m_numberOfResults == 1 ? numberOfResults + " lobby found." : numberOfResults + " lobbies found.";

    if (m_state == ScreenStateType::Active)
    {
        SimpleMath::Vector2 messagePosition = SimpleMath::Vector2(0, viewportHeight / 2.0f - 10);
        XMVECTOR size = spriteFont->MeasureString(searchMessage.c_str());
        messagePosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;
        renderContext->DrawString(spriteFont, searchMessage, messagePosition, searchingColor, 0, DirectX::XMFLOAT2{ 0,0 }, scale);
    }

    renderContext->End();

    MenuScreen::Draw(totalTime, elapsedTime);
}

void LobbyBrowserMenu::OnCancel()
{
    ExitScreen();
}

void LobbyBrowserMenu::BeginSearch()
{
    if (m_isSearching)
    {
        return;
    }

    m_isSearching = true;
    auto onlineManager = Managers::Get<OnlineManager>();
    onlineManager->FindLobbies();
}

void LobbyBrowserMenu::NextPage()
{
    if (m_pageNumber < m_pageCount)
    {
        m_pageNumber++;
        Refresh();
    }
}

void LobbyBrowserMenu::PreviousPage()
{
    if (m_pageNumber > 0)
    {
        m_pageNumber--;
        Refresh();
    }
}

void LobbyBrowserMenu::Refresh()
{
    m_menuEntries.clear();

    // add lobbies
    if (m_numberOfResults > 0)
    {
        uint32_t startIndex = m_pageNumber * MAX_RESULTS_PER_PAGE;
        uint32_t endIndex = startIndex + MAX_RESULTS_PER_PAGE;
        endIndex = endIndex <= m_numberOfResults ? endIndex : m_numberOfResults;

        for (size_t i = startIndex; i < endIndex; i++)
        {
            LobbySearchResult searchResult = m_searchResults[i];

            std::string lobbyName = m_searchResults[i].Name + std::string(" - ");
            lobbyName += std::to_string(m_searchResults[i].CurrentMemberCount) + std::string(" / ") + std::to_string(m_searchResults[i].MaxMemberCount);

            m_menuEntries.push_back(MenuEntry(
                lobbyName,
                [searchResult]()
                {
                    auto onlineManager = Managers::Get<OnlineManager>();
                    onlineManager->JoinMultiplayerGame(searchResult.ConnectionString);
                },
                nullptr,
                ""));
        }

        // add page switching
        if (m_numberOfResults > MAX_RESULTS_PER_PAGE)
        {
            m_menuEntries.push_back(MenuEntry(
                "NEXT PAGE",
                [this]()
                {
                    NextPage();
                },
                nullptr,
                    ""));

            m_menuEntries.push_back(MenuEntry(
                "PREVIOUS PAGE",
                [this]()
                {
                    PreviousPage();
                },
                nullptr,
                    ""));
        }
    }

    m_menuEntries.push_back(MenuEntry(
        "SEARCH",
        [this]()
        {
            BeginSearch();
        },
        nullptr,
        ""));

    m_menuEntries.push_back(MenuEntry(
        "CLOSE",
        [this]()
        {
            ExitScreen();
        },
        nullptr,
        ""));
}
