/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-08-27 17:20:00

  File test class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <testutils/scxunit.h>
#include <testutils/scxtestutils.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxlocale.h>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>

#if !defined(WIN32)
#include <sys/types.h>
#include <regex.h>
#else
#include <io.h>
#endif /* !WIN32 */
#include <errno.h>

using namespace SCXCoreLib;
using namespace std;

class SCXFileTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE( SCXFileTest );

    CPPUNIT_TEST( TestFileExists );
    CPPUNIT_TEST( TestFileDoesNotExists );
    CPPUNIT_TEST( TestFileGetAttributesFile );
    CPPUNIT_TEST( TestFileSystemInfo_SetAttributes );
    CPPUNIT_TEST( TestFileGetAttributesDirectory );
    CPPUNIT_TEST( TestFileSetAttributes );
    CPPUNIT_TEST( TestFileDelete );
    CPPUNIT_TEST( TestFileMove );
    CPPUNIT_TEST( TestReadFilesWithPresetCharacterConversion );
#if !defined(WIN32) && !defined(SCX_STACK_ONLY)
    CPPUNIT_TEST( TestReadFilesWithCharacterConversion );
#endif
    CPPUNIT_TEST( TestReadAllLinesDirectory );
    CPPUNIT_TEST( TestReadAllLinesNonExistingFile );
    CPPUNIT_TEST( TestWriteAllLines );
    CPPUNIT_TEST( TestWriteAllLinesAsUTF8 );
    CPPUNIT_TEST( TestSeek );
#if !defined(DISABLE_WIN_UNSUPPORTED)
    CPPUNIT_TEST( TestCreateTempFile );
    CPPUNIT_TEST( TestCreateTempFileValidDirectory );
    CPPUNIT_TEST( TestCreateTempFileInvalidDirectory );
    CPPUNIT_TEST( TestCreateTempFileNonDefaultDirectory );
    CPPUNIT_TEST( TestNonBlockingReadNonExistingFile );
    CPPUNIT_TEST( TestNonBlockingReadFileLarger );
    CPPUNIT_TEST( TestNonBlockingReadFileSmaller );
    CPPUNIT_TEST( TestNonBlockingReadOfRandom );
    CPPUNIT_TEST( TestSCXFileHandle );
#endif
    SCXUNIT_TEST_ATTRIBUTE(TestReadFilesWithCharacterConversion, SLOW);
    CPPUNIT_TEST_SUITE_END();

    private:
    SCXFilePath m_path1;
    SCXFilePath m_path2;

    public:
    SCXFileTest() : m_path1(L"SCXFileTestTemporary.txt"),
                    m_path2(L"SCXFileTestTemporary2.txt")
    {
    }

    void setUp() {
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("Failed to delete file. Maybe it is not writeable.", SCXFile::Delete(m_path1));
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("Failed to delete file. Maybe it is not writeable.", SCXFile::Delete(m_path2));
        ofstream file(SCXFileSystem::EncodePath(m_path1).c_str());
        file.close();
    }

    void tearDown() {
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("Failed to delete file. Maybe it is not writeable.", SCXFile::Delete(m_path1));
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("Failed to delete file. Maybe it is not writeable.", SCXFile::Delete(m_path2));
    }

    void TestFileExists() {
        CPPUNIT_ASSERT(SCXFile::Exists(m_path1));
        SCXFile::Delete(m_path1);
        CPPUNIT_ASSERT(!SCXFile::Exists(m_path1));
        CPPUNIT_ASSERT(!SCXFile::Exists(SCXFilePath(L"/")));
    }

    void TestFileDoesNotExists()
    {
        SCXUNIT_ASSERT_THROWN_EXCEPTION(SCXFile::OpenWFstream(m_path2, std::ios_base::in), SCXFilePathNotFoundException, m_path2.Get());
    }

    void TestFileGetAttributesFile() {
        CPPUNIT_ASSERT(SCXFileInfo(m_path1).GetAttributes().count(SCXFileSystem::eWritable) > 0);
        CPPUNIT_ASSERT(SCXFileInfo(m_path2).GetAttributes().count(SCXFileSystem::eDirectory) == 0);
        SCXFile::Delete(m_path1);
        CPPUNIT_ASSERT(SCXFileInfo(m_path1).GetAttributes().count(SCXFileSystem::eWritable) == 0);
        CPPUNIT_ASSERT(SCXFileInfo(m_path1).GetAttributes().count(SCXFileSystem::eReadable) == 0);
    }

    void TestFileSystemInfo_SetAttributes()
    {
        // Improve code coverage on SCXFileSystemInfo
#if defined(SCX_UNIX)
        SCXFileSystem::Attribute readable = SCXFileSystem::eUserRead;
        SCXFileSystem::Attribute writable = SCXFileSystem::eUserWrite;
#else
        SCXFileSystem::Attribute readable = SCXFileSystem::eReadable;
        SCXFileSystem::Attribute writable = SCXFileSystem::eWritable;
#endif

        SCXFileInfo fi(m_path1);

        SCXFileSystem::Attributes attrRO, attrRW;
        attrRO.insert(readable);
        attrRW.insert(readable);
        attrRW.insert(writable);

        // Test SCXFileSystemInfo::SetAttributes and SCXFileSystemInfo::isWritable (and make sure it's right!)
        fi.SetAttributes(attrRO);
        CPPUNIT_ASSERT(! fi.isWritable());
        CPPUNIT_ASSERT(fi.GetAttributes().count(writable) == 0);
        fi.SetAttributes(attrRW);
        fi.Refresh();
        CPPUNIT_ASSERT(fi.isWritable());
        CPPUNIT_ASSERT(fi.GetAttributes().count(writable) != 0);

        // Create a new "junk" object to test directory - test SCXFileSystemInfo::GetDirectoryPath
        // (Use operator += to add a filename to pick up an additional test of that operator)
        SCXFilePath fbad = fi.GetDirectoryPath();
        fbad += L"file";
        fbad += L".txt";
        fbad.SetDirectory(L"/bogus/directory/path");
#if defined(SCX_UNIX)
        CPPUNIT_ASSERT(fbad.GetDirectory() == L"/bogus/directory/path/");
        CPPUNIT_ASSERT(fbad.Get() == L"/bogus/directory/path/file.txt");
#else
        CPPUNIT_ASSERT(fbad.GetDirectory() == L"\\bogus\\directory\\path\\");
        CPPUNIT_ASSERT(fbad.Get() == L"\\bogus\\directory\\path\\file.txt");
#endif
        // Original path was created without directory - test SCXFileSystemInfo::GetOriginalPath
        CPPUNIT_ASSERT(fi.GetOriginalPath().GetDirectory() == L"");
        CPPUNIT_ASSERT(fi.GetFullPath().GetDirectory() != L"");
    }

    void TestFileGetAttributesDirectory() {
        SCXFileSystem::Attributes attribs(SCXFileSystem::GetAttributes(SCXFilePath(L".")));
        CPPUNIT_ASSERT(attribs.count(SCXFileSystem::eDirectory) == 1);
    }

    void TestFileSetAttributes() {
        SCXFileSystem::Attributes readable;
        readable.insert(SCXFileSystem::eReadable);
        SCXFile::SetAttributes(m_path1, readable);
#if defined(SCX_UNIX)
        // have to add eUserRead since it will also be set when getting the attributes.
        readable.insert(SCXFileSystem::eUserRead);
#endif
        CPPUNIT_ASSERT(SCXFileSystem::GetAttributes(m_path1) == readable);
        SCXFileSystem::Attributes readablewritable;
        readablewritable.insert(SCXFileSystem::eReadable);
        readablewritable.insert(SCXFileSystem::eWritable);
        SCXFile::SetAttributes(m_path1, readablewritable);
#if defined(SCX_UNIX)
        // have to add eUserRead & eUserWrite since they will also be set when getting the attributes.
        readablewritable.insert(SCXFileSystem::eUserRead);
        readablewritable.insert(SCXFileSystem::eUserWrite);
#endif
        CPPUNIT_ASSERT(SCXFileSystem::GetAttributes(m_path1) == readablewritable);
#if defined(SCX_UNIX)
        SCXFileSystem::Attributes chmod0755;
        chmod0755.insert(SCXFileSystem::eReadable);
        chmod0755.insert(SCXFileSystem::eWritable);
        chmod0755.insert(SCXFileSystem::eUserRead);
        chmod0755.insert(SCXFileSystem::eUserWrite);
        chmod0755.insert(SCXFileSystem::eUserExecute);
        chmod0755.insert(SCXFileSystem::eGroupRead);
        chmod0755.insert(SCXFileSystem::eGroupExecute);
        chmod0755.insert(SCXFileSystem::eOtherRead);
        chmod0755.insert(SCXFileSystem::eOtherExecute);
        SCXFile::SetAttributes(m_path1, chmod0755);
        CPPUNIT_ASSERT(SCXFileSystem::GetAttributes(m_path1) == chmod0755);
#endif
    }

    void TestFileDelete() {
        SCXFile::Delete(m_path1);
        CPPUNIT_ASSERT(!SCXFile::Exists(m_path1));
    }

    void TestFileMove() {
        SCXFile::Move(m_path1, m_path1);
        CPPUNIT_ASSERT(SCXFile::Exists(m_path1));
        SCXFile::Move(m_path1, m_path2);
        CPPUNIT_ASSERT(!SCXFile::Exists(m_path1));
        CPPUNIT_ASSERT(SCXFile::Exists(m_path2));
    }

    //! Search for an existing filename using case insensitive comparision
    SCXFilePath SearchExistingFilename(const wstring &directory, const wstring &name) {
        vector<SCXCoreLib::SCXFilePath> files = SCXDirectory::GetFiles(directory);
        for (size_t nr = 0; nr < files.size(); nr++) {
            if (StrCompare(files[nr].GetFilename(), name, true) == 0) {
                return files[nr];
            }
        }
        // No existing name found, return the original path
        SCXFilePath original;
        original.SetDirectory(directory);
        original.SetFilename(name);
        return original;
    }


    //! Replace every occurance of a string in another string
    //! \param oldstr  To be searched for and replaced
    //! \param newstr  Replacement for "oldstr"
    //! \param str     To be modified
    void Replace(const std::string &oldstr, const std::string &newstr, string &str) {
        size_t pos = str.find(oldstr);
        while (pos != string::npos) {
            str.replace(pos, oldstr.size(), newstr);
            pos = str.find(oldstr, pos + newstr.size());
        }
    }

    /*
     * Uses the currently set locale to read a pre-defined file and then compares
     * the output with a reference file that is read with the UTF-8 functions.
     * These should of course be the same for this test to be successful.
     *
     * What encoded file should be read for a certain locale, and which reference file
     * should be used to test against is handled with a configurations file
     * that has the name "scxfile_test-locale-map.txt" that consists of multiple 
     * lines like this:
     * <name of locale> <name of encoded file> <name of reference file>
     * 
     * The "encoded file" is read with named locale active into an array.
     * The reference file is read with our own UTF-8 decoding routines, into 
     * another array. These two should result in exactly the same result for this
     * test to be successful.
     * 
     * If you're writing a new encoded or reference file, you'll at some point need
     * to see exactly what it contains byte-for-byte. The this command will be 
     * useful: "od -t x1 <filename>".
     *
     * If the current locale is not found in the configuration file, this results
     * in a warning.
     */
    void TestReadFilesWithPresetCharacterConversion()
    {
        bool found = false;

        std::wifstream locmap("./testfiles/scxfile_test-locale-map.txt");
        wstring locName, encodedFileName, referenceFileName;

        // This is the name of the currently selected locale for the Ctype facet
        wstring presetLocaleName(SCXLocaleContext::GetCtypeName());

        if (presetLocaleName == L"C" || presetLocaleName == L"POSIX") {
            SCXUNIT_WARNING(L"Testing with C/POSIX locale is meaningless.");
        }

        wcout << "\nTesting preset locale " << presetLocaleName << endl;

        while (locmap) {
            locmap >> locName >> encodedFileName >> referenceFileName;

            // Diagnostic output
            // wcout << "Name " << locName << endl;
            // wcout << "File " << encodedFileName << endl;
            // wcout << "File " << referenceFileName << endl;

            if (locName == presetLocaleName) {
                // wcout << L"found " << locName << endl;
                found = true;
                break;
            }
        }

        if (!found) {
            SCXUNIT_WARNING(L"Can't find preset locale " + presetLocaleName + L" in locale-map.txt. Please add it and test again.");
            return;
        }

        SCXFilePath encodedFileFP;
        encodedFileFP.SetDirectory(L"./testfiles/");
        encodedFileFP.SetFilename(encodedFileName);

        SCXFilePath referenceFileFP;
        referenceFileFP.SetDirectory(L"./testfiles/");
        referenceFileFP.SetFilename(referenceFileName);

        SCXStream::NLFs nlfs;
        vector<wstring> localLines;
        vector<wstring> utf8Lines;
        SCXFile::ReadAllLines(encodedFileFP, localLines, nlfs);
        SCXFile::ReadAllLinesAsUTF8(referenceFileFP, utf8Lines, nlfs);
        CPPUNIT_ASSERT_MESSAGE("Failure for preset locale " + locale().name(),
                               localLines == utf8Lines);

    }

// We do not need char conversion support when building stack only.
// These tests currently fail on Mac since we do not support changing locale to anything but 'C' on that OS.
#if !defined(WIN32) && !defined(SCX_STACK_ONLY)

    /** Test automatic character conversion according to locale setting by
        reading one or more encoded files and compare the result to a file that is
        read with explicit UTF-8 conversion.
    */
    void TestReadFilesWithCharacterConversion()
    {
        /* List of patterns, in preference order, that should
           match the name of a locale that provides 8859-1, or 8859-15 translation.
        */
        const char *iso8859patterns[] = {
            "^en.?us.?iso.?8859.?1$|^en.?us.?iso.?8859.?15$",
            "iso.?8859.?1$|iso.?8859.?15$",
            (char*)0
        };

        /* List of patterns, in preference order, that should
           match the name of a locale that provides UTF-8 translation.
        */
        const char *utf8patterns[] = {
            "^en.?us.?utf.?8",
            "utf.?8",
            (char*)0
        };

        wstring utf8ReferenceFileName(L"scxfile_test-UTF8.txt");
        SCXFilePath utf8EncodedFile(SearchExistingFilename(L"./testfiles/",
                                                           utf8ReferenceFileName));

        vector<string>::const_iterator pos;
        vector<string> match;
        const char* pattern = 0;
        int i = 0;

        /*
         * Find a UTF-8 locale and do file reading tests.
         */

        wstring localeEncodedFileName(L"scxfile_test-en_US.UTF-8.txt");
        SCXFilePath systemEncodedFile(SearchExistingFilename(L"./testfiles/",
                                                             localeEncodedFileName));
        CPPUNIT_ASSERT_MESSAGE("Missing locale test file  " +
                               StrToUTF8(systemEncodedFile.Get()),
                               SCXFile::Exists(systemEncodedFile));

        i = 0;
        // Walk over patterns. If the pattern matches multiple locales, then test
        // all of them, but once one has passed, don't try any more patterns.
        while((pattern = utf8patterns[i++])) {
            bool found = false;
            match.clear();
            // cout << "Trying UTF-8 pattern: " << pattern << endl;

            bool res = findInstalledLocale(pattern, match);
            if (!res) {
                // We could not find an installed locale that matches this pattern
                // cout << "No match to UTF-8 pattern " << pattern << endl;
                continue;
            }

            // Walk over results and remember if at least one is successful
            for(pos = match.begin(); pos != match.end(); ++pos) {
                // cout << "Match to UTF-8 pattern: " << *pos << endl;
                found |= doFileComparisonWithLocale(*pos, systemEncodedFile, utf8EncodedFile);
            }
            if (found) { break; } // We've found one pattern that works.
        }


        /*
         * Find a ISO-8859-1(5) locale and do file reading tests.

         Note: We can compare the ISO 8859-1/15 locales here because the internal
         (i.e. numeric) representation of the characters that we have in the test
         files are the same as for UTF-8 for those characters that we test.
         This is unique to 8815-1/15 and not true for non-unicode character sets in general.
         We don't test the C locale here because characters > 127 are undefined.

        */

        wstring isoEncodedFileName(L"scxfile_test-iso8859-1.txt");
        SCXFilePath isoEncodedFile(SearchExistingFilename(L"./testfiles/",
                                                          isoEncodedFileName));
        CPPUNIT_ASSERT_MESSAGE("Missing locale test file  " +
                               StrToUTF8(isoEncodedFile.Get()),
                               SCXFile::Exists(isoEncodedFile));


        i = 0;
        // Walk over patterns. If the pattern matches multiple locales, then test
        // all of them, but once one has passed, don't try any more patterns.
        while((pattern = iso8859patterns[i++])) {
            bool found = false;
            match.clear();
            // cout << "Trying ISO8859-1 pattern: " << pattern << endl;

            bool res = findInstalledLocale(pattern, match);
            if (!res) {
                // We could not find an installed locale that matches this pattern
                // cout << "No match to ISO8859-1 pattern " << pattern << endl;
                continue;
            }
            // Walk over results and remember if at least one is successful
            for(pos = match.begin(); pos != match.end(); ++pos) {
                // cout << "Match to ISO8859-1 pattern: " << *pos << endl;
                found |= doFileComparisonWithLocale(*pos, isoEncodedFile, utf8EncodedFile);
            }
            if (found) { break; } // We've found one pattern that works.
        }

    }

    /* Read two files under a certain locale setting and compare their contents
       for equality. The first file is translated according to the locale, and the
       other file is read and interpreted as a UTF-8 encoded file.
       If the requested locale can't be set a warning is printed.
       If the files don't compare equal, there is an assertion failure.
       This is a helper function to TestReadFilesWithCharacterConversion()
    */
    bool doFileComparisonWithLocale(string localeName, const SCXFilePath& systemEncodedFile,
                                    const SCXFilePath& utf8EncodedFile)
    {
        SCXStream::NLFs nlfs;
        vector<wstring> systemLines;
        vector<wstring> utf8Lines;
        string nameAsReportedByLocale;

        cout << "\nTesting locale " << localeName << endl;
        try {
            /* Run in separate locale context */
            SCXCoreLib::SCXLocaleContext testedLocale(localeName.c_str());
            nameAsReportedByLocale = testedLocale.name();

            SCXFile::ReadAllLines(systemEncodedFile, systemLines, nlfs);
            SCXFile::ReadAllLinesAsUTF8(utf8EncodedFile, utf8Lines, nlfs);

        } catch (exception &e) {
            // Note: If ReadAllLines or ReadAllLinesAsUTF8 cases an exception, or
            // more important, suffers an assertion failure, it will be caught
            // here too. It would be possible to work around, but too awkward.

            // We could assert here, but a failure is most likely a configuration
            // problem on the local system. Instead we can go on and try another locale
            // for better luck, and instead assert if no-one works.
            //CPPUNIT_FAIL("Can't set locale to " + localeName);

            SCXUNIT_WARNING(StrFromUTF8("Exception when setting locale to " + localeName));
            return false;
        }

        CPPUNIT_ASSERT_MESSAGE("Failure for locale " + nameAsReportedByLocale,
                               systemLines == utf8Lines);

        return true;
    }


    /* Utility function to get a the name of an installed locale based on a
       regular expression. Will fail if there is no matching locale installed.
       All matching names are returned in "localnames".
       The matching is case insensitive and uses the extended syntax.
       Not available on WIN32 since it isn't POSIX compilant.
    */
    bool findInstalledLocale(const string& pattern, vector<string>& localenames)
    {
        char cmdbuf[100];
        char resultbuf[300];
        bool success = false;
        regex_t re;
        int status;
        int eno = 0;

        /* Compile regexp */
        CPPUNIT_ASSERT_MESSAGE("Regexp failed to compile",
                               regcomp(&re, pattern.c_str(),
                                       REG_EXTENDED|REG_NOSUB|REG_ICASE|REG_NEWLINE) == 0);

        /* The command "locale -a" lists all installed locales. */
        strcpy(cmdbuf, "LC_ALL=C locale -a");

        errno = 0;              // Reset errno

        FILE *localeOutput = popen(cmdbuf, "r");
        if (!localeOutput) { return false; }

        while (fgets(resultbuf, sizeof(resultbuf), localeOutput)) {
            // Get rid of trailing newline
            char *p = strchr(resultbuf, '\n');
            if (p) { *p = '\0'; }

            status = regexec(&re, resultbuf, 0, 0, 0);
            if (REG_NOMATCH == status) { continue; }
            CPPUNIT_ASSERT_MESSAGE("Regexp matching returned an error", 0 == status);

            /* Match found. Save the value. */
            localenames.push_back(string(resultbuf));
            success = true;
        }

        if (ferror(localeOutput)) {
          eno = errno;
        }

        pclose(localeOutput);
        regfree(&re);

        errno = eno;
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(eno), errno == 0);

        return success;
    }

#endif /* !WIN32 */

    void TestReadAllLinesDirectory() {
        vector<wstring> lines;
        SCXStream::NLFs nlfs;
        try {
            SCXFile::ReadAllLines(SCXFilePath(L"."), lines, nlfs);
            CPPUNIT_ASSERT(!"Detected problem");
        } catch (SCXUnauthorizedFileSystemAccessException& e1) {
            CPPUNIT_ASSERT(e1.GetAttributes().count(SCXFileSystem::eDirectory) == 1);
            try {
                SCXFile::ReadAllLinesAsUTF8(SCXFilePath(L"."), lines, nlfs);
                CPPUNIT_ASSERT(!"Detected problem");
            } catch (SCXUnauthorizedFileSystemAccessException& e2) {
                CPPUNIT_ASSERT(e2.GetAttributes().count(SCXFileSystem::eDirectory) == 1);
            }
        }
    }

    void TestReadAllLinesNonExistingFile() {
        vector<wstring> lines;
        lines.push_back(L"Row 1");
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(SCXFilePath(L"jdhfjsdhfjjsdhfj"), lines, nlfs);
        CPPUNIT_ASSERT(lines.size() == 0 && nlfs.empty());
        lines.push_back(L"Row 1");
        SCXFile::ReadAllLinesAsUTF8(SCXFilePath(L"jdhfjsdhfjjsdhfj"), lines, nlfs);
        CPPUNIT_ASSERT(lines.size() == 0 && nlfs.empty());
    }

    void TestWriteAllLines() {
        vector<wstring> lines1;
        lines1.push_back(L"Row 1");
        lines1.push_back(L"Row 2");
        SCXFile::WriteAllLines(m_path1, lines1, ios_base::out | ios_base::trunc);
        vector<wstring> lines2;
        lines2.push_back(L"Row 3");
        lines2.push_back(L"Row 4");
        SCXFile::WriteAllLines(m_path1, lines2, ios_base::out | ios_base::app);
        vector<wstring> allWrittenLines;
        copy(lines1.begin(), lines1.end(), back_inserter(allWrittenLines));
        copy(lines2.begin(), lines2.end(), back_inserter(allWrittenLines));
        vector<wstring> allReadLines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(m_path1, allReadLines, nlfs);
        CPPUNIT_ASSERT(allReadLines == allWrittenLines);
    }

    void TestWriteAllLinesAsUTF8() {
        vector<wstring> lines1;
#if defined(sun)
        lines1.push_back(StrFromUTF8("Row \xc3\x85\xc3\x84\xc3\x96")); // Swedish chars, upper case aa, ae, oe
#else
        lines1.push_back(L"Row \x00c5\x00c4\x00d6"); // Swedish chars, upper case aa, ae, oe
#endif
        lines1.push_back(L"Row ABC");
        SCXFile::WriteAllLinesAsUTF8(m_path1, lines1, ios_base::out | ios_base::trunc);
        vector<wstring> lines2;
#if defined(sun)
        lines2.push_back(StrFromUTF8("Row \xc3\xa5\xc3\xa4\xc3\xb6")); // Swedish chars, lower case aa, ae, oe
#else
        lines2.push_back(L"Row \x00e5\x00e4\x00f6"); // Swedish chars, lower case aa, ae, oe
#endif
        lines2.push_back(L"Row abc");
        SCXFile::WriteAllLinesAsUTF8(m_path1, lines2, ios_base::out | ios_base::app);
        vector<wstring> allWrittenLines;
        copy(lines1.begin(), lines1.end(), back_inserter(allWrittenLines));
        copy(lines2.begin(), lines2.end(), back_inserter(allWrittenLines));
        vector<wstring> allReadLines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLinesAsUTF8(m_path1, allReadLines, nlfs);

        CPPUNIT_ASSERT(allReadLines == allWrittenLines);
    }

    void TestSeek() {
        wstring testfile(L"teststreampos");
        {
            SCXHandle<std::wfstream> outstream = SCXFile::OpenWFstream(testfile, std::ios_base::out);
            *outstream << L"0123456789";
        }
        {
            SCXHandle<std::wfstream> instream = SCXFile::OpenWFstream(testfile, std::ios_base::in);
            SCXFile::SeekG(*instream, 4);
            CPPUNIT_ASSERT(4 == instream->tellg());
            SCXFile::SeekG(*instream, 2);
            CPPUNIT_ASSERT(2 == instream->tellg());
            SCXFile::SeekG(*instream, 0);
            CPPUNIT_ASSERT(0 == instream->tellg());
        }
        // on windows you have to close ("{}" above) file before deleting it
        SCXFile::Delete(testfile);
    }

#if !defined(DISABLE_WIN_UNSUPPORTED)
    void TestCreateTempFile() {
        wstring content(L"This is the file content\non the temp file\n");
        SCXFilePath path = SCXFile::CreateTempFile(content);
        SelfDeletingFilePath sfdPath(path);
        CPPUNIT_ASSERT(SCXFile::Exists(path));
        SCXCoreLib::SCXHandle<std::wfstream> fileStream = SCXFile::OpenWFstream(path, std::ios::in);
        std::wostringstream compare;
        (*fileStream) >> compare.rdbuf();
        if (content != compare.str()) {
            wcout << L"content: " << content << std::endl;
            wcout << L"compare: " << compare.str() << std::endl;
            CPPUNIT_ASSERT(content == compare.str());
        }
    }
    
    void TestCreateTempFileValidDirectory()
    {
    	wstring content(L"This is the file content\non the temp file\n");
    	SCXFilePath path = SCXFile::CreateTempFile(content,L"/tmp/");
    	SelfDeletingFilePath sfdPath(path);
    	CPPUNIT_ASSERT(SCXFile::Exists(path));
    	SCXCoreLib::SCXHandle<std::wfstream> fileStream = SCXFile::OpenWFstream(path, std::ios::in);
    	std::wostringstream compare;
    	(*fileStream) >> compare.rdbuf();
    	if (content != compare.str()) 
    	{
    	    wcout << L"content: " << content << std::endl;
    	    wcout << L"compare: " << compare.str() << std::endl;
    	    CPPUNIT_ASSERT(content == compare.str());
    	}
    }
    
    void TestCreateTempFileInvalidDirectory()
    {
        wstring content(L"This is the file content\non the temp file\n");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(SCXFile::CreateTempFile(content,L"/not/valid/directory/"), SCXFilePathNotFoundException, L"/not/valid/directory");
    }
    
    void TestCreateTempFileNonDefaultDirectory()
    {
    	wstring content(L"This is the file content\non the temp file\n");
    	// Creating Temp file in CWD
    	SCXFilePath path = SCXFile::CreateTempFile(content,L"./");
    	SelfDeletingFilePath sfdPath(path);
    	CPPUNIT_ASSERT(SCXFile::Exists(path));
    	SCXCoreLib::SCXHandle<std::wfstream> fileStream = SCXFile::OpenWFstream(path, std::ios::in);
    	std::wostringstream compare;
    	(*fileStream) >> compare.rdbuf();
    	if (content != compare.str())
    	{
    		wcout << L"content: " << content << std::endl;
    		wcout << L"compare: " << compare.str() << std::endl;
    		CPPUNIT_ASSERT(content == compare.str());
    	}
    }

    void TestNonBlockingReadOfRandom()
    {
        // Typical system have max 4096 bits of entropy (i.e. 512 bytes).
        // Reading 10K of data should be possible from /dev/urandom but not /dev/random.
        // Especially if a read to /dev/urandom has emptied the entropy cache.
        char buf[10*1024];
        CPPUNIT_ASSERT_EQUAL(sizeof(buf), SCXFile::ReadAvailableBytes(L"/dev/urandom", buf, sizeof(buf)));

        // Solaris and Mac (and sometimes HPUX) gives much more random data than other platforms.
        // Looks like newer Linuxes return much more random data as well ...
#if !defined(sun) && !defined(macos) && !defined(hpux) && !(defined(PF_DISTRO_SUSE) && PF_MAJOR >= 12)
        std::stringstream ss;
        ss << "sizeof(buf): " << sizeof(buf);
        size_t bytesRead = SCXFile::ReadAvailableBytes(L"/dev/random", buf, sizeof(buf));
        ss << ", bytes read from SCXFile::ReadAvailableBytes(L\"/dev/random\", buf, sizeof(buf)): "
           << bytesRead;
        CPPUNIT_ASSERT_MESSAGE(ss.str(), sizeof(buf) >= bytesRead);
#endif
    }

    void TestNonBlockingReadNonExistingFile() {
        char buf[10];
        CPPUNIT_ASSERT_THROW(SCXFile::ReadAvailableBytes(m_path2, buf, sizeof(buf)), SCXErrnoException);
    }

    void TestNonBlockingReadFileLarger() {
        vector<wstring> lines;
        lines.push_back(L"ABC123");
        SCXFile::WriteAllLinesAsUTF8(m_path1, lines, ios_base::out | ios_base::trunc);
        char buf[10];
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), SCXFile::ReadAvailableBytes(m_path1, buf, 3));
        CPPUNIT_ASSERT_MESSAGE("Read data not the expected data",0 == strncmp("ABC",buf,3));
        // Test with offset:
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), SCXFile::ReadAvailableBytes(m_path1, buf, 3, 3));
        CPPUNIT_ASSERT_MESSAGE("Read data not the expected data",0 == strncmp("123",buf,3));
    }

    void TestNonBlockingReadFileSmaller() {
        vector<wstring> lines;
        lines.push_back(L"ABC123");
        SCXFile::WriteAllLinesAsUTF8(m_path1, lines, ios_base::out | ios_base::trunc);
        char buf[10];
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(6), SCXFile::ReadAvailableBytes(m_path1, buf, sizeof(buf)));
        CPPUNIT_ASSERT_MESSAGE("Read data not the expected data",0 == strncmp("ABC123",buf,6));
    }

    void verifyIsClosed_FILE(FILE *fp, const char* msg) {
        /*
          Behavior of fileno() varies by system.  On some systems, it actually verifies the file (even if
          you've copied the FILE * pointer).  On other systems, it just looks at the local pointer and then
          fileno() returns success even though it's closed.

          On systems where we can verify closure of file, do it.

          We can't try writing to the file: fopen() allocates memory, and the FILE* pointer is actually
          referring to allocated memory.  Thus, if you try to write to the file, you can get memory
          corruption problems (i.e. cores due to double-free errors, etc).
         */

        int f = fileno(fp);

#if defined(macos)  // Mac returns -1 if file is closed but does not set errno
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, f, -1);
#else
        if (f != -1) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, -1, close(f));
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, EBADF, errno);
#endif
    }

    void TestSCXFileHandle() {
        // Open a file for testing
        FILE* fp =  fopen("TestSCXFileHandle.temp", "w");
        CPPUNIT_ASSERT( NULL != fp );

        // Test RAII (resource acquisition is initialization) access
        {
            SCXFileHandle fh(fp);
            CPPUNIT_ASSERT( fh.GetFile() == fp );
        }
        // Destruction of SCXFileHandle above should have closed the stream - verify that
        verifyIsClosed_FILE(fp, "Verify closed using RAII to close");

        // Open the file again ...
        fp =  fopen("TestSCXFileHandle.temp", "w");
        CPPUNIT_ASSERT( NULL != fp );

        // Test CloseFile() explicitly (although, at time of writing, that's called by destructor)
        {
            SCXFileHandle fh(fp);
            CPPUNIT_ASSERT( fh.GetFile() == fp );
            fh.CloseFile();
            CPPUNIT_ASSERT( NULL == fh.GetFile() );
            verifyIsClosed_FILE(fp, "Verify closed after explicit close");
        }

        // Delete the file afterwards ... (if an error occurs, oh well)
        unlink("TestSCXFileHandle.temp");
    }
#endif
};
CPPUNIT_TEST_SUITE_REGISTRATION( SCXFileTest );
