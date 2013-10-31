/*----------------------------------------------------------------------------
 Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 \file      Unicode.h

 \brief     Support for Unicode character encodings
*/

#ifndef UNICODE_H
#define UNICODE_H

#include <unistd.h>
#include <limits.h>

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iterator>
#include <iomanip>

#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>

namespace SCX
{
    namespace Util
    {
        namespace Encoding
        {
            /*----------------------------------------------------------------------------*/
            /**
             Enumeration for text encoding when outputting exception strings

             \date      2012-09-22 12:00:00

             \author    Niel Orcutt
            */
            enum Encoding
            {
                eUnknown = 0,
                eASCII,
                eUTF8,
                eUTF16LE,
                eUTF16BE,
                eUTF32LE,
                eUTF32BE
            };

            /** String constants for Encoding*/
            const std::string c_UNKNOWN  =   "Unknown";
            const std::string c_ASCII    =   "ASCII";
            const std::string c_UTF8     =   "UTF-8";
            const std::string c_UTF16LE  =   "UTF-16LE";
            const std::string c_UTF16BE  =   "UTF-16BE";
            const std::string c_UTF32LE  =   "UTF-32LE";
            const std::string c_UTF32BE  =   "UTF-32BE";

            /*----------------------------------------------------------------------------*/
            /**
             Convert Encoding enumeration value to string

             \param [in]    encoding  encoding enumeration

             \returns       encoding string
            */
            inline std::string GetEncodingString(const Encoding encoding)
            {
                switch (encoding)
                {
                case eASCII:
                    return c_ASCII;
                    break;

                case eUTF8:
                    return c_UTF8;
                    break;

                case eUTF16LE:
                    return c_UTF16LE;
                    break;

                case eUTF16BE:
                    return c_UTF16BE;
                    break;

                case eUTF32LE:
                    return c_UTF32LE;
                    break;

                case eUTF32BE:
                    return c_UTF32BE;
                    break;

                case eUnknown:
                default:
                    return c_UNKNOWN;
                    break;
                }
            }
        }

        /*----------------------------------------------------------------------------*/
        /**
         Exception thrown when an invalid UTF code unit is encountered

         \date   10-24-11 10:34:39

         \author ShivkS
        */
        class InvalidCodeUnitException : public SCXCoreLib::SCXException
        {
        public:
            /*----------------------------------------------------------------------------*/
            /**
             Create an Invalid UTF-8 Code Unit exception

             \param [in]    Invalid Character
            */
            InvalidCodeUnitException(Encoding::Encoding encoding,
                                     unsigned int invalidCodeUnit,
                                     size_t bytePosition,
                                     const std::string& description)
            :
                m_encoding(encoding),
                m_invalidCodeUnit(invalidCodeUnit),
                m_bytePosition(bytePosition),
                m_description(description)
            {}

            ~InvalidCodeUnitException() {};

            /*----------------------------------------------------------------------------*/
            /**
             Get the exception message

             \returns       Exception message
            */
            std::wstring What() const
            {
                std::stringstream message;
                message <<
                    "Invalid "<< GetEncodingString(m_encoding) <<
                    " code unit found : 0x" <<
                    std::setw(2) <<
                    std::hex <<
                    m_invalidCodeUnit;
                message <<
                    " at position : " <<
                    std::dec <<
                    m_bytePosition <<
                    "\n";
                message <<
                    "Description : " <<
                    m_description;
                return SCXCoreLib::StrFromUTF8(message.str());
            }

        private:
            /** Encoding of the current error */
            Encoding::Encoding m_encoding;

            /** The invalid character encountered */
            unsigned int m_invalidCodeUnit;

            /** Position of the invalid character */
            size_t m_bytePosition;

            /** Additional description of the current error */
            std::string m_description;
        };

        // Typedef for defining a Unicode codepoint
        typedef unsigned int CodePoint;

        // A UTF-16 character in the native byte ordering of the CPU
        typedef unsigned short Utf16Char;

        // A UTF-8 byte
        typedef unsigned char Utf8Char;

        /*----------------------------------------------------------------------------*/
        /**
         Constants
        */

        const CodePoint c_CODE_POINT_MAXIMUM_VALUE       =   0x10FFFF;
        const CodePoint c_CODE_POINT_SURROGATE_HIGH_MIN  =   0xD800;
        const CodePoint c_CODE_POINT_SURROGATE_HIGH_MAX  =   0xDBFF;
        const CodePoint c_CODE_POINT_SURROGATE_LOW_MIN   =   0xDC00;
        const CodePoint c_CODE_POINT_SURROGATE_LOW_MAX   =   0xDFFF;

        /*----------------------------------------------------------------------------*/
        /**
         Inline functions
        */

        /*
         A way to test endianness of the CPU that can be evaluated at compile time.  This would not
         work if using a cross-compiler, unless the cross-compiler had the same endianness or it
         simulated the math used on the target CPU.
        */
        const unsigned short s_EndianTestBytes = 0x0100;

        inline bool CPU_IS_BIG_ENDIAN()
        {
            return *(reinterpret_cast<unsigned char const*>(&s_EndianTestBytes)) != 0;
        }

        /*----------------------------------------------------------------------------*/
        /**
         Utility functions
        */

        /*----------------------------------------------------------------------------*/
        /**
         Get the one or two words that make up a UTF-16 code point

         \param [in]     cp  the code point
         \param [out]    word1  the high or only UTF-16 word representing the code point
         \param [out]    word2  the low UTF-16 word representing the code point

         \returns        1 if word1 is the only word, 2 if both word1 and word2 have values
        */
        bool CodePointToUtf16(CodePoint cp, Utf16Char* word1, Utf16Char* word2);

        /*----------------------------------------------------------------------------*/
        /**
         Get the UTF-8 bytes that represent a Unicode code point

         \param [in]     cp  the code point
         \param [in]     str  ptr. to an array of Utf8Char at least 4 bytes in size

         \returns        the number of bytes used in *str to represent the code point
        */
        size_t CodePointToUtf8(CodePoint cp, Utf8Char* str);

        /*----------------------------------------------------------------------------*/
        /**
         Get a Unicode code point value from a UTF-16 string

         \param [in]     str  the base of the UTF-16 string
         \param [in]     size  how many words are in the string
         \param [in]     pos  where in the string to get the code point value
         \param [out]    bytes  how many bytes in the substring the code point occupies

         \returns        the code point value

         \throws         InvalidCodeUnitException
        */
        CodePoint Utf16StringToCodePoint(
            const Utf16Char* str,
            size_t size,
            size_t pos,
            size_t* codePointWords);

        /*----------------------------------------------------------------------------*/
        /**
         Get a Unicode code point value from a UTF-8 string

         \param [in]     str  the base of the UTF-8 string
         \param [in]     size  how many bytes are in the string
         \param [in]     pos  where in the string to get the code point value
         \param [out]    bytes  how many bytes in the substring the code point occupies

         \returns        the code point value

         \throws         InvalidCodeUnitException
        */
        CodePoint Utf8StringToCodePoint(
            const Utf8Char* str,
            size_t size,
            size_t pos,
            size_t* bytes);

        /*----------------------------------------------------------------------------*/
        /**
         Get the position of a code point with a given index in a Utf16 string

         \param [in]     str  the base of the UTF-16 string
         \param [in]     _size  how many words are in the string
         \param [in]     index  which code point's offset to get
         \param [in]     alllowLast  if true, do not throw exception for position at end of string

         \returns        the position of the code point with the given index

         \throws         InvalidCodeUnitException
         \throws         SCXCoreLib::SCXIllegalIndexException<size_t>
        */
        size_t Utf16StringOffsetOfIndex(
            const Utf16Char* str,
            size_t _size,
            size_t index,
            bool allowLast);

        /*----------------------------------------------------------------------------*/
        /**
         Get the number of code points in the current string

         \param [in]    str  the base of the UTF-16 string
         \param [in]    size  how many words are in the string

         \returns       the number of code points in the string

         \throws        InvalidCodeUnitException
        */
        size_t Utf16StringCodePointCount(
            const Utf16Char* str,
            size_t _size);

        /*----------------------------------------------------------------------------*/
        /**
         Get the code point pointed to by a Utf16String iterator

         \param [inout] it  the iterator; it is incremented it it was a 2-word code point

         \returns       the code point
        */
        CodePoint GetCodePoint(
            std::basic_string<Utf16Char>::iterator& it);

        /*----------------------------------------------------------------------------*/
        /**
            Utf16String class
        */
        class Utf16String : public std::basic_string<Utf16Char>
        {

    public:

            typedef CodePoint Char;

            typedef std::basic_string<Utf16Char>::iterator Iterator;

            /*----------------------------------------------------------------------------*/
            /**
             Assign a NUL-terminated UTF-16 character array to the current string

             \param [in]     input  the input array to assign

             \throws         InvalidCodeUnitException
            */
            void Assign(const Utf16Char* str);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a counted-length UTF-16 character array to the current string

             \param [in]     input  the input array to assign
             \param [in]     size  the number of words in the array

             \throws         InvalidCodeUnitException
            */
            void Assign(const Utf16Char* str, size_t _size);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a vector of words in machine byte order to the current string

             \param [in]     str  the input string

             \throws         InvalidCodeUnitException
            */
            void Assign(const std::basic_string<Utf16Char>& str);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a range of words in machine byte order to the current string

             \param [in]     v  the input vector

             \throws         InvalidCodeUnitException
            */
            void Assign(const std::basic_string<Utf16Char>::iterator _begin,
                        const std::basic_string<Utf16Char>::iterator _end);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a vector of bytes representing a UTF-16 string in LE order to the current string

             \param [in]     v  the input vector

             \throws         InvalidCodeUnitException
            */
            void Assign(const std::vector<unsigned char>& v);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a counted-length UTF-8 string to the current string

             \param [in]     str  the input string

             \throws         InvalidCodeUnitException
            */
            void Assign(const Utf8Char* str, size_t _size);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a NUL-terminated UTF-8 string to the current string

             \param [in]     str  the input string

             \throws         InvalidCodeUnitException
            */
            void Assign(const Utf8Char* str);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a std::string of UTF-8 characters to the current string

             \param [in]     str  the input string

             \throws         InvalidCodeUnitException
            */
            void Assign(const std::string& str);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a NUL-terminated UTF-8 string to the current string. This coerces
             the argument to an unsigned char* for platforms where char is signed and
             then calls its Assign.

             \param [in]     str  the input string

             \throws         InvalidCodeUnitException
            */
            void Assign(const char* str)
            {
                Assign((const Utf8Char*)str);
                return;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create an empty Utf16String
            */
            Utf16String()
            {
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf16String from an unsigned short NUL-terminated array in
             UTF-16 encoding, whose endianness matches the host computer

             \param [in]    str  Input character array

             \throws        InvalidCodeUnitException
            */
            Utf16String(const Utf16Char* str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf16String from a string of unsigned characters that represent
             a UTF-16LE string

             \param [in] str  input string

             \throws        InvalidCodeUnitException
            */

            Utf16String(const std::basic_string<Utf16Char>& str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a UTF-16 string from an unsigned char stream, where each pair
             of bytes represents a UTF-16 character in LE form or 1/2 character
             for extended ones.

             \param [in] stream stream of UTF-16 LE characters

             \throws        InvalidCodeUnitException
            */
            Utf16String(const std::vector<unsigned char>& v)
            {
                Assign(v);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Copy constructor for Utf16String

             \param [in]    src  sstring to copy from

             \throws        InvalidCodeUnitException
            */
            Utf16String(const Utf16String& str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utttf8String from a std::string in UTF-8 encoding.

             \param [in]    str  input string

             \throws        InvalidCodeUnitException
            */
            Utf16String(const std::string& str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Unicode string from a NUL-terminated Utf8Char array in UTF-8 encoding

             \param [in]    str  input string

             \throws        InvalidCodeUnitException
            */
            Utf16String(const Utf8Char* str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String from a NUL-terminated string constant in UTF-8 encoding.

             \param [in]    str  input string

             \throws        InvalidCodeUnitException
            */
            Utf16String(const char* str)
            :
            std::basic_string<Utf16Char>()
            {
                Assign(str);
            }


            /*----------------------------------------------------------------------------*/
            /**
             Destructor for Utf16String string

             \throws        InvalidCodeUnitException
            */
            ~Utf16String()
            {
                clear();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Assignment operator for assigning a string of Utf16Chars to a Utf16String

             \param [in]    str  string to assign to this string

             \returns       the new string

             \throws        InvalidCodeUnitException
            */
            Utf16String& operator=(const std::basic_string<Utf16Char>& str)
            {
                Assign(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Assignment operator for assigning one Utf16String to another

             \param [in]    str  string to assign

             \returns       the new string

             \throws        InvalidCodeUnitException
            */

            Utf16String& operator=(const Utf16String& str)
            {
                Assign(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Assign a NUL-terminated UTF-8 string to the current string

             \param [in]    str  the input string

             \returns       the new string

             \throws        InvalidCodeUnitException
            */
            Utf16String& operator=(const Utf8Char* str)
            {
                Assign(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Assign a std::string of UTF-8 characters to the current string

             \param [in]    str  the input string

             \returns       the new string

             \throws        InvalidCodeUnitException
            */
            Utf16String& operator=(const std::string& str)
            {
                Assign(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Assign a NUL-terminated UTF-8 string to the current string

             \param [in]    str  the input string

             \returns       the new string

             \throws        InvalidCodeUnitException
            */
            Utf16String& operator=(const char* str)
            {
                Assign(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Check if the string is empty

             \returns       true if the string is empty

             \throws        InvalidCodeUnitException
            */
            bool Empty() const
            {
                return empty();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Clear the string data
            */
            void Clear()
            {
                clear();
                return;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Reserve a predetermined number of words for the string

             \param [in]    size  no. of words to reserve storage for
            */
            void Reserve(size_t count)
            {
                reserve(count);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Put the UTF-16 representation of a string into a std::baseic_string<unsigned short>

             \returns       the string
            */
            std::basic_string<unsigned short> Str() const
            {
                std::basic_string<unsigned short> str(*this);

                return str;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare two Utf16String strings

             \param [in]    str  string to compare
             \param [in]    caseInsensitive  specifies if the comparison is case-insensistive

             \returns       true if the strings are lexically equal

             \throws        SCXCoreLib::SCXInvalideArgumentException

             \warning       case-insensitive compare is not implemented

             \note          This implementation differs from the std::basic_string one as
                            this returns a Boolean comparision instead of a
                            dictionary-order comparision
            */
            bool Compare(const Utf16String& str, bool caseInsensitive = false) const
            {
                if (&str == this)
                {
                    return true;
                }

                if (caseInsensitive)
                {                       // this can be implemented using the locale-independent
                                        // case-insensitive code point comparison from the PAL
                                        // if needed
                    throw SCXCoreLib::SCXInvalidArgumentException(L"caseInsensitive",
                                                                  L"This functionality has not been implemented yet",
                                                                  SCXSRCLOCATION);
                }

                return compare(str) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a substring to a string

             \param [in]    pos  position to start comparing from (Unicode Characeter Poistion)
             \param [in]    n  number of characters to compare (Unicode characters)
             \param [in]    caseInsensitive  specifies if the comparison is case sensistive

             \returns       true if the strings are lexically equal

             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            bool Compare(size_t pos, size_t n, Utf16String& str, bool caseInsensitive = false)
            {
                if (&str == this)
                {
                    return true;
                }

                if (caseInsensitive)
                {
                    throw SCXCoreLib::SCXInvalidArgumentException(L"caseInsensitive",
                                                                  L"This functionality has not been implemented yet",
                                                                  SCXSRCLOCATION);
                }

                // If pos > size of the string, throw an exception
                if (pos > size())
                {
                   throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
                }

                return compare(pos, n, str) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a Utf16String to the current string

             \param [in]    str  Unicode string to append

             \returns       the appended string (reference to the current string)
            */
            Utf16String& Append(const Utf16String& str)
            {
                append(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a Unicode character to the current string

             \param [in]    cp  Unicode character to append

             \returns       the appended string (reference to the current string)
            */
            Utf16String& Append(const CodePoint& cp)
            {
                Utf16Char word1, word2;
                if (!CodePointToUtf16(cp, &word1, &word2))
                {
                    append(1, (Utf16Char)cp);
                }
                else
                {
                    append(1, word1);
                    append(2, word2);
                }


                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a character array to the Unicode string

             \param [in]    right  character array to append (This must be NUL-terminated)

             \returns       The appended string (The pointer to the current string)
            */
            Utf16String& operator+=(const Utf16Char* right)
            {
                return Append(right);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a basic_string of Utf16Char values to the current string

             \param [in]    str  basic_string<unsigned short> to append

             \returns       the appended string (The pointer to the current string)
            */
            Utf16String& operator+=(const std::basic_string<Utf16Char>& str)
            {
                append(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a Utf16String to the current String

             \param [in]    str  string to append

             \returns       the appended string (The pointer to the current string)
            */
            Utf16String& operator+=(const Utf16String& str)
            {
                append(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
                Append a Unicode character to the current string

             \param [in]    cp  the Unicode character to append

             \returns       the appended string (reference to the current string)
            */

            Utf16String& operator+=(const CodePoint& cp)
            {
             return Append(cp);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Returns a substring of [pos, pos+count) characters/character halves

             \param [in]    pos  position of the first character to include
             \param [in]    count  length of the substring

             \returns       string containing the substring [pos, pos+count)

             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            Utf16String SubStr(size_t pos = 0, size_t count = std::string::npos)
            {
                if (pos > size())
                {
                    throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
                }

                // don't make a temporary if the substring is simply the current string
                if (pos == 0 && count == std::string::npos)
                {
                    return *this;
                }

                // make and return a temporary containing the substring
                if (pos + count > size())
                {
                    count = size() - pos;
                }
                Utf16String s;
                s.assign(substr(pos, count));

                return s;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Erase a part of the Utf16String

             \param [in]    pos  position to start deleting
             \param [in]    count  number of characters to delete

             \returns       Reference to the erased string

             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            Utf16String Erase(size_t pos = 0, size_t count = std::string::npos)
            {
                if (pos == 0 && count == std::string::npos)
                {
                    clear();
                }
                else
                {
                    if (pos >= size())
                    {
                        throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
                    }
                    erase(pos, count);
                }
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Trim the current string on both ends

             \note          In the Unicode specification about 26 characters are designated
                            white space. The current implementation only handles the ASCII
                            space characters: U+0009, U+000A, U+000B, U+000C, U+000D, U+0020.

                            If a Unicode trim is needed, the support can be added.
            */
            void Trim();

            /*----------------------------------------------------------------------------*/
            /**
             Search for a code point character in the string

             \param [in]    cp  Character to search for
             \param [in]    pos  position to search from

             \returns       the index of the first Utf16Char in the substring

             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            size_t Find(CodePoint cp, size_t pos = 0) const;

            /*----------------------------------------------------------------------------*/
            /**
             Search the current string for the substring

             \param [in]    str  The substring to search for
             \param [in]    pos  The position to start search from

             \returns       the index of the first Utf16Char in the substring

             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            size_t Find(Utf16String& str, size_t pos = 0);

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String to the current string for equality

             \param [in]    right  Utf16String string to compare with

             \returns       true if the strings are lexically equal
            */
            bool operator==(const Utf16String& right) const
            {
                if (&right == this)
                {
                    return true;
                }

                return compare(right) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String to the current string for inequality

             \param [in]    right  Utf16String string to compare with

             \returns       true if the strings are lexically not equal
            */
            bool operator!=(Utf16String& right) const
            {
                if (&right == this)
                {
                    return false;
                }

                return compare(right) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String and a std::string for equality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are equal
            */
            bool operator==(const std::string& right) const
            {
                Utf16String s(right);
                return compare(s) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String and a std::string for inequality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are not equal
            */
            bool operator!=(const std::string& right) const
            {
                Utf16String s(right);
                return compare(s) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String and a NUL-terminated UTF-8 string for equality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are equal
            */
            bool operator==(const char* right) const
            {
                Utf16String s(right);
                return compare(s) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf16String and a NUL-terminated UTF-8 string for inequality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are not equal
            */
            bool operator!=(const char* right) const
            {
                Utf16String s(right);
                return compare(s) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Less than operator for use in STL. STL derives <=, > and >= from this and
             the == operator

             \param [in]    right  string to compare

             \returns       true if current string is lexically less than the one passed in
            */
            bool operator<(const Utf16String& right) const
            {
                return (compare(right) < 0);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the UTF-16 representation of a string into a std::vector<unsigned char>
             Each pair of elements in the vector represent one character, in
             low-endian order.

             \param [out]   v  Where to write the data
             \param [in]    addBOM  true to prepend a byte order mark
            */
            void Write(std::vector<unsigned char>& v, bool addBOM = true) const;

            /*----------------------------------------------------------------------------*/
            /*
             Code that works on code points instead of on words.  Most application won't
             need this and should use the stl functions, begin, end, at, etc., instead of
             these for better performance.
            */

            /*----------------------------------------------------------------------------*/
            /**
             Get the code point iterator to the first character to the string

             \returns       an iterator to the beginning of the string
            */
            std::basic_string<Utf16Char>::iterator Begin()
            {
                return begin();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the code point iterator to the end of the string

             \returns       an iterator to the end of the string + 1
            */
            std::basic_string<Utf16Char>::iterator End()
            {
                return end();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the number of 16-bit words in the string

             \returns       size number of words in the string
            */
            size_t Size() const
            {
                return size();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the number of code points in the current string

             \returns       the number of code points in the string

             \throws        InvalidCodeUnitException
            */
            size_t CodePoints() const
            {
                return Utf16StringCodePointCount(&(*this)[0], size());
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the code point at an offset, in code points, from the beginning of
             the current string

             \param [in]    which code point to get

             \returns       the code point at the given offset

             \throws        InvalidCodeUnitException
             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            CodePoint GetCodePoint(size_t pos)
            {
                size_t codePointWords;
                return Utf16StringToCodePoint(&(*this)[0], size() - pos, pos, &codePointWords);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the code point at an offset, in code points, from the beginning of
             the current string

             \param [in]    which code point to get

             \returns       the code point at the given offset

             \throws        InvalidCodeUnitException
             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            CodePoint GetCodePointAtIndex(size_t index)
            {
                size_t pos = Utf16StringOffsetOfIndex(&(*this)[0], size(), index, false);
                size_t codePointWords;
                return Utf16StringToCodePoint(&(*this)[0], size(), pos, &codePointWords);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Get the code point at an offset, in code points, from the beginning of
             the current string

             \param [in]    which code point to get

             \returns       the code point at the given offset

             \throws        InvalidCodeUnitException
             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            CodePoint GetCodePointAtIndex(int index)
            {
                return GetCodePointAtIndex((size_t)index);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Set the code point at an offset, in code points, from the beginning of
             the current string

             \param [in]    which code point to get

             \returns       the code point at the given offset

             \throws        InvalidCodeUnitException
             \throws        SCXCoreLib::SCXIllegalIndexException<size_t>
            */
            void SetCodePointAtIndex(size_t index, CodePoint cp);

            /*----------------------------------------------------------------------------*/
            /**
             Return the wide string equivalent of a Utf16string

             This code converts a Utf16String to a std::wstring by iterating through the
             underlying UTF-16 character string and passing over the extension bytes as needed.

             \returns       the wide (UTF-32) string equivalent
            */
            void ToWideString(std::wstring& wstr)
            {
                size_t utf16Chars = size();
                size_t codePointWords;
                unsigned int codePoint;

                wstr.reserve(utf16Chars);
                for (size_t index = 0; index < utf16Chars; )
                {
                    codePoint = Utf16StringToCodePoint(&(*this)[0], utf16Chars, index, &codePointWords);
                    wstr += (wchar_t)codePoint;
                    index += codePointWords;                
                }
                return;
            }

        };

        /*----------------------------------------------------------------------------*/
        /**
         Utf8String class
        */
        class Utf8String : public Utf16String
        {

    public:

            typedef CodePoint Char;

            typedef std::basic_string<Utf16Char>::iterator Iterator;

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String string from an unsigned char stream encoded in UTF-8

             This is needed because the matching Utf16String Assign has a different
             interpretaion of the input vector

             \param [in]    stream  stream of Unicode characters
            */
            void Assign(const std::vector<unsigned char>& v);

            /*----------------------------------------------------------------------------*/
            /**
             Assign a range of words in machine byte order to the current string

             \param [in]     v  the input vector

             \throws         InvalidCodeUnitException
            */
            void Assign(const std::basic_string<Utf16Char>::iterator _begin,
                        const std::basic_string<Utf16Char>::iterator _end);

            /*----------------------------------------------------------------------------*/
            /**
             Create an empty Utf8String
            */
            Utf8String()
            {
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String string from a Utf8Char array of characters in UTF-8 encoding

             \param [in]    str  input character array
            */
            Utf8String(const Utf8Char* str)
            :
            Utf16String()
            {
                Utf16String::Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String string from a Utf8Char array of characters in UTF-8 encoding

             \param [in]    str  input character array
            */
            Utf8String(const char* str)
            :
            Utf16String()
            {
                Utf16String::Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String string from a std::string of characters in UTF-8 encoding

             \param [in]    str  input character array
            */
            Utf8String(const std::string& str)
            :
            Utf16String()
            {
                Utf16String::Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String from an unsigned short array in UTF-16 encoding, whose
             endianness matches the host computer

             \param [in]    str  input character array
            */
            Utf8String(const Utf16Char* str)
            :
            Utf16String()
            {
                Utf16String::Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String from a std::basic_string<Char16>

             \param [in]    str  input string
            */
            Utf8String(const std::basic_string<Utf16Char>& str)
            :
            Utf16String()
            {
                Utf16String::Assign(str);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Create a Utf8String string from an unsigned char stream encoded in UTF-8

             \param [in]    stream  stream of Unicode characters
            */
            Utf8String(const std::vector<unsigned char>& v)
            :
            Utf16String()               // this is needed because the matching Utf16String
                                        // constructor has a different interpretaion
                                        // of the vector
            {
                Assign(v);
            }

            /*----------------------------------------------------------------------------*/
            /**
             Copy constructor for Utf8String

             \param [in]    str  string to copy from
            */
            Utf8String(const Utf8String& str)
            :
            Utf16String()
            {
                if (&str != this)
                {
                    assign(&str[0], str.size());
                }
            }

            /*----------------------------------------------------------------------------*/
            /**
             Destructor for Utf8String
            */
            ~Utf8String()
            {
                clear();
            }

            /*----------------------------------------------------------------------------*/
            /**
             Put the UTF-8 representation of a string into a std::string

             \returns       the string

             \throws        InvalidCodeUnitException
            */
            std::string Str() const;

            /*----------------------------------------------------------------------------*/
            /**
             Put the UTF-8 representation of a string into a std::vector<unsigned char>

             \param [out]   v  Where to write the data
             \param [in]    addBOM  True to add the Byte Order Mark

             \throws        InvalidCodeUnitException
            */
            void Write(std::vector<unsigned char>& v, bool addBOM = true) const;

            /*----------------------------------------------------------------------------*/
            /**
             Put the UTF-8 representation of the current string into a std::ostream

             \param [out]   v  Where to write the data
             \param [in]    addBOM  True to add the Byte Order Mark

             \throws        InvalidCodeUnitException
            */
            void Write(std::ostream& stream, bool addBOM = true) const;

            /*----------------------------------------------------------------------------*/
            /**
             Compare two Utf8Strings for equality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are equal
            */
            bool operator==(const Utf8String& right) const
            {
                if (&right == this)
                {
                    return true;
                }
                return compare(right) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare two Utf8Strings for inequality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are not equal
            */
            bool operator!=(const Utf8String& right) const
            {
                if (&right == this)
                {
                    return false;
                }
                return compare(right) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf8String and a std::string for equality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are equal
            */
            bool operator==(const std::string& right) const
            {
                Utf8String s(right);
                return compare(s) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf8String and a std::string for inequality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are not equal
            */
            bool operator!=(const std::string& right) const
            {
                Utf8String s(right);
                return compare(s) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf8String and a NUL-terminated UTF-8 string for equality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are equal
            */
            bool operator==(const char* right) const
            {
                Utf8String s(right);
                return compare(s) == 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Compare a Utf8String and a std::string for inequality

             \param [in]    right  string to compare to current string

             \returns       true if the strings are not equal
            */
            bool operator!=(const char* right) const
            {
                Utf8String s(right);
                return compare(s) != 0;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a Utf8String to the current string

             \param [in]    str  string to append

             \returns       the current string after appending
            */
            Utf8String& Append(const Utf8String& str)
            {
                append(str);
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Append a Unicode character to the current string

             \param [in]    cp  Unicode character to append

             \returns       the appended string (reference to the current string)
            */
            Utf8String& Append(const CodePoint& cp)
            {
                if (cp < 0x00010000)
                {
                    append(1, (Utf16Char)cp);
                }
                else
                {
                    CodePoint cp2 = cp - 0x00010000;
                    append(1, (Utf16Char)(((cp2 >> 10) & 0x000003FF) + c_CODE_POINT_SURROGATE_HIGH_MIN));
                    append(2, (Utf16Char)((cp2 & 0x000003FF) + c_CODE_POINT_SURROGATE_LOW_MIN));
                }
                return *this;
            }

            /*----------------------------------------------------------------------------*/
            /**
             Return the wide string equivalent of a Utf8string

             \returns       the wide (UTF-32) string equivalent
            */
            void ToWideString(std::wstring& wstr)
            {
                Utf16String::ToWideString(wstr);
                return;
            }

        };

        /*----------------------------------------------------------------------------*/
        /**
         Put a Utf8String into an output stream

         \param [inout] os  stream to add to
         \param [in]    str  string to add

         \returns       the reference to the passed-in stream
        */
        inline std::ostream& operator<<(std::ostream& os, const Utf8String& str)
        {
            str.Write(os);
            return os;
        }

    }
}

#endif // UNICODE_H
