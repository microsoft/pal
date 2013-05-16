/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-08-05 12:45:04

    Log stdout backend test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxlogitem.h>
#include "scxcorelib/util/log/scxlogstdoutbackend.h"

using namespace SCXCoreLib;


class SCXLogStdoutBackendTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogStdoutBackendTest );
    CPPUNIT_TEST( TestInitialize );
    CPPUNIT_TEST( TestLogThisItem );
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestInitialize()
    {
        SCXLogStdoutBackend b;
        CPPUNIT_ASSERT( b.IsInitialized() );
    }

    void TestLogThisItem()
    {
        try
        {
            SCXFile::Delete(L"testlogfile.log");
        } 
        catch (SCXUnauthorizedFileSystemAccessException&)
        {
            // Ignore
        }
        SCXLogStdoutBackend b;
        b.SetSeverityThreshold(L"scx.core", eHysterical);

        SCXLogItem hysterical = SCXLogItem(L"scx.core", eHysterical, L"this", SCXSRCLOCATION, 0);
        SCXLogItem trace      = SCXLogItem(L"scx.core", eTrace,      L"is", SCXSRCLOCATION, 0);
        SCXLogItem info       = SCXLogItem(L"scx.core", eInfo,       L"not", SCXSRCLOCATION, 0);
        SCXLogItem warning    = SCXLogItem(L"scx.core", eWarning,    L"an", SCXSRCLOCATION, 0);
        SCXLogItem error      = SCXLogItem(L"scx.core", eError,      L"easter egg", SCXSRCLOCATION, 0);

        // Redirect stdout output to file.
        std::wfstream redirect("testlogfile.log", std::ios::out);
        std::wstreambuf* stdoutbuf = std::wcout.rdbuf(redirect.rdbuf());

        b.LogThisItem(hysterical);
        b.LogThisItem(trace);
        b.LogThisItem(info);
        b.LogThisItem(warning);
        b.LogThisItem(error);

        // Restore stdout.
        std::wcout.rdbuf(stdoutbuf);
        redirect.close();

        std::wfstream fs("testlogfile.log", std::ios::in);
        CPPUNIT_ASSERT(fs.is_open());
        std::wstring line;
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"this"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"Hysterical"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"scx.core"));
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"is"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"Trace"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"scx.core"));
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"not"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"Info"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"scx.core"));
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"an"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"Warning"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"scx.core"));
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"easter egg"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"Error"));
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"scx.core"));
        std::getline(fs, line);
        CPPUNIT_ASSERT(line == L"");
        CPPUNIT_ASSERT(fs.eof());
        fs.close();

        try
        {
            SCXFile::Delete(L"testlogfile.log");
        } 
        catch (SCXUnauthorizedFileSystemAccessException&)
        {
            // Ignore
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogStdoutBackendTest );
