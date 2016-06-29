//******************************************************************************
//
//  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
//  
//  \File: XMLWriter.h
//  
//  \brief Definition of the direct-to-file XML document writer
//         this file contains the CXElement class.  That class contains a Save() method that
//         will turn the current element recursively into a string.  Call Save() on the root,
//         and you will get a string that is the entire tree
//
//  \Date: 11-01-11
//  Implementation: LibXML.cpp
//
//******************************************************************************

#ifndef _XML_WRITER_H_
#define _XML_WRITER_H_

#include <util/Unicode.h>
#include <stack>
#include <string>
#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <errno.h>

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>

namespace SCX {
    namespace Util {
        namespace Xml {
                // XML element type tag
                enum XML_Type
                {
                    XML_NONE = 0,
                    XML_START,
                    XML_END,
                    XML_INSTRUCTION,
                    XML_CHARS,
                    XML_COMMENT
                };

                //******************************************************************************
                //
                //  \name      CXElement class
                //
                //  \brief     This class represents a single XML element.  It contains its 
                //             children but does not know who its parent is
                //
                //  \date      11-01-11
                //
                //******************************************************************************
                class CXElement;                                       // Forward declaration
                typedef SCXCoreLib::SCXHandle<CXElement> pCXElement;   // Forward declaration
                class CXElement
                {
                    public:
                        /*----------------------------------------------------------------------------*/
                        /**
                           Constructor
                        */
                        CXElement();

                        /*----------------------------------------------------------------------------*/
                        /**
                           Constructor

                           \param [in] Name -- The element name
                           \param [in] Text -- The value of the element
                        */
                        CXElement(const Utf8String& Name, const Utf8String& Text); 

                        /*----------------------------------------------------------------------------*/
                        /**
                           Destructor
                        */
                        ~CXElement();

                        /*----------------------------------------------------------------------------*/
                        /**
                           Deep copy an element into this element
                       
                           \param [in] source - The source element
                           \returns    None
                       
                        */
                        void CopyElement(CXElement* source);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Set the element name
                       
                           \param [in] Name - Element name
                           \returns    None
                       
                        */
                        void SetName(const Utf8String& Name);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Set the element content
                       
                           \param [in] Text - Element content
                           \returns    None
                       
                        */
                        void SetText(const Utf8String& Text);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Add an attribute pair to the current element
                       
                           \param [in] attributeName - The name
                           \param [in] attributeValue - The Value
                           \returns    None
                       
                        */
                        void AddAttribute(const Utf8String& AttributeName, const Utf8String& AttributeValue);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Add a child element to this one
                       
                           \param [in] child - The child to be added
                           \returns    None
                       
                        */
                        void AddChild(pCXElement& child);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Get the named child of the current element
                       
                           \param [in] Name - The name of the child to find
                           \returns    The specificed child, if it exists
                       
                        */
                        pCXElement GetChild(const Utf8String& Name);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Convert the current element and all children to a text string
                       
                           \param [out] sOut - A place to stuff the string
                           \param [in] bAddIndenataiton - If true, the indentaiton string will be placed in front of any
                                                          generated strings.  This is intended to be used to make the
                                                          text more human-readable.
                           \param [in] sIndenataion - The string to put in front.  Presumably, this will start out as an ampty string.
                                                      Each level of indentation will add 4 spaces to it before calling the
                                                      recursive Save() method.
                           \returns     None
                       
                        */
                        void Save(Utf8String& sOut, bool AddIndentation, Utf8String& Indentation);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Create a new element that is a child of Parent
                       
                           \param [in/Out] Parent - The parent element
                           \param [in] Name - The name
                           \param [in] Text - The Value
                           \returns    The new child element
                       
                        */
                        static pCXElement NewXElement(pCXElement& Parent, const Utf8String& Name, const Utf8String& Text);

                        /*----------------------------------------------------------------------------*/
                        /**
                           enable line separators on output
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        void EnableLineSeparators( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Disable line separators on output
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        void DisableLineSeparators( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Determine if the line separators are enabled
                       
                           \param [in] None
                           \returns    TRUE if line separators are enabled
                       
                        */
                        inline bool AreLineSeparatorsEnabled( void ) { return m_LineSeparatorsOn; };

                        /*----------------------------------------------------------------------------*/
                        /**
                           Return a pointer to this
                       
                           \param [in] None
                           \returns    This
                       
                        */
                        inline CXElement* Get( void ) {return this;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Set the element type
                       
                           \param [in] Type - The incoming type
                           \returns    None
                       
                        */
                        inline void SetType(const XML_Type type) {m_Type = type;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Get the element type
                       
                           \param [in] None
                           \returns    The current element type
                       
                        */
                        inline XML_Type GetType( void ) {return m_Type;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Return the current element contents
                       
                           \param [in] None
                           \returns    The current element contents
                       
                        */
                        inline const Utf8String GetText( void ) {return m_sText;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Return the current element name
                       
                           \param [in] None
                           \returns    The current name
                       
                        */
                        inline const Utf8String GetName( void ) {return m_sName;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Return the number of attributes currently assigned to this object
                       
                           \param [in] None
                           \returns    The current attribute count
                       
                        */
                        inline int GetAttributeCount( void ) {return (int)m_listAttribute.size();};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Get the name of a specific attribute
                       
                           \param [in] index - The index of the current attribute
                           \returns    The name at that index
                       
                        */
                        inline const Utf8String GetAttributeName(int index) {return m_listAttribute[index]->m_sName;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Get the value of a specific attribute
                       
                           \param [in] index - The index of the current attribute
                           \returns    The value at that index
                       
                        */
                        inline const Utf8String GetAttributeValue(int index) {return m_listAttribute[index]->m_sValue;};

                        /*----------------------------------------------------------------------------*/
                        /**
                           For a given name, find the matching ettribute lement and return that elements contents.
                       
                           \param [in] Name - The name of the attribute to find
                           \returns    The value of the attribute
                       
                        */
                        const Utf8String CXElement_GetAttr(const Utf8String& name);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Dump the current element and all of its children
                       
                           \param [in] None
                           \returns    None, but it writes a bunch to the log file
                       
                        */
                        void CXElement_Dump( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Reset all the attributes for this element
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        inline void ClearAttributes( void ) { m_listAttribute.clear(); };

                        /*----------------------------------------------------------------------------*/
                        /**
                           Clear all children of this element
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        inline void ClearChild( void ) { m_listChild.clear(); };

                    private:
                        // information about an attribute
                        class CXAttribute
                        {
                            friend class CXElement;

                            private:
                                CXAttribute (const Utf8String& Name, const Utf8String& Value) :
                                    m_sName(Name),
                                    m_sValue(Value)
                                {
                                };

                                ~CXAttribute()
                                {
                                    m_sName.Clear();
                                    m_sValue.Clear();
                                }

                                // name of the attribute
                                Utf8String m_sName;

                                // value of the attribute
                                Utf8String m_sValue;
                        };


                        /*----------------------------------------------------------------------------*/
                        /**
                           output an element text
                       
                           \param [in/out] sOut   - The string to which the encoded text will be appended
                           \param [in]     TextIn - The string containing the text to be encoded
                           \returns        0 if successful
                       
                        */
                        void PutText (Utf8String &sOut, Utf8String& TextIn);

                        // name of the element
                        Utf8String m_sName;

                        // text of the element
                        Utf8String m_sText;

                        // child elements of the element
                        std::vector<pCXElement> m_listChild;

                        // attributes of the element
                        std::vector<CXAttribute *> m_listAttribute;

                        // depth of the element in the xml tree
                        int m_nDepth;

                        // Enable line separators
                        bool m_LineSeparatorsOn;

                        // TYpe
                        XML_Type m_Type;
                };
        } // Namespace Xml
    } // Namespace Util
} // Namespace SCX
#endif // _XML_WRITER_H_
