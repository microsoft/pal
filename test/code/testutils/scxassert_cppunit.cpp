/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    20070618 09:21:00

    Implements the assert failed method for unit tests.
    
*/
/*----------------------------------------------------------------------------*/

#include <sstream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxassert.h>
#include <testutils/scxassert_cppunit.h>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib
{
    //! Number of asserts failed since last reset.
    unsigned int SCXAssertCounter::nrAssertsFailed = 0;
    //! Message of last assert.
    std::wstring SCXAssertCounter::lastMessage = L"";

    /*----------------------------------------------------------------------------*/
    /**
        Reset assert counter.
    
    */
    void SCXAssertCounter::Reset() {
        nrAssertsFailed = 0;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of failed asserts since last reset.
        \returns Asserts failed.
    */
    unsigned int SCXAssertCounter::GetFailedAsserts() {
        return nrAssertsFailed;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the message of the last assert with message.
        \returns Message of last assert.
    */
    const std::wstring& SCXAssertCounter::GetLastMessage() {
        return lastMessage;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add a new assert.
        \param m Assert message.

    */
    void SCXAssertCounter::AssertFailed(const std::wstring& m) {
        lastMessage = m;
        ++nrAssertsFailed;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Report assertion failed suitable for unit tests.
    
        Parameters:  c - The condition that failed (as a string).
                     f - name of file where asserttion failed.
                     l - line in file where assertion failed.
                     m - extra log message. Ignored if 0.
        Retval:      None.
        
        This method is called by the SCXASSERT macro when the assertion failes.
        For unit tests this macro does nothing more than increment the assertion
        failed counter.
    
    */
    void scx_assert_failed(const char* c, const char* f, const unsigned int l, const wchar_t* m /*= 0*/)
    {
        std::wstringstream ss;

        // this routine formats message for unit-test to display, so output should look like:
        // 
        // 1) test: SCXRunAsProviderTest::testDoInvokeMethodPartParams (F) 
        // Unexpected assertion failure
        // - SCXRunAsProviderTest::testDoInvokeMethodPartParams; Condiiton: ; File: /home/dp/dev/source/code/include/scxcorelib/scxexception.h: 373; 
        // [/home/dp/dev/source/code/providers/runas_provider/runasprovider.cpp:234]Internal Error: missing arguments to ExecuteCommand method

        ss << 
            L"Conditon: " <<
            SCXCoreLib::StrFromMultibyte(c ? c : "") << 
            L"; File: " <<
            SCXCoreLib::StrFromMultibyte(f ? f : "") <<
            L": " <<
            l <<
            L"; " << 
            (m ? m : L"");

        SCXAssertCounter::AssertFailed(ss.str());
    }
} /* namespace SCXCoreLib */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
