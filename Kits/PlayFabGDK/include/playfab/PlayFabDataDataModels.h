#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace DataModels
    {
        // Data Enums
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

        // Data Classes
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

        struct AbortFileUploadsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::list<std::string> FileNames;
            Boxed<Int32> ProfileVersion;

            AbortFileUploadsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FileNames(),
                ProfileVersion()
            {}

            AbortFileUploadsRequest(const AbortFileUploadsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FileNames(src.FileNames),
                ProfileVersion(src.ProfileVersion)
            {}

            ~AbortFileUploadsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FileNames"], FileNames);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FileNames; ToJsonUtilS(FileNames, each_FileNames); output["FileNames"] = each_FileNames;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct AbortFileUploadsResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            Int32 ProfileVersion;

            AbortFileUploadsResponse() :
                PlayFabResultCommon(),
                Entity(),
                ProfileVersion()
            {}

            AbortFileUploadsResponse(const AbortFileUploadsResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                ProfileVersion(src.ProfileVersion)
            {}

            ~AbortFileUploadsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct DeleteFilesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::list<std::string> FileNames;
            Boxed<Int32> ProfileVersion;

            DeleteFilesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FileNames(),
                ProfileVersion()
            {}

            DeleteFilesRequest(const DeleteFilesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FileNames(src.FileNames),
                ProfileVersion(src.ProfileVersion)
            {}

            ~DeleteFilesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FileNames"], FileNames);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FileNames; ToJsonUtilS(FileNames, each_FileNames); output["FileNames"] = each_FileNames;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct DeleteFilesResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            Int32 ProfileVersion;

            DeleteFilesResponse() :
                PlayFabResultCommon(),
                Entity(),
                ProfileVersion()
            {}

            DeleteFilesResponse(const DeleteFilesResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                ProfileVersion(src.ProfileVersion)
            {}

            ~DeleteFilesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct FinalizeFileUploadsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::list<std::string> FileNames;
            Int32 ProfileVersion;

            FinalizeFileUploadsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FileNames(),
                ProfileVersion()
            {}

            FinalizeFileUploadsRequest(const FinalizeFileUploadsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FileNames(src.FileNames),
                ProfileVersion(src.ProfileVersion)
            {}

            ~FinalizeFileUploadsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FileNames"], FileNames);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FileNames; ToJsonUtilS(FileNames, each_FileNames); output["FileNames"] = each_FileNames;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct GetFileMetadata : public PlayFabBaseModel
        {
            std::string Checksum;
            std::string DownloadUrl;
            std::string FileName;
            time_t LastModified;
            Int32 Size;

            GetFileMetadata() :
                PlayFabBaseModel(),
                Checksum(),
                DownloadUrl(),
                FileName(),
                LastModified(),
                Size()
            {}

            GetFileMetadata(const GetFileMetadata& src) :
                PlayFabBaseModel(),
                Checksum(src.Checksum),
                DownloadUrl(src.DownloadUrl),
                FileName(src.FileName),
                LastModified(src.LastModified),
                Size(src.Size)
            {}

            ~GetFileMetadata() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Checksum"], Checksum);
                FromJsonUtilS(input["DownloadUrl"], DownloadUrl);
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilT(input["LastModified"], LastModified);
                FromJsonUtilP(input["Size"], Size);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Checksum; ToJsonUtilS(Checksum, each_Checksum); output["Checksum"] = each_Checksum;
                Json::Value each_DownloadUrl; ToJsonUtilS(DownloadUrl, each_DownloadUrl); output["DownloadUrl"] = each_DownloadUrl;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_LastModified; ToJsonUtilT(LastModified, each_LastModified); output["LastModified"] = each_LastModified;
                Json::Value each_Size; ToJsonUtilP(Size, each_Size); output["Size"] = each_Size;
                return output;
            }
        };

        struct FinalizeFileUploadsResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            std::map<std::string, GetFileMetadata> Metadata;
            Int32 ProfileVersion;

            FinalizeFileUploadsResponse() :
                PlayFabResultCommon(),
                Entity(),
                Metadata(),
                ProfileVersion()
            {}

            FinalizeFileUploadsResponse(const FinalizeFileUploadsResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                Metadata(src.Metadata),
                ProfileVersion(src.ProfileVersion)
            {}

            ~FinalizeFileUploadsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["Metadata"], Metadata);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_Metadata; ToJsonUtilO(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct GetFilesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;

            GetFilesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity()
            {}

            GetFilesRequest(const GetFilesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity)
            {}

            ~GetFilesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                return output;
            }
        };

        struct GetFilesResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            std::map<std::string, GetFileMetadata> Metadata;
            Int32 ProfileVersion;

            GetFilesResponse() :
                PlayFabResultCommon(),
                Entity(),
                Metadata(),
                ProfileVersion()
            {}

            GetFilesResponse(const GetFilesResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                Metadata(src.Metadata),
                ProfileVersion(src.ProfileVersion)
            {}

            ~GetFilesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["Metadata"], Metadata);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_Metadata; ToJsonUtilO(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct GetObjectsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            Boxed<bool> EscapeObject;

            GetObjectsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                EscapeObject()
            {}

            GetObjectsRequest(const GetObjectsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                EscapeObject(src.EscapeObject)
            {}

            ~GetObjectsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["EscapeObject"], EscapeObject);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_EscapeObject; ToJsonUtilP(EscapeObject, each_EscapeObject); output["EscapeObject"] = each_EscapeObject;
                return output;
            }
        };

        struct ObjectResult : public PlayFabResultCommon
        {
            Json::Value DataObject;
            std::string EscapedDataObject;
            std::string ObjectName;

            ObjectResult() :
                PlayFabResultCommon(),
                DataObject(),
                EscapedDataObject(),
                ObjectName()
            {}

            ObjectResult(const ObjectResult& src) :
                PlayFabResultCommon(),
                DataObject(src.DataObject),
                EscapedDataObject(src.EscapedDataObject),
                ObjectName(src.ObjectName)
            {}

            ~ObjectResult() = default;

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

        struct GetObjectsResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            std::map<std::string, ObjectResult> Objects;
            Int32 ProfileVersion;

            GetObjectsResponse() :
                PlayFabResultCommon(),
                Entity(),
                Objects(),
                ProfileVersion()
            {}

            GetObjectsResponse(const GetObjectsResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                Objects(src.Objects),
                ProfileVersion(src.ProfileVersion)
            {}

            ~GetObjectsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["Objects"], Objects);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_Objects; ToJsonUtilO(Objects, each_Objects); output["Objects"] = each_Objects;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct InitiateFileUploadMetadata : public PlayFabBaseModel
        {
            std::string FileName;
            std::string UploadUrl;

            InitiateFileUploadMetadata() :
                PlayFabBaseModel(),
                FileName(),
                UploadUrl()
            {}

            InitiateFileUploadMetadata(const InitiateFileUploadMetadata& src) :
                PlayFabBaseModel(),
                FileName(src.FileName),
                UploadUrl(src.UploadUrl)
            {}

            ~InitiateFileUploadMetadata() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilS(input["UploadUrl"], UploadUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_UploadUrl; ToJsonUtilS(UploadUrl, each_UploadUrl); output["UploadUrl"] = each_UploadUrl;
                return output;
            }
        };

        struct InitiateFileUploadsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::list<std::string> FileNames;
            Boxed<Int32> ProfileVersion;

            InitiateFileUploadsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FileNames(),
                ProfileVersion()
            {}

            InitiateFileUploadsRequest(const InitiateFileUploadsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FileNames(src.FileNames),
                ProfileVersion(src.ProfileVersion)
            {}

            ~InitiateFileUploadsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FileNames"], FileNames);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FileNames; ToJsonUtilS(FileNames, each_FileNames); output["FileNames"] = each_FileNames;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                return output;
            }
        };

        struct InitiateFileUploadsResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            Int32 ProfileVersion;
            std::list<InitiateFileUploadMetadata> UploadDetails;

            InitiateFileUploadsResponse() :
                PlayFabResultCommon(),
                Entity(),
                ProfileVersion(),
                UploadDetails()
            {}

            InitiateFileUploadsResponse(const InitiateFileUploadsResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                ProfileVersion(src.ProfileVersion),
                UploadDetails(src.UploadDetails)
            {}

            ~InitiateFileUploadsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
                FromJsonUtilO(input["UploadDetails"], UploadDetails);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                Json::Value each_UploadDetails; ToJsonUtilO(UploadDetails, each_UploadDetails); output["UploadDetails"] = each_UploadDetails;
                return output;
            }
        };

        struct SetObject : public PlayFabBaseModel
        {
            Json::Value DataObject;
            Boxed<bool> DeleteObject;
            std::string EscapedDataObject;
            std::string ObjectName;

            SetObject() :
                PlayFabBaseModel(),
                DataObject(),
                DeleteObject(),
                EscapedDataObject(),
                ObjectName()
            {}

            SetObject(const SetObject& src) :
                PlayFabBaseModel(),
                DataObject(src.DataObject),
                DeleteObject(src.DeleteObject),
                EscapedDataObject(src.EscapedDataObject),
                ObjectName(src.ObjectName)
            {}

            ~SetObject() = default;

            void FromJson(const Json::Value& input) override
            {
                DataObject = input["DataObject"];
                FromJsonUtilP(input["DeleteObject"], DeleteObject);
                FromJsonUtilS(input["EscapedDataObject"], EscapedDataObject);
                FromJsonUtilS(input["ObjectName"], ObjectName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["DataObject"] = DataObject;
                Json::Value each_DeleteObject; ToJsonUtilP(DeleteObject, each_DeleteObject); output["DeleteObject"] = each_DeleteObject;
                Json::Value each_EscapedDataObject; ToJsonUtilS(EscapedDataObject, each_EscapedDataObject); output["EscapedDataObject"] = each_EscapedDataObject;
                Json::Value each_ObjectName; ToJsonUtilS(ObjectName, each_ObjectName); output["ObjectName"] = each_ObjectName;
                return output;
            }
        };

        struct SetObjectInfo : public PlayFabBaseModel
        {
            std::string ObjectName;
            std::string OperationReason;
            Boxed<OperationTypes> SetResult;

            SetObjectInfo() :
                PlayFabBaseModel(),
                ObjectName(),
                OperationReason(),
                SetResult()
            {}

            SetObjectInfo(const SetObjectInfo& src) :
                PlayFabBaseModel(),
                ObjectName(src.ObjectName),
                OperationReason(src.OperationReason),
                SetResult(src.SetResult)
            {}

            ~SetObjectInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ObjectName"], ObjectName);
                FromJsonUtilS(input["OperationReason"], OperationReason);
                FromJsonUtilE(input["SetResult"], SetResult);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ObjectName; ToJsonUtilS(ObjectName, each_ObjectName); output["ObjectName"] = each_ObjectName;
                Json::Value each_OperationReason; ToJsonUtilS(OperationReason, each_OperationReason); output["OperationReason"] = each_OperationReason;
                Json::Value each_SetResult; ToJsonUtilE(SetResult, each_SetResult); output["SetResult"] = each_SetResult;
                return output;
            }
        };

        struct SetObjectsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            Boxed<Int32> ExpectedProfileVersion;
            std::list<SetObject> Objects;

            SetObjectsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                ExpectedProfileVersion(),
                Objects()
            {}

            SetObjectsRequest(const SetObjectsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                ExpectedProfileVersion(src.ExpectedProfileVersion),
                Objects(src.Objects)
            {}

            ~SetObjectsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilP(input["ExpectedProfileVersion"], ExpectedProfileVersion);
                FromJsonUtilO(input["Objects"], Objects);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_ExpectedProfileVersion; ToJsonUtilP(ExpectedProfileVersion, each_ExpectedProfileVersion); output["ExpectedProfileVersion"] = each_ExpectedProfileVersion;
                Json::Value each_Objects; ToJsonUtilO(Objects, each_Objects); output["Objects"] = each_Objects;
                return output;
            }
        };

        struct SetObjectsResponse : public PlayFabResultCommon
        {
            Int32 ProfileVersion;
            std::list<SetObjectInfo> SetResults;

            SetObjectsResponse() :
                PlayFabResultCommon(),
                ProfileVersion(),
                SetResults()
            {}

            SetObjectsResponse(const SetObjectsResponse& src) :
                PlayFabResultCommon(),
                ProfileVersion(src.ProfileVersion),
                SetResults(src.SetResults)
            {}

            ~SetObjectsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["ProfileVersion"], ProfileVersion);
                FromJsonUtilO(input["SetResults"], SetResults);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ProfileVersion; ToJsonUtilP(ProfileVersion, each_ProfileVersion); output["ProfileVersion"] = each_ProfileVersion;
                Json::Value each_SetResults; ToJsonUtilO(SetResults, each_SetResults); output["SetResults"] = each_SetResults;
                return output;
            }
        };

    }
}

#endif
