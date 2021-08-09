#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace ProfilesModels
    {
        // Profiles Enums
        enum class EffectType
        {
            EffectTypeAllow,
            EffectTypeDeny
        };

        inline void ToJsonEnum(const EffectType input, Json::Value& output)
        {
            if (input == EffectType::EffectTypeAllow)
            {
                output = Json::Value("Allow");
                return;
            }
            if (input == EffectType::EffectTypeDeny)
            {
                output = Json::Value("Deny");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, EffectType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Allow")
            {
                output = EffectType::EffectTypeAllow;
                return;
            }
            if (inputStr == "Deny")
            {
                output = EffectType::EffectTypeDeny;
                return;
            }
        }

        enum class OperationTypes
        {
            OperationTypesCreated,
            OperationTypesUpdated,
            OperationTypesDeleted,
            OperationTypesNone
        };

        inline void ToJsonEnum(const OperationTypes input, Json::Value& output)
        {
            if (input == OperationTypes::OperationTypesCreated)
            {
                output = Json::Value("Created");
                return;
            }
            if (input == OperationTypes::OperationTypesUpdated)
            {
                output = Json::Value("Updated");
                return;
            }
            if (input == OperationTypes::OperationTypesDeleted)
            {
                output = Json::Value("Deleted");
                return;
            }
            if (input == OperationTypes::OperationTypesNone)
            {
                output = Json::Value("None");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, OperationTypes& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Created")
            {
                output = OperationTypes::OperationTypesCreated;
                return;
            }
            if (inputStr == "Updated")
            {
                output = OperationTypes::OperationTypesUpdated;
                return;
            }
            if (inputStr == "Deleted")
            {
                output = OperationTypes::OperationTypesDeleted;
                return;
            }
            if (inputStr == "None")
            {
                output = OperationTypes::OperationTypesNone;
                return;
            }
        }

        // Profiles Classes
        struct EntityDataObject : public PlayFabBaseModel
        {
            Json::Value DataObject;
            std::string EscapedDataObject;
            std::string ObjectName;

            EntityDataObject() :
                PlayFabBaseModel(),
                DataObject(),
                EscapedDataObject(),
                ObjectName()
            {}

            EntityDataObject(const EntityDataObject& src) :
                PlayFabBaseModel(),
                DataObject(src.DataObject),
                EscapedDataObject(src.EscapedDataObject),
                ObjectName(src.ObjectName)
            {}

            ~EntityDataObject() = default;

            void FromJson(const Json::Value& input) override
            {
                DataObject = input["DataObject"];
                FromJsonUtilS(input["EscapedDataObject"], EscapedDataObject);
                FromJsonUtilS(input["ObjectName"], ObjectName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["DataObject"] = DataObject;
                Json::Value each_EscapedDataObject; ToJsonUtilS(EscapedDataObject, each_EscapedDataObject); output["EscapedDataObject"] = each_EscapedDataObject;
                Json::Value each_ObjectName; ToJsonUtilS(ObjectName, each_ObjectName); output["ObjectName"] = each_ObjectName;
                return output;
            }
        };

        struct EntityKey : public PlayFabBaseModel
        {
            std::string Id;
            std::string Type;

            EntityKey() :
                PlayFabBaseModel(),
                Id(),
                Type()
            {}

            EntityKey(const EntityKey& src) :
                PlayFabBaseModel(),
                Id(src.Id),
                Type(src.Type)
            {}

            ~EntityKey() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Type"], Type);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Type; ToJsonUtilS(Type, each_Type); output["Type"] = each_Type;
                return output;
            }
        };

        struct EntityLineage : public PlayFabBaseModel
        {
            std::string CharacterId;
            std::string GroupId;
            std::string MasterPlayerAccountId;
            std::string NamespaceId;
            std::string TitleId;
            std::string TitlePlayerAccountId;

            EntityLineage() :
                PlayFabBaseModel(),
                CharacterId(),
                GroupId(),
                MasterPlayerAccountId(),
                NamespaceId(),
                TitleId(),
                TitlePlayerAccountId()
            {}

            EntityLineage(const EntityLineage& src) :
                PlayFabBaseModel(),
                CharacterId(src.CharacterId),
                GroupId(src.GroupId),
                MasterPlayerAccountId(src.MasterPlayerAccountId),
                NamespaceId(src.NamespaceId),
                TitleId(src.TitleId),
                TitlePlayerAccountId(src.TitlePlayerAccountId)
            {}

            ~EntityLineage() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["GroupId"], GroupId);
                FromJsonUtilS(input["MasterPlayerAccountId"], MasterPlayerAccountId);
                FromJsonUtilS(input["NamespaceId"], NamespaceId);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilS(input["TitlePlayerAccountId"], TitlePlayerAccountId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_GroupId; ToJsonUtilS(GroupId, each_GroupId); output["GroupId"] = each_GroupId;
                Json::Value each_MasterPlayerAccountId; ToJsonUtilS(MasterPlayerAccountId, each_MasterPlayerAccountId); output["MasterPlayerAccountId"] = each_MasterPlayerAccountId;
                Json::Value each_NamespaceId; ToJsonUtilS(NamespaceId, each_NamespaceId); output["NamespaceId"] = each_NamespaceId;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_TitlePlayerAccountId; ToJsonUtilS(TitlePlayerAccountId, each_TitlePlayerAccountId); output["TitlePlayerAccountId"] = each_TitlePlayerAccountId;
                return output;
            }
        };

        struct EntityPermissionStatement : public PlayFabBaseModel
        {
            std::string Action;
            std::string Comment;
            Json::Value Condition;
            EffectType Effect;
            Json::Value Principal;
            std::string Resource;

            EntityPermissionStatement() :
                PlayFabBaseModel(),
                Action(),
                Comment(),
                Condition(),
                Effect(),
                Principal(),
                Resource()
            {}

            EntityPermissionStatement(const EntityPermissionStatement& src) :
                PlayFabBaseModel(),
                Action(src.Action),
                Comment(src.Comment),
                Condition(src.Condition),
                Effect(src.Effect),
                Principal(src.Principal),
                Resource(src.Resource)
            {}

            ~EntityPermissionStatement() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Action"], Action);
                FromJsonUtilS(input["Comment"], Comment);
                Condition = input["Condition"];
                FromJsonEnum(input["Effect"], Effect);
                Principal = input["Principal"];
                FromJsonUtilS(input["Resource"], Resource);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Action; ToJsonUtilS(Action, each_Action); output["Action"] = each_Action;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                output["Condition"] = Condition;
                Json::Value each_Effect; ToJsonEnum(Effect, each_Effect); output["Effect"] = each_Effect;
                output["Principal"] = Principal;
                Json::Value each_Resource; ToJsonUtilS(Resource, each_Resource); output["Resource"] = each_Resource;
                return output;
            }
        };

        struct EntityProfileFileMetadata : public PlayFabBaseModel
        {
            std::string Checksum;
            std::string FileName;
            time_t LastModified;
            Int32 Size;

            EntityProfileFileMetadata() :
                PlayFabBaseModel(),
                Checksum(),
                FileName(),
                LastModified(),
                Size()
            {}

            EntityProfileFileMetadata(const EntityProfileFileMetadata& src) :
                PlayFabBaseModel(),
                Checksum(src.Checksum),
                FileName(src.FileName),
                LastModified(src.LastModified),
                Size(src.Size)
            {}

            ~EntityProfileFileMetadata() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Checksum"], Checksum);
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilT(input["LastModified"], LastModified);
                FromJsonUtilP(input["Size"], Size);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Checksum; ToJsonUtilS(Checksum, each_Checksum); output["Checksum"] = each_Checksum;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_LastModified; ToJsonUtilT(LastModified, each_LastModified); output["LastModified"] = each_LastModified;
                Json::Value each_Size; ToJsonUtilP(Size, each_Size); output["Size"] = each_Size;
                return output;
            }
        };

        struct EntityStatisticChildValue : public PlayFabBaseModel
        {
            std::string ChildName;
            std::string Metadata;
            Int32 Value;

            EntityStatisticChildValue() :
                PlayFabBaseModel(),
                ChildName(),
                Metadata(),
                Value()
            {}

            EntityStatisticChildValue(const EntityStatisticChildValue& src) :
                PlayFabBaseModel(),
                ChildName(src.ChildName),
                Metadata(src.Metadata),
                Value(src.Value)
            {}

            ~EntityStatisticChildValue() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ChildName"], ChildName);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ChildName; ToJsonUtilS(ChildName, each_ChildName); output["ChildName"] = each_ChildName;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct EntityStatisticValue : public PlayFabBaseModel
        {
            std::map<std::string, EntityStatisticChildValue> ChildStatistics;
            std::string Metadata;
            std::string Name;
            Boxed<Int32> Value;
            Int32 Version;

            EntityStatisticValue() :
                PlayFabBaseModel(),
                ChildStatistics(),
                Metadata(),
                Name(),
                Value(),
                Version()
            {}

            EntityStatisticValue(const EntityStatisticValue& src) :
                PlayFabBaseModel(),
                ChildStatistics(src.ChildStatistics),
                Metadata(src.Metadata),
                Name(src.Name),
                Value(src.Value),
                Version(src.Version)
            {}

            ~EntityStatisticValue() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["ChildStatistics"], ChildStatistics);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["Value"], Value);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ChildStatistics; ToJsonUtilO(ChildStatistics, each_ChildStatistics); output["ChildStatistics"] = each_ChildStatistics;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct EntityProfileBody : public PlayFabBaseModel
        {
            std::string AvatarUrl;
            time_t Created;
            std::string DisplayName;
            Boxed<EntityKey> Entity;
            std::string EntityChain;
            std::list<std::string> ExperimentVariants;
            std::map<std::string, EntityProfileFileMetadata> Files;
            std::string Language;
            std::string LeaderboardMetadata;
            Boxed<EntityLineage> Lineage;
            std::map<std::string, EntityDataObject> Objects;
            std::list<EntityPermissionStatement> Permissions;
            std::map<std::string, EntityStatisticValue> Statistics;
            Int32 VersionNumber;

            EntityProfileBody() :
                PlayFabBaseModel(),
                AvatarUrl(),
                Created(),
                DisplayName(),
                Entity(),
                EntityChain(),
                ExperimentVariants(),
                Files(),
                Language(),
                LeaderboardMetadata(),
                Lineage(),
                Objects(),
                Permissions(),
                Statistics(),
                VersionNumber()
            {}

            EntityProfileBody(const EntityProfileBody& src) :
                PlayFabBaseModel(),
                AvatarUrl(src.AvatarUrl),
                Created(src.Created),
                DisplayName(src.DisplayName),
                Entity(src.Entity),
                EntityChain(src.EntityChain),
                ExperimentVariants(src.ExperimentVariants),
                Files(src.Files),
                Language(src.Language),
                LeaderboardMetadata(src.LeaderboardMetadata),
                Lineage(src.Lineage),
                Objects(src.Objects),
                Permissions(src.Permissions),
                Statistics(src.Statistics),
                VersionNumber(src.VersionNumber)
            {}

            ~EntityProfileBody() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AvatarUrl"], AvatarUrl);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["EntityChain"], EntityChain);
                FromJsonUtilS(input["ExperimentVariants"], ExperimentVariants);
                FromJsonUtilO(input["Files"], Files);
                FromJsonUtilS(input["Language"], Language);
                FromJsonUtilS(input["LeaderboardMetadata"], LeaderboardMetadata);
                FromJsonUtilO(input["Lineage"], Lineage);
                FromJsonUtilO(input["Objects"], Objects);
                FromJsonUtilO(input["Permissions"], Permissions);
                FromJsonUtilO(input["Statistics"], Statistics);
                FromJsonUtilP(input["VersionNumber"], VersionNumber);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AvatarUrl; ToJsonUtilS(AvatarUrl, each_AvatarUrl); output["AvatarUrl"] = each_AvatarUrl;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_EntityChain; ToJsonUtilS(EntityChain, each_EntityChain); output["EntityChain"] = each_EntityChain;
                Json::Value each_ExperimentVariants; ToJsonUtilS(ExperimentVariants, each_ExperimentVariants); output["ExperimentVariants"] = each_ExperimentVariants;
                Json::Value each_Files; ToJsonUtilO(Files, each_Files); output["Files"] = each_Files;
                Json::Value each_Language; ToJsonUtilS(Language, each_Language); output["Language"] = each_Language;
                Json::Value each_LeaderboardMetadata; ToJsonUtilS(LeaderboardMetadata, each_LeaderboardMetadata); output["LeaderboardMetadata"] = each_LeaderboardMetadata;
                Json::Value each_Lineage; ToJsonUtilO(Lineage, each_Lineage); output["Lineage"] = each_Lineage;
                Json::Value each_Objects; ToJsonUtilO(Objects, each_Objects); output["Objects"] = each_Objects;
                Json::Value each_Permissions; ToJsonUtilO(Permissions, each_Permissions); output["Permissions"] = each_Permissions;
                Json::Value each_Statistics; ToJsonUtilO(Statistics, each_Statistics); output["Statistics"] = each_Statistics;
                Json::Value each_VersionNumber; ToJsonUtilP(VersionNumber, each_VersionNumber); output["VersionNumber"] = each_VersionNumber;
                return output;
            }
        };

        struct GetEntityProfileRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> DataAsObject;
            Boxed<EntityKey> Entity;

            GetEntityProfileRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DataAsObject(),
                Entity()
            {}

            GetEntityProfileRequest(const GetEntityProfileRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DataAsObject(src.DataAsObject),
                Entity(src.Entity)
            {}

            ~GetEntityProfileRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["DataAsObject"], DataAsObject);
                FromJsonUtilO(input["Entity"], Entity);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DataAsObject; ToJsonUtilP(DataAsObject, each_DataAsObject); output["DataAsObject"] = each_DataAsObject;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                return output;
            }
        };

        struct GetEntityProfileResponse : public PlayFabResultCommon
        {
            Boxed<EntityProfileBody> Profile;

            GetEntityProfileResponse() :
                PlayFabResultCommon(),
                Profile()
            {}

            GetEntityProfileResponse(const GetEntityProfileResponse& src) :
                PlayFabResultCommon(),
                Profile(src.Profile)
            {}

            ~GetEntityProfileResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Profile"], Profile);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Profile; ToJsonUtilO(Profile, each_Profile); output["Profile"] = each_Profile;
                return output;
            }
        };

        struct GetEntityProfilesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> DataAsObject;
            std::list<EntityKey> Entities;

            GetEntityProfilesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DataAsObject(),
                Entities()
            {}

            GetEntityProfilesRequest(const GetEntityProfilesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DataAsObject(src.DataAsObject),
                Entities(src.Entities)
            {}

            ~GetEntityProfilesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["DataAsObject"], DataAsObject);
                FromJsonUtilO(input["Entities"], Entities);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DataAsObject; ToJsonUtilP(DataAsObject, each_DataAsObject); output["DataAsObject"] = each_DataAsObject;
                Json::Value each_Entities; ToJsonUtilO(Entities, each_Entities); output["Entities"] = each_Entities;
                return output;
            }
        };

        struct GetEntityProfilesResponse : public PlayFabResultCommon
        {
            std::list<EntityProfileBody> Profiles;

            GetEntityProfilesResponse() :
                PlayFabResultCommon(),
                Profiles()
            {}

            GetEntityProfilesResponse(const GetEntityProfilesResponse& src) :
                PlayFabResultCommon(),
                Profiles(src.Profiles)
            {}

            ~GetEntityProfilesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Profiles"], Profiles);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Profiles; ToJsonUtilO(Profiles, each_Profiles); output["Profiles"] = each_Profiles;
                return output;
            }
        };

        struct GetGlobalPolicyRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetGlobalPolicyRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetGlobalPolicyRequest(const GetGlobalPolicyRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetGlobalPolicyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct GetGlobalPolicyResponse : public PlayFabResultCommon
        {
            std::list<EntityPermissionStatement> Permissions;

            GetGlobalPolicyResponse() :
                PlayFabResultCommon(),
                Permissions()
            {}

            GetGlobalPolicyResponse(const GetGlobalPolicyResponse& src) :
                PlayFabResultCommon(),
                Permissions(src.Permissions)
            {}

            ~GetGlobalPolicyResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Permissions"], Permissions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Permissions; ToJsonUtilO(Permissions, each_Permissions); output["Permissions"] = each_Permissions;
                return output;
            }
        };

        struct GetTitlePlayersFromMasterPlayerAccountIdsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::list<std::string> MasterPlayerAccountIds;
            std::string TitleId;

            GetTitlePlayersFromMasterPlayerAccountIdsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                MasterPlayerAccountIds(),
                TitleId()
            {}

            GetTitlePlayersFromMasterPlayerAccountIdsRequest(const GetTitlePlayersFromMasterPlayerAccountIdsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                MasterPlayerAccountIds(src.MasterPlayerAccountIds),
                TitleId(src.TitleId)
            {}

            ~GetTitlePlayersFromMasterPlayerAccountIdsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["MasterPlayerAccountIds"], MasterPlayerAccountIds);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MasterPlayerAccountIds; ToJsonUtilS(MasterPlayerAccountIds, each_MasterPlayerAccountIds); output["MasterPlayerAccountIds"] = each_MasterPlayerAccountIds;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct GetTitlePlayersFromMasterPlayerAccountIdsResponse : public PlayFabResultCommon
        {
            std::string TitleId;
            std::map<std::string, EntityKey> TitlePlayerAccounts;

            GetTitlePlayersFromMasterPlayerAccountIdsResponse() :
                PlayFabResultCommon(),
                TitleId(),
                TitlePlayerAccounts()
            {}

            GetTitlePlayersFromMasterPlayerAccountIdsResponse(const GetTitlePlayersFromMasterPlayerAccountIdsResponse& src) :
                PlayFabResultCommon(),
                TitleId(src.TitleId),
                TitlePlayerAccounts(src.TitlePlayerAccounts)
            {}

            ~GetTitlePlayersFromMasterPlayerAccountIdsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilO(input["TitlePlayerAccounts"], TitlePlayerAccounts);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_TitlePlayerAccounts; ToJsonUtilO(TitlePlayerAccounts, each_TitlePlayerAccounts); output["TitlePlayerAccounts"] = each_TitlePlayerAccounts;
                return output;
            }
        };

        struct SetEntityProfilePolicyRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::list<EntityPermissionStatement> Statements;

            SetEntityProfilePolicyRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                Statements()
            {}

            SetEntityProfilePolicyRequest(const SetEntityProfilePolicyRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                Statements(src.Statements)
            {}

            ~SetEntityProfilePolicyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["Statements"], Statements);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_Statements; ToJsonUtilO(Statements, each_Statements); output["Statements"] = each_Statements;
                return output;
            }
        };

        struct SetEntityProfilePolicyResponse : public PlayFabResultCommon
        {
            std::list<EntityPermissionStatement> Permissions;

            SetEntityProfilePolicyResponse() :
                PlayFabResultCommon(),
                Permissions()
            {}

            SetEntityProfilePolicyResponse(const SetEntityProfilePolicyResponse& src) :
                PlayFabResultCommon(),
                Permissions(src.Permissions)
            {}

            ~SetEntityProfilePolicyResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Permissions"], Permissions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Permissions; ToJsonUtilO(Permissions, each_Permissions); output["Permissions"] = each_Permissions;
                return output;
            }
        };

        struct SetGlobalPolicyRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::list<EntityPermissionStatement> Permissions;

            SetGlobalPolicyRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Permissions()
            {}

            SetGlobalPolicyRequest(const SetGlobalPolicyRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Permissions(src.Permissions)
            {}

            ~SetGlobalPolicyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Permissions"], Permissions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Permissions; ToJsonUtilO(Permissions, each_Permissions); output["Permissions"] = each_Permissions;
                return output;
            }
        };

        struct SetGlobalPolicyResponse : public PlayFabResultCommon
        {

            SetGlobalPolicyResponse() :
                PlayFabResultCommon()
            {}

            SetGlobalPolicyResponse(const SetGlobalPolicyResponse&) :
                PlayFabResultCommon()
            {}

            ~SetGlobalPolicyResponse() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetProfileLanguageRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            Boxed<Int32> ExpectedVersion;
            std::string Language;

            SetProfileLanguageRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                ExpectedVersion(),
                Language()
            {}

            SetProfileLanguageRequest(const SetProfileLanguageRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                ExpectedVersion(src.ExpectedVersion),
                Language(src.Language)
            {}

            ~SetProfileLanguageRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["ExpectedVersion"], ExpectedVersion);
                FromJsonUtilS(input["Language"], Language);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_ExpectedVersion; ToJsonUtilP(ExpectedVersion, each_ExpectedVersion); output["ExpectedVersion"] = each_ExpectedVersion;
                Json::Value each_Language; ToJsonUtilS(Language, each_Language); output["Language"] = each_Language;
                return output;
            }
        };

        struct SetProfileLanguageResponse : public PlayFabResultCommon
        {
            Boxed<OperationTypes> OperationResult;
            Boxed<Int32> VersionNumber;

            SetProfileLanguageResponse() :
                PlayFabResultCommon(),
                OperationResult(),
                VersionNumber()
            {}

            SetProfileLanguageResponse(const SetProfileLanguageResponse& src) :
                PlayFabResultCommon(),
                OperationResult(src.OperationResult),
                VersionNumber(src.VersionNumber)
            {}

            ~SetProfileLanguageResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["OperationResult"], OperationResult);
                FromJsonUtilP(input["VersionNumber"], VersionNumber);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OperationResult; ToJsonUtilE(OperationResult, each_OperationResult); output["OperationResult"] = each_OperationResult;
                Json::Value each_VersionNumber; ToJsonUtilP(VersionNumber, each_VersionNumber); output["VersionNumber"] = each_VersionNumber;
                return output;
            }
        };

    }
}

#endif
