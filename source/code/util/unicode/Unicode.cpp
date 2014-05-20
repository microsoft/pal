/*----------------------------------------------------------------------------
 Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 \file      Unicode.cpp

 \brief     Support for conversions between different representations of Unicode

 \date      2012-09-10 12:00:00

 \author    storcutt
*/

#include <stddef.h>

#include <string>
#include <util/Unicode.h>

using namespace SCX::Util;
using namespace SCX::Util::Encoding;

/*----------------------------------------------------------------------------*/
/**
 Local functions
*/

/*----------------------------------------------------------------------------*/
/**
 Check a UTF-16 string format

 \param [in]     str  the base of the UTF-16 string
 \param [in]     words  how many words are in the string, or, if < 0, string is NUL-terminated
 \param [out]    the number of words needed to store the string

 \returns        1 if the string began with a BOM; 0 otherwise

 \throws         InvalidCodeUnitException
*/
static size_t Utf16StringCheck(
    const Utf16Char* str,
    ssize_t words,
    size_t* neededWords)
{
    if (NULL == str)
    {
        return 0;
    }

    Utf16Char nullChar('\0');
    size_t pos;
    size_t first = 0;
    if (words != 0 && *str == 0xFEFF)
    {                                   // advance past any leading BOM character
        first++;
    }
    Utf16Char ch;

    pos = first;
    while (1)
    {
        if (words < 0) // NULL terminated string
        {
            ch = *(str + pos);
            if (nullChar == ch)
            {
                break;
            }
        }
        else // String length was passed in
        {
            if (pos < (size_t)words)
            {
                ch = *(str + pos);
            }
            else
            {
                break;
            }
        }

        if (ch >= c_CODE_POINT_SURROGATE_HIGH_MIN && ch <= c_CODE_POINT_SURROGATE_HIGH_MAX)
        {
            Utf16Char ch2 = *(str + pos + 1);
            if (ch2 < c_CODE_POINT_SURROGATE_LOW_MIN || ch2 > c_CODE_POINT_SURROGATE_LOW_MAX)
            {               // a high word without a following low one
                throw InvalidCodeUnitException(eUTF16LE, ch, pos, "");
            }
            pos++;
        }
        else if (ch >= c_CODE_POINT_SURROGATE_LOW_MIN &&
                 ch <= c_CODE_POINT_SURROGATE_LOW_MAX)
        {                   // a low word without a preceeding high one
            throw InvalidCodeUnitException(eUTF16LE, ch, pos, "");
        }

        pos++;
    }

    if (neededWords != NULL)
    {
        *neededWords = pos - first;
    }

    return first;
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
    const Utf16Char* utf16,
    size_t _size,
    size_t* firstNonAscii,
    Utf8Char* utf8)
{
    Utf16Char c;
    Utf8Char* p = utf8;
    size_t codePointWords;
    // turn off the ASCII-only code for the first pass
    size_t firstNonAsciiChar = utf8 == NULL ? 0 : *firstNonAscii;

    // do the ASCII-only conversion for the beginning characters on the second pass
    for (size_t pos = 0; pos < firstNonAsciiChar; pos++)
    {
        *p++ = (Utf8Char)*(utf16 + pos);
    }

    // If this is the first pass or firstNonAsciiChar < _size, handle the
    // rest of the string, which includes multi-byte characters.  If the
    // string was found to be pure ASCII on the first pass, this is never
    // executed on the second pass
    *firstNonAscii = _size;
    for (size_t pos = firstNonAsciiChar; pos < _size; )
    {
        c = *(utf16 + pos);
        if (c < 0x0080)
        {                               // an ASCII character
            if (utf8 != NULL)
            {
                *p = (Utf8Char)c;
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
                    *p = static_cast<Utf8Char>((c >> 6) | 0x00C0);
                    *(p + 1) = static_cast<Utf8Char>((c & 0x003F) | 0x0080);
                }
                p += 2;
                pos++;
            }
            else
            {                           // a 3-byte or 4-byte character, perhaps in the extended region
                CodePoint cp = Utf16StringToCodePoint(utf16, _size, pos, &codePointWords);
                if (cp <= 0x00010000)
                {                       // a 3-byte character
                    if (utf8 != NULL)
                    {
                        *p = (Utf8Char)(((cp >> 12) & 0x0000000F) | 0x000000E0);
                        *(p + 1) = (Utf8Char)(((cp >> 6) & 0x0000003F) | 0x00000080);
                        *(p + 2) = (Utf8Char)((cp & 0x0000003F) | 0x00000080);
                    }
                    p += 3;
                }
                else
                {                       // a 4-byte character
                    if (utf8 != NULL)
                    {
                        *p = (Utf8Char)(((cp >> 18) & 0x00000007) | 0x000000F0);
                        *(p + 1) = (Utf8Char)(((cp >> 12) & 0x0000003F) | 0x00000080);
                        *(p + 2) = (Utf8Char)(((cp >> 6) & 0x0000003F) | 0x00000080);
                        *(p + 3) = (Utf8Char)((cp & 0x0000003F) | 0x00000080);
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
 Convert a UTF-8 string to a UTF-16 string

 \param [in]     utf8  the UTF-8 string
 \param [in]     utf8Bytes  the number of bytes in *utf8 or -1 to stop on a NUL character
 \param [inout]  firstNonAscii  the position in utf8 of the first non-ASCII character
 \param [out]    utf16  the UTF-16 string; NULL for the first (evaluation) pass

 \returns        the size of the converted string

 \throws         InvalidCodeUnitException
*/
static size_t Utf8ToUtf16Conv(
    const Utf8Char* utf8,
    ssize_t utf8Bytes,
    size_t *firstNonAscii,
    Utf16Char* utf16)
{
    Utf8Char c;
    Utf16Char * p = utf16;

    // turn off the ASCII-only code for the first pass
    size_t firstNonAsciiChar = utf16 == NULL ? 0 : *firstNonAscii;

    // do the ASCII-only conversion for the beginning characters
    for (size_t i = 0; i < firstNonAsciiChar; i++)
    {
        *p++ = (Utf16Char)*(utf8 + i);
    }

    // If this is the first pass or firstNonAsciiChar < size, handle the rest of the string,
    // which includes multi-byte characters.  If the string was found to be pure ASCII on the
    // first pass, this is never executed on the second pass
    *firstNonAscii = 0x7FFFFFFF;
    size_t i;
    size_t bytes;
    for (i = firstNonAsciiChar;
         utf8Bytes < 0 ? *(utf8 + i) != (Utf16Char)0 : i < (size_t)utf8Bytes; )
    {
        c = *(utf8 + i);
        if (c < 0x80)
        {
            if (utf16 != NULL)
            {
                *p = (Utf16Char)*(utf8 + i);
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
            CodePoint cp = Utf8StringToCodePoint(utf8, utf8Bytes < 0 ? 0x7FFFFFFF : (size_t)utf8Bytes, i, &bytes);
            if (cp < 0x00010000)
            {
                if (utf16 != NULL)
                {
                    *p = (Utf16Char)cp;
                }
                p++;
            }
            else
            {
                if (utf16 != NULL)
                {
                    cp -= 0x00010000;
                    *p = (Utf16Char)(((cp >> 10) & 0x000003FF) + c_CODE_POINT_SURROGATE_HIGH_MIN);
                    *(p + 1) = (Utf16Char)((cp & 0x000003FF) + c_CODE_POINT_SURROGATE_LOW_MIN);
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
    return (size_t)(p - utf16);
}

/*----------------------------------------------------------------------------*/
/**
 Exported functions
*/

/*----------------------------------------------------------------------------*/
/**
 Get the one or two words that make up a UTF-16 code point

 \param [in]     cp  the code point
 \param [out]    word1  the high or only UTF-16 word representing the code point
 \param [out]    word2  the low UTF-16 word representing the code point

 \returns        1 if word1 is the only word, 2 if both word1 and word2 have values
*/
bool SCX::Util::CodePointToUtf16(
    CodePoint cp,
    Utf16Char* word1,
    Utf16Char* word2)
{
    if (cp < 0x00010000)
    {
        *word1 = (Utf16Char)cp;
        return false;
    }
    else
    {
        cp -= 0x00010000;
        cp &= 0x000FFFFF;
        *word1 = (Utf16Char)(((cp >> 10) & 0x000003FF) + c_CODE_POINT_SURROGATE_HIGH_MIN);
        *word2 = (Utf16Char)((cp & 0x000003FF) + c_CODE_POINT_SURROGATE_LOW_MIN);
        return true;
    }
}

/*----------------------------------------------------------------------------*/
/**
 Get a Unicode code point value from a UTF-16 string

 \param [in]     str  the base of the UTF-16 string
 \param [in]     size  how many words are in the string
 \param [in]     pos  where in the string to get the code point value
 \param [out]    bytes  how many words in the substring the code point occupies: 1 or 2

 \returns        the code point value

 \throws         InvalidCodeUnitException
*/
CodePoint SCX::Util::Utf16StringToCodePoint(
    const Utf16Char* str,
    size_t size,
    size_t pos,
    size_t* codePointWords)
{
    CodePoint cp;

    cp = (CodePoint)*(str + pos);
    *codePointWords = 1;
    if (cp >= c_CODE_POINT_SURROGATE_HIGH_MIN &&
        cp <= c_CODE_POINT_SURROGATE_HIGH_MAX)
    {
        if (pos + 1 >= size ||
            *(str + pos + 1) < c_CODE_POINT_SURROGATE_LOW_MIN ||
            *(str + pos + 1) > c_CODE_POINT_SURROGATE_LOW_MAX)
        {
            throw InvalidCodeUnitException(eUTF16LE, *(str + pos + 1), pos, "");
        }
        cp &= 0x000003FF;
        cp <<= 10;
        cp |= (CodePoint)*(str + pos + 1) & 0x000003FF;
        cp += 0x00010000;
        *codePointWords = 2;
    }
    return cp;
}

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
size_t SCX::Util::Utf16StringOffsetOfIndex(
    const Utf16Char* str,
    size_t _size,
    size_t index,
    bool allowLast)
{
    CodePoint cp;
    size_t pos = 0;
    for ( ; index != 0; index--)
    {
        cp = (CodePoint)*(str + pos);
        if (cp >= c_CODE_POINT_SURROGATE_HIGH_MIN &&
            cp <= c_CODE_POINT_SURROGATE_HIGH_MAX)
        {
            if (pos + 1 >= _size ||
                *(str + pos + 1) < c_CODE_POINT_SURROGATE_LOW_MIN ||
               *(str + pos + 1) > c_CODE_POINT_SURROGATE_LOW_MAX)
            {
                throw InvalidCodeUnitException(eUTF16LE, *(str + pos + 1), pos, "");
            }
            cp &= 0x000003FF;
            cp <<= 10;
            cp |= (CodePoint)*(str + pos + 1) & 0x000003FF;
            cp += 0x00010000;
            pos++;
        }
        pos++;
        if (pos > _size || (pos == _size && !allowLast))
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
        }
    }
    return pos;
}

/*----------------------------------------------------------------------------*/
/**
 Get the code point pointed to by a Utf16String iterator

 \param [inout]     it  the iterator; it is incremented it it was a 2-word code point

 \returns       the code point
*/
CodePoint SCX::Util::GetCodePoint(
    std::basic_string<Utf16Char>::iterator& it)
{
    size_t codePointWords;
    CodePoint cp = Utf16StringToCodePoint(&*it, 2, 0, &codePointWords);
    if (codePointWords > 1)
    {
        ++it;
    }
    return cp;
}

/*----------------------------------------------------------------------------*/
/**
 Get the number of code points in the current string

 \param [in]    str  the base of the UTF-16 string
 \param [in]    size  how many words are in the string

 \returns       the number of code points in the string

 \throws        InvalidCodeUnitException
*/
size_t SCX::Util::Utf16StringCodePointCount(
    const Utf16Char* str,
    size_t _size)
{
    CodePoint cp;
    size_t codePointCount = 0;
    for (size_t pos = 0; pos < _size; pos++)
    {
        cp = (CodePoint)*(str + pos);
        if (cp >= c_CODE_POINT_SURROGATE_HIGH_MIN &&
            cp <= c_CODE_POINT_SURROGATE_HIGH_MAX)
        {
            if (pos + 1 >= _size ||
                *(str + pos + 1) < c_CODE_POINT_SURROGATE_LOW_MIN ||
               *(str + pos + 1) > c_CODE_POINT_SURROGATE_LOW_MAX)
            {
                throw InvalidCodeUnitException(eUTF16LE, *(str + pos + 1), pos, "");
            }
            cp &= 0x000003FF;
            cp <<= 10;
            cp |= (CodePoint)*(str + pos + 1) & 0x000003FF;
            cp += 0x00010000;
            pos++;
        }
        codePointCount++;
    }
    return codePointCount;
}

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
CodePoint SCX::Util::Utf8StringToCodePoint(
    const Utf8Char* str,
    size_t size,
    size_t pos,
    size_t* bytes)
{
    CodePoint cp;

    if (pos >= size)
    {
        *bytes = 1;
        return (CodePoint)'\0';
    }
    cp = (CodePoint)*(str + pos);
    if (cp < 0x00000080)
    {
        *bytes = 1;
        return cp;
    }
    if (cp < 0x000000C2 || cp >= 0x000000F5)
    {
        throw InvalidCodeUnitException(eUTF8, cp, pos, "");
    }
    if (cp < 0x000000E0)
    {
        if (pos + 1 >= size || (*(str + pos + 1) & 0xC0) != 0x80)
        {
            throw InvalidCodeUnitException(eUTF8, cp, pos, "");
        }
        *bytes = 2;
        return (((CodePoint)cp & 0x0000003F) << 6) |
                ((CodePoint)*(str + pos + 1) & 0x0000003F);
    }
    if (cp < 0x000000F0)
    {
        if (pos + 2 >= size || (*(str + pos + 1) & 0xC0) != 0x80 || (*(str + pos + 2) & 0xC0) != 0x80)
        {
            throw InvalidCodeUnitException(eUTF8, cp, pos, "");
        }
        *bytes = 3;
        cp = (((CodePoint)cp & 0x0000000F) << 12) |
             (((CodePoint)*(str + pos + 1) & 0x0000003F) << 6) |
             ((CodePoint)*(str + pos + 2) & 0x0000003F);
        if (cp < 0x00000800 ||          // illegal duplicate of shorter sequence or in surrogate region
            (cp >= c_CODE_POINT_SURROGATE_HIGH_MIN && cp <= c_CODE_POINT_SURROGATE_LOW_MAX))
        {
            throw InvalidCodeUnitException(eUTF8, cp, pos, "");
        }
        return cp;
    }
    if (pos + 3 >= size ||
        (*(str + pos + 1) & 0xC0) != 0x80 ||
        (*(str + pos + 2) & 0xC0) != 0x80 ||
        (*(str + pos + 3) & 0xC0) != 0x80)
    {
        throw InvalidCodeUnitException(eUTF8, cp, pos, "");
    }
    *bytes = 4;
    cp = (((CodePoint)cp & 0x00000007) << 18) |
         (((CodePoint)*(str + pos + 1) & 0x0000003F) << 12) |
         (((CodePoint)*(str + pos + 2) & 0x0000003F) << 6) |
          ((CodePoint)*(str + pos + 3) & 0x0000003F);
    if (cp < 0x00010000 || cp > c_CODE_POINT_MAXIMUM_VALUE) // illegal duplicate of shorter sequence or above Unicode range
    {
        throw InvalidCodeUnitException(eUTF8, cp, pos, "");
    }
    return cp;
}

/*----------------------------------------------------------------------------*/
/**
 Get the Utf-8 bytes that represent a Unicode code point

 \param [in]     cp  the code point
 \param [in]     str  ptr. to an array of Utf8Char at least 4 bytes in size

 \returns        the number of bytes used in *str to represent the code point
*/
size_t SCX::Util::CodePointToUtf8(
    CodePoint cp,
    Utf8Char* str)
{
    size_t bytes;

    if (cp < 0x80)
    {
        *str = (Utf8Char)cp;
        bytes = 1;
    }
    else if (cp < 0x0800)
    {                           // a 2-byte character
        *str = static_cast<Utf8Char>((cp >> 6) | 0x00C0);
        *(str + 1) = static_cast<Utf8Char>((cp & 0x003F) | 0x0080);
        bytes = 2;
    }
    else if (cp < 0x00010000)
    {                       // a 3-byte character
        *str = (Utf8Char)(((cp >> 12) & 0x0000000F) | 0x000000E0);
        *(str + 1) = (Utf8Char)(((cp >> 6) & 0x0000003F) | 0x00000080);
        *(str + 2) = (Utf8Char)((cp & 0x0000003F) | 0x00000080);
        bytes = 3;
    }
    else
    {
        *str = (Utf8Char)(((cp >> 18) & 0x00000007) | 0x000000F0);
        *(str + 1) = (Utf8Char)(((cp >> 12) & 0x0000003F) | 0x00000080);
        *(str + 2) = (Utf8Char)(((cp >> 6) & 0x0000003F) | 0x00000080);
        *(str + 3) = (Utf8Char)((cp & 0x0000003F) | 0x00000080);
        bytes = 4;
    }
    return bytes;
}

/*----------------------------------------------------------------------------*/
/**
 Utf16String class member functions
*/

void Utf16String::Assign(
    const Utf16Char* str)
{
    // find the length of the input string and check for errors
    size_t neededWords;
    size_t first = Utf16StringCheck(str, -1, &neededWords);

    if (neededWords == 0)
    {
        clear();
    }
    else
    {
        // copy the input string into the object
        (void)assign(str + first, neededWords);
    }

    return;
}

void Utf16String::Assign(
    const Utf16Char* str,
    size_t _size)
{
    // check for errors
    size_t neededWords;
    size_t first = Utf16StringCheck(str, _size, &neededWords);

    if (neededWords == 0)
    {
        clear();
    }
    else
    {
        // copy the input string into the object
        (void)assign(str + first, str + _size);
    }

    return;
}

void Utf16String::Assign(
    const std::basic_string<Utf16Char>& str)
{
    // check for errors
    size_t neededWords;
    size_t first = Utf16StringCheck(&str[0], str.size(), &neededWords);

    // copy the input string into the object
    if (neededWords == 0)
    {
        clear();
    }
    else
    {
        (void)assign(&str[first], &str[str.size()]);
    }

    return;
}

void Utf16String::Assign(
    const std::basic_string<unsigned short>::iterator _begin,
    const std::basic_string<unsigned short>::iterator _end)
{
    if (_begin == _end || _begin == _end - 1)
    {
        clear();
    }
    else
    {
        // check for errors
        size_t _size = (size_t)(&*_end - &*_begin);
        size_t first = Utf16StringCheck(&*_begin, _size, NULL);
        if (_size == 0)
        {
            clear();
        }
        else
        {
            // copy the input string into the object
            (void)assign(&*_begin + first, _size);
        }
    }
    return;
}

void Utf16String::Assign(
    const std::vector<unsigned char>& v)
{
    // check the size
    if ((v.size() & 1) != 0)
    {                                   // odd size is bad
       throw InvalidCodeUnitException(eUTF16LE, '\0', v.size(), "odd no. bytes");
    }
    if (v.size() == 0)
    {
        clear();
    }
    else
    {
        if (CPU_IS_BIG_ENDIAN())
        {
            // copy the input string into the object
            std::vector<unsigned char> v2;
            v2.assign(v.size(), 0);
            swab((const char*)&v[0], (char*)&v2[0], v.size());

            size_t neededWords;
            size_t first = Utf16StringCheck((const Utf16Char*)&v2[0], v2.size() / sizeof (Utf16Char), &neededWords);
            if (neededWords == 0)
            {
                clear();
            }
            else
            {
                (void)assign((const Utf16Char*)&v2[first * sizeof (Utf16Char)], neededWords);
            }

            v2.clear();
        }
        else
        {
            // check for errors
            size_t neededWords;
            size_t first = Utf16StringCheck((const Utf16Char*)&v[0], v.size() / 2, &neededWords);

            if (neededWords == 0)
            {
                clear();
            }
            else
            {
                (void)assign((const Utf16Char*)&v[first * sizeof (Utf16Char)], neededWords);
            }
        }
    }
    return;
}

void Utf16String::Assign(
    const Utf8Char* str,
    size_t _size)
{
    size_t firstNonAscii;
    if (_size >= 3 && *str == 0xEF && *(str + 1) == 0xBB && *(str + 2) == 0xBF)
    {                                   // omit byte order mark
        str += 3;
        _size -= 3;
    }
    size_t utf16Chars = Utf8ToUtf16Conv(str, _size, &firstNonAscii, NULL);
    if (utf16Chars == 0)
    {
        clear();
    }
    else
    {
        (void)assign(utf16Chars, 0);
        (void)Utf8ToUtf16Conv(str, _size, &firstNonAscii, &(*this)[0]);
    }

    return;
}

void Utf16String::Assign(
    const Utf8Char* str)
{
    size_t firstNonAscii;
    if (*str == 0xEF && *(str + 1) == 0xBB && *(str + 2) == 0xBF)
    {                           // omit byte order mark
        str += 3;
    }
    size_t utf16Chars = Utf8ToUtf16Conv(str, -1, &firstNonAscii, NULL);

    if (utf16Chars == 0)
    {
        clear();
    }
    else
    {
        (void)assign(utf16Chars, 0);
        (void)Utf8ToUtf16Conv(str, -1, &firstNonAscii, &(*this)[0]);
    }

    return;
}

void Utf16String::Assign(
    const std::string& str)
{
    size_t first = 0;
    if (str.size() >= 3 && (unsigned char)str[0] == 0xEF && (unsigned char)str[1] == 0xBB && (unsigned char)str[2] == 0xBF)
    {                       // omit byte order mark
        first = 3;
    }
    size_t firstNonAscii;
    size_t utf16Chars = Utf8ToUtf16Conv((const Utf8Char*)&str[first], str.size(), &firstNonAscii, NULL);
    if (utf16Chars == 0)
    {
        clear();
    }
    else
    {
        (void)assign(utf16Chars, 0);
        (void)Utf8ToUtf16Conv((const Utf8Char*)&str[first], str.size(), &firstNonAscii, &(*this)[0]);
    }

    return;
}

void Utf16String::Write(
    std::vector<unsigned char>& v,
    bool addBOM) const
{
    size_t start = 0;
    if (addBOM)
    {
        start = sizeof (Utf16Char);
        v.assign((size() + 1) * sizeof (Utf16Char), 0);
        v[0] = 0xFF;     // 16-bit forced space char. used as byte-order indicator
        v[1] = 0xFE;
    }
    else
    {
        v.assign(size() * sizeof (Utf16Char), 0);
    }
    if (CPU_IS_BIG_ENDIAN())
    {
        swab((const char*)&(*this)[0], (char*)&v[start], size() * sizeof (Utf16Char));
    }
    else
    {
        memcpy(&v[start], (const void *)&(*this)[0], size() * sizeof (Utf16Char));
    }

    return;
}

size_t Utf16String::Find(
    CodePoint cp,
    size_t pos) const
{
    if (pos > size())
    {
        throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
    }

    if (cp < 0x00010000)
    {
        return find((unsigned short)cp, pos);
    }

    Utf16Char word1, word2;
    (void)CodePointToUtf16(cp, &word1, &word2);
    unsigned short seq[4];
    seq[0] = word1;
    seq[1] = word2;
    seq[2] = (Utf16Char)'\0';
    Utf16String s(seq);

    return find(s, pos);
}

size_t Utf16String::Find(
    Utf16String& str,
    size_t pos)
{
    if (pos > size())
    {
        throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, SCXSRCLOCATION);
    }

    if (size() == 0 || str.size() == 0)
    {                       // this Find is unlike find() in that empty strings are not found
        return std::string::npos;
    }

    if (pos + str.size() > size())
    {
        return std::string::npos;
    }

    return find(str, pos);
}

void Utf16String::SetCodePointAtIndex(
    size_t index,
    CodePoint cp)
{
    size_t pos = Utf16StringOffsetOfIndex(&(*this)[0], size(), index, true);
    if (pos == size())
    {
        Append(cp);
    }
    else
    {
        CodePoint oldCp = GetCodePoint(pos);
        Utf16Char word1, word2;
        bool twoWords = CodePointToUtf16(cp, &word1, &word2);
        if (twoWords)
        {                   // replace current code point with double-word code point
            if (oldCp < 0x00010000)
            {               // a double-word code point replacing a single-word code point
                (*this).insert(pos + 1, 1, word2);
            }
            (*this)[pos] = word1;
            (*this)[pos + 1] = word2;
        }
        else
        {                   // insert a single-word code point
            if (oldCp >= 0x00010000)
            {               // a double-word code point replacing a single-word code point
                erase(pos + 1, 1);
            }
            (*this)[pos] = word1;
        }
    }

    return;
}

/*----------------------------------------------------------------------------*/
/**
 Utf8String class member functions
*/

void Utf8String::Assign(
    const std::vector<unsigned char>& v)
{
    size_t first = 0;
    if (v.size() >= 3 && v[0] == 0xEF && v[1] == 0xBB && v[2] == 0xBF)
    {                       // omit byte order mark
        first = 3;
    }
    size_t firstNonAscii;
    size_t utf16Chars = Utf8ToUtf16Conv(&v[first], (ssize_t)(v.size() - first), &firstNonAscii, NULL);
    if (utf16Chars == 0)
    {
        clear();
    }
    else
    {
        (void)assign(utf16Chars, 0);
        (void)Utf8ToUtf16Conv(&v[first], ((ssize_t)v.size() - first), &firstNonAscii, &(*this)[0]);
    }
    return;
}

void Utf8String::Assign(
     const std::basic_string<Utf16Char>::iterator _begin,
     const std::basic_string<Utf16Char>::iterator _end)
{
     Utf16String::Assign(_begin, _end);
     return;
}

std::string Utf8String::Str() const
{
    size_t firstNonAsciiPos;
    size_t utf8Bytes = Utf16ToUtf8Conv(&(*this)[0], size(), &firstNonAsciiPos, NULL);
    if (utf8Bytes == 0)
    {
        std::string str;
        return str;
    }
    else
    {
        std::string str(utf8Bytes, 0);
        (void)Utf16ToUtf8Conv(&(*this)[0], size(), &firstNonAsciiPos, (unsigned char*)&str[0]);

        return str;
    }
}

void Utf8String::Write(
    std::vector<unsigned char>& v,
    bool addBOM) const
{
    size_t start = 0;
    size_t firstNonAsciiPos;
    size_t utf8Bytes = Utf16ToUtf8Conv(&(*this)[0], size(), &firstNonAsciiPos, NULL);
    if (addBOM)
    {
        start = 3;
        v.assign(3 + utf8Bytes, 0);
        v[0] = 0xEF;
        v[1] = 0xBB;
        v[2] = 0xBF;
    }
    else
    {
        v.assign(utf8Bytes, 0);
    }
    (void)Utf16ToUtf8Conv(&(*this)[0], size(), &firstNonAsciiPos, &v[start]);

    return;
}

void Utf8String::Write(
    std::ostream& stream,
    bool /*not used*/) const
{
    std::vector<unsigned char> v;
    Write(v);
    stream.write((const char*)&v[0], v.size());
    return;
}

void Utf16String::Trim()
{
    bool trimmedStart = false;
    bool trimmedEnd = false;

    size_t startOffset = 0;
    for (size_t i = 0; i < size(); i++)
    {
        if (at(i) < 0x0009 || (at(i) > 0x000D && at(i) != ' '))
        {
            startOffset = i;
            trimmedStart = i != 0;
            break;
        }
    }
    
    // Erase till the start
    if (trimmedStart)
    {
        (void)erase(0, startOffset);
    }

    // Find the endoffset
    size_t endOffset = size();
    for (size_t i = size(); i > 0; --i)
    {
        if (at(i-1) < 0x0009 || (at(i-1) > 0x000D && at(i-1) != ' '))
        {
            endOffset = i;
            trimmedEnd = i != size();
            break;
        }
    }

    if (trimmedEnd)
    {
        // Erase from the endOffset
        (void)erase(endOffset, size() - endOffset);
    }
    return;
}
