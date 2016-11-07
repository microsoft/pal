/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2011-08-23 13:00

  System information PAL test class

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>

#include <scxsystemlib/scxsysteminfo.h>

#include <cppunit/extensions/HelperMacros.h>

#include <testutils/scxunit.h>

#include <string.h>


// Support for TestGetDefaultSudoPath()
//
// Assume /etc/opt/microsoft/scx/conf/sudodir/sudo for all platforms.

const std::wstring s_defaultSudoPath = L"/etc/opt/microsoft/scx/conf/sudodir/sudo";


using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;


class SystemInfoTestDependencies : public SystemInfoDependencies
{
public:
    char *m_pEnvString;
    bool m_fMockElevated;

    SystemInfoTestDependencies()
        : m_pEnvString(NULL),
          m_fMockElevated(false)
#if defined(aix)
          , m_mockPerfstat(NULL)
#endif
    {
        // Give some reasonable default for SHELL environment variable
        static char bashEnv[] = "/bin/bash";
        m_pEnvString = bashEnv;

        // Determine if we're running with privileges or not
        m_fMockElevated = (0 == ::geteuid());

#if defined(linux)
        // Give a reasonable default for a Linux VM
        m_MockLinuxVM = eNoVmDetected;
#endif // defined(linux)
    }

    // For test, we'd like to change the environment to change the value of
    // SHELL.  However, the state of UNIX is messy with this:
    //
    // 1. On Linux, we have setenv/unsetenv.  In particular, unsetenv will
    //    clear an environment variable so it's not defined at all.
    // 2. Older UNIX platforms (HP 11i V2 and Solaris <= 5.9) do not appear
    //    to support unsetenv (nor setenv).  Instead, these have putenv
    //    (indeed, putenv appears to be everywhere).  With putenv, you pass
    //    a static string (must be static because what you pass becomes part
    //    of the environment) of the form "name=value".  However, this model
    //    leaves no way of removing an envirnonment variable.
    //
    //    Some platfomrs support putenv() with a static string of "name" (no
    //    "=value" portion).  But this doesn't work on Solaris.
    //
    // In the end, for test, I decided to bag putenv entirely.  Instead, I've
    // created a virtual getenv() function that can be overridden here.
    virtual char *getenv(const char *name) const
    {
        // We should only be called to get the value of "SHELL" ...
        CPPUNIT_ASSERT_EQUAL(string("SHELL"), string(name));

        return m_pEnvString;
    }

    // For test: Based on m_fMockElevated, determine if we have privileges
    // or not.  If not, just return some random UID that isn't zero.
    virtual uid_t geteuid() const
    {
        if (m_fMockElevated)
        {
            return 0;
        }
        else
        {
            return 100;         // Return some random UID
        }
    }

#if defined(linux)
    //
    // Test mock for CallCPUID (linux detection of virtual machines)
    //

    LinuxVmType m_MockLinuxVM;      //!< Type of virtual machine to mock

#if !defined(ppc)
    virtual void CallCPUID(
        CpuIdFunction function,
        Registers& registers)
    {
        switch (function)
        {
            case eProcessorInfo:
                switch (m_MockLinuxVM)
                {
                    case eNoVmDetected:
                        registers.m_ecx = 0x0FFFFFFF;   // Bit 31 low -> No virtual machine
                        break;

                    case eDetectedHyperV:
                    case eDetectedVMware:
                    case eDetectedXen:
                    case eUnknownVmDetected:
                    default:
                        registers.m_ecx = 0x8FFFFFFF;   // Bit 31 set -> Virtual machine
                        break;
                }
                break;

            case eHypervisorInfo:
                switch (m_MockLinuxVM)
                {
                    case eNoVmDetected:
                        CPPUNIT_FAIL("Dazed and confused? Call to eHypervisorInfo when not in VM?");
                        break;

                    case eDetectedHyperV:
                        registers.m_ebx = 0x7263694D;   // 'Micr' (in little endian format)
                        registers.m_ecx = 0x666F736F;   // 'osof'
                        registers.m_edx = 0x76482074;   // 't Hv'
                        break;

                    case eDetectedVMware:
                        registers.m_ebx = 0x61774D56;   // 'VMwa' (in little endian format)
                        registers.m_ecx = 0x4D566572;   // 'reVM'
                        registers.m_edx = 0x65726177;   // 'ware'
                        break;

                    case eDetectedXen:
                        registers.m_ebx = 0x566E6558;   // 'XenV' (in little endian format)
                        registers.m_ecx = 0x65584D4D;   // 'MMXe'
                        registers.m_edx = 0x4D4D566E;   // 'nVMM'
                        break;

                    case eUnknownVmDetected:
                    default:
                        registers.m_ebx = registers.m_ecx = registers.m_edx = 0x20202020; // '    '
                        break;
                }
                break;

            case eHyperV_VendorNeutral:
                registers.m_ebx = registers.m_ecx = registers.m_edx = 0;
                registers.m_eax = 0x31237648;           // 'Hv#1' (in little endian format)
                break;

            case eHyperV_FeaturesID:
                registers.m_eax = registers.m_ecx = registers.m_edx = 0;
                registers.m_ebx = 0xFFFFFFFE;           // Bit 1 is clear for FeaturesID call
                break;

            default:
                break;
        }
    }
#endif

#elif defined(aix)
    //
    // Test mock for perfstat_partition_total (AIX detection of virtual machines)
    //

    perfstat_partition_total_t* m_mockPerfstat;  //!< Desired perfstat structure to return

    virtual int perfstat_partition_total(perfstat_id_t* name,
                                         perfstat_partition_total_t* userbuff,
                                         size_t sizeof_struct,
                                         int desired_number)
    {
        //
        // We get called all the time (even when not specifically testing VMs) by the
        // Update() function.  Thus, only mock this if a perfstat structure was previously
        // set.  Otherwise, just call through to the system.
        //

        if ( NULL != m_mockPerfstat )
        {
            CPPUNIT_ASSERT( 0 < desired_number );

            memcpy(userbuff, m_mockPerfstat, sizeof_struct);
            return desired_number;
        }
        else
        {
            return ::perfstat_partition_total(name, userbuff, sizeof_struct, desired_number);
        }
    }
#endif
};

class SCXSystemInfoTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXSystemInfoTest  );
    CPPUNIT_TEST( callDumpString );
    CPPUNIT_TEST( TestGetNativeBitSize );
#if defined(linux) && defined(ppc)
    CPPUNIT_TEST( TestVirtualMachine_True );
#elif defined(linux)
    // Linux-specific tests to verify virtual machine detection
    CPPUNIT_TEST( TestVirtualMachine_Physical );
    CPPUNIT_TEST( TestVirtualMachine_HyperV );
    CPPUNIT_TEST( TestVirtualMachine_VMware );
    CPPUNIT_TEST( TestVirtualMachine_Xen );
    CPPUNIT_TEST( TestVirtualMachine_Unknown );
#elif defined(aix)
    // AIX-specific tests to verify virtual machine detection
    CPPUNIT_TEST( TestVirtualMachine_SharedOnly );
    CPPUNIT_TEST( TestVirtualMachine_DonateOnly );
    CPPUNIT_TEST( TestVirtualMachine_SharedAndDonate );
    CPPUNIT_TEST( TestVirtualMachine_NotSharedAndNotDonate );
#endif // defined(linux)
    CPPUNIT_TEST( TestGetDefaultSudoPath );
    CPPUNIT_TEST( TestGetShellCommandWithNoShellDefined );
    CPPUNIT_TEST( TestGetShellCommandWithEmptyShellDefined );
    CPPUNIT_TEST( TestGetShellCommandWithShellDefined );
    CPPUNIT_TEST( TestGetElevatedCommandWithoutPrivs );
    CPPUNIT_TEST( TestGetElevatedCommandWithoutPrivsAndShell );
    CPPUNIT_TEST( TestGetElevatedCommandWithPrivs );
    CPPUNIT_TEST( TestGetElevatedCommandWithPrivsAndShell );
#if defined(aix)
    CPPUNIT_TEST( TestGetAIX_IsInWPAR );
#endif
#if defined(sun)
    CPPUNIT_TEST( TestGetSUN_IsInGlobalZone );
#endif
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void callDumpString()
    {
        SystemInfo sysInfo;
        wstring dumpOutput = sysInfo.DumpString();

        CPPUNIT_ASSERT(dumpOutput.find(L"SystemInfo") != wstring::npos);
        wcout << L" " << dumpOutput;
    }

    void TestGetNativeBitSize()
    {
        SystemInfo sysInfo;

        unsigned short bitSize = 0;
        int cBitSize = -1;
        CPPUNIT_ASSERT( sysInfo.GetNativeBitSize(bitSize) );

        wcout << L": " << bitSize;

#if defined(linux) || defined(sun)

        char buf[256];

#if defined(linux)
        std::stringstream command("uname -m");
#elif defined(sun)
        std::stringstream command("isainfo -b");
#endif

        FILE *fp = popen(command.str().c_str(), "r");
        CPPUNIT_ASSERT( 0 != fp );
        if (0 != fp)
        {
            // Get response, strip newline
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL);
            if (buf[strlen(buf)-1] == '\n')
            {
                buf[strlen(buf)-1] = '\0';
            }

            pclose(fp);
            wcout << L"(" << buf << L")";

#if defined(linux)
            if (strcmp(buf, "i386") == 0 || strcmp(buf, "i586") == 0 || strcmp(buf, "i686") == 0)
            {
                cBitSize = 32;
            }
            else if (strcmp(buf, "x86_64") == 0 || strcmp(buf, "ppc64le") == 0)
            {
                cBitSize = 64;
            }
#elif defined(sun)
            if (strcmp(buf, "32") == 0)
            {
                cBitSize = 32;
            }
            else if (strcmp(buf, "64") == 0)
            {
                cBitSize = 64;
            }
#endif
        }

#elif defined(macos)
        cBitSize = 0; // To force failure if we don't get expected results

        // sysctl below should output something like: "hw.optional.x86_64: 1"
        const string sysctlName("hw.optional.x86_64");
        string command("sysctl -a 2>/dev/null | grep ");
        FILE *fp = popen((command.append(sysctlName)).c_str(), "r");
        if (0 != fp)
        {
            char buf[256];

            do {
                // Get response, strip newline
                if (NULL == fgets(buf, sizeof(buf), fp) || strlen(buf) == 0)
                    break;

                if (buf[strlen(buf)-1] == '\n')
                {
                    buf[strlen(buf)-1] = '\0';
                }

            } while (0 != strncmp(buf, sysctlName.c_str(), sysctlName.length()));

            pclose(fp);

            if (strlen(buf) > 0)
            {
                std::vector<wstring> parts;
                SCXCoreLib::StrTokenize(SCXCoreLib::StrFromUTF8(buf), parts, L":");

                // If we got what we expected, set the resultant bit size
                if (parts[0] == SCXCoreLib::StrFromUTF8(sysctlName))
                {
                    if (parts[1] == L"1")
                    {
                        cBitSize = 64;
                    }
                    else if (parts[1] == L"0")
                    {
                        cBitSize = 32;
                    }
                }
            }
            else
            {
                // Did not find value in sysctl, that indicates a 32-bit system
                cBitSize = 32;
            }
        }

#else

        cBitSize = 64;

#endif

        CPPUNIT_ASSERT( bitSize == cBitSize );
    }

#if defined(linux) || defined(aix)
    //
    // Not a test, but a helper function to easily retrieve a Virtual Machine state
    //
    eVmType GetVMState(SystemInfo& si)
    {
        eVmType vmType;
        bool result = si.GetVirtualMachineState(vmType);
        CPPUNIT_ASSERT( true == result );
        return vmType;
    }
#endif // defined(linux) || defined(aix)

#if defined(linux) && defined(ppc)
    void TestVirtualMachine_True()
    {
        SystemInfo si;
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
    }
#elif defined(linux)
    //
    // Linux-specific tests to verify virtual machine detection
    //

    void TestVirtualMachine_Physical()
    {
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_MockLinuxVM = eNoVmDetected;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmNotDetected, (int) GetVMState(si) );
        CPPUNIT_ASSERT_EQUAL( (int) eNoVmDetected, (int) deps->DetermineLinuxVirtualMachineState() );
    }

    void TestVirtualMachine_HyperV()
    {
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_MockLinuxVM = eDetectedHyperV;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
        CPPUNIT_ASSERT_EQUAL( (int) eDetectedHyperV, (int) deps->DetermineLinuxVirtualMachineState() );
    }

    void TestVirtualMachine_VMware()
    {
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_MockLinuxVM = eDetectedVMware;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
        CPPUNIT_ASSERT_EQUAL( (int) eDetectedVMware, (int) deps->DetermineLinuxVirtualMachineState() );
    }

    void TestVirtualMachine_Xen()
    {
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_MockLinuxVM = eDetectedXen;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
        CPPUNIT_ASSERT_EQUAL( (int) eDetectedXen, (int) deps->DetermineLinuxVirtualMachineState() );
    }

    void TestVirtualMachine_Unknown()
    {
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_MockLinuxVM = eUnknownVmDetected;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmUnknown, (int) GetVMState(si) );
        CPPUNIT_ASSERT_EQUAL( (int) eUnknownVmDetected, (int) deps->DetermineLinuxVirtualMachineState() );
    }
#elif defined(aix)
    void TestVirtualMachine_SharedOnly()
    {
        perfstat_partition_total_t lparStats;
        memset(&lparStats, 0, sizeof(lparStats));
        lparStats.type.b.shared_enabled = 1;

        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_mockPerfstat = &lparStats;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
    }

    void TestVirtualMachine_DonateOnly()
    {
        perfstat_partition_total_t lparStats;
        memset(&lparStats, 0, sizeof(lparStats));
        lparStats.type.b.donate_enabled = 1;

        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_mockPerfstat = &lparStats;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
    }

    void TestVirtualMachine_SharedAndDonate()
    {
        perfstat_partition_total_t lparStats;
        memset(&lparStats, 0, sizeof(lparStats));
        lparStats.type.b.shared_enabled = lparStats.type.b.donate_enabled = 1;

        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_mockPerfstat = &lparStats;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmDetected, (int) GetVMState(si) );
    }

    void TestVirtualMachine_NotSharedAndNotDonate()
    {
        perfstat_partition_total_t lparStats;
        memset(&lparStats, 0, sizeof(lparStats));

        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_mockPerfstat = &lparStats;

        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL( (int) eVmUnknown, (int) GetVMState(si) );
    }
#endif // defined(linux)

    void TestGetDefaultSudoPath()
    {
        SystemInfo si;

        // Assume /usr/bin/sudo for all platforms except AIX and Solaris 8
        // (Support for this is in #define's at the top of the file)
        //
        // Note that we only test on the current platform.  Since the build is run
        // on all platforms, a full build on all platforms will test all platforms.

        CPPUNIT_ASSERT_EQUAL(StrToUTF8(s_defaultSudoPath),
                             StrToUTF8(si.GetDefaultSudoPath()));
    }

    void TestGetShellCommandWithNoShellDefined()
    {
        // Clear our default shell (in our mocked getenv() method)
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_pEnvString = NULL;

        // Verify that the shell properly defaults to sh
        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL(string("/bin/sh -c \"ls -lR\""),
                             StrToUTF8(si.GetShellCommand(L"ls -lR")));
    }

    void TestGetShellCommandWithEmptyShellDefined()
    {
        // Clear our default shell (in our mocked getenv() method)
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        static char emptyEnv[] = "";
        deps->m_pEnvString = emptyEnv;

        // Verify that the shell properly defaults to sh
        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL(string("/bin/sh -c \"ls -lR\""),
                             StrToUTF8(si.GetShellCommand(L"ls -lR")));
    }

    void TestGetShellCommandWithShellDefined()
    {
        // Set a new shell (in our mocked getenv() method)
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        static char kshEnv[] = "/bin/ksh";
        deps->m_pEnvString = kshEnv;

        // Verify that GetShellCommand() uses the ksh shell
        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL(string("/bin/ksh -c \"ls -lR\""),
                             StrToUTF8(si.GetShellCommand(L"ls -lR")));
    }

    void TestGetElevatedCommandWithoutPrivs()
    {
        // Mock to be running without privileges
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_fMockElevated = false;

        // Build a string that should be the result of GetElevatedCommand() ...
        SystemInfo si(deps);
        wstring expectedCommand(s_defaultSudoPath);
        expectedCommand.append(L" ls -lR");

        CPPUNIT_ASSERT_EQUAL(StrToUTF8(expectedCommand),
                             StrToUTF8(si.GetElevatedCommand(wstring(L"ls -lR"))));
    }

    void TestGetElevatedCommandWithoutPrivsAndShell()
    {
        // Mock to be running without privileges
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        static char kshEnv[] = "/bin/ksh";
        deps->m_pEnvString = kshEnv;
        deps->m_fMockElevated = false;

        // Test TestGetElevatedCommandWithoutPrivs() tests the specific command
        // Here, we just make sure that, non-elevated, result is identical even
        // with an unusual shell in the environment ...
        SystemInfo si(deps);
        wstring expectedCommand(s_defaultSudoPath);
        expectedCommand.append(L" ls -lR");

        CPPUNIT_ASSERT_EQUAL(StrToUTF8(expectedCommand),
                             StrToUTF8(si.GetElevatedCommand(wstring(L"ls -lR"))));
    }

    void TestGetElevatedCommandWithPrivs()
    {
        // Mock to be running with privileges
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_fMockElevated = true;

        // If we're elevated, input and output commands should be identical ...
        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL(string("ls -lR"), StrToUTF8(si.GetElevatedCommand(L"ls -lR")));
    }

    void TestGetElevatedCommandWithPrivsAndShell()
    {
        // Mock to be running with privileges
        SCXHandle<SystemInfoTestDependencies> deps(new SystemInfoTestDependencies());
        deps->m_fMockElevated = true;

        // Test TestGetElevatedCommandWithPrivs() tests the specific command.
        // Here, make sure that forcing a shell gives us a shell ...
        SystemInfo si(deps);
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(si.GetShellCommand(L"ls -lR")),
                             StrToUTF8(si.GetElevatedCommand(si.GetShellCommand(L"ls -lR"))));
    }

#if defined(aix)
    static void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " \n")
    {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos = str.find_first_of(delimiters, lastPos);

        while (std::string::npos != pos || std::string::npos != lastPos)
            {
                tokens.push_back(str.substr(lastPos, pos - lastPos));
                lastPos = str.find_first_not_of(delimiters, pos);
                pos = str.find_first_of(delimiters, lastPos);
            }
    }

    void TestGetAIX_IsInWPAR()
    {
        // First determine if SystemInfo thinks we're in a WPAR

        SystemInfo sysInfo;
        bool fIsSupported, fInWPAR;
        fIsSupported = sysInfo.GetAIX_IsInWPAR( fInWPAR );

        // WPARs exist on AIX 6 forward ...
#if PF_MAJOR >= 6
        CPPUNIT_ASSERT( fIsSupported );
#else
        CPPUNIT_ASSERT( !fIsSupported );
#endif // PF_MAJOR >= 6

        // The 'vmstat -v' system routine displays an "@" in a WPAR
        // for many of the statistics - we just pick "memory pages"
        // So, on a WPAR, we get a line like:
        //   1507328 @ memory pages
        // On a regular system, we get a line like:
        //   1507328 memory pages

        FILE* file = popen("vmstat -v", "r");
        if (file)
        {
            char buf[256];
            bool fMemoryPagesFound = false;
            for (char *o = fgets(buf, 256, file); o != NULL; o = fgets(buf, 256, file))
            {
                std::string output = buf;

                std::vector<std::string> tokens;
                Tokenize(output, tokens);

                if (tokens.size() > 2)
                {
                    if (("memory" == tokens[1] && "pages" == tokens[2])
                        || ("@" == tokens[1] && "memory" == tokens[2] && "pages" == tokens[3]))
                    {
                        fMemoryPagesFound = true;
                        CPPUNIT_ASSERT( fInWPAR == ("@" == tokens[1]) );
                    }
                }
            }
            pclose(file);
            CPPUNIT_ASSERT( fMemoryPagesFound );
        }
        else
        {
            CPPUNIT_ASSERT( ! "Unable to execute vmstat command!" );
        }
    }
#endif

#if defined(sun)
    void TestGetSUN_IsInGlobalZone()
    {
        // First determine if SystemInfo thinks we're in global zone

        SystemInfo sysInfo;
        bool fIsSupported, fInGlobalZone;
        fIsSupported = sysInfo.GetSUN_IsInGlobalZone( fInGlobalZone );

#if PF_MAJOR > 5 || (PF_MAJOR == 5 && PF_MINOR >= 10)
        CPPUNIT_ASSERT( fIsSupported );

        // Zones supported, so verify that command-line agrees w/implementation
        FILE* file = popen("zonename", "r");
        if (file)
        {
            char buf[256];
            char *result = fgets(buf, sizeof(buf), file);
            CPPUNIT_ASSERT( result != NULL );

            // Strip '\n' line termination if it exists
            if (strlen(buf) > 1 && buf[strlen(buf)-1] == '\n')
            {
                buf[strlen(buf)-1] = '\0';
            }

            if (fInGlobalZone)
            {
                std::string errText = "Expected \"global\", but received \"";
                errText.append(result).append("\"");
                CPPUNIT_ASSERT_MESSAGE( errText, 0 == strncmp(result, "global", sizeof(buf)) );
            }
            else
            {
                CPPUNIT_ASSERT_MESSAGE( "Did not expect \"global\", but received it", 0 != strncmp(result, "global", sizeof(buf)) );
            }
        }
        else
        {
            CPPUNIT_ASSERT( ! "Unable to execute 'zonename' command!" );
        }
#else
        // Zones not supported, so verify that we report we're in the global zone
        CPPUNIT_ASSERT( !fIsSupported );
        CPPUNIT_ASSERT( fInGlobalZone );
#endif
    }
#endif // defined(sun)
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXSystemInfoTest );
