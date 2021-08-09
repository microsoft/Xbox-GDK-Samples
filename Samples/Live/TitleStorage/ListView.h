//--------------------------------------------------------------------------------------
// File: ListView.h
//
// A helper class for managing a displaying a list of data
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------
#pragma once

struct ListViewConfig
{
    int MaxRows;
    int RowStartIndex;
    int RowIncrement;
    int ParentPanel;
};

template <typename ListItemType, typename ListViewRow>
class ListView
{
public:
    ListView(std::shared_ptr<ATG::UIManager> ui, const ListViewConfig &config) :
        m_ui(ui)
    {
        m_rows.resize(config.MaxRows);

        auto panel = ui->FindPanel<ATG::IPanel>(config.ParentPanel);

        for (int i = 0; i < config.MaxRows; ++i)
        {
            int row = config.RowStartIndex + i * config.RowIncrement;
            m_rows[i].SetControls(panel, row);
        }
    }

    void UpdateRows(const std::vector<ListItemType> &list, size_t offset = 0)
    {
        for (size_t i = 0; i < m_rows.size(); ++i)
        {
            if (i + offset >= list.size())
            {
                m_rows[i].Hide();
            }
            else
            {
                m_rows[i].Show();
                m_rows[i].Update(&list[i + offset]);
            }
        }
    }

    void SetSelectedCallback(ATG::IControl::callback_t callback)
    {
        for (auto &row : m_rows)
        {
            row.SetSelectedCallback(callback);
        }
    }
	
	void SetFocusedCallback(ATG::IControl::callback_t callback)
	{
		for (auto &row : m_rows)
		{
			row.SetFocusedCallback(callback);
		}
	}

    void ClearAllRows()
    {
        for (auto &row : m_rows)
        {
            row.Hide();
        }
    }
private:
    std::shared_ptr<ATG::UIManager> m_ui;
    std::vector<ListViewRow> m_rows;
};
