//--------------------------------------------------------------------------------------
// File: SampleGUI.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "SampleGUI.h"

#include <cstdarg>
#include <exception>
#include <list>
#include <map>
#include <stdexcept>
#include <utility>

#include <DirectXPackedVector.h>

#include "Effects.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "RenderTargetState.h"
#endif

// Other required ATG Tool Kit components
#include "ControllerFont.h"
#include "CSVReader.h"
#include "FindMedia.h"

using namespace DirectX;
using namespace ATG;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_HoldTimeStart = 1.f;
    constexpr float c_HoldTimeRepeat = .1f;
    constexpr float c_KeypressRepeatDelay = .05f;

    constexpr long c_ScrollWidth = 16;
    constexpr long c_MinThumbSize = 8;
    constexpr long c_BorderSize = 6;
    constexpr long c_MarginSize = 5;



    inline void DebugTrace(_In_z_ _Printf_format_string_ const char* format, ...)
    {
#ifdef _DEBUG
        va_list args;
        va_start(args, format);

        char buff[1024] = {};
        vsprintf_s(buff, format, args);
        OutputDebugStringA(buff);
        va_end(args);
#else
        UNREFERENCED_PARAMETER(format);
#endif
    }

    IControl* InitFocus(std::vector<IControl*>& controls)
    {
        IControl *firstCtrl = nullptr;
        IControl* defaultCtrl = nullptr;

        for (IControl* it : controls)
        {
            assert(it != nullptr);
            _Analysis_assume_(it != nullptr);

            if (!it->CanFocus())
                continue;

            if (!firstCtrl)
            {
                firstCtrl = it;
            }

            if (!defaultCtrl && it->DefaultFocus())
            {
                defaultCtrl = it;
                break;
            }
        }

        return (defaultCtrl) ? defaultCtrl : firstCtrl;
    }

    IControl* NextFocus(_In_opt_ IControl* currentFocus, const std::vector<IControl*>& controls)
    {
        if (!currentFocus)
            return nullptr;

        auto it = std::find(controls.cbegin(), controls.cend(), currentFocus);
        if (it == controls.cend())
            throw std::exception("NextFocus");

        for (;;)
        {
            ++it;
            if (it == controls.cend())
                it = controls.cbegin();

            if (currentFocus == *it)
                return currentFocus;

            if ( (*it)->CanFocus())
            {
                currentFocus->OnFocus(false);
                (*it)->OnFocus(true);
                return *it;
            }
        }
    }

    IControl* PrevFocus(_In_opt_ IControl* currentFocus, std::vector<IControl*>& controls)
    {
        if (!currentFocus)
            return nullptr;

        auto it = std::find(controls.cbegin(), controls.cend(), currentFocus);
        if (it == controls.cend())
            throw std::exception("PrevFocus");

        for (;;)
        {
            if (it == controls.cbegin())
                it = controls.end() - 1;
            else
                --it;

            if (currentFocus == *it)
                return currentFocus;

            if ((*it)->CanFocus())
            {
                currentFocus->OnFocus(false);
                (*it)->OnFocus(true);
                return *it;
            }
        }
    }

    IControl* MouseFocus(int x, int y, _In_opt_ IControl* currentFocus, std::vector<IControl*>& controls)
    {
        if (!currentFocus)
            return nullptr;

        for (auto it : controls)
        {
            assert(it != nullptr);
            _Analysis_assume_(it != nullptr);

            if (it->CanFocus() && it->Contains(x,y))
            {
                if (currentFocus != it)
                {
                    if (currentFocus)
                    {
                        currentFocus->OnFocus(false);
                    }
                    it->OnFocus(true);
                    return it;
                }
            }
        }

        return currentFocus;
    }

    IControl* HotKeyFocus(_In_opt_ IControl* currentFocus, std::vector<IControl*>& controls, const Keyboard::KeyboardStateTracker& kbstate)
    {
        for (auto it : controls)
        {
            assert(it != nullptr);
            _Analysis_assume_(it != nullptr);

            const unsigned hotkey = it->GetHotKey();
            if (hotkey != 0)
            {
                if (kbstate.IsKeyPressed(static_cast<Keyboard::Keys>(hotkey)))
                {
                    if (currentFocus == it)
                    {
                        return it;
                    }
                    else if (it->CanFocus())
                    {
                        if (currentFocus)
                        {
                            currentFocus->OnFocus(false);
                            it->OnFocus(true);
                        }
                        return it;
                    }
                }
            }
        }

        return nullptr;
    }

    IControl* SetFocusCtrl(_In_opt_ IControl* currentFocus, _In_ IControl* newFocus)
    {
        if (!newFocus)
            return currentFocus;

        if (!newFocus->CanFocus())
        {
            DebugTrace("WARNING: SetFocus control (%u) cannot be focus, ignored.\n", newFocus->GetId());
            return currentFocus;
        }

        if (currentFocus)
        {
            currentFocus->OnFocus(false);
        }

        newFocus->OnFocus(true);
        return newFocus;
    }

    void ControlSelected(_In_opt_ IControl *ctrl, _In_ IPanel* panel)
    {
        if (!ctrl)
            return;

        if (ctrl->OnSelected(panel))
        {
            panel->Close();
        }
    }

    void TrimTrailingWhitespace(_Inout_z_ wchar_t* str)
    {
        size_t len = wcslen(str);
        for(wchar_t *ptr = str + len; len > 0; --len, --ptr)
        {
            if (!iswspace( *(ptr-1) ))
            {
                *ptr = 0;
                break;
            }
        }
    }

    std::wstring WordWrap(
        _In_z_ const wchar_t* text,
        _In_ const SpriteFont* font,
        const RECT& rect,
        std::vector<size_t>* lineStarts = nullptr)
    {
        if (lineStarts)
        {
            lineStarts->clear();
            if (*text)
                lineStarts->push_back(0);
        }

        std::wstring str;
        str.reserve(wcslen(text));

        size_t line_start = 0;
        size_t last_line = 0;
        size_t last_word = 0;
        size_t extra = 0;

        const wchar_t *ptr = text;

        const XMFLOAT2 pos(float(rect.left), float(rect.top));

        wchar_t prevch = 0;
        while (*ptr != L'\0')
        {
            const wchar_t ch = *ptr;

            str.push_back(ch);

            if (iswspace(ch))
            {
                last_word = size_t(ptr - text);
            }
            else if (prevch == L'-')
            {
                last_word = size_t(ptr - text - 1);
            }

            const RECT textrect = font->MeasureDrawBounds(str.c_str() + line_start, pos);

            if (textrect.right > rect.right)
            {
                if (last_word > last_line)
                {
                    str.erase(str.cbegin() + ptrdiff_t(last_word + extra), str.cend());
                    str.push_back(L'\n');
                    ++extra;
                    last_line = last_word;
                    ptr = text + last_word + 1;
                }
                else
                {
                    str.erase(str.cend() - 1);
                    str.push_back(L'\n');
                    ++extra;
                    last_line = last_word = str.length() - extra;
                }

                line_start = last_line + extra;

                if (lineStarts)
                {
                    lineStarts->push_back(last_line + extra);
                }

                prevch = 0;
                continue;
            }

            ++ptr;
            prevch = ch;
        }

        return str;
    }

    void HandleEscapeCharacters(_Inout_z_ wchar_t* str)
    {
        for (wchar_t*ptr = str; *ptr != 0; ++ptr)
        {
            if (*ptr == L'|')
                *ptr = L'\n';
        }
    }

    unsigned HandleVirtualKeys(_Inout_z_ const wchar_t* str)
    {
        static const struct Map { const wchar_t* first; unsigned second; } s_map[] =
        {
            { L"F1", VK_F1 },
            { L"F2", VK_F2 },
            { L"F3", VK_F3 },
            { L"F4", VK_F4 },
            { L"F5", VK_F5 },
            { L"F6", VK_F6 },
            { L"F7", VK_F7 },
            { L"F8", VK_F8 },
            { L"F9", VK_F9 },
            { L"F10", VK_F10 },
        };

        for(size_t j = 0; j < std::size(s_map); ++j)
        {
            if (_wcsicmp(s_map[j].first, str) == 0)
            {
                return s_map[j].second;
            }
        }

        if (*(str + 1) == 0)
        {
            return unsigned(*str);
        }

        return 0;
    }


    bool UpdateScrollBar(RECT& thumbRect, const RECT& trackRect, int position, int start, int end, int pageSize)
    {
        assert(pageSize > 0);
        if (end - start > pageSize)
        {
            const int height = (trackRect.bottom - trackRect.top);

            const int thumbHeight = std::max<int>( height * pageSize / (end - start), c_MinThumbSize);
            const int maxPos = end - start - pageSize;

            thumbRect.top = trackRect.top + (position - start) * (height - thumbHeight) / maxPos;
            thumbRect.bottom = thumbRect.top + thumbHeight;

            if (thumbRect.top < trackRect.top)
                thumbRect.top = trackRect.top;

            if (thumbRect.bottom > trackRect.bottom)
                thumbRect.bottom = trackRect.bottom;

            return true;
        }
        else
        {
            thumbRect.bottom = thumbRect.top;
            return false;
        }
    }


    void MessageYesNoCancel(const IPanel* panel, unsigned id)
    {
        assert(panel != 0);
        assert(panel->GetUser() != 0);

        auto const *cb = reinterpret_cast<std::function<void(bool, bool)>*>(panel->GetUser());

        if (id == 1)
        {
            (*cb)(true, false);
        }
        else if (id == 2)
        {
            (*cb)(false, false);
        }
        else
        {
            (*cb)(false, true);
        }
    }
}


//=====================================================================================
// UIManager
//=====================================================================================
class UIManager::Impl
{
public:
    Impl(const UIConfig& config) :
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        m_defaultTexDescriptor{},
        m_commandList(nullptr),
#endif
        m_fullscreen{},
        m_focusPanel(nullptr),
        m_overlayPanel(nullptr),
        m_hudPanel(nullptr),
        m_heldTimer(0),
        m_mouseLastX(-1),
        m_mouseLastY(-1),
        mConfig(config)
    {
        if (s_uiManager)
        {
            throw std::exception("UIManager is a singleton");
        }

        s_uiManager = this;
    }

    ~Impl()
    {
        for (auto& it : m_panels)
        {
            delete it.second;
        }
        m_panels.clear();

        m_focusPanel = nullptr;
        m_overlayPanel = nullptr;
        m_hudPanel = nullptr;

        s_uiManager = nullptr;
    }

    void LoadLayout(const wchar_t* layoutFile, const wchar_t* imageDir, unsigned offset)
    {
        static const wchar_t* s_seps = L" ,;|";
        static wchar_t s_text[4096] = {};

        DX::CSVReader reader(layoutFile, DX::CSVReader::Encoding::UTF8, true);

        IPanel* currentPanel = nullptr;

        if (imageDir)
            m_layoutImageDir = imageDir;
        else
            m_layoutImageDir.clear();

        while (!reader.EndOfFile())
        {
            // Each record starts with ITEM,ID,RECTX,RECTY,DX,DY
            wchar_t item[1024] = {};
            if (!reader.NextItem(item))
            {
                reader.NextRecord();
                continue;
            }

            TrimTrailingWhitespace(item);

            unsigned id;
            {
                // IPanel object ids need to be globally unique
                wchar_t tmp[16] = {};
                if (!reader.NextItem(tmp))
                {
                    DebugTrace("ERROR: Expected an id for record %zu\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }
                id = static_cast<unsigned>(_wtoi(tmp));
            }

            RECT rct = { 0, 0, 0, 0 };
            {
                wchar_t tmp[16] = {};
                if (!reader.NextItem(tmp))
                {
                    DebugTrace("ERROR: Expected an rectx for record %zu\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                rct.left = _wtoi(tmp);

                if (!reader.NextItem(tmp))
                {
                    DebugTrace("ERROR: Expected an recty for record %zu\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                rct.top = _wtoi(tmp);

                if (!reader.NextItem(tmp))
                {
                    DebugTrace("ERROR: Expected an dx for record %zu\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                rct.right = rct.left + _wtoi(tmp);

                if (!reader.NextItem(tmp))
                {
                    DebugTrace("ERROR: Expected an dy for record %zu\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                rct.bottom = rct.top + _wtoi(tmp);
            }

            // Create item
            if (_wcsicmp(item, L"POPUP") == 0 || _wcsicmp(item, L"CUSTOM_POPUP") == 0)
            {
                id += offset;

                auto it = m_panels.find(id);
                if (it != m_panels.end())
                {
                    DebugTrace("ERROR: Duplicate panel id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned int styleFlags = (_wcsicmp(item, L"CUSTOM_POPUP") == 0) ? c_styleCustomPanel : 0;
                wchar_t styleStr[128] = {};
                if (reader.NextItem(styleStr))
                {
                    _wcsupr_s(styleStr);

                    wchar_t* context = nullptr;
                    const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                    while (tok)
                    {
                        if (wcscmp(tok, L"EMPHASIS") == 0)
                        {
                            styleFlags |= c_stylePopupEmphasis;
                        }
                        else if (wcscmp(tok, L"SUPPRESS_CANCEL") == 0)
                        {
                            styleFlags |= c_styleSuppressCancel;
                        }
                        tok = wcstok_s(nullptr, s_seps, &context);
                    }
                }

                currentPanel = new Popup(rct, styleFlags);

                m_panels[id] = currentPanel;
            }
            else if (_wcsicmp(item, L"HUD") == 0)
            {
                id += offset;

                auto it = m_panels.find(id);
                if (it != m_panels.end())
                {
                    DebugTrace("ERROR: Duplicate panel id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                currentPanel = new HUD(rct);

                m_panels[id] = currentPanel;

                if (!m_hudPanel)
                {
                    currentPanel->Show();
                }
            }
            else if (_wcsicmp(item, L"OVERLAY") == 0 || _wcsicmp(item, L"CUSTOM_OVERLAY") == 0)
            {
                id += offset;

                auto it = m_panels.find(id);
                if (it != m_panels.end())
                {
                    DebugTrace("ERROR: Duplicate panel id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned int styleFlags = (_wcsicmp(item, L"CUSTOM_OVERLAY") == 0) ? c_styleCustomPanel : 0;
                wchar_t styleStr[128] = {};
                if (reader.NextItem(styleStr))
                {
                    _wcsupr_s(styleStr);

                    wchar_t *context = nullptr;
                    const wchar_t *tok = wcstok_s(styleStr, s_seps, &context);
                    while (tok)
                    {
                        if (wcscmp(tok, L"SUPPRESS_CANCEL") == 0)
                        {
                            styleFlags |= c_styleSuppressCancel;
                        }
                        tok = wcstok_s(nullptr, s_seps, &context);
                    }
                }

                currentPanel = new Overlay(rct, styleFlags);

                m_panels[id] = currentPanel;
            }
            else if (_wcsicmp(item, L"LABEL") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: LABEL found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (!reader.NextItem(s_text))
                {
                    DebugTrace("ERROR: LABEL missing text string [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                HandleEscapeCharacters(s_text);

                unsigned style = 0;
                XMVECTOR fgColor = Colors::White;
                XMVECTOR bgColor = Colors::Transparent;

                wchar_t styleStr[128] = {};
                if (reader.NextItem(styleStr))
                {
                    _wcsupr_s(styleStr);

                    wchar_t* context = nullptr;
                    const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                    while (tok)
                    {
                        if (wcscmp(tok, L"LEFT") == 0)
                        {
                            style |= TextLabel::c_StyleAlignLeft | TextLabel::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"CENTER") == 0)
                        {
                            style |= TextLabel::c_StyleAlignCenter | TextLabel::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"RIGHT") == 0)
                        {
                            style |= TextLabel::c_StyleAlignRight | TextLabel::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"LARGE") == 0)
                        {
                            style |= TextLabel::c_StyleFontLarge;
                        }
                        else if (wcscmp(tok, L"SMALL") == 0)
                        {
                            style |= TextLabel::c_StyleFontSmall;
                        }
                        else if (wcscmp(tok, L"BOLD") == 0)
                        {
                            style |= TextLabel::c_StyleFontBold;
                        }
                        else if (wcscmp(tok, L"ITALIC") == 0)
                        {
                            style |= TextLabel::c_StyleFontItalic;
                        }
                        else if (wcscmp(tok, L"TRANSPARENT") == 0)
                        {
                            style |= TextLabel::c_StyleTransparent;
                        }
                        else if (wcscmp(tok, L"WORDWRAP") == 0)
                        {
                            style |= TextLabel::c_StyleWordWrap;
                        }

                        tok = wcstok_s(nullptr, s_seps, &context);
                    }

                    wchar_t colorStr[32] = {};
                    if (reader.NextItem(colorStr))
                    {
                        _wcsupr_s(colorStr);
                        fgColor = LoadColorItem(colorStr, reader.RecordIndex());
                    }

                    if (reader.NextItem(colorStr))
                    {
                        _wcsupr_s(colorStr);
                        bgColor = LoadColorItem(colorStr, reader.RecordIndex());
                    }
                }

                if (id)
                {
                    // check for duplicates if nonzero ids used
                    if (currentPanel->Find(id))
                    {
                        DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                        throw std::exception("LoadLayout");
                    }
                }

                auto label = new TextLabel(id, s_text, rct, style);
                label->SetForegroundColor(fgColor);
                label->SetBackgroundColor(bgColor);
                currentPanel->Add(label);
            }
            else if (_wcsicmp(item, L"LEGEND") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: LEGEND found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (!reader.NextItem(s_text))
                {
                    DebugTrace("ERROR: LEGEND missing text string [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                HandleEscapeCharacters(s_text);

                unsigned style = 0;
                XMVECTOR fgColor = Colors::White;
                XMVECTOR bgColor = Colors::Transparent;

                wchar_t styleStr[128] = {};
                if (reader.NextItem(styleStr))
                {
                    _wcsupr_s(styleStr);

                    wchar_t* context = nullptr;
                    const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                    while (tok)
                    {
                        if (wcscmp(tok, L"LEFT") == 0)
                        {
                            style |= Legend::c_StyleAlignLeft | Legend::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"CENTER") == 0)
                        {
                            style |= Legend::c_StyleAlignCenter | Legend::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"RIGHT") == 0)
                        {
                            style |= Legend::c_StyleAlignRight | Legend::c_StyleAlignMiddle;
                        }
                        else if (wcscmp(tok, L"LARGE") == 0)
                        {
                            style |= Legend::c_StyleFontLarge;
                        }
                        else if (wcscmp(tok, L"SMALL") == 0)
                        {
                            style |= Legend::c_StyleFontSmall;
                        }
                        else if (wcscmp(tok, L"BOLD") == 0)
                        {
                            style |= Legend::c_StyleFontBold;
                        }
                        else if (wcscmp(tok, L"ITALIC") == 0)
                        {
                            style |= Legend::c_StyleFontItalic;
                        }
                        else if (wcscmp(tok, L"TRANSPARENT") == 0)
                        {
                            style |= Legend::c_StyleTransparent;
                        }

                        tok = wcstok_s(nullptr, s_seps, &context);
                    }

                    wchar_t colorStr[32] = {};
                    if (reader.NextItem(colorStr))
                    {
                        _wcsupr_s(colorStr);
                        fgColor = LoadColorItem(colorStr, reader.RecordIndex());
                    }

                    if (reader.NextItem(colorStr))
                    {
                        _wcsupr_s(colorStr);
                        bgColor = LoadColorItem(colorStr, reader.RecordIndex());
                    }
                }

                if (id)
                {
                    // check for duplicates if nonzero ids used
                    if (currentPanel->Find(id))
                    {
                        DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                        throw std::exception("LoadLayout");
                    }
                }

                auto legend = new Legend(id, s_text, rct, style);
                legend->SetForegroundColor(fgColor);
                legend->SetBackgroundColor(bgColor);
                currentPanel->Add(legend);
            }
            else if (_wcsicmp(item, L"IMAGE") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: IMAGE found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned imageId = 0;
                wchar_t imageFile[MAX_PATH] = {};
                if (reader.NextItem(imageFile))
                {
                    TrimTrailingWhitespace(imageFile);
                    imageId = LoadImageItem(imageFile);
                }
                else
                {
                    DebugTrace("ERROR: IMAGE missing image file name [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (id)
                {
                    // check for duplicates if nonzero ids used
                    if (currentPanel->Find(id))
                    {
                        DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                        throw std::exception("LoadLayout");
                    }
                }

                currentPanel->Add(new Image(id, imageId, rct));
            }
            else if (_wcsicmp(item, L"BUTTON") == 0 || _wcsicmp(item, L"EXITBUTTON") == 0 || _wcsicmp(item, L"DEFBUTTON") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: BUTTON found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (!reader.NextItem(s_text))
                {
                    DebugTrace("ERROR: BUTTON missing text string [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned hotkey = 0;
                wchar_t hotkeyStr[64] = {};
                unsigned style = 0;
                bool showBorder = false;
                bool noFocusColor = false;
                bool focusOnText = false;
                XMVECTOR color = Colors::Black;

                if (reader.NextItem(hotkeyStr))
                {
                    TrimTrailingWhitespace(hotkeyStr);

                    hotkey = HandleVirtualKeys(hotkeyStr);

                    // Optional style
                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"LARGE") == 0)
                            {
                                style |= Button::c_StyleFontLarge;
                            }
                            else if (wcscmp(tok, L"SMALL") == 0)
                            {
                                style |= Button::c_StyleFontSmall;
                            }
                            else if (wcscmp(tok, L"BOLD") == 0)
                            {
                                style |= Button::c_StyleFontBold;
                            }
                            else if (wcscmp(tok, L"ITALIC") == 0)
                            {
                                style |= Button::c_StyleFontItalic;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= Button::c_StyleTransparent;
                            }
                            else if (wcscmp(tok, L"BORDER") == 0)
                            {
                                showBorder = true;
                            }
                            else if (wcscmp(tok, L"NO_FOCUS_COLOR") == 0)
                            {
                                noFocusColor = true;
                            }
                            else if (wcscmp(tok, L"FOCUS_ON_TEXT") == 0)
                            {
                                focusOnText = true;
                            }
                            tok = wcstok_s(nullptr, s_seps, &context);
                        }

                        wchar_t colorStr[32] = {};
                        if (reader.NextItem(colorStr))
                        {
                            _wcsupr_s(colorStr);
                            color = LoadColorItem(colorStr, reader.RecordIndex());
                        }
                    }
                }

                if (_wcsicmp(item, L"EXITBUTTON") == 0)
                {
                    style |= Button::c_StyleExit;
                }
                else if (_wcsicmp(item, L"DEFBUTTON") == 0)
                {
                    style |= Button::c_StyleExit | Button::c_StyleDefault;
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                auto *butn = new Button(id, s_text, rct);
                butn->SetStyle(style);
                butn->SetHotKey(hotkey);
                butn->SetColor(color);
                butn->ShowBorder(showBorder);
                butn->NoFocusColor(noFocusColor);
                butn->FocusOnText(focusOnText);
                currentPanel->Add(butn);
            }
            else if (_wcsicmp(item, L"IMAGEBUTTON") == 0 || _wcsicmp(item, L"EXITIMAGEBUTTON") == 0 || _wcsicmp(item, L"DEFIMAGEBUTTON") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: IMAGEBUTTON found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned imageId = 0;
                wchar_t imageFile[MAX_PATH] = {};
                if (reader.NextItem(imageFile))
                {
                    TrimTrailingWhitespace(imageFile);
                    imageId = LoadImageItem(imageFile);
                }
                else
                {
                    DebugTrace("ERROR: IMAGEBUTTON missing image file name [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned hotkey = 0;
                wchar_t hotkeyStr[64] = {};
                unsigned style = 0;
                if (reader.NextItem(hotkeyStr))
                {
                    TrimTrailingWhitespace(hotkeyStr);

                    hotkey = HandleVirtualKeys(hotkeyStr);

                    // Optional style
                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"BACKGROUND") == 0)
                            {
                                style |= ImageButton::c_StyleBackground;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= ImageButton::c_StyleTransparent | ImageButton::c_StyleBackground;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                if (_wcsicmp(item, L"EXITIMAGEBUTTON") == 0)
                {
                    style |= ImageButton::c_StyleExit;
                }
                else if (_wcsicmp(item, L"DEFIMAGEBUTTON") == 0)
                {
                    style |= ImageButton::c_StyleExit | ImageButton::c_StyleDefault;
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                auto *butn = new ImageButton(id, imageId, rct);
                butn->SetStyle(style);
                butn->SetHotKey(hotkey);
                currentPanel->Add(butn);
            }
            else if (_wcsicmp(item, L"CHECKBOX") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: CHECKBOX found outside of panel [record %zu]", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (!reader.NextItem(s_text))
                {
                    DebugTrace("ERROR: CHECKBOX missing text string [record %zu]", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned style = 0;
                bool checked = false;
                {
                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"LARGE") == 0)
                            {
                                style |= CheckBox::c_StyleFontLarge;
                            }
                            else if (wcscmp(tok, L"SMALL") == 0)
                            {
                                style |= CheckBox::c_StyleFontSmall;
                            }
                            else if (wcscmp(tok, L"BOLD") == 0)
                            {
                                style |= CheckBox::c_StyleFontBold;
                            }
                            else if (wcscmp(tok, L"ITALIC") == 0)
                            {
                                style |= CheckBox::c_StyleFontItalic;
                            }
                            else if (wcscmp(tok, L"CHECKED") == 0)
                            {
                                checked = true;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= CheckBox::c_StyleTransparent;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control id found [record %zu]", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                auto *box = new CheckBox(id, s_text, rct, checked);
                box->SetStyle(style);
                currentPanel->Add(box);
            }
            else if (_wcsicmp(item, L"SLIDER") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: SLIDER found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                unsigned style = 0;
                {
                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= Slider::c_StyleTransparent;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                auto slider = new Slider(id, rct);
                slider->SetStyle(style);
                currentPanel->Add(slider);
            }
            else if (_wcsicmp(item, L"PROGRESSBAR") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: PROGRESSBAR found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                currentPanel->Add(new ProgressBar(id, rct));
            }
            else if (_wcsicmp(item, L"TEXTLIST") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: TEXTLIST found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                int itemHeight = 0;
                wchar_t itemHeightStr[64] = {};
                unsigned style = 0;
                if (reader.NextItem(itemHeightStr))
                {
                    itemHeight = _wtoi(itemHeightStr);

                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"LARGE") == 0)
                            {
                                style |= ListBox::c_StyleFontLarge;
                            }
                            else if (wcscmp(tok, L"SMALL") == 0)
                            {
                                style |= ListBox::c_StyleFontSmall;
                            }
                            else if (wcscmp(tok, L"BOLD") == 0)
                            {
                                style |= ListBox::c_StyleFontBold;
                            }
                            else if (wcscmp(tok, L"ITALIC") == 0)
                            {
                                style |= ListBox::c_StyleFontItalic;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= ListBox::c_StyleTransparent;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                currentPanel->Add(new TextList(id, rct, style, itemHeight));
            }
            else if (_wcsicmp(item, L"LISTBOX") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: LISTBOX found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                // check for duplicate id
                if (currentPanel->Find(id))
                {
                    DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                int itemHeight = 0;
                wchar_t itemHeightStr[64] = {};
                unsigned style = 0;
                if (reader.NextItem(itemHeightStr))
                {
                    itemHeight = _wtoi(itemHeightStr);

                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"MULTISELECT") == 0)
                            {
                                style |= ListBox::c_StyleMultiSelection;
                            }
                            else if (wcscmp(tok, L"LARGE") == 0)
                            {
                                style |= ListBox::c_StyleFontLarge;
                            }
                            else if (wcscmp(tok, L"SMALL") == 0)
                            {
                                style |= ListBox::c_StyleFontSmall;
                            }
                            else if (wcscmp(tok, L"BOLD") == 0)
                            {
                                style |= ListBox::c_StyleFontBold;
                            }
                            else if (wcscmp(tok, L"ITALIC") == 0)
                            {
                                style |= ListBox::c_StyleFontItalic;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= ListBox::c_StyleTransparent;
                            }
                            else if (wcscmp(tok, L"SCROLLBAR") == 0)
                            {
                                style |= ListBox::c_StyleScrollBar;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                currentPanel->Add(new ListBox(id, rct, style, itemHeight));
            }
            else if (_wcsicmp(item, L"TEXTBOX") == 0)
            {
                if (!currentPanel)
                {
                    DebugTrace("ERROR: TEXTBOX found outside of panel [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                if (id)
                {
                    // check for duplicates if nonzero ids used
                    if (currentPanel->Find(id))
                    {
                        DebugTrace("ERROR: Duplicate control id found [record %zu]\n", reader.RecordIndex() + 1);
                        throw std::exception("LoadLayout");
                    }
                }

                if (!reader.NextItem(s_text))
                {
                    DebugTrace("ERROR: TEXTBOX missing text string [record %zu]\n", reader.RecordIndex() + 1);
                    throw std::exception("LoadLayout");
                }

                HandleEscapeCharacters(s_text);

                unsigned style = 0;
                {
                    wchar_t styleStr[128] = {};
                    if (reader.NextItem(styleStr))
                    {
                        _wcsupr_s(styleStr);

                        wchar_t* context = nullptr;
                        const wchar_t* tok = wcstok_s(styleStr, s_seps, &context);
                        while (tok)
                        {
                            if (wcscmp(tok, L"LARGE") == 0)
                            {
                                style |= TextBox::c_StyleFontLarge;
                            }
                            else if (wcscmp(tok, L"SMALL") == 0)
                            {
                                style |= TextBox::c_StyleFontSmall;
                            }
                            else if (wcscmp(tok, L"BOLD") == 0)
                            {
                                style |= TextBox::c_StyleFontBold;
                            }
                            else if (wcscmp(tok, L"ITALIC") == 0)
                            {
                                style |= TextBox::c_StyleFontItalic;
                            }
                            else if (wcscmp(tok, L"TRANSPARENT") == 0)
                            {
                                style |= TextBox::c_StyleTransparent;
                            }
                            else if (wcscmp(tok, L"SCROLLBAR") == 0)
                            {
                                style |= TextBox::c_StyleScrollBar;
                            }
                            else if (wcscmp(tok, L"NOBACKGROUND") == 0)
                            {
                                style |= TextBox::c_StyleNoBackground;
                            }

                            tok = wcstok_s(nullptr, s_seps, &context);
                        }
                    }
                }

                currentPanel->Add(new TextBox(id, s_text, rct, style));
            }

            reader.NextRecord();
        }
    }

    bool Update(float elapsedTime, const GamePad::State& pad)
    {
        if (m_heldTimer >= 0)
        {
            m_heldTimer -= elapsedTime;
        }

        m_padButtonState.Update(pad);

        bool result = false;

        if (m_focusPanel)
        {
            // Focus panel gets input processing
            result = m_focusPanel->Update(elapsedTime, pad);
        }

        for (auto& it : m_panels)
        {
            if (it.second == m_overlayPanel || it.second == m_focusPanel)
                continue;

            if (it.second->IsVisible())
            {
                it.second->Update(elapsedTime);
            }
        }

        // Overlay gets processed last
        if (m_overlayPanel)
        {
            if (!result)
            {
                result = m_overlayPanel->Update(elapsedTime, pad);
            }
            else
            {
                m_overlayPanel->Update(elapsedTime);
            }
        }

        // m_hudPanel never gets input, but does get Update for time

        return result;
    }

    bool Update(float elapsedTime, const Mouse& mouse, const Keyboard& kb)
    {
        if (m_heldTimer >= 0)
        {
            m_heldTimer -= elapsedTime;
        }

        auto const mstate = mouse.GetState();
        m_mouseButtonState.Update(mstate);

        auto const kbstate = kb.GetState();
        m_keyboardState.Update(kbstate);

        bool result = false;

        if (m_focusPanel)
        {
            // Focus panel gets input processing
            result = m_focusPanel->Update(elapsedTime, mstate, kbstate);
        }

        for (auto& it : m_panels)
        {
            if (it.second == m_overlayPanel || it.second == m_focusPanel)
                continue;

            if (it.second->IsVisible())
            {
                if (!result)
                {
                    result = it.second->Update(elapsedTime, mstate, kbstate);
                }
                else
                {
                    it.second->Update(elapsedTime);
                }
            }
        }

        // Overlay is last (it's below all other panels)
        if (m_overlayPanel)
        {
            if (!result)
            {
                result = m_overlayPanel->Update(elapsedTime, mstate, kbstate);
            }
            else
            {
                m_overlayPanel->Update(elapsedTime);
            }
        }

        if (mstate.positionMode == Mouse::MODE_ABSOLUTE)
        {
            m_mouseLastX = mstate.x;
            m_mouseLastY = mstate.y;
        }
        else
        {
            m_mouseLastX = m_mouseLastY = -1;
        }

        // m_hudPanel never gets input, but does get Update for time

        return result;
    }

    void Render()
    {
        // HUD panel on buton
        if (m_hudPanel)
        {
            m_hudPanel->Render();
        }

        // Overlay panel next
        if (m_overlayPanel)
        {
            m_overlayPanel->Render();
        }

        // Non-focus visible panels
        for (auto& it : m_panels)
        {
            if (it.second == m_hudPanel || it.second == m_overlayPanel || it.second == m_focusPanel)
                continue;

            if (it.second->IsVisible())
            {
                it.second->Render();
            }
        }

        // Focus panel on top
        if (m_focusPanel)
        {
            m_focusPanel->Render();
        }
    }

    void ReleaseDevice()
    {
        m_images.clear();
        m_batch.reset();
        m_smallFont.reset();
        m_smallItalicFont.reset();
        m_smallBoldFont.reset();
        m_midFont.reset();
        m_midItalicFont.reset();
        m_midBoldFont.reset();
        m_largeFont.reset();
        m_largeItalicFont.reset();
        m_largeBoldFont.reset();
        m_smallLegend.reset();
        m_largeLegend.reset();
        m_fxFactory.reset();
        m_defaultTex.Reset();
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
        m_blendState.Reset();
        m_scissorState.Reset();
        m_context.Reset();
#endif
    }

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    void RestoreDevice(_In_ ID3D12Device* device, const RenderTargetState& renderTarget, ResourceUploadBatch& resourceUpload, DescriptorPile& pile)
    {
        {
            SpriteBatchPipelineStateDescription pd(renderTarget, (mConfig.pmAlpha) ? &CommonStates::AlphaBlend : &CommonStates::NonPremultiplied);

            m_batch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
        }

        // Create 1x1 white default texture
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Width = 1;
        texDesc.Height = 1;
        texDesc.MipLevels = texDesc.DepthOrArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_defaultTex.GetAddressOf())));

        static constexpr uint32_t s_pixel = 0xffffffff;

        D3D12_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        resourceUpload.Upload(m_defaultTex.Get(), 0, &initData, 1);

        resourceUpload.Transition(
            m_defaultTex.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        size_t index = pile.Allocate();
        device->CreateShaderResourceView(m_defaultTex.Get(), nullptr, pile.GetCpuHandle(index));

        m_defaultTexDescriptor = pile.GetGpuHandle(index);

        // Create fonts
        wchar_t buff[MAX_PATH] = {};

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallFontName);
        index = pile.Allocate();
        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallItalicFontName);
        index = pile.Allocate();
        m_smallItalicFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallBoldFontName);
        index = pile.Allocate();
        m_smallBoldFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midFontName);
        index = pile.Allocate();
        m_midFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midItalicFontName);
        index = pile.Allocate();
        m_midItalicFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midBoldFontName);
        index = pile.Allocate();
        m_midBoldFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeFontName);
        index = pile.Allocate();
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeItalicFontName);
        index = pile.Allocate();
        m_largeItalicFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeBoldFontName);
        index = pile.Allocate();
        m_largeBoldFont = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeLegendName);
        index = pile.Allocate();
        m_largeLegend = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallLegendName);
        index = pile.Allocate();
        m_smallLegend = std::make_unique<SpriteFont>(device, resourceUpload, buff, pile.GetCpuHandle(index), pile.GetGpuHandle(index));

        m_smallFont->SetDefaultCharacter(L'?');
        m_smallBoldFont->SetDefaultCharacter(L'?');
        m_midFont->SetDefaultCharacter(L'?');
        m_midItalicFont->SetDefaultCharacter(L'?');
        m_midBoldFont->SetDefaultCharacter(L'?');
        m_largeFont->SetDefaultCharacter(L'?');
        m_largeItalicFont->SetDefaultCharacter(L'?');
        m_largeBoldFont->SetDefaultCharacter(L'?');

        // Create factory for images
        m_fxFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, pile.Heap());
        m_fxFactory->SetDirectory(m_layoutImageDir.c_str());
        m_fxFactory->EnableForceSRGB(mConfig.forceSRGB);

        unsigned imageId = c_LayoutImageIdStart;
        if (!m_layoutImages.empty())
        {
            for (auto& it : m_layoutImages)
            {
                index = pile.Allocate();
                const size_t slot = m_fxFactory->CreateTexture(it.c_str(), static_cast<int>(index));

                ComPtr<ID3D12Resource> tex;
                m_fxFactory->GetResource(slot, tex.GetAddressOf());

                std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, XMUINT2> entry;
                entry.first = pile.GetGpuHandle(index);
                entry.second = GetTextureSize(tex.Get());
                m_images[imageId] = entry;
                ++imageId;
            }
        }
    }
#else
    void RestoreDevice(_In_ ID3D11DeviceContext* context)
    {
        m_batch = std::make_unique<SpriteBatch>(context);

        ComPtr<ID3D11Device> device;
        context->GetDevice(device.GetAddressOf());

        // Create 1x1 white default texture
        {
            static constexpr uint32_t s_pixel = 0xffffffff;

            D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

            CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

            ComPtr<ID3D11Texture2D> tex;
            DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, &initData, tex.GetAddressOf()));

            DX::ThrowIfFailed(device->CreateShaderResourceView(tex.Get(), nullptr, m_defaultTex.ReleaseAndGetAddressOf()));
        }

        // Create blend state for sprites
        {
            CD3D11_DEFAULT def;
            CD3D11_BLEND_DESC desc(def);
            desc.RenderTarget[0].BlendEnable = TRUE;
            desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = (mConfig.pmAlpha) ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            DX::ThrowIfFailed(device->CreateBlendState(&desc, m_blendState.ReleaseAndGetAddressOf()));
        }

        // Create scissor rasterizer state for sprites
        {
            CD3D11_RASTERIZER_DESC rsDesc(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, 0, 0.f, 0.f, TRUE, TRUE, TRUE, FALSE);
            DX::ThrowIfFailed(device->CreateRasterizerState(&rsDesc, m_scissorState.ReleaseAndGetAddressOf()));
        }

        // Create fonts
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        wchar_t buff[MAX_PATH] = {};

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallFontName);
        m_smallFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallItalicFontName);
        m_smallItalicFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallBoldFontName);
        m_smallBoldFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midFontName);
        m_midFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midItalicFontName);
        m_midItalicFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.midBoldFontName);
        m_midBoldFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeFontName);
        m_largeFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeItalicFontName);
        m_largeItalicFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeBoldFontName);
        m_largeBoldFont = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.largeLegendName);
        m_largeLegend = std::make_unique<SpriteFont>(device.Get(), buff);

        DX::FindMediaFile(buff, MAX_PATH, mConfig.smallLegendName);
        m_smallLegend = std::make_unique<SpriteFont>(device.Get(), buff);
#else
        m_smallFont = std::make_unique<SpriteFont>(device.Get(), mConfig.smallFontName);
        m_smallItalicFont = std::make_unique<SpriteFont>(device.Get(), mConfig.smallItalicFontName);
        m_smallBoldFont = std::make_unique<SpriteFont>(device.Get(), mConfig.smallBoldFontName);

        m_midFont = std::make_unique<SpriteFont>(device.Get(), mConfig.midFontName);
        m_midItalicFont = std::make_unique<SpriteFont>(device.Get(), mConfig.midItalicFontName);
        m_midBoldFont = std::make_unique<SpriteFont>(device.Get(), mConfig.midBoldFontName);

        m_largeFont = std::make_unique<SpriteFont>(device.Get(), mConfig.largeFontName);
        m_largeItalicFont = std::make_unique<SpriteFont>(device.Get(), mConfig.largeItalicFontName);
        m_largeBoldFont = std::make_unique<SpriteFont>(device.Get(), mConfig.largeBoldFontName);

        m_largeLegend = std::make_unique<SpriteFont>(device.Get(), mConfig.largeLegendName);
        m_smallLegend = std::make_unique<SpriteFont>(device.Get(), mConfig.smallLegendName);

#endif
        m_smallFont->SetDefaultCharacter(L'?');
        m_smallBoldFont->SetDefaultCharacter(L'?');
        m_midFont->SetDefaultCharacter(L'?');
        m_midItalicFont->SetDefaultCharacter(L'?');
        m_midBoldFont->SetDefaultCharacter(L'?');
        m_largeFont->SetDefaultCharacter(L'?');
        m_largeItalicFont->SetDefaultCharacter(L'?');
        m_largeBoldFont->SetDefaultCharacter(L'?');

        // Create factory for images
        m_fxFactory = std::make_unique<EffectFactory>(device.Get());
        m_fxFactory->SetDirectory(m_layoutImageDir.c_str());
        m_fxFactory->EnableForceSRGB(mConfig.forceSRGB);

        unsigned imageId = c_LayoutImageIdStart;
        if (!m_layoutImages.empty())
        {
            for (auto& it : m_layoutImages)
            {
                ComPtr<ID3D11ShaderResourceView> tex;
                m_fxFactory->CreateTexture(it.c_str(), context, tex.GetAddressOf());
                m_images[imageId] = tex;
                ++imageId;
            }
        }

        m_context = context;
    }
#endif

    void Reset()
    {
        m_padButtonState.Reset();
        m_mouseButtonState.Reset();
        m_keyboardState.Reset();
        m_mouseLastX = m_mouseLastX = -1;
        m_heldTimer = 0;
    }

    SpriteFont* SelectFont(unsigned style)
    {
        static_assert(TextLabel::c_StyleFontSmall == Legend::c_StyleFontSmall, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontMid == Legend::c_StyleFontMid, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontLarge == Legend::c_StyleFontLarge, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontBold == Legend::c_StyleFontBold, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontItalic == Legend::c_StyleFontItalic, "style flags mismatch");

        static_assert(TextLabel::c_StyleFontSmall == Button::c_StyleFontSmall, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontMid == Button::c_StyleFontMid, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontLarge == Button::c_StyleFontLarge, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontBold == Button::c_StyleFontBold, "style flags mismatch");
        static_assert(TextLabel::c_StyleFontItalic == Button::c_StyleFontItalic, "style flags mismatch");

        SpriteFont* font = nullptr;

        if (style & TextLabel::c_StyleFontLarge)
        {
            if (style & TextLabel::c_StyleFontBold)
            {
                font = m_largeBoldFont.get();
            }
            else if (style & TextLabel::c_StyleFontItalic)
            {
                font = m_largeItalicFont.get();
            }
            else
            {
                font = m_largeFont.get();
            }
        }
        else if (style & TextLabel::c_StyleFontSmall)
        {
            if (style & TextLabel::c_StyleFontBold)
            {
                font = m_smallBoldFont.get();
            }
            else if (style & TextLabel::c_StyleFontItalic)
            {
                font = m_smallItalicFont.get();
            }
            else
            {
                font = m_smallFont.get();
            }
        }
        else if (style & TextLabel::c_StyleFontBold)
        {
            font = m_midBoldFont.get();
        }
        else if (style & TextLabel::c_StyleFontItalic)
        {
            font = m_midItalicFont.get();
        }
        else
        {
            font = m_midFont.get();
        }
        
        assert(font != 0);

        return font;
    }

    // Direct3D resources
    std::unique_ptr<SpriteBatch>        m_batch;
    std::unique_ptr<SpriteFont>         m_smallFont;
    std::unique_ptr<SpriteFont>         m_smallItalicFont;
    std::unique_ptr<SpriteFont>         m_smallBoldFont;
    std::unique_ptr<SpriteFont>         m_midFont;
    std::unique_ptr<SpriteFont>         m_midItalicFont;
    std::unique_ptr<SpriteFont>         m_midBoldFont;
    std::unique_ptr<SpriteFont>         m_largeFont;
    std::unique_ptr<SpriteFont>         m_largeItalicFont;
    std::unique_ptr<SpriteFont>         m_largeBoldFont;
    std::unique_ptr<SpriteFont>         m_smallLegend;
    std::unique_ptr<SpriteFont>         m_largeLegend;

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    std::unique_ptr<EffectTextureFactory>   m_fxFactory;
    D3D12_GPU_DESCRIPTOR_HANDLE             m_defaultTexDescriptor;
    ID3D12GraphicsCommandList*              m_commandList;
    ComPtr<ID3D12Resource>                  m_defaultTex;

    std::map<unsigned, std::pair<D3D12_GPU_DESCRIPTOR_HANDLE,DirectX::XMUINT2>> m_images;
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
    std::unique_ptr<EffectFactory>      m_fxFactory;
    ComPtr<ID3D11ShaderResourceView>    m_defaultTex;
    ComPtr<ID3D11BlendState>            m_blendState;
    ComPtr<ID3D11RasterizerState>       m_scissorState;
    ComPtr<ID3D11DeviceContext>         m_context;

    std::map<unsigned, ComPtr<ID3D11ShaderResourceView>> m_images;
#endif

    // UI objects
    RECT                                m_fullscreen;
    IPanel*                             m_focusPanel;
    IPanel*                             m_overlayPanel;
    IPanel*                             m_hudPanel;
    std::map<unsigned, IPanel*>         m_panels;

    std::vector<std::wstring>           m_layoutImages;
    std::wstring                        m_layoutImageDir;

    GamePad::ButtonStateTracker         m_padButtonState;
    Mouse::ButtonStateTracker           m_mouseButtonState;
    Keyboard::KeyboardStateTracker      m_keyboardState;
    float                               m_heldTimer;

    int                                 m_mouseLastX;
    int                                 m_mouseLastY;

    // Common handler objects
    std::list<std::function<void(bool,bool)>> m_stdyesno;

    // Contains configuration data provided on setup
    const UIConfig mConfig;

    static UIManager::Impl* s_uiManager;

    // Helpers
    void RenderControls(std::vector<IControl*>& controls)
    {
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        if (m_commandList == nullptr || m_batch == nullptr)
            return;

        for (auto it : controls)
        {
            if (it->IsVisible())
            {
                m_commandList->RSSetScissorRects(1, it->GetRectangle());

                m_batch->Begin(m_commandList);

                it->Render();

                m_batch->End();
            }
        }
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
        if (m_context == nullptr || m_batch == nullptr)
            return;

        for (auto it : controls)
        {
            if (it->IsVisible())
            {
                m_batch->Begin(SpriteSortMode_Deferred, m_blendState.Get(), nullptr, nullptr, m_scissorState.Get(), [=]()
                {
                    m_context->RSSetScissorRects(1, it->GetRectangle());
                });

                it->Render();

                m_batch->End();
            }
        }
#endif
    }

    void DrawRect(const RECT &rect, FXMVECTOR color)
    {
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        if (m_commandList == nullptr || m_batch == nullptr)
            return;

        static const XMUINT2 s_unit(1, 1);

        m_batch->Draw(m_defaultTexDescriptor, s_unit, rect, color);
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
        if (m_context == nullptr || m_batch == nullptr)
            return;

        m_batch->Draw(m_defaultTex.Get(), rect, color);
#endif
    }

    void DrawImage(unsigned id, const RECT &rect, FXMVECTOR color)
    {
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        D3D12_GPU_DESCRIPTOR_HANDLE tex;
        XMUINT2 texSize;

        auto it = m_images.find(id);
        if (it == m_images.end())
        {
            // If not found, use default texture
            tex = m_defaultTexDescriptor;
            texSize.x = texSize.y = 1;
        }
        else
        {
            tex = it->second.first;
            texSize = it->second.second;
        }

        m_batch->Draw(tex, texSize, rect, color);
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
        ID3D11ShaderResourceView* tex = nullptr;

        auto it = m_images.find(id);
        if (it == m_images.end())
        {
            // If not found, use default texture
            tex = m_defaultTex.Get();
        }
        else
        {
            tex = it->second.Get();
        }

        m_batch->Draw(tex, rect, color);
#endif
    }

private:
    unsigned LoadImageItem(_In_z_ const wchar_t* imageFile)
    {
        assert(imageFile != 0);

        unsigned imageId = 0;

        std::wstring wname(imageFile);
        auto it = std::find(m_layoutImages.cbegin(), m_layoutImages.cend(), wname);
        if (it == m_layoutImages.end())
        {
            imageId = c_LayoutImageIdStart + unsigned(m_layoutImages.size());
            m_layoutImages.push_back(wname);
        }
        else
        {
            imageId = c_LayoutImageIdStart + unsigned(it - m_layoutImages.cbegin());
        }

        return imageId;
    }

    XMVECTOR XM_CALLCONV LoadColorItem(_In_z_ const wchar_t* colorStr, size_t index)
    {
        assert(colorStr != 0);
        XMVECTOR color = Colors::White;

        if (*colorStr == L'#')
        {
            unsigned int bgra = 0xFFFFFF;
            if (swscanf_s(colorStr + 1, L"%x", &bgra) == 1)
            {
                using namespace PackedVector;

                bgra &= 0xFFFFFF;
                const XMVECTOR clr = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&bgra));
                color = XMVectorSelect(g_XMIdentityR3, clr, g_XMSelect1110);

                if (mConfig.forceSRGB)
                {
                    color = XMColorSRGBToRGB(color);
                }
            }
            else
            {
                DebugTrace("WARNING: Invalid color value [record %zu]\n", index + 1);
            }
        }
        else if (*colorStr)
        {
            if (wcscmp(colorStr, L"RED") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::RED]);
            }
            else if (wcscmp(colorStr, L"GREEN") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::GREEN]);
            }
            else if (wcscmp(colorStr, L"BLUE") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::BLUE]);
            }
            else if (wcscmp(colorStr, L"ORANGE") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::ORANGE]);
            }
            else if (wcscmp(colorStr, L"YELLOW") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::YELLOW]);
            }
            else if (wcscmp(colorStr, L"DARKGREY") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::DARK_GREY]);
            }
            else if (wcscmp(colorStr, L"LIGHTGREY") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::LIGHT_GREY]);
            }
            else if (wcscmp(colorStr, L"OFFWHITE") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::OFF_WHITE]);
            }
            else if (wcscmp(colorStr, L"WHITE") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::WHITE]);
            }
            else if (wcscmp(colorStr, L"BLACK") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::BLACK]);
            }
            else if (wcscmp(colorStr, L"MID_GREY") == 0)
            {
                color = XMLoadFloat4(&mConfig.colorDictionary[UIConfig::MID_GREY]);
            }
            else
            {
                DebugTrace("WARNING: Invalid color value [record %zu]\n", index + 1);
            }
        }

        return color;
    }
};

UIManager::Impl* UIManager::Impl::s_uiManager = nullptr;

// Public constructors
UIManager::UIManager(const UIConfig& config) :
    pImpl(new Impl(config))
{
}

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
UIManager::UIManager(
    _In_ ID3D12Device *device,
    const RenderTargetState& renderTarget,
    ResourceUploadBatch& resourceUpload,
    DescriptorPile& pile,
    const UIConfig& config) :
        pImpl(new Impl(config))
{
    pImpl->RestoreDevice(device, renderTarget, resourceUpload, pile);
}
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
UIManager::UIManager(_In_ ID3D11DeviceContext* context, const UIConfig& config) :
    pImpl(new Impl(config))
{
    pImpl->RestoreDevice(context);
}
#endif


UIManager::UIManager(UIManager&&) noexcept = default;
UIManager& UIManager::operator= (UIManager&&) noexcept = default;
UIManager::~UIManager() = default;


// Public methods.
void UIManager::LoadLayout(const wchar_t* layoutFile, const wchar_t* imageDir, unsigned offset)
{
    pImpl->LoadLayout(layoutFile, imageDir, offset);
}

void UIManager::Add(unsigned id, _In_ IPanel* panel)
{
    auto it = pImpl->m_panels.find(id);
    if (it != pImpl->m_panels.end())
    {
        if (pImpl->m_focusPanel == it->second)
        {
            // For now, no focus stack
            pImpl->m_focusPanel = nullptr;
        }
        if (pImpl->m_overlayPanel == it->second)
        {
            pImpl->m_overlayPanel = nullptr;
        }
        if (pImpl->m_hudPanel == it->second)
        {
            pImpl->m_hudPanel = nullptr;
        }
        delete it->second;
    }

    pImpl->m_panels[id] = panel;
}

IPanel* UIManager::Find(unsigned id) const
{
    auto it = pImpl->m_panels.find(id);
    if (it != pImpl->m_panels.end())
        return it->second;

    return nullptr;
}

void UIManager::CloseAll()
{
    std::for_each(pImpl->m_panels.begin(), pImpl->m_panels.end(), [](auto pair)
    {
        auto panel = pair.second;
        if (panel->IsVisible())
            panel->Close();
    });
}

bool UIManager::Update(float elapsedTime, const GamePad::State& pad)
{
    return pImpl->Update(elapsedTime, pad);
}

bool UIManager::Update(float elapsedTime, const Mouse& mouse, const Keyboard& kb)
{
    return pImpl->Update(elapsedTime, mouse, kb);
}

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
void UIManager::Render(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->m_commandList = commandList;
    pImpl->Render();
    pImpl->m_commandList = nullptr;
}
#else
void UIManager::Render()
{
    pImpl->Render();
}
#endif

void UIManager::SetWindow(const RECT& layout)
{
    pImpl->m_fullscreen = layout;

    for (auto it : pImpl->m_panels)
    {
        if (it.second->IsVisible())
        {
            it.second->OnWindowSize(layout);
        }
    }

    auto width = std::max<LONG>(layout.right - layout.left, 1);
    auto height = std::max<LONG>(layout.bottom - layout.top, 1);

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
    D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
#endif
    pImpl->m_batch->SetViewport(vp);
}

void UIManager::SetRotation(DXGI_MODE_ROTATION rotation)
{
    if (pImpl->m_batch)
    {
        pImpl->m_batch->SetRotation(rotation);
    }
}

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
void UIManager::RegisterImage(unsigned id, D3D12_GPU_DESCRIPTOR_HANDLE tex, XMUINT2 texSize)
{
    std::pair<D3D12_GPU_DESCRIPTOR_HANDLE, XMUINT2> entry;
    entry.first = tex;
    entry.second = texSize;
    pImpl->m_images[id] = entry;
}
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
void UIManager::RegisterImage(unsigned id, _In_ ID3D11ShaderResourceView* tex)
{
    pImpl->m_images[id] = tex;
}
#endif

void UIManager::UnregisterImage(unsigned id)
{
    auto it = pImpl->m_images.find(id);
    if (it != pImpl->m_images.end())
    {
        pImpl->m_images.erase(it);
    }
}

void UIManager::UnregisterAllImages()
{
    pImpl->m_images.clear();
}

void UIManager::ReleaseDevice()
{
    pImpl->ReleaseDevice();
}

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
void UIManager::RestoreDevice(
    _In_ ID3D12Device* device,
    const DirectX::RenderTargetState& renderTarget,
    DirectX::ResourceUploadBatch& resourceUpload,
    DirectX::DescriptorPile& pile)
{
    pImpl->RestoreDevice(device, renderTarget, resourceUpload, pile);
}
#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)
void UIManager::RestoreDevice(_In_ ID3D11DeviceContext* context)
{
    pImpl->RestoreDevice(context);
}
#endif

void UIManager::Reset()
{
    pImpl->Reset();
}

void UIManager::Clear()
{
    const UIConfig config = pImpl->mConfig;
    pImpl.reset(); // have to release singleton first
    pImpl.reset(new Impl(config));
}

void UIManager::Enumerate(std::function<void(unsigned id, IPanel*)> enumCallback)
{
    for (auto it = pImpl->m_panels.begin(); it != pImpl->m_panels.end(); ++it)
    {
        enumCallback(it->first, it->second);
    }
}

// Common callback adapters.
void UIManager::CallbackYesNoCancel(_In_ IPanel* panel, std::function<void(bool,bool)> callback)
{
    assert(panel != 0);
    assert(panel->GetUser() == nullptr);

    pImpl->m_stdyesno.push_back(callback);
    panel->SetUser(&pImpl->m_stdyesno.back());
    panel->SetCallback(MessageYesNoCancel);
}


//=====================================================================================
// Base control
//=====================================================================================
void IControl::ComputeLayout(const RECT& parent)
{
    m_screenRect.top = m_layoutRect.top + parent.top;
    m_screenRect.bottom = m_layoutRect.bottom + parent.top;

    m_screenRect.left = m_layoutRect.left + parent.left;
    m_screenRect.right = m_layoutRect.right + parent.left;

    // Constrain to parent bound
    if (m_screenRect.top < parent.top)
        m_screenRect.top = parent.top;

    if (m_screenRect.left < parent.left)
        m_screenRect.left = parent.left;

    if (m_screenRect.right > parent.right)
        m_screenRect.right = parent.right;

    if (m_screenRect.bottom > parent.bottom)
        m_screenRect.bottom = parent.bottom;
}

void IControl::ComputeLayout(const RECT& bounds, float dx, float dy)
{
    m_screenRect.left = long(float(m_layoutRect.left) * dx);
    m_screenRect.right = long(float(m_layoutRect.right ) * dx);

    m_screenRect.top = long(float(m_layoutRect.top) * dy);
    m_screenRect.bottom = long(float(m_layoutRect.bottom) * dy);

    // Add any offset
    m_screenRect.top = m_screenRect.top + bounds.top;
    m_screenRect.bottom = m_screenRect.bottom + bounds.top;

    m_screenRect.left = m_screenRect.left + bounds.left;
    m_screenRect.right = m_screenRect.right + bounds.left;

    // Constrain to bound
    if (m_screenRect.top < bounds.top)
        m_screenRect.top = bounds.top;

    if (m_screenRect.left < bounds.left)
        m_screenRect.left = bounds.left;

    if (m_screenRect.right > bounds.right)
        m_screenRect.right = bounds.right;

    if (m_screenRect.bottom > bounds.bottom)
        m_screenRect.bottom = bounds.bottom;
}

void IControl::SetVisible(bool visible)
{
#ifdef _DEBUG
    if (!visible && m_focus)
    {
        OutputDebugStringA("WARNING: Control made invisible while it was in focus!\n");
    }
#endif

    m_visible = visible;
}


//=====================================================================================
// Text static label control
//=====================================================================================
_Use_decl_annotations_
TextLabel::TextLabel(unsigned id, const wchar_t* text, const RECT& rect, unsigned style) :
    IControl(rect, id),
    m_style(style),
    m_text(text)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_fgColor = mgr->mConfig.colorNormal;
    m_bgColor = mgr->mConfig.colorBackground;
}

void TextLabel::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Determine font
    const SpriteFont* font = mgr->SelectFont(m_style);

    const wchar_t * text = m_text.c_str();

    if (m_style & c_StyleWordWrap)
    {
        if (m_wordWrap.empty()
            && (m_screenRect.right != 0 && m_screenRect.bottom != 0
                && m_screenRect.left != m_screenRect.right
                && m_screenRect.top != m_screenRect.bottom))
        {
            m_wordWrap = WordWrap(text, font, m_screenRect);
        }

        text = m_wordWrap.c_str();
    }
        
    // Determine layout
    XMFLOAT2 pos(float(m_screenRect.left), float(m_screenRect.top));
    if (m_style & (c_StyleAlignCenter | c_StyleAlignRight | c_StyleAlignMiddle | c_StyleAlignBottom))
    {
        const XMVECTOR fsize = font->MeasureString(text);

        if (m_style & c_StyleAlignCenter)
        {
            pos.x = float(m_screenRect.left) + float((m_screenRect.right - m_screenRect.left) / 2) - XMVectorGetX(fsize) / 2.f;
            if (pos.x < float(m_screenRect.left))
                pos.x = float(m_screenRect.left);
        }
        else if (m_style & c_StyleAlignRight)
        {
            pos.x = float(m_screenRect.right - 1) - XMVectorGetX(fsize);
            if (pos.x < float(m_screenRect.left))
                pos.x = float(m_screenRect.left);
        }

        if (m_style & c_StyleAlignMiddle)
        {
            pos.y = float(m_screenRect.top) + float((m_screenRect.bottom - m_screenRect.top) / 2) - XMVectorGetY(fsize) / 2.f;
            if (pos.y < float(m_screenRect.top))
                pos.y = float(m_screenRect.top);
        }
        else if (m_style & c_StyleAlignBottom)
        {
            pos.y = float(m_screenRect.bottom - 1) - XMVectorGetY(fsize);
            if (pos.y < float(m_screenRect.top))
                pos.y = float(m_screenRect.top);
        }
    }

    // Draw
    if (m_bgColor.w != 0.f)
    {
        const XMVECTOR bgColor = m_style & c_StyleTransparent ?
            XMLoadFloat4(&mgr->mConfig.colorTransparent) :
            XMLoadFloat4(&m_bgColor);

        mgr->DrawRect(m_screenRect, bgColor);
    }
    
    const XMVECTOR color = XMLoadFloat4(&m_fgColor);

    font->DrawString(mgr->m_batch.get(), text, pos, color);
}

void TextLabel::SetText(const wchar_t* text)
{
    m_text = text;
    m_wordWrap.clear();
}

void TextLabel::ComputeLayout(const RECT& parent)
{
    m_wordWrap.clear();
    IControl::ComputeLayout(parent);
}

void TextLabel::ComputeLayout(const RECT& bounds, float dx, float dy)
{
    m_wordWrap.clear();
    IControl::ComputeLayout(bounds, dx, dy);
}



//=====================================================================================
// Image static control
//=====================================================================================
Image::Image(unsigned id, unsigned imageId, const RECT& rect) :
    IControl(rect, id),
    m_imageId(imageId)
{
}

void Image::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    mgr->DrawImage(m_imageId, m_screenRect, Colors::White);
}


//=====================================================================================
// Static text label that supports the controller font
//=====================================================================================
_Use_decl_annotations_
Legend::Legend(unsigned id, const wchar_t* text, const RECT& rect, unsigned style) :
    IControl(rect, id),
    m_style(style),
    m_text(text)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_fgColor = mgr->mConfig.colorNormal;
    m_bgColor = mgr->mConfig.colorBackground;
}

void Legend::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Determine font
    const SpriteFont* font = mgr->SelectFont(m_style);

    SpriteFont* ctrlFont = nullptr;
    if (m_style & c_StyleFontLarge)
    {
        ctrlFont = mgr->m_largeLegend.get();
    }
    else if (m_style & c_StyleFontSmall)
    {
        ctrlFont = mgr->m_smallLegend.get();
    }
    else
    {
        ctrlFont = mgr->m_smallLegend.get();
    }

    assert(ctrlFont != 0);

    // Determine layout
    XMFLOAT2 pos(float(m_screenRect.left), float(m_screenRect.top));
    if (m_style & (c_StyleAlignCenter | c_StyleAlignRight | c_StyleAlignMiddle | c_StyleAlignBottom))
    {
        const RECT rect = DX::MeasureControllerDrawBounds(font, ctrlFont, m_text.c_str(), pos);

        if (m_style & c_StyleAlignCenter)
        {
            pos.x = float(m_screenRect.left) + float((m_screenRect.right - m_screenRect.left) / 2) - float(rect.right - rect.left) / 2.f;
            if (pos.x < float(m_screenRect.left))
                pos.x = float(m_screenRect.left);
        }
        else if (m_style & c_StyleAlignRight)
        {
            pos.x = float(m_screenRect.right - 1) - float(rect.right - rect.left);
            if (pos.x < float(m_screenRect.left))
                pos.x = float(m_screenRect.left);
        }

        if (m_style & c_StyleAlignMiddle)
        {
            pos.y = float(m_screenRect.top) + float((m_screenRect.bottom - m_screenRect.top) / 2) - float(rect.bottom - rect.top) / 2.f;
            if (pos.y < float(m_screenRect.top))
                pos.y = float(m_screenRect.top);
        }
        else if (m_style & c_StyleAlignBottom)
        {
            pos.y = float(m_screenRect.bottom - 1) - float(rect.bottom - rect.top);
            if (pos.y < float(m_screenRect.top))
                pos.y = float(m_screenRect.top);
        }
    }

    // Draw
    if(m_bgColor.w != 0.f)
    {
        const XMVECTOR bgColor = m_style & c_StyleTransparent ?
            XMLoadFloat4(&mgr->mConfig.colorTransparent) :
            XMLoadFloat4(&m_bgColor);

        mgr->DrawRect(m_screenRect, bgColor);
    }
        

    const XMVECTOR color = XMLoadFloat4(&m_fgColor);
    DX::DrawControllerString(mgr->m_batch.get(), font, ctrlFont, m_text.c_str(), pos, color);
}


//=====================================================================================
// Button control
//=====================================================================================
_Use_decl_annotations_
Button::Button(unsigned id, const wchar_t* text, const RECT& rect) :
    IControl(rect, id),
    m_enabled(true),
    m_showBorder(false),
    m_noFocusColor(false),
    m_focusOnText(false),
    m_style(c_StyleFontMid),
    m_text(text)
{
    DirectX::XMStoreFloat4(&m_color, Colors::Black);
}

void Button::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Determine font
    const SpriteFont* font = mgr->SelectFont(m_style);

    // Determine layout
    const XMVECTOR fsize = font->MeasureString(m_text.c_str());
    XMFLOAT2 pos(float(m_screenRect.left) + float((m_screenRect.right - m_screenRect.left) / 2) - XMVectorGetX(fsize) / 2.f,
                 float(m_screenRect.top) + float((m_screenRect.bottom - m_screenRect.top) / 2) - XMVectorGetY(fsize) / 2.f);
    if (pos.x < float(m_screenRect.left))
        pos.x = float(m_screenRect.left);
    if (pos.y < float(m_screenRect.top))
        pos.y = float(m_screenRect.top);

    constexpr int borderWidth = 5;
    RECT buttonRect = { m_screenRect.left + borderWidth, m_screenRect.top + borderWidth, m_screenRect.right - borderWidth, m_screenRect.bottom - borderWidth };

    auto batch = mgr->m_batch.get();

    XMVECTOR buttonColor;
    XMVECTOR borderColor;
    XMVECTOR fontColor;

    buttonColor = (m_focus) ? XMLoadFloat4(&mgr->mConfig.colorFocus) : XMLoadFloat4(&m_color);
    
    if (m_focus && !m_noFocusColor)
    {
        buttonColor = XMLoadFloat4(&mgr->mConfig.colorFocus);
    }
    else
    {
        buttonColor = XMLoadFloat4(&m_color);
    }

    borderColor = XMLoadFloat4(&mgr->mConfig.colorNormal);
    fontColor = XMLoadFloat4(&mgr->mConfig.colorNormal);

    if(!m_enabled)
    {
        buttonColor = XMVectorScale(buttonColor, 0.35f);
        borderColor = XMVectorScale(borderColor, 0.35f);
        fontColor = XMVectorScale(fontColor, 0.35f);
    }

    if (m_focusOnText && m_focus)
    {
        fontColor = XMLoadFloat4(&mgr->mConfig.colorFocus);
    }

    if (m_showBorder && m_focus)
    {
        mgr->DrawRect(m_screenRect, borderColor);
        mgr->DrawRect(buttonRect, buttonColor);
    }
    else
    {
        mgr->DrawRect(m_screenRect, buttonColor);
    }

    if (m_focus)
    {
        const float luminance = 0.2126f * mgr->mConfig.colorFocus.x * mgr->mConfig.colorFocus.x +
            0.7152f * mgr->mConfig.colorFocus.y * mgr->mConfig.colorFocus.y +
            0.0722f * mgr->mConfig.colorFocus.z * mgr->mConfig.colorFocus.z;

        if (luminance > 0.0722f)
        {
            // black would be best contrast with focus color
            font->DrawString(batch, m_text.c_str(), pos, Colors::Black);
        }
        else
        {
            font->DrawString(batch, m_text.c_str(), pos, fontColor);
        }
    }
    else
    {
        const float luminance = 0.2126f * m_color.x * m_color.x +
            0.7152f * m_color.y * m_color.y +
            0.0722f * m_color.z * m_color.z;

        if (luminance > 0.0722f)
        {
            // black would be best contrast with button color
            font->DrawString(batch, m_text.c_str(), pos, Colors::Black);
        }
        else
        {
            fontColor = XMVectorScale(fontColor, 0.6f);
            font->DrawString(batch, m_text.c_str(), pos, fontColor);
        }
    }
}

bool Button::OnSelected(IPanel* panel)
{
    if (m_callBack)
    {
        m_callBack(panel, this);
    }
    
    if (m_style & c_StyleExit)
    {
        return true;
    }
    return false;
}

//=====================================================================================
// Image button control
//=====================================================================================
ImageButton::ImageButton(unsigned id, unsigned imageId, const RECT& rect) :
    IControl(rect, id),
    m_enabled(true),
    m_style(0),
    m_imageId(imageId)
{
}

void ImageButton::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (m_style & c_StyleBackground)
    {
        XMVECTOR bgColor;
        if (m_focus)
        {
            bgColor = XMLoadFloat4(&mgr->mConfig.colorFocus);
        }
        else
        {
            bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);
        }

        mgr->DrawRect(m_screenRect, bgColor);
    }

    XMVECTOR fgColor;
    if (m_focus)
    {
        fgColor = XMLoadFloat4(&mgr->mConfig.colorSelected);
    }
    else
    {
        fgColor = XMLoadFloat4(m_enabled ? &mgr->mConfig.colorNormal : &mgr->mConfig.colorDisabled);
    }

    mgr->DrawImage(m_imageId, m_screenRect, fgColor);
}

bool ImageButton::OnSelected(IPanel* panel)
{
    if (m_callBack)
    {
        m_callBack(panel, this);
    }

    if (m_style & c_StyleExit)
    {
        return true;
    }
    return false;
}


//=====================================================================================
// CheckBox control
//=====================================================================================
_Use_decl_annotations_
CheckBox::CheckBox(unsigned id, const wchar_t* text, const RECT& rect, bool checked) :
    IControl(rect, id),
    m_enabled(true),
    m_checked(checked),
    m_style(c_StyleFontMid),
    m_text(text)
{
}

void CheckBox::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Determine font
    const SpriteFont* font = mgr->SelectFont(m_style);

    const float spacing = font->GetLineSpacing();

    const long boxThickness = std::max<long>(1, long(spacing * 0.1f));
    const long boxOuter = std::max<long>(2, long(spacing) - boxThickness * 2);

    // Determine layout
    RECT chkrct = { m_screenRect.left + boxThickness, m_screenRect.top + boxThickness, m_screenRect.left + boxOuter, m_screenRect.bottom - boxThickness };

    const XMFLOAT2 pos(float(m_screenRect.left) + boxOuter, float(m_screenRect.top));

    // Draw
    XMVECTOR bgColor;
    if (m_focus)
    {
        bgColor = XMLoadFloat4(&mgr->mConfig.colorFocus);
    }
    else
    {
        bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);
    }

    auto batch = mgr->m_batch.get();

    mgr->DrawRect(m_screenRect, bgColor);

    const XMVECTOR fgColor = XMLoadFloat4(m_enabled ? &mgr->mConfig.colorNormal : &mgr->mConfig.colorDisabled);
    const XMVECTOR ckColor = XMLoadFloat4(&mgr->mConfig.colorBackground);

    mgr->DrawRect(chkrct, fgColor);

    chkrct.left += boxThickness;
    chkrct.top += boxThickness;
    chkrct.right -= boxThickness;
    chkrct.bottom -= boxThickness;

    mgr->DrawRect(chkrct, ckColor);
    
    font->DrawString(batch, m_text.c_str(), pos, fgColor);

    if (m_checked)
    {
        chkrct.left += boxThickness;
        chkrct.top += boxThickness;
        chkrct.right -= boxThickness;
        chkrct.bottom -= boxThickness;

        mgr->DrawRect(chkrct, fgColor);
    }
}

bool CheckBox::OnSelected(IPanel* panel)
{
    if (m_enabled)
    {
        m_checked = !m_checked;
    }

    if (m_callBack)
    {
        m_callBack(panel, this);
    }

    return false;
}


//=====================================================================================
// Slider control
//=====================================================================================
Slider::Slider(unsigned id, const RECT& rect, int value, int minValue, int maxValue) :
    IControl(rect, id),
    m_enabled(true),
    m_dragging(false),
    m_style(0),
    m_value(value),
    m_minValue(minValue),
    m_maxValue(maxValue),
    m_thumbRect{}
{
}

void Slider::SetValue(int value)
{
    value = std::max(std::min(m_maxValue, value), m_minValue);

    if (value == m_value)
        return;

    m_value = value;

    if (m_callBack)
        m_callBack(m_parent, this);
}

void Slider::SetRange(int minValue, int maxValue)
{
    if (minValue >= maxValue)
    {
        throw std::out_of_range("Slider::SetRange");
    }

    m_minValue = minValue;
    m_maxValue = maxValue;

    const int value = std::max(std::min(m_maxValue, m_value), m_minValue);
    if (value == m_value)
        return;

    m_value = value;

    if (m_callBack)
        m_callBack(m_parent, this);
}

void Slider::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Compute 'thumb' rectangle
    const float dy = float(m_screenRect.right - m_screenRect.left);

    const long boxThickness = std::max<long>(4, long(dy * 0.05f));

    const int thumbX = static_cast<int>( float( m_value - m_minValue ) * dy / float(m_maxValue - m_minValue));

    m_thumbRect.top = m_screenRect.top + 2;
    m_thumbRect.bottom = m_screenRect.bottom - 2;

    m_thumbRect.left = m_screenRect.left + thumbX - boxThickness / 2;
    m_thumbRect.right = m_thumbRect.left + boxThickness;

    if (m_thumbRect.left < m_screenRect.left)
        m_thumbRect.left = m_screenRect.left;
    if (m_thumbRect.right > m_screenRect.right)
        m_thumbRect.right = m_screenRect.right;

    // Draw
    XMVECTOR bgColor;
    if (m_focus)
    {
        bgColor = XMLoadFloat4(&mgr->mConfig.colorFocus);
    }
    else
    {
        bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);
    }

    mgr->DrawRect(m_screenRect, bgColor);

    const XMVECTOR fgColor = XMLoadFloat4(m_enabled ? &mgr->mConfig.colorHighlight : &mgr->mConfig.colorDisabled);

    mgr->DrawRect(m_thumbRect, fgColor);
}

void Slider::OnFocus(bool in)
{
    IControl::OnFocus(in);

    if (!in)
    {
        m_dragging = false;
    }
}

bool Slider::Update(float elapsedTime, const DirectX::GamePad::State& pad)
{
    UNREFERENCED_PARAMETER(elapsedTime);

    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (pad.IsLeftThumbStickLeft())
    {
        SetValue(m_value - 1);
    }
    else if ( pad.IsLeftThumbStickRight() )
    {
        SetValue(m_value + 1);
    }

    if (mgr->m_padButtonState.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
    {
        SetValue(m_value - 1);
    }
    else if (mgr->m_padButtonState.dpadRight == GamePad::ButtonStateTracker::PRESSED)
    {
        SetValue(m_value + 1);
    }

    return false;
}

bool Slider::Update(float elapsedTime, const DirectX::Mouse::State& mstate, const DirectX::Keyboard::State& kbstate)
{
    UNREFERENCED_PARAMETER(elapsedTime);

    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Handle mouse
    if (m_dragging)
    {
        if (!mstate.leftButton)
        {
            m_dragging = false;
        }
        else
        {
            const float dy = float(m_screenRect.right - m_screenRect.left);
            const float vpp = float(m_maxValue - m_minValue) / dy;

            const int x = mstate.x;

            SetValue(int(0.5f + m_minValue + vpp * float(x - m_screenRect.left)));
        }

        return true;
    }
    else if (mstate.x >= m_screenRect.left && mstate.x < m_screenRect.right
             && mstate.y >= m_screenRect.top && mstate.y < m_screenRect.bottom)
    {
        if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
        {
            if (mstate.x >= m_thumbRect.left && mstate.x < m_thumbRect.right
                && mstate.y >= m_thumbRect.top && mstate.y < m_thumbRect.bottom)
            {
                m_dragging = true;
                return true;
            }
            else if (mstate.x > m_thumbRect.right)
            {
                SetValue(m_value + 1);
                return true;
            }
            else if (mstate.x < m_thumbRect.left)
            {
                SetValue(m_value - 1);
                return true;
            }
        }
    }

    // Handle keyboard
    if (kbstate.IsKeyDown(Keyboard::Left))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Left))
        {
            SetValue(m_value - 1);
        }

        return true;
    }
    else  if (kbstate.IsKeyDown(Keyboard::Right))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Right))
        {
            SetValue(m_value + 1);
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::Home))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Home))
        {
            SetValue(m_minValue);
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::End))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::End))
        {
            SetValue(m_maxValue);
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageUp))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageUp))
        {
            const int tenth = std::min( 10, (m_maxValue - m_minValue) / 10 );
            SetValue( m_value - tenth);
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageDown))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageDown))
        {
            const int tenth = std::min(10, (m_maxValue - m_minValue) / 10);
            SetValue(m_value + tenth);
        }

        return true;
    }

    return false;
}


//=====================================================================================
// Progress Bar
//=====================================================================================
ProgressBar::ProgressBar(unsigned id, const RECT& rect, bool visible, float start) :
    IControl(rect, id),
    m_progress(start),
    m_showPct(false)
{
    m_visible = visible;

}


ProgressBar::~ProgressBar()
{
}

void ProgressBar::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    auto batch = mgr->m_batch.get();
    auto font = mgr->m_smallFont.get();

    constexpr int border = 2;
    const long width = m_screenRect.left + long((m_screenRect.right - m_screenRect.left - 2 * border) * m_progress);
    RECT inner = { m_screenRect.left + border, m_screenRect.top + border, m_screenRect.right - border, m_screenRect.bottom - border };
    RECT bar = { m_screenRect.left + border, m_screenRect.top + border, width, m_screenRect.bottom - border };

    mgr->DrawRect(m_screenRect, XMLoadFloat4(&mgr->mConfig.colorBackground));
    mgr->DrawRect(inner, XMLoadFloat4(&mgr->mConfig.colorProgress));
    mgr->DrawRect(bar, XMLoadFloat4(&mgr->mConfig.colorFocus));

    if (m_showPct)
    {
        XMFLOAT2 pos;
        pos.x = float(m_screenRect.left + 2);
        pos.y = float(m_screenRect.top + 2);

        wchar_t str[32] = {};
        swprintf_s(str, 32, L"%.1f%%", m_progress * 100.f);

        font->DrawString(batch, str, pos, XMLoadFloat4(&mgr->mConfig.colorNormal));
    }
}


//=====================================================================================
// List Box
//=====================================================================================
ListBox::ListBox(unsigned id, const RECT& rect, unsigned style, int itemHeight) :
    IControl(rect, id),
    m_enabled(true),
    m_itemHeight(itemHeight),
    m_style(style),
    m_topItem(0),
    m_focusItem(0),
    m_itemRect{},
    m_scrollRect{},
    m_trackRect{},
    m_thumbRect{},
    m_lastHeight(0)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }
}

ListBox::~ListBox()
{
}

_Use_decl_annotations_
void ListBox::AddItem(const wchar_t* text, void *user)
{
    Item item = {};
    item.text = text;
    item.user = user;

    m_items.emplace_back(item);
}

_Use_decl_annotations_
void ListBox::InsertItem(int index, const wchar_t* text, void *user)
{
    Item item = {};
    item.text = text;
    item.user = user;

    const int item_size = static_cast<int>(m_items.size());
    if ((item_size != 0) && (index >= item_size))
    {
        m_items[size_t(index)] = item;
    }
    else
    {
        m_items.emplace(m_items.cbegin() + index, item);
    }
}

void ListBox::RemoveItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        throw std::out_of_range("RemoveItem");

    auto it = m_items.begin() + index;

    const bool selected = it->selected;

    m_items.erase(it);

    if (m_topItem >= static_cast<int>(m_items.size()))
        --m_topItem;

    if (m_focusItem >= static_cast<int>(m_items.size()))
        --m_focusItem;

    if (selected && m_callBack)
        m_callBack(m_parent, this);
}

void ListBox::RemoveAllItems()
{
    m_items.clear();
    m_topItem = m_focusItem = 0;
}

int ListBox::GetSelectedItem() const
{
    for(auto it = m_items.cbegin(); it != m_items.cend(); ++it)
    {
        if (it->selected)
            return static_cast<int>(it - m_items.cbegin());
    }

    return -1;
}

std::vector<int> ListBox::GetSelectedItems() const
{
    std::vector <int> selected;

    for (auto it = m_items.cbegin(); it != m_items.cend(); ++it)
    {
        if (it->selected)
            selected.push_back(static_cast<int>(it - m_items.cbegin()));
    }

    return selected;
}

void ListBox::ClearSelection()
{
    bool changed = false;

    for (auto&i : m_items)
    {
        if (i.selected)
        {
            i.selected = false;
            changed = true;
        }
    }

    if (changed && m_callBack)
        m_callBack(m_parent, this);
}

void ListBox::SelectItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        throw std::out_of_range("SelectItem");

    auto it = m_items.begin() + index;

    bool changed = false;

    if (m_style & c_StyleMultiSelection)
    {
        it->selected = !it->selected;
        changed = true;
    }
    else if (!it->selected)
    {
        for (auto& i : m_items)
        {
            i.selected = false;
        }

        it->selected = true;

        changed = true;
    }

    if (m_callBack && changed)
        m_callBack(m_parent, this);
}

void ListBox::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_itemRect.left = m_screenRect.left + c_BorderSize;
    m_itemRect.top = m_screenRect.top + c_BorderSize;

    m_itemRect.right = m_screenRect.right - c_BorderSize;
    if (m_style & c_StyleScrollBar)
        m_itemRect.right -= c_ScrollWidth;
    m_itemRect.bottom = m_screenRect.bottom - c_BorderSize;

    if (m_itemRect.top < m_screenRect.top)
        m_itemRect.top = m_screenRect.top;

    if (m_itemRect.left < m_screenRect.left)
        m_itemRect.left = m_screenRect.left;

    if (m_itemRect.right > m_screenRect.right)
        m_itemRect.right = m_screenRect.right;

    if (m_itemRect.bottom > m_screenRect.bottom)
        m_itemRect.bottom = m_screenRect.bottom;

    m_scrollRect.left = m_screenRect.right - c_ScrollWidth - c_BorderSize;
    m_scrollRect.right = m_screenRect.right - c_BorderSize;
    m_scrollRect.top = m_screenRect.top + c_BorderSize;
    m_scrollRect.bottom = m_screenRect.bottom - c_BorderSize;

    UpdateRects();

    auto batch = mgr->m_batch.get();

    const XMVECTOR bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);
    mgr->DrawRect(m_screenRect, bgColor);

    if (!m_items.empty())
    {
        const SpriteFont* font = mgr->SelectFont(m_style);

        m_lastHeight = (m_itemHeight <= 0) ? static_cast<int>(font->GetLineSpacing()) : m_itemHeight;
        const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize*2) / float(m_lastHeight));

        if (m_focusItem < m_topItem)
            m_topItem = m_focusItem;

        if (m_focusItem >= (m_topItem + maxl))
            m_topItem = m_focusItem;

        XMFLOAT2 pos = { float(m_itemRect.left), float(m_itemRect.top) };

        RECT selectRect = { m_itemRect.left, m_itemRect.top, m_itemRect.right, m_itemRect.top + m_lastHeight };

        for (int j = m_topItem; j < static_cast<int>(m_items.size()); ++j)
        {
            if (pos.y + float(m_lastHeight) >= (m_itemRect.bottom - c_MarginSize))
                break;

            auto& item = m_items[size_t(j)];

            if (m_focus && m_focusItem == j)
            {
                const XMVECTOR color = XMLoadFloat4(&mgr->mConfig.colorFocus);
                const XMVECTOR fgColor = XMLoadFloat4((item.selected) ? &mgr->mConfig.colorHighlight : &mgr->mConfig.colorNormal);

                mgr->DrawRect(selectRect, color);
                font->DrawString(batch, item.text.c_str(), pos, fgColor);
            }
            else
            {
                const XMVECTOR fgColor = XMLoadFloat4(m_enabled ? &mgr->mConfig.colorNormal : &mgr->mConfig.colorDisabled);

                if (item.selected)
                {
                    const XMVECTOR color = XMLoadFloat4(&mgr->mConfig.colorSelected);
                    mgr->DrawRect(selectRect, color);
                }

                font->DrawString(batch, item.text.c_str(), pos, fgColor);
            }

            pos.y += float(m_lastHeight);
            selectRect.top += m_lastHeight;
            selectRect.bottom += m_lastHeight;
        }
    }

    if (m_style & c_StyleScrollBar)
    {
        const XMVECTOR scrollColor = XMLoadFloat4(&mgr->mConfig.colorNormal);

        mgr->DrawRect(m_scrollRect, scrollColor);
        mgr->DrawRect(m_trackRect, bgColor);
        if (m_thumbRect.top != m_thumbRect.bottom)
        {
            mgr->DrawRect(m_thumbRect, scrollColor);
        }
    }
}

bool ListBox::Update(float elapsedTime, const DirectX::GamePad::State& pad)
{
    UNREFERENCED_PARAMETER(elapsedTime);
    UNREFERENCED_PARAMETER(pad);

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    assert(m_focusItem >= 0 && m_focusItem < static_cast<int>(m_items.size()));

    if (mgr->m_padButtonState.a == GamePad::ButtonStateTracker::PRESSED)
    {
        if (!m_items.empty())
        {
            SelectItem(m_focusItem);
        }
        return true;
    }
    else  if (mgr->m_padButtonState.y == GamePad::ButtonStateTracker::PRESSED)
    {
        if (m_style & c_StyleMultiSelection)
        {
            ClearSelection();
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickUp == GamePad::ButtonStateTracker::PRESSED)
    {
        mgr->m_heldTimer = c_HoldTimeStart;
        if (!m_items.empty())
        {
            --m_focusItem;
            if (m_focusItem < 0)
            {
                m_focusItem = static_cast<int>(m_items.size() - 1);
            }
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickUp == GamePad::ButtonStateTracker::HELD)
    {
        if (!m_items.empty() && mgr->m_heldTimer <= 0.f)
        {
            --m_focusItem;
            if (m_focusItem < 0)
            {
                m_focusItem = static_cast<int>(m_items.size() - 1);
            }
            mgr->m_heldTimer = c_HoldTimeRepeat;
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickDown == GamePad::ButtonStateTracker::PRESSED)
    {
        mgr->m_heldTimer = c_HoldTimeStart;
        if (!m_items.empty())
        {
            ++m_focusItem;
            if (m_focusItem >= static_cast<int>(m_items.size()))
            {
                m_focusItem = 0;
            }
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickDown == GamePad::ButtonStateTracker::HELD)
    {
        if (!m_items.empty() && mgr->m_heldTimer <= 0.f)
        {
            ++m_focusItem;
            if (m_focusItem >= static_cast<int>(m_items.size()))
            {
                m_focusItem = 0;
            }
            mgr->m_heldTimer = c_HoldTimeRepeat;
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStick == GamePad::ButtonStateTracker::PRESSED)
    {
        if (!m_items.empty())
        {
            m_focusItem = 0;
        }
    }

    return false;
}

bool ListBox::Update(float elapsedTime, const DirectX::Mouse::State& mstate, const DirectX::Keyboard::State& kbstate)
{
    UNREFERENCED_PARAMETER(elapsedTime);

    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    assert(m_focusItem >= 0 && m_focusItem < static_cast<int>(m_items.size()));

    // Handle mouse
    if (mstate.x >= m_itemRect.left && mstate.x < m_itemRect.right
        && mstate.y >= m_itemRect.top && mstate.y < m_itemRect.bottom)
    {
        if (m_lastHeight > 0)
        {
            const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
            int item = static_cast<int>(float(mstate.y - m_itemRect.top - c_MarginSize) / float(m_lastHeight));
            if (item >= 0 && item < maxl)
            {
                item = m_topItem + item;
                if (item >= 0 && item < static_cast<int>(m_items.size()))
                {
                    m_focusItem = item;

                    if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
                    {
                        SelectItem(m_focusItem);
                    }
                }
            }
        }
        return true;
    }

    if ((m_style & c_StyleScrollBar) && (m_thumbRect.top != m_thumbRect.bottom))
    {
        if (mstate.x >= m_scrollRect.left && mstate.x < m_scrollRect.right
            && mstate.y >= m_scrollRect.top && mstate.y < m_scrollRect.bottom)
        {
            if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
            {
                if (mstate.y < m_thumbRect.top)
                {
                    if (!m_items.empty() && (m_lastHeight > 0))
                    {
                        const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                        m_focusItem -= maxl;
                        if (m_focusItem < 0)
                            m_focusItem = 0;
                    }
                }
                else if (mstate.y > m_thumbRect.bottom)
                {
                    if (!m_items.empty() && (m_lastHeight > 0))
                    {
                        const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                        m_focusItem += maxl;
                        if (m_focusItem >= static_cast<int>(m_items.size()))
                            m_focusItem = static_cast<int>(m_items.size() - 1);
                    }
                }
            }
            return true;
        }
    }

    // Handle keyboard

    if (kbstate.IsKeyDown(Keyboard::Space))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Space))
        {
            if (!m_items.empty())
            {
                SelectItem(m_focusItem);
            }
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::W))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::W))
        {
            if (!m_items.empty())
            {
                --m_focusItem;
                if (m_focusItem < 0)
                {
                    m_focusItem = static_cast<int>(m_items.size() - 1);
                }
            }
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::S))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::S))
        {
            if (!m_items.empty())
            {
                ++m_focusItem;
                if (m_focusItem >= static_cast<int>(m_items.size()))
                {
                    m_focusItem = 0;
                }
            }
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::Home))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Home))
        {
            m_focusItem = 0;
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::End))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::End))
        {
            if (!m_items.empty())
            {
                m_focusItem = static_cast<int>(m_items.size() - 1);
            }
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageUp))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageUp))
        {
            if (!m_items.empty() && (m_lastHeight > 0))
            {
                const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                m_focusItem -= maxl;
                if (m_focusItem < 0)
                    m_focusItem = 0;
            }
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageDown))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageDown))
        {
            if (!m_items.empty() && (m_lastHeight > 0))
            {
                const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                m_focusItem += maxl;
                if (m_focusItem >= static_cast<int>(m_items.size()))
                    m_focusItem = static_cast<int>(m_items.size() - 1);
            }
        }

        return true;
    }

    return false;
}

void ListBox::UpdateRects()
{
    m_trackRect.top = m_scrollRect.top + 1;
    m_trackRect.left = m_scrollRect.left + 1;
    m_trackRect.right = m_scrollRect.right - 1;
    m_trackRect.bottom = m_scrollRect.bottom - 1;

    m_thumbRect.left = m_trackRect.left;
    m_thumbRect.right = m_trackRect.right;

    if (m_items.empty() || !m_lastHeight)
    {
        m_thumbRect.bottom = m_thumbRect.top;
        return;
    }

    const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));

    UpdateScrollBar(m_thumbRect, m_trackRect, m_topItem, 0, static_cast<int>(m_items.size()), maxl);
}

//=====================================================================================
// Text List
//=====================================================================================
TextList::TextList(unsigned id, const RECT& rect, unsigned style, int itemHeight) :
    IControl(rect, id),
    m_itemHeight(itemHeight),
    m_style(style),
    m_topItem(0),
    m_itemRect{},
    m_lastHeight(0)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }
}

TextList::~TextList()
{

}

_Use_decl_annotations_
void XM_CALLCONV TextList::AddItem(const wchar_t* text, FXMVECTOR color)
{
    Item item = {};
    item.text = text;
    XMStoreFloat4(&item.color, color);
    m_items.emplace_back(item);
}

_Use_decl_annotations_
void XM_CALLCONV TextList::InsertItem(int index, const wchar_t* text, FXMVECTOR color)
{
    Item item = {};
    item.text = text;
    XMStoreFloat4(&item.color, color);
    const int item_size = static_cast<int>(m_items.size());
    if ((item_size != 0) && (index >= item_size))
    {
        m_items[size_t(index)] = item;
    }
    else
    {
        m_items.emplace(m_items.cbegin() + index, item);
    }
}

void TextList::RemoveItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        throw std::out_of_range("RemoveItem");

    auto it = m_items.begin() + index;
    m_items.erase(it);
    if (m_topItem >= static_cast<int>(m_items.size()))
        --m_topItem;    
}

void TextList::RemoveAllItems()
{
    m_items.clear();
    m_topItem = 0;
}

void TextList::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_itemRect.left = m_screenRect.left + c_BorderSize;
    m_itemRect.top = m_screenRect.top + c_BorderSize;
    m_itemRect.right = m_screenRect.right - c_BorderSize;
    m_itemRect.bottom = m_screenRect.bottom - c_BorderSize;

    if (m_itemRect.top < m_screenRect.top)
        m_itemRect.top = m_screenRect.top;

    if (m_itemRect.left < m_screenRect.left)
        m_itemRect.left = m_screenRect.left;

    if (m_itemRect.right > m_screenRect.right)
        m_itemRect.right = m_screenRect.right;

    if (m_itemRect.bottom > m_screenRect.bottom)
        m_itemRect.bottom = m_screenRect.bottom;

    if (!m_items.empty())
    {
        const SpriteFont* font = mgr->SelectFont(m_style);

        m_lastHeight = (m_itemHeight <= 0) ? static_cast<int>(font->GetLineSpacing()) : m_itemHeight;
                
        XMFLOAT2 pos = { float(m_itemRect.left), float(m_itemRect.top) };

        RECT selectRect = { m_itemRect.left, m_itemRect.top, m_itemRect.right, m_itemRect.top + m_lastHeight };

        for (int j = m_topItem; j < static_cast<int>(m_items.size()); ++j)
        {
            if (pos.y + float(m_lastHeight) >= (m_itemRect.bottom - c_MarginSize))
                break;

            auto& item = m_items[size_t(j)];

            font->DrawString(mgr->m_batch.get(), item.text.c_str(), pos, XMLoadFloat4(&(item.color)));

            pos.y += float(m_lastHeight);
            selectRect.top += m_lastHeight;
            selectRect.bottom += m_lastHeight;
        }
    }
}

bool TextList::Update(float elapsedTime, const DirectX::GamePad::State& pad)
{
    UNREFERENCED_PARAMETER(elapsedTime);
    UNREFERENCED_PARAMETER(pad);

    return false;
}

bool TextList::Update(float elapsedTime, const DirectX::Mouse::State& mstate, const DirectX::Keyboard::State& kbstate)
{
    UNREFERENCED_PARAMETER(elapsedTime);
    UNREFERENCED_PARAMETER(mstate);
    UNREFERENCED_PARAMETER(kbstate);
    return false;
}


//=====================================================================================
// Text Box
//=====================================================================================
TextBox::TextBox(unsigned id, _In_z_ const wchar_t* text, const RECT& rect, unsigned style) :
    IControl(rect, id),
    m_style(style),
    m_topLine(0),
    m_itemRect{},
    m_scrollRect{},
    m_trackRect{},
    m_thumbRect{},
    m_lastHeight(0),
    m_text(text),
    m_lastWheelValue(0)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_color = mgr->mConfig.colorNormal;
}

TextBox::~TextBox()
{
}

void TextBox::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    m_itemRect.left = m_screenRect.left + c_BorderSize;
    m_itemRect.top = m_screenRect.top + c_BorderSize;

    m_itemRect.right = m_screenRect.right - c_BorderSize;
    if (m_style & c_StyleScrollBar)
        m_itemRect.right -= c_ScrollWidth;
    m_itemRect.bottom = m_screenRect.bottom - c_BorderSize;

    if (m_itemRect.top < m_screenRect.top)
        m_itemRect.top = m_screenRect.top;

    if (m_itemRect.left < m_screenRect.left)
        m_itemRect.left = m_screenRect.left;

    if (m_itemRect.right > m_screenRect.right)
        m_itemRect.right = m_screenRect.right;

    if (m_itemRect.bottom > m_screenRect.bottom)
        m_itemRect.bottom = m_screenRect.bottom;

    m_scrollRect.left = m_screenRect.right - c_ScrollWidth - c_BorderSize;
    m_scrollRect.right = m_screenRect.right - c_BorderSize;
    m_scrollRect.top = m_screenRect.top + c_BorderSize;
    m_scrollRect.bottom = m_screenRect.bottom - c_BorderSize;

    UpdateRects();

    XMVECTOR bgColor;
    if (m_focus)
    {
        const XMVECTOR fgColor = XMLoadFloat4(&mgr->mConfig.colorSelected);
        bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);

        if(!(m_style & c_StyleNoBackground))
        {
            mgr->DrawRect(m_screenRect, fgColor);

            RECT rect;
            rect.left = m_screenRect.left + 1;
            rect.top = m_screenRect.top + 1;
            rect.right = m_screenRect.right - 1;
            rect.bottom = m_screenRect.bottom - 1;

            mgr->DrawRect(rect, bgColor);
        }
    }
    else
    {
        bgColor = XMLoadFloat4((m_style & c_StyleTransparent) ? &mgr->mConfig.colorTransparent : &mgr->mConfig.colorBackground);

        if(!(m_style & c_StyleNoBackground))
        {
            mgr->DrawRect(m_screenRect, bgColor);
        }
    }

    if (!m_text.empty())
    {
        const SpriteFont* font = mgr->SelectFont(m_style);

        m_lastHeight = static_cast<int>(font->GetLineSpacing());

        const wchar_t * text = m_text.c_str();

        if (m_wordWrap.empty()
            && (m_itemRect.right != 0 && m_itemRect.bottom != 0
                && m_itemRect.left != m_itemRect.right
                && m_itemRect.top != m_itemRect.bottom))
        {
            m_wordWrap = WordWrap(text, font, m_itemRect, &m_wordWrapLines);
        }

        if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
            m_topLine = static_cast<int>(m_wordWrapLines.size()) - 1;
        m_topLine = std::max(m_topLine, 0);

        const XMFLOAT2 pos = { float(m_itemRect.left), float(m_itemRect.top) };

        const XMVECTOR color = XMLoadFloat4(&m_color);

        font->DrawString(mgr->m_batch.get(), &m_wordWrap.c_str()[m_wordWrapLines[size_t(m_topLine)]], pos, color);
    }

    if (m_style & c_StyleScrollBar)
    {
        const XMVECTOR scrollColor = XMLoadFloat4(&mgr->mConfig.colorNormal);

        mgr->DrawRect(m_scrollRect, scrollColor);
        mgr->DrawRect(m_trackRect, bgColor);
        if (m_thumbRect.top != m_thumbRect.bottom)
        {
            mgr->DrawRect(m_thumbRect, scrollColor);
        }
    }
}

bool TextBox::Update(float elapsedTime, const DirectX::GamePad::State& pad)
{
    UNREFERENCED_PARAMETER(elapsedTime);
    UNREFERENCED_PARAMETER(pad);

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_padButtonState.leftStickUp == GamePad::ButtonStateTracker::PRESSED)
    {
        mgr->m_heldTimer = c_HoldTimeStart;
        if (!m_wordWrapLines.empty())
        {
            if (m_topLine > 0)
            {
                --m_topLine;
            }
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickUp == GamePad::ButtonStateTracker::HELD)
    {
        if (!m_wordWrapLines.empty() && mgr->m_heldTimer <= 0.f)
        {
            if (m_topLine > 0)
            {
                --m_topLine;
            }
            mgr->m_heldTimer = c_HoldTimeRepeat;
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickDown == GamePad::ButtonStateTracker::PRESSED)
    {
        mgr->m_heldTimer = c_HoldTimeStart;
        if (!m_wordWrapLines.empty())
        {
            ++m_topLine;
            if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStickDown == GamePad::ButtonStateTracker::HELD)
    {
        if (!m_wordWrapLines.empty() && mgr->m_heldTimer <= 0.f)
        {
            ++m_topLine;
            if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
            mgr->m_heldTimer = c_HoldTimeRepeat;
        }
        return true;
    }
    else if (mgr->m_padButtonState.leftStick == GamePad::ButtonStateTracker::PRESSED)
    {
        if (!m_wordWrapLines.empty())
        {
            m_topLine = 0;
        }
    }

    return false;
}

bool TextBox::Update(float elapsedTime, const DirectX::Mouse::State& mstate, const DirectX::Keyboard::State& kbstate)
{
    UNREFERENCED_PARAMETER(elapsedTime);

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    // Handle mouse
    if ((m_style & c_StyleScrollBar) && (m_thumbRect.top != m_thumbRect.bottom))
    {
        if (mstate.x >= m_scrollRect.left && mstate.x < m_scrollRect.right
            && mstate.y >= m_scrollRect.top && mstate.y < m_scrollRect.bottom)
        {
            if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
            {
                if (mstate.y < m_thumbRect.top)
                {
                    if (!m_wordWrapLines.empty() && (m_lastHeight > 0))
                    {
                        const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                        m_topLine -= maxl;
                        if (m_topLine < 0)
                            m_topLine = 0;
                    }
                }
                else if (mstate.y > m_thumbRect.bottom)
                {
                    if (!m_wordWrapLines.empty() && (m_lastHeight > 0))
                    {
                        const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
                        m_topLine += maxl;
                        if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                            m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
                    }
                }
            }
            return true;
        }
    }

    if (mstate.scrollWheelValue > m_lastWheelValue)
    {
        if (!m_wordWrapLines.empty())
        {
            if (m_topLine > 0)
            {
                --m_topLine;
            }
        }
        m_lastWheelValue = mstate.scrollWheelValue;
        return true;
    }

    if (mstate.scrollWheelValue < m_lastWheelValue)
    {
        if (!m_wordWrapLines.empty())
        {
            ++m_topLine;
            if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
        }
        m_lastWheelValue = mstate.scrollWheelValue;
        return true;
    }

    // Handle keyboard

    if (kbstate.IsKeyDown(Keyboard::W) || kbstate.IsKeyDown(Keyboard::Up))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::W) || mgr->m_keyboardState.IsKeyPressed(Keyboard::Up))
        {
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }

        if (!m_wordWrapLines.empty() && mgr->m_heldTimer <= 0.f)
        {
            if (m_topLine > 0)
            {
                --m_topLine;
            }
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::S) || kbstate.IsKeyDown(Keyboard::Down))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::S) || mgr->m_keyboardState.IsKeyPressed(Keyboard::Down))
        {
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }

        if (!m_wordWrapLines.empty() && mgr->m_heldTimer <= 0.f)
        {
            ++m_topLine;
            if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::Home))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Home))
        {
            m_topLine = 0;
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::End))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::End))
        {
            if (!m_wordWrapLines.empty())
            {
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);
            }
        }

        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageUp))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageUp))
        {
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }

        if (!m_wordWrapLines.empty() && (m_lastHeight > 0) && mgr->m_heldTimer <= 0.f)
        {
            const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
            m_topLine -= maxl;
            if (m_topLine < 0)
                m_topLine = 0;

            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }
        return true;
    }
    else if (kbstate.IsKeyDown(Keyboard::PageDown))
    {
        if (mgr->m_keyboardState.IsKeyPressed(Keyboard::PageDown))
        {
            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }

        if (!m_wordWrapLines.empty() && (m_lastHeight > 0) && mgr->m_heldTimer <= 0.f)
        {
            const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));
            m_topLine += maxl;
            if (m_topLine >= static_cast<int>(m_wordWrapLines.size()))
                m_topLine = static_cast<int>(m_wordWrapLines.size() - 1);

            mgr->m_heldTimer = c_KeypressRepeatDelay;
        }

        return true;
    }

    return false;
}

void TextBox::ComputeLayout(const RECT& parent)
{
    m_wordWrap.clear();
    m_wordWrapLines.clear();
    IControl::ComputeLayout(parent);
}

void TextBox::ComputeLayout(const RECT& bounds, float dx, float dy)
{
    m_wordWrap.clear();
    m_wordWrapLines.clear();
    IControl::ComputeLayout(bounds, dx, dy);
}

void TextBox::SetText(const wchar_t* text)
{
    m_text = text;
    m_wordWrapLines.clear();
    m_wordWrap.clear();
    m_topLine = 0;
}

void TextBox::UpdateRects()
{
    m_trackRect.top = m_scrollRect.top + 1;
    m_trackRect.left = m_scrollRect.left + 1;
    m_trackRect.right = m_scrollRect.right - 1;
    m_trackRect.bottom = m_scrollRect.bottom - 1;

    m_thumbRect.left = m_trackRect.left;
    m_thumbRect.right = m_trackRect.right;

    if (m_wordWrapLines.empty() || !m_lastHeight)
    {
        m_thumbRect.bottom = m_thumbRect.top;
        return;
    }

    const int maxl = static_cast<int>(float(m_itemRect.bottom - m_itemRect.top - c_MarginSize * 2) / float(m_lastHeight));

    UpdateScrollBar(m_thumbRect, m_trackRect, m_topLine, 0, static_cast<int>(m_wordWrapLines.size()), maxl);
}


//=====================================================================================
// Popup
//=====================================================================================

Popup::Popup(const RECT& rect, unsigned int styleFlags) :
    IPanel(rect),
    m_select(false),
    m_cancel(false),
    m_suppressCancel((styleFlags & c_styleSuppressCancel) == c_styleSuppressCancel),
    m_emphasis((styleFlags & c_stylePopupEmphasis) == c_stylePopupEmphasis),
    m_custom((styleFlags & c_styleCustomPanel) == c_styleCustomPanel),
    m_focusControl(nullptr)
{
}

Popup::~Popup()
{
    for (IControl* it : m_controls)
    {
        delete it;
    }
    m_controls.clear();

    m_focusControl = nullptr;
}

void Popup::Show()
{
    if (m_visible)
        return;

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    OnWindowSize(mgr->m_fullscreen);

    m_focusControl = InitFocus(m_controls);
    if (m_focusControl)
    {
        m_focusControl->OnFocus(true);
    }
    else if (!m_custom)
    {
        throw std::exception("No usable controls");
    }

    // Make visible
    m_visible = true;

    // Clear any pending action
    m_select = false;
    m_cancel = false;

    // For now, no focus stack. Just grab focus for a popup
    mgr->m_focusPanel = this;
}

void Popup::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    auto batch = mgr->m_batch.get();

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    mgr->m_commandList->RSSetScissorRects(1, &mgr->m_fullscreen);
    batch->Begin(mgr->m_commandList);
#else
    batch->Begin(SpriteSortMode_Deferred, mgr->m_blendState.Get());
#endif

    if (m_emphasis)
    {
        // fade out all other elements to emphasize popup
        XMFLOAT4 color = mgr->mConfig.colorBackground;
        color.w *= 0.5f;
        mgr->DrawRect(mgr->m_fullscreen, XMLoadFloat4(&color));
    }

    const XMVECTOR bgColor = XMLoadFloat4(&mgr->mConfig.colorBackground);
    mgr->DrawRect(m_screenRect, bgColor);

    batch->End();

    mgr->RenderControls(m_controls);

    // Restore scissors to full screen
#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    mgr->m_commandList->RSSetScissorRects(1, &mgr->m_fullscreen);
#else
    mgr->m_context->RSSetScissorRects(1, &mgr->m_fullscreen);
#endif
}

bool Popup::Update(float elapsedTime, const GamePad::State& pad)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    bool result = false;

    if (!m_focusControl)
    {
        m_focusControl = InitFocus(m_controls);
        if (m_focusControl)
            m_focusControl->OnFocus(true);
    }

    if (m_focusControl)
    {
        result = m_focusControl->Update(elapsedTime, pad);
    }

    if (!result)
    {
        if (pad.IsDPadDownPressed())
        {
            if (mgr->m_padButtonState.dpadDown == GamePad::ButtonStateTracker::PRESSED)
            {
                m_focusControl = NextFocus(m_focusControl, m_controls);
            }
        }
        else if (pad.IsDPadUpPressed())
        {
            if (mgr->m_padButtonState.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            {
                m_focusControl = PrevFocus(m_focusControl, m_controls);
            }
        }
        else if (pad.IsBPressed())
        {
            if (mgr->m_padButtonState.b == GamePad::ButtonStateTracker::PRESSED)
            {
                Cancel();
            }
        }
        else if (pad.IsAPressed())
        {
            if (mgr->m_padButtonState.a == GamePad::ButtonStateTracker::PRESSED)
            {
                m_select = true;
            }
        }
        else if (m_select && !pad.IsAPressed())
        {
            ControlSelected(m_focusControl, this);
            m_select = false;
        }
    }

    return true;
}

bool Popup::Update(float elapsedTime, const Mouse::State& mstate, const Keyboard::State& kbstate)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    bool result = false;

    if (!m_focusControl)
    {
        m_focusControl = InitFocus(m_controls);
        if (m_focusControl)
            m_focusControl->OnFocus(true);
    }

    if (m_focusControl)
    {
        result = m_focusControl->Update(elapsedTime, mstate, kbstate);
    }

    if (!result)
    {
        // Handle mouse
        if (mstate.positionMode == Mouse::MODE_ABSOLUTE)
        {
            if ((m_screenRect.left < mstate.x) && (mstate.x < m_screenRect.right) && (m_screenRect.top < mstate.y) && (mstate.y < m_screenRect.bottom))
            {
                if (mgr->m_mouseLastX != mstate.x || mgr->m_mouseLastY != mstate.y)
                {
                    m_focusControl = MouseFocus(mstate.x, mstate.y, m_focusControl, m_controls);
                }
            }

            if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
            {
                if (m_focusControl && m_focusControl->Contains(mstate.x, mstate.y))
                {
                    m_select = true;
                }
            }
            else if (m_select && !mstate.leftButton)
            {
                ControlSelected(m_focusControl, this);
                m_select = false;
            }
        }

        // Handle keyboard
        if (kbstate.IsKeyDown(Keyboard::Down))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Down))
            {
                m_focusControl = NextFocus(m_focusControl, m_controls);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Up))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Up))
            {
                m_focusControl = PrevFocus(m_focusControl, m_controls);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Space))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Space))
            {
                ControlSelected(m_focusControl, this);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Enter))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Enter))
            {
                ControlSelected(m_focusControl, this);
            }
        }
        else if (kbstate.Escape)
        {
            m_cancel = true;
        }
        else if (m_cancel && !kbstate.Escape)
        {
            Cancel();
            m_cancel = false;
        }
        else if (m_focusControl)
        {
            auto ctrl = HotKeyFocus(m_focusControl, m_controls, mgr->m_keyboardState);
            if (ctrl)
            {
                m_focusControl = ctrl;
                ControlSelected(ctrl, this);
            }
        }
    }

    return true;
}

void Popup::Close()
{
    if (!m_visible)
        return;

    unsigned ctrlId = 0;
    if (m_focusControl)
    {
        ctrlId = m_focusControl->GetId();
    }

    if (m_callBack)
        m_callBack(this, ctrlId);

    if (m_focusControl)
    {
        m_focusControl->OnFocus(false);
        m_focusControl = nullptr;
    }

    m_visible = false;

    // For now, no focus stack. 
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_focusPanel == this)
    {
        mgr->m_focusPanel = nullptr;
    }
}

void Popup::Cancel()
{
    if (m_suppressCancel)
        return;

    if (m_callBack)
        m_callBack(this, unsigned(-1));

    if (m_focusControl)
    {
        m_focusControl->OnFocus(false);
        m_focusControl = nullptr;
    }

    m_visible = false;

    // For now, no focus stack. 
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_focusPanel == this)
    {
        mgr->m_focusPanel = nullptr;
    }
}

void Popup::Add(_In_ IControl* ctrl)
{
    if (ctrl)
    {
        m_controls.push_back(ctrl);
        ctrl->SetParent(this);
    }
}

IControl* Popup::Find(unsigned id)
{
    for (auto it : m_controls)
    {
        if (it->GetId() == id)
            return it;
    }

    return nullptr;
}

void Popup::SetFocus(_In_ IControl* ctrl)
{
    m_focusControl = ::SetFocusCtrl(m_focusControl, ctrl);
}

void Popup::OnWindowSize(const RECT& layout)
{
    // Popups should center automatically
    const long dx = (m_layoutRect.right - m_layoutRect.left);
    const long dy = (m_layoutRect.bottom - m_layoutRect.top);

    m_screenRect.left = layout.left + (layout.right - layout.left) / 2 - dx / 2;
    m_screenRect.top = layout.top + (layout.bottom - layout.top) / 2 - dy / 2;

    m_screenRect.right = m_screenRect.left + dx;
    m_screenRect.bottom = m_screenRect.top + dy;

    for (IControl* it : m_controls)
    {
        it->ComputeLayout(m_screenRect);
    }
}



//=====================================================================================
// HUD
//=====================================================================================

HUD::HUD(const RECT& rect) :
    IPanel(rect)
{
}


HUD::~HUD()
{
    for (IControl* it : m_controls)
    {
        delete it;
    }
    m_controls.clear();
}

void HUD::Show()
{
    if (m_visible)
        return;

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_hudPanel)
    {
        mgr->m_hudPanel->Close();
    }

    OnWindowSize(mgr->m_fullscreen);

    mgr->m_hudPanel = this;

    m_visible = true;
}

void HUD::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    mgr->RenderControls(m_controls);
}

void HUD::Close()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_hudPanel == this)
    {
        mgr->m_hudPanel = nullptr;
    }

    m_visible = false;
}

void HUD::Add(_In_ IControl* ctrl)
{
    if (ctrl)
    {
        m_controls.push_back(ctrl);
        ctrl->SetParent(this);
    }
}

IControl* HUD::Find(unsigned id)
{
    for (auto it : m_controls)
    {
        if (it->GetId() == id)
            return it;
    }

    return nullptr;
}

void HUD::OnWindowSize(const RECT& layout)
{
    const float dx = float(layout.right - layout.left) / float(m_screenRect.right - m_screenRect.left);
    const float dy = float(layout.bottom - layout.top) / float(m_screenRect.bottom - m_screenRect.top);

    for (IControl* it : m_controls)
    {
        it->ComputeLayout(layout, dx, dy);
    }
}


//=====================================================================================
// Overlay
//=====================================================================================

Overlay::Overlay(const RECT& rect, unsigned int styleFlags) :
    IPanel(rect),
    m_select(false),
    m_cancel(false),
    m_suppressCancel((styleFlags & c_styleSuppressCancel) == c_styleSuppressCancel),
    m_custom((styleFlags & c_styleCustomPanel) == c_styleCustomPanel),
    m_focusControl(nullptr)
{
}


Overlay::~Overlay()
{
    for (IControl* it : m_controls)
    {
        delete it;
    }
    m_controls.clear();

    m_focusControl = nullptr;
}

void Overlay::Show()
{
    if (m_visible)
        return;

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_overlayPanel)
    {
        mgr->m_overlayPanel->Close();
    }

    OnWindowSize(mgr->m_fullscreen);

    m_focusControl = InitFocus(m_controls);
    if (m_focusControl)
    {
        m_focusControl->OnFocus(true);
    }
    else if (!m_custom)
    {
        throw std::exception("No usable controls");
    }

    // Make visible
    m_visible = true;

    // Clear any pending action
    m_select = false;
    m_cancel = false;

    mgr->m_overlayPanel = this;
}

void Overlay::Render()
{
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    mgr->RenderControls(m_controls);
}

bool Overlay::Update(float elapsedTime, const GamePad::State& pad)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    bool result = false;

    if (!m_focusControl)
    {
        m_focusControl = InitFocus(m_controls);
        if (m_focusControl)
            m_focusControl->OnFocus(true);
    }

    if (m_focusControl)
    {
        result = m_focusControl->Update(elapsedTime, pad);
    }

    if (!result)
    {
        if (pad.IsDPadDownPressed())
        {
            if (mgr->m_padButtonState.dpadDown == GamePad::ButtonStateTracker::PRESSED)
            {
                m_focusControl = NextFocus(m_focusControl, m_controls);
            }
        }
        else if (pad.IsDPadUpPressed())
        {
            if (mgr->m_padButtonState.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            {
                m_focusControl = PrevFocus(m_focusControl, m_controls);
            }
        }
        else if (pad.IsBPressed())
        {
            if (mgr->m_padButtonState.b == GamePad::ButtonStateTracker::PRESSED)
            {
                Cancel();
           }
        }
        else if (pad.IsAPressed())
        {
            if (mgr->m_padButtonState.a == GamePad::ButtonStateTracker::PRESSED)
            {
                m_select = true;
            }
        }
        else if (m_select && !pad.IsAPressed())
        {
            ControlSelected(m_focusControl, this);
            m_select = false;
        }
    }

    return result;
}

bool Overlay::Update(float elapsedTime, const Mouse::State& mstate, const Keyboard::State& kbstate)
{
    auto const* mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    bool result = false;

    if (!m_focusControl)
    {
        m_focusControl = InitFocus(m_controls);
        if (m_focusControl)
            m_focusControl->OnFocus(true);
    }

    if (m_focusControl)
    {
        result = m_focusControl->Update(elapsedTime, mstate, kbstate);
    }

    if (!result)
    {
        // Handle mouse
        if (mstate.positionMode == Mouse::MODE_ABSOLUTE)
        {
            if ((m_screenRect.left < mstate.x) && (mstate.x < m_screenRect.right) && (m_screenRect.top < mstate.y) && (mstate.y < m_screenRect.bottom))
            {
                if (mgr->m_mouseLastX != mstate.x || mgr->m_mouseLastY != mstate.y)
                {
                    m_focusControl = MouseFocus(mstate.x, mstate.y, m_focusControl, m_controls);
                }
            }

            if (mgr->m_mouseButtonState.leftButton == Mouse::ButtonStateTracker::PRESSED)
            {
                if (m_focusControl && m_focusControl->Contains(mstate.x, mstate.y))
                {
                    m_select = true;
                }
            }
            else if (m_select && !mstate.leftButton)
            {
                ControlSelected(m_focusControl, this);
                m_select = false;
            }
        }

        // Handle keyboard
        if (kbstate.IsKeyDown(Keyboard::Down))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Down))
            {
                m_focusControl = NextFocus(m_focusControl, m_controls);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Up))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Up))
            {
                m_focusControl = PrevFocus(m_focusControl, m_controls);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Space))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Space))
            {
                ControlSelected(m_focusControl, this);
            }
        }
        else if (kbstate.IsKeyDown(Keyboard::Enter))
        {
            if (mgr->m_keyboardState.IsKeyPressed(Keyboard::Enter))
            {
                ControlSelected(m_focusControl, this);
            }
        }
        else if (kbstate.Escape)
        {
            m_cancel = true;
        }
        else if (m_cancel && !kbstate.Escape)
        {
            Cancel();
            m_cancel = false;
        }
        else if (m_focusControl)
        {
            auto ctrl = HotKeyFocus(m_focusControl, m_controls, mgr->m_keyboardState);
            if (ctrl)
            {
                m_focusControl = ctrl;
                ControlSelected(ctrl, this);
            }
        }
    }

    return true;
}

void Overlay::Close()
{
    if (!m_visible)
        return;

    unsigned ctrlId = 0;
    if (m_focusControl)
    {
        ctrlId = m_focusControl->GetId();
    }

    if (m_callBack)
        m_callBack(this, ctrlId);

    if (m_focusControl)
    {
        m_focusControl->OnFocus(false);
        m_focusControl = nullptr;
    }

    m_visible = false;

    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_overlayPanel == this)
    {
        mgr->m_overlayPanel = nullptr;
    }
}

void Overlay::Cancel()
{
    if (m_suppressCancel)
        return;

    if (m_callBack)
        m_callBack(this, unsigned(-1));

    if (m_focusControl)
    {
        m_focusControl->OnFocus(false);
        m_focusControl = nullptr;
    }

    m_visible = false;

    // For now, no focus stack. 
    auto mgr = UIManager::Impl::s_uiManager;
    if (!mgr)
    {
        throw std::exception("UIManager");
    }

    if (mgr->m_overlayPanel == this)
    {
        mgr->m_overlayPanel = nullptr;
    }
}

void Overlay::Add(_In_ IControl* ctrl)
{
    if (ctrl)
    {
        m_controls.push_back(ctrl);
        ctrl->SetParent(this);
    }
}

IControl* Overlay::Find(unsigned id)
{
    for (auto it : m_controls)
    {
        if (it->GetId() == id)
            return it;
    }

    return nullptr;
}

void Overlay::SetFocus(_In_ IControl* ctrl)
{
    m_focusControl = ::SetFocusCtrl(m_focusControl, ctrl);
}

void Overlay::OnWindowSize(const RECT& layout)
{
    const float dx = float(layout.right - layout.left) / float(m_screenRect.right - m_screenRect.left);
    const float dy = float(layout.bottom - layout.top) / float(m_screenRect.bottom - m_screenRect.top);

    for (IControl* it : m_controls)
    {
        it->ComputeLayout(layout, dx, dy);
    }
}
