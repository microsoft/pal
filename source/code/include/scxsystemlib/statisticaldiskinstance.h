/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines the statistical disk information instance PAL common
                 for physical and logical disks.
    
    \date        2008-04-28 15:20:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATISTICALDISKINSTANCE_H
#define STATISTICALDISKINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxsystemlib/datasampler.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxhandle.h>

#if defined(sun)
#include <scxsystemlib/scxkstat.h>
#endif

namespace SCXSystemLib
{
    /** Number of samples collected in the datasampler for Disk. */
    const int MAX_DISKINSTANCE_DATASAMPER_SAMPLES = 6;  // Sampling once every minute.

    /** Time between each sample in seconds. */
    const int DISK_SECONDS_PER_SAMPLE = 60;

    /** Datasampler for disk information. */
    typedef DataSampler<scxulong> DiskInstanceDataSampler;

    /*----------------------------------------------------------------------------*/
    /**
        Represents a single statistical disk instance. This class holds common parts 
        and is intended for sub-classing.
    */
    class StatisticalDiskInstance : public EntityInstance
    {
    public:
        StatisticalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal = false);

        void Reset();

        bool GetDiskDeviceID(std::wstring& value) const;
        bool GetDiskName(std::wstring& value) const;
        virtual bool GetReadsPerSecond(scxulong& value) const;
        virtual bool GetWritesPerSecond(scxulong& value) const;
        virtual bool GetTransfersPerSecond(scxulong& value) const;
        virtual bool GetBytesPerSecond(scxulong& read, scxulong& write) const;
        virtual bool GetBytesPerSecondTotal(scxulong& total) const;
        virtual bool GetIOPercentage(scxulong& read, scxulong& write) const;
        virtual bool GetIOPercentageTotal(scxulong& total) const;
        virtual bool GetIOTimes(double& read, double& write) const;
        virtual bool GetIOTimesTotal(double& total) const;
        virtual bool GetDiskQueueLength(double& value) const;
        virtual bool GetDiskSize(scxulong& mbUsed, scxulong& mbFree) const;
        virtual bool GetInodeUsage(scxulong& inodesTotal, scxulong& inodesFree) const;
        virtual bool GetBlockSize(scxulong& blockSize) const;
        virtual bool GetFSType(std::wstring& fsType) const;
        virtual bool GetHealthState(bool& healthy) const;
        
        virtual const std::wstring DumpString() const;
        virtual void Update();

        /*----------------------------------------------------------------------------*/
        /**
           Sample data for instance.
        */
        virtual void Sample() = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the last recorded sample values. Typically used in test methods and not by provider.
    
           \param       numR - output parameter where number of reads since boot is stored.
           \param       numW - output parameter where number of writes since boot is stored.
           \param       bytesR - output parameter where number of read bytes since boot is stored.
           \param       bytesW - output parameter where number of written bytes since boot is stored.
           \param       msR - output parameter where number of ms reading since boot is stored.
           \param       msW - output parameter where number of ms writing since boot is stored.
           \returns     true if value was set, otherwise false.
           
           \note On solaris/hpux msR and msW is not read/write but rather run/wait.
           \note On hpux physical disks numR/bytesR are number of/bytes transfered and numW/bytesW is always zero.
        */
        virtual bool GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const = 0;

        scxlong FindDiskInfoByID(scxlong id);
        scxlong FindLVInfoByID(scxlong id);
    protected:    
        SCXCoreLib::SCXHandle<DiskDepend> m_deps;//!< StaticDiskDepend object
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle
#if defined(sun)
        SCXCoreLib::SCXHandle<SCXKstat> m_kstat; //!< Reused kstat handle.
#endif
        bool m_online;             //!< Tells if disk is still connected.
        std::wstring m_device;     //!< Device name
        std::wstring m_mountPoint; //!< Mount point
        std::wstring m_fsType;     //!< FS type
        std::vector<std::wstring> m_samplerDevices; //!< Devices to sample data from.

        scxulong m_sectorSize;     //!< Sector size

        scxulong m_readsPerSec;    //!< Disk reads per second
        scxulong m_writesPerSec;   //!< Disk writes per second
        scxulong m_transfersPerSec;//!< Disk transfers per second 
        scxulong m_rBytesPerSec;   //!< Bytes read per second
        scxulong m_wBytesPerSec;   //!< Bytes written per second
        scxulong m_tBytesPerSec;   //!< Bytes transfered per second
        scxulong m_rPercentage;    //!< Read percentage of total time
        scxulong m_wPercentage;    //!< Write percentage of total time
        scxulong m_tPercentage;    //!< Read and write percentage of total time
        scxulong m_rTime;          //!< Read time
        scxulong m_wTime;          //!< Write time
        scxulong m_tTime;          //!< Transfer time
        scxulong m_runTime;        //!< Run time
        scxulong m_waitTime;       //!< Wait time
        double m_secPerRead;       //!< Seconds per read
        double m_secPerWrite;      //!< Seconds per write
        double m_secPerTransfer;   //!< Seconds per transfer
        scxulong m_mbUsed;         //!< MB used
        scxulong m_mbFree;         //!< MB free
        scxulong m_blockSize;      //!< Disk block size
        double m_qLength;          //!< Average disk queue length
        scxulong m_inodesTotal;    //!< Total inodes
        scxulong m_inodesFree;     //!< Free (available) inodes

        DiskInstanceDataSampler m_reads;     //!< Data sampler for reads
        DiskInstanceDataSampler m_writes;    //!< Data sampler for writes
        DiskInstanceDataSampler m_transfers; //!< Data sampler for transfers
        DiskInstanceDataSampler m_tBytes;    //!< Data sampler for bytes transfered
        DiskInstanceDataSampler m_rBytes;    //!< Data sampler for bytes read
        DiskInstanceDataSampler m_wBytes;    //!< Data sampler for bytes written
        DiskInstanceDataSampler m_waitTimes; //!< Data sampler for wait times
        DiskInstanceDataSampler m_tTimes;    //!< Data sampler for total times        
        DiskInstanceDataSampler m_rTimes;    //!< Data sampler for read times
        DiskInstanceDataSampler m_wTimes;    //!< Data sampler for write times
        DiskInstanceDataSampler m_runTimes;  //!< Data sampler for run times
        DiskInstanceDataSampler m_timeStamp; //!< Data sampler for time stamps
        DiskInstanceDataSampler m_qLengths;  //!< Data sampler for queue lengths
    };

}

#endif /* STATISTICALLOGICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
