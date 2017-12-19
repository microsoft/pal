/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief       Disk PAL tests for static information on logical disks.

   \date        2008-03-19 12:06:07

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/staticlogicaldiskenumeration.h>
#include <scxsystemlib/staticlogicaldiskfullenumeration.h>
#include <scxsystemlib/staticlogicaldiskinstance.h>
#include <scxsystemlib/statisticallogicaldiskenumeration.h>
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxtestutils.h>
#include "diskdepend_mock.h"

#ifdef linux

/*
  A table of correct data for the Linux device type test
*/
struct LogicalDeviceAttr
{
     std::wstring DeviceName;
     std::wstring MountPoint;
     std::wstring FSType;
     unsigned int DriveType;
};

static LogicalDeviceAttr LogicalDeviceTable[] =
{
     { L"/dev/sda1",     L"/boot",        L"ext4",    3 },
     { L"/dev/hdc5",     L"/",            L"ext4",    3 },
     { L"/dev/ram",      L"/mnt/ramdisk", L"vfat",    6 },
     { L"/dev/unknown",  L"/mnt/unknown", L"ext4",    0 },
     { L"/dev/xvdb3",    L"/mnt/host",    L"ext4",    3 },
     { L"/dev/cdrom",    L"/mnt/cdrom",   L"iso9660", 5 },
     { L"/dev/dvdrom",   L"/mnt/dvdrom",  L"ufs",     5 }
};

#endif

class SCXStaticLogicalDiskPalTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStaticLogicalDiskPalTest );
    CPPUNIT_TEST( TestCreation );
    CPPUNIT_TEST( TestDumpString );
    CPPUNIT_TEST( TestGetMethods );
    CPPUNIT_TEST( SanityTestSameLogicalDisksAsStatisticalDisks );
    SCXUNIT_TEST_ATTRIBUTE(TestSameLogicalDisksAsStatisticalDisks, SLOW);
#if defined (linux)
    CPPUNIT_TEST( bug2942598_SanityTestSameLogicalDisksAsStatisticalDisks );
    CPPUNIT_TEST( TestGetHealthStateChanges );
    CPPUNIT_TEST( TestDeviceTypesForLinux );
#endif
#if defined (sun)
    CPPUNIT_TEST( TestDeviceIgnoredForSolaris );
    CPPUNIT_TEST( TestRemovabilityForSolaris );
#endif
     CPPUNIT_TEST( TestSimulatedLogicalDisks );
     CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskEnumeration> m_diskEnum;
    std::wstring fauxMntTab;
    std::wstring fauxDevTab;
public:
    SCXStaticLogicalDiskPalTest()
    {
        fauxMntTab = L"test_mnttab";
        fauxDevTab = L"test_devicetab";
    }

    void setUp(void)
    {
        m_diskEnum = 0;
    }

    void tearDown(void)
    {
        //unlink(SCXCoreLib::StrToUTF8(fauxMntTab).c_str());

        if (m_diskEnum != 0)
        {
            m_diskEnum->CleanUp();
            m_diskEnum = 0;
        }
    }

    void TestCreation(void)
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
    }

    /* This serves to test straight iteration and dump string */
    void TestDumpString(void)
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticLogicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> di = *iter;

            CPPUNIT_ASSERT(0 != di);
            std::wcout << std::endl << di->DumpString();
        }
    }

    /* Tests each of the Get() methods and verifies that the results are reasonable */

    void TestGetMethods()
    {
        std::wstring strKnownFS = L"|btrfs|ext2|ext3|ext4|hfs|jfs|jfs2|reiserfs|ufs|vfat|vxfs|xfs|zfs|";

        // Solaris 11 has a new file system type for the /dev file system
#if defined(sun) && ((PF_MAJOR == 5 && PF_MINOR >= 11) || (PF_MAJOR > 5))
        strKnownFS += L"dev|";
#endif

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticLogicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(0 != di);

            bool res, boolVal;
            int intVal;
            std::wstring strVal, strFSType;
            scxulong ulongVal, ulongSize;

            res = di->GetHealthState(boolVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetHealthState() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetHealthState() returned offline!", boolVal);

            res = di->GetDeviceName(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetDeviceName() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetDeviceName() returned empty", strVal.size());

            res = di->GetDeviceID(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetDeviceID() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetDeviceID() returned empty", strVal.size());

            res = di->GetMountpoint(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetMountpoint() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetMountpoint() returned empty", strVal.size());

            /* We know the file systems we know - and this one should be in our list */
            res = di->GetFileSystemType(strFSType);
            CPPUNIT_ASSERT_MESSAGE("Method GetFileSystemType() failed", res);
            CPPUNIT_ASSERT_MESSAGE(
                string("GetFileSystemType() value wrong: ").append(SCXCoreLib::StrToUTF8(strFSType)),
                strKnownFS.find(L"|" + strFSType + L"|") != std::wstring::npos );

            res = di->GetSizeInBytes(ulongSize);
            CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", res);
            // On Solaris-11, the dev file system has a zero size in bytes;
            CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() returned zero",
                                   0 != ulongSize || L"dev" == strFSType);

            res = di->GetCompressionMethod(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetCompressionMethod() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetCompressionMethod() value wrong",
                (strVal == L"Not Compressed" ||
                 (strVal == L"Unknown" && strFSType == L"zfs")) );

            /* Without Dependency Injection, the likeluhood of really having a R/O file system on a test system is very low */
            res = di->GetIsReadOnly(boolVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetIsReadOnly() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetIsReadOnly() value wrong", boolVal == false);

            res = di->GetEncryptionMethod(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetEncryptionMethod() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetEncryptionMethod() value wrong",
                                   (strVal == L"Not Encrypted" ||
                                    (strVal == L"Unknown" && strFSType == L"zfs" )) );

            /* The file systems that are thrown to us ("real" file systems) are all persistent */
            res = di->GetPersistenceType(intVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetPersistenceType() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetPersistenceType() value wrong", intVal == 2);

            res = di->GetAvailableSpaceInBytes(ulongVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailableSpaceInBytes() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetAvailableSpaceInBytes() returned zero",
                                   0 != ulongVal || strFSType == L"dev");
            CPPUNIT_ASSERT_MESSAGE("GetAvailableSpaceInBytes() inconsistent", ulongVal <= ulongSize);

            /* Some file systems don't actually support these (inode support) */
            res = di->GetTotalInodes(ulongVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalInodes() failed", (res ? ulongVal > 0 : ulongVal == 0));
            res = di->GetAvailableInodes(ulongVal);
            if (L"dev" != strFSType)
            {
                CPPUNIT_ASSERT_MESSAGE("Method GetAvailableInodes() failed",
                                       (res ? ulongVal > 0 : ulongVal == 0));
            }

            res = di->GetIsCaseSensitive(boolVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetIsCaseSensitive() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetIsCaseSensitive() value wrong", boolVal == true);

            res = di->GetIsCasePreserved(boolVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetIsCasePreserved() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetIsCasePreserved() value wrong", boolVal == true);

            /* Code set is zero for all of our known fileset types */
            res = di->GetCodeSet(intVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetCodeSet() failed", res);
            CPPUNIT_ASSERT_MESSAGE("Method GetCodeSet() value wrong", intVal == 0);

            /* Don't know of a file system with less than 255 bytes in a filename ... */
            res = di->GetMaxFilenameLen(ulongVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetMaxFilenameLen() failed", res);
            CPPUNIT_ASSERT_MESSAGE("GetMaxFilenameLen() value wrong", ulongVal >= 255);

            /* Block size is generally power of 2 >= 512 and <= 8192 */
            res = di->GetBlockSize(ulongVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() failed", res);
            CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() value wrong",
                                   ulongVal == 512 || ulongVal == 1024 || ulongVal == 2048 ||
                                   ulongVal == 4096 || ulongVal == 8192 ||
                                   ulongVal == 65536 /* Oddball case for HP-UX pa-risc /stand file system */||
                                   ulongVal == 131072 /* this one was found on SUN/zfs */ );
        }
    }

    void SanityTestSameLogicalDisksAsStatisticalDisks()
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if (0 != geteuid())
        {
            SCXUNIT_WARNING(L"Platform needs privileges to run SanityTestSameLogicalDisksAsStatisticalDisks test");
            return;
        }
#endif
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new SCXSystemLib::DiskDependDefault() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        SCXSystemLib::StatisticalLogicalDiskEnumeration statisticalDisks(deps);
        statisticalDisks.Init();
        statisticalDisks.Update(true);

        CPPUNIT_ASSERT_EQUAL(statisticalDisks.Size(), m_diskEnum->Size());

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalLogicalDiskInstance>::EntityIterator iter = statisticalDisks.Begin(); iter != statisticalDisks.End(); iter++)
        {
            std::wstring name;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(di->GetDiskName(name));
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> inst = m_diskEnum->GetInstance(name);
            CPPUNIT_ASSERT(0 != inst);
        }

        statisticalDisks.CleanUp();
    }

#if defined (linux)

    void bug2942598_SanityTestSameLogicalDisksAsStatisticalDisks()
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if (0 != geteuid())
        {
            SCXUNIT_WARNING(L"Platform needs privileges to run SanityTestSameLogicalDisksAsStatisticalDisks test");
               return;
        }
#endif
        std::wstring mtabFileName=L"./testfiles/bug2942598_mnttab";
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest());
        deps->SetMountTabPath(mtabFileName);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        SCXSystemLib::StatisticalLogicalDiskEnumeration statisticalDisks(deps);
        statisticalDisks.Init();
        statisticalDisks.Update(true);

        CPPUNIT_ASSERT_EQUAL(statisticalDisks.Size(), m_diskEnum->Size());

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalLogicalDiskInstance>::EntityIterator iter = statisticalDisks.Begin(); iter != statisticalDisks.End(); iter++)
        {
            std::wstring name;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(di->GetDiskName(name));
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> inst = m_diskEnum->GetInstance(name);
            CPPUNIT_ASSERT(0 != inst);
        }

        statisticalDisks.CleanUp();
     }


    void TestGetHealthStateChanges()
    {
        bool res, online;
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(fp != NULL);
        std::string mtabInit("/dev/mapper/VolGroup-lv_root / ext4 rw 0 0\n"
            "proc /proc proc rw 0 0\n"
            "sysfs /sys sysfs rw 0 0\n"
            "devpts /dev/pts devpts rw,gid=5,mode=620 0 0\n"
            "tmpfs /dev/shm tmpfs rw,rootcontext=\"system_u:object_r:tmpfs_t:s0\" 0 0\n"
            "/dev/sda1 /boot ext4 rw 0 0\n"
            "none /proc/sys/fs/binfmt_misc binfmt_misc rw 0 0\n");
        std::string graphite("/dev/sdb1 /opt/graphite ext4 rw 0 0\n");
        fputs(mtabInit.c_str(), fp);
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        // Initially the tested disk is not present
        SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> disk;
        disk = m_diskEnum->GetInstance(L"/opt/graphite");
        CPPUNIT_ASSERT_MESSAGE("Found unexpected disk instance", disk == NULL);

        // When we add the disk, it should be present and online
        fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(fp != NULL);
        fputs((mtabInit + graphite).c_str(), fp);
        fclose(fp);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        disk = m_diskEnum->GetInstance(L"/opt/graphite");
        CPPUNIT_ASSERT_MESSAGE("Did not find expected disk", disk != NULL);
        res = disk->GetHealthState(online);
        CPPUNIT_ASSERT_MESSAGE("Method GetHealthState() failed", res);
        CPPUNIT_ASSERT_MESSAGE("Disk should be online", online);

        // When the disk is removed, it should still be present but offline
        fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(fp != NULL);
        fputs(mtabInit.c_str(), fp);
        fclose(fp);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        disk = m_diskEnum->GetInstance(L"/opt/graphite");
        CPPUNIT_ASSERT_MESSAGE("Did not find expected disk", disk != NULL);
        res = disk->GetHealthState(online);
        CPPUNIT_ASSERT_MESSAGE("Method GetHealthState() failed", res);
        CPPUNIT_ASSERT_MESSAGE("Disk should be offline", !online);
    }

    void TestDeviceTypesForLinux()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(fp != NULL);
        fputs("proc /proc proc rw 0 0\n"
              "sysfs /sys sysfs rw 0 0\n"
              "tmpfs /dev/shm tmpfs rw,rootcontext=\"system_u:object_r:tmpfs_t:s0\" 0 0\n"
              "/dev/sda1 /boot ext4 rw 0 0\n"
              "/dev/hdc5 / ext4 rw 0 0\n"
              "/dev/ram /mnt/ramdisk vfat rw 0 0\n"
              "/dev/unknown /mnt/unknown ext4 rw 0 0\n"
              "/dev/xvdb3 /mnt/host ext4 rw 0 0\n"
              "/dev/cdrom /mnt/cdrom iso9660 rw 0 0\n"
              "/dev/dvdrom /mnt/dvdrom ufs rw 0 0\n"
              "none /proc/fs binfmt_misc rw 0 0\n",
              fp);
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

            // Insure number of devices is correct (less /dev/cdrom, which is ignored)
        CPPUNIT_ASSERT_MESSAGE("Wrong number of (fake) logical drives", m_diskEnum->Size() == (sizeof LogicalDeviceTable / sizeof (LogicalDeviceAttr)) - 1);

        size_t i = 0;
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticLogicalDiskInstance>::EntityIterator it = m_diskEnum->Begin(); it != m_diskEnum->End(); ++it)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> di(*it);
            CPPUNIT_ASSERT(di != NULL);

            bool res;
            int intVal;
            unsigned int uintVal;
            std::wstring strVal, strFSType;

                // If we're pointing to the CD-ROM, skip it (ignored by PAL tests)
                while (0 == SCXCoreLib::StrCompare(LogicalDeviceTable[i].FSType,
                                               L"iso9660", true))
                {
                    i++;
                }

            res = di->GetDeviceID(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetDeviceID() failed", res);
            CPPUNIT_ASSERT_MESSAGE("Device name mismatch", strVal.compare(LogicalDeviceTable[i].DeviceName) == 0);

            res = di->GetMountpoint(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetMountpoint() failed", res);
            CPPUNIT_ASSERT_MESSAGE("Mount point mismatch", strVal.compare(LogicalDeviceTable[i].MountPoint) == 0);

            res = di->GetFileSystemType(strFSType);
            CPPUNIT_ASSERT_MESSAGE("Method GetFileSystemType() failed", res);
            CPPUNIT_ASSERT_MESSAGE("File system type mismatch", strFSType.compare(LogicalDeviceTable[i].FSType) == 0);

            res = di->GetDriveType(uintVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetDriveType() failed", res);
            CPPUNIT_ASSERT_MESSAGE("Drive type mismatch", uintVal == LogicalDeviceTable[i].DriveType);

            /* Code set is zero for all of our known fileset types */
            res = di->GetCodeSet(intVal);
            CPPUNIT_ASSERT_MESSAGE("Method GetCodeSet() failed", res);

            i++;
        }

        return;
    }

#endif // defined (linux)

#if defined (sun)
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
    void TestDeviceIgnoredForSolaris()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        // Note: sample data comes from a Solaris 9 development box with a UFS CD
        // in the drive. The SPARC (not the x86) installation media is an example
        // of a CD-ROM with a UFS file system.
        fprintf(fp,
                "/dev/dsk/c1t0d0s0       /       ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800010  1258671407\n"
                "/proc   /proc   proc    dev=4600000     1258671406\n"
                "mnttab  /etc/mnttab     mntfs   dev=46c0000     1258671406\n"
                "fd      /dev/fd fd      rw,suid,dev=4700000     1258671407\n"
                "swap    /var/run        tmpfs   xattr,dev=1     1258671408\n"
                "swap    /tmp    tmpfs   xattr,dev=2     1258671409\n"
                "/dev/dsk/c1t0d0s7       /export/home    ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800017  1258671409\n"
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

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        // The only two entries that should appear are for root (i.e. /) and
        // /export/home.  The other entries are ignored.
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Found the wrong number of disks from (a fake) MNT TAB file",
                                     static_cast<size_t>(2), m_diskEnum->Size());

        SCXSystemLib::StatisticalLogicalDiskEnumeration statisticalDisks(deps);
        statisticalDisks.Init();
        statisticalDisks.Update(true);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("Found the wrong number of disks from (a fake) MNT TAB file",
                                     static_cast<size_t>(2), statisticalDisks.Size());

        // Verify that none of the returned entries start with the string
        // '/vol/dev/dsk/c0t0d0/sol_10_606_sparc' for this case
        std::wstring cdrom(L"/vol/dev/dsk/c0t0d0/sol_10_606_sparc");
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalLogicalDiskInstance>::EntityIterator iter = statisticalDisks.Begin(); iter != statisticalDisks.End(); ++iter)
        {
            std::wstring name, mountPoint, diskDevice;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(di->GetDiskName(name));
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> inst = m_diskEnum->GetInstance(name);
            CPPUNIT_ASSERT(inst->GetMountpoint(mountPoint));
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Found CD-ROM path in the DiskDevice when it should be absent",
                                         std::wstring::npos, mountPoint.find(cdrom));
        }

        statisticalDisks.CleanUp();
    }

    void TestRemovabilityForSolaris()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath devTab(fauxDevTab);
        deps->SetDevTabPath(fauxDevTab);

        // This file does not exist on sol 11 and later
#if (PF_MAJOR <= 5 && PF_MINOR < 11)
        FILE* fpDev = fopen(SCXCoreLib::StrToUTF8(fauxDevTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fpDev);

        //Taken from /etc/device.tab and touched up for testing
        fprintf(fpDev,
                "#ident  \"@(#)device.tab        1.4     01/03/23 SMI\"       /* SVr4.0 1.10.1.1 */\n"
                "#\n"
                "#       Device Table\n"
                "#\n"
                "#  Format:  Colon-list\n"
                "#  alias:cdevice:bdevice:pathname:attrs\n"
                "#\n"
                "#  Fields:\n"
                "#       alias           The device alias (primary key)\n"
                "#       cdevice         Pathname to the inode for the character device\n"
                "#       bdevice         Pathname to the inode for the block device\n"
                "#       pathname        Pathname to the inode for the device\n"
                "#       attrs           Expression-list: attributes of the device\n"
                "#                       An expression in this list is of the form attr=\"value\"\n"
                "#                       where attr is the attribute name and value is the\n"
                "#                       value of that attribute.\n"
                "#\n"
                "spool:::/var/spool/pkg:desc=\"Packaging Spool Directory\"\n"
                "disk1:/dev/rdsk/c0d0s2:/dev/dsk/c0d0s2::desc=\"Non-removable Disk Drive\" type=\"disk\" part=\"true\" removable=\"false\" capacity=\"73336725\" dpartlist=\"dpart101,dpart102\"\n"
                "disk2:/dev/rdsk/c0d0s3:/dev/dsk/c0d0s3::desc=\"Removable Disk Drive\" type=\"disk\" part=\"true\" removable=\"true\" capacity=\"73336725\" dpartlist=\"dpart101,dpart102\"\n"
                "disk3:/dev/rdsk/c0d0s4:/dev/dsk/c0d0s4::desc=\"Unknown Removability Disk Drive\" type=\"disk\" part=\"true\" capacity=\"73336725\" dpartlist=\"dpart101,dpart102\"\n"
                "dpart101:/dev/rdsk/c0d0s1:/dev/dsk/c0d0s1::desc=\"Disk Partition\" type=\"dpart\" removable=\"false\" capacity=\"69079500\" dparttype=\"fs\" fstype=\"ufs\" mountpt=\"/\"\n"
                "diskette1:/dev/rdiskette:/dev/diskette::desc=\"Floppy Drive\" mountpt=\"/mnt\" volume=\"diskette\" type=\"diskette\" removable=\"true\" capacity=\"2880\" fmtcmd=\"/usr/bin/fdformat -f -v /dev/rdiskette\" erasecmd=\"/usr/sbin/fdformat -f -v /dev/rdiskette\" removecmd=\"/usr/bin/eject\" copy=\"true\" mkfscmd=\"/usr/sbin/mkfs -F ufs /dev/rdiskette 2880 18 2 4096 512 80 2 5 3072 t\"\n"
                "diskette2:/dev/rdiskette0:/dev/diskette0::desc=\"Floppy Drive\" mountpt=\"/mnt\" volume=\"diskette\" type=\"diskette\" removable=\"true\" capacity=\"2880\" fmtcmd=\"/usr/bin/fdformat -f -v /dev/rdiskette0\" erasecmd=\"/usr/sbin/fdformat -f -v /dev/rdiskette0\" removecmd=\"/usr/bin/eject\" copy=\"true\" mkfscmd=\"/usr/sbin/mkfs -F ufs /dev/rdiskette0 2880 18 2 4096 512 80 2 5 3072 t\"\n" );

        fclose(fpDev);
#elif  (PF_MAJOR <= 5 && PF_MINOR >= 11) || (PF_MAJOR >= 6)
        // These direct that a device file of the right name is/is not found in various circumstances
        deps->SetOpenErrno("/dev/removable-media/dsk/c0d0s2", ENOENT);
        deps->SetOpenErrno("/dev/removable-media/dsk/c0d0s3", 0);
#endif

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        CPPUNIT_ASSERT_EQUAL(static_cast<int>(SCXSystemLib::eDiskCapOther), m_diskEnum->GetDiskRemovability(L"/dev/dsk/c0d0s2"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(SCXSystemLib::eDiskCapSupportsRemovableMedia), m_diskEnum->GetDiskRemovability(L"/dev/dsk/c0d0s3"));
        // Solaris 11 does not return Unknown for removability type
#if (PF_MAJOR >= 5 && PF_MINOR < 11)
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(SCXSystemLib::eDiskCapUnknown), m_diskEnum->GetDiskRemovability(L"/dev/dsk/c0d0s4"));
#endif
        m_diskEnum->CleanUp();
    }

#endif

#if defined(aix) || defined(hpux) || defined(sun)
    void OneLogicalDiskTest(int instanceIndex,
            scxulong blockSizeExpected, scxulong totalSizeExpected, scxulong freeSizeExpected,
            scxulong maxFilenameLenExpected, const char *deviceIdExpected, const char *mountPointExpected,
            const char *deviceNameExpected, const char *fileSystemTypeExpected)
    {
        SCXSystemLib::StaticLogicalDiskInstance* diskinst = (m_diskEnum->GetInstance(instanceIndex).GetData());
        CPPUNIT_ASSERT(0 != diskinst);

        scxulong     blockSize=0;
        scxulong     totalSize=0;
        scxulong     freeSize=0;
        scxulong     maxFilenameLen=0;
        std::wstring    deviceId;
        std::wstring    mountPoint;
        std::wstring    deviceName;
        std::wstring    fileSystemType;
        bool         quotasDisabled = false;
        bool         supportsDiskQuotas = false;
        unsigned int driveType = 0;

        bool resBool = false;

        resBool = diskinst->GetBlockSize(blockSize);
        CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() failed", resBool);
        CPPUNIT_ASSERT_EQUAL(blockSizeExpected, blockSize);

        resBool = diskinst->GetSizeInBytes(totalSize);
        CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", resBool);
        CPPUNIT_ASSERT_EQUAL(totalSizeExpected, totalSize);

        resBool = diskinst->GetAvailableSpaceInBytes(freeSize);
        CPPUNIT_ASSERT_MESSAGE("Method GetAvailableSpaceInBytes() failed", resBool);
        CPPUNIT_ASSERT_EQUAL(freeSizeExpected, freeSize);

        resBool = diskinst->GetMaxFilenameLen(maxFilenameLen);
        CPPUNIT_ASSERT_MESSAGE("Method GetMaxFilenameLen() failed", resBool);
        CPPUNIT_ASSERT_EQUAL(maxFilenameLenExpected, maxFilenameLen);

        resBool = diskinst->GetDeviceID(deviceId);
        CPPUNIT_ASSERT_MESSAGE("Method GetDeviceID() failed", resBool);
        CPPUNIT_ASSERT(SCXCoreLib::StrFromUTF8(string(deviceIdExpected)) == deviceId);

        resBool = diskinst->GetMountpoint(mountPoint);
        CPPUNIT_ASSERT_MESSAGE("Method GetMountpoint() failed", resBool);
        CPPUNIT_ASSERT(SCXCoreLib::StrFromUTF8(string(mountPointExpected)) == mountPoint);

        resBool = diskinst->GetDeviceName(deviceName);
        CPPUNIT_ASSERT_MESSAGE("Method GetDeviceName() failed", resBool);
        CPPUNIT_ASSERT(SCXCoreLib::StrFromUTF8(string(deviceNameExpected)) == deviceName);

        resBool = diskinst->GetFileSystemType(fileSystemType);
        CPPUNIT_ASSERT_MESSAGE("Method GetFileSystemType() failed", resBool);
        CPPUNIT_ASSERT(SCXCoreLib::StrFromUTF8(string(fileSystemTypeExpected)) == fileSystemType);
#if !defined(sun)
        resBool = diskinst->GetQuotasDisabled(quotasDisabled);
        CPPUNIT_ASSERT_MESSAGE("Method GetQuotasDisabled() implemented", !resBool);

        resBool = diskinst->GetSupportsDiskQuotas(supportsDiskQuotas);
        CPPUNIT_ASSERT_MESSAGE("Method GetSupportsDiskQuotas() implemented", !resBool);

        resBool = diskinst->GetDriveType(driveType);
        CPPUNIT_ASSERT_MESSAGE("Method GetDriveType() implemented", !resBool);
#endif

        diskinst->CleanUp();
    }
#endif// defined(aix) || defined(hpux)

    // Simulates mock hardware with several devices and several mount points and checks if, as a result of
    // a call to enumeration method, exactly same hardware is returned.
    void TestSimulatedLogicalDisks()
    {

#if defined(linux)
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);

        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(fp != NULL);
        fputs("proc /proc proc rw 0 0\n"
              "sysfs /sys sysfs rw 0 0\n"
              "tmpfs /dev/shm tmpfs rw,rootcontext=\"system_u:object_r:tmpfs_t:s0\" 0 0\n"
              "/dev/sda1 /boot ext4 rw 0 0\n"
              "/dev/hdc5 / ext4 rw 0 0\n"
              "/dev/ram /mnt/ramdisk vfat rw 0 0\n"
              "/dev/unknown /mnt/unknown ext4 rw 0 0\n"
              "/dev/xvdb3 /mnt/host ext4 rw 0 0\n"
              "/dev/cdrom /mnt/cdrom iso9660 rw 0 0\n"
              "/dev/dvdrom /mnt/dvdrom ufs rw 0 0\n"
              "none /proc/fs binfmt_misc rw 0 0\n"
              "overlay /overlay overlayFS rw 0 0\n",
              fp);
        fclose(fp);

        deps->SetMountTabPath(fauxMntTab);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        // Insure number of devices is correct (less /dev/cdrom, which is ignored)
        CPPUNIT_ASSERT_MESSAGE("Wrong number of (fake) logical drives", m_diskEnum->Size() == (sizeof LogicalDeviceTable / sizeof (LogicalDeviceAttr)) - 1);

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticLogicalDiskInstance>::EntityIterator it = m_diskEnum->Begin(); it != m_diskEnum->End(); ++it)
          {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> di(*it);
            CPPUNIT_ASSERT(di != NULL);

            //Enumeration should not have Pseudo devices
            std::wstring value;
            di->GetDeviceName(value);
            CPPUNIT_ASSERT(value.find(L"/") != std::wstring::npos);

            scxulong blockSizeExpected = 2048;
            scxulong blockSize = 0;
            bool resBool = di->GetBlockSize(blockSize);
            CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(blockSizeExpected, blockSize);
          }

#elif defined(aix) || defined(hpux) || defined(sun)
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskPartLogVolDiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskFullEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update());
        CPPUNIT_ASSERT_EQUAL(m_diskEnum->Size(), static_cast<size_t>(logicalDisk_cnt));

        // First logical disk, hd0.
        OneLogicalDiskTest(0, mountPoint0_frsize, mountPoint0_frsize*mountPoint0_blocks, mountPoint0_frsize*mountPoint0_bfree,
                           mountPoint0_namemax, mountPoint0_devName, mountPoint0_name, mountPoint0_name, mountPoint0_basetype);
        // Second logical disk, hd1.
        OneLogicalDiskTest(1, mountPoint1_frsize, mountPoint1_frsize*mountPoint1_blocks, mountPoint1_frsize*mountPoint1_bfree,
                           mountPoint1_namemax, mountPoint1_devName, mountPoint1_name, mountPoint1_name, mountPoint1_basetype);
#endif// defined(aix) || defined(hpux)
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStaticLogicalDiskPalTest );
