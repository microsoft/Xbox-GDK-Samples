//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"

class Report
{
public:
    Report()
    {
        Reset();
    }
    ~Report() 
    {
    }
    void Reset()
    {
        m_currentColumn = 0;
        m_columns.clear();
        m_rows.clear();
        m_header.str(L"");
        m_currentRowText.str(L"");
    }   
    void AddColumn(const wchar_t* m_headerText, const wchar_t* m_unitText, uint32_t width, uint32_t precision = 0)
    {
        if (!m_rows.empty())
        {
            throw std::exception("Cannot add new columns after output has begun\n");
        }
        m_columns.push_back(Column(m_headerText, m_unitText, width, precision));
    }
    void AddHeader()
    {
        for (auto column : m_columns)
        {
            m_header << L"|";
            m_header << std::setw(column.m_width-1);
            m_header << column.m_headerText;
            if ((size_t)column.m_width <= column.m_headerText.length())
            {
                throw std::exception("Column width is too small to contain header\n");
            }
        }
        m_header << std::endl;
        m_header << std::setfill(L'-');
        for (auto column : m_columns)
        {
            m_header << L"|";
            m_header << std::setw(column.m_width-1);
            m_header << L"";
        }
        m_header << std::setfill(L' ');
        m_header << std::endl;
    }
    template<typename t_type>
    void AddRowData(const t_type data)
    {
        auto column = m_columns[m_currentColumn++];
        if (m_columns.size() < m_currentColumn)
        {
            throw std::exception("Too many data items added to row.\n");
        }
        if (0 == m_currentColumn)
        {
            m_currentRowText.imbue(std::locale(""));
        }

        m_currentRowText << L"|";
        m_currentRowText << std::setw(column.m_dataWidth);
        m_currentRowText << std::showpoint;
        m_currentRowText << std::fixed;
        m_currentRowText << std::setprecision(column.m_dataPrecision);
        m_currentRowText << data;
        m_currentRowText << column.m_unitText;
    }
    void EndRow(const DirectX::XMVECTOR& color = DirectX::Colors::Transparent)
    {
        auto rowColor = color;
        if(XMVector4Equal(DirectX::Colors::Transparent, color))
        {
            // default is to alternate
            const DirectX::XMVECTOR standardColors[] =
            {
                DirectX::Colors::Tan, 
                DirectX::Colors::White, 
            };
            rowColor = standardColors[m_rows.size() % _countof(standardColors)];
        }

        if (m_columns.size() != m_currentColumn)
        {
            throw std::exception("Too few data items added to row.\n");
        }
        m_currentColumn = 0;

        m_currentRowText << std::endl;
        m_rows.push_back(Row(m_currentRowText.str().c_str(), rowColor));

        m_currentRowText.str(L"");
    }
    size_t GetRowCount() const 
    { 
        if (0 != m_currentColumn)
        {
            throw std::exception("Report is not complete\n");
        }
        return m_rows.size();
    }
    std::wstring GetRow(uint32_t i) const 
    { 
        if (0 != m_currentColumn)
        {
            throw std::exception("Report is not complete\n");
        }
        return m_rows[i].m_text;
    }
    DirectX::XMVECTOR GetRowColor(uint32_t i) const 
    { 
        if (0 != m_currentColumn)
        {
            throw std::exception("Report is not complete\n");
        }
        return m_rows[i].m_color;
    }
    std::wstring GetHeader() const 
    { 
        if (0 != m_currentColumn)
        {
            throw std::exception("Report is not complete\n");
        }
        return m_header.str();
    }

private:
    struct Column
    {
        Column(const wchar_t* headerText, const wchar_t* unitText, size_t maxDataWidth, std::streamsize dataPrecision) :
            m_headerText(headerText),
            m_unitText(unitText),
            m_dataPrecision(dataPrecision)
        {
            // Total text length is max(header, data + units), with an extra space for separation
            m_width = std::streamsize(std::max(m_headerText.length(), maxDataWidth + m_unitText.length()) + 1);
            m_dataWidth = std::streamsize(m_width - m_unitText.length() - 1);
        }
        std::wstring                    m_headerText;
        std::wstring                    m_unitText;
        std::streamsize                 m_dataWidth;
        std::streamsize                 m_dataPrecision;
        std::streamsize                 m_width;
    };

    struct Row
    {
        Row(const wchar_t* text, const DirectX::XMVECTOR& color) :
            m_color(color),
            m_text(text) 
        {
        }

        DirectX::XMVECTOR               m_color;
        std::wstring                    m_text;
    };

    uint32_t                            m_currentColumn;
    std::vector<Column>                 m_columns;
    std::vector<Row>                    m_rows;
    std::wostringstream                 m_header;
    std::wostringstream                 m_currentRowText;
};
