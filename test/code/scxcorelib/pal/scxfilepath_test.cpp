/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-05-21 07:50:00

    File name test class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilepath.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxhandle.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <scxcorelib/scxexception.h>

class SCXFilePathTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXFilePathTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestConstructor );
    CPPUNIT_TEST( TestSet );
    CPPUNIT_TEST( TestSetFilename );
    CPPUNIT_TEST( TestSetFileSuffix );
    CPPUNIT_TEST( TestSetDirectory );
    CPPUNIT_TEST( TestGetFileSuffix );
    CPPUNIT_TEST( TestAppend );
    CPPUNIT_TEST( TestAppendDirectory );
    CPPUNIT_TEST( TestAssign );
    CPPUNIT_TEST( TestCompare );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXFilePath> m_empty;
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXFilePath> m_file;
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXFilePath> m_directory;
    SCXCoreLib::SCXHandle<SCXCoreLib::SCXFilePath> m_path;

public:
    void setUp(void)
    {
        m_empty = new SCXCoreLib::SCXFilePath();
        m_file = new SCXCoreLib::SCXFilePath(L"file");
        m_directory = new SCXCoreLib::SCXFilePath(L"dir/");
        m_path = new SCXCoreLib::SCXFilePath(L"some/path/file.ext");
    }

    void tearDown(void)
    {
        m_empty = 0;
        m_file = 0;
        m_directory = 0;
        m_path = 0;
    }

    void callDumpStringForCoverage()
    {
        CPPUNIT_ASSERT(m_path->DumpString().find(L"SCXFilePath") != std::wstring::npos);
        CPPUNIT_ASSERT(m_path->DumpString().find(m_path->GetDirectory()) != std::wstring::npos);
        CPPUNIT_ASSERT(m_path->DumpString().find(m_path->GetFilename()) != std::wstring::npos);
    }

    void TestConstructor(void)
    {
        SCXCoreLib::SCXFilePath fp1;
        SCXCoreLib::SCXFilePath fp2(L"/some/path");
        std::wstring sp = L"C:\\some/other\\path/";
        SCXCoreLib::SCXFilePath fp3(sp);
        SCXCoreLib::SCXFilePath fp4(fp2);
        // Check that constructors create a path as expected.
        CPPUNIT_ASSERT(fp1.Get().compare(L"") == 0);
#if defined(WIN32)
        CPPUNIT_ASSERT(fp2.Get() == L"\\some\\path");
        CPPUNIT_ASSERT(fp3.Get() == L"C:\\some\\other\\path\\");
#else
        CPPUNIT_ASSERT(fp2.Get() == L"/some/path");
        CPPUNIT_ASSERT(fp3.Get() == L"C:/some/other/path/");
#endif
        // Check that copy constructor creates identical content.
        CPPUNIT_ASSERT(fp2.Get() == fp4.Get());
    }

    void TestSet(void)
    {
        SCXCoreLib::SCXFilePath fp1, fp2, fp3;
        fp1.Set(L"/some/path");
        fp2.Set(L"file");
        fp3.Set(L"dir/");
        // Check that Set method splits path and file name correctly
        CPPUNIT_ASSERT(fp1.GetFilename() == L"path");
        CPPUNIT_ASSERT(fp2.GetFilename() == L"file");
        CPPUNIT_ASSERT(fp3.GetFilename() == L"");
        CPPUNIT_ASSERT(fp2.GetDirectory() == L"");
#if defined(WIN32)
        CPPUNIT_ASSERT(fp1.GetDirectory() == L"\\some\\");
        CPPUNIT_ASSERT(fp3.GetDirectory() == L"dir\\");
#else
        CPPUNIT_ASSERT(fp1.GetDirectory() == L"/some/");
        CPPUNIT_ASSERT(fp3.GetDirectory() == L"dir/");
#endif
    }

    void TestSetFilename(void)
    {
        // Check that only file name is changed.
        m_path->SetFilename(L"new.file");
#if defined(WIN32)
        CPPUNIT_ASSERT(m_path->Get() == L"some\\path\\new.file");
#else
        CPPUNIT_ASSERT(m_path->Get() == L"some/path/new.file");
#endif
        m_path->SetFilename(L"");
        // Check that empty filename removes filename...
        CPPUNIT_ASSERT(m_path->GetFilename() == L"");
        // ...but not directory
#if defined(WIN32)
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some\\path\\");
#else
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some/path/");
#endif

        // Check correct exception thrown when adding folder separators in file name.
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(m_path->SetFilename(L"not/valid"), SCXCoreLib::SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED(2); // both in append function and in SCXInvalidArgumentException constructor
        CPPUNIT_ASSERT_THROW(m_path->SetFilename(L"not\\valid"), SCXCoreLib::SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED(2); // both in append function and in SCXInvalidArgumentException constructor
    }

    void TestSetFileSuffix(void)
    {
        m_directory->SetFilesuffix(L"new");
        m_file->SetFilesuffix(L"new");
        m_path->SetFilesuffix(L"new");

        // Check no suffix added when file name is empty.
        CPPUNIT_ASSERT(m_directory->GetFilename() == L"");
        // Check file name is changed.
        CPPUNIT_ASSERT(m_file->GetFilename() == L"file.new");
        // Check file suffix is added.
        CPPUNIT_ASSERT(m_path->GetFilename() == L"file.new");
        // Make sure original directories are preserved.
#if defined(WIN32)
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"dir\\");
        CPPUNIT_ASSERT(m_file->GetDirectory() == L"");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some\\path\\");
#else
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"dir/");
        CPPUNIT_ASSERT(m_file->GetDirectory() == L"");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some/path/");
#endif
        // make sure a suffix can be removed (including suffix separator)
        m_file->SetFilesuffix(L"");
        CPPUNIT_ASSERT(m_file->GetFilename() == L"file");
    }

    void TestSetDirectory(void)
    {
        m_directory->SetDirectory(L"new/dir\\");
        m_path->SetDirectory(L"new/dir");
        m_file->SetDirectory(L"new/dir\\");
        // Check directiory is correctly set.
#if defined(WIN32)
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"new\\dir\\");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"new\\dir\\");
        CPPUNIT_ASSERT(m_file->GetDirectory() == L"new\\dir\\");
#else
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"new/dir/");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"new/dir/");
        CPPUNIT_ASSERT(m_file->GetDirectory() == L"new/dir/");
#endif
        // Check file name is not affected
        CPPUNIT_ASSERT(m_directory->GetFilename() == L"");
        CPPUNIT_ASSERT(m_file->GetFilename() == L"file");
        CPPUNIT_ASSERT(m_path->GetFilename() == L"file.ext");
    }

    void TestGetFileSuffix(void)
    {
        // Check for correct existing suffix.
        CPPUNIT_ASSERT(m_path->GetFilesuffix() == L"ext");
        // Check for correct non-existing suffix.
        CPPUNIT_ASSERT(m_file->GetFilesuffix() == L"");
        // Check for correct non-existing suffix when file name is missing.
        CPPUNIT_ASSERT(m_directory->GetFilesuffix() == L"");
    }

    void TestAppend(void)
    {
        m_empty->Append(L"some/append\\path/");
        m_directory->Append(L"some/append\\path/file");
        m_path->Append(L".new.ext");
        // Check correct parsing of folder and file name for empy paths,
        // folder only paths and paths with bor filename and folders.
#if defined(WIN32)
        CPPUNIT_ASSERT(m_empty->GetDirectory() == L"some\\append\\path\\");
        CPPUNIT_ASSERT(m_empty->GetFilename() == L"");
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"dir\\some\\append\\path\\");
        CPPUNIT_ASSERT(m_directory->GetFilename() == L"file");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some\\path\\");
        CPPUNIT_ASSERT(m_path->GetFilename() == L"file.ext.new.ext");
#else
        CPPUNIT_ASSERT(m_empty->GetDirectory() == L"some/append/path/");
        CPPUNIT_ASSERT(m_empty->GetFilename() == L"");
        CPPUNIT_ASSERT(m_directory->GetDirectory() == L"dir/some/append/path/");
        CPPUNIT_ASSERT(m_directory->GetFilename() == L"file");
        CPPUNIT_ASSERT(m_path->GetDirectory() == L"some/path/");
        CPPUNIT_ASSERT(m_path->GetFilename() == L"file.ext.new.ext");
#endif

        // Check for correct exception when folder separators in append string and path 
        // already has a file name.
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(m_path->Append(L"path/with\\file"), SCXCoreLib::SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED(2); // both in append function and in SCXInvalidArgumentException constructor

        m_empty->Append(L"/double/");
        // Check that any starting folder separators are removed if appending to existing folder.
#if defined(WIN32)
        CPPUNIT_ASSERT(m_empty->GetDirectory().find(L"\\\\") == std::wstring::npos);
#else
        CPPUNIT_ASSERT(m_empty->GetDirectory().find(L"//") == std::wstring::npos);
#endif
    }

    void TestAppendDirectory(void)
    {
        // Test appending to empty folder.
        m_file->AppendDirectory(L"/some\\append/");
        // Test appending folder without trailing folder separator.
        m_directory->AppendDirectory(L"some_append");
        // Test removing starting folder separators and adding trailing folder separator.
        m_path->AppendDirectory(L"/some/append");
#if defined(WIN32)
        CPPUNIT_ASSERT(m_file->Get() == L"\\some\\append\\file");
        CPPUNIT_ASSERT(m_directory->Get() == L"dir\\some_append\\");
        CPPUNIT_ASSERT(m_path->Get() == L"some\\path\\some\\append\\file.ext");
#else
        CPPUNIT_ASSERT(m_file->Get() == L"/some/append/file");
        CPPUNIT_ASSERT(m_directory->Get() == L"dir/some_append/");
        CPPUNIT_ASSERT(m_path->Get() == L"some/path/some/append/file.ext");
#endif
    }

    void TestAssign(void)
    {
        SCXCoreLib::SCXFilePath fp = *m_path;

        // Check may assign to itself.
        CPPUNIT_ASSERT_NO_THROW(*m_path = *m_path);
        // Check content after assign operation is equal.
        CPPUNIT_ASSERT(fp.Get() == m_path->Get());
    }

    void TestCompare(void)
    {
        SCXCoreLib::SCXFilePath fp1 = *m_path;
        SCXCoreLib::SCXFilePath fp2 = m_path->Get();
        SCXCoreLib::SCXFilePath fp3 = *m_directory;
        SCXCoreLib::SCXFilePath fp4 = *m_file;

        // Check compare operators to match content.
        CPPUNIT_ASSERT(fp1 == fp2);
        CPPUNIT_ASSERT(fp3 == *m_directory);
        CPPUNIT_ASSERT(fp4 == *m_file);
        CPPUNIT_ASSERT(fp1 != fp3);
        CPPUNIT_ASSERT(fp1 != fp4);
        CPPUNIT_ASSERT(fp3 != fp4);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXFilePathTest );
