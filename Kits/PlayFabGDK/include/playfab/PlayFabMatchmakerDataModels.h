#pragma once

#if defined(ENABLE_PLAYFABSERVER_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace MatchmakerModels
    {
        // Matchmaker Enums
        enum class Region
        {
            RegionUSCentral,
            RegionUSEast,
            RegionEUWest,
            RegionSingapore,
            RegionJapan,
            RegionBrazil,
            RegionAustralia
        };

        inline void ToJsonEnum(const Region input, Json::Value& output)
        {
            if (input == Region::RegionUSCentral)
            {
                output = Json::Value("USCentral");
                return;
            }
            if (input == Region::RegionUSEast)
            {
                output = Json::Value("USEast");
                return;
            }
            if (input == Region::RegionEUWest)
            {
                output = Json::Value("EUWest");
                return;
            }
            if (input == Region::RegionSingapore)
            {
                output = Json::Value("Singapore");
                return;
            }
            if (input == Region::RegionJapan)
            {
                output = Json::Value("Japan");
                return;
            }
            if (input == Region::RegionBrazil)
            {
                output = Json::Value("Brazil");
                return;
            }
            if (input == Region::RegionAustralia)
            {
                output = Json::Value("Australia");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, Region& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "USCentral")
            {
                output = Region::RegionUSCentral;
                return;
            }
            if (inputStr == "USEast")
            {
                output = Region::RegionUSEast;
                return;
            }
            if (inputStr == "EUWest")
            {
                output = Region::RegionEUWest;
                return;
            }
            if (inputStr == "Singapore")
            {
                output = Region::RegionSingapore;
                return;
            }
            if (inputStr == "Japan")
            {
                output = Region::RegionJapan;
                return;
            }
            if (inputStr == "Brazil")
            {
                output = Region::RegionBrazil;
                return;
            }
            if (inputStr == "Australia")
            {
                output = Region::RegionAustralia;
                return;
            }
        }

        // Matchmaker Classes
        struct AuthUserRequest : public PlayFabRequestCommon
        {
            std::string AuthorizationTicket;

            AuthUserRequest() :
                PlayFabRequestCommon(),
                AuthorizationTicket()
            {}

            AuthUserRequest(const AuthUserRequest& src) :
                PlayFabRequestCommon(),
                AuthorizationTicket(src.AuthorizationTicket)
            {}

            ~AuthUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthorizationTicket"], AuthorizationTicket);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthorizationTicket; ToJsonUtilS(AuthorizationTicket, each_AuthorizationTicket); output["AuthorizationTicket"] = each_AuthorizationTicket;
                return output;
            }
        };

        struct AuthUserResponse : public PlayFabResultCommon
        {
            bool Authorized;
            std::string PlayFabId;

            AuthUserResponse() :
                PlayFabResultCommon(),
                Authorized(),
                PlayFabId()
            {}

            AuthUserResponse(const AuthUserResponse& src) :
                PlayFabResultCommon(),
                Authorized(src.Authorized),
                PlayFabId(src.PlayFabId)
            {}

            ~AuthUserResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Authorized"], Authorized);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Authorized; ToJsonUtilP(Authorized, each_Authorized); output["Authorized"] = each_Authorized;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct ItemInstance : public PlayFabBaseModel
        {
            std::string Annotation;
            std::list<std::string> BundleContents;
            std::string BundleParent;
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomData;
            std::string DisplayName;
            Boxed<time_t> Expiration;
            std::string ItemClass;
            std::string ItemId;
            std::string ItemInstanceId;
            Boxed<time_t> PurchaseDate;
            Boxed<Int32> RemainingUses;
            std::string UnitCurrency;
            Uint32 UnitPrice;
            Boxed<Int32> UsesIncrementedBy;

            ItemInstance() :
                PlayFabBaseModel(),
                Annotation(),
                BundleContents(),
                BundleParent(),
                CatalogVersion(),
                CustomData(),
                DisplayName(),
                Expiration(),
                ItemClass(),
                ItemId(),
                ItemInstanceId(),
                PurchaseDate(),
                RemainingUses(),
                UnitCurrency(),
                UnitPrice(),
                UsesIncrementedBy()
            {}

            ItemInstance(const ItemInstance& src) :
                PlayFabBaseModel(),
                Annotation(src.Annotation),
                BundleContents(src.BundleContents),
                BundleParent(src.BundleParent),
                CatalogVersion(src.CatalogVersion),
                CustomData(src.CustomData),
                DisplayName(src.DisplayName),
                Expiration(src.Expiration),
                ItemClass(src.ItemClass),
                ItemId(src.ItemId),
                ItemInstanceId(src.ItemInstanceId),
                PurchaseDate(src.PurchaseDate),
                RemainingUses(src.RemainingUses),
                UnitCurrency(src.UnitCurrency),
                UnitPrice(src.UnitPrice),
                UsesIncrementedBy(src.UsesIncrementedBy)
            {}

            ~ItemInstance() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Annotation"], Annotation);
                FromJsonUtilS(input["BundleContents"], BundleContents);
                FromJsonUtilS(input["BundleParent"], BundleParent);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomData"], CustomData);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilT(input["Expiration"], Expiration);
                FromJsonUtilS(input["ItemClass"], ItemClass);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilT(input["PurchaseDate"], PurchaseDate);
                FromJsonUtilP(input["RemainingUses"], RemainingUses);
                FromJsonUtilS(input["UnitCurrency"], UnitCurrency);
                FromJsonUtilP(input["UnitPrice"], UnitPrice);
                FromJsonUtilP(input["UsesIncrementedBy"], UsesIncrementedBy);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Annotation; ToJsonUtilS(Annotation, each_Annotation); output["Annotation"] = each_Annotation;
                Json::Value each_BundleContents; ToJsonUtilS(BundleContents, each_BundleContents); output["BundleContents"] = each_BundleContents;
                Json::Value each_BundleParent; ToJsonUtilS(BundleParent, each_BundleParent); output["BundleParent"] = each_BundleParent;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomData; ToJsonUtilS(CustomData, each_CustomData); output["CustomData"] = each_CustomData;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_Expiration; ToJsonUtilT(Expiration, each_Expiration); output["Expiration"] = each_Expiration;
                Json::Value each_ItemClass; ToJsonUtilS(ItemClass, each_ItemClass); output["ItemClass"] = each_ItemClass;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_PurchaseDate; ToJsonUtilT(PurchaseDate, each_PurchaseDate); output["PurchaseDate"] = each_PurchaseDate;
                Json::Value each_RemainingUses; ToJsonUtilP(RemainingUses, each_RemainingUses); output["RemainingUses"] = each_RemainingUses;
                Json::Value each_UnitCurrency; ToJsonUtilS(UnitCurrency, each_UnitCurrency); output["UnitCurrency"] = each_UnitCurrency;
                Json::Value each_UnitPrice; ToJsonUtilP(UnitPrice, each_UnitPrice); output["UnitPrice"] = each_UnitPrice;
                Json::Value each_UsesIncrementedBy; ToJsonUtilP(UsesIncrementedBy, each_UsesIncrementedBy); output["UsesIncrementedBy"] = each_UsesIncrementedBy;
                return output;
            }
        };

        struct PlayerJoinedRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string LobbyId;
            std::string PlayFabId;

            PlayerJoinedRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                LobbyId(),
                PlayFabId()
            {}

            PlayerJoinedRequest(const PlayerJoinedRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                LobbyId(src.LobbyId),
                PlayFabId(src.PlayFabId)
            {}

            ~PlayerJoinedRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["LobbyId"], LobbyId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_LobbyId; ToJsonUtilS(LobbyId, each_LobbyId); output["LobbyId"] = each_LobbyId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct PlayerJoinedResponse : public PlayFabResultCommon
        {

            PlayerJoinedResponse() :
                PlayFabResultCommon()
            {}

            PlayerJoinedResponse(const PlayerJoinedResponse&) :
                PlayFabResultCommon()
            {}

            ~PlayerJoinedResponse() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct PlayerLeftRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string LobbyId;
            std::string PlayFabId;

            PlayerLeftRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                LobbyId(),
                PlayFabId()
            {}

            PlayerLeftRequest(const PlayerLeftRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                LobbyId(src.LobbyId),
                PlayFabId(src.PlayFabId)
            {}

            ~PlayerLeftRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["LobbyId"], LobbyId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_LobbyId; ToJsonUtilS(LobbyId, each_LobbyId); output["LobbyId"] = each_LobbyId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct PlayerLeftResponse : public PlayFabResultCommon
        {

            PlayerLeftResponse() :
                PlayFabResultCommon()
            {}

            PlayerLeftResponse(const PlayerLeftResponse&) :
                PlayFabResultCommon()
            {}

            ~PlayerLeftResponse() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct StartGameRequest : public PlayFabRequestCommon
        {
            std::string Build;
            std::string CustomCommandLineData;
            std::map<std::string, std::string> CustomTags;
            std::string ExternalMatchmakerEventEndpoint;
            std::string GameMode;
            Region pfRegion;

            StartGameRequest() :
                PlayFabRequestCommon(),
                Build(),
                CustomCommandLineData(),
                CustomTags(),
                ExternalMatchmakerEventEndpoint(),
                GameMode(),
                pfRegion()
            {}

            StartGameRequest(const StartGameRequest& src) :
                PlayFabRequestCommon(),
                Build(src.Build),
                CustomCommandLineData(src.CustomCommandLineData),
                CustomTags(src.CustomTags),
                ExternalMatchmakerEventEndpoint(src.ExternalMatchmakerEventEndpoint),
                GameMode(src.GameMode),
                pfRegion(src.pfRegion)
            {}

            ~StartGameRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Build"], Build);
                FromJsonUtilS(input["CustomCommandLineData"], CustomCommandLineData);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExternalMatchmakerEventEndpoint"], ExternalMatchmakerEventEndpoint);
                FromJsonUtilS(input["GameMode"], GameMode);
                FromJsonEnum(input["Region"], pfRegion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Build; ToJsonUtilS(Build, each_Build); output["Build"] = each_Build;
                Json::Value each_CustomCommandLineData; ToJsonUtilS(CustomCommandLineData, each_CustomCommandLineData); output["CustomCommandLineData"] = each_CustomCommandLineData;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExternalMatchmakerEventEndpoint; ToJsonUtilS(ExternalMatchmakerEventEndpoint, each_ExternalMatchmakerEventEndpoint); output["ExternalMatchmakerEventEndpoint"] = each_ExternalMatchmakerEventEndpoint;
                Json::Value each_GameMode; ToJsonUtilS(GameMode, each_GameMode); output["GameMode"] = each_GameMode;
                Json::Value each_pfRegion; ToJsonEnum(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                return output;
            }
        };

        struct StartGameResponse : public PlayFabResultCommon
        {
            std::string GameID;
            std::string ServerIPV4Address;
            std::string ServerIPV6Address;
            Uint32 ServerPort;
            std::string ServerPublicDNSName;

            StartGameResponse() :
                PlayFabResultCommon(),
                GameID(),
                ServerIPV4Address(),
                ServerIPV6Address(),
                ServerPort(),
                ServerPublicDNSName()
            {}

            StartGameResponse(const StartGameResponse& src) :
                PlayFabResultCommon(),
                GameID(src.GameID),
                ServerIPV4Address(src.ServerIPV4Address),
                ServerIPV6Address(src.ServerIPV6Address),
                ServerPort(src.ServerPort),
                ServerPublicDNSName(src.ServerPublicDNSName)
            {}

            ~StartGameResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GameID"], GameID);
                FromJsonUtilS(input["ServerIPV4Address"], ServerIPV4Address);
                FromJsonUtilS(input["ServerIPV6Address"], ServerIPV6Address);
                FromJsonUtilP(input["ServerPort"], ServerPort);
                FromJsonUtilS(input["ServerPublicDNSName"], ServerPublicDNSName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameID; ToJsonUtilS(GameID, each_GameID); output["GameID"] = each_GameID;
                Json::Value each_ServerIPV4Address; ToJsonUtilS(ServerIPV4Address, each_ServerIPV4Address); output["ServerIPV4Address"] = each_ServerIPV4Address;
                Json::Value each_ServerIPV6Address; ToJsonUtilS(ServerIPV6Address, each_ServerIPV6Address); output["ServerIPV6Address"] = each_ServerIPV6Address;
                Json::Value each_ServerPort; ToJsonUtilP(ServerPort, each_ServerPort); output["ServerPort"] = each_ServerPort;
                Json::Value each_ServerPublicDNSName; ToJsonUtilS(ServerPublicDNSName, each_ServerPublicDNSName); output["ServerPublicDNSName"] = each_ServerPublicDNSName;
                return output;
            }
        };

        struct UserInfoRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 MinCatalogVersion;
            std::string PlayFabId;

            UserInfoRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                MinCatalogVersion(),
                PlayFabId()
            {}

            UserInfoRequest(const UserInfoRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                MinCatalogVersion(src.MinCatalogVersion),
                PlayFabId(src.PlayFabId)
            {}

            ~UserInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["MinCatalogVersion"], MinCatalogVersion);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MinCatalogVersion; ToJsonUtilP(MinCatalogVersion, each_MinCatalogVersion); output["MinCatalogVersion"] = each_MinCatalogVersion;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct VirtualCurrencyRechargeTime : public PlayFabBaseModel
        {
            Int32 RechargeMax;
            time_t RechargeTime;
            Int32 SecondsToRecharge;

            VirtualCurrencyRechargeTime() :
                PlayFabBaseModel(),
                RechargeMax(),
                RechargeTime(),
                SecondsToRecharge()
            {}

            VirtualCurrencyRechargeTime(const VirtualCurrencyRechargeTime& src) :
                PlayFabBaseModel(),
                RechargeMax(src.RechargeMax),
                RechargeTime(src.RechargeTime),
                SecondsToRecharge(src.SecondsToRecharge)
            {}

            ~VirtualCurrencyRechargeTime() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["RechargeMax"], RechargeMax);
                FromJsonUtilT(input["RechargeTime"], RechargeTime);
                FromJsonUtilP(input["SecondsToRecharge"], SecondsToRecharge);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_RechargeMax; ToJsonUtilP(RechargeMax, each_RechargeMax); output["RechargeMax"] = each_RechargeMax;
                Json::Value each_RechargeTime; ToJsonUtilT(RechargeTime, each_RechargeTime); output["RechargeTime"] = each_RechargeTime;
                Json::Value each_SecondsToRecharge; ToJsonUtilP(SecondsToRecharge, each_SecondsToRecharge); output["SecondsToRecharge"] = each_SecondsToRecharge;
                return output;
            }
        };

        struct UserInfoResponse : public PlayFabResultCommon
        {
            std::list<ItemInstance> Inventory;
            bool IsDeveloper;
            std::string PlayFabId;
            std::string SteamId;
            std::string TitleDisplayName;
            std::string Username;
            std::map<std::string, Int32> VirtualCurrency;
            std::map<std::string, VirtualCurrencyRechargeTime> VirtualCurrencyRechargeTimes;

            UserInfoResponse() :
                PlayFabResultCommon(),
                Inventory(),
                IsDeveloper(),
                PlayFabId(),
                SteamId(),
                TitleDisplayName(),
                Username(),
                VirtualCurrency(),
                VirtualCurrencyRechargeTimes()
            {}

            UserInfoResponse(const UserInfoResponse& src) :
                PlayFabResultCommon(),
                Inventory(src.Inventory),
                IsDeveloper(src.IsDeveloper),
                PlayFabId(src.PlayFabId),
                SteamId(src.SteamId),
                TitleDisplayName(src.TitleDisplayName),
                Username(src.Username),
                VirtualCurrency(src.VirtualCurrency),
                VirtualCurrencyRechargeTimes(src.VirtualCurrencyRechargeTimes)
            {}

            ~UserInfoResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Inventory"], Inventory);
                FromJsonUtilP(input["IsDeveloper"], IsDeveloper);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["SteamId"], SteamId);
                FromJsonUtilS(input["TitleDisplayName"], TitleDisplayName);
                FromJsonUtilS(input["Username"], Username);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
                FromJsonUtilO(input["VirtualCurrencyRechargeTimes"], VirtualCurrencyRechargeTimes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Inventory; ToJsonUtilO(Inventory, each_Inventory); output["Inventory"] = each_Inventory;
                Json::Value each_IsDeveloper; ToJsonUtilP(IsDeveloper, each_IsDeveloper); output["IsDeveloper"] = each_IsDeveloper;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_SteamId; ToJsonUtilS(SteamId, each_SteamId); output["SteamId"] = each_SteamId;
                Json::Value each_TitleDisplayName; ToJsonUtilS(TitleDisplayName, each_TitleDisplayName); output["TitleDisplayName"] = each_TitleDisplayName;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                Json::Value each_VirtualCurrencyRechargeTimes; ToJsonUtilO(VirtualCurrencyRechargeTimes, each_VirtualCurrencyRechargeTimes); output["VirtualCurrencyRechargeTimes"] = each_VirtualCurrencyRechargeTimes;
                return output;
            }
        };

    }
}

#endif
