/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2011-04-02 09:51:06

  cpuproperties colletion test class.

  This class tests the Linux, Solaris(x86) implementations.

  It checks the result of cpuproperties detail information.

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/cpupropertiesenumeration.h>
#include <scxsystemlib/cpupropertiesinstance.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <math.h>
#include <fstream>
#include <testutils/scxunit.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;

#if defined(linux)
#include <scxcorelib/scxfile.h>
#include <scxsystemlib/procfsreader.h>
#endif

#if defined(linux)
/**
   Class for injecting test behavior into the cpuproperties PAL.
*/
class CPUInfoTestDependencies: public CPUInfoDependencies
{
public:
    CPUInfoTestDependencies(): m_cpuInfoFileType(-1){}
    void SetCpuInfoFileType(int type) { m_cpuInfoFileType = type; }
    SCXCoreLib::SCXHandle<std::wistream> OpenCpuinfoFile() const
    {
        SCXHandle<wstringstream> cpuInfoStream( new wstringstream );
        switch (m_cpuInfoFileType)
        {
            case 0: //no physcial id field
                * cpuInfoStream 
                << L"processor       : 0" << endl
                << L"vendor_id       : GenuineIntel" << endl
                << L"cpu family      : 6" << endl
                << L"model           : 12" << endl
                << L"model name      : Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2104.008" << endl
                << L"cache size      : 0 KB" << endl
                << L"fpu             : yes" << endl
                << L"fpu_exception   : yes" << endl
                << L"cpuid level     : 11" << endl
                << L"wp              : yes" << endl
                << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx lm pni cx16 ts" << endl
                << L"bogomips        : 3145.72" << endl
                << L"clflush size    : 64" << endl
                << L"cache_alignment : 64" << endl
                << L"address sizes   : 40 bits physical, 48 bits virtual" << endl
                << L"power management:" << endl

                << L"processor       : 1" << endl
                << L"vendor_id       : GenuineIntel" << endl
                << L"cpu family      : 6" << endl
                << L"model           : 12" << endl
                << L"model name      : Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2104.008" << endl
                << L"cache size      : 0 KB" << endl
                << L"fpu             : yes" << endl
                << L"fpu_exception   : yes" << endl
                << L"cpuid level     : 11" << endl
                << L"wp              : yes" << endl
                << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx lm pni cx16 ts" << endl
                << L"bogomips        : 2093.05" << endl
                << L"clflush size    : 64" << endl
                << L"cache_alignment : 64" << endl
                << L"address sizes   : 40 bits physical, 48 bits virtual" << endl
                << L"power management:" << endl;
                break;   
            case 1: //1 physical id exist with 2 cores on it
                * cpuInfoStream 
                << L"processor       : 0" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 0" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 0" << endl
                << L"cpu cores       : 2" << endl

                << L"processor       : 1" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 0" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 1" << endl
                << L"cpu cores       : 2" << endl;
                break;
            case 2: // 2 physical id exist with 2 cores on each
                * cpuInfoStream
                << L"processor       : 0" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 0" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 0" << endl
                << L"cpu cores       : 2" << endl

                << L"processor       : 1" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 0" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 1" << endl
                << L"cpu cores       : 2" << endl 

                << L"processor       : 2" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 2" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 0" << endl
                << L"cpu cores       : 2" << endl

                << L"processor       : 3" << endl
                << L"model           : 44" << endl
                << L"stepping        : 2" << endl
                << L"cpu MHz         : 2132.826" << endl
                << L"physical id     : 2" << endl
                << L"siblings        : 2" << endl
                << L"core id         : 1" << endl
                << L"cpu cores       : 2" << endl; 
                break;
            default:
                std::stringstream ss;
                ss << "Unknown stat filetype during call to OpenCpuinfoFile(): " << m_cpuInfoFileType;
                CPPUNIT_FAIL( ss.str() );

        }
        return cpuInfoStream; 
    }
private:
    int m_cpuInfoFileType;
};

/**
   Class for injecting processor family behavior into the cpuproperties PAL.
*/
class CPUFamilyTestDependencies: public CPUInfoDependencies
{
public:
    string m_vendorString;
    string m_brandString;
    SCXCoreLib::SCXHandle<std::wistream> OpenCpuinfoFile() const
    {
        SCXHandle<wstringstream> cpuInfoStream( new wstringstream );
        *cpuInfoStream 
        << L"processor       : 0" << endl
        << L"vendor_id       : " << StrFromMultibyte(m_vendorString) << endl
        << L"cpu family      : 6" << endl
        << L"model           : 12" << endl;
        
        if (m_brandString.empty() != true)
        {
            *cpuInfoStream << L"model name      : " << StrFromMultibyte(m_brandString) << endl;
        }
        
        *cpuInfoStream 
        << L"stepping        : 2" << endl
        << L"cpu MHz         : 2104.008" << endl
        << L"cache size      : 0 KB" << endl
        << L"fpu             : yes" << endl
        << L"fpu_exception   : yes" << endl
        << L"cpuid level     : 11" << endl
        << L"wp              : yes" << endl
        << L"flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx lm pni cx16 ts" << endl
        << L"bogomips        : 3145.72" << endl
        << L"clflush size    : 64" << endl
        << L"cache_alignment : 64" << endl
        << L"address sizes   : 40 bits physical, 48 bits virtual" << endl
        << L"power management:" << endl;

        return cpuInfoStream; 
    }
};
#endif


class CpuProperties_test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(CpuProperties_test);

    CPPUNIT_TEST(testGetCpuPropertiesAttr);
    CPPUNIT_TEST(testGetCpuFamilyAttr);

    SCXUNIT_TEST_ATTRIBUTE(testGetCpuPropertiesAttr,SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testGetCpuFamilyAttr,SLOW);

    CPPUNIT_TEST_SUITE_END();

public:

#if defined(linux)
    void OneFamilyTest(const char *vendorString, const char *brandString, unsigned short family)
    {
        std::ostringstream errMsg;
        errMsg << "Failed running test for vendor string \'" << vendorString << "\", brand string \"" << brandString <<"\"";
        
        SCXHandle<CPUFamilyTestDependencies> deps(new CPUFamilyTestDependencies());
        deps->m_vendorString = vendorString;
        deps->m_brandString = brandString;
        SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> filehandle =
        SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> (new ProcfsCpuInfoReader(deps));
        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum(filehandle);
        cpuPropertiesEnum.Init();
        CPPUNIT_ASSERT_EQUAL_MESSAGE(errMsg.str(), 1u, cpuPropertiesEnum.Size());
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);

        unsigned short retFamily;
        CPPUNIT_ASSERT_MESSAGE(errMsg.str(), inst->GetFamily(retFamily));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(errMsg.str(), family, retFamily);
    }
#endif

    void testGetCpuFamilyAttr()
    {
#if defined(linux)
        OneFamilyTest("GenuineIntel", "", 2); // Unknown family.
        OneFamilyTest("GenuineIntel", "Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz", 179);
        OneFamilyTest("GenuineIntel", "Xeon(R) CPU           L5630  @ 2.13GHz", 2);
        OneFamilyTest("GenuineIntel", "Mobile Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz", 179);
        OneFamilyTest("GenuineIntel", "Genuine Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz", 179);
        OneFamilyTest("GenuineIntel", "Intel(R) Pentium(R) CPU @ 2.13GHz", 11);
        OneFamilyTest("GenuineIntel", "Intel(R) Pentium(R) III CPU @ 2.13GHz", 17);
        OneFamilyTest("GenuineIntel", "Intel(R) Pentium(R) III Xeon CPU @ 2.13GHz", 176);
        OneFamilyTest("GenuineIntel", "Intel(R) Pentium(R) 4 CPU @ 2.13GHz", 178);
        OneFamilyTest("GenuineIntel", "Intel(R) Pentium(R) M CPU @ 2.13GHz", 185);
        OneFamilyTest("GenuineIntel", "Intel(R) Celeron(R) CPU @ 2.13GHz", 15);

        OneFamilyTest("AuthenticAMD", "", 2);
        OneFamilyTest("AuthenticAMD", "AMD-K5(tm) Processor", 25);
        OneFamilyTest("AuthenticAMD", "MOBILE AMD-K5(tm) Processor", 25);
        OneFamilyTest("AuthenticAMD", "DUAL CORE AMD-K5(tm) Processor", 25);
        OneFamilyTest("AuthenticAMD", "AMD-K6(tm) Processor", 26);
        OneFamilyTest("AuthenticAMD", "AMD-K7(tm) Processor", 190);
        OneFamilyTest("AuthenticAMD", "AMD Processor", 2);
        OneFamilyTest("AuthenticAMD", "AMD Athlon(tm) Processor", 29);
        OneFamilyTest("AuthenticAMD", "AMD Athlon(tm) 64 Processor", 131);
        OneFamilyTest("AuthenticAMD", "AMD Athlon(tm) XP Processor", 182);
        OneFamilyTest("AuthenticAMD", "AMD Duron(tm) Processor", 24);
        OneFamilyTest("AuthenticAMD", "AMD Opteron(tm) Processor", 132);
#endif
    }

#if defined(linux)
    void testGetCpuInfoWithoutPhysicalid()
    {
        //----------- test scenario, no physical id exist in cpuinfo table----------
        SCXHandle<CPUInfoTestDependencies> deps(new CPUInfoTestDependencies());
        deps->SetCpuInfoFileType(0);
        SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> filehandle = SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> (new ProcfsCpuInfoReader(deps));
        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum(filehandle);
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);

        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetRole(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"Central Processor", strtmp);

        strtmp = L"";
        CPPUNIT_ASSERT(inst->GetDeviceID(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"CPU 1", strtmp);

        CPPUNIT_ASSERT(inst->GetManufacturer(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"GenuineIntel", strtmp);

        unsigned short tmp = 0;
        CPPUNIT_ASSERT(inst->GetFamily(tmp));
        CPPUNIT_ASSERT_EQUAL(179, tmp);
         
        CPPUNIT_ASSERT(inst->GetStepping(strtmp));
        CPPUNIT_ASSERT_EQUAL("2",StrToUTF8(strtmp));
       
        tmp = 0;
        CPPUNIT_ASSERT(inst->GetCpuStatus(tmp));
        CPPUNIT_ASSERT_EQUAL(1, tmp);
        
        tmp = 0;
        CPPUNIT_ASSERT(inst->GetUpgradeMethod(tmp));
        CPPUNIT_ASSERT_EQUAL(2, tmp);

        unsigned int itmp = 0;
        CPPUNIT_ASSERT(inst->GetCurrentClockSpeed(itmp)); 
        CPPUNIT_ASSERT_EQUAL((unsigned int)2104, itmp); 

        CPPUNIT_ASSERT(inst->GetName(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz", strtmp);
        
        CPPUNIT_ASSERT(inst->GetDescription(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"GenuineIntel Family 179 Model 12 Stepping 2", strtmp);
        
        bool btmp = false;
        CPPUNIT_ASSERT(inst->GetIs64Bit(btmp));
        CPPUNIT_ASSERT_EQUAL(true, btmp);

        btmp = false; 
        CPPUNIT_ASSERT(inst->GetIsHyperthreadCapable(btmp));
        CPPUNIT_ASSERT_EQUAL(true, btmp);

        //true if vme || svm || vmx
        CPPUNIT_ASSERT(inst->GetIsVirtualizationCapable(btmp)); 
        CPPUNIT_ASSERT_EQUAL(true, btmp);
    }

    void testGetCpuInfoWithPhysicalIDOnechip()
    {
        //----------- test scenario, physical id exist in cpuinfo table----------
        SCXHandle<CPUInfoTestDependencies> deps(new CPUInfoTestDependencies());
        deps->SetCpuInfoFileType(1);
        SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> filehandle = SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> (new ProcfsCpuInfoReader(deps));
        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum(filehandle);
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);

        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetRole(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"Central Processor", strtmp);

        strtmp = L"";
        CPPUNIT_ASSERT(inst->GetDeviceID(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"CPU 0", strtmp);

        unsigned short tmp = 0;
        CPPUNIT_ASSERT(inst->GetCpuStatus(tmp));
        CPPUNIT_ASSERT_EQUAL(1, tmp);

        tmp = 0;
        CPPUNIT_ASSERT(inst->GetUpgradeMethod(tmp));
        CPPUNIT_ASSERT_EQUAL(2, tmp);

        unsigned int itmp = 0;
        CPPUNIT_ASSERT(inst->GetCurrentClockSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL((unsigned int)2132, itmp);
    }
        
    void testGetCpuInfoWithPhysicalIDTwochip()
    {
        //----------- test scenario, physical id exist in cpuinfo table, 2 chips and each with 2 cores on it----------
        SCXHandle<CPUInfoTestDependencies> deps(new CPUInfoTestDependencies());
        deps->SetCpuInfoFileType(2);
        SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> filehandle = SCXHandle<SCXSystemLib::ProcfsCpuInfoReader> (new ProcfsCpuInfoReader(deps));
        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum(filehandle);
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);

        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetRole(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"Central Processor", strtmp);

        strtmp = L"";
        CPPUNIT_ASSERT(inst->GetDeviceID(strtmp));
        CPPUNIT_ASSERT_EQUAL(L"CPU 0", strtmp);

        CPPUNIT_ASSERT(inst->GetStepping(strtmp));
        CPPUNIT_ASSERT_EQUAL("2",StrToUTF8(strtmp));

        unsigned short tmp = 0;
        CPPUNIT_ASSERT(inst->GetCpuStatus(tmp));
        CPPUNIT_ASSERT_EQUAL(1, tmp);

        tmp = 0;
        CPPUNIT_ASSERT(inst->GetUpgradeMethod(tmp));
        CPPUNIT_ASSERT_EQUAL(2, tmp);

        unsigned int itmp = 0;
        CPPUNIT_ASSERT(inst->GetCurrentClockSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL((unsigned int)2132, itmp);
    }
#endif

    void testGetCpuPropertiesAttr()
    {
#if defined(linux)
        testGetCpuInfoWithoutPhysicalid();
        testGetCpuInfoWithPhysicalIDOnechip();
        testGetCpuInfoWithPhysicalIDTwochip();
#elif defined(sun)
        testGetCpuPropertiesAttrSun();
#elif defined(hpux)
        testGetCpuPropertiesAttrbyCmd();
        testGetCpuPropertiesAttrCPUChipInfo();
        testGetCpuPropertiesAttrDevID();
#endif
    }

    void testGetCpuPropertiesAttrSun()
    {
#if defined(sun)

        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum;
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);
        inst->Update();

        std::string deviceId = "CPU ";
        unsigned short family(0);
        unsigned int normSpeed(0), currentSpeed(0);
        std::string stepping;

        string manufacturer, name, chipId, model;
        bool firstModule=false;

        std::istringstream streamInstr;
        std::ostringstream streamOutstr;
        std::ostringstream streamErrstr;
        std::string stdoutStr;
        SCXCoreLib::SCXStream::NLFs nlfs;
        vector<wstring> outLines;
        vector<wstring> tokens;

        wstring cmdKstatCpuinfo = L"kstat cpu_info"; 
        int procRet = SCXCoreLib::SCXProcess::Run(cmdKstatCpuinfo, streamInstr, streamOutstr, streamErrstr, 250000);
       
        if (procRet == 0 && streamErrstr.str().empty())
        {
            stdoutStr = streamOutstr.str();
            std::istringstream stdInStr(stdoutStr);
            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stdInStr, outLines, nlfs);

            for(unsigned int i=0; i<outLines.size(); i++)
            {
                StrTokenize(outLines[i], tokens, L" ");
                if (tokens.size() > 1)
                {
                    if (0 == tokens[0].compare(L"module:"))
                    {
                        if (firstModule == false)
                        {
                            firstModule = true;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if (0 == tokens[0].compare(L"chip_id"))
                    {
                        chipId = StrToUTF8(tokens[1]);
                        deviceId = deviceId + chipId;
                    }
                    else if (0 == tokens[0].compare(L"vendor_id"))
                    {
                        manufacturer = StrToUTF8(tokens[1]);
                    }
                    else if (0 == tokens[0].compare(L"stepping"))
                    {
                        stepping = StrToUTF8(tokens[1]);
                    }
                    else if (0 == tokens[0].compare(L"family"))
                    {
                        family = StrToUInt(tokens[1]);
                    }
                    else if (0 == tokens[0].compare(L"clock_MHz"))
                    {
                        normSpeed = StrToUInt(tokens[1]);
                        // (WI 520507) Set currentSpeed to be the normSpeed, in case
                        // it is not reported.
                        currentSpeed = normSpeed;
                    }
                    else if (0 == tokens[0].compare(L"current_clock_Hz"))
                    {
                        currentSpeed = StrToUInt(tokens[1])/1000000;
                    }
                    else if (0 == tokens[0].compare(L"brand"))
                    {
                        name = StrToUTF8(tokens[1]);
                    }
                    else if (0 == tokens[0].compare(L"model"))
                    {
                        model = StrToUTF8(tokens[1]);
                    }
                }
            }
        } 
        else
        {
            cout << "Kstat cpu_info command Run failed. The return value is : " << procRet << endl;
            cout << "The ErrorString is : " << streamErrstr.str() << endl;
        }

        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetDeviceID(strtmp));
        CPPUNIT_ASSERT_EQUAL(deviceId, StrToUTF8(strtmp));

        CPPUNIT_ASSERT(!inst->GetRole(strtmp));

#if !defined(sparc)
        std::stringstream ss;
        ss << manufacturer << " Family " << family << " Model " << model << " Stepping " << stepping;
        CPPUNIT_ASSERT(inst->GetDescription(strtmp));
        CPPUNIT_ASSERT_EQUAL(ss.str(), StrToUTF8(strtmp));
#endif
        
        unsigned int itmp = 0;
#if defined(ia32)
        CPPUNIT_ASSERT(inst->GetManufacturer(strtmp));
        CPPUNIT_ASSERT_EQUAL(manufacturer, StrToUTF8(strtmp));

        CPPUNIT_ASSERT(inst->GetStepping(strtmp));
        CPPUNIT_ASSERT_EQUAL(stepping, StrToUTF8(strtmp));
        
        unsigned short tmp; 
        CPPUNIT_ASSERT(inst->GetFamily(tmp));
        CPPUNIT_ASSERT_EQUAL(family, tmp);
     
        std::stringstream version(std::stringstream::out);
        version << "Model " << model << " Stepping " << stepping;      
        CPPUNIT_ASSERT(inst->GetVersion(strtmp));
        CPPUNIT_ASSERT_EQUAL(version.str(), StrToUTF8(strtmp));
#endif

#if defined(ia32) || (defined(sparc) && PF_MINOR >= 11)
        CPPUNIT_ASSERT(inst->GetCurrentClockSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL(currentSpeed, itmp);
#endif
        itmp = 0;
        CPPUNIT_ASSERT(inst->GetNormSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL(normSpeed, itmp);
        
        CPPUNIT_ASSERT(inst->GetName(strtmp));
        CPPUNIT_ASSERT(!strtmp.empty());
#endif
    }


    void testGetCpuPropertiesAttrbyCmd()
    {
#if defined(hpux) && ((PF_MAJOR > 11) || (PF_MINOR >= 31))
        std::istringstream streamInstr;
        std::ostringstream streamOutstr;
        std::ostringstream streamErrstr;
        int procRet;
        std::string stdoutStr;

        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum;
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);
        inst->Update();

        unsigned int numPhysicalCPU = 0;
        unsigned int clockspeed = 0;  
        wstring strClockspeed;
        wstring manufacturer;

        // Sample output of command in hp risc machine
        // cmd: /usr/contrib/bin/machinfo
        // CPU info:
        //  1 PA-RISC 8800 processor (1 GHz, 64 MB)
        //  CPU version 5
        //  2 logical processors (2 per socket)
        // Sample output of command in IA64 machine
        // cmd: /usr/contrib/bin/machinfo
        // CPU info:
        // 2 Intel(R) Itanium 2 9100 series processors (1.42 GHz, 6 MB)
        //  266 MHz bus, CPU version A1

  
        wstring cmdStrMachInfo = L"/usr/contrib/bin/machinfo -v";
        SCXCoreLib::SCXStream::NLFs nlfs;
        vector<wstring> outLines;
        vector<wstring> tokens;
        vector<wstring> newTokens;

        procRet = SCXCoreLib::SCXProcess::Run(cmdStrMachInfo, streamInstr, streamOutstr, streamErrstr, 250000);
     
        if (procRet == 0 && streamErrstr.str().empty())
        {
            stdoutStr = streamOutstr.str();
            std::istringstream stdInStr(stdoutStr);

            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stdInStr, outLines, nlfs);

            for(unsigned int i=0; i<outLines.size(); i++)
            {
                StrTokenize(outLines[i], tokens, L":");
                if (tokens.size() == 1)
                {
                    // To cover the case: 1 PA-RISC 8800 processor (1 GHz, 64 MB)
                    StrTokenize(tokens[0], newTokens, L" "); 
                    if ( 8 == newTokens.size() && 0 == newTokens[1].compare(L"PA-RISC") )
                    {
                        numPhysicalCPU = StrToUInt(newTokens[0]);
                        strClockspeed = StrStripL(newTokens[4], L"(");
                        if (newTokens[5].compare(L"MHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed));
                        }
                        else if (newTokens[5].compare(L"GHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed)*1000);
                        }
                    }
                    else if ( 9 == newTokens.size() && 0 == newTokens[1].compare(L"Intel(R)") )
                    {
                        // To cover the case: 8 Intel(R)  Itanium(R)  Processor 9540s (2.13 GHz, 24 MB)
                        numPhysicalCPU = StrToUInt(newTokens[0]);
                        strClockspeed = StrStripL(newTokens[5],  L"(");
                        if (newTokens[6].compare(L"MHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed));
                        }
                        else if (newTokens[6].compare(L"GHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed)*1000);
                        }
                    }
                    else if ( 11 == newTokens.size() && 0 == newTokens[1].compare(L"Intel(R)") )
                    {
                        // To cover the case: 2 Intel(R) Itanium 2 9100 series processors (1.42 GHz, 6 MB)
                        numPhysicalCPU = StrToUInt(newTokens[0]);
                        strClockspeed = StrStripL(newTokens[7],  L"(");
                        if (newTokens[8].compare(L"MHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed));
                        }
                        else if (newTokens[8].compare(L"GHz,") == 0)
                        {
                            clockspeed = (unsigned int)(StrToDouble(strClockspeed)*1000);
                        }
                    }
                }
                else if(tokens.size() > 1)
                {
                    if (0 == tokens[0].compare(L"Vendor identification") )
                    {
                        manufacturer = tokens[1];
                        break;
                    }
                }
            }
        }
        else
        {
            cout << "Machinfo command Run failed. The return value is : " << procRet << endl;
            cout << "The ErrorString is : " << streamErrstr.str() << endl;
        }

        // Validate that the number of instances is the same as what machinfo utility indicated
        CPPUNIT_ASSERT_EQUAL(numPhysicalCPU, cpuPropertiesEnum.Size());

        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetManufacturer(strtmp));
        CPPUNIT_ASSERT_EQUAL(manufacturer, strtmp);

        unsigned int itmp = 0; // clock speed in MHz.
        CPPUNIT_ASSERT(inst->GetCurrentClockSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL(clockspeed, (floor((itmp + 5)/10.0)*10));

        itmp = 0;
        CPPUNIT_ASSERT(inst->GetMaxClockSpeed(itmp));
        CPPUNIT_ASSERT_EQUAL(clockspeed, (floor((itmp + 5)/10.0)*10));
#endif //version checking
    }
    void testGetCpuPropertiesAttrDevID()
    {
#if defined(hpux) && ((PF_MAJOR > 11) || (PF_MINOR >= 31))

        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum;
        cpuPropertiesEnum.Init();
        unsigned int numInst = cpuPropertiesEnum.Size();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst;

        if (numInst > 1)
        {
            inst = cpuPropertiesEnum.GetInstance(1);
        }
        else
        {
            inst = cpuPropertiesEnum.GetInstance(0);
        }

        inst->Update();

        //device ID
        wstring strtmp = L"";
        CPPUNIT_ASSERT(inst->GetDeviceID(strtmp));
        if (numInst > 1)
        {
            CPPUNIT_ASSERT_EQUAL(L"CPU 1", strtmp);
        }
        else
        {
            CPPUNIT_ASSERT_EQUAL(L"CPU 0", strtmp);
        }
#endif
    }

    void testGetCpuPropertiesAttrCPUChipInfo()
    {
#if defined(hpux)
        std::istringstream streamInstr;
        std::ostringstream streamOutstr;
        std::ostringstream streamErrstr;
        int procRet;
        std::string stdoutStr;

        SCXSystemLib::CpuPropertiesEnumeration cpuPropertiesEnum;
        cpuPropertiesEnum.Init();
        SCXCoreLib::SCXHandle<CpuPropertiesInstance> inst = cpuPropertiesEnum.GetInstance(0);
        inst->Update();

        wstring cmdGetconf = L"getconf _SC_CPU_CHIP_TYPE";
        SCXCoreLib::SCXStream::NLFs nlfs;
        vector<wstring> outLines;
        vector<wstring> tokens;
        vector<wstring> newTokens;


        procRet = SCXCoreLib::SCXProcess::Run(cmdGetconf, streamInstr, streamOutstr, streamErrstr, 250000);

        if (procRet == 0 && streamErrstr.str().empty())
        {
            stdoutStr.clear();
            stdoutStr = streamOutstr.str();
            std::istringstream stdInStr(stdoutStr);
            uint64_t cpuChipVal;
            stdInStr >> cpuChipVal;
            unsigned short stepping = (cpuChipVal >> 8) & 0xFF;
            unsigned short model =  (cpuChipVal >> 16) & 0xFF ;
            std::stringstream version(std::stringstream::out);
            version << "Model " << model << " Stepping " << stepping;
 
            wstring strtmp = L""; 
            CPPUNIT_ASSERT(inst->GetStepping(strtmp));
            CPPUNIT_ASSERT_EQUAL(stepping, StrToUInt(strtmp));
            
            strtmp = L"";
            CPPUNIT_ASSERT(inst->GetVersion(strtmp));
            CPPUNIT_ASSERT_EQUAL(version.str(), StrToUTF8(strtmp)); 
        }
        else
        {
            cout << "getconf command Run failed. The return value is : " << procRet << endl;
            cout << "The ErrorString is : " << streamErrstr.str() << endl;
        }
#endif
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( CpuProperties_test );
