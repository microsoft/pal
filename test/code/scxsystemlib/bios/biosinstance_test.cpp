/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2011-03-03 16:21:06

  BIOS colletion test class.

  This class tests the Linux, Solaris(x86) implementations.

  It checks the result of BIOS detail information. 

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/biosinstance.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxtime.h>
#include <fstream>

#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;



/**
    Class for injecting test behavior into the BIOS PAL.
 */
#if defined(linux) || (defined(sun) && defined(ia32))
class BIOSPALTestDependencies : public SMBIOSPALDependencies 
{
public:
    BIOSPALTestDependencies()
    {
    }

    bool ReadSpecialMemory(MiddleData &buf) const
    {
       size_t tableLength = cEndAddress - cStartAddress + 1;
       ifstream fin("./testfiles/entrypoint.dat",ios::binary);
       char *ptmpBuf = reinterpret_cast<char*>(&(buf[0]));
       fin.read(ptmpBuf,tableLength);
       fin.close();

        return true;
    }

    bool GetSmbiosTable(const struct SmbiosEntry& entryPoint,MiddleData &buf) const
    {

       ifstream fin("./testfiles/smbiostable.dat",ios::binary);
       char *ptmpBuf = reinterpret_cast<char*>(&(buf[0]));
       fin.read(ptmpBuf,entryPoint.tableLength);
       fin.close();

       return true;
    }

};
#endif


//for non-SMBios Bios test dependency 
#if defined(sun) && defined(sparc)
class BiosTestDependencies : public BiosDependencies
{
public:
    BiosTestDependencies()
    {
    }
    void GetPromVersion(wstring& version)
    {
        //prom version is like OBP 4.30.4 2009/08/19 07:25
        version=wstring(L"OBP 4.30.4 2009/08/19 07:25");
    }

    void GetPromManufacturer(wstring& manufacturer)
    {
        //it is using prom's model value for manufacturer value
        manufacturer=wstring(L"SUNW,4.30.4");
    }

};
#endif

class BIOS_test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(BIOS_test);

    CPPUNIT_TEST(testGetBiosAttr);
    CPPUNIT_TEST(testGetTargetOS);
    SCXUNIT_TEST_ATTRIBUTE(testGetBiosAttr,SLOW);
#if defined(linux) || (defined(sun) && defined(ia32))
    CPPUNIT_TEST(TestBiosCharacteristics_wi478597);
#endif

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {

    }

    void tearDown(void)
    {

    }

    void testGetBiosAttr()
    {
#if defined(linux) || (defined(sun) && defined(ia32))

        SCXHandle<BIOSPALTestDependencies> deps(new BIOSPALTestDependencies());
        SCXHandle<SCXSmbios> smbios(new SCXSmbios(deps));
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance(smbios));
        biosInstance->Update();

        bool smbiosPresent = true;
        std::string smbiosBiosVersion = "4.0.1_21326_03-0.3";
        unsigned short smbiosMajorVersion = 2;
        unsigned short smbiosMinorVersion= 4;
        std::string manufacturer = "Xen";
        SCXCoreLib::SCXCalendarTime installDate=SCXCalendarTime::FromPosixTime(0L);
        installDate.SetMonth(StrToUInt(L"12"));
        installDate.SetDay(StrToUInt(L"28"));
        installDate.SetYear(StrToUInt(L"2010"));

        wstring tmp = L""; 
        CPPUNIT_ASSERT(biosInstance->GetSmbiosBiosVersion(tmp));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmp),smbiosBiosVersion);

        CPPUNIT_ASSERT(biosInstance->GetManufacturer(tmp));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmp),manufacturer);

        SCXCalendarTime tmpDate;
        CPPUNIT_ASSERT(biosInstance->GetInstallDate(tmpDate));
        //CPPUNIT_ASSERT_EQUAL(tmpDate,installDate);

        unsigned short tmpVersion;
        CPPUNIT_ASSERT(biosInstance->GetSMBIOSMajorVersion(tmpVersion));
        CPPUNIT_ASSERT_EQUAL(tmpVersion,smbiosMajorVersion);

        CPPUNIT_ASSERT(biosInstance->GetSMBIOSMinorVersion(tmpVersion));
        CPPUNIT_ASSERT_EQUAL(tmpVersion,smbiosMinorVersion);

        bool tmpPresent;
        CPPUNIT_ASSERT(biosInstance->GetSmbiosPresent(tmpPresent));
        CPPUNIT_ASSERT_EQUAL(tmpPresent,smbiosPresent);

        #elif defined(sun) && defined(sparc)

        SCXHandle<BiosDependencies> deps(new BiosTestDependencies());
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance(deps));
        biosInstance->Update();

        wstring tmp; 
        CPPUNIT_ASSERT(biosInstance->GetManufacturer(tmp));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmp), string("SUNW,4.30.4"));

        CPPUNIT_ASSERT(biosInstance->GetVersion(tmp));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmp), string("OBP 4.30.4 2009/08/19 07:25"));

        SCXCalendarTime tmpDate;
        CPPUNIT_ASSERT(biosInstance->GetInstallDate(tmpDate));
#elif defined(aix)
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance());
        biosInstance->Update();
        wstring tmp;
        
        CPPUNIT_ASSERT(biosInstance->GetSystemSerialNumber(tmp));
        CPPUNIT_ASSERT(tmp.size() > 0);
        CPPUNIT_ASSERT_EQUAL(getCuAtValue(L"attribute=systemid"), tmp);

        CPPUNIT_ASSERT(biosInstance->GetVersion(tmp));
        CPPUNIT_ASSERT(tmp.size() > 0);
        CPPUNIT_ASSERT_EQUAL(getCuAtValue(L"attribute=fwversion"), tmp);
#endif
    }

    void testGetTargetOS()
    {
#if defined(sun) && defined(sparc)
        SCXHandle<BiosDependencies> deps(new BiosDependencies());
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance(deps));
#else
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance());
#endif
        biosInstance->Update();
        
        unsigned short targetOS;
        CPPUNIT_ASSERT(biosInstance->GetTargetOperatingSystem(targetOS));
        CPPUNIT_ASSERT_MESSAGE("Unknown target operating system", targetOS != 0);
    }

#if defined(aix)
    wstring getCuAtValue(wstring query)
    {
        std::istringstream in;
        std::ostringstream out, err;
        
        wstring command = L"odmget -q\"" + query + L"\" CuAt";
        int retValue = SCXCoreLib::SCXProcess::Run(command, in, out, err);
        
        CPPUNIT_ASSERT_EQUAL("", err.str());
        CPPUNIT_ASSERT_EQUAL(0, retValue);
        wstring output = StrFromUTF8(out.str());

        wstring leftbound = L"value = \"";
        string::size_type start = output.find(leftbound);
        CPPUNIT_ASSERT(start != string::npos);
        start += leftbound.size();

        wstring rightbound = L"\"";
        string::size_type end = output.find(rightbound, start + 1);
        CPPUNIT_ASSERT(end != string::npos);
        return output.substr(start, end - start);
    }
#endif

#if defined(linux) || (defined(sun) && defined(ia32))
    void TestBiosCharacteristics_wi478597()
    {
        SCXHandle<BIOSPALTestDependencies> deps(new BIOSPALTestDependencies());
        SCXHandle<SCXSmbios> smbios(new SCXSmbios(deps));
        SCXCoreLib::SCXHandle<BIOSInstance> biosInstance(new BIOSInstance(smbios));
        biosInstance->Update();

        vector<unsigned short> biosCharacteristics; // Actual value.
        unsigned short biosChars[]={4,5,6,7,19,33,34,35,36,37,39}; // Expected values hardcoded in ./testfiles/smbiostable.dat
                                                                   //   (Byte offsets: 10,11,12,13(0x800f0) and 18(0xbe))
        biosInstance->GetBiosCharacteristics(biosCharacteristics);
        CPPUNIT_ASSERT_EQUAL(biosChars[0], biosCharacteristics[0]);
        CPPUNIT_ASSERT_EQUAL(biosChars[1], biosCharacteristics[1]);
        CPPUNIT_ASSERT_EQUAL(biosChars[2], biosCharacteristics[2]);
        CPPUNIT_ASSERT_EQUAL(biosChars[3], biosCharacteristics[3]);
        CPPUNIT_ASSERT_EQUAL(biosChars[4], biosCharacteristics[4]);
        CPPUNIT_ASSERT_EQUAL(biosChars[5], biosCharacteristics[5]);
        CPPUNIT_ASSERT_EQUAL(biosChars[6], biosCharacteristics[6]);
        CPPUNIT_ASSERT_EQUAL(biosChars[7], biosCharacteristics[7]);
        CPPUNIT_ASSERT_EQUAL(biosChars[8], biosCharacteristics[8]);
        CPPUNIT_ASSERT_EQUAL(biosChars[9], biosCharacteristics[9]);
        CPPUNIT_ASSERT_EQUAL(biosChars[10], biosCharacteristics[10]);

    } // End of TestBiosCharacteristicsWI478597()
#endif

};

CPPUNIT_TEST_SUITE_REGISTRATION( BIOS_test );
