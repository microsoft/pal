/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Process PAL tests.

    \date        2007-10-12 15:43:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxthread.h>
#include <testutils/scxunit.h>
#include <testutils/scxtestutils.h>
#include <scxcorelib/scxexception.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>
#include <iostream>
#include <sstream>

// For definition of getpid
#if defined(SCX_UNIX)
#include <unistd.h>
#include <errno.h>
#include <time.h>  // time() definition, along with time_t
#endif

#if defined(SCX_UNIX)
namespace {
    class SCXProcessTestDouble : public SCXCoreLib::SCXProcess
    {
    public:
        bool m_WroteZeroLength;
        int m_ForceWriteErrno;

        SCXProcessTestDouble(const std::wstring &command,
                             const SCXCoreLib::SCXFilePath& cwd = SCXCoreLib::SCXFilePath(),
                             const SCXCoreLib::SCXFilePath& chrootPath = SCXCoreLib::SCXFilePath())
            : SCXCoreLib::SCXProcess(SplitCommand(command), cwd, chrootPath)
        {
            m_WroteZeroLength = false;
            m_ForceWriteErrno = 0;
        }

    protected:
        virtual ssize_t DoWrite(int fd, const void* buf, size_t size)
        {
            if (0 == size) 
            {
                m_WroteZeroLength = true;
            }
            if (m_ForceWriteErrno != 0)
            {
                errno = m_ForceWriteErrno;
                return -1;
            }
            return SCXProcess::DoWrite(fd, buf, size);
        }
    };
}
#endif

class SCXProcessTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXProcessTest );

    CPPUNIT_TEST( TestCurrentProcessID );
    CPPUNIT_TEST( TestSplitCommand );
#if defined(SCX_UNIX)
    CPPUNIT_TEST( TestRunOut );
    CPPUNIT_TEST( TestRunErr );
    CPPUNIT_TEST( TestRunInputFree );
    CPPUNIT_TEST( TestKill );
    CPPUNIT_TEST( TestKillGroup );
    CPPUNIT_TEST( TestLongTimeout );
    CPPUNIT_TEST( TestShortTimeout );
    CPPUNIT_TEST( TestTimeoutWithChildren );
    CPPUNIT_TEST( TestUnnecessaryFileDescriptorsAreClosed );
    CPPUNIT_TEST( WritingZeroBytesToProcessShouldNeverHappen );
    CPPUNIT_TEST( WritingToProcessWithClosedStdinShouldNotFail );
    CPPUNIT_TEST( VerifyParsing_WI_421069 );
    CPPUNIT_TEST( TestCompareVector );
    CPPUNIT_TEST( TestFakeCommand );
    CPPUNIT_TEST( TestWriteRaceCondition );
    CPPUNIT_TEST( TestSplitCommand_Empty );
    CPPUNIT_TEST( TestSplitCommand_NoSpaces );
    CPPUNIT_TEST( TestSplitCommand_SpacesNoQuotes );
    CPPUNIT_TEST( TestSplitCommand_SpacesUnescapedQuotes );
    CPPUNIT_TEST( TestSplitCommand_SpacesAndEscapedQuotes );
    CPPUNIT_TEST( TestSplitCommand_CheckCimServer );
    CPPUNIT_TEST( TestSplitCommand_CheckAwk );
    CPPUNIT_TEST( TestSplitCommand_CheckOracle );
    CPPUNIT_TEST( TestSplitCommand_NestedQuotes );
    CPPUNIT_TEST( TestSplitCommand_NestedQuotes_Apos );
    CPPUNIT_TEST( TestSplitCommand_EscapedQuotes );
    CPPUNIT_TEST( TestSplitCommand_EscapesInApostrophe );
    CPPUNIT_TEST( TestSplitCommand_NoSwallowBackslashes_1 );
    CPPUNIT_TEST( TestSplitCommand_NoSwallowBackslashes_InProcess_1 );
    CPPUNIT_TEST( TestSplitCommand_NoSwallowBackslashes_2 );
    CPPUNIT_TEST( TestSplitCommand_NoSwallowBackslashes_InProcess_2 );
#endif
    SCXUNIT_TEST_ATTRIBUTE(TestRunInputFree, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestLongTimeout, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestShortTimeout, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestTimeoutWithChildren, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(WritingToProcessWithClosedStdinShouldNotFail, SLOW);
    CPPUNIT_TEST_SUITE_END();
private:


public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {

    }

    bool doesCalcExist(std::wstring testName)
    {
        if (SCXCoreLib::SCXFile::Exists(L"/usr/bin/bc"))
        {
            return true;
        }

        std::wstring warnText;

        warnText = L"Utility /usr/bin/bc must exist to run " + testName + L" test";
        SCXUNIT_WARNING(warnText);
        return false;
    }

    void TestCurrentProcessID(void)
    {
#if defined(WIN32)
        SCXCoreLib::SCXProcessId native = GetCurrentProcessId();
#elif defined(SCX_UNIX)
        SCXCoreLib::SCXProcessId native = getpid();
#else
#error Not implemented on this platform
#endif
        CPPUNIT_ASSERT_EQUAL(native, SCXCoreLib::SCXProcess::GetCurrentProcessID());
    }

#if defined(SCX_UNIX)
    void TestRunOut()
    {
        if (doesCalcExist(L"TestRunOut"))
        {
            std::istringstream input("1+2\nquit\n");
            std::ostringstream output;
            std::ostringstream error;
            SCXCoreLib::SCXProcess::Run(L"/usr/bin/bc", input, output, error);
            CPPUNIT_ASSERT(output.str() == "3\n");
        }
    }

    void TestRunErr()
    {
        if (doesCalcExist(L"TestRunErr"))
        {
            std::istringstream input("1/0\nquit\n");
            std::ostringstream output;
            std::ostringstream error;
            SCXCoreLib::SCXProcess::Run(L"/usr/bin/bc", input, output, error);
            std::string::size_type bypos = error.str().find("by");
            CPPUNIT_ASSERT(bypos > 0);
        }
    }

    void TestRunInputFree() {
#if defined(linux)
        const std::wstring psPath(L"/bin/ps");
#else
        const std::wstring psPath(L"/usr/bin/ps");
#endif
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(psPath, input, output, error);
        std::string::size_type pathpos = output.str().find("ps");
        CPPUNIT_ASSERT(pathpos > 0);
    }
#endif /*SCX_UNIX*/

    void TestSplitCommand() {
        std::vector<std::wstring> cmd1 = SCXCoreLib::SCXProcess::SplitCommand(L"ls");
        CPPUNIT_ASSERT(cmd1.at(0) == L"ls");
        CPPUNIT_ASSERT(cmd1.size() == 1);

        std::vector<std::wstring> cmd2 = SCXCoreLib::SCXProcess::SplitCommand(L" ls ");
        CPPUNIT_ASSERT(cmd2.at(0) == L"ls");
        CPPUNIT_ASSERT(cmd2.size() == 1);

        std::vector<std::wstring> cmd3 = SCXCoreLib::SCXProcess::SplitCommand(L"ls \" kalle olle\" pelle");
        CPPUNIT_ASSERT(cmd3.at(0) == L"ls");
        CPPUNIT_ASSERT(cmd3.at(1) == L" kalle olle");
        CPPUNIT_ASSERT(cmd3.at(2) == L"pelle");
        CPPUNIT_ASSERT(cmd3.size() == 3);

        std::vector<std::wstring> cmd4 = SCXCoreLib::SCXProcess::SplitCommand(L"ls 'kalle'pelle'olle'");
        CPPUNIT_ASSERT(cmd4.at(0) == L"ls");
        CPPUNIT_ASSERT(cmd4.at(1) == L"kallepelleolle");
        CPPUNIT_ASSERT(cmd4.size() == 2);

        std::vector<std::wstring> cmd5 = SCXCoreLib::SCXProcess::SplitCommand(L"ls 'kalle\"pelle\"olle'");
        CPPUNIT_ASSERT(cmd5.at(0) == L"ls");
        CPPUNIT_ASSERT(cmd5.at(1) == L"kalle\"pelle\"olle");
        CPPUNIT_ASSERT(cmd5.size() == 2);
    }

#if defined(SCX_UNIX)
    void TestKill() {
        if (doesCalcExist(L"TestKill"))
        {
            std::vector<std::wstring> argv;
            argv.push_back(L"/usr/bin/bc");
            SCXCoreLib::SCXProcess process(argv);
            CPPUNIT_ASSERT_NO_THROW(process.Kill());
            SCXUNIT_ASSERT_THROWN_EXCEPTION(process.WaitForReturn(), SCXCoreLib::SCXInterruptedProcessException, L"interrupted");
        }
    }

    void TestKillGroup() {
        const unsigned int c_waitIterations = 100;
        const unsigned int c_waitIterationMS = 100;
        
        // Start a process that starts another process; when that second process begins, it writes its pid to a file in the
        // testfiles directory. We then read that file to get the pid, and assert that 'ps -p PID' returns 0 (i.e. it finds the process).
        // Then we kill the process that we started, and assert that 'ps -p PID' returns 1 (i.e. the subprocess is no longer alive).
        std::vector<std::wstring> argv;
        argv.push_back(L"./testfiles/killgrouptest.sh");
        SCXCoreLib::SCXProcess process(argv);

        // Wait for the pid file to get created.  When this is created, the subprocess we will later kill should be alive.
        unsigned int count = 0;
        std::wstring pidfile  = L"./testfiles/killgrouptest_hang.pid";
        while (!SCXCoreLib::SCXFile::Exists(pidfile))
        {
            if (++count > c_waitIterations)
            {
                CPPUNIT_FAIL("killgrouptest_hang.pid is not being created in time (or at all) for this unit test.");
                return;
            }     
            SCXCoreLib::SCXThread::Sleep(c_waitIterationMS);
        }

        // Sleep for a small amount of time so that our slower systems can actually write (not just create) this pidfile.
        SCXCoreLib::SCXThread::Sleep(500);

        SCXCoreLib::SelfDeletingFilePath pidfileDelete(pidfile);
        
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        int returnCode;

        // Get the pid of the subprocess so we can check on its status.        
        std::vector<std::wstring> lines;
        SCXCoreLib::SCXStream::NLFs nlfs;
        SCXCoreLib::SCXFile::ReadAllLines(SCXCoreLib::SCXFilePath(L"./testfiles/killgrouptest_hang.pid"), lines, nlfs);
        std::wstring pid = lines[0];
        
        // Assert that the subprocess is currently running.
        std::wstring ps_command = L"ps -p " + pid;
        returnCode = SCXCoreLib::SCXProcess::Run(ps_command, input, output, error, 10000);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Command \"" + SCXCoreLib::StrToUTF8(ps_command) + "\" returned an unexpected value", 0, returnCode);

        // Kill the process group.
        CPPUNIT_ASSERT_NO_THROW(process.Kill());
        SCXUNIT_ASSERT_THROWN_EXCEPTION(process.WaitForReturn(), SCXCoreLib::SCXInterruptedProcessException, L"interrupted");

        // Wait for the system to clean up the processes and remove them from the process list.  If it doesn't, then fail this unit test.
        input.str("");
        output.str("");
        error.str("");
        count = 0;  
        while ((returnCode = SCXCoreLib::SCXProcess::Run(ps_command, input, output, error, 10000)) != 1)
        {
            if (++count > c_waitIterations)
            {
                CPPUNIT_FAIL("Process group associated with killgrouptest_hang.sh was not killed.");
                return;
            }     
            SCXCoreLib::SCXThread::Sleep(c_waitIterationMS);
            input.str("");
            output.str("");
            error.str("");
        }
    }

    void TestLongTimeout()
    {
        if (doesCalcExist(L"TestLongTimeout"))
        {
            try
            {
                std::istringstream input("1+2\nquit\n");
                std::ostringstream output;
                std::ostringstream error;
                int returnCode = SCXCoreLib::SCXProcess::Run(L"/usr/bin/bc", input, output, error, 10000);
                CPPUNIT_ASSERT(returnCode == 0);
                CPPUNIT_ASSERT(output.str() == "3\n");
            }
            catch (SCXCoreLib::SCXException &e)
            {
                std::wcout << L"Exception " << e.What() << L"  " << e.Where() << L"\n";
            }
        }
    }

    void TestShortTimeout()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        std::vector<std::wstring> myargv;
        myargv.push_back(L"/usr/bin/wc");
        CPPUNIT_ASSERT_THROW(SCXCoreLib::SCXProcess::Run(myargv, input, output, error, 1),
                             SCXCoreLib::SCXInterruptedProcessException);
    }

    // WI 52359: Launch process with children, be sure that timeout is honored
    //
    // Note: This seems to be fixed already, likely in WI 46406 & WI 46509.
    // This test verifies that this behavior continues to work, and to give
    // us a unit test if the above WI's are ported to v1 CU6 (or something).
    void TestTimeoutWithChildren()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        std::vector<std::wstring> myargv;
        myargv.push_back(L"sleep");
        myargv.push_back(L"60");

        // Get the current system time
        time_t startTime = time(NULL);

        // Run the test, aborting in 500ms (well before the 60 second sleep timeout)
        CPPUNIT_ASSERT_THROW(SCXCoreLib::SCXProcess::Run(myargv, input, output, error, 500),
                             SCXCoreLib::SCXInterruptedProcessException);

        // Verified that we finished within relatively short period of time
        time_t endTime = time(NULL);
        CPPUNIT_ASSERT( endTime <= (startTime + 2) );
    }

    void TestUnnecessaryFileDescriptorsAreClosed()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(L"./testfiles/closefiledescriptors", input, output, error);
        CPPUNIT_ASSERT(output.str() == "0\n");
    }

    void WritingZeroBytesToProcessShouldNeverHappen()
    {
        if (doesCalcExist(L"WritingZeroBytesToProcessShouldNeverHappen"))
        {
            std::istringstream input("40+2\nquit\n");
            std::ostringstream output;
            std::ostringstream error;
            SCXProcessTestDouble process(L"/usr/bin/bc");
            SCXCoreLib::SCXProcess::Run(process, input, output, error);
            CPPUNIT_ASSERT_MESSAGE("SCXProcess implementation called write with zero length.", ! process.m_WroteZeroLength);
        }
    }

    // This test does not work on all platforms for some reason. WI 13164
    void WritingToProcessWithClosedStdinShouldNotFail()
    {
        if (doesCalcExist(L"WritingToProcessWithClosedStdinShouldNotFail"))
        {
            std::istringstream input("40+2\nquit\n");
            std::ostringstream output;
            std::ostringstream error;
            SCXProcessTestDouble process(L"/usr/bin/bc");
            process.m_ForceWriteErrno = EPIPE; // This is assuming SIGPIPE is handled.
            CPPUNIT_ASSERT_THROW_MESSAGE("SCXProcess implementation did not handle EPIPE errors.", 
                                         SCXCoreLib::SCXProcess::Run(process, input, output, error, 2*1000),
                                         SCXCoreLib::SCXInterruptedProcessException);
        }
    }

    void VerifyParsing_WI_421069()
    {
        // We need a temporary file for output of command
        std::wstring myTempFile = L"./testfiles/index.php";
        SCXCoreLib::SelfDeletingFilePath sdel(myTempFile);

        std::wostringstream cmdStream;
        cmdStream << "/bin/sh -c \"echo \\\"<?php phpinfo();?>\\\" > " << myTempFile << "\"";

        // Gp run the command in cmdStream
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(cmdStream.str(), input, output, error);
        CPPUNIT_ASSERT( !output.str().length() );
        CPPUNIT_ASSERT( !error.str().length() );

        // Verify the contents of the file created by the above command
        SCXCoreLib::SCXProcess::Run(L"cat " + myTempFile, input, output, error);
        CPPUNIT_ASSERT( !error.str().length() );
        CPPUNIT_ASSERT_EQUAL( std::string("<?php phpinfo();?>"),
                              SCXCoreLib::StrToUTF8(
                                  SCXCoreLib::StrTrimR(
                                      SCXCoreLib::StrFromUTF8(output.str() ))) );
    }

    //
    // The following are parsing tests for SCXProcess::SplitCommand
    //

    // Utility function to compare that a vector matches with the argumetns passed
    void CompareVector(std::vector<std::wstring> vec, const char *expectedArgs[])
    {
        for (size_t i = 0; i < vec.size(); i++)
        {
            const char *theArg = expectedArgs[i];
            CPPUNIT_ASSERT_EQUAL_MESSAGE( SCXCoreLib::StrToUTF8(
                                              SCXCoreLib::StrAppend(L"Difference found in element ", i) ),
                                          std::string(theArg), SCXCoreLib::StrToUTF8(vec[i]) );
        }
    }

    // Test that CompareVector does the right thing
    void TestCompareVector()
    {
        std::vector<std::wstring> vec;

        const char *expectedEmpty[1] = { };
        CompareVector( vec, expectedEmpty );

        const char *expectedOne[] = { "One Element" };
        vec.push_back( L"One Element" );
        CompareVector( vec, expectedOne );
        vec.clear();

        const char *expectedFive[] = { "One", "Two", "Three", "Four", "Five" };
        vec.push_back( L"One" );
        vec.push_back( L"Two" );
        vec.push_back( L"Three" );
        vec.push_back( L"Four" );
        vec.push_back( L"Five" );
        CompareVector( vec, expectedFive );
    }

    void TestFakeCommand()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        int ret = SCXCoreLib::SCXProcess::Run(L"fakeCommand123 -i", input, output, error);
        CPPUNIT_ASSERT_MESSAGE("Return code indicates success although it shouldn't", ret != 0);
        // Do we fail for the good reason
        CPPUNIT_ASSERT_MESSAGE("Could not find the expected failure reason on stderr: " + error.str(),
            error.str().find("Failed to start child process") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE("Unexpected output on stdout: " + output.str(), output.str().size() == 0);
    }

    void TestWriteRaceCondition()
    {
        // On some slower systems, the child process can die between the time we poll the stdin pipe
        // to validate it and the time we write to it creating a race condition.
        // We will execute the SIGPIPE test multiple times to increase our chances of catching it.
        for (int i = 0; i <= 10; i++)
        {
            std::istringstream input2("anything");
            std::ostringstream output2, error2;
            // This next line used to cause a sigpipe. If not then the test passes!
            int ret = SCXCoreLib::SCXProcess::Run(L"fakeCommand123", input2, output2, error2);

            CPPUNIT_ASSERT_MESSAGE("Return code indicates success although it shouldn't", ret != 0);
        }
    }

    //
    // Some (very) basic tests for obvious functionality
    //
    void TestSplitCommand_Empty()
    {
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"" );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (0), vec.size() );
    }

    void TestSplitCommand_NoSpaces()
    {
        const char *expected[] = { "RandomText" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"RandomText" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (1), vec.size() );
    }

    void TestSplitCommand_SpacesNoQuotes()
    {
        const char *expected[] = { "One", "Two", "Three" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"One Two Three" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_SpacesUnescapedQuotes()
    {
        const char *expected[] = { "One", "Two", "Three Four", "Five" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"One Two \"Three Four\" Five" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (4), vec.size() );
    }

    // This is just a repeat of WI 421069, but calling SplitCommand directly
    // (It's tested above too, via the SCXProcess::Run() interface)
    void TestSplitCommand_SpacesAndEscapedQuotes()
    {
        const char *expected[] = { "/bin/sh", "-c", "echo \"<?php phpinfo();?>\" > /tmp/somefile" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"echo \\\"<?php phpinfo();?>\\\" > /tmp/somefile\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    //
    // The following three tests are incresingly ugly examples known to be used in the wild (by customers)
    //
    void TestSplitCommand_CheckCimServer()
    {
        const char *expected[] = { "/bin/sh", "-c", "pid=`ps -eo pid -o cmd | grep -v grep | grep -m 1 scxcimserver | awk '{print $1}'` && [ -f /proc/$pid/stat ] && expr `date +%s` - `stat -c %Z /proc/$pid/stat` || echo '0'" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"pid=`ps -eo pid -o cmd | grep -v grep | grep -m 1 scxcimserver | awk '{print $1}'` && [ -f /proc/$pid/stat ] && expr `date +%s` - `stat -c %Z /proc/$pid/stat` || echo '0'\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_CheckAwk()
    {
        const char *expected[] = { "/bin/sh", "-c", "awk 'BEGIN { if ((1 == getline < \"/apps/auto/malice/etc/malice.sys\") || (1 == getline < \"/etc/FFO_Role\")) print \"1\"; else print \"0\"}'" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"awk 'BEGIN { if ((1 == getline < \\\"/apps/auto/malice/etc/malice.sys\\\") || (1 == getline < \\\"/etc/FFO_Role\\\")) print \\\"1\\\"; else print \\\"0\\\"}'\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_CheckOracle()
    {
        const char *expected[] = { "/bin/sh", "-c", "ORACLE_HOME=$Config/OracleHome$;export ORACLE_HOME;ORACLE_SID=$Config/OracleSID$;export ORACLE_SID;sqllogin=`cat /tmp/manage-X/mxosqp`;FSTMPVAR=`printf 'SET HEADING OFF;\nselect distinct destination from v$archive_dest where destination like '\''%%/%%'\''  and status='\''VALID'\'';'   |$Config/OracleHome$/bin/sqlplus -S $sqllogin|grep /`;if [ -z $FSTMPVAR ]; then FSTMPVAR=\"/null\";fi;df -Pk $FSTMPVAR|grep /|awk '{ print $6}'" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"ORACLE_HOME=$Config/OracleHome$;export ORACLE_HOME;ORACLE_SID=$Config/OracleSID$;export ORACLE_SID;sqllogin=`cat /tmp/manage-X/mxosqp`;FSTMPVAR=`printf 'SET HEADING OFF;\nselect distinct destination from v$archive_dest where destination like '\''%%/%%'\''  and status='\''VALID'\'';'   |$Config/OracleHome$/bin/sqlplus -S $sqllogin|grep /`;if [ -z $FSTMPVAR ]; then FSTMPVAR=\\\"/null\\\";fi;df -Pk $FSTMPVAR|grep /|awk '{ print $6}'\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_NestedQuotes()
    {
        const char *expected[] = { "/bin/sh", "-c", "echo \"'This is one arg'\"" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"echo \\\"'This is one arg'\\\"\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_NestedQuotes_Apos()
    {
        const char *expected[] = { "/bin/sh", "-c", "echo '\"This is one arg\"'" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"echo '\\\"This is one arg\\\"'" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (3), vec.size() );
    }

    void TestSplitCommand_EscapedQuotes()
    {
        const char *expected[] = { "/bin/sh", "-c", "echo '\\\"a b c\\\"'", "3", "more", "parts" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"/bin/sh -c \"echo '\\\\\\\"a b c\\\\\\\"'\" 3 more parts" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (6), vec.size() );
    }

    void TestSplitCommand_EscapesInApostrophe()
    {
        const char *expected[] = { "echo", "\\First Second\\" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"echo '\\First Second\\'" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (2), vec.size() );
    }

    void TestSplitCommand_NoSwallowBackslashes_1()
    {
        // Testing command like: sh -c "echo "'\\\"%%/%%\"\\' 3 more parts

        const char *expected[] = { "echo", "\\\"%%/%%\\\"", "3", "more", "parts" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"echo '\\\"%%/%%\\\"' 3 more parts" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (5), vec.size() );
    }

    void TestSplitCommand_NoSwallowBackslashes_InProcess_1()
    {
        // Run the above test (TestSplitCommand_NoSwallowBackslashes_1()) in shell to verify output
        // Note that due to quoting within a string of the compiler, we have lots of extra \\ characters

        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(L"/bin/sh -c \"echo \"'\\\\\\\"%%/%%\\\"\\\\'", input, output, error);
        CPPUNIT_ASSERT_EQUAL( std::string(""), error.str() );
        CPPUNIT_ASSERT_EQUAL( std::string("\\\"%%/%%\"\\\n"), output.str() );
    }

    void TestSplitCommand_NoSwallowBackslashes_2()
    {
        // Testing command like: sh -c "echo "'\"'"%%/%%"'\"'" \!= \'%%/%%\'"
        // (Should output: "%%/%%" != '%%/%%')

        const char *expected[] = { "echo", "\"%%/%%\" != \'%%/%%\'" };
        std::vector<std::wstring> vec = SCXCoreLib::SCXProcess::SplitCommand( L"echo '\"%%/%%\"'\" != \'%%/%%\'\"" );
        CompareVector( vec, expected );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t> (2), vec.size() );
    }

    void TestSplitCommand_NoSwallowBackslashes_InProcess_2()
    {
        // Run the above test (TestSplitCommand_NoSwallowBackslashes_2()) in shell to verify output
        // Note that due to quoting within a string of the compiler, we have lots of extra \\ characters

        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(L"/bin/sh -c \"echo \"'\\\"%%/%%\\\"'\" != \\\'%%/%%\\\'\"", input, output, error);
        CPPUNIT_ASSERT_EQUAL( std::string(""), error.str() );
        CPPUNIT_ASSERT_EQUAL( std::string("\"%%/%%\" != '%%/%%'\n"), output.str() );
    }
#endif /* SCX_UNIX */
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXProcessTest );
