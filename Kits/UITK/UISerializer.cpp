#include "pch.h"
#include "UICore.h"
#include "UISerializer.h"

NAMESPACE_ATG_UITK_BEGIN

using BindingsMap = std::map<std::string, UIDataPtr>;

INITIALIZE_CLASS_LOG_DEBUG(UIDataDefinitions);

bool UIDataDefinitions::IsCSSColor(const std::string& value)
{
    auto len = value.size();
    if ((len == c_opaqueColorCodeLength || len == c_alphaColorCodeLength)
        && value[0] == c_definitionChar
        && std::all_of(value.begin() + 1, value.end(), isxdigit))
    {
        return true;
    }
    return false;
}

void UIDataDefinitions::LoadDefinitions(UIDataPtr definitionsRoot, bool replaceExistingDefinition)
{
    auto keys = definitionsRoot->GetKeys();
    for (const auto& key : keys)
    {
        if (IsDefinition(key) && (!Util::Contains(m_definitions, key) || replaceExistingDefinition))
        {
            SetDefinition(key, definitionsRoot->GetObjectValue(key));
        }
    }
}

UIDataPtr UIDataDefinitions::GetDefinition(const std::string& key)
{
    auto js = GetDefinitionJson(key);
    if (!js.empty())
    {
        return std::make_shared<UISerializedObject>(js);
    }

    return nullptr;
}

inline void UIDataDefinitions::SetDefinition(const std::string& key, UIDataPtr value)
{
    SetDefinition(key, *value->m_myJson);
}

inline void UIDataDefinitions::SetDefinition(const std::string& key, const json& value)
{
    m_definitions[key] = value;
}

json UIDataDefinitions::ParseCSSColorJson(const std::string& value) const
{
    uint32_t result = 0;
    try
    {
        std::from_chars(value.data() + 1, value.data() + value.size(), result, 16);
    }
    catch (const std::exception e)
    {
        UILOG_ERROR(e.what());
        throw e;
    }

    json color;
    float alpha = 1.0f;

    if (value.size() == c_alphaColorCodeLength)
    {
        alpha = float(result & 0xFF) / 255.f;
        result >>= 8;
    }
    int offset = 16;
    while (offset >= 0)
    {
        color.push_back((result >> offset) & 0xFF);
        offset -= 8;
    }

    color.push_back(alpha);

    return color;
}

bool UIDataDefinitions::ReplaceAllDefinitionReferences(UIDataPtr data)
{
    std::function<bool(json&)> replaceDefinitionsInner = [&](json& js)
    {
        bool wasUpdated = false;
        UILOG_TRACE(js.dump());
        if (js.is_object())
        {
            for (auto& [key, val] : js.items())
            {
                if (val.is_string())
                {
                    auto valString = val.get<std::string>();
                    if (IsDefinition(valString))
                    {
                        json temp(GetDefinitionJson(valString));
                        val.swap(temp);
                        wasUpdated = true;
                    }
                }

                if (val.is_object() || val.is_array())
                {
                    wasUpdated |= replaceDefinitionsInner(val);
                }
            }
        }
        else if (js.is_array())
        {
            for (auto& item : js)
            {
                if (item.is_string())
                {
                    auto valString = item.get<std::string>();
                    if (IsDefinition(valString))
                    {
                        json temp(GetDefinitionJson(valString));
                        item.swap(temp);
                        wasUpdated = true;
                    }
                }

                if (item.is_object() || item.is_array())
                {
                    wasUpdated |= replaceDefinitionsInner(item);
                }
            }
        }

        return wasUpdated;
    };

    auto& js = *data->m_myJson;
    return replaceDefinitionsInner(js);
}

json UIDataDefinitions::GetDefinitionJson(const std::string& key)
{
    const auto& iter = m_definitions.find(key);
    if (iter != m_definitions.end())
    {
        return iter->second;
    }
    else if (IsCSSColor(key))
    {
        auto color = ParseCSSColorJson(key);
        SetDefinition(key, color);
        return color;
    }
    else
    {
        UILOG_WARN("Unrecognized definition { %s } - Ensure a value for this definition is included in the layout files.", key.c_str());
        return json();
    }
}

NAMESPACE_ATG_UITK_END


