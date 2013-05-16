/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-07-25 14:00:46

    Log file backend test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxlogitem.h>
#include "scxcorelib/util/log/scxlogfilebackend.h"

using namespace SCXCoreLib;


class SCXLogFileBackendTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogFileBackendTest );
    CPPUNIT_TEST( TestInitialize );
    CPPUNIT_TEST( TestHeader );
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
        SCXLogFileBackend b;
        CPPUNIT_ASSERT( ! b.IsInitialized() );
        b.SetProperty(L"WHATEVER", L"value");
        CPPUNIT_ASSERT( ! b.IsInitialized() );
        b.SetProperty(L"PATH", L"testlogfile.log");
        CPPUNIT_ASSERT( b.IsInitialized() );

        SCXLogFileBackend b2(L"testlogfile2.log");
        CPPUNIT_ASSERT( b2.IsInitialized() );
    }

    void TestHeader()
    {
        try
        {
            SCXFile::Delete(L"testlogfile.log");
        } 
        catch (SCXUnauthorizedFileSystemAccessException&)
        {
            // Ignore
        }
        SCXLogFileBackend b(L"testlogfile.log");
        b.SetSeverityThreshold(L"scx.core", eWarning);
        SCXLogItem warning    = SCXLogItem(L"scx.core", eWarning,    L"No need to open file until something is logged.", SCXSRCLOCATION, 0);
        b.LogThisItem(warning);

        std::wfstream fs("testlogfile.log", std::ios::in);
        CPPUNIT_ASSERT(fs.is_open());
        std::wstring line;
        std::getline(fs, line);

        CPPUNIT_ASSERT(L"*" == line);
        std::getline(fs, line);
        CPPUNIT_ASSERT(L"* SCX Platform Abstraction Layer" == line);
#if !defined(WIN32)
        std::getline(fs, line);
        std::wstringstream ss;
        ss << L"<MAJOR>."
           << L"<MINOR>."
           << L"<PATCH>-"
           << L"<BUILDNR> "
           << L"(STATUS)";
        CPPUNIT_ASSERT(L"* Build number: " + ss.str()  == line);
#endif
        std::getline(fs, line);
        CPPUNIT_ASSERT(L"* Process id: " + StrFrom(SCXProcess::GetCurrentProcessID()) == line);
        std::getline(fs, line);
        CPPUNIT_ASSERT( StrIsPrefix(line, L"* Process started: "));
        std::getline(fs, line);
        CPPUNIT_ASSERT(L"*" == line);
        std::getline(fs, line);
        CPPUNIT_ASSERT(L"* Log format: <date> <severity>     [<code module>:<line number>:<process id>:<thread id>] <message>" == line);
        std::getline(fs, line);
        CPPUNIT_ASSERT(L"*" == line);
        std::getline(fs, line);
        CPPUNIT_ASSERT(std::wstring::npos != line.find(L"No need to open file until something is logged."));
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
        SCXLogFileBackend b(L"testlogfile.log");
        b.SetSeverityThreshold(L"scx.core", eHysterical);

        SCXLogItem hysterical = SCXLogItem(L"scx.core", eHysterical, L"this (which is not part of the file header)", SCXSRCLOCATION, 0);
        SCXLogItem trace      = SCXLogItem(L"scx.core", eTrace,      L"is", SCXSRCLOCATION, 0);
        SCXLogItem info       = SCXLogItem(L"scx.core", eInfo,       L"not", SCXSRCLOCATION, 0);
        SCXLogItem warning    = SCXLogItem(L"scx.core", eWarning,    L"an", SCXSRCLOCATION, 0);
        SCXLogItem error      = SCXLogItem(L"scx.core", eError,      L"easter egg", SCXSRCLOCATION, 0);

        b.LogThisItem(hysterical);
        b.LogThisItem(trace);
        b.LogThisItem(info);
        b.LogThisItem(warning);
        b.LogThisItem(error);

        std::wfstream fs("testlogfile.log", std::ios::in);
        CPPUNIT_ASSERT(fs.is_open());
        // Skip past file header.
        std::wstring line;
        for (getline(fs, line); std::wstring::npos == line.find(L"this (which is not part of the file header)"); getline(fs, line))
        {
            CPPUNIT_ASSERT( ! fs.eof() );
        }
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

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogFileBackendTest );
