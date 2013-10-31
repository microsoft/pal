/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implements the physical disk enumeration pal for static information.
    
    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/staticphysicaldiskenumeration.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdirectoryinfo.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Constructor.
    
       \param       deps A StaticDiscDepend object which can be used.
        
    */
    StaticPhysicalDiskEnumeration::StaticPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : m_deps(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticphysicaldiskenumeration");
        m_deps = deps;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Virtual destructor.
    */
    StaticPhysicalDiskEnumeration::~StaticPhysicalDiskEnumeration()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Init method.
       
       Initial caching of data is performed here.

    */
    void StaticPhysicalDiskEnumeration::Init()
    {
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Cleanup method.

       Release of cached resources.

    */
    void StaticPhysicalDiskEnumeration::CleanUp()
    {
        EntityEnumeration<StaticPhysicalDiskInstance>::CleanUp();
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Update the enumeration.

       \param updateInstances If true (default) all instances will be updated.
                              Otherwise only the content of the enumeration will be updated.

    */
    void StaticPhysicalDiskEnumeration::Update(bool updateInstances/*=true*/)
    {
        for (EntityIterator iter=Begin(); iter!=End(); iter++)
        {
            SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk = *iter;
            disk->m_online = false;
        }

#if defined(linux)
        // First detect optical devices so later we can just skip the mount points which we determined are optical disks.
        // Various projects may or may not require optical drive detection. We check if iso9660 file system is to be
        // ignored to determine if optical drives should be detected.
        std::vector<std::wstring> drives;
        if ( ! m_deps->FileSystemIgnored(L"iso9660") )
        {
            // Get CD-ROM and DVD drives directly from the kernel interface in /proc.
            SCXCoreLib::SCXHandle<std::wistream> cdStrm = m_deps->GetWIStream("/proc/sys/dev/cdrom/info");
            std::vector<std::wstring> cdStrmLines;
            SCXCoreLib::SCXStream::NLFs nlfs;
            // /proc/sys/dev/cdrom/info format:
            // CD-ROM information, Id: cdrom.c 3.20 2003/12/17
            //
            // drive name:             sr0      hdc
            // drive speed:            0        0
            // drive # of slots:       1        1
            // ...
            SCXCoreLib::SCXStream::ReadAllLines(*cdStrm, cdStrmLines, nlfs);
            std::wstring lineID(L"drive name:");
            size_t i;
            for (i = 0; i < cdStrmLines.size(); i++)
            {
                if (cdStrmLines[i].substr(0, lineID.size()) == lineID)
                {
                    std::wstring drivesLine = cdStrmLines[i].substr(lineID.size(), std::wstring::npos);
                    SCXCoreLib::StrTokenize(drivesLine, drives, L" \t");
                    size_t d;
                    for (d = 0; d < drives.size(); d++)
                    {
                        AddDiskInstance(L"/dev/" + drives[d], L"/dev/" + drives[d], true);
                    }                
                    break;
                }
            }
        }
#endif

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin(); 
             it != m_deps->GetMNTTab().end(); it++)
        {
#if defined(linux)
            // It this is an optical device just skip it. We already processed it.
            size_t d;
            bool found = false;
            for (d = 0; d < drives.size(); d++)
            {
                if ((L"/dev/" + drives[d]) == it->device)
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                continue;
            }
#endif
            if ( ! m_deps->FileSystemIgnored(it->fileSystem) &&
                 ! m_deps->DeviceIgnored(it->device) &&
                 m_deps->LinkToPhysicalExists(it->fileSystem, it->device, it->mountPoint) )
            {
                std::map<std::wstring, std::wstring> devices = m_deps->GetPhysicalDevices(it->device);
                if (devices.size() == 0)
                {
                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
                    std::wstringstream                  out;

                    out << L"Unable to locate physical devices for: " << it->device;
                    SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
                    continue;
                }
                for (std::map<std::wstring, std::wstring>::const_iterator dev_it = devices.begin();
                     dev_it != devices.end(); dev_it++)
                {
                    SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk = AddDiskInstance(dev_it->first, dev_it->second);
                }
            }
        }
#if defined(sun)
        this->UpdateSolarisHelper();
#endif

        if (updateInstances)
        {
            UpdateInstances();
        }
    }

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Helper for the Solaris platform. Not all disks are available from
       MNTTAB on this platform, this it is necessary to perform some additional
       searching of the file system.
    */
    void StaticPhysicalDiskEnumeration::UpdateSolarisHelper()
    {
        // workaround for unknown FS/devices
        // try to get a list of disks from /dev/dsk
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > disk_infos = m_deps->GetDevDskInfo();
        std::map< std::wstring, int > found_devices;

        // iterate through all devices 
        for ( unsigned int i = 0; i < disk_infos.size(); i++ ){
            std::wstring dev_name = disk_infos[i]->GetFullPath().GetFilename();

            dev_name = dev_name.substr(0,dev_name.find_last_not_of(L"0123456789"));

            if ( found_devices.find( dev_name ) != found_devices.end() )
                continue; // already considered

            found_devices[dev_name] = 0;

            try {
                SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk = GetInstance(dev_name);

                if ( disk == 0 ){
                    disk = new StaticPhysicalDiskInstance(m_deps);
                    disk->SetId(dev_name);
                    disk->m_device = disk_infos[i]->GetDirectoryPath().Get() + dev_name;
                    disk->m_online = true;
                    // NOTE: Update will throw in case if disk is removable media, so 
                    // we will skip it (no call to AddInstance)
                    disk->Update();
                    AddInstance(disk);
                   
                } else {
                    disk->Update(); // check if disk is still 'alive'
                    // if disk goes off-line, Update throws and status remains 'false'
                    disk->m_online = true;
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
       Add a new disk instance if it does not already exist.

       \param   name name of instance.
       \param   device device string (only used if new instance created).
       \param   cdDrive device is an optical drive.
       \returns NULL if a disk with the given name already exists - otherwise the new disk.

       \note The disk will be marked as online if found.
    */
    SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> StaticPhysicalDiskEnumeration::AddDiskInstance(
        const std::wstring& name, const std::wstring& device
#if defined(linux)
        , bool cdDrive
#endif
        )
    {
        SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> disk = GetInstance(name);
        if (0 == disk)
        {
            disk = new StaticPhysicalDiskInstance(m_deps);
            disk->SetId(name);
            disk->m_device = device;
            disk->m_online = true;
#if defined(linux)
            disk->m_cdDrive = cdDrive;
#endif
            AddInstance(disk);
            return disk;
        }
        disk->m_online = true;
        return SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance>(0);
    }
    
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
