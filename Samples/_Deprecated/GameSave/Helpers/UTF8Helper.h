//--------------------------------------------------------------------------------------
// UTF8Helper.h
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

#pragma once

namespace ATG
{
   namespace Text
   {
      /**
       *	The maximum number of bytes a given unicode character could possibly require as storage when converted to 
       * UTF-8 representation.
       */
      constexpr size_t MAX_BYTES_PER_UTF8_CHAR = 4;
         
      /**
       * Converts a C-style wchar string to a UTF-8 std::string.
       * 
       * This function creates a temporary buffer on the stack the maximum size that the output string could possibly
       * be. In many cases this is faster than running the conversion twice (the first time to calculate the length, and 
       * the second to generate the output).
       *
       * \note In general, prefer the version that take as input a std::wstring as it does not need to count the 
       *       characters in the string first, and will be much faster.
       *
       * \param const wchar_t * text - the text string to convert from UCS2 to UTF-8
       * \return std::string - the Utf-8 encoded output string.
       */
      std::string ToUTF8String( const wchar_t* text );

      /**
       * Converts a C-style wchar string to a UTF-8 std::string.
       *
       * This function creates a temporary buffer on the stack the maximum size that the output string could possibly
       * be. In many cases this is faster than running the conversion twice (the first time to calculate the length, and
       * the second to generate the output).
       *
       * \param const std::wstring& text - the text string to convert from UCS2 to UTF-8.
       * \return std::string - the Utf-8 encoded output string.
       */
      std::string ToUTF8String( const std::wstring& text );

      /**
       * Calculates an estimate of the upper bound on the number of bytes needed to represent a given number of UCS-2 
       * characters when converted to UTF-8 encoding.
       *
       * \param size_t numUCS2Chars - the number of UCS2 unicode characters to calculate an upper bound for.
       * \return size_t - an estimate of the maximum number of bytes that could represent the input string when 
       *                  converted to UTF-8.
       */
      size_t CalculateMaxUTF8Length( size_t numUCS2Chars );

      /**
       * Calculates an estimate of the upper bound on the number of bytes needed to represent the provided string of 
       * UCS-2 characters when converted to UTF-8 encoding.
       *
       * \note Like strlen, this function does not include the nul terminator in its calculator.
       * \note Where possible, use the std::wstring version of this function as it does not need to count the number of
       *       characters in the string.
       *
       * \param const wchar_t * text - the text string to estimate the size of.
       * \return size_t - the maximum number of bytes needed to store the converted output.
       */
      size_t CalculateMaxUTF8Length( const wchar_t* text );

      /**
       * Calculates an estimate of the upper bound on the number of bytes needed to represent the provided string of
       * UCS-2 characters when converted to UTF-8 encoding.
       *
       * \note Like strlen, this function does not include the nul terminator in its calculator.
       *
       * \param const std::wstring& text - the text string to estimate the size of.
       * \return size_t - the maximum number of bytes needed to store the converted output.
       */
      size_t CalculateMaxUTF8Length( const std::wstring& text );

      /**
       * Formats a string using sprintf_s into a temporary per-thread (thread-local storage) scratch buffer.
       * 
       * This is useful for performing quick formatting operations which will be used elsewhere, when you don't want to
       * perform allocations. The only requirement is that must use the output value before another call to 
       * FormatStringScratch is made on the same thread.
       * 
       * \note The temporary scratch buffer is 2048 bytes long.
       *
       * \param const char * format - the printf-style format string.
       * \param ... - the formatting parameters.
       * \return char* - the formatted output string.
       */
      char* FormatStringScratch( const char* format, ... );

      /**
       * Formats a string using sprintf_s into a temporary per-thread (thread-local storage) scratch buffer.
       *
       * This is useful for performing quick formatting operations which will be used elsewhere, when you don't want to
       * perform allocations. The only requirement is that must use the output value before another call to
       * FormatStringScratch is made on the same thread.
       *
       * \note The temporary scratch buffer is 2048 bytes long.
       *
       * \param const char * format - the printf-style format string.
       * \param ... - the formatting parameters.
       * \return char* - the formatted output string.
       */      
      wchar_t* FormatStringScratch( const wchar_t* format, ... );
   };
};
