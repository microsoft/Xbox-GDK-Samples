#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace ExperimentationModels
    {
        // Experimentation Enums
        enum class AnalysisTaskState
        {
            AnalysisTaskStateWaiting,
            AnalysisTaskStateReadyForSubmission,
            AnalysisTaskStateSubmittingToPipeline,
            AnalysisTaskStateRunning,
            AnalysisTaskStateCompleted,
            AnalysisTaskStateFailed,
            AnalysisTaskStateCanceled
        };

        inline void ToJsonEnum(const AnalysisTaskState input, Json::Value& output)
        {
            if (input == AnalysisTaskState::AnalysisTaskStateWaiting)
            {
                output = Json::Value("Waiting");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateReadyForSubmission)
            {
                output = Json::Value("ReadyForSubmission");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateSubmittingToPipeline)
            {
                output = Json::Value("SubmittingToPipeline");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateRunning)
            {
                output = Json::Value("Running");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateCompleted)
            {
                output = Json::Value("Completed");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateFailed)
            {
                output = Json::Value("Failed");
                return;
            }
            if (input == AnalysisTaskState::AnalysisTaskStateCanceled)
            {
                output = Json::Value("Canceled");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AnalysisTaskState& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Waiting")
            {
                output = AnalysisTaskState::AnalysisTaskStateWaiting;
                return;
            }
            if (inputStr == "ReadyForSubmission")
            {
                output = AnalysisTaskState::AnalysisTaskStateReadyForSubmission;
                return;
            }
            if (inputStr == "SubmittingToPipeline")
            {
                output = AnalysisTaskState::AnalysisTaskStateSubmittingToPipeline;
                return;
            }
            if (inputStr == "Running")
            {
                output = AnalysisTaskState::AnalysisTaskStateRunning;
                return;
            }
            if (inputStr == "Completed")
            {
                output = AnalysisTaskState::AnalysisTaskStateCompleted;
                return;
            }
            if (inputStr == "Failed")
            {
                output = AnalysisTaskState::AnalysisTaskStateFailed;
                return;
            }
            if (inputStr == "Canceled")
            {
                output = AnalysisTaskState::AnalysisTaskStateCanceled;
                return;
            }
        }

        enum class ExperimentState
        {
            ExperimentStateNew,
            ExperimentStateStarted,
            ExperimentStateStopped,
            ExperimentStateDeleted
        };

        inline void ToJsonEnum(const ExperimentState input, Json::Value& output)
        {
            if (input == ExperimentState::ExperimentStateNew)
            {
                output = Json::Value("New");
                return;
            }
            if (input == ExperimentState::ExperimentStateStarted)
            {
                output = Json::Value("Started");
                return;
            }
            if (input == ExperimentState::ExperimentStateStopped)
            {
                output = Json::Value("Stopped");
                return;
            }
            if (input == ExperimentState::ExperimentStateDeleted)
            {
                output = Json::Value("Deleted");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ExperimentState& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "New")
            {
                output = ExperimentState::ExperimentStateNew;
                return;
            }
            if (inputStr == "Started")
            {
                output = ExperimentState::ExperimentStateStarted;
                return;
            }
            if (inputStr == "Stopped")
            {
                output = ExperimentState::ExperimentStateStopped;
                return;
            }
            if (inputStr == "Deleted")
            {
                output = ExperimentState::ExperimentStateDeleted;
                return;
            }
        }

        enum class ExperimentType
        {
            ExperimentTypeActive,
            ExperimentTypeSnapshot
        };

        inline void ToJsonEnum(const ExperimentType input, Json::Value& output)
        {
            if (input == ExperimentType::ExperimentTypeActive)
            {
                output = Json::Value("Active");
                return;
            }
            if (input == ExperimentType::ExperimentTypeSnapshot)
            {
                output = Json::Value("Snapshot");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ExperimentType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Active")
            {
                output = ExperimentType::ExperimentTypeActive;
                return;
            }
            if (inputStr == "Snapshot")
            {
                output = ExperimentType::ExperimentTypeSnapshot;
                return;
            }
        }

        // Experimentation Classes
        struct Variable : public PlayFabBaseModel
        {
            std::string Name;
            std::string Value;

            Variable() :
                PlayFabBaseModel(),
                Name(),
                Value()
            {}

            Variable(const Variable& src) :
                PlayFabBaseModel(),
                Name(src.Name),
                Value(src.Value)
            {}

            ~Variable() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct Variant : public PlayFabBaseModel
        {
            std::string Description;
            std::string Id;
            bool IsControl;
            std::string Name;
            std::string TitleDataOverrideLabel;
            Uint32 TrafficPercentage;
            std::list<Variable> Variables;

            Variant() :
                PlayFabBaseModel(),
                Description(),
                Id(),
                IsControl(),
                Name(),
                TitleDataOverrideLabel(),
                TrafficPercentage(),
                Variables()
            {}

            Variant(const Variant& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                Id(src.Id),
                IsControl(src.IsControl),
                Name(src.Name),
                TitleDataOverrideLabel(src.TitleDataOverrideLabel),
                TrafficPercentage(src.TrafficPercentage),
                Variables(src.Variables)
            {}

            ~Variant() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilP(input["IsControl"], IsControl);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["TitleDataOverrideLabel"], TitleDataOverrideLabel);
                FromJsonUtilP(input["TrafficPercentage"], TrafficPercentage);
                FromJsonUtilO(input["Variables"], Variables);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_IsControl; ToJsonUtilP(IsControl, each_IsControl); output["IsControl"] = each_IsControl;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_TitleDataOverrideLabel; ToJsonUtilS(TitleDataOverrideLabel, each_TitleDataOverrideLabel); output["TitleDataOverrideLabel"] = each_TitleDataOverrideLabel;
                Json::Value each_TrafficPercentage; ToJsonUtilP(TrafficPercentage, each_TrafficPercentage); output["TrafficPercentage"] = each_TrafficPercentage;
                Json::Value each_Variables; ToJsonUtilO(Variables, each_Variables); output["Variables"] = each_Variables;
                return output;
            }
        };

        struct CreateExperimentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            Uint32 Duration;
            Boxed<ExperimentType> pfExperimentType;
            std::string Name;
            std::string SegmentId;
            time_t StartDate;
            std::list<std::string> TitlePlayerAccountTestIds;
            std::list<Variant> Variants;

            CreateExperimentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                Duration(),
                pfExperimentType(),
                Name(),
                SegmentId(),
                StartDate(),
                TitlePlayerAccountTestIds(),
                Variants()
            {}

            CreateExperimentRequest(const CreateExperimentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                Duration(src.Duration),
                pfExperimentType(src.pfExperimentType),
                Name(src.Name),
                SegmentId(src.SegmentId),
                StartDate(src.StartDate),
                TitlePlayerAccountTestIds(src.TitlePlayerAccountTestIds),
                Variants(src.Variants)
            {}

            ~CreateExperimentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["Duration"], Duration);
                FromJsonUtilE(input["ExperimentType"], pfExperimentType);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["SegmentId"], SegmentId);
                FromJsonUtilT(input["StartDate"], StartDate);
                FromJsonUtilS(input["TitlePlayerAccountTestIds"], TitlePlayerAccountTestIds);
                FromJsonUtilO(input["Variants"], Variants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_Duration; ToJsonUtilP(Duration, each_Duration); output["Duration"] = each_Duration;
                Json::Value each_pfExperimentType; ToJsonUtilE(pfExperimentType, each_pfExperimentType); output["ExperimentType"] = each_pfExperimentType;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_SegmentId; ToJsonUtilS(SegmentId, each_SegmentId); output["SegmentId"] = each_SegmentId;
                Json::Value each_StartDate; ToJsonUtilT(StartDate, each_StartDate); output["StartDate"] = each_StartDate;
                Json::Value each_TitlePlayerAccountTestIds; ToJsonUtilS(TitlePlayerAccountTestIds, each_TitlePlayerAccountTestIds); output["TitlePlayerAccountTestIds"] = each_TitlePlayerAccountTestIds;
                Json::Value each_Variants; ToJsonUtilO(Variants, each_Variants); output["Variants"] = each_Variants;
                return output;
            }
        };

        struct CreateExperimentResult : public PlayFabResultCommon
        {
            std::string ExperimentId;

            CreateExperimentResult() :
                PlayFabResultCommon(),
                ExperimentId()
            {}

            CreateExperimentResult(const CreateExperimentResult& src) :
                PlayFabResultCommon(),
                ExperimentId(src.ExperimentId)
            {}

            ~CreateExperimentResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                return output;
            }
        };

        struct DeleteExperimentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ExperimentId;

            DeleteExperimentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ExperimentId()
            {}

            DeleteExperimentRequest(const DeleteExperimentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ExperimentId(src.ExperimentId)
            {}

            ~DeleteExperimentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                return output;
            }
        };

        struct EmptyResponse : public PlayFabResultCommon
        {

            EmptyResponse() :
                PlayFabResultCommon()
            {}

            EmptyResponse(const EmptyResponse&) :
                PlayFabResultCommon()
            {}

            ~EmptyResponse() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
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

        struct Experiment : public PlayFabBaseModel
        {
            std::string Description;
            Uint32 Duration;
            Boxed<ExperimentType> pfExperimentType;
            std::string Id;
            std::string Name;
            std::string SegmentId;
            time_t StartDate;
            Boxed<ExperimentState> State;
            std::list<std::string> TitlePlayerAccountTestIds;
            std::list<Variant> Variants;

            Experiment() :
                PlayFabBaseModel(),
                Description(),
                Duration(),
                pfExperimentType(),
                Id(),
                Name(),
                SegmentId(),
                StartDate(),
                State(),
                TitlePlayerAccountTestIds(),
                Variants()
            {}

            Experiment(const Experiment& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                Duration(src.Duration),
                pfExperimentType(src.pfExperimentType),
                Id(src.Id),
                Name(src.Name),
                SegmentId(src.SegmentId),
                StartDate(src.StartDate),
                State(src.State),
                TitlePlayerAccountTestIds(src.TitlePlayerAccountTestIds),
                Variants(src.Variants)
            {}

            ~Experiment() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["Duration"], Duration);
                FromJsonUtilE(input["ExperimentType"], pfExperimentType);
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["SegmentId"], SegmentId);
                FromJsonUtilT(input["StartDate"], StartDate);
                FromJsonUtilE(input["State"], State);
                FromJsonUtilS(input["TitlePlayerAccountTestIds"], TitlePlayerAccountTestIds);
                FromJsonUtilO(input["Variants"], Variants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_Duration; ToJsonUtilP(Duration, each_Duration); output["Duration"] = each_Duration;
                Json::Value each_pfExperimentType; ToJsonUtilE(pfExperimentType, each_pfExperimentType); output["ExperimentType"] = each_pfExperimentType;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_SegmentId; ToJsonUtilS(SegmentId, each_SegmentId); output["SegmentId"] = each_SegmentId;
                Json::Value each_StartDate; ToJsonUtilT(StartDate, each_StartDate); output["StartDate"] = each_StartDate;
                Json::Value each_State; ToJsonUtilE(State, each_State); output["State"] = each_State;
                Json::Value each_TitlePlayerAccountTestIds; ToJsonUtilS(TitlePlayerAccountTestIds, each_TitlePlayerAccountTestIds); output["TitlePlayerAccountTestIds"] = each_TitlePlayerAccountTestIds;
                Json::Value each_Variants; ToJsonUtilO(Variants, each_Variants); output["Variants"] = each_Variants;
                return output;
            }
        };

        struct GetExperimentsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetExperimentsRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetExperimentsRequest(const GetExperimentsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetExperimentsRequest() = default;

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

        struct GetExperimentsResult : public PlayFabResultCommon
        {
            std::list<Experiment> Experiments;

            GetExperimentsResult() :
                PlayFabResultCommon(),
                Experiments()
            {}

            GetExperimentsResult(const GetExperimentsResult& src) :
                PlayFabResultCommon(),
                Experiments(src.Experiments)
            {}

            ~GetExperimentsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Experiments"], Experiments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Experiments; ToJsonUtilO(Experiments, each_Experiments); output["Experiments"] = each_Experiments;
                return output;
            }
        };

        struct GetLatestScorecardRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ExperimentId;

            GetLatestScorecardRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ExperimentId()
            {}

            GetLatestScorecardRequest(const GetLatestScorecardRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ExperimentId(src.ExperimentId)
            {}

            ~GetLatestScorecardRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                return output;
            }
        };

        struct MetricData : public PlayFabBaseModel
        {
            double ConfidenceIntervalEnd;
            double ConfidenceIntervalStart;
            float DeltaAbsoluteChange;
            float DeltaRelativeChange;
            std::string InternalName;
            std::string Movement;
            std::string Name;
            float PMove;
            float PValue;
            float PValueThreshold;
            std::string StatSigLevel;
            float StdDev;
            float Value;

            MetricData() :
                PlayFabBaseModel(),
                ConfidenceIntervalEnd(),
                ConfidenceIntervalStart(),
                DeltaAbsoluteChange(),
                DeltaRelativeChange(),
                InternalName(),
                Movement(),
                Name(),
                PMove(),
                PValue(),
                PValueThreshold(),
                StatSigLevel(),
                StdDev(),
                Value()
            {}

            MetricData(const MetricData& src) :
                PlayFabBaseModel(),
                ConfidenceIntervalEnd(src.ConfidenceIntervalEnd),
                ConfidenceIntervalStart(src.ConfidenceIntervalStart),
                DeltaAbsoluteChange(src.DeltaAbsoluteChange),
                DeltaRelativeChange(src.DeltaRelativeChange),
                InternalName(src.InternalName),
                Movement(src.Movement),
                Name(src.Name),
                PMove(src.PMove),
                PValue(src.PValue),
                PValueThreshold(src.PValueThreshold),
                StatSigLevel(src.StatSigLevel),
                StdDev(src.StdDev),
                Value(src.Value)
            {}

            ~MetricData() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["ConfidenceIntervalEnd"], ConfidenceIntervalEnd);
                FromJsonUtilP(input["ConfidenceIntervalStart"], ConfidenceIntervalStart);
                FromJsonUtilP(input["DeltaAbsoluteChange"], DeltaAbsoluteChange);
                FromJsonUtilP(input["DeltaRelativeChange"], DeltaRelativeChange);
                FromJsonUtilS(input["InternalName"], InternalName);
                FromJsonUtilS(input["Movement"], Movement);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["PMove"], PMove);
                FromJsonUtilP(input["PValue"], PValue);
                FromJsonUtilP(input["PValueThreshold"], PValueThreshold);
                FromJsonUtilS(input["StatSigLevel"], StatSigLevel);
                FromJsonUtilP(input["StdDev"], StdDev);
                FromJsonUtilP(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConfidenceIntervalEnd; ToJsonUtilP(ConfidenceIntervalEnd, each_ConfidenceIntervalEnd); output["ConfidenceIntervalEnd"] = each_ConfidenceIntervalEnd;
                Json::Value each_ConfidenceIntervalStart; ToJsonUtilP(ConfidenceIntervalStart, each_ConfidenceIntervalStart); output["ConfidenceIntervalStart"] = each_ConfidenceIntervalStart;
                Json::Value each_DeltaAbsoluteChange; ToJsonUtilP(DeltaAbsoluteChange, each_DeltaAbsoluteChange); output["DeltaAbsoluteChange"] = each_DeltaAbsoluteChange;
                Json::Value each_DeltaRelativeChange; ToJsonUtilP(DeltaRelativeChange, each_DeltaRelativeChange); output["DeltaRelativeChange"] = each_DeltaRelativeChange;
                Json::Value each_InternalName; ToJsonUtilS(InternalName, each_InternalName); output["InternalName"] = each_InternalName;
                Json::Value each_Movement; ToJsonUtilS(Movement, each_Movement); output["Movement"] = each_Movement;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_PMove; ToJsonUtilP(PMove, each_PMove); output["PMove"] = each_PMove;
                Json::Value each_PValue; ToJsonUtilP(PValue, each_PValue); output["PValue"] = each_PValue;
                Json::Value each_PValueThreshold; ToJsonUtilP(PValueThreshold, each_PValueThreshold); output["PValueThreshold"] = each_PValueThreshold;
                Json::Value each_StatSigLevel; ToJsonUtilS(StatSigLevel, each_StatSigLevel); output["StatSigLevel"] = each_StatSigLevel;
                Json::Value each_StdDev; ToJsonUtilP(StdDev, each_StdDev); output["StdDev"] = each_StdDev;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct ScorecardDataRow : public PlayFabBaseModel
        {
            bool IsControl;
            std::map<std::string, MetricData> MetricDataRows;
            Uint32 PlayerCount;
            std::string VariantName;

            ScorecardDataRow() :
                PlayFabBaseModel(),
                IsControl(),
                MetricDataRows(),
                PlayerCount(),
                VariantName()
            {}

            ScorecardDataRow(const ScorecardDataRow& src) :
                PlayFabBaseModel(),
                IsControl(src.IsControl),
                MetricDataRows(src.MetricDataRows),
                PlayerCount(src.PlayerCount),
                VariantName(src.VariantName)
            {}

            ~ScorecardDataRow() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IsControl"], IsControl);
                FromJsonUtilO(input["MetricDataRows"], MetricDataRows);
                FromJsonUtilP(input["PlayerCount"], PlayerCount);
                FromJsonUtilS(input["VariantName"], VariantName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IsControl; ToJsonUtilP(IsControl, each_IsControl); output["IsControl"] = each_IsControl;
                Json::Value each_MetricDataRows; ToJsonUtilO(MetricDataRows, each_MetricDataRows); output["MetricDataRows"] = each_MetricDataRows;
                Json::Value each_PlayerCount; ToJsonUtilP(PlayerCount, each_PlayerCount); output["PlayerCount"] = each_PlayerCount;
                Json::Value each_VariantName; ToJsonUtilS(VariantName, each_VariantName); output["VariantName"] = each_VariantName;
                return output;
            }
        };

        struct Scorecard : public PlayFabBaseModel
        {
            std::string DateGenerated;
            std::string Duration;
            double EventsProcessed;
            std::string ExperimentId;
            std::string ExperimentName;
            Boxed<AnalysisTaskState> LatestJobStatus;
            bool SampleRatioMismatch;
            std::list<ScorecardDataRow> ScorecardDataRows;

            Scorecard() :
                PlayFabBaseModel(),
                DateGenerated(),
                Duration(),
                EventsProcessed(),
                ExperimentId(),
                ExperimentName(),
                LatestJobStatus(),
                SampleRatioMismatch(),
                ScorecardDataRows()
            {}

            Scorecard(const Scorecard& src) :
                PlayFabBaseModel(),
                DateGenerated(src.DateGenerated),
                Duration(src.Duration),
                EventsProcessed(src.EventsProcessed),
                ExperimentId(src.ExperimentId),
                ExperimentName(src.ExperimentName),
                LatestJobStatus(src.LatestJobStatus),
                SampleRatioMismatch(src.SampleRatioMismatch),
                ScorecardDataRows(src.ScorecardDataRows)
            {}

            ~Scorecard() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DateGenerated"], DateGenerated);
                FromJsonUtilS(input["Duration"], Duration);
                FromJsonUtilP(input["EventsProcessed"], EventsProcessed);
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
                FromJsonUtilS(input["ExperimentName"], ExperimentName);
                FromJsonUtilE(input["LatestJobStatus"], LatestJobStatus);
                FromJsonUtilP(input["SampleRatioMismatch"], SampleRatioMismatch);
                FromJsonUtilO(input["ScorecardDataRows"], ScorecardDataRows);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DateGenerated; ToJsonUtilS(DateGenerated, each_DateGenerated); output["DateGenerated"] = each_DateGenerated;
                Json::Value each_Duration; ToJsonUtilS(Duration, each_Duration); output["Duration"] = each_Duration;
                Json::Value each_EventsProcessed; ToJsonUtilP(EventsProcessed, each_EventsProcessed); output["EventsProcessed"] = each_EventsProcessed;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                Json::Value each_ExperimentName; ToJsonUtilS(ExperimentName, each_ExperimentName); output["ExperimentName"] = each_ExperimentName;
                Json::Value each_LatestJobStatus; ToJsonUtilE(LatestJobStatus, each_LatestJobStatus); output["LatestJobStatus"] = each_LatestJobStatus;
                Json::Value each_SampleRatioMismatch; ToJsonUtilP(SampleRatioMismatch, each_SampleRatioMismatch); output["SampleRatioMismatch"] = each_SampleRatioMismatch;
                Json::Value each_ScorecardDataRows; ToJsonUtilO(ScorecardDataRows, each_ScorecardDataRows); output["ScorecardDataRows"] = each_ScorecardDataRows;
                return output;
            }
        };

        struct GetLatestScorecardResult : public PlayFabResultCommon
        {
            Boxed<Scorecard> pfScorecard;

            GetLatestScorecardResult() :
                PlayFabResultCommon(),
                pfScorecard()
            {}

            GetLatestScorecardResult(const GetLatestScorecardResult& src) :
                PlayFabResultCommon(),
                pfScorecard(src.pfScorecard)
            {}

            ~GetLatestScorecardResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Scorecard"], pfScorecard);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_pfScorecard; ToJsonUtilO(pfScorecard, each_pfScorecard); output["Scorecard"] = each_pfScorecard;
                return output;
            }
        };

        struct GetTreatmentAssignmentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;

            GetTreatmentAssignmentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity()
            {}

            GetTreatmentAssignmentRequest(const GetTreatmentAssignmentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity)
            {}

            ~GetTreatmentAssignmentRequest() = default;

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

        struct TreatmentAssignment : public PlayFabBaseModel
        {
            std::list<Variable> Variables;
            std::list<std::string> Variants;

            TreatmentAssignment() :
                PlayFabBaseModel(),
                Variables(),
                Variants()
            {}

            TreatmentAssignment(const TreatmentAssignment& src) :
                PlayFabBaseModel(),
                Variables(src.Variables),
                Variants(src.Variants)
            {}

            ~TreatmentAssignment() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Variables"], Variables);
                FromJsonUtilS(input["Variants"], Variants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Variables; ToJsonUtilO(Variables, each_Variables); output["Variables"] = each_Variables;
                Json::Value each_Variants; ToJsonUtilS(Variants, each_Variants); output["Variants"] = each_Variants;
                return output;
            }
        };

        struct GetTreatmentAssignmentResult : public PlayFabResultCommon
        {
            Boxed<TreatmentAssignment> pfTreatmentAssignment;

            GetTreatmentAssignmentResult() :
                PlayFabResultCommon(),
                pfTreatmentAssignment()
            {}

            GetTreatmentAssignmentResult(const GetTreatmentAssignmentResult& src) :
                PlayFabResultCommon(),
                pfTreatmentAssignment(src.pfTreatmentAssignment)
            {}

            ~GetTreatmentAssignmentResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["TreatmentAssignment"], pfTreatmentAssignment);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_pfTreatmentAssignment; ToJsonUtilO(pfTreatmentAssignment, each_pfTreatmentAssignment); output["TreatmentAssignment"] = each_pfTreatmentAssignment;
                return output;
            }
        };

        struct StartExperimentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ExperimentId;

            StartExperimentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ExperimentId()
            {}

            StartExperimentRequest(const StartExperimentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ExperimentId(src.ExperimentId)
            {}

            ~StartExperimentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                return output;
            }
        };

        struct StopExperimentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ExperimentId;

            StopExperimentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ExperimentId()
            {}

            StopExperimentRequest(const StopExperimentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ExperimentId(src.ExperimentId)
            {}

            ~StopExperimentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExperimentId"], ExperimentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExperimentId; ToJsonUtilS(ExperimentId, each_ExperimentId); output["ExperimentId"] = each_ExperimentId;
                return output;
            }
        };

        struct UpdateExperimentRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            Uint32 Duration;
            Boxed<ExperimentType> pfExperimentType;
            std::string Id;
            std::string Name;
            std::string SegmentId;
            time_t StartDate;
            std::list<std::string> TitlePlayerAccountTestIds;
            std::list<Variant> Variants;

            UpdateExperimentRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                Duration(),
                pfExperimentType(),
                Id(),
                Name(),
                SegmentId(),
                StartDate(),
                TitlePlayerAccountTestIds(),
                Variants()
            {}

            UpdateExperimentRequest(const UpdateExperimentRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                Duration(src.Duration),
                pfExperimentType(src.pfExperimentType),
                Id(src.Id),
                Name(src.Name),
                SegmentId(src.SegmentId),
                StartDate(src.StartDate),
                TitlePlayerAccountTestIds(src.TitlePlayerAccountTestIds),
                Variants(src.Variants)
            {}

            ~UpdateExperimentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["Duration"], Duration);
                FromJsonUtilE(input["ExperimentType"], pfExperimentType);
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["SegmentId"], SegmentId);
                FromJsonUtilT(input["StartDate"], StartDate);
                FromJsonUtilS(input["TitlePlayerAccountTestIds"], TitlePlayerAccountTestIds);
                FromJsonUtilO(input["Variants"], Variants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_Duration; ToJsonUtilP(Duration, each_Duration); output["Duration"] = each_Duration;
                Json::Value each_pfExperimentType; ToJsonUtilE(pfExperimentType, each_pfExperimentType); output["ExperimentType"] = each_pfExperimentType;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_SegmentId; ToJsonUtilS(SegmentId, each_SegmentId); output["SegmentId"] = each_SegmentId;
                Json::Value each_StartDate; ToJsonUtilT(StartDate, each_StartDate); output["StartDate"] = each_StartDate;
                Json::Value each_TitlePlayerAccountTestIds; ToJsonUtilS(TitlePlayerAccountTestIds, each_TitlePlayerAccountTestIds); output["TitlePlayerAccountTestIds"] = each_TitlePlayerAccountTestIds;
                Json::Value each_Variants; ToJsonUtilO(Variants, each_Variants); output["Variants"] = each_Variants;
                return output;
            }
        };

    }
}

#endif
