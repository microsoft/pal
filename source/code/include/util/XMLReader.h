/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        XMLReader.h

    \brief       Contains the class definition for XML input parser class
    
    \date        11-13-11 15:58:46
   
*/
/*----------------------------------------------------------------------------*/


#ifndef _XML_READER_H_
#define XML_READER_H_

#include <util/Unicode.h>
#include <stddef.h>
#include <string>
#include <vector>
#include <deque>

#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>
#include <util/XMLWriter.h>

namespace SCX {
    namespace Util {
        namespace Xml {
                // Forward declaration
                class XMLReader;

                /*----------------------------------------------------------------------------*/
                /**
                   Represents an XML namespace as registered by the client
                   
                   \date        11-13-11 16:01:13
                */
                class XML_RegisteredNameSpace
                {
                    friend class XMLReader;
                    friend class XML_NameSpace;

                    public:
                       XML_RegisteredNameSpace() {};
                      ~XML_RegisteredNameSpace() {};
 
                    private:
                        // URI for this namespace
                       Utf8String uri;

                        // Hash code for uri
                        unsigned int uriCode;

                        // Single character namespace name expected by client
                        char id;
                };
                typedef SCXCoreLib::SCXHandle<XML_RegisteredNameSpace> pXML_RegisteredNameSpace;

                /*----------------------------------------------------------------------------*/
                /**
                   Represents an XML namespace as encountered during parsing
                   
                   \date        11-13-11 16:01:13
                */
                class XML_NameSpace : public XML_RegisteredNameSpace
                {
                    friend class XMLReader;

                    public:
                       XML_NameSpace() {};
                      ~XML_NameSpace() {};

                    private:
                        // Namespace name
                        Utf8String name;

                        // Hash code for name
                        unsigned int nameCode;

                        // Depth at which this definition was encountered
                        size_t depth;

                        /*----------------------------------------------------------------------------*/
                        /**
                           Dump the contents of a single namespace to stdout
                       
                           \param None
                       
                        */
                        void XML_NameSpace_Dump( void );
                };
                typedef SCXCoreLib::SCXHandle<XML_NameSpace> pXML_NameSpace;

                /*----------------------------------------------------------------------------*/
                /**
                   This is the primary class for the reading side of the parser.  It
                   uses variable m_InternalString to store the current XML string, and provides methods
                   that operate on the string.  The reader builds and manages a stack of
                   pCXElement objects, which are the individual elements of the input XML.

                   The input XML is destroyed during processing.
                   
                   \date        11-13-11 16:01:13
                */
                class XMLReader
                {
                    private:
                        // This is the current string, as it is being operated on.  It starts out as the entire
                        // XML string passed in, and becomes smaller as parsing progresses.  At the end of parsing,
                        // if successful, this will be empty, and member elemStack below will be fully populated.
                        Utf8String m_InternalString;

                        // The number of lines (\r\n) we've processed.
                        size_t m_Line;

                        // Status of the last operation: 0=Okay, 1=Done, 2=Failed
                        int m_Status;

                        // Error message from the current operation.  This can not be a string
                        char m_Message[256];
                        
                        // Stack of open tags (used to match closing tags) during parsing operations.
                        // Once push for each open, one pop for each close
                        std::deque<Utf8String> m_Stack;
                        size_t m_StackSize;

                        // Current nesting level
                        size_t m_Nesting;

                        // This is the element stack.  It starts at the root and goes down until the entire
                        // document is represented.  The top of this queue is what is returned by Load()
                        std::deque<pCXElement> m_ElemStack;
                        size_t m_ElemStackSize;

                        // Array of namespaces
                        std::vector <pXML_NameSpace> m_NameSpaces;
                        size_t m_NameSpacesSize;

                        // Index of last namespace lookup from nameSpaces[] array
                        size_t m_NameSpacesCacheIndex;

                        // Predefined namespaces
                        std::vector<pXML_RegisteredNameSpace> m_RegisteredNameSpaces;
                        size_t m_RegisteredNameSpacesSize;

                        // Whether XML root element has been encountered
                        int m_FoundRoot;

                        // The maximum number of nested XML elements
                        const unsigned int XML_MAX_NESTED;

                        // The maximum number of XML namespaces
                        const unsigned int XML_MAX_NAMESPACES;

                        // The maximum number of registered XML namespaces
                        const unsigned int XML_MAX_REGISTERED_NAMESPACES;

                        // The maximum number of attributes in a start tag
                        const unsigned int XML_MAX_ATTRIBUTES;

                        // Represents case where tag has no namespace
                        const unsigned int XML_NAMESPACE_NONE;

                        // The current state of the parser
                        typedef enum _XML_State
                        {
                            STATE_START = 0,
                            STATE_TAG,
                            STATE_CHARS
                        } XML_State;

                        // Internal parser state.  See enumerations above
                        XML_State m_State;

                        // Internal start/stop string positions
                        Utf8String::Iterator m_CharStartPos;
                        size_t m_CharPos;

                        // Strip the namespace name from the element (default TRUE)
                        bool m_stripNamespaces;

                    protected:
                        // SCX Log Handle
                        SCXCoreLib::SCXLogHandle m_logHandle;

                    private:
                        /*----------------------------------------------------------------------------*/
                        /**
                           Determine if the incoming character is an XML whitespace char
                       
                           \param [in] uc - The incoming character
                           \returns    0 if the character is not a space, 1 otherwise
                       
                        */
                        unsigned int _IsSpace(Utf8String::Char uc);

                        /*----------------------------------------------------------------------------*/
                        /**
                           See if the character is a valid XML starting character
                       
                           \param [in] c - The character to check
                           \returns 0 if this is not a valid character, 1 otherwise
                       
                        */
                        bool _IsFirst(Utf8String::Char c);
                        /*----------------------------------------------------------------------------*/
                        /**
                           Determine if the incoming character is a valid XML string char
                       
                           \param [in] c - The incoming character
                           \returns    0 if the character is valid, 1 otherwise
                       
                        */
                        bool _IsInner(Utf8String::Char c);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Advance the character pointer past the inner characters to the next control
                       
                           \param [in] p - The incoming string.
                           \returns    The shortened string
                       
                        */
                        void _SkipInner(Utf8String& p);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Advance the pointer past XML whitespace characters
                       
                           \param [in] p - the incoming string
                           \returns    The shortened string
                       
                        */
                        void _SkipSpacesAux(Utf8String& p);
                        void _SkipSpaces(Utf8String& p);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Change an entity string to the single dharacter it represents
                       
                           \param [in] p - The incoming string, starting one to the right of the &
                           \param [out] c - The replacement character
                        */
                        void _ToEntityRef(Utf8String& p, char& ch);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Change a reference character string (0xXXXX) to the single dharacter it represents
                       
                           \param [in] p - The incoming string, starting one to the right of the 0
                           \param [out] c - The replacement character
                        */
                        void _ToCharRef(Utf8String& p, char& ch);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Change a reference to a character (calls _ToEntityRef and _ToCharRef)
                       
                           \param [in] - The incoming string
                           \param [out] c - The changed character
                        */
                        void _ToRef(Utf8String& p, char& ch);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Take an attribute value string that may or may not contain references, and turn
                           it into a regular string

                           This method operates on member variable ptr
                       
                           \param [in] eos - The End Of String character
                           \returns    The changed string
                       
                        */
                        Utf8String _ReduceAttrValue(Utf8String::Char eos);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Take a character data element that may or may not contain references and change it into
                           a 'regular' string
                       
                           \param None -- operates on member variable ptr
                           \returns    The character string
                       
                        */
                        Utf8String _ReduceCharData( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Generate a has code for a namespace
                       
                           \param [in] s - The namespace
                           \param [in] n - How many characters
                           \returns    The computed has
                       
                        */
                        unsigned int _HashCode(Utf8String& s, size_t n);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Given a namespace string, find the single character ID that represents said namespace
                       
                           \param [in] uri - The URI containing the requested namespace string
                           \param [in] uriSize - The number of bytes to be considered
                           \returns     The character representing the namespace
                       
                        */
                        char _FindNameSpaceID(Utf8String uri, size_t uriSize);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Translate a name from a single character referenced string to a full string
                       
                           \param [in] name - The incoming namespace, including the colon
                           \param [in] colonLoc - The location of the colon in the incoming string
                           \returns    The translated string
                       
                        */
                        Utf8String _TranslateName(Utf8String& name, size_t colonLoc);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse and attribute pair.  Operates on member ptr

                           <tag attrName="attrValue> -- attrName and attrValue are an attribute pair
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseAttr(pCXElement& elemu);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse processing instructions.  This mehthod operates on member ptr

                           <?xml ...?>
                       
                           \param [in/out] elem - The element to which the attibutes will be added

                           // JWF NOTE:  Neil says he wants to deal with the processing instructions in the XDocument class,
                           //            so the processing instruction element IS NOT added to the stack
                        */
                        void _ParseProcessingInstruction(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse a start tag.  This method operates on member variable ptr

                           < -- Yes, that's it.  A single less than sign.
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseStartTag(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse an ending tag.  This method operates on member variable ptr
         
                           />  
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseEndTag(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse a comment.  Operates on member ptr
                       
                           <!-- comment -->

                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseComment(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse a CDATA element.  Operates on member ptr

                           <![CDATA[...]]>
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseCDATA(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse a DOCTYPE element.  Operates on member ptr

                           <!DOCTYPE ...>
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        void _ParseDOCTYPE(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Parse character data.  It may be a name or a value.  Only valid XML characters are allowed. 
                           Operates on member ptr
                       
                           \param [in/out] elem - The element to which the attibutes will be added
                        */
                        int _ParseCharData(pCXElement& elem);

                    public:
                        /*----------------------------------------------------------------------------*/
                        /**
                           Constructor
                        */
                        XMLReader( void )
                          :  XML_MAX_NESTED(64),
                             XML_MAX_NAMESPACES(32),
                             XML_MAX_REGISTERED_NAMESPACES(32),
                             XML_MAX_ATTRIBUTES(32),
                             XML_NAMESPACE_NONE(0),
                             m_CharStartPos(m_InternalString.Begin()),
                             m_CharPos(0),
                             m_logHandle(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.client.utilities.xml.XMLReader")) {};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Destructor
                        */
                        ~XMLReader( void ) {};

                        /*----------------------------------------------------------------------------*/
                        /**
                           Return a string containing all the attribute name/value pairs for a given element
                       
                           \param [in] Name - The name of the element you seek
                           \returns    The attribute list, as a string
                       
                        */
                        const Utf8String XML_Elem_GetAttr(const Utf8String& name);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Object initialization
                       
                           \param [in] None
                           \param [in] Strip namespaces as they are loaded
                           \returns    None
                       
                        */
                        void XML_Init( bool stripNamespaces = true );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Set the XML string the parser will be working on
                       
                           \param [in] inText - The XML to be parsed
                           \returns    None
                       
                        */
                        void XML_SetText(const Utf8String& inText);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Process the next element in the string
                       
                           \param [in/out] elem - The element to be used as root for this node
                           \returns    Status - 0 = success, 1 = done, 2 = error
                       
                        */
                        int XML_Next(pCXElement& elem);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Make sure that the next element in the string is the required type and has the correct name
                       
                           \param [in] type - The expected type of the next element
                           \param [in] name - the expected name
                           \returns    0 if the string matches, -1 otherwise
                       
                        */
                        int XML_Expect(pCXElement& elem, XML_Type type, Utf8String name);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Advance to the next start tag
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        int XML_Skip( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Register a namespace and the single character ID that represents it
                       
                           \param [in] id - The ID character for the namespace
                           \param [in] uri - The incoming namespace
                           \returns    0 for success
                       
                        */
                        int XML_RegisterNameSpace(char id, Utf8String uri);

                        /*----------------------------------------------------------------------------*/
                        /**
                           Print the current XML tree
                       
                           \param [in] None
                           \returns    None, but it spits out a bunch of stuff
                       
                        */
                        void XML_Dump( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Log the current error
                       
                           \param [in] None
                           \returns    None
                       
                        */
                        void XML_PutError( void );

                        /*----------------------------------------------------------------------------*/
                        /**
                           Raise an XML error.  This sets the internal state, and sets the internal message
                       
                           \param [in] varargs string to print
                           \returns    None
                       
                        */
                        void XML_Raise(const char* format, ...);


                        /*----------------------------------------------------------------------------*/
                        /**
                           Return the current error state
                       
                           \param [in] None
                           \returns    TRUE if the status is ERROR
                       
                        */
                        inline bool XML_GetError( void ) { return m_Status == -1; };

                        /*----------------------------------------------------------------------------*/
                        /**
                           Retrieve the current error message
                       
                           \param [In] None
                           \returns    The current error message string
                       
                        */
                        inline const std::string XML_GetErrorMessage( void ) { return m_Message; };
                }; // Class XMLReader
                typedef SCXCoreLib::SCXHandle<XMLReader> pXMLReader;
        } // namespace Xml
    } // namespace Util
} // namespace SCX

#endif /* XML_READER_H_ */
