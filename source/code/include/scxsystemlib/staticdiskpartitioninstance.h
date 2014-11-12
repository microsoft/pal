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
        StaticDiskPartitionInstance(SCXCoreLib::SCXHandle<DiskDepend> deps = SCXCoreLib::SCXHandle<DiskDependDefault>(new DiskDependDefault()));

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
        std::wstring         m_deviceID;                //!< What the name of the device is (perhaps a disk drive identifier)
        unsigned int         m_index;                   //!< Index number of the partition
        scxulong             m_numberOfBlocks;          //!< Total number of consecutive blocks
        scxulong             m_partitionSize;           //!< Total size of the partition (bytes)
        scxulong             m_startingOffset;          //!< Starting offset (in bytes) of partition

        //Let's use a safe handle to our Regex objects
        typedef SCXCoreLib::SCXHandle<SCXRegex> SCXRegexPtr;

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

        SCXCoreLib::SCXHandle<DiskDepend> m_deps; //!< Dependencies to rely on. Used for unit-testing.

    };
} /* namespace SCXSystemLib */
#endif /* STATICDISKPARTITIONINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
