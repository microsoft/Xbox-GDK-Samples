//
// DPadUI.cpp - A simple UI class that uses the DPad to selection options
//

#include <pch.h>
#include "DPadUI.h"
#include <memoryapi.h>
#include <algorithm>
#include <cstring>



DPadUI::DPadUI()
    : m_names(nullptr)
    , m_rowData(nullptr)
    , m_selectedRow(0)
    , m_rows(0)
    , m_rowsAdded(0)
    , m_fieldsPerRow(0)
{
}


DPadUI::~DPadUI()
{
    Release();
}


void DPadUI::Initialize(UINT numRows, UINT maxFieldsPerRow)
{
    Release();

    if (!(numRows && maxFieldsPerRow))
        return;

    UINT numStringAllocations = numRows * (maxFieldsPerRow + 1);

    m_names         = (const wchar_t**)VirtualAlloc(nullptr, numStringAllocations * sizeof(void*), MEM_COMMIT, PAGE_READWRITE);
    m_rowData       = (RowData*)VirtualAlloc(nullptr, numRows * sizeof(RowData), MEM_COMMIT, PAGE_READWRITE);

    if (!(m_names && m_rowData))
    {
        // oh dear!
        DPadUI::~DPadUI();
        return;
    }
    ZeroMemory(m_names, numStringAllocations * sizeof(void*));
    ZeroMemory(m_rowData, numRows * sizeof(RowData));

    m_rows          = numRows;
    m_fieldsPerRow  = maxFieldsPerRow;
}

void DPadUI::Release()
{
    if (m_names)
    {
        VirtualFree(m_names, 0, MEM_RELEASE);
        m_names = nullptr;
    }
    if (m_rowData)
    {
        VirtualFree(m_rowData, 0, MEM_RELEASE);
        m_rowData = nullptr;
    }
    m_selectedRow = 0;
    m_rows = 0;
    m_rowsAdded = 0;
    m_fieldsPerRow = 0;
}

bool DPadUI::SetFieldName(const wchar_t* name, UINT rowIndex, UINT fieldIndex)
{
    if (rowIndex >= m_rowsAdded)
        return false;

    RowData& rowData = m_rowData[rowIndex];

    if ((fieldIndex >= rowData.m_fieldCount) || rowData.m_isBooleanOption)
        return false;

    m_names[rowIndex * (m_fieldsPerRow + 1) + (fieldIndex + 1)] = name;

    return true;
}


bool DPadUI::ReplaceRow(UINT rowIndex, const wchar_t* name, UINT8 fieldCount, bool isBoolean, UINT8 initialValue)
{
    if (rowIndex > m_rowsAdded)
        return false;

    if (fieldCount > m_fieldsPerRow)
        return false;

    if (rowIndex == m_rowsAdded)
    {
        if (m_rowsAdded >= m_rows)
            return false;

        ++m_rowsAdded;
    }
    m_names[rowIndex * (m_fieldsPerRow + 1)] = name;

    RowData& rowData            = m_rowData[rowIndex];
    rowData.m_fieldCount        = fieldCount;
    rowData.m_isBooleanOption   = isBoolean;

    if (isBoolean)
    {
        rowData.m_selected = UINT8(initialValue ? 1 : 0);
    }
    else
    {
        rowData.m_selected = UINT8(initialValue >= fieldCount ? 0 : initialValue);
    }
    return true;
}


const wchar_t* DPadUI::GetRowName(UINT rowIndex) const
{
    if (rowIndex >= m_rowsAdded)
        return nullptr;

    const wchar_t* name = m_names[rowIndex * (m_fieldsPerRow + 1)];

    return name ? name : L"<nullptr>";
}


const wchar_t* DPadUI::GetRowSelectionFieldName(UINT rowIndex) const
{
    if (rowIndex >= m_rowsAdded)
        return nullptr;

    const RowData& rowData = m_rowData[rowIndex];

    if (rowData.m_isBooleanOption)
        return rowData.m_selected ? L"true" : L"false";
        
    const wchar_t* name = m_names[rowIndex * (m_fieldsPerRow + 1) + (rowData.m_selected + 1)];

    return name ? name : L"<nullptr>";
}


UINT DPadUI::GetRowSelection(UINT rowIndex) const
{
    if (rowIndex >= m_rowsAdded)
        return 0;

    const RowData& rowData = m_rowData[rowIndex];

    return rowData.m_selected;
}


bool DPadUI::SetRowSelection(UINT rowIndex, UINT8 selection)
{
    if (rowIndex >= m_rowsAdded)
        return false;

    RowData& rowData = m_rowData[rowIndex];

    if (selection >= rowData.m_fieldCount)
        return false;

    rowData.m_selected = selection;

    return true;
}


void DPadUI::Update(bool up, bool down, bool left, bool right)
{
    if (!m_rowsAdded)
        return;

    if (up && down)
    {
        up = false;
        down = false;
    }
    if (left && right)
    {
        left = false;
        right = false;
    }
    if (up)
    {
        if (m_selectedRow)
            --m_selectedRow;
    }
    if (down)
    {
        ++m_selectedRow;

        if (m_selectedRow >= m_rowsAdded)
            m_selectedRow = m_rowsAdded - 1;
    }
    RowData& rowData = m_rowData[m_selectedRow];

    if (rowData.m_isBooleanOption)
    {
        if (left || right)
            rowData.m_selected ^= 1;
    }
    else
    {
        if (left)
        {
            if (rowData.m_selected)
                --rowData.m_selected;
            else
                rowData.m_selected = UINT8(rowData.m_fieldCount - 1);
        }
        if (right)
        {
            if (rowData.m_selected < (rowData.m_fieldCount - 1))
                ++rowData.m_selected;
            else
                rowData.m_selected = 0;
        }
    }
}
