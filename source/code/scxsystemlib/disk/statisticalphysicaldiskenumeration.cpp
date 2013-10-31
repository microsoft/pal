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
#include <scxsystemlib/statisticalphysicaldiskenumeration.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxsystemlib/staticphysicaldiskenumeration.h>
#include <sys/stat.h> 
#if defined(hpux)
#include <sys/pstat.h>
#include <scxsystemlib/scxlvmtab.h>
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
    StatisticalPhysicalDiskEnumeration::StatisticalPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : m_deps(0), m_sampler(0)
    { 
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticalphysicaldiskenumeration");
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
    StatisticalPhysicalDiskEnumeration::~StatisticalPhysicalDiskEnumeration()
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
    const std::wstring StatisticalPhysicalDiskEnumeration::DumpString() const
    {
        return L"StatisticalPhysicalDiskEnumeration";
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
    SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> StatisticalPhysicalDiskEnumeration::FindDiskByDevice(const std::wstring& device, bool includeSamplerDevice /*= false*/)
    {
        if ((0 != GetTotalInstance()) && (GetTotalInstance()->m_device == device))
        {
            return GetTotalInstance();
        }

        for (EntityIterator iter = Begin(); iter != End(); iter++)
        {
            SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = *iter;
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
        return SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Initializes the disk collection and starts the sampler thread.
    
    */
    void StatisticalPhysicalDiskEnumeration::Init()
    {
        InitInstances();

        StatisticalPhysicalDiskSamplerParam* p = new StatisticalPhysicalDiskSamplerParam();
        p->m_diskEnum = this;
        m_sampler = new SCXCoreLib::SCXThread(DiskSampler, p);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Initializes the disk instances.
    
       \note This method is a helper to the Init method and can be used directly
       if the sampler thread is not needed.
    
    */
    void StatisticalPhysicalDiskEnumeration::InitInstances()
    {
        SetTotalInstance(SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance>(new StatisticalPhysicalDiskInstance(m_deps, true)));
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Release the resources allocated.
    
       Must be called before deallocating this object. Will wait when stopping 
       the sampler thread.
    
    */
    void StatisticalPhysicalDiskEnumeration::CleanUp()
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
    void StatisticalPhysicalDiskEnumeration::Update(bool updateInstances)
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);
        FindPhysicalDisks();
        if (updateInstances)
        {
            UpdateInstances();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update all instances.

    */
    void StatisticalPhysicalDiskEnumeration::UpdateInstances()
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
        SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> total = GetTotalInstance();
        if (0 != total)
        {
            total->Reset();
            total->m_online = true;
        }
        
        for (EntityIterator iter = Begin(); iter != End(); iter++)
        {
            SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = *iter;
            disk->Update();
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
                total->m_mbFree += disk->m_mbFree;
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
    void StatisticalPhysicalDiskEnumeration::SampleDisks()
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);
#if defined(linux)
        m_deps->RefreshProcDiskStats();
#endif
        for (EntityIterator iter = Begin(); iter != End(); iter++)
        {
            SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = *iter;

            disk->Sample();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       The disk sampler thread body.
    
       \param       param - thread parameters.

    */
    void StatisticalPhysicalDiskEnumeration::DiskSampler(SCXCoreLib::SCXThreadParamHandle& param)
    {
        StatisticalPhysicalDiskSamplerParam* p = static_cast<StatisticalPhysicalDiskSamplerParam*>(param.GetData());
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
                                     std::wstring(L"StatisticalPhysicalDiskEnumeration::DiskSampler() - Unexpected exception caught: ").append(e.What()).append(L" - ").append(e.Where()));
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
       Discover physical disks.
    
       Logical disks are identified by the /etc/mnttab file (by design). Physical
       disks discovered will be those "hosting" the logical disks found. If ever
       seen in that file, the disk will be discovered. If the disk is removed it 
       will be marked as offline.

       How to identify physical disks from logical disks:

       Linux --

       Logical disks are named things like /dev/hda0, /dev/hda1 and so on.
       The numeric value in the end of the name is the partition/logical ID
       of the physical disk. In the example above both logical disks are on
       physical disk /dev/hda.

       For LVM partitions on Linux have two entries for the same device.  An LVM
       device entry is stored in the /dev/mapper directory with a name in the form
       <logical-volume-group>-<logical-volume>.  There is also a device mapper (dm)
       device entry stored in the /dev directory in the form dm-<id>.  This is a
       1-to-1 relationship and the <id> is equal to the device minor ID that both
       device entries have in common.  Discovery of the physical device(s) that
       contain the LVM partition is done by mapping the LVM device to the dm device,
       then looking at the dm devices slave entries in Sysfs, and then finally
       performing the same conversion from a logical Linux partition name to a
       physical drive name that is done for all other partitions.

       Solaris --

       Logical disks are named things like /dev/dsk/c1t0d0s0, /dev/dsk/c1t0d0s1
       and so on. The last letter/numeric pair is the partition/logical ID
       of the physical disk. In the example above both logical disks are on
       physical disk /dev/dsk/c1t0d0.

       HPUX --

       Logical disks are logical volumes with names like /dev/vg00/lvol3.
       /dev/vg00 in the example name is the volume group name. Using the /etc/lvmtab
       file the volume group can be translated to a partition named /dev/disk/disk3_p2
       (or /dev/dsk/c2t0d0s2 using a naming standard deprecated as of HPUX 11.3). The
       old naming standard works like the one for solaris while the new one identifies
       the physical disk as /dev/disk/disk3 in the example above.

       AIX --

       TODO: Document how disks are enumerated on AIX.
    */
    void StatisticalPhysicalDiskEnumeration::FindPhysicalDisks()
    {
        for (EntityIterator iter=Begin(); iter!=End(); iter++)
        {
            SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = *iter;
            disk->m_online = false;
        }

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin(); 
             it != m_deps->GetMNTTab().end(); it++)
        {
            if ( ! m_deps->FileSystemIgnored(it->fileSystem) &&
                 ! m_deps->DeviceIgnored(it->device) &&
                 m_deps->LinkToPhysicalExists(it->fileSystem, it->device, it->mountPoint) )
            {
                std::map<std::wstring, std::wstring> devices = m_deps->GetPhysicalDevices(it->device);
                if (devices.size() == 0)
                {
                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
                    std::wstringstream               out;

                    out << L"Unable to locate physical devices for: " << it->device;
                    SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
                    continue;
                }
                for (std::map<std::wstring, std::wstring>::const_iterator dev_it = devices.begin();
                     dev_it != devices.end(); dev_it++)
                {
                    SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = AddDiskInstance(dev_it->first, dev_it->second);
#if defined(hpux)
                    if (0 != disk)
                    {
                        if (m_pathToRdev.end() == m_pathToRdev.find(disk->m_device))
                        {
                            SCXCoreLib::SCXFilePath fp(disk->m_device);
                            fp.SetFilename(L"");
                            UpdatePathToRdev(fp.Get());
                        }
                        SCXASSERT(m_pathToRdev.end() != m_pathToRdev.find(disk->m_device));

                        scxlong diskInfoIndex = disk->FindDiskInfoByID(m_pathToRdev.find(disk->m_device)->second);
                        m_deps->AddDeviceInstance(disk->m_device, L"", diskInfoIndex, m_pathToRdev.find(disk->m_device)->second);
                    }
#endif
                }
            }
        }

#if defined(sun)
        this->UpdateSolarisHelper();
#endif

    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Helper for the Solaris platform. Not all disks are available from
       MNTTAB on this platform, this it is necessary to perform some additional
       searching of the file system.
    */
    void StatisticalPhysicalDiskEnumeration::UpdateSolarisHelper()
    {
        // workaround for unknown FS/devices
        // try to get a list of disks from /dev/dsk
        SCXCoreLib::SCXDirectoryInfo oDisks( L"/dev/dsk/" );

        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > disk_infos = oDisks.GetSysFiles();
        std::map< std::wstring, int > found_devices;

        // iterate through all devices 
        for ( unsigned int i = 0; i < disk_infos.size(); i++ ){
            std::wstring dev_name = disk_infos[i]->GetFullPath().GetFilename();

            dev_name = dev_name.substr(0,dev_name.find_last_not_of(L"0123456789"));

            if ( found_devices.find( dev_name ) != found_devices.end() )
                continue; // already considered

            found_devices[dev_name] = 0;

            try {
                SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = FindDiskByDevice(dev_name);

                if ( disk == 0 ){
                    disk = new StatisticalPhysicalDiskInstance(m_deps);
                    disk->SetId(dev_name);
                    disk->m_device = disk_infos[i]->GetDirectoryPath().Get() + dev_name;
                    disk->m_online = true;

                    // verify that hardware is accessible by calling 'physical' disk instance
                    {
                        SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk_physical;
                        
                        disk_physical = new StaticPhysicalDiskInstance(m_deps);
                        disk_physical->SetId(dev_name);
                        disk_physical->SetDevice(disk_infos[i]->GetDirectoryPath().Get() + dev_name);
                        // update will throw exception if disk is not accessible
                        disk_physical->Update();
                    }
                    
                    AddInstance(disk);
                } else {
                    if ( !disk->m_online ){
                        // verify if dsik is online
                        {
                            SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk_physical;
                            
                            disk_physical = new StaticPhysicalDiskInstance(m_deps);
                            disk_physical->SetId(dev_name);
                            disk_physical->SetDevice(disk_infos[i]->GetDirectoryPath().Get() + dev_name);
                            disk_physical->Update();
                        }
                        
                        disk->m_online = true;
                    }
                }
                
            } catch ( SCXCoreLib::SCXException& e )
            {
                //std::wcout << L"excp in dsk update: " << e.What() << endl << e.Where() << endl;
                // ignore errors, since disk may not be accessible and it's fine
            }
        }
    }
#endif
             
    /*----------------------------------------------------------------------------*/
    /**
       Updates the path to rdev map.

       \param[in]   dir directory to update.

       Will scan the given directory and update the path to rdev map with the rdev
       value of all files in given directory.
    */
    void StatisticalPhysicalDiskEnumeration::UpdatePathToRdev(const std::wstring& dir)
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
       Add a new disk instacne if it does not already exist.
    
       \param   name name of instance.
       \param   device device string (only used if new instance created).
       \returns NULL if a disk with the given name already exists - otherwise the new disk.

       \note The disk will be marked as online if found.
    */
    SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> StatisticalPhysicalDiskEnumeration::AddDiskInstance(const std::wstring& name, const std::wstring& device)
    {
        SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> disk = FindDiskByDevice(device);
        if (0 == disk)
        {
            disk = new StatisticalPhysicalDiskInstance(m_deps);
            disk->SetId(name);
            disk->m_device = device;
            disk->m_online = true;
            AddInstance(disk);
            return disk;
        }
        disk->m_online = true;
        return SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove an instance with given id

        \param id Id of instance to remove.
        \returns true if the Id was found, otherwise false

        \note The removed instance is deleted.
    */
    bool StatisticalPhysicalDiskEnumeration::RemoveInstanceById(const EntityInstanceId& id)
    {
        SCXCoreLib::SCXThreadLock lock(m_lock);

        return EntityEnumeration<StatisticalPhysicalDiskInstance>::RemoveInstanceById(id);

    }
   
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
