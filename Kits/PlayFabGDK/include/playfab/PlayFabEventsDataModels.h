#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace EventsModels
    {
        // Events Enums
        // Events Classes
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

        struct EventContents : public PlayFabBaseModel
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            std::string EventNamespace;
            std::string Name;
            std::string OriginalId;
            Boxed<time_t> OriginalTimestamp;
            Json::Value Payload;
            std::string PayloadJSON;

            EventContents() :
                PlayFabBaseModel(),
                CustomTags(),
                Entity(),
                EventNamespace(),
                Name(),
                OriginalId(),
                OriginalTimestamp(),
                Payload(),
                PayloadJSON()
            {}

            EventContents(const EventContents& src) :
                PlayFabBaseModel(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                EventNamespace(src.EventNamespace),
                Name(src.Name),
                OriginalId(src.OriginalId),
                OriginalTimestamp(src.OriginalTimestamp),
                Payload(src.Payload),
                PayloadJSON(src.PayloadJSON)
            {}

            ~EventContents() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["EventNamespace"], EventNamespace);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["OriginalId"], OriginalId);
                FromJsonUtilT(input["OriginalTimestamp"], OriginalTimestamp);
                Payload = input["Payload"];
                FromJsonUtilS(input["PayloadJSON"], PayloadJSON);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_EventNamespace; ToJsonUtilS(EventNamespace, each_EventNamespace); output["EventNamespace"] = each_EventNamespace;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_OriginalId; ToJsonUtilS(OriginalId, each_OriginalId); output["OriginalId"] = each_OriginalId;
                Json::Value each_OriginalTimestamp; ToJsonUtilT(OriginalTimestamp, each_OriginalTimestamp); output["OriginalTimestamp"] = each_OriginalTimestamp;
                output["Payload"] = Payload;
                Json::Value each_PayloadJSON; ToJsonUtilS(PayloadJSON, each_PayloadJSON); output["PayloadJSON"] = each_PayloadJSON;
                return output;
            }
        };

        struct WriteEventsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::list<EventContents> Events;

            WriteEventsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Events()
            {}

            WriteEventsRequest(const WriteEventsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Events(src.Events)
            {}

            ~WriteEventsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Events"], Events);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Events; ToJsonUtilO(Events, each_Events); output["Events"] = each_Events;
                return output;
            }
        };

        struct WriteEventsResponse : public PlayFabResultCommon
        {
            std::list<std::string> AssignedEventIds;

            WriteEventsResponse() :
                PlayFabResultCommon(),
                AssignedEventIds()
            {}

            WriteEventsResponse(const WriteEventsResponse& src) :
                PlayFabResultCommon(),
                AssignedEventIds(src.AssignedEventIds)
            {}

            ~WriteEventsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AssignedEventIds"], AssignedEventIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AssignedEventIds; ToJsonUtilS(AssignedEventIds, each_AssignedEventIds); output["AssignedEventIds"] = each_AssignedEventIds;
                return output;
            }
        };

    }
}

#endif
