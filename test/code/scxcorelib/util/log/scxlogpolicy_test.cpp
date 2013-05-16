/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Tests the functionality of the log policy.

    \date        2008-09-02 14:49:05

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlogpolicy.h>
#include <testutils/scxunit.h> /* This will include CPPUNIT helper macros too */
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxstream.h>

using namespace SCXCoreLib;

class SCXLogPolicyTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogPolicyTest );
    CPPUNIT_TEST( TestTestrunnerDefault );
    CPPUNIT_TEST( TestTestrunnerDefaultLogFile );
    CPPUNIT_TEST_SUITE_END();

private:

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestTestrunnerDefault(void)
    {
        SCXHandle<SCXLogPolicy> p = CustomLogPolicyFactory();
        CPPUNIT_ASSERT(SCXFilePath(L"./scxlog.conf") == p->GetConfigFileName());
        CPPUNIT_ASSERT(SCXFilePath(L"./scxtestrunner.log") == p->GetDefaultLogFileName());
        CPPUNIT_ASSERT(eInfo == p->GetDefaultSeverityThreshold());
    }
    
    void TestTestrunnerDefaultLogFile(void)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(L"my.test.log.handle");

        // Try to make the log item as unique as possible by appending a time to the message.
        // This time will then be used in comparison later on.
        SCXCalendarTime t = SCXCalendarTime::CurrentLocal();

        SCX_LOGERROR(log, t.ToExtendedISO8601().append(L" - This is an error message"));
        SCX_LOGERROR(log, L"Looks like we need two messages here");
        
        SCXHandle<std::wfstream> stream =
        SCXFile::OpenWFstream(CustomLogPolicyFactory()->GetDefaultLogFileName(), std::ios_base::in);

        SCXStream::NLF nlf;
        for (std::wstring logrow;
             SCXStream::IsGood(*stream);
             SCXStream::ReadLine(*stream, logrow, nlf))
        {
            if (logrow.find(L"my.test.log.handle") != std::wstring::npos &&
                logrow.find(t.ToExtendedISO8601().append(L" - This is an error message")) != std::wstring::npos)
            {
                // Test is ok.
                return;
            }
        }
        CPPUNIT_FAIL("Log message not written to file");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogPolicyTest );
