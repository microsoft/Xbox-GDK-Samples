//--------------------------------------------------------------------------------------
// File: UICore.h
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
#include <cctype>
#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <map>
#include <assert.h>
#include <string_view>
#include "StringUtil.h"
#include "SimpleMath.h"
#include "UIDebugConfig.h"
#include "UIKeywords.h"
#include "UILog.h"

#define NAMESPACE_ATG_UITK_BEGIN namespace ATG { namespace UITK {
#define NAMESPACE_ATG_UITK_END } }

NAMESPACE_ATG_UITK_BEGIN

#define TODO_IMPL_PARAM(item) item

// Enum lookup table definitions
#define ID_ENUM_PAIR(id, e) { ID(id), e }
#define ENUM_LOOKUP_TABLE(Type, ...)														    \
        inline bool UIEnumLookup(const ID& id, Type& dest)									    \
        {																					    \
            static_assert(std::is_enum<Type>::value, "Needs to be an enum type.");			    \
            static const std::pair<ID, Type> enums[] = { __VA_ARGS__ };					        \
                                                                                                \
            if (!id) { return false; }															\
            auto it = std::find_if(std::begin(enums), std::end(enums),						    \
                [id](const std::pair<ID, Type>& enum_pair) -> bool							    \
            {																				    \
                return enum_pair.first == id;												    \
            });																				    \
                                                                                                \
            const bool exists = (it != std::end(enums));									    \
            if (exists) { dest = it->second; }												    \
            return exists;																	    \
        }																					    \
                                                                                                \
        inline bool UIEnumLookup(const ID&id, Type& dest, const Type& def)						\
        {																					    \
            const bool exists = UIEnumLookup(id, dest); 									    \
            if (!exists) { dest = def; }		   	    										\
            return exists;																	    \
        }

/// UI strings are strings which are displayed by the UI framework and
/// have to be localizable (they should be UTF-8 encoded)
using UIDisplayString = std::string;

/// this is used for declaraing UIDisplayString constants using the
/// constexpr keyword which requires a primitive type
using UIConstDisplayString = const char*;

constexpr char empty[] = "";
extern const UIDisplayString emptyItemDisplayString;

std::string NewUUID();

using Offsets   = RECT;
using Rectangle = DirectX::SimpleMath::Rectangle;
using Color     = DirectX::SimpleMath::Color;
using Vector2   = DirectX::SimpleMath::Vector2;
using Vector3   = DirectX::SimpleMath::Vector3;
using Vector4   = DirectX::SimpleMath::Vector4;
using TextureHandle = uint32_t;
using FontHandle = uint32_t;
using UIDataPtr = std::shared_ptr<class UISerializedObject>;
using TexturedQuad = std::pair<Rectangle, Rectangle>;   // source, dest

/// <summary>
/// UI elements are universally identified through a defined identifier
/// type as defined here to be a standard byte-per-character string.  These
/// identifiers are NOT case-sensitive.
/// </summary>
class ID
{
public:
    ID() : m_idStr(empty) {}

    explicit ID(const char* str)
    {
        m_idStr = str;
        DX::ToLowerInPlace(m_idStr);
    }

    explicit ID(const std::string& str)
    {
        m_idStr = str;
        DX::ToLowerInPlace(m_idStr);
    }

    ID(const ID& id) = default;
    ID(ID&& id) = default;

    ~ID() = default;

    ID& operator=(const char* str)
    {
        m_idStr = str;
        DX::ToLowerInPlace(m_idStr);
        return *this;
    }

    ID& operator=(const std::string& str)
    {
        m_idStr = str;
        DX::ToLowerInPlace(m_idStr);
        return *this;
    }

    ID& operator=(const ID& id) = default;
    ID& operator=(ID&& id) = default;

    const std::string& AsStr() const { return m_idStr; }
    const char* AsCStr() const { return m_idStr.c_str(); }

    bool operator<(const ID& id) const
    {
        return m_idStr < id.m_idStr;
    }

    bool operator==(const ID& id) const
    {
        return m_idStr == id.m_idStr;
    }

    operator bool() const
    {
        return m_idStr.length() > 0;
    }

    const static ID Default;

    static ID CreateUUID(const std::string& prefix = "") { return ID(prefix + ATG::UITK::NewUUID()); }

public:
    friend std::ostream &operator<<(std::ostream &output, const ID& id) {
        output << id.m_idStr;
        return output;
    }

private:
    std::string m_idStr;
};

enum class UIRotation : int
{
    Unspecified = 0,
    Identity = 1,
    Rotate90 = 2,
    Rotate180 = 3,
    Rotate270 = 4
};

/// horizontal anchors relate to positioning a UI element with respect to its
/// parent rectangle, or sizing it relative to its position, in the x dimension
enum class HorizontalAnchor : int
{
    Left = 0,
    Center = 1,
    Right = 2
};

/// vertical anchors relate to positioning a UI element with respect to its
/// parent rectangle, or sizing it relative to its position, in the y dimension
enum class VerticalAnchor : int
{
    Top = 0,
    Middle = 1,
    Bottom = 2,
};

struct Anchor
{
    HorizontalAnchor Horizontal;
    VerticalAnchor Vertical;
    constexpr Anchor() : Horizontal(HorizontalAnchor::Left), Vertical(VerticalAnchor::Top) {}
    constexpr Anchor(HorizontalAnchor horizontal, VerticalAnchor vertical) : Horizontal(horizontal), Vertical(vertical) {}
    constexpr Anchor(const Anchor&) = default;
    Anchor& operator=(const Anchor&) = default;
    static Anchor FromIDs(const ID& horizontal, const ID& vertical, const Anchor& defaultAnchor = Anchor());
    const static std::map<ID, int> AnchorIDMap;
    std::tuple<const char *, const char *> GetIDs() const;
    static char DebugShortName(HorizontalAnchor anchor) { return s_debugHorizontalAnchors[(int)anchor]; }
    static char DebugShortName(VerticalAnchor anchor) { return s_debugVerticalAnchors[(int)anchor]; }
private:
    static char s_debugHorizontalAnchors[3];
    static char s_debugVerticalAnchors[3];
};

/// A basic exception class that takes an ID from the UI and also a message with which
/// to post the exception with.
class UIException : public std::exception
{
public:
    std::string m_id;
    std::string m_message;

    UIException(const char* message) : m_id(), m_message(message) {}
    UIException(const std::string& message) : m_id(), m_message(message) {}

    UIException(const ID& id, const char* message) : m_id(id.AsStr())
    {
        FormatIDMessage(id, message);
    }

    UIException(const ID& id, const std::string& message) : m_id(id.AsStr())
    {
        FormatIDMessage(id, message);
    }

    template<typename T>
    inline void FormatIDMessage(const ID& id, const T& message)
    {
        std::stringstream ss;
        ss << message << " (id = " << id << ")";
        m_message = ss.str();
    }

    const char* what() const override
    {
        return m_message.c_str();
    }
};

struct FrameComputedValues
{
    static uint32_t s_currentFrame;
    static void NextFrame()
    {
        ++s_currentFrame;
    }
};

template<typename TValue>
class FrameComputedValue
{
    uint32_t frame;
    TValue value;

public:
    using ComputeFunc = std::function<TValue(void)>;
    FrameComputedValue() : frame{}, value{} {}
    TValue GetValue(ComputeFunc compute)
    {
        auto currentFrame = FrameComputedValues::s_currentFrame;
        if (currentFrame > frame)
        {
            value = compute();
            frame = currentFrame;
        }

        return value;
    }

    void InvalidateCache()
    {
        frame = 0;
    }
};

template<typename KeyType, typename ValueType>
class FrameComputedEvictCache
{
public:
    FrameComputedEvictCache(uint32_t targetSize = 64, uint32_t maxAge = 120) :
        m_valueMap(),
        m_frameMap(),
        m_targetSize(targetSize),
        m_maxAge(maxAge)
    {

    }

public:
    void Add(KeyType key, ValueType value)
    {
        if (Purge(m_targetSize) == 0 && Size() >= m_targetSize)
        {
            m_targetSize *= 2;
        }
        m_valueMap[key] = value;
        m_frameMap[key] = FrameComputedValues::s_currentFrame;
    }

    auto Find(KeyType &key)
    {
        auto iter = m_valueMap.find(key);
        if (iter != m_valueMap.end())
        {
            m_frameMap[key] = FrameComputedValues::s_currentFrame;
        }
        return iter;
    }

    auto Begin() { return m_valueMap.begin(); }
    auto End() { return m_valueMap.end(); }

    size_t Size() { return m_valueMap.size(); }

    inline size_t TargetSize() { return m_targetSize; }

protected:
    int Purge(uint32_t targetSize)
    {
        if (m_valueMap.size() < targetSize || FrameComputedValues::s_currentFrame < m_maxAge) { return 0; }

        auto frameIter = m_frameMap.begin();
        auto frameIterEnd = m_frameMap.end();
        auto valueIter = m_valueMap.begin();

        int removed = 0;

        while (frameIter != frameIterEnd)
        {
            if (frameIter->second <= FrameComputedValues::s_currentFrame - m_maxAge)
            {
                valueIter = m_valueMap.erase(valueIter);
                frameIter = m_frameMap.erase(frameIter);
                ++removed;
            }
            else
            {
                ++frameIter;
                ++valueIter;
            }
        }

        return removed;
    }

protected:
    std::map<KeyType, ValueType>    m_valueMap;
    std::map<KeyType, uint32_t>     m_frameMap;
    uint32_t                        m_targetSize;
    uint32_t                        m_maxAge;
};

namespace Util
{
    template<typename ItemType, typename Collection>
    bool Contains(const Collection& collection, const ItemType& item)
    {
        return (collection.find(item) != collection.end());
    }

    bool FileExists(const std::string& path);

    namespace Map
    {
        template<typename KeyType = ID, typename ItemType>
        ItemType GetOrCreate(const ID& key, std::map<KeyType, ItemType>& collection, std::function<ItemType()> factory)
        {
            using MapType = std::map<KeyType, ItemType>;

            typename MapType::iterator iter = collection.find(key);
            if (iter == collection.end())
            {
                iter = collection.insert(collection.begin(), std::pair<KeyType, ItemType>(key, factory()));
            }
            return iter->second;
        }
    }
}

NAMESPACE_ATG_UITK_END
