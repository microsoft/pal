/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2011-04-26 15:23:06

  ComputerSystem colletion test class.

  This class tests the Linux implementations.

  It checks the result of ComputerSystem detail information. 

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/computersystemenumeration.h>
#include <scxsystemlib/computersysteminstance.h>
#include <fstream>

#include <testutils/scxunit.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>


using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;


#if FILTERLINUX
/**
    Class for injecting test behavior into the ComputerSystem PAL.
 */
class ComputerSystemSmbiosDependencies : public SMBIOSPALDependencies 
{
public:
    ComputerSystemSmbiosDependencies()
    {
    }

    bool ReadSpecialMemory(MiddleData &buf) const
    {
        size_t tableLength = cEndAddress - cStartAddress + 1;
        ifstream fin("./testfiles/entrypoint_computersystem.dat",ios::binary);
        char *ptmpBuf = reinterpret_cast<char*>(&(buf[0]));
        fin.read(ptmpBuf,tableLength);
        fin.close();

        return true;
    }

    bool GetSmbiosTable(const struct SmbiosEntry& entryPoint,MiddleData &buf) const
    {
        ifstream fin("./testfiles/smbiostable_computersystem.dat",ios::binary);
        char *ptmpBuf = reinterpret_cast<char*>(&(buf[0]));
        fin.read(ptmpBuf,entryPoint.tableLength);
        fin.close();

        return true;
    }

};
/**
    Class for injecting test behavior into the ComputerSystem PAL.
 */
class ComputerSystemPALDependencies : public ComputerSystemDependencies
{
public:
    ComputerSystemPALDependencies()
    {
        m_cpuLines.clear();
    }

    const vector<wstring>& GetCpuInfo()
    {
        m_cpuLines.clear();
        wstring lineOne = L"processor : 0";
        wstring lineTwo = L"processor : 1";
        wstring lineThree = L"processor : 2";
        m_cpuLines.push_back(lineOne);
        m_cpuLines.push_back(lineTwo);
        m_cpuLines.push_back(lineThree);

        return m_cpuLines;
    }

    private:
    vector<wstring> m_cpuLines;    //!< The content of CPUInfo. 

};
#elif defined(sun) || defined(aix) || defined(hpux)
/**
    Class for injecting test behavior into the ComputerSystemDependencies.
 */
class ComputerSystemTestDependencies : public ComputerSystemDependencies
{
public:
    ComputerSystemTestDependencies()
    {

    }

    bool GetSystemRunLevel(wstring &runLevel) const
    {
        runLevel = L"run-level 3";
        return true;
    }

    bool GetSystemTimeZone(bool &dayLight) const
    {
        dayLight = false;
        return true;
    }

    bool GetPowerCfg(std::vector<wstring>& allLines)
    {
        allLines.push_back(L"# Copyright (c) 1996 - 2001 by Sun Microsystems, Inc.");
        allLines.push_back(L"# All rights reserved.");
        allLines.push_back(L"#");
        allLines.push_back(L"#pragma ident   \"@(#)power.conf 1.16    01/03/19 SMI\"");
        allLines.push_back(L"# Power Management Configuration File");
        allLines.push_back(L"#");
        allLines.push_back(L"device-dependency-property removable-media /devd/fb");
        allLines.push_back(L"");
        allLines.push_back(L"");
        allLines.push_back(L"autopm                  disable");
        allLines.push_back(L"statefile               //.CPR");
        allLines.push_back(L"# Auto-Shutdown         Idle(min)       Start/Finish(hh:mm)     Behavior");
        allLines.push_back(L"autoshutdown            30              9:00 9:00               default");
        return true;
    }
};
#endif


class ComputerSystem_test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(ComputerSystem_test);

    CPPUNIT_TEST(testGetComputerSystemAttr);
    SCXUNIT_TEST_ATTRIBUTE(testGetComputerSystemAttr,SLOW);

    CPPUNIT_TEST_SUITE_END();

public:

#if defined(sun) || defined(aix) || defined(hpux)
    ComputerSystemEnumeration* m_pEnum;
#endif

    void setUp(void)
    {

    }

    void tearDown(void)
    {
#if defined(sun) || defined(aix) || defined(hpux)
        if (m_pEnum != 0)
        {
            m_pEnum->CleanUp();
            delete m_pEnum;
            m_pEnum = 0;
        }
#endif
    }

    void testGetComputerSystemAttr()
    {
        std::string model;
        wstring strtmp;
#if FILTERLINUX
        SCXHandle<ComputerSystemSmbiosDependencies> depsSmbios(new ComputerSystemSmbiosDependencies());
        SCXHandle<SCXSmbios> smbios(new SCXSmbios(depsSmbios));
        SCXHandle<ComputerSystemPALDependencies> deps(new ComputerSystemPALDependencies());
        SCXCoreLib::SCXHandle<ComputerSystemEnumeration> computerSystemEnum(new ComputerSystemEnumeration(smbios,deps));
        computerSystemEnum->Init();
        SCXCoreLib::SCXHandle<ComputerSystemInstance> inst = computerSystemEnum->GetTotalInstance();
        inst->Update();

        unsigned short chassisBootupState = 3;
        unsigned short wakeUp = 6;
        model = "HVM domU";
        unsigned short powerSupplyState = 3;
        unsigned short thermalState = 3;

        unsigned short tmp = 0;
        CPPUNIT_ASSERT(inst->GetChassisBootupState(tmp));
        CPPUNIT_ASSERT_EQUAL(tmp,chassisBootupState);

        CPPUNIT_ASSERT(inst->GetWakeUpType(tmp));
        CPPUNIT_ASSERT_EQUAL(tmp,wakeUp);

        CPPUNIT_ASSERT(inst->GetPowerSupplyState(tmp));
        CPPUNIT_ASSERT_EQUAL(tmp,powerSupplyState);

        CPPUNIT_ASSERT(inst->GetThermalState(tmp));
        CPPUNIT_ASSERT_EQUAL(tmp,thermalState);
#elif defined(sun) || defined(aix) || defined(hpux) 

        //
        // Mock dependencies object
        //
        SCXHandle<ComputerSystemTestDependencies> deps(new ComputerSystemTestDependencies());
        m_pEnum = new ComputerSystemEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->Update(true);

        //
        //Get first instance
        //
        SCXCoreLib::SCXHandle<ComputerSystemInstance> inst = m_pEnum->GetTotalInstance();
        CPPUNIT_ASSERT(0 != inst);
        inst->Update();
#endif

        //-----Description--------        
#if defined (hpux)
        std::string tmpDescription;
        wstring description;
        #if defined(__hppa)
            tmpDescription = "PA RISC";
        #else
            tmpDescription = "Itanium";
        #endif

        CPPUNIT_ASSERT(inst->GetDescription(description));
        CPPUNIT_ASSERT_EQUAL(tmpDescription, StrToUTF8(description));
 
#endif

        //-----ManagementCapabilities---
        vector<unsigned int> powerManagementCapabilities;
#if defined(aix) || defined(hpux) || FILTERLINUX
        CPPUNIT_ASSERT(!inst->GetPowerManagementCapabilities(powerManagementCapabilities));
#elif defined(sun)
        unsigned int intPowerManagementCapabilities  = 2; //mean "disable"
        CPPUNIT_ASSERT(inst->GetPowerManagementCapabilities(powerManagementCapabilities));
        if ( powerManagementCapabilities.size() >0)
        {
            CPPUNIT_ASSERT_EQUAL(powerManagementCapabilities[0], intPowerManagementCapabilities);
        }
#endif
        //-----Daylight In Effect-------
        bool tmpBoolData = false;
#if defined(linux)
        CPPUNIT_ASSERT(!inst->GetDaylightInEffect(tmpBoolData));
#elif defined(hpux) || defined(aix) || defined(sun)
        CPPUNIT_ASSERT(inst->GetDaylightInEffect(tmpBoolData));
        CPPUNIT_ASSERT_EQUAL(false, tmpBoolData);
#endif

        //-----Manufacturer-------------
        wstring tmpData = L"";
#if defined(linux) || defined(aix) || defined(hpux) || defined(sun)
        CPPUNIT_ASSERT(inst->GetManufacturer(tmpData));
#if defined(linux)
        CPPUNIT_ASSERT_EQUAL(wstring(L"Xen"), tmpData); 
#elif defined(hpux)
        CPPUNIT_ASSERT_EQUAL(wstring(L"Hewlett-Packard Company"), tmpData);
#elif defined(aix)
        CPPUNIT_ASSERT_EQUAL(wstring(L"International Business Machines Corporation"), tmpData);
#elif defined(sun)
        CPPUNIT_ASSERT_EQUAL(wstring(L"Oracle Corporation"), tmpData);
#endif
#endif

        //-----Power Management---------
#if defined(sun)
        CPPUNIT_ASSERT(inst->GetPowerManagementSupported(tmpBoolData));
        CPPUNIT_ASSERT_EQUAL(true, tmpBoolData);
#elif defined(hpux)
        struct stat st;
        if(stat("/dev/GSPdiag1",&st) == 0)
        {
            CPPUNIT_ASSERT(inst->GetPowerManagementSupported(tmpBoolData));
            CPPUNIT_ASSERT_EQUAL(true, tmpBoolData);
        }
        else
        {
            CPPUNIT_ASSERT(inst->GetPowerManagementSupported(tmpBoolData));
        }

#elif defined(linux) || defined(aix)
        CPPUNIT_ASSERT(!inst->GetPowerManagementSupported(tmpBoolData));
#endif

        //-----Network Server Mode------
        CPPUNIT_ASSERT(!inst->GetNetworkServerModeEnabled(tmpBoolData));
        CPPUNIT_ASSERT_EQUAL(false, tmpBoolData);

        //-----Model Name section----
        strtmp = L"";
        CPPUNIT_ASSERT(inst->GetModel(strtmp));
        
    #if !defined(linux)
        wstring cmdStrModelname;
        std::istringstream streamInstr;
        std::ostringstream streamOutstr;
        std::ostringstream streamErrstr;
        wstring resultModelname;

        #if defined(aix)
            cmdStrModelname = L"lsattr -El sys0 -a modelname -F value"; 
        #elif defined(hpux)
            cmdStrModelname = L"model";
        #elif defined(sun)
            cmdStrModelname = L"uname -i";
        #endif

        int procRet = SCXCoreLib::SCXProcess::Run(cmdStrModelname, streamInstr, streamOutstr, streamErrstr, 150000);
        if (procRet == 0 && streamErrstr.str().empty())
        {
            model = (streamOutstr.str()).substr(0, streamOutstr.str().size() -1);
        }
        else
        {
            cout << "Command Run failed. The return value is : " << procRet << endl;
            cout << "The ErrorString is : " << streamErrstr.str() << endl;
        }
    #endif
        CPPUNIT_ASSERT_EQUAL(model, StrToUTF8(strtmp)); 
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( ComputerSystem_test );
