/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-08-28 16:00:56

  File directory listing test class.

*/
/*----------------------------------------------------------------------------*/
#include <testutils/scxunit.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxexception.h>
#include <testutils/scxunit.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <errno.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <vector>

// On windows CreateDirectory is defined to CreateDirectoryW.
#if defined(WIN32)
#undef CreateDirectory
#endif

using std::vector;
using std::string;
using SCXCoreLib::SCXFilePath;
using SCXCoreLib::SCXDirectory;
using SCXCoreLib::SCXDirectoryInfo;
using SCXCoreLib::SCXFileSystem;
using SCXCoreLib::SCXFile;
using SCXCoreLib::SCXUnauthorizedFileSystemAccessException;

const bool s_bDebugOutput = false;
const bool s_bDebugDetailed = false;

class SCXDirectoryInfoTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXDirectoryInfoTest );
#if !defined(DISABLE_WIN_UNSUPPORTED)
    CPPUNIT_TEST( TestListDirAll );
#endif
    CPPUNIT_TEST( TestNonexistentDir );
    // Retired test: CPPUNIT_TEST( TestReadprotectedDir );
    CPPUNIT_TEST( TestNondirDir );
#if defined(WIN32)
    CPPUNIT_TEST( TestAlternateListFilesCommandForFiles );
    CPPUNIT_TEST( TestAlternateListFilesCommandForDirectories );
#endif
    CPPUNIT_TEST( TestListRegularFilesDeterministic_ZeroFiles );
    CPPUNIT_TEST( TestListRegularFilesDeterministic_OneFile );
    CPPUNIT_TEST( TestListRegularFilesDeterministic_FourFiles );
    CPPUNIT_TEST( TestListRegularFilesNondeterministic );
    CPPUNIT_TEST( TestListDirectories );
    CPPUNIT_TEST( TestListSystemFiles );
    CPPUNIT_TEST( TestAttributes );
    CPPUNIT_TEST( TestStatBasedProperties );
    CPPUNIT_TEST( TestRefresh );
    CPPUNIT_TEST( TestDeleteEmptyDirectory );
    CPPUNIT_TEST( TestDeleteTree );
    CPPUNIT_TEST( TestMove );
    CPPUNIT_TEST( TestCreate );
#if !defined(DISABLE_WIN_UNSUPPORTED)
    CPPUNIT_TEST( TestCreateTempDir );
#endif
#if defined(WIN32)
    SCXUNIT_TEST_ATTRIBUTE(TestAlternateListFilesCommandForFiles, SLOW );
    SCXUNIT_TEST_ATTRIBUTE(TestAlternateListFilesCommandForDirectories, SLOW );
#endif
    SCXUNIT_TEST_ATTRIBUTE(TestListDirAll, SLOW );
    SCXUNIT_TEST_ATTRIBUTE(TestListRegularFilesDeterministic_ZeroFiles, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestListRegularFilesDeterministic_OneFile, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestListRegularFilesDeterministic_FourFiles, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestListRegularFilesNondeterministic, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestListDirectories, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestListSystemFiles, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestRefresh, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestDeleteEmptyDirectory, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestDeleteTree, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestMove, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestCreate, SLOW);
    CPPUNIT_TEST_SUITE_END();

    public:
    void setUp(void)
    {
        FILE* fp = fopen("atestfile.txt", "wb");
        fclose(fp);
    }

    void tearDown(void)
    {
        unlink("atestfile.txt");

        RemoveFauxDirectoryStructure();
    }

    /**
       List all files in directory with ls and compare contents and order
    */
    void TestListDirAll(void)
    {
#if defined(WIN32)
        SCXCoreLib::SCXFilePath fp(this->GetRootDir()); // Root path
#else
        SCXCoreLib::SCXFilePath fp(L"/"); // Root path
#endif
        vector<SCXCoreLib::SCXFilePath> files;

        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));

        CPPUNIT_ASSERT_NO_THROW(files = SCXCoreLib::SCXDirectory::GetFileSystemEntries(fp));

        /* We can't have an empty root dir */
        CPPUNIT_ASSERT(files.size() > 0);

        /* read the same with ls / instead */
        vector<SCXCoreLib::SCXFilePath> testDir;
        std::wstring wideRootDir = GetRootDir();
        string narrowRootDir = SCXCoreLib::StrToUTF8(wideRootDir);
#ifdef WIN32
        CPPUNIT_ASSERT(true == listDirectoryWithDir(testDir, narrowRootDir.c_str(), 'a'));  // DIR /A:DHS
#else
        CPPUNIT_ASSERT(true == listDirectoryWithls(testDir, narrowRootDir.c_str(), 'a'));
#endif
        /* Compare contents. */
        std::sort(files.begin(), files.end(), onPath);
        std::sort(testDir.begin(), testDir.end(), onPath);
        vector<SCXCoreLib::SCXFilePath>::const_iterator pos;
        vector<SCXCoreLib::SCXFilePath>::const_iterator pos2;
        for(pos = testDir.begin(), pos2 = files.begin(); pos != testDir.end(); ++pos, ++pos2) {
            /* Test that each and every name are equal in the OS-native encoding. */
            //std::wcout << L"** checking >" << pos->Get() << L"< == >" << pos2->Get() << L"<" << std::endl;
            CPPUNIT_ASSERT_EQUAL(SCXCoreLib::StrToUTF8(pos2->Get()),
                                 SCXCoreLib::StrToUTF8(pos->Get()));
        }

        /* All elements should have been consumed */
        CPPUNIT_ASSERT(pos2 == files.end());

        /* verify info result matches the path result. */
        SCXCoreLib::SCXDirectoryInfo dir(fp);
        vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> > fileInfos;
        CPPUNIT_ASSERT_NO_THROW(fileInfos = dir.GetFileSystemInfos());
        for (vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> >::const_iterator it = fileInfos.begin();
             it != fileInfos.end(); ++it)
        {
            CPPUNIT_ASSERT(RemoveFilePath(files, (*it)->GetFullPath()));
        }
        CPPUNIT_ASSERT(files.empty());
    }

    /**
       When we try to list the non-existent directory there should be an exception.
    */
    void TestNonexistentDir(void)
    {
#ifdef WIN32
        SCXCoreLib::SCXFilePath fp(L"c:\\we\\can\\be\\certain\\that\\this\\one\\is\\nonexistent\\");
#else
        SCXCoreLib::SCXFilePath fp(L"/we/can/be/certain/that/this/one/is/nonexistent/");
#endif
        CPPUNIT_ASSERT( ! SCXCoreLib::SCXDirectory::Exists(fp));
        //SCXCoreLib::SCXDirectoryInfo dir(fp);

        vector<SCXCoreLib::SCXFilePath> files;
        CPPUNIT_ASSERT_THROW(files = SCXCoreLib::SCXDirectory::GetFileSystemEntries(fp), SCXCoreLib::SCXFilePathNotFoundException);
    }

    /** Try to read a protected directory
        [This test is curently disabled since the nightly builds run as root and
        root can always read all files irrepectively of their protection. Will
        reinstate this test if that changes.]
    */
#if 0
    void TestReadprotectedDir(void)
    {

        // The following file is read-protected for non-root users on Suse
        SCXCoreLib::SCXFilePath fp(L"/etc/uucp/");
        SCXCoreLib::SCXDirectoryInfo dir(fp);
        vector<SCXCoreLib::SCXFilePath> files;
        CPPUNIT_ASSERT_THROW(dir.GetFiles(files, true, true, false), SCXCoreLib::SCXInvalidArgumentException);
    }
#endif

    /** Try to read a regular file as if it was a directory */
    void TestNondirDir(void)
    {
        SCXCoreLib::SCXFilePath fp;

        fp.SetDirectory(L"atestfile.txt");

        vector<SCXCoreLib::SCXFilePath> files;
        CPPUNIT_ASSERT_THROW(files = SCXCoreLib::SCXDirectory::GetFileSystemEntries(fp), SCXCoreLib::SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

#if defined(WIN32)
    /**
    * Verification of unit-test only code for scraping the output of DIR. This test
    * focuses specifically on verifying output that are files.
    */
    void TestAlternateListFilesCommandForFiles()
    {
        // American-style dates
        TestAlternateListFilesSuccessCommandHelper(L"07/08/2008  03:40 PM                 0 AUTOEXEC.BAT", L"AUTOEXEC.BAT", false);
        TestAlternateListFilesSuccessCommandHelper(L"12/08/2008  01:55 AM                 0 CONFIG.SYS", L"CONFIG.SYS", false);
        TestAlternateListFilesSuccessCommandHelper(L"12/08/2008  12:41 PM                24 DriveCheck.txt", L"DriveCheck.txt", false);
        TestAlternateListFilesSuccessCommandHelper(L"06/22/2009  11:01 PM             1,896 Inconfig.log", L"Inconfig.log", false);
        TestAlternateListFilesSuccessCommandHelper(L"12/09/2008  05:49 PM               634 installtime.log", L"installtime.log", false);

        // ISO-style dates
        TestAlternateListFilesSuccessCommandHelper(L"2008-07-08  03:40 PM                 0 AUTOEXEC.BAT", L"AUTOEXEC.BAT", false);
        TestAlternateListFilesSuccessCommandHelper(L"2008-12-08  01:55 AM                 0 CONFIG.SYS", L"CONFIG.SYS", false);
        TestAlternateListFilesSuccessCommandHelper(L"2008-12-08  12:41 PM                24 DriveCheck.txt", L"DriveCheck.txt", false);
        TestAlternateListFilesSuccessCommandHelper(L"2009-06-22  11:01 PM             1,896 Inconfig.log", L"Inconfig.log", false);
        TestAlternateListFilesSuccessCommandHelper(L"2008-12-08  05:49 PM               634 installtime.log", L"installtime.log", false);
    }

    /**
    * Verification of unit-test only code for scraping the output of DIR. This test
    * focuses specifically on verifying output that are directories.
    */
    void TestAlternateListFilesCommandForDirectories()
    {
        // American-style dates
        TestAlternateListFilesSuccessCommandHelper(L"02/05/2009  02:01 PM    <DIR>          6b507d1f6a0e351ea1ee2f70", L"6b507d1f6a0e351ea1ee2f70", true);
        TestAlternateListFilesSuccessCommandHelper(L"10/02/2009  11:07 AM    <DIR>          devel", L"devel", true);
        TestAlternateListFilesSuccessCommandHelper(L"06/10/2009  03:07 PM    <DIR>          Documents and Settings", L"Documents and Settings", true);
        TestAlternateListFilesSuccessCommandHelper(L"07/09/2008  01:24 PM    <DIR>          Inetpub", L"Inetpub", true);
        TestAlternateListFilesSuccessCommandHelper(L"07/08/2008  04:13 PM    <DIR>          MSOCache", L"MSOCache", true);
        TestAlternateListFilesSuccessCommandHelper(L"10/01/2009  03:18 PM    <DIR>          OpsMgr", L"OpsMgr", true);
        TestAlternateListFilesSuccessCommandHelper(L"09/22/2009  02:41 PM    <DIR>          Program Files", L"Program Files", true);
        TestAlternateListFilesSuccessCommandHelper(L"06/17/2009  04:00 PM    <DIR>          TFSCHECK", L"TFSCHECK", true);
        TestAlternateListFilesSuccessCommandHelper(L"03/17/2009  11:16 AM    <DIR>          Visual Studio 8", L"Visual Studio 8", true);
        TestAlternateListFilesSuccessCommandHelper(L"09/26/2009  04:01 AM    <DIR>          WINDOWS", L"WINDOWS", true);
        TestAlternateListFilesSuccessCommandHelper(L"07/27/2009  11:01 AM    <DIR>          x", L"x", true);

        // ISO-style dates
        TestAlternateListFilesSuccessCommandHelper(L"2009-02-05  02:01 PM    <DIR>          6b507d1f6a0e351ea1ee2f70", L"6b507d1f6a0e351ea1ee2f70", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-10-02  11:07 AM    <DIR>          devel", L"devel", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-06-10  03:07 PM    <DIR>          Documents and Settings", L"Documents and Settings", true);
        TestAlternateListFilesSuccessCommandHelper(L"2008-07-09  01:24 PM    <DIR>          Inetpub", L"Inetpub", true);
        TestAlternateListFilesSuccessCommandHelper(L"2008-07-08  04:13 PM    <DIR>          MSOCache", L"MSOCache", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-10-01  03:18 PM    <DIR>          OpsMgr", L"OpsMgr", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-09-22  02:41 PM    <DIR>          Program Files", L"Program Files", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-06-17  04:00 PM    <DIR>          TFSCHECK", L"TFSCHECK", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-03-17  11:16 AM    <DIR>          Visual Studio 8", L"Visual Studio 8", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-09-26  04:01 AM    <DIR>          WINDOWS", L"WINDOWS", true);
        TestAlternateListFilesSuccessCommandHelper(L"2009-07-27  11:01 AM    <DIR>          x", L"x", true);
    }

    /**
    * Parameterized Helper Utility for verifying the test-only logic of parsing the output of DIR.
    * This method is for lines that should be successfully parsed.
    */
    void TestAlternateListFilesSuccessCommandHelper(std::wstring input, std::wstring expectedName, bool expectedIsDir)
    {
        std::wstring output(L"");
        bool result = ParseDirLine(input, output);

        std::wstringstream warnMsgBool;
        warnMsgBool << L"Did not determine is file versus directory properly for '";
        warnMsgBool << input << L"'";
        SCXUNIT_ASSERT_MESSAGEW(warnMsgBool.str(), result == expectedIsDir);

        std::wstringstream warnMsgFilename;
        warnMsgFilename << L"Filename improperly parsed for " << expectedName;
        warnMsgFilename << L" from '" << input << L"'";
        SCXUNIT_ASSERT_MESSAGEW(warnMsgFilename.str(), output == expectedName);
    }

#endif

    /**
     * Verify that edge case that no files are returned when aimed at an empty directory.
     */
    void TestListRegularFilesDeterministic_ZeroFiles(void)
    {
        // (1) Setup
        std::wstring wideDirName = CreateFauxDirectoryStructure();
        string narrowDirName = SCXCoreLib::StrToUTF8(wideDirName);
        SCXCoreLib::SCXFilePath fp(wideDirName);
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));
        vector<SCXCoreLib::SCXFilePath> fpa;

        // (2) Run
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetFiles(fp));

        // (3) Verify
        CPPUNIT_ASSERT(fpa.size() == 0);
    }

    /**
     * Verify listing of files correctly returns one file
     */
    void TestListRegularFilesDeterministic_OneFile(void)
    {
        // (1) Setup
        std::wstring wideDirName = CreateFauxDirectoryStructure();
        wideDirName.append(L"dirmove").append(1, SCXCoreLib::SCXFilePath::GetFolderSeparator());
        string narrowDirName = SCXCoreLib::StrToUTF8(wideDirName);
        SCXCoreLib::SCXFilePath fp(wideDirName);
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));
        vector<SCXCoreLib::SCXFilePath> fpa;

        // (2) Run
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetFiles(fp));

        // (3) Verify
        CPPUNIT_ASSERT(fpa.size() == 1);
        CPPUNIT_ASSERT(fpa[0].GetFilename() == L"hej.txt");
    }

    /**
     * Verify listing of files correctly returns four file
     */
    void TestListRegularFilesDeterministic_FourFiles(void)
    {
        // (1) Setup
        std::wstring wideDirName = CreateFauxDirectoryStructure();
        wideDirName.append(L"dirmove").append(1, SCXCoreLib::SCXFilePath::GetFolderSeparator());
        wideDirName.append(L"A").append(1, SCXCoreLib::SCXFilePath::GetFolderSeparator());
        string narrowDirName = SCXCoreLib::StrToUTF8(wideDirName);
        SCXCoreLib::SCXFilePath fp(wideDirName);
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));
        vector<SCXCoreLib::SCXFilePath> fpa;

        // (2) Run
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetFiles(fp));

        // (3) Verify
        CPPUNIT_ASSERT(fpa.size() == 4);

        // We have no guarantee of the order in which files are returned. So we're going
        // to build a list of files that should be the same.
        vector<SCXCoreLib::SCXFilePath> expectedFilenames;
        SCXCoreLib::SCXFilePath answer;
        answer.SetDirectory(fpa[0].GetDirectory());
        answer.SetFilename(L"hej.txt");
        expectedFilenames.push_back(answer);

        answer.SetFilename(L"hej hej.txt");
        expectedFilenames.push_back(answer);

        answer.SetFilename(L"1.txt");
        expectedFilenames.push_back(answer);

        answer.SetFilename(L"1");
        expectedFilenames.push_back(answer);


        VerifyListOfFilesAreIdentical(fp, fpa, expectedFilenames);
    }

    /** Read all regular files in a directory (that has symlinks and subdirectories) */
    void TestListRegularFilesNondeterministic(void)
    {
#ifdef WIN32
        std::wstring wideRootDir = GetRootDir();
        SCXCoreLib::SCXFilePath fp(wideRootDir);
#else
        SCXCoreLib::SCXFilePath fp(L"/etc/");
#endif
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));

        vector<SCXCoreLib::SCXFilePath> fpa;
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetFiles(fp));

        vector<SCXCoreLib::SCXFilePath> fpb;
#ifdef WIN32
        string narrowRootDir = SCXCoreLib::StrToUTF8(wideRootDir);
        CPPUNIT_ASSERT(true == listDirectoryWithDir(fpb, narrowRootDir.c_str(), 'f'));  // DIR /A:-S-D
#else
        CPPUNIT_ASSERT(true == listDirectoryWithls(fpb, "/etc/", 'f'));
#endif

        VerifyListOfFilesAreIdentical(fp, fpa, fpb);
    }

    /**
     * Test Helper Utility method for verifying that two vectors of File objects are identical.
     * Right now this has no return value, it asserts in the helper utility.
     *
     * @arg fp: file path to the directory in question
     * @arg actualList: list of actual files returned
     * @arg expectedList: list of expected files that should be returned
     */
    void VerifyListOfFilesAreIdentical(SCXCoreLib::SCXFilePath fp,
            vector<SCXCoreLib::SCXFilePath> actualList,
            vector<SCXCoreLib::SCXFilePath> expectedList)
    {
        std::sort(actualList.begin(), actualList.end(), onPath);
        std::sort(expectedList.begin(), expectedList.end(), onPath);

        // Generate list of names that are in one list but not the other
        vector<SCXCoreLib::SCXFilePath> fpc;
        std::set_symmetric_difference(actualList.begin(), actualList.end(),
                expectedList.begin(), expectedList.end(),
                                      back_inserter(fpc), onPath);

        std::wstringstream assertMessage;
        assertMessage << std::endl;
        vector<SCXCoreLib::SCXFilePath>::const_iterator posa;
        for(posa = actualList.begin(); posa != actualList.end(); ++posa) {
            assertMessage << L"A>" << (*posa).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posb;
        for(posb = expectedList.begin(); posb != expectedList.end(); ++posb) {
            assertMessage << L"B>" << (*posb).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posc;
        for(posc = fpc.begin(); posc != fpc.end(); ++posc) {
            assertMessage << L"C>" << (*posc).Get() << L"<" << std::endl;
        }

        // There shouldn't be any files in common.
        SCXUNIT_ASSERT_MESSAGEW(assertMessage.str(),fpc.empty() == true);

        /* verify info result matches the path result. */
        SCXCoreLib::SCXDirectoryInfo dir(fp);
        vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > fileInfos;
        CPPUNIT_ASSERT_NO_THROW(fileInfos = dir.GetFiles());
        for (vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> >::const_iterator it = fileInfos.begin();
             it != fileInfos.end(); ++it)
        {
            CPPUNIT_ASSERT(0 == (*it)->GetAttributes().count(SCXFileSystem::eDirectory));
            CPPUNIT_ASSERT(RemoveFilePath(actualList, (*it)->GetFullPath()));
        }
        CPPUNIT_ASSERT(actualList.empty());

    }

    /** Read all directories files in a directory (that has symlinks and regular files) */
    void TestListDirectories(void)
    {
#ifdef WIN32
        std::wstring wideRootDir = GetRootDir();
        SCXCoreLib::SCXFilePath fp(wideRootDir);
#else
        SCXCoreLib::SCXFilePath fp(L"/etc/");
#endif
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));

        vector<SCXCoreLib::SCXFilePath> fpa;
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetDirectories(fp));
        std::sort(fpa.begin(), fpa.end(), onPath);

        vector<SCXCoreLib::SCXFilePath> fpb;
#ifdef WIN32
        string narrowRootDir = SCXCoreLib::StrToUTF8(wideRootDir);
        CPPUNIT_ASSERT(true == listDirectoryWithDir(fpb, narrowRootDir.c_str(), 'd'));
#else
        CPPUNIT_ASSERT(true == listDirectoryWithls(fpb, "/etc/", 'd'));
#endif
        std::sort(fpb.begin(), fpb.end(), onPath);

        // Generate list of names that are in one list but not the other
        vector<SCXCoreLib::SCXFilePath> fpc;
        std::set_symmetric_difference(fpa.begin(), fpa.end(), fpb.begin(), fpb.end(),
                                      back_inserter(fpc), onPath);

        std::wstringstream assertMessage;
        assertMessage << std::endl;
        vector<SCXCoreLib::SCXFilePath>::const_iterator posa;
        for(posa = fpa.begin(); posa != fpa.end(); ++posa) {
            assertMessage << L"A>" << (*posa).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posb;
        for(posb = fpb.begin(); posb != fpb.end(); ++posb) {
            assertMessage << L"B>" << (*posb).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posc;
        for(posc = fpc.begin(); posc != fpc.end(); ++posc) {
            assertMessage << L"C>" << (*posc).Get() << L"<" << std::endl;
        }

        // There shouldn't be any files in common.
        SCXUNIT_ASSERT_MESSAGEW(assertMessage.str(), fpc.empty() == true);

        /* verify info result matches the path result. */
        SCXCoreLib::SCXDirectoryInfo dir(fp);
        vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> > fileInfos;
        CPPUNIT_ASSERT_NO_THROW(fileInfos = dir.GetDirectories());
        for (vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> >::const_iterator it = fileInfos.begin();
             it != fileInfos.end(); ++it)
        {
            CPPUNIT_ASSERT((*it)->IsDirectory());
            CPPUNIT_ASSERT(RemoveFilePath(fpa, (*it)->GetFullPath()));
        }
        CPPUNIT_ASSERT(fpa.empty());
    }

    void TestListSystemFiles(void)
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if (0 != geteuid())
        {
            SCXUNIT_WARNING(L"Platform needs privileges to run TestListSystemFiles test");
            return;
        }
#endif

#ifdef WIN32
        std::wstring wideRootDir = GetRootDir();
        SCXCoreLib::SCXFilePath fp(wideRootDir);
#else
        SCXCoreLib::SCXFilePath fp(L"/dev/");
#endif
        CPPUNIT_ASSERT(SCXCoreLib::SCXDirectory::Exists(fp));

        vector<SCXCoreLib::SCXFilePath> fpa;
        CPPUNIT_ASSERT_NO_THROW(fpa = SCXCoreLib::SCXDirectory::GetSysFiles(fp));
        std::sort(fpa.begin(), fpa.end(), onPath);

        vector<SCXCoreLib::SCXFilePath> fpb;
#ifdef WIN32
        string narrowRootDir = SCXCoreLib::StrToUTF8(wideRootDir);
        CPPUNIT_ASSERT(true == listDirectoryWithDir(fpb, narrowRootDir.c_str(), 's'));
#else
        CPPUNIT_ASSERT(true == listDirectoryWithls(fpb, "/dev/", 's'));
#endif
        std::sort(fpb.begin(), fpb.end(), onPath);

        // Generate list of names that are in one list but not the other
        vector<SCXCoreLib::SCXFilePath> fpc;
        std::set_symmetric_difference(fpa.begin(), fpa.end(), fpb.begin(), fpb.end(),
                                      back_inserter(fpc), onPath);

        std::wstringstream assertMessage;
        assertMessage << std::endl;
        vector<SCXCoreLib::SCXFilePath>::const_iterator posa;
        for(posa = fpa.begin(); posa != fpa.end(); ++posa) {
            assertMessage << L"A>" << (*posa).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posb;
        for(posb = fpb.begin(); posb != fpb.end(); ++posb) {
            assertMessage << L"B>" << (*posb).Get() << L"<" << std::endl;
        }

        vector<SCXCoreLib::SCXFilePath>::const_iterator posc;
        for(posc = fpc.begin(); posc != fpc.end(); ++posc) {
            assertMessage << L"C>" << (*posc).Get() << L"<" << std::endl;
        }

        // There shouldn't be any files in common.
        SCXUNIT_ASSERT_MESSAGEW(assertMessage.str(), fpc.empty() == true);

        /* verify info result matches the path result. */
        /* On Linux, this can be very slow - '/dev' has 7400+ files */
        SCXCoreLib::SCXDirectoryInfo dir(fp);
        vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > fileInfos;
        CPPUNIT_ASSERT_NO_THROW(fileInfos = dir.GetSysFiles());
        for (vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> >::const_iterator it = fileInfos.begin();
             it != fileInfos.end(); ++it)
        {
            CPPUNIT_ASSERT(0 == (*it)->GetAttributes().count(SCXFileSystem::eDirectory));
            CPPUNIT_ASSERT(RemoveFilePath(fpa, (*it)->GetFullPath()));
        }
        CPPUNIT_ASSERT(fpa.empty());
    }

    void TestAttributes()
    {
        CPPUNIT_ASSERT( system("mkdir TestAttributes") >= 0 );
        SCXCoreLib::SCXDirectoryInfo dir(L"TestAttributes/");
        CPPUNIT_ASSERT(dir.PathExists());

        CPPUNIT_ASSERT(1 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eDirectory));
        CPPUNIT_ASSERT(1 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eReadable));
        CPPUNIT_ASSERT(1 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eWritable));

        dir.Delete();
        CPPUNIT_ASSERT( ! dir.PathExists());
    }

    void TestStatBasedProperties()
    {
        // Wrap a for-loop around this routine to help diagnose timing issues.  We only run
        // once normally, but the framework is left in place in case of future debugging.

        for (int i = 1; i <= 1; i++)
        {
            SCXCoreLib::SCXCalendarTime timeBefore = SCXCoreLib::SCXCalendarTime::CurrentUTC();
            timeBefore.SetDecimalCount(0); // Since create time sometimes only holds seconds.

            std::wstring deployDir = GetDeploymentDirectory().Get();
            std::stringstream createDirectoryStructure;
            createDirectoryStructure << "mkdir " << SCXCoreLib::StrToUTF8(deployDir);
            CPPUNIT_ASSERT_EQUAL(0, system(createDirectoryStructure.str().c_str()));

            SCXCoreLib::SCXDirectoryInfo dir(deployDir);
            CPPUNIT_ASSERT(dir.PathExists());

            SCXCoreLib::SCXCalendarTime timeAfter = SCXCoreLib::SCXCalendarTime::CurrentUTC();

            if ( s_bDebugOutput )
            {
                std::wcout << std::endl << L"Loop count: " << i;
                std::wcout << std::endl << timeBefore.ToBasicISO8601() << std::endl;
                std::wcout << dir.GetLastAccessTimeUTC().ToBasicISO8601() << std::endl;
                std::wcout << timeAfter.ToBasicISO8601() << std::endl;
            }

#if !defined(DISABLE_WIN_UNSUPPORTED) // Not spending any time on verifying these at the moment. See WI#7400 for details.
#if defined(SCX_UNIX)
            // WI 10361: On UNIX (HP & Redhat), the file will sometimes be created
            // one second prior to the actual system time (to replicate, I put a
            // for-loop around this entire routine running it 10000 times).
            //
            // In this case, the debug output above will show the following lines:
            //   20090126222701Z
            //   20090126222700,000000Z
            //   20090126222701,005Z
            //
            // or sometimes:
            //   20090127101011Z
            //   20090127101011,665334Z
            //   20090127101011,665Z
            //
            // To work around this, we have a timeFudge that we apply to the times.
            // This time fudge appears to resolve the problem.

            SCXCoreLib::SCXRelativeTime timeFudge(0, 0, 0, 0, 0, 1.0);
            timeBefore -= timeFudge;
            timeAfter += timeFudge;
#endif // defined(SCX_UNIX)

            SCXUNIT_ASSERT_BETWEEN(dir.GetLastAccessTimeUTC(), timeBefore, timeAfter);
            SCXUNIT_ASSERT_BETWEEN(dir.GetLastModificationTimeUTC(), timeBefore, timeAfter);
            SCXUNIT_ASSERT_BETWEEN(dir.GetLastStatusChangeTimeUTC(), timeBefore, timeAfter);
#endif // !defined(DISABLE_WIN_UNSUPPORTED)

            SCXCoreLib::SCXFileSystem::SCXStatStruct statData;
            CPPUNIT_ASSERT_NO_THROW(SCXCoreLib::SCXFileSystem::Stat(dir.GetFullPath(), &statData));

            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_nlink), dir.GetLinkCount());
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_size), dir.GetSize());
#if defined(SCX_UNIX)
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_blksize), dir.GetBlockSize());
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_blocks), dir.GetBlockCount());
#endif
            CPPUNIT_ASSERT_EQUAL(statData.st_uid, dir.GetUserID());
            CPPUNIT_ASSERT_EQUAL(statData.st_gid, dir.GetGroupID());
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_dev), dir.GetDevice());
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_rdev), dir.GetDeviceNumber());
            CPPUNIT_ASSERT_EQUAL(static_cast<scxulong>(statData.st_ino), dir.GetSerialNumber());
#if defined(SCX_UNIX)
            CPPUNIT_ASSERT((statData.st_mode&S_IRUSR)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eUserRead));
            CPPUNIT_ASSERT((statData.st_mode&S_IWUSR)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eUserWrite));
            CPPUNIT_ASSERT((statData.st_mode&S_IXUSR)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eUserExecute));
            CPPUNIT_ASSERT((statData.st_mode&S_IRGRP)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eGroupRead));
            CPPUNIT_ASSERT((statData.st_mode&S_IWGRP)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eGroupWrite));
            CPPUNIT_ASSERT((statData.st_mode&S_IXGRP)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eGroupExecute));
            CPPUNIT_ASSERT((statData.st_mode&S_IROTH)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eOtherRead));
            CPPUNIT_ASSERT((statData.st_mode&S_IWOTH)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eOtherWrite));
            CPPUNIT_ASSERT((statData.st_mode&S_IXOTH)?1:0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eOtherExecute));
#endif

            dir.Delete();
            CPPUNIT_ASSERT( ! dir.PathExists());
        }
    }

    void TestRefresh()
    {
        CPPUNIT_ASSERT( system("mkdir TestRefresh") >= 0 );
        SCXCoreLib::SCXDirectoryInfo dir(L"TestRefresh/");

        CPPUNIT_ASSERT(dir.PathExists());
        CPPUNIT_ASSERT(1 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eDirectory));

#ifdef WIN32
        system("rd TestRefresh");
#else
        CPPUNIT_ASSERT( system("rm -r TestRefresh") >= 0 );
#endif

        CPPUNIT_ASSERT(dir.PathExists()); // Using cached info.
        CPPUNIT_ASSERT(1 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eDirectory)); // Using cached info

        dir.Refresh();

        CPPUNIT_ASSERT( ! dir.PathExists());
        CPPUNIT_ASSERT(0 == dir.GetAttributes().count(SCXCoreLib::SCXFileSystem::eDirectory));
    }

    void TestDeleteEmptyDirectory()
    {
        CPPUNIT_ASSERT( system("mkdir TestPathExistsAndDelete") >= 0 );
        SCXCoreLib::SCXDirectoryInfo dir(L"TestPathExistsAndDelete/");
        CPPUNIT_ASSERT(dir.PathExists());
        dir.Delete();
        CPPUNIT_ASSERT( ! dir.PathExists());
        vector<SCXCoreLib::SCXFilePath> fpb;
#ifdef WIN32
        CPPUNIT_ASSERT(0 != system("dir TestPathExistsAndDelete"));
#else
        CPPUNIT_ASSERT(0 != system("ls TestPathExistsAndDelete"));
#endif
    }

    void TestDeleteTree()
    {
#ifdef WIN32
        system("rmdir /S /Q recursiveDelete");
        system("mkdir recursiveDelete");
        system("mkdir recursiveDelete\\A");
        system("mkdir recursiveDelete\\A\\B");
        system("mkdir recursiveDelete\\B");
        system("echo hej > recursiveDelete\\hej.txt");
        system("echo hej > recursiveDelete\\A\\hej.txt");
        system("echo hej > recursiveDelete\\A\\B\\hej.txt");
        system("echo hej > recursiveDelete\\B\\hej.txt");
        SCXCoreLib::SCXFilePath path(L"recursiveDelete\\");
#else
        CPPUNIT_ASSERT( system("rm -fR recursiveDelete") >= 0 );
        CPPUNIT_ASSERT( system("mkdir recursiveDelete") >= 0 );
        CPPUNIT_ASSERT( system("mkdir recursiveDelete/A") >= 0 );
        CPPUNIT_ASSERT( system("mkdir recursiveDelete/A/B") >= 0 );
        CPPUNIT_ASSERT( system("mkdir recursiveDelete/B") >= 0 );
        CPPUNIT_ASSERT( system("echo hej > recursiveDelete/hej.txt") >= 0 );
        CPPUNIT_ASSERT( system("echo hej > recursiveDelete/A/hej.txt") >= 0 );
        CPPUNIT_ASSERT( system("echo hej > recursiveDelete/A/B/hej.txt") >= 0 );
        CPPUNIT_ASSERT( system("echo hej > recursiveDelete/B/hej.txt") >= 0 );
        SCXCoreLib::SCXFilePath path(L"recursiveDelete/");
#endif

        SCXUNIT_ASSERT_THROWN_EXCEPTION(SCXCoreLib::SCXDirectory::Delete(L"recursiveDelete", false),
                                        SCXCoreLib::SCXUnauthorizedFileSystemAccessException, L"recursiveDelete");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(SCXCoreLib::SCXDirectory::Delete(L"recursiveDelete"),
                                        SCXCoreLib::SCXUnauthorizedFileSystemAccessException, L"recursiveDelete");
        SCXCoreLib::SCXDirectory::Delete(path, true);
#ifdef WIN32
        CPPUNIT_ASSERT(0 != system("dir TestPathExistsAndDelete"));
#else
        CPPUNIT_ASSERT(0 != system("ls TestPathExistsAndDelete"));
#endif

    }

    void TestMove() {
        // (1) Setup
        std::wstring deployLocation = CreateFauxDirectoryStructure();
        SCXCoreLib::SCXFilePath root(deployLocation);
        root.AppendDirectory(L"dirmove");

        // SCXDirectory::Move should only move directories
        SCXFilePath oldFile(root);
        oldFile.SetFilename(L"hej.txt");
        SCXFilePath newFile(oldFile);
        newFile.SetFilename(L"hej2.txt");
        CPPUNIT_ASSERT(SCXFile::Exists(oldFile));
        CPPUNIT_ASSERT(!SCXFile::Exists(newFile));
        //SCXDirectory::Move(oldFile, newFile);
        CPPUNIT_ASSERT_THROW(SCXDirectory::Move(oldFile, newFile),
                            SCXUnauthorizedFileSystemAccessException);
        CPPUNIT_ASSERT(SCXFile::Exists(oldFile));
        CPPUNIT_ASSERT(!SCXFile::Exists(newFile));

        // Pure rename, same directory
        SCXFilePath oldDir(root);
        oldDir.AppendDirectory(L"B");
        SCXFilePath renamedDir(root);
        renamedDir.AppendDirectory(L"C");
        CPPUNIT_ASSERT(SCXDirectory::Exists(oldDir));
        SCXDirectory::Move(oldDir, renamedDir);
        CPPUNIT_ASSERT(!SCXDirectory::Exists(oldDir));
        CPPUNIT_ASSERT(SCXDirectory::Exists(renamedDir));

        // Shouldn't be possible to move a directory to a file
        CPPUNIT_ASSERT_THROW(SCXDirectory::Move(renamedDir, oldFile),
                            SCXUnauthorizedFileSystemAccessException);
        CPPUNIT_ASSERT(SCXDirectory::Exists(renamedDir));

        SCXFilePath parentDir(root);
        parentDir.AppendDirectory(L"A");
        SCXFilePath movedDir(parentDir);
        movedDir.AppendDirectory(L"C");
        SCXDirectory::Move(renamedDir, movedDir);
        CPPUNIT_ASSERT(!SCXDirectory::Exists(renamedDir));
        CPPUNIT_ASSERT(SCXDirectory::Exists(movedDir));
        SCXDirectory::Delete(movedDir, true);

        SCXDirectory::Delete(root, true);
    }

    void TestCreate() {
#ifdef WIN32
        system("rmdir /S /Q testCreate");
#else
        CPPUNIT_ASSERT( system("rm -fR testCreate") >= 0 );
#endif

        // Create on an existing directory.
        SCXDirectoryInfo result = SCXDirectory::CreateDirectory(SCXFilePath(L"./"));
        CPPUNIT_ASSERT(result.PathExists());

        SCXFilePath createPath(L"testCreate/subfolder/");
        SCXDirectoryInfo dirinfo = SCXDirectory::CreateDirectory(createPath);
        CPPUNIT_ASSERT(SCXDirectory::Exists(createPath));
#ifdef WIN32
        system("rmdir /S /Q testCreate");
#else
        CPPUNIT_ASSERT( system("rm -fR testCreate") >= 0 );
#endif
    }

#if !defined(DISABLE_WIN_UNSUPPORTED)
    void TestCreateTempDir() {
        SCXDirectoryInfo result = SCXDirectory::CreateTempDirectory();
        CPPUNIT_ASSERT(result.PathExists());
        result.Delete();
    }
#endif

    /***********************************************************************************/
    /* Utility methods */
    /***********************************************************************************/

    /**
     * Test Utility Method for returning the name of the root
     * directory on this system.
     *
     * Windows users, this may NOT be C:\, although in most cases
     * it is. Its value is the drive upon which the system folder
     * was placed. A conversation with members of the Build team
     * resulted in the recommendation that it not necessarily be
     * assumed that this is a 1-to-1 mapping.
     */
    static std::wstring GetRootDir()
    {
#if defined(WIN32)
        char* envValue = getenv("SYSTEMDRIVE");
        std::string rootNarrow(envValue);
        rootNarrow.append("\\");
        std::wstring rootDir(SCXCoreLib::StrFromUTF8(rootNarrow));
#else
        std::wstring rootDir(L"/");
#endif
        return rootDir;
    }

    /**
     * Test Utility Method for returning the name of the temporary
     * directory available on this system.
     */
    static std::wstring GetTmpDir()
    {
#if defined(WIN32)
        char* envValue = getenv("TMP");
        std::string tmpNarrow(envValue);
        tmpNarrow.append("\\");
        std::wstring rootDir(SCXCoreLib::StrFromUTF8(tmpNarrow));
#else
        std::stringstream tmpNarrow;
        tmpNarrow << "/tmp/scx-" << getenv("USER") << "/";
        std::wstring rootDir(SCXCoreLib::StrFromUTF8(tmpNarrow.str()));
#endif
        return rootDir;
    }

    bool RemoveFilePath(std::vector<SCXCoreLib::SCXFilePath>& paths, const SCXCoreLib::SCXFilePath& path)
    {
        std::vector<SCXCoreLib::SCXFilePath>::iterator it = paths.begin();
        while (paths.end() != it)
        {
            if (it->Get() == path.Get())
            {
                paths.erase(it);
                return true;
            }
            ++it;
        }
        return false;
    }


    /** Sortning criterion for SCXFilePath is the filename */
    //    static bool onName (const SCXCoreLib::SCXFilePath& a, const SCXCoreLib::SCXFilePath& b)
    //    { return a.GetFilename() < b.GetFilename(); }

    /** Sortning criterion for SCXFilePath is the full name and path */
    static bool onPath (const SCXCoreLib::SCXFilePath& a, const SCXCoreLib::SCXFilePath& b)
    { return a.Get() < b.Get(); }

    static bool onInfoFullPath(const SCXCoreLib::SCXFileSystemInfo& a, const SCXCoreLib::SCXFileSystemInfo& b)
    { return a.GetFullPath().Get() < b.GetFullPath().Get(); }

#ifndef WIN32
    /**
       Utility method to list the contents of a directory using the ls command and place
       the result in a string vector.
    */
    static bool listDirectoryWithlsOLD(vector<string>& target, const char *dir)
    {
        const char *cmd = "/bin/ls -f1 ";
        char cmdbuf[100];
        char resultbuf[100];

        if (!dir) { return false; }
        strcpy(cmdbuf, cmd);
        strcat(cmdbuf, dir);

        FILE *lsOutput = popen(cmdbuf, "r");
        if (!lsOutput) { return false; }

        while (fgets(resultbuf, sizeof(resultbuf), lsOutput)) {
            // Ignore . and ..
            if (!strcmp(resultbuf, ".\n") || !strcmp(resultbuf, "..\n")) { continue; }

            string tmp(resultbuf);
            tmp.resize(tmp.size() - 1); // Remove last char, a '\n'.
            target.push_back(tmp);
        }

        int eno = errno;

        pclose(lsOutput);
        return eno == 0;                // Should be zero if ok
    }

    /* Utility function to find the position in a string that comes immediately
       after last date string. We search for "HH:MM", where H and M are digits and
       take the filename position relative to that.
       If not found, return -1; This is for parsing of output from the ls command.
       -rw-r--r--  1 root root       258 1995-02-20 19:12:08.000000000 +0100 ttytype
       drwxr-xr-x  3 root root       104 2006-06-16 18:47:22.000000000 +0200 udev/
    */
    static int positionAfterISODate(const char *in)
    {
#if defined(hpux)
        /* On HPUX I have to cheat badly since there is no way to printout ISO dates.
           Just search for last blank char and hope that there aren't any filenames
           with blanks in them. */
        if (in[1] != 'r' && in[1] != '-') return -1; // Rid of non-entries
        return static_cast<int>(strrchr(in, ' ') - in + 1);
#elif defined(aix)
        /* On AIX the output of the file name is always in the same column,
           provided that the locale and all other options are the same.
           Unfortunately that position is 57 on AIX 5.3, and 58 on AIX 6.1
           and AIX 7.1.
        */
        if (in[1] != 'r' && in[1] != '-') return -1; // Rid of non-entries
#if (PF_MAJOR == 5)
        return 57;
#elif (PF_MAJOR == 6 || PF_MAJOR == 7)
        return 58;
#else
#error Need to determine where ls filename is returned for 'LC_TIME=C /bin/ls -lpLA /'
#endif
#elif defined(sun) && (PF_MINOR < 10)
    /* Older Sparc systems tends to have a blank character in column 53
       and the filename follows immediately. If the size field is very large
       then the blank char and the filename is pushed to the right.
    */
        if (in[1] != 'r' && in[1] != '-') return -1; // Rid of non-entries
        for (int i = 53; in[i]; ++i) {
          if (in[i] == ' ') { return i + 1; }
        }
        return -1;
#elif defined(macos)
        // Exmaple line on mac:
        // -rw-rw-r--@   1 admin       admin         6148 Sep 10 04:52:10 2008 .DS_Store
        // Get position of last : and then add 5 to get past space before year and then
        // search for next space to get filename right after that
        const char *colpos = strrchr(in, ':');
        if (colpos == 0) return -1;             // Not found
        int pos = (colpos - in) + 5;
        for (int i = pos; in[i]; ++i) {
          if (in[i] == ' ') { return i + 1; }
        }
        return -1;
#else
        /* This is the proper implementation for systems with ISO date */
        const char *colpos = strchr(in, ':');
        if (colpos == 0) return -1;             // Not found
        if ((colpos - in) < 2) return -1;       // Found too early in string
        if (!colpos[1] || !colpos[2] || !colpos[3]) return -1; // String end found
        if (!isdigit(colpos[-2]) || !isdigit(colpos[-1]) // Test that only digits surround :
            || !isdigit(colpos[1]) || !isdigit(colpos[2])) return -1;
        if (!isspace(colpos[22])) return -1;    // Position after date should be blank
        return static_cast<int>(colpos - in + 23);
#endif
    }

    /**
       Utility method to list the contents of a directory using the ls command and place
       the results in a SCXDirectoryInfo, that is a vector of SCXFilePath objects, including
       the paths.
    */
    static bool listDirectoryWithls(vector<SCXCoreLib::SCXFilePath>& target,
                                    const char *dir, char type)
    {
        char cmdbuf[1024];
        char resultbuf[1024];

        if (!dir) { return false; }
        // We use the long ISO date output format to get a string that is consistently parsable
        // This is generated with different commands on Linux and Solaris.
        // (ISO output is unavailable on HPUX, AIX and Sparc v8)
#if defined(linux)
    // Older dists has a broken version of 'ls' with non-std behaviour of the -p option
#if (defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)) || (defined(PF_DISTRO_SUSE) && (PF_MAJOR==9))
        strcpy(cmdbuf, "/bin/ls -lLA --time-style=full-iso ");
#else
        strcpy(cmdbuf, "/bin/ls -lpLA --time-style=full-iso ");
#endif
#elif defined(sun)
        // No ISO dates on v8 (and v9)
# if (PF_MINOR < 10)
        strcpy(cmdbuf, "LC_TIME=C /bin/ls -lpLA ");
# else
        strcpy(cmdbuf, "/bin/ls -lpLAE ");
# endif /* PF_MINOR */
#elif defined(hpux)
        strcpy(cmdbuf, "/bin/ls -alpLA ");
#elif defined(aix)
        // We run the command with a locale setting that makes the time field very predicatable
        strcpy(cmdbuf, "LC_TIME=C /bin/ls -lpLA ");
#elif defined(macos)
        strcpy(cmdbuf, "/bin/ls -lpLAT ");
#else
#error "Fill in the proper directory listing command here"
#endif
        strcat(cmdbuf, dir);
        /* The above command gives us something like this
           prw-------   1 root     root           0 2007-09-17 09:21:54.000000000 +0200 initpipe
           Drw-r--r--   1 root     root           0 2007-08-22 01:33:28.953517846 +0200 .syslog_door
           prw-------   1 root     root           0 2007-09-17 14:30:32.000000000 +0200 utmppipe
           -rw-r--r--   1 root     sys          424 2007-07-05 15:19:13.644785000 +0200 vfstab
        */

        // Redirect stderr since misdirected symlinks produce output that is mistaken for error
        strcat(cmdbuf, " 2>/dev/null");

#if defined(macos)
        const char* grepcmd = " | /usr/bin/egrep ";
#else
        const char* grepcmd = " | /bin/egrep ";
#endif

        switch (type) {
        case 'a':
            /* add nothing */
            break;
        case 'd':
            /* Sort out those with 'd' in first column */
            strcat(cmdbuf, grepcmd);
            strcat(cmdbuf, "\\^d");
            break;
        case 'f':
            /* Sort out those with '-' in first column */
            strcat(cmdbuf, grepcmd);
            strcat(cmdbuf, "\\^-");
            break;
        case 's':
            /* Sort out those with neither 'd' nor '-' in first column */
            strcat(cmdbuf, grepcmd);
            strcat(cmdbuf, "-v \\^\\(d\\|-\\)");
            break;
        default:
            return false;
        }

        FILE *findOutput = popen(cmdbuf, "r");
        if (!findOutput) { return false; }

        while (fgets(resultbuf, sizeof(resultbuf), findOutput)) {
#if (defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)) || (defined(PF_DISTRO_SUSE) && (PF_MAJOR==9))
            bool isdir = (resultbuf[0] == 'd');
#endif
            int p = positionAfterISODate(resultbuf);
            if (p < 0) continue; // Shouldn't happen
            string tmp(resultbuf + p);
            size_t s = tmp.size();
            tmp.resize(s - 1);  // Rid of '\n' at the end.
            if (tmp == "./" || tmp == "../") continue; // Skip . and ..
#if (defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)) || (defined(PF_DISTRO_SUSE) && (PF_MAJOR==9))
            if (isdir && tmp[tmp.size()-1] != '/')
            {
                tmp.append("/");
            }
#endif
            SCXCoreLib::SCXFilePath sfp;
            sfp.SetDirectory(SCXCoreLib::StrFromUTF8(dir));
            sfp.Append(SCXCoreLib::StrFromUTF8(tmp));   // Append either directory or filename
            target.push_back(sfp);
        }

        int eno = errno;
        pclose(findOutput);
        return eno == 0;                // Should be zero if ok
    }

    /**
       Utility method to list the contents of a directory using the find command and place
       the results in a SCXDirectoryInfo, that is a vector of SCXFilePath objects, including
       the paths. Despite using find, only one level of directories are returned.
    */
    static bool listDirectoryWithfind(vector<SCXCoreLib::SCXFilePath>& target,
                                      const char *dir, char type)
    {
        // find -L /usr/local -mindepth 1 -maxdepth 1 -type d                 Directories
        // find -L /etc -mindepth 1 -maxdepth 1 -type f                       Regular files
        // find -L /dev -mindepth 1 -maxdepth 1 ! \( -type d -o -type f \)    System files
        // Note: -L means follow symbolic links, which is the behaviour of tested method.
        // Followed by a printf "%p%y\n" to set the filetype as the last character

        char cmdbuf[100];
        char resultbuf[100];

        if (!dir) { return false; }
        strcpy(cmdbuf, "/usr/bin/find -L ");
        strcat(cmdbuf, dir);
        strcat(cmdbuf, " -mindepth 1 -maxdepth 1");
        switch (type) {
        case 'a':
            /* add nothing */
            break;
        case 'd':
            strcat(cmdbuf, " -type d");
            break;
        case 'f':
            strcat(cmdbuf, " -type f");
            break;
        case 's':
            strcat(cmdbuf, " ! \\( -type d -o -type f \\)");
            break;
        default:
            return false;
        }
        strcat(cmdbuf, "  -printf \"%p%y\\n\"");


        FILE *findOutput = popen(cmdbuf, "r");
        if (!findOutput) { return false; }

        while (fgets(resultbuf, sizeof(resultbuf), findOutput)) {
            string tmp(resultbuf);
            size_t s = tmp.size();

            if (tmp.at(s - 2) == 'd') {
                tmp.resize(s - 2); // Remove two last char, a type specifier and '\n'.
                tmp += '/';
            } else {
                tmp.resize(s - 2); // Remove two last char, a type specifier and '\n'.
            }
            SCXCoreLib::SCXFilePath tmp2(SCXCoreLib::StrFromUTF8(tmp));
            target.push_back(tmp2);
        }

        int eno = errno;
        pclose(findOutput);
        return eno == 0;                // Should be zero if ok
    }

#else /* !WIN32 */

    // It was simpler to make my own version than to use the MS so called secure versions
    static void mystrcpy (char *p, const char *s)
    {
        while ((*p++ = *s++));
    }
    static void mystrcat (char *p, const char *s)
    {
        while (*p) p++;
        while ((*p++ = *s++));
    }

    /**
       Utility method to list the contents of a directory using the DIR command and place
       the results in a SCXDirectoryInfo, that is a vector of SCXFilePath objects, including
       the paths.
       Does the DIR command in a DOS shell and parses the output. Depending on
       what type of files are requested (file, directory, or system), different
       flags are supplied.
       This method is very sensitive o the layout of what's returned from DIR. Using
       a localized version of windows may break ths test.
       Note that the _popen() call is only available if compiled as a Console Application
       in Windows.
    */
    static bool listDirectoryWithDir(vector<SCXCoreLib::SCXFilePath>& target,
                                     const char *dir, char type)
    {
        wchar_t wbuf[200];
        char cmdbuf[100];

        if (!dir) { return false; }
        mystrcpy(cmdbuf, "DIR ");
        switch (type) {
        case 'a': // todo
            mystrcat(cmdbuf, " /A:DHS ");
            break;
        case 'd': // todo
            mystrcat(cmdbuf, " /A:D-L-S ");
            break;
        case 'f':
            mystrcat(cmdbuf, " /A:-D-S ");
            break;
        case 's': // todo
            mystrcat(cmdbuf, " /A:S-L-D ");
            break;
        default:
            return false;
        }
        mystrcat(cmdbuf, dir);

        FILE *findOutput = _popen(cmdbuf, "rt");
        if (!findOutput) { return false; }

        errno = 0;
        wstring dirname;                                // Directory name as in listing
        while (!feof(findOutput)) {
            if(!fgetws(wbuf, sizeof(wbuf)/sizeof(wchar_t), findOutput)) break;
            std::wstring tmp(wbuf);
            size_t s = tmp.size();
            if (s < 10) { continue; }                   // Short lines do not match
            tmp.resize(s - 1);                          // Remove last char
            if (tmp.substr(1,9) == L"Directory") {
                dirname = tmp.substr(14);
                continue;
            }

            // Different versions of windows have different layouts of DIR. It's a mess!!!
            std::wstring name(L"");
            bool isDir = false;

            if (tmp[4] == '-' || tmp[2] == '/') { //A date with yyyy-MM-dd or MM/dd/yyyy
                isDir = ParseDirLine(tmp, name);
            } else { continue; }                // Means that we don't have a date first

            SCXCoreLib::SCXFilePath pth;        // Create path from dirname
            pth.SetDirectory(dirname);          // Set the base directory

            if (isDir) {
                // This is a directory
                pth.AppendDirectory(name);
            } else {
                pth.SetFilename(name);
            }
            target.push_back(pth);
        }
        int eno = errno;
        _pclose(findOutput);
        return eno == 0;                // Should be zero if ok
    }

    /**
    * Helper function for the test class that parses the output of
    * a DIR command. Previously, there was an attempt to split based
    * on the character position; however, this has broken on some
    * build machines.  The present approach is to use STL to split
    * the string and then make decisions based on the tokens.
    *
    * An important assumption is that there is a expected input format (see below)
    */
    static bool ParseDirLine(const std::wstring& input, std::wstring& output)
    {
        // Use STL to split strings on whitespace similar to either:
        //
        // 02/05/2009  02:01 PM    <DIR>          6b507d1f6a0e351ea1ee2f70
        // 07/08/2008  03:40 PM                 0 AUTOEXEC.BAT
        //
        // By splitting on the whitespace, ideally we would have 5 tokens.
        // By looking at the fourth token it can be determined if it is a
        // a file or directory. By using the fifth token, we can use that
        // to determine the location of the file name. If there are less
        // than five tokens the input is assumed to not be a file.
        std::vector<std::wstring> tokens;
        SCXCoreLib::StrTokenize(input, tokens);

        std::wstringstream warnMsgFilename;
        warnMsgFilename << L"Did not parse enough tokens for given line: '";
        warnMsgFilename << input << L"'";
        SCXUNIT_ASSERT_MESSAGEW(warnMsgFilename.str(), tokens.size() >= 4);
        output = input.substr(input.find(tokens[4]));
        bool isDir = (L"<DIR>" == tokens[3]);
        return isDir;
    }

#endif /* !WIN32 */

    /**
    * Helper utility to create a well-defined fake directory structure for consistent
    * unit-test runs.  This is necessary so that we can have
    * deterministic runs of our test code (i.e. we know the expected
    * results and these will not vary from system-to-system).
    *
    * @returns Full path to the deployment directory
    */
    static std::wstring CreateFauxDirectoryStructure()
    {
        std::stringstream createDirectoryStructure;
        std::wstring wideDeployDir = GetDeploymentDirectory().Get();
        std::string narrowDeployDir = SCXCoreLib::StrToUTF8(wideDeployDir);
        std::wstring wideFolderSeparator(1, SCXCoreLib::SCXFilePath::GetFolderSeparator());
        std::string narrowFolderSeparator = SCXCoreLib::StrToUTF8(wideFolderSeparator);
        RemoveFauxDirectoryStructure();

#if defined(WIN32)
        createDirectoryStructure << "mkdir " << narrowDeployDir;
        system(createDirectoryStructure.str().c_str());
        createDirectoryStructure.str("");
#else
        createDirectoryStructure << "mkdir -p " << narrowDeployDir;
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0);
        createDirectoryStructure.str("");
#endif
        createDirectoryStructure << "mkdir " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove ";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "mkdir " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "A ";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "mkdir " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "B ";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "echo hej > " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove"<< narrowFolderSeparator << "hej.txt ";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "echo hej > " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "A" << narrowFolderSeparator <<"hej.txt";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "echo hej hej> " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "A" << narrowFolderSeparator
#if defined(WIN32)
                                 << "\"hej hej.txt\"";
#else
                                 << "hej\\ hej.txt";
#endif
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "echo 1> " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "A" << narrowFolderSeparator <<"1.txt";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        createDirectoryStructure << "echo 1> " << narrowDeployDir << narrowFolderSeparator
                                 << "dirmove" << narrowFolderSeparator << "A" << narrowFolderSeparator <<"1";
        CPPUNIT_ASSERT( system(createDirectoryStructure.str().c_str()) >= 0 );
        createDirectoryStructure.str("");

        return wideDeployDir;
    }

    /**
    * Helper utility to remove the Faux Directory Structure.
    * This is a "never fail" method b/c it is called intended to
    * be called from the unit test tearDown() method.
    */
    static void RemoveFauxDirectoryStructure()
    {
        std::stringstream removeDirectoryStructure;
        std::wstring wideDeployDir = GetDeploymentDirectory().Get();
        std::string narrowDeployDir = SCXCoreLib::StrToUTF8(wideDeployDir);

#if defined( WIN32 )
        removeDirectoryStructure << "rmdir /S /Q " << narrowDeployDir;
#else
        removeDirectoryStructure << "rm -fR " << narrowDeployDir ;
#endif
        CPPUNIT_ASSERT( system(removeDirectoryStructure.str().c_str()) >= 0 );
    }


    /**
    * Test Helper Utility method for getting a name for temporary
    * directory located in the system's temporary directory that should
    * be used for unit-testing.  This is the name of the directory
    * only, it is not guaranteed that this directory either exists or
    * is empty. This merely states where the temporary file SHOULD be.
    */
    static SCXCoreLib::SCXFilePath GetDeploymentDirectory()
    {
        SCXFilePath deploymentBase = SCXFilePath(GetTmpDir());
        deploymentBase.AppendDirectory(L"DirectoryInfoUnitTests");
        return deploymentBase;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXDirectoryInfoTest );
