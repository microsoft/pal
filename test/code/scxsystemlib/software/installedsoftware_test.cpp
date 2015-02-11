/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2011-01-25 14:58:11

  Installed Software colletion test class.

  This class tests the Linux, Solaris, and HP/UX implementations.

  It checks the result of installed software detail information. 

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#if defined(hpux)
#include <scxcorelib/scxdirectoryinfo.h>
#endif

#include <scxsystemlib/installedsoftwareenumeration.h>
#include <scxsystemlib/installedsoftwareinstance.h>

#include <testutils/scxunit.h>

#include <stdio.h>
#include <vector>

#include <iostream>

using namespace std;
using namespace SCXCoreLib;
using namespace SCXSystemLib;

#if defined(PF_DISTRO_ULINUX)
/**
   Class for injecting a DPKG status file location to test the DPKG parser
 */
class TestDPKGSoftwareDependencies : public SoftwareDependencies
{
public:
    virtual std::wstring GetDPKGStatusLocation()
    {
        return wstring(L"./testfiles/dpkg_status_test");
    }
    
};
#endif

/**
    Class for injecting test behavior into the InstalledSoftware.
 */
class InstalledSoftwareTestDependencies : public InstalledSoftwareDependencies
{

#if defined(hpux)
#define TEST_PUBLISHER          L"ISTest Vendor Title"
#define TEST_DISPLAY_NAME       L"ISTest Product Title"
#define TEST_PRODUCT_NAME       L"ISTestProdId"
#define TEST_INSTALL_SOURCE     L"scx_anyHPmachine.scx.com:/var/opt/install_source"
#define TEST_INSTALL_DATE       L"201201312359.59"
#define TEST_INSTALLED_LOCATION L"/install_location"
#define TEST_PRODUCT_VERSION    L"B.11.31.0909"
#define TEST_VERSION_MAJOR      11
#define TEST_VERSION_MINOR      31
    SCXFilePath m_TestProduct;
    SCXFilePath m_TempDirectory;
#endif

public:
    InstalledSoftwareTestDependencies()
    {
#if defined(hpux)
        vector<wstring> INDEX_lines;

        SCXDirectoryInfo tmpDirInfo = SCXDirectory::CreateTempDirectory();
        m_TempDirectory = tmpDirInfo.GetFullPath();
        m_TestProduct = m_TempDirectory;
        m_TestProduct.Append(L"/ISTest/");
        SCXFilePath testProductIndex = m_TestProduct;
        testProductIndex.Append(L"pfiles/");
        SCXDirectoryInfo result = SCXDirectory::CreateDirectory(testProductIndex);
        CPPUNIT_ASSERT(result.PathExists());
        testProductIndex.SetFilename(L"INDEX");

        INDEX_lines.push_back(L"vendor");
        INDEX_lines.push_back(L"tag HP");
        INDEX_lines.push_back(L"title \"" TEST_PUBLISHER L"\"");
        INDEX_lines.push_back(L"description \"ISTest Vendor Description\"");
        INDEX_lines.push_back(L"end");
        INDEX_lines.push_back(L"product");
        INDEX_lines.push_back(L"tag " TEST_PRODUCT_NAME);
        INDEX_lines.push_back(L"data_model_revision 9999.9999");
        INDEX_lines.push_back(L"instance_id 1");
        INDEX_lines.push_back(L"control_directory ISTest");
        INDEX_lines.push_back(L"revision " TEST_PRODUCT_VERSION);
        INDEX_lines.push_back(L"title \"" TEST_DISPLAY_NAME L"\"");
        INDEX_lines.push_back(L"description \"Vendor Name                     ISTest vendor name");
        INDEX_lines.push_back(L"");
        INDEX_lines.push_back(L"Product Name                    ISTest product name");
        INDEX_lines.push_back(L"");
        INDEX_lines.push_back(L"The test software product introductory paragraph goes here.");
        INDEX_lines.push_back(L"\"");
        INDEX_lines.push_back(L"mod_time 1294005058");
        INDEX_lines.push_back(L"create_time 1294003675");
        INDEX_lines.push_back(L"install_date " TEST_INSTALL_DATE);
        INDEX_lines.push_back(L"architecture HP-UX_B.11.31_IA");
        INDEX_lines.push_back(L"machine_type ia64*");
        INDEX_lines.push_back(L"os_name HP-UX");
        INDEX_lines.push_back(L"os_release B.11.31");
        INDEX_lines.push_back(L"os_version *");
        INDEX_lines.push_back(L"install_source " TEST_INSTALL_SOURCE);
        INDEX_lines.push_back(L"install_type physical");
        INDEX_lines.push_back(L"vendor_tag HP");
        INDEX_lines.push_back(L"directory " TEST_INSTALLED_LOCATION);
        INDEX_lines.push_back(L"all_filesets TestProductFileset");
        INDEX_lines.push_back(L"is_locatable false");
        INDEX_lines.push_back(L"location /");
        INDEX_lines.push_back(L"copyright \"(c)Copyright 2000 Test Company, L.P.\"");
        INDEX_lines.push_back(L"");
        INDEX_lines.push_back(L"Proprietary computer software. Valid license from HP required for");
        INDEX_lines.push_back(L"possession, use or copying. Consistent with FAR 12.211 and 12.212,");
        INDEX_lines.push_back(L"Commercial Computer Software, Computer Software Documentation, and");
        INDEX_lines.push_back(L"Technical Data for Commercial Items are licensed to the U.S. Government");
        INDEX_lines.push_back(L"under vendor's standard commercial license.");
        INDEX_lines.push_back(L"\"");
        INDEX_lines.push_back(L"readme <README");

        SCXFile::WriteAllLines(testProductIndex, INDEX_lines, ios_base::out);
#endif
    }

#if defined(hpux)
    virtual ~InstalledSoftwareTestDependencies()
    {
        int failure = 0;
        wstring productPath(m_TestProduct.GetDirectory() + L"pfiles/");
        wstring indexFile(productPath + L"INDEX");

        SCXFile::Delete(indexFile);

        if (!productPath.empty())
        {
            failure = rmdir(StrToUTF8(productPath).c_str());
            if (!failure)
            {
                productPath = m_TestProduct.GetDirectory();
                if (!productPath.empty())
                {
                    failure = rmdir(StrToUTF8(productPath).c_str());
                    if (!failure)
                    {
                        productPath = m_TempDirectory.GetDirectory();
                        if (!productPath.empty())
                        {
                            failure = rmdir(StrToUTF8(productPath).c_str());
                        }
                    }
                }
            }

            if (failure)
            {
                std::wcout << L"Failed to remove directory " << productPath << L" errno: " << errno << std::endl;
            }
        }

    }
#endif

    void GetInstalledSoftwareIds(std::vector<wstring>& ids)
    {
#if defined(hpux)
        ids.push_back(m_TestProduct);
#else
        ids.push_back(L"ISTest");
#endif
    }

#if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX)
        void GetSoftwareInfoRawData(const wstring& softwareName, std::vector<wstring>& contents)
        {
            // Do nothing, just avoid compiler error
            softwareName.size();

            contents.push_back(L"Name:ISTest");
            contents.push_back(L"Version:1.0");
            contents.push_back(L"Vendor:ISTest, Inc.");
            contents.push_back(L"Release:26");
            contents.push_back(L"BuildTime:1000000000");
            contents.push_back(L"InstallTime:1000000000");
            contents.push_back(L"BuildHost:ISTest.com");
            contents.push_back(L"Group:Development/Libraries");
            contents.push_back(L"SourceRPM:ISTest1.0-27.2.src.rpm");
            contents.push_back(L"License:GPL");
            contents.push_back(L"Packager:ISTest, Inc. <_http://ISTest>");
            contents.push_back(L"Summary:Summary:ISTest.");
        }
#endif

#if defined(sun)
        void GetAllLinesOfPkgInfo(const wstring& pkgFile, std::vector<wstring>& allLines)
        {
            allLines.push_back(L"CLASSES=none preserve");
            allLines.push_back(L"BASEDIR=/");
            allLines.push_back(L"LANG=C");
            allLines.push_back(L"PATH=/sbin:/usr/sbin:/usr/bin:/usr/sadm/install/bin");
            allLines.push_back(L"OAMBASE=/usr/sadm/sysadm");
            allLines.push_back(L"ARCH=i386");
            allLines.push_back(L"CATEGORY=system");
            allLines.push_back(L"DESC=Installed software test case data.");
            allLines.push_back(L"EMAIL=");
            allLines.push_back(L"HOTLINE=Please contact your local service provider");
            allLines.push_back(L"MAXINST=1000");
            allLines.push_back(L"NAME=ISTest");
            allLines.push_back(L"PKG=ISTest");
            allLines.push_back(L"SUNW_PKGTYPE=root");
            allLines.push_back(L"SUNW_PKGVERS=1.0");
            allLines.push_back(L"SUNW_PKG_ALLZONES=true");
            allLines.push_back(L"SUNW_PKG_HOLLOW=true");
            allLines.push_back(L"SUNW_PRODNAME=SunOS");
            allLines.push_back(L"SUNW_PRODVERS=5.10/Generic Patch");
            allLines.push_back(L"VENDOR=ISTest, Inc.");
            allLines.push_back(L"VERSION=1.0.26,REV=2011.01.28.12.22");
            allLines.push_back(L"PSTAMP=on10-patch-x20051208060844");
            allLines.push_back(L"PATCHLIST=121805-03 113000-07");
            allLines.push_back(L"PATCH_INFO_121805-03=Installed: Tue Nov 24 16:34:53 PST 2006 From: mum Obsoletes:  Requires:  Incompatibles:");
            allLines.push_back(L"PATCH_INFO_113000-07=Installed: Tue Nov 24 10:39:39 PST 2006 From: mum Obsoletes:  Requires: 119255-08 121127-01 Incompatibles:");
            allLines.push_back(L"PKGINST=ISTest");
            allLines.push_back(L"PKGSAV=/var/sadm/pkg/ISTest/save");
            allLines.push_back(L"INSTDATE=Sep 9 2001 1:46");
        }
#endif

};


class InstalledSoftware_test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(InstalledSoftware_test);

    CPPUNIT_TEST(testGetSoftwareAttr);
#if defined(PF_DISTRO_ULINUX)
    CPPUNIT_TEST(testDPKGParser_Version);
    CPPUNIT_TEST(testDPKGParser_UTF);
#else
    CPPUNIT_TEST(testInstallDate);
#endif
    SCXUNIT_TEST_ATTRIBUTE(testGetSoftwareAttr,SLOW);

    CPPUNIT_TEST_SUITE_END();

public:

    InstalledSoftwareEnumeration* m_pEnum;

    void setUp(void)
    {
        m_pEnum = 0;
    }

    void tearDown(void)
    {
        if (m_pEnum != 0)
        {
            m_pEnum->CleanUp();
            delete m_pEnum;
            m_pEnum = 0;
        }
    }

    void testGetSoftwareAttr()
    {
#if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX) || defined(sun)
        // Mock dependencies object
        SCXHandle<InstalledSoftwareTestDependencies> deps(new InstalledSoftwareTestDependencies());
        m_pEnum = new InstalledSoftwareEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->Update();

        //Get first instance
        SCXCoreLib::SCXHandle<InstalledSoftwareInstance> inst = m_pEnum->GetInstance(0);
        CPPUNIT_ASSERT(0 != inst);

        inst->Update();
        wstring tmpData = L"";
        unsigned int scxuint32tmpData = 0;
        SCXCalendarTime tmpInstallDate;

        std::string ARPDisplayName = "ISTest";
        std::string InstallSource = "ISTest1.0-27.2.src.rpm";
        std::string InstalledLocation = "/";
        std::string ProductName = "ISTest";
    #if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX)
        std::string ProductVersion = "1.0";
    #elif defined(sun)
        std::string ProductVersion = "1.0.26,REV=2011.01.28.12.22";
    #endif
        std::string Publisher = "ISTest, Inc.";
        std::string EvidenceSource = "M";
        unsigned int VersionMajor = 1;
        //unsigned int VersionMinor = 0;
        //SCXCalendarTime InstallDate = SCXCalendarTime::FromPosixTime(1000000000);

    #if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX)
        CPPUNIT_ASSERT(inst->GetDisplayName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ARPDisplayName);

        CPPUNIT_ASSERT(inst->GetEvidenceSource(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), EvidenceSource);

        CPPUNIT_ASSERT(inst->GetInstallSource(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), InstallSource);

        CPPUNIT_ASSERT(inst->GetProductName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ProductName);

        CPPUNIT_ASSERT(inst->GetProductVersion(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ProductVersion);

        CPPUNIT_ASSERT(inst->GetPublisher(tmpData) );
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), Publisher);

        CPPUNIT_ASSERT(inst->GetVersionMajor(scxuint32tmpData) );
        CPPUNIT_ASSERT_EQUAL(scxuint32tmpData, VersionMajor);

        CPPUNIT_ASSERT(inst->GetVersionMinor(scxuint32tmpData));
        //CPPUNIT_ASSERT_EQUAL(scxuint32tmpData, VersionMinor);

        CPPUNIT_ASSERT(inst->GetInstallDate(tmpInstallDate));
        /*CPPUNIT_ASSERT_EQUAL(tmpInstallDate, InstallDate);*/

    #elif defined(sun)
        CPPUNIT_ASSERT(inst->GetDisplayName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ARPDisplayName);

        CPPUNIT_ASSERT(inst->GetEvidenceSource(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), EvidenceSource);

        CPPUNIT_ASSERT(inst->GetInstalledLocation(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), InstalledLocation);

        CPPUNIT_ASSERT(inst->GetProductName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ProductName);

        CPPUNIT_ASSERT(inst->GetProductVersion(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), ProductVersion);

        CPPUNIT_ASSERT(inst->GetPublisher(tmpData) );
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(tmpData), Publisher);

        CPPUNIT_ASSERT(inst->GetVersionMajor(scxuint32tmpData) );
        CPPUNIT_ASSERT_EQUAL(scxuint32tmpData, VersionMajor);

        CPPUNIT_ASSERT(inst->GetVersionMinor(scxuint32tmpData));
        //CPPUNIT_ASSERT_EQUAL(scxuint32tmpData, VersionMinor);

        CPPUNIT_ASSERT(inst->GetInstallDate(tmpInstallDate));
        //CPPUNIT_ASSERT_EQUAL(tmpInstallDate, InstallDate);

    #endif

#elif defined(hpux)
        // Mock dependencies object
        SCXHandle<InstalledSoftwareTestDependencies> deps(new InstalledSoftwareTestDependencies());
        std::cout << std::endl;
        m_pEnum = new InstalledSoftwareEnumeration(deps);
        m_pEnum->Init();
        m_pEnum->Update();

        //Get first instance
        SCXCoreLib::SCXHandle<InstalledSoftwareInstance> inst = m_pEnum->GetInstance(0);
        CPPUNIT_ASSERT(0 != inst);

        wstring tmpData = L"";
        unsigned int scxuint32tmpData = 0;
        SCXCalendarTime tmpInstallDate;

        wstring ARPDisplayName = TEST_DISPLAY_NAME;
        wstring InstallSource = TEST_INSTALL_SOURCE;
        wstring InstalledLocation = TEST_INSTALLED_LOCATION;
        wstring ProductName = TEST_PRODUCT_NAME;
        wstring ProductVersion = TEST_PRODUCT_VERSION;
        wstring Publisher = TEST_PUBLISHER;
        unsigned int VersionMajor = TEST_VERSION_MAJOR;
        unsigned int VersionMinor = TEST_VERSION_MINOR;

        std::cout << "Testing DisplayName" << std::endl;
        CPPUNIT_ASSERT(inst->GetDisplayName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(/*Expected*/ARPDisplayName), StrToUTF8(/*Actual*/tmpData));

        std::cout << "Testing InstalledLocation" << std::endl;
        CPPUNIT_ASSERT(inst->GetInstalledLocation(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(InstalledLocation), StrToUTF8(tmpData));

        std::cout << "Testing ProductName" << std::endl;
        CPPUNIT_ASSERT(inst->GetProductName(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(ProductName), StrToUTF8(tmpData));

        std::cout << "Testing ProductVersion" << std::endl;
        CPPUNIT_ASSERT(inst->GetProductVersion(tmpData));
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(ProductVersion), StrToUTF8(tmpData));

        std::cout << "Testing Publisher" << std::endl;
        CPPUNIT_ASSERT(inst->GetPublisher(tmpData) );
        CPPUNIT_ASSERT_EQUAL(StrToUTF8(Publisher), StrToUTF8(tmpData));

        std::cout << "Testing Version Major" << std::endl;
        CPPUNIT_ASSERT(inst->GetVersionMajor(scxuint32tmpData) );
        CPPUNIT_ASSERT_EQUAL(VersionMajor, scxuint32tmpData);

        std::cout << "Testing Version Minor" << std::endl;
        CPPUNIT_ASSERT(inst->GetVersionMinor(scxuint32tmpData));
        CPPUNIT_ASSERT_EQUAL(VersionMinor, scxuint32tmpData);

        std::cout << "Testing Install Date" << std::endl;
        CPPUNIT_ASSERT(inst->GetInstallDate(tmpInstallDate));
#endif
    }

#if defined(PF_DISTRO_ULINUX)
    /*
      This unit test injects a dpkg status file to be parsed by the InstalledSoftwareDependencies class.
      The file is parsed on creation and stored throughout the lifetime of the InstalledSoftwareDependencies object,
      and this is why this function is composed of multiple tests.
     */
    void testDPKGParser_Version()
    {
        /*
          This tests basic functionality for the existence of 'testDPKGpackage' and that the versions match.
        */
        SCXCoreLib::SCXHandle<TestDPKGSoftwareDependencies> deps(new TestDPKGSoftwareDependencies());
        InstalledSoftwareDependencies dpkgSoftware(deps);

        const wstring package = L"testDPKGpackage";
        const wstring version = L"Version:8.6.q-16";
        std::vector<wstring> contents;
        dpkgSoftware.GetSoftwareInfoRawData(package, contents);
        
        CPPUNIT_ASSERT_MESSAGE("Unable to find expected package " + StrToUTF8(package) + " in dependency injected file", contents.size() > 0);
        
        wstring foundVersion = L"";
        for (std::vector<wstring>::iterator it = contents.begin(); it != contents.end(); it++)
        {
            if (it->substr(0,8) == L"Version:")
            {
                foundVersion = *it;
                break;
            }
        }
        
        CPPUNIT_ASSERT_MESSAGE("Unable to find Version key for package " + StrToUTF8(package), foundVersion != L"");
        string outmsg = "Versions do not match: " + StrToUTF8(version) + " != " + StrToUTF8(foundVersion);
        bool versionsMatch = (version == foundVersion) ? true : false;
        
        CPPUNIT_ASSERT_MESSAGE(outmsg, versionsMatch);
    }

    void testDPKGParser_UTF()    
    {
        /*
          This tests that the package 'lzma_dpkg', which has a wide character and exists near the middle of the dpkg_status_test file, is properly parsed.
        */
        SCXCoreLib::SCXHandle<TestDPKGSoftwareDependencies> deps(new TestDPKGSoftwareDependencies());
        InstalledSoftwareDependencies dpkgSoftware(deps);

        const wstring package = L"lzma_dpkg";
        const wstring version = L"Version:4.43-è14";
        std::vector<wstring> contents;
        dpkgSoftware.GetSoftwareInfoRawData(package, contents);
        
        CPPUNIT_ASSERT_MESSAGE("Unable to find expected package " + StrToUTF8(package) + " in dependency injected file", contents.size() > 0);
        
        wstring foundVersion = L"";
        for (std::vector<wstring>::iterator it = contents.begin(); it != contents.end(); it++)
        {
            if (it->substr(0,8) == L"Version:")
            {
                foundVersion = *it;
                break;
            }
        }
        
        CPPUNIT_ASSERT_MESSAGE("Unable to find Version key for package " + StrToUTF8(package), foundVersion != L"");
        string outmsg = "Versions do not match: " + StrToUTF8(version) + " != " + StrToUTF8(foundVersion);
        bool versionsMatch = (version == foundVersion) ? true : false;
        
        CPPUNIT_ASSERT_MESSAGE(outmsg, versionsMatch);
    }
#else  // defined(PF_DISTRO_ULINUX)
    /* This property should be implemented on platforms that do not use dpkg. Test it without mocking the deps */
    void testInstallDate()
    {
        m_pEnum = new InstalledSoftwareEnumeration();
        m_pEnum->Init();
        m_pEnum->Update();

        CPPUNIT_ASSERT_MESSAGE("The InstalledSoftwareEnumeration did not find any installed software", m_pEnum->Size() > 0);

        SCXCalendarTime installDate;
        SCXCalendarTime currentTime = SCXCalendarTime::CurrentLocal();
        wstring productName;
        ostringstream err_detail;
        bool hasInstallDate;
        int installDateFoundCount = 0;

        for(std::vector< SCXCoreLib::SCXHandle<InstalledSoftwareInstance> >::iterator it = m_pEnum->Begin();
            it != m_pEnum->End(); ++it)
        {
            productName = (*it)->GetId();
            hasInstallDate = (*it)->GetInstallDate(installDate);
            installDateFoundCount += hasInstallDate ? 1 : 0;
            
            if (hasInstallDate)
            {
                err_detail.str("");
                err_detail << StrToUTF8(productName)
                        << ", Current Time: " << StrToUTF8(currentTime.ToBasicISO8601())
                        << ", Install Time: " << StrToUTF8(installDate.ToBasicISO8601());
                CPPUNIT_ASSERT_MESSAGE("Found sofware installed in the future: " + err_detail.str() , currentTime > installDate);
                CPPUNIT_ASSERT_MESSAGE("Found software installed over 20 years ago: " + err_detail.str(), currentTime.GetYear() - installDate.GetYear() <= 20);
            }
        }
        std::cout << " " << installDateFoundCount << " / " << m_pEnum->Size();
        CPPUNIT_ASSERT_MESSAGE("Failed to retrieve install date of all instances", installDateFoundCount > 0);
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION( InstalledSoftware_test );
