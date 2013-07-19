/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test class the statistical disk pal for physical disks.

    \date        2008-06-18 11:08:56

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/statisticalphysicaldiskenumeration.h>
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxtestutils.h>

using namespace SCXSystemLib;

#include "diskdepend_mock.h"

class SCXStatisticalPhysicalDiskTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStatisticalPhysicalDiskTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
#if defined(sun)
    CPPUNIT_TEST( Test_Huge_Mount_Table_bug507232 );
    CPPUNIT_TEST( TestBug15583_DoNotDiscoverCdromForSolaris );
#endif
#if defined(hpux)
    CPPUNIT_TEST( TestBug6755_PartialHPUXDiscovery );
#endif
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskEnumeration> m_diskEnum;
    std::wstring fauxMntTab;

public:
    SCXStatisticalPhysicalDiskTest()
    {
        fauxMntTab = L"test_mnttab";
    }

    void setUp(void)
    {
        m_diskEnum = 0;
    }

    void tearDown(void)
    {
        unlink(SCXCoreLib::StrToUTF8(fauxMntTab).c_str());

        if (m_diskEnum != 0)
        {
            m_diskEnum->CleanUp();
            m_diskEnum = 0;
        }
    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        StatisticalPhysicalDiskInstance inst(deps);
        CPPUNIT_ASSERT(inst.DumpString().find(L"StatisticalDiskInstance") != std::wstring::npos);
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       Created for bug #507232 (RFC: File Descriptor leak in OM2007 R2 Xplat agent 
       on Solaris 8 and 10)
       This problem occurs on systems with a mount table with more than ~500 to ~1000 
       entries that are not ignored file systems.  There is a kstat handle that is opened
       for each StatisticalDiskInstance for performance reasons. This customer's system 
       had tens of thousands of 'mvfs' mount points, each of which  would have a 
       StatisticalDiskInstance, and thus a kstat handle.  This led to the failure of 
       'kstat_open' once too many kstat handles were open.  This problem was resolved by 
       including 'mvfs' in the ignored file systems array.

       \date            2013-02-14
    */
    void Test_Huge_Mount_Table_bug507232()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        deps->SetMountTabPath(L"./testfiles/bug507232_mnttab");

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::MockSolarisStatisticalPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       Created for Bug #15583 (QFE: CSS: Customer get 'disk full' alerts when
       mounting CD-roms). The problem occurs in the Statistical Logical Disk
       Enumeration (which reads /etc/mnttab).  From the Solaris documentation,
       we know that "the file /etc/mnttab is really a file system that provides
       read-only access to the table of mounted file systems for the current host."
       Thus for Solaris it is not sufficient to decide on the file system format, the
       device path must also be examined.

       \date            12/02/2009
    */
    void TestBug15583_DoNotDiscoverCdromForSolaris()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        deps->SetOpenErrno("/dev/dsk/c0t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s0",0); // Set to zero to fake file and ioctl operations.
        deps->SetOpenErrno("/dev/dsk/c0t0d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s7",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s0",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s3",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s7",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s0",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s3",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s7",0);

        struct stat statData;
        memset(&statData, 0, sizeof(statData));

        deps->SetStat("/dev/dsk/c0t0d0", statData);
        deps->SetStat("/dev/dsk/c0t0d0s0", statData);
        deps->SetStat("/dev/dsk/c0t0d0s0",statData);
        deps->SetStat("/dev/dsk/c0t0d0s1",statData);
        deps->SetStat("/dev/dsk/c0t0d0s2",statData);
        deps->SetStat("/dev/dsk/c0t0d0s4",statData);
        deps->SetStat("/dev/dsk/c0t0d0s5",statData);
        deps->SetStat("/dev/dsk/c0t0d0s6",statData);
        deps->SetStat("/dev/dsk/c0t0d0s7",statData);
        deps->SetStat("/dev/dsk/c9t0d0", statData);
        deps->SetStat("/dev/dsk/c9t0d0s0",statData);
        deps->SetStat("/dev/dsk/c9t0d0s1",statData);
        deps->SetStat("/dev/dsk/c9t0d0s2",statData);
        deps->SetStat("/dev/dsk/c9t0d0s3",statData);
        deps->SetStat("/dev/dsk/c9t0d0s4",statData);
        deps->SetStat("/dev/dsk/c9t0d0s5",statData);
        deps->SetStat("/dev/dsk/c9t0d0s6",statData);
        deps->SetStat("/dev/dsk/c9t0d0s7",statData);
        deps->SetStat("/dev/dsk/c9t1d0s0",statData);
        deps->SetStat("/dev/dsk/c9t1d0s1",statData);
        deps->SetStat("/dev/dsk/c9t1d0s2",statData);
        deps->SetStat("/dev/dsk/c9t1d0s3",statData);
        deps->SetStat("/dev/dsk/c9t1d0s4",statData);
        deps->SetStat("/dev/dsk/c9t1d0s5",statData);
        deps->SetStat("/dev/dsk/c9t1d0s6",statData);
        deps->SetStat("/dev/dsk/c9t1d0s7",statData);

        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        // Note: sample data comes from a Solaris 9 development box with a UFS CD
        // in the drive. The SPARC (not the x86) installation media is an example
        // of a CD-ROM with a UFS file system.
        fprintf(fp,
                "/dev/dsk/c9t0d0s0       /       ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800010  1258671407\n"
                "/proc   /proc   proc    dev=4600000     1258671406\n"
                "mnttab  /etc/mnttab     mntfs   dev=46c0000     1258671406\n"
                "fd      /dev/fd fd      rw,suid,dev=4700000     1258671407\n"
                "swap    /var/run        tmpfs   xattr,dev=1     1258671408\n"
                "swap    /tmp    tmpfs   xattr,dev=2     1258671409\n"
                "/dev/dsk/c9t0d0s7       /export/home    ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800017  1258671409\n"
                "-hosts  /net    autofs  indirect,nosuid,ignore,nobrowse,dev=4880001     1258671410\n"
                "auto_home       /home   autofs  indirect,ignore,nobrowse,dev=4880002    1258671410\n"
                "-xfn    /xfn    autofs  indirect,ignore,dev=4880003     1258671410\n"
                "scxsun14:vold(pid345)   /vol    nfs     ignore,noquota,dev=4840001      1258671413\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s6 /cdrom/sol_10_606_sparc/s6      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0007       1259791871\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s5 /cdrom/sol_10_606_sparc/s5      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0006       1259791871\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s4 /cdrom/sol_10_606_sparc/s4      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0005       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s3 /cdrom/sol_10_606_sparc/s3      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0004       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s2 /cdrom/sol_10_606_sparc/s2      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0003       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s1 /cdrom/sol_10_606_sparc/s1      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0002       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s0 /cdrom/sol_10_606_sparc/s0      hsfs    maplcase,noglobal,nosuid,ro,rr,traildot,dev=16c0001     1259791873\n");
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);

        deps->SetOpenErrno("/dev/dsk/c9t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s7", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s6", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s5", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s4", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s3", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s2", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s1", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s0", 0);

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::MockSolarisStatisticalPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        CPPUNIT_ASSERT_EQUAL_MESSAGE("Found the wrong number of disks from (a fake) MNT TAB file",
                static_cast<size_t>(1), m_diskEnum->Size());

        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> di = *(m_diskEnum->Begin());
        std::wstring expected(L"c9t0d0");
        std::wstring actual;
        CPPUNIT_ASSERT(di->GetDiskDeviceID(actual));

        // Verify that the name of the only device returned
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Received wrong name of for the discovered device.", 0, expected.compare(actual));
    }

#if defined(hpux)
    void TestBug6755_PartialHPUXDiscovery()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        deps->SetOpenErrno("/dev/rdisk/disk3",0); // Set to zero to fake file and ioctl operations.
        deps->SetOpenErrno("/dev/rdisk/disk5",ENXIO);
        deps->SetOpenErrno("/dev/rdisk/disk7",ENXIO);
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        fprintf(fp, 
                "/dev/vg00/lvol3 / vxfs ioerror=nodisable,log,dev=40000003 0 1 1213709666\n"
                "DevFS /dev/deviceFileSystem DevFS defaults,dev=4000000 0 0 1213709709\n"
                "-hosts /net autofs ignore,indirect,nosuid,soft,nobrowse,dev=4000003 0 0 1213709740\n");
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);
        
        SCXCoreLib::SCXHandle<LvmTabTest> lvmtab( new LvmTabTest() );
        lvmtab->AddVG(L"/dev/vg00", L"/dev/disk/disk3", L"/dev/disk/disk5");
        lvmtab->AddVG(L"/dev/vg01", L"/dev/disk/disk7");
        deps->SetLvmTab(lvmtab);
        
        memset(&m_diskInfo, 0, sizeof(m_diskInfo));
        m_diskInfo.psd_dev.psd_minor = 3;
        deps->SetPstDiskInfo(&m_diskInfo, 1);

        struct stat statData;
        memset(&statData, 0, sizeof(statData));

        deps->SetStat("/dev/disk/disk5", statData);
        deps->SetStat("/dev/disk/disk7", statData);
        statData.st_rdev = 3;        
        deps->SetStat("/dev/disk/disk3", statData);

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), m_diskEnum->Size());

        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> di = *m_diskEnum->Begin();

        std::wstring id;
        CPPUNIT_ASSERT(di->GetDiskName(id));
        CPPUNIT_ASSERT(L"disk3" == id);

/*
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalPhysicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXSystemLib::StatisticalPhysicalDiskInstance* di = *iter;

            std::wcout << std::endl << di->DumpString();
        }
*/
    }

private:
    // data for test case TestBug6755_PartialHPUXDiscovery - must live longer than funciotn live-time
    struct pst_diskinfo m_diskInfo;
    
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStatisticalPhysicalDiskTest );
