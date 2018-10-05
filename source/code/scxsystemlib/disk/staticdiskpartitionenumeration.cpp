/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file        staticdiskpartitionenumeration.cpp

   \brief       Implementation of the Enumeration of Static Disk Partition 

   \date        09-12-11 14:12:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/logsuppressor.h>

#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/staticdiskpartitionenumeration.h>
#include <scxsystemlib/staticdiskpartitioninstance.h>

#if defined(hpux)
#include <sys/diskio.h>
#endif
#if defined(aix)
#include <lvm.h>
#endif

using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /** Module name string */
    const wchar_t *StaticDiskPartitionEnumeration::moduleIdentifier = L"scx.core.common.pal.system.disk.staticdiskpartitionenumeration";

    /*----------------------------------------------------------------------------*/
    /**
       Standard Constructor
    */
    StaticDiskPartitionEnumeration::StaticDiskPartitionEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) :
        m_deps(deps), m_log(SCXLogHandleFactory::GetLogHandle(moduleIdentifier))
    {
        SCX_LOGTRACE(m_log, L"StaticDiskPartitionEnumeration Standard constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor.
    */
    StaticDiskPartitionEnumeration::~StaticDiskPartitionEnumeration()
    {
        SCX_LOGTRACE(m_log, L"StaticDiskPartitionEnumeration::~StaticDiskPartitionEnumeration()");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Creates the total DiskPartition instances.
    */
    void StaticDiskPartitionEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"StaticDiskPartitionEnumeration Init()");
        Update(true);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Cleanup method.

       Release of cached resources.

    */
    void StaticDiskPartitionEnumeration::CleanUp()
    {
        EntityEnumeration<StaticDiskPartitionInstance>::CleanUp();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update the enumeration.

       \param updateInstances If true (default) all instances will be updated.
       Otherwise only the content of the enumeration will be updated.

    */
    void StaticDiskPartitionEnumeration::Update(bool updateInstances /*=true*/)
    {
        SCX_LOGTRACE(m_log, L"StaticDiskPartitionEnumeration Update() Entering . . .");
        if (!updateInstances)
        {
            return;
        }

#if defined(linux)

        /************* Start Linux ************************************/
        int                major     = 0;                  // major number of a partition
        int                minor     = 0;                  // minor number of a partition
        scxulong           blocks    = 0;                  // blocks of a partition
        vector<std::wstring>  allLines;                       // all lines read from partitions table
        std::wstring          partname;                       // name of the partition
        const std::wstring    dev_dir = L"/dev/";              // device directory
        std::map<std::wstring, std::wstring> partitions;
        std::string partedOutput;

        if (!GetPartedOutput(partedOutput) || !ParsePartedOutput(partedOutput, partitions))
        {
            // An error happened and was logged
            return;
        }

        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(m_deps->LocateProcPartitions(), allLines, nlfs);

        // Let's read through line-by-line the /proc/partitions pseudo-file.
        for (vector<std::wstring>::iterator it = allLines.begin(); it != allLines.end(); it++)
        {
            std::wstring devPath;
            std::wistringstream is(*it);

            // Each line of /proc/partitions has four fields. Here's the header:
            //     major minor  #blocks  name
            if (is>>major>>minor>>blocks>>partname)
            {
                devPath = dev_dir + partname;
            }
            else
            {
                SCX_LOGINFO(m_log, L"This line in /proc/partitions doesn't contain Blocksize and Partition Name. line:" + is.str());
                continue;
            }

            //Partition names end in the partitionIndex, skip the 'dm-*' because those are device-mapped
            if (m_deps->FileExists(devPath) && isdigit(partname[partname.size()-1]) && (std::wstring::npos == partname.find(L"-"))) 
            {
                // Check existence first to prevent duplicate instances
                SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partit = GetInstance(partname);
                if (NULL == partit)
                {
                    partit = new StaticDiskPartitionInstance();
                }
                partit->SetId(partname);
                partit->m_deviceID = devPath;
                //Last chars of partition name is the index. Need to handle 1, 2 or 3 digits
                partit->m_index = SCXCoreLib::StrToUInt(partname.substr(partname.find_first_of(L"0123456789"))); 

                // Double check to make sure this DevicePath is also listed with parted
                if (partitions.find(devPath) != partitions.end())
                {
                    AddInstance(partit);
                    std::wstring detail = partitions.find(devPath)->second;
                    // Set whether this is a boot partition
                    partit->m_bootPartition = (detail.find(L"boot") != std::string::npos);
                }
                else
                {
                    SCX_LOGINFO(m_log, L"This partition is listed in /proc/partitions, but not in parted: Name: " + partname);
                    //delete partit;  Handle destructed when exiting this block
                }
            }

        } //EndFor
        /************* End Linux **************************************/

#elif defined(sun)
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

        // Sun doesn't have /proc/partitions or /proc/mounts, so we need to handle it differently.
        const std::wstring dev_dsk_dir = L"/dev/dsk";              // device directory

        bool firstInstance = true;
        std::wstring bootp;
        // ZFS partition indices start after regular partitions are assigned their indices.
        size_t zfsFirstIndex = 0;

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin(); 
             it != m_deps->GetMNTTab().end(); it++)
        {
            SCX_LOGTRACE(m_log, L"DPEnum::Update():: Inside FOR loop, Device=" + it->device + L"  And file=" + it->fileSystem + L"  MountPt=" + it->mountPoint);

            if (! m_deps->FileSystemIgnored(it->fileSystem) && ! m_deps->DeviceIgnored(it->device) &&
                (std::wstring::npos != (it->device).find(dev_dsk_dir)))
            {

                SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partit = GetInstance(it->device);
                if (0 == partit)
                {
                    partit = new StaticDiskPartitionInstance();
                    std::wstring devPath(it->device);
                    std::wstring partname = devPath.substr(devPath.find_last_of(L"/")+1);

                    partit->SetId(partname);
                    partit->m_deviceID = devPath;
                    //Last char of partition name is the index.
                    partit->m_index = 
                    SCXCoreLib::StrToUInt(partname.substr(partname.find_last_of(L"0123456789")));
                    if ((partit->m_index + 1) > zfsFirstIndex)
                    {
                        // ZFS partition indices start after last regular partition index.
                        zfsFirstIndex = partit->m_index + 1;
                    }

                    //Getting the bootdrive is expensive, let's just do it once. 
                    if (firstInstance)
                    {
                        if (partit->GetBootDrivePath(bootp))
                        {
                            firstInstance = false;
                        }
                    }

                    if (devPath == bootp)
                    {
                        partit->m_bootPartition = true;
                    }
                    else
                    {
                        partit->m_bootPartition = false;
                    }                   

                    AddInstance(partit);

                }
            }
            else if (SCXCoreLib::StrToLower(it->fileSystem) == L"zfs")
            {
                // It is ZFS partition.
                SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partit = GetInstance(it->device);
                if (NULL == partit)
                {
                    partit = new StaticDiskPartitionInstance();
                    partit->m_isZfsPartition = true;
                    std::wstring zfsPath(it->device);
                    partit->SetId(zfsPath);
                    partit->m_deviceID = zfsPath;
                    // Get filesystem info.
                    struct statvfs64 stat;
                    memset(&stat, 0, sizeof(stat));
                    if (m_deps->statvfs64(SCXCoreLib::StrToUTF8(it->mountPoint).c_str(), &stat) == 0)
                    {
                        partit->m_blockSize = stat.f_frsize;
                        partit->m_numberOfBlocks = stat.f_blocks;
                        partit->m_partitionSize = partit->m_blockSize * partit->m_numberOfBlocks;
                    }
                    else
                    {
                        // Just log in case we can not get file system data.
                        std::wstring noStatvfs = L"Statvfs failed for mountpoint \"" +
                        it->mountPoint + L"\", errno = " + StrFrom(errno);
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(noStatvfs));
                        SCX_LOG(m_log, severity, noStatvfs);
                    }
                    // ZFS boot process works in a way that first the active boot environment is determined. Then
                    // the OS installed in the root file system of that boot environment is loaded. Finally the the
                    // root file system is mounted at '/'. Therefore the file system with the mountpoint '/' represents
                    // the boot file system. If administrator mounts some other root file system at some other mount
                    // point, this file system will be returned in our list of disk partitions as well. However,
                    // although it is possible to boot from that root file system as well, it will not be marked as
                    // a bootable file system.
                    if (it->mountPoint == L"/")
                    {
                        partit->m_bootPartition = true;
                    }
                    else
                    {
                        partit->m_bootPartition = false;
                    }
                    AddInstance(partit);
                }
            }
        }
        // All regular partitions received their indices. Continue assigning indices to the ZFS partitions.
        size_t i;
        for (i = 0; i < Size(); i++)
        {
            SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partit = GetInstance(i);
            if (partit->m_isZfsPartition == true)
            {
                partit->m_index = zfsFirstIndex;
                zfsFirstIndex++;
            }
        }

#elif defined(aix)

        // We are mapping AIX logical volumes to the Windows concept of partitions, because AIX
        // has no direct equivalent to partitions.

        // Get mounted file systems first. We will need them as we go through all logical volumes.
        int ret = -1;
        std::vector<char> vm;
        // Get the size of the required buffer.
        int vmsz = 0;
        ret = m_deps->mntctl(MCTL_QUERY, sizeof(vmsz), (char*)&vmsz);
        if(ret != 0)
        {
            throw SCXErrnoException(
                L"mntctl failed trying to get required buffer size", errno, SCXSRCLOCATION);
        }
        vm.resize(vmsz);
        // Fill the buffer with mount points data.
        int mount_point_cnt = m_deps->mntctl(MCTL_QUERY, vm.size(), &vm[0]);
        if(mount_point_cnt == -1)
        {
            throw SCXErrnoException(L"mntctl failed trying to get mount points", errno, SCXSRCLOCATION);
        }

        // Now get all of the partitions.

        unsigned int partition_index = 0;
        
        // Get list of all volume groups.
        struct queryvgs *vgs = NULL;
        int ret_lvmvgs = m_deps->lvm_queryvgs(&vgs, NULL);
        if (ret_lvmvgs != 0)
        {
            throw SCXInternalErrorException(L"lvm_queryvgs failed with error code: " + StrFrom(ret_lvmvgs) +
                                            L".", SCXSRCLOCATION);
        }
        // Iterate through volume groups.
        for (long ivg = 0; ivg < vgs->num_vgs; ivg++)
        {
            // For each volume group get list of logical volumes.
            struct queryvg *vg = NULL;
            int ret_lvmvg = m_deps->lvm_queryvg(&vgs->vgs[ivg].vg_id, &vg, NULL);
            if (ret_lvmvg != 0)
            {
                throw SCXInternalErrorException(L"lvm_queryvg failed with error code: " + StrFrom(ret_lvmvg) +
                                                L".", SCXSRCLOCATION);
            }
            // Iterate through logical volumes.
            for (long ilv = 0; ilv < vg->num_lvs; ilv++)
            {
                // Get logical volume data. We use try catch so we don't crash just because
                // there was error getting data for one volume.
                try
                {
                    struct querylv *lv = NULL;
                    int ret_lvmlv = m_deps->lvm_querylv(&vg->lvs[ilv].lv_id, &lv, NULL);
                    if (ret_lvmlv != 0)
                    {
                        throw SCXInternalErrorException(L"lvm_querylv for logical volume \"" +
                                                        StrFromUTF8(vg->lvs[ilv].lvname) + L"\" failed with error code: " +
                                                        StrFrom(ret_lvmlv) + L".", SCXSRCLOCATION);
                    }
                    ProcessOneDiskPartition(vm, mount_point_cnt, lv->lvname, 
                                            static_cast<scxlong>(lv->currentsize) << lv->ppsize, partition_index);
                }
                catch(SCXCoreLib::SCXException& e)
                {
                    SCX_LOGERROR(m_log, e.What() + L" " + e.Where());
                }
            }
        }        
#elif defined(hpux)
        vector<SCXLogicalVolumes> logVol;
        bool bootLvFound = false;
        size_t fullBootLvIndex = 0;
        GetLogicalVolumesBoot(logVol, bootLvFound, fullBootLvIndex);
        size_t i;
        for(i = 0; i<logVol.size(); i++)
        {
            SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partition_instance = GetInstance(logVol[i].name);
            if (NULL == partition_instance)
            {
                partition_instance = new StaticDiskPartitionInstance();
                partition_instance->SetId(logVol[i].name);
                partition_instance->m_deviceID = logVol[i].name;
                partition_instance->m_index = i;

                if(logVol[i].mntDir.size() != 0)
                {
                    // We have a mount point for this logical volume. Get info about the file system.
                    struct statvfs64 stat;
                    memset(&stat, 0, sizeof(stat));
                    int r = m_deps->statvfs(StrToUTF8(logVol[i].mntDir).c_str(), &stat);
                    if(r != 0)
                    {
                        wstring msg = L"statvfs() failed for mountpoint \"" + logVol[i].mntDir + L"\".";
                        SCX_LOGERROR(m_log, msg);
                    }
                    // Now we have all file system data. Update the disk partition instance.
                    scxulong partition_size =
                    static_cast<scxulong>(stat.f_frsize) * static_cast<scxulong>(stat.f_blocks);

                    partition_instance->m_blockSize = stat.f_frsize;
                    partition_instance->m_numberOfBlocks = stat.f_blocks;
                    partition_instance->m_partitionSize = partition_size;
                }
                else
                {
                    // No mount point. Get the size by other means. We use try catch so we don't crash just because
                    // there was error getting one datafield for one volume.
                    try
                    {
                        // Before calling ioctl verify that name matches logical volume name. We expect name in format:
                        // "/def/some_vg_name/some_lv_name"
                        SCXRegex regEx(L"(^/dev/[^/]+/[^/]+$)");
                        vector<std::wstring> matches;
                        if (!regEx.ReturnMatch(logVol[i].name, matches, 0))
                        {
                            if (!matches[0].empty())
                            {
                                wstring msg = L"Error encountered when trying to verify device name \"" + logVol[i].name +
                                L"\". " + matches[0];
                                throw SCXInternalErrorException(msg, SCXSRCLOCATION);
                            }
                            else
                            {
                                wstring msg = L"Device name \"" + logVol[i].name + L"\" is invalid.";
                                throw SCXInternalErrorException(msg, SCXSRCLOCATION);
                            }
                        }
                        // Name matches, make raw device name.
                        wstring rname = logVol[i].name;
                        size_t namePos = rname.rfind(L'/');
                        if (namePos == wstring::npos)
                        {
                            wstring msg = L"Device name \"" + rname + L"\" missing full path.";
                            throw SCXErrnoException(msg, errno, SCXSRCLOCATION);
                        }
                        rname.insert(namePos + 1, L"r");
                        // Open the device file, call ioctl(DIOC_CAPACITY), set the size value and close the device file.
                        int fd = m_deps->_open(StrToUTF8(rname).c_str(), O_RDONLY);
                        if (fd == -1)
                        {
                            wstring msg = L"open(O_RDONLY) failed for device \"" + rname + L"\".";
                            throw SCXErrnoException(msg, errno, SCXSRCLOCATION);
                        }
                        capacity_type ct;
                        memset(&ct, 0, sizeof(ct));
                        int ret = m_deps->_ioctl(fd, DIOC_CAPACITY, &ct);
                        if (ret == -1)
                        {
                            wstring msg = L"ioctl(DIOC_CAPACITY) failed for device \"" + rname + L"\".";
                            throw SCXErrnoException(msg, errno, SCXSRCLOCATION);
                        }
                        partition_instance->m_partitionSize = ct.lba * DEV_BSIZE;
                        if (m_deps->_close(fd) != 0)
                        {
                            wstring msg = L"close() failed for device \"" + rname + L"\".";
                            throw SCXErrnoException(msg, errno, SCXSRCLOCATION);
                        }
                    }
                    catch(SCXCoreLib::SCXException& e)
                    {
                        SCX_LOGERROR(m_log, e.What() + L" " + e.Where());
                    }
                }
                if(bootLvFound && fullBootLvIndex == i)
                {
                    partition_instance->m_bootPartition = true;
                }
                AddInstance(partition_instance);
            }
        }
#endif
    }

#if defined(aix)
    /*----------------------------------------------------------------------------*/
    /**
       Processes one disk partition during enumeration.

       \param mountPoints Buffer containing mount points.
       \param mountPointCnt Number of mountpoints in the buffer.
       \param partitionName Partition name.
       \param partitionSize Partition size.
       \param partitionIndex Partition index.
    */
    void StaticDiskPartitionEnumeration::ProcessOneDiskPartition(const std::vector<char> &mountPoints, int mountPointCnt,
                                                                 const char* partitionName, scxlong partitionSize, unsigned int &partitionIndex)
    {
        SCXCoreLib::SCXHandle<SCXodm> odm_deps = m_deps->CreateOdm();
        std::string partition_name = partitionName;

        struct CuAt ret_data_at;
        void *ret_at;
        std::string criteria_at = "name=";
        criteria_at += partition_name;
        // Find disk partition instance or create one.
        SCXCoreLib::SCXHandle<StaticDiskPartitionInstance> partition_instance =
        GetInstance(StrFromUTF8(partition_name));
        if (NULL == partition_instance)
        {
            partition_instance = new StaticDiskPartitionInstance();
            partition_instance->SetId(StrFromUTF8(partition_name));

            std::string type;
            // Get first attribute of the partition.
            memset(&ret_data_at, 0, sizeof(ret_data_at));
            ret_at = odm_deps->Get(CuAt_CLASS, (char*)criteria_at.c_str(), &ret_data_at, SCXodm::eGetFirst);
            while(ret_at != NULL)
            {
                std::string attribute = ret_data_at.attribute;
                if(attribute == "type")
                {
                    type = ret_data_at.value;
                }

                // Get next attribute of the partition.
                memset(&ret_data_at, 0, sizeof(ret_data_at));
                ret_at = odm_deps->Get(CuAt_CLASS, NULL, &ret_data_at, SCXodm::eGetNext);
            }

            // Find the mount point for this partition.
            const struct vmount *vmp = reinterpret_cast<const struct vmount *>(&mountPoints[0]);
            int i;
            for(i = 0; i < mountPointCnt; i++)
            {
                if(vmp->vmt_flags & MNT_DEVICE)
                {
                    // Consider only physical device mounts.
                    std::string full_device_name = "/dev/" + partition_name;
                    if(full_device_name == reinterpret_cast<const char*>(vmp) + vmp->vmt_data[VMT_OBJECT].vmt_off)
                    {
                        // We have found our mount point. Get info about the file system.
                        struct statvfs64 stat;
                        memset(&stat, 0, sizeof(stat));
                        int r = m_deps->statvfs64(reinterpret_cast<const char*>(vmp) + vmp->vmt_data[VMT_STUB].vmt_off,
                                                  &stat);
                        if(r != 0)
                        {
                            throw SCXErrnoException(L"statvfs failed", errno, SCXSRCLOCATION);
                        }

                        // Now we have all file system data. Update the disk partition instance.
                        scxulong partition_size =
                        static_cast<scxulong>(stat.f_frsize) * static_cast<scxulong>(stat.f_blocks);

                        partition_instance->m_blockSize = stat.f_frsize;
                        partition_instance->m_numberOfBlocks = stat.f_blocks;
                        partition_instance->m_partitionSize = partition_size;
                        break;
                    }
                }

                // Get next mount point.
                vmp = reinterpret_cast<const struct vmount *>(reinterpret_cast<const char*>(vmp) + vmp->vmt_length);
            }

            partition_instance->m_deviceID = StrFromUTF8(partition_name);
            partition_instance->m_index = partitionIndex;
            if(partition_instance->m_partitionSize == 0)
            {
                // Didn't get partition size from mount point. Use size from lvm.
                partition_instance->m_partitionSize = partitionSize;
            }
            if(type == "boot")
            {
                partition_instance->m_bootPartition = true;
            }

            AddInstance(partition_instance);
        }
        partitionIndex++;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).
    
       Parameters:  None
       Retval:      The object represented as a string suitable for logging.
    
    */
    const std::wstring StaticDiskPartitionEnumeration::DumpString() const
    {
        return L"StaticDiskPartitionEnumeration";
    }

#if defined(hpux)

    void StaticDiskPartitionEnumeration::GetBootLV(
        const vector<SCXLogicalVolumes> &logVol, const vector<std::wstring> &logVolShort,
        const vector<size_t> &logVolVGIndex,
        const vector<std::wstring> &physVol, const vector<size_t> &physVolVGIndex,
        size_t &fullBootLvIndex)
    {
        // Execute 'vgdisplay -v' to get list of logical volumes.
        std::istringstream lvProcIn;
        std::ostringstream lvProcOut;
        std::ostringstream lvProcErr;
        std::wstring procCmd = L"/sbin/lvlnboot -v";
        int procRet = m_deps->Run(procCmd, lvProcIn, lvProcOut, lvProcErr, 15000);
        std::string lvlnbootStr = lvProcOut.str();
        std::string errStr = lvProcErr.str();
        if(procRet == 0 && errStr.size() == 0)
        {
            // Read lines returned by 'lvlnboot -v'. We are looking for the lines in format:
            // [wspace | nothing]["Boot:" | "Root:"][wspace][short lv name][wspace]["on:"][wspace][pv name][endl]
            // For example:
            // Boot: lvol1   on:   /dev/dsk/c0t0d0s2
            std::istringstream lvlnbootStrm(lvlnbootStr);
            SCXCoreLib::SCXStream::NLFs nlfs;
            vector<std::wstring> lvlnbootLines;
            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(lvlnbootStrm, lvlnbootLines, nlfs);

            // Prepare SCXRegex regular expression to extract LG, LV or PV name from the lines.
            // This regular expression can match a boot or root line, for example "Boot: l vol 1  on: /dev/dsk/c0t0d0s2".
            // In the case of the match logical volume name is stripped of leading and trailing spaces and rule that the
            // name can not contain any '/' characters is enforced. Also rule is enforced that physical volume name must
            // have "/dev/" prefix and additional '/' character somewhere in the middle of the remainder of the name.
            // When match is made expression will load "Boot:" or "Root:" in the return value number 1. Logical volume
            // name, in the short form and stripped of leading and trailing spaces, will be loaded in the in the return
            // value number 2 and physical volume name will be loaded in the return value number 3.
            SCXRegex regEx(L"^[ \t]*(Boot:|Root:)[ \t]+([^/ \t]|[^/ \t][^/]+[^/ \t])[ \t]+on:[ \t]+(/dev/[^/]+/[^/]+)$");

            size_t i;
            std::wstring bootLvName;
            std::wstring bootPvName;
            bool bootOrRootFound = false;
            for(i = 0; i < lvlnbootLines.size(); i++)
            {
                std::wstring bootOrRoot;
                std::wstring lvN;
                std::wstring pvN;
                vector<std::wstring> regExMatches;
                if(regEx.ReturnMatch(lvlnbootLines[i], regExMatches, 0))
                {
                    // Boot or root volume is found.
                    bootOrRoot = regExMatches[1];
                    bootLvName = regExMatches[2];
                    bootPvName = regExMatches[3];
                    bootOrRootFound = true;
                    if(bootOrRoot == L"Boot:")
                    {
                        break;
                    }
                    // If no boot is found then root volume is also boot volume.
                }
            }
            if(bootOrRootFound == false)
            {
                throw SCXCoreLib::SCXInternalErrorException(
                    std::wstring(L"Output from 'lvlnboot -v' does not contain any boot or root data."), SCXSRCLOCATION);
            }

            // We have boot LV name but it is stripped of leading and trailing white spaces and is missing path
            // that contains VG name to which LV belongs to. We must recover the full logical volume name.
            // First match the boot PV with the one in the list of all PV's available on the system.
            vector<std::wstring>::const_iterator it = find(physVol.begin(), physVol.end(), bootPvName);
            if(it == physVol.end())
            {
                // PV name from "lvlnboot -v" not found in "vgdisplay -v".
                std::wstring invalidPvName = L"PV Name '" + bootPvName +
                L"' from 'lvlnboot -v' output not found in 'vgdisplay -v' output.";
                throw SCXCoreLib::SCXInternalErrorException(invalidPvName, SCXSRCLOCATION);
            }
            // Get the index of the matched PV.
            size_t physVolIndex = distance(physVol.begin(), it);
            // Get the index of the VG the PV belongs to.
            size_t bootVGIndex = physVolVGIndex[physVolIndex];
            // Search list of logical volumes with same Volume Group index and find one that matches boot LV name.
            size_t lvFoundCount = 0;
            for(i = 0; i < logVolVGIndex.size(); i++)
            {
                if(logVolVGIndex[i] == bootVGIndex)
                {
                    if(logVolShort[i] == bootLvName)
                    {
                        fullBootLvIndex = i;
                        lvFoundCount++;
                    }
                }
            }
            if(lvFoundCount == 0)
            {
                // LV name from "lvlnboot -v" not found in "vgdisplay -v".
                std::wstring invalidLvName = L"Boot LV Name '" + bootLvName +
                L"' from 'lvlnboot -v' output not found in 'vgdisplay -v' output.";
                throw SCXCoreLib::SCXInternalErrorException(invalidLvName, SCXSRCLOCATION);
            }
            if(lvFoundCount > 1)
            {
                std::wstring invalidLvName = L"Boot LV Name '" + bootLvName +
                L"' from 'lvlnboot -v' output found multiple times in 'vgdisplay -v' output. Names differ "
                L"only in leading and trailing spaces. impossible to determine actual boot logical volume.";
                throw SCXCoreLib::SCXInternalErrorException(invalidLvName, SCXSRCLOCATION);
            }

        }
        else
        {
            std::wstring procExcMsg = L"Execution of '" + procCmd + L"' failed with return code " +
            StrFrom(procRet) + L".\nOutput:\n" + StrFromUTF8(lvlnbootStr) + L"\nError output:\n" +
            StrFromUTF8(errStr) + L"\n";
            throw SCXCoreLib::SCXInternalErrorException(procExcMsg, SCXSRCLOCATION);
        }
    }

    /**
       Retrieves vector of mount points. This helper function is necessary to put HP-s fix that ensures data integrity
       around setmntent(), getmntent(), endmntent() sequence. Fix description and sample code is from
       HP-UX reference on getmntent(3X). To achieve data integrity we compare MNT_MNTTAB file size and modification time
       before and after data reads and use data only if there was no modification.

       \param log Handle to a log file.
       \param deps External dependencies.
       \param mountPoints Return vector to be filled with mount points.
    */
    void GetMountPoints(SCXCoreLib::SCXLogHandle &log, SCXCoreLib::SCXHandle<DiskDepend> &deps,
                        vector<SCXLogicalVolumes> &mountPoints)
    {
        static SCXCoreLib::LogSuppressor supressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        mountPoints.clear();

        // HP-s fix for flaw in setmntent(). To achieve data integrity we compare file modification time before and
        // after data reads. Data integrity code is from HP-UX reference on getmntent(3X).
        struct stat statbuf;
        FILE *fp;
        time_t origMtime;
        off_t origSize;
        bool readStatus = false;
        const int noOfRetries = 5;
        int retry;
        for (retry = 0; retry < noOfRetries; retry++)
        {
            if (deps->stat(MNT_MNTTAB, &statbuf) != 0)
            {
                throw SCXErrnoException(L"stat(MNT_MNTTAB, &statbuf)" , errno, SCXSRCLOCATION);
            }
            // If file is empty, do not bother reading it. Sleep and then retry the stat().
            if (statbuf.st_size == 0)
            {
                std::wstring emptyMntTabFileWarning(L"File MNT_MNTTAB is empty.");
                SCXCoreLib::SCXLogSeverity severity(supressor.GetSeverity(emptyMntTabFileWarning));
                SCX_LOG(log, severity, emptyMntTabFileWarning);
                SCXCoreLib::SCXThread::Sleep(100);
                if (deps->stat(MNT_MNTTAB, &statbuf) != 0)
                {
                    throw SCXErrnoException(L"stat(MNT_MNTTAB, &statbuf)" , errno, SCXSRCLOCATION);
                }
                else
                {
                    if (statbuf.st_size == 0)
                    {
                        continue; 
                    } 
                }
            }
            if ((fp = deps->setmntent(MNT_MNTTAB, "r")) == NULL)
            {
                throw SCXErrnoException(L"setmntent(MNT_MNTTAB, \"r\")" , errno, SCXSRCLOCATION);
            }

            // Get mount points.
            struct mntent *mountpoint = deps->getmntent(fp);
            while(mountpoint != NULL)
            {
                SCXLogicalVolumes currentMountPoint;
                currentMountPoint.name = SCXCoreLib::StrFromUTF8(mountpoint->mnt_fsname);
                currentMountPoint.mntDir = SCXCoreLib::StrFromUTF8(mountpoint->mnt_dir);
                currentMountPoint.mntType = SCXCoreLib::StrFromUTF8(mountpoint->mnt_type);
                currentMountPoint.mntOpts = SCXCoreLib::StrFromUTF8(mountpoint->mnt_opts);
                mountPoints.push_back(currentMountPoint);
                // Next mount point.
                mountpoint = deps->getmntent(fp);
            }
            deps->endmntent(fp);

            // Compare file time to ensure data integrity.
            origMtime = statbuf.st_mtime; origSize = statbuf.st_size;
            if (deps->stat(MNT_MNTTAB, &statbuf) != 0)
            {
                throw SCXErrnoException(L"stat(MNT_MNTTAB, &statbuf)" , errno, SCXSRCLOCATION);
            }
            if ((statbuf.st_mtime == origMtime) && (statbuf.st_size == origSize))
            {
                readStatus = true;
                break;
            }
        }

        // Delete data if data integrity is in question.
        if (readStatus == false)
        {
            mountPoints.clear();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Executes "vgdisplay -v" command and returns lists of volume groups, logical and physical volumes found on
       the system.

       \param log Logging handle.
       \param deps Dependencies handle.
       \param volGroup Returned list of volume groups found on the system.
       \param logVol Returned list of logical volumes found on the system.
       \param logVolShort Returned list of logical volume names in the short form, without path and with leading
       and trailing spaces removed.
       \param logVolVGIndex Index of a volume group to which particular logical volume belongs to.
       \param physVol Returned list of physical volumes found on the system.
       \param physVolVGIndex Index of a volume group to which particular physical volume belongs to.
    */
    void GetVgLvPv(SCXCoreLib::SCXLogHandle &log, 
                   SCXCoreLib::SCXHandle<DiskDepend> &deps, vector<std::wstring> &volGroup,
                   vector<SCXLogicalVolumes> &logVol, vector<std::wstring> &logVolShort, vector<size_t> &logVolVGIndex,
                   vector<std::wstring> &physVol, vector<size_t> &physVolVGIndex)
    {
        volGroup.clear();
        logVol.clear();
        logVolShort.clear();
        logVolVGIndex.clear();
        physVol.clear();
        physVolVGIndex.clear();

        // Get mount points.
        vector<SCXLogicalVolumes> mountPoints;
        GetMountPoints(log, deps, mountPoints);
        // Execute 'vgdisplay -v' to get list of logical volumes.
        std::istringstream lvProcIn;
        std::ostringstream lvProcOut;
        std::ostringstream lvProcErr;
        std::wstring procCmd = L"/sbin/vgdisplay -v";
        int procRet = deps->Run(procCmd, lvProcIn, lvProcOut, lvProcErr, 15000);
        std::string vgStr = lvProcOut.str();
        std::string errStr = lvProcErr.str();
        if(procRet == 0 && errStr.size() == 0)
        {
            // Read lines returned by 'vgdisplay -v'. We are looking for the lines in format:
            // [wspace | nothing]["VG" | "LV" | "PV"][wspace]["Name"][wspace][name][endl]
            // for example:
            //   LV Name /dev/vg00/lvol8
            std::istringstream vgStrm(vgStr);
            SCXCoreLib::SCXStream::NLFs nlfs;
            vector<std::wstring> lvLines;
            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(vgStrm, lvLines, nlfs);

            // Prepare SCXRegex regular expression to extract LG, LV or PV name from the lines.
            // This regular expression can match volume group line, logical volume line or physical volume line.
            // In the case of the volume group line, for example "VG Name /dev/vg00", it will enforce the rule
            // that name must contain "/dev/" prefix. When match is made it will load the full name "/dev/vg00"
            // into the return value number 1.
            // In the case of the logical or physical volume line, for example " LV Name /dev/vg00/ l vol 1 " it
            // will enforce the rule that name must contain "/dev/" prefix and one aditional '/' character somewhere
            // in the middle of the reminder of the name. When match is made it will load "LV" or "PV" in the return
            // value number 2, full name "/dev/vg00/ l vol 1 " into the return value number 3, and short name without
            // the path and stripped from leading and trailing spaces "l vol 1" into the return value number 4.
            SCXRegex regEx(L"^[ \t]*VG[ \t]+Name[ \t]+(/dev/[^/]+)$|"
                           L"^[ \t]*(LV|PV)[ \t]+Name[ \t]+(/dev/[^/]+/[ \t]*([^/ \t]|[^/ \t][^/]+[^/ \t])[ \t]*)$");

            size_t i;
            for(i = 0; i < lvLines.size(); i++)
            {
                vector<SCXRegExMatch> regExMatches;
                if(regEx.ReturnMatch(lvLines[i], regExMatches, 5, 0) == true)
                {
                    if(regExMatches[1].matchFound ==true)
                    {
                        // Volume group match.
                        volGroup.push_back(regExMatches[1].matchString);
                    }
                    else
                    {
                        // Logical volume or physical volume is matched.

                        // First check if there is volume group for this logical volume.
                        if(volGroup.size() == 0)
                        {
                            throw SCXCoreLib::SCXInternalErrorException(
                                std::wstring(L"vgdisplay -v returned corrupt data."
                                             L"LV or PV name encountered before it's VG name."),
                                SCXSRCLOCATION);
                        }

                        std::wstring nameType = regExMatches[2].matchString;
                        std::wstring name = regExMatches[3].matchString;
                        if(nameType == L"LV")
                        {
                            // Logical volume name is found.
                            SCXLogicalVolumes currentLogVol;
                            currentLogVol.name = name;

                            // Find mount point for this logical volume, if any.
                            size_t m;
                            for(m = 0; m < mountPoints.size(); m++)
                            {
                                if(mountPoints[m].name == currentLogVol.name)
                                {
                                    currentLogVol = mountPoints[m];
                                    break;
                                }
                            }
                            // Add to the result logical volume and it's mount point if any.
                            logVol.push_back(currentLogVol);
                            logVolShort.push_back(regExMatches[4].matchString);
                            logVolVGIndex.push_back(volGroup.size() - 1);
                        }
                        else
                        {
                            //nameType == "PV"
                            physVol.push_back(name);
                            physVolVGIndex.push_back(volGroup.size() - 1);
                        }
                    }
                }
            }
        }
        else
        {
            std::wstring procExcMsg = L"Execution of '" + procCmd + L"' failed with return code " +
            StrFrom(procRet) + L".\nOutput:\n" + StrFromUTF8(vgStr) + L"\nError output:\n" +
            StrFromUTF8(errStr) + L"\n";
            throw SCXCoreLib::SCXInternalErrorException(procExcMsg, SCXSRCLOCATION);
        }
    }

    void GetLogicalVolumes(SCXCoreLib::SCXLogHandle &log, 
                           SCXCoreLib::SCXHandle<DiskDepend> &deps, vector<SCXLogicalVolumes> &logVol)
    {
        vector<std::wstring> volGroup;
        vector<std::wstring> logVolShort;
        vector<size_t> logVolVGIndex;
        vector<std::wstring> physVol;
        vector<size_t> physVolVGIndex;
        GetVgLvPv(log, deps, volGroup, logVol, logVolShort, logVolVGIndex, physVol, physVolVGIndex);
    }

    void StaticDiskPartitionEnumeration::GetLogicalVolumesBoot(vector<SCXLogicalVolumes> &logVol,
                                                               bool &bootLvFound, size_t &fullBootLvIndex)
    {
        bootLvFound = false;
        fullBootLvIndex = 0;

        // Retrieve lists of volume groups, logical and physical volumes present in the system.
        vector<std::wstring> volGroup;
        vector<std::wstring> logVolShort;
        vector<size_t> logVolVGIndex;
        vector<std::wstring> physVol;
        vector<size_t> physVolVGIndex;
        GetVgLvPv(m_log, m_deps, volGroup, logVol, logVolShort, logVolVGIndex, physVol, physVolVGIndex);

        try
        {
            // Try to find whish is the boot volume.
            GetBootLV(logVol, logVolShort, logVolVGIndex, physVol, physVolVGIndex, fullBootLvIndex);
            bootLvFound = true;
        }
        catch(SCXCoreLib::SCXException& e)
        {
            // Catch all boot flag processing exceptions here. We don't want to lose all other
            // data just because boot flag processing failed.
            SCX_LOGERROR(m_log, "Failed to find boot logical volume.");
            SCX_LOGERROR(m_log, e.What());
        }
    }
#endif// hpux

#if defined(linux)

    bool StaticDiskPartitionEnumeration::GetPartedPath(std::wstring &parted_path)
    {
        std::vector<wstring> possible_paths;
        possible_paths.push_back(L"/sbin/parted");
        possible_paths.push_back(L"/usr/sbin/parted");

        for (std::vector<wstring>::const_iterator it = possible_paths.begin(); it != possible_paths.end(); ++it)
        {
            if (SCXFile::Exists(*it))
            {
                parted_path = *it;
                return true;
            }
        }
        return false;
    }

    bool StaticDiskPartitionEnumeration::GetPartedOutput(std::string &partedOutput)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
        SCX_LOGTRACE(m_log, L"DiskPartitionEnum::GetPartedOutput() Entering");

        std::istringstream processInput;
        std::ostringstream processOutput, processErr;
        bool success = false;
        std::wstring parted_path;
        
        if (!StaticDiskPartitionEnumeration::GetPartedPath(parted_path))
        {
            SCX_LOG(m_log, suppressor.GetSeverity(L"NoPartedFound"), L"Could not find parted in /sbin or /usr/sbin");
            return false;
        }
        
        try
        {
            std::wstring command = parted_path + L" -ls";
            SCX_LOGTRACE(m_log, L"Invoking command : \"" + command + L"\"");
            int ret = m_deps->Run(command, processInput, processOutput, processErr, 15000);

            std::string errOut = processErr.str();
            if (errOut.size())
            {
                SCX_LOGWARNING(m_log, StrAppend(L"Got this error string from parted command: ", StrFromUTF8(errOut)));
            }

            partedOutput = processOutput.str();
            SCX_LOGTRACE(m_log, StrAppend(L"  Got this output: ", StrFromUTF8(partedOutput)));
            
            success = (ret == 0 && partedOutput.size() > 0);
        }
        catch (SCXCoreLib::SCXInternalErrorException &e) 
        {
            std::wostringstream error;
            error << L"Attempt to execute parted command for the purpose of retrieving partition information failed : " << e.What();
            SCX_LOG(m_log, suppressor.GetSeverity(L"InternalError"), error.str());
        }

        if (!success)
        {
            try
            {
                std::wstring command = parted_path + L" -i";
                SCX_LOGTRACE(m_log, L"Using fallback interactive parted command : \"" + command + L"\"");

                processErr.str(""); // Reset the error stream

                // We use ignore in case parted shows an interactive warning
                std::istringstream input("ignore\nignore\nprint\nquit\n");

                // We need the -i flag for stdin to be used correctly
                int ret = m_deps->Run(command, input, processOutput, processErr, 15000);

                std::string errOut = processErr.str();
                partedOutput = processOutput.str();
                SCX_LOGTRACE(m_log, StrAppend(L"  Got this output: ", StrFromUTF8(partedOutput)));

                if (errOut.size())
                {
                    SCX_LOGWARNING(m_log, StrAppend(L"Got this error string from parted command: ", StrFromUTF8(errOut)));
                }
                success = (ret == 0 && errOut.empty());
            }
            catch (SCXCoreLib::SCXInternalErrorException &e) 
            {
                std::wostringstream error;
                error << L"Attempt to execute parted command for the purpose of retrieving partition information failed : " << e.What();
                SCX_LOG(m_log, suppressor.GetSeverity(L"InternalError"), error.str());
            }
            catch (SCXCoreLib::SCXInterruptedProcessException &e)
            {
                std::wostringstream error;
                error << L"The parted process was interrupted while retrieving partition information : " << e.What();
                SCX_LOG(m_log, suppressor.GetSeverity(L"Interrupted"), error.str());
            }
        }

        if (partedOutput.empty())
        {
            SCX_LOG(m_log, suppressor.GetSeverity(L"EmptyOutput"), L"Unable to retrieve partition information from OS...");
            return false;
        }

        return success;
    }

    bool StaticDiskPartitionEnumeration::ParsePartedOutput(const std::string &partedOutput, std::map<std::wstring, std::wstring> &partitions)
    {
        SCX_LOGTRACE(m_log, L"DiskPartitionEnum::ParsePartedOutput() Entering");

        typedef SCXCoreLib::SCXHandle<SCXRegex> SCXRegexPtr;

        SCXRegexPtr diskRegExPtr(NULL);
        SCXRegexPtr detailRegExPtr(NULL);
        
        // Gets the path of the disk. For example:
        // "/dev/sda" in "Disk /dev/sda: 112GB"
        wstring partedDiskPattern(L"Disk[^/]+(/dev/[^ ]*):");

        // Get the partition number on detail lines. For example:
        // "1" in " 1      1049kB  525MB  524MB  primary  ext4         boot"
        // "2" in "2        101.975 102398.686  primary               lvm"
        wstring partedDetailPattern(L"^[ ]?([0-9]+)");

        std::vector<wstring> matchingVector;

        //Let's build our RegEx:
        try
        {
            diskRegExPtr = new SCXCoreLib::SCXRegex(partedDiskPattern);
            detailRegExPtr = new SCXCoreLib::SCXRegex(partedDetailPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return false;
        }

        // All lines read from parted output
        vector<wstring>  allLines;
        allLines.clear();

        std::istringstream partedStringStrm(partedOutput);
        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(partedStringStrm, allLines, nlfs);
        wstring currentDisk(L"");

        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); it++)
        {
            wstring curLine(*it);
            SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"DiskPartitionEnum::ParseParted() Top of FOR: We have a line= ", (*it)));
            matchingVector.clear();

            if (diskRegExPtr->ReturnMatch(curLine, matchingVector, 0))
            {
                currentDisk = matchingVector[1];
            }
            else if (detailRegExPtr->ReturnMatch(curLine, matchingVector, 0))
            {
                // We found a partition, save it.
                wstring deviceID = currentDisk + matchingVector[1]; 
                partitions[deviceID] = curLine;
            }
        }

        if (m_log.GetSeverityThreshold() <= SCXCoreLib::eHysterical )
        {
            SCX_LOGHYSTERICAL(m_log, L"Parted output parsing result");
            for (std::map<std::wstring, std::wstring>::const_iterator it = partitions.begin(); it != partitions.end(); ++it)
            {
                SCX_LOGHYSTERICAL(m_log, it->first + L" : " + it->second);
            }
        }

        return true;
    }
#endif
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/

