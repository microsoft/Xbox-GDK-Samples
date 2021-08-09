//--------------------------------------------------------------------------------------
// File: UIManager.h
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

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <type_traits>

#include "SimpleMath.h"

#include "UIInputState.h"
#include "UIStyleManager.h"
#include "UILog.h"

NAMESPACE_ATG_UITK_BEGIN

class UIManager;
class UIElement;
class UIElementFactoryBase;

using namespace DirectX::SimpleMath;

using UIElementPtr = std::shared_ptr<UIElement>;
using UIElementFactoryPtr = std::unique_ptr<UIElementFactoryBase>;

template <typename T>
std::shared_ptr<T> CastPtr(UIElementPtr elementPtr)
{
    return std::dynamic_pointer_cast<T>(elementPtr);
}

#define UI_ELEMENT_CLASS_INIT(CLAZZ, CLAZZID) \
        public: \
            static const ID& ClassID() { static ID id(#CLAZZID); return id; } \
            const ID& GetClassID() const override { return ClassID(); } \
        protected: \
            friend class UIManager; \
            friend class UIElementFactory<CLAZZ>; \
            friend class CLAZZ ## Factory; 

/// The singleton manager which manages a hierarchical "scene" of UI elements.  The
/// manager is responsible for managing the elements, handling events, handling
/// updating and rendering, handling serialization, and other high level UI related
/// functions like registering element classes.
class UIManager
{
    DECLARE_CLASS_LOG();

public:
    UIManager();

public:
    template <typename T>
    void RegisterElementFactory(const ID& elementClassId)
    {
        UILOG_SCOPE("RegisterElementFactory");
        if (m_elementFactories.find(elementClassId) != m_elementFactories.end())
        {
            throw UIException(elementClassId, "Duplicate element class name registered.");
        }

        m_elementFactories.emplace(
            elementClassId,
            std::make_unique<T>());

        UILOG_DEBUG("Element factory registered. %s", elementClassId.AsCStr());
    }

    UIStyleManager& GetStyleManager()
    {
        return m_styleManager;
    }

    UIElementPtr GetRootElement() const { return m_hierarchyRoot; }
    UIElementPtr GetElementUnderPixel(int x, int y);
    UIElementPtr GetVisibleElementUnderPixel(int x, int y);
    std::vector<UIElementPtr>& GetDepthOrderedElements();
    std::vector<UIElementPtr> GetDepthOrderedElementsUnderPixel(int x, int y);

    UIElementPtr FindById(ID id) const;

    template <typename T>
    std::shared_ptr<T> FindTypedById(ID id) const
    {
        return CastPtr<T>(FindById(id));
    }

    void AttachTo(UIElementPtr child, UIElementPtr parent, bool isSubElement = false);
    void Detach(UIElementPtr child);
    void Clear(UIElementPtr element);
    void ClearChildren(UIElementPtr parent);
    void ClearAllElements();

    template<typename T>
    std::shared_ptr<T> CreateDefaultElement(const ID& id, bool registerIt = false)
    {
        auto newElement = std::shared_ptr<T>(new T(*this, id));
        if (registerIt)
        {
            RegisterElement(newElement);
        }
        return newElement;
    }

    template<typename T>
    std::shared_ptr<T> CreateDefaultElementWithStyle(const ID& id, UIStylePtr style, bool registerIt = false)
    {
        auto element = CreateDefaultElement<T>(id, registerIt);
        element->m_style = style;
        return element;
    }

    UIElementPtr LoadLayoutFromData(const ID& contextId, UIDataPtr root);

    // Initializes the root UI node graph using the provided layout file
    void InitializeFromLayoutFile(const std::string& layoutFilePath);
    // Creates an element containing the element, but does not modify the node graph
    UIElementPtr LoadLayoutFromFile(const std::string& layoutFilePath);
    // Instantiate a prefab from a file, but does not modify the node graph
    UIElementPtr InstantiatePrefab(const std::string& prefabFilePath);

    float GetRefUnitsToPixelsScale() const;
    float GetPixelsToRefUnitsScale() const;
    DirectX::SimpleMath::Rectangle GetScreenRectInRefUnits() const;

    void Render();
    void SetWindowSize(int w, int h);
    void Update(float elapsedTimeInS, const UIInputState& inputState);

    uint32_t GetFrameCounter() { return m_frameCounter; }
    void GetReferenceResolution(int* result)
    {
        result[0] = c_referenceResolution[0];
        result[1] = c_referenceResolution[1];
    }

    UIElementPtr GetFocusScopeRoot()
    {
        return m_currentFocusScopeRootElement;
    }

    void SetRotation(UIRotation rotation)
    {
        m_styleManager.SetRotation(rotation);
    }

    void SetFocusScopeRoot(UIElementPtr focusRoot);
    bool IsInFocusScope(UIElementPtr element);
    void SetFocus(UIElementPtr element);

    /// what this method does is -- from the perspective of the currently
    /// focused element -- decide what a "good" next focused element should
    /// be.  it starts with the set of all "CanBeFocused()" elements that are
    /// under the same parent as the currently focused one, and from there allows
    /// the caller to filter out some of them, and then score the rest to
    /// determine the "winner" with the best one being the one with the score
    /// that is lowest compared to all of the others.
    UIElementPtr GetNextFocusableElement(
        std::function<bool(UIElementPtr)> filterMethod,
        std::function<int(UIElementPtr)> scoreMethod);
    UIElementPtr GetLeftFocusableElement();
    UIElementPtr GetRightFocusableElement();
    UIElementPtr GetUpFocusableElement();
    UIElementPtr GetDownFocusableElement();

private:
    using UIElementFactoryLookup = std::map<ID, UIElementFactoryPtr>;
    using UIElementLookup = std::map<ID, std::weak_ptr<UIElement>>;
    using UIPrefabLookup = std::map<ID, UIDataPtr>;

private:
    int                                 m_renderWindowSize[2];
    float                               m_renderScale;
    UIStyleManager                      m_styleManager;

    UIElementFactoryLookup              m_elementFactories;
    UIElementLookup                     m_hashedElements;
    UIPrefabLookup                      m_prefabCache;

    UIElementPtr m_hierarchyRoot;

    UIDataDefinitions                   m_dataDefinitions;

    UIElementPtr m_previousFocusableElementUnderPixel;
    UIElementPtr m_currentFocusableElementUnderPixel;
    UIElementPtr m_currentFocusScopeRootElement;

    // NOTE: this is the current mouse over element that is NOT focused
    UIElementPtr m_currentOverElement;
    // NOTE: this is the current focused element regardless of what the mouse is over
    UIElementPtr m_currentFocusedElement;

    static constexpr int                c_referenceResolution[2] = { 1920, 1080 };

    uint32_t                            m_frameCounter;
    std::vector<UIElementPtr>           m_depthOrderedElements;
    bool                                m_updated;
    bool                                m_orderInvalidated;

private:
    static void GetDepthOrderedElements(UIElementPtr root, std::vector<UIElementPtr>& orderedElements);
    void MakeFocusElement(UIElementPtr element, const UIInputState& inputState);
    UIElementPtr FindFocusElement(UIElementPtr root);

    UIDataPtr LoadPrefabDataFromFile(const std::string& prefabFilePath);

    void RegisterInternalElementFactories();
    UIElementPtr MakeElementFromData(const ID& context, UIDataPtr& data);
    UIElementPtr AllocateElement(const ID& elementClassId, const ID& elementId, UIDataPtr data);

    void RegisterElement(UIElementPtr element);

    // note: the return value here is whether or not the event was handled by
    // someone between the "recipient" and the root up the hierarchy chain.
    bool DispatchInputUpdateEvent(UIElementPtr recipient, const UIInputState& inputState);

    // note: mouse input is treated as the lowest priority input and is to be handled
    // first by the manager
    /// uses the mouse state and tracking information to determine focused UI element
    /// changes, and hover states.
    void HandleMouseInput(
        const UIInputState& inputState,
        const Mouse::State& mouseState,
        const UIInputState::MouseButtonStates& mouseButtons);

    // hovered element related helper methods

    bool IsMouseOverCurrentHoveredElement() const
    {
        return m_currentOverElement && m_currentOverElement == m_currentFocusableElementUnderPixel;
    }

    bool IsMouseStillOverCurrentHoveredElement() const
    {
        return
            m_currentOverElement &&
            m_currentOverElement == m_currentFocusableElementUnderPixel &&
            m_currentOverElement == m_previousFocusableElementUnderPixel;
    }

    bool IsMouseNoLongerOverCurrentHoveredElement() const
    {
        return
            m_currentOverElement &&
            m_currentOverElement != m_currentFocusableElementUnderPixel &&
            m_currentOverElement == m_previousFocusableElementUnderPixel;
    }

    // focused element related helper methods

    bool IsMouseOverCurrentFocusedElement() const
    {
        return m_currentFocusedElement && m_currentFocusedElement == m_currentFocusableElementUnderPixel;
    }

    bool IsMouseNotOverCurrentFocusedElement() const
    {
        return m_currentFocusedElement && m_currentFocusedElement != m_currentFocusableElementUnderPixel;
    }

    bool IsMouseStillOverCurrentFocusedElement() const
    {
        return
            m_currentFocusedElement &&
            m_currentFocusedElement == m_currentFocusableElementUnderPixel &&
            m_currentFocusedElement == m_previousFocusableElementUnderPixel;
    }

    bool IsMouseNoLongerOverCurrentFocusedElement() const
    {
        return
            m_currentFocusedElement &&
            m_currentFocusedElement != m_currentFocusableElementUnderPixel &&
            m_currentFocusedElement == m_previousFocusableElementUnderPixel;
    }

    bool IsMouseNowOverCurrentFocusedElement() const
    {
        return
            m_currentFocusedElement &&
            m_currentFocusedElement == m_currentFocusableElementUnderPixel &&
            m_currentFocusedElement != m_previousFocusableElementUnderPixel;
    }

    // note: keyboard input is treated as the next lowest priority input and is to be
    // handled after mouse input by the manager
    /// uses the keyboard state and tracking information to determine focused UI element
    /// changes.
    void HandleKeyboardInput(
        const UIInputState& inputState,
        const Keyboard::State& keyboardState,
        const UIInputState::KeyboardKeyStates& keyboardKeys);

    // note: gamepad input is treaded as the highest priority input and is to be handled
    // last by the manager
    /// uses the gamepad state and tracking information to determine focused UI element
    /// changes.
    void HandleGamepadInput(
        const UIInputState& inputState,
        const GamePad::State& gamePadState,
        const UIInputState::GamePadButtonStates& gamePadButtons);

    bool HandleGlobalInputState(const UIInputState&);

};

NAMESPACE_ATG_UITK_END
