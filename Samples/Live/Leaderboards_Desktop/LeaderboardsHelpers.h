//--------------------------------------------------------------------------------------
// LeaderboardsHelpers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace LBHelpers
{
    // Utility functions to create a leaderboard query
    void InitDefaultLeaderboardQuery(
        __inout XblLeaderboardQuery &query,
        const std::string &serviceConfigId,
        const std::string &leaderboardName,
        const std::string &statName,
        XblLeaderboardSortOrder sortOrder,
        uint32_t maxItems)
    {
        query = {};
        strcpy_s(query.scid, sizeof(query.scid), serviceConfigId.c_str());

        query.order = sortOrder;
        query.maxItems = maxItems;

        query.statName = statName.c_str();
        query.leaderboardName = leaderboardName.c_str();
    }

    void InitSocialQuery(__inout XblLeaderboardQuery &query, XblSocialGroupType socialGroupType, uint64_t xboxUserId)
    {
        assert(socialGroupType != XblSocialGroupType::None);
        assert(xboxUserId != 0);

        query.socialGroup = socialGroupType;
        query.xboxUserId = xboxUserId;
    }

    void InitAdditionalColumns(__inout XblLeaderboardQuery &query, const char ** columns, size_t columnCount)
    {
        assert(columns != nullptr);
        assert(columnCount > 0);

        query.additionalColumnleaderboardNames = columns;
        query.additionalColumnleaderboardNamesCount = columnCount;
    }
}
