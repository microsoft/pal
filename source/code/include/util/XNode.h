/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
*/
/**
 \file      XNode.h

 \brief     Contains the class definition for XNode

 \date      2011-10-20 15:58

 \author    Niel Orcutt
*/
/*----------------------------------------------------------------------------*/

#ifndef XNODE_H
#define XNODE_H

#include <string>

namespace SCX
{
    namespace Util
    {
        namespace Xml
        {
                /*
                 The types of XML nodes.  Most are not implemented here yet.
                */
                enum XmlNodeType
                {
                    None,                   // Nothing
                    Element,                // An element (for example, <item> ).
                    Attribute,              // An attribute (for example, id='123').
                    Text,                   // The text content of a node.
                    CDATA,                  // A CDATA section (for example, <![CDATA[my escaped text]]>).
                    EntityReference,        // A reference to an entity (for example, &num;).
                    Entity,                 // An entity declaration (for example, <!ENTITY...>).
                    ProcessingInstruction,  // A processing instruction (for example, <?pi test?>).
                    Comment,                // A comment (for example, <!-- my comment -->).
                    Document,               // A document object that, as the root of the document tree, provides access to the entire XML document.
                    DocumentType,           // The document type declaration, indicated by the following tag (for example, <!DOCTYPE...> ).
                    DocumentFragment,       // A document fragment.
                    Notation,               // A notation in the document type declaration (for example, <!NOTATION...>).
                    Whitespace,             // White space between markup.
                    SignificantWhitespace,  // White space between markup in a mixed content model or white space within the xml:space="preserve" scope.
                    EndElement,             // An end element tag (for example, </item>).
                    EndEntity,              // Returned when XmlReader gets to the end of the entity replacement as a result of a call to ResolveEntity.
                    XmlDeclaration          // The XML declaration (for example, <?xml version='1.0'?>
                };

                class XDocument;
                class XContainer;

                /*
                 \brief     Information about the an XML document node.  A node is an object in XML that can
                            can be represented as an XML string or fragment.  In this implementation, a node is: a
                            document, element, attribute, comment, document type, or processing instruction.
                */
                class XNode
                {
                private:

                    /*
                     The parent document or element of this item
                    */
                    XContainer* m_Parent;

                    /*
                     The type of this item
                    */
                    XmlNodeType m_NodeType;

                public:

                    /*
                     \brief         Get the base URI for this node.  Currently, an empty string
                    */
                    inline std::string GetBaseUri()
                    {
                        return "";
                    }

                    /*
                     \brief         Set the node type for this node.

                     \param[in]     NodeType - the type of this node
                    */
                    inline void SetNodeType(XmlNodeType NodeType)
                    {
                        m_NodeType = NodeType;
                    }

                    /*
                     \brief         Get the type of this node.
                    */
                    inline XmlNodeType GetNodeType()
                    {
                        return m_NodeType;
                    }

                    /*
                     \brief         Get the XDocument that contains this node
                    */
                    XDocument* GetDocument();

                    /*
                     \brief         Set the parent of this node

                     \param[in]     Parent - The object's parent
                    */
                    inline void SetParent(XContainer* Parent)
                    {
                        m_Parent = Parent;
                    }

                    /*
                     \brief         Get the parent of this node.  Return NULL if this is the root
                    */
                    inline XContainer* GetParent()
                    {
                        return m_Parent;
                    }

                    /*
                     \brief         Get the string representation of this node
                    */
                    std::string ToString();
                };

                /*
                 \brief        A node that can contain other nodes--either a document or an element.
                               This class is only used for deriving XDocument and XElement as separate
                               types to make them the only node types that can be used in XNode::SetParent.
                */
                class XContainer : public XNode
                {
                };
        }
    }
}

#endif //ndef XNODE_H
