//--------------------------------------------------------------------------------------
// console.hpp
//
// A simple onscreen console that renders using the UISprite12 class.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#if defined(_XBOX_ONE) && defined(_TITLE)
#include <xdk.h>
#endif
#include <stdio.h>
#include "UISprite12.h"

class ConsoleWindow
{
private:
    UINT32 m_DisplayLineCount;
    UINT32 m_StorageLineCount;
    UINT32 m_CharsPerLine;
    WCHAR* m_pStorage;
    UINT32 m_CurrentLine;
    bool m_Wrap;
    UINT32 m_RelativeCursor;
    FLOAT m_TypematicTimeout;

    WCHAR* GetLine( UINT32 Index )
    {
        if (Index >= m_StorageLineCount)
        {
            return nullptr;
        }
        return m_pStorage + ( Index * m_CharsPerLine );
    }

    WCHAR* GetCurrentLine() { return GetLine( m_CurrentLine ); }

    void AdvanceLine()
    {
        UINT32 NextLine = ( m_CurrentLine + 1 ) % m_StorageLineCount;
        if (NextLine < m_CurrentLine)
        {
            m_Wrap = true;
        }
        m_CurrentLine = NextLine;

        if (m_RelativeCursor > 0)
        {
            SetRelativeCursor(GetRelativeCursor() + 1);
        }
    }

public:
    ConsoleWindow( UINT32 StorageLineCount, UINT32 CharsPerLine, UINT32 DisplayLineCount )
    {
        m_DisplayLineCount = DisplayLineCount;
        m_StorageLineCount = StorageLineCount;
        m_CharsPerLine = CharsPerLine;
        m_pStorage = new WCHAR[m_StorageLineCount * m_CharsPerLine];
        Clear();
    }
    ~ConsoleWindow()
    {
        if (m_pStorage != nullptr)
        {
            delete[] m_pStorage;
            m_pStorage = nullptr;
        }
    }

    void Clear()
    {
        m_TypematicTimeout = 0.0f;
        m_CurrentLine = 0;
        m_Wrap = false;
        m_RelativeCursor = 0;
    }

    UINT32 GetLineCount() const
    {
        return m_Wrap ? m_StorageLineCount : m_CurrentLine;
    }
    UINT32 GetRelativeCursor() const { return m_RelativeCursor; }
    void SetRelativeCursor( UINT32 Cursor ) 
    { 
        UINT32 MaxRelativeCursor = GetLineCount();
        if (MaxRelativeCursor >= m_DisplayLineCount)
        {
            MaxRelativeCursor -= m_DisplayLineCount;
        }
        else
        {
            MaxRelativeCursor = 0;
        }
        m_RelativeCursor = std::min( Cursor, MaxRelativeCursor );
    }

    void AddLine( const WCHAR* strLine )
    {
        wcscpy_s( GetCurrentLine(), m_CharsPerLine, strLine );
        AdvanceLine();
    }

    void AddLine( const CHAR* strLine )
    {
        MultiByteToWideChar( CP_ACP, 0, strLine, strlen(strLine) + 1, GetCurrentLine(), m_CharsPerLine );
        AdvanceLine();
    }

    void PrintLine( const WCHAR* strFormat, ... )
    {
        WCHAR* strOutput = GetCurrentLine();

        va_list args;
        va_start( args, strFormat );
        vswprintf_s( strOutput, m_CharsPerLine, strFormat, args );
        va_end( args );

        AdvanceLine();
    }

    void Update( UINT32 LastButtons, UINT32 PressedButtons, FLOAT DeltaTime )
    {
        const FLOAT TypematicDelay = 0.5f;
        const FLOAT TypematicRepeat = 0.05f;

        bool Up = false;
        bool Down = false;

        if (PressedButtons & (UINT32)GamepadButtons::DPadUp)
        {
            Up = true;
            m_TypematicTimeout = TypematicDelay;
        }
        else if (PressedButtons & (UINT32)GamepadButtons::DPadDown)
        {
            Down = true;
            m_TypematicTimeout = TypematicDelay;
        }
        else if (LastButtons & (UINT32)GamepadButtons::DPadUp)
        {
            m_TypematicTimeout -= DeltaTime;
            if (m_TypematicTimeout <= 0)
            {
                Up = true;
                m_TypematicTimeout += TypematicRepeat;
            }
        }
        else if (LastButtons & (UINT32)GamepadButtons::DPadDown)
        {
            m_TypematicTimeout -= DeltaTime;
            if (m_TypematicTimeout <= 0)
            {
                Down = true;
                m_TypematicTimeout += TypematicRepeat;
            }
        }
        else
        {
            m_TypematicTimeout = TypematicDelay;
        }

        if (Up)
        {
            SetRelativeCursor( GetRelativeCursor() + 1 );
        }
        else if (Down)
        {
            if (GetRelativeCursor() > 0)
            {
                SetRelativeCursor( GetRelativeCursor() - 1 );
            }
        }
    }

    void SetDisplayLineCount(UINT32 NewCount)
    {
        if (NewCount != m_DisplayLineCount)
        {
            m_DisplayLineCount = NewCount;
            SetRelativeCursor(GetRelativeCursor());
        }
    }

    void Render( UISprite12* pSprite, INT Xpos, INT Ypos, const FLOAT pColor[4], FLOAT FontSize = 20.0f )
    {
        const INT LineHeight = (INT)FontSize;

        for (UINT32 i = 0; i < m_DisplayLineCount; ++i)
        {
            UINT32 StorageLineIndex = ( ( m_StorageLineCount + m_CurrentLine - m_RelativeCursor ) - m_DisplayLineCount + i ) % m_StorageLineCount;
            const WCHAR* strLine = nullptr;
            if (i == 0 && StorageLineIndex > 0)
            {
                strLine = L"^    ^    ^    ^    ^    ^    ^    ^";
            }
            else if (i == (m_DisplayLineCount - 1) && m_RelativeCursor > 0)
            {
                strLine = L"v    v    v    v    v    v    v    v";
            }
            else
            {
                strLine = GetLine(StorageLineIndex);
            }
            if (StorageLineIndex < m_CurrentLine || m_Wrap)
            {
                pSprite->DrawText( Xpos, Ypos, FontSize, strLine, pColor );
                Ypos += LineHeight;
            }
        }
    }
};
