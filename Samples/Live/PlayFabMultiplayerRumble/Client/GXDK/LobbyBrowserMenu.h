//--------------------------------------------------------------------------------------
// LobbyBrowserMenu.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace PlayFabMultiplayerRumble
{

class LobbyBrowserMenu : public MenuScreen
{
public:
    LobbyBrowserMenu();
    virtual ~LobbyBrowserMenu();

    virtual void Draw(float totalTime, float elapsedTime) override;

protected:
    virtual void OnCancel() override;

private:
    const uint32_t MAX_RESULTS_PER_PAGE = 3;

    struct LobbySearchResult
    {
        std::string Name;
        std::string ConnectionString;
        uint32_t CurrentMemberCount;
        uint32_t MaxMemberCount;
    };

    std::vector<LobbySearchResult> m_searchResults;
    bool m_isSearching;
    uint32_t m_pageNumber;
    uint32_t m_pageCount;
    uint32_t m_numberOfResults;

    void BeginSearch();
    void NextPage();
    void PreviousPage();
    void Refresh();
};

}


