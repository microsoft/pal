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

    std::string GetCommandLineBootTime()
    {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
#if defined(PF_DISTRO_SUSE) && PF_MAJOR <= 10
        int procRet = SCXCoreLib::SCXProcess::Run(L"bash -c \"who -a | head -1\"", input, output, error);
#else
        int procRet = SCXCoreLib::SCXProcess::Run(L"who -b", input, output, error);
#endif
        CPPUNIT_ASSERT_EQUAL( std::string(""), error.str() );
        CPPUNIT_ASSERT_EQUAL( 0, procRet );
        return output.str();
    }

    // Return the POSIX time parsed from the 'who -b' output
    scxlong parseBootTime(string bootTime, int yearHint)
    {

#if defined(PF_DISTRO_SUSE) && PF_MAJOR <= 10
        string timeStr(bootTime);
#else
        // Remove the part before the date from any of the below formats
        //    .        system boot Sep 28 10:55
        //          system boot  2014-01-23 21:18
        string user("system boot");
        size_t start_user = bootTime.find(user);
        CPPUNIT_ASSERT_MESSAGE("Could not find 'system boot' in : " + bootTime, start_user != string::npos);
        string timeStr = bootTime.substr(start_user + user.size(), bootTime.size());
#endif
        // Parse the date
#if defined(aix) || defined(sun) || defined(hpux)
        const char timeFormat[] = " %b %d %H:%M";
#else
        const char timeFormat[] = " %Y-%m-%d %H:%M";
#endif
        struct tm timeStruct;
        char *ret = strptime(timeStr.c_str(), timeFormat, &timeStruct);
        ostringstream parse_err_msg;
        parse_err_msg << "Date parsing error. Date='" << timeStr << "' Format='" << timeFormat << "'";
        CPPUNIT_ASSERT_MESSAGE(parse_err_msg.str(), ret != NULL);

        // Fixup tm fields
#if defined(aix) || defined(sun) || defined(hpux)
        timeStruct.tm_year = yearHint - 1900;
#else
        (void) yearHint;
#endif        
        timeStruct.tm_sec = 0;
        timeStruct.tm_isdst = -1; // Unknown Daylight Saving Time

        // Convert to POSIX time
        time_t time = mktime(&timeStruct);
        ostringstream invalid_date_err_msg;
        invalid_date_err_msg << "Invalid date in timeStruct: '" << timeStruct.tm_year + 1900 << "-"
                             << timeStruct.tm_mon + 1 << "-" << timeStruct.tm_mday << " "
                             << timeStruct.tm_hour << ":" << timeStruct.tm_min << ":" << timeStruct.tm_sec
                             << " errno reason : " << strerror(errno);
        CPPUNIT_ASSERT_MESSAGE(invalid_date_err_msg.str(), time != -1);
        return (scxlong)time;
    } 

    void testBootTime()
    {
        m_osEnum = new OSEnumeration();
        m_osEnum->Init();
        m_osEnum->Update(true);

        SCXCoreLib::SCXHandle<OSInstance> inst = m_osEnum->GetTotalInstance();
        SCXCalendarTime currentTime = SCXCalendarTime::CurrentLocal();
        SCXCalendarTime SCXBootTime;

        inst->GetLastBootUpTime(SCXBootTime);
        CPPUNIT_ASSERT((currentTime.GetYear()-SCXBootTime.GetYear() ) <= 2);
        CPPUNIT_ASSERT(currentTime > SCXBootTime);

        string whoBoutput = GetCommandLineBootTime();
        CPPUNIT_ASSERT_MESSAGE("No output was found to compare boot time", whoBoutput.size() > 0);
        scxlong CMDBootTime = parseBootTime(whoBoutput, (int)SCXBootTime.GetYear());
        scxlong acceptable_fudge_seconds = 61;
        SCXUNIT_ASSERT_BETWEEN(SCXBootTime.ToPosixTime(), CMDBootTime - acceptable_fudge_seconds, CMDBootTime + acceptable_fudge_seconds);
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
