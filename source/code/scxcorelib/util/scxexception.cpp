/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Implementation regarding the SCXException base class
 
    \date      07-05-24 14:30:18
 
    < Optional detailed description of file purpose >  
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>

using namespace std;

namespace SCXCoreLib {


    /******************************************************************************
     *  
     *   SCXCodeLocation implementation
     *  
     ******************************************************************************/

    /*----------------------------------------------------------------------------*/
    /**
       Returns a formatted string with where an exception occured
    
    */
    wstring SCXCodeLocation::Where() const 
    {
        if (m_File.length())
        {
            // Compose e.g. [my_file.cpp:434]
            return L"[" + m_File + L":" + StrFrom(m_Line) + L"]"; 
        }
        else
        {
            return L"[unknown]";
        }
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Returns the line number where an exception occured, else returns "unknown"

    */
    wstring SCXCodeLocation::WhichLine() const
    {

        // Return line number if present
        if (m_File.length())
        {
            return StrFrom(m_Line);
        }
        else
        {
            return L"unknown";
        }
        
    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns filename where an exception occured if present, else returns "unknown"

    */
    wstring SCXCodeLocation::WhichFile() const
    {
        // Return file name if present
        if (m_File.length())
        {
            return m_File;
        }
        else
        {
            return L"unknown";
        }
        
    }

    /******************************************************************************
     *  
     *  SCXException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /**
       Returns a formatted string with details on an exception.
       
       Normally this does not need to be overridden in subclasses.
    
    */
    wstring SCXException::Where() const 
    { 
        if (m_StackContext.length())
        {
            return m_StackContext + L", thrown from " + m_OriginatingLocation.Where(); 
        }
        else
        {
            return m_OriginatingLocation.Where();
        }
    }
    
    
    /*----------------------------------------------------------------------------*/
    /**
       Add relevant stack context to an exception

       When a function catches an exception and pass it on, this method can be 
       used to add any information relevant to the end user.
       
    */
    void SCXException::AddStackContext(const wstring& context, const SCXCodeLocation& location) 
    {
        wstring thisContext = context;        // The context to add composed here

        if (location.GotInfo())
        {
            // Got line/position info, concatinate
            thisContext += location.Where();
        }

        // Add to existing context in the exception
        if (m_StackContext.length())
        {
            m_StackContext = thisContext + wstring(L"->") + m_StackContext;
        }
        else
        {
            m_StackContext = thisContext;
        }
    }

    void SCXException::AddStackContext(const wstring& context) 
    {
        AddStackContext(context, SCXCodeLocation());
    }

    void SCXException::AddStackContext(const SCXCodeLocation& location) 
    {
        AddStackContext(L"", location);
    }


    /******************************************************************************
     *  
     *  SCXInvalidArgumentException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format reason why the argument was invalid
       
    */
    wstring SCXInvalidArgumentException::What() const 
    { 
        // Example
        // Formal argument 'myArgument' is invalid: Syntax error
        return L"Formal argument '" + m_FormalArg + L"' is invalid: " + m_Reason;
    }


    /******************************************************************************
     *  
     *  SCXNULLPointerException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format which pointer was NULL
       
    */
    wstring SCXNULLPointerException::What() const 
    { 
        // Example:
        // A NULL pointer was supplied in argument 'myPointer'
        return L"A NULL pointer was supplied in argument '" + m_PointerName + L"'";
    }


    /******************************************************************************
     *  
     *  SCXIllegalIndexException Implementation 
     *  
     ******************************************************************************/
    
        

    /******************************************************************************
     *  
     *  SCXNotSupportedException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format details of violation
       
    */
    wstring SCXNotSupportedException::What() const 
    { 
        // Example
        // Enumeration of dead birds not supported
        return m_Functionality + L" not supported";
    }


    /******************************************************************************
     *  
     *  SCXInternalErrorException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format details of violation
       
    */
    wstring SCXInternalErrorException::What() const 
    { 
        // Example
        // Internal Error: expected element not found in list
        return L"Internal Error: " + m_Reason;
    }


    /******************************************************************************
     *  
     *  SCXResourceExhaustedException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format details of violation
       
    */
    wstring SCXResourceExhaustedException::What() const 
    { 
        // Example
        // Failed to allocate resource of type process: too many pids in system
        return L"Failed to allocate resource of type " + m_ResourceType + L": " + m_ResourceDetails;
    }

    /******************************************************************************
     *  
     *  SCXInvalidStateException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format reason why the state was invalid
       
    */
    wstring SCXInvalidStateException::What() const 
    { 
        // Example
        // Invalid state: Can not call method DoOtherStuff before DoStuff is called.
        return L"Invalid state: " + m_Reason;
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
