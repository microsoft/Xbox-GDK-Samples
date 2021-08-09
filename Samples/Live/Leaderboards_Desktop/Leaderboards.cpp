//--------------------------------------------------------------------------------------
// Leaderboards.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// SAMPLE NOTE:
// The sample class is split between two implementation files to allow one file
// to focus on the statistics and leaderboard calls specifically while the other
// handles rendering boilerplate and UI interactions.
//
// This is the statistics and leaderboards usage file
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Leaderboards.h"
#include "ExploreGameSimulation.h"
#include "LeaderboardsHelpers.h"

constexpr int c_maxItemsPerQuery = 5; // Deliberately set low in this sample to demonstrate querying multiple pages

// Initiates a leaderboard query
void Sample::QueryLeaderboards(
    const std::string &leaderboardName, // Name of the leaderboard from Partner Center
    const std::string &statName,        // Stat Name the leaderboard is based off of from Partner Center
    XblSocialGroupType queryGroupType,  // Should the query use the Global or Social leaderboards
    const char **additionalColumns,     // Additional columns to include with the leaderboards, defined in Partner Center
    size_t additionalColumnsCount,      // Number of additional columns
    std::function<void(HRESULT, LeaderboardsQueryContext *)> onResultsReceived // Results handler for this sample
)
{
    XblLeaderboardQuery query;
    LBHelpers::InitDefaultLeaderboardQuery(query, m_liveResources->GetServiceConfigId(), leaderboardName, statName, XblLeaderboardSortOrder::Descending, c_maxItemsPerQuery);

    if (queryGroupType != XblSocialGroupType::None)
    {
        // Social queries must include a valid XUID
        // If the user has no friends or favorited people who have played this sample, only this user's results will be returned
        LBHelpers::InitSocialQuery(query, XblSocialGroupType::People, m_liveResources->GetXuid());
    }

    if (additionalColumns && additionalColumnsCount > 0)
    {
        // Additional columns are not aggregated, but rather the "latest" value at the time the stat was changed
        // This is useful for stats which use MIN/MAX aggregations in their stat rules to include information that triggered the update
        // (e.g. MostItemsFound includes the "Environment" and "DistanceTraveled" fields to indicate
        //       where those items were found and how far they traveled).
        LBHelpers::InitAdditionalColumns(query, additionalColumns, additionalColumnsCount);
    }

    auto *ctx = new LeaderboardsQueryContext{};
    ctx->sample = this;
    ctx->onResultsReceived = onResultsReceived;
    ctx->maxItems = query.maxItems;

    ctx->async.queue = m_mainAsyncQueue;
    ctx->async.context = ctx;
    ctx->async.callback = [](XAsyncBlock *async)
    {
        Sample::ProcessLeaderboardResults(async);
    };

    auto hr = XblLeaderboardGetLeaderboardAsync(m_liveResources->GetLiveContext(), query, &ctx->async);
    if (FAILED(hr))
    {
        onResultsReceived(hr, ctx);
        delete ctx;
    }
}

// Displays and fetches additional leaderboard records from the service
void Sample::ProcessLeaderboardResults(XAsyncBlock *async)
{
    auto ctx = static_cast<LeaderboardsQueryContext *>(async->context);

    size_t resultSize = 0;

    HRESULT hr = S_OK;

    if (ctx->page == 0)
    {
        hr = XblLeaderboardGetLeaderboardResultSize(async, &resultSize);
        if (SUCCEEDED(hr))
        {
            ctx->resultData.resize(resultSize);

            hr = XblLeaderboardGetLeaderboardResult(async, ctx->resultData.size(), ctx->resultData.data(), &ctx->result, nullptr);
        }
        else
        {
            auto xberr = XblGetErrorCondition(hr);
            UNREFERENCED_PARAMETER(xberr); // Debug Tip: Get the bucketed error reason here (e.g. Auth, Network, etc.)
        }
    }
    else
    {
        hr = XblLeaderboardResultGetNextResultSize(async, &resultSize);
        if (SUCCEEDED(hr))
        {
            ctx->resultData.resize(resultSize);
            hr = XblLeaderboardResultGetNextResult(async, resultSize, ctx->resultData.data(), &ctx->result, nullptr);
        }
    }

    if (ctx->onResultsReceived)
    {
        ctx->onResultsReceived(hr, ctx);
    }

    if (SUCCEEDED(hr) && ctx->result && ctx->result->hasNext)
    {
        ctx->page++;
        ZeroMemory(&async->internal[0], _countof(async->internal));
        XblLeaderboardResultGetNextAsync(ctx->sample->m_liveResources->GetLiveContext(), ctx->result, ctx->maxItems, async);
    }
    else
    {
        delete ctx;
    }

}

// Queries an individual statistic for the current user
void Sample::QueryStatistics(const std::string &statName)
{
    struct GetStatisticContext
    {
        XAsyncBlock async;
        Sample *sample;
    };

    auto *ctx = new GetStatisticContext{ XAsyncBlock{}, this };

    ctx->async.context = ctx;
    ctx->async.queue = m_mainAsyncQueue;
    ctx->async.callback = [](XAsyncBlock *async)
    {
        auto ctx = static_cast<GetStatisticContext*>(async->context);
        std::vector<uint8_t> buffer;
        size_t size = 0;
        XblUserStatisticsResult *result;

        if (SUCCEEDED(XblUserStatisticsGetSingleUserStatisticResultSize(async, &size)))
        {
            buffer.reserve(size);
            if (SUCCEEDED(XblUserStatisticsGetSingleUserStatisticResult(async, size, buffer.data(), &result, &size)))
            {
                ctx->sample->RenderStatisticsResult(result);
            }
        }

        delete ctx;
    };

    if (FAILED(XblUserStatisticsGetSingleUserStatisticAsync(
        m_liveResources->GetLiveContext(),
        m_liveResources->GetXuid(),
        m_liveResources->GetServiceConfigId().c_str(),
        statName.c_str(),
        &ctx->async)))
    {
        delete ctx;
    }
}

// Simulate a play session
void Sample::PlayGame()
{
    auto result = ExploreGameSimulation::PlayGame();

    auto json = result.ToEventJson();

    WriteToLog(json, false);

    // While both properties and measurements are supplied, it's ok for one to be empty as they are merged server-side anyway
    XblEventsWriteInGameEvent(m_liveResources->GetLiveContext(), "AreaExplored", json.c_str(), "{}");
}
