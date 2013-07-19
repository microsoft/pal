/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file      scxglob_test.cpp
   \brief     Unit test implementation for SCXGlob class
   
   \date      08-05-27 15:35:00
*/
/*----------------------------------------------------------------------------*/
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxglob.h>
#include <testutils/scxunit.h>

#include <cppunit/extensions/HelperMacros.h>

using namespace std;
using namespace SCXCoreLib;

class SCXGlobTest : public CPPUNIT_NS::TestFixture
{
public:
    /*----------------------------------------------------------------------------*/
    /* Macros to create the SCXGlobTest test suite. */
    CPPUNIT_TEST_SUITE(SCXGlobTest);
    CPPUNIT_TEST(testBackSlashEscapeState);
    CPPUNIT_TEST(testErrorAbortState);
    CPPUNIT_TEST(testGetPattern);
    CPPUNIT_TEST(testWildChar);
    CPPUNIT_TEST(testWildCharWithTrailingSlash);
    CPPUNIT_TEST(testDotWithWildChar);
    CPPUNIT_TEST(testDotDotChar);
    CPPUNIT_TEST(testQMarkCharTwice);
    CPPUNIT_TEST(testBracketCharWithWildCardDir);
    CPPUNIT_TEST(testBackSlashEscapedSpecialChar);
    CPPUNIT_TEST(testNonExistingFilePath);
    CPPUNIT_TEST(testEmptyPattern);
    CPPUNIT_TEST(testRelativePathWithCurrentDir1);
    CPPUNIT_TEST(testRelativePathWithCurrentDir2);
    CPPUNIT_TEST(testRelativePathWithParentDir1);
    CPPUNIT_TEST(testRelativePathWithParentDir2);
    CPPUNIT_TEST(testSCXFilePathUse);
    CPPUNIT_TEST(testReGlobbing1);
    CPPUNIT_TEST(testReGlobbing2);
    CPPUNIT_TEST(testNextAfterEndOfResults);
    CPPUNIT_TEST(testGetCurrentWithoutNext);
    CPPUNIT_TEST_SUITE_END();
    
private:
    /*----------------------------------------------------------------------------*/
    /* The current working directory. */
    wstring cwd;
    /* The current working directory as an SCXFilePath */
    SCXFilePath cwdFP;

public:
    /*----------------------------------------------------------------------------*/
    void setUp()
    {
        // Creates the following directory structure.

        /*
            cwd/    dir1/   dir2/   special*.txt
                            dir3/   special*.txt
                            .hidden.txt
                            linkone
                                                        ordinary_file.txt
                            special*.txt
        */

        mkdir("./dir1", S_IRWXU);
        mkdir("./dir1/dir2", S_IRWXU);
        mkdir("./dir1/dir3", S_IRUSR | S_IWUSR | S_IRWXU);

        int out = open("./dir1/.hidden.txt", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some data\n", 18) >= 0 );
        close(out);

        out = open("./dir1/special*.txt", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some data\n", 18) >= 0 );
        close(out);

        out = open("./dir1/dir2/special*.txt", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some data\n", 18) >= 0 );
        close(out);

        out = open("./dir1/dir3/special*.txt", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some data\n", 18) >= 0 );
        close(out);

        out = open("./dir1/ordinary_file.txt", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some ordinary data\n", 18) >= 0 );
        close(out);

        // Gets the current working directory.
        char *buffer = new char[BUFSIZ];
        buffer[BUFSIZ-1] = '\0';
        CPPUNIT_ASSERT( getcwd(buffer, BUFSIZ-1) != NULL );
        cwd = StrFromUTF8(buffer);
                cwdFP.SetDirectory(cwd);

        // Creates a symbolic link.
        out = symlink(strcat(buffer, "/dir1/.hidden.txt"), "./dir1/linkone");

        delete [] buffer;
    }

    void tearDown()
    {
        // Removes the file.
        unlink("./dir1/linkone");
        remove("./dir1/.hidden.txt");
        remove("./dir1/ordinary_file.txt");
        remove("./dir1/special*.txt");
        remove("./dir1/dir2/special*.txt");
        remove("./dir1/dir3/special*.txt");
        rmdir("./dir1/dir3");
        rmdir("./dir1/dir2");
        rmdir("./dir1");
    }

   /*----------------------------------------------------------------------------*/
   /**
    *  Unit test to retrieve the glob object's search pattern.
    */
    void testGetPattern()
    {
        wstring pattern = cwd + wstring(L"/*");
        SCXGlob globObj(pattern);
        CPPUNIT_ASSERT(pattern == globObj.GetPattern());
    }

   /*----------------------------------------------------------------------------*/
   /**
    *  Unit test to get/set the backslash-escaping flag value.
    */
    void testBackSlashEscapeState()
    {
        wstring pattern = cwd + wstring(L"/*");
        SCXGlob globObj(pattern);
        CPPUNIT_ASSERT(true == globObj.BackSlashEscapeState());
        globObj.BackSlashEscapeState(false);
        CPPUNIT_ASSERT(false == globObj.BackSlashEscapeState());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to get/set the error-abort flag value.
    */
    void testErrorAbortState()
    {
        wstring pattern = cwd + wstring(L"/*");
        SCXGlob globObj(pattern);
        CPPUNIT_ASSERT(false == globObj.ErrorAbortState());
        globObj.ErrorAbortState(true);
        CPPUNIT_ASSERT(true == globObj.ErrorAbortState());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern ending with *.
    */
    void testWildChar()
    {
        SCXFilePath pattern(cwdFP);
                pattern.Append(L"/dir1/*");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/dir2") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/dir3") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/linkone") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/ordinary_file.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/special*.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern ending with * followed by trailing slashes.
    */
    void testWildCharWithTrailingSlash()
    {
        SCXFilePath pattern(cwdFP);
                pattern.Append(L"/dir1/*//////");
        SCXGlob globObj(pattern);
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/*") == globObj.GetPattern());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern ending with . followed by *. 
    */
    void testDotWithWildChar()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"/dir1/.*");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/.") != globObj.Current().Get());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/.hidden.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern ending with two dots.
    */
    void testDotDotChar()
    {
        SCXFilePath pattern(cwdFP);
                pattern.Append(L"/..");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern containing ?? in its filename.
    */
    void testQMarkCharTwice()
    {
        wstring pattern = cwd + wstring(L"/dir1/link??e");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/linkone") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());

        // Adds another matching file.
        int out = open("./dir1/linkeee", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT( write(out, "Here is some data\n", 18) >= 0 );
        close(out);

        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/linkeee") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/linkone") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());

        remove("./dir1/linkeee");
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern containing * in its directory path and [] in its filename.
    */
    void testBracketCharWithWildCardDir()
    {
        wstring pattern = cwd + wstring(L"/dir1/*/spe[a,b,c,d]i[a,b,c,d]l*");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/dir2/special*.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/dir3/special*.txt") == globObj.Current().Get());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern containing \\* with the back-slash escaping state being on and off.
    */
    void testBackSlashEscapedSpecialChar()
    {
        wstring pattern = cwd + wstring(L"/dir1/s*l\\**");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/special*.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());

        // Disables back-slash escaping.
        globObj.BackSlashEscapeState(false);
        globObj.DoGlob();
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern that does not exist in the file system.
    */
    void testNonExistingFilePath()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"/dir1/hogehoge.txt");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a empty string search pattern.
    */
    void testEmptyPattern()
    {
                SCXFilePath pattern;
                
        SCXUNIT_RESET_ASSERTION();
                CPPUNIT_ASSERT_THROW(SCXGlob globObj(pattern), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED(1); 
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern containing ./ in its directory path.
    */
    void testRelativePathWithCurrentDir1()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"/dir1/./.*");
        SCXGlob globObj(pattern);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true ==  globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/./.hidden.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern starting with ./ characters.
    */
    void testRelativePathWithCurrentDir2()
    {
                SCXFilePath pattern(L"./*");
        SCXUNIT_RESET_ASSERTION();
                CPPUNIT_ASSERT_THROW(SCXGlob globObj(pattern), SCXInvalidArgumentException);
                SCXUNIT_ASSERTIONS_FAILED(1);
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern containing ../ in its directory path.
    */
    void testRelativePathWithParentDir1()
    {
        SCXFilePath pattern1(cwdFP);
                pattern1.Append(L"dir1/dir2/../special*.txt");
        SCXGlob globObj(pattern1);
        globObj.DoGlob();
        CPPUNIT_ASSERT(true ==  globObj.Next());
        CPPUNIT_ASSERT(cwd + wstring(L"/dir1/dir2/../special*.txt") == globObj.Current().Get());
        CPPUNIT_ASSERT(false == globObj.Next());
    }

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to glob a search pattern starting with ../ characters.
    */
    void testRelativePathWithParentDir2()
    {
                SCXFilePath pattern(L"../*");
        SCXUNIT_RESET_ASSERTION();
                CPPUNIT_ASSERT_THROW(SCXGlob globObj(pattern), SCXInvalidArgumentException);
                SCXUNIT_ASSERTIONS_FAILED(1);
    }
        
   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to test the SCXFilePath part of the functionality
    */
    void testSCXFilePathUse()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"dir1/dir*");
        SCXGlob globObj(pattern);
                globObj.DoGlob();
        CPPUNIT_ASSERT(true == globObj.Next());
                SCXFilePath d1(cwdFP); 
                d1.Append(L"dir1/dir2");
        CPPUNIT_ASSERT(d1 == globObj.Current());

    }
        

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to test if the instance is used more than one time
    */
    void testReGlobbing1()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"dir1/ordinary*.txt");

        SCXGlob globObj(pattern);
                globObj.DoGlob();

                // Get the only match
        CPPUNIT_ASSERT(true == globObj.Next());
                SCXFilePath path_verify1(cwdFP);
                path_verify1.Append(L"dir1/");
                path_verify1.Append(L"ordinary_file.txt");
                CPPUNIT_ASSERT(path_verify1 == globObj.Current());
                // Should not be more than the first match 
                CPPUNIT_ASSERT(false == globObj.Next());
                
                // Remove the found file from disk
                unlink(StrToUTF8(path_verify1.Get()).c_str());
                // Re-glob
                globObj.DoGlob();
                // No match should be found
        CPPUNIT_ASSERT(false == globObj.Next());
    }
        
   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to test if the instance is used more than one time
    */
    void testReGlobbing2()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"dir1/ordinary*.txt");

        SCXGlob globObj(pattern);
                // No match until globbed
        CPPUNIT_ASSERT(false == globObj.Next());
                
                globObj.DoGlob();
                globObj.DoGlob();  // Should not affect anything

                // Now there should be one match
        CPPUNIT_ASSERT(true == globObj.Next());

                // Should not affect current (so next Current() will work)
        CPPUNIT_ASSERT(false == globObj.Next());
                
                // Get the only match
                SCXFilePath path_verify1(cwdFP);
                path_verify1.Append(L"dir1/");
                path_verify1.Append(L"ordinary_file.txt");
                CPPUNIT_ASSERT(path_verify1 == globObj.Current());
    }
        
   /*----------------------------------------------------------------------------*/
   /**
    * Unit test to test if the instance is used more than one time
    */
    void testNextAfterEndOfResults()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"dir1/ordinary*.txt");

        SCXGlob globObj(pattern);
                // No match until globbed
        CPPUNIT_ASSERT(false == globObj.Next());
                
                globObj.DoGlob();

                // Now there should be one match, and doing many Next() should be no problem
        CPPUNIT_ASSERT(true == globObj.Next());
                SCXFilePath path_verify1(cwdFP);
                path_verify1.Append(L"dir1/");
                path_verify1.Append(L"ordinary_file.txt");
                CPPUNIT_ASSERT(path_verify1 == globObj.Current());
                
        CPPUNIT_ASSERT(false == globObj.Next());
        CPPUNIT_ASSERT(false == globObj.Next());
        CPPUNIT_ASSERT(false == globObj.Next());
                // Shoudl still point at last 
                CPPUNIT_ASSERT(path_verify1 == globObj.Current());
    }
        

   /*----------------------------------------------------------------------------*/
   /**
    * Unit test for the case Current is not valid 
    */
    void testGetCurrentWithoutNext()
    {
                SCXFilePath pattern(cwdFP);
                pattern.Append(L"dir1/ordinary*.txt");  // Not relevant for this test
        SCXGlob globObj(pattern);

                // Assuming empty 
                CPPUNIT_ASSERT(SCXFilePath() == globObj.Current());

                SCXFilePath pattern2(cwdFP);
                pattern2.Append(L"dir1/no_match_for_this"); 
        SCXGlob globObj2(pattern);
                CPPUNIT_ASSERT(SCXFilePath() == globObj.Current());
        CPPUNIT_ASSERT(false == globObj2.Next());
        CPPUNIT_ASSERT(false == globObj2.Next());
        CPPUNIT_ASSERT(false == globObj2.Next());
    }
        

};

CPPUNIT_TEST_SUITE_REGISTRATION(SCXGlobTest);
