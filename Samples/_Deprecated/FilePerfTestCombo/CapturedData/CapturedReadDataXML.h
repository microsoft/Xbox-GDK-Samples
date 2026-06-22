//--------------------------------------------------------------------------------------
// CapturedReadDataXML.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "CapturedReadData.h"

#pragma warning (push,0)
#include "rapidxml-1.13\rapidxml.hpp"
#include "rapidxml-1.13\rapidxml_utils.hpp"
#include "rapidxml-1.13\rapidxml_print.hpp"
#pragma warning (pop)

typedef rapidxml::xml_document<char> XMLDoc;
typedef rapidxml::xml_node<char> XMLNode;
typedef rapidxml::xml_attribute<char> XMLAttribute;

class CapturedReadDataXML : public CapturedReadData
{
public:
    CapturedReadDataXML() {}

    virtual ~CapturedReadDataXML() {  }

    bool ReadDataSetFile(const std::vector<std::wstring>& files, const std::wstring& baseDirectory);
    bool WriteDataSetFile(const std::wstring& outputFileName, const std::string& dataName, const std::string& dataSetName);
};
