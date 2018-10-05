/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
   \file        

   \brief       Defines the dependency interface for disk data retrieval
    
   \date        2008-03-19 11:42:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef DISKDEPEND_H
#define DISKDEPEND_H

#if defined(aix)
#include <libperfstat.h>
#include <sys/mntctl.h>
#include <lvm.h>
#elif defined(hpux)
#include <sys/pstat.h>
#include <mntent.h>
#elif defined(sun)
#include <scxsystemlib/scxkstat.h>
#endif
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/logsuppressor.h>
#include <scxsystemlib/scxlvmtab.h>
#include <scxsystemlib/scxraid.h>
#include <scxcorelib/scxdirectoryinfo.h>
#if defined(aix)
#include <scxsystemlib/scxodm.h>
#endif
#include <map>
#include <set>

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>

namespace SCXSystemLib
{
    /**
       Common data type for 64-bit statvfs() system call
    */
    typedef struct statvfs64  SCXStatVfs;

    /**
       The types of disks interfaces recognized by the disk PAL.

    */
    enum DiskInterfaceType
    {
        eDiskIfcUnknown = 0,
        eDiskIfcIDE,
        eDiskIfcSCSI,
        eDiskIfcVirtual,
        eDiskIfcMax
    };

    /**
       The types of disks availability attribute
    */
    enum DiskAvailabilityType
    {
        eDiskAvaInvalid = 0xFFFF,       //!< indicate it is a invalid value, not defined in WMI
        eDiskAvaOther   = 0x01,         //!< 1   (0x1), Other
        eDiskAvaUnknown,                //!< 2   (0x2), Unknown
        eDiskAvaRunningOrFullPower,     //!< 3   (0x3), Running or Full Power
        eDiskAvaWarning,                //!< 4   (0x4), Warning
        eDiskAvaInTest,                 //!< 5   (0x5), In Test
        eDiskAvaNotApplicable,          //!< 6   (0x6), Not Applicable
        eDiskAvaPowerOff,               //!< 7   (0x7), Power Off
        eDiskAvaOffLine,                //!< 8   (0x8), Off Line
        eDiskAvaOffDuty,                //!< 9   (0x9), Off Duty
        eDiskAvaDegraded,               //!< 10  (0xA), Degraded
        eDiskAvaNotInstalled,           //!< 11  (0xB), Not Installed
        eDiskAvaInstallError,           //!< 12  (0xC), Install Error
        eDiskAvaPowerSave_Unknown,      //!< 13  (0xD), Power Save - Unknown.
        eDiskAvaPowerSave_LowPowerMode, //!< 14  (0xE), Power Save - Low Power Mode.
        eDiskAvaPowerSave_Standby,      //!< 15  (0xF), Power Save - Standby.
        eDiskAvaPowerCycle,             //!< 16  (0x10), Power Cycle
        eDiskAvaPowerSave_Warning,      //!< 17  (0x11), Power Save - Warning.
        eDiskAvaPowerCnt                //!< count of defined type in WMI
    };
    /**
       The types of disk capabilities
    */
    enum DiskCapabilitiesType
    {
        eDiskCapInvalid = 0xFFFF,                     //!< invalid type
        eDiskCapUnknown = 0,                          //!< 0, Unknown
        eDiskCapOther,                                //!< 1, Other
        eDiskCapSequentialAccess,                     //!< 2, Sequential Access
        eDiskCapRandomAccess,                         //!< 3, Random Access
        eDiskCapSupportsWriting,                      //!< 4, Supports Writing
        eDiskCapEncryption,                           //!< 5, Encryption
        eDiskCapCompression,                          //!< 6, Compression
        eDiskCapSupportsRemovableMedia,               //!< 7, Supports Removable Media
        eDiskCapManualCleaning,                       //!< 8, Manual Cleaning
        eDiskCapAutomaticCleaning,                    //!< 9, Automatic Cleaning
        eDiskCapSMARTNotification,                    //!< 10, SMART Notification
        eDiskCapSupportsDualSidedMedia,               //!< 11, Supports Dual-Sided Media
        eDiskCapEjectPriortoDriveDismountNotRequired, //!< 12, Ejection Prior to Drive Dismount Not Required
        eDiskCapCnt                                   //!< supported Capabilities type countnow
    };
    /**
       capabilitiy descriptions array
    */
    static const std::wstring capabilityDescriptions[eDiskCapCnt] =
    {
        L"Unknown",                         //!< 0, Unknown
        L"Other",                           //!< 1, Other
        L"Sequential Access",               //!< 2, Sequential Access
        L"Random Access",                   //!< 3, Random Access
        L"Supports Writing",                //!< 4, Supports Writing
        L"Encryption",                      //!< 5, Encryption
        L"Compression",                     //!< 6, Compression
        L"Supports Removable Media",        //!< 7, Supports Removable Media
        L"Manual Cleaning",                 //!< 8, Manual Cleaning
        L"Automatic Cleaning",              //!< 9, Automatic Cleaning
        L"SMART Notification",              //!< 10, SMART Notification
        L"Supports Dual-Sided Media",       //!< 11, Supports Dual-Sided Media
        L"Ejection Prior to Drive Dismount Not Required" //!< 12, Ejection Prior to Drive Dismount Not Required
    };

    /**
       Type of media used or accesed by this device.
    */
    static const std::wstring mediaTypeNames[] =
    {
        L"External hard disk media",          //!< External hard disk media, can't determine it now
        L"Removable media other than floppy", //!< Removable media other than floppy
        L"Fixed hard disk media",             //!< Fixed hard disk media
        L"Format is unknown"                  //!< Format is unknown
    };

    /** Represents a device instance. */
    struct DeviceInstance
    {
        std::wstring    m_name;     //!< Instance name.
        scxlong      m_instance; //!< Instance number.
        scxlong      m_devID;    //!< Device ID.
    };

    /** Represents a single row in /etc/mtab (/etc/mnttab) */
    struct MntTabEntry
    {
        std::wstring    device;       //!< Device path
        std::wstring    fileSystem;   //!< File system name
        std::wstring    mountPoint;   //!< Mount point (root) of file system.
        std::wstring    devAttribute; //!< Device attribute value (or empty if no such attribute).
    };

    /*----------------------------------------------------------------------------*/
    /**
       Define the interface for disk dependencies.    
    */
    class DiskDepend
    {
    public:
        static const scxlong s_cINVALID_INSTANCE; //!< Constant representing an invalid instance.

        virtual ~DiskDepend() { } //!< Virtual destructor

        /**
           Get the path to mount tab file.

           \returns path to mount tab file.
        */
        virtual const SCXCoreLib::SCXFilePath& LocateMountTab() = 0;

        /**
           Get the path to the diskstats file.

           \returns the path to the diskstats file.
        */
        virtual const SCXCoreLib::SCXFilePath& LocateProcDiskStats() = 0;

        /**
           Refresh the disk stats file cache.
        */
        virtual void RefreshProcDiskStats() = 0;

        /**
           Get the path to the partitions file.

           \returns the path to the partitions file.
        */
        virtual const SCXCoreLib::SCXFilePath& LocateProcPartitions() = 0;

        /**
           Get a proc disk stats row.

           \param device The device we want statistics for.
           \returns A vector with the stats tokenized as a vector of strings.
        */
        virtual const std::vector<std::wstring>& GetProcDiskStats(const std::wstring& device) = 0;

        /**
           Get a list of files in a directory.

           \param[in] path Path to a directory.
           \param[out] files A vector of file paths to all files in the given directory.

           \note The files vector is cleared and will be empty if the given directory does
           not exist.
        */
        virtual void GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files) = 0;

        /**
           Get a parsed version of lvmtab.

           \returns a SCXLvmTab object.        
        */
        virtual const SCXLvmTab& GetLVMTab() = 0;

        /**
           Get a parsed version of mount tab.

           \returns a vector of MntTabEntry objects.
        */ 
        virtual const std::vector<MntTabEntry>& GetMNTTab() = 0;

        /**
           Refresh the mount tab state.
        */ 
        virtual void RefreshMNTTab() = 0;

#if defined(sun)
        /**
           Set the path to dev tab file.

           \returns Nothing, but sets path to file.
        */
        virtual void SetDevTabPath(const std::wstring& newValue) = 0;
        /**
           Return the path to dev tab file.

           \returns Path to file.
        */
        virtual const SCXCoreLib::SCXFilePath&  LocateDevTab() = 0;
        /**
           Return vector of system files from /dev/dsk.

           \returns vector of system files from /dev/dsk.
        */
        virtual std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > GetDevDskInfo() = 0;
#endif

        /**
           Check if a given file system should be ignored or not.

           \param       fs Name of file system.
           \returns     true if the file system should be ignored, otherwise false.

           Ignored file systems are file systems we know we will not want to monitor.
           For example CD/DVD devices, system devices etc.
        */
        virtual bool FileSystemIgnored(const std::wstring& fs) = 0;

        /**
           Checks if the given device should be given in the given enumeration.

           \param[in] device  the device to check.

           \return This method will return true if the given device should be
           ignored; false otherwise.

           Devices may be ignored because they are know to cause problems.  For
           example CD/DVD devices on Solaris, and LVM on old Linux distributions.
        */
        virtual bool DeviceIgnored(const std::wstring& device) = 0;

        /**
           Check if a given file system is represented by 'known' physical device.
           in mnttab file. Currently, we do not know how to get list of
           physical device(s) for ZFS filesystem, there is also the issue that
           on for example a solaris zone there is no physical disk so the info
           in mnttab is the same for device and mountpoint.

           \param       fs Name of file system.
           \param       dev_path Device path fount in mount table.
           \param       mountpoint Mount point found in mount table.
           \returns     true if the file system has 'real' device in mnt-tab.
        */
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint) = 0;

        /**
           Decide interface type from the device name.

           \param dev device name.
           \returns a disk interface type.
        */
        virtual DiskInterfaceType DeviceToInterfaceType(const std::wstring& dev) const = 0;

        /**
           Given a device path from mount tab file, return related physical devices.

           \param device A device path as found in mount tab file.
           \returns A string map (name -> device path) with all physical devices 
           related to given device.

           Several devices may be returned if the device for example is a logical volume.
        */
        virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& device) = 0;

#if defined(sun)
        /**
           Read a kstat object from a disk device path, i.e. a logical device
           path that has had the slice information truncated.

           \param kstat Kstat object to use when reading.
           \param dev_path Path to a disk device, ex: /dev/dsk/c0t0d0
           \returns true if the read was successful, otherwise false.
        */
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path) = 0;

        /**
           Read a kstat object for a mounted file system.

           \param kstat Kstat object to use when reading.
           \param dev_path Path to the logical device, ex: /dev/dsk/c0t0d0s0
           \param mountpoint Mount point found in mount table.
           \returns true if the read was successful, otherwise false.
        */
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, const std::wstring& mountpoint) = 0;
#endif
        /**
           Add a device instance to the device instance cache.

           \param device Device path
           \param name Device name
           \param instance Device instance number
           \param devID Device ID number

           Typically used to cache information needed to create KStat paths.
        */
        virtual void AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID) = 0;

        /**
           Find a device instance in the device instance cache.

           \param device Device path searched for.
           \returns A pointer to a device instance object or zero if no object is found in the cache.
        */
        virtual SCXCoreLib::SCXHandle<DeviceInstance> FindDeviceInstance(const std::wstring& device) const = 0; 

        /**
           Wrapper for the system call open.

           \param pathname Path to file to open.
           \param flags open flags.
           \returns true success, false fail.
        */
        virtual bool open(const char* pathname, int flags) = 0;

        /**
           Wrapper for system call close.

           \returns Zero on success, otherwise -1.
        */
        virtual int close(void) = 0;

        /**
           Wrapper for the system call ioctl.

           \param request Type of ioctl request.
           \param data Data related to the request.
           \returns -1 for errors.
        */
        virtual int ioctl(unsigned long int request, void* data) = 0;

        /**
           Wrapper for the system call read.

           \param request Type of ioctl request.
           \param data Data related to the request.
           \returns -1 for errors.
        */
        virtual ssize_t read(void *pbuf, size_t bytecount) = 0;

        /** 
            Wrapper for the system call statvfs.

            \param path Path to file to check.
            \param buf statvfs structure to fill with result.
            \returns zero on success, otherwise -1.
        */
        virtual int statvfs(const char* path, SCXStatVfs* buf) = 0;

        /**
           Wrapper for the system call lstat.

           \param path Path to file to stat.
           \param buf stat structure to fill with result.
           \returns zero on success, otherwise -1.
        */
        virtual int lstat(const char* path, struct stat *buf) = 0;

        /**
           Wrapper for file exists calls to static method.

           \param path Path to test if it exists.
           \returns true if path exists, otherwise false.
        */
        virtual bool FileExists(const std::wstring& path) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Get monut table options value list content.
    
           \param       value list.
        */
        virtual void readMNTTab(std::vector<std::wstring>& mntOptions) = 0;

#if defined(hpux)
        /**
           Wrapper for the system call open.

           \param pathname Path to file to open.
           \param flags Open flags.
           \returns On success returns file descriptor, otherwise returns -1 and sets errno.
        */
        virtual int _open(const char* pathname, int flags) = 0;

        /**
           Wrapper for system call close.

           \param fd File descriptor of the device.             
           \returns On success returns 0. On error returns -1 and sets errno.
        */
        virtual int _close(int fd) = 0;

        /**
           Wrapper for the system call ioctl.

           \param fd File descriptor of the device.             
           \param request Type of ioctl request.
           \param data Data related to the request.
           \returns On error returns -1 and sets errno.
        */
        virtual int _ioctl(int fd, int request, void* data) = 0;

        /**
           Wrapper for system call pstat_getdisk

           \param buf Pointer to one or more pst_diskinfo structure.
           \param elemsize Size of each element (size of the struct).
           \param elemcount Number of elements to retreive (number of structs pointed to).
           \param index Element offset.
           \returns -1 on failure or number of elements returned.
        */
        virtual int pstat_getdisk(struct pst_diskinfo* buf, size_t elemsize, size_t elemcount, int index) = 0;

        /**
           Wrapper for system call pstat_getlv

           \param buf Pointer to one or more pst_lvinfo structure.
           \param elemsize Size of each element (size of the struct).
           \param elemcount Number of elements to retreive (number of structs pointed to).
           \param index Element offset.
           \returns -1 on failure or number of elements returned.
        */
        virtual int pstat_getlv(struct pst_lvinfo* buf, size_t elemsize, size_t elemcount, int index) = 0;

        /**
           Wrapper for system call setmntent

           \param path Pointer to the name of the file to be opened.
           \param type File access mode.
           \returns File pointer which can be used with getmntent() and endmntent(), NULL on error.
        */
        virtual FILE *setmntent(const char *path, const char *type) = 0;

        /**
           Wrapper for system call getmntent

           \param stream File pointer returned by setmntent().
           \returns NULL on error or EOF, otherwise pointer to a mntent structure.
        */
        virtual struct mntent *getmntent(FILE *stream) = 0;

        /**
           Wrapper for system call endmntent

           \param stream File pointer returned by setmntent().
           \returns 1 and unlocks the file if it was locked by setmntent().
        */
        virtual int endmntent(FILE *stream) = 0;
        /**
           Wrapper for system call stat

           \param path pointer to a file path name.
           \param buf pointer to a stat structure into which file information is placed.
           \returns 0 on success, otherwise -1 and sets errno.
        */
        virtual int stat(const char *path, struct stat *buf) = 0;
#endif /* hpux */
#if defined(aix)
        /**
           Wrapper for system call perfstat_disk.

           \param name first perfstat ID wanted and next is returned. 
           \param buf Buffer to hold returned data.
           \param struct_size Size of the struct returned.
           \param n Desired number of structs to return.
           \returns Number of structures returned.
        */
        virtual int perfstat_disk(perfstat_id_t* name, perfstat_disk_t* buf, size_t struct_size, int n) = 0;

        /**
           Wrapper for system call mntctl.

           \param command Command to perform.
           \param size Size of buffer.
           \param buf Buffer to fill with data.
           \returns Number of vmount structures copied into the buffer
        */
        virtual int mntctl(int command, int size, char* buf) = 0;

        /**
           Creates a new SCXodm object. Provided for dependency injection purposes.

           \returns     Handle to newly created SCXodm class.
        */
        virtual const SCXCoreLib::SCXHandle<SCXodm> CreateOdm(void) const =0;

        /**
           Queries volume groups.

           \param queryVGS Pointer to the queryvgs structure.
           \param kmid Entry point of the logical volume device driver. For backward compatibility. Can be set to 0.
           \returns On success returns 0, otherwise one of the error codes: LVM_ALLOCERR, LVM_INVALID_PARAM or
           LVM_INVCONFIG.
        */
        virtual int lvm_queryvgs(struct queryvgs **queryVGS, mid_t kmid) = 0;

        /**
           Queries particular volume group.

           \param vgId Id of the volume group to be queried.
           \param queryVG Pointer to the queryvg structure.
           \param pvName Name of the physical drive containing the descriptor area. If NULL, then function returns in
           memory data for varied on devices.
           \returns On success returns 0, otherwise one of the error codes: LVM_ALLOCERR, LVM_INVALID_PARAM,
           LVM_FORCEOFF or LVM_OFFLINE.
        */
        virtual int lvm_queryvg(struct unique_id *vgId, struct queryvg **queryVG, char *pvName) = 0;

        /**
           Queries particular volume group.

           \param lvId Id of the logical volume to be queried.
           \param queryLV Pointer to the querylv structure.
           \param pvName Name of the physical drive containing the descriptor area. If NULL, then function returns in
           memory data for varied on devices.
           \returns On success returns 0. For list of all error codes see AIX documentation at http://www.ibm.com/us/en/.
        */
        virtual int lvm_querylv(struct lv_id *lvId, struct querylv **queryLV, char *pvName) = 0;
#endif /* aix */
#if defined(aix) || defined(sun)

        /** 
            Wrapper for the system call statvfs64.

            \param path Path to file to check.
            \param buf statvfs64 structure to fill with result.
            \returns zero on success, otherwise -1.
        */
        virtual int statvfs64(const char* path, struct statvfs64* buf) = 0;

#endif

        /**
           Wrapper for SCXProcess::Run()
           \param[in]  command     Corresponding to what would be entered by a user in a command shell
           \param[in]  mystdin     stdin for the process
           \param[in]  mystdout    stdout for the process
           \param[in]  mystderr    stderr for the process
           \param[in]  timeout     Accepted number of millieconds to wait for return
           \param[in]  cwd         Directory to be set as current working directory for process.
           \param[in]  chrootPath  Directory to be used for chrooting process.
           \returns exit code of run process.
           \throws     SCXInternalErrorException           Failure that could not be prevented
        */
        virtual int Run(const std::wstring &command, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr,
                        unsigned timeout, const SCXCoreLib::SCXFilePath& cwd = SCXCoreLib::SCXFilePath(),
                        const SCXCoreLib::SCXFilePath& chrootPath  = SCXCoreLib::SCXFilePath() ) = 0;

#if defined(linux)
        /**
           Returns an input stream.

           \param name of the stream to be returned. This is usualy a filename.
           \returns stream identified by the name.
        */
        virtual SCXCoreLib::SCXHandle<std::wistream> GetWIStream(const char *name) = 0;
#endif
    protected:
        DiskDepend() { } //!< Protected default constructor
    };

    /*----------------------------------------------------------------------------*/
    /**
       Implement default behaviour for DiskDepend.
    */
    class DiskDependDefault : public DiskDepend
    {
    private:
        void InitializeObject();
    public:
        DiskDependDefault();
        DiskDependDefault(const SCXCoreLib::SCXLogHandle& log);
        virtual ~DiskDependDefault();

        virtual const SCXCoreLib::SCXFilePath& LocateMountTab();
        virtual const SCXCoreLib::SCXFilePath& LocateProcDiskStats();
        virtual const SCXCoreLib::SCXFilePath& LocateProcPartitions();
        virtual void RefreshProcDiskStats();
        virtual const std::vector<std::wstring>& GetProcDiskStats(const std::wstring& device);
        virtual void GetFilesInDirectory(const std::wstring& path, std::vector<SCXCoreLib::SCXFilePath>& files);
        virtual const SCXLvmTab& GetLVMTab();
        virtual const std::vector<MntTabEntry>& GetMNTTab();
        virtual void RefreshMNTTab();
#if defined(sun)
        virtual void SetDevTabPath(const std::wstring& newValue); 
        virtual const SCXCoreLib::SCXFilePath& LocateDevTab();
        virtual std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > GetDevDskInfo()
        {
            SCXCoreLib::SCXDirectoryInfo oDisks( L"/dev/dsk/" );
            return oDisks.GetSysFiles();
        }
#endif  
        virtual bool FileSystemIgnored(const std::wstring& fs);
        virtual bool DeviceIgnored(const std::wstring& device);
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint);
        virtual bool LinkToPhysicalExists(const std::wstring& fs, const std::wstring& dev_path, const std::wstring& mountpoint, SCXCoreLib::LogSuppressor& suppressor);
        virtual DiskInterfaceType DeviceToInterfaceType(const std::wstring& dev) const;
        virtual std::map<std::wstring, std::wstring> GetPhysicalDevices(const std::wstring& device);

        /*----------------------------------------------------------------------------*/
        /**
           Get monut table options value list content.
    
           \param       value list.
        */
        virtual void readMNTTab(std::vector<std::wstring>& mntOptions);
#if defined(sun)
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path)
        {
            return ReadKstat(kstat, dev_path, L"", true);
        }

        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, const std::wstring& mountpoint)
        {
            return ReadKstat(kstat, dev_path, mountpoint, false);
        }
#endif
        virtual void AddDeviceInstance(const std::wstring& device, const std::wstring& name, scxlong instance, scxlong devID);
        virtual SCXCoreLib::SCXHandle<DeviceInstance> FindDeviceInstance(const std::wstring& device) const;

        /**
           \copydoc SCXSystemLib::DiskDepend::open
        */
        virtual bool open(const char* pathname, int flags);

        /**
           \copydoc SCXSystemLib::DiskDepend::close
        */
        virtual int close(void);

        /**
           \copydoc SCXSystemLib::DiskDepend::ioctl
        */
        virtual int ioctl(unsigned long int request, void* data);

        /**
           \copydoc SCXSystemLib::DiskDepend::read
        */
        virtual ssize_t read(void *pbuf, size_t bytecount);

        /**
           \copydoc SCXSystemLib::DiskDepend::statvfs
        */
        virtual int statvfs(const char* path, SCXStatVfs* buf)
        {
            SCXASSERT(buf);
            return ::statvfs64(path, buf);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::lstat
        */
        virtual int lstat(const char* path, struct stat *buf) { return ::lstat(path, buf); }

        virtual bool FileExists(const std::wstring& path);
#if defined(hpux)
        /**
           \copydoc SCXSystemLib::DiskDepend::open_
        */
        virtual int _open(const char* pathname, int flags)
        {
            return ::open(pathname, flags);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::close
        */
        virtual int _close(int fd)
        {
            return ::close(fd);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::ioctl
        */
        virtual int _ioctl(int fd, int request, void* data)
        {
            return ::ioctl(fd, request, data);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::pstat_getdisk
        */
        virtual int pstat_getdisk(struct pst_diskinfo* buf, size_t elemsize, size_t elemcount, int index)
        {
            return ::pstat_getdisk(buf, elemsize, elemcount, index);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::pstat_getlv
        */
        virtual int pstat_getlv(struct pst_lvinfo* buf, size_t elemsize, size_t elemcount, int index)
        {
            return ::pstat_getlv(buf, elemsize, elemcount, index);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::setmntent
        */
        virtual FILE *setmntent(const char *path, const char *type)
        {
            return ::setmntent(path, type);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::getmntent
        */
        virtual struct mntent *getmntent(FILE *stream)
        {
            return ::getmntent(stream);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::endmntent
        */
        virtual int endmntent(FILE *stream)
        {
            return ::endmntent(stream);
        }
        /**
           \copydoc SCXSystemLib::DiskDepend::stat
        */
        virtual int stat(const char *path, struct stat *buf)
        {
            return ::stat(path, buf);
        }
#endif /* hpux */
#if defined(aix)
        /**
           \copydoc SCXSystemLib::DiskDepend::perfstat_disk
        */
        virtual int perfstat_disk(perfstat_id_t* name, perfstat_disk_t* buf, size_t struct_size, int n)
        {
            return ::perfstat_disk(name, buf, struct_size, n);
        }

        /**
           \copydoc SCXSystemLib::DiskDepend::mntctl
        */
        virtual int mntctl(int command, int size, char* buf)
        {
            return ::mntctl(command, size, buf);
        }

        /** 
            \copydoc SCXSystemLib::DiskDepend::CreateOdm
        */
        virtual const SCXCoreLib::SCXHandle<SCXodm> CreateOdm(void) const
        {
            return SCXCoreLib::SCXHandle<SCXodm>(new SCXodm());
        }

        /** 
            \copydoc SCXSystemLib::DiskDepend::lvm_queryvgs
        */
        virtual int lvm_queryvgs(struct queryvgs **queryVGS, mid_t kmid)
        {
            return ::lvm_queryvgs(queryVGS, kmid);
        }

        /** 
            \copydoc SCXSystemLib::DiskDepend::lvm_queryvg
        */
        virtual int lvm_queryvg(struct unique_id *vgId, struct queryvg **queryVG, char *pvName)
        {
            return ::lvm_queryvg(vgId, queryVG, pvName);
        }

        /** 
            \copydoc SCXSystemLib::DiskDepend::lvm_querylv
        */
        virtual int lvm_querylv(struct lv_id *lvId, struct querylv **queryLV, char *pvName)
        {
            return ::lvm_querylv(lvId, queryLV, pvName);
        }

#endif /* aix */
#if defined(aix) || defined(sun)

        /**
           \copydoc SCXSystemLib::DiskDepend::statvfs64
        */
        virtual int statvfs64(const char* path, struct statvfs64* buf)
        {
            return ::statvfs64(path, buf);
        }

#endif

        /**
           \copydoc SCXSystemLib::DiskDepend::Run
        */
        virtual int Run(const std::wstring &command, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr,
                        unsigned timeout, const SCXCoreLib::SCXFilePath& cwd = SCXCoreLib::SCXFilePath(),
                        const SCXCoreLib::SCXFilePath& chrootPath  = SCXCoreLib::SCXFilePath() )
        {
            return SCXCoreLib::SCXProcess::Run(command, mystdin, mystdout, mystderr, timeout, cwd, chrootPath);
        }

#if defined(linux)
        /**
           \copydoc SCXSystemLib::DiskDepend::GetWIStream
        */
        virtual SCXCoreLib::SCXHandle<std::wistream> GetWIStream(const char *name)
        {
            return SCXCoreLib::SCXHandle<std::wistream>(new std::wifstream(name));
        }
#endif
    private:
        SCXCoreLib::SCXLogHandle m_log; //!< Log handle.
    protected:
        typedef std::map<std::wstring, SCXCoreLib::SCXHandle<DeviceInstance> >  DeviceMapType;  //!< Type used for the device-path-to-instance map
        SCXCoreLib::SCXFilePath m_MntTabPath; //!< path to mount tab file.
#if defined(sun)
        SCXCoreLib::SCXFilePath m_DevTabPath; //!< path to device tab file
#endif
        SCXCoreLib::SCXFilePath m_ProcDiskStatsPath; //!< path to proc diskstats file.
        SCXCoreLib::SCXFilePath m_ProcPartitionsPath; //!< path to the partitions file
        SCXCoreLib::SCXHandle<SCXLvmTab> m_pLvmTab; //!< A parsed lvmtab file object.
        SCXCoreLib::SCXHandle<SCXRaid> m_pRaid; //!< A parsed RAID configuration.
        std::vector<MntTabEntry> m_MntTab; //!< A parsed mnttab object.
        DeviceMapType m_deviceMap; //!< Device path to instance map.
        std::map<std::wstring, std::vector<std::wstring> > m_ProcDiskStats; //!< parsed /proc/diskstats data.
        std::map<std::wstring, std::wstring> m_fsMap; //!< Used to map filesystem identifiers to names.

        static const int CLOSED_DESCRIPTOR = -1;
        int  m_fd;
        char m_PathName[MAXPATHLEN];
        int  m_OpenFlags;
        virtual void reopen(void);


        virtual bool FileSystemNoLinkToPhysical(const std::wstring& fs);
#if defined(sun)
        virtual std::wstring GetVopstatName(const std::wstring & dev_path, const std::wstring & mountpoint);
        virtual bool IsDiskInKstat(const std::wstring& dev_path);
        virtual bool GuessKstatPath(const std::wstring& dev_path, std::wstring& module, std::wstring& name, scxlong& instance, bool isDisk);
        virtual bool GuessVopstat(const std::wstring& dev_path, std::wstring& vopstat);
        virtual bool ReadKstat(SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstat> kstat, const std::wstring& dev_path, const std::wstring& mountpoint, bool isDisk);
#endif
        std::wstring GuessPhysicalFromLogicalDevice(const std::wstring& logical_dev);
        std::wstring RemoveTailNumberOrOther(const std::wstring& str);

        /** Declaring a compare function type to be used with IsStringInArray method */
        typedef bool CompareFunction(const std::wstring& needle, const std::wstring& heystack);

        static bool IsStringInSet(const std::wstring& str, const std::set<std::wstring>& arrSet, CompareFunction compare); 
        static bool IsStringInSet(const std::wstring& str, const std::set<std::wstring>& arrSet); 

        static bool CompareEqual(const std::wstring& needle, const std::wstring& heystack);
        static bool CompareStartsWith(const std::wstring& needle, const std::wstring& heystack);
        static bool CompareContains(const std::wstring& needle, const std::wstring& heystack);
    };


} /* namespace SCXSystemLib */
#endif /* DISKDEPEND_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
