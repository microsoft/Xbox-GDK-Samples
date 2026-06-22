//--------------------------------------------------------------------------------------
// menu.hpp
//
// An onscreen menu that is controllable with a gamepad and renders via UISprite12.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <vector>

class UISprite12;

typedef VOID(*SettingsTriggerCallback)(VOID* pContext, INT Delta);
typedef VOID(*IntPrintCallback)(INT Value, WCHAR* strBuffer, UINT32 BufferSizeChars);
typedef VOID(*FloatPrintCallback)(FLOAT Value, WCHAR* strBuffer, UINT32 BufferSizeChars);

class SettingsNumpad
{
private:
    bool m_Visible;
    bool m_Success;

    UINT32 m_Value;
    UINT32 m_KeyIndex;
    const WCHAR* m_strCaption;
    FLOAT m_FracTime;

public:
    SettingsNumpad();
    bool IsVisible() const { return m_Visible; }
    bool IsSuccess() const { return m_Success; }
    UINT32 GetValue() const { return m_Value; }

    void ShowNumpad(const WCHAR* strCaption, UINT32 InitialValue);
    BOOL Update(UINT32 PressedButtons, FLOAT DeltaTime);
    void Render(UISprite12* pSprite, INT Xpos, INT Ypos, FLOAT FontSize);

private:
    void TypeDigit(UINT32 Digit);
    void Backspace();
};

class SettingsStringList
{
private:
    bool m_Visible;
    bool m_Success;

    UINT32 m_Value;
    UINT32 m_MaxValue;
    UINT32 m_MaxWidthChars;
    IntPrintCallback m_pPrintCallback;
    const WCHAR** m_ppEnums;
    UINT32 m_Padding;

public:
    SettingsStringList();
    bool IsVisible() const { return m_Visible; }
    bool IsSuccess() const { return m_Success; }
    UINT32 GetValue() const { return m_Value; }

    void ShowStringList(UINT32 MaxValue, const WCHAR** ppEnums, IntPrintCallback pCallback, UINT32 InitialValue);
    BOOL Update(UINT32 PressedButtons, FLOAT DeltaTime);
    void Render(UISprite12* pSprite, INT Xpos, INT Ypos, FLOAT FontSize);
};

class SettingsMenu
{
private:
    enum class SettingType
    {
        Integer = 0,
        Integer64,
        Trigger,
        Float,
        FloatDiscrete,
        Bool,
        SimpleTrigger,
        Menu,
    };

    struct SettingItem
    {
        const WCHAR* strName;
        VOID* pData;
        VOID* pContext;
        SettingType Type;
        FLOAT ChangePerSecond;
        FLOAT MinFloat;
        FLOAT MaxFloat;
        INT MinInt;
        INT MaxInt;
        const WCHAR** ppEnums;
    };

    typedef std::vector<SettingItem> ItemList;
    ItemList m_RootItems;
    ItemList* m_pItems;

    ItemList* m_pPrevItems;
    static const FLOAT m_NavigationTimeSeconds;
    FLOAT m_NavigationCountdown;
    INT m_NavigationDirection;

    UINT m_ItemIndex;
    FLOAT m_Countdown;
    FLOAT m_DisplayTimeout;
    bool m_IsModal;
    FLOAT m_BackgroundAlpha;

    SettingsNumpad m_Numpad;
    SettingsStringList m_StringList;

public:
    SettingsMenu()
        : m_ItemIndex( 0 ),
          m_Countdown( -1 ),
          m_DisplayTimeout( 0 ),
          m_IsModal(false),
          m_BackgroundAlpha(0)
    {
        NavigateToRoot();
        m_pPrevItems = nullptr;
        m_NavigationCountdown = 0;
        m_NavigationDirection = 0;
    }

    UINT32 GetSettingCount() const { return (UINT32)m_pItems->size(); }

    bool IsVisible() const { return m_Countdown != 0.0f; }
    void SetVisible(bool Visible)
    {
        if (Visible)
        {
            if (m_DisplayTimeout > 0)
            {
                m_Countdown = m_DisplayTimeout;
            }
            else
            {
                m_Countdown = -1.0f;
            }
        }
        else
        {
            m_Countdown = 0.0f;
        }
    }

    VOID SetDisplayTimeout( FLOAT Timeout, bool Modal = false )
    {
        if (Timeout <= 0.0f)
        {
            m_DisplayTimeout = 0.0f;
        }
        else
        {
            m_DisplayTimeout = Timeout;
        }
        SetVisible(true);
        m_IsModal = Modal;
    }

    void SetBackgroundAlpha(FLOAT Alpha)
    {
        m_BackgroundAlpha = Alpha;
    }

    void NavigateToRoot()
    {
        m_pItems = &m_RootItems;
        m_ItemIndex = 0;
    }

    VOID AddFloat(const WCHAR* strName, FLOAT* pValue, FLOAT MinValue, FLOAT MaxValue, FLOAT ChangePerSecond, FloatPrintCallback pCallback = nullptr)
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::Float;
        Item.MinFloat = MinValue;
        Item.MaxFloat = MaxValue;
        Item.ChangePerSecond = ChangePerSecond;
        Item.pContext = (void*)pCallback;
        m_pItems->push_back( Item );
    }
    VOID AddFloatDiscrete(const WCHAR* strName, FLOAT* pValue, FLOAT MinValue, FLOAT MaxValue, FLOAT ChangePerTick, FloatPrintCallback pCallback = nullptr)
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::FloatDiscrete;
        Item.MinFloat = MinValue;
        Item.MaxFloat = MaxValue;
        Item.ChangePerSecond = ChangePerTick;
        Item.pContext = (void*)pCallback;
        m_pItems->push_back( Item );
    }
    VOID AddInt( const WCHAR* strName, INT* pValue, INT MinValue, INT MaxValue, const WCHAR** ppEnums = nullptr )
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::Integer;
        Item.MinInt = MinValue;
        Item.MaxInt = MaxValue;
        Item.ppEnums = ppEnums;
        m_pItems->push_back( Item );
    }
    VOID AddInt64( const WCHAR* strName, INT64* pValue, INT64 MinValue, INT64 MaxValue )
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::Integer64;
        Item.MinInt = (INT)MinValue;
        Item.MaxInt = (INT)MaxValue;
        m_pItems->push_back( Item );
    }
    VOID AddUint( const WCHAR* strName, UINT* pValue, UINT MinValue, UINT MaxValue, const WCHAR** ppEnums = nullptr )
    {
        AddInt( strName, (INT*)pValue, (INT)MinValue, (INT)MaxValue, ppEnums );
    }
    VOID AddUint64( const WCHAR* strName, UINT64* pValue, UINT64 MinValue, UINT64 MaxValue )
    {
        AddInt64( strName, (INT64*)pValue, (INT64)MinValue, (INT64)MaxValue );
    }
    VOID AddPrintCallbackInt( const WCHAR* strName, INT* pValue, INT MinValue, INT MaxValue, IntPrintCallback pCallback )
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::Integer;
        Item.MinInt = MinValue;
        Item.MaxInt = MaxValue;
        Item.pContext = (VOID*)pCallback;
        m_pItems->push_back( Item );
    }
    VOID AddPrintCallbackUint( const WCHAR* strName, UINT* pValue, UINT MinValue, UINT MaxValue, IntPrintCallback pCallback )
    {
        AddPrintCallbackInt( strName, (INT*)pValue, (INT)MinValue, (INT)MaxValue, pCallback );
    }
    VOID AddBool( const WCHAR* strName, BOOL* pValue )
    {
        static const WCHAR* strBools[] = { L"False", L"True" };
        AddInt( strName, (INT*)pValue, 0, 1, strBools );
    }
    void AddBool(const WCHAR* strName, bool* pValue)
    {
        static const WCHAR* strBools[] = { L"False", L"True" };
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pValue;
        Item.Type = SettingType::Bool;
        Item.MinInt = 0;
        Item.MaxInt = 1;
        Item.ppEnums = strBools;
        m_pItems->push_back( Item );
    }
    VOID AddTrigger( const WCHAR* strName, SettingsTriggerCallback pCallback, VOID* pContext = nullptr )
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = (VOID*)pCallback;
        Item.Type = SettingType::Trigger;
        Item.pContext = pContext;
        m_pItems->push_back( Item );
    }
    void AddTrigger(const WCHAR* strName, UINT32 Value, UINT32* pWriteback)
    {
        SettingItem Item = {};
        Item.strName = strName;
        Item.pData = pWriteback;
        Item.Type = SettingType::SimpleTrigger;
        Item.MinInt = (INT)Value;
        m_pItems->push_back( Item );
    }
    void AddChildMenu(const WCHAR* strName)
    {
        ItemList* pChildMenu = new ItemList();
        SettingItem Item = {};
        Item.strName = L"Go Back (B)";
        Item.pData = m_pItems;
        Item.Type = SettingType::Menu;
        Item.MinInt = GetSettingCount();
        Item.MaxInt = -1;
        pChildMenu->push_back(Item);

        Item.strName = strName;
        Item.pData = pChildMenu;
        Item.MinInt = 0;
        Item.MaxInt = 1;
        m_pItems->push_back(Item);

        m_pItems = pChildMenu;
    }

    bool NavigateUp()
    {
        if (m_pItems == &m_RootItems)
        {
            return false;
        }
        const SettingItem& FirstItem = (*m_pItems)[0];
        if (FirstItem.Type != SettingType::Menu)
        {
            return false;
        }
        m_pItems = (ItemList*)FirstItem.pData;
        m_ItemIndex = FirstItem.MinInt;
        return true;
    }

    BOOL Update(UINT32 LastButtons, UINT32 PressedButtons, FLOAT DeltaTime, BOOL* pValueChanged = nullptr);
    void Render( UISprite12* pSprite, INT Xpos, INT Ypos, INT NameWidth, FLOAT FontSize = 20.0f );

private:
    void RenderItems(ItemList* pItems, FLOAT Alpha, bool RenderValues, UISprite12* pSprite, INT Xpos, INT Ypos, INT NameWidth, FLOAT FontSize);
    void NavigateMenu(const SettingItem& Item);
};

