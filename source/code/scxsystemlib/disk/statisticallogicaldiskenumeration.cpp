/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implements the physical disk enumeration pal for statistical information.

    \date        2008-04-28 15:20:00

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxexception.h>
#include <scxsystemlib/statisticallogicaldiskenumeration.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxmath.h>
#include <sys/stat.h>
#if defined(hpux)
#include <sys/pstat.h>
#include <scxsystemlib/scxlvmtab.h>
#elif defined(linux)
#include <scxsystemlib/scxlvmutils.h>
#elif defined(sun)
#include <scxsystemlib/scxkstat.h>
#endif

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param       deps - dependencies

    */
    StatisticalLogicalDiskEnumeration::StatisticalLogicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : m_deps(0), m_sampler(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticallogicaldiskenumeration");
        m_lock = SCXCoreLib::ThreadLockHandleGet();
        m_deps = deps;
#if defined(hpux)
        // Try to init LVM TAB and log errors.
        try
        {
            m_deps->GetLVMTab();
        }
        catch(SCXCoreLib::SCXException& e)
        {
            SCX_LOGERROR(m_log, e.What());
            throw;
        }
        UpdatePathToRdev(L"/dev/dsk/");
        UpdatePathToRdev(L"/dev/disk/");
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor

       Kills the sampler thread hard if not shut down gracefully (by using CleanUp).

    */
    StatisticalLogicalDiskEnumeration::~StatisticalLogicalDiskEnumeration()
    {
        if (0 != m_sampler)
        {
            if (m_sampler->IsAlive())
            {
                CleanUp();
            }
            m_sampler = 0;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Dump the object as a string for logging purposes.

       \returns     String representation of object, suitable for logging.

    */
    const std::wstring StatisticalLogicalDiskEnumeration::DumpString() const
    {
        return L"StatisticalLogicalDiskEnumeration";
    }

    /*----------------------------------------------------------------------------*/
    /**
       Find a disk instance given its device.

       \param       device - Disk device to search for.
       \param       includeSamplerDevice - if true sampler devices (which may be
       different) are also searched. Default is false.
       \returns     Pointer to disk instance with given device or zero if not found.

       Searching for "/dev/sda" or "sda" will return the same instance.

    */
    SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> StatisticalLogicalDiskEnumeration::FindDiskByDevice(const std::wstring& device, bool includeSamplerDevice /*= false*/)
    {
        if ((0 != GetTotalInstance()) && (GetTotalInstance()->m_device == device))
        {
            return GetTotalInstance();
        }

        for (EntityIterator iter = Begin(); iter != End(); ++iter)
        {
            SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> disk = *iter;
            SCXCoreLib::SCXFilePath path(disk->m_device);
            if ((disk->m_device == device) || (path.GetFilename() == device))
            {
                return disk;
            }
            if (includeSamplerDevice)
            {
                for (size_t i=0; i < disk->m_samplerDevices.size(); ++i)
                {
                    path.Set(disk->m_samplerDevices[i]);
                    if ((disk->m_samplerDevices[i] == device) || (path.GetFilename() == device))
                    {
                        return disk;
                    }
                }
            }
        }
        return SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Initializes the disk collection and starts the sampler thread.

    */
    void StatisticalLogicalDiskEnumeration::Init()
    {
        InitInstances();

        StatisticalLogicalDiskSamplerParam* p = new StatisticalLogicalDiskSamplerParam();
        p->m_diskEnum = this;
        m_sampler = new SCXCoreLib::SCXThread(DiskSampler, p);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Initializes the disk instances.

       \note This method is a helper to the Init method and can be used directly
       if the sampler thread is not needed.

    */
    void StatisticalLogicalDiskEnumeration::InitInstances()
    {
        SetTotalInstance(
            SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance>(
            new StatisticalLogicalDiskInstance(m_deps, true) ));
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Release the resources allocated.

       Must be called before deallocating this object. Will wait when stopping
       the sampler thread.

    */
    void StatisticalLogicalDiskEnumeration::CleanUp()
    {
        if (0 != m_sampler)
        {
            m_sampler->RequestTerminate();
            m_sampler->Wait();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update the enumeration potentially discovering new instances.

       \param       updateInstances If true, update state of all instances in collection.
       \throws      SCXInternalErrorException If object is of unknown disk enumeration type.

    */
    void StatisticalLogicalDiskEnumeration::Update(bool updateInstances)
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);
        FindLogicalDisks();

        if (updateInstances)
        {
            UpdateInstances();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update all instances.

    */
    void StatisticalLogicalDiskEnumeration::UpdateInstances()
    {
        scxulong total_reads = 0;
        scxulong total_writes = 0;
#if defined(hpux)
        scxulong total_tTime = 0;
#endif
        scxulong total_rTime = 0;
        scxulong total_wTime = 0;
        scxulong total_transfers = 0;
        scxulong total_rPercent = 0;
        scxulong total_wPercent = 0;
        scxulong total_tPercent = 0;

        set<wstring> diskSet;
        pair<set<wstring>::iterator,bool> Pair;
        bool excludeDeviceFreeSpace=false;

        SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> total = GetTotalInstance();
        if (0 != total)
        {
            total->Reset();
            SCX_LOGTRACE(m_log, L"Device being set to ONLINE for TOTAL instance");
            total->m_online = true;
        }

        for (EntityIterator iter = Begin(); iter != End(); ++iter)
        {
            SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> disk = *iter;
            disk->Update();

            /***
            * Fix for Bug10019956:
            *
            *   Suppose /etc/mnttab file has following content. (On Solaris /etc/mnttab files is parsed to get the mounted device names)
            *    ----------------------------------------------------------------------------------------------------------------------------------
            *    [Mounted Device]                [Mount point]   [FS Type]     [Attributes]
            *    ----------------------------------------------------------------------------------------------------------------------------------
            *    rpool/ROOT/s10s_u11wos_24a      /               zfs                                                          dev=4010002        0
            *    rpool/export                    /export         zfs     rw,devices,setuid,nonbmand,exec,rstchown,xattr,atime,dev=4010003        1490816764
            *    rpool/export/home               /export/home    zfs     rw,devices,setuid,nonbmand,exec,rstchown,xattr,atime,dev=4010004        1490816764
            *    rpool                           /rpool          zfs     rw,devices,setuid,nonbmand,exec,rstchown,xattr,atime,dev=4010005        1490816764
            *    rpool/fs1                       /rpool/fs1      zfs     rw,devices,setuid,nonbmand,exec,rstchown,xattr,atime,dev=4010006        1491216737
            *    ----------------------------------------------------------------------------------------------------------------------------------
            *
            *    Output of df -h(It shows the available free space for each FS in 4th column)
            *    -----------------------------------------------------------------------
            *    Filesystem                      size   used  avail capacity  Mounted on
            *    ------------------------------------------------------------------------
            *    rpool/ROOT/s10s_u11wos_24a      98G   7.4G    81G     9%    /
            *    rpool/export                    98G    32K    81G     1%    /export
            *    rpool/export/home               98G   7.4G    81G     9%    /export/home
            *    rpool                           98G   107K    81G     1%    /rpool
            *    rpool/fs1                       98G    98M    81G     1%    /rpool/fs1
                -------------------------------------------------------------------------
            *
            *    Here all zfs partitions are on zpool designated as "rpool".
            *
            *    All mounted devices are of 'zfs' type.
            *    In this exampe all but 'rpool' mounted device name has pattern "/".
            *    Hence 'rpool' is the name of the zpool and free space in it should be considered for free space calculation.
            ***/
#if defined(sun)
            if (excludeDeviceFreeSpace)
                excludeDeviceFreeSpace = false;

            if(disk->m_fsType == L"zfs" && (disk->m_device).find(L"/") != wstring::npos)
                excludeDeviceFreeSpace=true;
#endif

            Pair = diskSet.insert(disk->m_device);
            if(Pair.second == false)
                continue;

            if (0 != total)
            {
                total->m_readsPerSec += disk->m_readsPerSec;
                total->m_writesPerSec += disk->m_writesPerSec;
                total->m_transfersPerSec += disk->m_transfersPerSec;
                total->m_rBytesPerSec += disk->m_rBytesPerSec;
                total->m_wBytesPerSec += disk->m_wBytesPerSec;
                total->m_tBytesPerSec += disk->m_tBytesPerSec;
                total->m_rTime += disk->m_rTime;
                total->m_wTime += disk->m_wTime;
                total->m_tTime += disk->m_tTime;
                total->m_runTime += disk->m_runTime;
                total->m_waitTime += disk->m_waitTime;
                total->m_mbUsed += disk->m_mbUsed;
                total->m_mbFree += excludeDeviceFreeSpace?0:disk->m_mbFree;
                total_reads += disk->m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_writes += disk->m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
#if defined (hpux)
                total_transfers += disk->m_transfers.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_tTime += disk->m_tTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
#elif defined (linux)
                total_transfers += disk->m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + disk->m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_rTime += disk->m_rTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_wTime += disk->m_wTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
#elif defined (sun)
                total_transfers += disk->m_reads.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES) + disk->m_writes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_rTime += disk->m_runTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
                total_wTime += disk->m_waitTimes.GetDelta(MAX_DISKINSTANCE_DATASAMPER_SAMPLES);
#endif
                total_rPercent += disk->m_rPercentage;
                total_wPercent += disk->m_wPercentage;
                total_tPercent += disk->m_tPercentage;
            }
        }

        if (0 != total)
        {
            if (Size() > 0)
            {
                total->m_rPercentage = total_rPercent/Size();
                total->m_wPercentage = total_wPercent/Size();
                total->m_tPercentage = total_tPercent/Size();
            }

            if (total_reads != 0)
            {
                total->m_secPerRead = static_cast<double>(total_rTime) / static_cast<double>(total_reads) / 1000.0;
            }
            if (total_writes != 0)
            {
                total->m_secPerWrite = static_cast<double>(total_wTime) / static_cast<double>(total_writes) / 1000.0;
            }
            if (total_transfers != 0)
            {
#if defined(hpux)
                total->m_secPerTransfer = static_cast<double>(total_tTime) / static_cast<double>(total_transfers) / 1000.0;
#elif defined(linux) || defined(sun)
                total->m_secPerTransfer = static_cast<double>(total_rTime+total_wTime) / static_cast<double>(total_transfers) / 1000.0;
#endif
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Store sample data for all instances in collection.

    */
    void StatisticalLogicalDiskEnumeration::SampleDisks()
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);
#if defined(linux)
        m_deps->RefreshProcDiskStats();
#endif
        for (EntityIterator iter = Begin(); iter != End(); ++iter)
        {
            SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> disk = *iter;

            try {
                disk->Sample();
            }
            catch (const SCXCoreLib::SCXException& e)
            {
                SCX_LOGERROR(m_log,
                            std::wstring(L"StatisticalLogicalDiskEnumeration::SampleDisks() - Unexpected exception caught: ").append(
                            e.What()).append(L" - ").append(e.Where()).append(
                            L"; for logical disk ").append(disk->m_device) );
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       The disk sampler thread body.

       \param       param - thread parameters.

    */
    void StatisticalLogicalDiskEnumeration::DiskSampler(SCXCoreLib::SCXThreadParamHandle& param)
    {
        StatisticalLogicalDiskSamplerParam* p = static_cast<StatisticalLogicalDiskSamplerParam*>(param.GetData());
        SCXASSERT(0 != p);
        SCXASSERT(0 != p->m_diskEnum);

        bool bUpdate = true;
        p->m_cond.SetSleep(DISK_SECONDS_PER_SAMPLE * 1000);
        {
            SCXCoreLib::SCXConditionHandle h(p->m_cond);
            while( ! p->GetTerminateFlag())
            {
                if (bUpdate)
                {
                    try
                    {
                        p->m_diskEnum->SampleDisks();
                    }
                    catch (const SCXCoreLib::SCXException& e)
                    {
                        SCX_LOGERROR(p->m_diskEnum->m_log,
                                     std::wstring(L"StatisticalLogicalDiskEnumeration::DiskSampler() - Unexpected exception caught: ").append(e.What()).append(L" - ").append(e.Where()));
                    }
                    bUpdate = false;
                }

                enum SCXCoreLib::SCXCondition::eConditionResult r = h.Wait();
                if (SCXCoreLib::SCXCondition::eCondTimeout == r)
                {
                    bUpdate = true;
                }
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Discover logical disks.

       Logical disks are identified by the /etc/mnttab file (by design). If ever
       seen in that file, the disk will be discovered. If the disk is removed it
       will be marked as offline.

    */
    void StatisticalLogicalDiskEnumeration::FindLogicalDisks()
    {
        SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Size of enumeration: ", this->Size()));
        for (EntityIterator iter=Begin(); iter!=End(); ++iter)
        {
            SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> disk = *iter;
            SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Device being set to OFFLINE, disk: ", disk->m_mountPoint));
            disk->m_online = false;
        }

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin();
             it != m_deps->GetMNTTab().end(); ++it)
        {
            if ( ! m_deps->FileSystemIgnored(it->fileSystem) && ! m_deps->DeviceIgnored(it->device))
            {
                SCXCoreLib::SCXHandle<StatisticalLogicalDiskInstance> disk = GetInstance(it->mountPoint);
                if (0 == disk)
                {
                    disk = new StatisticalLogicalDiskInstance(m_deps);
                    disk->m_device = it->device;
                    disk->m_mountPoint = it->mountPoint;
                    disk->m_fsType = it->fileSystem;
                    disk->SetId(disk->m_mountPoint);

#if defined(linux)
                    static SCXLVMUtils lvmUtils;

                    if (lvmUtils.IsDMDevice(it->device))
                    {
                    try
                    {
                            // Try to convert the potential LVM device path into its matching
                            // device mapper (dm) device path.
                            std::wstring dmDevice = lvmUtils.GetDMDevice(it->device);

                            SCXASSERT(!dmDevice.empty());
                            disk->m_samplerDevices.push_back(dmDevice);
                    }
                        catch (SCXCoreLib::SCXException& e)
                    {
                            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
                            std::wstringstream               out;

                            out << L"An exception occurred resolving the dm device that represents the LVM partition " << it->device
                                << L" : " << e.What();
                            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
                        }
                    }
                    // no else required; device was not an LVM device
#endif

                    AddInstance(disk);

#if defined(hpux)
                    if (m_pathToRdev.end() == m_pathToRdev.find(disk->m_device))
                    {
                        SCXCoreLib::SCXFilePath fp(disk->m_device);
                        fp.SetFilename(L"");
                        UpdatePathToRdev(fp.Get());
                    }
                    SCXASSERT(m_pathToRdev.end() != m_pathToRdev.find(disk->m_device));

                    m_deps->AddDeviceInstance(disk->m_device, L"", disk->FindLVInfoByID(m_pathToRdev.find(disk->m_device)->second), m_pathToRdev.find(disk->m_device)->second);
#endif
                }
                SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Device being set to ONLINE, disk: ", disk->m_mountPoint));
                disk->m_online = true;
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Updates the path to rdev map.

       \param[in]   dir directory to update.

       Will scan the given directory and update the path to rdev map with the rdev
       value of all files in given directory.
    */
    void StatisticalLogicalDiskEnumeration::UpdatePathToRdev(const std::wstring& dir)
    {
        std::vector<SCXCoreLib::SCXFilePath> files;
        m_deps->GetFilesInDirectory(dir, files);

        for(size_t i=0; i < files.size(); ++i)
        {
            struct stat s;
            if (0 == m_deps->lstat(SCXCoreLib::StrToUTF8(files[i].Get()).c_str(), &s))
            {
                m_pathToRdev[files[i].Get()] = s.st_rdev;
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove an instance with given id

        \param id Id of instance to remove.
        \returns true if the Id was found, otherwise false

        \note The removed instance is deleted.
    */
    bool StatisticalLogicalDiskEnumeration::RemoveInstanceById(const EntityInstanceId& id)
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);

        return EntityEnumeration<StatisticalLogicalDiskInstance>::RemoveInstanceById(id);

    }
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
