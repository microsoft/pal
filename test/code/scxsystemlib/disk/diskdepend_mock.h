/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Disk dependency mock object. 

    \date        2008-06-30 15:44:07

*/
/*----------------------------------------------------------------------------*/
#ifndef DISKDEPEND_MOCK_H
#define DISKDEPEND_MOCK_H

#include <scxsystemlib/scxlvmtab.h>
#include <scxsystemlib/staticphysicaldiskenumeration.h>
#include <scxsystemlib/statisticalphysicaldiskenumeration.h>

#include <errno.h>

#if defined(linux)
#include <sys/ioctl.h> // For ioctl() system call.
#include <scsi/sg.h> // For sg_io_hdr_t
#include <linux/fs.h>
#include <linux/hdreg.h>
#endif
#if defined(sun)
#include <sys/dkio.h>
#include <sys/vtoc.h>
#endif
#if defined(hpux)
#include <sys/diskio.h>
#endif
// Set to true to instrument tests for debugging purposes.
const static bool s_instrumentTests = false;

namespace
{
    class LvmTabTest : public SCXSystemLib::SCXLvmTab
    {
    public:
        LvmTabTest() { }

        void AddVG(const std::wstring& vg, const std::vector<std::wstring>& lvs)
        {
            SCXSystemLib::SCXLvmTab::SCXVG item;
            item.m_name = vg;
            item.m_part = lvs;
            m_vg.push_back(item);
        }

        void AddVG(const std::wstring& vg, const std::wstring& lv)
        {
            std::vector<std::wstring> lvs;
            lvs.push_back(lv);
            AddVG(vg, lvs);
        }

        void AddVG(const std::wstring& vg, const std::wstring& lv1, const std::wstring& lv2)
        {
            std::vector<std::wstring> lvs;
            lvs.push_back(lv1);
            lvs.push_back(lv2);
            AddVG(vg, lvs);
        }
    };

    class DiskDependTest : public SCXSystemLib::DiskDependDefault
    {
    public:
        DiskDependTest()
#if defined(linux)        
            : m_WI_479079_TestNumber(-1)
#elif defined(hpux)
            : m_pDiskInfo(NULL), m_pDiskInfoCount(0)
#endif
        {

        }

        void SetOpenErrno(const std::string path, int e)
        {
            m_openErrno[path] = e;
        }

        void SetMountTabPath(const SCXCoreLib::SCXFilePath& path)
        {
            m_MntTabPath = path;
        }

        //void SetProcDiskStatsPath(const SCXCoreLib::SCXFilePath& path)
        //{
        //    m_ProcDiskStatsPath = path;
        //}
        
        void SetLvmTab(SCXCoreLib::SCXHandle<SCXSystemLib::SCXLvmTab> lvmTab)
        {
            m_pLvmTab = lvmTab;
        }

        void SetStat(const std::string& path, const struct stat& data)
        {
            m_mapStat[path] = data;
        }
        
        virtual void GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files)
        {
            files.clear();
            for (std::map<std::string, struct stat>::const_iterator it = m_mapStat.begin();
                 it != m_mapStat.end(); ++it)
            {
                SCXCoreLib::SCXFilePath f(SCXCoreLib::StrFromUTF8(it->first));
                if (f.GetDirectory() == path)
                {
                    files.push_back(f);
                }
            }
        }

        //virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& device) = 0;

        //virtual std::vector<std::wstring> GetLogicalAlternativeDevices(const std::wstring& device) = 0;

        //virtual void AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID) = 0;

        //virtual DeviceInstance* FindDeviceInstance(const std::wstring& device) const = 0; 

        virtual bool open(const char* pathname, int flags)
        {
            std::map<std::string, int> ::const_iterator openErrnoIt = m_openErrno.find(pathname);
            if (openErrnoIt == m_openErrno.end())
            {
                return SCXSystemLib::DiskDependDefault::open(pathname, flags);
            }
            else if (0 != openErrnoIt->second)
            {
                errno = openErrnoIt->second;
                return false;
            }
            return true;
        }

        virtual int close(void)
        {
            return SCXSystemLib::DiskDependDefault::close();
        }

        // If a file has been mocked for open we use that info to return if  a file exists or not.
        // Otherwise we fall back to the default behavior.
        virtual bool FileExists(const std::wstring& path)
        {
            std::string pathname = SCXCoreLib::StrToUTF8(path);
            std::map<std::string, int> ::const_iterator openErrnoIt = m_openErrno.find(pathname);

            if (openErrnoIt == m_openErrno.end())
            {
                return SCXSystemLib::DiskDependDefault::FileExists(path);
            }
            else if (0 != openErrnoIt->second)
            {
                return false;
            }
            return true;
        }

#if !defined(linux)
        virtual int ioctl(unsigned long int request, void* data)
        {
            return SCXSystemLib::DiskDependDefault::ioctl(request, data);
        }
#else
    // Determines what test is being run for WI_479079. If WI_479079 tests are not being run then m_WI_479079_TestNumber
    // is set to -1 and disables ioctl logic.
    int m_WI_479079_TestNumber;
    /* This function overwrites the SCXSystemLib::DiskDependDefault::ioctl().
    * As long as other tests which are including this file don't care about the 
    * coverage test of DiskDependDefault::ioctl(), this would work for them i.e.
    * it would always zero, otherwise this function needs to be overwritten.
    * At the moment, it doesn't seem they do care!
    */
    virtual int ioctl(unsigned long int command, void *vptr)
    {
        if (m_WI_479079_TestNumber == -1)
        {
            return 0; // Let the rest of unit-test remain intact.
        }

        switch (command) 
        {
            case SG_IO:
            {
                CPPUNIT_ASSERT_MESSAGE("ioctl(SG_IO) called with vptr == NULL", vptr != NULL);
                sg_io_hdr_t &ioh = *static_cast<sg_io_hdr_t *>(vptr);
                CPPUNIT_ASSERT_EQUAL(SG_DXFER_FROM_DEV, ioh.dxfer_direction);
                CPPUNIT_ASSERT_EQUAL(0, ioh.iovec_count);
                CPPUNIT_ASSERT_MESSAGE("ioctl(SG_IO) called with vptr == NULL", ioh.dxfer_len > 13);
                if(m_WI_479079_TestNumber == 0)
                {
                    // Should generate an Unknown availability.
                    return -1;
                }
                ioh.status = 0;  
                ioh.host_status = 0;
                ioh.driver_status = 0;
                ioh.masked_status = 0;
                memset(ioh.dxferp, 't', ioh.dxfer_len);

                switch(m_WI_479079_TestNumber)
                {
                    case 1:
                    { // Should generate an Unknown availability.
                        ioh.host_status = 1;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        return 0;
                    }
                    case 2:
                    { // Should generate an Unknown availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 1;
                        ioh.masked_status = 0;
                        return 0;
                    }
                    case 3:
                    { // Should generate an Unknown availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 1;
                        return 0;
                    }
                    case 4:
                    { // Should generate an  Running or Full Power availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x0; // Sense key
                        return 0;
                    }
                    case 5:
                    { // Should generate an  SELF-TEST availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x04; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x09; // ASCQ 
                        return 0;
                    }
                    case 6:
                    { // Should generate an  OFFLINE availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x04; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x12; // ASCQ 
                        return 0;
                    }
                    case 7:
                    { // Should generate an WARNING availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x0B; // ASC 
                        return 0;
                    }
                    case 8:
                    { // Should generate an LOW POWER availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x5E; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x00; // ASCQ
                        return 0;
                    }
                    case 9:
                    { // Should generate an ACTIVE/IDEL availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x5E; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x41; // ASCQ
                        return 0;
                    }
                    case 10:
                    { // Should generate an ACTIVE/IDEL availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x5E; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x42; // ASCQ
                        return 0;
                    }
                    case 11:
                    { // Should generate an STANDBY availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x5E; // ASC 
                        *((char *)(ioh.dxferp)+13) = 0x43; // ASCQ
                        return 0;
                    }
                    case 12:
                    { // Should generate an Unknown availability.
                        ioh.host_status = 0;
                        ioh.driver_status = 0;
                        ioh.masked_status = 0;
                        *((char *)(ioh.dxferp)+2) = 0x1; // a non-zero Sense key
                        *((char *)(ioh.dxferp)+12) = 0x0E; // ASC 
                        return 0;
                    }
                    default:
                    {
                        return -1;
                    }
                }
            }
            case SG_GET_VERSION_NUM:
            {
                CPPUNIT_ASSERT_MESSAGE("ioctl(SG_GET_VERSION_NUM) called with vptr == NULL", vptr != NULL);
                *(int *)vptr = 20000;
                return 0;
            }
            case HDIO_DRIVE_CMD:
            {
                errno = ENOMEM;
                return -1;
            }
            default:
                return 0;
        }
    }
#endif 

        virtual int statvfs(const char* path, SCXSystemLib::SCXStatVfs* buf)
        {
            memset(buf, 0, sizeof (struct statvfs));    // clear OS-specific fields
            if (strncmp(path, "/dev/cd", 7) == 0 || strncmp(path, "/dev/dvd", 8) == 0)
            {                           // values for CD-ROM and DVD-ROMs
                buf->f_bsize = 2048;        /* file system block size, 2048 bytes */
                buf->f_frsize = 2048;       /* fragment size, 2048 bytes */
                buf->f_blocks = 382000;     /* size of fs in f_frsize units*/
                buf->f_bfree = 0;           /* # free blocks */
                buf->f_bavail = 0;          /* # free blocks for non-root */
                buf->f_files = 0;           /* # inodes */
                buf->f_ffree = 0;           /* # free inodes */
                buf->f_favail = 0;          /* # free inodes for non-root */
#ifdef linux    // fsid_t is an array on HP-UX and a struct on Solaris and AIX
                buf->f_fsid = 5;            /* file system ID */
#endif
                buf->f_flag = 1;            /* mount flags, 1 = read only */
                buf->f_namemax = 176;       /* maximum filename length */
                      }
            else
            {                          // values for logical disks
                buf->f_bsize = 4096;        /* file system block size, 4096 bytes */
                buf->f_frsize = 2048;       /* fragment size, 2048 bytes */
                buf->f_blocks = 2048;       /* size of fs in f_frsize units, 8 Gb */
                buf->f_bfree = 1024;        /* # free blocks, 4 Gb */
                buf->f_bavail = 1024;       /* # free blocks for non-root, 4 Gb */
                buf->f_files = 10240;       /* # inodes, 10 k */
                buf->f_ffree = 5120;        /* # free inodes, 5 k */
                buf->f_favail = 5120;       /* # free inodes for non-root, 5k */
#ifdef linux    // fsid_t is an array on HP-UX and a struct on Solaris and AIX
                buf->f_fsid = 4;            /* file system ID */
#endif
                buf->f_flag = 0;            /* mount flags, 0 = read/write */
                buf->f_namemax = 255;       /* maximum filename length */
            }
            return 0;
                }

        virtual int lstat(const char *path, struct stat *buf)
        {
            if (m_mapStat.end() != m_mapStat.find(path))
            {
                memcpy(buf, &(m_mapStat.find(path)->second), sizeof(struct stat));
                return 0;
            }
            return -1;
        }
#if defined(hpux)
        void SetPstDiskInfo(struct pst_diskinfo* buf, size_t count)
        {
            m_pDiskInfo = buf;
            m_pDiskInfoCount = count;
        }
        
        virtual int pstat_getdisk(struct pst_diskinfo* buf, size_t elemsize, size_t elemcount, int index)
        {
            if (index >= m_pDiskInfoCount)
                return -1;
            memcpy(buf, &(m_pDiskInfo[index]), elemsize * MIN(elemcount, m_pDiskInfoCount-index));
            return MIN(elemcount, m_pDiskInfoCount-index);
        }

        //virtual int pstat_getlv(struct pst_lvinfo* buf, size_t elemsize, size_t elemcount, int index) = 0;

    private:
        struct pst_diskinfo* m_pDiskInfo;
        size_t m_pDiskInfoCount;
#endif /* hpux */
#if defined(sun)
    protected:
        virtual bool IsDiskInKstat(const std::wstring& /*dev_path*/)
        {
            return true;
        }
#endif /* sun */

    private:
        std::map<std::string, struct stat> m_mapStat;
        std::map<std::string, int> m_openErrno;
    };
}

namespace SCXSystemLib
{
    class MockSolarisStaticPhysicalDiskEnumeration : public SCXSystemLib::StaticPhysicalDiskEnumeration
    {
    public:
        MockSolarisStaticPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : SCXSystemLib::StaticPhysicalDiskEnumeration(deps)
        {}

    protected:
        virtual void UpdateSolarisHelper()
        {
            // Purposely does nothing. The parent function reads the local file system
            // to determine possible devices. In a unit-test scenario, it is not
            // desired to touch the local file system.
            // Added for testing Bug 15583.
        }
    };

    class MockSolarisStatisticalPhysicalDiskEnumeration : public SCXSystemLib::StatisticalPhysicalDiskEnumeration
    {
    public:
        MockSolarisStatisticalPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps)
        {}

    protected:
        virtual void UpdateSolarisHelper()
        {
            // Purposely does nothing. The parent function reads the local file system
            // to determine possible devices. In a unit-test scenario, it is not
            // desired to touch the local file system.
            // Added for testing Bug 15583.
        }
    }; 
}
#if defined(aix) || defined(hpux) || defined(sun)
namespace
{
// Classes used for exact disk partition/logical volume tests. With exact tests we simulate particular
// hardware and expect to receive exactly same hardware data from hardvare enumeration calls.

// System with three disk partitions and three mount points is created. One partition is not mounted
// and one mount point (proc) is a disk mount point.

// Use instrumentTest to debug test code.
    static bool instrumentTest = false;

    // Calculated values. If code that simulates hardware changes then update this values.
    const int logicalDisk_cnt = 2;
#if defined(aix)
    const int partition_cnt = 3;
#elif defined(hpux)
    const int partition_cnt = 4;
#elif defined(sun)
    const int partition_cnt = 2;
#endif

    // For hpux we have 2 separate cases as far as boot volume is concerned. In one case boot and root are
    // separate logical volumes and in the other case they are the same logical volume. bootRootShareLV flag
    // determines what case will be tested.
#if defined(hpux)
    bool bootRootShareLV = false;
#endif

#if defined(aix)
    typedef blksize64_t testblksize;
    const size_t testFSTypSz = _FSTYPSIZ;
#elif defined(hpux)
    typedef unsigned long testblksize;
    const size_t testFSTypSz = _FSTYPSZ;
#elif defined(sun)
    typedef unsigned long testblksize;
    const size_t testFSTypSz = FSTYPSZ;
#endif
// Mount point 0
#if defined(aix)
    const char* mountPoint0_devName = "/dev/hd0";
#elif defined(hpux)
    const char* mountPoint0_devName = "/dev/vg00/lvol1";
    const char* mountPoint0_devNameShort = "lvol1";
#elif defined(sun)
const char* mountPoint0_devName = "rpool/ROOT/solaris";
#endif
#if defined(aix) || defined(hpux)
    const char* mountPoint0_name = "/abc";
#elif defined(sun)
    const char* mountPoint0_name = "/";
#endif
    const testblksize mountPoint0_bsize = 2048;
    const testblksize mountPoint0_frsize = 1024;
    const blkcnt64_t mountPoint0_blocks = 2000000;
    const blkcnt64_t mountPoint0_bfree = 1000000;
    const blkcnt64_t mountPoint0_bavail = 999998;
    const blkcnt64_t mountPoint0_files = 800000;
    const blkcnt64_t mountPoint0_ffree = 700000;
    const blkcnt64_t mountPoint0_favail = 650000;
    const ulong_t mountPoint0_namemax = 64;
#if defined(aix) || defined(hpux)
    const char* mountPoint0_basetype = "jfs";
#elif defined(sun)
    const char* mountPoint0_basetype = "zfs";
#endif
    const char* mountPoint0_opts = "-a -b -c";

// Mount point 1
#if defined(aix)
    const char* mountPoint1_devName = "/dev/hd1";
#elif defined(hpux)
    const char* mountPoint1_devName = "/dev/vg00/ lvol 2 ";
    const char* mountPoint1_devNameShort = " lvol 2 ";
#elif defined(sun) 
const char* mountPoint1_devName = "rpool/export";
#endif
    const char* mountPoint1_name = "/xyz/def";
    const testblksize mountPoint1_bsize = 8192;
    const testblksize mountPoint1_frsize = 4096;
    const blkcnt64_t mountPoint1_blocks = 4000000;
    const blkcnt64_t mountPoint1_bfree = 1500000;
    const blkcnt64_t mountPoint1_bavail = 1399998;
    const blkcnt64_t mountPoint1_files = 1000000;
    const blkcnt64_t mountPoint1_ffree = 900000;
    const blkcnt64_t mountPoint1_favail = 850000;
    const ulong_t mountPoint1_namemax = 1024;
#if defined(aix)
    const char* mountPoint1_basetype = "jfs2";
#elif defined(hpux)
    const char* mountPoint1_basetype = "vxfs";
#elif defined(sun)
    const char* mountPoint1_basetype = "zfs";
#endif
    const char* mountPoint1_opts = "-d -e -f";

// Mount point 2
    const char* mountPoint2_devName = "/proc";
    const char* mountPoint2_name = "/proc";
    const testblksize mountPoint2_bsize = 2048;
    const testblksize mountPoint2_frsize = 2048;
    const blkcnt64_t mountPoint2_blocks = 45000;
    const blkcnt64_t mountPoint2_bfree = 20000;
    const blkcnt64_t mountPoint2_bavail = 15997;
    const blkcnt64_t mountPoint2_files = 15000;
    const blkcnt64_t mountPoint2_ffree = 13000;
    const blkcnt64_t mountPoint2_favail = 12000;
    const ulong_t mountPoint2_namemax = 256;
    const char* mountPoint2_basetype = "proc";
    const char* mountPoint2_opts = "-g -h -i";


// Partition 0
#if defined(aix)
    const char* partition0_name = "hd0";
#elif defined(hpux)
    const char* partition0_name = mountPoint0_devName;
    const char* partition0_rname = "/dev/vg00/rlvol1";
    const char* partition0_nameShort = mountPoint0_devNameShort;
#elif defined(sun)
    const char* partition0_name = "rpool/ROOT/solaris";
#endif

#if defined(aix) || defined(hpux)
    // AIX: partition0_boot is calculated value. Particular partition must have attribute named "type" with value "boot"
    // for partition0_boot value to be true. If code that simulates hardware changes update partitionX_boot values.
    const bool partition0_boot = false;
#elif defined(sun)
    // Partition with the mount point '/' is a boot partition.
    const bool partition0_boot = true;
#endif

// Partition 1
#if defined(aix)
    const char* partition1_name = "hd1";
#elif defined(hpux)
    const char* partition1_name = mountPoint1_devName;
    const char* partition1_rname = "/dev/vg00/r lvol 2 ";
    const char* partition1_nameShort = mountPoint1_devNameShort;
#elif defined(sun)
    const char* partition1_name = "rpool/export";
#endif
    const bool partition1_boot = false;

// Partition 2
#if defined(aix)
    const char* partition2_name = "hd2";
    long partition2_blks = 5;
    long partition2_blk_size = 25;
    long long partition2_size = static_cast<long long>(partition2_blks) << partition2_blk_size;
#elif defined(hpux)
    const char* partition2_name = "/dev/vg00/ lvol 3 ";
    const char* partition2_rname = "/dev/vg00/r lvol 3 ";
    const char* partition2_nameShort = " lvol 3 ";
    long long partition2_size_blks = 22041232;
    long long partition2_size = partition2_size_blks * DEV_BSIZE;
#endif
    const bool partition2_boot = true;

// Partition 3
// For hpux we add partition with same Logical Volume name as the boot partition but in a different Volume Group so we
// can try to confuse disk partition provider's boot logic.
#if defined(hpux)
    const char* partition3_name = "/dev/vg01/ lvol 3 ";
    const char* partition3_rname = "/dev/vg01/r lvol 3 ";
    const bool partition3_boot = false;
    long long partition3_size_blks = 45023484;
    long long partition3_size = partition3_size_blks * DEV_BSIZE;
#endif

    int statvfs64test(const char* path, struct statvfs64* buf)
    {
        CPPUNIT_ASSERT(path != NULL);
        CPPUNIT_ASSERT(buf != NULL);
        memset(buf, 0, sizeof(struct statvfs64));
        std::string pn=path;
        if (pn == mountPoint0_name)
        {
            buf->f_bsize = mountPoint0_bsize;
            buf->f_frsize = mountPoint0_frsize;
            buf->f_blocks = mountPoint0_blocks;
            buf->f_bfree = mountPoint0_bfree;
            buf->f_bavail = mountPoint0_bavail;
            buf->f_files = mountPoint0_files;
            buf->f_ffree = mountPoint0_ffree;
            buf->f_favail = mountPoint0_favail;
            buf->f_namemax = mountPoint0_namemax;
            strncpy(buf->f_basetype, mountPoint0_basetype, testFSTypSz - 1);
        }
        else if (pn == mountPoint1_name)
        {
            buf->f_bsize = mountPoint1_bsize;
            buf->f_frsize = mountPoint1_frsize;
            buf->f_blocks = mountPoint1_blocks;
            buf->f_bfree = mountPoint1_bfree;
            buf->f_bavail = mountPoint1_bavail;
            buf->f_files = mountPoint1_files;
            buf->f_ffree = mountPoint1_ffree;
            buf->f_favail = mountPoint1_favail;
            buf->f_namemax = mountPoint1_namemax;
            strncpy(buf->f_basetype, mountPoint1_basetype, testFSTypSz - 1);
        }
        else if (pn == mountPoint2_name)
        {
            buf->f_bsize = mountPoint2_bsize;
            buf->f_frsize = mountPoint2_frsize;
            buf->f_blocks = mountPoint2_blocks;
            buf->f_bfree = mountPoint2_bfree;
            buf->f_bavail = mountPoint2_bavail;
            buf->f_files = mountPoint2_files;
            buf->f_ffree = mountPoint2_ffree;
            buf->f_favail = mountPoint2_favail;
            buf->f_namemax = mountPoint2_namemax;
            strncpy(buf->f_basetype, mountPoint2_basetype, testFSTypSz - 1);
        }
        else
        {
            std::wstring failstr = L"Invalid mount point \"" + SCXCoreLib::StrFromUTF8(pn) + L"\".";
            CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(failstr));
        }
        return 0;
    }

#if defined(aix)
    // Overrides odm_initialize, odm_terminate, odm_get_first and odm_get_next calls to AIX ODM database.
    class DiskPartLogVolDiskDependTestSCXodmDependencies : public SCXSystemLib::SCXodmDependencies
    {
        int CuDv_pos;// CuDv class enumeration position.
        std::string CuDv_criteria;// Criteria for the current CuAt query.
        int CuAt_pos;// CuAt class enumeration position.
        std::string CuAt_criteria;// Criteria for the current CuAt query.
        const char*get_CuDv_criteria(){return "PdDvLn=logical_volume/lvsubclass/lvtype";}
    public:
        DiskPartLogVolDiskDependTestSCXodmDependencies():CuDv_pos(-1), CuAt_pos(-1){}
        virtual int Initialize(){return 0;}
        virtual int Terminate(){return 0;}
        void verify_input_parameters(CLASS_SYMBOL cs, char *criteria, void *returnData, bool verify_criteria)
        {
            CPPUNIT_ASSERT_MESSAGE("returnData is NULL, Not an error but our mock dependencies support only"
                " case where result structure is already allocated.", returnData != NULL);
            CPPUNIT_ASSERT_MESSAGE("Invalid ODM class", cs == CuDv_CLASS || cs == CuAt_CLASS);
            CPPUNIT_ASSERT_MESSAGE("Invalid criteria for CuDv_CLASS.",
                        cs != CuDv_CLASS || !verify_criteria || strcmp(criteria , get_CuDv_criteria()) == 0);
            CPPUNIT_ASSERT_MESSAGE("Invalid criteria for CuAt_CLASS.",
                        cs != CuAt_CLASS || !verify_criteria ||
                        strcmp(criteria , "name=hd0") == 0 ||
                        strcmp(criteria , "name=hd1") == 0 ||
                        strcmp(criteria , "name=hd2") == 0);
        }
        void *get_CuDv(void *returnData)
        {
            struct CuDv *dv = static_cast<struct CuDv*>(returnData);
            memset(dv, 0, sizeof(struct CuDv));
            // Return all logical volumes.
            if(CuDv_criteria == get_CuDv_criteria())
            {
                switch(CuDv_pos)
                {
                case 0:
                    strncpy(dv->name, partition0_name, sizeof(dv->name) - 1);
                    strncpy(dv->location, "rootvg", sizeof(dv->location) - 1);
                    CuDv_pos++;
                    return returnData;
                case 1:
                    strncpy(dv->name, partition1_name, sizeof(dv->name) - 1);
                    strncpy(dv->location, "rootvg", sizeof(dv->location) - 1);
                    CuDv_pos++;
                    return returnData;
                case 2:
                    strncpy(dv->name, partition2_name, sizeof(dv->name) - 1);
                    strncpy(dv->location, "rootvg", sizeof(dv->location) - 1);
                    CuDv_pos++;
                    return returnData;
                default:
                    CuDv_pos = 0;
                    return NULL;
                }
            }
            else
            {
                // omd_get_first was not called, return random data.
                strncpy(dv->name, "some name", sizeof(dv->name) - 1);
                strncpy(dv->location, "some location", sizeof(dv->location) - 1);
                strncpy(dv->parent, "some parent", sizeof(dv->parent) - 1);
                strncpy(dv->connwhere, "some connection", sizeof(dv->connwhere) - 1);
                return returnData;
            }
        }
        void *get_CuAt(void *returnData)
        {
            struct CuAt *at = static_cast<struct CuAt*>(returnData);
            memset(at, 0, sizeof(struct CuAt));
            // Return various attributes for various devices.
            if(CuAt_criteria == "name=hd0")
            {
                switch(CuAt_pos)
                {
                case 0:
                    strncpy(at->name, partition0_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "lvserial_id", sizeof(at->attribute) - 1);
                    strncpy(at->value, "0101010101010101", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                case 1:
                    strncpy(at->name, partition0_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "size", sizeof(at->attribute) - 1);
                    strncpy(at->value, "2", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                default:
                    CuAt_pos = 0;
                    return NULL;
                }
            }
            else if(CuAt_criteria == "name=hd1")
            {
                switch(CuAt_pos)
                {
                case 0:
                    strncpy(at->name, partition1_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "lvserial_id", sizeof(at->attribute) - 1);
                    strncpy(at->value, "2121212121212121", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                case 1:
                    strncpy(at->name, partition1_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "size", sizeof(at->attribute) - 1);
                    strncpy(at->value, "4", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                default:
                    CuAt_pos = 0;
                    return NULL;
                }
            }
            else if(CuAt_criteria == "name=hd2")
            {
                switch(CuAt_pos)
                {
                case 0:
                    strncpy(at->name, partition2_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "lvserial_id", sizeof(at->attribute) - 1);
                    strncpy(at->value, "5454545454545454", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                case 1:
                    strncpy(at->name, partition2_name, sizeof(at->name) - 1);
                    strncpy(at->attribute, "type", sizeof(at->attribute) - 1);
                    strncpy(at->value, "boot", sizeof(at->value) - 1);
                    CuAt_pos++;
                    return returnData;
                default:
                    CuAt_pos = 0;
                    return NULL;
                }
            }
            else
            {
                // omd_get_first was not called, return random data.
                strncpy(at->name, "some name", sizeof(at->name) - 1);
                strncpy(at->attribute, "some attribute", sizeof(at->attribute) - 1);
                strncpy(at->value, "some value", sizeof(at->value) - 1);
                return returnData;
            }
        }
        virtual void *GetFirst(CLASS_SYMBOL cs, char *criteria, void *returnData)
        {
            void *ret = NULL;
            verify_input_parameters(cs, criteria, returnData, true);
            if(cs == CuDv_CLASS)
            {
                // List of CuDv objects is requested. Return first in the list.
                CuDv_pos = 0;
                CuDv_criteria = criteria;
                ret = get_CuDv(returnData);
            }
            else if(cs == CuAt_CLASS)
            {
                // List of CuAt objects is requested. Return first in the list.
                CuAt_pos = 0;
                CuAt_criteria = criteria;
                ret = get_CuAt(returnData);
            }
            return ret;
        }
        virtual void *GetNext(CLASS_SYMBOL cs, void *returnData)
        {
            void *ret = NULL;
            verify_input_parameters(cs, NULL, returnData, false);
            if(cs == CuDv_CLASS)
            {
                // List of CuDv objects is requested. Return next in the list.
                ret = get_CuDv(returnData);
            }
            else if(cs == CuAt_CLASS)
            {
                // List of CuAt objects is requested. Return next in the list.
                ret = get_CuAt(returnData);
            }
            return ret;
        }
    };

    // Overrides SCXodm object so we can insert overriden SCXodmDependencies and intercept calls to AIX ODM database.
    class DiskPartLogVolDiskDependTestSCXodm: public SCXSystemLib::SCXodm
    {
    public:
        DiskPartLogVolDiskDependTestSCXodm()
        {
            // m_deps operator= cleans up SCXodmDependencies created by base class and sets up mock one.
            // No need to call m_deps->Initialize() since mock Initialize does nothing.
            m_deps = new DiskPartLogVolDiskDependTestSCXodmDependencies();
        }
    };

    class DiskPartLogVolDiskDependTest : public SCXSystemLib::DiskDependDefault
    {
        // AIX documentation is extremely ambiguous.
        // Data items do not have to be aligned on an absolute word (32 bit) boundary, but they do have
        // to be aligned relative to the starting buffer address passed to the mntctl call.
        void align32(int &offset)
        {
            if(offset & 0x0003)
            {
                // Need to align offset on 4 byte boundary.
                offset += 4;
                offset &= ~3;
            }
        }
        void one_vmount_data(int &offset, int vmount_offset, char* buf, const char* str, struct vmount::vmt_data &vmtd, bool write)
        {
            size_t start_offset = offset;
            offset += strlen(str) + 1;
            align32(offset);
            if(write)
            {
                strcpy(buf + start_offset, str);
                vmtd.vmt_off = start_offset - vmount_offset;
                vmtd.vmt_size = offset - start_offset;
            }
        }
        // Process single mount point.
        void one_vmount_data_array(int &offset, char* buf, int vmt_flags, int vmt_gfstype,
                        const char* vmt_object, const char* vmt_stub, const char* vmt_host,
                        const char* vmt_host_name, const char* vmt_info, const char* vmt_args, bool write)
        {
            int start_offset = offset;
            struct vmount* vm = reinterpret_cast<struct vmount*>(buf + offset);
            offset += sizeof(struct vmount);
            align32(offset);
            one_vmount_data(offset, start_offset, buf, vmt_object, vm->vmt_data[VMT_OBJECT], write);
            one_vmount_data(offset, start_offset, buf, vmt_stub, vm->vmt_data[VMT_STUB], write);
            one_vmount_data(offset, start_offset, buf, vmt_host, vm->vmt_data[VMT_HOST], write);
            one_vmount_data(offset, start_offset, buf, vmt_host_name, vm->vmt_data[VMT_HOSTNAME], write);
            one_vmount_data(offset, start_offset, buf, vmt_info, vm->vmt_data[VMT_INFO], write);
            one_vmount_data(offset, start_offset, buf, vmt_args, vm->vmt_data[VMT_ARGS], write);
            if(write)
            {
                vm->vmt_length = offset - start_offset;
                vm->vmt_flags = vmt_flags;
                vm->vmt_gfstype = vmt_gfstype;
            }
        }
        // Process all mount points. Or return required size or fill the buffer.
        int mntctl_main(char* buf, bool write)
        {
            int offset = 0;
            // First mount point.
            one_vmount_data_array(offset, buf, MNT_DEVICE, MNT_JFS,
                        mountPoint0_devName, mountPoint0_name, "HOST",
                        "full host name", "101010101010", mountPoint0_opts, write);
            // Second mount point.
            one_vmount_data_array(offset, buf, MNT_DEVICE, MNT_J2,
                        mountPoint1_devName, mountPoint1_name, "HOST",
                        "full host name", "20202020", mountPoint1_opts, write);
            // Third mount point.
            one_vmount_data_array(offset, buf, 0, MNT_PROCFS,
                        mountPoint2_devName, mountPoint2_name, "HOST",
                        "full host name", "30303030", mountPoint2_opts, write);
            return offset;
        }
        virtual int mntctl(int command, int size, char* buf)
        {
            CPPUNIT_ASSERT_EQUAL(MCTL_QUERY, command);
            // Undocumented feature, doc says error if size not positive but error is also returned if less than int.
            CPPUNIT_ASSERT(size >= sizeof(int));
            CPPUNIT_ASSERT(buf != NULL);
            // Get required buffer size.
            int required_size = mntctl_main(buf, false);
            if(required_size > size)
            {
                *reinterpret_cast<int*>(buf) = required_size;
                return 0;
            }
            // Write data into buffer.
            memset(buf, 0, size);
            mntctl_main(buf, true);
            // Return number of mount points.
            return 3;
        }
        virtual int statvfs64(const char* path, struct statvfs64* buf)
        {
            return statvfs64test(path, buf);
        }
        virtual int statvfs(const char* path, SCXSystemLib::SCXStatVfs* buf)
        {
            return statvfs64(path, buf);
        }
        virtual const SCXCoreLib::SCXHandle<SCXSystemLib::SCXodm> CreateOdm(void) const
        {
            return SCXCoreLib::SCXHandle<DiskPartLogVolDiskDependTestSCXodm>(new DiskPartLogVolDiskDependTestSCXodm());
        }
        virtual int lvm_queryvgs(struct queryvgs **queryVGS, mid_t kmid)
        {
            CPPUNIT_ASSERT(queryVGS != NULL);
            CPPUNIT_ASSERT_EQUAL(static_cast<intptr_t>(NULL), (unsigned int)kmid);
            *queryVGS = &volumeGroups;
            return 0;
        }
        virtual int lvm_queryvg(struct unique_id *vgId, struct queryvg **queryVG, char *pvName)
        {
            CPPUNIT_ASSERT(vgId != NULL);
            CPPUNIT_ASSERT(queryVG != NULL);
            CPPUNIT_ASSERT_EQUAL(static_cast<char*>(NULL), pvName);
            // We have only one VG, VG 0. Id must match.
            CPPUNIT_ASSERT(memcmp(vgId, &volumeGroups.vgs[0].vg_id, sizeof(*vgId)) == 0);
            *queryVG = &volumeGroup0;
            return 0;
        }
        virtual int lvm_querylv(struct lv_id *lvId, struct querylv **queryLV, char *pvName)
        {
            CPPUNIT_ASSERT(lvId != NULL);
            CPPUNIT_ASSERT(queryLV != NULL);
            CPPUNIT_ASSERT_EQUAL(static_cast<char*>(NULL), pvName);
            for(long i = 0; i < volumeGroup0.num_lvs; i++)
            {
                if(memcmp(lvId, &volumeGroup0.lvs[i].lv_id, sizeof(*lvId)) == 0)
                {
                    *queryLV = &logicalVolumes[i];
                    return 0;
                }
            }
            CPPUNIT_FAIL("Invalid lvId used in lvm_querylv call.");
            return -1;
        }
        struct queryvgs volumeGroups;
        struct queryvg volumeGroup0;// We have only one VG.
        struct lv_array lvArray[partition_cnt];
        struct querylv logicalVolumes[partition_cnt];
    public:
        DiskPartLogVolDiskDependTest()
        {
            // Create necessary lvm structures.
            
            // Setup all volume groups.
            memset(&volumeGroups, 0, sizeof(volumeGroups));
            volumeGroups.num_vgs = 1;// only one VG, volumeGroup0.
            volumeGroups.vgs[0].vg_id.word1 = 1234;// Id for VG 0.
            
            //Setup volume group 0.
            memset(&volumeGroup0, 0, sizeof(volumeGroup0));
            volumeGroup0.num_lvs = partition_cnt;
            volumeGroup0.lvs = lvArray;
            for(long i = 0; i < volumeGroup0.num_lvs; i++)
            {
                memset(&volumeGroup0.lvs[i], 0, sizeof(volumeGroup0.lvs[i]));
                volumeGroup0.lvs[i].lv_id.vg_id = volumeGroups.vgs[0].vg_id;// Sets logical volume id.
                volumeGroup0.lvs[i].lv_id.minor_num = i;// Sets logical volume id.
                strncpy(volumeGroup0.lvs[i].lvname, partition0_name, LVM_NAMESIZ);
                volumeGroup0.lvs[i].lvname[LVM_NAMESIZ - 1] = 0;
            }
            // Set partition names in VG array of LVs.
            strncpy(volumeGroup0.lvs[0].lvname, partition0_name, LVM_NAMESIZ);
            volumeGroup0.lvs[0].lvname[LVM_NAMESIZ - 1] = 0;
            strncpy(volumeGroup0.lvs[1].lvname, partition1_name, LVM_NAMESIZ);
            volumeGroup0.lvs[1].lvname[LVM_NAMESIZ - 1] = 0;
            strncpy(volumeGroup0.lvs[2].lvname, partition2_name, LVM_NAMESIZ);
            volumeGroup0.lvs[2].lvname[LVM_NAMESIZ - 1] = 0;
            
            // Set LVs.
            for(long i = 0; i < volumeGroup0.num_lvs; i++)
            {
                memset(&logicalVolumes[i], 0, sizeof(logicalVolumes[i]));
                strncpy(logicalVolumes[i].lvname, volumeGroup0.lvs[i].lvname, LVM_NAMESIZ);
                logicalVolumes[i].lvname[LVM_NAMESIZ - 1] = 0;
            }
            // Only partition 2 is unmounted and needs size info from lvm.
            logicalVolumes[2].ppsize = partition2_blk_size;
            logicalVolumes[2].currentsize = partition2_blks;
        }
    };
#elif defined(hpux)
    class DiskPartLogVolDiskDependTest : public SCXSystemLib::DiskDependDefault
    {
        FILE mntentFile;// Mock MNT_MNTTAB file handle.
        bool mntentFileOpen;// Flag set to true if MNT_MNTTAB is open.
        int mntentCnt;// Mount point counter.
        virtual int stat(const char *path, struct stat *buf)
        {
            if (instrumentTest)
            {
                std::cout<<"stat() "<<path<<endl;
            }
            CPPUNIT_ASSERT(path != NULL);
            CPPUNIT_ASSERT_MESSAGE("stat() called with invalid path.", strcmp(path, MNT_MNTTAB) == 0 );
            CPPUNIT_ASSERT(buf != NULL);
            memset(buf, 0, sizeof(struct stat));
            buf->st_mtime = 10101010;
            buf->st_size = 240;
            if (instrumentTest)
            {
                std::cout<<"stat() exit"<<endl;
            }
            return 0;
        }
        virtual FILE *setmntent(const char *path, const char *type)
        {
            if (instrumentTest)
            {
                std::cout<<"setmntent() "<<path<<endl;
            }
            CPPUNIT_ASSERT_EQUAL(false, mntentFileOpen);
            CPPUNIT_ASSERT(path != NULL);
            CPPUNIT_ASSERT_MESSAGE("setmntent() called with invalid path.", strcmp(path, MNT_MNTTAB) == 0 );
            CPPUNIT_ASSERT(path != NULL);
            CPPUNIT_ASSERT_MESSAGE("setmntent() called with invalid type.", strcmp(type, "r") == 0 );
            mntentCnt = 0;
            mntentFileOpen = true;
            if (instrumentTest)
            {
                std::cout<<"setmntent() exit"<<endl;
            }
            return &mntentFile;
        }
        virtual struct mntent *getmntent(FILE *stream)
        {
            if (instrumentTest)
            {
                std::cout<<"getmntent() "<<mntentCnt<<endl;
            }
            CPPUNIT_ASSERT_EQUAL(true, mntentFileOpen);
            CPPUNIT_ASSERT_EQUAL(&mntentFile, stream);
            static struct mntent mountPoints;
            memset(&mountPoints, 0, sizeof(mountPoints));
            switch(mntentCnt)
            {
            case 0:
                mountPoints.mnt_fsname = const_cast<char*>(mountPoint0_devName);
                mountPoints.mnt_dir = const_cast<char*>(mountPoint0_name);
                mountPoints.mnt_type = const_cast<char*>(mountPoint0_basetype);
                mountPoints.mnt_opts = const_cast<char*>(mountPoint0_opts);
                break;
            case 1:
                mountPoints.mnt_fsname = const_cast<char*>(mountPoint1_devName);
                mountPoints.mnt_dir = const_cast<char*>(mountPoint1_name);
                mountPoints.mnt_type = const_cast<char*>(mountPoint1_basetype);
                mountPoints.mnt_opts = const_cast<char*>(mountPoint1_opts);
                break;
            case 2:
                mountPoints.mnt_fsname = const_cast<char*>(mountPoint2_devName);
                mountPoints.mnt_dir = const_cast<char*>(mountPoint2_name);
                mountPoints.mnt_type = const_cast<char*>(mountPoint2_basetype);
                mountPoints.mnt_opts = const_cast<char*>(mountPoint2_opts);
                break;
            default:
                if (instrumentTest)
                {
                    std::cout<<"getmntent() NULL exit"<<endl;
                }
                return NULL;
            }
            mntentCnt++;
            if (instrumentTest)
            {
                std::cout<<"getmntent() exit"<<endl;
            }
            return &mountPoints;
        }
        virtual int endmntent(FILE *stream)
        {
            if (instrumentTest)
            {
                std::cout<<"endmntent() "<<endl;
            }
            CPPUNIT_ASSERT_EQUAL(true, mntentFileOpen);
            CPPUNIT_ASSERT_EQUAL(&mntentFile, stream);
            mntentCnt = 0;
            mntentFileOpen = false;
            if (instrumentTest)
            {
                std::cout<<"endmntent() exit"<<endl;
            }
            return 1;
        }
        virtual int statvfs(const char* path, struct statvfs64* buf)
        {
            if (instrumentTest)
            {
                std::cout<<"statvfs64() "<<path<<" enter/exit?"<<endl;
            }
            return statvfs64test(path, buf);
        }
        virtual int Run(const std::wstring &command, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr,
                unsigned timeout, const SCXCoreLib::SCXFilePath& cwd, const SCXCoreLib::SCXFilePath& chrootPath )
        {
            if (instrumentTest)
            {
                std::cout<<"Run() "<<endl;
            }
            CPPUNIT_ASSERT(std::wstring(L"/sbin/vgdisplay -v") == command || std::wstring(L"/sbin/lvlnboot -v") == command);
            CPPUNIT_ASSERT(SCXCoreLib::SCXFilePath() == cwd);
            CPPUNIT_ASSERT(SCXCoreLib::SCXFilePath() == chrootPath);
            if(std::wstring(L"/sbin/vgdisplay -v") == command)
            {
                mystdout<<"--- Volume groups ---"<<endl;
                mystdout<<"VG Name                     /dev/vg00"<<endl;
                mystdout<<endl;
                mystdout<<"   --- Logical volumes ---"<<endl;
                mystdout<<"   LV Name                     "<<partition0_name<<endl;
                mystdout<<"   LV Size (Mbytes)            1792"<<endl;
                mystdout<<endl;
                mystdout<<"   LV Name                     "<<partition1_name<<endl;
                mystdout<<"   LV Size (Mbytes)            2048"<<endl;
                mystdout<<endl;
                mystdout<<"   LV Name                     "<<partition2_name<<endl;
                mystdout<<"   LV Size (Mbytes)            5120"<<endl;
                mystdout<<endl;
                mystdout<<endl;
                mystdout<<"   --- Physical volumes ---"<<endl;
                mystdout<<"   PV Name                     /dev/dsk/c0t1d0"<<endl;
                mystdout<<endl;
                mystdout<<endl;
                mystdout<<"VG Name                     /dev/vg01"<<endl;
                mystdout<<endl;
                mystdout<<"   --- Logical volumes ---"<<endl;
                mystdout<<"   LV Name                     "<<partition3_name<<endl;
                mystdout<<"   LV Size (Mbytes)            1792"<<endl;
                mystdout<<endl;
                mystdout<<endl;
                mystdout<<"   --- Physical volumes ---"<<endl;
                mystdout<<"   PV Name                     /dev/dsk/c0t0d0s2"<<endl;
                mystdout<<endl;
                mystdout<<endl;
            }
            else // Output from "lvlnboot -v" command.
            {
                mystdout<<"Boot Definitions for Volume Group /dev/vg00:"<<endl;
                mystdout<<"Physical Volumes belonging in Root Volume Group:"<<endl;
                mystdout<<"       /dev/dsk/c0t1d0 (0/0/1/0.0.0_ -- Boot Disk"<<endl;
                if(bootRootShareLV)
                {
                    mystdout<<"Root: "<<partition2_nameShort<<" on: /dev/dsk/c0t1d0"<<endl;
                }
                else
                {
                    mystdout<<"Boot: "<<partition2_nameShort<<" on: /dev/dsk/c0t1d0"<<endl;
                    mystdout<<"Root: "<<partition0_nameShort<<" on: /dev/dsk/c0t1d0"<<endl;
                }
                mystdout<<"Swap: "<<partition1_nameShort<<" on: /dev/dsk/c0t1d0"<<endl;
                mystdout<<"Dump: "<<partition1_nameShort<<" on: /dev/dsk/c0t1d0, 0"<<endl;
                mystdout<<endl;
                mystdout<<endl;
            }
            if (instrumentTest)
            {
                std::cout<<"Run() exit"<<endl;
            }
            return 0;
        }
        bool rdevOpen[partition_cnt];
        int GetFD(int partitionIndex)
        {
            // Each partition has it's own index file descriptor for the purpose of calling ioctl. We use high
            // file descriptor values in order not to interfere with the descriptors returned by the OS in case some
            // other code also calls same system calls.
            return partitionIndex + 10000;
        }
        virtual int _open(const char* pathname, int flags)
        {
            CPPUNIT_ASSERT(pathname != NULL);
            string pathNameStr(pathname);
            int partitionIndex = 0;
            // Right now files are opened only for ioctl(DIOC_CAPACITY) and only for unmounted drives.
            if (pathNameStr == partition2_rname)
            {
                partitionIndex = 2;
            }
            else if (pathNameStr == partition3_rname)
            {
                partitionIndex = 3;
            }
            else
            {
                CPPUNIT_FAIL("Tried to open file with invalid file name \"" + pathNameStr + "\".");
            }
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Tried to open file \"" + pathNameStr + "\" with invalid flags = " + 
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(flags)) + ".", O_RDONLY, flags);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("File \"" + pathNameStr + "\" already opened.", false, rdevOpen[partitionIndex]);
            rdevOpen[partitionIndex] = true;
            return GetFD(partitionIndex);
        }
        virtual int _close(int fd)
        {
            CPPUNIT_ASSERT_MESSAGE("When trying to close file, invalid file descriptor fd = " +
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) + ".", fd >= GetFD(0));
            int partitionIndex = fd - GetFD(0);
            CPPUNIT_ASSERT_MESSAGE("When trying to close file, invalid file descriptor fd = " +
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) + ".", partitionIndex < partition_cnt);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("File with fd = " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) +
                " already closed.", true, rdevOpen[partitionIndex]);
            rdevOpen[partitionIndex] = false;
            return 0;
        }
        virtual int _ioctl(int fd, int request, void* data)
        {
            CPPUNIT_ASSERT_MESSAGE("Trying to call ioctl with invalid file descriptor fd = " +
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) + ".", fd >= GetFD(0));
            int partitionIndex = fd - GetFD(0);
            CPPUNIT_ASSERT_MESSAGE("Trying to call ioctl with invalid file descriptor fd = " +
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) + ".", partitionIndex < partition_cnt);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("File with fd = " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) +
                " not opened.", true, rdevOpen[partitionIndex]);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("ioctl with fd = " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) +
                " received invalid request = " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(request)) + ".",
                DIOC_CAPACITY, request);
            CPPUNIT_ASSERT_MESSAGE("ioctl with fd = " + SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) +
                " received NULL data pointer.", data != NULL);
            capacity_type *ct = static_cast<capacity_type*>(data);
            memset(ct, 0, sizeof(*ct));
            switch (partitionIndex)
            {
            case 2:
                ct->lba = partition2_size_blks;
                return 0;
            case 3:
                ct->lba = partition3_size_blks;
                return 0;
            }
            // Right now this ioctl is called only for unmounted partitions.
            CPPUNIT_FAIL("Trying to call ioctl with invalid file descriptor fd = " +
                SCXCoreLib::StrToUTF8(SCXCoreLib::StrFrom(fd)) + ".");
            return -1;
        }
    public:
        DiskPartLogVolDiskDependTest():mntentFileOpen(false), mntentCnt(0)
        {
            memset(&mntentFile, 0, sizeof(mntentFile));
            size_t i;
            for(i = 0; i < partition_cnt; i++)
            {
                rdevOpen[i] = false;
            }
        }
    };
#elif defined(sun)
    class DiskPartLogVolDiskDependTest : public SCXSystemLib::DiskDependDefault
    {
    protected:
        virtual int statvfs64(const char* path, struct statvfs64* buf)
        {
            if (instrumentTest)
            {
                std::cout<<"statvfs64test() "<<path<<" enter/exit?"<<endl;
            }
            return statvfs64test(path, buf);
        }
        virtual int statvfs(const char* path, SCXSystemLib::SCXStatVfs* buf)
        {
            if (instrumentTest)
            {
                std::cout<<"statvfs64() "<<path<<" enter/exit?"<<endl;
            }
            return statvfs64(path, buf);
        }

        virtual void RefreshMNTTab()
        {
            if (instrumentTest)
            {
                std::cout<<"RefreshMNTTab()"<<endl;
            }
            SCXSystemLib::MntTabEntry mte;
            mte.device = SCXCoreLib::StrFromUTF8(partition0_name);
            mte.fileSystem = SCXCoreLib::StrFromUTF8(mountPoint0_basetype);
            mte.mountPoint = SCXCoreLib::StrFromUTF8(mountPoint0_name);
            m_MntTab.push_back(mte);
            mte.device = SCXCoreLib::StrFromUTF8(partition1_name);
            mte.fileSystem = SCXCoreLib::StrFromUTF8(mountPoint1_basetype);
            mte.mountPoint = SCXCoreLib::StrFromUTF8(mountPoint1_name);
            m_MntTab.push_back(mte);
        }
        virtual const std::vector<SCXSystemLib::MntTabEntry>& GetMNTTab()
        {
            if (instrumentTest)
            {
                std::cout<<"GetMNTTab()"<<endl;
            }
            return m_MntTab;
        }
    private:
        std::vector<SCXSystemLib::MntTabEntry> m_MntTab;
    };
#endif
}
#endif// defined(aix) || defined(hpux) || defined(sun)

#if defined(linux) || defined(sun)
namespace
{
    // This class is used to verify the physical disk geometry info returned by the provider.
    // Class simulates mock operating system with necessary system calls required to simulate number of
    // physical hard disks, some with correct disk geometry info and some with incorrect disk geometry info.
    class PhysicalDiskSimulationDepend : public SCXSystemLib::DiskDependDefault
    {
        // This method defines mount points for the simulated physical disks.
        virtual void RefreshMNTTab()
        {
            m_MntTab.clear();
            SCXSystemLib::MntTabEntry mte;
            size_t i;
            for (i = 0; i < m_Tests.size(); i++)
            {
                if (m_Tests[i].mounted == false)
                {
                    continue;
                }
#if defined(linux)
                if (m_Tests[i].cdDrive)
                {
                    mte.fileSystem = L"iso9660";
                }
                else
                {
                    mte.fileSystem = L"ext3";
                }
#elif defined(sun)
                mte.fileSystem = L"ufs";
#endif
                mte.device = m_Tests[i].strDiskName;
                mte.mountPoint = L"/abc" + SCXCoreLib::StrFrom(i);
                mte.devAttribute = L"";
                m_MntTab.push_back(mte);
            }
        }
        // This method provides translation from the mount point device info to the raw device name.
        virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& deviceName)
        {
            std::map<std::wstring, std::wstring> devices;
            size_t i;
            for (i = 0; i < m_Tests.size(); i++)
            {
                if (m_Tests[i].strDiskName == deviceName)
                {
                    devices[deviceName] = m_Tests[i].strDiskDevice;
                    return devices;
                }
            }
 
            string msg = "Invalid deviceName argument when calling PhysicalDiskSimulationDepend::"
                    "GetPhysicalDevices(): " + SCXCoreLib::StrToUTF8(deviceName) + ".";
            CPPUNIT_FAIL(msg);
            return devices;
        }
        virtual std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > GetDevDskInfo()
        {
            // Return empty vector. We already have devices from the mount points.
            return std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> >();
        }
        // Starting file descriptor for mock device files. We use high numbers so if parts of the dependencies that
        // are not overriden in this test try to use descriptors, they will fail.
        int fdStart()
        {
            return 1000;
        }
        // Raw device file descriptor to be used by the system calls. Each physical disk get's its own file descriptor.
        int fdFromDiskDevice(const char* pathName)
        {
            size_t i;
            for (i = 0; i < m_Tests.size(); i++)
            {
                if (SCXCoreLib::StrToUTF8(m_Tests[i].strDiskDevice) == pathName)
                {
                // Use high numbers so calls to the OS that are not overridden in deps will fail if trying to use mock
                // file deskriptor.
                    return static_cast<int>(i) + fdStart();
                }
            }
            string msg = "Invalid pathName argument when calling PhysicalDiskSimulationDepend::fdFromDiskDevice(): " +
                    string(pathName) + ".";
            CPPUNIT_FAIL(msg);
            return CLOSED_DESCRIPTOR;
        }
        // Opens the raw device to be used by the ioctl.
        virtual bool open(const char* pathName, int flags)
        {
            stringstream ss;
            ss << "Invalid flags argument when calling PhysicalDiskSimulationDepend::open(" <<
                pathName << ", " << flags << ").";
#if defined(linux)
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), O_RDONLY | O_NONBLOCK, flags);
#elif defined(sun)
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ss.str(), O_RDONLY, flags);
#endif

            close();
            m_OpenFlags = flags;
            strncpy(m_PathName, pathName, MAXPATHLEN);
            SCXASSERT(0 == strncmp(m_PathName, pathName, MAXPATHLEN));
            m_PathName[MAXPATHLEN - 1] = '\0'; // z-term in case original string was not z-term'ed.
            m_fd = fdFromDiskDevice(pathName);
            return true;
        }
        // Reopens the file if necessary.
        virtual void reopen()
        {
            close();
            open(m_PathName, m_OpenFlags);
        }
        // Closes the file.
        virtual int close()
        {
            m_fd = CLOSED_DESCRIPTOR;
            return 0;
        }
        // This function simulates the ioctl on a mock operating system with mock physical disks. Each disk has it's own
        // file descriptor.
        virtual int ioctl(unsigned long int request, void* data)
        {
            CPPUNIT_ASSERT(data != NULL);
            CPPUNIT_ASSERT(m_fd != CLOSED_DESCRIPTOR);
            CPPUNIT_ASSERT(m_fd >= fdStart());
            size_t testIndex = m_fd - fdStart();
            CPPUNIT_ASSERT(testIndex < m_Tests.size());

#if defined(linux)
            if (request == BLKGETSIZE64)
            {
                if (m_Tests[testIndex].totalSize != 0)
                {
                    u_int64_t* size64 = static_cast<u_int64_t*>(data);
                    memset(size64, 0, sizeof(*size64));
                    *size64 = m_Tests[testIndex].totalSize;
                    return 0;
                }
            }
            else if (request == BLKSSZGET)
            {
                if (m_Tests[testIndex].sectorSize != 0)
                {
                    unsigned long* sectorSize = static_cast<unsigned long*>(data);
                    memset(sectorSize, 0, sizeof(*sectorSize));
                    *sectorSize = static_cast<unsigned long>(m_Tests[testIndex].sectorSize);
                    return 0;
                }
            }
            else if (request == HDIO_GETGEO)
            {
                if (m_Tests[testIndex].headCnt != 0)
                {
                    struct hd_geometry* geo = static_cast<struct hd_geometry*>(data);
                    memset(geo, 0, sizeof(*geo));
                    geo->heads = static_cast<unsigned char>(m_Tests[testIndex].headCnt);
                    geo->cylinders = static_cast<unsigned short>(m_Tests[testIndex].cylCnt);
                    geo->sectors = static_cast<unsigned char>(m_Tests[testIndex].sectPerTrackCnt);
                    return 0;
                }
            }
            else if (request == HDIO_GET_32BIT)
            {
                if (s_instrumentTests)
                {
                    cout << "ioctl(" << SCXCoreLib::StrToUTF8(m_Tests[testIndex].strDiskName) <<
                            ", HDIO_GET_32BIT)" << endl;
                }
                int* io32bit = static_cast<int*>(data);
                *io32bit = 0;
                return 0;
            }
            else if (request == HDIO_GET_IDENTITY)
            {
                if (s_instrumentTests)
                {
                    cout << "ioctl(" << SCXCoreLib::StrToUTF8(m_Tests[testIndex].strDiskName) <<
                            ", HDIO_GET_IDENTITY)" << endl;
                }
                if(m_Tests[testIndex].ioctl_HDIO_GET_IDENTITY_OK)
                {
                    struct hd_driveid* hdid = static_cast<struct hd_driveid*>(data);
                    memset(hdid, 0, sizeof(*hdid));
                    string serialNumber = SCXCoreLib::StrToUTF8(m_Tests[testIndex].strSerialNumber);
                    // Not necesarily null terminated string in the output buffer.
                    strncpy(reinterpret_cast<char*>(hdid->serial_no), serialNumber.c_str(), sizeof(hdid->serial_no));
                    return 0;
                }
            }
            else if (request == SG_GET_VERSION_NUM)
            {
                if (s_instrumentTests)
                {
                    cout << "ioctl(" << SCXCoreLib::StrToUTF8(m_Tests[testIndex].strDiskName) <<
                            ", SG_GET_VERSION_NUM)" << endl;
                }
                int* version = static_cast<int*>(data);
                *version = 31000;
                return 0;
            }
            else if (request == SG_IO)
            {
                if (m_Tests[testIndex].ioctl_SG_IO_OK)
                {
                    sg_io_hdr_t* io_hdr = static_cast<sg_io_hdr_t*>(data);
                    if (io_hdr == NULL)
                    {
                        CPPUNIT_ASSERT_MESSAGE("io_hdr pointer is null", false);
                    }
                    else
                    {
                        CPPUNIT_ASSERT_EQUAL(6, io_hdr->cmd_len);// We support only SCSI INQUIRY command.
                    }

                    if (s_instrumentTests)
                    {
                        if (io_hdr == NULL)
                        {
                            cout << "ioctl(" << SCXCoreLib::StrToUTF8(m_Tests[testIndex].strDiskName) <<
                                    ", SG_IO, io_hdr = NULL)" << endl;
                        }
                        else
                        {
                            cout << "ioctl(" << SCXCoreLib::StrToUTF8(m_Tests[testIndex].strDiskName) <<
                                    ", SG_IO)" << endl;
                            cout << "  io_hdr = " << io_hdr << endl;
                            cout << "  io_hdr->interface_id = " << static_cast<char>(io_hdr->interface_id) << endl;
                            cout << "  io_hdr->cmd_len = " << static_cast<int>(io_hdr->cmd_len) << endl;
                            cout << "  io_hdr->cmdp = " << static_cast<void*>(io_hdr->cmdp) << endl;
                            cout << "  io_hdr->cmdp[0] = " << static_cast<int>(io_hdr->cmdp[0]) << endl;
                            cout << "  io_hdr->cmdp[1] = " << static_cast<int>(io_hdr->cmdp[1]) << endl;
                            cout << "  io_hdr->cmdp[2] = " << static_cast<int>(io_hdr->cmdp[2]) << endl;
                            cout << "  io_hdr->dxfer_direction = " << io_hdr->dxfer_direction << endl;
                            cout << "  io_hdr->dxfer_len = " << io_hdr->dxfer_len << endl;
                            cout << "  io_hdr->dxferp = " << io_hdr->dxferp << endl;
                            cout << "  io_hdr->mx_sb_len = " << static_cast<int>(io_hdr->mx_sb_len) << endl;
                            cout << "  io_hdr->sbp = " << static_cast<void*>(io_hdr->sbp) << endl;
                        }
                    }
                    CPPUNIT_ASSERT_EQUAL('S', io_hdr->interface_id);
                    CPPUNIT_ASSERT_EQUAL(SG_DXFER_FROM_DEV, io_hdr->dxfer_direction);// We support only reading.
                    CPPUNIT_ASSERT(io_hdr->dxfer_len != 0);
                    CPPUNIT_ASSERT(io_hdr->dxferp != NULL);
                    CPPUNIT_ASSERT(io_hdr->cmdp != NULL);

                    // We support only op code 0x03 or 0x12.
                    CPPUNIT_ASSERT(0x03 == io_hdr->cmdp[0] || 0x12 == io_hdr->cmdp[0]);
                    if (0x03 == io_hdr->cmdp[0])
                    {
                        // This dependency class does not support all physical disk properties. Availability
                        // property is not supported.
                        return -1;
                    }
                    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>((io_hdr->dxfer_len >> 8) & 0xff), io_hdr->cmdp[3]);
                    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(io_hdr->dxfer_len & 0xff), io_hdr->cmdp[4]);

                    CPPUNIT_ASSERT(io_hdr->mx_sb_len >= 3);
                    CPPUNIT_ASSERT(io_hdr->sbp != NULL);

                    // We only support page 0 - evpd 0 or page 0x80 - evpd 1.
                    bool page0x00evpd0 = false;
                    bool page0x80evpd1 = false;
                    if (((io_hdr->cmdp[1] & 1) == 0) && (io_hdr->cmdp[2] == 0))
                    {
                        page0x00evpd0 = true;
                    }
                    else if (((io_hdr->cmdp[1] & 1) == 1) && (io_hdr->cmdp[2] == 0x80))
                    {
                        page0x80evpd1 = true;
                    }
                    if (page0x00evpd0 || page0x80evpd1)
                    {
                        io_hdr->status = 0;
                        io_hdr->host_status = 0;
                        io_hdr->driver_status = 0;
                        io_hdr->sbp[0] = 0;
                        io_hdr->sbp[1] = 0;
                        io_hdr->sbp[2] = 0;
                        memset(io_hdr->dxferp, 0, io_hdr->dxfer_len);
                        char* dxferp = static_cast<char*>(io_hdr->dxferp);
                        
                        if (page0x00evpd0)
                        {
                            CPPUNIT_ASSERT(io_hdr->dxfer_len >= 16);// At least enough space for 8 byte vendor ID.
                            string manufacturer = SCXCoreLib::StrToUTF8(m_Tests[testIndex].strManufacturer);
                            // Not necesarily null terminated string in the output buffer.
                            strncpy(dxferp + 8, manufacturer.c_str(), 8);
                        }
                        else if (page0x80evpd1)
                        {
                            CPPUNIT_ASSERT(io_hdr->dxfer_len >= 12);// At least enough space for 8 byte serial num.
                            dxferp[1] = static_cast<char>(0x80);
                            dxferp[3] = 0x08;
                            string serialNumber = SCXCoreLib::StrToUTF8(m_Tests[testIndex].strSerialNumber);
                            // Not necesarily null terminated string in the output buffer.
                            strncpy(dxferp + 4, serialNumber.c_str(), 8);
                        }
                        return 0;
                    }
                }
            }
#elif defined(sun)
            if (request == DKIOCGMEDIAINFO)
            {
                if (m_Tests[testIndex].totalSize != 0)
                {
                    struct dk_minfo* minfo = static_cast<struct dk_minfo*>(data);
                    memset(minfo, 0, sizeof(*minfo));
                    minfo->dki_lbsize = 1024;
                    // +1 to make it little bigger than c*h*spt but still less than one cylinder size.
                    minfo->dki_capacity = m_Tests[testIndex].totalSize / 1024;
                    minfo->dki_media_type = DK_FIXED_DISK;
                    return 0;
                }
            }
            else if (request == DKIOCGVTOC)
            {
                if (m_Tests[testIndex].sectorSize != 0)
                {
                    struct vtoc* v = static_cast<struct vtoc*>(data);
                    memset(v, 0, sizeof(*v));
                    v->v_sectorsz = m_Tests[testIndex].sectorSize;
                    return 0;
                }
            }
            else if (request == DKIOCGGEOM)
            {
                if (m_Tests[testIndex].headCnt != 0)
                {
                    struct dk_geom* geo = static_cast<struct dk_geom*>(data);
                    memset(geo, 0, sizeof(*geo));
                    geo->dkg_nhead = m_Tests[testIndex].headCnt;
                    geo->dkg_pcyl = m_Tests[testIndex].cylCnt;
                    geo->dkg_nsect = m_Tests[testIndex].sectPerTrackCnt;
                    return 0;
                }
            }
#endif
            return -1;
        }
        
    public:
        // Structure returning what is the expected result of a particular test case.
        struct PhysicalDiskSimulationExpectedResults
        {
            // Expected results.
            std::wstring strDiskName, strDiskDevice;
            std::wstring strSerialNumber, strManufacturer;
            scxulong valSizeInBytes, valCylCount, valHeadCount, valSectorCount, valTracksPerCylinder, valTotalTracks;
            unsigned int valSectorSize, valSectorsPerTrack;
            // Inputs feed into the mock OS.
            scxulong totalSize, sectorSize, headCnt, sectPerTrackCnt, cylCnt;
            bool mounted;
#if defined(linux)            
            bool ioctl_HDIO_GET_IDENTITY_OK, ioctl_SG_IO_OK;// Determines if ioctl will succeed.
            bool cdDrive;
#endif
            void Clear()
            {
                strDiskName = strDiskDevice = L"";
                valSizeInBytes = valCylCount = valHeadCount = valSectorCount = valTracksPerCylinder = valTotalTracks = 0;
                valSectorSize = valSectorsPerTrack = 0;
                totalSize = sectorSize = headCnt = sectPerTrackCnt = cylCnt = 0;
                mounted = true;
#if defined(linux)            
                ioctl_HDIO_GET_IDENTITY_OK = ioctl_SG_IO_OK = false;
                cdDrive = false;
#endif
                strSerialNumber = strManufacturer = L"";
            }
        };
        // Method sets up a mock OS with number of mock physical disks necessary to perform the tests. Each
        // member of the vector contains expected results and various OS parameters for one test case. Each test case
        // is done using one mock physical disk.
        void SetupMockOS(const std::vector<PhysicalDiskSimulationExpectedResults> &tests)
        {
            m_Tests = tests;
        }
    protected:
        // List of test cases to be executed. Each test case represents one physical disc.
        std::vector<PhysicalDiskSimulationExpectedResults> m_Tests;
    };
}
#endif// defined(linux) || defined(sun)
#if defined(linux)
namespace
{
    // This class enables CD drive detection in PhysicalDiskSimulationDepend class.
    class PhysicalDiskSimulationDependCD : public PhysicalDiskSimulationDepend
    {
        // Signal that optical devices should not be ignored.
        // This will allow detection of all optical devices, not only CD with iso9660.
        virtual bool FileSystemIgnored(const std::wstring& fs)
        {
            if (fs == L"iso9660")
            {
                return false;
            }
            return PhysicalDiskSimulationDepend::FileSystemIgnored(fs);
        }
        // Create mock /proc/sys/dev/cdrom/info stream.
        virtual SCXCoreLib::SCXHandle<std::wistream> GetWIStream(const char *name)
        {
            CPPUNIT_ASSERT_EQUAL("/proc/sys/dev/cdrom/info", std::string(name));
            std::wstring cdromInfoStr =
                L"CD-ROM information, Id: cdrom.c 3.20 2003/12/17\n"
                L"drive name:";
            size_t i;
            for (i = 0; i < m_Tests.size(); i++)
            {
                if (m_Tests[i].cdDrive)
                {
                    cdromInfoStr += L"  ";
                    cdromInfoStr += m_Tests[i].strDiskDevice.substr(5);
                }
            }
            std::wistringstream* pStr = new std::wistringstream;
            pStr->str(cdromInfoStr);
            SCXCoreLib::SCXHandle<std::wistream> ret(pStr);
            return ret;
        }
    };
}
#endif
#endif /* DISKDEPEND_MOCK_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
