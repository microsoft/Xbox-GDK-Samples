//--------------------------------------------------------------------------------------
// File: UISerializer.h
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <fstream>
#include <charconv>
#include <assert.h>

#include "Json.h"
#include "SimpleMath.h"

#include "UICore.h"

#define UI_ASSERT_ISOBJECT()                                                                \
{                                                                                           \
    auto msg = std::string("Element is not an object, it is a: ") + m_myJson->type_name();  \
    UI_ASSERT(IsObject() || UI_INVALID_OPERATION, msg);                                     \
}

#define UI_ASSERT_ISARRAY()                                                                 \
{                                                                                           \
    auto msg = std::string("Element is not an array, it is a: ") + m_myJson->type_name();  \
    UI_ASSERT(IsArray() || UI_INVALID_OPERATION, msg);                                      \
}

#define NAMESPACE_DIRECTX_SIMPLEMATH_BEGIN namespace DirectX { namespace SimpleMath {
#define NAMESPACE_DIRECTX_SIMPLEMATH_END } }

NAMESPACE_DIRECTX_SIMPLEMATH_BEGIN

/// json type conversion for the Vector2 type to json
inline void to_json(json& j, const DirectX::SimpleMath::Vector2& vector)
{
    std::array<float, 2> arr{ vector.x, vector.y };
    j = json(arr);
}

/// json type conversion for the Vector2 type from json
inline void from_json(const json& j, DirectX::SimpleMath::Vector2& vector)
{
    std::vector<float> v = j.get<std::vector<float>>();
    assert(v.size() == 2);
    vector = DirectX::SimpleMath::Vector2(v.data());
}

/// json type conversion for the Vector3 type to json
inline void to_json(json& j, const DirectX::SimpleMath::Vector3& vector)
{
    std::array<float, 3> arr{ vector.x, vector.y, vector.z };
    j = json(arr);
}

/// json type conversion for the Vector3 type from json
inline void from_json(const json& j, DirectX::SimpleMath::Vector3& vector)
{
    std::vector<float> v = j.get<std::vector<float>>();
    assert(v.size() == 3);
    vector = DirectX::SimpleMath::Vector3(v.data());
}

/// json type conversion for the Vector4 type to json
inline void to_json(json& j, const DirectX::SimpleMath::Vector4& vector)
{
    std::array<float, 4> arr{ vector.x, vector.y, vector.z, vector.w };
    j = json(arr);
}

/// json type conversion for the Vector4 type from json
inline void from_json(const json& j, DirectX::SimpleMath::Vector4& vector)
{
    std::vector<float> v = j.get<std::vector<float>>();
    assert(v.size() == 4);
    vector = DirectX::SimpleMath::Vector4(v.data());
}

/// json type conversion for the Color type to json
inline void to_json(json& j, const Color& color)
{
    std::array<float, 4> arr
    {
        std::floor(color.R() * 255.0f),
        std::floor(color.G() * 255.0f),
        std::floor(color.B() * 255.0f),
        color.A()
    };
    j = json(arr);
}

/// json type conversion for the Color type from json
inline void from_json(const json& j, Color& color)
{
    std::vector<float> v = j.get<std::vector<float>>();
    if (v.size() < 4)
    {
        v.push_back(1.f);
    }
    assert(v.size() == 4);

    Vector4 v4 = Vector4(v[0] / 255.f, v[1] / 255.f, v[2] / 255.f, v[3]);
    v4.Clamp(Vector4::Zero, Vector4::One);
    color = v4;
}

/// json type conversion for the Rectangle type to json
inline void to_json(json& j, const Rectangle& rectangle)
{
    std::array<long, 4> arr{ rectangle.x, rectangle.y, rectangle.width, rectangle.height };
    j = json(arr);
}

/// json type conversion for the Rectangle type from json
inline void from_json(const json& j, Rectangle& rectangle)
{
    std::vector<long> v = j.get<std::vector<long>>();
    assert(v.size() == 4);
    rectangle = Rectangle(v[0], v[1], v[2], v[3]);
}

NAMESPACE_DIRECTX_SIMPLEMATH_END

inline void to_json(json& j, const RECT& rect)
{
    std::array<long, 4> arr{ rect.left, rect.top, rect.right, rect.bottom };
    j = json(arr);
}

inline void from_json(const json& j, RECT& rect)
{
    std::vector<long> v = j.get<std::vector<long>>();
    assert(v.size() == 4);
    rect = RECT{ v[0], v[1], v[2], v[3] };
}

///////////////////////////////////////////////////////////////////////////////

NAMESPACE_ATG_UITK_BEGIN

struct Anchor;
void to_json(json& j, const Anchor& anchor);
void from_json(const json& j, Anchor& anchor);

class UISerializedObject;

std::vector<std::string> GetKeysFromObject(const json& jsObj);

/// An interface for accessing the runtime properties for a piece of
/// serialized data.  The properties follow a JSON convention of either
/// being an atomic value property, an array of properties, or a container
/// of key/value pairs of properties.  It uses the MIT-license
/// NHolmann JSON header (https://github.com/nlohmann/json) to implement
/// the serialization.
class UISerializedObject
{
private:
    std::shared_ptr<json> m_myJson;
    bool m_validated = false;

private:
    static bool IsValid(const json& j)
    {
        return (!j.empty() && j.is_structured()) || (j.is_primitive() && !j.is_null());
    }

public:
    UISerializedObject(const json& myJson) :
        m_myJson(std::make_shared<json>(myJson)),
        m_validated(IsValid(myJson))
    {

    }

    UISerializedObject(const std::string& filename)
    {
        UI_ASSERT(Util::FileExists(filename), std::string("File does not exist: ") + filename);

        std::ifstream filestream(filename);
        m_myJson = std::make_shared<json>();
        filestream >> *m_myJson;
        m_validated = IsValid(*m_myJson);
    }

    UISerializedObject(std::istream& istream)
    {
        m_myJson = std::make_shared<json>();
        istream >> *m_myJson;
        m_validated = IsValid(*m_myJson);
    }

    virtual ~UISerializedObject() = default;

    operator bool() const
    {
        return m_validated;
    }

public:
    void ApplyPatch(_In_ UIDataPtr patch)
    {
        if (patch && patch->IsValid())
        {
            m_myJson->merge_patch(*patch->m_myJson);
        }
    }

    template <typename T>
    auto Get() -> decltype(auto)
    {
        return m_myJson->get<T>();
    }

    template <>
    auto Get<ID>() -> decltype(auto)
    {
        const auto id = m_myJson->get<std::string>();
        return ID(id);
    }

    template <typename T>
    auto Get(uint32_t index) -> decltype(auto)
    {
        UI_ASSERT_ISARRAY();
        return GetArrayValue(index)->Get<T>();
    }

    template <typename T>
    auto Get(const std::string& key) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        return GetObjectValue(key)->Get<T>();
    }

    template<>
    auto Get<ID>(const std::string& key) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        return ID(GetObjectValue(key)->Get<std::string>());
    }

    template<>
    auto Get<DirectX::SimpleMath::Vector2>(const std::string& key) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        return GetObjectValue(key)->Get<DirectX::SimpleMath::Vector2>();
    }

    template<>
    auto Get<DirectX::SimpleMath::Vector3>(const std::string& key) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        return GetObjectValue(key)->Get<DirectX::SimpleMath::Vector3>();
    }

    template<>
    auto Get<DirectX::SimpleMath::Vector4>(const std::string& key) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        return GetObjectValue(key)->Get<DirectX::SimpleMath::Vector4>();
    }

    template <typename T>
    auto GetIfExists(const std::string& key, uint32_t index, T defaultValue) -> decltype(auto)
    {
        UI_ASSERT_ISOBJECT();
        if (IsObject())
        {
            return GetObjectValue(key)->GetIfExists<T>(index, defaultValue);
        }
        else
        {
            return defaultValue;
        }
    }

    template <typename T>
    auto GetIfExists(uint32_t index, const T& defaultValue) -> decltype(auto)
    {
        if (IsArray() && index < GetArrayCount())
        {
            return GetArrayValue(index)->Get<T>();
        }
        else
        {
            return T(defaultValue);
        }
    }

    template <>
    auto GetIfExists<ID>(uint32_t index, const ID& defaultValue) -> decltype(auto)
    {
        if (IsArray() && index < GetArrayCount())
        {
            return GetArrayValue(index)->Get<ID>();
        }

        return ID(defaultValue);
    }

    template <typename T>
    auto GetIfExists(const std::string& key, const T& defaultValue) -> decltype(auto)
    {
        if (IsObject() && Exists(key))
        {
            return GetObjectValue(key)->Get<T>();
        }

        return T(defaultValue);
    }

    template<>
    auto GetIfExists<ID>(const std::string& key, const ID& defaultValue) -> decltype(auto)
    {
        if (IsObject() && Exists(key))
        {
            return ID(GetObjectValue(key)->Get<std::string>());
        }

        return ID(defaultValue);
    }

    template<typename T>
    bool GetTo(const std::string& key, T& dest)
    {
        auto item = m_myJson->find(key);
        const bool exists = (item != m_myJson->end());

        if (exists)
        {
            dest = item.value().get<T>();
        }
        return exists;
    }

    template<>
    bool GetTo<ID>(const std::string& key, ID& dest)
    {
        std::string temp;
        const bool exists = GetTo(key, temp);
        if (exists)
        {
            dest = ID(temp);
        }
        return exists;
    }

    bool Exists(uint32_t index) const
    {
        UI_ASSERT_ISARRAY();
        return index < GetArrayCount();
    }

    bool Exists(const std::string& key) const
    {
        UI_ASSERT_ISOBJECT();
        return m_myJson->contains(key) && UISerializedObject::IsValid(m_myJson->at(key));
    }

    inline bool IsValid() const
    {
        return m_validated;
    }

    bool IsBoolean() const
    {
        return IsValid() && m_myJson->is_boolean();
    }

    bool IsNumber() const
    {
        return IsValid() && m_myJson->is_number();
    }

    bool IsInteger() const
    {
        return IsValid() && m_myJson->is_number_integer();
    }

    bool IsString() const
    {
        return IsValid() && m_myJson->is_string();
    }

    bool IsObject() const
    {
        return IsValid() && m_myJson->is_object();
    }

    bool IsArray() const
    {
        return IsValid() && m_myJson->is_array();
    }

    size_t GetKeyCount() const
    {
        UI_ASSERT_ISOBJECT();

        return m_myJson->size();
    }

    std::vector<std::string> GetKeys()
    {
        UI_ASSERT_ISOBJECT();

        return GetKeysFromObject(*m_myJson);
    }

    UIDataPtr operator[](const std::string& key)
    {
        return GetObjectValue(key);
    }

    UIDataPtr GetObjectValue(const std::string& key)
    {
        UI_ASSERT_ISOBJECT();

        return std::make_shared<UISerializedObject>((*m_myJson)[key]);
    }

    size_t GetArrayCount() const
    {
        UI_ASSERT_ISARRAY();

        return m_myJson->size();
    }

    UIDataPtr operator[](uint32_t index)
    {
        return GetArrayValue(index);
    }

    UIDataPtr GetArrayValue(uint32_t index)
    {
        UI_ASSERT(index <= GetArrayCount(), "Index is out of range!");
        return std::make_shared<UISerializedObject>((*m_myJson)[index]);
    }

    template<typename T>
    auto GetArrayValues(const std::string& key) -> std::vector<T>
    {
        UI_ASSERT_ISOBJECT();
        auto results = m_myJson->get<std::vector<T>>(key);
        return results;
    }


    void ReplaceObjectValue(const std::string& key, const UIDataPtr& replacement);

    void ReplaceArrayValue(uint32_t index, const UIDataPtr& replacement);

    std::string Dump()
    {
        const int c_indentSize = 4;
        return m_myJson->dump(c_indentSize);
    }

public:
    friend class UI_SERIALIZER;         // tests
    friend class UI_DATA_DEFINITIONS;   // tests
    friend class UIDataDefinitions;     // close friend...
};

/// A structure for holding onto data elements that represent data definitions whereby
/// a key-word symbolically represents another data construct.  For example:
///     "#keyword": [1, 2, 3, 4]
/// means that whenever some data value is a string that equals to "#keyword" we
/// can replace the key-word portion with the corresponding data before parsing like so:
///     "myDataKey": "#keyword" ... becomes
///     "myDataKey": [1, 2, 3, 4]
class UIDataDefinitions
{
    DECLARE_CLASS_LOG();
public:
    using BindingsMap = std::map<std::string, json>;

    static constexpr char c_definitionChar = u8'#';
    static constexpr int c_opaqueColorCodeLength = 7;  // #aabbcc
    static constexpr int c_alphaColorCodeLength = 9;   // #aabbccdd

public:
    static bool IsDefinition(const std::string& key)
    {
        return key.size() > 1 && key[0] == c_definitionChar && key[1] != c_definitionChar;
    }

    static bool IsCSSColor(const std::string& value);
    json ParseCSSColorJson(const std::string& value) const;

    void LoadDefinitions(UIDataPtr definitionsRoot, bool replaceExistingDefinition = true);

    UIDataPtr GetDefinition(const std::string& key);

    void SetDefinition(const std::string& key, UIDataPtr value);
    void SetDefinition(const std::string& key, const json& value);

    bool ReplaceAllDefinitionReferences(UIDataPtr data);

private:
    json GetDefinitionJson(const std::string& key);

private:
    BindingsMap m_definitions;

    friend class UI_DATA_DEFINITIONS;
};

/*public:*/

inline void UISerializedObject::ReplaceObjectValue(const std::string& key, const UIDataPtr& replacement)
{
    if (Exists(key))
    {
        json temp = *replacement->m_myJson;
        (*m_myJson)[key].swap(temp);
    }
}

inline void UISerializedObject::ReplaceArrayValue(uint32_t index, const UIDataPtr& replacement)
{
    if (Exists(index))
    {
        json temp = *replacement->m_myJson;
        (*m_myJson)[index].swap(temp);
    }
}

NAMESPACE_ATG_UITK_END
