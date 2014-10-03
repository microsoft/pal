/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2008-03-04 16:25

  OS information PAL test class

*/
/*----------------------------------------------------------------------------*/
#include <errno.h>
#include <sys/wait.h>
#include <iostream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>

#include <cppunit/extensions/HelperMacros.h>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;

/*
 Values used to test the LANG variable parser

 LangStr is the input--what the LANG variable would contain
 WindowsLocaleCode, CountryCode and CodePage are what the LANG variable parser
 should output from the given input
 */
struct LocaleTestValues
{
    const std::wstring LangStr;
    unsigned int WindowsLocaleCode;
    unsigned int CountryCode;
    unsigned int CodePage;
};

static LocaleTestValues TestValues[] =
{
    { L"",                     0x0009,   1,     0 },     // invalid
    { L"xx_YY",                0x0009,   1,     0 },     // invalid
    { L"XX_yy",                0x0009,   1,     0 },     // invalid
    { L"long",                 0x0009,   1,     0 },     // invalid
    { L"long.no-page",         0x0009,   1,     0 },     // invalid
    { L"longer.no-page",       0x0009,   1,     0 },     // invalid
    { L"en_XX",                0x0009,   1,     0 },     // unrecognized country
    { L"en_CA.nopage",         0x1009,   1,     0 },     // unrecognized code page
    { L"C",                    0x0009,   1,     0 },
    { L"en",                   0x0009,   1,     0 },
    { L"en_US",                0x0409,   1,     0 },
    { L"en_US.ANSI_X3.4-1968", 0x0409,   1, 20127 },
    { L"en_US.ISO8859-1",      0x0409,   1, 28591 },
    { L"en_US.UTF-8",          0x0409,   1, 65001 },
    { L"en_GB",                0x0809,  44,     0 },
    { L"en_GB.ISO8859-1",      0x0809,  44, 28591 },
    { L"en_CA",                0x1009,   1,     0 },
    { L"de",                   0x0007,  49,     0 },
    { L"de_DE",                0x0407,  49,     0 },
    { L"de-DE.ISO8859-1",      0x0407,  49, 28591 },
    { L"de_DE.UTF-8",          0x0407,  49, 65001 },
    { L"es",                   0x000A,  34,     0 },
    { L"es_ES",                0x0C0A,  34,     0 },
    { L"es_ES.ISO8859-1",      0x0C0A,  34, 28591 },
    { L"es_ES.UTF8",           0x0C0A,  34, 65001 },
    { L"es_AR",                0x2C0A,  54,     0 },
    { L"es_AR.ISO8859-1",      0x2C0A,  54, 28591 },
    { L"es-CL",                0x340A,  56,     0 },
    { L"es_CL.ISO8859-1",      0x340A,  56, 28591 },
    { L"es_MX",                0x080A,  52,     0 },
    { L"es_MX.ISO8859-1",      0x080A,  52, 28591 },
    { L"fr",                   0x000C,  33,     0 },
    { L"fr_FR",                0x040C,  33,     0 },
    { L"fr_FR.ISO8859-1",      0x040C,  33, 28591 },
    { L"it",                   0x0010,  39,     0 },
    { L"it_IT",                0x0010,  39,     0 },
    { L"it_IT.ISO8859-1",      0x0010,  39, 28591 },
    { L"ja",                   0x0011,  81,     0 },
    { L"ja_JP",                0x0411,  81,     0 },
    { L"ja_JP.UTF-8",          0x0411,  81, 65001 },
    { L"ko",                   0x0012,  82,     0 },
    { L"ko_KR",                0x0412,  82,     0 },
    { L"ko_KR.UTF-8",          0x0412,  82, 65001 },
    { L"pt",                   0x0016,  55,     0 },
    { L"pt_BR",                0x0416,  55,     0 },
    { L"pt_BR.ISO8859-1",      0x0416,  55, 28591 },
    { L"pt_PT",                0x0816, 351,     0 },
    { L"pt_PT.ISO8859-1",      0x0816, 351, 28591 },
    { L"ru",                   0x0019,   7,     0 },
    { L"ru_RU",                0x0419,   7,     0 },
    { L"ru_RU.UTF-8",          0x0419,   7, 65001 },
    { L"zh",                   0x0004,  86,     0 },
    { L"zh_CN",                0x0804,  86,     0 },
    { L"zh_CN.UTF-8",          0x0804,  86, 65001 },
    { L"zh_SG",                0x1004,  65,     0 },
    { L"zh_SG.UTF-8",          0x1004,  65, 65001 },
    { L"zh_TW",                0x0404, 886,     0 },
    { L"zh_TW.UTF-8",          0x0404, 886, 65001 }
};

class OSPAL_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( OSPAL_Test  );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( testTotalInstanceExists );
    CPPUNIT_TEST( testParseLangVariable );
    CPPUNIT_TEST( testBootTime );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<OSEnumeration> m_osEnum;

public:

    void setUp(void)
    {
        m_osEnum = 0;
    }

    void tearDown(void)
    {
        if (0 != m_osEnum)
        {
            m_osEnum->CleanUp();
            m_osEnum = 0;
        }
    }

    void callDumpStringForCoverage()
    {
        m_osEnum = new OSEnumeration();
        m_osEnum->Init();
        m_osEnum->Update(true);
        SCXCoreLib::SCXHandle<OSInstance> instance = m_osEnum->GetTotalInstance();

        CPPUNIT_ASSERT(m_osEnum->DumpString().find(L"OSEnumeration") != wstring::npos);
        CPPUNIT_ASSERT(instance->DumpString().find(L"OSInstance") != wstring::npos);
    }

    void testTotalInstanceExists()
    {
        m_osEnum = new OSEnumeration();
        m_osEnum->Init();
        m_osEnum->Update(true);

        SCXCoreLib::SCXHandle<OSInstance> instance = m_osEnum->GetTotalInstance();
        CPPUNIT_ASSERT(instance != 0);
        CPPUNIT_ASSERT(m_osEnum->Begin() == m_osEnum->End());

        // And then test that nothing dumps core
        SweepOSInstance(instance);
    }

    void testParseLangVariable()
    {
        wstring countryCode;
        unsigned int osLanguage;
        wstring codeSet;
        bool codePageSpecified;

        for (size_t i = 0; i < sizeof TestValues / sizeof (LocaleTestValues); i++)
        {
            codePageSpecified = SCXSystemLib::ParseLangVariable(TestValues[i].LangStr,
                                                                countryCode,
                                                                osLanguage,
                                                                codeSet);
            if (StrFrom(TestValues[i].CountryCode).compare(countryCode) != 0 ||
                osLanguage != TestValues[i].WindowsLocaleCode ||
                codePageSpecified ? StrFrom(TestValues[i].CodePage).compare(codeSet) != 0 : TestValues[i].CodePage != 0)
            {
                std::string msg("Failure in LANG string parser test of LANG = ");
                msg += StrToUTF8(TestValues[i].LangStr);
                CPPUNIT_FAIL(msg);
            }
        }
    }

    std::wstring GetCommandLineBootTime()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        int procRet = SCXCoreLib::SCXProcess::Run(L"who -b", input, output, error);
        CPPUNIT_ASSERT_EQUAL( std::string(""), error.str() );
        CPPUNIT_ASSERT_EQUAL( 0, procRet );
        return StrFromUTF8(output.str());
    }

    wstring GetExpectedNeedle(const SCXCalendarTime& boot_time)
    {
        char timeBuffer[128];
        const time_t posixTime = static_cast<time_t>(boot_time.ToPosixTime());
        struct tm* timeInfo = localtime(&posixTime);
#if ( defined(PF_DISTRO_REDHAT) && PF_MAJOR <= 4 ) || ( defined(PF_DISTRO_SUSE) && PF_MAJOR <= 9 ) ||  !defined(linux)
        const char timeFormat[] = "%b %e %H:%M";
#else
        const char timeFormat[] = "%F %R";
#endif

        if (strftime(timeBuffer, sizeof(timeBuffer), timeFormat, timeInfo) == 0) {
            // error timeBuffer length not enough
            timeBuffer[0] = '\0';
        }

        std::wostringstream buf;
        buf << timeBuffer;
        return buf.str();
    }

    void testBootTime()
    {
        m_osEnum = new OSEnumeration();
        m_osEnum->Init();
        m_osEnum->Update(true);

        SCXCoreLib::SCXHandle<OSInstance> inst = m_osEnum->GetTotalInstance();
        SCXCalendarTime currentTime = SCXCalendarTime::CurrentLocal();
        SCXCalendarTime ASCXCalendarTime;

        inst->GetLastBootUpTime(ASCXCalendarTime);
        CPPUNIT_ASSERT((currentTime.GetYear()-ASCXCalendarTime.GetYear() ) <= 2);
        CPPUNIT_ASSERT(currentTime > ASCXCalendarTime);

        wstring cmdBootTime = GetCommandLineBootTime();
        if (cmdBootTime.size() == 0)
        {
            SCXUNIT_WARNING(L"OSPAL_Test::testBootTime() Command 'who -b' returned empty string");
        }
        else
        {
            std::size_t found;
            wstring expected_needle(L"");
            // We allow a little fudge factor to prevent rounding errors of the minutes
            for (scxseconds fudge_seconds = -60.0; fudge_seconds <= 61.0; fudge_seconds += 60.0 )
            {
                SCXCalendarTime fudgeTime = ASCXCalendarTime + SCXRelativeTime().SetSeconds(fudge_seconds);
                expected_needle = GetExpectedNeedle(fudgeTime);
                found = cmdBootTime.find(expected_needle);
                if (found != std::string::npos)
                    break;
            }
            
            std::wostringstream buf;
            buf << L"Could not find expected boot time: '" << expected_needle 
                << L"' in 'who -b' output'" << cmdBootTime << L"'";
            CPPUNIT_ASSERT_MESSAGE(StrToUTF8(buf.str()), found != std::string::npos);
        }
    }

    void SweepOSInstance(SCXCoreLib::SCXHandle<OSInstance> inst)
    {
        // Just access all methods so we can see that nothing fails fatal

        SCXCalendarTime ASCXCalendarTime;
        scxulong Ascxulong;
        unsigned short Aunsignedshort;
        vector<wstring> Avector;
        vector<unsigned short> Aushortvector;
        wstring Ascxstring;
        short Ashort;
        unsigned int Auint;
        bool retVal;
        
        CPPUNIT_ASSERT(inst->GetOSType(Aunsignedshort));
        CPPUNIT_ASSERT(inst->GetOtherTypeDescription(Ascxstring));
#if !defined(PF_DISTRO_ULINUX)
        CPPUNIT_ASSERT(inst->GetVersion(Ascxstring));
        CPPUNIT_ASSERT(inst->GetManufacturer(Ascxstring));
#endif    
        
        CPPUNIT_ASSERT(inst->GetLastBootUpTime(ASCXCalendarTime));
        CPPUNIT_ASSERT(inst->GetLocalDateTime(ASCXCalendarTime));
        CPPUNIT_ASSERT(inst->GetCurrentTimeZone(Ashort));
        CPPUNIT_ASSERT(inst->GetNumberOfLicensedUsers(Auint));
        CPPUNIT_ASSERT(inst->GetNumberOfUsers(Auint));

        retVal = inst->GetMaxNumberOfProcesses(Auint);
#if defined (linux) || defined (hpux)
        CPPUNIT_ASSERT(retVal);
#endif

        CPPUNIT_ASSERT(inst->GetMaxProcessMemorySize(Ascxulong));
        inst->GetMaxProcessesPerUser(Auint);

        retVal = inst->GetSystemUpTime(Ascxulong);
#if defined (linux) || defined (hpux)
        CPPUNIT_ASSERT(retVal);
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( OSPAL_Test );
