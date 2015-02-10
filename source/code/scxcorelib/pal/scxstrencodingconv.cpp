/*------------------------------------------------------------------------------
        Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
        \file

        \brief     Implements the string encoding conversion PAL

        \date      07-Jan-2011 09:30 PST
*/
/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxstrencodingconv.h>
#include <scxcorelib/scxexception.h>

/*----------------------------------------------------------------------------*/
/**
 Local Type
*/
#if defined(OS_WINDOWS)
typedef unsigned long CodePoint;
#else
typedef unsigned int CodePoint;
#endif

/*----------------------------------------------------------------------------*/
/**
 Local Constants
*/

static const CodePoint s_CODE_POINT_MAXIMUM_VALUE       =   0x10FFFF;
static const CodePoint s_CODE_POINT_SURROGATE_HIGH_MIN  =   0xD800;
static const CodePoint s_CODE_POINT_SURROGATE_HIGH_MAX  =   0xDBFF;
static const CodePoint s_CODE_POINT_SURROGATE_LOW_MIN   =   0xDC00;
static const CodePoint s_CODE_POINT_SURROGATE_LOW_MAX   =   0xDFFF;

/*----------------------------------------------------------------------------*/
/**
 Inline functions
*/

/*
 A way to test endianness of the CPU that can be evaluated at compile time.
 This would not work if using a cross-compiler, unless the cross-compiler had
 the same endianness or it simulated the math used on the target CPU.
*/
static const unsigned short s_EndianTestBytes = 0x0100;

static inline bool CPU_IS_BIG_ENDIAN()
{
    return (*(const unsigned char*)&s_EndianTestBytes != 0);
}

/*----------------------------------------------------------------------------*/
/**
 Exception thrown when an invalid UTF code unit is encountered
*/
namespace SCXCoreLib
{
    class InvalidCodeUnitException : public SCXCoreLib::SCXException
    {
private:
        /** Encoding of the current error */
        std::wstring m_encoding;

        /** The invalid character encountered */
        CodePoint m_invalidCodeUnit;

        /** Position of the invalid character */
        size_t m_bytePosition;

        /** Additional description of the current error */
        std::wstring m_description;

public:
        /*----------------------------------------------------------------------------*/
        /**
         Create an invalid UTF-8 code unit exception

         \param [in]    The invalid character
        */
        InvalidCodeUnitException(const std::wstring& encoding,
                                 CodePoint invalidCodeUnit,
                                 size_t bytePosition,
                                 const std::wstring& description)
        :
            m_encoding(encoding),
            m_invalidCodeUnit(invalidCodeUnit),
            m_bytePosition(bytePosition),
            m_description(description)
        {
        }

        /*----------------------------------------------------------------------------*/
        /**
         Get the exception message

         \returns       Exception message
        */
        std::wstring What() const
        {
            std::wstringstream s;
            s <<
                L"Invalid code unit " <<
                std::hex << m_invalidCodeUnit <<
                L" in " << m_encoding.c_str() <<
                L" at offset " <<
                m_bytePosition <<
                L". " <<
                m_description.c_str();
            return s.str();
        }
    };
}

/*----------------------------------------------------------------------------*/
/**
 File-scope functions
*/

/*----------------------------------------------------------------------------*/
/**
 Get a Unicode code point value from a UTF-8 string

 \param [in]     str  the base of the UTF-8 string
 \param [in]     size  how many bytes are in the string
 \param [in]     pos  where in the string to get the code point value
 \param [out]    bytes  how many bytes in the substring the code point occupies

 \returns        the code point value

 \throws         SCXCoreLib::InvalidCodeUnitException
*/
static CodePoint Utf8StringToCodePoint(
    const char* str,
    size_t size,
    size_t pos,
    size_t* bytes)
{
    if (pos >= size)
    {
        *bytes = 1;
        return (CodePoint)'\0';
    }
    CodePoint cp = (CodePoint)(unsigned char)*(str + pos);
    if (cp < 0x00000080)
    {
        *bytes = 1;
        return cp;
    }
    if (cp < 0x000000C2 || cp >= 0x000000F5)
    {
        throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"invalid prefix byte");
    }
    if (cp < 0x000000E0)
    {
        if (pos + 1 >= size || (*(str + pos + 1) & 0xC0) != 0x80)
        {
            throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"dangling 2-byte prefix byte");
        }
        *bytes = 2;
        return ((cp & 0x0000003F) << 6) | ((CodePoint)(unsigned char)*(str + pos + 1) & 0x0000003F);
    }
    if (cp < 0x000000F0)
    {
        if (pos + 2 >= size || (*(str + pos + 1) & 0xC0) != 0x80 || (*(str + pos + 2) & 0xC0) != 0x80)
        {
            throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"dangling 3-byte prefix byte");
        }
        *bytes = 3;
        cp = ((cp & 0x0000000F) << 12) |
             (((CodePoint)(unsigned char)*(str + pos + 1) & 0x0000003F) << 6) |
             ( (CodePoint)(unsigned char)*(str + pos + 2) & 0x0000003F);
        if (cp < 0x00000800 ||          // illegal duplicate of shorter sequence or in surrogate region
            (cp >= s_CODE_POINT_SURROGATE_HIGH_MIN && cp <= s_CODE_POINT_SURROGATE_LOW_MAX))
        {
            throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"overlong form");
        }
        return cp;
    }
    if (pos + 3 >= size ||
        (*(str + pos + 1) & 0xC0) != 0x80 ||
        (*(str + pos + 2) & 0xC0) != 0x80 ||
        (*(str + pos + 3) & 0xC0) != 0x80)
    {
        throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"dangling 4-byte prefix byte");
    }
    *bytes = 4;
    cp = ((cp & 0x00000007) << 18) |
         (((CodePoint)(unsigned char)*(str + pos + 1) & 0x0000003F) << 12) |
         (((CodePoint)(unsigned char)*(str + pos + 2) & 0x0000003F) << 6) |
          ((CodePoint)(unsigned char)*(str + pos + 3) & 0x0000003F);
    if (cp < 0x00010000 || cp > s_CODE_POINT_MAXIMUM_VALUE) // illegal duplicate of shorter sequence or above Unicode range
    {
        throw SCXCoreLib::InvalidCodeUnitException(L"UTF-8", cp, pos, L"overlong form or out of range");
    }
    return cp;
}

/*----------------------------------------------------------------------------*/
/**
 Convert a UTF-8 string to a UTF-16 string

 \param [in]     utf8  the UTF-8 string
 \param [in]     utf8Bytes  the number of bytes in *utf8 or -1 to stop on a NUL character
 \param [in]     swapBytes  true if the bytes in the output vector should be swapped
 \param [inout]  firstNonAscii  the position in utf8 of the first non-ASCII character
 \param [out]    utf16  the UTF-16 string; NULL for the first (evaluation) pass

 \throws         SCCCoreLib::InvalidCodeUnitException

 \returns        the size of the converted string
*/
static size_t Utf8ToUtf16Conv(
    const char* utf8,
    size_t utf8Bytes,
    bool swapBytes,
    size_t* firstNonAscii,
    unsigned short* utf16)
{
    unsigned char c;
    unsigned short* p = utf16;

    // turn off the ASCII-only code for the first pass
    size_t firstNonAsciiChar = utf16 == NULL ? 0 : *firstNonAscii;

    // do the ASCII-only conversion for the beginning characters
    for (size_t i = 0; i < firstNonAsciiChar; i++)
    {
        *p++ = (unsigned short)(unsigned char)*(utf8 + i);
    }

    // If this is the first pass or firstNonAsciiChar < size, handle the rest of the string,
    // which includes multi-byte characters.  If the string was found to be pure ASCII on the
    // first pass, this is never executed on the second pass
    *firstNonAscii = 0x7FFFFFFF;
    size_t i;
    size_t bytes;
    for (i = firstNonAsciiChar; i < utf8Bytes; )
    {
        c = (unsigned char)*(utf8 + i);
        if (c < 0x80)
        {
            if (utf16 != NULL)
            {
                *p = (unsigned short)(unsigned char)*(utf8 + i);
            }
            p++;
            i++;
        }
        else
        {
            if (i < *firstNonAscii)
            {
                *firstNonAscii = i;
            }
            CodePoint cp = Utf8StringToCodePoint(utf8, utf8Bytes, i, &bytes);
            if (cp < 0x00010000)
            {
                if (utf16 != NULL)
                {
                    *p = (unsigned short)cp;
                }
                p++;
            }
            else
            {
                if (utf16 != NULL)
                {
                    cp -= 0x00010000;
                    *p = (unsigned short)(((cp >> 10) & 0x000003FF) + s_CODE_POINT_SURROGATE_HIGH_MIN);
                    *(p + 1) = (unsigned short)((cp & 0x000003FF) + s_CODE_POINT_SURROGATE_LOW_MIN);
                }
                p += 2;
            }
            i += bytes;
        }
    }
    if (i < *firstNonAscii)
    {                                   // if all ASCII, set the value to the string size
        *firstNonAscii = i;
    }

    size_t utf16Chars = (size_t)(p - utf16);
    if (utf16 != NULL && swapBytes)
    {
        swab((char*)utf16, (char*)utf16, utf16Chars * 2);
    }

    return utf16Chars;
}

/*----------------------------------------------------------------------------*/
/**
 Get a Unicode code point value from a UTF-16 string

 \param [in]     str  the base of the UTF-16 string
 \param [in]     size  how many words are in the string
 \param [in]     pos  where in the string to get the code point value
 \param [out]    bytes  how many bytes in the substring the code point occupies

 \returns        the code point value

 \throws         SCXCoreLib::InvalidCodeUnitException
*/
static CodePoint Utf16StringToCodePoint(
    const unsigned short* str,
    size_t size,
    size_t pos,
    size_t* codePointWords)
{
    CodePoint cp = (CodePoint)*(str + pos);

    *codePointWords = 1;
    if (cp >= s_CODE_POINT_SURROGATE_HIGH_MIN && cp <= s_CODE_POINT_SURROGATE_HIGH_MAX)
    {
        if (pos + 1 >= size ||
            *(str + pos + 1) < s_CODE_POINT_SURROGATE_LOW_MIN || *(str + pos + 1) > s_CODE_POINT_SURROGATE_LOW_MAX)
        {
            throw SCXCoreLib::InvalidCodeUnitException(L"UTF-16", cp, pos, L"Dangling surrogate high character");
        }
        cp &= 0x000003FF;
        cp <<= 10;
        cp |= (CodePoint)*(str + pos + 1) & 0x000003FF;
        cp += 0x00010000;
        *codePointWords = 2;
        if (cp > s_CODE_POINT_MAXIMUM_VALUE)
        {
            throw SCXCoreLib::InvalidCodeUnitException(L"UTF-16", cp, pos, L"Dangling surrogate low character");
        }
    }
    else if (cp >= s_CODE_POINT_SURROGATE_LOW_MIN &&  cp <= s_CODE_POINT_SURROGATE_LOW_MAX)
    {
        throw SCXCoreLib::InvalidCodeUnitException(L"UTF-16", cp, pos, L"Dangling surrogate low character");
    }
    return cp;
}

/*----------------------------------------------------------------------------*/
/**
 Convert a UTF-16 string to a UTF-8 string

 \param [in]     utf16  the UTF-16 string
 \param [out]    _size  the number of words in the string
 \param [inout]  firstNonAscii  the position in utf8 of the first non-ASCII character
 \param [out]    utf8   the UTF-8 string; NULL for the first (evaluation) pass

 \returns        the size of the converted string

 \throws         InvalidCodeUnitException
*/
static size_t Utf16ToUtf8Conv(
    const unsigned short* utf16,
    size_t inputSize,
    size_t* firstNonAscii,
    unsigned char* utf8)
{
    unsigned short c;
    unsigned char* p = utf8;
    size_t codePointWords;
    // turn off the ASCII-only code for the first pass
    size_t firstNonAsciiChar = utf8 == NULL ? 0 : *firstNonAscii;

    // do the ASCII-only conversion for the beginning characters on the second pass
    for (size_t pos = 0; pos < firstNonAsciiChar; pos++)
    {
        *p++ = (unsigned char)*(utf16 + pos);
    }

    // If this is the first pass or firstNonAsciiChar < _size, handle the
    // rest of the string, which includes multi-byte characters.  If the
    // string was found to be pure ASCII on the first pass, this is never
    // executed on the second pass
    *firstNonAscii = inputSize;
    for (size_t pos = firstNonAsciiChar; pos < inputSize; )
    {
        c = *(utf16 + pos);
        if (c < 0x0080)
        {                               // an ASCII character
            if (utf8 != NULL)
            {
                *p = (unsigned char)c;
            }
            p++;
            pos++;
        }
        else
        {
            if (pos < *firstNonAscii)
            {
                *firstNonAscii = pos;
            }
            if (c < 0x0800)
            {                           // a 2-byte character
                if (utf8 != NULL)
                {
                    *p = (unsigned char)((c >> 6) | 0x00C0);
                    *(p + 1) = (unsigned char)((c & 0x003F) | 0x0080);
                }
                p += 2;
                pos++;
            }
            else
            {                           // a 3-byte or 4-byte character, perhaps in the extended region
                CodePoint cp = Utf16StringToCodePoint(utf16, inputSize, pos, &codePointWords);
                if (cp <= 0x00010000)
                {                       // a 3-byte character
                    if (utf8 != NULL)
                    {
                        *p = (unsigned char)(((cp >> 12) & 0x0000000F) | 0x000000E0);
                        *(p + 1) = (unsigned char)(((cp >> 6) & 0x0000003F) | 0x00000080);
                        *(p + 2) = (unsigned char)((cp & 0x0000003F) | 0x00000080);
                    }
                    p += 3;
                }
                else
                {                       // a 4-byte character
                    if (utf8 != NULL)
                    {
                        *p = (unsigned char)(((cp >> 18) & 0x00000007) | 0x000000F0);
                        *(p + 1) = (unsigned char)(((cp >> 12) & 0x0000003F) | 0x00000080);
                        *(p + 2) = (unsigned char)(((cp >> 6) & 0x0000003F) | 0x00000080);
                        *(p + 3) = (unsigned char)((cp & 0x0000003F) | 0x00000080);
                    }
                    p += 4;
                }
                pos += codePointWords;
            }
        }
    }
    return (size_t)(p - utf8);
}

/*----------------------------------------------------------------------------*/
/**
   Conversion from a UTF-16 native-endian encoded byte stream with no leading
   byte-order mark to a UTF-8encoded string with no leading byte-order mark.

   \date        2013-02-06 09:30 PST

   \param [in]  inUtf16LEBytes  A std::vector containing the UTF-16LE encoded bytes
   \param [out] outUtf8Str  A std::string containing a UTF-8 encoded string

   \returns     true if the conversion was successful.
*/
static bool Utf16nativeToUtf8(const std::vector<unsigned char>& inUtf16Bytes, std::string& outUtf8Str)
{
    try
    {
        size_t firstNonAsciiPos;

        size_t utf8Bytes = Utf16ToUtf8Conv((const unsigned short*)&inUtf16Bytes[0],
                                           inUtf16Bytes.size() / 2,
                                           &firstNonAsciiPos,
                                           NULL);
        if (utf8Bytes == 0)
        {
            outUtf8Str.clear();
        }
        else
        {
            outUtf8Str.assign(utf8Bytes, '\0');
            (void)Utf16ToUtf8Conv((const unsigned short*)&inUtf16Bytes[0],
                                  inUtf16Bytes.size() / 2,
                                  &firstNonAsciiPos,
                                  (unsigned char*)&outUtf8Str[0]);
        }
    }
    catch (SCXCoreLib::InvalidCodeUnitException &e)
    {
        return false;
    }

    return true;
}

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Conversion from a UTF-8 encoded string with no leading byte-order mark to
       a UTF-16 little-endian encoded byte stream with the correct byte-order mark.

       \date        2013-02-06 09:30 PST

       \param [in]  inUtf8Str  A std::string containing a UTF-8-encoded string
       \param [out] outUtf16Bytes  A std::vector containing the UTF-16-encoded
                                   bytes with a leading BOM. The ordering will be LE.
       \returns     true if the conversion was successful.
    */
    bool Utf8ToUtf16(const std::string& inUtf8Str, std::vector<unsigned char>& outUtf16Bytes)
    {
        size_t inputSize = inUtf8Str.size();
        size_t firstNonAscii;

        try
        {
            size_t utf16Chars = Utf8ToUtf16Conv(&inUtf8Str[0],
                                                inputSize,
                                                CPU_IS_BIG_ENDIAN(),
                                                &firstNonAscii,
                                                NULL);
            if (utf16Chars == 0)
            {
                outUtf16Bytes.clear();
            }
            else
            {
                (void)outUtf16Bytes.assign(2 + utf16Chars * 2, 0);
                outUtf16Bytes[0] = 0xFF;
                outUtf16Bytes[1] = 0xFE;
                (void)Utf8ToUtf16Conv(&inUtf8Str[0],
                                      inputSize,
                                      CPU_IS_BIG_ENDIAN(),
                                      &firstNonAscii,
                                      (unsigned short*)&outUtf16Bytes[2]);
            }
        }
        catch (SCXCoreLib::InvalidCodeUnitException &e)
        {
            return false;
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Conversion from a UTF-8 encoded string with no leading byte-order mark to
       a UTF-16 little-endian encoded byte stream with no leading byte-order mark.

       \date        2013-02-06 09:30 PST

       \param [in]  inUtf8Str  A std::string containing a UTF-8-encoded string
       \param [out] outUtf16LEBytes  A std::vector containing the UTF-16LE-encoded
                                     bytes.
       \returns     true if the conversion was successful.
    */
    bool Utf8ToUtf16le(const std::string& inUtf8Str, std::vector<unsigned char>& outUtf16LEBytes)
    {
        size_t inputSize = inUtf8Str.size();
        size_t firstNonAscii;

        try
        {
            size_t utf16Chars = Utf8ToUtf16Conv(&inUtf8Str[0],
                                                inputSize,
                                                CPU_IS_BIG_ENDIAN(),
                                                &firstNonAscii,
                                                NULL);
            if (utf16Chars == 0)
            {
                outUtf16LEBytes.clear();
            }
            else
            {
                (void)outUtf16LEBytes.assign(utf16Chars * 2, 0);
                (void)Utf8ToUtf16Conv(&inUtf8Str[0],
                                      inputSize,
                                      CPU_IS_BIG_ENDIAN(),
                                      &firstNonAscii,
                                      (unsigned short*)&outUtf16LEBytes[0]);
            }
        }
        catch (SCXCoreLib::InvalidCodeUnitException &e)
        {
            return false;
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Conversion from a UTF-16 little-endian encoded byte stream with no leading
       byte-order mark to a UTF-8 encoded string with no leading byte-order mark.

       \date        2013-02-06 09:30 PST

       \param [in]  inUtf16LEBytes  A std::vector containing the UTF-16LE encoded bytes
       \param [out] outUtf8Str  A std::string containing a UTF-8 encoded string

       \returns     true if the conversion was successful.
    */
    bool Utf16leToUtf8(const std::vector<unsigned char>& inUtf16LEBytes, std::string& outUtf8Str)
    {
        // Does the input have an odd number of bytes? Error if so.
        if ((inUtf16LEBytes.size() & 1) != 0)
        {
            outUtf8Str.clear();
            return false;
        }

        // Swap byte order ot native order and convert
        if (CPU_IS_BIG_ENDIAN())
        {
            std::vector<unsigned char> v(inUtf16LEBytes.begin(), inUtf16LEBytes.end());

            swab((char*)&v[0], (char*)&v[0], inUtf16LEBytes.size());
            return Utf16nativeToUtf8(v, outUtf8Str);
        }

        return Utf16nativeToUtf8(inUtf16LEBytes, outUtf8Str);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Conversion from a UTF-16 encoded byte stream with a leading byte-order mark
       to a UTF-8 encoded string with no leading byte-order mark.

       \date        2013-02-06 09:30 PST

       \param [in]  inUtf16Bytes  A std::vector containing the UTF-16-encoded bytes
       \param [out] outUtf8Str  A std::string containing a UTF-8-encoded string

       \returns     true  if the conversion was successful.
    */
    bool Utf16ToUtf8(const std::vector<unsigned char>& inUtf16Bytes, std::string& outUtf8Str)
    {
        // Is the incoming string empty or does it have an odd no. of bytes?
        //  That's invalid, so return false.
        if (inUtf16Bytes.size() < 2 || (inUtf16Bytes.size() & 1) != 0)
        {
            outUtf8Str.clear();
            return false;
        }

        // Check for valid byte-order mark
        bool beInput;
        if (inUtf16Bytes[0] == 0xFE && inUtf16Bytes[1] == 0xFF)
        {
            beInput = true;
        }
        else if (inUtf16Bytes[0] == 0xFF && inUtf16Bytes[1] == 0xFE)
        {
            beInput = false;
        }
        else
        {
            return false;
        }

        // Make a new vector without the byte-order bytes
        std::vector<unsigned char> utf16Bytes(inUtf16Bytes.begin() + 2, inUtf16Bytes.end());
printf("Utf16ToUtf8 - CPU_IS_BIG_ENDIAN: %d, beInput: %d\n", CPU_IS_BIG_ENDIAN(), beInput);
        // If the input endianness does not match the CPU endianess, swap
        if (CPU_IS_BIG_ENDIAN() ? !beInput : beInput)
        {
            // convert to CPU endianness
            swab((char*)&utf16Bytes[0], (char*)&utf16Bytes[0], utf16Bytes.size());
        }

        return Utf16nativeToUtf8(utf16Bytes, outUtf8Str);
    }

} /* namespace SCXCoreLib */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
