/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Defines the static disk information instance PAL for physical disks.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/
#ifndef STATICPHYSICALDISKINSTANCE_H
#define STATICPHYSICALDISKINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/scxdatadef.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <vector>
#if defined(aix)

#include <odmi.h>

// Forward declaration
struct CuVPD;

#endif
namespace SCXSystemLib
{

    /**
       Filter the value of some bits in a digital
      
       \tparam T simple integer type
       
       \param target number to check
       \param flag   mask to filter the value
    */
    template<typename T>
    inline T filter(T target, T flag)
    {
        return static_cast<T>(target & flag);
    }
    /**
       Filter the value of one single bit in a digital
       
       \tparam T simple integer type
       \param target value to test
       \param bit bit position
    */
    template<typename T>
    inline T filterbit(T target, T bit)
    {
        return static_cast<T>(target & (0x01<<bit));
    }

        /*----------------------------------------------------------------------------*/
    /**
      Struct representing all attributes for StaticPhysicalDisk 

     */
    struct StaticPhysicalDiskAttributes 
{
        bool           mediaLoaded;                         //!< If True, the media for a disk drive is loaded, which means that the device has a readable file system and is accessible
        bool           powermanagementSupported;            //!< If True, the device can be power-managed (can be put into suspend mode, and so on).
        unsigned short availability;                        //!< Availability and status of the device, must be value in enum DiskAvailabilityType
        unsigned int   SCSIBus;                             //!< SCSI bus number of the disk drive.
        unsigned short SCSIPort;                            //!< SCSI disk drive port.
        unsigned short SCSILogicalUnit;                     //!< SCSI logical unit number (LUN) of the disk drive.
        unsigned short SCSITargetId;                        //!< SCSI identifier number of the disk drive.
        std::vector<unsigned short> powerManagementCapabilities; // Power management capabilities of the disk.
        unsigned short capabilities[eDiskCapCnt];           //!< Array of capabilities of the media access device. values defined in enum DiskCapabilitiesType
        std::wstring      firmwareRevision;                    //!< Revision for the disk drive firmware that is assigned by the manufacturer.
        std::wstring      mediaType;                           //!< Type of media used or accessed by this device.
        std::wstring      serialNumber;                        //!< Number allocated by the manufacturer to identify the physical media. 
        unsigned int   partitions;                          //!< Number of partitions on this physical disk drive that are recognized by the operating system.
        unsigned int   sectorsPerTrack;                     //!< Number of sectors in each track for this physical disk drive.
        unsigned int   signature;                           //!< Disk identification. This property can be used to identify a shared resource. 
    };
/*----------------------------------------------------------------------------*/
/**
    Represents a single disk instance with static data.
*/
    class StaticPhysicalDiskInstance : public EntityInstance
    {
        friend class StaticPhysicalDiskEnumeration;

    public:
        StaticPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticPhysicalDiskInstance();

        bool GetHealthState(bool& healthy) const;
        bool GetDiskName(std::wstring& value) const;
        bool GetDiskDevice(std::wstring& value) const;

        bool GetInterfaceType(DiskInterfaceType& value) const;
        bool GetManufacturer(std::wstring& value) const;
        bool GetModel(std::wstring& value) const;
        bool GetSizeInBytes(scxulong& value) const;
        bool GetTotalCylinders(scxulong& value) const;
        bool GetTotalHeads(scxulong& value) const;
        bool GetTotalSectors(scxulong& value) const;
        bool GetSectorSize(unsigned int& value) const;
        bool GetAvailability(unsigned short& value) const;
        bool GetCapabilities(std::vector<unsigned short>& value) const;
        bool GetCapabilityDescriptions(std::vector<std::wstring>& value) const;
        bool GetFirmwareRevision(std::wstring& value) const;
        bool GetMediaLoaded(bool& value) const;
        bool GetMediaType(std::wstring& value) const;
        bool GetPartitions(unsigned int& value) const;
        bool GetPowerManagementCapabilities(std::vector<unsigned short>& value) const;
        bool GetPowerManagementSupported(bool& value) const;
        bool GetSCSIBus(unsigned int & value) const;
        bool GetSCSIPort(unsigned short & value) const;
        bool GetSCSILogicalUnit(unsigned short& value) const;
        bool GetSCSITargetId(unsigned short & value) const;
        bool GetSectorsPerTrack(unsigned int & value) const;
        bool GetSerialNumber(std::wstring& value) const;
        bool GetSignature(unsigned int & value) const;
        bool GetTracksPerCylinder(scxulong& value) const;
        bool GetTotalTracks(scxulong& value) const;


        

        /** Set the device ID for this instance, e.g. /dev/sda */
        void SetDevice( const std::wstring& device ) {m_device = device;}

        virtual const std::wstring DumpString() const;
        virtual void Update();
        
        virtual void  SetUnexpectedException( const SCXCoreLib::SCXException& e );

        /*----------------------------------------------------------------------------*/
        /**
           Test aid. Gets the number of instances that currently exist.
        
           \returns     number of instances currently in existance.
        */
        static size_t GetCurrentInstancesCount()
        {
            return m_currentInstancesCount;
        }
        /*----------------------------------------------------------------------------*/
        /**
           Test aid. Gets the number of instances created since the module was started and static varables
           were initialized.
        
           \returns     number of instances created since the start of the module.
        */
        static size_t GetInstancesCountSinceModuleStart()
        {
            return m_instancesCountSinceModuleStart;
        }
    private:
        // For testing purposes we count the number of instances currently in existance.
        static size_t m_currentInstancesCount;
        // For testing purposes we count the number of instances created since the module was started and static varables
        // were initialized.
        static size_t m_instancesCountSinceModuleStart;

        //! Private constructor (this should never be called!)
        StaticPhysicalDiskInstance();            //!< Default constructor (intentionally not implemented)
        StaticPhysicalDiskInstance(const StaticPhysicalDiskInstance&); //!< Copy constructor (intentionally not implemented)
        void Clear();

#if defined(linux)

        /**
           update disk signature by file descriptor
        */
        void UpdateDiskSignature();

        /**
           check support writing status
        */
        void CheckSupportWriting();

        // HDIO power mode codes:
        static const unsigned char POWERMODE_UNSET    = (unsigned char)0xF0;
        static const unsigned char POWERMODE_STANDBY  = (unsigned char)0x00;
        static const unsigned char POWERMODE_SPINDOWN = (unsigned char)0x40;
        static const unsigned char POWERMODE_SPINUP   = (unsigned char)0x41;
        static const unsigned char POWERMODE_IDLE     = (unsigned char)0x80;
        static const unsigned char POWERMODE_ACTIVE   = (unsigned char)0xFF;

        /**
           check power mode for IDE disks.
        */
        void CheckATAPowerMode();

        /*
           (WI-479079) check power mode for SCSI disks.
        */
        void CheckSCSIPowerMode();

        /**
           issue power mode drive command.
        */
        unsigned char DriveCmdATAPowerMode(unsigned char cmd);

        /**
           update the SCSI attributes include SCSIBus, SCSILogicalUnit, SCSITargetId
        */
        void UpdateSCSIAttributes();

        /**
           get partitions information from the file /proc/partitions
        */

        void ParsePartitions();
        /**
           SCSI Generic(sg) inquery.
           Get the data retrieved from the SCSI INQUIRY command.
           
           \param[in] page vital product data page code
           \param[in] evpd if set the evpd bit of the command
           \param[in|out] dxferp INQUIRY data
           \param[in] dxfer_len length of the dxferp in bytes

           \return true if inquery succeeded, false if ioctl fail or error happened.
         */
        bool SqInq(int page, int evpd, void * dxferp, unsigned short dxfer_len);

        /**
            This method gets from the kernel disk size and disk geometry for both SCSI and ATA drives.
            Then it validates the data and sets disk size and geometry for this disk instance.
        */
        void DiskSizeAndGeometryFromKernel();
#endif
#if defined(linux) || defined(sun)
        /**
            This method validates the disk size and geometry data and if data is not valid falls back to 255
            heads - 63 cylinders per track geometry, just like command line utilities like fdisk.
            Then it sets disk size and geometry data for this physical disk instance.

            \param totalSizeFromK       - Total disk size returned by the kernel or 0 if kernel could not provide the data.
            \param sectorSizeFromK      - Sector size returned by the kernel or 0 if kernel could not provide the data.
            \param cylinderCntFromK     - Cylinder count returned by the kernel or 0 if kernel could not provide the data.
            \param headCntFromK         - Head count returned by the kernel or 0 if kernel could not provide the data.
            \param sectorsPerTrackFromK - Sectors/track returned by the kernel or 0 if kernel could not provide the data.
        */
        void GetDiskGeometry(scxulong totalSize, scxulong sectorSize,
                scxulong cylinderCnt, scxulong headCnt, scxulong sectorsPerTrack);
#endif// defined(linux) || defined(sun)
#if defined(aix)
        // Some AIX-specific routines to reduce complexity
        void DecodeVPD(const struct CuVPD *vpdItem);
        int LookupODM(CLASS_SYMBOL c, const std::wstring &criteria, void *pData);
#endif

        SCXCoreLib::SCXHandle<DiskDepend> m_deps;//!< StaticDiskDepend object
        SCXCoreLib::SCXLogHandle m_log;          //!< Log handle
        bool m_online;                           //!< Tells if disk is still connected.
        std::wstring m_device;                   //!< Device ID (i.e. /dev/sda)
        std::wstring m_rawDevice;                //!< Raw device name (internal use only)
#if defined(linux)
        bool m_cdDrive;                          //!< Optical drive
#endif

        bool m_isMBR;                            //!< indicate if it is a MBR
        DiskInterfaceType m_intType;             //!< Interface type of device (IDE, SCSI, etc)
        std::wstring m_manufacturer;             //!< Disk drive manufacturer
        std::wstring m_model;                    //!< Disk drive model
        scxulong m_sizeInBytes;                  //!< Total size, in bytes
        scxulong m_totalCylinders;               //!< Total number of cylinders
        scxulong m_totalHeads;                   //!< Total number of heads
        scxulong m_totalSectors;                 //!< Total number of sectors
        scxulong m_totalTracks;                  //!< Total number of tracks
        scxulong m_trackSize;                    //!< Track size, in bytes
        scxulong m_tracksPerCylinder;            //!< Average number of tracks per cylinder
        unsigned int m_sectorSize;               //!< Sector size, in bytes
        struct StaticPhysicalDiskAttributes m_Properties;   //!< StaticPhysicalDiskAttributes.

    };
} /* namespace SCXSystemLib */
#endif /* STATICPHYSICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
