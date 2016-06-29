//******************************************************************************
//  Copyright (c) Microsoft.  All rights reserved.
//  File: XMLWriter.cpp
//  Implementation of the direct-to-file XML document writer
//  Declaration  : XMLWriter.h
//******************************************************************************

#include <util/XMLWriter.h>
#include <stdio.h>
#include <sstream>

using namespace SCX::Util;
using namespace SCX::Util::Xml;

static const Utf8String c_XmlAmp  = "&amp;" ;
static const Utf8String c_XmlLT   = "&lt;"  ;
static const Utf8String c_XmlGT   = "&gt;"  ;
static const Utf8String c_XmlApos = "&apos;";
static const Utf8String c_XmlQuot = "&quot;";
static const Utf8String c_XmlTab  = "&#";
static const Utf8String c_XmlEsc  = "\\\\";

static const Utf8String open_bracket("<");
static const Utf8String open_with_slash("</");
static const Utf8String close_bracket_with_term("/>");
static const Utf8String close_bracket_with_question("?>");
static const Utf8String close_bracket(">");
static const Utf8String u8_space(" ");
static const Utf8String equals_with_quote("=\"");
static const Utf8String ending_quote("\"");
static const Utf8String crlf("\r\n");
static const Utf8Char single_question('?');
static const Utf8String four_spaces("    ");
static const Utf8String empty_string("");

/*
**==============================================================================
**
**  EncodeChar - encode an xml character
**
**==============================================================================
*/
static inline void EncodeChar(CodePoint cp, Utf8String& Out)
{
    // Encode the reserved XML characters (&, <, >, ' and ") and remaining per the XML Spec
    // ref: http://www.w3.org/TR/REC-xml/#charsets
    // Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]  /* any Unicode character, excluding the surrogate blocks, FFFE, and FFFF. */
    // The surrogate
    switch (cp)
    {
        case '&':
            Out =  c_XmlAmp;
            break;
        case '<':
            Out = c_XmlLT  ;
            break;
        case '>':
            Out = c_XmlGT  ;
            break;
        case '\'':
            Out = c_XmlApos;
            break;
        case '\"':
            Out = c_XmlQuot;
            break;
        case '\\':
            Out = c_XmlEsc;
            break;
        default:
            // UTF-8 Characters are valid as per XML Specification. So for those, encode the individual characters
            // However, as stated above, control characters other than 0x09, 0x0A and 0x0D are invalid, so do not include them
            if ( ((0x0020  <= cp) && (cp <= 0xD7FF  )) ||
                 ((0xE000  <= cp) && (cp <= 0xFFFD  )) ||
                  (0x09    == cp) ||
                  (0x0A    == cp) ||
                  (0x0d    == cp) )
            {
                if (cp <= 0x20)
                {
                    Out = "&#x";

                    std::stringstream ss;
                    ss << std::hex << std::setw(4) << std::setfill('0') << cp;

                    Out += ss.str();
                    Out += ";";
                }
                else
                {
                    Utf8Char str[8];
                    size_t bytes = CodePointToUtf8(cp, str);
                    str[bytes] = '\0';
                    Out = str;
                }
            }
            break;
    }
}

//******************************************************************************
//  CXElement class
//******************************************************************************

/*
**==============================================================================
**
**  Constructor
**
**==============================================================================
*/

CXElement::CXElement( void ) :
    m_sName(""),
    m_sText(""),
    m_nDepth(0),
    m_LineSeparatorsOn(false),
    m_Type(XML_NONE)
{
}

/*
**==============================================================================
**
**  Constructor
**
**==============================================================================
*/
CXElement::CXElement(const Utf8String& Name, const Utf8String& Text) :
    m_sName(""),
    m_sText(""),
    m_nDepth(0),
    m_LineSeparatorsOn(false),
    m_Type(XML_NONE)
{
    SetName (Name);
    SetText (Text);
}

/*
**==============================================================================
**
**  Destructor
**
**==============================================================================
*/
CXElement::~CXElement()
{
    // Recursively delete the child elements, and then delete the attribute
    // list.
    m_listChild.clear();

    for (size_t i = 0; i < m_listAttribute.size(); i++)
    {
        CXAttribute *singleAttribute = m_listAttribute[i];

        delete singleAttribute;
    }
    m_listAttribute.clear();
}

/*
**==============================================================================
**
**  Perform a deep copy of an element into this element
**
**==============================================================================
*/
void CXElement::CopyElement(CXElement* source)
{
    // name of the element
    m_sName = source->m_sName;

    // text of the element
    m_sText = source->m_sText;

    // child elements of the element
    m_listChild = source->m_listChild;

    // attributes of the element
    m_listAttribute = source->m_listAttribute;

    // depth of the element in the xml tree
    m_nDepth = source->m_nDepth;

    // Enable line separators
    m_LineSeparatorsOn = source->m_LineSeparatorsOn;

    // TYpe
    m_Type = source->m_Type;
}

/*
**==============================================================================
**
**  Set the name of this element
**
**==============================================================================
*/
void CXElement::SetName(const Utf8String& Name)
{
    m_sName = Name;
}

/*
**==============================================================================
**
**  Set the text (content) for this element
**
**==============================================================================
*/
void CXElement::SetText(const Utf8String& Text)
{
    m_sText = Text;
}

/*
**==============================================================================
**
**  Add an attribute name/value pair to this element
**
**==============================================================================
*/
void CXElement::AddAttribute(const Utf8String& AttributeName, const Utf8String& AttributeValue)
{
    // Add the attribute/value pair to the list of attributes.
    CXAttribute *pAttr = new CXAttribute(AttributeName, AttributeValue);
    if (pAttr != NULL)
    {
        m_listAttribute.push_back (pAttr);
    }
}

/*
**==============================================================================
**
**  Get a named child of this element
**
**==============================================================================
*/
pCXElement CXElement::GetChild(const Utf8String& Name)
{
    // Find the child that matches the given name and return it.
    std::vector<pCXElement>::iterator i;
    for (i = m_listChild.begin();  i != m_listChild.end();  i++)
    {
        if ((*i)->m_sName == Name)
        {
            return (*i);
        }
    }
    return (pCXElement(NULL));
}

/*
**==============================================================================
**
**  Add a child to this element
**
**==============================================================================
*/
void CXElement::AddChild(pCXElement& child)
{
    m_listChild.push_back(child);
    m_nDepth++;
}

/*
**==============================================================================
**
**  Change the stored text for this element to one that has characters encoded
**  for output
**
**==============================================================================
*/
void CXElement::PutText(Utf8String& sOut, Utf8String& TextIn)
{
    if (!TextIn.Empty())
    {
        // Encode each character of the input string and add it to the output
        // string.
        Utf8String replacementString;
        Utf8String::Iterator start = TextIn.Begin();
        Utf8String::Iterator end = TextIn.End();
        for (; start != end; ++start)
        {
            Utf8String encodedStr;
            EncodeChar (*start, encodedStr);
            replacementString.Append(encodedStr);
        }

        sOut += replacementString;
    }

}

/*
**==============================================================================
**
**  Enable and disable line separators for this element
**
**==============================================================================
*/
void CXElement::EnableLineSeparators( void )
{
    m_LineSeparatorsOn = true;
}

void CXElement::DisableLineSeparators( void )
{
    m_LineSeparatorsOn = false;
}

/*
**==============================================================================
**
**  Starting with this element, build the output string that represents the entire
**  tree from this point down, building on the passed in string.  In other words,
**  the tree is traversed, and each element has its own save() method called.
**
**==============================================================================
*/
void CXElement::Save(Utf8String& sOut, bool bAddIndentation, Utf8String& sIndentation)
{
    // Output our start tag.
    if (bAddIndentation)
    {
        sOut += sIndentation;
    }

    sOut += open_bracket;
    sOut += m_sName;

    // Output all the attributes and close the start tag.
    for (size_t j = m_listAttribute.size();  j > 0;  j--)
    {
        sOut += u8_space + m_listAttribute[j - 1]->m_sName + equals_with_quote;
        PutText(sOut, m_listAttribute[j - 1]->m_sValue);
        sOut += ending_quote;
    }

    if (m_sName[0] != single_question) // XML_INSTRUCTION
    {
        // If this was an empty tag (just the name and attributes), and there were no children
        // then just close the tag.
        if (m_sText.Empty() && m_listChild.empty())
        {
            sOut += close_bracket_with_term;

            if (m_LineSeparatorsOn)
            {
                sOut += crlf;
            }

            return;
        }
        else
        {
            sOut += close_bracket;
        }
    }
    else
    {
        sOut += close_bracket_with_question;
    }

    if (!m_listChild.empty() && m_LineSeparatorsOn)
    {
        sOut += crlf;
    }

    // Output the text.
    // *** DO NOT put a line separator at the end of this data, or add indentation to the front.
    //     If you do so, and the data happens to be string data, you will be adding an
    //     extra \n\r to the output that will not be parsed out on input.  Changing the
    //     data is generally considered bad practice.
    if (!m_sText.Empty())
    {
        PutText (sOut, m_sText);
    }

    // Output all the child elements.
    std::vector<pCXElement>::iterator i;
    for (i = m_listChild.begin();  i != m_listChild.end();  i++)
    {
        pCXElement singleElement = *i;

        // Call the child element's save() method.
        sIndentation += four_spaces;
        singleElement->Save(sOut, bAddIndentation, sIndentation);
        sIndentation = sIndentation.substr(0, sIndentation.size() - 4);
    }

    if (m_sName[0] != single_question) // XML_INSTRUCTION
    {
        if (m_sText.Empty() && bAddIndentation)
        {
            sOut += sIndentation;
        }

        // Output the end tag.
        sOut += open_with_slash + m_sName + close_bracket;
    }

    // And close us up
    if (m_LineSeparatorsOn)
    {
        sOut += crlf;
    }
}

/*
**==============================================================================
**
**  Externally, create a new element and make it a child of the passed-in parent
**
**==============================================================================
*/
pCXElement CXElement::NewXElement (pCXElement& Parent, const Utf8String& Name, const Utf8String& Text)
{
    pCXElement newElement(new CXElement(Name, Text));
    if (Parent != NULL)
    {
        Parent->AddChild(newElement);
    }
    return newElement;
}

/*
**==============================================================================
**
**  Return the attribute value for a given name
**
**==============================================================================
*/
const Utf8String CXElement::CXElement_GetAttr(const Utf8String& name)
{
    size_t i;

    for (i = 0; i < m_listAttribute.size(); i++)
    {
        if (name == m_listAttribute[i]->m_sName)
            return m_listAttribute[i]->m_sValue;
    }

    /* Not found! */
    return empty_string;
}

/*
**==============================================================================
**
**  Dump an element to the log
**
**==============================================================================
*/
void CXElement::CXElement_Dump( void )
{
/*
    static const char* _typeNames[] =
    {
        "NONE",
        "START",
        "END",
        "INSTRUCTION",
        "CHARS",
        "COMMENT",
    };
    size_t i;
*/
    SCXCoreLib::SCXLogHandle logHandle(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.client.utilities.xml.CSElement"));

    SCX_LOGINFO(logHandle, ("==== CXElement:"));
    //SCX_LOGINFO(logHandle, ("type=") + _typeNames[(int)m_Type]);
    //SCX_LOGINFO(logHandle, ("name=") + m_sName);
    //SCX_LOGINFO(logHandle, ("text=") + m_sText);
/*
    if (!m_listAttribute.empty())
    {
        for (i = 0; i < m_listAttribute.size(); i++)
        {
            //SCX_LOGINFO(logHandle, m_listAttribute[i]->m_sName + (" = ") + m_listAttribute[i]->m_sValue);
        }
    }

    putchar('\n');
*/
}

