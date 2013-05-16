/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test cases for the OS Info code

    \date        2009-05-11 11:11:54

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>

#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxprocess.h>
#include <scxsystemlib/scxostypeinfo.h> 
#include <scxsystemlib/scxsysteminfo.h>
#include <testutils/scxunit.h> /* This will include CPPUNIT helper macros too */
#include <testutils/scxtestutils.h>

#include <errno.h>

using namespace std;
using namespace SCXSystemLib;
using namespace SCXCoreLib;

class SCXOSTypeInfoTestDependencies : public SCXOSTypeInfoDependencies
{
public:
    SCXOSTypeInfoTestDependencies() {};

#if defined(PF_DISTRO_ULINUX)
    const wstring getScriptPath(void) const
    {
        return L"./testfiles/GetLinuxOS.sh";
    }

    const wstring getReleasePath() const
    {
        return L"./scx-release";
    }
#endif // defined(PF_DISTRO_ULINUX)
};

class SCXOSTypeInfoTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXOSTypeInfoTest ); 
    CPPUNIT_TEST( TestOSName ); 
    CPPUNIT_TEST( TestGetOSNameCompatFlag );
    CPPUNIT_TEST( TestOSVersion ); 
    CPPUNIT_TEST( TestOSAlias ); 
    CPPUNIT_TEST( TestOSFamily );
    CPPUNIT_TEST( TestUnameArchitecture );
    CPPUNIT_TEST( TestArchitecture );
    CPPUNIT_TEST( TestGetCaption );
    CPPUNIT_TEST( TestGetDescription );
    CPPUNIT_TEST_SUITE_END();

private:


public:
    void setUp(void)
    {

    }

    void tearDown(void)
    {
        // Be sure to clean up any generated release file
        SelfDeletingFilePath releaseFile( L"./scx-release" );
    }

    void TestOSName(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);

        wstring correctAnswer;

        // Make variable contain what we want to match on
        // Really would like to pick this up from uname info
        // dynamically
#if defined(aix)
        correctAnswer = L"AIX";
#elif defined(hpux)
        correctAnswer = L"HP-UX";
#elif defined(linux)

        //
        // Linux distributions
        //

  #if defined(PF_DISTRO_ULINUX)
        std::istringstream in;
        std::ostringstream out, err;
        if (SCXCoreLib::SCXProcess::Run(L"sh -c 'grep OSName= ./scx-release | cut -f2 -d='", in, out, err))
        {
            throw SCXCoreLib::SCXErrnoException(StrAppend(L"sh script: ", StrFromMultibyte(err.str())), errno, SCXSRCLOCATION);
        }

        correctAnswer = StrTrim(StrFromMultibyte(out.str()));
  #elif defined(PF_DISTRO_REDHAT)
    #if (PF_MAJOR >= 5)
        correctAnswer = L"Red Hat Enterprise Linux Server";
    #else
        correctAnswer = L"Red Hat Enterprise Linux ES";
    #endif
  #elif defined(PF_DISTRO_SUSE)
    #if (PF_MAJOR < 10)
        // Caseing differences on SLES9, WI9326 
        correctAnswer = L"SUSE LINUX Enterprise Server";
    #else
        correctAnswer = L"SUSE Linux Enterprise Server";
    #endif
  #else
    #error "Unknown Linux distribution"
  #endif // defined(PF_DISTRO_ULINUX)

#elif defined(sun)
        correctAnswer = L"SunOS";
#elif defined(macos)
        correctAnswer = L"Mac OS";
#else 
#warning "Platform not supported for OSAlias"
        correctAnswer = L"This platform does not seem to be implemented";
#endif

        // OK, let's verify that PAL returns that
        wostringstream sout;
        sout << L"\"" << infoObject.GetOSName() << L"\" != \"" << correctAnswer << L"\"";
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), 
                               infoObject.GetOSName() == correctAnswer);
    }

    void TestGetOSNameCompatFlag(void)
    {
        wstring correctAnswer;
        SCXOSTypeInfo infoObject;
        
#if defined(linux) && defined(PF_DISTRO_SUSE)
        correctAnswer = L"SuSE Distribution";
#elif defined(linux) && defined(PF_DISTRO_REDHAT)
        correctAnswer = L"Red Hat Distribution";
#elif defined(linux) && defined(PF_DISTRO_ULINUX)
        correctAnswer = L"Linux Distribution";
#else
        // For all the other, the correct answer should be same as with 
        // compat flag. Note that this includes SLED and RHED too.
        correctAnswer = infoObject.GetOSName();
#endif

        wostringstream sout;
        
        sout << infoObject.GetOSName(true) << L" != " << correctAnswer;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), 
                               infoObject.GetOSName(true) == correctAnswer);
    }


    void TestOSFamily(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);

        wstring correctAnswer;
#if defined(hpux)
        correctAnswer = L"HPUX";
#elif defined(linux)
        correctAnswer= L"Linux";
#elif defined(sun)
        correctAnswer = L"Solaris";
#elif defined(aix)
        correctAnswer = L"AIX";
#elif defined(macos)
        correctAnswer = L"MacOS";
#else 
#warning "Platform not supported for OSAlias"
#endif

        wostringstream sout;
        sout << infoObject.GetOSFamilyString() << L" != " << correctAnswer;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), infoObject.GetOSFamilyString() == correctAnswer);
    }

    void TestOSVersion(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);

        wstring correctAnswer;
        // Per-platform indication whether test code collect value dynamically or not
        // In the end it would be nice to eliminate this and have dynamic for all 
        // platforms...
        bool bCompareWithDynamic; 
#if defined(hpux)
        bCompareWithDynamic = false;
#elif defined(linux) && defined(PF_DISTRO_REDHAT)
        bCompareWithDynamic = false;
#elif defined(linux) && defined(PF_DISTRO_SUSE)
        bCompareWithDynamic = false;
#elif defined(linux) && defined(PF_DISTRO_ULINUX)
        std::istringstream in;
        std::ostringstream out, err;
        if (SCXCoreLib::SCXProcess::Run(L"sh -c 'grep OSVersion= ./scx-release | cut -f2 -d='", in, out, err))
        {
            throw SCXCoreLib::SCXErrnoException(StrAppend(L"sh script: ", StrFromMultibyte(err.str())), errno, SCXSRCLOCATION);
        }

        correctAnswer = StrTrim(StrFromMultibyte(out.str()));
        bCompareWithDynamic = true;
#elif defined(sun)
        bCompareWithDynamic = false;
#elif defined(aix)
        bCompareWithDynamic = false;
#elif defined(macos)
        char buf[256];
        FILE* cmdOutput = popen("sw_vers -productVersion", "r");
        fgets(buf, 256, cmdOutput);
        pclose(cmdOutput);
        string correctAnswerMB;  // Multi-byte string 
        correctAnswerMB = buf;
        correctAnswer = StrTrim(StrFromMultibyte(correctAnswerMB));
        bCompareWithDynamic = true;
#else 
#warning "Platform not supported for OSAlias"
#endif
        if (bCompareWithDynamic) 
        {
            wostringstream sout;
            sout << L"[" << infoObject.GetOSVersion() << L"] != [" << correctAnswer << L"]";
            CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), 
                                   infoObject.GetOSVersion() == correctAnswer);
        }
        else
        {
            CPPUNIT_ASSERT_MESSAGE("GetOSVersion() returned string of zero length", 
                                   infoObject.GetOSVersion().length() > 0);
        }
    }

    void TestOSAlias(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);
        wstring correctAnswer;
#if defined(hpux)
        correctAnswer = L"HPUX";
#elif defined(linux) && defined(PF_DISTRO_REDHAT)
        correctAnswer= L"RHEL";
#elif defined(linux) && defined(PF_DISTRO_SUSE)
        correctAnswer = L"SLES";
#elif defined(sun)
        correctAnswer = L"Solaris";
#elif defined(aix)
        correctAnswer = L"AIX";
#elif defined(macos)
        correctAnswer = L"MacOS";
#else 

#endif
        wostringstream sout;
#if defined(PF_DISTRO_ULINUX)
        // Universal Linux has many acceptable aliases, 
        // so just ensure the alias is an actual string with length > 0
        sout << infoObject.GetOSAlias().size() << L" == " << 0;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), infoObject.GetOSAlias().size() != 0);
#else
        sout << infoObject.GetOSAlias() << L" != " << correctAnswer;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), 
                               infoObject.GetOSAlias() == correctAnswer);
#endif
    }


    void TestUnameArchitecture()
    {
        char buf[256];
#if defined(linux) || defined(hpux) || defined(macos)
        FILE* unameOutput = popen("uname -m", "r");
#elif defined(sun) || defined(aix)
        FILE* unameOutput = popen("uname -p", "r");
#endif
        // If this fail we have some fundamental problem
        CPPUNIT_ASSERT(unameOutput);
        
        fgets(buf, 256, unameOutput);
        std::string testUnameString = buf;
        pclose(unameOutput);

        wstring testUname = StrTrim(StrFromMultibyte(testUnameString));

        SCXOSTypeInfo infoObject;
        wstring palUname = infoObject.GetUnameArchitectureString();

        wostringstream sout;
        sout << palUname << L" != " << testUname;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(), 
                               palUname == testUname);
    }


    void TestArchitecture()
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);

        CPPUNIT_ASSERT_MESSAGE("GetArchitectureString() returned an empty string", 
                               infoObject.GetArchitectureString().length() > 0);

#if defined(macos)
        // For MacOS (non-PPC platforms), we return x86/x64, not i386, based on
        // the CPU capabilities (even though we always build the same kit for
        // both).  Verify that's the case.
        //
        // Note that this test won't currently handle PPC, but the implementation
        // does.

        wstring testArch;

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
                SCXCoreLib::StrTokenize(SCXCoreLib::StrFromMultibyte(buf), parts, L":");

                // If we got what we expected, set the resultant bit size
                if (parts[0] == SCXCoreLib::StrFromMultibyte(sysctlName))
                {
                    if (parts[1] == L"1")
                    {
                        testArch = L"x64";
                    }
                    else if (parts[1] == L"0")
                    {
                        testArch = L"x86";
                    }
                }
            }
            else
            {
                // Did not find value in sysctl, that indicates a 32-bit system
                testArch = L"x86";
            }
        }

        wostringstream sout;
        sout << infoObject.GetArchitectureString() << L" != " << testArch;
        CPPUNIT_ASSERT_MESSAGE(StrToMultibyte(sout.str()).c_str(),
                               infoObject.GetArchitectureString() == testArch);
#endif
    }


    void TestGetCaption(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);

        std::wstring caption = infoObject.GetCaption(); 
        CPPUNIT_ASSERT_MESSAGE("GetCaption() returned an empty string", 
                               caption.length() > 0);
        CPPUNIT_ASSERT_MESSAGE("GetCaption() should not report global zone support, ever", caption.find(L"Global Zone") == wstring::npos);        
    }

    void TestGetDescription(void)
    {
        SCXHandle<SCXOSTypeInfoTestDependencies> deps(new SCXOSTypeInfoTestDependencies());
        SCXOSTypeInfo infoObject(deps);
        std::wstring description = infoObject.GetDescription();
        CPPUNIT_ASSERT_MESSAGE("GetDescription() returned an empty string", 
                               description.length() > 0);

        std::istringstream in;
        std::ostringstream out, err;
#if defined(sun) && (PF_MAJOR > 5 || PF_MINOR >= 10)
        
        if (SCXCoreLib::SCXProcess::Run(std::wstring(L"zonename"), in, out, err) == 0)
        {
            CPPUNIT_ASSERT_MESSAGE(std::string("Running \'zonename\' caused data to be written to stderr:") + err.str(), err.str().length() == 0);

            // Have support for zones, let's see which kind we are in, global or non-global
            // Non-global zones return name of host, e.g. scxsun12-z1, so unless someone
            // names one of our test machines "global" we are OK here.
            if (out.str().compare("global"))
            {
                // Global zone 
                CPPUNIT_ASSERT_MESSAGE("GetDescription() did not return zone support string properly", 
                                       description.find(L"Global Zone") != wstring::npos && description.find(L"Non-Global") == wstring::npos);
            }
            else
            {
                // Non-global
                CPPUNIT_ASSERT_MESSAGE("GetDescription() should not report global zone support on this platform", 
                                       description.find(L"Non-Global Zone") != wstring::npos);
            }
        }
        // No global zone support
        else
        {
            CPPUNIT_ASSERT_MESSAGE(std::string("zonename failed, stderr output follows:") + (err.str().length() > 0 ? err.str() : std::string("(none)")), 
                                   false); 
        }
#else
        CPPUNIT_ASSERT_MESSAGE(std::string("\'zonename\' should not be supported on this platform, but it seems to have run anyway, output is:") + (out.str().length() ? out.str() : std::string("(none)")), 
                               SCXCoreLib::SCXProcess::Run(std::wstring(L"zonename"), in, out, err) != 0); 
        
        // No sun platform, no global zones, please
        CPPUNIT_ASSERT_MESSAGE("GetDescription() should not report global zone support on this platform", 
                               description.find(L"Non-Global Zone") == wstring::npos);
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXOSTypeInfoTest );
