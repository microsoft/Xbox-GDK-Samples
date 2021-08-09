#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace InsightsModels
    {
        // Insights Enums
        // Insights Classes
        struct InsightsEmptyRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            InsightsEmptyRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            InsightsEmptyRequest(const InsightsEmptyRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~InsightsEmptyRequest() = default;

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

        struct InsightsPerformanceLevel : public PlayFabBaseModel
        {
            Int32 ActiveEventExports;
            Int32 CacheSizeMB;
            Int32 Concurrency;
            double CreditsPerMinute;
            Int32 EventsPerSecond;
            Int32 Level;
            Int32 MaxMemoryPerQueryMB;
            Int32 VirtualCpuCores;

            InsightsPerformanceLevel() :
                PlayFabBaseModel(),
                ActiveEventExports(),
                CacheSizeMB(),
                Concurrency(),
                CreditsPerMinute(),
                EventsPerSecond(),
                Level(),
                MaxMemoryPerQueryMB(),
                VirtualCpuCores()
            {}

            InsightsPerformanceLevel(const InsightsPerformanceLevel& src) :
                PlayFabBaseModel(),
                ActiveEventExports(src.ActiveEventExports),
                CacheSizeMB(src.CacheSizeMB),
                Concurrency(src.Concurrency),
                CreditsPerMinute(src.CreditsPerMinute),
                EventsPerSecond(src.EventsPerSecond),
                Level(src.Level),
                MaxMemoryPerQueryMB(src.MaxMemoryPerQueryMB),
                VirtualCpuCores(src.VirtualCpuCores)
            {}

            ~InsightsPerformanceLevel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["ActiveEventExports"], ActiveEventExports);
                FromJsonUtilP(input["CacheSizeMB"], CacheSizeMB);
                FromJsonUtilP(input["Concurrency"], Concurrency);
                FromJsonUtilP(input["CreditsPerMinute"], CreditsPerMinute);
                FromJsonUtilP(input["EventsPerSecond"], EventsPerSecond);
                FromJsonUtilP(input["Level"], Level);
                FromJsonUtilP(input["MaxMemoryPerQueryMB"], MaxMemoryPerQueryMB);
                FromJsonUtilP(input["VirtualCpuCores"], VirtualCpuCores);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveEventExports; ToJsonUtilP(ActiveEventExports, each_ActiveEventExports); output["ActiveEventExports"] = each_ActiveEventExports;
                Json::Value each_CacheSizeMB; ToJsonUtilP(CacheSizeMB, each_CacheSizeMB); output["CacheSizeMB"] = each_CacheSizeMB;
                Json::Value each_Concurrency; ToJsonUtilP(Concurrency, each_Concurrency); output["Concurrency"] = each_Concurrency;
                Json::Value each_CreditsPerMinute; ToJsonUtilP(CreditsPerMinute, each_CreditsPerMinute); output["CreditsPerMinute"] = each_CreditsPerMinute;
                Json::Value each_EventsPerSecond; ToJsonUtilP(EventsPerSecond, each_EventsPerSecond); output["EventsPerSecond"] = each_EventsPerSecond;
                Json::Value each_Level; ToJsonUtilP(Level, each_Level); output["Level"] = each_Level;
                Json::Value each_MaxMemoryPerQueryMB; ToJsonUtilP(MaxMemoryPerQueryMB, each_MaxMemoryPerQueryMB); output["MaxMemoryPerQueryMB"] = each_MaxMemoryPerQueryMB;
                Json::Value each_VirtualCpuCores; ToJsonUtilP(VirtualCpuCores, each_VirtualCpuCores); output["VirtualCpuCores"] = each_VirtualCpuCores;
                return output;
            }
        };

        struct InsightsGetLimitsResponse : public PlayFabResultCommon
        {
            Int32 DefaultPerformanceLevel;
            Int32 DefaultStorageRetentionDays;
            Int32 StorageMaxRetentionDays;
            Int32 StorageMinRetentionDays;
            std::list<InsightsPerformanceLevel> SubMeters;

            InsightsGetLimitsResponse() :
                PlayFabResultCommon(),
                DefaultPerformanceLevel(),
                DefaultStorageRetentionDays(),
                StorageMaxRetentionDays(),
                StorageMinRetentionDays(),
                SubMeters()
            {}

            InsightsGetLimitsResponse(const InsightsGetLimitsResponse& src) :
                PlayFabResultCommon(),
                DefaultPerformanceLevel(src.DefaultPerformanceLevel),
                DefaultStorageRetentionDays(src.DefaultStorageRetentionDays),
                StorageMaxRetentionDays(src.StorageMaxRetentionDays),
                StorageMinRetentionDays(src.StorageMinRetentionDays),
                SubMeters(src.SubMeters)
            {}

            ~InsightsGetLimitsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["DefaultPerformanceLevel"], DefaultPerformanceLevel);
                FromJsonUtilP(input["DefaultStorageRetentionDays"], DefaultStorageRetentionDays);
                FromJsonUtilP(input["StorageMaxRetentionDays"], StorageMaxRetentionDays);
                FromJsonUtilP(input["StorageMinRetentionDays"], StorageMinRetentionDays);
                FromJsonUtilO(input["SubMeters"], SubMeters);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DefaultPerformanceLevel; ToJsonUtilP(DefaultPerformanceLevel, each_DefaultPerformanceLevel); output["DefaultPerformanceLevel"] = each_DefaultPerformanceLevel;
                Json::Value each_DefaultStorageRetentionDays; ToJsonUtilP(DefaultStorageRetentionDays, each_DefaultStorageRetentionDays); output["DefaultStorageRetentionDays"] = each_DefaultStorageRetentionDays;
                Json::Value each_StorageMaxRetentionDays; ToJsonUtilP(StorageMaxRetentionDays, each_StorageMaxRetentionDays); output["StorageMaxRetentionDays"] = each_StorageMaxRetentionDays;
                Json::Value each_StorageMinRetentionDays; ToJsonUtilP(StorageMinRetentionDays, each_StorageMinRetentionDays); output["StorageMinRetentionDays"] = each_StorageMinRetentionDays;
                Json::Value each_SubMeters; ToJsonUtilO(SubMeters, each_SubMeters); output["SubMeters"] = each_SubMeters;
                return output;
            }
        };

        struct InsightsGetOperationStatusResponse : public PlayFabResultCommon
        {
            std::string Message;
            time_t OperationCompletedTime;
            std::string OperationId;
            time_t OperationLastUpdated;
            time_t OperationStartedTime;
            std::string OperationType;
            Int32 OperationValue;
            std::string Status;

            InsightsGetOperationStatusResponse() :
                PlayFabResultCommon(),
                Message(),
                OperationCompletedTime(),
                OperationId(),
                OperationLastUpdated(),
                OperationStartedTime(),
                OperationType(),
                OperationValue(),
                Status()
            {}

            InsightsGetOperationStatusResponse(const InsightsGetOperationStatusResponse& src) :
                PlayFabResultCommon(),
                Message(src.Message),
                OperationCompletedTime(src.OperationCompletedTime),
                OperationId(src.OperationId),
                OperationLastUpdated(src.OperationLastUpdated),
                OperationStartedTime(src.OperationStartedTime),
                OperationType(src.OperationType),
                OperationValue(src.OperationValue),
                Status(src.Status)
            {}

            ~InsightsGetOperationStatusResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Message"], Message);
                FromJsonUtilT(input["OperationCompletedTime"], OperationCompletedTime);
                FromJsonUtilS(input["OperationId"], OperationId);
                FromJsonUtilT(input["OperationLastUpdated"], OperationLastUpdated);
                FromJsonUtilT(input["OperationStartedTime"], OperationStartedTime);
                FromJsonUtilS(input["OperationType"], OperationType);
                FromJsonUtilP(input["OperationValue"], OperationValue);
                FromJsonUtilS(input["Status"], Status);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Message; ToJsonUtilS(Message, each_Message); output["Message"] = each_Message;
                Json::Value each_OperationCompletedTime; ToJsonUtilT(OperationCompletedTime, each_OperationCompletedTime); output["OperationCompletedTime"] = each_OperationCompletedTime;
                Json::Value each_OperationId; ToJsonUtilS(OperationId, each_OperationId); output["OperationId"] = each_OperationId;
                Json::Value each_OperationLastUpdated; ToJsonUtilT(OperationLastUpdated, each_OperationLastUpdated); output["OperationLastUpdated"] = each_OperationLastUpdated;
                Json::Value each_OperationStartedTime; ToJsonUtilT(OperationStartedTime, each_OperationStartedTime); output["OperationStartedTime"] = each_OperationStartedTime;
                Json::Value each_OperationType; ToJsonUtilS(OperationType, each_OperationType); output["OperationType"] = each_OperationType;
                Json::Value each_OperationValue; ToJsonUtilP(OperationValue, each_OperationValue); output["OperationValue"] = each_OperationValue;
                Json::Value each_Status; ToJsonUtilS(Status, each_Status); output["Status"] = each_Status;
                return output;
            }
        };

        struct InsightsGetDetailsResponse : public PlayFabResultCommon
        {
            Uint32 DataUsageMb;
            std::string ErrorMessage;
            Boxed<InsightsGetLimitsResponse> Limits;
            std::list<InsightsGetOperationStatusResponse> PendingOperations;
            Int32 PerformanceLevel;
            Int32 RetentionDays;

            InsightsGetDetailsResponse() :
                PlayFabResultCommon(),
                DataUsageMb(),
                ErrorMessage(),
                Limits(),
                PendingOperations(),
                PerformanceLevel(),
                RetentionDays()
            {}

            InsightsGetDetailsResponse(const InsightsGetDetailsResponse& src) :
                PlayFabResultCommon(),
                DataUsageMb(src.DataUsageMb),
                ErrorMessage(src.ErrorMessage),
                Limits(src.Limits),
                PendingOperations(src.PendingOperations),
                PerformanceLevel(src.PerformanceLevel),
                RetentionDays(src.RetentionDays)
            {}

            ~InsightsGetDetailsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["DataUsageMb"], DataUsageMb);
                FromJsonUtilS(input["ErrorMessage"], ErrorMessage);
                FromJsonUtilO(input["Limits"], Limits);
                FromJsonUtilO(input["PendingOperations"], PendingOperations);
                FromJsonUtilP(input["PerformanceLevel"], PerformanceLevel);
                FromJsonUtilP(input["RetentionDays"], RetentionDays);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DataUsageMb; ToJsonUtilP(DataUsageMb, each_DataUsageMb); output["DataUsageMb"] = each_DataUsageMb;
                Json::Value each_ErrorMessage; ToJsonUtilS(ErrorMessage, each_ErrorMessage); output["ErrorMessage"] = each_ErrorMessage;
                Json::Value each_Limits; ToJsonUtilO(Limits, each_Limits); output["Limits"] = each_Limits;
                Json::Value each_PendingOperations; ToJsonUtilO(PendingOperations, each_PendingOperations); output["PendingOperations"] = each_PendingOperations;
                Json::Value each_PerformanceLevel; ToJsonUtilP(PerformanceLevel, each_PerformanceLevel); output["PerformanceLevel"] = each_PerformanceLevel;
                Json::Value each_RetentionDays; ToJsonUtilP(RetentionDays, each_RetentionDays); output["RetentionDays"] = each_RetentionDays;
                return output;
            }
        };

        struct InsightsGetOperationStatusRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string OperationId;

            InsightsGetOperationStatusRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                OperationId()
            {}

            InsightsGetOperationStatusRequest(const InsightsGetOperationStatusRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                OperationId(src.OperationId)
            {}

            ~InsightsGetOperationStatusRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["OperationId"], OperationId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_OperationId; ToJsonUtilS(OperationId, each_OperationId); output["OperationId"] = each_OperationId;
                return output;
            }
        };

        struct InsightsGetPendingOperationsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string OperationType;

            InsightsGetPendingOperationsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                OperationType()
            {}

            InsightsGetPendingOperationsRequest(const InsightsGetPendingOperationsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                OperationType(src.OperationType)
            {}

            ~InsightsGetPendingOperationsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["OperationType"], OperationType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_OperationType; ToJsonUtilS(OperationType, each_OperationType); output["OperationType"] = each_OperationType;
                return output;
            }
        };

        struct InsightsGetPendingOperationsResponse : public PlayFabResultCommon
        {
            std::list<InsightsGetOperationStatusResponse> PendingOperations;

            InsightsGetPendingOperationsResponse() :
                PlayFabResultCommon(),
                PendingOperations()
            {}

            InsightsGetPendingOperationsResponse(const InsightsGetPendingOperationsResponse& src) :
                PlayFabResultCommon(),
                PendingOperations(src.PendingOperations)
            {}

            ~InsightsGetPendingOperationsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["PendingOperations"], PendingOperations);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PendingOperations; ToJsonUtilO(PendingOperations, each_PendingOperations); output["PendingOperations"] = each_PendingOperations;
                return output;
            }
        };

        struct InsightsOperationResponse : public PlayFabResultCommon
        {
            std::string Message;
            std::string OperationId;
            std::string OperationType;

            InsightsOperationResponse() :
                PlayFabResultCommon(),
                Message(),
                OperationId(),
                OperationType()
            {}

            InsightsOperationResponse(const InsightsOperationResponse& src) :
                PlayFabResultCommon(),
                Message(src.Message),
                OperationId(src.OperationId),
                OperationType(src.OperationType)
            {}

            ~InsightsOperationResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Message"], Message);
                FromJsonUtilS(input["OperationId"], OperationId);
                FromJsonUtilS(input["OperationType"], OperationType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Message; ToJsonUtilS(Message, each_Message); output["Message"] = each_Message;
                Json::Value each_OperationId; ToJsonUtilS(OperationId, each_OperationId); output["OperationId"] = each_OperationId;
                Json::Value each_OperationType; ToJsonUtilS(OperationType, each_OperationType); output["OperationType"] = each_OperationType;
                return output;
            }
        };

        struct InsightsSetPerformanceRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 PerformanceLevel;

            InsightsSetPerformanceRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PerformanceLevel()
            {}

            InsightsSetPerformanceRequest(const InsightsSetPerformanceRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PerformanceLevel(src.PerformanceLevel)
            {}

            ~InsightsSetPerformanceRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PerformanceLevel"], PerformanceLevel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PerformanceLevel; ToJsonUtilP(PerformanceLevel, each_PerformanceLevel); output["PerformanceLevel"] = each_PerformanceLevel;
                return output;
            }
        };

        struct InsightsSetStorageRetentionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 RetentionDays;

            InsightsSetStorageRetentionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                RetentionDays()
            {}

            InsightsSetStorageRetentionRequest(const InsightsSetStorageRetentionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                RetentionDays(src.RetentionDays)
            {}

            ~InsightsSetStorageRetentionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["RetentionDays"], RetentionDays);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_RetentionDays; ToJsonUtilP(RetentionDays, each_RetentionDays); output["RetentionDays"] = each_RetentionDays;
                return output;
            }
        };

    }
}

#endif
