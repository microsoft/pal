/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
 \file      XNode.h

 \brief     Contains the class definition for XNode

 \date      2011-10-20 15:58:46

 \author    Niel Orcutt
*/
/*----------------------------------------------------------------------------*/

#ifndef XDOCUMENT_H
#define XDOCUMENT_H

#include <util/Unicode.h>
#include <string>

#include <util/XNode.h>

namespace SCX
{
    namespace Util
    {
        namespace Xml
        {
                /*
                 \brief         A class that stores information about an XML document.

                                In this implementation, there is no class XComment derived from XNode.
                                This is because the current XElement type cannot contain comments and
                                the document can only contain a single comment, so the comment is stored
                                in this class as a string.
                */
                class XDocument : public XContainer
                {
                private:

                    /*
                     The comment in the document.
                    */
                    std::string m_Comment;

                    /*
                     The doctype string.
                    */
                    std::string m_DocumentType;

                    /*

                     The one root element in this document, NULL for none.
                    */
                    XElementPtr m_RootElement;

                public:

                    /*
                     \brief         Constructor for an empty document
                    */
                    XDocument();

                    /*
                     \brief         Destructor
                    */
                    ~XDocument()
                    {
                        m_RootElement = NULL;
                    }

                    /*
                     \brief         Constructor for a document with a given root element

                     \param[in]     Declaration - the information for the declaration section of the document
                     \param[in]     RootElement - a pointer to the element in the document
                    */
                    XDocument(XElementPtr RootElement);

                    /*
                     \brief         Get the XDocument that contains this node
                    */
                    inline XDocument* GetDocument()
                    {
                        return this;
                    }

                    /*
                     \brief         Set the root element in the document

                     \param[in]     A pointer to the root element in the document.  The root element
                                    is created and destroyed by the caller.  The root element must not
                                    be deleted until the XDocument class is deleted.
                    */
                    inline void SetRootElement(XElementPtr RootElement)
                    {
                        m_RootElement = RootElement;
                    }

                    /*
                     \brief         Get the node in the document

                     \returns       A pointer to the root element of the document or NULL if none
                    */
                    inline XElementPtr GetRootElement()
                    {
                        return m_RootElement;
                    }

                    /*
                     \brief         Set the comment in the document.  This implementation allows
                                    only a single comment at the document level and none at
                                    all below that.

                     \param[in]     The comment
                    */
                    inline void SetComment(std::string comment)
                    {
                        m_Comment = comment;
                    }

                    /*
                     \brief         Get the comment string in the document.
            
                     \returns       The comment or NULL if there is none.
                    */
                    inline std::string GetComment()
                    {
                        return m_Comment;
                    }

                    /*
                     \brief         Set the doctype string in the document.  This implementation
                                    takes the entire doctype string after "<!DOCTYPE " and before
                                    the final">".

                     \param[in]     The doctype string
                    */
                    inline void SetDocumentType(std::string docType)
                    {
                        m_DocumentType = docType;
                    }

                    /*
                     \brief         Get the doctype string in the document.  This implementation
                                    return the entire doctype string after "<!DOCTYPE " and before
                                    the final">".

                     \returns       The doctype string
                    */
                    inline std::string GetDocumentType()
                    {
                        return m_DocumentType;
                    }

                    /*
                     \brief         Get the string representation of this node
                    */
                    Utf8String ToString(const bool EnableLineSeparators = true);

                    /*
                     \brief         Read the XML document from a string.
 
                     \param[in]     The name of the file to read.
                     \param[out]    An XDocument containing the structured representation of the string's file 
                    */
                    static void Load(const std::string& File, XDocument& Document);

                    /*
                     \brief         Read the XML document from a character vector.
 
                     \param[in]     the vector
                     \param[out]    An XDocument containing the structured representation of the string's file 
                    */
                    static void Load(const std::vector<char>& SourceXml, XDocument& Document);

                    /*
                     \brief         Read the XML document from a file.
 
                     \param[in]     The name of the file to read.
                     \param[out]    An XDocument containing the structured representation of the string's file
 
                     \throws        XmlException for invalid syntax
                    */
                    static void LoadFile(const std::string& XmlString, XDocument& Document);

                    /*
                     \brief         Serialize the XML document to an XML string and save it to a file.
 
                     \param[in]     The name of the file to save.
                     \param[out]    An XDocument containing the structured representation of the string's file

                     \throws        An exception for file not found or other file write exceptions
                    */
                    void Save(const std::string& File);

                    static const std::string EXCEPTION_DOCUMENT_INPUT_EMPTY;
                    static const std::string EXCEPTION_INVALID_MESSAGE_HEADER;
                    static const std::string EXCEPTION_INVALID_DOCTYPE;
                    static const std::string EXCEPTION_INVALID_COMMENT;
                    static const std::string EXCEPTION_FILE_READ_ERROR;
                    static const std::string EXCEPTION_FILE_WRITE_ERROR;
                };
                typedef SCXCoreLib::SCXHandle<XDocument> XDocumentPtr;
        }
    }
}

#endif  //ndef XDOCUMENT_H
