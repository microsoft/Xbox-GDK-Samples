//--------------------------------------------------------------------------------------
// menu.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "menu.hpp"
#include "UISprite12.h"

const FLOAT SettingsMenu::m_NavigationTimeSeconds = 0.33f;

BOOL SettingsMenu::Update(UINT32 LastButtons, UINT32 PressedButtons, FLOAT DeltaTime, BOOL* pValueChanged)
{
    if (PressedButtons & (UINT32)GamepadButtons::Menu)
    {
        if (m_IsModal)
        {
            SetVisible(!IsVisible());
            return (BOOL)IsVisible();
        }
    }

    if (m_IsModal && !IsVisible())
    {
        return FALSE;
    }

    BOOL ValueChanged = FALSE;

    if (m_Numpad.IsVisible())
    {
        BOOL Result = m_Numpad.Update(PressedButtons, DeltaTime);
        if (!m_Numpad.IsVisible() && m_Numpad.IsSuccess())
        {
            const SettingItem& Item = (*m_pItems)[m_ItemIndex];
            INT Value = (INT)m_Numpad.GetValue();
            Value = std::min(std::max(Value, Item.MinInt), Item.MaxInt);
            switch (Item.Type)
            {
            case SettingType::Integer:
                *(INT*)Item.pData = Value;
                ValueChanged = TRUE;
                break;
            case SettingType::Integer64:
                *(INT64*)Item.pData = (INT64)Value;
                ValueChanged = TRUE;
                break;
            }
        }
        if (pValueChanged != nullptr)
        {
            *pValueChanged = ValueChanged;
        }
        return Result;
    }
    else if (m_StringList.IsVisible())
    {
        BOOL Result = m_StringList.Update(PressedButtons, DeltaTime);
        if (!m_StringList.IsVisible() && m_StringList.IsSuccess())
        {
            const SettingItem& Item = (*m_pItems)[m_ItemIndex];
            INT Value = (INT)m_StringList.GetValue();
            Value = std::min(std::max(Value, Item.MinInt), Item.MaxInt);
            switch (Item.Type)
            {
            case SettingType::Integer:
                *(INT*)Item.pData = Value;
                ValueChanged = TRUE;
                break;
            case SettingType::Integer64:
                *(INT64*)Item.pData = (INT64)Value;
                ValueChanged = TRUE;
                break;
            }
        }
        if (pValueChanged != nullptr)
        {
            *pValueChanged = ValueChanged;
        }
        return Result;
    }

    BOOL Pressed = FALSE;
    const UINT32 NumItems = (UINT32)m_pItems->size();
    if (PressedButtons & (UINT32)GamepadButtons::DPadDown)
    {
        m_ItemIndex = ( m_ItemIndex + 1 ) % NumItems;
        Pressed = TRUE;
    }
    else if (PressedButtons & (UINT32)GamepadButtons::DPadUp)
    {
        m_ItemIndex = ( m_ItemIndex + NumItems - 1 ) % NumItems;
        Pressed = TRUE;
    }
    if (m_ItemIndex >= NumItems)
    {
        m_ItemIndex = 0;
        return Pressed;
    }
    const SettingItem& Item = (*m_pItems)[m_ItemIndex];
    INT Delta = 0;
    UINT32 ButtonMask = (Item.Type == SettingType::Float) ? LastButtons : PressedButtons;
    if (Item.Type == SettingType::Trigger || Item.Type == SettingType::SimpleTrigger)
    {
        if ((ButtonMask & (UINT32)GamepadButtons::A) != 0)
        {
            Delta = 1;
            Pressed = TRUE;
        }
    }
    else
    {
        if (ButtonMask & (UINT32)GamepadButtons::DPadLeft)
        {
            Delta = -1;
            Pressed = TRUE;
        }
        else if (ButtonMask & (UINT32)GamepadButtons::DPadRight)
        {
            Delta = 1;
            Pressed = TRUE;
        }
        else if ((ButtonMask & (UINT32)GamepadButtons::A) != 0)
        {
            Delta = 0;
            Pressed = TRUE;
            switch (Item.Type)
            {
            case SettingType::Integer:
                if (Item.MinInt == 0 && (Item.ppEnums != nullptr || Item.pContext != nullptr))
                {
                    m_StringList.ShowStringList(Item.MaxInt, Item.ppEnums, (IntPrintCallback)Item.pContext, *(INT*)Item.pData);
                }
                else
                {
                    m_Numpad.ShowNumpad(Item.strName, *(INT*)Item.pData);
                }
                break;
            case SettingType::Integer64:
                m_Numpad.ShowNumpad(Item.strName, (UINT32)*(INT64*)Item.pData);
                break;
            case SettingType::Menu:
            case SettingType::Bool:
                Delta = 1;
                break;
            }
        }
        else if ((ButtonMask & (UINT32)GamepadButtons::B) != 0)
        {
            if (m_pItems != &m_RootItems)
            {
                const SettingItem& BackItem = (*m_pItems)[0];
                assert(BackItem.Type == SettingType::Menu);
                NavigateMenu(BackItem);
                return TRUE;
            }
        }
    }

    if (m_DisplayTimeout > 0.0f )
    {
        if (Pressed)
        {
            SetVisible(true);
        }
        else
        {
            m_Countdown = std::max( 0.0f, m_Countdown - DeltaTime );
        }
    }

    if (m_NavigationCountdown > 0)
    {
        m_NavigationCountdown = std::max(0.0f, m_NavigationCountdown - DeltaTime);
        if (m_NavigationCountdown <= 0)
        {
            m_NavigationDirection = 0;
            m_pPrevItems = nullptr;
        }
    }

    if (Delta != 0)
    {
        switch( Item.Type )
        {
        case SettingType::Float:
            {
                FLOAT FDelta = (FLOAT)Delta * DeltaTime * Item.ChangePerSecond;
                FLOAT NewValue = *(FLOAT*)Item.pData + FDelta;
                NewValue = std::min(std::max( NewValue, Item.MinFloat ), Item.MaxFloat );
                *(FLOAT*)Item.pData = NewValue;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::Integer:
            {
                INT NewValue = *(INT*)Item.pData + Delta;
                if (Item.ppEnums != nullptr)
                {
                    NewValue = ( NewValue + Item.MaxInt + 1 ) % ( Item.MaxInt + 1 );
                }
                NewValue = std::min(std::max( NewValue, Item.MinInt ), Item.MaxInt );
                *(INT*)Item.pData = NewValue;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::Bool:
            {
                bool NewValue = !(*(bool*)Item.pData);
                *(bool*)Item.pData = NewValue;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::Integer64:
            {
                INT64 NewValue = *(INT64*)Item.pData + Delta;
                NewValue = std::min(std::max( NewValue, (INT64)Item.MinInt ), (INT64)Item.MaxInt );
                *(INT64*)Item.pData = NewValue;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::Trigger:
            {
                SettingsTriggerCallback pCallback = (SettingsTriggerCallback)Item.pData;
                (*pCallback)( Item.pContext, Delta );
                ValueChanged = TRUE;
                break;
            }
        case SettingType::SimpleTrigger:
            {
                *(UINT32*)Item.pData = (UINT32)Item.MinInt;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::FloatDiscrete:
            {
                FLOAT FDelta = (FLOAT)Delta * Item.ChangePerSecond;
                FLOAT NewValue = *(FLOAT*)Item.pData + FDelta;
                NewValue = std::min(std::max( NewValue, Item.MinFloat ), Item.MaxFloat );
                *(FLOAT*)Item.pData = NewValue;
                ValueChanged = TRUE;
                break;
            }
        case SettingType::Menu:
            {
                NavigateMenu(Item);
                break;
            }
        }
    }

    if (pValueChanged != nullptr)
    {
        *pValueChanged = ValueChanged;
    }
    return Pressed;
}

void SettingsMenu::NavigateMenu(const SettingItem& Item)
{
    assert(Item.Type == SettingType::Menu);
    m_pPrevItems = m_pItems;
    m_pItems = (ItemList*)Item.pData;
    assert(m_pItems != nullptr);
    m_ItemIndex = Item.MinInt;
    m_NavigationDirection = Item.MaxInt;
    m_NavigationCountdown = m_NavigationTimeSeconds;
    if (m_ItemIndex >= m_pItems->size())
    {
        m_ItemIndex = 0;
    }
}

void SettingsMenu::Render(UISprite12* pSprite, INT Xpos, INT Ypos, INT NameWidth, FLOAT FontSize)
{
    if (!IsVisible())
    {
        return;
    }

    if (m_NavigationDirection == 0)
    {
        RenderItems(m_pItems, 1.0f, true, pSprite, Xpos, Ypos, NameWidth, FontSize);
    }
    else
    {
        const FLOAT PercentRemaining = m_NavigationCountdown / m_NavigationTimeSeconds;
        const FLOAT SquaredPercentRemaining = PercentRemaining * PercentRemaining;

        const FLOAT CurrentAlpha = 1.0f - PercentRemaining;
        const FLOAT PrevAlpha = PercentRemaining * 0.5f;

        const FLOAT NavDistance = FontSize * 3.0f;
        const FLOAT NavDir = (FLOAT)m_NavigationDirection;
        const FLOAT NavOffset = NavDistance * (1.0f - SquaredPercentRemaining) * -NavDir;
        const FLOAT PrevNavOffset = NavOffset;
        const FLOAT CurrentNavOffset = NavOffset + NavDistance * NavDir;

        RenderItems(m_pPrevItems, PrevAlpha, false, pSprite, (INT)(Xpos + PrevNavOffset), Ypos, NameWidth, FontSize);
        RenderItems(m_pItems, CurrentAlpha, true, pSprite, (INT)(Xpos + CurrentNavOffset), Ypos, NameWidth, FontSize);
    }
}

void SettingsMenu::RenderItems(ItemList* pItems, FLOAT Alpha, bool RenderValues, UISprite12* pSprite, INT Xpos, INT Ypos, INT NameWidth, FLOAT FontSize)
{
    const FLOAT ColorWhite[4] = { 1, 1, 1, Alpha };

    const UINT32 NumItems = (UINT32)pItems->size();
    const INT SelectorXOffset = 25;

    if (m_BackgroundAlpha > 0)
    {
        const INT BorderWidth = (INT)(FontSize * 0.5f);
        const INT LeftBorderWidth = SelectorXOffset + BorderWidth;
        const INT Height = (INT)(NumItems * FontSize) + BorderWidth * 2;
        FLOAT ColorBackground[4] = { 0, 0, 0, m_BackgroundAlpha * Alpha };
        pSprite->DrawRect(Xpos - LeftBorderWidth, Ypos - BorderWidth, 2048, Height, ColorBackground);
    }

    WCHAR strText[100];

    const INT NumpadXpos = Xpos + NameWidth;
    INT NumpadYpos = Ypos;

    for (UINT32 i = 0; i < NumItems; ++i)
    {
        const SettingItem& Item = (*pItems)[i];
        if (RenderValues && i == m_ItemIndex)
        {
            pSprite->DrawText( Xpos - SelectorXOffset, Ypos, FontSize, L"<>", ColorWhite );
            NumpadYpos = Ypos;
        }

        if (Item.Type == SettingType::Menu)
        {
            swprintf_s(strText, L"[%s]", Item.strName);
            pSprite->DrawText(Xpos, Ypos, FontSize, strText, ColorWhite);
        }
        else
        {
            pSprite->DrawText(Xpos, Ypos, FontSize, Item.strName, ColorWhite);
        }

        if (RenderValues)
        {
            switch (Item.Type)
            {
            case SettingType::Float:
            case SettingType::FloatDiscrete:
            {
                if (Item.pContext != nullptr)
                {
                    FloatPrintCallback pFunc = (FloatPrintCallback)Item.pContext;
                    FLOAT Value = *(FLOAT*)Item.pData;
                    (*pFunc)(Value, strText, ARRAYSIZE(strText));
                }
                else
                {
                    swprintf_s(strText, L"%0.3f", *(FLOAT*)Item.pData);
                }
                pSprite->DrawText(Xpos + NameWidth, Ypos, FontSize, strText, ColorWhite);
                break;
            }
            case SettingType::Trigger:
            case SettingType::SimpleTrigger:
                if (i == m_ItemIndex)
                {
                    pSprite->DrawText(Xpos + NameWidth, Ypos, FontSize, L"Press A", ColorWhite);
                }
                break;
            case SettingType::Menu:
                break;
            default:
                if (Item.pContext != nullptr)
                {
                    IntPrintCallback pFunc = (IntPrintCallback)Item.pContext;
                    INT Value = *(INT*)Item.pData;
                    (*pFunc)(Value, strText, ARRAYSIZE(strText));
                    pSprite->DrawText(Xpos + NameWidth, Ypos, FontSize, strText, ColorWhite);
                }
                else if (Item.ppEnums != nullptr)
                {
                    INT Value = 0;
                    if (Item.Type == SettingType::Bool)
                    {
                        Value = (*(const bool*)Item.pData) ? 1 : 0;
                    }
                    else
                    {
                        Value = *(INT*)Item.pData;
                    }
                    const WCHAR* strEnum = Item.ppEnums[Value - Item.MinInt];
                    pSprite->DrawText(Xpos + NameWidth, Ypos, FontSize, strEnum, ColorWhite);
                }
                else
                {
                    if (Item.Type == SettingType::Integer64)
                    {
                        swprintf_s(strText, L"%I64d", *(INT64*)Item.pData);
                    }
                    else
                    {
                        swprintf_s(strText, L"%d", *(INT*)Item.pData);
                    }
                    pSprite->DrawText(Xpos + NameWidth, Ypos, FontSize, strText, ColorWhite);
                }
                break;
            }
        }

        Ypos += (INT)FontSize;
    }

    if (RenderValues)
    {
        m_Numpad.Render(pSprite, NumpadXpos, NumpadYpos, FontSize);
        m_StringList.Render(pSprite, NumpadXpos, NumpadYpos, FontSize);
    }
}

SettingsNumpad::SettingsNumpad()
    : m_Visible(false),
      m_Success(false),
      m_Value(0),
      m_KeyIndex(0),
      m_strCaption(nullptr),
      m_FracTime(0)
{

}

void SettingsNumpad::ShowNumpad(const WCHAR* strCaption, UINT32 InitialValue)
{
    m_Value = InitialValue;
    m_Success = false;
    m_Visible = true;
    m_KeyIndex = 4;
    m_strCaption = strCaption;
    m_FracTime = 0;
}

BOOL SettingsNumpad::Update(UINT32 PressedButtons, FLOAT DeltaTime)
{
    if (!IsVisible())
    {
        return FALSE;
    }

    m_FracTime += DeltaTime;
    m_FracTime -= floorf(m_FracTime);

    if (PressedButtons & (UINT32)GamepadButtons::B)
    {
        m_Success = false;
        m_Visible = false;
        return TRUE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::X)
    {
        Backspace();
        return TRUE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::Y)
    {
        m_Value = 0;
        return TRUE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::A)
    {
        switch (m_KeyIndex)
        {
        case 9:
            Backspace();
            break;
        case 10:
            TypeDigit(0);
            break;
        case 11:
            m_Success = true;
            m_Visible = false;
            break;
        default:
            TypeDigit(m_KeyIndex + 1);
            break;
        }
        return TRUE;
    }

    static const UINT32 DpadMask = (UINT32)GamepadButtons::DPadLeft | 
                                   (UINT32)GamepadButtons::DPadRight |
                                   (UINT32)GamepadButtons::DPadUp |
                                   (UINT32)GamepadButtons::DPadDown;

    if (PressedButtons & DpadMask)
    {
        const UINT32 ColumnCount = 3;
        const UINT32 RowCount = 4;
        UINT32 Row = m_KeyIndex / ColumnCount;
        UINT32 Column = m_KeyIndex % ColumnCount;
        if (PressedButtons & (UINT32)GamepadButtons::DPadLeft)
        {
            Column = (Column + (ColumnCount - 1)) % ColumnCount;
        }
        else if (PressedButtons & (UINT32)GamepadButtons::DPadRight)
        {
            Column = (Column + 1) % ColumnCount;
        }
        else if (PressedButtons & (UINT32)GamepadButtons::DPadUp)
        {
            Row = (Row + (RowCount - 1)) % RowCount;
        }
        else if (PressedButtons & (UINT32)GamepadButtons::DPadDown)
        {
            Row = (Row + 1) % RowCount;
        }
        m_KeyIndex = (Row * ColumnCount) + Column;
        return TRUE;
    }

    return FALSE;
}

void SettingsNumpad::Backspace()
{
    m_Value /= 10;
}

void SettingsNumpad::TypeDigit(UINT32 Digit)
{
    assert(Digit < 10);
    m_Value = (m_Value * 10) + Digit;
}

void SettingsNumpad::Render(UISprite12* pSprite, INT Xpos, INT Ypos, FLOAT FontSize)
{
    if (!IsVisible())
    {
        return;
    }

    const FLOAT FontWidth = pSprite->GetTextAspectRatio() * FontSize;

    const INT TextSpacing = 4;
    const INT KeyWidth = (TextSpacing * 2) + (INT)(FontWidth * 3);
    const INT KeyHeight = (TextSpacing * 2) + (INT)FontSize;
    const INT KeySpacing = 5;
    const INT DisplayHeight = (INT)FontSize * 2;
    const INT TextWidth = (INT)((FLOAT)wcslen(m_strCaption) * FontWidth);
    const INT RectWidth = std::max(TextWidth + KeySpacing * 2, KeySpacing + (KeyWidth + KeySpacing) * 3);
    const INT RectHeight = KeySpacing + DisplayHeight + (KeyHeight + KeySpacing) * 4;

    Ypos = std::min(Ypos, 1060 - RectHeight);

    const FLOAT ColorBackground[4] = { 0, 0, 0, 0.9f };
    const FLOAT ColorWhite[4] = { 1, 1, 1, 1 };
    const FLOAT ColorGray[4] = { 0.5f, 0.5f, 0.5f, 1 };

    pSprite->DrawRect(Xpos, Ypos, RectWidth, RectHeight, ColorBackground);
    pSprite->DrawRectOutline(Xpos, Ypos, RectWidth, RectHeight, 3, ColorWhite);

    if (m_strCaption != nullptr)
    {
        pSprite->DrawText(Xpos + KeySpacing, Ypos + KeySpacing, FontSize, m_strCaption, ColorWhite);
    }

    WCHAR strValue[32] = L"";
    if (m_Value > 0)
    {
        swprintf_s(strValue, L"%u", m_Value);
    }
    if (m_FracTime < 0.5f)
    {
        wcscat_s(strValue, L"_");
    }
    pSprite->DrawText(Xpos + KeySpacing, Ypos + KeySpacing + (INT)FontSize, FontSize, strValue, ColorWhite);

    const WCHAR* strKeys[] = { L" 1 ", L" 2 ", L" 3 ", 
                               L" 4 ", L" 5 ", L" 6 ",
                               L" 7 ", L" 8 ", L" 9 ",
                               L"Del", L" 0 ", L"Ok " };

    for (INT y = 0; y < 4; ++y)
    {
        const INT KeyYpos = Ypos + KeySpacing + DisplayHeight + (KeyHeight + KeySpacing) * y;
        for (INT x = 0; x < 3; ++x)
        {
            const UINT32 KeyIndex = y * 3 + x;
            const INT KeyXpos = Xpos + KeySpacing + (KeyWidth + KeySpacing) * x;
            if (KeyIndex == m_KeyIndex)
            {
                pSprite->DrawRectOutline(KeyXpos, KeyYpos, KeyWidth, KeyHeight, 3, ColorWhite);
            }
            else
            {
                pSprite->DrawRectOutline(KeyXpos, KeyYpos, KeyWidth, KeyHeight, 2, ColorGray);
            }
            pSprite->DrawText(KeyXpos + TextSpacing, KeyYpos + TextSpacing, FontSize, strKeys[KeyIndex], ColorWhite);
        }
    }
}

SettingsStringList::SettingsStringList()
    : m_Visible(false),
      m_Success(false),
      m_MaxValue(0),
      m_Value(0),
      m_ppEnums(nullptr),
      m_pPrintCallback(nullptr),
      m_Padding(6)
{
}

void SettingsStringList::ShowStringList(UINT32 MaxValue, const WCHAR** ppEnums, IntPrintCallback pCallback, UINT32 InitialValue)
{
    m_MaxValue = MaxValue;
    assert(InitialValue <= m_MaxValue);
    m_Value = InitialValue;
    m_Visible = true;
    m_Success = true;
    m_pPrintCallback = pCallback;
    m_ppEnums = ppEnums;

    m_MaxWidthChars = 0;
    WCHAR strText[100];
    for (UINT32 i = 0; i <= m_MaxValue; ++i)
    {
        UINT32 StringLength = 0;
        if (m_pPrintCallback != nullptr)
        {
            (*m_pPrintCallback)((INT)i, strText, ARRAYSIZE(strText));
            StringLength = wcslen(strText);
        }
        else
        {
            StringLength = wcslen(m_ppEnums[i]);
        }
        if (StringLength > m_MaxWidthChars)
        {
            m_MaxWidthChars = StringLength;
        }
    }
}

BOOL SettingsStringList::Update(UINT32 PressedButtons, FLOAT DeltaTime)
{
    if (!IsVisible())
    {
        return FALSE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::B)
    {
        m_Success = false;
        m_Visible = false;
        return TRUE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::A)
    {
        m_Success = true;
        m_Visible = false;
        return TRUE;
    }

    if (PressedButtons & (UINT32)GamepadButtons::DPadUp)
    {
        if (m_Value > 0)
        {
            --m_Value;
        }
        return TRUE;
    }
    else if (PressedButtons & (UINT32)GamepadButtons::DPadDown)
    {
        if (m_Value < m_MaxValue)
        {
            ++m_Value;
        }
        return TRUE;
    }
    return FALSE;
}

void SettingsStringList::Render(UISprite12* pSprite, INT Xpos, INT Ypos, FLOAT FontSize)
{
    if (!IsVisible())
    {
        return;
    }

    Xpos -= m_Padding;
    Ypos -= m_Padding;

    const FLOAT FontWidth = pSprite->GetTextAspectRatio() * FontSize;
    const UINT RectHeight = (UINT)((FLOAT)(m_MaxValue + 1) * FontSize) + m_Padding * 2;
    const UINT LineWidth = (UINT)((FLOAT)m_MaxWidthChars * FontWidth);
    const UINT RectWidth = LineWidth + m_Padding * 2;

    const FLOAT ColorBackground[4] = { 0, 0, 0, 0.9f };
    const FLOAT ColorWhite[4] = { 1, 1, 1, 1 };
    const FLOAT ColorGray[4] = { 0.5f, 0.5f, 0.5f, 1 };

    pSprite->DrawRect(Xpos, Ypos, RectWidth, RectHeight, ColorBackground);
    pSprite->DrawRectOutline(Xpos, Ypos, RectWidth, RectHeight, 2, ColorWhite);

    const INT HighlightYpos = Ypos + m_Padding + (INT)((FLOAT)m_Value * FontSize);
    const INT LineXpos = Xpos + m_Padding;

    pSprite->DrawRect(LineXpos, HighlightYpos, LineWidth, (UINT)FontSize, ColorGray);

    WCHAR strText[100];
    for (UINT32 i = 0; i <= m_MaxValue; ++i)
    {
        const WCHAR* strLine = strText;
        if (m_pPrintCallback != nullptr)
        {
            (*m_pPrintCallback)(i, strText, ARRAYSIZE(strText));
        }
        else
        {
            strLine = m_ppEnums[i];
        }
        const INT LineYpos = Ypos + (INT)((FLOAT)i * FontSize) + m_Padding;
        pSprite->DrawText(LineXpos, LineYpos, FontSize, strLine, ColorWhite);
    }
}