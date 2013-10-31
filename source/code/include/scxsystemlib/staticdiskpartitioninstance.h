/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        staticdiskpartitioninstance.h

    \brief       Implements the disk partition instance pal for static information.

    \date        2011-08-31 11:42:00

*/
/*----------------------------------------------------------------------------*/
#ifndef STATICDISKPARTITIONINSTANCE_H
#define STATICDISKPARTITIONINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/scxdatadef.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxprocess.h>

#include <vector>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    //! Encapsulates all external dependencies for unit-testing.
    class StaticDiskPartitionInstanceDeps 
    {
    public:
        //! Constructor
        StaticDiskPartitionInstanceDeps () { }

        /**
          Wrapper for SCXProcess::Run()
          \param[in]  command     Corresponding to what would be entered by a user in a command shell
          \param[in]  mystdin     stdin for the process
          \param[out] mystdout    stdout for the process
          \param[in]  mystderr    stderr for the process
          \param[in]  timeout     Accepted number of millieconds to wait for return
          \param[in]  cwd         Directory to be set as current working directory for process.
          \param[in]  chrootPath  Directory to be used for chrooting process.
          \returns exit code of run process.
          \throws     SCXInternalErrorException           Failure that could not be prevented
          */
        virtual int Run(const std::wstring &command, std::istream &mystdin,
                   std::ostream &mystdout, std::ostream &mystderr, unsigned timeout)
        {
            return SCXCoreLib::SCXProcess::Run(command, mystdin, mystdout, mystderr, timeout); 
        }

        //! Destructor
        virtual ~StaticDiskPartitionInstanceDeps () { }
    protected:
        //! Prevent copying to avoid slicing
        StaticDiskPartitionInstanceDeps (const StaticDiskPartitionInstanceDeps  &);
    };
/*----------------------------------------------------------------------------*/
/**
    Represents a single disk partition instance with static data.
*/
    class StaticDiskPartitionInstance : public EntityInstance
    {
        friend class StaticDiskPartitionEnumeration;

    public:

        /*----------------------------------------------------------------------------*/
        /**
           Default constructor
        */
        StaticDiskPartitionInstance(SCXCoreLib::SCXHandle<StaticDiskPartitionInstanceDeps> deps = SCXCoreLib::SCXHandle<StaticDiskPartitionInstanceDeps>(new StaticDiskPartitionInstanceDeps()));

        /*----------------------------------------------------------------------------*/
        /**
           Virtual destructor
        */
        virtual ~StaticDiskPartitionInstance();

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the size of the blocks for this partition.

           \param       value - output parameter as the number of blocks.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetPartitionBlockSize(scxulong& value) const {value = m_blockSize;
             return true;}

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the bool that indicates if this is the Boot Partition.

           \param       bootp - output parameter, if true this is the boot partition.
           \returns     true if value was set, otherwise false.
        */
        bool GetBootPartition(bool& bootp) const { bootp = m_bootPartition;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the disk drive/partition name (i.e. '/dev/sda' on Linux).

           \param       value - output parameter of the deviceId.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetDeviceId(std::wstring& value) const { value = m_deviceID;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the Partition Index.

           \param       value - output parameter where the interface of the disk is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetIndex(scxulong& value) const { value = m_index;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the Total number of consecutive blocks, each block the size of the value 
               contained in the BlockSize property, which form this storage extent.

           \param       value - output parameter which is the total number of blocks.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetNumberOfBlocks(scxulong& value) const { value = m_numberOfBlocks;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the partition size of the device, in bytes.

           \param       value - output parameter where the size of the partition is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetPartitionSizeInBytes(scxulong& value) const { value = m_partitionSize;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the Starting offset of the Partition.

           \param       value - output parameter where the starting offset of the partition is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetStartingOffset(scxulong& value) const { value = m_startingOffset;
             return true; }

        /*----------------------------------------------------------------------------*/
        /**
           Create a string version of this object suitable for logging.

           \param       None.
           \returns     std::wstring version of object.
        */
        virtual const std::wstring DumpString() const;

        /*----------------------------------------------------------------------------*/
        /**
           Update the instance. Filling in the fields of StaticDiskPartitionAttributes
           for later retrieval.

           \param       None.
           \returns     None.
        */
        virtual void Update();

#if defined(linux)

        /**
           The Linux version of Update
        */
        void Update_Linux();

        /**
           The check to make sure this DeviceName is also listed with fdisk

           \param       None.
           \returns     bool, True if found in fdisk, False otherwise
        */
        bool CheckFdiskLinux();

        /**
           Execute the 'fdisk -ul' command and save the results to m_fdiskResult

           \param       None.
           \returns     bool, True if successful, false otherwise
        */
        bool GetFdiskResult();

#elif defined(sun)

        /**
           The Solaris version of Update
        */
        void Update_Solaris();

        /**
           get boot drive path, so we only have to do it once
        */
        bool GetBootDrivePath(std::wstring& bootpathStr);

#endif

    private:

        SCXCoreLib::SCXLogHandle m_log;                 //!< Log handle
  
        scxulong             m_blockSize;               //!< Size of a block on this partition
        bool                 m_bootPartition;           //!< If true, this struct is referring to the active boot partition 
        std::wstring            m_deviceID;                //!< What the name of the device is (perhaps a disk drive identifier)
        unsigned int         m_index;                   //!< Index number of the partition
        scxulong             m_numberOfBlocks;          //!< Total number of consecutive blocks
        scxulong             m_partitionSize;           //!< Total size of the partition (bytes)
        scxulong             m_startingOffset;          //!< Starting offset (in bytes) of partition

        //Let's use a safe handle to our Regex objects
        typedef SCXCoreLib::SCXHandle<SCXRegex> SCXRegexPtr;

        // Let's define our Regular Expressions as String constants:
#if defined(linux)
        std::string m_fdiskResult;
        const std::wstring c_cmdFdiskStr;
        const std::wstring c_cmdBlockDevSizeStr;
        const std::wstring c_cmdBlockDevBszStr;
        const std::wstring c_RedHSectorSizePattern;
        const std::wstring c_RedHBootPDetailPattern;
        const std::wstring c_RedHDetailPattern;
#endif
#if defined(sun)

        bool m_isZfsPartition;                                       //!< Sun Microsystems ZFS partition
        const std::wstring c_SolPrtconfPattern;
        const std::wstring c_SolLsPatternBeg;

        const std::wstring c_SolDfPattern;
        const std::wstring c_SolLsPatternEnd;
        const std::wstring c_SolPrtvtocBpSPattern;
        const std::wstring c_SolprtvtocDetailPattern;
#endif
        const size_t MATCHCOUNT;

        SCXCoreLib::SCXHandle<StaticDiskPartitionInstanceDeps> m_deps; //!< Dependencies to rely on. Used for unit-testing.

    };
} /* namespace SCXSystemLib */
#endif /* STATICDISKPARTITIONINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
