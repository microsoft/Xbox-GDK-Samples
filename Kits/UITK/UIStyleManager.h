//--------------------------------------------------------------------------------------
// File: UIStyleManager.h
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

#include "SimpleMath.h"

#include "UISerializer.h"
#include "UIStyle.h"
#include "UIStyleRenderer.h"

NAMESPACE_ATG_UITK_BEGIN

class UIManager;

template <typename T>
std::shared_ptr<T> CastPtr(UIStylePtr stylePtr)
{
    return std::static_pointer_cast<T>(stylePtr);
}

/// The singleton manager which manages all of the uniquely named UI styles.  The
/// manager is responsible for managing the styles, rendering with styles, handling
/// serialization, and other high level style related functions like registering
/// style classes.
class UIStyleManager
{
    DECLARE_CLASS_LOG();

public:
    UIStyleManager(UIDataDefinitions&);

public:
    template <typename T>
    void RegisterStyleFactory(ID styleClassId)
    {
        if (m_styleFactories.find(styleClassId) != m_styleFactories.end())
        {
            throw UIException(styleClassId, "Duplicate style class name registered.");
        }

        auto newFactory = new T();
        m_styleFactories.emplace(
            styleClassId,
            UIStyleFactoryPtr(newFactory));
    }

    inline UIStyleRenderer& GetStyleRenderer() const
    {
        return *m_styleRenderer.get();
    }

    UIStylePtr GetById(const ID& id) const;

    template <typename T>
    std::shared_ptr<T> GetTypedById(const ID& id) const
    {
        return std::static_pointer_cast<T>(GetById(id));
    }

    ID LoadStyleFromData(const ID& context, UIDataPtr data, bool createAnonymousId = false);

    template<typename T>
    std::shared_ptr<T> CreateDefaultStyle(const ID& id)
    {
        auto style = std::shared_ptr<T>(new T(*this, id, ID::Default));
        m_stylesById[id] = style;
        return style;
    }

public:
    void InitializeStyleRenderer(UIStyleRendererPtr&& styleRenderer);
    void ResetStyleRenderer();
    bool HasStyleRenderer()
    {
        return bool(m_styleRenderer);
    }

    void SetRotation(UIRotation rotation)
    {
        m_styleRenderer->SetRotation(rotation);
    }

private:
    uint32_t m_styleIdCounter;
    UIDataDefinitions& m_dataDefinitions;
    UIStyleRendererPtr m_styleRenderer;
    std::map<ID, UIStyleFactoryPtr> m_styleFactories;
    std::map<ID, UIStylePtr> m_stylesById;

private:
    void RegisterInternalStyleFactories();
    UIStylePtr MakeStyleFromData(const ID& context, UIDataPtr data, bool createAnonymousId = false);
    UIStylePtr AllocateStyle(const ID& styleClassId, const ID& styleId, UIDataPtr data);

    ID GenerateStyleID(const std::string& styleClassId);
    void FlattenAllStyles();

    friend class UIManager;
};

NAMESPACE_ATG_UITK_END
