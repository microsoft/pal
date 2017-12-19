/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Implements the default implementation for disk dependencies

   \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxregex.h>
#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/scxsysteminfo.h>
#include <scxsystemlib/scxproductdependencies.h>

#if defined(aix)
#include <sys/vmount.h>
#include <vector>
#elif defined(linux)
#include <scxsystemlib/scxlvmutils.h>
#endif
#include <errno.h>

namespace SCXSystemLib
{
    const scxlong DiskDepend::s_cINVALID_INSTANCE = -1;

    /*----------------------------------------------------------------------------*/
    /**
       Constructor used for injecting a log handle.
    */
    DiskDependDefault::DiskDependDefault(const SCXCoreLib::SCXLogHandle& log):
        m_log(log),
        m_pLvmTab(0),
        m_pRaid(0),
        m_fd(CLOSED_DESCRIPTOR),
        m_OpenFlags(O_RDONLY)
    {
        m_PathName[0] = '\0';
        InitializeObject();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor
    */
    DiskDependDefault::DiskDependDefault():
        m_log(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.diskdepend")),
        m_pLvmTab(0),
        m_pRaid(0),
        m_fd(CLOSED_DESCRIPTOR)
    {
        m_PathName[0] = '\0';
        InitializeObject();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Private method that initializes the object. Called from the constructors.
    */
    void DiskDependDefault::InitializeObject()
    {
#if defined(aix)
#elif defined(linux)
        m_ProcDiskStatsPath.Set(L"/proc/diskstats");
        m_ProcPartitionsPath.Set(L"/proc/partitions");
        m_MntTabPath.Set(L"/etc/mtab");
#elif defined(sun)
        m_MntTabPath.Set(L"/etc/mnttab");
        m_DevTabPath.Set(L"/etc/device.tab");
#elif defined(hpux)
        m_MntTabPath.Set(L"/etc/mnttab");
#else
#error "Platform not supported"
#endif
#if defined(aix)
        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(L"/etc/vfs", std::ios::in));
        fs.SetOwner();
        while ( ! fs->eof() && fs->is_open() )
        {
            std::wstring line;
            getline( *fs, line );
            if (line.length() == 0)
            {
                continue;
            }
            if (line.substr(0,1) == L"#" || line.substr(0,1) == L"%")
            { // Line is a comment.
                continue;
            }
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(line, parts, L" \n\t\r");
            if (parts.size() < 4)
            {
                continue;
            }
            m_fsMap[parts[1]] = parts[0];
        }
        fs->close();
#elif defined(sun)
        try
        {
            SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(L"/etc/path_to_inst", std::ios::in));
            fs.SetOwner();
            while ( ! fs->eof() && fs->is_open() )
            {
                std::wstring line;
                getline( *fs, line );
                if (line.length() == 0)
                {
                    continue;
                }
                if (line.substr(0,1) == L"#")
                { // Line is a comment.
                    continue;
                }
                std::vector<std::wstring> parts;
                SCXCoreLib::StrTokenize(line, parts, L" \n\t");
                if (parts.size() < 3)
                {
                    continue;
                }
                SCXCoreLib::SCXHandle<DeviceInstance> di( new DeviceInstance );
                try
                {
                    di->m_instance = SCXCoreLib::StrToLong(parts[1]);
                }
                catch (SCXCoreLib::SCXNotSupportedException)
                {
                    di->m_instance = s_cINVALID_INSTANCE;
                }
                di->m_name = SCXCoreLib::StrStrip(parts[2], L"\" \t\n\r");
                m_deviceMap[SCXCoreLib::StrStrip(parts[0], L"\" \t\n\r")] = di;
            }
            fs->close();
        }
        catch (SCXCoreLib::SCXFilePathNotFoundException)
        {
            // The file '/etc/path_to_inst' may not exist (like in a zone).  If this
            // happens, then we have no physical disk devices that can be found ...
            //
            // If we're not in the global zone, then this is okay ...

            bool fIsInGlobalZone;
            SCXSystemLib::SystemInfo si;
            si.GetSUN_IsInGlobalZone( fIsInGlobalZone );

            if ( fIsInGlobalZone ) throw;
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Disk opener

       \param       path name of device
       \param       access flags.
       \returns     true == success, false == failure (see errno)
    */
    bool DiskDependDefault::open(const char* pathname, int flags)
    {
        bool fRet = false;
        int  fd   = 0;

        SCXASSERT(pathname != NULL);
        SCXASSERT(pathname[0] != '\0');

        this->close();  // Close any prior opens.

        if (CLOSED_DESCRIPTOR != (fd = ::open(pathname, flags)))
        {
            m_fd = fd;
            fRet = true;

            fd = 0;
            // Save parameters for subsequent re-opens.
            m_OpenFlags = flags;
            strncpy(m_PathName, pathname, MAXPATHLEN);
            SCXASSERT(0 == strncmp(m_PathName, pathname, MAXPATHLEN));
            m_PathName[MAXPATHLEN - 1] = '\0'; // z-term in case original string was not z-term'ed.
            SCX_LOGTRACE(m_log, SCXCoreLib::StrFromUTF8(std::string("Opened \"") + pathname + " flags: ")
                         + SCXCoreLib::StrFrom(flags)
                         + L" stored as\""
                         + SCXCoreLib::StrFromUTF8(m_PathName)
                         + L"\"");
        }
        else
        {
            fRet = false;
            SCX_LOGERROR(m_log, SCXCoreLib::StrFromUTF8(std::string("Failed to open \"") + pathname + "\" flags: ") + SCXCoreLib::StrFrom(flags));
        }

        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Disk opener (internal use only).
    */
    void DiskDependDefault::reopen()
    {
        if(m_PathName[0] == '\0')
        {
            SCX_LOGTRACE(m_log, L"Reopen attempt on empty filename");
        }
        else
        {
            int fd = 0;

            this->close();  // Close any prior opens.

            errno = 0;
            if (CLOSED_DESCRIPTOR != (fd = ::open(m_PathName, m_OpenFlags)))
            {
                m_fd = fd;
                SCX_LOGTRACE(m_log, SCXCoreLib::StrFromUTF8(std::string("re-opened \"") + m_PathName + "\" flags: ") + SCXCoreLib::StrFrom(m_OpenFlags));
            }
            else
            {
                SCX_LOGERROR(m_log, SCXCoreLib::StrFromUTF8(std::string("Failed to re-open \"") + m_PathName + "\" flags: ") + SCXCoreLib::StrFrom(m_OpenFlags));
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Disk closer

       \param       ignored. member file descriptor is used instead.
       \returns  0 == success, -1 == failure (see errno)
    */
    int DiskDependDefault::close()
    {
        int rc = 0;

        if (m_fd != CLOSED_DESCRIPTOR)
        {
            errno = 0;
            rc = ::close(m_fd);
            if (rc == -1)
            {
                // Clear out bad m_fd.
                if (errno == EBADF)
                {
                    m_fd = CLOSED_DESCRIPTOR;
                    rc = 0;
                }
            }
            else
            {
                m_fd = CLOSED_DESCRIPTOR;
            }
        }

        return rc;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Disk accessor

       \param       ignored. member file descriptor is used instead.
       \param       request code.
       \param       output memory (depends on request code).
       \returns  0 == success, -1 == failure (see errno)
    */
    int DiskDependDefault::ioctl(unsigned long int request, void* data)
    {
        int rc = 0;

        if (m_fd == CLOSED_DESCRIPTOR)
        {
            this->reopen();
            rc = (m_fd == CLOSED_DESCRIPTOR) ? -1 : 0;
            // Preserve errno
            SCX_LOG(m_log, SCXCoreLib::eTrace, SCXCoreLib::StrFromUTF8(std::string("Opened \"") + m_PathName + "\" rc: ") + SCXCoreLib::StrFrom(rc));
        }

        if (rc != -1)
        {
            errno = 0;
            rc = ::ioctl(m_fd, request, data);
            if (rc == -1)
            {
                std::wstringstream out;
                out << L"ioctl fail. errno=" << errno << L", fd=" << m_fd;
                SCX_LOG(m_log, SCXCoreLib::eTrace, out.str());
                // failed ioctl's sometimes break a handle.
                // Closing this handle forces a re-open at the next read/write access.
                this->close();
            }
        }

        return rc;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Disk accessor

       \param       ignored. member file descriptor is used instead.
       \param       output memory (depends on request code).
       \param       count of bytes in output memory.
       \returns  0 == success, -1 == failure (see errno)
    */
    ssize_t DiskDependDefault::read(void *pbuf, size_t bytecount)
    {
        ssize_t rc = 0;

        SCXASSERT(pbuf != NULL);

        if (m_fd == CLOSED_DESCRIPTOR)
        {
            errno = 0;
            this->reopen();
            rc = (m_fd == CLOSED_DESCRIPTOR) ? -1 : 0;
            // Preserve errno
            SCX_LOGTRACE(m_log, SCXCoreLib::StrFromUTF8(std::string("Opened \"") + m_PathName + "\" rc: ") + SCXCoreLib::StrFrom(rc));
        }

        if (rc != -1)
        {
            rc = ::read(m_fd, pbuf, bytecount);
        }

        return rc;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Virtual destructor
    */
    DiskDependDefault::~DiskDependDefault()
    {
        this->close();
    }


    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::LocateMountTab
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateMountTab()
    {
        return m_MntTabPath;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::LocateProcDiskStats
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateProcDiskStats()
    {
        return m_ProcDiskStatsPath;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::RefreshProcDiskStats
    */
    void DiskDependDefault::RefreshProcDiskStats()
    {
        m_ProcDiskStats.clear();

        SCXCoreLib::SCXHandle<std::wfstream> fsDiskStats(SCXCoreLib::SCXFile::OpenWFstream(LocateProcDiskStats(), std::ios::in));
        fsDiskStats.SetOwner();
        std::wstring line;
        while ( ! fsDiskStats->eof() && fsDiskStats->is_open() )
        {
            getline( *fsDiskStats, line );
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(line, parts, L" \n\t");
            if (parts.size() < 3)
            {
                continue;
            }
            m_ProcDiskStats[parts[2]] = parts;
        }
        fsDiskStats->close();
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::LocateProcPartitions
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateProcPartitions()
    {
        return m_ProcPartitionsPath;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetProcDiskStats
    */
    const std::vector<std::wstring>& DiskDependDefault::GetProcDiskStats(const std::wstring& device)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        const std::wstring slashdevslash(L"/dev/");
        std::wstring tailstr;

        // We assume device path is all of the device name after '/dev/'
        if(device.find(slashdevslash) == 0)
            tailstr = device.substr(slashdevslash.length());
        else
        {
            // This is the former way of doing the lookup ... do not find leading '/dev/'
            SCXCoreLib::SCXFilePath dev(device);
            tailstr = dev.GetFilename();
        }

        std::map<std::wstring, std::vector<std::wstring> >::const_iterator it =
        m_ProcDiskStats.find(tailstr);

        if (it == m_ProcDiskStats.end())
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(device));
            std::wstringstream out ;
            out << L"Did not find key '" << tailstr << L"' in proc_disk_stats map, device name was '" << device << L"'.";
            SCX_LOG(m_log, severity, out.str());
            static std::vector<std::wstring> empty;
            return empty;
        }
        return it->second;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetFilesInDirectory
    */
    void DiskDependDefault::GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files)
    {
        files.clear();
        if (SCXCoreLib::SCXDirectory::Exists(path))
        {
            files = SCXCoreLib::SCXDirectory::GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionFile | SCXCoreLib::eDirSearchOptionSys);
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetLVMTab
    */
    const SCXLvmTab& DiskDependDefault::GetLVMTab()
    {
        if (0 ==  m_pLvmTab)
        {
            try
            {
                m_pLvmTab = new SCXLvmTab(L"/etc/lvmtab");
            }
            catch (SCXSystemLib::SCXLvmTabFormatException& e)
            {
                SCXRETHROW(e, SCXCoreLib::StrAppend(L"Wrong lvmtab format: ", e.What()));
            }
            catch (SCXCoreLib::SCXUnauthorizedFileSystemAccessException& e2)
            {
                SCXRETHROW(e2, L"Unable to parse /etc/lvmtab without root access");
            }
        }
        return *m_pLvmTab;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetMNTTab

       \note Not thread safe.
    */
    const std::vector<MntTabEntry>& DiskDependDefault::GetMNTTab()
    {
        return m_MntTab;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::RefreshMNTTab

       \note Not thread safe.
    */
    void DiskDependDefault::RefreshMNTTab()
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        SCX_LOGTRACE(m_log, L"RefreshMNTTab: mnttab file being read");
        if (0 < m_MntTab.size())
        {
            SCX_LOGTRACE(m_log, L"RefreshMNTTab: Clearing m_MntTab");
            m_MntTab.clear();
        }
#if defined(aix)
        int needed = 0;
        // Get the number of bytes needed for all mntctl data.
        int r = mntctl(MCTL_QUERY, sizeof(needed), reinterpret_cast<char*>(&needed));
        if (0 == r)
        {
            std::vector<char> buf(needed);
            char* p = &buf[0];
            // Returns number of structs in buffer; use that to limit data walk
            r = mntctl(MCTL_QUERY, needed, &buf[0]);
            if (r < 0)
            {
                SCX_LOGERROR(m_log, L"mntctl(MCTL_QUERY) failed with errno = " + SCXCoreLib::StrFrom(errno));
            }
            for (int i = 0; i < r; i++)
            {
                struct vmount* vmt = reinterpret_cast<struct vmount*>(p);
                std::wstring fs = SCXCoreLib::StrFrom(vmt->vmt_gfstype);
                if (vmt->vmt_data[VMT_OBJECT].vmt_size > 0 &&
                    vmt->vmt_data[VMT_STUB].vmt_size > 0 &&
                    m_fsMap.find(fs) != m_fsMap.end())
                {
                    MntTabEntry entry;
                    std::string device(p + vmt->vmt_data[VMT_OBJECT].vmt_off);
                    std::string mountPoint(p + vmt->vmt_data[VMT_STUB].vmt_off);

                    entry.device = SCXCoreLib::StrFromUTF8(device);
                    entry.mountPoint = SCXCoreLib::StrFromUTF8(mountPoint);
                    entry.fileSystem = m_fsMap.find(fs)->second;
                    m_MntTab.push_back(entry);
                }
                p += vmt->vmt_length;
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"mntctl(MCTL_QUERY) failed with errno = " + SCXCoreLib::StrFrom(errno));
        }
#else
        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(
                                                    LocateMountTab(), std::ios::in));
        fs.SetOwner();
        while ( ! fs->eof() && fs->is_open() )
        {
            std::wstring line;
            std::vector<std::wstring> parts;
            getline( *fs, line );
#if defined (linux)
            if (line.find(L"loop=") != std::wstring::npos || line.find(L"/dev/loop") != std::wstring::npos)
            {
                // for Linux, ignore files mounted as devices using the loopback driver
                continue;
            }
#endif
            SCXCoreLib::StrTokenize(line, parts, L" \n\t");
            if (parts.size() > 3)
            {
                if (std::wstring::npos != parts[0].find('#')) // Comment
                {
                    continue;
                }
#if defined(linux)
                // WI 53975427:
                //
                // The fix here is meant to exclude pseudo file system in file system enumeration.
                // It can also be fixed by adding these FS in IGFS. But the problem with this approacch is that every time new pseudo FS get introduced we need to make changes in IGFS
                // and need to have subsequent release to fix the issue.
                // The fix here is based on fundamental property of pseudo FS that it is not associated with any block device, hence not associated with any path.

                if (parts[0].find(L"/") == std::wstring::npos)
                {
                    continue;
                }
                // WI 574703:
                //
                // On Debian 7 systems, the system disk may come in with a device like:
                //    /dev/disk/by-uuid/e62e95e9-502b-463a-998d-23cf7130d7d2
                // This differs from what's in /proc/diskstats (which is the real physical
                // device).  Since the path in /dev/disk/by-uuid is actually a soft link
                // to the physical device, just resolve it if that's what we've got.

                if (parts[0].find(L"/dev/disk/by-uuid/") == 0)
                {
                    char buf[1024];
                    memset(buf, 0, sizeof(buf));
                    if (-1 == readlink(SCXCoreLib::StrToUTF8(parts[0]).c_str(), buf, sizeof(buf)))
                    {
                        std::wstringstream message;
                        message << L"readlink(file='" << parts[0] << "',...)";

                        SCXCoreLib::SCXErrnoException e(message.str(), errno, SCXSRCLOCATION);
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(message.str()));
                        std::wstringstream out;
                        out << "RefreshMNTTab: Error : " << e.What() << " at " << e.Where();
                        SCX_LOG(m_log, severity, out.str());
                    }
                    else
                    {
                        // readlink returns something like "../../sda1"; trim to return "/dev/sda1"
                        std::wstring link = SCXCoreLib::StrFromUTF8(buf);
                        size_t pos;
                        if ( (pos = link.rfind(L"/")) != std::wstring::npos )
                        {
                            parts[0] = L"/dev/" + link.substr(pos+1);
                        }
                        else
                        {
                            std::wstringstream message;
                            message << L"RefreshMNTTab: Unable to find physical define in link: " << link
                                    << " (Original file: " << parts[0] << ")";

                            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(message.str()));
                            SCX_LOG(m_log, severity, message.str());
                        }
                    }
                }
#endif
                MntTabEntry entry;
                entry.device = parts[0];
                entry.mountPoint = parts[1];
                entry.fileSystem = parts[2];
                if (parts[3].find(L"dev=") != std::wstring::npos)
                {
                    entry.devAttribute = parts[3].substr(parts[3].find(L"dev="));
                    if (entry.devAttribute.length() > 0)
                    {
                        entry.devAttribute = entry.devAttribute.substr(4); // Removing "dev="
                        entry.devAttribute = entry.devAttribute.substr(0,entry.devAttribute.find_first_not_of(L"0123456789abcdef"));
                    }
                }

                SCX_LOGTRACE(m_log,
                             L"RefreshMNTTab: Storing device '" + entry.device
                             + L"', mountpoint '" + entry.mountPoint
                             + L"', filesysstem '" + entry.fileSystem + L"'" );

                m_MntTab.push_back(entry);
            }
        }
        fs->close();
        SCX_LOGTRACE(m_log, L"RefreshMNTTab: Done writing m_MntTab");
#endif
    }

    /**
       Helper function for FileSystemIgnored.  Inserts each string from arr into newSet.

       \param[out] newSet - each wstring from arr is inserted into this.
       \param[in] arr - an array of wstrings terminated by an empty string.
    */
    static void AddToSet(std::set<std::wstring>& newSet, std::wstring arr[])
    {
        for (int i = 0; arr[i].length() != 0; i++)
        {
            newSet.insert(arr[i]);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::FileSystemIgnored

    */
    bool DiskDependDefault::FileSystemIgnored(const std::wstring& fs)
    {
        // Remember to NEVER change this list without a first failing the test called:
        // IgnoredFilesystemShouldNotBeCaseSensitive
        static std::wstring IGFS[] = {
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

        // File systems that are identified if they begin with anything in this list.
        static std::wstring IGFS_START[] = {
            L"nfs",
            L"" };

        // File systems that are identified if any part matches anything in this list.
        static std::wstring IGFS_PARTS[] = {
            L"gvfs",
            L"" };

        static std::set<std::wstring> IGFS_set, IGFS_Start_set, IGFS_Parts_set;
        static bool fInitialized = false;

        if ( ! fInitialized )
        {
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet(L"DiskDependFSI"));
            if ( ! fInitialized )
            {
                AddToSet(IGFS_set, IGFS);
                AddToSet(IGFS_Start_set, IGFS_START);
                AddToSet(IGFS_Parts_set, IGFS_PARTS);
                SCXSystemLib::SCXProductDependencies::Disk_IgnoredFileSystems(IGFS_set);
                SCXSystemLib::SCXProductDependencies::Disk_IgnoredFileSystems_StartsWith(IGFS_Start_set);
                SCXSystemLib::SCXProductDependencies::Disk_IgnoredFileSystems_Contains(IGFS_Parts_set);
                fInitialized = true;
            }
        }

        std::wstring fs_in_lower_case = SCXCoreLib::StrToLower(fs);
        return IsStringInSet(fs_in_lower_case, IGFS_set)
        || IsStringInSet(fs_in_lower_case, IGFS_Parts_set, CompareContains)
        || IsStringInSet(fs_in_lower_case, IGFS_Start_set, CompareStartsWith);
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::DeviceIgnored
    */
    bool DiskDependDefault::DeviceIgnored(const std::wstring& device)
    {
#if defined (sun)
        // Bug #15583: UFS CD/DVD-ROMs on Solaris systems are causing
        // false alarms about the disk being full. Prior to this fix,
        // logic for determining whether or not to report a disk was
        // based solely on the file system type. This does not hold
        // because ufs is the default file system type for Solaris.
        // The location of the mount point must also be examined.
        // For Solaris, CD-ROMs are automatically mounted in
        // '/vol/dev/dsk/'
        std::wstring volPath(L"/vol/dev/dsk/");
        return SCXCoreLib::StrIsPrefix(device, volPath);
#else
        // warnings as errors, so deal with the unused param
        (void) device;

        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if a given file system is represented by 'known' physical device.
       in mnttab file. Currently, we do not know how to get list of
       physical device(s) for ZFS filesystem

       \param       fs Name of file system.
       \returns     true if the file system has no 'real' device in mnt-tab.

    */
    bool DiskDependDefault::FileSystemNoLinkToPhysical(const std::wstring& fs)
    {
        static std::wstring IGFS[] = {
// Through an OEM agreement, VxFS is used as the primary filesystem on HPUX
#if !defined(hpux)
            L"vxfs",
#endif
            L"zfs",
            L"" };

        static std::set<std::wstring> IGFS_set;
        static bool fInitialized = false;

        if ( ! fInitialized )
        {
            SCXCoreLib::SCXThreadLock lock(SCXCoreLib::ThreadLockHandleGet(L"DiskDependNLTP"));
            if ( ! fInitialized )
            {
                AddToSet(IGFS_set, IGFS);
                SCXSystemLib::SCXProductDependencies::Disk_IgnoredFileSystems_NoLinkToPhysical(IGFS_set);
                fInitialized = true;
            }
        }

        return IsStringInSet(SCXCoreLib::StrToLower(fs), IGFS_set);
    }


    /*----------------------------------------------------------------------------*/
    /*
      \copydoc SCXSystemLib::DiskDepend::LinkToPhysicalExists
    */
    bool DiskDependDefault::LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

        return LinkToPhysicalExists(fs, dev_path, mountpoint, suppressor);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Used for injecting a log suppressor, otherwise same as SCXSystemLib::DiskDepend::LinkToPhysicalExists().

       \param       fs Name of file system.
       \param       dev_path Device path fount in mount table.
       \param       mountpoint Mount point found in mount table.
       \param[in] suppressor log suppressor to inject.

       \returns     true if the file system has 'real' device in mnt-tab.

    */
    bool DiskDependDefault::LinkToPhysicalExists(const std::wstring& fs,
                                                 const std::wstring& dev_path,
                                                 const std::wstring& mountpoint,
                                                 SCXCoreLib::LogSuppressor& suppressor)
    {
        if (dev_path == mountpoint || FileSystemNoLinkToPhysical(fs))
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(dev_path));
            SCX_LOG(m_log, severity,
                    L"No link exists between the logical device \"" + dev_path +
                    L"\" at mount point \"" + mountpoint +
                    L"\" with filesystem \"" + fs +
                    L"\". Some statistics will be unavailable.");

            return false;
        }
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::DeviceToInterfaceType
    */
#if defined(linux)
    DiskInterfaceType DiskDependDefault::DeviceToInterfaceType(const std::wstring& dev) const
    {
        SCXCoreLib::SCXFilePath path(dev);
        std::wstring name(path.GetFilename());
        if (name.size() == 0)
        {
            return eDiskIfcUnknown;
        }

        switch (name[0])
        {
            case 'h':
                return eDiskIfcIDE;
            case 's':
                return eDiskIfcSCSI;
            case 'x':
                if (name.size() >= 3 && name[1] == 'v' && name[2] =='d')
                {
                    return eDiskIfcVirtual;
                }
                return eDiskIfcUnknown;
            default:
                return eDiskIfcUnknown;
        }
    }
#else
    DiskInterfaceType DiskDependDefault::DeviceToInterfaceType(const std::wstring& ) const
    {
        return eDiskIfcUnknown;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::GetPhysicalDevices
    */
    std::map<std::wstring, std::wstring> DiskDependDefault::GetPhysicalDevices(const std::wstring& device)
    {
        std::map<std::wstring, std::wstring> devices;

        SCXCoreLib::SCXFilePath path(device);
        std::wstring name = path.GetFilename();
#if defined(aix)
        // Since we do not have the associaction between logical and physical disks - return them all
        perfstat_id_t id;
        perfstat_disk_t data;
        strcpy(id.name, FIRST_DISKPATH);
        do {
            int r = perfstat_disk(&id, &data, sizeof(data), 1);
            if (1 == r && 0 != strncmp(data.name, "cd", 2)) // TODO: better way to exclude CD/DVD!
            {
                name = SCXCoreLib::StrFromUTF8(data.name);
                devices[name] = L"/dev/" + name;
            }
            // TODO: Error handling?
        } while (0 != strcmp(id.name, FIRST_DISKPATH));
#elif defined(hpux)
        size_t vg_idx;
        for (vg_idx = 0; vg_idx < GetLVMTab().GetVGCount(); ++vg_idx)
        {
            // Stored name is without trailing slash.
            if (SCXCoreLib::StrAppend(GetLVMTab().GetVG(vg_idx),L"/") == path.GetDirectory())
            {
                for (size_t pidx = 0; pidx < GetLVMTab().GetPartCount(vg_idx); ++pidx)
                {
                    path.Set(GetLVMTab().GetPart(vg_idx, pidx));
                    name = path.GetFilename();
                    if (0 == SCXCoreLib::StrCompare(name.substr(0,4),L"disk",true))
                    { // New style: disk1_p2 (or just disk3)
                        name = name.substr(0, name.find_last_of(L"_"));
                    }
                    else
                    { // "sun" style: c1t2d3s4
                        if (name.substr(name.find_last_not_of(L"0123456789"),1) == L"s")
                        { // Remove partition identifier
                            name = name.substr(0,name.find_last_not_of(L"0123456789"));
                        }
                    }
                    path.SetFilename(name);
                    // Bug 6755 & 6883: partial discoveries of disks.
                    // Using algorithm from static PAL for exclusion.
                    std::wstring rawDevice = path.Get();
                    if (std::wstring::npos != rawDevice.find(L"/dsk/"))
                    {
                        rawDevice = SCXCoreLib::StrAppend(L"/dev/rdsk/", path.GetFilename());
                    }
                    else
                    {
                        rawDevice = SCXCoreLib::StrAppend(L"/dev/rdisk/", path.GetFilename());
                    }
                    if (this->open(SCXCoreLib::StrToUTF8(rawDevice).c_str(), O_RDONLY))
                    {
                        devices[name] = path.Get();
                        this->close();
                    }
                }
                break;
            }
        }
#elif defined(linux)
        // Given a device path to a partition (for example /dev/hda5 or /dev/cciss/c0d0p1), convert
        // it to a path to the base device (for example /dev/hda or /dev/cciss/c0d0).

        try
        {
            static SCXLVMUtils lvmUtils;

            // Try to convert the potential LVM device path into its matching
            // device mapper (dm) device path.
            std::wstring dmDevice = lvmUtils.GetDMDevice(device);

            if (dmDevice.empty())
            {
                // device is a normal partition device path
                path = GuessPhysicalFromLogicalDevice(device);
                name = path.GetFilename();
                devices[name] = path.Get();
            }
            else
            {
                // device was an LVM device path and dmDevice is the path to
                // the same device using the device mapper name.  The dm device
                // is located on one or more normal devices, known as slaves.
                std::vector< std::wstring > slaves = lvmUtils.GetDMSlaves(dmDevice);
                if (slaves.size() == 0)
                {
                    // this condition can only be reached on RHEL4/SLES9 systems
                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eInfo, SCXCoreLib::eHysterical);
                    std::wstringstream               out;

                    out << L"Because of limited support for LVM on "
#   if defined(PF_DISTRO_SUSE)
                        << L"SuSE Linux Enterprise Server 9"
#   else
                        << L"Red Hat Enterprise Linux 4"
#   endif
                        << L", the logical device " << device << L": cannot be mapped to the physical device(s) that contain it.";
                    SCX_LOG(m_log, suppressor.GetSeverity(device), out.str());
                }
                else
                {
                    for (std::vector< std::wstring >::const_iterator iter = slaves.begin();
                         iter != slaves.end(); iter++)
                    {
                        if ((*iter).empty() || !isdigit((int)(*iter)[(*iter).size() - 1]))
                        {
                            path = *iter;
                        }
                        else
                        {
                            path = GuessPhysicalFromLogicalDevice(*iter);
                        }
                        name = path.GetFilename();
                        devices[name] = path.Get();
                    }
                }
            }
        }
        catch (SCXCoreLib::SCXException& e)
        {
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
            std::wstringstream               out;

            out << L"An exception occurred resolving the physical devices that contain the LVM device " << device << L": " << e.What();
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
#elif defined(sun)
        std::vector<std::wstring> devs;

        if (device.find(L"/md/") != std::wstring::npos)
        {
            // meta devices can be built from multiple normal devices
            if (0 == m_pRaid)
            {
                SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> raidCfgParser( new SCXSystemLib::SCXRaidCfgParserDefault() );
                m_pRaid = new SCXRaid(raidCfgParser);
            }

            m_pRaid->GetDevices(name, devs);

            // rewrite the path for mapping physical devices kstat module, instance and name
            path.SetDirectory(L"/dev/dsk/");
        }
        else
        {
            // normal device
            devs.push_back(name);
        }

        for (std::vector<std::wstring>::const_iterator it = devs.begin(); it != devs.end(); ++it)
        {
            name = it->substr(0,it->find_last_not_of(L"0123456789"));

            std::wstring dev = SCXCoreLib::StrAppend(L"/dev/dsk/", *it);

            try
            {
                if (IsDiskInKstat(path.GetDirectory() + L"/" + name))
                {
                    devices[name] = dev.substr(0,dev.find_last_not_of(L"0123456789"));
                }
            }
            catch (SCXCoreLib::SCXException& )
            {

            }
        }
#endif
        return devices;
    }

#if defined(sun)

    /*----------------------------------------------------------------------------*/
    /**
       Gets the kstat name for the vopstats for the given device path.

       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0.
       \param[in]   mountpoint Mount point found in mount table.
       \returns     the kstat name for the vopstats for the given device path.
       \throws      SCXErrnoException when system calls fail.
    */
    std::wstring DiskDependDefault::GetVopstatName(const std::wstring & dev_path, const std::wstring & mountpoint)
    {
        std::wstringstream out;

        SCXStatVfs fsStats;
        if (statvfs(SCXCoreLib::StrToUTF8(mountpoint).c_str(), &fsStats) != 0)
        {
            std::wstringstream message;
            message << L"statvfs failed for device " << dev_path
                    << L" mounted at " << mountpoint;

            out << L"GetVopstatName : Error : SCXErrnoException : " << errno << L" - " << message.str();
            SCX_LOGHYSTERICAL(m_log, out.str());

            throw SCXCoreLib::SCXErrnoException(message.str(), errno, SCXSRCLOCATION);
        }

        std::wstringstream stream;
        stream << L"vopstats_" << std::noshowbase << std::nouppercase << std::hex << fsStats.f_fsid;

        out << L"GetVopstatName : Succeeded : The kstat parameters for device " << dev_path
            << L" mounted at " << mountpoint
            << L" are unix:0:" << stream.str();
        SCX_LOGHYSTERICAL(m_log, out.str());

        return stream.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Checks if the physical disk can be mapped to a kstat module, instance
       and name entry.

       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0
       \returns     true if the disk maps to a kstat entry; false otherwise.
       \throws      SCXErrnoException when system calls fail.
       \throws      SCXInternalErrorException For errors that should not occur normally.
    */
    bool DiskDependDefault::IsDiskInKstat(const std::wstring& dev_path)
    {
        static SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat(new SCXSystemLib::SCXKstat());
        static SCXCoreLib::SCXThreadLockHandle handle = SCXCoreLib::ThreadLockHandleGet(L"Guess Kstat Global");

        std::wstringstream out;
        out << L"IsDiskInKstat : Entering : dev_path: " << dev_path;
        SCX_LOGHYSTERICAL(m_log, out.str());

        std::wstring module;
        std::wstring name;
        scxlong      instance;

        if (!GuessKstatPath(dev_path, module, name, instance, true))
        {
            return false;
        }

        SCXCoreLib::SCXThreadLock lock(handle);

        kstat->Update();

        try
        {
            kstat->Lookup(module, name, static_cast<int>(instance));

            out.str(L"");
            out << L"IsDiskInKstat : Succeeded : The kstat parameters for device " << dev_path
                << L" are " << module << L":" << instance << L":" << name;
            SCX_LOGHYSTERICAL(m_log, out.str());

            return true;
        }
        catch (SCXKstatNotFoundException& exception)
        {
            // this is unexpected, and there is no fallback for physical devices
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eHysterical);

            out.str(L"");
            out << L"IsDiskInKstat : Failed : The kstat lookup failed for device " << dev_path
                << L" using the parameters " << module << L":" << instance << L":" << name
                << L" : " << typeid(exception).name()
                << L" : " << exception.What()
                << L" : " << exception.Where();
            SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());
        }

        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Given a logical device path, guess the kstat module, instance and name.

       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0
       \param[out]  module Output parameter with guessed kstat module.
       \param[out]  name Output parameter with guessed kstat name.
       \param[out]  instance Output parameter with guessed kstat instance number.
       \param[in]   isDisk true if the logical device path represents a physical
       disk device; false if it represents a purely logical device,
       such as a file system partition or volume.
       \returns     true if the kstat module, instance and name were guessed;
       false otherwise.
       \throws      SCXErrnoException when system calls fail.
       \throws      SCXInternalErrorException For errors that should not occur normally.

       This method tries to determine the the kstat module, instance and name
       for a logical device path.  This will work when the logical device path
       is a link to a physical device path in (essentially) Open Boot PROM format.
       The physical device path is then looked up in "/etc/path_to_inst".  If it
       exists there, then the kstat module, instance and name can be determined.

       In the case where the logical device path represents a disk device stored
       in Solaris CTDS format (for example, /dev/dsk/c1t0d0), this method will
       assume that the disk at least has a slice 0, and use that link to determine
       the physical device path.

       If the logical device is a pseudo device, then this method is unable to
       determine the the kstat module, instance and name.

       \note On Solaris do:
       \verbatim
       readlink(dev_path, link_path)
       find link_path (except "/devices") in "/etc/path_to_inst" and conclude kstat path.
       For logical disks use filename of link_path = <s1>@<i1>,<i2>:<s2>
       example: sd@0,0:a
       Logical disk name = <module><instance>,<s2>
       \endverbatim
    */
    bool DiskDependDefault::GuessKstatPath(const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isDisk)
    {
        std::wstringstream out;
        out << L"GuessKstatPath : Entering : dev_path: " << dev_path << L", isDisk: " << (isDisk ? L"true" : L"false");
        SCX_LOGHYSTERICAL(m_log, out.str());

        // If this is a disk device path in apparent CTDS format, then use the
        // link to slice 0 to find the physical device path.
        std::wstring dpath = dev_path;
        if (isDisk &&
            dpath.substr(dpath.rfind(L"/")+1,1) == L"c")
        {
            dpath += L"s0"; //Assume at least one partition.

            out.str(L"");
            out << L"GuessKstatPath :: Assuming slice 0 exists for CTDS disk device path " << dpath;
            SCX_LOGHYSTERICAL(m_log, out.str());
        }

        char buf[1024];
        memset(buf, 0, sizeof(buf));
        if (-1 == readlink(SCXCoreLib::StrToUTF8(dpath).c_str(), buf, sizeof(buf)))
        {
            std::wstringstream message;
            message << L"readlink failed for " << (isDisk ? L"disk" : L"logical") << L" device path "
                    << dpath;

            out.str(L"");
            out << L"GuessKstatPath : Error : SCXErrnoException : " << errno << L" - " << message.str();
            SCX_LOGHYSTERICAL(m_log, out.str());

            throw SCXCoreLib::SCXErrnoException(message.str(), errno, SCXSRCLOCATION);
        }

        SCXCoreLib::SCXFilePath link_path(SCXCoreLib::StrFromUTF8(buf));
        if (link_path.GetDirectory().find(L"pseudo") != std::wstring::npos)
        {
            // there is no way to determine the the kstat module, instance and
            // name for a pseudo device
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eHysterical);

            out.str(L"");
            out << L"GuessKstatPath : Failed : Unable to determine kstat lookup parameters for "
                << (isDisk ? L"disk" : L"logical") << L" pseudo device " << dpath;
            SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());

            return false;
        }
        else
        {
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(link_path.GetFilename(), parts, L":");
            if (parts.size() != 2)
            {
                std::wstringstream message;
                message << L"The physical device link is not in the expected format: "
                        << dpath << L" -> " << link_path.Get();

                out.str(L"");
                out << L"GuessKstatPath : Error : SCXInternalErrorException :  - " << message.str();
                SCX_LOGHYSTERICAL(m_log, out.str());

                throw SCXCoreLib::SCXInternalErrorException(message.str(), SCXSRCLOCATION);
            }

            link_path.SetFilename(parts[0]);

            // Remove any ".." or "devices" in link path beginning
            while (link_path.GetDirectory().substr(0,1) == L"/")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(1));
            }
            while (link_path.GetDirectory().substr(0,3) == L"../")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(3));
            }
            while (link_path.GetDirectory().substr(0,7) == L"devices")
            {
                link_path.SetDirectory(link_path.GetDirectory().substr(7));
            }

            std::wstring path_to_inst = link_path.Get();

            SCXCoreLib::SCXHandle<DeviceInstance> di = FindDeviceInstance(path_to_inst);
            if (0 == di)
            {
                // there is no entry in /etc/path_to_inst
                static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eHysterical);

                out.str(L"");
                out << L"GuessKstatPath : Failed : Cannot find physical device path instance "
                    << path_to_inst
                    << L" for " << (isDisk ? L"disk" : L"logical") << L" device " << dpath;
                SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());

                return false;
            }

            module   = di->m_name;
            instance = di->m_instance;

            std::wstringstream stream;
            stream << module << instance;
            if (!isDisk)
            {
                stream << L"," << parts[1];
            }

            name = stream.str();

            out.str(L"");
            out << L"GuessKstatPath : Succeeded : The best guess kstat parameters for device " << dev_path
                << L" assuming physical device path instance " << path_to_inst
                << L" are " << module << L":" << instance << L":" << name;
            SCX_LOGHYSTERICAL(m_log, out.str());

            return true;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Given a device path, guess the vopstat name using the file system ID given
       in the device attributes from the mount table.

       \param[in]   dev_path Path to device ex: /dev/dsk/c0t0d0s0
       \param[in/out]  vopstat on input, this is the name of the previously tried
       vopstat name, on output this is an untried vopstat name.

       \returns     true if a new vopstat name was guessed; false otherwise.

       Logical devices on S10+ use vopstats, which are based on the devices file
       system ID.  Prior to this point, an attempt has already been made to create
       the vopstat name using the file system ID obtained by calling statvfs for
       dev_path.  It is possible, but unlikely that there is a different file
       system ID for the device in the mount table attributes.

       Note: This is mainly here for extra paranoia.  The old code path did this,
       so we continue to do it because we can't test all possible Solaris
       file system configurations.  It seems unlikely that this would ever
       return a different value than that returned by GetVopstatName.
    */
    bool DiskDependDefault::GuessVopstat(const std::wstring& dev_path, std::wstring& vopstat)
    {
        std::wstringstream out;
        out << L"GuessVopstat : Entering : dev_path: " << dev_path << L", previously tried vopstat: " << vopstat;
        SCX_LOGHYSTERICAL(m_log, out.str());

#if (PF_MAJOR == 5 && PF_MINOR <= 9)
        // vopstats are only supported on S10+
        out.str(L"");
        out << L"GuessVopstat : Failed : Support for vopstat only available in Solaris 10 and higher.";
        SCX_LOGHYSTERICAL(m_log, out.str());

        return false;
#endif

        for (std::vector<MntTabEntry>::const_iterator mtab_it = GetMNTTab().begin();
             mtab_it != GetMNTTab().end(); mtab_it++)
        {
            if (mtab_it->device == dev_path)
            {
                std::wstring nameFromAttr = L"vopstats_" + mtab_it->devAttribute;
                if (nameFromAttr != vopstat)
                {
                    vopstat = nameFromAttr;

                    out.str(L"");
                    out << L"GuessVopstat : Succeeded : The last guess kstat parameters for device " << dev_path
                        << L" are unix:0:" << vopstat;
                    SCX_LOGHYSTERICAL(m_log, out.str());

                    return true;
                }

                out.str(L"");
                out << L"GuessVopstat : Failed : Out of guesses.";
                SCX_LOGHYSTERICAL(m_log, out.str());

                return false;
            }
        }

        out.str(L"");
        out << L"GuessVopstat : Failed : The mount table does not contain device attributes for " << dev_path;
        SCX_LOGHYSTERICAL(m_log, out.str());

        return false;
    }

#endif

    /*----------------------------------------------------------------------------*/
    /**
       Removes all numbers or one other character from end of given string.

       \param[in]   str Input string.
       \returns     The input string where the last character is removed or if the
       string ends with numbers all ending numbers are removed.

    */
    std::wstring DiskDependDefault::RemoveTailNumberOrOther(const std::wstring& str)
    {
        if (str.length() == 0)
        {
            return str;
        }
        std::wstring result = str;
        if (str.find_last_of(L"0123456789") == (str.length() - 1))
        {
            while (result.length() > 0 && result.find_last_of(L"0123456789") == (result.length() - 1))
            {
                result.resize(result.length() - 1);
            }
        }
        else
        {
            result.resize(result.length() - 1);
        }
        return result;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Given a logical disk device path, returns a device path to physical device
       on which tyhe logical device should be residing.

       \param[in]   logical_dev Path to logical disk device.
       \returns     physical_dev Path to physical disk device.

       \note This algorithm is based on the fact that physical disks resides in the same
       folder as logical disks. It also assumes that logical disk names in Linux format
       (partitions/slices) have names that are [physical device] followed by one or more digits
       and that logical disk names in Solaris format have names that are "c#d#{p,s}##" or
       "c#t#d#{p,s}##", which is what we've observed so far.
    */
    std::wstring DiskDependDefault::GuessPhysicalFromLogicalDevice(const std::wstring& logical_dev)
    {
        std::wstring physical_dev = logical_dev;
        SCXCoreLib::SCXFilePath path(physical_dev);
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXRegex> SolarisPartitionNamePatternRegExPtr(NULL);
        static wchar_t SolarisPartitionNamePattern[] = { L"c[0-9]+(t[0-9])?d[0-9][ps][0-9]+" };

        // Build the RegEx template
        try
        {
            SolarisPartitionNamePatternRegExPtr = new SCXCoreLib::SCXRegex(SolarisPartitionNamePattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return L"";
        }

        if (SolarisPartitionNamePatternRegExPtr->IsMatch(path.GetFilename()))
        {    // remove the "p#", "s#", "p##" or "s##" from the end of the name
            physical_dev = RemoveTailNumberOrOther(physical_dev);
            physical_dev = physical_dev.substr(0, physical_dev.size() - 1);
            if (this->FileExists(physical_dev))
            {
                return physical_dev;
            }
            return logical_dev;
        }
        else
        {
            while (path.GetFilename().length() > 0)
            {
                physical_dev = RemoveTailNumberOrOther(physical_dev);
                path = physical_dev;
                if (this->FileExists(physical_dev) && path.GetFilename().length() > 0)
                {
                    return physical_dev;
                }
            }
            return logical_dev;
        }
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::ReadKstat
    */
    bool DiskDependDefault::ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, const std::wstring& mountpoint, bool isDisk)
    {
        std::wstringstream out;
        out << L"ReadKstat : Entering : dev_path: " << dev_path << L", mountpoint: " << mountpoint << L", isDisk: " << (isDisk ? L"true" : L"false");
        SCX_LOGHYSTERICAL(m_log, out.str());

        bool isKstatUpdated = false;
        int  tries          = 0;

#if ((PF_MAJOR == 5 && PF_MINOR > 9) || (PF_MAJOR > 5))
        std::wstring vopstat;

        if (!isDisk)
        {
            vopstat = GetVopstatName(dev_path, mountpoint);

            kstat->Update();
            isKstatUpdated = true;

            try
            {
                tries++;
                kstat->Lookup(L"unix", vopstat, 0);

                out.str(L"");
                out << L"ReadKstat : Succeeded : The file system kstat parameters for device " << dev_path
                    << L" mounted at " << mountpoint
                    << L" are unix:0:" << vopstat;
                SCX_LOGHYSTERICAL(m_log, out.str());

                return true;
            }
            catch (SCXKstatNotFoundException& exception)
            {
                // all devices on S10+ are expected to have vopstats, so log once as Informational
                // and then only hysterically
                static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eInfo, SCXCoreLib::eHysterical);

                out.str(L"");
                out << L"ReadKstat :: The kstat lookup failed for device " << dev_path
                    << L" mounted at " << mountpoint
                    << L" using the file system parameters unix:0:" << vopstat
                    << L" : " << typeid(exception).name()
                    << L" : " << exception.What()
                    << L" : " << exception.Where();
                SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());
            }
        }
#endif

        std::wstring module;
        std::wstring name;
        scxlong      instance;

        if (GuessKstatPath(dev_path, module, name, instance, isDisk))
        {
            if (!isKstatUpdated)
            {
                kstat->Update();
                isKstatUpdated = true;
            }

            try
            {
                tries++;
                kstat->Lookup(module, name, static_cast<int>(instance));

                out.str(L"");
                out << L"ReadKstat : Succeeded : The device instance kstat parameters for device " << dev_path
                    << L" are " << module << L":" << instance << L":" << name;
                SCX_LOGHYSTERICAL(m_log, out.str());

                return true;
            }
            catch (SCXKstatNotFoundException& exception)
            {
                // this is unexpected, and the fallback (GuessVopstat) is no longer
                // expected to succeed
                static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eHysterical);

                out.str(L"");
                out << L"ReadKstat :: The kstat lookup failed for device " << dev_path
                    << L" using the device instance parameters " << module << L":" << instance << L":" << name
                    << L" : " << typeid(exception).name()
                    << L" : " << exception.What()
                    << L" : " << exception.Where();
                SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());
            }
        }

#if ((PF_MAJOR == 5 && PF_MINOR > 9) || (PF_MAJOR > 5))
        if (!isDisk)
        {
            if (GuessVopstat(dev_path, vopstat))
            {
                if (!isKstatUpdated)
                {
                    kstat->Update();
                    isKstatUpdated = true;
                }

                try
                {
                    tries++;
                    kstat->Lookup(L"unix", vopstat, 0);

                    out.str(L"");
                    out << L"ReadKstat : Succeeded : The fallback kstat parameters for device " << dev_path
                        << L" are unix:0:" << vopstat;
                    SCX_LOGHYSTERICAL(m_log, out.str());

                    return true;
                }
                catch (SCXKstatNotFoundException& exception)
                {
                    out.str(L"");
                    out << L"ReadKstat :: The kstat lookup failed for device " << dev_path
                        << L" using the fallback parameters unix:0:" << vopstat
                        << L" : " << typeid(exception).name()
                        << L" : " << exception.What()
                        << L" : " << exception.Where();
                    SCX_LOGHYSTERICAL(m_log, out.str());
                }
            }
        }
#endif

        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eHysterical);

        out.str(L"");
        if (tries > 1)
        {
            out << L"ReadKstat : Failed : After trying " << tries << L" strategies, "
                << L"the system was unable to determine the kstat lookup parameters for "
                << (isDisk ? L"disk" : L"logical")
                << L" device " << dev_path;
        }
        else
        {
            out << L"ReadKstat : Failed : Cannot determine the kstat lookup parameters for "
                << (isDisk ? L"disk" : L"logical")
                << L" device " << dev_path;
        }

        SCX_LOG(m_log, suppressor.GetSeverity(dev_path), out.str());

        return false;
    }
#endif /* sun */

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::AddDeviceInstance
    */
    void DiskDependDefault::AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID)
    {
        SCXCoreLib::SCXHandle<DeviceInstance> di( new DeviceInstance() );
        di->m_name = name;
        di->m_devID = devID;
        di->m_instance = instance;
        m_deviceMap[device] = di;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::FindDeviceInstance
    */
    SCXCoreLib::SCXHandle<DeviceInstance> DiskDependDefault::FindDeviceInstance(const std::wstring& device) const
    {
        if (m_deviceMap.find(device) != m_deviceMap.end())
        {
            return m_deviceMap.find(device)->second;
        }
        return SCXCoreLib::SCXHandle<DeviceInstance>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns true if the string is found in the array.  Uses set find for equality comparison
       \param str String to look for.
       \param arrSet set of strings to search.
       \returns true if the string is found in the array or otherwise false.
       \throws SCXInvalidArgumentException if arr or compare is NULL.
    */
    bool DiskDependDefault::IsStringInSet(const std::wstring& str,
                                          const std::set<std::wstring>& arrSet)
    {
        if (arrSet.find(str) != arrSet.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns true if the string is found in the array.
       \param str String to look for.
       \param arrSet set of strings to search.
       \param compare A compare function to be used when determine if string is in set.
       \returns true if the string is found in the array or otherwise false.
       \throws SCXInvalidArgumentException if arr or compare is NULL.
    */
    bool DiskDependDefault::IsStringInSet(const std::wstring& str,
                                          const std::set<std::wstring>& arrSet,
                                          CompareFunction compare)
    {
        if (NULL == compare)
        {
            throw SCXCoreLib::SCXInvalidArgumentException(L"compare",L"Should never be NULL", SCXSRCLOCATION);
        }
        for (std::set<std::wstring>::const_iterator it = arrSet.begin(); it != arrSet.end(); ++it)
        {
            if  (compare(str, *it))
            {
                return true;
            }
        }

        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle equals heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle equals heystack. Otherwise false.
       Example:
    */
    bool DiskDependDefault::CompareEqual(const std::wstring& needle, const std::wstring& heystack)
    {
        return needle == heystack;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle starts with the heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle starts with heystack. Otherwise false.
    */
    bool DiskDependDefault::CompareStartsWith(const std::wstring& needle, const std::wstring& heystack)
    {
        return needle.substr(0, heystack.length()) == heystack;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if needle contains heystack.
       \param needle a string
       \param heystack a string
       \returns true if needle contains heystack. Otherwise false.
    */
    bool DiskDependDefault::CompareContains(const std::wstring& needle, const std::wstring& heystack)
    {
        return needle.find(heystack) != std::wstring::npos;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::FileExists
    */
    bool DiskDependDefault::FileExists(const std::wstring& path)
    {
        SCXCoreLib::SCXFileInfo fi(path);
        return fi.PathExists();
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDepend::SetDevTabPath
    */
    void DiskDependDefault::SetDevTabPath(const std::wstring& newValue)
    {
        m_DevTabPath = newValue;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::DiskDependDefault::LocateDevTab
    */
    const SCXCoreLib::SCXFilePath& DiskDependDefault::LocateDevTab()
    {
        return m_DevTabPath;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc read mount table attr options content
       \returns false if error happen.
    */
    void DiskDependDefault::readMNTTab(std::vector<std::wstring>& mntOptions)
    {
#if defined(aix)
        // Not implemented platform
        throw SCXCoreLib::SCXNotSupportedException(L"readMNTTab", SCXSRCLOCATION);
#else

        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(
                                                    LocateMountTab(), std::ios::in));
        fs.SetOwner();
        while ( ! fs->eof() && fs->is_open() )
        {
            std::wstring line;
            std::vector<std::wstring> parts;
            getline( *fs, line );
            SCXCoreLib::StrTokenize(line, parts, L" \n\t");
            if (parts.size() > 3)
            {
                if (std::wstring::npos != parts[0].find('#')) // Comment
                {
                    continue;
                }
                mntOptions.push_back(parts[3]);
            }
        }
        fs->close();
#endif
    }
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
