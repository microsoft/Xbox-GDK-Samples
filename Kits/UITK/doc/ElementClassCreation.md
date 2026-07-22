# How to create a custom UITK element class

For this how-to guide, we will use the ```UITwistMenu``` class as an example for each of the steps involved in setting up your own UI class.

## Step 1: Declare your new UI class

Note: The "built-in" UI element class declarations can be found within the **UITK** (located in **_gx_dev/Kits/UITK_**) in the source file ```UIElements.h```.  You can place your element in whatever header file you choose, so long as the file ```UIManager.h``` is included before your class declaration.

The outer namespace used for "built-in" UI framework classes is ```ATG::UITK```, and so any additions make directly to the **UITK** must adhere to this namespace convention.  Classes declared within your own project can be within whatever namespace you choose.

Shown below is an example UI class declaration (found in [gx_dev/Kits/UITK/UIElements.h](../UIElements.h)) skeleton:

```cpp
class UITwistMenu : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UITwistMenu, TwistMenu)

public:
    virtual ~UITwistMenu() = default;

    // add your type-specific public APIs here...

protected:
    struct TwistMenuDataProperties
    {
        // add your data properties and defaults here...
    }

    TwistMenuDataProperties m_twistMenuDataProperties;

    // add your internal member variables here...

protected:
    UITwistMenu(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
        // add your construction logic here...
    }

    virtual bool HandleInputEvent(const InputEvent& inputEvent) override;
    virtual void Update(float elapsedTimeInS) override;
    virtual void Render() override;
};
```

Notice the **TwistMenu** as the second argument to the ```UI_ELEMENT_CLASS_INIT``` macro.  That represents the actual class name string that will be recognized in data.

The new UI class declares (through the ```UI_ELEMENT_CLASS_INIT``` macro) the ```UIManager```, the base **UIElementFactory<_NewType_>**, and the derived **_NewTypeFactory_** as friends since the managed methods remain protected, but need to be called by those framework classes.

## Step 2: Declare your new UI class data properties

Remember that structure declaration stub placed within a ```protected:``` section in the ```UITwistMenu``` class named ```TwistMenuDataProperties```?  This is the step where you would fill that out.  If you do not know what data properties your class will utilize, now might be a good time to figure that out.

For this ```UITwistMenu```, we will make a twist menu that is a _composite_ element, and is going to use sub-elements to do its bidding.  Specifically, a twist menu will more or less look like the following:

![twist_menu_sample](images/twist_menu_sample.png)

As you can see, we basically have a "left" arrow button, a "right" arrow button, and a static text field that shows what the display text for the twist menu's current item is.  

In addition to references to these 3 sub-elements, we need to know what the actual items for the menu are (if they are statically defined), and whether or not to infinitely cycle through the items.  Our data properties structure ends up looking like:

```cpp
    struct TwistMenuDataProperties
    {
        // element used to decrement the current selected item
        ID m_leftButtonId;

        // element used to display the current selected item
        ID m_displayStaticTextId;

        // element used to increment the current selected item
        ID m_rightButtonId;

        // the actual items to select between
        UIDisplayStringList m_item;

        // whether or not to infinitely cycle through the items
        bool m_cycleItems;
    }
```

Since the layout data may not possess one or more of the declared properties, it is a good practice to make sure that reasonable defaults are established for the properties.  For **ID** properties, they automatically have a default of the empty id, or ```ID()```.  The remaining 2 properties are given the defaults:

```cpp
    static constexpr UIConstDisplayString c_defaultItem = u8"<empty>";
    static constexpr bool c_defaultCycleItems = false;
```

## Step 3: Declare your new UI class factory

Just like how "built-in" element classes have been placed within the ```UIElements.h``` file, the factories that are declared, for the purposes of creating those element instances from data, are declared in the [gx_dev/Kits/UITK/UIElementFactories.h](../UIElementFactories.h) source file.

A new UI element factory class basically possesses 2 methods -- it will eventually be 3 if/when we stand up our own editor capable of _serializing out_ UI elements to a file.  _The 2 methods are:_ a ```Create()``` virtual override method, and a ```DeserializeDataProperties()``` static method.  

The declaration looks like the following:

```cpp
class UITwistMenuFactory : public UIElementFactory<UITwistMenu>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ UITwistMenu::TwistMenuDataProperties&);

protected:
    virtual UITwistMenu* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newTwistMenu = UIElementFactory<UITwistMenu>::Create(manager, id, data);
        UITwistMenuFactory::DeserializeDataProperties(
            data,
            newTwistMenu->m_twistMenuDataProperties);
        return newTwistMenu;
    }
};
```

We extend the base ```UIElementFactory<>``` template so that we leverage boilerplate allocation logic, and then immediately utilize the static ```DeserializeDataProperties()``` method to fill in the class instance data properties from data.

As mentioned previously in [Step 1](#step1), the element class factory can be found in any header so long as the header that declared the new element class is included, as well as ```UIManager.h```, before your class declaration.

## Step 4: Implement the DeserializeDataProperties() method

The ```UIDataPtr``` provided to the _deserialize_ method is the means of parsing properties from loaded data.  The property data itself generally consists of key/value pair(s), which in **json** looks something like:

```json
{
    "classId": "TwistMenu",
    "id": "catTwist",
    "leftButtonId": "aButton",
    "displayTextId": "Label",
    "rightButtonId": "anotherButton",
    "cycleItems": false,
    "items": [ "CAT", "FERRET", "DOG", "ALLIGATOR" ]
}
```

The ```classId``` and ```id``` fields are boilerplate fields that are handled by the **UITK** for managing the UI element, but the remaining 5 properties mirror those declared in the class structure.

We need to parse 3 ```ID```s, a vector of ```UIDisplayString```s, and a ```bool```.  The code ends up looking like the following (for ```UITwistMenuFactory``` found in [gx_dev/Kits/UITK/UIElementFactories.cpp](../UIElementFactories.cpp)):

```cpp
/*static*/ void UITwistMenuFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ UITwistMenu::TwistMenuDataProperties& twistMenuDataProperties)
{
    // parse the left button element ID
    data->GetTo<ID>(Keywords::c_leftButtonId, twistMenuDataProperties.m_leftButtonId);

    // parse the right button element ID
    data->GetTo<ID>(Keywords::c_rightButtonId, twistMenuDataProperties.m_rightButtonId);

    // parse the display static text element ID
    data->GetTo<ID>(Keywords::c_displayTextId, twistMenuDataProperties.m_displayStaticTextId);

    // parse the staticly defined items
    data->GetTo(Keywords::c_items, twistMenuDataProperties.m_items);

    // parse whether or not to infinitely cycle through items
    data->GetTo(Keywords::c_cycleItems, twistMenuDataProperties.m_cycleItems);
}
```

The class ```Keywords``` declares in one place (in [gx_dev/Kits/UITK/UIKeywords.h](../UIKeywords.h) to be exact) where all the property keys can be found for data needed by the **UITK** and its "built-in" element and style classes.  Code declaring its own properties can define these keys where they wish as they are just constant **UTF8** encoded strings.

## Step 5: Register the UI class factory

This is a crucial step in getting the **UITK** to recognize your new class from data, and successfully make instances.  It involves making 1 function call to the ```UIManager``` from somewhere within your initialization code like so:

```m_uiManager->RegisterElementFactory<``` _NewTypeFactory_ ```>(``` _NewType_ ```::ClassId());```

For the "built-in" element classes such as ```UITwistMenu```, those registrations happen internally within the ```UIManager``` itself in a method named ```RegisterInternalElementFactories()``` and looks like the following:

```cpp
void UIManager::RegisterInternalElementFactories()
{
  RegisterElementFactory<UIPanelFactory>(UIPanel::ClassID());
  RegisterElementFactory<UIStaticTextFactory>(UIStaticText::ClassID());
  RegisterElementFactory<UIImageFactory>(UIImage::ClassID());
  RegisterElementFactory<UIButtonFactory>(UIButton::ClassID());
  RegisterElementFactory<UIProgressBarFactory>(UIProgressBar::ClassID());

  // the UITwistMenu element class factory is registered...
  RegisterElementFactory<UITwistMenuFactory>(UITwistMenu::ClassID());
}
```

## Step 6: Implement the UI element logic

Remember the "boilerplate" element methods that we declared in [Step 1](#step1) that remain to be implemented?  Here we will go ahead and demonstrate the logic employed by the ```UITwistMenu``` to make it work.

### HandleInputEvent() 
The ```HandleInputEvent()``` method allows for the element to take action based on an **InputEvent** passed to it from the framework.  The ```InputEvent``` struct provides the type of event, and the current state of all the recognized input devices (declared in [gx_dev/Kits/UITK/UIInputState.h](../UIInputState.h)).

The ```UITwistMenu``` does not need to handle input events, and so simply returns ```false``` meaning that the element did **_not_** handle the event:

```cpp
/*virtual*/ bool UITwistMenu::HandleInputEvent(const InputEvent& /*inputEvent*/)
{
    // We have no need to handle input events on our own.  
    // We rely on sub-element input handling instead.
    return false;
}
```

### Render()
We go ahead and implement a ```Render()``` method for the element to allow it to draw anything, that is specific to the element, to the screen each frame.  The ```UIManager``` calls the render methods for elements that are deemed to be visible (through the ```UIElement::IsVisible()``` base method).

All positioning and sizing of the element's UI rectangle is generally handled through the **UITK** and the base classes, such as ```UIElement```.  Rendering of elements requires the use of **_UIStyle(s)_** to perform the actual rendering chores.

For the ```UITwistMenu```, it relies entirely on the rendering of its referenced sub-elements to render itself, and chooses not to rendering anything else specific to the twist menu itself.  So its method does nothing for now:

```cpp
/*virtual*/ void UITwistMenu::Render()
{
    // We do not need to render anything specific.
}
```

If we did want to render something, the base ```UIElement``` class recognizes a ```style``` property which allows for the element to render itself through a style such as a ```UITextStyle``` or a ```UISpriteStyle``` (both found in [gx_dev/Kits/UITK/UIStyles.h](../UIStyles.h)).

### PostLoad()/Update() 

The ```Update()``` provides the means for the element to do frame-by-frame logic, and the ```PostLoad()``` is essentially a first update of sorts that is always called after the element has been loaded from a layout.  

The ```UIManager``` calls the input event handling methods first, then proceeds to call ```Update()```s on those UI elements which are **_not_** disabled (for which ```UIElement::IsEnabled()``` returns _true_).  The ```UITwistMenu``` uses this mechanism to wire up its references to the sub-elements which help provide its functionality like so:

```cpp
/*virtual*/ void UITwistMenu::Update()
{
  // no frame-by-frame logic needed.
}

/*virtual*/ void UITwistMenu::PostLoad()
{
    WireUpElements();
}

void UITwistMenu::WireUpElements()
{
    // cache a reference to our display static text element and set
    // the display text to be our current item's text.
    if (!m_cachedDisplayText)
    {
        m_cachedDisplayText = m_uiManager.FindTypedById<UIStaticText>(
            m_twistMenuDataProperties.m_displayStaticTextId);
        m_cachedDisplayText->SetDisplayText(GetCurrentDisplayString());
    }

    // cache a reference to our "left" (or decrement) button and
    // make sure to listen for when that button is clicked.
    if (!m_cachedLeftButton)
    {
        m_cachedLeftButton = m_uiManager.FindTypedById<UIButton>(
            m_twistMenuDataProperties.m_leftButtonId);
        m_cachedLeftButton->ButtonState.AddListenerWhen(UIButton::Pressed, [this](UIButton*)
            {
                if (m_twistMenuDataProperties.m_cycleItems || SelectedItemState.Get().CurrentItem > 0)
                {
                    SetSelectedItem(
                        (SelectedItemState.Get().CurrentItem + GetItemCount() - 1) % GetItemCount()
                    );
                }
            });
    }

    // cache a reference to our "right" (or increment) button and
    // make sure to listen for when that button is clicked.
    if (!m_cachedRightButton)
    {
        m_cachedRightButton = m_uiManager.FindTypedById<UIButton>(
            m_twistMenuDataProperties.m_rightButtonId);
        m_cachedRightButton->ButtonState.AddListenerWhen(UIButton::Pressed, [this](UIButton*)
            {
                if (m_twistMenuDataProperties.m_cycleItems || SelectedItemState.Get().CurrentItem < (GetItemCount() - 1))
                {
                    SetSelectedItem(
                        (SelectedItemState.Get().CurrentItem + 1) % GetItemCount()
                    );
                }
            });
    }
}
```

and with those methods, we now have a working twist menu that will change the display text for the currently selected option.  The **_next step_** will provide the means for other code to receive an event when the selected item has changed -- which is what the ```SelectedItemState``` member shown in the code above does.

## Step 7: Expose UI element change events

There is a built-in class within the **UITK** that allows for elements to be able to easily notify interested code when the element's state changes.  The class is ```UIStateEvent<>```, found in [gx_dev/Kits/UITK/UIEvent.h](../UIEvent.h), and requires 2 template parameters: the UI element class; and the type of state value to track.

The ```UITwistMenu``` element class tracks the currently selected item as an index into the vector of items it maintains. For convenience, as well as to tell when the item has changed, it also tracks the previously selected item.  The state struct ends up looking like:

```cpp
    struct SelectedItem
    {
        SelectedItem() : CurrentItem(0), PreviousItem(0) {}
        SelectedItem(uint32_t currentItem, uint32_t previousItem) : CurrentItem(currentItem), PreviousItem(previousItem) {}

        bool operator==(const SelectedItem& other) const
        {
            return CurrentItem == other.CurrentItem && PreviousItem == other.PreviousItem;
        }

        uint32_t CurrentItem;
        uint32_t PreviousItem;
    };
```

with this structure, being able to tell when the currently selected item has changed is a matter of comparing the state values -- like is done in the convenience method ```UITwistMenu::ChangedSelectedItem()```:

```cpp
    bool ChangedSelectedItem() const
    {
        auto currentState = SelectedItemState.Get();
        return currentState.CurrentItem != currentState.PreviousItem;
    }
```

Before any updating of the UI element instances happens by the ```UIManager```, the **UITK** gives the element the chance to reset its event state for the upcoming frame so that frame-by-frame state changes do not happen across frames.  The ```UITwistMenu``` does the following to set the _previous_ item to be the _current_ item:

```cpp
    virtual void ResetEventState()
    {
        auto currentState = SelectedItemState.Get();
        SelectedItemState.ClearTo(
            SelectedItem{ currentState.CurrentItem, currentState.CurrentItem }
        );
    }
```

Notice in the **_last step_** ([Step 6](#step6)) that we called an internal method ```SetSelectedItem()``` in response to sub-element button clicks.  It is by _setting_ the state for the **SelectedItemState** that the notification will happen:

```cpp
    void SetSelectedItem(uint32_t selectedIndex)
    {
        if (selectedIndex < GetItemCount())
        {
            auto currentState = SelectedItemState.Get();
            if (currentState.CurrentItem != selectedIndex)
            {
                if (m_cachedDisplayText)
                {
                    m_cachedDisplayText->SetDisplayText(
                        GetItemDisplayString(selectedIndex)
                    );
                }

                SelectedItemState.Set(
                    UITwistMenu::SelectedItem{ selectedIndex, currentState.CurrentItem },
                    this
                );
            }
        }
    }
```

In the method above, we modify the display text for the sub-element that shows our current selected item, and then call ```.Set()``` on the state event with the new index.  All code registered to listen to that event through either ```AddListener()``` (or ```AddListenerWhen()``` if the state represents a specific value) will be notified of the change.

```UITwistMenu``` finally exposes this event as a part of its public API so that other code can register and unregister listeners to this state change:

```cpp
public:
    UIStateEvent<UITwistMenu, SelectedItem> SelectedItemState;
```

...and that about covers the basics of standing up a new UI element class within the **UITK** framework.
