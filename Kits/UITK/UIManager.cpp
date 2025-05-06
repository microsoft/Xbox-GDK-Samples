//--------------------------------------------------------------------------------------
// File: UIManager.cpp
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

#include "pch.h"

#include "UIMath.h"
#include "UIManager.h"
#include "UIKeywords.h"
#include "UIElement.h"
#include "UIWidgets.h"

NAMESPACE_ATG_UITK_BEGIN

using ElementAndJson = std::pair<UIElementPtr, UIDataPtr>;

INITIALIZE_CLASS_LOG_DEBUG(UIManager);

/// The internal one and only "screen" UI element that serves as the root
/// of the entire UI hierarchy.
class Screen : public UIElement
{
public:
    Screen(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
    }

    static const ID& ClassID() { static ID id("screen"); return id; }
    const ID& GetClassID() const override { return ClassID(); }

    void Update(float) override {}
    void Render() override {}
    void PostRender() override {}
};

/// <summary>
/// 
/// </summary>
UIManager::UIManager() :
    m_renderWindowSize{ c_referenceResolution[0], c_referenceResolution[1] },
    m_renderScale(1.0f),
    m_styleManager(m_dataDefinitions),
    m_frameCounter(0),
    m_depthOrderedElements(),
    m_updated(false),
    m_orderInvalidated(true)

{
    RegisterInternalElementFactories();
    m_hierarchyRoot = std::make_shared<Screen>(*this, ID("SCREEN"));
    m_currentFocusScopeRootElement = m_hierarchyRoot;
}

/*public:*/

UIElementPtr UIManager::GetElementUnderPixel(int x, int y)
{
    auto depthOrderedElements = GetDepthOrderedElements();

    // NOTE: hit testing will be the exact *opposite* of the order
    // with which we would want to render the elements in.

    for (auto iter = depthOrderedElements.rbegin(); iter != depthOrderedElements.rend(); ++iter)
    {
        const auto& element = *iter;
        if (element->HitTestPixels(x, y))
        {
            return element;
        }
    }

    return nullptr;
}

UIElementPtr UIManager::GetVisibleElementUnderPixel(int x, int y)
{
    auto depthOrderedElements = GetDepthOrderedElements();

    // NOTE: hit testing will be the exact *opposite* of the order
    // with which we would want to render the elements in.

    for (auto iter = depthOrderedElements.rbegin(); iter != depthOrderedElements.rend(); ++iter)
    {
        const auto& element = *iter;
        if (element->IsVisible() && element->HitTestPixels(x, y))
        {
            return element;
        }
    }

    return nullptr;
}

std::vector<UIElementPtr>& UIManager::GetDepthOrderedElements()
{
    // if we are searching based off of a non-common root, then we need
    // to always invalidate
    if (m_orderInvalidated)
    {
        UILOG_INFO_FUNC("Recomputing element order.");
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIManager_GetDepthOrderedElements");
        GetDepthOrderedElements(GetRootElement(), m_depthOrderedElements);
        m_orderInvalidated = false;
    }

    return m_depthOrderedElements;
}

std::vector<UIElementPtr> UIManager::GetDepthOrderedElementsUnderPixel(int x, int y)
{
    auto depthOrderedElements = GetDepthOrderedElements();
    for (int index = int(depthOrderedElements.size() - 1); index >= 0; --index)
    {
        const auto& element = depthOrderedElements[size_t(index)];
        if (!element->HitTestPixels(x, y))
        {
            depthOrderedElements.erase(depthOrderedElements.begin() + index);
        }
    }
    return depthOrderedElements;
}

UIElementPtr UIManager::FindById(ID id) const
{
    auto hashedIter = m_hashedElements.find(id);

    if (hashedIter != m_hashedElements.end())
    {
        return hashedIter->second.lock();
    }

    UILOG_WARN_SCOPED("FindById", "Element not found. %s", id.AsCStr());

    return nullptr;
}

void UIManager::AttachTo(UIElementPtr child, UIElementPtr parent, bool isSubElement)
{
    UILOG_SCOPE("AttachTo");
    if (!child || !parent)
    {
        UILOG_WARN_IF(!child, "Child element is null.");
        UILOG_WARN_IF(!parent, "Parent element is null.");
        return;
    }

    if (isSubElement)
    {
        assert(child->m_parent == nullptr);
        UILOG_TRACE_SCOPED("SubElement", "(Parent) %s -> (Child) %s", parent->GetID().AsCStr(), child->GetID().AsCStr());
        parent->m_subElements.emplace_back(child);
    }
    else
    {
        Detach(child);
        UILOG_TRACE_SCOPED("ChildElement", "(Parent) %s -> (Child) %s", parent->GetID().AsCStr(), child->GetID().AsCStr());
        parent->m_children.emplace_back(child);
    }
    child->m_isSubElement = isSubElement;
    child->m_parent = parent;
    m_orderInvalidated = true;
}

void UIManager::Detach(UIElementPtr child)
{
    if (!child || !child->GetParent())
    {
        UILOG_SCOPE("Detach");
        UILOG_WARN_IF(!child, "Child element is null.");
        return;
    }

    auto parent = child->m_parent;

    if (!child->IsSubElement())
    {
        auto childIndex = parent->GetChildIndex(child);

        if (childIndex != parent->GetChildCount())
        {
            parent->m_children.erase(parent->m_children.begin() + static_cast<long>(childIndex));
        }
    }
    else
    {
        auto subElementIndex = parent->GetSubElementIndex(child);

        if (subElementIndex != parent->GetSubElementCount())
        {
            parent->m_subElements.erase(parent->m_subElements.begin() + static_cast<long>(subElementIndex));
        }
    }

    child->m_parent = nullptr;
    m_orderInvalidated = true;
}

void UIManager::Clear(UIElementPtr element)
{
    // NOTE: make sure that we are not currently in a loop which
    // would break that loop such as:
    // - Updating
    // - Rendering

    Detach(element);
    element->Clear();
}

void UIManager::ClearChildren(UIElementPtr parent)
{
    // NOTE: make sure that we are not currently in a loop which
    // would break that loop such as:
    // - Updating
    // - Rendering

    for (auto& child : parent->m_children)
    {
        child->Clear();
    }
    parent->m_children.clear();
    m_orderInvalidated = true;
}

void UIManager::ClearAllElements()
{
    // NOTE: make sure that we are not currently in a loop which
    // would break that loop such as:
    // - Updating
    // - Rendering

    ClearChildren(m_hierarchyRoot);
    m_hashedElements.clear();
}

UIElementPtr UIManager::LoadLayoutFromData(const ID& contextId, UIDataPtr root)
{
    UILOG_SCOPE("LoadLayoutFromData");
    // load any definitions that might exist in the file

    auto definitions = root->GetObjectValue(UITK_FIELD(definitions));

    if (definitions && definitions->IsObject())
    {
        m_dataDefinitions.LoadDefinitions(definitions);
    }

    // load any styles that might exist in the file

    auto styles = root->GetObjectValue(UITK_FIELD(styles));

    if (styles && styles->IsArray())
    {
        m_dataDefinitions.ReplaceAllDefinitionReferences(styles);

        auto styleCount = styles->GetArrayCount();
        for (size_t styleIndex = 0; styleIndex < styleCount; ++styleIndex)
        {
            auto style = styles->GetArrayValue(styleIndex);
            m_styleManager.LoadStyleFromData(contextId, style);
        }
    }

    // load the layout that *must* exist in the file

    auto layout = root->GetObjectValue(UITK_FIELD(layout));

    assert(layout && layout->IsValid());

    m_dataDefinitions.ReplaceAllDefinitionReferences(layout);

    auto rootElement = MakeElementFromData(contextId, layout);
    RegisterElement(rootElement);

    std::vector<ElementAndJson> elementStack;
    elementStack.emplace_back(ElementAndJson(rootElement, layout));

    while (!elementStack.empty())
    {
        auto& topElement = elementStack.back();

        auto element = topElement.first;
        auto elementJson = topElement.second;

        UILOG_SCOPE(element->GetID().AsStr());

        elementStack.pop_back();

        // deal with sub elements before child elements

        auto subElements = elementJson->GetObjectValue(UITK_FIELD(subElements));
        if (subElements && subElements->IsArray())
        {
            UILOG_SCOPE("AddSubElements");
            for (size_t subElementIndex = 0; subElementIndex < subElements->GetArrayCount(); ++subElementIndex)
            {
                auto subElementData = subElements->GetArrayValue(subElementIndex);
                auto subElement = MakeElementFromData(element->GetID(), subElementData);

                element->AddSubElement(subElement);

                elementStack.emplace_back(ElementAndJson(subElement, subElementData));
            }
        }

        // sub elements are not allowed to have child elements defined within their data

        if (!element->IsSubElement())
        {
            auto childElements = elementJson->GetObjectValue(UITK_FIELD(childElements));
            if (childElements && childElements->IsArray())
            {
                UILOG_SCOPE("AddChildElements");
                for (uint32_t childIndex = 0; childIndex < childElements->GetArrayCount(); ++childIndex)
                {
                    auto childData = childElements->GetArrayValue(childIndex);
                    auto childElement = MakeElementFromData(element->GetID(), childData);

                    RegisterElement(childElement);

                    element->AddChild(childElement);

                    elementStack.emplace_back(ElementAndJson(childElement, childData));
                }
            }
        }
    }

    // we are free now to flatten all loaded styles and
    // return the root layout element

    m_styleManager.FlattenAllStyles();

    // before returning, we grant every loaded UI element the chance
    // to perform some post loading processing

    std::vector<UIElementPtr> loadedElements;
    GetDepthOrderedElements(rootElement, loadedElements);

    for (auto& loadedElement : loadedElements)
    {
        loadedElement->PostLoad();
    }

    return rootElement;
}

void UIManager::InitializeFromLayoutFile(const std::string& layoutFilePath)
{
    GetRootElement()->AddChildFromLayout(layoutFilePath);
}

UIElementPtr UIManager::LoadLayoutFromFile(const std::string& layoutFilePath)
{
    auto contextId = ID(layoutFilePath);
    auto root = std::make_shared<UISerializedObject>(layoutFilePath);

    // handle file "includes" (which are only allowed within layout files
    // and not within nested include files to reduce the changes of
    // circular references and diamond-pattern references, etc.

    auto includes = root->GetObjectValue(UITK_FIELD(includes));

    if (includes && includes->IsArray())
    {
        auto includeRoot = UIDataPtr();
        for (size_t includeIndex = 0; includeIndex < includes->GetArrayCount(); ++includeIndex)
        {
            auto includePath = includes->Get<std::string>(includeIndex);

            // check that we do not have a circular reference (a reference to
            // ourselves in other words)
            assert(!(ID(includePath) == contextId));

            auto include = std::make_shared<UISerializedObject>(includePath);

            // now is a good time to assert that a dependency included data file
            // also *does not* have its own "include" block since that is not 
            // allowed
            assert(!include->Exists(UITK_FIELD(includes)));
            assert(!include->Exists(UITK_FIELD(layout)));

            if (!includeRoot)
            {
                includeRoot = include;
            }
            else
            {
                includeRoot->ApplyPatch(include);
            }
        }

        includeRoot->ApplyPatch(root);
        root.swap(includeRoot);

        UILOG_DEBUG_EXT(64 * 1024, root->Dump());
    }

    return LoadLayoutFromData(contextId, root);
}

UIElementPtr UIManager::InstantiatePrefab(const std::string& prefabFilePath)
{
    ID contextId;

    if (m_dataDefinitions.IsDefinition(prefabFilePath))
    {
        auto definition = m_dataDefinitions.GetDefinition(prefabFilePath);

        if (definition)
        {
            contextId = definition->Get<ID>();
        }
        else
        {
            throw std::exception(("Provided definition for prefab file path is not defined: " + prefabFilePath).c_str());
        }
    }
    else
    {
        contextId = ID(prefabFilePath);
    }

    auto prefab = LoadPrefabDataFromFile(contextId.AsStr());

    // we need to wrap this prefab data into an element instance
    // at this point, the load should have already made sure that all definitions
    // have been replaced and the data is ready for parsing, which we can treat
    // the way we treat a layout

    auto rootElementId = ID::CreateUUID();
    auto rootElementClassId = prefab->Get<ID>(UITK_FIELD(classId));

    auto rootElement = AllocateElement(rootElementClassId, rootElementId, prefab);
    RegisterElement(rootElement);

    std::vector<ElementAndJson> elementStack;
    elementStack.emplace_back(ElementAndJson(rootElement, prefab));

    size_t stackOffset = 0;

    while (stackOffset < elementStack.size())
    {
        auto& stackedElement = elementStack[stackOffset];

        auto element = stackedElement.first;
        auto elementJson = stackedElement.second;

        stackOffset++;

        // deal with only sub elements for prefabs

        auto subElements = elementJson->GetObjectValue(UITK_FIELD(subElements));
        if (subElements && subElements->IsArray())
        {
            for (size_t subElementIndex = 0; subElementIndex < subElements->GetArrayCount(); ++subElementIndex)
            {
                auto subElementData = subElements->GetArrayValue(subElementIndex);
                auto subElement = MakeElementFromData(element->GetID(), subElementData);

                element->AddSubElement(subElement);

                elementStack.emplace_back(ElementAndJson(subElement, subElementData));
            }
        }
    }

    // we are free now to flatten all loaded styles and
    // return the root layout element

    m_styleManager.FlattenAllStyles();

    // before returning, we grant every loaded UI element the chance
    // to perform some post loading processing

    std::vector<UIElementPtr> loadedElements;
    GetDepthOrderedElements(rootElement, loadedElements);

    for (auto& loadedElement : elementStack)
    {
        loadedElement.first->PostLoad();
    }

    return rootElement;
}

float UIManager::GetRefUnitsToPixelsScale() const
{
    return m_renderScale;
}

float UIManager::GetPixelsToRefUnitsScale() const
{
    return 1.0f / m_renderScale;
}

Rectangle UIManager::GetScreenRectInRefUnits() const
{
    if (m_renderScale < FLT_EPSILON)
    {
        return Rectangle();
    }
    else
    {
        auto invRenderScale = 1.0f / m_renderScale;
        return Rectangle(
            0,
            0,
            int(m_renderWindowSize[0] * invRenderScale),
            int(m_renderWindowSize[1] * invRenderScale));
    }
}

void UIManager::Render()
{
    if (!m_styleManager.HasStyleRenderer())
    {
        return;
    }

    ++m_frameCounter;
    UILOG_TRACE_FUNC("Rendering Frame - %d", m_frameCounter);
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIManager_Render");
    m_styleManager.GetStyleRenderer().SetupRender();

    auto globalFontTextScaleIndex = m_styleManager.GetStyleRenderer().PushFontTextScale(m_renderScale);

    std::vector<UIElementPtr>& renderQueue = GetDepthOrderedElements();

    std::vector<UIElementPtr> ancestorStack;

    // NOTE: we traverse forward depth-first with parents being
    // rendered before their descendants.

    UIElementPtr prevElement = nullptr;

    for (const auto& element : renderQueue)
    {
        if (!element->IsVisible())
        {
            continue;
        }

        const auto& parent = element->GetParent();

        // start a new ancestor scope if either we are the root element
        // or the previous element we encountered was our parent

        if (parent && prevElement == parent)
        {
            ancestorStack.emplace_back(prevElement);
        }

        // make sure to perform a PostRender() on the parent element
        // that was in scope for the current element.

        while (ancestorStack.size() > 0 && parent && ancestorStack.back() != parent)
        {
            ancestorStack.back()->PostRender();
            ancestorStack.pop_back();
        }

        // render the currently visible element...

        element->Render();

        // also perform a PostRender() for the element if it is a leaf

        if (parent && element->GetChildCount() == 0 && element->GetSubElementCount() == 0)
        {
            element->PostRender();
        }

        prevElement = element;
    }

    // finally clean up any remaining ancestors in the reverse
    // order of their stacked scopes.

    while (ancestorStack.size() > 0)
    {
        ancestorStack.back()->PostRender();
        ancestorStack.pop_back();
    }

    m_styleManager.GetStyleRenderer().PopFontTextScale(globalFontTextScaleIndex);

    m_styleManager.GetStyleRenderer().FinalizeRender();
}

void UIManager::SetWindowSize(int w, int h)
{
    m_styleManager.GetStyleRenderer().SetWindowSize(w, h);

    auto scaleW = float(w) / float(c_referenceResolution[0]);
    auto scaleH = float(h) / float(c_referenceResolution[1]);

    // choose the scale that is closest to 1.0f
    m_renderScale = fabsf(1.0f - scaleW) < fabsf(1.0f - scaleH) ? scaleW : scaleH;

    m_renderWindowSize[0] = w;
    m_renderWindowSize[1] = h;
}

void UIManager::Update(float elapsedTimeInS, const UIInputState& inputState)
{
    // First, we need to make sure that we allow the elements to reset
    // their event states.

    FrameComputedValues::NextFrame();
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIManager_Update");

    std::vector<UIElementPtr>& updateQueue = GetDepthOrderedElements();

    for (const auto& element : updateQueue)
    {
        if (element->IsEnabled())
        {
            element->ResetEventState();
        }
    }

    // Call into element (or parent) to see if input is handled and should bypass normal input handling
    bool skipHandleInput = HandleGlobalInputState(inputState);
        
    if (!skipHandleInput)
    {
        // Then, we will perform input/event handling before we move on
        // to the element updates.
        HandleMouseInput(inputState, inputState.GetMouseState(), inputState.GetMouseButtons());
        HandleKeyboardInput(inputState, inputState.GetKeyboardState(), inputState.GetKeyboardKeys());
        HandleGamepadInput(inputState, inputState.GetGamePadState(0), inputState.GetGamePadButtons(0));
    }
    
    // refresh the focus scope

    SetFocusScopeRoot(m_currentFocusScopeRootElement);

    // find an element to focus on if we currently do not have one and there is one
    // available to be focused since we should always have a focused element whenever possible
    if (!m_currentFocusedElement || !m_currentFocusedElement->CanBeFocused() || !IsInFocusScope(m_currentFocusedElement))
    {
        MakeFocusElement(FindFocusElement(GetFocusScopeRoot()), inputState);
    }

    // clear out the current hover element if it has become the current focused
    // element since they are treated differently.
    if (m_currentOverElement == m_currentFocusedElement)
    {
        m_currentOverElement = nullptr;
    }

    // if we have a currently focused element, we should dispatch an input state
    // changed to that element on a frame-by-frame basis.
    if (m_currentFocusedElement)
    {
        DispatchInputUpdateEvent(m_currentFocusedElement, inputState);
    }

    // NOTE: we traverse forward depth-first with parents being
    // updated before their descendants.

    for (const auto& element : updateQueue)
    {
        if (element->IsEnabled())
        {
            element->Update(elapsedTimeInS);
        }
    }
}

void UIManager::SetFocusScopeRoot(UIElementPtr focusRoot)
{
    m_currentFocusScopeRootElement = focusRoot && focusRoot->IsAttachedToScene() ? focusRoot : GetRootElement();
}

bool UIManager::IsInFocusScope(UIElementPtr element)
{
    return
        (element && m_currentFocusScopeRootElement) &&
        (element == m_currentFocusScopeRootElement || element->IsAncestorOfMe(m_currentFocusScopeRootElement));
}

void UIManager::SetFocus(UIElementPtr element)
{
    if (element &&
        element->IsFocusable() &&
        element->IsVisible() &&
        element->IsEnabled() &&
        !element->IsFocused() &&
        IsInFocusScope(element))
    {
        MakeFocusElement(element, {});
    }
}

/*private:*/

/*static*/ void UIManager::GetDepthOrderedElements(UIElementPtr root, std::vector<UIElementPtr>& depthOrderedElements)
{
    assert(root != nullptr);

    depthOrderedElements.clear();
    std::vector<UIElementPtr> elementsToProcessStack;
    elementsToProcessStack.emplace_back(root);

    while (elementsToProcessStack.size() > 0)
    {
        auto element = elementsToProcessStack.back();
        depthOrderedElements.emplace_back(element);
        elementsToProcessStack.pop_back();

        for (auto childIter = element->m_subElements.rbegin();
            childIter != element->m_subElements.rend();
            ++childIter)
        {
            elementsToProcessStack.emplace_back(*childIter);
        }

        // take the children in reverse order so that the first
        // child is on the top of the stack

        for (auto childIter = element->m_children.rbegin();
            childIter != element->m_children.rend();
            ++childIter)
        {
            elementsToProcessStack.emplace_back(*childIter);
        }
    }
}

void UIManager::MakeFocusElement(UIElementPtr element, const UIInputState& inputState)
{
    // since the mouse (when clicked) is not over the current focused element, that element
    // is going to get a "LoseFocus" and no longer be the current focused element.
    if (m_currentFocusedElement)
    {
        m_currentFocusedElement->HandleInputEvent(InputEvent{ InputEvent::LoseFocus, inputState });
        m_currentFocusedElement->m_elementDataProperties.focused = false;
        m_currentFocusedElement = nullptr;
    }

    // since the mouse (when clicked) is now over another element that was not focused,
    // that element is going to get a "GainFocus" and will now be the current focused element.
    if (element)
    {
        m_currentFocusedElement = element;
        m_currentFocusedElement->m_elementDataProperties.focused = true;
        m_currentFocusedElement->HandleInputEvent(InputEvent{ InputEvent::GainFocus, inputState });
    }
}

UIElementPtr UIManager::FindFocusElement(UIElementPtr root)
{
    std::function<bool(UIElementPtr)> pred = [](UIElementPtr element) { return element->CanBeFocused(); };

    std::vector<UIElementPtr> depthOrderedElements;
    GetDepthOrderedElements(root, depthOrderedElements);

    auto focusableElement = std::find_if(
        depthOrderedElements.begin(),
        depthOrderedElements.end(),
        pred);
    return focusableElement != depthOrderedElements.end() ? *focusableElement : nullptr;
}

UIElementPtr UIManager::GetNextFocusableElement(
    std::function<bool(UIElementPtr)> filterMethod,
    std::function<int(UIElementPtr)> scoreMethod)
{
    assert(m_currentFocusedElement);
    assert(m_currentFocusedElement->GetParent());
    assert(filterMethod);
    assert(scoreMethod);

    // first we eliminate all of the elements that definitely should not even
    // be considered

    std::vector<UIElementPtr> candidates;
    GetDepthOrderedElements(m_currentFocusScopeRootElement, candidates);

    int index = int(candidates.size()) - 1;
    while (index >= 0)
    {
        const auto& candidate = candidates[size_t(index)];
        if (candidate == m_currentFocusedElement || !candidate->CanBeFocused())
        {
            candidates.erase(candidates.begin() + index);
        }
        index--;
    }

    // next, we will filter out the remaining elements based on the provided
    // filter function.

    index = int(candidates.size()) - 1;
    while (index >= 0)
    {
        const auto& candidate = candidates[size_t(index)];
        if (!filterMethod(candidate))
        {
            candidates.erase(candidates.begin() + index);
        }
        index--;
    }

    // finally we will score each element that survives and keep track of the
    // one that is deemed the "best" by having the lowest score.

    std::pair<UIElementPtr, int> bestCandidate;
    index = int(candidates.size()) - 1;
    while (index >= 0)
    {
        const auto& candidate = candidates[size_t(index)];
        auto candidateScore = scoreMethod(candidate);
        if (!bestCandidate.first || bestCandidate.second > candidateScore)
        {
            bestCandidate.first = candidate;
            bestCandidate.second = candidateScore;

        }
        index--;
    }

    return bestCandidate.first;
}

UIElementPtr UIManager::GetLeftFocusableElement()
{
    if (!m_currentFocusedElement)
    {
        return nullptr;
    }
    UIMath::RectangleExt focusedRect = m_currentFocusedElement->GetScreenRectInPixels();
    auto leftTop = focusedRect.GetLeftTop();
    auto leftBottom = focusedRect.GetLeftBottom();
    auto leftEdgeCtr = focusedRect.GetLeftEdgeCenter();
    return GetNextFocusableElement(
        [leftTop, leftBottom](UIElementPtr candidate)
    {
        // what this chunk of code is doing is making sure that the right
        // edge of the candidate rect lies to the left of the focused edge
        // and also that it is within the diagonal quadrant formed by
        // the focused left growing leftward
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateRightTop = candidateRect.GetRightTop();
        auto candidateRightBottom = candidateRect.GetRightBottom();
        auto deltaX = leftTop.x - candidateRightTop.x;
        if (deltaX < 0)
        {
            return false;
        }
        auto deltaTopY = leftTop.y - candidateRightBottom.y;
        auto deltaBottomY = candidateRightTop.y - leftBottom.y;
        if (deltaTopY > deltaX || deltaBottomY > deltaX)
        {
            return false;
        }
        return true;
    },
        [leftEdgeCtr](UIElementPtr candidate)
    {
        // we just use the squared distance from the focused edge to
        // the candidate edge as the score
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateCtr = candidateRect.GetCenter();
        auto deltaX = candidateCtr.x - leftEdgeCtr.x;
        auto deltaY = candidateCtr.y - leftEdgeCtr.y;
        auto score = (deltaX * deltaX) + (deltaY * deltaY);
        return score;
    });
}

UIElementPtr UIManager::GetRightFocusableElement()
{
    if (!m_currentFocusedElement)
    {
        return nullptr;
    }
    UIMath::RectangleExt focusedRect = m_currentFocusedElement->GetScreenRectInPixels();
    auto rightTop = focusedRect.GetRightTop();
    auto rightBottom = focusedRect.GetRightBottom();
    auto rightEdgeCtr = focusedRect.GetRightEdgeCenter();
    return GetNextFocusableElement(
        [rightTop, rightBottom](UIElementPtr candidate)
    {
        // what this chunk of code is doing is making sure that the left
        // edge of the candidate rect lies to the right of the focused edge
        // and also that it is within the diagonal quadrant formed by
        // the focused right growing rightward
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateLeftTop = candidateRect.GetLeftTop();
        auto candidateLeftBottom = candidateRect.GetLeftBottom();
        auto deltaX = candidateLeftTop.x - rightTop.x;
        if (deltaX < 0)
        {
            return false;
        }
        auto deltaTopY = rightTop.y - candidateLeftBottom.y;
        auto deltaBottomY = candidateLeftTop.y - rightBottom.y;
        if (deltaTopY > deltaX || deltaBottomY > deltaX)
        {
            return false;
        }
        return true;
    },
        [rightEdgeCtr](UIElementPtr candidate)
    {
        // we just use the squared distance from the focused edge to
        // the candidate edge as the score
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateCtr = candidateRect.GetCenter();
        auto deltaX = candidateCtr.x - rightEdgeCtr.x;
        auto deltaY = candidateCtr.y - rightEdgeCtr.y;
        auto score = (deltaX * deltaX) + (deltaY * deltaY);
        return score;
    });
}

UIElementPtr UIManager::GetUpFocusableElement()
{
    if (!m_currentFocusedElement)
    {
        return nullptr;
    }
    UIMath::RectangleExt focusedRect = m_currentFocusedElement->GetScreenRectInPixels();
    auto leftTop = focusedRect.GetLeftTop();
    auto rightTop = focusedRect.GetRightTop();
    auto topEdgeCtr = focusedRect.GetTopEdgeCenter();
    return GetNextFocusableElement(
        [leftTop, rightTop](UIElementPtr candidate)
    {
        // what this chunk of code is doing is making sure that the bottom
        // edge of the candidate rect lies to the top of the focused edge
        // and also that it is within the diagonal quadrant formed by
        // the focused top growing upward
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateLeftBottom = candidateRect.GetLeftBottom();
        auto candidateRightBottom = candidateRect.GetRightBottom();
        auto deltaY = leftTop.y - candidateLeftBottom.y;
        if (deltaY < 0)
        {
            return false;
        }
        auto deltaLeftX = leftTop.x - candidateRightBottom.x;
        auto deltaRightX = candidateLeftBottom.x - rightTop.x;
        if (deltaLeftX > deltaY || deltaRightX > deltaY)
        {
            return false;
        }
        return true;
    },
        [topEdgeCtr](UIElementPtr candidate)
    {
        // we just use the squared distance from the focused edge to
        // the candidate edge as the score
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateCtr = candidateRect.GetCenter();
        auto deltaX = candidateCtr.x - topEdgeCtr.x;
        auto deltaY = candidateCtr.y - topEdgeCtr.y;
        auto score = (deltaX * deltaX) + (deltaY * deltaY);
        return score;
    });
}

UIElementPtr UIManager::GetDownFocusableElement()
{
    if (!m_currentFocusedElement)
    {
        return nullptr;
    }
    UIMath::RectangleExt focusedRect = m_currentFocusedElement->GetScreenRectInPixels();
    auto leftBottom = focusedRect.GetLeftBottom();
    auto rightBottom = focusedRect.GetRightBottom();
    auto bottomEdgeCtr = focusedRect.GetBottomEdgeCenter();
    return GetNextFocusableElement(
        [leftBottom, rightBottom](UIElementPtr candidate)
    {
        // what this chunk of code is doing is making sure that the top
        // edge of the candidate rect lies to the bottom of the focused edge
        // and also that it is within the diagonal quadrant formed by
        // the focused bottom growing downward
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateLeftTop = candidateRect.GetLeftTop();
        auto candidateRightTop = candidateRect.GetRightTop();
        auto deltaY = candidateLeftTop.y - leftBottom.y;
        if (deltaY < 0)
        {
            return false;
        }
        auto deltaLeftX = leftBottom.x - candidateRightTop.x;
        auto deltaRightX = candidateLeftTop.x - rightBottom.x;
        if (deltaLeftX > deltaY || deltaRightX > deltaY)
        {
            return false;
        }
        return true;
    },
        [bottomEdgeCtr](UIElementPtr candidate)
    {
        // we just use the squared distance from the focused edge to
        // the candidate edge as the score
        UIMath::RectangleExt candidateRect = candidate->GetScreenRectInPixels();
        auto candidateCtr = candidateRect.GetCenter();
        auto deltaX = candidateCtr.x - bottomEdgeCtr.x;
        auto deltaY = candidateCtr.y - bottomEdgeCtr.y;
        auto score = (deltaX * deltaX) + (deltaY * deltaY);
        return score;
    });
}

UIDataPtr UIManager::LoadPrefabDataFromFile(const std::string& prefabFilePath)
{
    auto contextId = ID(prefabFilePath);
    auto root = std::make_shared<UISerializedObject>(prefabFilePath);

    // load any definitions that might exist in the file

    auto definitions = root->GetObjectValue(UITK_FIELD(definitions));

    if (definitions && definitions->IsObject())
    {
        // NOTE: only load the definition if it is not already present
        m_dataDefinitions.LoadDefinitions(definitions, false);
    }

    // load any styles that might exist in the file

    auto styles = root->GetObjectValue(UITK_FIELD(styles));

    if (styles && styles->IsArray())
    {
        m_dataDefinitions.ReplaceAllDefinitionReferences(styles);

        auto styleCount = styles->GetArrayCount();
        for (size_t styleIndex = 0; styleIndex < styleCount; ++styleIndex)
        {
            auto style = styles->GetArrayValue(styleIndex);
            auto styleId = style->GetIfExists(UITK_FIELD(id), ID::Default);
            // NOTE: only load the style if it is not already present
            if (styleId && !m_styleManager.GetById(styleId))
            {
                m_styleManager.LoadStyleFromData(contextId, style);
            }
        }
    }

    // load the layout that *must* exist in the file

    auto prefab = root->GetObjectValue(UITK_FIELD(prefab));

    assert(prefab && prefab->IsValid());

    m_dataDefinitions.ReplaceAllDefinitionReferences(prefab);

    // we need to assert that the prefab root element does not contain any child
    // elements since that is not allowed

    assert(!prefab->Exists(UITK_FIELD(childElements)));

    // a prefab must *always* have a class id

    if (!prefab->Exists(UITK_FIELD(classId))) {
        throw UIException(contextId, "the JSON file prefab node is missing a required 'classId' property.");
    }

    return prefab;
}

void UIManager::RegisterInternalElementFactories()
{
    RegisterElementFactory<UIPanelFactory>(UIPanel::ClassID());
    RegisterElementFactory<UIStaticTextFactory>(UIStaticText::ClassID());
    RegisterElementFactory<UIImageFactory>(UIImage::ClassID());
    RegisterElementFactory<UIButtonFactory>(UIButton::ClassID());
    RegisterElementFactory<UIProgressBarFactory>(UIProgressBar::ClassID());
    RegisterElementFactory<UITwistMenuFactory>(UITwistMenu::ClassID());
    RegisterElementFactory<UISliderFactory>(UISlider::ClassID());
    RegisterElementFactory<UIConsoleWindowFactory>(UIConsoleWindow::ClassID());
    RegisterElementFactory<UICheckBoxFactory>(UICheckBox::ClassID());
    RegisterElementFactory<UIPipStripFactory>(UIPipStrip::ClassID());
    RegisterElementFactory<UIVerticalStackFactory>(UIVerticalStack::ClassID());
    RegisterElementFactory<UIStackPanelFactory>(UIStackPanel::VerticalStackPanelClassID());
    RegisterElementFactory<UIStackPanelFactory>(UIStackPanel::HorizontalStackPanelClassID());
    RegisterElementFactory<UIDebugPanelFactory>(UIDebugPanel::ClassID());
}

UIElementPtr UIManager::MakeElementFromData(const ID& context, UIDataPtr& data)
{
    if (!data->IsObject() || !data->Exists(UITK_FIELD(id)))
    {
        throw UIException(context, "incorrect and/or malformed JSON file being loaded (needs to be an object with an 'id' property.");
    }

    // merge in any prefab data first (element data will trump prefab data)

    ID prefabReference = data->GetIfExists(UITK_FIELD(prefabRef), ID::Default);
    if (prefabReference)
    {
        auto prefabData = LoadPrefabDataFromFile(prefabReference.AsStr());
        m_dataDefinitions.ReplaceAllDefinitionReferences(prefabData);
        prefabData->ApplyPatch(data);
        data.swap(prefabData);
    }

    if (!data->Exists(UITK_FIELD(classId)))
    {
        throw UIException(context, "the JSON file element node is missing a required 'classId' property.");
    }

    // now we can proceed with normal serialization
    std::string classId = data->Get<std::string>(UITK_FIELD(classId));

    auto rootElementClassId = ID(classId);
    auto rootElementId = data->Get<ID>(UITK_FIELD(id));

    /*
    This block of code implements the differences between element schema v1 and v2.
    In element schema v1, the element-type fields are siblings of the core fields (e.g. id, classId, etc.)
    In element schema v2, the element-type fields are in a child object named after the type of element (e.g. panel, checkBox, etc.)
    For backwards compatibility, we support both formats.
    To keep the parsing logic consistent between v1 and v2, we promote the element-type fields to be siblings of the core fields as they are in v1.
     */
    classId[0] = char(std::tolower(classId[0])); // force initial word is camelCase
    if (data->Exists(classId))
    {
        data->ApplyPatch(data->GetObjectValue(classId));
    }
    else
    {
        for (auto& key : data->GetKeys())
        {
            if (ID(key) == rootElementClassId)
            {
                UILOG_WARN("Key { %s } matches the classId { %s } but is not camelCased and did not match.", key.c_str(), rootElementClassId.AsCStr());
                data->ApplyPatch(data->GetObjectValue(key));
#if UI_STRICT
                throw std::exception("Field did not use proper camelCasing!");
#endif
                break;
            }
        }
    }

    return AllocateElement(rootElementClassId, rootElementId, data);
}

UIElementPtr UIManager::AllocateElement(const ID& elementClassId, const ID& elementId, UIDataPtr json)
{
    auto iterator = m_elementFactories.find(elementClassId);

    if (iterator == m_elementFactories.end())
    {
        throw UIException(elementClassId, "No element class name registered to allocate: ");
    }

    auto newElement = std::shared_ptr<UIElement>(
        iterator->second->Create(*this, elementId, json));

    return newElement;
}

void UIManager::RegisterElement(UIElementPtr element)
{
    assert("An element with a duplicate id has been detected." &&
        Util::Contains(m_hashedElements, element->GetID()) == false);
    m_hashedElements[element->GetID()] = element;
}

bool UIManager::DispatchInputUpdateEvent(UIElementPtr recipient, const UIInputState& inputState)
{
    // let the parent have a crack at the event first.
    auto parent = recipient->GetParent();
    if (parent)
    {
        // if it so happens that the parent has handled the event, then we stop
        // passing the event through.
        if (DispatchInputUpdateEvent(parent, inputState))
        {
            return true;
        }
    }

    // since the recipient's parent did not handle it, that element should receive
    // input state changes.
    InputEvent inputEvent{ InputEvent::InputStateChange, inputState };
    return recipient->HandleInputEvent(inputEvent);
}

// note: mouse input is treated as the lowest priority input and is to be handled
// first by the manager
void UIManager::HandleMouseInput(
    const UIInputState& inputState,
    const Mouse::State& mouseState,
    const UIInputState::MouseButtonStates& mouseButtons)
{
    // determine the previous and current focusable elements under the mouse pixel

    m_previousFocusableElementUnderPixel = m_currentFocusableElementUnderPixel;

    auto elementsUnderPixel = GetDepthOrderedElementsUnderPixel(mouseState.x, mouseState.y);
    m_currentFocusableElementUnderPixel = UIElementPtr();

    for (auto iter = elementsUnderPixel.rbegin(); iter != elementsUnderPixel.rend(); ++iter)
    {
        const auto& element = *iter;
        if (element->CanBeFocused())
        {
            m_currentFocusableElementUnderPixel = element;
            break;
        }
    }

    // perform mouse hover and focus related states for elements

    if (IsMouseStillOverCurrentHoveredElement())
    {
        // since the mouse remains over the current hover (non-focused) element, then the
        // current hover element gets a "MouseOver" until no longer true.
        m_currentOverElement->HandleInputEvent(InputEvent{ InputEvent::MouseOver, inputState });
    }
    else if (IsMouseNoLongerOverCurrentHoveredElement())
    {
        // since the mouse is no longer over the current hover (non-focused) element,
        // then the current hover gets a "MouseOut" and is no longer the current hover element.
        m_currentOverElement->HandleInputEvent(InputEvent{ InputEvent::MouseOut, inputState });
        m_currentOverElement = nullptr;
    }

    if (IsMouseStillOverCurrentFocusedElement())
    {
        // since the mouse remains over the current focused element, then the current
        // focused element gets a "MouseOverFocus" until no longer true.
        m_currentFocusedElement->HandleInputEvent(InputEvent{ InputEvent::MouseOverFocus, inputState });
    }
    else if (IsMouseNoLongerOverCurrentFocusedElement())
    {
        // since the mouse is no longer over the current focused element,
        // then the current focus gets a "MouseOutFocus" but remains the current focused element.
        m_currentFocusedElement->HandleInputEvent(InputEvent{ InputEvent::MouseOutFocus, inputState });
    }
    else if (IsMouseNowOverCurrentFocusedElement())
    {
        // since the mouse suddenly is over the current focused element, then the current
        // focused element gets a "MouseInFocus" and remains the current focused element.
        m_currentFocusedElement->HandleInputEvent(InputEvent{ InputEvent::MouseInFocus, inputState });
    }

    if (m_currentFocusableElementUnderPixel && !IsMouseOverCurrentFocusedElement() && !IsMouseOverCurrentHoveredElement())
    {
        // since the mouse suddenly is over a non-focused element, then the element
        // gets a "MouseIn" and becomes the current hover element.
        m_currentOverElement = m_currentFocusableElementUnderPixel;
        m_currentOverElement->HandleInputEvent(InputEvent{ InputEvent::MouseIn, inputState });
    }

    // whatever is clicked on is either already currently focused or it is not
    // and if we clicked on something focusable, then we should switch to it as the currently
    // focused element.

    auto mainButtonPressed = mouseButtons.leftButton == Mouse::ButtonStateTracker::PRESSED;

    if (mainButtonPressed)
    {
        if (!IsMouseOverCurrentFocusedElement() && m_currentFocusableElementUnderPixel)
        {
            MakeFocusElement(m_currentFocusableElementUnderPixel, inputState);
        }
    }
}

// note: keyboard input is treated as the next lowest priority input and is to be
// handled after mouse input by the manager
void UIManager::HandleKeyboardInput(
    const UIInputState& inputState,
    const Keyboard::State& /*keyboardState*/,
    const UIInputState::KeyboardKeyStates& keyboardKeys)
{
    // what we will do here is use the arrows keys to navigate the currently
    // focused element by virtual of the "GetNextFocusableElement()"
    // method.

    // the strategy we will use is to find the element that is the best next
    // element in terms of direction and proximity.  the filter method will be
    // to restrict the focusable elements to those that are in the same direction
    // as the arrow, and then score by the distance to the current focus.

    if (keyboardKeys.IsKeyPressed(DirectX::Keyboard::Keys::Left))
    {
        auto leftElement = GetLeftFocusableElement();
        if (leftElement)
        {
            MakeFocusElement(leftElement, inputState);
        }
    }
    else if (keyboardKeys.IsKeyPressed(DirectX::Keyboard::Keys::Right))
    {
        auto rightElement = GetRightFocusableElement();
        if (rightElement)
        {
            MakeFocusElement(rightElement, inputState);
        }
    }
    else if (keyboardKeys.IsKeyPressed(DirectX::Keyboard::Keys::Up))
    {
        auto upElement = GetUpFocusableElement();
        if (upElement)
        {
            MakeFocusElement(upElement, inputState);
        }
    }
    else if (keyboardKeys.IsKeyPressed(DirectX::Keyboard::Keys::Down))
    {
        auto downElement = GetDownFocusableElement();
        if (downElement)
        {
            MakeFocusElement(downElement, inputState);
        }
    }
}

// note: gamepad input is treaded as the highest priority input and is to be handled
// last by the manager
void UIManager::HandleGamepadInput(
    const UIInputState& inputState,
    const GamePad::State& /*gamePadState*/,
    const UIInputState::GamePadButtonStates& /*gamePadButtons*/)
{
    // what we will do here is use the d-pad buttons to navigate the currently
    // focused element by virtual of the "GetNextFocusableElement()"
    // method.

    // the strategy we will use is to find the element that is the best next
    // element in terms of direction and proximity.  the filter method will be
    // to restrict the focusable elements to those that are in the same direction
    // as the d-pad button, and then score by the distance to the current focus.

    auto leftPressed = inputState.AnyDPLIsState(GamePad::ButtonStateTracker::PRESSED);
    auto rightPressed = inputState.AnyDPRIsState(GamePad::ButtonStateTracker::PRESSED);
    auto upPressed = inputState.AnyDPUIsState(GamePad::ButtonStateTracker::PRESSED);
    auto downPressed = inputState.AnyDPDIsState(GamePad::ButtonStateTracker::PRESSED);

    if (leftPressed)
    {
        auto leftElement = GetLeftFocusableElement();
        if (leftElement)
        {
            MakeFocusElement(leftElement, inputState);
        }
    }
    else if (rightPressed)
    {
        auto rightElement = GetRightFocusableElement();
        if (rightElement)
        {
            MakeFocusElement(rightElement, inputState);
        }
    }
    else if (upPressed)
    {
        auto upElement = GetUpFocusableElement();
        if (upElement)
        {
            MakeFocusElement(upElement, inputState);
        }
    }
    else if (downPressed)
    {
        auto downElement = GetDownFocusableElement();
        if (downElement)
        {
            MakeFocusElement(downElement, inputState);
        }
    }
}

bool UIManager::HandleGlobalInputState(const UIInputState& inputState)
{
    // Some widgets, such as StackPanel and SliderElement have elements that
    // should accept input to manipulate element state rather than to
    // immediately seek out the next item to focus.
    // For example, the StackPanel arrow or dpad could be used to scroll to
    // offscreen items rather than to move to the next focus element;
    // A slider's control should be moved if it's aligned in the same direction
    // as the input and only move onto next focus element when the edges are
    // reached.

    // When this happens, inputLocallyConsumed is true and this skips the
    // HandleKeyboard/Mouse/GamepadInput normally performed in Update().

    // The recursive check for parent is because typically it's a parent that has
    // to act on input, e.g. the slider Button is focused but input on it needs to
    // make its parent SliderElement move the actual button; or the StackPanel items
    // are the ones focused during navigation, but it's their parent StackPanel that
    // needs to do the shifting around, hiding and unhiding elements as needed.

    bool inputConsumed = false;
    bool checkingOverElement = false;
    auto currentElement = m_currentFocusedElement;

    while (!inputConsumed && currentElement)
    {
        inputConsumed = currentElement->HandleGlobalInputState(inputState);

        if (!inputConsumed)
        {
            currentElement = currentElement->GetParent();

            if (!currentElement && !checkingOverElement)
            {
                // No parent for focused element, now check hover element;
                // Once hover element parent is checked, we are done
                currentElement = m_currentOverElement;
                checkingOverElement = true;
            }
        }
    }

    return inputConsumed;
}

NAMESPACE_ATG_UITK_END
