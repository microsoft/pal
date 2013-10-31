/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implementation of operating system error handling utilities 
    
    \date        07-09-27 1815:00

    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/stringaid.h>
#include <sstream>
#if defined(WIN32)
#include <windows.h>
#endif

using namespace std;

namespace SCXCoreLib {
    /*----------------------------------------------------------------------------*/
    //! Retrieves the text corresponding to "last error" in windows.
    //! \param[in]     lastError     as returned by GetLastError()
    //! \returns     The corresponding text, if available.
    //!\note If the text is not available due to platform or some other reason,
    //!      the empty string will be returned.
    wstring TextOfWindowsLastError(unsigned int lastError) {
#if defined(WIN32)
        LPVOID  lpMsgBuf;
        wstring message(L"");
        unsigned int nrOfCharsInMessage = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            lastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR> (&lpMsgBuf),
            0, NULL );
        if (0 != nrOfCharsInMessage)
        {
            message = static_cast<LPTSTR>(lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        return message;
#else
        // Return an empty string in a way that avoids compiler warnings about unused parameter
        wstring text(StrFrom(lastError));
        text = L"";
        return text;
#endif
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs a text describing a problem caused by the occurance of 
    //! an unexpected windows error.
    //! \param[in]     problem     Description of a problem
    //! \param[in]     lastError     as returned by GetLastError()
    //! \returns     A text describing the problem and its cause    
    wstring UnexpectedWindowsError(const wstring &problem, 
            unsigned int lastError) {
        wostringstream buf;
        buf << problem << L" due to unexpected windows error " << lastError;
        buf << L" (" << TextOfWindowsLastError(lastError) << L")";
        return buf.str();
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs a text describing a problem caused by the occurance of 
    //! an unexpected errno when making a call to an operating system function.
    //! \param[in]     problem     Description of a problem
    //! \param[in]     errnoValue  The value of global variable erro right after the system call
    //! \returns     A text describing the problem and its cause    
    wstring UnexpectedErrno(const wstring &problem, int errnoValue) {
        wostringstream buf;
        buf << problem << L" due to unexpected errno " << errnoValue;
        return buf.str();
    }
    
    
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
