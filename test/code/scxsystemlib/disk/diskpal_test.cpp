/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-08-08 14:20:00

  Disk PAL tests.

*/
/**
   \file

   \brief       Disk PAL tests
   \date        2007-08-08 14:20:00
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxmath.h>

#if defined(hpux) || defined(sun)
#define RESOLVE_HOSTNAME_FORBLOCKINGTESTS  1
#include <scxcorelib/scxnameresolver.h>
#endif

#include <scxsystemlib/statisticalphysicaldiskenumeration.h>
#include <scxsystemlib/statisticallogicaldiskenumeration.h>
#if defined(linux)
#include <scxsystemlib/scxlvmutils.h>
#endif
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>
#include <testutils/disktestutils.h>
#include <scxcorelib/testlogframeworkhelper.h>

#include <vector>
#include <sstream>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <math.h>
#include <iterator>

#include "diskdepend_mock.h"

#if defined(sun)
#include <sys/dkio.h>
#include <sys/vtoc.h>
#endif

using SCXCoreLib::SCXException;
using SCXCoreLib::StrToUTF8;

#if defined(aix)
class TestDiskDependDefault : public SCXSystemLib::DiskDependDefault
{
private:
    std::vector<std::string> m_disks;
public:
    TestDiskDependDefault(std::vector<std::string> disks)
        : m_disks(disks)
    {
    }

    /*
      This function expects on first call that name->name is FIRST_DISKPATH. This will initialize
      the static int curDisk, which is used to keep track of current position in the vector.
      perfstat_disk is called once for each disk, and our code assumes that name->name = FIRST_DISKPATH
      when there are no more disks in to be looked at.
    */
    virtual int perfstat_disk(perfstat_id_t* name, perfstat_disk_t* buf, size_t struct_size, int n)
    {
        static int curDisk = -1;

        if (std::string(name->name) == FIRST_DISKPATH)
        {
            curDisk = 0;
        }

        if (curDisk < m_disks.size())
        {
            // return in buf->name the name of the disk we are currently looking at
            strncpy(buf->name, m_disks[curDisk].c_str(), struct_size);
        }
        else
        {
            // We only get here if perfstat_disk is called more than necessary
            strcpy(name->name, FIRST_DISKPATH);
            return -1;
        }

        if (curDisk == m_disks.size() - 1)
        {
            // if this is the last disk, communicate this by setting name->name = FIRST_DISKPATH
            strcpy(name->name, FIRST_DISKPATH);
        }
        else
        {
            // return in name->name something that isn't FIRST_DISKPATH, so that the caller knows there are more disks.
            strcpy(name->name, "NOT FIRST_DISKPATH");
        }

        // move to the next disk in the list and return that we've filled one structure
        curDisk++;
        return 1;
    }
};
#endif // defined(aix)

class TestDisk
{
public:
    TestDisk()
    {
        m_name = L"N/A";
        m_blockSize = 0;
        m_mbUsed = 0;
        m_mbFree = 0;
    }

    std::wstring m_type;
    std::wstring m_name;
    std::wstring m_dev;
    std::wstring m_mountPoint;
    std::wstring m_fs;
    scxulong m_mbUsed;
    scxulong m_mbFree;
    scxulong m_blockSize;

    struct {
        scxulong num;
        scxulong bytes;
        scxulong ms;
    } read;

    struct {
        scxulong num;
        scxulong bytes;
        scxulong ms;
    } write;
};

std::wostream & operator<<(std::wostream &os, SCXCoreLib::SCXHandle<TestDisk> p)
{
    os << p->m_name;
    return os;
}

class TestDisks
{
private:
    SCXSystemLib::DiskDependDefault m_deps;
public:
    static pid_t ExerciseDiskProcess(const char* path)
    {
        static unsigned int i = 0;
        char buf[512];

        pid_t pid = fork();
        if (0 != pid)
            return pid; // parent process;
        sprintf(buf, ">/tmp/find.test.%u", ++i);
        CPPUNIT_ASSERT(freopen("/dev/null","r",stdin) != NULL);
        CPPUNIT_ASSERT(freopen(buf,"w",stdout) != NULL);
        CPPUNIT_ASSERT(freopen(buf,"w",stderr) != NULL);
        execlp("find","find",path,"-name","*","-type","f","-xdev", static_cast<char*>(0));
        exit(0);
#if defined(aix)
        return -1;
#endif
    }

    static void ExerciseDiskProcessKill(pid_t pid)
    {
        kill(pid, SIGKILL);
        waitpid(pid, 0, 0);
    }

    static void ExerciseDiskProcessCleanup()
    {
        CPPUNIT_ASSERT(system("rm -rf /tmp/find.test.*") != -1);
    }

    TestDisks()
    {
        m_metaDeviceFound = false;
        GetMnttabData(); // Check what we can expect...
        GetPhysicalData(); // Finds the physical disks
        GetLogicalData(); // Finds the logical disks
        GetDFData();     // Gets disk sizes.
        GetBlockSize(); // Get Block sizes of disks.
    }

    ~TestDisks()
    {
    }

    SCXCoreLib::SCXHandle<TestDisk> FindDisk(const std::wstring& id)
    {
        for(std::vector< SCXCoreLib::SCXHandle<TestDisk> >::iterator it = physical.begin();
            it != physical.end(); it++)
        {
            if ((*it)->m_dev == id)
                return *it;
        }
        for(std::vector<SCXCoreLib::SCXHandle<TestDisk> >::iterator it = logical.begin();
            it != logical.end(); it++)
        {
            if ((*it)->m_dev == id || id == GetLVFromDevice((*it)->m_dev))
                return *it;
        }
        return SCXCoreLib::SCXHandle<TestDisk>(0);
    }

    SCXCoreLib::SCXHandle<TestDisk> FindDiskByMountPoint(const std::wstring& mp)
    {
        // Only logical disks should have mount points
        for(std::vector<SCXCoreLib::SCXHandle<TestDisk> >::iterator it = logical.begin();
            it != logical.end(); it++)
        {
            if ((*it)->m_mountPoint == mp)
                return *it;
        }
        return SCXCoreLib::SCXHandle<TestDisk>(0);
    }

    void GetPhysicalData()
    {
        char buf[256];
        std::stringstream command("");
#if defined(aix)
        command << "iostat -d";
#elif defined(linux)
        command << "vmstat -d";
#elif defined(sun)
        command << "iostat -xn";
#elif defined(hpux)
        command << "iostat";
#endif
        FILE *fp = popen(command.str().c_str(), "r");

        if (0 != fp)
        {
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header
#if defined(aix)
            while (std::string("Disks:") != std::string(buf, 6))
            {
                CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header
            }
#endif
            while ( ! feof(fp))
            {
                SCXCoreLib::SCXHandle<TestDisk> disk(0);
                memset(buf, 0, sizeof(buf));
                char *ret = fgets(buf, sizeof(buf), fp);
                CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
#if defined(aix)
                if (parts.size() > 5)
                {
                    if (parts[0].substr(0,2) != L"cd")
                    {
                        disk = new TestDisk();
                        disk->m_dev = L"/dev/" + parts[0];
                        disk->m_name = parts[0];
                        physical.push_back(disk);
                    }
                }
#elif defined(hpux)
                if (parts.size() > 3)
                {
                    disk = new TestDisk();
                    if (parts[0].substr(0,4) == L"disk")
                    {
                        disk->m_dev = L"/dev/disk/" + parts[0];
                        if ( ! IsInLVMtab(disk->m_dev))
                        { // HPUX should only havd LVMs - disks not LVMs might be CD-ROMs shadows.
                            continue;
                        }
                    }
                    else
                    {
                        disk->m_dev = L"/dev/dsk/" + parts[0];
                        if ( ! IsInMnttab(disk->m_dev) && // For example CD-ROMs might show up like this
                             ! IsInLVMtab(disk->m_dev)) // Old style names may occur in LVMTAB too.
                        {
                            continue;
                        }
                    }
                    disk->m_name = parts[0];
                    physical.push_back(disk);
                }
#elif defined(linux)
/*
  Example of output from vmstat -d

> vmstat -d
disk- ------------reads------------ ------------writes----------- -----IO------
       total merged sectors      ms  total merged sectors      ms    cur    sec
ram0       0      0       0       0      0      0       0       0      0      0
ram1       0      0       0       0      0      0       0       0      0      0
ram2       0      0       0       0      0      0       0       0      0      0
ram3       0      0       0       0      0      0       0       0      0      0
ram4       0      0       0       0      0      0       0       0      0      0
ram5       0      0       0       0      0      0       0       0      0      0
ram6       0      0       0       0      0      0       0       0      0      0
ram7       0      0       0       0      0      0       0       0      0      0
ram8       0      0       0       0      0      0       0       0      0      0
ram9       0      0       0       0      0      0       0       0      0      0
ram10      0      0       0       0      0      0       0       0      0      0
ram11      0      0       0       0      0      0       0       0      0      0
ram12      0      0       0       0      0      0       0       0      0      0
ram13      0      0       0       0      0      0       0       0      0      0
ram14      0      0       0       0      0      0       0       0      0      0
ram15      0      0       0       0      0      0       0       0      0      0
xvda  1136124  38011 18006883 1924184 2506805 3535429 49138072 46101292      0   2240
xvdb     480    920    5468     152      0      0       0       0      0      0
md0        0      0       0       0      0      0       0       0      0      0
loop0      0      0       0       0      0      0       0       0      0      0
loop1      0      0       0       0      0      0       0       0      0      0

 */
                if (parts.size() > 0)
                {
                    if (parts[0][1] == 'd'
                        /* Also handle 'xvd' devices - virtual Xen disks */
                        || (parts[0][0] == 'x' && parts[0][1] == 'v' && parts[0][2] == 'd'))
                    {
                        disk = new TestDisk();
                        if (parts[0][0] == 'f')
                        {
                            disk->m_type = L"floppy";
                        }
                        if (parts[0][0] == 'h')
                        {
                            disk->m_type = L"ide";
                        }
                        if (parts[0][0] == 's')
                        {
                            disk->m_type = L"scsi";
                        }
                        disk->m_dev = L"/dev/" + parts[0];
                        disk->m_name = parts[0];

                        // We only report on mounted disks, so ignore unmounted Xen and IDE disks
                        if (parts[0][0] == 'h' || parts[0][0] == 'x')
                        {
                            if ( ! IsMountedDev(disk->m_dev) )
                            {
                                continue;
                            }
                        }

                        if  (std::wstring::npos == parts[0].find_first_of(L"0123456789"))
                        {
                            physical.push_back(disk);
                        }
                    }
                }
#elif defined(sun)
                if (parts.size() > 10)
                {
                    // The created ID will not match any RAID devices
                    // since those are in /dev/md/dsk/
                    std::wstring id = L"/dev/dsk/" + parts[10];
                    for (unsigned int i=0; 0 == disk && i<mnttab.size(); i++)
                    {
                        if (mnttab[i].substr(0,id.length()) == id)
                        {
                            disk = new TestDisk();
                        }
                    }
                    // WI 11689: we support 'not-mounted' physical drives
                    // check the type of 'not-mounted' and include 'FIXED'
                    while ( 0 == disk ){
                        int fd = 0;

                        /*
                         * Get an FD to device (Note: We must have privileges for this to work)
                         *
                         * Note: We usually get a device like "/dev/dsk/c0d0", but this won't
                         * work.  We try once (just in case), but if that fails, we build our
                         * own path to look like: "/dev/rdsk/c0d0s0".  If that fails too, then
                         * we just bag it.
                         */

                        if (0 > (fd = open(StrToUTF8(id).c_str(), O_RDONLY)))
                        {
                            /* Reconstruct the path from the name and try again */
                            /* Note that we need to check several slices if the disk does not use all of them. */
                            for (int i = 0; i<=9 && fd < 0; ++i)
                            {
                                std::wstring rawDevice = L"/dev/rdsk/" + parts[10] + SCXCoreLib::StrAppend(L"s", i);
                                if (0 > (fd = open(StrToUTF8(rawDevice).c_str(), O_RDONLY)))
                                {
                                    if ((EIO != errno && ENXIO != errno) || 9 <= i) // EIO _or_ ENXIO is received if the slice is not used.
                                        break;  // will skip it
                                }
                            }
                            if ( 0 > fd )
                                break;  // failed to open
                        }

                        // check type
                        struct dk_minfo dkmedia;
                        memset(&dkmedia, '\0', sizeof(dkmedia));
                        if (0 == ioctl(fd, DKIOCGMEDIAINFO, &dkmedia))
                        {
                            // verify that media is supported
                            if ( dkmedia.dki_media_type != DK_FIXED_DISK ){
                                close(fd);
                                break;
                            }
                        }
                        close( fd );
                        // accept this disk in addition
                        disk = new TestDisk();
                        break;
                    }

                    if (0 != disk)
                    {
                        disk->m_dev = id;
                        disk->m_name = parts[10];
                        physical.push_back(disk);
                    }
                }
#endif
            }
            pclose(fp);
        }
    }

    bool IsInLVMtab(const std::wstring& path)
    {
        char buf[256];
        std::stringstream command("");
        command << "/bin/sh -c \"LANG=C grep -l " << SCXCoreLib::StrToUTF8(path) << " /etc/lvmtab\"";
        FILE *fp = popen(command.str().c_str(), "r");

        if (0 != fp)
        {
            memset(buf, 0, sizeof(buf));
            char *ret = fgets(buf, sizeof(buf), fp);
            CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
            std::wstring line = SCXCoreLib::StrTrim(SCXCoreLib::StrFromUTF8(buf));
            if (line == L"/etc/lvmtab")
            {
                pclose(fp);
                return true;
            }
        }
        pclose(fp);
        return false;
    }

    void GetLogicalData()
    {
        char buf[256];
        std::stringstream command("");
#if defined(aix)
        command << "mount";
#elif defined(linux)
        command << "df -TP";
#elif defined(sun)
        command << "df";
#elif defined(hpux)
        command << "bdf -l";
#endif
        FILE *fp = popen(command.str().c_str(), "r");

        if (0 != fp)
        {
#if ! defined(sun)
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header
#if defined(aix)
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header
#endif
#endif
            while ( ! feof(fp))
            {
                SCXCoreLib::SCXHandle<TestDisk> disk(0);
                memset(buf, 0, sizeof(buf));
                char *ret = fgets(buf, sizeof(buf), fp);
                CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);

#if !defined(sun)
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
#endif // !defined(sun)

#if defined(aix)
                if (parts.size() > 4)
                {
                    if (parts[2] == L"jfs2" || parts[2] == L"jfs")
                    {
                        disk = new TestDisk();
                    }
                    if (0 != disk)
                    {
                        disk->m_dev = parts[0];
                        logical.push_back(disk);
                    }
                }
#elif defined(hpux)
                if (parts.size() > 5)
                {
                    if (IsInMnttab(parts[0]) && (parts[0] != L"DevFS"))
                    {
                        disk = new TestDisk();
                    }
                    if (0 != disk)
                    {
                        disk->m_dev = parts[0];
                        logical.push_back(disk);
                    }
                }
#elif defined(linux)
                // Output from "df -TP" looks like this:
                //
                //   Filesystem    Type 1024-blocks      Used Available Capacity Mounted on
                //   /dev/hda1     ext3    23727064   1519256  21002536       7% /
                //   varrun       tmpfs      192932        44    192888       1% /var/run
                //   varlock      tmpfs      192932         4    192928       1% /var/lock
                //   udev         tmpfs      192932        52    192880       1% /dev
                //   devshm       tmpfs      192932         0    192932       0% /dev/shm
                //
                // We care about the filesystem and the type.

                if (line.find(L"loop=") == std::wstring::npos && line.find(L"/dev/loop") == std::wstring::npos && line.find(L".iso") == std::wstring::npos && parts.size() > 6)
                {
                    // Might need to add more file system types
                    if (parts[1] == L"btrfs" ||
                        parts[1] == L"ext2" ||
                        parts[1] == L"ext3" ||
                        parts[1] == L"ext4" ||
                        parts[1] == L"reiserfs" ||
                        parts[1] == L"vfat" ||
                        parts[1] == L"xfs" ||
                        parts[1] == L"ufs")
                    {
                        // on RHEL4/SLES9 anything that IsDMDevice gets ignored
                        // because it is not possible to detect what level of
                        // LVM support is/isn't available from the unit test
                        bool skipOnRhel4Sles9 = false;

#if ( ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) || \
      ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) ) )
                        {
                            static SCXSystemLib::SCXLVMUtils lvmUtils;
                            if (lvmUtils.IsDMDevice(parts[0]))
                            {
                                skipOnRhel4Sles9 = true;
                            }
                        }
#endif // if RHEL4 or SLES9

                        if (!skipOnRhel4Sles9)
                        {
                            CPPUNIT_ASSERT( IsInMnttab(parts[0]) );
                            disk = new TestDisk();
                        }
                    }
                    if (0 != disk)
                    {
                        disk->m_dev = parts[0];
                        logical.push_back(disk);
                    }
                }
#elif defined(sun)
                // Output from "df" looks like this:
                // /                  (/dev/dsk/c0t0d0s0 ): 8031696 blocks  1041163 files
                // /devices           (/devices          ):       0 blocks        0 files
                // /system/contract   (ctfs              ):       0 blocks 2147483556 files
                // /proc              (proc              ):       0 blocks    29891 files
                // /scxfiles          (scxfiles:/nfs     ):2474912072 blocks 309364009 files

                // Use "()" as separators; this protects us for very long device names, like:
                //
                // /export/home/jeffcof(rpool/export/home/jeffcof):27009314 blocks 27009314 files
                //
                // By using "()" separators, things work even without surrounding spaces

                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L"()");

                if (parts.size() >= 3)
                {
                    std::wstring id = parts[1];
                    if (IsInMnttab(id))
                    {
                        disk = new TestDisk();
                    }
                    if (0 != disk)
                    {
                        disk->m_dev = id;
                        disk->m_fs = GetMnttabFS(id);
                        logical.push_back(disk);
                    }
                }
#endif
            }
            pclose(fp);
        }
    }

    /*
      The data parsed in this function looks like this:

      Linux:
      4096

      HPUX:
      /                      (/dev/vg00/lvol3       ) :
      8192 file system block size            8192 fragment size
      131072 total blocks                     83242 total free blocks
      82600 allocated free blocks            23136 total i-nodes
      20796 total free i-nodes               20796 allocated free i-nodes
      1073741827 file system id                    vxfs file system type
      0x10 flags                             255 file system name length
      / file system specific string

      Solaris:
      /                  (/dev/dsk/c1t0d0s0 ):         8192 block size          1024 frag size
      8810220 total blocks    1665368 free blocks  1577266 available         529984 total files
      354391 free files      8650752 filesys id
      ufs fstype       0x00000004 flag             255 filename length
    */
    void GetBlockSize()
    {
        for(std::vector<SCXCoreLib::SCXHandle<TestDisk> >::iterator it = logical.begin();
            it != logical.end(); it++)
        {
            char buf[256];
            std::stringstream command("");
            SCXCoreLib::SCXHandle<TestDisk> disk = *it;
#if defined(aix)
            // All documentation points to the fact there is no command line command to get this but it is always this value.
            // So using minimum test effort in this case we just assume this value until proven wrong. Note that the PAL code
            // retrieves this value using system calls.
            disk->m_blockSize = 4096;
            continue;
#endif
            std::string mp = SCXCoreLib::StrToUTF8(disk->m_mountPoint);
#if defined(hpux) || defined(sun)
            command << "df -g " << mp;
#elif defined(linux)
            command << "stat -f --format=%s " << mp;
#endif
            FILE* fp = popen(command.str().c_str(), "r");
            if (0 != fp)
            {
                while ( ! feof(fp) )
                {
                    memset(buf, 0, sizeof(buf));
                    char *ret = fgets(buf, sizeof(buf), fp);
                    CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
#if defined(hpux)
                    std::string line = buf;
                    if (std::string::npos != line.find("file system block size"))
                    {
                        disk->m_blockSize = atoi(buf);
                    }
#elif defined(linux)
                    disk->m_blockSize = atoi(buf);
                    break;
#elif defined(sun)
                    std::string line = buf;
                    if (std::string::npos != line.find("block size"))
                    {
                        line = line.substr(line.rfind(":")+1, line.find("block size"));
                        disk->m_blockSize = atoi(line.c_str());
                    }
#endif
                }
            }
            pclose(fp);
        }
    }

    void GetDFData()
    {
        char buf[256];
        FILE *fp;
#if defined(sun) || defined(hpux)
        fp = popen("df -n", "r");
        if (0 != fp)
        {
            while ( ! feof(fp))
            {
                memset(buf, 0, sizeof(buf));
                char *ret = fgets(buf, sizeof(buf), fp);
                CPPUNIT_ASSERT_MESSAGE("There was an error reading from : df -n" , ret != NULL || feof(fp));
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);
                std::vector<std::wstring> parts;
#if defined(sun)
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
#elif defined(hpux)
                SCXCoreLib::StrTokenize(line, parts, L" \n\t():");
#endif
                if (parts.size() > 2)
                {
                    std::wstring id = parts[0];
                    id = id.substr(1, id.size());
                    SCXCoreLib::SCXHandle<TestDisk> disk = FindDisk(id);
                    if (disk != 0)
                    {
                        disk->m_fs = parts[2];
                    }
                }
            }
            pclose(fp);
        }
#endif

        std::stringstream command("");
#if defined(aix)
        command << "df -kP";
#elif defined(linux)
        command << "df -mTP";
#elif defined(sun)
        command << "df -k";
#elif defined(hpux)
        command << "bdf -l";
#else
#error "Plattform not supported"
#endif
        fp = popen(command.str().c_str(), "r");

        if (0 != fp)
        {
            CPPUNIT_ASSERT(fgets(buf, sizeof(buf), fp) != NULL); // Header

            while ( ! feof(fp))
            {
                memset(buf, 0, sizeof(buf));
                char *ret = fgets(buf, sizeof(buf), fp);
                CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
                if (parts.size() > 3)
                {
                    std::wstring id = parts[0];
                    SCXCoreLib::SCXHandle<TestDisk> disk = FindDisk(id);
//                    SCXCoreLib::SCXHandle<TestDisk> disk = FindDiskByMountPoint(parts[0]);
                    if (disk != 0)
                    {
#if defined(linux)
                        disk->m_fs = parts[1];
                        disk->m_mountPoint = parts[6];
                        disk->m_name = disk->m_mountPoint;
                        disk->m_mbUsed = SCXCoreLib::StrToULong(parts[2]) - SCXCoreLib::StrToULong(parts[4]);
                        disk->m_mbFree = SCXCoreLib::StrToULong(parts[4]);
#elif defined(aix) || defined(hpux) || defined (sun)
                        disk->m_mountPoint = parts[5];
                        disk->m_name = disk->m_mountPoint;
                        disk->m_mbUsed = SCXCoreLib::StrToULong(parts[1]) - SCXCoreLib::StrToULong(parts[3]);
                        disk->m_mbFree = SCXCoreLib::StrToULong(parts[3]);
                        disk->m_mbUsed = static_cast<scxulong>(ceil(disk->m_mbUsed/1024.0));
                        disk->m_mbFree = static_cast<scxulong>(ceil(disk->m_mbFree/1024.0));
                        if (disk->m_fs == L"zfs")
                        {
                            disk->m_mbUsed = SCXCoreLib::StrToULong(parts[2]) / 1024.0;
                        }
#endif
                    }
                }
            }
            pclose(fp);
        }
    }

    void GetMnttabData()
    {
        mnttab.clear();
        char buf[256];
        std::stringstream command("");
#if defined(aix)
        command << "mount";
#elif defined(linux)
        command << "cat /etc/mtab";
#elif defined(sun) || defined(hpux)
        command << "cat /etc/mnttab";
#else
#error "Plattform not supported"
#endif
        FILE *fp = popen(command.str().c_str(), "r");

        if (0 != fp)
        {
            while ( ! feof(fp))
            {
                memset(buf, 0, sizeof(buf));
                char *ret = fgets(buf, sizeof(buf), fp);
                CPPUNIT_ASSERT_MESSAGE("There was an error reading from : " + command.str() , ret != NULL || feof(fp));
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
                if (parts.size() > 3)
                {
                    if (m_deps.FileSystemIgnored(parts[2]) ||
                        m_deps.DeviceIgnored(parts[0]))
                    {
                        continue;
                    }
                    if (parts[0].find(L"/dev/md/") == 0)
                    {
                        m_metaDeviceFound = true;
                    }
#if defined(linux)

                    static SCXSystemLib::SCXLVMUtils lvmUtils;

                    if (lvmUtils.IsDMDevice(parts[0]))
                    {
                        try
                        {
                            // Try to convert the potential LVM device path into its matching
                            // device mapper (dm) device path.
                            std::wstring dmDevice = lvmUtils.GetDMDevice(parts[0]);

                            CPPUNIT_ASSERT_MESSAGE("GetDMDevice returned empty string", !dmDevice.empty());
                            dev2lv[dmDevice] = parts[0];
                        }
                        catch (SCXCoreLib::SCXException& e)
                        {
                            std::wstringstream out;

                            out << L"An exception occurred resolving the dm device that represents the LVM partition " << parts[0]
                                << L" : " << e.What();
                            SCXUNIT_WARNING(out.str());
                        }
                    }
                    // no else required; device was not an LVM device

#endif // if linux
                    mnttab.push_back(parts[0]);
                    mnttab_fs.push_back(parts[2]);
                }
            }
            pclose(fp);
        }
    }

    bool IsInMnttab(const std::wstring& id)
    {
        for (unsigned int i = 0; i < mnttab.size(); i++)
        {
            if (mnttab[i] == id)
            {
                return true;
            }
        }
        return false;
    }

    std::wstring GetMnttabFS(const std::wstring& id)
    {
        for (unsigned int i=0; i<mnttab.size(); i++)
        {
            if (mnttab[i] == id)
            {
                return mnttab_fs[i];
            }
        }
        return wstring(L"");
    }

    /*------------------------------------------------------------------------*/
    /**
       Check if the device listed has a mounted logical part.
       \param[in] dev device to be tested.
       \returns true if the device is in the mount table.

       Checks the mount table to see if the base device name is part of any of
       the mounted partitions.

       \date            2/22/2008

    */
    bool IsMountedDev(const std::wstring & dev )
    {
        std::wstring check(dev);
        std::vector<std::wstring>::const_iterator it = mnttab.begin();
        for(; it != mnttab.end(); ++it )
        {
            std::wstring::size_type pos = (*it).find(check);
            if (std::wstring::npos != pos)
            {
                return true;
            }
        }
        return false;
    }

    const std::wstring& GetLVFromDevice(const std::wstring& lv)
    {
        std::map<std::wstring,std::wstring>::const_iterator it = dev2lv.find(lv);
        if (it != dev2lv.end())
            return it->second;
        return lv;
    }

    bool m_metaDeviceFound;
    std::vector< SCXCoreLib::SCXHandle<TestDisk> > physical;
    std::vector< SCXCoreLib::SCXHandle<TestDisk> > logical;
    std::vector<std::wstring> mnttab;
    std::vector<std::wstring> mnttab_fs;
    std::map<std::wstring,std::wstring> dev2lv;
};

class SCXStatisticalDiskPalSanityTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStatisticalDiskPalSanityTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( DeviceIgnoredTest );
    CPPUNIT_TEST( RegressionTestForRefactoringIgnoredFileSystems_WI12506 );
    CPPUNIT_TEST( AllowedFilesystemShouldNotBeCaseSensitive );
    CPPUNIT_TEST( IgnoredFilesystemShouldNotBeCaseSensitive );
    CPPUNIT_TEST( LinkToPhysicalFilesystemShouldBeCaseInsensitive );
    CPPUNIT_TEST( PhysicalDeviceShouldNotExistIfDeviceAndMountPointAreSame );
    CPPUNIT_TEST( LinkToPhysicalExistsLogsWhenReturningFalseFirstTime );
    CPPUNIT_TEST( LinkToPhysicalExistsLogsTraceWhenReturningFalseSecondTime );
    CPPUNIT_TEST( TestFindByDevice );
#if defined(aix)
    CPPUNIT_TEST( Test_perfstat_disk_Regarding_Devices_Inside_Subdirectories_In_slashdev_Directory_RFC_483999 );
#endif // defined(aix)
    SCXUNIT_TEST_ATTRIBUTE(TestFindByDevice, SLOW);

    CPPUNIT_TEST( TestLogicalDiskCount );
    SCXUNIT_TEST_ATTRIBUTE(TestLogicalDiskCount, SLOW);

    CPPUNIT_TEST( TestPhysicalDiskCount );
    SCXUNIT_TEST_ATTRIBUTE(TestPhysicalDiskCount, SLOW);

    CPPUNIT_TEST( TestPhysicalDiskAttributes );
    SCXUNIT_TEST_ATTRIBUTE(TestPhysicalDiskAttributes, SLOW);

    CPPUNIT_TEST( TestLogicalDiskAttributes );
    SCXUNIT_TEST_ATTRIBUTE(TestLogicalDiskAttributes, SLOW);

    CPPUNIT_TEST( TestPhysicalDiskPerfCounters );               // Uses ExcerciseDisk(), can be slow
    SCXUNIT_TEST_ATTRIBUTE(TestPhysicalDiskPerfCounters, SLOW);

    CPPUNIT_TEST( TestLogicalDiskPerfCounters );                // Uses ExcerciseDisk(), can be slow
    SCXUNIT_TEST_ATTRIBUTE(TestLogicalDiskPerfCounters, SLOW);

    CPPUNIT_TEST( TestTotalPhysicalDisk );                      // Uses ExcerciseDisk(), can be slow
    SCXUNIT_TEST_ATTRIBUTE(TestTotalPhysicalDisk, SLOW);

    CPPUNIT_TEST( TestTotalLogicalDisk );                       // Uses ExcerciseDisk(), can be slow
    SCXUNIT_TEST_ATTRIBUTE(TestTotalLogicalDisk, SLOW);

    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskEnumeration> m_diskEnumPhysical;
    SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskEnumeration> m_diskEnumLogical;
    unsigned int m_try;
    unsigned int m_sectorSize;
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
    bool m_fBlockedHost;
#endif

    bool MeetsPrerequisites(std::wstring testName)
    {
#if defined(aix)
        /* No privileges needed on AIX */
        return true;
#elif defined(linux) | defined(hpux) | defined(sun)
        /* Most platforms need privileges to execute Update() method */
        if (0 == geteuid())
        {
            return true;
        }

        std::wstring warnText;

        warnText = L"Platform needs privileges to run SCXStatisticalDiskPalSanityTest::" + testName + L" test";

        SCXUNIT_WARNING(warnText);
        return false;
#else
#error Must implement method MeetsPrerequisites for this platform
#endif
    }

    // Method used to return one characher in the input as upper case.
    // Character changed is determined using random.
    // This is used to verify case insensitivy of filesystem names.
    std::wstring RandomizeCase(const std::wstring& in)
    {
        static bool isFirstTime = true;
        if (isFirstTime)
        {
            srand(static_cast<unsigned int>(time(0)));
            isFirstTime = false;
        }
        if (0 == in.length())
        {
            return in;
        }
        size_t idx = rand() % in.length();
        std::wstring out = in.substr(0,idx);
        out.append(SCXCoreLib::StrToUpper(in.substr(idx,1)));
        out.append(in.substr(idx+1));
        return out;
    }

    // This method is used to generate disk read and write events.
    // The m_try member is used to control how disk should be excercised.
    // m_try is incremented before method returns. Number returned is the number of
    // seconds disk was excercised.
    // The overall strategy is:
    // - Total excercise time is 10s for first 10 calls, then 20s for call 11-20 and so on.
    // - 1st, 11th, 21st, ... call uses a single find to excercise disk, 2nd, 12th, ... use 2 finds
    // - The last second of the test is sleeping (after find processes have been killed)
    //   in order to "stabilize" disk data minimizing differences between two tsatistics reads.
    void ExcerciseDisk()
    {
        static const unsigned int TEST_DELTA = 5;
        static const unsigned int TEST_INACTIVE = 1;
        static const char* LIBS[] = { "/","/tmp","/usr","/etc","/home","/var","/bin","/proc","/lib","/sbin" };
        pid_t pid[10];
        unsigned int used_delta = TEST_DELTA * (1+m_try/10);
        for (unsigned int i=0; i<10; i++)
        {
            pid[i] = (i<=(m_try%10))?TestDisks::ExerciseDiskProcess(LIBS[m_try%10]):0;
        }
        SCXCoreLib::SCXThread::Sleep((used_delta-TEST_INACTIVE)*1000); //make sure last second have few events
        for (size_t i=0; i<(sizeof(pid)/sizeof(*pid)); i++)
        {
            if (pid[i] != 0)
            {
                TestDisks::ExerciseDiskProcessKill(pid[i]);
            }
        }
        TestDisks::ExerciseDiskProcessCleanup();
        SCXCoreLib::SCXThread::Sleep(TEST_INACTIVE * 1000);
        pid_t sync_pid = fork();
        if (0 != sync_pid)
        { // parent process
            waitpid(sync_pid, 0, 0);
        }
        else if (0 == sync_pid)
        { // child process
            CPPUNIT_ASSERT(freopen("/dev/null","r",stdin)  != NULL);
            CPPUNIT_ASSERT(freopen("/dev/null","w",stdout) != NULL);
            CPPUNIT_ASSERT(freopen("/dev/null","w",stderr) != NULL);
                        // Looks like this can take a while on certain platforms/certain circumstances
            execlp("sync","sync", static_cast<char*>(0));
            exit(0);
        }
        ++m_try;
    }

public:
    SCXStatisticalDiskPalSanityTest() :
        m_diskEnumPhysical(0),
        m_diskEnumLogical(0),
        m_try(0),
        m_sectorSize(0)
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        /**
         * TODO: RESOLVE BLOCKING ISSUE WITH UNMOUNTED DRIVE ON scxrrhpr13 before
         * removing this RESOLVE_HOSTNAME_TESTFINDBYDEVICE code.
         */
        SCXCoreLib::NameResolver nr;
        std::wstring hostname = nr.GetHostname();
        m_fBlockedHost = (bool)((0 == hostname.find(L"scxrrhpr13")) || (0 == hostname.find(L"scxbld-sol10-05")));
#endif
    }
    void setUp(void)
    {
        m_diskEnumPhysical = 0;
        m_diskEnumLogical = 0;
        m_try = 0;
        m_sectorSize = 512;
    }

    void tearDown(void)
    {
        if (0 != m_diskEnumPhysical)
        {
            m_diskEnumPhysical->CleanUp();
            m_diskEnumPhysical = 0;
        }
        if (0 != m_diskEnumLogical)
        {
            m_diskEnumLogical->CleanUp();
            m_diskEnumLogical = 0;
        }
    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
        m_diskEnumLogical = new SCXSystemLib::StatisticalLogicalDiskEnumeration(deps);
        m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);

        CPPUNIT_ASSERT(m_diskEnumLogical->DumpString().find(L"StatisticalLogicalDiskEnumeration") != std::wstring::npos);
        CPPUNIT_ASSERT(m_diskEnumPhysical->DumpString().find(L"StatisticalPhysicalDiskEnumeration") != std::wstring::npos);
    }

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
    void DeviceIgnoredTest()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );

        // Solaris should ignore paths begining with /vol/dev/dsk/
        // Old Linux distribtions should ignore paths begining with /dev/mapper
        // nothing else should be ignored
        std::wstring solarisIgnore(L"/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s1");
        std::wstring oldDistrosIgnore(L"/dev/mapper/lvgGroup-lvVolume");
        std::wstring doNotIgnore(L"/dev/dsk/c1t0d0s0");
        CPPUNIT_ASSERT_MESSAGE("Device should not have been ignored", ! deps->DeviceIgnored(doNotIgnore));

#if defined (sun)
        CPPUNIT_ASSERT_MESSAGE("Device should have been ignored", deps->DeviceIgnored(solarisIgnore));
#else
        CPPUNIT_ASSERT_MESSAGE("Device should not have been ignored", ! deps->DeviceIgnored(solarisIgnore));
#endif

#if defined(linux)
        CPPUNIT_ASSERT_MESSAGE("Device should not have been ignored", ! deps->DeviceIgnored(oldDistrosIgnore));
#endif

        CPPUNIT_ASSERT_MESSAGE("Device should not have been ignored", ! deps->DeviceIgnored(doNotIgnore));
    }

    void RegressionTestForRefactoringIgnoredFileSystems_WI12506()
    {
        /*
         * This is the same list as in old FileSystemIgnored implementation
         * with all the ifdefs removed.
         */
        static std::wstring FS[] = {
            L"procfs",  L"nfs",     L"nfs3",    L"cachefs",
            L"udfs",    L"cifs",    L"nfs4",    L"autofs",  L"namefs",
            L"tmpfs",   L"nfs",     L"cachefs", L"specfs",
            L"procfs",  L"sockfs",  L"fifofs",  L"autofs",  L"lofs",
            L"devfs",   L"ctfs",    L"proc",    L"mntfs",   L"objfs",
            L"fd",      L"sharefs",
            L"sysfs",       L"rootfs",      L"bdev",        L"proc",        L"debugfs",
            L"securityfs",  L"sockfs",      L"pipefs",      L"futexfs",     L"tmpfs",
            L"inotifyfs",   L"eventpollfs", L"devpts",      L"ramfs",       L"hugetlbfs",
            L"mqueue",      L"vmware-hgfs", L"binfmt_misc", L"cifs",
            L"vmblock",     L"vmhgfs",      L"rpc_pipefs",  L"nfs",         L"usbfs",
            L"subfs",   L"fusectl",
#if defined(linux)
            L"udev", L"devtmpfs", L"tracefs",
#endif
            L"nfs",     L"DevFS",   L"autofs",
            L"cachefs", L"ffs", L"lofs",    L"nfs3",    L"procfs",
#if 0 // these were ignored for OM but are needed for CM Xplat
            L"cdrfs", L"cdfs", L"hsfs", L"iso9660",
#endif
            L"cifs",    L"pipefs", L"" };

        SCXSystemLib::DiskDependDefault deps;

        for (int i = 0; FS[i].size() != 0; i++)
        {
            std::string msg = "File system should be ignored: ";
            msg.append(SCXCoreLib::StrToUTF8(FS[i]));
            CPPUNIT_ASSERT_MESSAGE(msg, deps.FileSystemIgnored(FS[i]));
        }
    }

    void AllowedFilesystemShouldNotBeCaseSensitive()
    {
        static std::wstring FS[] = {
            L"jfs2", L"reiserfs", L"ufs", L"vxfs",
#if defined(sun)
            L"zfs",
#endif
            L"" };
        SCXSystemLib::DiskDependDefault deps;

        for (int i = 0; FS[i].size() != 0; i++)
        {
            std::wstring fs = RandomizeCase(FS[i]);
            std::string msg = "File system should NOT be ignored: ";
            msg.append(SCXCoreLib::StrToUTF8(fs));
            CPPUNIT_ASSERT_MESSAGE(msg, ! deps.FileSystemIgnored(fs));
        }
    }

    void LinkToPhysicalFilesystemShouldBeCaseInsensitive()
    {
        static std::wstring FS[] = {
#if defined(sun)
            L"zfs",
#endif
            L"" };
        SCXSystemLib::DiskDependDefault deps;

        for (int i = 0; FS[i].size() != 0; i++)
        {
            std::wstring fs = RandomizeCase(FS[i]);
            std::string msg = "File system used: ";
            msg.append(SCXCoreLib::StrToUTF8(fs));
            CPPUNIT_ASSERT_MESSAGE(msg, ! deps.LinkToPhysicalExists(fs, L"diff", L"erent"));
        }
    }

    void PhysicalDeviceShouldNotExistIfDeviceAndMountPointAreSame()
    {
        SCXSystemLib::DiskDependDefault deps;
        std::wstring path = L"/";
        CPPUNIT_ASSERT_MESSAGE("When both the device path and the mount point are the same, we should not be able to find the physical device.",
                               ! deps.LinkToPhysicalExists(L"something", path, path));
    }

    void LinkToPhysicalExistsLogsWhenReturningFalseFirstTime()
    {
        TestLogFrameworkHelper logframework;
        SCXSystemLib::DiskDependDefault deps(logframework.GetHandle());

        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        deps.LinkToPhysicalExists(L"something", L"/", L"/", suppressor);

        SCXCoreLib::SCXLogItem i = logframework.GetLastLogItem();
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eWarning, i.GetSeverity());
        std::wstring expected(L"No link exists between the logical device \"/\" at mount point \"/\" with filesystem \"something\". Some statistics will be unavailable.");
        CPPUNIT_ASSERT_MESSAGE("Expected: \"" + StrToUTF8(expected) + "\"\nReceived: \"" + StrToUTF8(i.GetMessage()) + "\"",
                               expected == i.GetMessage());
    }

    void LinkToPhysicalExistsLogsTraceWhenReturningFalseSecondTime()
    {
        TestLogFrameworkHelper logframework;
        SCXSystemLib::DiskDependDefault deps(logframework.GetHandle());

        SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        deps.LinkToPhysicalExists(L"something", L"/", L"/", suppressor);
        deps.LinkToPhysicalExists(L"something", L"/", L"/", suppressor);

        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::eTrace, logframework.GetLastLogItem().GetSeverity());
    }

    void IgnoredFilesystemShouldNotBeCaseSensitive()
    {
        static std::wstring FS[] = {
            L"autofs",
            L"bdev", L"binfmt_misc",
            L"cachefs", L"cdfs", L"cdrfs", L"cifs", L"cgroup", L"configfs", L"ctfs",
            L"debugfs", L"devfs", L"devpts",
#if defined(sun) && ((PF_MAJOR == 5 && PF_MINOR >= 11) || (PF_MAJOR > 5))
            // On Solaris 11, /dev is a pseudo file system.
            // Always ignore to eliminate inode detection, etc
            L"dev",
#endif
#if defined(linux)
            L"devtmpfs", L"efivarfs", L"fuse.lxcfs",
#endif
            L"eventpollfs",
            L"fd", L"ffs", L"fifofs", L"fusectl", L"futexfs",
            L"hugetlbfs", L"hsfs",
            L"inotifyfs", L"iso9660",
            L"lofs",
            L"mntfs", L"mqueue", L"mvfs",
            L"namefs",
            // WI 24875: Ignore file system type "none" (these are NFS-mounted on the local system)
            L"none",
            L"objfs",
            L"pipefs", L"proc", L"procfs", L"pstore",
            L"ramfs", L"rootfs", L"rpc_pipefs",
            L"securityfs", L"selinuxfs", L"sharefs", L"sockfs", L"specfs", L"subfs", L"sysfs",
            L"tmpfs",
            L"udfs", L"usbfs",
#if defined(linux)
            L"udev", L"tracefs",
#endif
            L"vmblock", L"vmhgfs", L"vmware-hgfs",
#if ! defined(sun)
            L"zfs",
#endif
            L"" };

        SCXSystemLib::DiskDependDefault deps;

        for (int i = 0; FS[i].size() != 0; i++)
        {
            std::wstring fs = RandomizeCase(FS[i]);
            std::string msg = "File system should be ignored: ";
            msg.append(SCXCoreLib::StrToUTF8(fs));
            CPPUNIT_ASSERT_MESSAGE(msg, deps.FileSystemIgnored(fs));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Create a message containing expected devices and devices found in the
       enumerated list.

       \param disks1 TestDisk instance.
       \returns The error message.

       \date            2/22/2008
    */
    std::wstring GetExpectFoundPhysical(TestDisks & disks1)
    {
        std::wostringstream ss;
        ss << L"Expected: [ ";
        std::ostream_iterator<SCXCoreLib::SCXHandle<TestDisk>, wchar_t> osi(ss, L" ");
        copy(disks1.physical.begin(), disks1.physical.end(), osi);
        ss << L"] " << endl << L"- Found    : [ ";
        for (size_t j = 0; j<m_diskEnumPhysical->Size(); j++)
        {
            std::wstring dev;
            m_diskEnumPhysical->GetInstance(j)->GetDiskDeviceID(dev);
            ss << dev << " ";
        }
        ss << L"]";
        return ss.str();
    }

    void TestFindByDevice()
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        if (m_fBlockedHost)
            return;
#endif
        if ( ! MeetsPrerequisites(L"TestFindByDevice"))
            return;
        if ( ! HasPhysicalDisks(L"TestFindByDevice"))
            return;

        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);
            m_diskEnumPhysical->InitInstances();
            TestDisks disks1;
            TestDisks disks2;
            for(size_t i = 0; i<disks1.physical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td1 = disks1.physical[i];
                SCXCoreLib::SCXHandle<TestDisk> td2 = disks2.FindDisk(td1->m_dev);
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = m_diskEnumPhysical->FindDiskByDevice(td1->m_dev);
                CPPUNIT_ASSERT(0 != td2);
                if (0 == disk)
                {
                    for (size_t j = 0; j<m_diskEnumPhysical->Size(); j++)
                    {
                        std::wstring dev;
                        m_diskEnumPhysical->GetInstance(j)->GetDiskDeviceID(dev);
                    }
                }
                CPPUNIT_ASSERT_MESSAGE(StrToUTF8(GetExpectFoundPhysical(disks1)), 0 != disk);
                CPPUNIT_ASSERT(td1->m_dev == td2->m_dev);
                std::wstring diskDevice;
                CPPUNIT_ASSERT(disk->GetDiskDeviceID(diskDevice));
                SCXCoreLib::SCXFilePath path(td2->m_dev);
                CPPUNIT_ASSERT(path.GetFilename() == diskDevice);
                // Test that the same disk is returned for "short" device name.
                CPPUNIT_ASSERT(disk == m_diskEnumPhysical->FindDiskByDevice(diskDevice));
                // Test that name and device returns the same instance
                CPPUNIT_ASSERT(disk == m_diskEnumPhysical->GetInstance(diskDevice));
            }
            CPPUNIT_ASSERT(m_diskEnumPhysical->GetTotalInstance() == m_diskEnumPhysical->FindDiskByDevice(L"_Total"));
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    void TestLogicalDiskCount(void)
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if ( ! MeetsPrerequisites(L"TestLogicalDiskCount"))
            return;
#endif
        if ( ! HasPhysicalDisks(L"TestLogicalDiskCount"))
            return;

        size_t numDisksEnumerated = 0;
        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumLogical = new SCXSystemLib::StatisticalLogicalDiskEnumeration(deps);
            m_diskEnumLogical->InitInstances();
            CPPUNIT_ASSERT(0 != m_diskEnumLogical->GetTotalInstance());

            TestDisks disks;

            numDisksEnumerated = m_diskEnumLogical->Size();

            if (disks.logical.size() != m_diskEnumLogical->Size())
            {
                // If we can't read some of the device files we will not be
                // able to find information about volume groups
                if ( ! MeetsPrerequisites(L"TestLogicalDiskCount"))
                    return;

                // Helps identify why test fails. Typically because of new, never seen before, file systems.
                std::wcout << std::endl << L"Control:" << std::endl;
                for(size_t i = 0; i<disks.logical.size(); i++)
                {
                    std::wcout << disks.logical[i]->m_dev << std::endl;
                }
                std::wcout << L"diskEnum:" << std::endl;
                for(size_t i = 0; i<m_diskEnumLogical->Size(); i++)
                {
                    std::wstring dev;
                    std::wstring name;
                    std::wstring message = L"";

                    m_diskEnumLogical->GetInstance(i)->GetDiskDeviceID(dev);
                    m_diskEnumLogical->GetInstance(i)->GetDiskName(name);

#if defined(linux) &&                                          \
           ( ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) || \
             ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) ) )
                    // without the full device path, the only way to guess if
                    // the device is LVM is to assume only LVM devices have a
                    // dash ('-') in the name
                    if (dev.find('-') != std::wstring::npos)
                    {
                        message = L"(apparent LVM partitions are ignored on RHEL4 and SLES9 systems)";
                        --numDisksEnumerated;
                    }
#endif

                    std::wcout << dev << L" " << name << L" " << message << std::endl;
                }
#if defined(sun)
                if (disks.logical.size() == 0)
                { // Fail with a more descriptive message on solaris if we thinkthe reason is vopstats (WI 3490 & 3704 for more info)
                    SCXUNIT_WARNING(L"Test class did not find any partitions - probably because they have their data under \"vopstats\"-entries (WIs: 3490, 3704, 4631) - PAL handles this correctly");
                    return;
                }
#endif
            }

#if defined(linux) &&                                          \
           ( ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) || \
             ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) ) )
            SCXUNIT_WARNING(L"test: SCXStatisticalDiskPalSanityTest::TestLogicalDiskCount : the presence of LVM on RHEL4 and SLES9 systems can effect this unit test outcome.");
#endif

            CPPUNIT_ASSERT_EQUAL(disks.logical.size(), numDisksEnumerated);
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    void TestPhysicalDiskCount(void)
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        if (m_fBlockedHost)
            return;
#endif
        if ( ! MeetsPrerequisites(L"TestPhysicalDiskCount"))
            return;
        if ( ! HasPhysicalDisks(L"TestPhysicalDiskCount"))
            return;

        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);
            m_diskEnumPhysical->InitInstances();
            CPPUNIT_ASSERT(0 != m_diskEnumPhysical->GetTotalInstance());

            TestDisks disks;
            if (disks.physical.size() != m_diskEnumPhysical->Size())
            { // Helps identify why test fails. Typically because of new, never seen before, file systems.
                for(size_t i = 0; i<m_diskEnumPhysical->Size(); i++)
                {
                    std::wstring dev,name;
                    m_diskEnumPhysical->GetInstance(i)->GetDiskDeviceID(dev);
                    m_diskEnumPhysical->GetInstance(i)->GetDiskName(name);
                }

                if (disks.m_metaDeviceFound && disks.physical.size() < m_diskEnumPhysical->Size())
                {
                    SCXUNIT_WARNING(L"PAL finds more physical disks than test code - Probably because meta devices are in use.");
                    return;
                }
            }
            CPPUNIT_ASSERT_EQUAL(disks.physical.size(), m_diskEnumPhysical->Size());
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    void TestPhysicalDiskAttributes()
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        if (m_fBlockedHost)
            return;
#endif
        if ( ! MeetsPrerequisites(L"TestPhysicalDiskAttributes"))
            return;
        if ( ! HasPhysicalDisks(L"TestPhysicalDiskAttributes"))
            return;

        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);
            m_diskEnumPhysical->InitInstances();
            m_diskEnumPhysical->Update(true);
            TestDisks disks;
            for(size_t i = 0; i<disks.physical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td = disks.physical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = m_diskEnumPhysical->FindDiskByDevice(td->m_dev);
                CPPUNIT_ASSERT_MESSAGE(StrToUTF8(GetExpectFoundPhysical(disks)), 0 != disk);

                // Disk size
                scxulong mbFree, mbUsed;
                CPPUNIT_ASSERT( ! disk->GetDiskSize(mbUsed, mbFree));

                // Block size
                scxulong blocksize;
                CPPUNIT_ASSERT( ! disk->GetBlockSize(blocksize));

                // Healthy
                bool healthy;
                CPPUNIT_ASSERT(disk->GetHealthState(healthy));
                CPPUNIT_ASSERT(healthy);
            }
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    void TestLogicalDiskAttributes()
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if ( ! MeetsPrerequisites(L"TestLogicalDiskAttributes"))
            return;
#endif
        if ( ! HasPhysicalDisks(L"TestLogicalDiskAttributes"))
            return;

        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumLogical = new SCXSystemLib::StatisticalLogicalDiskEnumeration(deps);
            m_diskEnumLogical->InitInstances();
            m_diskEnumLogical->Update(true);
            TestDisks disks;
            for(size_t i = 0; i<disks.logical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td = disks.logical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> disk = m_diskEnumLogical->FindDiskByDevice(td->m_dev, true);
                CPPUNIT_ASSERT(0 != disk);

                // Disk size
                scxulong mbFree, mbUsed;
                CPPUNIT_ASSERT(disk->GetDiskSize(mbUsed, mbFree));
                if (L"zfs" != td->m_fs)
                {
#if defined(hpux)
                    const int delta = 2048;
#else
                    const int delta = 10;
#endif
                    CPPUNIT_ASSERT_DOUBLES_EQUAL(static_cast<double>(td->m_mbUsed), static_cast<double>(mbUsed), delta); // Other activities on the machine might affect the test.
                    CPPUNIT_ASSERT_DOUBLES_EQUAL(static_cast<double>(td->m_mbFree), static_cast<double>(mbFree), delta); // Other activities on the machine might affect the test.
                }

                // Block size
                scxulong blocksize;
                CPPUNIT_ASSERT(disk->GetBlockSize(blocksize));
                CPPUNIT_ASSERT_EQUAL(td->m_blockSize, blocksize);

                // Healthy
                bool healthy;
                CPPUNIT_ASSERT(disk->GetHealthState(healthy));
                CPPUNIT_ASSERT(healthy);

                // Mounting Point
                std::wstring mp;
                CPPUNIT_ASSERT(disk->GetDiskName(mp)); // Logical disk should have mount point as name.
                CPPUNIT_ASSERT(td->m_mountPoint == mp);
            }
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    // PerfCounters are tested using the following strategy:
    // - Take a snap shot of disk values.
    // - Excercise disk.
    // - Verify that counters that should increase do that.
    // BVT tests are used to test they actually return good enough values.
    void TestPhysicalDiskPerfCounters()
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        if (m_fBlockedHost)
            return;
#endif
        if ( ! MeetsPrerequisites(L"TestPhysicalDiskPerfCounters"))
            return;
        if ( ! HasPhysicalDisks(L"TestPhysicalDiskPerfCounters"))
            return;

        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);
            m_diskEnumPhysical->InitInstances();
            try
            {
                m_diskEnumPhysical->SampleDisks();
                m_diskEnumPhysical->Update(true);
            }
            catch (SCXCoreLib::SCXException& e)
            {
                std::wcout << std::endl << e.What() << std::endl << e.Where() << std::endl;
                throw;
            }

            // Save initial values.
            TestDisks disks_pre;
            for(size_t i = 0; i<disks_pre.physical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td_pre = disks_pre.physical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = m_diskEnumPhysical->FindDiskByDevice(td_pre->m_dev);
                if (0 == disk)
                {
                    std::wostringstream ss;
                    ss << "Did not find disk " << td_pre->m_dev << " in enumerated instances: [ ";

                    for (size_t j = 0; j<m_diskEnumPhysical->Size(); j++)
                    {
                        std::wstring dev;
                        m_diskEnumPhysical->GetInstance(j)->GetDiskDeviceID(dev);
                        ss << dev << " ";
                    }

                    //copy(disks_pre.physical.begin(), disks_pre.physical.end(), std::ostream_iterator<TestDisk*,wchar_t>(ss, L" "));

                    ss << "]";

                    CPPUNIT_ASSERT_MESSAGE(StrToUTF8(ss.str()), 0 != disk);
                }
                CPPUNIT_ASSERT(disk->GetLastMetrics(td_pre->read.num,td_pre->write.num,td_pre->read.bytes,td_pre->write.bytes,td_pre->read.ms,td_pre->write.ms));
            }

            ExcerciseDisk();
            m_diskEnumPhysical->SampleDisks();
            m_diskEnumPhysical->Update(true);

            // Save new values.
            TestDisks disks_post;
            for(size_t i = 0; i<disks_post.physical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td_post = disks_post.physical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = m_diskEnumPhysical->FindDiskByDevice(td_post->m_dev);
                CPPUNIT_ASSERT_MESSAGE("Did not find disk in enumerated instances", 0 != disk);
                CPPUNIT_ASSERT(disk->GetLastMetrics(td_post->read.num,td_post->write.num,td_post->read.bytes,td_post->write.bytes,td_post->read.ms,td_post->write.ms));
            }

            for(size_t i = 0; i<disks_pre.physical.size(); i++)
            {
                scxulong nR,nW,bR,bW,tR,tW;
                SCXCoreLib::SCXHandle<TestDisk> td_pre = disks_pre.physical[i];
                SCXCoreLib::SCXHandle<TestDisk> td_post = disks_post.FindDisk(td_pre->m_dev);
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = m_diskEnumPhysical->FindDiskByDevice(td_pre->m_dev);

                CPPUNIT_ASSERT(disk->GetLastMetrics(nR,nW,bR,bW,tR,tW));

                CPPUNIT_ASSERT(nR >= td_pre->read.num);
                CPPUNIT_ASSERT(nW >= td_pre->write.num);
                CPPUNIT_ASSERT(bR >= td_pre->read.bytes);
                CPPUNIT_ASSERT(bW >= td_pre->write.bytes);
                CPPUNIT_ASSERT(tR >= td_pre->read.ms);
                CPPUNIT_ASSERT(tW >= td_pre->write.ms);

                // Reads/writes/transfers per second
                scxulong readsPerSec,writesPerSec,transfersPerSec;
#if defined(linux) || defined(sun)
                CPPUNIT_ASSERT(disk->GetReadsPerSecond(readsPerSec));
                CPPUNIT_ASSERT(disk->GetWritesPerSecond(writesPerSec));
                CPPUNIT_ASSERT(disk->GetTransfersPerSecond(transfersPerSec));
                CPPUNIT_ASSERT_EQUAL((td_post->read.num - td_pre->read.num)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, readsPerSec);
                CPPUNIT_ASSERT_EQUAL((td_post->write.num - td_pre->write.num)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, writesPerSec);
                SCXUNIT_ASSERT_BETWEEN(readsPerSec + writesPerSec, MIN(0, transfersPerSec-1), transfersPerSec);
#elif defined(hpux)
                CPPUNIT_ASSERT( ! disk->GetReadsPerSecond(readsPerSec));
                CPPUNIT_ASSERT( ! disk->GetWritesPerSecond(writesPerSec));
                CPPUNIT_ASSERT(disk->GetTransfersPerSecond(transfersPerSec));
                CPPUNIT_ASSERT_EQUAL(((td_post->read.num - td_pre->read.num) + (td_post->write.num - td_pre->write.num))/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, transfersPerSec);
#endif

                // Bytes per second
                scxulong rBytesPerSec, wBytesPerSec, tBytesPerSec;
                CPPUNIT_ASSERT(disk->GetBytesPerSecondTotal(tBytesPerSec));
#if defined(linux) || defined(sun)
                CPPUNIT_ASSERT(disk->GetBytesPerSecond(rBytesPerSec, wBytesPerSec));
                CPPUNIT_ASSERT_EQUAL((td_post->read.bytes - td_pre->read.bytes)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, rBytesPerSec);
                CPPUNIT_ASSERT_EQUAL((td_post->write.bytes - td_pre->write.bytes)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, wBytesPerSec);
                SCXUNIT_ASSERT_BETWEEN(rBytesPerSec + wBytesPerSec, MIN(0, tBytesPerSec-1), tBytesPerSec);
#elif defined(hpux)
                CPPUNIT_ASSERT( ! disk->GetBytesPerSecond(rBytesPerSec, wBytesPerSec));
                CPPUNIT_ASSERT_EQUAL(((td_post->read.bytes - td_pre->read.bytes) + (td_post->write.bytes - td_pre->write.bytes))/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, tBytesPerSec);
#endif

                // Times reading/writing/total
                scxulong readP, writeP, totalP;
                CPPUNIT_ASSERT( ! disk->GetIOPercentage(readP, writeP));
#if defined(linux) || defined(hpux)
                CPPUNIT_ASSERT( ! disk->GetIOPercentageTotal(totalP));
#elif defined(sun)
                CPPUNIT_ASSERT(disk->GetIOPercentageTotal(totalP));
                CPPUNIT_ASSERT(totalP <= 100); // totalP unsigned so comparision with zero not needed (and warned by compiler)
#endif

                // Disk operations/second
                double rTime, wTime, tTime, rTest, wTest, tTest, qLength;
                scxulong rOps, wrOps, rOpsT, wrOpsT;
                rOps = td_post->read.num - td_pre->read.num;
                wrOps = td_post->write.num - td_pre->write.num;
                rOpsT = td_post->read.ms - td_pre->read.ms;
                wrOpsT = td_post->write.ms - td_pre->write.ms;
                rTest = rOps?(static_cast<double>(rOpsT)/static_cast<double>(rOps)/1000.0):0;
                wTest = wrOps?(static_cast<double>(wrOpsT)/static_cast<double>(wrOps)/1000.0):0;
                tTest = (rOps||wrOps)?((static_cast<double>(rOpsT+wrOpsT))/(static_cast<double>(rOps+wrOps))/1000.0):0;

                CPPUNIT_ASSERT(disk->GetDiskQueueLength(qLength));

#if defined(linux)
                CPPUNIT_ASSERT(disk->GetIOTimes(rTime, wTime));
                CPPUNIT_ASSERT(disk->GetIOTimesTotal(tTime));
                SCXUNIT_ASSERT_BETWEEN(rTime, rTest*0.99, rTest*1.01);
                SCXUNIT_ASSERT_BETWEEN(wTime, wTest*0.99, wTest*1.01);
                SCXUNIT_ASSERT_BETWEEN(tTime, tTest*0.99, tTest*1.01);
#elif defined(sun) || defined(hpux)
                CPPUNIT_ASSERT( ! disk->GetIOTimes(rTime,wTime));
                CPPUNIT_ASSERT(disk->GetIOTimesTotal(tTime));
                CPPUNIT_ASSERT_EQUAL(tTest,tTime);
#endif
            }
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    // This method uses same strategy as TestPhysicalDiskPerfCounters. Se comment for that method.
    void TestLogicalDiskPerfCounters()
    {
        // This test needs root access on RHEL
#if defined(PF_DISTRO_REDHAT)
        if ( ! MeetsPrerequisites(L"TestLogicalDiskPerfCounters"))
            return;
#endif
        if ( ! HasPhysicalDisks(L"TestLogicalDiskPerfCounters"))
            return;

#if defined(sun)
        TestDisks disks;
        if (0 == disks.logical.size())
        {
            SCXUNIT_WARNING(L"Test class did not find any partitions - probably because they have their data under \"vopstats\"-entries (WIs: 3490, 3704, 4631) - PAL handles this correctly");
            return;
        }
#endif
        try
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
            m_diskEnumLogical = new SCXSystemLib::StatisticalLogicalDiskEnumeration(deps);
            m_diskEnumLogical->InitInstances();
            try
            {
                m_diskEnumLogical->SampleDisks();
                m_diskEnumLogical->Update(true);
            }
            catch (SCXCoreLib::SCXException& e)
            {
                std::wcout << std::endl << e.What() << std::endl << e.Where() << std::endl;
                throw;
            }

#if defined(sun)
            bool gotAnyMetrics = false;
#endif

            // Save initial values.
            TestDisks disks_pre;
            for(size_t i = 0; i<disks_pre.logical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td_pre = disks_pre.logical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> disk = m_diskEnumLogical->FindDiskByDevice(td_pre->m_dev,true);
                CPPUNIT_ASSERT_MESSAGE("Did not find disk in enumerated instances", 0 != disk);
                bool gotLastMetrics = disk->GetLastMetrics(td_pre->read.num,td_pre->write.num,td_pre->read.bytes,td_pre->write.bytes,td_pre->read.ms,td_pre->write.ms);
#if defined(sun)
                if ( ! gotLastMetrics)
                { // All metrics not available with zfs or "vopstats" on solaris so we can't assert.
                    continue;
                }
                else
                {
                    gotAnyMetrics = true;
                }
#endif
                CPPUNIT_ASSERT_MESSAGE("Unable to get last metrics.", gotLastMetrics);
            }

#if defined(sun)
            if ( ! gotAnyMetrics)
            {
                SCXUNIT_WARNING(L"Unable to verify any logical disk performance counters");
                return;
            }
#endif

            ExcerciseDisk();
            m_diskEnumLogical->SampleDisks();
            m_diskEnumLogical->Update(true);

            // Save new values.
            TestDisks disks_post;
            for(size_t i = 0; i<disks_post.logical.size(); i++)
            {
                SCXCoreLib::SCXHandle<TestDisk> td_post = disks_post.logical[i];
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> disk = m_diskEnumLogical->FindDiskByDevice(td_post->m_dev,true);
                CPPUNIT_ASSERT_MESSAGE("Did not find disk in enumerated instances", 0 != disk);
                bool gotLastMetrics = disk->GetLastMetrics(td_post->read.num,td_post->write.num,td_post->read.bytes,td_post->write.bytes,td_post->read.ms,td_post->write.ms);
#if defined(sun)
                if ( ! gotLastMetrics)
                { // All metrics not available with zfs or "vopstats" on solaris so we can't assert.
                    continue;
                }
#endif
                CPPUNIT_ASSERT_MESSAGE("Unable to get last metrics.", gotLastMetrics);

            }

            for(size_t i = 0; i<disks_pre.logical.size(); i++)
            {
                scxulong nR,nW,bR,bW,tR,tW;
                SCXCoreLib::SCXHandle<TestDisk> td_pre = disks_pre.logical[i];
                SCXCoreLib::SCXHandle<TestDisk> td_post = disks_post.FindDisk(td_pre->m_dev);
                SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> disk = m_diskEnumLogical->FindDiskByDevice(td_pre->m_dev, true);

                CPPUNIT_ASSERT(0 != disk);
                bool gotLastMetrics = disk->GetLastMetrics(nR,nW,bR,bW,tR,tW);
#if defined(sun)
                if ( ! gotLastMetrics)
                { // All metrics not available with zfs or "vopstats" on solaris so we can't assert.
                    continue;
                }
#endif
                CPPUNIT_ASSERT_MESSAGE("Unable to get last metrics.", gotLastMetrics);
                CPPUNIT_ASSERT(nR >= td_pre->read.num);
                CPPUNIT_ASSERT(nW >= td_pre->write.num);
                CPPUNIT_ASSERT(bR >= td_pre->read.bytes);
                CPPUNIT_ASSERT(bW >= td_pre->write.bytes);
                CPPUNIT_ASSERT(tR >= td_pre->read.ms);
                CPPUNIT_ASSERT(tW >= td_pre->write.ms);

                // Reads/writes/transfers per second
                scxulong readsPerSec,writesPerSec,transfersPerSec;
#if defined(aix)
                CPPUNIT_ASSERT( ! disk->GetReadsPerSecond(readsPerSec));
                CPPUNIT_ASSERT( ! disk->GetWritesPerSecond(writesPerSec));
                CPPUNIT_ASSERT( ! disk->GetTransfersPerSecond(transfersPerSec));
#else
                CPPUNIT_ASSERT(disk->GetReadsPerSecond(readsPerSec));
                CPPUNIT_ASSERT(disk->GetWritesPerSecond(writesPerSec));
                CPPUNIT_ASSERT(disk->GetTransfersPerSecond(transfersPerSec));
#endif
                CPPUNIT_ASSERT_EQUAL((td_post->read.num - td_pre->read.num)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, readsPerSec);
                CPPUNIT_ASSERT_EQUAL((td_post->write.num - td_pre->write.num)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, writesPerSec);
                SCXUNIT_ASSERT_BETWEEN(readsPerSec + writesPerSec, MIN(0, transfersPerSec-1), transfersPerSec);

                // Bytes per second
                scxulong rBytesPerSec, wBytesPerSec, tBytesPerSec;
#if defined(aix)
                CPPUNIT_ASSERT( ! disk->GetBytesPerSecondTotal(tBytesPerSec));
                CPPUNIT_ASSERT( ! disk->GetBytesPerSecond(rBytesPerSec, wBytesPerSec));
#else
                CPPUNIT_ASSERT(disk->GetBytesPerSecondTotal(tBytesPerSec));
                CPPUNIT_ASSERT(disk->GetBytesPerSecond(rBytesPerSec, wBytesPerSec));
#endif
                CPPUNIT_ASSERT_EQUAL((td_post->read.bytes - td_pre->read.bytes)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, rBytesPerSec);
                CPPUNIT_ASSERT_EQUAL((td_post->write.bytes - td_pre->write.bytes)/SCXSystemLib::DISK_SECONDS_PER_SAMPLE, wBytesPerSec);
                SCXUNIT_ASSERT_BETWEEN(rBytesPerSec + wBytesPerSec, MIN(0, tBytesPerSec-1), tBytesPerSec);

                // Times reading/writing/total
                scxulong readP, writeP, totalP;
                CPPUNIT_ASSERT( ! disk->GetIOPercentage(readP, writeP));
#if defined(linux) || defined(hpux)
                CPPUNIT_ASSERT( ! disk->GetIOPercentageTotal(totalP));
#elif defined(sun)
                CPPUNIT_ASSERT(disk->GetIOPercentageTotal(totalP));
                CPPUNIT_ASSERT(totalP <= 100); // totalP unsigned so comparision with zero not needed (and warned by compiler)
#endif

                // Disk operations/second
                double rTime, wTime, tTime;
                CPPUNIT_ASSERT( ! disk->GetIOTimes(rTime, wTime));
#if defined(linux)
                CPPUNIT_ASSERT( ! disk->GetIOTimesTotal(tTime));
#elif defined(sun) || defined(hpux)
                scxulong rOps, wrOps, rOpsT, wrOpsT;
                rOps = td_post->read.num - td_pre->read.num;
                wrOps = td_post->write.num - td_pre->write.num;
                rOpsT = td_post->read.ms - td_pre->read.ms;
                wrOpsT = td_post->write.ms - td_pre->write.ms;

                double tTest;
                tTest = (rOps||wrOps)?((static_cast<double>(rOpsT+wrOpsT))/(static_cast<double>(rOps+wrOps))/1000.0):0;

                CPPUNIT_ASSERT(disk->GetIOTimesTotal(tTime));
                CPPUNIT_ASSERT_EQUAL(tTest,tTime);
#endif

                double qLength;
#if defined(sun)
                CPPUNIT_ASSERT(disk->GetDiskQueueLength(qLength));
                CPPUNIT_ASSERT(0 <= qLength);
#else
                CPPUNIT_ASSERT( ! disk->GetDiskQueueLength(qLength));
#endif
            }
        }
        catch(SCXException & e)
        {
            std::wostringstream ss;
            ss << e.What() << std::endl << e.Where() << std::endl;
            CPPUNIT_FAIL( StrToUTF8(ss.str()) );
        }
    }

    void TestTotalPhysicalDisk()
    {
#if defined(RESOLVE_HOSTNAME_FORBLOCKINGTESTS)
        if (m_fBlockedHost)
            return;
#endif

        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if ( ! MeetsPrerequisites(L"TestTotalPhysicalDisk"))
            return;
#endif

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
        m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);
        enum { MIN_VALUE = 0, AVG_VALUE = 1, MAX_VALUE = 2 };
        m_diskEnumPhysical->InitInstances();
        m_diskEnumPhysical->SampleDisks();
        ExcerciseDisk();
        m_diskEnumPhysical->SampleDisks();
        m_diskEnumPhysical->Update(true);

        scxulong rPerSecond = 0;
        scxulong wPerSecond = 0;
        scxulong tPerSecond = 0;
        scxulong rBytesPerSecond = 0;
        scxulong wBytesPerSecond = 0;
        scxulong tBytesPerSecond = 0;
        double secondsPerRead[3] = {0,0,0};
        double secondsPerWrite[3] = {0,0,0};
        double secondsPerTransfer[3] = {0,0,0};
#if defined(sun)
        scxulong tPercentage = 0;
#endif
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalPhysicalDiskInstance>::EntityIterator iter = m_diskEnumPhysical->Begin(); iter != m_diskEnumPhysical->End(); iter++)
        {
            scxulong rps, wps, tps, rbps, wbps, tbps;
            double spr, spw, spt;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> disk = *iter;
            CPPUNIT_ASSERT(0 != disk);
#if defined(aix) || defined(linux) || defined(sun)
            CPPUNIT_ASSERT(disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT(disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT(disk->GetBytesPerSecond(rbps, wbps));
#elif defined(hpux)
            CPPUNIT_ASSERT( ! disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT( ! disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT(! disk->GetBytesPerSecond(rbps, wbps));
            rps = wps = 0;
            rbps = wbps = 0;
#endif
            CPPUNIT_ASSERT(disk->GetTransfersPerSecond(tps));
            CPPUNIT_ASSERT(disk->GetBytesPerSecondTotal(tbps));

            rPerSecond += rps;
            wPerSecond += wps;
            tPerSecond += tps;
            rBytesPerSecond += rbps;
            wBytesPerSecond += wbps;
            tBytesPerSecond += tbps;
#if defined(sun)
            scxulong tP;
            CPPUNIT_ASSERT(disk->GetIOPercentageTotal(tP));
            tPercentage += tP;
#endif
            spr = spw = spt = 0;
#if defined(linux)
            CPPUNIT_ASSERT(disk->GetIOTimes(spr, spw));
            CPPUNIT_ASSERT(disk->GetIOTimesTotal(spt));
#elif defined(hpux) || defined(sun)
            CPPUNIT_ASSERT( ! disk->GetIOTimes(spr, spw));
            CPPUNIT_ASSERT(disk->GetIOTimesTotal(spt));
#endif
            secondsPerRead[AVG_VALUE] += spr;
            secondsPerWrite[AVG_VALUE] += spw;
            secondsPerTransfer[AVG_VALUE] += spt;
            if (iter == m_diskEnumPhysical->Begin())
            {
                secondsPerRead[MIN_VALUE] = spr;
                secondsPerWrite[MIN_VALUE] = spw;
                secondsPerTransfer[MIN_VALUE] = spt;
                secondsPerRead[MAX_VALUE] = spr;
                secondsPerWrite[MAX_VALUE] = spw;
                secondsPerTransfer[MAX_VALUE] = spt;
            }
            else {
                secondsPerRead[MIN_VALUE] = MIN(spr,secondsPerRead[MIN_VALUE]);
                secondsPerWrite[MIN_VALUE] = MIN(spw,secondsPerWrite[MIN_VALUE]);
                secondsPerTransfer[MIN_VALUE] = MIN(spt,secondsPerTransfer[MIN_VALUE]);
                secondsPerRead[MAX_VALUE] = MAX(spr,secondsPerRead[MAX_VALUE]);
                secondsPerWrite[MAX_VALUE] = MAX(spw,secondsPerWrite[MAX_VALUE]);
                secondsPerTransfer[MAX_VALUE] = MAX(spt,secondsPerTransfer[MAX_VALUE]);
            }
            //std::cout << std::endl << "TEST " << spt << " " << m_diskEnumPhysical->Size() << std::endl;
        }
        if (m_diskEnumPhysical->Size() > 0)
        {
            secondsPerRead[AVG_VALUE] = secondsPerRead[AVG_VALUE] / static_cast<double>(m_diskEnumPhysical->Size());
            secondsPerWrite[AVG_VALUE] = secondsPerWrite[AVG_VALUE] / static_cast<double>(m_diskEnumPhysical->Size());
            secondsPerTransfer[AVG_VALUE] = secondsPerTransfer[AVG_VALUE] / static_cast<double>(m_diskEnumPhysical->Size());
        }
        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> total = m_diskEnumPhysical->GetTotalInstance();
        scxulong rps, wps, tps, rbps, wbps, tbps;
        double spr, spw, spt;
        CPPUNIT_ASSERT(0 != total);
#if defined (aix) || defined(sun) || defined(linux)
        CPPUNIT_ASSERT(total->GetReadsPerSecond(rps));
        CPPUNIT_ASSERT(total->GetWritesPerSecond(wps));
        CPPUNIT_ASSERT(total->GetBytesPerSecond(rbps, wbps));
#elif defined(hpux)
        CPPUNIT_ASSERT( ! total->GetReadsPerSecond(rps));
        CPPUNIT_ASSERT( ! total->GetWritesPerSecond(wps));
        CPPUNIT_ASSERT( ! total->GetBytesPerSecond(rbps, wbps));
        rps = wps = 0;
        rbps = wbps = 0;
#endif
        CPPUNIT_ASSERT(total->GetTransfersPerSecond(tps));
        CPPUNIT_ASSERT(total->GetBytesPerSecondTotal(tbps));
        CPPUNIT_ASSERT_EQUAL(rPerSecond, rps);
        CPPUNIT_ASSERT_EQUAL(wPerSecond, wps);
        CPPUNIT_ASSERT_EQUAL(tPerSecond, tps);
        CPPUNIT_ASSERT_EQUAL(rBytesPerSecond, rbps);
        CPPUNIT_ASSERT_EQUAL(wBytesPerSecond, wbps);
        CPPUNIT_ASSERT_EQUAL(tBytesPerSecond, tbps);
#if defined(aix) || defined(linux)
        spr = spw = spt = 0;
        CPPUNIT_ASSERT(total->GetIOTimes(spr, spw));
        CPPUNIT_ASSERT(total->GetIOTimesTotal(spt));
#elif defined(hpux) || defined(sun)
        CPPUNIT_ASSERT( ! total->GetIOTimes(spr, spw));
        CPPUNIT_ASSERT(total->GetIOTimesTotal(spt));
#endif
        SCXUNIT_ASSERT_BETWEEN(secondsPerRead[AVG_VALUE], secondsPerRead[MIN_VALUE], secondsPerRead[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(secondsPerWrite[AVG_VALUE], secondsPerWrite[MIN_VALUE], secondsPerWrite[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(secondsPerTransfer[AVG_VALUE], secondsPerTransfer[MIN_VALUE], secondsPerTransfer[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spr, secondsPerRead[MIN_VALUE], secondsPerRead[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spw, secondsPerWrite[MIN_VALUE], secondsPerWrite[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spt, secondsPerTransfer[MIN_VALUE], secondsPerTransfer[MAX_VALUE]);

#if defined(sun)
        // Avoid division by zero if we have no physical disks
        if ( HasPhysicalDisks(L"TestLogicalDiskPerfCounters", true) )
        {
            scxulong tP;
            CPPUNIT_ASSERT(total->GetIOPercentageTotal(tP));
            CPPUNIT_ASSERT_EQUAL(tPercentage/m_diskEnumPhysical->Size(), tP);
        }
#endif

        std::wstring dev,name;
        CPPUNIT_ASSERT(total->GetDiskName(name));
        CPPUNIT_ASSERT(L"_Total" == name);
        CPPUNIT_ASSERT(total->GetDiskDeviceID(dev));
        CPPUNIT_ASSERT(L"_Total" == dev);
    }

    void TestTotalLogicalDisk()
    {
        // This test needs root access on RHEL4
#if defined(PF_DISTRO_REDHAT) && (PF_MAJOR==4)
        if ( ! MeetsPrerequisites(L"TestTotalLogicalDisk"))
            return;
#endif

#if defined(sun)
        TestDisks disks;
        if (0 == disks.logical.size())
        {
            SCXUNIT_WARNING(L"Test class did not find any partitions - probably because they have their data under \"vopstats\"-entries (WIs: 3490, 3704, 4631) - PAL handles this correctly");
            return;
        }
#endif
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new SCXSystemLib::DiskDependDefault());
        m_diskEnumLogical = new SCXSystemLib::StatisticalLogicalDiskEnumeration(deps);
        enum { MIN_VALUE = 0, AVG_VALUE = 1, MAX_VALUE = 2 };
        m_diskEnumLogical->InitInstances();
        m_diskEnumLogical->SampleDisks();
        ExcerciseDisk();
        m_diskEnumLogical->SampleDisks();
        m_diskEnumLogical->Update(true);

        scxulong rPerSecond = 0;
        scxulong wPerSecond = 0;
        scxulong tPerSecond = 0;
        scxulong rBytesPerSecond = 0;
        scxulong wBytesPerSecond = 0;
        scxulong tBytesPerSecond = 0;
        double secondsPerRead[3] = {0,0,0};
        double secondsPerWrite[3] = {0,0,0};
        double secondsPerTransfer[3] = {0,0,0};
        scxulong mbUsed = 0;
        scxulong mbFree = 0;
        bool excludeDeviceFreeSpace=false;
#if defined(sun)
        scxulong tPercentage = 0;
#endif
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalLogicalDiskInstance>::EntityIterator iter = m_diskEnumLogical->Begin(); iter != m_diskEnumLogical->End(); iter++)
        {
            scxulong rps, wps, tps, rbps, wbps, tbps, mbu, mbf;
            double spr, spw, spt;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> disk = *iter;
            CPPUNIT_ASSERT(0 != disk);
            CPPUNIT_ASSERT(disk->GetDiskSize(mbu, mbf));
            mbUsed += mbu;
#if defined(sun)
           if (excludeDeviceFreeSpace)
               excludeDeviceFreeSpace = false;
           wstring m_name;
           wstring m_fsType;
           if(disk->GetFSType(m_fsType))
           {
               std:wstring mountDev=disk->DumpString();
               if (m_fsType == L"zfs" && mountDev.find(L"/") != wstring::npos)
                       excludeDeviceFreeSpace=true;
            }
#endif
           mbFree += excludeDeviceFreeSpace?0:mbf;
#if defined(aix)
            CPPUNIT_ASSERT( ! disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT( ! disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT( ! disk->GetBytesPerSecond(rbps, wbps));
            CPPUNIT_ASSERT( ! disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT( ! disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT( ! disk->GetBytesPerSecond(rbps, wbps));
            CPPUNIT_ASSERT( ! disk->GetTransfersPerSecond(tps));
            CPPUNIT_ASSERT( ! disk->GetBytesPerSecondTotal(tbps));
            CPPUNIT_ASSERT( ! disk->GetIOTimes(spr, spw));
            CPPUNIT_ASSERT( ! disk->GetIOTimesTotal(spt));
#else
            CPPUNIT_ASSERT(disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT(disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT(disk->GetBytesPerSecond(rbps, wbps));
            CPPUNIT_ASSERT(disk->GetReadsPerSecond(rps));
            CPPUNIT_ASSERT(disk->GetWritesPerSecond(wps));
            CPPUNIT_ASSERT(disk->GetBytesPerSecond(rbps, wbps));
            CPPUNIT_ASSERT(disk->GetTransfersPerSecond(tps));
            CPPUNIT_ASSERT(disk->GetBytesPerSecondTotal(tbps));

            rPerSecond += rps;
            wPerSecond += wps;
            tPerSecond += tps;
            rBytesPerSecond += rbps;
            wBytesPerSecond += wbps;
            tBytesPerSecond += tbps;
#if defined(sun)
            scxulong tP;
            CPPUNIT_ASSERT(disk->GetIOPercentageTotal(tP));
            tPercentage += tP;
#endif
            spr = spw = spt = 0;
#if defined(linux)
            CPPUNIT_ASSERT( ! disk->GetIOTimes(spr, spw));
            CPPUNIT_ASSERT( ! disk->GetIOTimesTotal(spt));
#elif defined(hpux) || defined(sun)
            CPPUNIT_ASSERT( ! disk->GetIOTimes(spr, spw));
            CPPUNIT_ASSERT(disk->GetIOTimesTotal(spt));
#endif
            secondsPerRead[AVG_VALUE] += spr;
            secondsPerWrite[AVG_VALUE] += spw;
            secondsPerTransfer[AVG_VALUE] += spt;
            if (iter == m_diskEnumLogical->Begin())
            {
                secondsPerRead[MIN_VALUE] = spr;
                secondsPerWrite[MIN_VALUE] = spw;
                secondsPerTransfer[MIN_VALUE] = spt;
                secondsPerRead[MAX_VALUE] = spr;
                secondsPerWrite[MAX_VALUE] = spw;
                secondsPerTransfer[MAX_VALUE] = spt;
            }
            else {
                secondsPerRead[MIN_VALUE] = MIN(spr,secondsPerRead[MIN_VALUE]);
                secondsPerWrite[MIN_VALUE] = MIN(spw,secondsPerWrite[MIN_VALUE]);
                secondsPerTransfer[MIN_VALUE] = MIN(spt,secondsPerTransfer[MIN_VALUE]);
                secondsPerRead[MAX_VALUE] = MAX(spr,secondsPerRead[MAX_VALUE]);
                secondsPerWrite[MAX_VALUE] = MAX(spw,secondsPerWrite[MAX_VALUE]);
                secondsPerTransfer[MAX_VALUE] = MAX(spt,secondsPerTransfer[MAX_VALUE]);
            }
#endif
        }
        if (m_diskEnumLogical->Size() > 0)
        {
            secondsPerRead[AVG_VALUE] = secondsPerRead[AVG_VALUE] / static_cast<double>(m_diskEnumLogical->Size());
            secondsPerWrite[AVG_VALUE] = secondsPerWrite[AVG_VALUE] / static_cast<double>(m_diskEnumLogical->Size());
            secondsPerTransfer[AVG_VALUE] = secondsPerTransfer[AVG_VALUE] / static_cast<double>(m_diskEnumLogical->Size());
        }
        SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalLogicalDiskInstance> total = m_diskEnumLogical->GetTotalInstance();
        scxulong rps, wps, tps, rbps, wbps, tbps, mbu, mbf;
        double spr, spw, spt;
        CPPUNIT_ASSERT(0 != total);
#if defined(aix)
        rps = wps = rbps = wbps = tps = tbps = 0;
        CPPUNIT_ASSERT( ! total->GetReadsPerSecond(rps));
        CPPUNIT_ASSERT( ! total->GetWritesPerSecond(wps));
        CPPUNIT_ASSERT( ! total->GetBytesPerSecond(rbps, wbps));
        CPPUNIT_ASSERT( ! total->GetTransfersPerSecond(tps));
        CPPUNIT_ASSERT( ! total->GetBytesPerSecondTotal(tbps));
#else
#if defined(sun) || defined(linux)
        CPPUNIT_ASSERT(total->GetReadsPerSecond(rps));
        CPPUNIT_ASSERT(total->GetWritesPerSecond(wps));
        CPPUNIT_ASSERT(total->GetBytesPerSecond(rbps, wbps));
#elif defined(hpux)
        CPPUNIT_ASSERT(total->GetReadsPerSecond(rps));
        CPPUNIT_ASSERT(total->GetWritesPerSecond(wps));
        CPPUNIT_ASSERT(total->GetBytesPerSecond(rbps, wbps));
#endif
        CPPUNIT_ASSERT(total->GetTransfersPerSecond(tps));
        CPPUNIT_ASSERT(total->GetBytesPerSecondTotal(tbps));
#endif
        CPPUNIT_ASSERT_EQUAL(rPerSecond, rps);
        CPPUNIT_ASSERT_EQUAL(wPerSecond, wps);
        CPPUNIT_ASSERT_EQUAL(tPerSecond, tps);
        CPPUNIT_ASSERT_EQUAL(rBytesPerSecond, rbps);
        CPPUNIT_ASSERT_EQUAL(wBytesPerSecond, wbps);
        CPPUNIT_ASSERT_EQUAL(tBytesPerSecond, tbps);
        CPPUNIT_ASSERT(total->GetDiskSize(mbu, mbf));
        CPPUNIT_ASSERT_EQUAL(mbUsed, mbu);
        CPPUNIT_ASSERT_EQUAL(mbFree, mbf);
#if defined(aix) || defined(linux)
        spr = spw = spt = 0;
        CPPUNIT_ASSERT( ! total->GetIOTimes(spr, spw));
        CPPUNIT_ASSERT( ! total->GetIOTimesTotal(spt));
#elif defined(hpux) || defined(sun)
        spr = spw = 0;
        CPPUNIT_ASSERT( ! total->GetIOTimes(spr, spw));
        CPPUNIT_ASSERT(total->GetIOTimesTotal(spt));
#endif
        SCXUNIT_ASSERT_BETWEEN(secondsPerRead[AVG_VALUE], secondsPerRead[MIN_VALUE], secondsPerRead[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(secondsPerWrite[AVG_VALUE], secondsPerWrite[MIN_VALUE], secondsPerWrite[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(secondsPerTransfer[AVG_VALUE], secondsPerTransfer[MIN_VALUE], secondsPerTransfer[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spr, secondsPerRead[MIN_VALUE], secondsPerRead[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spw, secondsPerWrite[MIN_VALUE], secondsPerWrite[MAX_VALUE]);
        SCXUNIT_ASSERT_BETWEEN(spt, secondsPerTransfer[MIN_VALUE], secondsPerTransfer[MAX_VALUE]);

#if defined(sun)
        scxulong tP;
        CPPUNIT_ASSERT(total->GetIOPercentageTotal(tP));
        CPPUNIT_ASSERT_EQUAL(tPercentage/m_diskEnumLogical->Size(), tP);
#endif

        std::wstring dev,name;
        CPPUNIT_ASSERT(total->GetDiskName(name));
        CPPUNIT_ASSERT(L"_Total" == name);
        CPPUNIT_ASSERT(total->GetDiskDeviceID(dev));
        CPPUNIT_ASSERT(L"_Total" == dev);
    }

#if defined(aix)
    /*
      This test exists because some systems have disks under locations like /dev/asm/acfs_vol001-41, and previously
      we would fail on InitInstances due to an exception getting thrown when we attempted to pass "asm/acfs_vol001-41"
      into the parameter for SCXFilePath::SetFilename.
     */
    void Test_perfstat_disk_Regarding_Devices_Inside_Subdirectories_In_slashdev_Directory_RFC_483999()
    {
        // Simulate the customer's return values from perfstat_disk
        const size_t numElements = 83;
        const char* dev_array[] = {
            "hdisk1", "hdisk0", "hdisk32", "hdisk30", "hdisk19", "hdisk28", "hdisk34", "hdisk43", "hdisk23", "hdisk27",
            "hdisk33", "hdisk24", "hdisk22", "hdisk31", "hdisk50", "hdisk55", "hdisk29", "hdisk57", "hdisk51", "hdisk26",
            "hdisk52", "hdisk53", "hdisk54", "hdisk35", "hdisk25", "hdisk14", "hdisk5", "hdisk3", "hdisk12", "hdisk13",
            "hdisk10", "hdisk6", "hdisk16", "hdisk4", "hdisk40", "hdisk11", "hdisk7", "hdisk44", "hdisk17", "hdisk15",
            "hdisk2", "hdisk47", "hdisk45", "hdisk39", "hdisk46", "hdisk9", "hdisk8", "hdisk56", "hdisk48", "hdisk18",
            "hdisk21", "hdisk20", "hdiskpower0", "hdiskpower1", "hdiskpower2", "hdiskpower3", "hdiskpower4", "hdiskpower27",
            "hdiskpower7", "hdiskpower8", "hdiskpower9", "hdiskpower10", "hdiskpower11", "hdiskpower12", "hdiskpower13",
            "hdiskpower14", "hdiskpower28", "hdiskpower29", "hdiskpower19", "hdiskpower20", "hdiskpower21", "hdiskpower22",
            "hdiskpower23", "hdiskpower24", "hdiskpower25", "hdiskpower26", "asm/acfs_vol001-41", "hdiskpower30",
            "hdiskpower31", "hdisk38", "hdisk36", "hdisk41", "hdisk37"
        };

        std::vector<std::string> disks;
        for (size_t i = 0; i < numElements; i++)
        {
            disks.push_back(std::string(dev_array[i]));
        }

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps(new TestDiskDependDefault(disks));
        m_diskEnumPhysical = new SCXSystemLib::StatisticalPhysicalDiskEnumeration(deps);

        // This was previously throwing an exception due to the asm/acfs_vol001-41 name.
        CPPUNIT_ASSERT_NO_THROW(m_diskEnumPhysical->InitInstances());

        // Ensure that all disks can be found with FindDiskByDevice.
        for (size_t i = 0; i< numElements; i++)
        {
            std::string name = std::string(dev_array[i]);
            CPPUNIT_ASSERT_MESSAGE("Cannot find disk with name " + name, m_diskEnumPhysical->FindDiskByDevice(L"/dev/" + SCXCoreLib::StrFromUTF8(name)) != 0);
        }

        CPPUNIT_ASSERT_NO_THROW(m_diskEnumPhysical->Update(true));
    }
#endif // defined(aix)
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStatisticalDiskPalSanityTest );

