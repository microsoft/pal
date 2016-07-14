//******************************************************************************
//
//  Copyright (c) Microsoft.  All rights reserved.
//
//  File: XMLReader.cpp
//
//  Implementation of the direct-from-string XML document parser
//
//  Declaration  : XMLReader.h
//
//******************************************************************************
#include <util/Unicode.h>
#include <util/XMLReader.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <scxcorelib/stringaid.h>

using namespace SCX::Util;
using namespace SCX::Util::Xml;

/*
**==============================================================================
**
** Local definitions
**
**==============================================================================
*/
static const Utf8Char u8_LessThan    = '<';
static const Utf8Char u8_GreaterThan = '>';
static const Utf8Char u8_EqualTo     = '=';
static const Utf8Char u8_Ampersand   = '&';
static const Utf8Char u8_PoundSign   = '#';
static const Utf8Char u8_Colon       = ':';
static const Utf8Char u8_SemiColon   = ';';
static const Utf8Char u8_NewLine     = '\n';
static const Utf8Char u8_Apos        = '\'';
static const Utf8Char u8_Quote       = '"';
static const Utf8Char u8_Question    = '?';
static const Utf8Char u8_Slash       = '/';
static const Utf8Char u8_Bang        = '!';
static const Utf8Char u8_Dash        = '-';
static const Utf8Char u8_NullChar    = '\0';

static Utf8String c_CDATA("[CDATA[");
static Utf8String c_DOCTYPE("DOCTYPE");

static const Utf8String open_bracket("<");
static const Utf8String open_with_slash("</");
static const Utf8String close_bracket_with_term("/>");
static const Utf8String close_bracket_with_question("?>");
static const Utf8String close_bracket(">");
static const Utf8String u8_space(" ");
static const Utf8String equals_with_quote("=\"");
static const Utf8String ending_quote("\"");
static const Utf8String crlf("\r\n");
static const Utf8String empty_string("");
static const Utf8Char single_question('?');
static const Utf8Char u8_semicolon(';');
static const Utf8Char u8_a('a');
static const Utf8Char u8_b('b');
static const Utf8Char u8_c('c');
static const Utf8Char u8_d('d');
static const Utf8Char u8_e('e');
static const Utf8Char u8_f('f');
static const Utf8Char u8_g('g');
static const Utf8Char u8_h('h');
static const Utf8Char u8_i('i');
static const Utf8Char u8_j('j');
static const Utf8Char u8_k('k');
static const Utf8Char u8_l('l');
static const Utf8Char u8_m('m');
static const Utf8Char u8_n('n');
static const Utf8Char u8_o('o');
static const Utf8Char u8_p('p');
static const Utf8Char u8_q('q');
static const Utf8Char u8_r('r');
static const Utf8Char u8_s('s');
static const Utf8Char u8_t('t');
static const Utf8Char u8_u('u');
static const Utf8Char u8_v('v');
static const Utf8Char u8_w('w');
static const Utf8Char u8_x('x');
static const Utf8Char u8_y('y');
static const Utf8Char u8_z('z');
static const Utf8Char u8_quote('\"');
static const Utf8Char u8_slash('/');

/*
**==============================================================================
****************************** Private Methods *********************************
**==============================================================================
*/
/* Space characters include [\n\t\r ]
 *     _spaceChar['\n'] => 1
 *     _spaceChar['\r'] => 2
 *     _spaceChar['\t'] => 2
 *     _spaceChar[' '] => 2
 *
 * Note that ISO 8859-1 character 0xA0, No Break Space, is not a
 * space character here.
 */
static unsigned char _spaceChar[256] =
{
    0,0,0,0,0,0,0,0,0,2,1,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


/*
**==============================================================================
**
** return non-zero if character is an ASCII whitespace character
**
**==============================================================================
*/
unsigned int XMLReader::_IsSpace(CodePoint uc)
{
    // If this is a multi-byte character then ignore as there no multi-byte
    // spaces
    if (uc > 0x00000100)
    {
        return 0;
    }
    else
    {
        return _spaceChar[uc];
    }
}


/* Matches XML name characters of the form: [A-Za-z_][A-Za-z0-9_-.:]*
 *     _nameChar[A-Za-z_] => 2 (first character)
 *     _nameChar[A-Za-z0-9_-.:] => 1 or 2 (inner character)
 
static unsigned char _nameChar[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,2,
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
*/

/*
**==============================================================================
**
** Return non-zero if the character is a valid XML starting character
**
**==============================================================================
*/
bool XMLReader::_IsFirst(CodePoint cp)
{
    // ref: http://www.w3.org/TR/REC-xml/#NT-Name
    // NameStartChar ::= ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] |
    //                      [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]

    if ((cp == 0x3A) || (cp == 0x5F) || ((cp >= 0x41) && (cp <= 0x5A)) || ((cp >= 0x61) && (cp <= 0x7A)) || ((cp >= 0xC0) && (cp <= 0xD6)) || ((cp >= 0xD8) && (cp <= 0xF6)) ||
        ((cp >= 0xF8) && (cp <= 0x2FF)) || ((cp >= 0x370) && (cp <= 0x37D)) || ((cp >= 0x37F) && (cp <= 0x1FFF)) || ((cp >= 0x200C) && (cp <= 0x200D)) ||
         ((cp >= 0x2070) && (cp <= 0x218F)) || ((cp >= 0x2C00) && (cp <= 0x2FEF)) || ((cp >= 0x3001) && (cp <= 0xD7FF)) || ((cp >= 0xF900) && (cp <= 0xFDCF)) ||
          ((cp >= 0xFDF0) && (cp <= 0xFFFD)) || ((cp >= 0x10000) && (cp <= 0xEFFFF)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
**==============================================================================
**
** Return non-zero if the character is a valid XML inner string character
**
**==============================================================================
*/
bool XMLReader::_IsInner(CodePoint cp)
{
    // ref: http://www.w3.org/TR/REC-xml/#NT-Name
    // NameChar   ::=   NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]

    if (_IsFirst(cp) || (cp == 0x2D) || (cp == 0x2E) || ((cp >= 0x30) && (cp <= 0x39)) || (cp == 0xB7) ||
         ((cp >= 0x0300) && (cp <= 0x036F)) || ((cp >= 0x203F) && (cp <= 0x2040)))
    {
        return true;
    }
    else
    {
        return false;
    }

}


/*
**==============================================================================
**
** Skip past the inner characters to the next control character
**
**==============================================================================
*/
void XMLReader::_SkipInner(Utf8String& /* unused */)
{
    while (_IsInner(GetCodePoint(m_CharStartPos)))
    {
        ++m_CharStartPos;
        ++m_CharPos;
    }

    return;
}


/*
**==============================================================================
**
** Skip past the spaces in front of the control character
**
**==============================================================================
*/
void XMLReader::_SkipSpacesAux(Utf8String& /* unused */)
{
    unsigned int x;
    size_t n = 0;

    while ((x = _IsSpace(GetCodePoint(m_CharStartPos))) != 0)
    {
        n += 0x01 & (size_t)x;
        ++m_CharStartPos;
        ++m_CharPos;
    }

    m_Line += n;

    return;
}


/*
**==============================================================================
**
** Skip past all leading whitespace characters
**
**==============================================================================
*/
void XMLReader::_SkipSpaces(Utf8String& /*unused*/)
{
    int n = 0;
    while (_IsSpace(*m_CharStartPos))
    { 
        if (*m_CharStartPos == u8_NewLine)
            m_Line++; 
        m_CharStartPos++;
        ++m_CharPos;
        n++; 
    } 
    return;
}


/*
**==============================================================================
**
** Change a string like &quot to its single character value (")
**
**==============================================================================
*/
void XMLReader::_ToEntityRef(Utf8String& /*unused*/, char& ch)
{
    /* Note: we collected the following statistics on the frequency of
     * each entity reference in a large body of XML documents:
     *     
     *     &quot; - 74,480 occurences
     *     &apos; - 13,877 occurences
     *     &lt;   -  9,919 occurences
     *     &gt;   -  9,853 occurences
     *     &amp;  -    111 occurences
     *
     * The cases below are organized in order of statistical frequency.
     */

    /* Match one of these: "lt;", "gt;", "amp;", "quot;", "apos;" */
    Utf8Char firstChar  = static_cast<Utf8Char>(*m_CharStartPos++);
    Utf8Char secondChar = static_cast<Utf8Char>(*m_CharStartPos++);
    Utf8Char thirdChar  = static_cast<Utf8Char>(*m_CharStartPos++);
    m_CharPos += 3;
    if (firstChar == u8_l && secondChar == u8_t && thirdChar == u8_SemiColon)
    {
        ch = u8_LessThan;
        return;
    }

    if (firstChar == u8_g && secondChar == u8_t && thirdChar == u8_SemiColon)
    {
        ch = u8_GreaterThan;
        return;
    }

    Utf8Char fourthChar = static_cast<Utf8Char>(*m_CharStartPos++);
    m_CharPos++;
    if (firstChar == u8_a && secondChar == u8_m && thirdChar == u8_p && fourthChar == u8_SemiColon)
    {
        ch = u8_Ampersand;
        return;
    }

    Utf8Char fifthChar = static_cast<Utf8Char>(*m_CharStartPos++);
    m_CharPos++;
    if (firstChar == u8_q && secondChar == u8_u && thirdChar == u8_o && 
        fourthChar == u8_t && fifthChar == u8_SemiColon)
    {
        ch = u8_Quote;
        return;
    }

    if (firstChar == u8_a && secondChar == u8_p && thirdChar == u8_o && 
        fourthChar == u8_s && fifthChar == u8_SemiColon)
    {
        ch = u8_Apos;
        return;
    }
    XML_Raise("bad entity reference");
}


/*
**==============================================================================
**
** Change a character reference to its single character value
**
**==============================================================================
*/
void XMLReader::_ToCharRef(Utf8String& /*unused*/, char& ch)
{
    bool isHex = false;
    bool foundSemiColon = false;

    // Temporary string to hold the number
    std::string tempString;

    // The first character is the pound sign, so the index starts with 1
    size_t i = 1;
    m_CharStartPos++;
    m_CharPos++;
    if (*m_CharStartPos == u8_x)
    {
        // This is hex
        isHex = true;
        // Move one more for x
        m_CharStartPos++;
        m_CharPos++;
 
        i = 2;
    }
    
    // Move the pointer to the next 6 (8-2) characters looking for the end ';'
    for (; i < 8 ; ++i)
    {
        if (*m_CharStartPos == u8_SemiColon)
        {
            foundSemiColon = true;
            break;
        }
        else
        {
            CodePoint cp = (CodePoint)*m_CharStartPos;
            if (cp <= 255)
            {
                tempString += (char) cp;
            }
            else
            {
                // None of the character entities can be more than ASCII
                XML_Raise("bad character reference");
            }
        }

        m_CharStartPos++;
         m_CharPos++;
    }

    tempString += '\0';

    if (!foundSemiColon)
    {
        XML_Raise("bad character reference");
    }

    // Now conver the reference to an int
    unsigned int ref = 0;
    std::stringstream ss;

    if (isHex)
    {
        // Set the format
        ss << std::hex;
    }

    ss << tempString;

    // Convert the string to int
    ss >> ref;

    // Now check if the convertion failed or the ref value is greater than 255
    // Character entities cannot be greater than 255
    if ((!ss) || (ref > 255))
    {
        XML_Raise("bad character reference");
    }

    // Now set the character
    ch = (char) ref;

    // Now lets delete the characters from the original string 
    // the size is i+1 (Inclusive of the ;)
    ++m_CharStartPos;
    ++m_CharPos;
}


/*
**==============================================================================
**
** Change a reference (starts with & or #) to a single character value
**
**==============================================================================
*/
void XMLReader::_ToRef(Utf8String& p, char& ch)
{
    /* Examples:
     *     &#64;
     *     &xFF;
     *     &amp;
     *     &lt;
     */
    if (*m_CharStartPos == u8_PoundSign)
    {
        _ToCharRef(p, ch);
    }
    else
    {
        _ToEntityRef(p, ch);
    }
}


/*
**==============================================================================
**
** Reduce entity references and remove leading and trailing whitespace 
**
**==============================================================================
*/
Utf8String XMLReader::_ReduceAttrValue(/*Utf8String& pInOut,*/ Utf8String::Char eos)
{
    /* Matches all but '\0', '\'', '"', and '&'. All matching charcters
     * yeild 2, except for '\n', which yields 1 
     */
#if 0
    static unsigned char _match[256] =
    {
        0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    };
#endif
    // std::string p(pInOut);
    Utf8String end;

    end.Clear();

    // Skip uninteresting characters
/*
    for (;;)
    {
        CodePoint codePoint = m_CharStartPos->GetCodePoint();
        while ((codePoint >= 0x7f) || (_match[codePoint] != 0))
        {
            m_CharStartPos++;
            m_CharPos++;
            codePoint = m_CharStartPos->GetCodePoint();
        }

        if (*m_CharStartPos != u8_NewLine)
        {
            break;
        }

        end += *m_CharStartPos;

        ++m_CharStartPos;
        ++m_CharPos;
        m_Line++;
    }
*/

    // Now that we know where the string end is, we have to run through the string
    // a character at a timne and do string expansion/contraction.
    int n = 0;
    while (m_CharPos <= m_InternalString.Size() && *m_CharStartPos != u8_quote) 
    {
        if (m_CharPos == m_InternalString.Size())
        {
            // End of input
            break;
        }

        if (*m_CharStartPos == eos)
        {
            // Reached the closing quote
            break;
        }

        // If this was a reference, do the replace
        if (*m_CharStartPos == u8_Ampersand)
        {
            char c = u8_NullChar;
            
            m_CharStartPos++;
            m_CharPos++;

            _ToRef(m_InternalString, c);
 
            if (m_Status != 0)
            {
                // Propagate error  by returning NULL
                return empty_string;
            }

            // Put the converted character into the output string
            end.Append(std::string(&c, 1));
        }
        else
        {
            // Count cones
            if (*m_CharStartPos == u8_NewLine)
                n++;

            // No conversion -- add to the output
            end.Append(*m_CharStartPos);

            m_CharStartPos++;
            m_CharPos++;
        }
    }

    m_Line += n;
    return end;
}


/*
**==============================================================================
**
** Reduce character data, advance pointer, and return end
**
**==============================================================================
*/
Utf8String XMLReader::_ReduceCharData( void )
{
    /* Match all but these: '\0', '<', '&', '\n' */
    static unsigned char _match[256] =
    {
        0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    };
    size_t n = 0;
    size_t index = 0;
    Utf8String end("");

    if (m_InternalString.Size()  < (m_CharPos + 1) || *m_CharStartPos == u8_NullChar)
    {
        return  empty_string;
    }

    size_t oldStart = m_CharPos;
    for (;;)
    {
        // Find the end of the string
        CodePoint cp = (CodePoint)*m_CharStartPos;
        while (1)
        {
            if (cp < 256)
            {
                if (!_match[cp])
                {
                    break;
                }
            }

            // Value was either > 256 (Multibyte) or ASCII and not an XML control character
            m_CharStartPos++;
            m_CharPos++;
            cp = (CodePoint)*m_CharStartPos;
        }

        // If it wasn't a new Line, break
        if (*m_CharStartPos != u8_NewLine)
            break;
        
        // Skip over the new line
        m_Line++;

                m_CharStartPos++;
        m_CharPos++;
        cp = (CodePoint)*m_CharStartPos;
    }

    // If index was set, clear the "end" string from the point of the existing 
    // string.  The result will be the first set of characters in the string, which
    // is the value we want.  If the next character is not a <, however, we have to do some
    // tag wrangling to get the output string
    if (m_CharPos != oldStart)
    {
        // If we removed some leading garbage, set the pointer right
        end = m_InternalString.SubStr(oldStart, m_CharPos - oldStart);
    }

    // Can we return now? 
    if (*m_CharStartPos == u8_LessThan)
    {
        m_Line += n;
        return  end;
    }
    
    // Well, looks like we got a string with embedded XML
    // We have to seek the next start tag, building the text from the current location.
    index = 0;

    // Seek next tag start
    while (m_CharPos < m_InternalString.Size() && *m_CharStartPos != u8_LessThan)
    {
        // While we're looking for the next start tag, we'll keep checking the internal string
        // to see if we need to translate any references
        if (*m_CharStartPos == u8_Ampersand)
        {
            char c = u8_NullChar;
            Utf8String tmp;
            m_CharStartPos++;
            m_CharPos++;

            _ToRef(m_InternalString, c);

            end += std::string(&c, 1);
        }
        else
        {
            size_t oldStart2 = m_CharPos;
            for (;;)
            {
                CodePoint cp = (CodePoint)*m_CharStartPos;
                while (1)
                {
                    if (cp < 256)
                    {
                        if (!_match[cp])
                        {
                            break;
                        }
                    }

                    index++;
                    ++m_CharStartPos;
                    ++m_CharPos;
                    cp = (CodePoint)*m_CharStartPos;
                }
    
                if (*m_CharStartPos != u8_NewLine)
                {
                    break;
                }

                m_CharStartPos++;
                m_CharPos++;
                m_Line++;
            }
            end.Append(m_InternalString.SubStr(oldStart2, m_CharPos - oldStart2));
        }
    }

    // Document cannot end with character data
    if (m_CharPos > m_InternalString.Size() || *m_CharStartPos == u8_NullChar)
    {
        return empty_string;
    }

    m_Line += n;

    return end;
}


/*
**==============================================================================
**
** Calculate a fast hash code for a non-zero-length strings
**
**==============================================================================
*/
unsigned int XMLReader::_HashCode(Utf8String& s, size_t n)
{
    /* This hash algorithm excludes the first character since for many strings 
     * (e.g., URIs) the first character is not unique. Instead the hash 
     * comprises two components:
     *     (1) The length
     *     (2) The last chacter
     */
    return (int)(n ^ (size_t)s[n - 1]);
}

/*
**==============================================================================
**
** Translate the m_NameSpace name used in the document to a single-character
** m_NameSpace name specified by the client in the XML_RegisterNameSpace() call. 
** For example: "wsman:OptimizeEnumeration" => "w:OptimizeEnumeration".
**
**==============================================================================
*/
Utf8String XMLReader::_TranslateName(Utf8String& name, size_t colonLoc)
{
    // Remove colon Prefix, if exists
    if ( m_stripNamespaces && colonLoc != std::string::npos)
    {
        Utf8String returnString = name.SubStr(colonLoc+1);
        return returnString;
    }
    else
    {
        return name;
    }

    //
    // TODO: Why is this code here???
    //

    // Properly support namespaces - Task 390350
    unsigned int code;
    size_t i;

    // Temporarily zero-out the ':' character 
    name[colonLoc] = u8_NullChar;

    // Calculate hash code
    code = _HashCode(name, colonLoc);

    // First check single entry cache
    if (!m_NameSpaces.empty())
    {
        pXML_NameSpace ns = m_NameSpaces.back();

        if (ns->nameCode == code && ns->name == name)
        {
            if (ns->id)
            {
                name[colonLoc - 1] = ns->id;
                name[colonLoc] = u8_Colon;
                return name.Erase(colonLoc);
            }
            else
            {
                name[colonLoc] = u8_Colon;
                return name;
            }
        }
    }

    // Translate name to the one found in the m_NameSpaces[] array
    for (i = m_NameSpaces.size(); i--; )
    {
        pXML_NameSpace ns = m_NameSpaces[i];

        if (ns->nameCode == code && ns->name == name)
        {
            // Cache 
            m_NameSpacesCacheIndex = i;

            if (ns->id)
            {
                name[colonLoc - 1] = ns->id;
                name[colonLoc] = u8_Colon;
                return name.Erase(colonLoc);
            }
            else
            {
                name[colonLoc] = u8_Colon;
                return name;
            }
        }
    }

    // Restore the ':' character
    name[colonLoc] = u8_Colon;
    return name;
}


/*
**==============================================================================
**
** Map a URI to a single character namespace identifier
**
**==============================================================================
*/
char XMLReader::_FindNameSpaceID(Utf8String uri, size_t uriSize)
{
    size_t i;
    unsigned int code = _HashCode(uri, uriSize);

    // Resolve from client namespace registrations
    for (i = 0; i < m_RegisteredNameSpacesSize; i++)
    {
        pXML_RegisteredNameSpace rns = m_RegisteredNameSpaces[i];

        if (rns->uriCode == code && rns->uri == uri)
            return rns->id;
    }

    // Not found so return null id
    return u8_NullChar;
}


/*
**==============================================================================
**
** Parse an attribute name/value pair
**
**==============================================================================
*/
void XMLReader::_ParseAttr(pCXElement& elem)
{
    Utf8String name;
    Utf8String value;
    Utf8String valueEnd;
    size_t colonLoc = 0;

    // Parse the attribute name
    size_t startPos = m_CharPos;

    if (!_IsFirst(*m_CharStartPos))
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected attribute name");
        return;
    }

    // Advance the pointer
    m_CharStartPos++;
    m_CharPos++;
    _SkipInner(m_InternalString);

    if (*m_CharStartPos == u8_Colon)
    {
        m_CharStartPos++;
        m_CharPos++;
        _SkipInner(m_InternalString);
    }

    // Set the name.  This is the leftmost part of the string, saved before we started
    // getting rid of stuff
    size_t distance = m_CharPos - startPos;
    name = m_InternalString.SubStr(startPos, distance);

    // If this was a xmlns name, note it for later.
    colonLoc = name.Find(u8_Colon);

    // Seek the quote character (position p beyond quote)
    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Expect a '=' character
    if (*m_CharStartPos != u8_EqualTo)
    {
        XML_Raise("expected = character");
        return;
    }
    m_CharStartPos++;
    m_CharPos++;

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Parse the value
    
    // Expect opening quote
    if (*m_CharStartPos != u8_Quote && *m_CharStartPos != u8_Apos)
    {
        XML_Raise("expected opening quote");
        return;
    }

    Utf8String::Char quote = *m_CharStartPos;
    m_CharStartPos++;
    m_CharPos++;

    // Get the value of the attribute
    value = _ReduceAttrValue(quote);

    if (m_Status != 0)
    {
        // Propagate error
        value.Clear();
        return;
    }

    // Expect closing quote
    if (*m_CharStartPos != quote)
    {
        XML_Raise("expected closing quote");
        value.Clear();
        return;
    }

    // Skip past the closing quote
    m_CharStartPos++;
    m_CharPos++;

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // If attribute name is "xmlns", extract m_NameSpace
    Utf8String nsName = name.SubStr(0, 5);
    if (nsName.Compare("xmlns"))
    {
        // ATTN: implement default m_NameSpaces
        if (name[5] != u8_Colon)
        {
          //XML_Raise("default m_NameSpaces not supported: xmlns");
            return;
        }

        // Add new m_NameSpace entry
        {
            // Check form_Stack overflow
            if (m_NameSpacesSize == XML_MAX_NAMESPACES)
            {
                XML_Raise("too many m_NameSpaces (>%u)",
                    (int)XML_MAX_NAMESPACES);
                return;
            }
            {
                pXML_NameSpace ns(new XML_NameSpace());
                ns->name = name;
                ns->name = ns->name.Erase(0, 6);
                ns->nameCode = _HashCode(ns->name, ns->name.Size());
                ns->id = _FindNameSpaceID(value, value.Size());
                ns->uri = value;
                ns->depth =m_StackSize;
                m_NameSpaces.push_back(ns);
            }
        }
    }
    else
    {
        // Translate the name (possibly replacing m_NameSpace with single char)
        if (colonLoc != std::string::npos)
            name = _TranslateName(name, colonLoc);
    }

    // Append attribute to Element
    {
        elem->AddAttribute(name, value);
    }
    value.Clear();
}


/*
**==============================================================================
**
** Parse processing instructions
**
**==============================================================================
*/
void XMLReader::_ParseProcessingInstruction(pCXElement& elem)
{
    // NOTE:  We parse this tag, but we do not store it.  All we're doing is
    // moving past the tag.

    // <?xml version="1.0" encoding="UTF-8" standalone="yes"?> 

    // Advance past '?' character
    m_CharStartPos++;
    m_CharPos++;

    size_t startPos = m_CharPos;

    // Get tag identifier
    _SkipInner(m_InternalString);

    if (*m_CharStartPos == u8_Colon)
    {
        m_CharStartPos++;
        m_CharPos++;
        _SkipInner(m_InternalString);
    }

    // If input exhuasted
    if (*m_CharStartPos == u8_NullChar)
    {
        XML_Raise("premature end of input");
        return;
    }

    size_t distance = m_CharPos - startPos;
    Utf8String start = m_InternalString.SubStr(startPos, distance);

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Process attributes
    while (m_InternalString.Size() > m_CharPos && 
           *m_CharStartPos != u8_NullChar && 
           *m_CharStartPos != u8_Question)
    {
        _ParseAttr(elem);

        if (m_Status != 0)
        {
            // Propagate error
            return;
        }
    }

    m_CharStartPos++;
    m_CharPos++;

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Expect '>'
    if (*m_CharStartPos != u8_GreaterThan)
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected closing angle bracket");
        return;
    }
    m_CharStartPos++;
    m_CharPos++;

    // Return element object
    elem->SetName(start);
    elem->SetType(XML_INSTRUCTION);
    elem->SetText("");

    if (m_FoundRoot)
        m_State = STATE_CHARS;
    else
        m_State = STATE_START;
}


/*
**==============================================================================
**
** Parse a start tag
**
**==============================================================================
*/
void XMLReader::_ParseStartTag(pCXElement& elem)
{
    Utf8String name;
    size_t colonLoc;

    // Found the root
    m_FoundRoot = 1;

    // Get tag identifier
    size_t startPos = m_CharPos;

    if (!_IsFirst(*m_CharStartPos))
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected element name");
        return;
    }

    _SkipInner(m_InternalString);

    if (*m_CharStartPos == u8_Colon)
    {
        m_CharStartPos++;
        m_CharPos++;
        _SkipInner(m_InternalString);
    }

    // If input exhuasted
    if (*m_CharStartPos == u8_NullChar)
    {
        XML_Raise("premature end of input");
        return;
    }
    // Distance is the amount of string stripped away
    unsigned long distance = m_CharPos - startPos;
    name = m_InternalString.SubStr(startPos, distance);

    colonLoc = name.Find(u8_Colon);

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Process attributes
    while (m_InternalString.Size() > m_CharPos && 
           *m_CharStartPos != u8_NullChar && 
           *m_CharStartPos != u8_slash && 
           *m_CharStartPos != u8_GreaterThan)
    {
        _ParseAttr(elem);

        if (m_Status != 0)
            return;
    }

    // Check for empty tag
    if (*m_CharStartPos == u8_slash)
    {
        m_CharStartPos++;
        m_CharPos++;

        // Translate tag name (possibly replacing m_NameSpace with single char
        if (colonLoc != std::string::npos)
            name = _TranslateName(name, colonLoc);

        // Create the element
        elem->SetType(XML_START);
        elem->SetName(name);

        // Inject an empty tag onto elementm_Stack
        {
            // Check form_Stack overflow
            if (m_ElemStackSize == XML_MAX_NESTED)
            {
                XML_Raise("elementm_Stack overflow (>%u)", XML_MAX_NESTED);
                return;
            }

            pCXElement emptyElem(new CXElement());
            emptyElem->SetType(XML_END);
            emptyElem->SetName(name);
            m_ElemStack.push_back(emptyElem);
            m_ElemStackSize++;
            m_Nesting++;
        }

        // Skip space
        _SkipSpaces(m_InternalString);

        // Expect '>'
        if (*m_CharStartPos != u8_GreaterThan)
        {
            m_CharStartPos++;
            m_CharPos++;
            XML_Raise("expected closing angle bracket");
            return;
        }
        m_CharStartPos++;
        m_CharPos++;

        m_State = STATE_CHARS;
        return;
    }

    // Expect '>'
    if (*m_CharStartPos != u8_GreaterThan)
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected closing angle bracket");
        return;
    }
    m_CharStartPos++;
    m_CharPos++;

    // Translate the m_NameSpace prefix
    if (colonLoc != std::string::npos)
        name = _TranslateName(name, colonLoc);

    // Push opening tag
    {
        if (m_StackSize == XML_MAX_NESTED)
        {
            XML_Raise("elementm_Stack overflow (>%u)", XML_MAX_NESTED);
            return;
        }

        m_Stack.push_back(name);
        m_StackSize++;
        m_Nesting++;
    }

    // Return element object
    elem->SetType(XML_START);
    elem->SetName(name);

    if (m_FoundRoot)
        m_State = STATE_CHARS;
    else
        m_State = STATE_START;
}


/*
**==============================================================================
**
** Parse end tag
**
**==============================================================================
*/
void XMLReader::_ParseEndTag(pCXElement& elem)
{
    // Closing element: </name>
    Utf8String name;
    size_t colonLoc = 0;

    m_CharStartPos++;
    m_CharPos++;

    // Skip space
    _SkipSpaces(m_InternalString);

    // Skip name
    if (!_IsFirst(*m_CharStartPos))
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected element name");
        return;
    }

    size_t startPos = m_CharPos;
    m_CharStartPos++;
    m_CharPos++;

    _SkipInner(m_InternalString);

    if (*m_CharStartPos == u8_Colon)
    {
        m_CharStartPos++;
        m_CharPos++;
        _SkipInner(m_InternalString);
    }

    // If input exhuasted
    if (m_CharPos > m_InternalString.Size() || *m_CharStartPos == u8_NullChar)
    {
        XML_Raise("premature end of input");
        return;
    }

    // NUll terminate the name
    unsigned long distance = m_CharPos - startPos;
    name = m_InternalString.SubStr(startPos, distance);

    colonLoc = name.Find(u8_Colon);

    // Skip spaces
    _SkipSpaces(m_InternalString);

    // Expect '>'
    if (*m_CharStartPos != u8_GreaterThan)
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("expected closing angle bracket");
        return;
    }
    m_CharStartPos++;
    m_CharPos++;
 
    // Tranlate the m_NameSpace part of the name
    if (colonLoc != std::string::npos)
        name = _TranslateName(name, colonLoc);

    // Return element object
    elem->SetType(XML_END);
    elem->SetName(name);

    // Match opening name
    // Check form_Stack underflow
    if (m_StackSize-- == 0)
    {
        XML_Raise("too many closing tags: %s", name.Str().c_str());
        return;
    }

    m_Nesting--;

    // Check that closing name matches opening name
    {
        Utf8String xn = m_Stack.back();
        m_Stack.pop_back();

        if (xn != name)
        {
            XML_Raise("open/close tag mismatch: %s/%s", 
                xn.Str().c_str(), name.Str().c_str());
            return;
        }
    }

    // Remove m_NameSpaces that have just gone out of scope
    size_t i;
    size_t n = 0;

    for (i = m_NameSpacesSize; i--; )
    {
        if (m_NameSpaces[i]->depth >=m_StackSize)
            n++;
    }

    if (n)
    {
        m_NameSpacesSize -= n;

        // Clear single-entry cache
        if (m_NameSpacesCacheIndex >= m_NameSpacesSize)
            m_NameSpacesCacheIndex = (size_t)-1;
    }

    // Set next m_State
    m_State = STATE_CHARS;
}


/*
**==============================================================================
**
** Parse a comment tag
**
**==============================================================================
*/
void XMLReader::_ParseComment(pCXElement& elem)
{
    // Note:  Comments cannot be optimized because we don't have any really good way 
    // to tell how long they are.
    // Comment: <!-- blah blah blah --> 

    m_CharPos += 2; // Past the <!
    m_CharStartPos++;
    m_CharStartPos++;

    size_t startPos = m_CharPos;
    Utf8String start;

    while ((m_InternalString.Size() > m_CharPos) && *m_CharStartPos != u8_NullChar)
    {
        // Check if the next three characters are "-->" (end of comment)
        // (Note: We rely on short-circuiting to not run beyond end of string)
        if (   *m_CharStartPos == u8_Dash
            && *(m_CharStartPos+1) == u8_Dash
            && *(m_CharStartPos+2) == u8_GreaterThan)
        {
            // Null-terminate this comment
            size_t distance = m_CharPos - startPos;
            start = m_InternalString.SubStr(startPos, distance);

            m_CharPos += 3; // Past the -->
            m_CharStartPos++;
            m_CharStartPos++;
            m_CharStartPos++;

            // Prepare element
            elem->SetType(XML_COMMENT);
            elem->SetText(start);

            if (m_FoundRoot)
                m_State = STATE_CHARS;
            else
                m_State = STATE_START;

            return;
        }
        else if (*m_CharStartPos == u8_NewLine)
            m_Line++;

        m_CharStartPos++;
        m_CharPos++;
    }

    XML_Raise("malformed comment");
}


/*
**==============================================================================
**
** Parse a CDATA tag
**
**==============================================================================
*/
void XMLReader::_ParseCDATA(pCXElement& elem)
{
    // <![CDATA[...]]>
    for (int i = 0; i < 7; i++)
    {
        m_CharStartPos++;
        m_CharPos++;
    }
    size_t startPos = m_CharPos;
    Utf8String start;

    static Utf8String cdataEnd("]]>");

    while ((m_InternalString.Size() > m_CharPos) && *m_CharStartPos != u8_NullChar)
    {
        if (m_InternalString.Compare(m_CharPos, cdataEnd.Size(), cdataEnd))
        {
            unsigned long distance = m_CharPos - startPos;
            start = m_InternalString.SubStr(startPos, distance);

            m_CharPos += 3;
            m_CharStartPos++;
            m_CharStartPos++;
            m_CharStartPos++;

            // Prepare element
            elem->SetType(XML_CHARS);
            elem->SetText(start);

            // Set next m_State
            m_State = STATE_CHARS;

            return;

        } 
        else if (*m_CharStartPos == u8_NewLine)
        {
            m_Line++;
        }

        m_CharStartPos++;
        m_CharPos++;
    }

    XML_Raise("unterminated CDATA section");
    return;
}


/*
**==============================================================================
**
** Parse a DOCTYPE tag
**
**==============================================================================
*/
void XMLReader::_ParseDOCTYPE(pCXElement& /*unused*/)
{
    // NOTE:  We parse this tag, but we do not store it.  All we're doing is
    // moving past the tag.

    // Recognize <!DOCTYPE ...>
    for (int i = 0; i < 7; i++)
    {
        m_CharStartPos++;
        m_CharPos++;
    }

    while (m_InternalString.Size() > m_CharPos  && 
           *m_CharStartPos != u8_NullChar && 
           *m_CharStartPos != u8_GreaterThan)
    {
        if (*m_CharStartPos == u8_NewLine)
            m_Line++;

        m_CharStartPos++;
        m_CharPos++;
    }

    if (*m_CharStartPos != u8_GreaterThan)
    {
        m_CharStartPos++;
        m_CharPos++;
        XML_Raise("unterminated DOCTYPE element");
        return;
    }
    m_CharStartPos++;
    m_CharPos++;

    // Set next m_State
    if (m_FoundRoot)
        m_State = STATE_CHARS;
    else
        m_State = STATE_START;
}


/*
**==============================================================================
**
** Parse XML character data
**
**==============================================================================
*/
int XMLReader::_ParseCharData(pCXElement& elem)
{
    Utf8String end;

    // Skip leading spaces
    _SkipSpaces(m_InternalString);

    // Reject input if it does appear inside tags
    if (m_StackSize == 0)
    {
        if (m_InternalString.Size() >= m_CharPos || *m_CharStartPos == u8_NullChar)
        {
            // Proper end of input so set m_Status to zero
            m_Status = 1;
            return 0;
        }

        XML_Raise("markup outside root element");
        return 0;
    }

    // Remove leading spaces
    _SkipSpaces(m_InternalString);

    if (*m_CharStartPos == u8_LessThan)
    {
        m_CharStartPos++;
        m_CharPos++;
        m_State = STATE_TAG;
        return 0;
    }

    // reduce character data
    // This call potentially changes the input data.  That is, this is where things
    // like < get changed to &lt for serialization.  The string returned is everything in
    // the original string *after* the conversion has stopped.
    end = _ReduceCharData();

    if (m_Status != 0)
    {
        // Propagate error
        end.Clear();
        return 0;
    }

    // Process character data
    if (*m_CharStartPos != u8_LessThan)
    {
        XML_Raise("expected opening angle bracket");
        return 0;
    }

    // Return character data element if non-empty
    if (end.Empty())
      {
        return 0;
      }

    // Set next m_State
    m_CharStartPos++;
    m_CharPos++;
    m_State = STATE_TAG;

    // Prepare Element
    elem->SetType(XML_CHARS);
    elem->SetText(end);

    end.Clear();

    // Return 1 to indicate non-empty element
    return 1;
}

/*
**==============================================================================
******************************* Public Methods *********************************
**==============================================================================
*/

/*
**==============================================================================
**
** Dump the namespace table to the log file
**
**==============================================================================
*/
void XML_NameSpace::XML_NameSpace_Dump( void )
{
  /*
    SCXCoreLib::SCXLogHandle logHandle(LogHandleCache::Instance().GetLogHandle(std::string("scx.client.utilities.xml.xml_namespace")));

    SCX_LOGINFO(logHandle, ("==== XML_NameSpace:"));
    SCX_LOGINFO(logHandle, ("name=") + name.Str());
    SCX_LOGINFO(logHandle, ("id=") + id);
    SCX_LOGINFO(logHandle, ("uri=") + uri.Str());
    std::string depthStr;
    std::stringstream strDepth;
    strDepth << (int)depth;
    depthStr = strDepth.str();
    SCX_LOGINFO(logHandle, ("depth=") + depthStr);
    putchar(u8_NewLine);
  */
}


/*
**==============================================================================
**
** Initialize/Reinitialize the object
**
**==============================================================================
*/
void XMLReader::XML_Init( bool stripNamespaces /* = true */ )
{
    m_InternalString = "";
    m_Line = 0;
    m_Status = 0;
    strncpy(m_Message, "No error", 8); // This is used as a varargs, so it can't be a string
    m_StackSize = 0;
    m_Nesting = 0;
    m_ElemStackSize = 0;
    m_NameSpacesSize = 0;
    m_RegisteredNameSpacesSize = 0;
    m_State = STATE_START;
    m_FoundRoot = 0;
    m_NameSpacesCacheIndex = (size_t)-1;
    m_CharStartPos = m_InternalString.Begin();
    m_CharPos = 0;
    m_stripNamespaces = stripNamespaces;
}


/*
**==============================================================================
**
** Set the string to be operated upon
**
**==============================================================================
*/
void XMLReader::XML_SetText(const Utf8String& inText)
{
    // text = inText;
    m_InternalString = inText;
    m_Line = 1;
    m_State = STATE_START;
    m_CharStartPos = m_InternalString.Begin();
    m_CharPos = 0;
}


/*
**==============================================================================
**
** PARSER MAIN LOOP ROUTINE
**
** This method is the parser main loop, after being called by the loader main loop in
** XElement::Load.  XElement::Load calls this routine to get a single element back,
** and to save that element.  It is not concerned with what the element actually is.
** THis method, on the other hand, does care what the element is.  It parses the 
** incoming string and builds the CXElement to pass back to the XElement layer.
**
**==============================================================================
*/
int XMLReader::XML_Next(pCXElement& elem)
{
    // If the previous element was empty (i.e. <foo/>, then we pushed an 
    // atificial XML_END object onto the stack.  Pop that element off
    // and pass it back to the higher level.
    if (m_ElemStack.size() > 0)
    {
        elem = m_ElemStack.front();
        m_ElemStack.pop_front();
        --m_ElemStackSize;
        m_Nesting--;
        return 0;
    }

    // This loop will exit on error, input exhaustion, or successful processing of a
    // element portion.  THose portions are START, TAG, and CHARS.
    while (1)
    {
        if (m_State == STATE_START)
        {
          // Skip spaces
            _SkipSpaces(m_InternalString);

            // Expect '<'
            if (*m_CharStartPos != u8_LessThan)
            {
                XML_Raise("expected opening angle bracket");
                return -1;
            }

            // The next state must be TAG
            m_CharStartPos++;
            m_CharPos++;
            m_State = STATE_TAG;
        } 
        else if (m_State == STATE_TAG)
        {
            // Skip spaces
            _SkipSpaces(m_InternalString);

            // Expect one of these
            if (*m_CharStartPos == u8_Slash)
            {
                // This was an element end ('/>')
                _ParseEndTag(elem);
                return m_Status;
            }
            else if (_IsFirst(*m_CharStartPos))
            {
                // This was one of the valid tag start characters
                _ParseStartTag(elem);
                return m_Status;
            }
            else if (*m_CharStartPos == u8_Question)
            {
                // The processing instructions (<?xml...?>)
                _ParseProcessingInstruction(elem);
                return m_Status;
            }
            else if (*m_CharStartPos == u8_Bang)
            {
                m_CharStartPos++;
                m_CharPos++;

                if (*m_CharStartPos == u8_Dash && m_InternalString[m_CharPos + 1] == u8_Dash)
                {
                    _ParseComment(elem);
                    return m_Status;
                }
                else if (m_InternalString.Compare(m_CharPos, c_CDATA.Size(), c_CDATA))
                {
                    _ParseCDATA(elem);
                    return m_Status;
                }
                else if (m_InternalString.Compare(m_CharPos, 7, c_DOCTYPE))
                {
                    _ParseDOCTYPE(elem);

                    if (m_Status != 0)
                        return -1;
                }
                else
                {
                    XML_Raise("expected comment, CDATA, or DOCTYPE");
                    return -1;
                }
            }
            else
            {
                XML_Raise("expected element");
                return -1;
            }
         }
         else if (m_State == STATE_CHARS)
         {
            if (_ParseCharData(elem) == 1)
            {
                // Return character data to caller
                return 0;
            }

            // A zero m_Status indicates empty character data
            if (m_Status != 0)
                return m_Status;
         }
         else
         {
             XML_Raise("unexpected m_State");
             return -1;
         }
    }

    return -1;
}


/*
**==============================================================================
**
** Get the next element in the string, then see if it's the same name and 
** type as the one passed in
**
**==============================================================================
*/
int XMLReader::XML_Expect(pCXElement& elem, XML_Type type, Utf8String name)
{
    if (XML_Next(elem) == 0 && 
        elem->GetType() == type && 
        (name.Empty() || elem->GetName() ==  name))
    {
        return 0;
    }

    if (type == XML_START)
        XML_Raise("expected element: <%s>: %s", name.Str().c_str(), elem->GetName().Str().c_str());
    else if (type == XML_END)
        XML_Raise("expected element: </%s>: %s", name.Str().c_str(), elem->GetName().Str().c_str());
    else if (type == XML_CHARS)
        XML_Raise("expected character data");

    return -1;
}


/*
**==============================================================================
**
** Skip the next element -- advance to the next start tag
**
**==============================================================================
*/
int XMLReader::XML_Skip( void )
{
    pCXElement tmp(new CXElement);
    size_t localm_Nesting = m_Nesting;

    while (localm_Nesting >= m_Nesting)
    {
        if (XML_Next(tmp) != 0)
            return -1;
    }

    return 0;
}


/*
**==============================================================================
**
** Register a namespace and the single character ID associated with it
**
**==============================================================================
*/
int XMLReader::XML_RegisterNameSpace(char id, Utf8String uri)
{
    pXML_RegisteredNameSpace rns(new XML_RegisteredNameSpace);
    // ATTN: we do not check for duplicates

    // Reject out of range ids
    if (id < u8_a || id > u8_z)
        return -1;

    // Check for overflow of the array
    if (m_RegisteredNameSpacesSize == XML_MAX_REGISTERED_NAMESPACES)
        return -1;

    // Reject zero-length URI's
    if (uri[0] == u8_NullChar)
        return -1;
    
    rns->id = id;
    rns->uri = uri;
    rns->uriCode = _HashCode(uri, uri.Size());

    m_RegisteredNameSpaces.push_back(rns);
    m_RegisteredNameSpacesSize++;

    return 0;
}


/*
**==============================================================================
**
** Dump the namespace tree to the log file
**
**==============================================================================
*/
void XMLReader::XML_Dump( void )
{
  
    size_t i;

    SCX_LOGINFO(m_logHandle, "==== XML:\n");
    SCX_LOGINFO(m_logHandle, "m_NameSpaces:\n");

    for (i = 0; i < m_NameSpacesSize; i++)
    {
        m_NameSpaces[i]->XML_NameSpace_Dump();
    }

    putchar(u8_NewLine);
  
}


/*
**==============================================================================
**
** Log an error
**
**==============================================================================
*/
void XMLReader::XML_PutError( void )
{
  
    if (m_Status == -1)
        SCX_LOGERROR(m_logHandle, m_Message);
  
}


/*
**==============================================================================
**
** Set the error state and message
**
**==============================================================================
*/
void XMLReader::XML_Raise(const char* format, ...)
{
    va_list ap;
    memset(&ap, 0, sizeof(ap));

    m_Status = -1;
    m_Message[0] = u8_NullChar;

    va_start(ap, format);
    vsnprintf(m_Message, sizeof(m_Message), format, ap);
    va_end(ap);

    SCX_LOGINFO(m_logHandle, ("XML_Raise called...") + std::string(m_Message));
    SCX_LOGINFO(m_logHandle, m_Message);
}
