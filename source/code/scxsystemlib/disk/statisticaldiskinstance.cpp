/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implements the common parts for physical and logical disk 
                 instance pal for statistical information.
    
    \date        2008-04-28 15:20:00

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/statisticaldiskinstance.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxfilepath.h>

#include <errno.h>
#include <math.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
   Standard constructor.
    
   \param       deps The dependency object.
   \param       isTotal - true if creating a total instance (default:false)
   \returns     N/A
    
*/
    StatisticalDiskInstance::StatisticalDiskInstance(
        SCXCoreLib::SCXHandle<DiskDepend> deps,
        bool isTotal /* = false*/)
      : EntityInstance(isTotal),
        m_deps(0),
        m_reads(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_writes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_transfers(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_tBytes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_rBytes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_wBytes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_waitTimes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_tTimes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_rTimes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_wTimes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_runTimes(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_timeStamp(MAX_DISKINSTANCE_DATASAMPER_SAMPLES),
        m_qLengths(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)
#if defined(sun)
           , m_kstat(0)
#endif
    {
        m_deps = deps;
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticaldiskinstance");
#if defined(sun)
        m_kstat = new SCXKstat();
#endif
        SetId(isTotal?L"_Total":L"?");
        m_device = isTotal?L"_Total":L"?";
        m_mountPoint = L"";
        m_sectorSize = 512; // This value is assumed and accurate for all known systems at the moment. Not easy to look up at run time.
        m_samplerDevices.clear();
        Reset();
    }

/*----------------------------------------------------------------------------*/
/**
    Dump the object as a string for logging purposes.
    
    \returns     String representation of object, suitable for logging.
*/
    const std::wstring StatisticalDiskInstance::DumpString() const
    {
        return L"StatisticalDiskInstance: " + m_device;
    }

/*----------------------------------------------------------------------------*/
/**
    Reset all values of the object.
*/
    void StatisticalDiskInstance::Reset()
    {
        m_readsPerSec = 0;
        m_writesPerSec = 0;
        m_transfersPerSec = 0;
        m_rBytesPerSec = 0;
        m_wBytesPerSec = 0;
        m_tBytesPerSec = 0;
        m_rPercentage = 0;
        m_wPercentage = 0;
        m_tPercentage = 0;
        m_rTime = 0;
        m_wTime = 0;
        m_tTime = 0;
        m_runTime = 0;
        m_waitTime = 0;
        m_secPerRead = 0;
        m_secPerWrite = 0;
        m_secPerTransfer = 0;
        m_mbUsed = 0;
        m_mbFree = 0;
        m_inodesTotal = 0;
        m_inodesFree = 0;
        m_blockSize = 0;
        m_qLength = 0;

        m_reads.Clear();
        m_writes.Clear();
        m_rBytes.Clear();
        m_wBytes.Clear();
        m_transfers.Clear();
        m_tBytes.Clear();
        m_tTimes.Clear();
        m_rTimes.Clear();
        m_wTimes.Clear();
        m_runTimes.Clear();
        m_waitTimes.Clear();
        m_timeStamp.Clear();
        m_qLengths.Clear();
    }

/*----------------------------------------------------------------------------*/
/**
    Update the instance.
*/
    void StatisticalDiskInstance::Update()
    {
        if (IsTotal())
        { // Total instance updated from DiskEnumeration.
            return;
        }

        m_mbFree = 0;
        m_mbUsed = 0;
        m_inodesTotal = 0;
        m_inodesFree = 0;
        m_readsPerSec = m_reads.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_writesPerSec = m_writes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_transfersPerSec = m_transfers.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_rBytesPerSec = m_rBytes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_wBytesPerSec = m_wBytes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_tBytesPerSec = m_tBytes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_tTime = m_tTimes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_rTime = m_rTimes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_wTime = m_wTimes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_runTime = m_runTimes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_waitTime = m_waitTimes.GetAverageDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) / DISK_SECONDS_PER_SAMPLE;
        m_qLength = m_qLengths.GetAverage<double>();

#if defined(linux) || defined(hpux)
        m_tPercentage = m_rPercentage + m_wPercentage;
#elif defined(sun)
        if (0 != m_timeStamp.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))
        {
            m_tPercentage = ((m_rTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + m_wTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))*100)/m_timeStamp.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
        }
        else
        {
            m_tPercentage = 0;
        }
#endif
        if (!m_rTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)
        {
            m_secPerRead = static_cast<double>(m_rTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / static_cast<double>(m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / 1000.0;
        }
        else
        {
            m_secPerRead = 0;
        }

        if (!m_wTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)
        {
            m_secPerWrite = static_cast<double>(m_wTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / static_cast<double>(m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / 1000.0;
        }
        else
        {
            m_secPerWrite = 0;
        }

#if defined(aix)
        if (!m_tTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && m_transfers.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)
        {
            m_secPerTransfer = static_cast<double>(m_tTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / static_cast<double>(m_transfers.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / 1000000.0; // TODO: check this value
        }
#elif defined(hpux)
        if (!m_tTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && m_transfers.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)
        {
            m_secPerTransfer = static_cast<double>(m_tTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / static_cast<double>(m_transfers.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES)) / 1000.0;
        }
#elif defined(linux)
        if (!m_rTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && !m_wTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && 
            ((m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0) || (m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)))
        {
            m_secPerTransfer = (static_cast<double>(m_rTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + m_wTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))) 
                / (static_cast<double>(m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))) 
                / 1000.0;
        }
#elif defined(sun)
        if (!m_runTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && !m_waitTimes.HasWrapped(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) && 
            ((m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0) || (m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) != 0)))
        {
            m_secPerTransfer = (static_cast<double>(m_runTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + m_waitTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))) 
                / (static_cast<double>(m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES))) 
                / 1000.0;
        }
#endif
        else
        {
            m_secPerTransfer = 0;
        }

        if (0 < m_mountPoint.length())
        {
            SCXStatVfs s_vfs;
            memset(&s_vfs, 0, sizeof(s_vfs));
            if (0 == m_deps->statvfs(SCXCoreLib::StrToUTF8(m_mountPoint).c_str(), &s_vfs))
            {
                // ceil is used here since df system command rounds values up and we want to show values as presented
                // when using system commands.
                m_mbFree = static_cast<scxulong>(ceil(SCXCoreLib::BytesToMegaBytes(static_cast<double>(s_vfs.f_bavail)*static_cast<double>(s_vfs.f_frsize))));
                m_mbUsed = static_cast<scxulong>(ceil(SCXCoreLib::BytesToMegaBytes((static_cast<double>(s_vfs.f_blocks)-
                                                                                    static_cast<double>(s_vfs.f_bavail))*static_cast<double>(s_vfs.f_frsize))));
                m_blockSize = s_vfs.f_bsize;

                // Grab the inode information while we have it
                m_inodesTotal = s_vfs.f_files;
                m_inodesFree = s_vfs.f_ffree;
            }
            else
            {
                // Ignore EOVERFLOW (if disk is too big) to keep disk 'on-line' even without statistics
                if ( EOVERFLOW != errno )
                {
                    SCX_LOGERROR(m_log, 
                        SCXCoreLib::StrAppend(L"statvfs() failed for " + m_mountPoint + L"; errno = ", errno ) );
                } 
                else 
                {
                    SCX_LOGHYSTERICAL(m_log, SCXCoreLib::StrAppend(L"statvfs() failed with EOVERFLOW for ", m_mountPoint));
                }
            }
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the instance device ID.
    
    \param       value - output parameter where the instance device ID will be stored.
    \returns     True if an device could be returned, otherwise false.
*/
    bool StatisticalDiskInstance::GetDiskDeviceID(std::wstring& value) const
    {
        SCXCoreLib::SCXFilePath dev(m_device);
        value = dev.GetFilename();
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the instance name.
    
    \param       value - output parameter where the instance name wil be stored.
    \returns     True if a name could be returned, otherwise false.
*/
    bool StatisticalDiskInstance::GetDiskName(std::wstring& value) const
    {
        value = GetId();
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve number of reads per second.
    
    \param       value - output parameter where number of reads per second is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetReadsPerSecond(scxulong& value) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            value = 0;
        }
        else
        {
            value = m_readsPerSec;
        }
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve number of writes per second.
    
    \param       value - output parameter where number of writes per second is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetWritesPerSecond(scxulong& value) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            value = 0;
        }
        else
        {
            value = m_writesPerSec;
        }
        return true;
    }
    
/*----------------------------------------------------------------------------*/
/**
    Retrieve number of transfers per second.
    
    \param       value - output parameter where number of transfers per second is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetTransfersPerSecond(scxulong& value) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            value = 0;
        }
        else
        {
            value = m_transfersPerSec;
        }
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrive number of bytes read/written per second.
    
    \param       read - output parameter where number of read bytes per second is stored.
    \param       write - output parameter where number of written bytes per second is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetBytesPerSecond(scxulong& read, scxulong& write) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            read = 0;
            write = 0;
        }
        else
        {
            read = m_rBytesPerSec;
            write = m_wBytesPerSec;
        }
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrive number of bytes transfered per second.
    
    \param       total - output parameter where number of transfered bytes per second is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetBytesPerSecondTotal(scxulong& total) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            total = 0;
        }
        else
        {
            total = m_tBytesPerSec;
        }
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve IO operation percentages (read/write) per second.
    
    \param       read - output parameter where percentage of time reading is stored.
    \param       write - output parameter where percentage of time writing is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetIOPercentage(scxulong& /*read*/, scxulong& /*write*/) const
    {
        return false;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve total IO operation percentages per second.
    
    \param       total - output parameter where percentage of time reading and writing is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetIOPercentageTotal(scxulong& total) const
    {
        total = m_tPercentage;
#if defined(aix) || defined(linux) || defined(hpux)
        return false;
#elif defined(sun)
        return true;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the number of seconds used per read/write operation.
    
    \param       read - output parameter where average time per read is stored.
    \param       write - output parameter where average time per write is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetIOTimes(double& read, double& write) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            read = 0;
            write = 0;
            return true;
        }
        
#if defined(aix) || defined(linux)
        read = m_secPerRead;
        write = m_secPerWrite;
        return true;
#elif defined(sun) || defined(hpux)
        read = write = 0; // mainly to avoid parameters not used warnings
        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the number of seconds used per read and write operations.
    
    \param       total - output parameter where average time per read and write is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetIOTimesTotal(double& total) const
    {
        // workitem 18449, 5275
        // This returns 0 and not null in the case where we have multiple partitions in a volume group
        if (m_samplerDevices.size() > 1)
        {
            total = 0;
        }
        else
        {
            total = m_secPerTransfer;
        }
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Get the average IO queue length.
    
    \param       queueLength - output parameter where average IO queue length is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetDiskQueueLength(double& queueLength) const
    {
        queueLength = m_qLength;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve disk size in terms of used/free MB.
    
    \param       mbUsed - output parameter where MBs used on disk is stored.
    \param       mbFree - output parameter where MBs free on disk is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetDiskSize(scxulong& mbUsed, scxulong& mbFree) const
    {
        mbUsed = m_mbUsed;
        mbFree = m_mbFree; 
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve disk inode usage in terms of total/free inodes.

    \param       inodesTotal - output parameter where total inodes used on disk is stored.
    \param       inodesFree - output parameter where free (availabe) inodes on disk is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetInodeUsage(scxulong& inodesTotal, scxulong& inodesFree) const
    {
        inodesTotal = m_inodesTotal;
        inodesFree = m_inodesFree; 
        return (inodesTotal != 0);
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve disk block size.
    
    \param      blockSize - output parameter where disk block size is stored.
    \returns    true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetBlockSize(scxulong& blockSize) const
    {
        blockSize = m_blockSize;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
    Retrieve the disk health state.
    
    \param       healthy - output parameter where the health status of the disk is stored.
    \returns     true if value was set, otherwise false.
*/
    bool StatisticalDiskInstance::GetHealthState(bool& healthy) const
    {
        healthy = m_online;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Find the disk info index of a disk with given id.
    
       \param[in]   id id (rdev) of disk.
       \returns The pstat index of the disk or -1 if not found
    */
    scxlong StatisticalDiskInstance::FindDiskInfoByID(scxlong id)
    {
#if defined(hpux)
        struct pst_diskinfo di;
        int r = 1;
        for (int i=0; 1 == r; ++i)
        {
            r = m_deps->pstat_getdisk(&di, sizeof(di), 1, i);
            if (1 == r)
            {
                if (id == ((di.psd_dev.psd_major << 24) | di.psd_dev.psd_minor))
                {
                    return i;
                }
            }
        }
#endif
        SCX_LOGHYSTERICAL(m_log, SCXCoreLib::StrAppend(L"FindDiskInfoByID failed for ID: ", id));
        return -1;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Find the lv info index of a logical volume with given id.
    
       \param[in]   id id (rdev) of logical volume.
       \returns The pstat index of the lv info. -1 is returned if it cannot be found.
       \throws SCXNotSupportedException if not supported on platform.
    */
    scxlong StatisticalDiskInstance::FindLVInfoByID(scxlong id)
    {
#if defined(hpux)
        struct pst_lvinfo lvi;
        int r = 1;
        for (int i=0; 1 == r; ++i)
        {
            r = m_deps->pstat_getlv(&lvi, sizeof(lvi), 1, i);
            if (1 == r)
            {
                if (id == ((lvi.psl_dev.psd_major << 24) | lvi.psl_dev.psd_minor))
                {
                    return i;
                }
            }
        }
#else
        throw SCXCoreLib::SCXInternalErrorException(L"Unable to find lv id: " + SCXCoreLib::StrFrom(id), SCXSRCLOCATION);
#endif

        return -1;
    }

    /*-------------------------------------------------------------------------------*/
    /**
       Retrieve the disk file system type.

       \param        - output parameter where the file system type of the disk is stored.
       \returns     true if value was set, otherwise false.
    */
    bool StatisticalDiskInstance::GetFSType(std::wstring& fsType) const
    {
        fsType = m_fsType;
        return true;
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
