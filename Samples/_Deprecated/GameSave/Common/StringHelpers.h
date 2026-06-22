//--------------------------------------------------------------------------------------
// StringHelpers.h
//
// A variety of string and formatting helper functions.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

#include "UTF8Helper.h"

/**
 * Formats an HRESULT as a hexadecimal text string.
 *
 * For example: "0x8007C045"
 * 
 * \param int hResult - the HRESULT to format
 * \return a std::string containing the result.
 */
inline std::string FormatHResult( int hResult )
{
   char hrBuf[ 11 ] = {};
   sprintf_s( hrBuf, u8"0x%08X", hResult );
   return std::string( hrBuf );
}

/**
 *	Formats a GameInput APP_LOCAL_DEVICE_ID for a controller as a hexadecimal text string.
 *
 * For example: "[A196E6CC18264A6C75CCAA49786EC620513CF869B021EBE01041C0FB1225D686]"
 *
 * \param const APPLOCAL_DEVICE& id - the id value to format.
 * \return a std::string containing the result.
 */
inline std::string FormatControllerId( const APP_LOCAL_DEVICE_ID& id )
{
   std::string result;
   result.reserve( 2 + APP_LOCAL_DEVICE_ID_SIZE * 2 );
   result += "[";
   char temp[ 3 ];
   for ( size_t i = 0; i < APP_LOCAL_DEVICE_ID_SIZE; ++i )
   {
      sprintf_s( temp, "%02X", (int) id.value[ i ] );
      result += temp;
   }
   result += "]";

   return result;
}

/**
 * Converts an XUserHandle into a string representation.
 * 
 * For example: "UserGamerTag (ID:0xXUIDXUIDXUIDXUID)"
 *
 * \param _In_ XUserHandle user - the user to emit.
 * \param bool includeXuid - Whether or not to include the XUID of the user in the representation.
 * \param bool returnEmptyStringForNullUser - whether to emit an empty string for a null user, or the name "(unknown user)"
 * \return std::string - the formatted user information.
 */
inline std::string FormatUserName( _In_ XUserHandle user, bool includeXuid = true, bool returnEmptyStringForNullUser = false )
{
   if ( user != nullptr )
   {
      char gamertag[XUserGamertagComponentClassicMaxBytes] = {};
      XUserGetGamertag( user, XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes, gamertag, nullptr );

      if ( includeXuid )
      {
         uint64_t id;
         XUserGetId( user, &id );
         char* temp = ATG::Text::FormatStringScratch( u8"%s (ID:0x%016llX)", gamertag, id );
         return std::string( temp );
      }
      else
      {
         return std::string( gamertag );
      }
   }

   if ( returnEmptyStringForNullUser )
   {
      return std::string( u8"" );
   }
   else
   {
      return std::string( u8"(unknown user)" );
   }
}

/**
 * Converts a time_t timestamp into a local date-time string.
 *
 * For example: "Tue Jul 23 14:57:22 2019"
 * 
 * \param const time_t & timestamp - the UTC timestamp to format.
 * \return std::string - the formatted time and date string.
 */
inline std::string FormatLocalTimeFromDateTime( const time_t& timestamp )
{
   char temp[ 100 ];
   tm localTimestamp;
   localtime_s( &localTimestamp, &timestamp );
   strftime( temp, 100, "%D %T", &localTimestamp );

   return std::string( temp );
}

/**
 * Checks two wchar text strings for equality in a case-insensitive fashion.
 * 
 * \param const wchar_t * val1 - the first string
 * \param const wchar_t * val2 - the second string
 * \return bool - true if they are the same (ignoring case differences).
 */
inline bool IsStringEqualCaseInsensitive( const wchar_t* val1, const wchar_t* val2 )
{
   return ( _wcsicmp( val1, val2 ) == 0 );
}

/**
 * Checks two text strings for equality in a case-insensitive fashion.
 *
 * \param const char_t * val1 - the first string
 * \param const char_t * val2 - the second string
 * \return bool - true if they are the same (ignoring case differences).
 */
inline bool IsStringEqualCaseInsensitive( const char* val1, const char* val2 )
{
   return ( _stricmp( val1, val2 ) == 0 );
}

/**
 *	The memory units to use, in both metric and binary forms.
 */
enum class MemoryUnits
{
   BestMatch, //< Use the binary units which match the value the best. 
   BestMatchMetric, //< Use the metric units (10^N) that match the value the best.
   Bytes,
   Kibibyte,
   Mebibyte,
   Gibibyte,
   Tebibyte,
   Kilobyte,
   Megabyte,
   Gigabyte,
   Terabyte
};


/**
 * \brief Formats a uint64_t value as a size in memory, using the specified units.
 * Formats a value as a memory size. You may use MemorySize::BestMatch to have it auto-determine the best units
 * to show. 
 * \param uint64_t value - the value to format as a string.
 * \param MemoryUnits units - the units to use, or MemoryUnits::BestMatch if you want the system to select  
 * \param int decimalPlaces - the number of decimal places to render. (Note: MemoryUnits::Bytes always renders to 0
*                             decimal places).
 * \return std::string - the formatted value as a std::string.
 */
inline std::string FormatByteValueAsStringWithUnits(uint64_t value, MemoryUnits units = MemoryUnits::BestMatch, int decimalPlaces = 1 )
{
   if ( units == MemoryUnits::BestMatch )
   {
      if (value <= 1023ULL)
      {
         units = MemoryUnits::Bytes;
      }
      else if (value >= 1024ULL && value < (1024ULL * 1024ULL))
      {
         units = MemoryUnits::Kibibyte;
      }
      else if (value >= (1024ULL * 1024ULL) && value < (1024ULL * 1024ULL * 1024ULL))
      {
         units = MemoryUnits::Mebibyte;
      }
      else if (value >= (1024ULL * 1024ULL * 1024ULL) && value < (1024ULL * 1024ULL * 1024ULL * 1024ULL))
      {
         units = MemoryUnits::Gibibyte;
      }
      else 
      {
         units = MemoryUnits::Tebibyte;
      }
   }
   else if (units == MemoryUnits::BestMatchMetric)
   {
      if (value <= 1000ULL)
      {
         units = MemoryUnits::Bytes;
      }
      else if (value >= 1000ULL && value < 1000000ULL)
      {
         units = MemoryUnits::Kilobyte;
      }
      else if (value >= 1000000ULL && value < 1000000000ULL)
      {
         units = MemoryUnits::Megabyte;
      }
      else if (value >= 1000000000ULL && value < 1000000000000ULL)
      {
         units = MemoryUnits::Gigabyte;
      }
      else 
      {
         units = MemoryUnits::Terabyte;
      }
   }

   const char* unitString = "";
   double divisor;

   switch (units)
   {
      case MemoryUnits::Bytes:
      {
         unitString = "B";
         divisor = 1.0;
         decimalPlaces = 0; // No point showing fractional bytes :)
         break;
      }
      case MemoryUnits::Kibibyte:
      {
         unitString = "KiB";
         divisor = 1024.0;
         break;
      }
      case MemoryUnits::Mebibyte:
      {
         unitString = "MiB";
         divisor = 1024.0 * 1024.0;
         break;
      }
      case MemoryUnits::Gibibyte:
      {
         unitString = "GiB";
         divisor = 1024.0 * 1024.0 * 1024.0;
         break;
      }
      case MemoryUnits::Tebibyte:
      {
         unitString = "TiB";
         divisor = 1024.0 * 1024.0 * 1024.0 * 1024.0;
         break;
      }
      case MemoryUnits::Kilobyte:
      {
         unitString = "kB";
         divisor = 1.0E3;
         break;
      }
      case MemoryUnits::Megabyte:
      {
         unitString = "MB";
         divisor = 1.0E6;
         break;
      }
      case MemoryUnits::Gigabyte:
      {
         unitString = "GB";
         divisor = 1.0E9;
         break;
      }
      case MemoryUnits::Terabyte:
      {
         unitString = "TB";
         divisor = 1.0E12;
         break;
      }
      default:
      {
         assert(false && "This shouldn't happen");
         __assume(false);
      }
   }

   std::string output;
   double rvalue = static_cast<double>(value);

   if (decimalPlaces == 0)
   {
      uint64_t integerPart = static_cast<uint64_t>(round(rvalue / divisor));
      output += std::to_string(integerPart);
   }
   else
   {
      double scaledValue = rvalue / divisor;
      double places = pow(10, decimalPlaces);
      uint64_t intPlacesValue = uint64_t(places);
      double intPart = floor(scaledValue);
      double fractionalPart = round((scaledValue - intPart) * places);
      
      uint64_t integerPart = static_cast<uint64_t>(intPart);
      uint64_t fractPart = static_cast<uint64_t>(fractionalPart);

      if (fractPart == intPlacesValue)
      {
         fractPart = 0;
         ++integerPart;
      }

      output += std::to_string(integerPart);
      output += '.';
      output += std::to_string(fractPart);
   }

   output += unitString;

   return output;
}
