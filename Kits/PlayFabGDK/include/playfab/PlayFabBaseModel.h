#pragma once

#include <playfab/PlayFabPlatformMacros.h>
#include <playfab/PlayFabPlatformTypes.h>
#include <playfab/PlayFabJsonHeaders.h>
#include <playfab/PlayFabPlatformTypes.h>
#include <playfab/PlayFabPlatformUtils.h>

#include <assert.h>
#include <functional>
#include <list>
#include <memory>

namespace PlayFab
{
    class PlayFabAuthenticationContext;

    template <typename BoxedType>
    class Boxed
    {
    public:
        Boxed() : boxedValue(), isSet(false) {}
        Boxed(BoxedType value) : boxedValue(value), isSet(true) {}

        inline Boxed& operator=(BoxedType value) { boxedValue = value; isSet = true; return *this; }
        inline operator const BoxedType& () const { assert(notNull()); return *operator->(); }
        inline BoxedType* operator->() { return isSet ? &boxedValue : nullptr; }
        inline const BoxedType* operator->() const { return isSet ? &boxedValue : nullptr; }

        inline void setNull() { isSet = false; }
        inline bool notNull() const { return isSet; }
        inline bool isNull() const { return !isSet; }
    private:
        BoxedType boxedValue;
        bool isSet;
    };

    template<typename ResType> using ProcessApiCallback = std::function<void(const ResType& result, void* customData)>;

    /// <summary>
    /// Base class for all PlayFab Models
    /// Provides interface for converting to/from json
    /// </summary>
    struct PlayFabBaseModel
    {
        virtual ~PlayFabBaseModel() {}
        virtual void FromJson(const Json::Value& input) = 0;
        virtual Json::Value ToJson() const = 0;
    };

    /// <summary>
    /// Base class for all PlayFab Requests
    /// Adds a parameter that controls how requests are threaded
    /// </summary>
    struct PlayFabRequestCommon : public PlayFabBaseModel {
        /// <summary>
        /// An optional authentication context, it can used in multi-user scenarios
        /// </summary>
        std::shared_ptr<PlayFabAuthenticationContext> authenticationContext;
    };

    /// <summary>
    /// Base class for all PlayFab Results
    /// </summary>
    struct PlayFabResultCommon : public PlayFabBaseModel {
        Json::Value Request;
    };

    /// <summary>
    /// Base class for all PlayFab Login method Results
    /// </summary>
    struct PlayFabLoginResultCommon : public PlayFabResultCommon {
        /// <summary>
        /// An authentication context returned by Login methods, it can used in multi-user scenarios
        /// </summary>
        std::shared_ptr<PlayFabAuthenticationContext> authenticationContext;
    };

    // Utilities for [de]serializing time_t to/from json
    inline void ToJsonUtilT(const time_t input, Json::Value& output)
    {
        output = Json::Value(TimeTToIso8601String(input));
    }
    inline void FromJsonUtilT(const Json::Value& input, time_t& output)
    {
        if (input == Json::Value::null)
        {
            return;
        }
        output = Iso8601StringToTimeT(input.asString());
    }

    inline void ToJsonUtilT(const Boxed<time_t>& input, Json::Value& output)
    {
        if (input.isNull())
        {
            output = Json::Value();
        }
        else
        {
            ToJsonUtilT(static_cast<time_t>(input), output);
        }
    }

    inline void FromJsonUtilT(const Json::Value& input, Boxed<time_t>& output)
    {
        if (input == Json::Value::null)
        {
            output.setNull();
        }
        else
        {
            time_t outputVal = {};
            FromJsonUtilT(input, outputVal);
            output = outputVal;
        }
    }

    inline void ToJsonUtilT(const std::list<time_t>& input, Json::Value& output)
    {
        if (input.size() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(Json::arrayValue);
            Json::Value eachOutput;
            int index = 0;
            for (auto iter = input.begin(); iter != input.end(); ++iter)
            {
                ToJsonUtilT(*iter, eachOutput);
                output[index++] = eachOutput;
            }
        }
    }

    inline void FromJsonUtilT(const Json::Value& input, std::list<time_t>& output)
    {
        output.clear();
        if (input == Json::Value::null || !input.isArray())
        {
            return;
        }

        time_t eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilT(*iter, eachOutput);
            output.push_back(eachOutput);
        }
    }

    inline void ToJsonUtilT(const std::map<std::string, time_t>& input, Json::Value& output)
    {
        output = Json::Value(Json::objectValue);
        Json::Value eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            ToJsonUtilT(iter->second, eachOutput);
            output[iter->first] = eachOutput;
        }
    }

    inline void FromJsonUtilT(const Json::Value& input, std::map<std::string, time_t>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        time_t eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilT(*iter, eachOutput);
            output[iter.key().asString()] = eachOutput;
        }
    }

    // Utilities for [de]serializing EnumType to/from json
    template <typename EnumType> inline void ToJsonUtilE(const Boxed<EnumType>& input, Json::Value& output)
    {
        if (input.isNull())
        {
            output = Json::Value();
        }
        else
        {
            ToJsonEnum(input, output);
        }
    }

    template <typename EnumType> inline void FromJsonUtilE(const Json::Value& input, Boxed<EnumType>& output)
    {
        if (input == Json::Value::null)
        {
            output.setNull();
        }
        else
        {
            EnumType outputVal;
            FromJsonEnum(input, outputVal);
            output = outputVal;
        }
    }

    template <typename EnumType> inline void ToJsonUtilE(const std::list<EnumType>& input, Json::Value& output)
    {
        if (input.size() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(Json::arrayValue);
            Json::Value eachOutput;
            int index = 0;
            for (auto iter = input.begin(); iter != input.end(); ++iter)
            {
                ToJsonEnum(*iter, eachOutput);
                output[index++] = eachOutput;
            }
        }
    }

    template <typename EnumType> inline void FromJsonUtilE(const Json::Value& input, std::list<EnumType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        EnumType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonEnum(*iter, eachOutput);
            output.push_back(eachOutput);
        }
    }

    template <typename EnumType> inline void ToJsonUtilE(const std::map<std::string, EnumType>& input, Json::Value& output)
    {
        output = Json::Value(Json::objectValue);
        Json::Value eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            ToJsonEnum(iter->second, eachOutput);
            output[iter->first] = eachOutput;
        }
    }

    template <typename EnumType> inline void FromJsonUtilE(const Json::Value& input, std::map<std::string, EnumType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        EnumType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonEnum(iter, eachOutput);
            output[iter.key()] = eachOutput;
        }
    }

    // Utilities for [de]serializing string to/from json
    inline void ToJsonUtilS(const std::string& input, Json::Value& output)
    {
        if (input.length() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(input);
        }
    }

    inline void FromJsonUtilS(const Json::Value& input, std::string& output)
    {
        if (input == Json::Value::null)
        {
            output.clear();
        }
        else
        {
            output = input.asString();
        }
    }

    inline void ToJsonUtilS(const std::list<std::string>& input, Json::Value& output)
    {
        if (input.size() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(Json::arrayValue);
            Json::Value eachOutput;
            int index = 0;
            for (auto iter = input.begin(); iter != input.end(); ++iter)
            {
                ToJsonUtilS(*iter, eachOutput);
                output[index++] = eachOutput;
            }
        }
    }

    inline void FromJsonUtilS(const Json::Value& input, std::list<std::string>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        std::string eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilS(*iter, eachOutput);
            output.push_back(eachOutput);
        }
    }

    inline void ToJsonUtilS(const std::map<std::string, std::string>& input, Json::Value& output)
    {
        output = Json::Value(Json::objectValue);
        Json::Value eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            ToJsonUtilS(iter->second, eachOutput);
            output[iter->first] = eachOutput;
        }
    }

    inline void FromJsonUtilS(const Json::Value& input, std::map<std::string, std::string>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        std::string eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilS(*iter, eachOutput);
            output[iter.key().asString()] = eachOutput;
        }
    }

    // Utilities for [de]serializing PlayFabObjects to/from json
    inline void ToJsonUtilO(const PlayFabBaseModel& input, Json::Value& output)
    {
        output = input.ToJson();
    }

    inline void FromJsonUtilO(const Json::Value& input, PlayFabBaseModel& output)
    {
        output.FromJson(input);
    }

    template <typename ObjectType> inline void ToJsonUtilO(const Boxed<ObjectType> input, Json::Value& output)
    {
        if (input.isNull())
        {
            output = Json::Value();
        }
        else
        {
            output = static_cast<ObjectType>(input).ToJson();
        }
    }

    template <typename ObjectType> inline void FromJsonUtilO(const Json::Value& input, Boxed<ObjectType>& output)
    {
        if (input == Json::Value::null)
        {
            output.setNull();
        }
        else
        {
            ObjectType outputTemp;
            outputTemp.FromJson(input);
            output = outputTemp;
        }
    }

    template <typename ObjectType> inline void ToJsonUtilO(const std::list<ObjectType>& input, Json::Value& output)
    {
        if (input.size() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(Json::arrayValue);
            Json::Value eachOutput;
            int index = 0;
            for (auto iter = input.begin(); iter != input.end(); ++iter)
            {
                ToJsonUtilO(*iter, eachOutput);
                output[index++] = eachOutput;
            }
        }
    }

    template <typename ObjectType> inline void FromJsonUtilO(const Json::Value& input, std::list<ObjectType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        ObjectType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilO(*iter, eachOutput);
            output.push_back(eachOutput);
        }
    }

    template <typename ObjectType> inline void ToJsonUtilO(const std::map<std::string, ObjectType>& input, Json::Value& output)
    {
        output = Json::Value(Json::objectValue);
        Json::Value eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            ToJsonUtilO(iter->second, eachOutput);
            output[iter->first] = eachOutput;
        }
    }

    template <typename ObjectType> inline void FromJsonUtilO(const Json::Value& input, std::map<std::string, ObjectType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        ObjectType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilO(*iter, eachOutput);
            output[iter.key().asString()] = eachOutput;
        }
    }

    // Utilities for [de]serializing primitives to/from json
    template <typename PrimitiveType> inline void ToJsonUtilP(const PrimitiveType& input, Json::Value& output)
    {
        output = Json::Value(input);
    }

    inline void FromJsonUtilP(const Json::Value& input, bool& output)
    {
        output = input == Json::Value::null ? false : input.asBool();
    }

    inline void FromJsonUtilP(const Json::Value& input, Int16& output)
    {
        output = input == Json::Value::null ? 0 : static_cast<Int16>(input.asInt());
    }

    inline void FromJsonUtilP(const Json::Value& input, Uint16& output)
    {
        output = input == Json::Value::null ? static_cast<Uint16>(0) : static_cast<Uint16>(input.asInt());
    }

    inline void FromJsonUtilP(const Json::Value& input, Int32& output)
    {
        output = input == Json::Value::null ? 0 : input.asInt();
    }

    inline void FromJsonUtilP(const Json::Value& input, Uint32& output)
    {
        output = input == Json::Value::null ? 0 : input.asUInt();
    }

    inline void FromJsonUtilP(const Json::Value& input, Int64& output)
    {
        output = input == Json::Value::null ? 0 : input.asInt64();
    }

    inline void FromJsonUtilP(const Json::Value& input, Uint64& output)
    {
        output = input == Json::Value::null ? 0 : input.asUInt64();
    }

    inline void FromJsonUtilP(const Json::Value& input, float& output)
    {
        output = input == Json::Value::null ? 0 : input.asFloat();
    }

    inline void FromJsonUtilP(const Json::Value& input, double& output)
    {
        output = input == Json::Value::null ? 0 : input.asDouble();
    }

    template <typename PrimitiveType> inline void ToJsonUtilP(const Boxed<PrimitiveType>& input, Json::Value& output)
    {
        if (input.isNull())
        {
            output = Json::Value();
        }
        else
        {
            ToJsonUtilP(static_cast<PrimitiveType>(input), output);
        }
    }

    template <typename PrimitiveType> inline void FromJsonUtilP(const Json::Value& input, Boxed<PrimitiveType>& output)
    {
        if (input == Json::Value::null)
        {
            output.setNull();
        }
        else
        {
            PrimitiveType outputVal;
            FromJsonUtilP(input, outputVal);
            output = outputVal;
        }
    }

    template <typename PrimitiveType> inline void ToJsonUtilP(const std::list<PrimitiveType>& input, Json::Value& output)
    {
        if (input.size() == 0)
        {
            output = Json::Value::null;
        }
        else
        {
            output = Json::Value(Json::arrayValue);
            Json::Value eachOutput;
            int index = 0;
            for (auto iter = input.begin(); iter != input.end(); ++iter)
            {
                ToJsonUtilP(*iter, eachOutput);
                output[index++] = eachOutput;
            }
        }
    }

    template <typename PrimitiveType> inline void FromJsonUtilP(const Json::Value& input, std::list<PrimitiveType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        PrimitiveType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilP(*iter, eachOutput);
            output.push_back(eachOutput);
        }
    }

    template <typename PrimitiveType> inline void ToJsonUtilP(const std::map<std::string, PrimitiveType>& input, Json::Value& output)
    {
        output = Json::Value(Json::objectValue);
        Json::Value eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            ToJsonUtilP(iter->second, eachOutput);
            output[iter->first] = eachOutput;
        }
    }

    template <typename PrimitiveType> inline void FromJsonUtilP(const Json::Value& input, std::map<std::string, PrimitiveType>& output)
    {
        output.clear();
        if (input == Json::Value::null)
        {
            return;
        }

        PrimitiveType eachOutput;
        for (auto iter = input.begin(); iter != input.end(); ++iter)
        {
            FromJsonUtilP(*iter, eachOutput);
            output[iter.key().asString()] = eachOutput;
        }
    }
}
