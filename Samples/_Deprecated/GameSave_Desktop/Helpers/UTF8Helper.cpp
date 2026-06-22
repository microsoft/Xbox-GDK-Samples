//--------------------------------------------------------------------------------------
// UTF8Helper.cpp
//
// Helps with conversion between Unicode and UTF8 text
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UTF8Helper.h"
#include "malloc.h"

__declspec(thread) char scratchFormatSpace[ 2048 ];

size_t ATG::Text::CalculateMaxUTF8Length( const wchar_t* text )
{
   return wcslen( text ) * MAX_BYTES_PER_UTF8_CHAR;
}

size_t ATG::Text::CalculateMaxUTF8Length( size_t numUCS2Chars )
{
   return numUCS2Chars * MAX_BYTES_PER_UTF8_CHAR;
}

wchar_t* ATG::Text::FormatStringScratch( const wchar_t* format, ... )
{
   va_list args;
   va_start( args, format );
   vswprintf_s( (wchar_t*)scratchFormatSpace, sizeof(scratchFormatSpace)/sizeof(wchar_t), format, args );
   va_end( args );

   return (wchar_t*)scratchFormatSpace;
}

char* ATG::Text::FormatStringScratch( const char* format, ... )
{
   va_list args;
   va_start( args, format );
   vsprintf_s( scratchFormatSpace, format, args );
   va_end( args );

   return scratchFormatSpace;
}

size_t ATG::Text::CalculateMaxUTF8Length( const std::wstring& text )
{
   return text.length() * MAX_BYTES_PER_UTF8_CHAR;
}

std::string ATG::Text::ToUTF8String( const wchar_t* text )
{
   assert( text != nullptr && "string must not be null" );
   size_t numwchar = wcslen( text );
   size_t tempBufSize = MAX_BYTES_PER_UTF8_CHAR * numwchar + 1;
   auto buf = (char*)alloca( tempBufSize );

   ::WideCharToMultiByte( CP_UTF8, 0, text, int(numwchar + 1), buf, int(tempBufSize), nullptr, nullptr );
   return std::string( buf );
}

std::string ATG::Text::ToUTF8String( const std::wstring& text )
{
   size_t numwchar = text.length();
   if ( numwchar == 0 )
   {
      return std::string();
   }

   size_t tempBufSize = MAX_BYTES_PER_UTF8_CHAR * numwchar + 1;
   auto buf = (char*)alloca( tempBufSize );

   ::WideCharToMultiByte( CP_UTF8, 0, text.c_str(), int(numwchar + 1), buf, int(tempBufSize), nullptr, nullptr );
   return std::string( buf );
}
