//
// DPadUI.h - A simple UI class that uses the DPad to selection options
//

#pragma once



class DPadUI
{
    struct RowData
    {
        UINT8 m_fieldCount;
        UINT8 m_selected;
        bool m_isBooleanOption;
    };
    const wchar_t** m_names;
    RowData* m_rowData;
    UINT m_selectedRow;
    UINT m_rows;
    UINT m_rowsAdded;
    UINT m_fieldsPerRow;

public:

    DPadUI();
    ~DPadUI();

    void Initialize(UINT numRows, UINT maxFieldsPerRow);
    void Release();

    UINT GetNumRows() const
    {
        return m_rowsAdded;
    }

    UINT GetRowSelection(UINT rowIndex) const;
    bool SetRowSelection(UINT rowIndex, UINT8 selection);   // returns true on success
    const wchar_t* GetRowName(UINT rowIndex) const;
    const wchar_t* GetRowSelectionFieldName(UINT rowIndex) const;
    bool IsSelected(UINT rowIndex) const
    {
        return (m_selectedRow == rowIndex);
    }

    UINT GetSelectedRow() const
    {
        return m_selectedRow;
    }

    bool SetFieldName(const wchar_t* name, UINT rowIndex, UINT fieldIndex);
    bool SetFieldNameLastAddedRow(const wchar_t* name, UINT fieldIndex)
    {
        if (!m_rowsAdded)
            return false;

        return SetFieldName(name, m_rowsAdded - 1, fieldIndex);
    }
    bool ReplaceRow(UINT rowIndex, const wchar_t* name, UINT8 fieldCount, bool isBoolean = false, UINT8 initialValue = 0);
    bool AddRow(const wchar_t *name, UINT8 fieldCount, bool isBoolean = false, UINT8 initialValue = 0)
    {
        return ReplaceRow(m_rowsAdded, name, fieldCount, isBoolean, initialValue);
    }

    void Update(bool up, bool down, bool left, bool right);

};
