/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        XElement.h

    \brief       Contains the class definition for Xml Utilities
    
    \date        01-13-11 15:58:46
   
*/
/*----------------------------------------------------------------------------*/

#ifndef XELEMENT_H
#define XELEMENT_H

#include <util/Unicode.h>
#include <string>
#include <vector>
#include <map>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/stringaid.h>
#include <util/XMLWriter.h>
#include <util/XMLReader.h>

namespace SCX
{
    namespace Util
    {
        namespace Xml
        {
                // Forward declaring to enable XElementPr typedef
                class XElement;

                /*----------------------------------------------------------------------------*/
                /**
                   Represents a XElement Safe Pointer
                   
                   \date    01-14-11 09:19:18
                   
                   auto_ptr is not safe to be used in STL, so using ScxHandle
                */
                typedef SCXCoreLib::SCXHandle<XElement> XElementPtr;

                /*----------------------------------------------------------------------------*/
                /**
                   Represents a XElement Safe Pointer Vector

                   \date    01-14-11 09:19:18
                   
                */
                typedef std::vector<XElementPtr> XElementList;

                /*----------------------------------------------------------------------------*/
                /**
                   Represents a XML Element for processing and creating XML
                   
                   \date    01-13-11 16:01:13
                   
                   Most the requirements for the XML processing for the client requires a lot of
                   in memory processing. In memory processing are applicable for small XML files.
                   
                   \warning There are two outstanding bugs that will be fixed in the future
                   \li The static method Load needs to be made thread safe
                   \li Protect against adding looping children
                */
                class XElement
                {
                public:

                    /*----------------------------------------------------------------------------*/
                    /**
                       Create a XElement object with name
                       
                       \param [in] name of the Element
                       
                    */
                    XElement(const Utf8String& name);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Create a XElement object with name
                       
                       \param [in] name Name of the element
                       \param [in] content Content text of the element
                       
                    */
                    XElement(const Utf8String& name, const Utf8String& content);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Virtual destructor for XElement
                    */
                    virtual ~XElement();
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the name of the element
                       
                       \return Name of the element
                        */
                    Utf8String GetName() const;
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the content text of the element
                       
                       \return Content text of the element
                    */
                    Utf8String GetContent() const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the content text of the element

                       \param [out] content to be returned
                    */
                    void GetContent(std::string& content) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the content text of the element

                       \param [out] content to be returned
                    */
                    void GetContent(std::wstring& content) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Set the content text of the element
                       
                       \param [in] Content text of the element
                    */
                    void SetContent(const Utf8String& content);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Add a child to the element
                       
                       \param [in] child Pointer to Element to be added as a child
                    */
                    void AddChild(const XElementPtr child);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the first child with name
                       
                       \param [in] name Name of the child to search
                       \param [out] child Child Element Pointer
                       \return Return true if a child with the name was found
                       
                       If tranversing a element with the root child having multiple siblings
                       use GetChildren. 
                       \warning The returned child is editable. 
                    */
                    bool GetChild(const Utf8String& name, XElementPtr& child) const;
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get all the children of the element
                       
                       \param [out] childElements Vector of all child elements
                    */
                    void GetChildren(XElementList& childElements) const;
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Set the value of an attribute. If a particular attribute name is not found.
                       the attribute is added.
                       
                       \param [in] name Name of the attribute
                       \param [in] vale Value of the attribute
                    */
                    void SetAttributeValue(const Utf8String& name, const Utf8String& value);

                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the value of the attribute. If the value is not found a false is returned
                       
                       \param [in] name Name of the attribute
                       \param [out] value Value of the attribute
                       \return true if the attribute is present
                    */
                    bool GetAttributeValue(const Utf8String& name, Utf8String& value) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the value of the attribute. If the value is not found a false is returned
                       
                       \param [in] name Name of the attribute
                       \param [out] value Value of the attribute
                       \return true if the attribute is present
                    */
                    bool GetAttributeValue(const std::string& name, std::string& value) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the attribute map of the element
                       
                       \param [out] attributeMap map of the element
                    */
                    void GetAttributeMap(std::map<Utf8String, Utf8String>& attributeMap) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Load a XML string into the XElement
                       
                       \param [in] xmlString String representation of an Xml 
                       \param [out] element The loaded xml's root element
                       \param [in] Strip namespaces as they are loaded

                       This is a static method and is thread safe. The thread safety might cause the
                       loading of "large" xml files slow.
                    */
                    static void Load(const Utf8String& xmlString, XElementPtr& element, bool stripNamespaces = true);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Save the XElement as Xml String
                       
                       \param [out] xmlString String representation of an Xml 
                       \param [in] enableLineSeperators Enable insertion of new lines between elements
                    */
                    void ToString(Utf8String& xmlString, bool enableLineSeperators);
                    
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get a pointer to the Parent of this node:
                       
                       ////////////////////////////////
                       Note: We allow only 1 parent per child node.
                       This keeps our loop detection working.
                       ////////////////////////////////
                       
                       \param None.
                       \return Ptr to Parent
                    */
                    XElement *GetParentNode() { return mp_myParent; }

                    /*----------------------------------------------------------------------------*/
                    /**
                       Set a pointer to the Parent of this node:
                       
                       \param [in] Ptr to the XElement parent of this node.
                       \return None.
                    */
                    void SetParentNode(XElement *myParent); 
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Select all nodes that match the given XPATH query
                       
                       \param [int] xpath XPATH query to execute
                       \param [out] selectionList List of XElementPtr that matches the query
                       \return true if the selection yeilded atleast one element
                       
                       \warning NOT IMPLEMENTED
                    */
                    bool SelectNodes(const Utf8String& xpath, XElementList selectionList);

                    /*----------------------------------------------------------------------------*/
                    /**
                       Select the first that matches the given XPATH query
                       
                       \param [int] xpath XPATH query to execute
                       \param [out] selection The first XElementPtr that matches the query
                       \return true if the selection yielded one element
                       
                       \warning NOT IMPLEMENTED
                    */
                    bool SelectSingleNode(const Utf8String& xpath, XElementPtr selection);
                    

                private:
                    /** The name of the XElement */
                    Utf8String m_name;
                    
                    /** The content string of the XElement */
                    Utf8String m_content;
                    
                    /** The vector of child element pointers */
                    XElementList m_childList;

                    /** The map of attribute names and values */
                    std::map<Utf8String, Utf8String> m_attributeMap;
                    
                    
                    class XmlWriterImpl; // Forward declaration to hide implementation
                    
                    /** Pointer to the XmlWriter Implmentation */
                    XmlWriterImpl* m_writer;
                    // Note: auto_ptr could not be used for the above. As its desctructor will not be called
                    
                    /** Pointer to the Parent Element */
                    XElement *mp_myParent;
                    // Note: auto_ptr could not be used for the above. As its desctructor destroys the other node,
                    // and causes Sigsegv's
                    
                    // The processing instructions element
                    bool m_IsProcessingInstruction;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Hiding the copy constructor to save unintended consequence of copy STL 
                       container members.
                    */
                    XElement(const XElement& element);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Set the name of the XElement

                       /param [in] name  Name of the XElement
                    */
                    void SetName(const Utf8String& name);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Validate if the name is a valid XML Element\Attribute name
                       
                       /param [in] name Name to validate
                       /return true if the passed name is a valid XML name
                    */
                    bool IsValidName(const Utf8String& name) const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Add the element to XML Writer
                       
                       /param [in] parnetElement  The parent for this node.  NULL means this is root
                       /param [in] element  element add to the Writer
                    */
                    void AddToWriter(pCXElement& parentElement, XElement* element, bool IsRootElement = false);
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Do a check to see if there are any loops back to this child.
                       

                       \param [in] origChild: Ptr to the child to be added 
                       \return bool: TRUE means no circular reference found,
                       FALSE means there is a circular reference. 
                    */
                    bool CheckParentsForRecursion(const XElementPtr origChild, XElement *origParent) const;
                    
                    /*----------------------------------------------------------------------------*/
                    /**
                       Get the number of children for the given node
                       
                       \param None.
                       \return Count of the number of Children
                                                           
                    */
                    int GetChildCount() const;

                    /*----------------------------------------------------------------------------*/
                    /**
                       Set the flag that says this element contains processing instructions
                       
                       \param TRUE if this is a processing instructions element.
                       \return None
                                                           
                    */
                    inline void SetProcessingInstructionsFlag(bool Flag) {m_IsProcessingInstruction = Flag;};

                    /** 
                     * Constants
                     */
                    
                    static const std::string EXCEPTION_MESSAGE_EMPTY_NAME;
                    static const std::string EXCEPTION_MESSAGE_NULL_CHILD;
                    static const std::string EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME;
                    static const std::string EXCEPTION_MESSAGE_INPUT_EMPTY;
                    static const std::string EXCEPTION_MESSAGE_INVALID_NAME;
                    static const std::string EXCEPTION_MESSAGE_RECURSIVE_CHILD;
                };

                /*----------------------------------------------------------------------------*/
                /**
                   Base class for all Xml Parsing \ Building Exceptions
                   
                   \date    01-14-11 09:37:53
                */
                class XmlException : public SCXCoreLib::SCXException
                {
                        
                public:

                    /*----------------------------------------------------------------------------*/
                    /**
                    Create a MessagingException object

                    \param [in] message The exception message
                    */
                    
                    XmlException(const std::string& message, 
                                 const Utf8String& xmlComponent) 
                        : m_message(SCXCoreLib::StrFromUTF8(message)),
                          m_xmlComponent(xmlComponent)
                          {}
                    
                    ~XmlException() throw() 
                        {}
                    
                    std::wstring What() const
                    { 
                        std::wstring errMsg = L"Error Message: ";
                                  errMsg += m_message;
                                  errMsg += L" XML Component: ";
                                  errMsg += SCXCoreLib::StrFromUTF8(m_xmlComponent.Str());
                        return errMsg;
                    }
                    
                protected:

                    // Message to be printed
                    std::wstring m_message;

                    // Faulty XML Component(XML string, name, value, attribute) thats causing the exception
                    Utf8String m_xmlComponent;
                };

                static SCXCoreLib::SCXThreadLockHandle XElementLoadLock(L"XElement::Load");
        }
    }
}
#endif /* XELEMENT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
