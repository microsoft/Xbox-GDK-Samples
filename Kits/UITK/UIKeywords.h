//--------------------------------------------------------------------------------------
// File: UIKeywords.h
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

#ifndef NAMESPACE_ATG_UITK_BEGIN
#define NAMESPACE_ATG_UITK_BEGIN namespace ATG { namespace UITK {
#define NAMESPACE_ATG_UITK_END } }
#endif

NAMESPACE_ATG_UITK_BEGIN

namespace Keywords
{
#define UITK_BOOL
#define UITK_STRING
#define UITK_NUMBER
#define UITK_VECTOR2
#define UITK_VECTOR3
#define UITK_VECTOR4
#define UITK_NUMBERARRAY
#define UITK_STRINGARRAY
#define UITK_FILENAME
#define UITK_STYLEID
#define UITK_CLASSID
#define UITK_IDREF

// Special UITK value types
#define UITK_CHILDELEMENTS
#define UITK_DEFINITIONS
#define UITK_ID
#define UITK_INCLUDES
#define UITK_LAYOUT
#define UITK_PREFAB
#define UITK_STYLE
#define UITK_STYLES
#define UITK_SUBELEMENTS

#define UITK_DECL_FIELD(field, ...) \
            static constexpr const char* c_##field = #field

#define UITK_DECL_VALUE(value, ...) \
            static constexpr const char* cv_##value = #value

#define UITK_FIELD(field) Keywords::c_##field
#define UITK_VALUE(value) Keywords::cv_##value
#define UITK_VALUES(...)
#define UITK_CLASS_ENABLE(...) // Validator ensures this value is limited in use to the class 

    // NOTE: keep these in alphabetic order for easy lookup

    // UITK reserved Sub-Objects
    UITK_DECL_FIELD(childElements, UITK_CHILDELEMENTS);
    UITK_DECL_FIELD(definitions, UITK_DEFINITIONS);
    UITK_DECL_FIELD(includes, UITK_INCLUDES);
    UITK_DECL_FIELD(layout, UITK_LAYOUT);
    UITK_DECL_FIELD(prefab, UITK_PREFAB);
    UITK_DECL_FIELD(style, UITK_STYLE);
    UITK_DECL_FIELD(styles, UITK_STYLES);
    UITK_DECL_FIELD(subElements, UITK_SUBELEMENTS);

    ///////////////////////////////////////////////////////
    // alphabetical lists of keys/fields
    ///////////////////////////////////////////////////////
#pragma region Keys/Fields

    //! @page activePipStyleId
    //! @brief The @ref ID of the style to use for rendering the active pip.
    //!
    //! ### activePipStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "activePipStyleId": "active_pip_sprite_style" }
    //! @endcode
    //! @b Notes <p> @ref ID must reference a @ref SpriteStyle.
    UITK_DECL_FIELD(activePipStyleId, UITK_IDREF);

    //! @page backgroundSubElementId
    //! @brief The @ref ID of the sub element to use for the background extents.
    //!
    //! ### backgroundSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "backgroundSubElementId": "MySliderBackground" } 
    //! @endcode
    //! @b Notes <p>
    UITK_DECL_FIELD(backgroundSubElementId, UITK_IDREF);

    //! @page checked
    //! @brief The initial checked state of a @ref CheckBox.
    //!
    //! ### checked JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | false
    //! @b owner | @ref CheckBox
    //!
    //! @b Example
    //! @code 
    //! { "checked": true } 
    //! @endcode
    //! @b Notes <p>     
    UITK_DECL_FIELD(checked, UITK_BOOL);

    //! @page classId
    //! @brief The class of widget to create for the UI element.
    //!
    //! ### classId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "classId": "Button" }
    //! @endcode
    //! @b Notes <p> The @ref ID must refer to a registered Widget class.
    UITK_DECL_FIELD(classId, UITK_CLASSID);

    //! @page clipChildren
    //! @brief Whether or not to clip child/sub-element rendering.
    //!
    //! ### clipChildren JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | false
    //! @b owner | @ref Panel
    //!
    //! @b Example
    //! @code 
    //! { "clipChildren": true }
    //! @endcode
    //! @b Notes <p> Clips all descendents to the screen rectangle of the @ref Panel 
    //! unless overridden by a different scissor rectangle.
    UITK_DECL_FIELD(clipChildren, UITK_BOOL);

    //! @page color
    //! @brief The red, green, blue, and alpha components for a style's render color.
    //!
    //! ### color JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 4 numbers
    //! @b range | RGB: 0-255; A: 0.0-1.0
    //! @b default | [ 255, 255, 255, 1.0 ]
    //! @b owner | @ref UIStyle
    //!
    //! @b Example
    //! @code 
    //! { "color": [ 0, 128, 192, 0.5 ] }
    //! @endcode
    //! @b Notes <p> How the color value is used is impacted by the @ref colorUsage property.
    UITK_DECL_FIELD(color, UITK_VECTOR4);

    //! @page colorUsage
    //! @brief How the render @ref color is applied to the render state.
    //!
    //! ### colorUsage JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID (@ref ColorUsage)
    //! @b range | "useExisting", "setExisting", or "override"
    //! @b default | "useExisting"
    //! @b owner | @ref UIStyle
    //!
    //! @b Example
    //! @code 
    //! { "colorUsage": "override" }
    //! @endcode
    //! @b Notes <p> The @ref useExisting color usage causes the @ref color to be ignored.
    UITK_DECL_FIELD(colorUsage, UITK_STRING, UITK_VALUES(override, setExisting, useExisting));

    //! @page disabledStyleId
    //! @brief The @ref ID of the @ref SpriteStyle to use for rendering the disabled button.
    //!
    //! ### disabledStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Button
    //!
    //! @b Example
    //! @code 
    //! { "disabledStyleId": "button_disabled_sprite_style" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined @ref SpriteStyle.
    UITK_DECL_FIELD(disabledStyleId, UITK_IDREF);

    //! @page displayTextSubElementId
    //! @brief The @ref ID for the descendent sub element for the displayed @ref StaticText. 
    //!
    //! ### displayTextSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "displayTextSubElementId": "TwistMenuText" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a descendent sub element.
    UITK_DECL_FIELD(displayTextSubElementId, UITK_IDREF);

    //! @page enabled
    //! @brief Sets the initial enabled state for a UI element.
    //!
    //! ### enabled JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | true
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "enabled": false }
    //! @endcode
    //! @b Notes <p> Disabled elements do not receive input events or handle updates, including descendants.
    UITK_DECL_FIELD(enabled, UITK_BOOL);

    //! @page focusable
    //! @brief Determines whether or not the UI element can be focused.
    //!
    //! ### focusable JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | false
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "focusable": true }
    //! @endcode
    //! @b Notes <p>
    UITK_DECL_FIELD(focusable, UITK_BOOL);

    //! @page focusedStyleId
    //! @brief The @ref ID of the @ref SpriteStyle to use for rendering the focused button.
    //!
    //! ### focusedStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Button
    //!
    //! @b Example
    //! @code 
    //! { "focusedStyleId": "button_focused_sprite_style" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined @ref SpriteStyle.
    UITK_DECL_FIELD(focusedStyleId, UITK_IDREF);

    //! @page font
    //! @brief The relative asset path to a font used for @ref StaticText text rendering.
    //!
    //! ### font JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | string
    //! @b range | <N/A>
    //! @b default | "Assets/Fonts/SegoeUI"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "font": "Assets/Fonts/Courier" }
    //! @endcode
    //! @b Notes <p> The path must point to an existing spritefont asset which contains UTF8 character glyphs.
    UITK_DECL_FIELD(font, UITK_STRING);

    //! @page fontType
    //! @brief Font type of the @ref font.
    //!
    //! ### fontType JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref FontType)
    //! @b range | "sprite" or "freetype"
    //! @b default | "sprite"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "fontType": "sprite" }
    //! @endcode
    //! @b Notes <p> If not specified, it will be considered as sprite font by default.
    UITK_DECL_FIELD(fontType, UITK_STRING, UITK_VALUES(Sprite, FreeType));


    //! @page fontSize
    //! @brief The point size to use for a specific @ref font in a @ref TextTyle.
    //!
    //! ### font JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 1
    //! @b default | 18
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "fontSize": 24 }
    //! @endcode
    //! @b Notes <p> When fontType property is set to sprite or is not set, the final @ref font filename is determined partially by the point size.
    //! So the point size must refer to an asset that exists for that point size.
    //! <p> When fontType property is set to freetype, the final @ref fount filename is not determined by the point size.
    UITK_DECL_FIELD(fontSize, UITK_NUMBER);

    //! @page gridSize
    //! @brief The ref unit dimensions to use for a @ref DebugPanel single rendered grid cell.
    //!
    //! ### gridSize JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 2 numbers
    //! @b range | 0.0 to ref resolution max (1920)
    //! @b default | 5.0
    //! @b owner | @ref DebugPanel
    //!
    //! @b Example
    //! @code 
    //! { "gridSize": [ 20.0, 20.0 ] }
    //! @endcode
    //! @b Notes <p> Making the grid size outside the listed range is undefined.
    UITK_DECL_FIELD(gridSize, UITK_VECTOR2, UITK_CLASS_ENABLE(UIDebugPanel));

    // VALUE TYPE: ID 
    // RANGE: "left", "center", or "right"
    // DEFAULT: "left"
    // USED BY: TextStyle
    // DESCRIPTION: 
    // EXAMPLE: { "horizontalAlign": "center" }

    //! @page horizontalAlign
    //! @brief The horizontal alignment to use for text rendered within a @ref UIElement
    //! screen rectangle.
    //!
    //! ### horizontalAlign JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref HorizontalAlignment)
    //! @b range | "left", "center", or "right"
    //! @b default | "left"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "horizontalAlign": "center" }
    //! @endcode
    //! @b Notes <p> @ref ID must reference a @ref HorizontalAlignment enumerated value string.
    UITK_DECL_FIELD(horizontalAlign, UITK_VALUES(Left, Center, Right));

    //! @page horzWrap
    //! @brief Specifies the horizontal text wrapping policy to apply to the @ref StaticText.
    //!
    //! ### horzWrap JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref HorizontalTextWrapping)
    //! @b range | "overflow", "wrapAtSpace"
    //! @b default | "overflow"
    //! @b owner | @ref StaticText
    //!
    //! @b Example
    //! @code 
    //! { "horzWrap": "wrapAtSpace" }
    //! @endcode
    //! @b Notes <p> As of 2020-10-15, "overflow" and "wrapAtSpace" are implemented.  Wrapping can also be
    //! forceably applied by using the '\\n' character in the display text.  Wrapping will not break words
    //! that possess a dash in the middle.
    UITK_DECL_FIELD(horzWrap, UITK_VALUES(Overflow, WrapAtSpace));

    //! @page hoveredStyleId
    //! @brief The @ref ID of the @ref SpriteStyle to use for rendering the hovered (mouse over) button.
    //!
    //! ### hoveredStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Button
    //!
    //! @b Example
    //! @code 
    //! { "hoveredStyleId": "button_hovered_style_id" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined @ref SpriteStyle.
    UITK_DECL_FIELD(hoveredStyleId, UITK_IDREF);

    // VALUE TYPE: ID 
    // RANGE:
    // DEFAULT: ""
    // USED BY: UIElement
    // DESCRIPTION: ID is a string that identifies the element
    // EXAMPLE: { "id": "FooWidget" }

    //! @page id
    //! @brief The instance name for the @ref UIElement that identifies it.
    //!
    //! ### id JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | a non-empty string of alphabetic, numeric, and symbol characters.
    //! @b default | ""
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "id": "MyElementID" }
    //! @endcode
    //! @b Notes <p> The IDs for elements are case-sensitive and must match case exactly
    //! when referred to by another piece of data.
    UITK_DECL_FIELD(id, UITK_ID);

    //! @page inactivePipStyleId
    //! @brief The @ref ID of the style to use for rendering the inactive pip.
    //!
    //! ### inactivePipStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "inactivePipStyleId": "inactive_pip_sprite_style" }
    //! @endcode
    //! @b Notes <p> @ref ID must reference a @ref SpriteStyle.
    UITK_DECL_FIELD(inactivePipStyleId, UITK_IDREF);

    //! @page infinitelyCycleItems
    //! @brief Determine whether or not the wrap around when scrolling through items.
    //!
    //! ### infinitelyCycleItems JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | false
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "infinitelyCycleItems": true }
    //! @endcode
    //! @b Notes <p> This single value applies to all possible scrolling directions.
    UITK_DECL_FIELD(infinitelyCycleItems, UITK_BOOL);

    //! @page inheritsFromId
    //! @brief The style ID for a @ref UIStyle from which to derive and override style
    //! data values.
    //!
    //! ### inheritsFromId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref UIStyle
    //!
    //! @b Example
    //! @code 
    //! { "inheritsFromId": "base_style_id" }
    //! @endcode
    //! @b Notes <p> An empty inherit @ref ID means do not inherit.  A non-empty @ref ID
    //! must refer to a previously defined style @ref ID.
    UITK_DECL_FIELD(inheritsFromId, UITK_IDREF);

    //! @page initialValue
    //! @brief The initial value to set for a @ref Slider.
    //!
    //! ### initialValue JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | -3.40282e+038 to +3.40282e+038
    //! @b default | 0.5
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "initialValue": 5.0 }
    //! @endcode
    //! @b Notes <p> The initial value will be clamped to the @ref valueRange.
    UITK_DECL_FIELD(initialValue, UITK_NUMBER); // BugBug: This should be scoped to slider in some way

    //! @page innerUVExtents
    //! @brief The inner UV values from which to render eight or nine-sliced textured
    //! quads for a sprite using a @ref SpriteStyle.
    //!
    //! ### innerUVExtents JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 4 numbers
    //! @b range | all are from 0.0 to 1.0
    //! @b default | [ 0.0, 0.0, 1.0, 1.0 ]
    //! @b owner | @ref SpriteStyle
    //!
    //! @b Example
    //! @code 
    //! { "innerUVExtents": [ 0.1, 0.1, 0.9, 0.9 ] }
    //! @endcode
    //! @b Notes <p> The values are ordered as: minU, minV, maxU, maxV.  Having
    //! values which exceed beyond the acceptable range, or extend beyond the
    //! @ref outerUVExtents, is considered to be UNDEFINED behavior.
    UITK_DECL_FIELD(innerUVExtents, UITK_VECTOR4);

    //! @page isLegend
    //! @brief Determines whether or not to render the @ref StaticText as regular display
    //! text, or as a mixed display/controller text string.
    //!
    //! ### isLegend JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | false
    //! @b owner | @ref StaticText
    //!
    //! @b Example
    //! @code 
    //! { "isLegend": true }
    //! @endcode
    //! @b Notes <p> The corresponding @ref TextStyle referenced by the @ref StaticText
    //! must be sure to have the @ref legendFont property set to a legitimate controller
    //! font asset.
    UITK_DECL_FIELD(isLegend, UITK_BOOL);

    //! @page items
    //! @brief The initial set of display text items to include in the UI element.
    //!
    //! ### items JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | Array of display strings.
    //! @b range | <N/A>
    //! @b default | [ "<empty list>" ]
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "items": [ "option 1", "option 2", "option 3" ] }
    //! @endcode
    //! @b Notes <p> If the item list is empty, it is expected to be populated at runtime.
    UITK_DECL_FIELD(items, UITK_STRINGARRAY);
    
    //! @page leftButtonSubElementId
    //! @brief The @ref ID for the descendent sub element for the left item scroll @ref Button.
    //!
    //! ### leftButtonSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "leftButtonSubElementId": "MyLeftButtonId" }
    //! @endcode
    //! @b Notes <p> The @ref ID must point to a defined sub element which is a @ref Button class.
    UITK_DECL_FIELD(leftButtonSubElementId, UITK_IDREF);
    
    //! @page legendFont
    //! @brief The relative asset path to a legend font used for @ref StaticText legend rendering.
    //!
    //! ### legendFont JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | string
    //! @b range | <N/A>
    //! @b default | "Assets/Fonts/XboxOneControllerLegendSmall"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "legendFont": "Assets/Fonts/MySpecialControllerLegend" }
    //! @endcode
    //! @b Notes <p> The path must point to an existing font which contains the controller character set.
    UITK_DECL_FIELD(legendFont, UITK_STRING);

    //! @page margin
    //! @brief The extra outside space to add to a @ref UIElement when determining the
    //! placement of an element within its parent rectangle.
    //!
    //! ### margin JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 4 numbers
    //! @b range | all are -INFINITY to +INFINITY
    //! @b default | [ 0, 0, 0, 0 ]
    //! @b owner | @ref UIStyle
    //!
    //! @b Example
    //! @code 
    //! { "margin": [ 10, 10, 10, 10 ] }
    //! @endcode
    //! @b Notes <p> The units used for the outward extension of the UI element's
    //! rectangle for placement are reference resolution units.
    UITK_DECL_FIELD(margin, UITK_VECTOR4);

    //! @page maxConsoleLines
    //! @brief Sets the maximum number of console text lines to maintain for a @ref ConsoleWindow.
    //!
    //! ### maxConsoleLines JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 0
    //! @b default | 50
    //! @b owner | @ref ConsoleWindow
    //!
    //! @b Example
    //! @code 
    //! { "maxConsoleLines": 100 }
    //! @endcode
    //! @b Notes <p> 
    UITK_DECL_FIELD(maxConsoleLines, UITK_NUMBER);
    
    //! @page numberOfPips
    //! @brief The number of pips to display for a @ref PipStrip.
    //!
    //! ### numberOfPips JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 0
    //! @b default | 0
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "numberOfPips": 3 }
    //! @endcode
    //! @b Notes <p> If the number is 0, nothing is rendered.  If the number is greater
    //! than 0, then 1 active pip is rendered, and the rest are rendered as inactive.
    UITK_DECL_FIELD(numberOfPips, UITK_NUMBER);

    //! @page numDiscreteSteps
    //! @brief For a @ref Slider that is a @ref Discrete value type, this determines the number of discrete steps.
    //!
    //! ### numDiscreteSteps JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 1
    //! @b default | 10
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "numDiscreteSteps": 20 }
    //! @endcode
    //! @b Notes <p> The number is interpreted as an unsigned integer, but the @ref Slider value
    //! does not have to be a whole integer to utilize discrete steps.
    UITK_DECL_FIELD(numDiscreteSteps, UITK_NUMBER);

    //! @page maxVisibleItems
    //! @brief Maximum number of @ref StackPanel elements that are visible; StackPanel becomes scrollable
    //! if number of items exceeds this value
    //!
    //! ### maxVisibleItems JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 0
    //! @b default | 0
    //! @b owner | @ref StackPanel
    //!
    //! @b Example
    //! @code 
    //! { "maxVisibleItems": 10 }
    //! @endcode
    //! @b Notes <p> Invisible items shifted in when navigation attempts to go past the (maxVisibleItems)th item
    //! 0 means unlimited, i.e. all items visible
    UITK_DECL_FIELD(maxVisibleItems, UITK_NUMBER);

    //! @page sliderSubElementId
    //! @brief The @ref ID for the descendant sub element @ref Slider.
    //!
    //! ### sliderSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref StackPanel @ref StaticText
    //!
    //! @b Example
    //! @code 
    //! { "sliderSubElementId": "Slider" }
    //! @endcode
    //! @b Notes <p> The @ref ID must point to a defined sub element descendant under the @ref StackPanel
    //! or @ref StaticText that is a @ref Slider class.  
    UITK_DECL_FIELD(sliderSubElementId, UITK_IDREF);
    
    //! @page outerUVExtents
    //! @brief The outer UV values from which to render any textured
    //! quad(s) for a sprite using a @ref SpriteStyle.
    //!
    //! ### outerUVExtents JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 4 numbers
    //! @b range | all are from 0.0 to 1.0
    //! @b default | [ 0.0, 0.0, 1.0, 1.0 ]
    //! @b owner | @ref SpriteStyle
    //!
    //! @b Example
    //! @code 
    //! { "outerUVExtents": [ 0.0, 0.0, 0.5, 0.5 ] }
    //! @endcode
    //! @b Notes <p> The values are ordered as: minU, minV, maxU, maxV.  Having
    //! values which exceed beyond the acceptable range, or do not extend beyond the
    //! @ref innerUVExtents (if applicable), is considered to be UNDEFINED behavior.
    UITK_DECL_FIELD(outerUVExtents, UITK_VECTOR4);
    
    //! @page padding
    //! @brief The extra inside space to subtract from a @ref UIElement when determining the
    //! placement of an element within its parent rectangle.
    //!
    //! ### padding JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 4 numbers
    //! @b range | all are -INFINITY to +INFINITY
    //! @b default | [ 0, 0, 0, 0 ]
    //! @b owner | @ref UIStyle
    //!
    //! @b Example
    //! @code 
    //! { "padding": [ 0, 20, 0, 20 ] }
    //! @endcode
    //! @b Notes <p> The units used for the inward compression of the UI element's
    //! rectangle for placement are reference resolution units.
    UITK_DECL_FIELD(padding, UITK_VECTOR4);

    //! @page pipSize
    //! @brief The horizontal and vertical render size of an individual pip for a @ref PipStrip.
    //!
    //! ### pipSize JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 2 numbers (horizontal, vertical)
    //! @b range | both values >= 0.0
    //! @b default | [ 20, 20 ]
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "pipSize": [ 40, 30 ] }
    //! @endcode
    //! @b Notes <p> The units for the pip size values are in reference resolution units.
    UITK_DECL_FIELD(pipSize, UITK_VECTOR2);

    //! @page pipSpacing
    //! @brief The amount of spacing to place in between pips in a @ref PipStrip.
    //!
    //! ### pipSpacing JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | >= 0.0
    //! @b default | 0.0
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "pipSpacing": 10 }
    //! @endcode
    //! @b Notes <p> The spacing is in reference resolution units.  Whether the spacing is
    //! is interpreted as horizontal spacing, or vertical spacing, is subject to the
    //! @ref stripOrientation of the @ref PipStrip.
    UITK_DECL_FIELD(pipSpacing, UITK_NUMBER);

    //! @page pipStripSubElementId
    //! @brief The @ref ID for the descendent sub element for an optional associated @ref PipStrip.
    //!
    //! ### pipStripSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "pipStripSubElementId": "MyPipStripId" }
    //! @endcode
    //! @b Notes <p> The @ref must point to a defined sub element which is a @ref PipStrip class.
    UITK_DECL_FIELD(pipStripSubElementId, UITK_IDREF);

    //! @page position
    //! @brief The anchor position of the element relative to its parent's positioning
    //! and its own @ref positioningAnchor.
    //!
    //! ### position JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | Array of 2 numbers 
    //! @b range | All are -INFINITY to +INFINITY
    //! @b default | [ 0.0, 0.0 ]
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "position": [ 100.0, 100.0 ] }
    //! @endcode
    //! @b Notes <p> The units for position are reference resolution units, not
    //! actual pixels.  Negative values go leftward/upward, positive values go
    //! rightward/downward.
    UITK_DECL_FIELD(position, UITK_VECTOR2);

    //! @page positioningAnchor
    //! @brief The horizontal and vertical positioning anchor for the element,
    //! relative to its parent's rectangle.
    //!
    //! ### positioningAnchor JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | Array of 2 anchor values: the @ref HorizontalAnchor and the @ref VerticalAnchor.
    //! @b range | "left", "center", or "right" or horizontal anchor.\n"top", "middle", or "bottom" for vertical anchor.
    //! @b default | [ "left", "top" ]
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "positioningAnchor": [ "center", "middle" }
    //! @endcode
    //! @b Notes <p> The @ref position represents an @b offset from this anchor.  
    UITK_DECL_FIELD(positioningAnchor, UITK_STRINGARRAY);

    //! @page prefabRef
    //! @brief Relative asset file path to a UITK prefab layout file to instantiate.
    //!
    //! ### prefabRef JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | String (asset relative path)
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | <N/A>
    //!
    //! @b Example
    //! @code 
    //! { "prefabRef": "Assets/Layouts/my_prefab.json" }
    //! @endcode
    //! @b Notes <p> The reference must either be empty (none), or a legitimate UITK
    //! prefab layout file using the @ref prefab JSON container property that contains
    //! UI element property definitions to instantiate and potentially override.
    UITK_DECL_FIELD(prefabRef, UITK_FILENAME);

    //! @page pressedStyleId
    //! @brief The @ref ID of the @ref SpriteStyle to use for rendering the pressed (mouse down) button.
    //!
    //! ### pressedStyleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Button
    //!
    //! @b Example
    //! @code 
    //! { "pressedStyleId": "button_pressed_style_id" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined @ref SpriteStyle.
    UITK_DECL_FIELD(pressedStyleId, UITK_IDREF);

    //! @page progressPercentage
    //! @brief The initial percentage of completion to set for a @ref ProgressBar UI element.
    //!
    //! ### progressPercentage JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | number
    //! @b range | between 0.0 and 1.0
    //! @b default | 0.0
    //! @b owner | @ref ProgressBar
    //!
    //! @b Example
    //! @code 
    //! { "progressPercentage": 0.5 }
    //! @endcode
    //! @b Notes <p> The @ref ProgressBar draws a portion of itself based upon its
    //! current progress percentage.  Percentages are clamped.
    UITK_DECL_FIELD(progressPercentage, UITK_NUMBER);

    //! @page rightButtonSubElementId
    //! @brief The @ref ID for the descendent sub element for the right item scroll @ref Button.
    //!
    //! ### rightButtonSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref TwistMenu
    //!
    //! @b Example
    //! @code 
    //! { "rightButtonSubElementId": "MyRightButtonId" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined sub element which is a @ref Button class.
    UITK_DECL_FIELD(rightButtonSubElementId, UITK_IDREF);
    
    //! @page size
    //! @brief The horizontal and vertical dimensions of the element.
    //!
    //! ### size JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | Array of 2 numbers
    //! @b range | Each dimension: 0 <= size <= INFINITY
    //! @b default | UNDEFINED
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "size": [ 80, 40 ] }
    //! @endcode
    //! @b Notes <p> The units for the dimensions are in reference resolution units.
    UITK_DECL_FIELD(size, UITK_VECTOR2);

    //! @page sizingAnchor
    //! @brief The horizontal and vertical sizing anchoring for the element,
    //! relative to its own @ref position.
    //!
    //! ### sizingAnchor JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | Array of 2 anchor values: the @ref HorizontalAnchor and the @ref VerticalAnchor.
    //! @b range | "left", "center", or "right" for horizontal.\n"top", "middle", or "bottom" for vertical.
    //! @b default | [ "left", "top" ]
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "sizingAnchor": [ "right", "bottom" ] }
    //! @endcode
    //! @b Notes <p> The anchors describe how the element's rectangle sizes @b from
    //! its own anchor position.  For example, if the sizing anchor is "right" and
    //! "bottom", then it sizes itself leftward and upward.
    UITK_DECL_FIELD(sizingAnchor, UITK_STRINGARRAY);

    //! @page sliderOrientation
    //! @brief The @ref SliderOrientation to use for the @ref Slider UI element.
    //!
    //! ### sliderOrientation JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref SliderOrientation)
    //! @b range | "lefttoright" or "toptobottom"
    //! @b default | "lefttoright"
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "sliderOrientation": "toptobottom" }
    //! @endcode
    //! @b Notes <p> The @ref SliderOrientation specifies the direction toward which values
    //! proceed from the range's minimum value to the range's maximum value.
    UITK_DECL_FIELD(sliderOrientation, UITK_VALUES(LeftToRight, TopToBottom));

    //! @page sliderType
    //! @brief The type of value to consider the @ref Slider element as modifying.
    //!
    //! ### sliderType JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref SliderType)
    //! @b range | "continuous" or "discrete"
    //! @b default | "continuous"
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "sliderType": "discrete" }
    //! @endcode
    //! @b Notes <p> The @ref numDiscreteSteps property will work in conjunction with a "discrete"
    //! @ref Slider.  A "continuous" slider does not rely on discrete stepping and will adjust the
    //! @ref Slider value up to pixel-level granularity.
    UITK_DECL_FIELD(sliderType, UITK_STRING, UITK_VALUES(Continuous, Discrete));

    //! @page spacingBetweenElements
    //! @brief [description]
    //!
    //! ### spacingBetweenElements JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | [type]
    //! @b range | [range]
    //! @b default | [default]
    //! @b owner | [owner]
    //!
    //! @b Example
    //! @code 
    //! { "[key]": "[value]" }
    //! @endcode
    //! @b Notes <p> [notes]
    UITK_DECL_FIELD(spacingBetweenElements, UITK_NUMBER);

    //! @page spriteType
    //! @brief The type of sprite to render for a sprite rendered with a @ref SpriteStyle. 
    //!
    //! ### spriteType JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref SpriteType)
    //! @b range | "simple", "eightsliced", or "ninesliced"
    //! @b default | "simple"
    //! @b owner | @ref SpriteStyle
    //!
    //! @b Example
    //! @code 
    //! { "spriteType": "nineSliced" }
    //! @endcode
    //! @b Notes <p> Eight and nine-sliced sprites expect values for both @ref innerUVExtents and
    //! @ref outerUVExtents, where "simple" sprites only requires @ref outerUVExtents.
    UITK_DECL_FIELD(spriteType, UITK_STRING, UITK_VALUES(Simple, EightSliced, NineSliced, ThreeSlicedVert, ThreeSlicedHorz));

    //! @page stackElementPadding
    //! @brief this is the spacing between stacked elements in a @ref StackPanel.
    //!
    //! ### stackElementPadding JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | a number
    //! @b range | -INFINITY to +INFINITY
    //! @b default | 0
    //! @b owner | @ref UIStackPanel
    //!
    //! @b Example
    //! @code 
    //! { "stackElementPadding": 10 }
    //! @endcode
    //! @b Notes <p> the number represents the distance in display ref units.
    UITK_DECL_FIELD(stackElementPadding, UITK_NUMBER);

    //! @page stackElementAlignment
    //! @brief Horizontal or Vertical alignment anchor to use for a @ref StackPanel.
    //!
    //! ### stackElementAlignment JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref HorizontalAnchor, or @ref VerticalAnchor
    //! @b range | "left", "center" or "right" (HorizontalAnchor); "top", "middle", or "bottom" (VerticalAnchor)
    //! @b default | "left" or "top"
    //! @b owner | @ref UIStackPanel
    //!
    //! @b Example
    //! @code 
    //! { "stackElementAlignment": "center" }
    //! @endcode
    //! @b Notes <p> The anchor setting must correspond with the stacking orientation.
    UITK_DECL_FIELD(stackElementAlignment, UITK_STRING, UITK_VALUES(Left, Center, Right, Top, Middle, Bottom));

    //! @page stackingAnchor
    //! @brief The horizontal anchor from which to stack each contained element.
    //!
    //! ### stackingAnchor JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref HorizontalAnchor)
    //! @b range | "left", "center", or "right"
    //! @b default | "top"
    //! @b owner | @ref VerticalStack
    //!
    //! @b Example
    //! @code 
    //! { "stackingAnchor": "center" }
    //! @endcode
    //! @b Notes <p> None.
    UITK_DECL_FIELD(stackingAnchor, UITK_STRING, UITK_VALUES(Left, Center, Right));

    //! @page stackingDirection
    //! @brief The vertical stacking direction which all contained stacked elements follow.
    //!
    //! ### stackingDirection JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref StackingDirection)
    //! @b range | "downward" or "upward"
    //! @b default | "downward"
    //! @b owner | UIVerticalStack
    //!
    //! @b Example
    //! @code 
    //! { "stackingDirection": "upward" }
    //! @endcode
    //! @b Notes <p> None.
    UITK_DECL_FIELD(stackingDirection, UITK_VALUES(Downward, Upward));

    //! @page stackingOrientation
    //! @brief The direction from which to stack contained elements for a @ref StackPanel.
    //!
    //! ### stackingOrientation JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | StackingOrientation
    //! @b range | "down", "up", "right", "left"
    //! @b default | "down" or "right" 
    //! @b owner | UIStackPanel
    //!
    //! @b Example
    //! @code 
    //! { "stackingOrientation": "up" }
    //! @endcode
    //! @b Notes <p> The default value depends on the @ref StackPanel type of Horizontal or Vertical.
    UITK_DECL_FIELD(stackingOrientation, UITK_VALUES(Down, Up, Left, Right));

    //! @page stripOrientation
    //! @brief The orientation and direction with which to render the pips of a @ref PipStrip.
    //!
    //! ### stripOrientation JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref PipStripOrientation)
    //! @b range | "lefttoright" or "toptobottom"
    //! @b default | "lefttoright"
    //! @b owner | @ref PipStrip
    //!
    //! @b Example
    //! @code 
    //! { "stripOrientation": "toptobottom" }
    //! @endcode
    //! @b Notes <p> The orientation specifies which direction the ordering of the
    //! pip rendering occurs for a @ref PipStrip.
    UITK_DECL_FIELD(stripOrientation, UITK_VALUES(LeftToRight, TopToBottom));

    //! @page styleId
    //! @brief The @ref ID of the base @ref UIStyle to use for rendering the element.
    //!
    //! ### styleId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "styleId": "my_basic_style" }
    //! @endcode
    //! @b Notes <p> The @ref ID must reference a defined @ref UIStyle.
    UITK_DECL_FIELD(styleId, UITK_STYLEID);

    //! @private
    UITK_DECL_FIELD(texelsPerRefUnit, UITK_NUMBER);

    //! @page text
    //! @brief Display text string to render onto the @ref StaticText element.
    //!
    //! ### text JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | UTF8 encoded string
    //! @b range | <N/A>
    //! @b default | "Lorem Ipsum"
    //! @b owner | @ref StaticText
    //!
    //! @b Example
    //! @code 
    //! { "text": "Sample Title" }
    //! @endcode
    //! @b Notes <p> 
    UITK_DECL_FIELD(text, UITK_STRING);

    //! @page texture
    //! @brief The relative asset path for the texture file to use for the @ref SpriteStyle
    //! textured rendering.
    //!
    //! ### texture JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | string
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref SpriteStyle
    //!
    //! @b Example
    //! @code 
    //! { "texture": "Assets/Textures/rounded_rect.png" }
    //! @endcode
    //! @b Notes <p> The file type for the texture must be one of the WIC supported types
    //! which include the common formats: BMP, DDS, JPG, PNG, and TIF.
    UITK_DECL_FIELD(texture, UITK_FILENAME);

    //! @page thumbButtonSubElementId
    //! @brief The @ref ID for the descendant sub element @ref Button to use as the @ref Slider thumb.
    //!
    //! ### thumbButtonSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "thumbButtonSubElementId": "MyThumbButtonID" }
    //! @endcode
    //! @b Notes <p> The @ref ID must point to a defined sub element descendant under the @ref Slider
    //! that is a @ref Button class.  The @ref Slider is manipulated through the movement of the
    //! thumb.
    UITK_DECL_FIELD(thumbButtonSubElementId, UITK_IDREF);
    
    //! @page valueRange
    //! @brief The minimum and maximum values defined for a @ref Slider.
    //!
    //! ### valueRange JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | array of 2 numbers
    //! @b range | -3.40282e+038 to +3.40282e+038
    //! @b default | [ 0.0, 1.0 ]
    //! @b owner | @ref Slider
    //!
    //! @b Example
    //! @code 
    //! { "valueRange": [ 1.0, 10.0 ] }
    //! @endcode
    //! @b Notes <p> The maximum is expected to be greater than the minimum for clamping values.
    UITK_DECL_FIELD(valueRange, UITK_VECTOR2, UITK_CLASS_ENABLE(UISlider)); // BugBug: Needs to be scoped

    //! @page verticalAlign
    //! @brief The vertical alignment to use for text rendered within a @ref UIElement
    //! screen rectangle.
    //!
    //! ### verticalAlign JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref VerticalAlignment)
    //! @b range | "top", "middle", or "bottom"
    //! @b default | "top"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "verticalAlign": "bottom" }
    //! @endcode
    //! @b Notes <p> @ref ID must reference a @ref VerticalAlignment enumerated value string.
    UITK_DECL_FIELD(verticalAlign, UITK_STRING, UITK_VALUES(Bottom, Middle, Top));

    //! @page verticalSliderSubElementId
    //! @brief The @ref ID for the descendent sub element @ref Slider used as a scroll bar.
    //!
    //! ### verticalSliderSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref ConsoleWindow
    //!
    //! @b Example
    //! @code 
    //! { "verticalSliderSubElementId": "MyVerticalScrollBar" }
    //! @endcode
    //! @b Notes <p> The @ref ID must point to a sub element @ref Slider.
    UITK_DECL_FIELD(verticalSliderSubElementId, UITK_IDREF);

    //! @page vertTrunc
    //! @brief Specifies the vertical truncation to apply to the @ref StaticText.
    //!
    //! ### vertTrunc JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID (@ref VerticalTextTruncation)
    //! @b range | "overflow", or "truncate"
    //! @b default | "overflow"
    //! @b owner | @ref StaticText
    //!
    //! @b Example
    //! @code 
    //! { "vertTrunc": "truncate" }
    //! @endcode
    //! @b Notes <p> As of 2020-10-15, both the "overflow" and "truncate" truncations are implemented.
    //! Truncation can also be forceably applied by having a parent @ref Panel that clips
    //! the child @ref StaticText element.
    UITK_DECL_FIELD(vertTrunc, UITK_STRING, UITK_VALUES(Overflow, Truncate));

    //! @page viewportPanelSubElementId
    //! @brief The @ref ID for the @ref Panel container to use for the @ref ConsoleWindow text content.
    //!
    //! ### viewportPanelSubElementId JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | @ref ID
    //! @b range | <N/A>
    //! @b default | ""
    //! @b owner | @ref ConsoleWindow
    //!
    //! @b Example
    //! @code 
    //! { "viewportPanelSubElementId": "MyViewportID" }
    //! @endcode
    //! @b Notes <p> The @ref ID must point to a defined @ref Panel sub element descendant.
    UITK_DECL_FIELD(viewportPanelSubElementId, UITK_IDREF);

    //! @page visible
    //! @brief Sets the initial visible state for a UI element.
    //!
    //! ### visible JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | boolean
    //! @b range | false, true
    //! @b default | true
    //! @b owner | @ref UIElement
    //!
    //! @b Example
    //! @code 
    //! { "visible": false }
    //! @endcode
    //! @b Notes <p> Invisible elements do not receive input events or render, including descendants.
    UITK_DECL_FIELD(visible, UITK_BOOL);

    //! @page weight
    //! @brief The weight styling to use for a @ref TextStyle font.
    //!
    //! ### weight JSON property
    //!
    //! characteristics ||
    //! ---|---
    //! @b type | ID (@ref TypeWeight)
    //! @b range | normal, italic, or bold
    //! @b default | "normal"
    //! @b owner | @ref TextStyle
    //!
    //! @b Example
    //! @code 
    //! { "weight": "bold" }
    //! @endcode
    //! @b Notes <p> Sprite font filenames must embed a @b non-normal weight into their filename
    //! like so: <TypefaceName>_<size>_<weight>.
    //! <p> for example: SegoeUI_18_italic.spritefont
    //! <p> When fontType property is set to freetype, this value will be ignored.    
    UITK_DECL_FIELD(weight, UITK_STRING, UITK_VALUES(Normal, Bold, Italic)); // BugBug: Needs to be scoped - FontWeight?

#pragma endregion

    ///////////////////////////////////////////////////////
    // alphabetical lists of values
    ///////////////////////////////////////////////////////
#pragma region Values

    // Alignment Values
    UITK_DECL_VALUE(bottom, "Align element to bottom of rect.");
    UITK_DECL_VALUE(center);
    UITK_DECL_VALUE(left);
    UITK_DECL_VALUE(middle);
    UITK_DECL_VALUE(right);
    UITK_DECL_VALUE(top);

    // Directional/Orientation Values
    UITK_DECL_VALUE(up);
    UITK_DECL_VALUE(down);

    // Fonts type values
    UITK_DECL_VALUE(sprite);
    UITK_DECL_VALUE(freetype);    

    // Fonts weight values
    UITK_DECL_VALUE(bold, "Use bold font style.");
    UITK_DECL_VALUE(italic);
    UITK_DECL_VALUE(normal);

    // Texture Styling values
    UITK_DECL_VALUE(eightSliced);
    UITK_DECL_VALUE(nineSliced);
    UITK_DECL_VALUE(simple);
    UITK_DECL_VALUE(threeSlicedHorz);
    UITK_DECL_VALUE(threeSlicedVert);

    // Fitting values
    UITK_DECL_VALUE(overflow);
    UITK_DECL_VALUE(truncate);
    UITK_DECL_VALUE(wrapAnywhere);
    UITK_DECL_VALUE(wrapAtSpace);

    // Color override values
    UITK_DECL_VALUE(override);
    UITK_DECL_VALUE(setExisting);
    UITK_DECL_VALUE(useExisting);

    // UISlider Type values
    UITK_DECL_VALUE(continuous, UITK_CLASS_ENABLE(UISlider));
    UITK_DECL_VALUE(discrete, UITK_CLASS_ENABLE(UISlider));

    // UISlider/UIPinStrip Direction values
    UITK_DECL_VALUE(leftToRight, UITK_CLASS_ENABLE(UISlider), UITK_CLASS_ENABLE(UIPinStrip));
    UITK_DECL_VALUE(topToBottom, UITK_CLASS_ENABLE(UISlider), UITK_CLASS_ENABLE(UIPinStrip));

    // UIVerticalStack Direction values
    UITK_DECL_VALUE(downward, UITK_CLASS_ENABLE(UIVerticalStack));
    UITK_DECL_VALUE(upward, UITK_CLASS_ENABLE(UIVerticalStack));
    
#pragma endregion

#undef UITK_DECL_FIELD
#undef UITK_DECL_VALUE
};

NAMESPACE_ATG_UITK_END
