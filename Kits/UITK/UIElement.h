//--------------------------------------------------------------------------------------
// File: UIElement.h
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

#include "UIManager.h"
#include "UIStyle.h"
#include "UIEvent.h"

#include <type_traits>

NAMESPACE_ATG_UITK_BEGIN

//! @anchor UIElement
//! @class ElementDataProperties UIElement.h "UITK/UIElement.h"
//! @brief Basic UI element instance properties shared by all UI element property classes.
struct ElementDataProperties
{
    ElementDataProperties() :
        positioningAnchor(c_defaultHorizontalAnchor, c_defaultVerticalAnchor),
        sizingAnchor(c_defaultHorizontalAnchor, c_defaultVerticalAnchor),
        relativePosition(c_defaultPosition),
        relativeSize(c_undefinedSize),
        visible(c_defaultVisibility),
        enabled(c_defaultEnabling),
        focusable(c_defaultFocusable),
        focused(c_defaultFocusing)
    {
    }

    //! @subpage styleId "styleId JSON property"
    ID styleId;                         // TODO: default to a default style from UIStyleManager?
    //! @subpage positioningAnchor "positioningAnchor JSON property"
    Anchor positioningAnchor;           // ordered as horizontal & vertical
    //! @subpage sizingAnchor "sizingAnchor JSON property"
    Anchor sizingAnchor;                // ordered as horizontal & vertical
    //! @subpage position "position JSON property"
    Vector2 relativePosition;
    //! @subpage size "size JSON property"
    Vector2 relativeSize;
    //! @subpage visible "visible JSON property"
    bool visible;
    //! @subpage enabled "enabled JSON property"
    bool enabled;
    //! @subpage focusable "focusable JSON property" 
    bool focusable;
    //! @private
    bool focused;

    //! @privatesection
    static constexpr HorizontalAnchor   c_defaultHorizontalAnchor = HorizontalAnchor::Left;
    static constexpr VerticalAnchor     c_defaultVerticalAnchor = VerticalAnchor::Top;
    static constexpr Anchor				c_defaultAnchor{ c_defaultHorizontalAnchor, c_defaultVerticalAnchor };
    static constexpr bool               c_defaultVisibility = true;
    static constexpr bool               c_defaultEnabling = true;
    static constexpr bool               c_defaultFocusable = false;
    static constexpr bool               c_defaultFocusing = false;
    static constexpr Vector2            c_defaultPosition = { 0.0f, 0.0f };
    static constexpr Vector2            c_undefinedSize = { -INFINITY, -INFINITY };
};

//! @private
/// This is the basic building block for all simple UI elements, compound controls
/// and other visual rectangles that are placed on the screen in order to stand
/// up a sample's UI.
class UIElement : public std::enable_shared_from_this<UIElement>
{
    DECLARE_CLASS_LOG();

public:
    // due to the nature of shared pointers, this destructor should *only*
    // be called when all references to this element have been severed
    // which means it must have no children and be fully detached already.

    virtual ~UIElement() = default;

    void Initialize(ElementDataProperties&& props) { m_elementDataProperties = std::move(props); }

public:
    const ID& GetID() const { return m_id; }
protected:
    static const ID& ClassID() { static ID id("base"); return id; }
public:
    virtual const ID& GetClassID() const { return ClassID(); }

    UIElementPtr GetParent() const { return m_parent; }

    size_t GetChildCount() const
    {
        return m_children.size();
    }

    size_t GetSubElementCount() const
    {
        return m_subElements.size();
    }

    UIElementPtr GetChildByIndex(size_t index) const
    {
        return m_children[index];
    }

    UIElementPtr GetSubElementByIndex(size_t index) const
    {
        return m_subElements[index];
    }

    UIElementPtr GetChildById(ID id, bool searchDescendants = false) const
    {
        auto findIter = std::find_if(
            m_children.begin(),
            m_children.end(),
            [id](const UIElementPtr& child)
        {
            return child->GetID() == id;
        });

        if (findIter != m_children.end())
        {
            return *findIter;
        }
        else
        {
            if (searchDescendants)
            {
                for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
                {
                    auto result = (*iter)->GetChildById(id, searchDescendants);
                    if (result)
                    {
                        return result;
                    }
                }
            }
            return nullptr;
        }
    }

    UIElementPtr GetSubElementById(ID id, bool searchDescendants = false) const
    {
        auto findIter = std::find_if(
            m_subElements.begin(),
            m_subElements.end(),
            [id](const UIElementPtr& child)
        {
            return child->GetID() == id;
        });

        if (findIter != m_subElements.end())
        {
            return *findIter;
        }
        else
        {
            if (searchDescendants)
            {
                for (auto iter = m_subElements.begin(); iter != m_subElements.end(); ++iter)
                {
                    auto result = (*iter)->GetSubElementById(id, searchDescendants);
                    if (result)
                    {
                        return result;
                    }
                }
            }
            return nullptr;
        }
    }

    // note: returns ChildCount() if not found
    size_t GetChildIndex(UIElementPtr child) const
    {
        for (size_t index = 0; index < m_children.size(); ++index)
        {
            if (m_children[index] == child)
            {
                return index;
            }
        }

        return m_children.size();
    }

    // note: returns SubElementCount() if not found
    size_t GetSubElementIndex(UIElementPtr child) const
    {
        for (size_t index = 0; index < m_subElements.size(); ++index)
        {
            if (m_subElements[index] == child)
            {
                return index;
            }
        }

        return m_subElements.size();
    }

    template <typename T>
    std::shared_ptr<T> GetTypedChildById(ID id) const
    {
        return CastPtr<T>(GetChildById(id));
    }

    template <typename T>
    std::shared_ptr<T> GetTypedSubElementById(ID id) const
    {
        return CastPtr<T>(GetSubElementById(id));
    }

    virtual bool HitTestPixels(int x, int y);
    virtual bool HitTestRefUnits(int x, int y);
    virtual Rectangle GetScreenRectInPixels();
    virtual Rectangle GetScreenRectInRefUnits();

    Vector2 GetSizeInRefUnits();

    Rectangle GetPaddedRectInPixels();
    Rectangle GetPaddedRectInRefUnits();

    Rectangle GetMarginedRectInPixels();
    Rectangle GetMarginedRectInRefUnits();

    // Add a child from an already existing element
    void AddChild(UIElementPtr child);
    // Add a child using a provided layout file
    UIElementPtr AddChildFromLayout(const std::string& layoutFilePath);
    // Instantiate a prefab and add it as a child
    UIElementPtr AddChildFromPrefab(const std::string& prefabFilePath);

    ID GetStyleId()
    {
        return m_elementDataProperties.styleId;
    }

    void SetStyleId(ID newStyleId)
    {
        m_elementDataProperties.styleId = newStyleId;
        HandleStyleIdChanged();
    }

    bool IsAttachedToScene() const
    {
        return IsAncestorOfMe(m_uiManager.GetRootElement());
    }

    bool IsAncestorOfMe(UIElementPtr element) const;

#pragma region Focus state

    bool CanBeFocused() const
    {
        return IsVisible() && IsEnabled() && IsFocusable() && IsAttachedToScene();
    }

    bool IsFocusable() const
    {
        return m_elementDataProperties.focusable;
    }

    void SetFocusable(bool isFocusable)
    {
        m_elementDataProperties.focusable = isFocusable;
    }

    bool IsFocused() const
    {
        return m_elementDataProperties.focused;
    }

#pragma endregion

#pragma region Visible state

    bool IsVisible() const
    {
        return GetVisible() && (!m_parent || m_parent->IsVisible());
    }

    bool GetVisible() const
    {
        return m_elementDataProperties.visible;
    }

    void SetVisible(bool isVisible)
    {
        m_elementDataProperties.visible = isVisible;
    }

#pragma endregion

#pragma region Enable state

    bool IsEnabled() const
    {
        return GetEnabled() && (!m_parent || m_parent->IsEnabled());
    }

    bool GetEnabled() const
    {
        return m_elementDataProperties.enabled;
    }

    void SetEnabled(bool isEnabled)
    {
        m_elementDataProperties.enabled = isEnabled;
    }

#pragma endregion

    const Anchor& GetPositioningAnchor() const
    {
        return m_elementDataProperties.positioningAnchor;
    }

    void SetPositioningAnchor(const Anchor& positioningAnchor)
    {
        m_elementDataProperties.positioningAnchor = positioningAnchor;
        InvalidateRectangleCaches();
    }

    const Anchor& GetSizingAnchor() const
    {
        return m_elementDataProperties.sizingAnchor;
    }

    void SetSizingAnchor(const Anchor& sizingAnchor)
    {
        m_elementDataProperties.sizingAnchor = sizingAnchor;
        InvalidateRectangleCaches();
    }

    const Vector2& GetRelativePositionInRefUnits() const
    {
        return m_elementDataProperties.relativePosition;
    }

    void SetRelativePositionInRefUnits(const Vector2& newPosition)
    {
        m_elementDataProperties.relativePosition = newPosition;
        InvalidateRectangleCaches();
    }

    const Vector2& GetRelativeSizeInRefUnits() const
    {
        return m_elementDataProperties.relativeSize;
    }

    void SetRelativeSizeInRefUnits(const Vector2& newSize)
    {
        m_elementDataProperties.relativeSize = newSize;
        InvalidateRectangleCaches();
    }

protected:
    void InvalidateRectangleCaches()
    {
        m_screenRectInPixels.InvalidateCache();
        m_screenRectInRefUnits.InvalidateCache();
        m_paddedRectInPixels.InvalidateCache();
        m_paddedRectInRefUnits.InvalidateCache();
        m_marginedRectInPixels.InvalidateCache();
        m_marginedRectInRefUnits.InvalidateCache();
        m_sizeInPixels.InvalidateCache();
    }

protected:
    UIManager&                      m_uiManager;
    ID                              m_id;
    UIElementPtr                    m_parent;
    std::vector<UIElementPtr>       m_children;
    std::vector<UIElementPtr>       m_subElements;
    bool                            m_isSubElement;

    FrameComputedValue<Rectangle>   m_screenRectInPixels;
    FrameComputedValue<Rectangle>   m_screenRectInRefUnits;
    FrameComputedValue<Rectangle>   m_paddedRectInPixels;
    FrameComputedValue<Rectangle>   m_paddedRectInRefUnits;
    FrameComputedValue<Rectangle>   m_marginedRectInPixels;
    FrameComputedValue<Rectangle>   m_marginedRectInRefUnits;
    FrameComputedValue<Vector2>     m_sizeInPixels;

    UIStylePtr                      m_style;

    // basic data-driven properties

    ElementDataProperties m_elementDataProperties;

protected:
    UIElement() = delete;
    UIElement(const UIElement&) = delete;
    UIElement(UIElement&& element) = delete;
    UIElement(UIManager& uiManager, ID id) : m_uiManager(uiManager), m_id(id), m_isSubElement(false) {}

    void Clear();
    void AddSubElement(UIElementPtr subElement);
    bool IsSubElement() const { return m_isSubElement; }

    virtual void PostLoad() {}
    virtual void ResetEventState() {}
    virtual bool HandleInputEvent(const InputEvent&) { return false; }
    virtual bool HandleGlobalInputState(const UIInputState&) { return false; }
    virtual void Update(float elapsedTimeInS) { UNREFERENCED_PARAMETER(elapsedTimeInS); };
    virtual void Render() = 0;
    virtual void PostRender();
    virtual void HandleStyleIdChanged();

    friend class UIManager;
    friend class UIElementFactoryBase;

    template <typename T>
    friend class UIElementFactory;
};

//! @private
/// This is the base factory class that is used by the UIManager to
/// create intances of UI elements from data.  All UI element classes
/// need to have a corresponding factory class for creating their instances.
class UIElementFactoryBase
{
public:
    virtual ~UIElementFactoryBase() = default;

    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _In_ UIStyleManager& styleManager,
        _Out_ ElementDataProperties&);

protected:
    virtual UIElement* Create(UIManager&, ID, UIDataPtr) = 0;

    friend class UIManager;
};

//! @private
/// Template which has default UI element creation boilerplate logic
/// that many simple UI element factories will use.
template <typename T>
class UIElementFactory : public UIElementFactoryBase
{
protected:
    T* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newElement = new T(manager, id);
        UIElementFactoryBase::DeserializeDataProperties(
            data,
            manager.GetStyleManager(),
            newElement->m_elementDataProperties);
        newElement->m_style = manager.GetStyleManager().GetById(newElement->m_elementDataProperties.styleId);
        return newElement;
    }
};

NAMESPACE_ATG_UITK_END
