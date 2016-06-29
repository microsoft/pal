/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/

#include "scxcorelib/scxcmn.h"
#include <util/XElement.h>
#include <stack>
#include <assert.h>
#include <cctype>
#include <algorithm>

using namespace SCX::Util;
using namespace SCX::Util::Xml;

const std::string XElement::EXCEPTION_MESSAGE_EMPTY_NAME            = "The Element name is empty";
const std::string XElement::EXCEPTION_MESSAGE_NULL_CHILD            = "The child is null";
const std::string XElement::EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME  = "The Attribute name cannot be negative";
const std::string XElement::EXCEPTION_MESSAGE_INPUT_EMPTY           = "The input xml string is empty";
const std::string XElement::EXCEPTION_MESSAGE_INVALID_NAME          = "The name is not valid XML name";
const std::string XElement::EXCEPTION_MESSAGE_RECURSIVE_CHILD       = "Attempted to add recursive child";

class XElement::XmlWriterImpl
{
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Default constructor
        */
        XmlWriterImpl() :
            m_writer(pCXElement(new CXElement()))
        {
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            Member access operator to access the writer directly
    
            \returns     Pointer to the writer
            
            This is just to increase readability of the code
            So we can write (*m_writer)->DoSomething() than m_writer->g
        */
        CXElement* operator->() const
        {
            return m_writer->Get();
        }

        CXElement* Get( void )
        {
            return m_writer->Get();
        }
        
    private:
        /** Actual XML Writer */
        pCXElement m_writer;
};

/*----------------------------------------------------------------------------*/
/**
    Validate if the given character is valid XML Name first character
    
    /param [in] c character to validate
    /return true if it is a valid XML Name first character
     NameStartChar = [_A-Za-z]
*/
static inline bool IsNameStartChar(char c)
{
    return ((isalpha(c) || c == '_') || c == '?'); // JWF -- had to have ? for processing instruction
}


/*----------------------------------------------------------------------------*/
/**
    Validate if the given character is valid XML Name non first character
    
    /param [in] c character to validate
    /return true if it is a valid XML Name non first character
    NameChar      = [_A-Za-z\-.0-9]
*/
static inline bool IsNameChar(char c)
{
    return ((IsNameStartChar(c) || isdigit(c) || c == '-' || c =='.' || c == ':'));
}

// reference http://www.w3.org/TR/REC-xml/#NT-NameStartChar
// NameStartChar = [_A-Za-z]
// NameChar      = [_A-Za-z\-.0-9]
// returning false for all the Extended ASCII characters. As we are working only with < 127
bool XElement::IsValidName(const Utf8String& name) const
{
    if (name.Empty())
    {
        // I think that, by definition, an empty name is pretty much invalid.
        return false;
    }

    // Check if the first character is valid
    if (!IsNameStartChar(static_cast<char>(name[0])))
    {
        // First character is not valid return false
        return false;
    }
    else
    {
        for (size_t pos = 1; pos < name.size(); pos++)
        {
            // Break on first invalid character
            if (!IsNameChar(static_cast<char>(name[pos])))
            {
                return false;
            }
        }
    }
    // All the characters have been verified--return true
    return true;
}

void XElement::SetName(const Utf8String& name)
{
    if (name.Empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_EMPTY_NAME, name);
    }
    
    if (IsValidName(name))
    {
        m_name = name;
    }
    else
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INVALID_NAME, name);
    }
}


XElement::XElement(const Utf8String& name) : 
    m_writer(NULL), 
    mp_myParent(NULL), 
    m_IsProcessingInstruction(false)
{
    SetName(name);
}

XElement::XElement(const Utf8String& name, const Utf8String& content) : 
    m_writer(NULL), 
    mp_myParent(NULL), 
    m_IsProcessingInstruction(false)
{
    SetName(name);
    SetContent(content);
} 

XElement::~XElement() 
{
    std::map<Utf8String, Utf8String>::iterator mapIt = m_attributeMap.begin();

    while (mapIt != m_attributeMap.end())
    {
        Utf8String key = (*mapIt).first;
        Utf8String value = (*mapIt).second;

        key.Clear();
        value.Clear();

        mapIt++;
    }

    m_childList.clear();
    m_attributeMap.clear();

    if( m_writer != NULL )
    {
        delete m_writer;
        m_writer = NULL;
    }
}

Utf8String XElement::GetName() const
{
    return m_name;
}

Utf8String XElement::GetContent() const
{
    return m_content;
}

void XElement::GetContent(std::string& content) const
{
    content = GetContent().Str();
    return;
}

void XElement::GetContent(std::wstring& content) const
{
    content = SCXCoreLib::StrFromUTF8(GetContent().Str());
    return;
}

void XElement::SetContent(const Utf8String& content)
{
    m_content = content;
}

void XElement::AddChild(const XElementPtr child)
{
    if (child == NULL)
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_NULL_CHILD, "NULL");
    }
    else if (child == this)
    { 
        //Base Case: Can't add yourself as your own child
        throw XmlException(XElement::EXCEPTION_MESSAGE_RECURSIVE_CHILD, child->GetName());
    }
#ifdef THIS_BREAKS_WHEN_CLONING_CHILDREN
    else if ( GetParentNode() != NULL )
    {
        if ( !CheckParentsForRecursion( child, this ) )
        {
            throw XmlException(XElement::EXCEPTION_MESSAGE_RECURSIVE_CHILD, child->GetName());
        }
    }
#endif

    // Everything checks out, let's add him
    child->SetParentNode(this);
    m_childList.push_back(child);
}

int XElement::GetChildCount() const
{
  return static_cast<int>(m_childList.size());
}

void XElement::SetParentNode(XElement *myParent) 
{
   mp_myParent = myParent;
#ifdef LET_ANYBODY_BE_A_PARENT
  (mp_myParent == NULL) ? 
       mp_myParent = myParent : 
      throw XmlException(XElement::EXCEPTION_MESSAGE_RECURSIVE_CHILD, myParent->GetName()); 
#endif
}

bool XElement::CheckParentsForRecursion(const XElementPtr origChild, XElement *origParent) const
{
    XElement* nextParent = origParent->GetParentNode();

    while (nextParent)
    {
        if (nextParent == origChild)
        {
            return false;  //We've met the enemy, and he is us!
        }

        nextParent = nextParent->GetParentNode();
    }

    return true;

}

void XElement::GetChildren(XElementList& childElements) const
{
    childElements = m_childList;
}
 
bool XElement::GetChild(const Utf8String& name, XElementPtr& child) const
{
    if (!name.Empty())
    {
        XElementList::const_iterator it;
        for (it = m_childList.begin(); it != m_childList.end(); ++it)
        {
            Utf8String childName = (*it)->GetName();
            if (childName == name)
            {
                child = *it;
                return true;
            }
        }
    }
    
    // If we are here. No children have been found return false;
    return false;
}

void XElement::SetAttributeValue(const Utf8String& name, const Utf8String& value)
{
    if (name.Empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME, name);
    }
    
    if (IsValidName(name))
    {
        // Insert or update
        std::map<Utf8String, Utf8String>::iterator it = m_attributeMap.find(name);
        
        if (it != m_attributeMap.end())
        {
            m_attributeMap.erase(it);
        }
        m_attributeMap.insert(make_pair(name, value));
    }
    else
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INVALID_NAME, name);
    }
}

bool XElement::GetAttributeValue(const Utf8String& name, Utf8String& value) const
{
    std::map<Utf8String, Utf8String>::const_iterator it;

    // if name is empty. It wil automatically return false;
    it = m_attributeMap.find(name);

    if (it != m_attributeMap.end())
    {
        value = it->second;
        return true;
    }

    // If we are here the attribute has not been found return false
    return false;
}

bool XElement::GetAttributeValue(const std::string& nameParam, std::string& valueParam) const
{
    const Utf8String name(nameParam);
    Utf8String value;

    bool r = GetAttributeValue(name, value);
    valueParam = value.Str();
    return r;
}

void XElement::GetAttributeMap(std::map<Utf8String, Utf8String>& attributeMap) const
{
    attributeMap = m_attributeMap;
}

void XElement::AddToWriter(pCXElement& parentElement, XElement* element, bool IsRootElement)
{
    // Get the List of Attributes and add it to the writer
    std::map<Utf8String, Utf8String> attributeMap;
    element->GetAttributeMap(attributeMap);
    
    Utf8String elemName = element->GetName();
    Utf8String elemContent = element->GetContent();

    pCXElement singleElement;

    if (!IsRootElement)
    {
        pCXElement newElement(new CXElement(elemName, elemContent));
        if(parentElement->AreLineSeparatorsEnabled())
        {
            newElement->EnableLineSeparators();
        }
        parentElement->AddChild(newElement);
        singleElement = newElement;
    }
    else
    {
        singleElement = parentElement;
        singleElement->SetName(elemName);
        singleElement->SetText(elemContent);
    }

    if (attributeMap.size() != 0)
    {
        std::map<Utf8String, Utf8String>::reverse_iterator it = attributeMap.rbegin();
        while(it != attributeMap.rend())
        {
            singleElement->AddAttribute(it->first, it->second);
            
            // increment the iterator
            ++it;
        }
    }
    
    // for each child do that same
    std::vector<XElementPtr> childElementList;
    element->GetChildren(childElementList);
    if (childElementList.size() != 0)
    {
        std::vector<XElementPtr>::const_iterator vi = childElementList.begin();
        while (vi != childElementList.end())
        {
            // Call recursive write
            AddToWriter(singleElement, vi->GetData());
            
            // increment iterator
            ++vi;
        }
    }
}

void XElement::ToString(Utf8String& xmlString, bool enableLineSeperators)
{
    // The writer has to be null
    assert(m_writer == NULL);
    
    m_writer = new XmlWriterImpl();
    
    // Write the current element and all its children to the writer
    pCXElement nullElement(new CXElement());
    if (enableLineSeperators)
    {
        nullElement->EnableLineSeparators();
    }
    else
    {
        nullElement->DisableLineSeparators();
    }

    AddToWriter(nullElement, this, true);
    
    // Clear the string before passing in, just in case.
    xmlString.Clear();
    Utf8String Indentation = "";
    nullElement->Save(xmlString, enableLineSeperators, Indentation);
    
    // delete the XML writer
    delete m_writer;
    m_writer = NULL;
}

void XElement::Load(const Utf8String& xmlString, XElementPtr& rootElement, bool stripNamespaces /* = true */)
{
    SCXCoreLib::SCXThreadLock Lock(XElementLoadLock);

    // Create a stack of ElementPtr
    std::stack<XElementPtr> elementStack;
    
    if (xmlString.Empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INPUT_EMPTY, xmlString);
    }
    
    XMLReader* reader = new XMLReader();
    pCXElement parseElement(new CXElement());

    reader->XML_Init(stripNamespaces);
    reader->XML_SetText(xmlString);
    
    XElementPtr currentElement(NULL);
    int stat;
    while(1)
    {
        stat = reader->XML_Next(parseElement);

        if (stat != 0)
        {
            break;
        }

        XML_Type elemType = parseElement->GetType();

        if (elemType ==  XML_START /* || elemType == XML_INSTRUCTION -- JWF -- NOT YET */)
        {
            size_t attributeCount;
    
            // if current element is not NULL, then this is a child element
            // push the current element to the stack
            if (currentElement != NULL)
            {
                elementStack.push(currentElement);
            }
  
            // Create the ElementPtr
            Utf8String parseElementName = parseElement->GetName();
            currentElement = new XElement(parseElementName);
            
            attributeCount = parseElement->GetAttributeCount();
            
            // Loop through the attributes and add them
            for (size_t i = 0; i < attributeCount; ++i)
            {
                currentElement->SetAttributeValue(parseElement->GetAttributeName(static_cast<int>(i)),
                                                  parseElement->GetAttributeValue(static_cast<int>(i)));
            }

            if (elemType == XML_INSTRUCTION)
            {
                currentElement->m_IsProcessingInstruction = true;
            }
        } 
        else if (elemType ==  XML_CHARS)
        {
            // If Chars was found add it as content to the current element
            currentElement->SetContent(parseElement->GetText());
        }
        else if (elemType ==  XML_END)
        {
            // The end tag should match to the current tag. The error condition should
            // have been caught by the reader. In case it is not, it would be a bug on that
            // library. So setting an assert
            Utf8String currentName = currentElement->GetName();
            Utf8String parseName = parseElement->GetName();
            assert(currentName == parseName);
            
            // The current element is complete. The top of the stack contains the parent
            // If the stack is empty then the current element is the root
            if (!elementStack.empty())
            {
                // Get the parent Element
                XElementPtr parentElement = elementStack.top();
                
                // Pop the stack to remove the parent element
                elementStack.pop();
                
                // Add the current element as a child 
                parentElement->AddChild(currentElement);
                
                // Set the current element as the parent
                currentElement = parentElement;
            }
        }
    }
    
    //Check for errors if any throw exception
    if (reader->XML_GetError())
    {
        std::string errorMsg(reader->XML_GetErrorMessage());
        delete reader;
        throw XmlException(errorMsg, xmlString);
    }

    delete reader;

    rootElement = currentElement;
}


