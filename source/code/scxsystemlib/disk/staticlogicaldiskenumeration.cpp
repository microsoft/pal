/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implements the logical disk enumeration pal for static information.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/staticlogicaldiskenumeration.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Constructor.

    \param       deps A StaticDiscDepend object which can be used.

*/
    StaticLogicalDiskEnumeration::StaticLogicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps) : m_deps(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticlogicaldiskenumeration");
        m_deps = deps;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Virtual destructor.
    */
    StaticLogicalDiskEnumeration::~StaticLogicalDiskEnumeration()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Init method.

       Initial caching of data is performed here.

    */
    void StaticLogicalDiskEnumeration::Init()
    {
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Enumeration Cleanup method.

       Release of cached resources.

    */
    void StaticLogicalDiskEnumeration::CleanUp()
    {
        EntityEnumeration<StaticLogicalDiskInstance>::CleanUp();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Update the enumeration.

       \param updateInstances If true (default) all instances will be updated.
                              Otherwise only the content of teh enumeration will be updated.

    */
    void StaticLogicalDiskEnumeration::Update(bool updateInstances/*=true*/)
    {
        SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Size of enumeration: ", this->Size()));
        for (EntityIterator iter=Begin(); iter!=End(); ++iter)
        {
            SCXCoreLib::SCXHandle<StaticLogicalDiskInstance> disk = *iter;
            SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Device being set to OFFLINE, disk: ", disk->m_mountPoint));
            disk->m_online = false;
        }

        m_deps->RefreshMNTTab();
        for (std::vector<MntTabEntry>::const_iterator it = m_deps->GetMNTTab().begin();
             it != m_deps->GetMNTTab().end(); ++it)
        {
            if ( ! m_deps->FileSystemIgnored(it->fileSystem) && ! m_deps->DeviceIgnored(it->device))
            {
                SCXCoreLib::SCXHandle<StaticLogicalDiskInstance> disk = GetInstance(it->mountPoint);
                if (0 == disk)
                {
                    disk = new StaticLogicalDiskInstance(m_deps);
                    disk->m_device = it->device;
                    disk->m_mountPoint = it->mountPoint;
                    disk->SetId(disk->m_mountPoint);
                    disk->m_fileSystemType = it->fileSystem;
                    disk->m_diskRemovability = GetDiskRemovability(it->device);
                    AddInstance(disk);
                }
                SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"Device being set to ONLINE, disk: ", disk->m_mountPoint));
                disk->m_online = true;
            }
        }

        if (updateInstances)
        {
            UpdateInstances();
        }
    }


     /*----------------------------------------------------------------------------*/
     /**
        \copydoc SCXSystemLib::DiskDepend::RefreshDevTab

        \note Not thread safe.
     */
     void StaticLogicalDiskEnumeration::RefreshDevTab()
     {
          // This function is visible to other builds (but empty) to avoid
          // #ifdef'ing the enumerator

          // The /etc/device.tab file exists only on Solaris before 5.11
          // On all other distros this is a null function
#if defined(sun)  &&  (PF_MAJOR <= 5 && PF_MINOR < 11)
          m_devTab.clear();

          SCX_LOGTRACE(m_log, L"device.tab file being read");
          SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(
                                                       m_deps->LocateDevTab(), std::ios::in));
          fs.SetOwner();
          while (fs->is_open() && !fs->eof())
          {
               std::wstring line;

               getline(*fs, line);

               // File looks like this:
               // # Comment line
               // # Data line follows
               // alias:cdevice:bdevice:pathname:attrs
               // # ---end file-----
               //
               // Basically each line is *exactly* five (possibly empty) colon separated fields,
               // if it is not a comment, which starts with '#'.

               if(line.find(L'#') == 0)
                    continue;

               if(line.length() == 0)
                    continue;

               size_t lead = std::wstring::npos, trail = 0;

               // Now break down the line into pieces.
               std::vector<std::wstring> parts;

               // Break at colons and newlines, trim tokens, return empty fields, don't keep delimiters.
               SCXCoreLib::StrTokenize(line, parts, L":\n", true, true, false);

               // If we did not get exactly 5 fields, reject invalid line
               if(parts.size() != 5)
                    continue;

               // Discard devices with blank names (they do occur but not in disks,
               // e.g. print spoolers and disk *partitions* can have empty names)
               if(parts[2].length() == 0)
                    continue;

               SCX_LOGTRACE(m_log, L"device.tab line parsed:" + line);
               DevTabEntry entry;
               entry.alias = parts[0];
               entry.cdevice = parts[1];
               entry.bdevice = parts[2];
               entry.pathName = parts[3];
               entry.attrs = parts[4];
               // make block device name the key ... we are interested in disks after all ...
               m_devTab.insert(std::make_pair<std::wstring, DevTabEntry>(entry.bdevice, entry));
          }

          fs->close();
#endif
     }

#if defined(sun)
     /*----------------------------------------------------------------------------*/
     /**
        \copydoc SCXSystemLib::DiskDepend::GetDevTab

        \note Not thread safe.
     */
     const std::map<std::wstring, DevTabEntry>& StaticLogicalDiskEnumeration::GetDevTab()
     {
          return m_devTab;
     }
#endif

     int StaticLogicalDiskEnumeration::GetDiskRemovability(const std::wstring& name)
     {
          SCX_LOGTRACE(m_log, L"GetDiskRemovability(), name is: " + name);

#if defined(sun) && ((PF_MAJOR == 5 && PF_MINOR >= 11) || (PF_MAJOR >= 6))
          std::wstring sDev = L"/dev";
          std::wstring sRemovable = L"/removable-media/dsk/";

          static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);

          if(!m_deps->FileExists(SCXCoreLib::SCXFilePath(sDev + sRemovable)))
            {
                 std::wstringstream out;
                 out << L"Directory '" << sDev  <<  sRemovable << L"' does not exist";
                 SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
                 return eDiskCapUnknown;
            }

          SCXCoreLib::SCXFilePath name_path(name);
          if(name.find(sDev) != 0)
          {
               SCX_LOGTRACE(m_log, L"GetDiskRemovability(), invalid name: " + name);
               return eDiskCapUnknown;
          }

          std::wstring sNameOnly = name_path.GetFilename();
          if(sNameOnly.length() == 0)
          {
               SCX_LOGTRACE(m_log, L"GetDiskRemovability(), invalid file name: " + sNameOnly);
               return eDiskCapUnknown;
          }

          SCXCoreLib::SCXFilePath removable_path(sDev + sRemovable + sNameOnly);
          SCX_LOGTRACE(m_log, L"GetDiskRemovability() checking for removability: " + sDev + sRemovable + sNameOnly);

          if(m_deps->FileExists(removable_path))
               return  eDiskCapSupportsRemovableMedia;
          else
               return  eDiskCapOther; // i.e. not removable

#elif  defined(sun) && (PF_MAJOR <= 5 && PF_MINOR < 11)
          // Solaris before 5.11

          if(m_devTab.size() == 0)
          {
               SCX_LOGTRACE(m_log, L"GetDiskRemovability(), empty m_devTab");
               RefreshDevTab();
               if(m_devTab.size() == 0)
               {
                    SCX_LOGTRACE(m_log, L"GetDiskRemovability(), persistently empty m_devTab");
                    return eDiskCapUnknown;
               }
          }
          SCX_LOGTRACE(m_log, L"GetDiskRemovability(), nonempty m_devTab");
          std::map<std::wstring, DevTabEntry>::const_iterator i = m_devTab.find(name);
          if(i == m_devTab.end())
               return eDiskCapUnknown;

          DevTabEntry dte = (*i).second;
          std::wstring attrs = dte.attrs;

          // Now we have an attributes vector made from a string that looks like this:
          // desc="Disk Partition" type="dpart" removable="false" capacity="69079500" dparttype="fs" fstype="ufs" mountpt="/"
          // Let's fish for the 'removable'' part ...

          static std::wstring sRemovable = L"removable=";
          const wchar_t *sTrue = L"true", *sFalse = L"false";
          size_t removable_pos = attrs.find(sRemovable);
          if(removable_pos == std::wstring::npos)
               return eDiskCapUnknown;

          // Find next space char after "removable=", use max if we get  npos
          size_t space_pos = attrs.find(L' ', removable_pos + sRemovable.length());
          if(space_pos == std::wstring::npos)
               space_pos = attrs.length();

          // Advance past any space chars between '=' and attribute, e.g. `removable=      "false"`
          space_pos = attrs.find_first_not_of(L" \n\t", space_pos);
          if(space_pos == std::wstring::npos)
               space_pos = attrs.length();

          // Search between end of "removable=" (plus any whitespace) and next downstream whitespace char
          size_t quarry_pos = 0;

          // Search for 'false'
          quarry_pos = attrs.find(sFalse, removable_pos + sRemovable.length());
          if(quarry_pos != std::wstring::npos && quarry_pos < space_pos)
               return eDiskCapOther; // i.e. not removable

          // Search for 'true'
          quarry_pos = attrs.find(sTrue, removable_pos + sRemovable.length());
          if(quarry_pos != std::wstring::npos && quarry_pos < space_pos)
               return eDiskCapSupportsRemovableMedia; // i.e. removable

          // Nothing doing ..
          return eDiskCapUnknown;
#else
          return eDiskCapUnknown;
#endif
     }
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
