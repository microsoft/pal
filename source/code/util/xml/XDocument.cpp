/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
 \file      XDocument.h

 \brief     Contains the code for the XDocument class

 \date      2011-10-20 16:40

 \author    Niel Orcutt
*/
/*----------------------------------------------------------------------------*/

#include <stddef.h>

#include <string>
#include <fstream> 
#include <streambuf> 

#include <util/XNode.h>
#include <util/XElement.h>
#include <util/XDocument.h>

using namespace SCX::Util;
using namespace SCX::Util::Xml;

/*
 Exception messages
*/
const std::string XDocument::EXCEPTION_DOCUMENT_INPUT_EMPTY("Document string empty");
const std::string XDocument::EXCEPTION_INVALID_MESSAGE_HEADER("Invalid message header");
const std::string XDocument::EXCEPTION_INVALID_DOCTYPE("Invalid DOCTYPE");
const std::string XDocument::EXCEPTION_INVALID_COMMENT("Invalid document comment");
const std::string XDocument::EXCEPTION_FILE_READ_ERROR("Error reading file");
const std::string XDocument::EXCEPTION_FILE_WRITE_ERROR("Error writing file");

/*
 \brief         Constructor for an empty document
*/
XDocument::XDocument()
{
    SetParent(NULL);
    SetNodeType(Document);
}

/*
 \brief         Constructor for a document with a given root element

 \param[in]     Declaration - the information for the declaration section of the document
 \param[in]     RootElement - a pointer to the element in the document
*/
XDocument::XDocument(XElementPtr RootElement)
{
    SetParent(NULL);
    SetNodeType(Document);
    m_RootElement = RootElement;
}

/*
 \brief         Serialize an XDocument document into a string.

 \return        The XML text string describing the document.
*/
Utf8String XDocument::ToString(const bool EnableLineSeparators /* = true */)
{
    Utf8String XmlString("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\r\n");

    if (m_DocumentType.length() != 0)
    {
        XmlString += std::string("<!DOCTYPE ") + m_DocumentType + " >\r\n";
    }
    if (m_Comment.length() != 0)
    {
        XmlString += std::string("<!-- ") + m_Comment + " -->\r\n";
    }
    if (m_RootElement != NULL)
    {
        Utf8String BodyString;

        m_RootElement->ToString(BodyString, EnableLineSeparators);
        XmlString += BodyString;
    }
 
    if (EnableLineSeparators)
    {
        XmlString += "\r\n";
    }
    return XmlString;
}

/*
 \brief         Read the XML document from a string.
 
 \param[in]     The name of the file to read.
 \param[out]    A new XDocument containing the structured representation of the file's data
 
 \throws        XmlException for invalid syntax
*/
void XDocument::Load(const std::string& XmlString, XDocument& doc)
{
    // size_t j, k;
    size_t i;

    if (XmlString.empty())
    {
        throw XmlException(XDocument::EXCEPTION_DOCUMENT_INPUT_EMPTY, "XDocument");
    }

    // Get rid of byte-order marks and leading spaces
    i = 0;
    while (i < XmlString.length() && (signed char)XmlString[i] <= ' ')
    {
        i++;
    }

    if (i != 0)
    {
        XElement::Load(XmlString.substr(i), doc.m_RootElement);
    }
    else
    {
        XElement::Load(XmlString, doc.m_RootElement);
    }

    return;
}

/*
 \brief         Read the XML document from a char vector.
 
 \param[in]     The vector
 \param[out]    A new XDocument containing the structured representation of the file's data
 
 \throws        XmlException for invalid syntax
*/
void XDocument::Load(const std::vector<char>& XMLVector, XDocument& doc)
{
    if (XMLVector.empty())
    {
        throw XmlException(XDocument::EXCEPTION_DOCUMENT_INPUT_EMPTY, "XDocument");
    }

    // If there's a BOM, skip past.  Either way, pass the rest to the Load method.
    size_t startPoint = 0;
    if (XMLVector.size() >= 3 &&
        XMLVector[0] == '\xEF' &&
        XMLVector[1] == '\xBB' &&
        XMLVector[2] == '\xBF')
    {
        startPoint = 3;
    }

    Load(&XMLVector[startPoint], doc);

    return;
}

/*
 \brief         Read the XML document from a file.
 
 \param[in]     The name of the file to read.
 \param[out]    A new XDocument containing the structured representation of the file's data
 
 \throws        XmlException for invalid syntax, file not found or other file read exceptions
*/
void XDocument::LoadFile(const std::string& File, XDocument& doc)
{
    std::vector<char> XMLSourceData;

    std::ifstream ClassFile(File.c_str());
    if (ClassFile.is_open())
    {
        // get length of file:
        ClassFile.seekg (0, std::ios::end);
        unsigned int Length = static_cast<unsigned int>(ClassFile.tellg());

        // Go back to the beginning
        ClassFile.seekg (0, std::ios::beg);

        // allocate memory to hold the file.  Here's the trick -- The member
        // XMLSourceData is a vector, and it needs to be for the rest of the code to
        // work right.  If you just do a loop and push_back onto the vector, that little
        // guy is going to resize lots and lots and lots, and that's not a good thing.  So,
        // instead, we know how many bytes there are in the file from above.  Resize the vector
        // to get contiguous memory, then do a bulk load.  If I ever see anyone else pulling
        // clever tricks like this, they'd better explain it at least this well.  Magic isn't cool.
        //
        // Yes, vectors are guaranteed to be contiguous memory.
        // http://herbsutter.com/2008/04/07/cringe-not-vectors-are-guaranteed-to-be-contiguous/
        XMLSourceData.clear();
        XMLSourceData.resize(Length+1, '\0');

        // read data as a block:
        ClassFile.read (&XMLSourceData[0], Length);
        ClassFile.close();

        // We have to NULL terminate the vector so the parsing routines know where the end of the
        // data is.  If we don't do this, the parser will continue reading memory until it finds a NULL,
        // or until it aborts.
        if (XMLSourceData.size() > 0)
        {
	    // If there's a BOM, skip past.  Either way, pass the rest to the Load method.
            if (XMLSourceData.size() >= 3 &&
                XMLSourceData[0] == '\xEF' &&
                XMLSourceData[1] == '\xBB' &&
                XMLSourceData[2] == '\xBF')
            {
                XMLSourceData.erase(XMLSourceData.begin(), XMLSourceData.begin() + 3);
            }

            Load(&XMLSourceData[0], doc);
        }

        XMLSourceData.clear();
    }
    else
    {
        throw XmlException(XDocument::EXCEPTION_FILE_READ_ERROR, File);
    }
    
    return;
}

/*
 \brief         Write the XML document in string form to a file.
 
 \param[in]     The name of the file to save the XML into.

 \throws        An exception for file not opened or other file writing exceptions
*/
void XDocument::Save(const std::string& File)
{

    std::ofstream OutputFileStream(File.c_str(), std::ios::out);
    if (!OutputFileStream)
    {
        throw XmlException(XDocument::EXCEPTION_FILE_WRITE_ERROR, File);
    }
    Utf8String XmlString = ToString();                      // add string
    XmlString.Write(OutputFileStream);

    return;
}
