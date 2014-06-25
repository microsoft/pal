/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2007-08-27 17:20:00

    File test class.

*/
/*----------------------------------------------------------------------------*/
#include <testutils/scxunit.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace SCXCoreLib;
using namespace std;

class SCXFileSystemTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE( SCXFileSystemTest );

    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( createFileSystemExhaustedExceptionForCoverage );
    CPPUNIT_TEST( TestCreateFullPath );
    CPPUNIT_TEST( TestCreateFullPathForCoverage );
    CPPUNIT_TEST( TestEncodeDecodePath );
    CPPUNIT_TEST( TestFileAttributeAsTextConversion );
    CPPUNIT_TEST( TestStatFailsWithoutPermission );

    CPPUNIT_TEST_SUITE_END();

private:
    SCXFilePath m_path1;

public:
    void setUp() {
        m_path1 = SCXFilePath(L"SCXFileTestTemporary.txt");
        SCXFile::Delete(m_path1);
        ofstream file(SCXFileSystem::EncodePath(m_path1).c_str());
        file.close();
    }

    void tearDown() {
        SCXFile::Delete(m_path1);
    }

    void callDumpStringForCoverage()
    {
        CPPUNIT_ASSERT(SCXFileInfo(m_path1).DumpString().find(L"SCXFileTestTemporary.txt") != wstring::npos);
    }

    void createFileSystemExhaustedExceptionForCoverage()
    {
        SCXFileSystemExhaustedException e(L"RESOURCE", L"PATH", SCXSRCLOCATION);
        CPPUNIT_ASSERT(e.What().find(L"RESOURCE") != wstring::npos);
        CPPUNIT_ASSERT(e.What().find(L"PATH") != wstring::npos);
    }

    void TestCreateFullPath() {
        try {
            SCXFileInfo info(m_path1);
            CPPUNIT_ASSERT(SCXFileSystem::CreateFullPath(m_path1) ==  info.GetFullPath());
            CPPUNIT_ASSERT(SCXFileSystem::CreateFullPath(info.GetFullPath()) ==  info.GetFullPath());
        } catch (SCXException& e) {
            wstring problem(e.What() + L" occured at " + e.Where());
            CPPUNIT_ASSERT(!problem.c_str());
        }


        SCXFilePath fp1 = SCXFileSystem::CreateFullPath(SCXFilePath(L"/some/path/1/"));
        SCXFilePath fp2 = SCXFileSystem::CreateFullPath(SCXFilePath(L"/some/path/../1/"));
        SCXFilePath fp3 = SCXFileSystem::CreateFullPath(SCXFilePath(L"../some/path/../1/./"));
        SCXFilePath fp4 = SCXFileSystem::CreateFullPath(SCXFilePath(L"/../"));

#if defined(WIN32)
        CPPUNIT_ASSERT(wstring(fp1.Get(), 1) == L":\\some\\path\\1\\");
        CPPUNIT_ASSERT(wstring(fp2.Get(), 1) == L":\\some\\1\\");
        CPPUNIT_ASSERT(fp3.Get()[1] == L':');
        CPPUNIT_ASSERT(fp3.Get()[2] == L'\\');
        CPPUNIT_ASSERT(fp3.Get().rfind(L"some\\1\\") != wstring::npos);
        CPPUNIT_ASSERT(wstring(fp4.Get(), 1) == L":\\");
#else
        CPPUNIT_ASSERT(fp1.Get() == L"/some/path/1/");
        CPPUNIT_ASSERT(fp2.Get() == L"/some/1/");
        CPPUNIT_ASSERT(fp3.Get()[0] == L'/');
        CPPUNIT_ASSERT(fp3.Get().rfind(L"some/1/") != wstring::npos);
        CPPUNIT_ASSERT(fp4.Get() == L"/");
#endif
    }

    void TestCreateFullPathForCoverage() {
        try {
            m_path1.SetDirectory(L".");
            SCXFileInfo info(m_path1);
            CPPUNIT_ASSERT(SCXFileSystem::CreateFullPath(m_path1) ==  info.GetFullPath());
            CPPUNIT_ASSERT(SCXFileSystem::CreateFullPath(info.GetFullPath()) ==  info.GetFullPath());
        } catch (SCXException& e) {
            wstring problem(e.What() + L" occured at " + e.Where());
            CPPUNIT_ASSERT(!problem.c_str());
        }
    }

    void TestEncodeDecodePath() {
        CPPUNIT_ASSERT(SCXFileSystem::DecodePath(SCXFileSystem::EncodePath(m_path1)) == m_path1);
    }

    // This test was added to increase code coverage
    void TestFileAttributeAsTextConversion()
    {
        CPPUNIT_ASSERT_THROW(SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eUnknown), SCXCoreLib::SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
        CPPUNIT_ASSERT(L"Readable" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eReadable));
        CPPUNIT_ASSERT(L"Writable" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eWritable));
        CPPUNIT_ASSERT(L"Directory" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eDirectory));
        CPPUNIT_ASSERT(L"UserRead" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eUserRead));
        CPPUNIT_ASSERT(L"UserWrite" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eUserWrite));
        CPPUNIT_ASSERT(L"UserExecute" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eUserExecute));
        CPPUNIT_ASSERT(L"GroupRead" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eGroupRead));
        CPPUNIT_ASSERT(L"GroupWrite" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eGroupWrite));
        CPPUNIT_ASSERT(L"GroupExecute" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eGroupExecute));
        CPPUNIT_ASSERT(L"OtherRead" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eOtherRead));
        CPPUNIT_ASSERT(L"OtherWrite" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eOtherWrite));
        CPPUNIT_ASSERT(L"OtherExecute" == SCXCoreLib::SCXFileSystem::AsText(SCXCoreLib::SCXFileSystem::eOtherExecute));

        SCXCoreLib::SCXFileSystem::Attributes a;
        a.insert(SCXCoreLib::SCXFileSystem::eReadable);
        a.insert(SCXCoreLib::SCXFileSystem::eWritable);
        a.insert(SCXCoreLib::SCXFileSystem::eDirectory);
        a.insert(SCXCoreLib::SCXFileSystem::eUserRead);
        a.insert(SCXCoreLib::SCXFileSystem::eUserWrite);
        CPPUNIT_ASSERT(L"Directory,Readable,Writable,UserRead,UserWrite" == SCXCoreLib::SCXFileSystem::AsText(a));
    }

    void TestStatFailsWithoutPermission()
    {
#if defined(SCX_UNIX)
        SCXUser user;

        if (user.IsRoot())
        {
            SCXUNIT_WARNING(L"SCXFileSystem::Stat() test can only be run as non-root user");
        }
        else
        {
            // Create a test file in a new directory with rwx permissions
            SCXFilePath tmp_path = SCXFilePath(L"SCXFileTestTemporaryDir/");
            SCXFilePath tmp_file(tmp_path);
            tmp_file.SetFilename(L"test.tst");

            SCXDirectoryInfo dir = SCXDirectory::CreateDirectory(tmp_path);

            SCXFileSystem::Attributes attr;
            attr.insert(SCXFileSystem::eDirectory);
            attr.insert(SCXFileSystem::eUserRead);
            attr.insert(SCXFileSystem::eUserWrite);
            attr.insert(SCXFileSystem::eUserExecute);

            CPPUNIT_ASSERT_NO_THROW(SCXFileSystem::SetAttributes(tmp_path, attr));

            vector<wstring> lines;
            lines.push_back(L"test");

            CPPUNIT_ASSERT_NO_THROW(SCXFile::WriteAllLines(tmp_file, lines, ios_base::out));

            SCXFileSystem::SCXStatStruct stats;

            // Should be able to do stat() on this new file
            CPPUNIT_ASSERT_NO_THROW(SCXFileSystem::Stat(tmp_file, &stats));

            // Remove all permissions from the new directory
            SCXFileSystem::Attributes attr2;
            attr2.insert(SCXFileSystem::eDirectory);

            CPPUNIT_ASSERT_NO_THROW(SCXFileSystem::SetAttributes(tmp_path, attr2));

            // Should NOT be able to do stat() on the file anymore
            CPPUNIT_ASSERT_THROW(SCXFileSystem::Stat(tmp_file, &stats), SCXCoreLib::SCXUnauthorizedFileSystemAccessException);

            // Clean up by removing directory and file
            CPPUNIT_ASSERT_NO_THROW(SCXFileSystem::SetAttributes(tmp_path, attr));
            CPPUNIT_ASSERT_NO_THROW(dir.Delete(true));
        }
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXFileSystemTest );
