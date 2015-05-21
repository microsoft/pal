/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file        staticlogicaldiskinstance.cpp

   \brief       Implements the logical disk instance pal for static information.

   \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>

#include <scxsystemlib/staticlogicaldiskinstance.h>
#if defined(linux)
#include <scxcorelib/scxregex.h>
typedef SCXCoreLib::SCXHandle<SCXCoreLib::SCXRegex> SCXRegexPtr;
#endif
#include <errno.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
#if defined(linux) || defined(sun)
  /* Fs name list of support quota */
  const std::wstring cSupportQuotasFS[] = {L"ufs", L"zfs"};
  /* String flag value "quota" in mnttab when quota is enable */
  const std::wstring cIsQuotaFlag(L"quota");
  /* String flag value "noquota" in mnttab when quota is disable */
  const std::wstring cNoQuotaFlag(L"noquota");
#endif

  /** The flag string to check whether a logical disk is a RAM Disk*/
  const std::wstring cAnchorRAMDisk = L"/dev/ram";
  /** The file system type corresponding to CDROM*/
  const std::wstring cCDROMFS= L"iso9660";
  /** The file system type corresponding to DVDROM*/
  const std::wstring cUFSFS= L"ufs";

#if defined(linux)
  /** Device name patterns that indicate a partition on a local disk */
  const std::wstring c_fixedDiskPartitionPattern(L"/dev/[hs]d[a-z][0-9].*");
  const std::wstring c_xenDiskPartitionPattern(L"/dev/xvd[a-z][0-9].*");
#endif

  /*----------------------------------------------------------------------------*/
  /**
     Constructor.

     \param[in]    deps - A StaticDiscDepend object which can be used.
  */
  StaticLogicalDiskInstance::StaticLogicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps)
    : m_deps(0), m_online(false), m_sizeInBytes(0), m_isReadOnly(false), m_persistenceType(0), m_availableSpace(0),
      m_isNumFilesSupported(false), m_numTotalInodes(0), m_numAvailableInodes(0),
      m_isCaseSensitive(false), m_isCasePreserved(false), m_codeSet(0),
      m_maxFilenameLen(0), m_blockSize(0), m_quotasDisabled(false), m_supportsDiskQuotas(false), m_driveType(eUnknown)
  {
    m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticlogicaldiskinstance");
    m_deps = deps;

    m_logicDiskOptions = L"";
  }

  /*----------------------------------------------------------------------------*/
  /**
     Virtual destructor.
  */
  StaticLogicalDiskInstance::~StaticLogicalDiskInstance()
  {

  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the disk health state.

     \param[out]   healthy - output parameter where the health status of the disk is stored.

     \returns      true if value was set, otherwise false.
  */
  bool StaticLogicalDiskInstance::GetHealthState(bool& healthy) const
  {
    healthy = m_online;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the device name (i.e. '/').

     \param[out]   value - output parameter where the name of the device is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetDeviceName(std::wstring& value) const
  {
    value = GetId();
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the device ID (i.e. '/dev/sda2' on Linux).

     \param[out]   value - output parameter where the ID of the device is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetDeviceID(std::wstring& value) const
  {
    value = m_device;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the file system type

     \param[out]    value - output parameter where the file system type

     \returns       true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetFileSystemType(std::wstring& value) const
  {
    value = m_fileSystemType;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the mount point (i.e. '/').

     \param[out]   value - output parameter where the mountpoint is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetMountpoint(std::wstring& value) const
  {
    value = m_mountPoint;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the size of the file system in bytes

     \param[out]   value - output parameter where the size is stored.

     \returns       true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetSizeInBytes(scxulong& value) const
  {
    value = m_sizeInBytes;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the compression method.

     Valid compression methods are: "Unknown", "Compressed", or "Uncompressed".

     \param[out]   value - output parameter where the compression method is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetCompressionMethod(std::wstring& value) const
  {
    value = m_compressionMethod;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the Read-Only state of the device.

     \param[out]   value - output parameter where the read-only state is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetIsReadOnly(bool& value) const
  {
    value = m_isReadOnly;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the encryption method.

     Valid encryption methods are: "Unknown", "Encrypted", or "Not Encrypted"

     \param[out]   value - output parameter where the encryption method is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetEncryptionMethod(std::wstring& value) const
  {
    value = m_encryptionMethod;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the Persistence Type.

     Valid persistence types are: 0, 1, 2, 3, or 4
     to refer to: "Unknown", "Other", "Persistent", "Temporary", or "External"

     \param[out]   value - output parameter where the persistence type is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetPersistenceType(int& value) const
  {
    value = m_persistenceType;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the space available on the volume in bytes.

     \param[out]   value - output parameter where the available space is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetAvailableSpaceInBytes(scxulong& value) const
  {
    value = m_availableSpace;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the block size in bytes.

     \param[out]   value - output parameter where the block size is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetBlockSize(scxulong& value) const
  {
    value = m_blockSize;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the number of inodes allocated to the file system.

     Note that some implementations of file systems return this while others
     do not.  If a file system does not support this, zero is returned and the
     method will return false.

     \param[out]   value - output parameter where the number of inodes is stored.

     \returns      true if value is supported on file system, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetTotalInodes(scxulong& value) const
  {
    value = m_numTotalInodes;
    return (m_isNumFilesSupported);
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the number of inodes available in the file system.

     This call retrieves the number of inodes available in the file system
     (regardless of privilege level).  No "buffer" is left aside for
     privileged users.

     Note that some implementations of file systems return this while others
     do not.  If a file system does not support this, zero is returned and the
     method will return false.

     \param[out]   value - output parameter where the number of files is stored.

     \returns      true if value is supported on file system, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetAvailableInodes(scxulong& value) const
  {
    value = m_numAvailableInodes;
    return (m_isNumFilesSupported);
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the number of files stored on the file system.

     Note that some implementations of file systems return this while others
     do not.  If a file system does not support this, zero is returned and the
     method will return false.

     \param[out]   value - output parameter where the number of files is stored.

     \returns      true if value is supported on file system, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetNumberOfFiles(scxulong& value) const
  {
    value = m_numFiles;
    return (m_isNumFilesSupported);
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the case sensitivity of the file system (true/false)

     \param[out]   value - output parameter where the case sensitivity is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetIsCaseSensitive(bool& value) const
  {
    value = m_isCaseSensitive;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the case preservation of the file system (true/false)

     \param[out]   value - output parameter where the case preservation is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetIsCasePreserved(bool& value) const
  {
    value = m_isCasePreserved;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the code set of the file system.

     Valid code sets are: 0, 1, 2, 3, 4, 5, 6, 7, or 8
     to refer to: "Unknown", "Other", "ASCII", "Unicode", "ISO2022", "ISO8859",
     "Extended UNIX Code", "UTF-8", or "UCS-2" respectively.

     \param[out]   value - output parameter where the code set is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetCodeSet(int& value) const
  {
    value = m_codeSet;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Retrieve the maximum filename length of the file system.

     \param[out]   value - output parameter where the maximum filename length is stored.

     \returns      true if value is supported on platform, false otherwise.
  */
  bool StaticLogicalDiskInstance::GetMaxFilenameLen(scxulong& value) const
  {
    value = m_maxFilenameLen;
    return true;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Dump the object as a string for logging purposes.

     \returns       String representation of object, suitable for logging.
  */
  const std::wstring StaticLogicalDiskInstance::DumpString() const
  {
    return SCXDumpStringBuilder("StaticLogicalDiskInstance")
      .Text("Name", GetId())
      .Text("Device", m_device)
      .Text("MountPoint", m_mountPoint)
      .Text("FileSystemType", m_fileSystemType)
      .Scalar("SizeInBytes", m_sizeInBytes)
      .Text("CompressionMethod", m_compressionMethod)
      .Scalar("ReadOnly", m_isReadOnly)
      .Text("EncryptionMethod", m_encryptionMethod)
      .Scalar("PersistenceType", m_persistenceType)
      .Scalar("AvailableSpace", m_availableSpace)
      .Scalar("isNumFilesSupported", m_isNumFilesSupported)
      .Scalar("NumberOfFiles", m_numFiles)
      .Scalar("TotalFilesAllowed", m_numTotalInodes)
      .Scalar("TotalFilesAvailable", m_numAvailableInodes)
      .Scalar("CaseSensitive", m_isCaseSensitive)
      .Scalar("CasePreserved", m_isCasePreserved)
      .Scalar("CodeSet", m_codeSet)
      .Scalar("MaxFilenameLen", m_maxFilenameLen)
      .Scalar("BlockSize", m_blockSize);
  }

  /*----------------------------------------------------------------------------*/
  /**
     Get default settings for this file system

     On UNIX, almost all file systems are the same with the sorts of things we care about.
     But this isn't necessarily ALWAYS the case.  This routine gives us the opportunity to
     change some of the values easily for specific file systems.
  */
    void StaticLogicalDiskInstance::UpdateDefaults()
    {
        static struct {
            std::wstring fsType;
            std::wstring compression;
            std::wstring encryption;
            int persistenceType;
            bool isCasePreserved;
            bool isCaseSensitive;
            int codeSet;
        } fsProperties[] =
          {
        /*    fsType         Compression          Encryption        PI   CaseP   CaseS   CS  */
#if defined(sun) && ((PF_MAJOR == 5 && PF_MINOR >= 11) || (PF_MAJOR > 5))
              { L"dev",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
#endif
              { L"btrfs",      L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"ext2",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"ext3",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"ext4",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              /* Hi Performance FileSystem on HP-UX (not HPFS or Hierarchical File System) */
              { L"hfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"jfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"jfs2",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"reiserfs",   L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"ufs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"vfat",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"vxfs",       L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"xfs",        L"Not Compressed",   L"Not Encrypted",   2,   true,   true,   0  },
              { L"zfs",        L"Unknown",          L"Unknown",         2,   true,   true,   0  },

              /*
               * Taking liberties with the case-sensitive/case-preserved values
               * (Boolean anyway, so there's no concept of "Unknown" here)
               */

              { L"",           L"Unknown",          L"Unknown",         0,   true,   true,   0 }
          };

        /* Look for the file system in list of known file systems */

        for (int i=0; /* None; we break out when done */ ; i++)
        {
            if (0 == SCXCoreLib::StrCompare(m_fileSystemType, fsProperties[i].fsType, true)
                || 0 == fsProperties[i].fsType.size())
            {
                m_compressionMethod = fsProperties[i].compression;
                m_encryptionMethod = fsProperties[i].encryption;
                m_persistenceType = fsProperties[i].persistenceType;
                m_isCasePreserved = fsProperties[i].isCasePreserved;
                m_isCaseSensitive = fsProperties[i].isCaseSensitive;
                m_codeSet = fsProperties[i].codeSet;
                break;
            }
        }
    }

  /*----------------------------------------------------------------------------*/
  /**
     Update the instance.
  */
  void StaticLogicalDiskInstance::Update()
  {
    UpdateDefaults();

    /* Do a statvfs() call to get file system statistics */
    SCXStatVfs fsstat;
    if (0 != m_deps->statvfs(SCXCoreLib::StrToUTF8(GetId()).c_str(), &fsstat))
    {
        // Ignore EOVERFLOW (if disk is too big) to keep disk 'on-line' even without statistics
      if ( EOVERFLOW == errno )
      {
          SCX_LOGHYSTERICAL(m_log, SCXCoreLib::StrAppend(L"statvfs() failed with EOVERFLOW for ", GetId()));
      }
      else
      {
          SCX_LOGERROR(m_log, SCXCoreLib::StrAppend(L"statvfs() failed for " + GetId() + L"; errno = ", errno));
      }
      return;
    }
    m_sizeInBytes = static_cast<scxulong> (fsstat.f_blocks) * fsstat.f_frsize;
    m_isReadOnly = (fsstat.f_flag & ST_RDONLY);
    m_availableSpace = static_cast<scxulong> (fsstat.f_bfree) * fsstat.f_frsize;
    if (fsstat.f_files) {
      m_isNumFilesSupported = true;
      m_numTotalInodes = fsstat.f_files;
      m_numAvailableInodes = fsstat.f_ffree;
      m_numFiles = fsstat.f_files - fsstat.f_ffree;
    }
    m_maxFilenameLen = fsstat.f_namemax;
    m_blockSize =  fsstat.f_frsize;

#if defined(linux)
    //
    //Get the drive type.
    //
    m_driveType = FindDriveTypeFlag(cAnchorRAMDisk);

#elif defined(sun)

    //
    // Get the drive type.
    //
    m_driveType = FindDriveTypeFlag(cAnchorRAMDisk);

    //
    // Determine whether the logic disk support quotas.
    //
    m_supportsDiskQuotas = isSupportQuotas();
    //
    // Determine whether the quotas is disabled.
    //
    m_quotasDisabled = isQuotasDisabled();
#endif

  }

    /*----------------------------------------------------------------------------*/
    /**
      Get QuotasDisabled for this logical disk.

      \param[out]  quotasDisabled
      \returns     whether the implementation for this platform supports the value or not.
    */
    bool StaticLogicalDiskInstance::GetQuotasDisabled(bool& value) const
    {
        bool fRet = false;
#if defined(sun)
        value = m_quotasDisabled;
        fRet = true;
#else
        (void) value;  // Suppress unused variable warning.
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get SupportsDiskQuotas for this logic disk.

      \param[out]   supportsDiskQuotas
      \returns      whether the implementation for this platform supports the value or not.
    */
    bool StaticLogicalDiskInstance::GetSupportsDiskQuotas(bool& value) const
    {
        bool fRet = false;
#if defined(sun)
        value = m_supportsDiskQuotas;
        fRet = true;
#else
        (void) value;  // Suppress unused variable warning.
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get DriveType.

      \param[out]   driveType - RETURN: Numeric value that corresponds to the type of disk drive this logical disk represents.
      \returns      true if a value is supported by this implementation
    */
    bool StaticLogicalDiskInstance::GetDriveType(unsigned int& driveType) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun)
        driveType = m_driveType;
        fRet = true;
#else
        (void) driveType;  // Suppress unused variable warning.
#endif
        return fRet;
    }

  /*----------------------------------------------------------------------------*/
  /**
     Find the driver type string on m_device.

     \param[out]   anchorRAMDisk type

     \returns      DriveType. If not match, a default will be returned.
  */
  unsigned int StaticLogicalDiskInstance::FindDriveTypeFlag(const std::wstring& anchorRAMDisk)
  {
    unsigned int driveType = (unsigned int)eUnknown;
    SCX_LOGTRACE(m_log, L"File system type is: " + m_fileSystemType);

    // Get RAMDisk type via Anchor string "/dev/ram". If not contained, compare against the CDROMFS type.
    //
    if (std::wstring::npos != m_device.find(anchorRAMDisk))
      {                               // a RAM disk
        driveType = eRAMDisk;
      }
    else if (m_fileSystemType.compare(cCDROMFS) == 0)
      {                               // a CD-ROM or DVD-ROM
        driveType = eCompactDisk;
      }
    else if (m_fileSystemType.compare(cUFSFS) == 0)
      {
#if defined(linux)
        driveType = eCompactDisk;
#else
        switch(m_diskRemovability)
          {
          case eDiskCapSupportsRemovableMedia:
            driveType = eRemovableDisk;
            break;

          case eDiskCapOther:
            driveType = eLocalDisk; // i.e., not removable
            break;

          case eDiskCapUnknown: // drop through to default
          default:
            driveType = eUnknown;
            break;
          }
#endif
      }
#if defined(linux)
    else
          {                               // see if the name has a recognized form for a fixed or virtual disk partition
        SCXRegexPtr fixedDiskPartitionRegExPtr(NULL);
        SCXRegexPtr xenDiskPartitionRegExPtr(NULL);

        try
          {
                    fixedDiskPartitionRegExPtr = new SCXCoreLib::SCXRegex(c_fixedDiskPartitionPattern);
                    xenDiskPartitionRegExPtr = new SCXCoreLib::SCXRegex(c_xenDiskPartitionPattern);
          }
        catch (SCXCoreLib::SCXInvalidRegexException &e)
          {
                    SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
                    return false;
          }

        if (fixedDiskPartitionRegExPtr->IsMatch(m_device))
          {                           // a partition on an ATAPI/IDE or SCSI drive
                    driveType = eLocalDisk;
          }
        else if (xenDiskPartitionRegExPtr->IsMatch(m_device))
          {                           // a partition on a XEN virtual drive
                    driveType = eLocalDisk;
          }
        else
          {                           // unknown drive type
                    driveType = eUnknown;
          }
          }
#endif
    return driveType;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Determine whether the logical disk supports quotas.

     \returns      true if quotas is supported
  */
  bool StaticLogicalDiskInstance::isSupportQuotas()
  {
    bool supportsDiskQuotas = false;
#if defined(sun) && defined(sparc) && (PF_MINOR == 10)
    //
    //Compare current filesystem value is in supported filesystem array list.
    //
    for (int i = 0; i < sizeof(cSupportQuotasFS) / sizeof(cSupportQuotasFS[0]); i++)
          {
        if (cSupportQuotasFS[i] == m_fileSystemType)
          {
                    supportsDiskQuotas = true;
                    break;
          }
          }
#endif
    return supportsDiskQuotas;
  }

  /*----------------------------------------------------------------------------*/
  /**
     Determine whether the quotas is disabled.

     \returns      true if quotas disabled
  */
  bool StaticLogicalDiskInstance::isQuotasDisabled()
  {
    bool quotasDisabled = true;
#if defined(sun) && defined(sparc) && (PF_MINOR == 10)
    //
    // Find quota flag in mnttab options string.
    //
    if ( m_logicDiskOptions.find(cNoQuotaFlag) == std::wstring::npos && std::wstring::npos != m_logicDiskOptions.find(cIsQuotaFlag) )
          {
        quotasDisabled = false;
          }
#endif
    return quotasDisabled;
  }

} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
