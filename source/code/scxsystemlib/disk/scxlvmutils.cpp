/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Provides helper methods for working with LVM and devicemapper (dm)
                devices.

   \date        2010-12-14 18:11:00
*/
/*----------------------------------------------------------------------------*/

#include <string.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <stack>

#include <scxsystemlib/scxlvmutils.h>

extern void LogToDiskLogFile(std::wstring);

namespace SCXSystemLib {
    SCXCoreLib::LogSuppressor SCXLVMUtils::m_errorSuppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
    SCXCoreLib::LogSuppressor SCXLVMUtils::m_warningSuppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
    SCXCoreLib::LogSuppressor SCXLVMUtils::m_infoSuppressor(SCXCoreLib::eInfo, SCXCoreLib::eTrace);
    const unsigned int maxLoopCount = 1000;

    /**
       Checks if the given path is in /dev/mapper.

       \param[in] the device path to check.

       \return This method will return true if the given path is a device-mapper
               path; false otherwise.

       \note Depending on the Linux distribution, a device-mapper (dm) device
             may have two paths.  On all distributions that support device-mapper,
             each dm device will have a path under /dev/mapper.  All entries
             under /dev/mapper represent a dm device partition, with the exception
             of /dev/mapper/control, which is a reserved name for the actual
             device that enables device-mapper.

             Many Linux distributions will have a second entry for each dm device.
             The second entry is named /dev/dm-<minor> where <minor> is the
             minor device id for the particular dm device.

             This method does not check the second path.  This method only checks
             if the given device path is located in /dev/mapper.
     */
    bool SCXLVMUtils::IsDMDevice(const std::wstring & device)
    {
        // All LVM devices are in the /dev/mapper directory, but the path
        // /dev/mapper/control is a reserved name.
        std::wstring devMapper = L"/dev/mapper/";
        std::wstring dmControl = L"/dev/mapper/control";

        return ((0 == device.compare(0, devMapper.length(), devMapper)) &&
                (0 != device.compare(0, dmControl.length(), dmControl)));
    }

    /**
       Get the devicemapper (dm) device that contains the given LVM device.

       \param[in] lvmDevice the path to an LVM partition.

       \return This method will return the path to the containing dm device if
               \c lvmDevice is an actual LVM device; otherwise it will return
               an empty string.

       \throws This method will re-throw any of several exceptions that can
               occur while trying to access the devices on the file system.
               Any of these exceptions indicate that the path in lvmDevice
               is not valid for *any* device.  If the path is just not an
               LVM/dm device path, no exception is thrown; the method simply
               returns an empty string.

       note The dm device and the LVM device are just two different file paths
             for the same underlying device.  If the right map is found, both
             paths will `stat' the same.
     */
    std::wstring SCXLVMUtils::GetDMDevice(const std::wstring & lvmDevice)
    {
        std::wstring result;

        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.scxlvmutils");
        std::wstringstream       out;

        SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(L"Looking for LVM device: ", lvmDevice));

        // WI 891635: Debian 8 DiskDrive reports weird file system
        //
        // Turns out that Debian 8 LVM is set up slightly differently, such that
        // the raw DM device (i.e. /dev/dm-0) is mounted directly. Just return
        // the device if that's what we got.
        if (SCXCoreLib::StrIsPrefix(lvmDevice, L"/dev/dm-"))
        {
            SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(SCXCoreLib::StrAppend(L"Device ", lvmDevice), L" is already DM device, returning it"));
            return lvmDevice;
        }

        // All LVM devices are in the /dev/mapper directory.
        if (IsDMDevice(lvmDevice))
        {
            SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(SCXCoreLib::StrAppend(L"Device: ", lvmDevice), L" IsDMDevice ..."));

            // Stat the LVM device.  Its minor ID number will indicate which
            // dm device it maps to.
            unsigned int major = 0;
            unsigned int minor = 0;

            // Any exceptions here are unexpected, so just let them move up-stack
            StatPathId(lvmDevice, major, minor);

            // On some systems, the device /dev/dm-<minor>, where <minor> is
            // minor device ID from the lvmDevice stat can be used as a quick
            // reference to the dm device name.
            std::wstringstream dmDevice;
            unsigned int       dmMajor = major;
            unsigned int       dmMinor = minor;
            bool               isMatch = false;

            dmDevice << L"/dev/dm-" << minor;

            try
            {
                isMatch = StatPathId(dmDevice.str(), dmMajor, dmMinor);
                SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(
                                           SCXCoreLib::StrAppend(
                                               SCXCoreLib::StrAppend(L"  Stat of ", dmDevice.str()),
                                               L" succeeded, isMatch: "),
                                           SCXCoreLib::StrFrom(isMatch)));
            }
            catch (SCXCoreLib::SCXFilePathNotFoundException & e)
            {
                // Unfortunately some systems don't have this handy way of
                // doing things, but there is a more convoluted way of
                // verifying that dm-<minor> is a valid dm device.
                out.str(L"");

                out << L"The device \"" << dmDevice.str() << L"\" does not exist, attempting secondary confirmation strategy";
                SCX_LOG(log, m_infoSuppressor.GetSeverity(out.str()), out.str());

                // Failed to find the device /dev/dm-<minor>.  If the file
                // /sys/block/dm-<minor>/dev contains the same major/minor
                // versions then return "dm-<minor>"
                //
                // Note: This won't be an absolute path to a real device.  It
                //       will be a string that can be found in /proc/diskstats,
                //       so it should get stats, but there may be unforseen
                //       consequences.
                dmDevice.str(L"");
                dmDevice << L"dm-" << minor;

                std::wstring dmDeviceName = dmDevice.str();

                dmDevice.str(L"");
                dmDevice << L"/sys/block/" << dmDeviceName << L"/dev";

                // Any exceptions can just move up-stack
                if (MatchIdInFile(dmDevice.str(), major, minor))
                {
                    // Again, in this case, just the name of the dm device is
                    // returned; not a valid device path.
                    return dmDeviceName;
                }
                else
                {
                    out.str(L"");

                    out << L"The device \"" << lvmDevice << L"\" does not map to \"" << dmDeviceName << L"\"";
                    SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

                    throw SCXBadLVMDeviceException(lvmDevice, out.str(), SCXSRCLOCATION);
                }
            }
            // let all other exceptions move up-stack

            if (!isMatch)
            {
                // /dev/dm-<minor>, but with non-matching device IDs
                out.str(L"");

                out << L"The the LVM device \"" << lvmDevice << L"\" and the dm device \"" << dmDevice.str() << L"\" do not have matching device IDs";
                SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

                throw SCXBadLVMDeviceException(lvmDevice, out.str(), SCXSRCLOCATION);
            }

            // done, found best match 100% confidence

            SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(L"Returning Device: ", dmDevice.str()));
            return dmDevice.str();
        }
        else
        {
            out.str(L"");

            out << L"The device \"" << lvmDevice << L"\" is not in the path \"/dev/mapper\"";
            SCX_LOGHYSTERICAL(log, out.str());
        }

        SCX_LOGHYSTERICAL(log, SCXCoreLib::StrAppend(L"Returning result: ", result));
        return result;
    }

    static inline std::wstring GetSysfsPath(const std::wstring& dmDeviceName)
    {
        // First look in Sysfs to find out what partitions the dm device
        // maps onto.  This is done by listing the slaves entries for the
        // given dm device.
        //
        // Note: The entries in slaves are links and it is the link name
        //       that is important for this, *not* the resolved link name.
        std::wstringstream sysfsPath;
        sysfsPath << L"/sys/block/" << dmDeviceName << L"/slaves/";
        return sysfsPath.str();
    }

    static inline bool PushIfDMSlave(const SCXCoreLib::SCXFilePath & slave, std::stack<std::wstring> &dmDeviceStack)
    {
       if (!slave.Get().empty() && *(slave.Get().rbegin()) == SCXCoreLib::SCXFilePath::GetFolderSeparator())
       {
           // A precautionary check - avoid any possible entries in the slaves folder that are not links to  directories
           SCXCoreLib::SCXFilePath slaveName = slave.Get().substr(0, slave.Get().length() - 1);
           if(SCXCoreLib::StrIsPrefix(slaveName.GetFilename(), L"dm-"))
           {
               // add only those files which have the dm- prefix
               dmDeviceStack.push(slaveName.GetFilename());
               return true;
           }
       }

       return false;
    }

    /**
       Get the slave devices that contain the given devicemapper (dm) device.

       \param[in] dmDevice the path to a dm device.

       \return This method will return a vector of paths to the devices that
               contain the given dm device.  If an error occurs the vector is
               empty.

       \throws SCXFileSystemException if the slaves associated with this
               device cannot be located in /dev because of a failure in the
               assumptions about the content of /c dmDevice/slaves.

       \throws This method will also re-throw any of several exceptions that
               can occur while trying to access the devices on the file system.
               Any of these exceptions indicate that the LVM LVM/dm device's
               physical volumes cannot be resolved.
     */
    std::vector< std::wstring > SCXLVMUtils::GetDMSlaves(const std::wstring& dmDevice)
    {
        std::vector< std::wstring > result;
        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.scxlvmutils");
        std::wstringstream       out;

        // At times, the entries in '/sys/block/<dm-device>/slaves/' point to another dm-device.
        // in which case, we must navigate to the slaves of that dm device.
        // we use a non-recursive depth-first traversal.
        std::stack<std::wstring> dmDeviceStack;
        dmDeviceStack.push(((SCXCoreLib::SCXFilePath) dmDevice).GetFilename());
        std::vector< SCXCoreLib::SCXFilePath > slaves;
        unsigned int loopCount = 0;

        while(!dmDeviceStack.empty())
        {
            loopCount++;
            try
            {
                std::wstring currentDeviceName = dmDeviceStack.top();
                dmDeviceStack.pop();
                std::vector< SCXCoreLib::SCXFilePath > slaveEntries = m_extDepends->GetFileSystemEntries(GetSysfsPath(currentDeviceName));
                for( std::vector< SCXCoreLib::SCXFilePath >::const_iterator iter = slaveEntries.begin(); iter != slaveEntries.end(); ++iter)
                {
                    if(!PushIfDMSlave(*iter, dmDeviceStack))
                    {
                        slaves.push_back(*iter);
                    }
                }
                if (loopCount > maxLoopCount)
                {
                    // We assume that the links form an acyclic graph.
                    // But we still need to be wary of infinite loops - perhaps we are on a corrupted file system, or, for some other reason
                    // we are in a circular graph.
                    //
                    // we bail out if the loop count reaches maxLoopCount.
                    //
                    // Note: It is assumed that a cycle of links  will be a very rare scenario. Hence, we are using this "low-tech" way of identifying
                    // cycles instead of relying on more traditional approaches.
                    // This way the vast majority of positive scenarios will only pay the price of one integer increment and one comparison.
                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
                    out.str(L"");
                    out << L"Exceeded " << maxLoopCount << L" while evaluating device " << dmDevice;
                    SCX_LOG(log,suppressor.GetSeverity(out.str()), out.str());
                    slaves.clear();
                    break;
                }
            }
            catch (SCXCoreLib::SCXException & e)
            {
#if defined(linux) &&                                   \
    ( ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) || \
      ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) ) )
                // LVM support on RHEL4 and SLES9 is limited by things like the
                // distribution update, what packages are installed, and even the
                // kernel version.  There are too many variables to try to determine
                // whether or not full LVM support is expected and this is an error
                // or when LVM support is minimal and this can be ignored.  In most
                // cases, this can be ignored, and the warning is logged for the
                // remaining few.
                out.str(L"");
                out << L"Support for LVM on "
#   if defined(PF_DISTRO_SUSE)
                    << L"SuSE Linux Enterprise Server 9 "
#   else
                    << L"Red Hat Enterprise Linux 4 "
#   endif
                    << L"is limited to logical disk metrics.";

                SCX_LOG(log, m_warningSuppressor.GetSeverity(L"SCXLVMUtils::LegacyLvmWarnOneTime"), out.str());

                out.str(L"");
                out << L"Missing LVM support in SysFS; the path " << sysfsPath.str() << L" does not exist.";
                SCX_LOGHYSTERICAL(log, out.str());

                return result;
#endif
                out.str(L"");
                out << L"An exception occurred while getting the slave devices for \"" << dmDevice << L"\": " << e.What();
                SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());
                throw;
            }
        }

        if (slaves.size() == 0)
        {
#if defined(linux) &&                                          \
           ( ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) || \
             ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) ) )
            // LVM support on RHEL4 and SLES9 is limited by things like the
            // distribution update, what packages are installed, and even the
            // kernel version.  There are too many variables to try to determine
            // whether or not full LVM support is expected and this is an error
            // or when LVM support is minimal and this can be ignored.  In most
            // cases, this can be ignored, and the warning is logged for the
            // remaining few.
            out.str(L"");
            out << L"Support for LVM on "
#   if defined(PF_DISTRO_SUSE)
                << L"SuSE Linux Enterprise Server 9 "
#   else
                << L"Red Hat Enterprise Linux 4 "
#   endif
                << L"is limited to logical disk metrics.";

            SCX_LOG(log, m_warningSuppressor.GetSeverity(L"SCXLVMUtils::LegacyLvmWarnOneTime"), out.str());

            out.str(L"");
            out << L"Incomplete LVM support in SysFS; the path " << sysfsPath.str() << L" is empty.";
            SCX_LOGHYSTERICAL(log, out.str());

            return result;
#endif

            out.str(L"");

            out << L"There are no slave entries for the device \"" << dmDevice  << L"\"";
            SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

            throw SCXBadLVMDeviceException(dmDevice, out.str(), SCXSRCLOCATION);
        }

        // Each slave entry should be the name of a block device in /dev.
        for (std::vector< SCXCoreLib::SCXFilePath >::const_iterator iter = slaves.begin();
             iter != slaves.end(); iter++)
        {
            std::wstring dirpath    = iter->GetDirectory();
            size_t       dirPathLen = dirpath.length();
            size_t       pos        = dirpath.rfind(L'/', dirPathLen - 2) + 1;
            size_t       count      = dirPathLen - pos - 1;

            if ((dirPathLen <= 2) || (dirpath[dirPathLen - 1] != L'/') || (pos == std::wstring::npos) || (count == 0))
            {
                out.str(L"");

                out << L"The slave device entry \"" << dirpath << L"\" could not be parsed and will be ignored";
                SCX_LOG(log, m_warningSuppressor.GetSeverity(dirpath), out.str());
                continue;
            }
            std::wstring dirname = dirpath.substr(pos, count);

            // replace all '!' with '/' if special file is in a subdirectory of the /dev directory
            std::replace(dirname.begin(), dirname.end(), '!', '/');

            unsigned int       major = 0;
            unsigned int       minor = 0;
            std::wstringstream devPath;

            devPath << L"/dev/" << dirname;

            // Any exceptions here are unexpected, so just let them move up-stack
            StatPathId(devPath.str(), major, minor);

            // There are some pretty big assumptions being made about paths
            // here.  The assumptions are normally safe, but to be certain
            // that the device at devPath is the same device referenced by
            // /sys... ...slaves/, it is a good idea to match the device
            // major/minor ID.  The major/minor ID values are stored in the
            // dev file within the individual slave entry directories.
            SCXCoreLib::SCXFilePath slaveDevFilePath;

            slaveDevFilePath.SetDirectory(iter->Get());
            slaveDevFilePath.SetFilename(L"dev");

            if (MatchIdInFile(slaveDevFilePath, major, minor))
            {
                // Good!  The device named in /sys... ...slaves/ is the device
                // with the same name in /dev, so add it to the results.
                result.push_back(devPath.str());
            }
            else
            {
                // This is very suspicious.  Reaching this point means that
                // there is either a bad assumption or a half-installed/
                // half-uninstalled LVM partition.
                out.str(L"");

                out << L"The slave device " << iter->Get() << L" does not map to the expected device path " << devPath.str();
                SCX_LOG(log, m_warningSuppressor.GetSeverity(out.str()), out.str());
            }
        }

        return result;
    }

    /**
       Uses stat to get the major/minor device ID for the given path.

       /param[in]     path   the path to a device to stat.

       /param[in/out] major  the value from stat will be compared with this
                             input value, and then this is set to the stat
                             value.

       /param[in/out] minor  the value from stat will be compared with this
                             input value, and then this is set to the stat
                             value.

       \return This method will return true if the input major/minor match
               the values read.

       \throws SCXFileSystemException  if file cannot be stat'ed.
     */
    bool SCXLVMUtils::StatPathId(const std::wstring & path, unsigned int & major, unsigned int & minor)
    {
        bool result = false;

        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.scxlvmutils");
        std::wstringstream       out;

        SCXCoreLib::SCXFileSystem::SCXStatStruct stat;
        memset(&stat, 0, sizeof(stat));

        try
        {
            m_extDepends->Stat(path, &stat);
        }
        catch (SCXCoreLib::SCXException & e)
        {
            out.str(L"");

            out << L"An exception occurred while getting the file status for " << path << L": " << e.What();
            SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

            throw;
        }

        unsigned int majorFromStat = major(stat.st_rdev);
        unsigned int minorFromStat = minor(stat.st_rdev);

        if ((majorFromStat == major) && (minorFromStat == minor))
        {
            result = true;
        }

        // return the values read from stat
        major = majorFromStat;
        minor = minorFromStat;

        return result;
    }

    /**
       Matches the given major/minor device ID with the text read from the
       first line of the file at the given path.

       The files contents are read.  The file must have at least one line of
       text, or it is considered an exception (probably the caller sent a
       bad path in).  The first line shoud be ASCII text (a proper subset
       of UTF-8) in the form "<major> ':' <minor>".  All other text is
       ignored, but a 'normal' Sysfs dev file should contain only that text,
       so any additional text will generate a warning.

       /param[in] path   the path to a file that uses the Sysfs 'dev' file
                         format.

       /param[in] major  the expected major device ID value.

     /param[in] minor  the expected minor device ID value.

       \return This method will return true if the first line of the file at
               the given path contains "<major> ':' <minor>" and these values
               match the values passed in; false otherwise.

       \throws SCXFileSystemException  if file cannot be read or it has no
               lines of text.
     */
    bool SCXLVMUtils::MatchIdInFile(const SCXCoreLib::SCXFilePath & path, unsigned int major, unsigned int minor)
    {
        bool result = false;

        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.scxlvmutils");
        std::wstringstream       out;

        // Read the file, there should be at least one line, and the first
        // line should begin with ASCII (UTF-8) text in the form <major> ':' <minor>.
        std::vector< std::wstring > lines;

        try
        {
            SCXCoreLib::SCXStream::NLFs nlfs;

            m_extDepends->ReadAllLinesAsUTF8(path, lines, nlfs);
        }
        catch (SCXCoreLib::SCXException & e)
        {
            out.str(L"");

            out << L"An exception occurred while reading the file " << path.Get() << L": " << e.What();
            SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

            throw;
        }

        // There should only be exactly one line containting <major> ':' <minor>
        if (lines.size() != 1)
        {
            out.str(L"");

            if (lines.size() == 0)
            {
                out << L"The file " << path.Get() << L" is empty";
                SCX_LOG(log, m_errorSuppressor.GetSeverity(out.str()), out.str());

                throw SCXBadLVMDeviceException(path, out.str(), SCXSRCLOCATION);
            }

            // This is unexpected, but it can be ignored for now.
            out << L"After reading " << path.Get() << L", expected 1 line, but found " << lines.size();
            SCX_LOG(log, m_warningSuppressor.GetSeverity(out.str()), out.str());
        }

        std::wstringstream firstLine(lines[0]);
        int                majorFromFile;
        int                minorFromFile;
        wchar_t          colon;

        firstLine >> majorFromFile >> colon >> minorFromFile;

        if ((static_cast< unsigned int >(majorFromFile) == major) &&
            (static_cast< unsigned int >(minorFromFile) == minor))
        {
            result = true;
        }

        return result;
    }
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
