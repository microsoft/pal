/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
*/
/**
   \file      scxlibglob_test.cpp
   \brief     Unit test implementation for LibGlob class

   \date      12-17-12 15:30:00
*/
/*----------------------------------------------------------------------------*/
#if defined(linux)
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <scxcorelib/scxlibglob.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

#include <cppunit/extensions/HelperMacros.h>

using namespace std;
using namespace SCXCoreLib;

class SCXLibGlobTest : public CPPUNIT_NS::TestFixture
{
public:
    CPPUNIT_TEST_SUITE(SCXLibGlobTest);
    CPPUNIT_TEST(testOneDirectoryTwoVersions);
    CPPUNIT_TEST(testManyDirectoriesManyVersions);
    CPPUNIT_TEST(testManyVersionTypes);
    CPPUNIT_TEST_SUITE_END();

private:
    /* The current working directory. */
    wstring cwd;

public:
    void setUp()
    {
        /*
          This will create the following directory structure

          cwd/  lg1/  libtest.so.1.0.9
                      libtest.so.1.0.10
                lg2/  libtest.so.1.1.2
                lg3/  libtest.so.2.1.0
                      libtest.so.3.0.1
                      libtest.so.2.4.5
                lg4/  libtest.so.2.6.7
                lg5/  libtestdb-4.4.so
                      libtest-4.4.so
                      libtest.so

        */
        mkdir("./lg1", S_IRWXU);
        mkdir("./lg2", S_IRWXU);
        mkdir("./lg3", S_IRWXU);
        mkdir("./lg4", S_IRWXU);
        mkdir("./lg5", S_IRWXU);

        int out;
        static const char to_write []= "Here is some data\n";
        static const int count = (int)strlen(to_write);

        out = open("./lg1/libtest.so.1.0.9", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg1/libtest.so.1.0.10", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg2/libtest.so.1.1.2", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg3/libtest.so.2.1.0", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg3/libtest.so.3.0.1", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg3/libtest.so.2.40.5", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg4/libtest.so.2.6.7", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg5/libtestdb-4.4.so", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg5/libtest-4.4.so", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        out = open("./lg5/libtest.so", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);
        CPPUNIT_ASSERT_EQUAL(count, write(out, to_write, count));
        close(out);

        char *buffer = new char[BUFSIZ];
        buffer[BUFSIZ-1] = '\0';
        CPPUNIT_ASSERT(getcwd(buffer, BUFSIZ-1) != NULL);
        cwd = StrFromUTF8(buffer);
    }

    void tearDown()
    {
        remove("./lg1/libtest.so.1.0.9");
        remove("./lg1/libtest.so.1.0.10");
        remove("./lg2/libtest.so.1.1.2");
        remove("./lg3/libtest.so.2.1.0");
        remove("./lg3/libtest.so.3.0.1");
        remove("./lg3/libtest.so.2.40.5");
        remove("./lg4/libtest.so.2.6.7");
        remove("./lg5/libtestdb-4.4.so");
        remove("./lg5/libtest-4.4.so");
        remove("./lg5/libtest.so");

        rmdir("./lg1");
        rmdir("./lg2");
        rmdir("./lg3");
        rmdir("./lg4");
        rmdir("./lg5");
    }

    void testOneDirectoryTwoVersions()
    {
        /*
          In an alphabetical sort, libtest.so.1.0.9 would come before libtest.so.1.0.10.
          This test asserts the order to be libtest.so.1.0.10 before libtest.so.1.0.9
         */
        vector<wstring> dirs;
        dirs.push_back(cwd + L"/lg1");

        SCXLibGlob libglobtest(L"libtest*so*", dirs);

        vector<SCXFilePath> paths = libglobtest.Get();

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), paths.size());
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.1.0.10"), StrToUTF8(paths[0].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(cwd) + string("/lg1/libtest.so.1.0.9"), StrToUTF8(paths[1].Get()));
    }

    void testManyDirectoriesManyVersions()
    {
        /*
          This tests multiple directory support.
         */
        vector<wstring> dirs;
        dirs.push_back(cwd + L"/lg1");
        dirs.push_back(cwd + L"/lg2");
        dirs.push_back(cwd + L"/lg3");
        dirs.push_back(cwd + L"/lg4");

        SCXLibGlob libglobtest(L"libtest*so*", dirs);

        vector<SCXFilePath> paths = libglobtest.Get();

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(7), paths.size());
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.3.0.1"), StrToUTF8(paths[0].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.2.40.5"), StrToUTF8(paths[1].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.2.6.7"), StrToUTF8(paths[2].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.2.1.0"), StrToUTF8(paths[3].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.1.1.2"), StrToUTF8(paths[4].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.1.0.10"), StrToUTF8(paths[5].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.1.0.9"), StrToUTF8(paths[6].GetFilename()));
    }

    void testManyVersionTypes()
    {
        /*
          This tests behavior when version numbers are not the only differences between matched filenames.
          For example, on some test systems "librpm" has the filename "librpm-4.4.so", but on others it has
          "librpm.so.1.0.0".
          There are also some test systems where globbing for "librpm*so*" matches libraries like "librpmdb-4.4.so".
          This library would come before the "librpm.so" library, because it differs alphabetically before any numeric value.
         */
        vector<wstring> dirs;
        dirs.push_back(cwd + L"/lg2");
        dirs.push_back(cwd + L"/lg5");

        SCXLibGlob libglobtest(L"libtest*so*", dirs);

        vector<SCXFilePath> paths = libglobtest.Get();

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), paths.size());
        CPPUNIT_ASSERT_EQUAL(string("libtestdb-4.4.so"), StrToUTF8(paths[0].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so.1.1.2"), StrToUTF8(paths[1].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest.so"), StrToUTF8(paths[2].GetFilename()));
        CPPUNIT_ASSERT_EQUAL(string("libtest-4.4.so"), StrToUTF8(paths[3].GetFilename()));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(SCXLibGlobTest);

#endif // defined(linux)
