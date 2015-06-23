/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implements the physical disk instance pal for static information.

    \date        2008-03-19 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/staticphysicaldiskinstance.h>

/* System specific includes - generally for Update() method */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#if defined(linux)
#include <sstream>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

#elif defined(aix)

#include <scxsystemlib/scxodm.h>

#elif defined(hpux)

#include <sys/diskio.h>
#include <sys/scsi.h>

#elif defined(sun)

#include <sys/dkio.h>
#include <sys/vtoc.h>

#endif

#if defined(aix)

// Debug output decoding a CuVPD structure on AIX
#include <iostream>
const bool debugDumpVPD = 0;

#endif

using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{
    size_t StaticPhysicalDiskInstance::m_currentInstancesCount = 0;
    size_t StaticPhysicalDiskInstance::m_instancesCountSinceModuleStart = 0;

/*----------------------------------------------------------------------------*/
/**
   Constructor.

   \param       deps A StaticDiscDepend object which can be used.

*/
    StaticPhysicalDiskInstance::StaticPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps)
        : m_deps(deps), m_online(0), m_rawDevice(L"")
#if defined(linux)
        , m_cdDrive(false)
#endif
    {
        m_currentInstancesCount++;
        m_instancesCountSinceModuleStart++;
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(
                L"scx.core.common.pal.system.disk.staticphysicaldiskinstance");

        // Let's reset the global error number while we're at it.
        errno = 0;

        // Clear all properties returned by the object. Clear() is also called at the beginning of the Update()
        // method because the data refresh algorithms in the Update() require that properties are cleared at the start.
        // If new properties are added to the object they should be initialized inside the Clear() so they are
        // cleared both at the object creation and at the beginning of the Update() method.
        Clear();
    }

/*----------------------------------------------------------------------------*/
/**
   Virtual destructor.
*/
    StaticPhysicalDiskInstance::~StaticPhysicalDiskInstance()
    {
        m_currentInstancesCount--;
    }

/*----------------------------------------------------------------------------*/
/**
   Clears disk instance. All of the properties must be initialized to their default values here. This method is
   called by the constructor and the Update() method.
*/
    void StaticPhysicalDiskInstance::Clear()
    {
        m_intType = eDiskIfcUnknown;
        m_isMBR                               = false;
        m_manufacturer                        = L"";
        m_model                               = L"";
    
        m_Properties.mediaLoaded              = false;
        m_Properties.powermanagementSupported = false;
        m_Properties.availability             = eDiskAvaUnknown;
        m_Properties.SCSIBus                  = 0;
        m_Properties.SCSIPort                 = 0;
        m_Properties.SCSILogicalUnit          = 0;
        m_Properties.SCSITargetId             = 0;
        m_Properties.firmwareRevision         = L"";
        m_Properties.mediaType                = mediaTypeNames[3]; // Default value is "Format is unknown".
        m_Properties.serialNumber             = L"";
        m_Properties.partitions               = 0;
        m_Properties.sectorsPerTrack          = 0;
        m_Properties.signature                = 0;
        m_Properties.powerManagementCapabilities.clear();
        for(unsigned short i = 0; i < eDiskCapCnt; ++i)
        {
            m_Properties.capabilities[i] = eDiskCapInvalid;
        }

        m_sizeInBytes = 0;
        m_totalCylinders = 0;
        m_totalHeads = 0;
        m_totalSectors = 0;
        m_totalTracks = 0;
        m_trackSize = 0;
        m_tracksPerCylinder = 0;
        m_sectorSize = 0;
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the disk health state.

   \param       healthy - output parameter where the health status of the disk is stored.
   \returns     true if value was set, otherwise false.
*/
    bool StaticPhysicalDiskInstance::GetHealthState(bool& healthy) const
    {
        healthy = m_online;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the disk name (i.e. 'sda' on Linux).

   \param       value - output parameter where the name of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetDiskName(wstring& value) const
    {
        value = GetId();
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the disk name (i.e. '/dev/sda' on Linux).

   \param       value - output parameter where the device of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetDiskDevice(wstring& value) const
    {
        value = m_device;
        return true;
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the disk interface type.

   \param       value - output parameter where the interface of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetInterfaceType(DiskInterfaceType& value) const
    {
        value = m_intType;
#if defined(linux) || defined(hpux) || defined(sun) || defined(aix)
        return true;
#else
#error Define return type on platform for method GetInterfaceType
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the manufacturer of the device.

   \param       value - output parameter where the manufacturer of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetManufacturer(wstring& value) const
    {
        value = m_manufacturer;
#if defined(linux) || defined(aix) || defined(hpux)
        return true;
#elif defined(sun)
        return false;
#else
#error Define return type on platform for method GetManufacturer
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the model of the device.

   \param       value - output parameter where the model of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetModel(wstring& value) const
    {
        value = m_model;
#if defined(linux) || defined(aix) || defined(hpux)
        return true;
#elif defined(sun)
        return false;
#else
#error Define return type on platform for method GetModel
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the total size of the device, in bytes.

   \param       value - output parameter where the size of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSizeInBytes(scxulong& value) const
    {
        value = m_sizeInBytes;
#if defined(linux) || defined(aix) || defined(hpux) || defined(sun)
        return true;
#else
#error Define return type on platform for method GetSizeInBytes
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the cylinder count of the device.

   \param       value - output parameter where the cylinder count of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalCylinders(scxulong& value) const
    {
        value = m_totalCylinders;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalCylinders
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the head count of the device.

   \param       value - output parameter where the head count of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalHeads(scxulong& value) const
    {
        value = m_totalHeads;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalHeads
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the sector count of the device.

   \param       value - output parameter where the sector count of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalSectors(scxulong& value) const
    {
        value = m_totalSectors;
#if defined(linux) || defined(sun)
        return true;
#elif defined(aix) || defined(hpux)
        return false;
#else
#error Define return type on platform for method GetTotalSectors
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve the sector size of the device.

   \param       value - output parameter where the sector size of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.

   Note: The sector size is almost always 512 bytes, if not always.  However, if we can't get
   it for a platform, we return FALSE, and the provider can provide a default if desired.
*/
    bool StaticPhysicalDiskInstance::GetSectorSize(unsigned int& value) const
    {
        value = m_sectorSize;
#if defined(aix)
        return false;
#elif defined(hpux) || defined(sun) || defined(linux)
        return true;
#else
#error Define return type on platform for method GetSectorSize
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve availability and status of the device.

   \param       value - output parameter where the availability of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetAvailability(unsigned short& value) const
    {
#if defined(linux)
        value = m_Properties.availability;

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve array of capabilities of the media access device .

   \param       value - output parameter where the Capabilities of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetCapabilities(std::vector<unsigned short>& value) const
    {
#if defined(linux)
        unsigned short di; // loop counter

        value.clear();
        value.reserve(eDiskCapCnt);

        for(di = 0; di < eDiskCapCnt; ++di)
        {
            if(m_Properties.capabilities[di] < eDiskCapCnt)
            {
                value.push_back(m_Properties.capabilities[di]);
            }
        }

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve list of more detailed explanations for any of the access device features indicated in the Capabilities array .

   \param       value - output parameter where the CapabilityDescriptions of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetCapabilityDescriptions(std::vector<wstring>& value) const
    {
#if defined(linux)
        unsigned short di;
        value.clear();

        for(di = 0; di < eDiskCapCnt; ++di)
        {
            if(m_Properties.capabilities[di] < eDiskCapCnt)
            {
                value.push_back(capabilityDescriptions[di]);
            }
        }

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve revision for the disk drive firmware that is assigned by the manufacturer .

   \param       value - output parameter where the FirmwareRevision of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetFirmwareRevision(wstring& value) const
    {
#if defined(linux)
        value = m_Properties.firmwareRevision;

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve if the media for a disk drive is loaded .

   \param       value - output parameter where the MediaLoaded of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetMediaLoaded(bool& value) const
    {
#if defined(linux)|| defined(sun)
        value = m_Properties.mediaLoaded;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve type of media used or accessed by this device .

   \param       value - output parameter where the MediaType of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetMediaType(wstring& value) const
    {
#if defined(linux)|| defined(sun)
        value = m_Properties.mediaType;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve number of partitions on this physical disk drive that are recognized by the operating system .

   \param       value - output parameter where the Partitions of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetPartitions(unsigned int& value) const
    {
#if defined(linux)|| defined(sun)
        value = m_Properties.partitions;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve array of the specific power-related capabilities of a logical device .

   \param       value - output parameter where the PowerManagementCapabilities of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetPowerManagementCapabilities(std::vector<unsigned short>& value) const
    {
#if defined(linux)
        value.assign(m_Properties.powerManagementCapabilities.begin(),
                     m_Properties.powerManagementCapabilities.end());

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve if the device can be power-managed (can be put into suspend mode, and so on).

   \param       value - output parameter where the PowerManagementSupported of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetPowerManagementSupported(bool& value) const
    {
#if defined(linux) || defined(sun)
        value = m_Properties.powermanagementSupported;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve SCSI bus number of the disk drive .

   \param       value - output parameter where the SCSIBus of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSCSIBus(unsigned int & value) const
    {
#if defined(linux) || defined(sun)
        if (eDiskIfcSCSI == m_intType)
        {
            value = m_Properties.SCSIBus;
            return true;
        }

        return false;
#else
        (void) value;

        return false;
#endif
    }


/*----------------------------------------------------------------------------*/
/**
   Retrieve SCSI logical unit number (LUN) of the disk drive .

   \param       value - output parameter where the SCSILogicalUnit of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSCSILogicalUnit(unsigned short& value) const
    {
#if defined(linux) || defined(sun)
        if (eDiskIfcSCSI == m_intType)
        {
            value = m_Properties.SCSILogicalUnit;
            return true;
        }

        return false;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve SCSI port number of the disk drive .

   \param       value - output parameter where the SCSIPort of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSCSIPort(unsigned short& value) const
    {
#if defined(linux)
        if (eDiskIfcSCSI == m_intType)
        {
            value = m_Properties.SCSIPort;
            return true;
        }

        return false;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve SCSI identifier number of the disk drive .

   \param       value - output parameter where the SCSITargetId of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSCSITargetId(unsigned short & value) const
    {
#if defined(linux) || defined(sun)
        if (eDiskIfcSCSI == m_intType)
        {
            value = m_Properties.SCSITargetId;
            return true;
        }

        return false;
#else
        (void) value;

        return false;
#endif
    }


/*----------------------------------------------------------------------------*/
/**
   Retrieve number of sectors in each track for this physical disk drive .

   \param       value - output parameter where the SectorsPerTrack of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSectorsPerTrack(unsigned int & value) const
    {
#if defined(linux) || defined(sun)
        value = m_Properties.sectorsPerTrack;

        return true;
#else
        (void) value;

        return false;
#endif
    }


/*----------------------------------------------------------------------------*/
/**
   Retrieve number allocated by the manufacturer to identify the physical media .

   \param       value - output parameter where the SerialNumber of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSerialNumber(wstring& value) const
    {
#if defined(linux)
        value = m_Properties.serialNumber;

        return true;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }


/*----------------------------------------------------------------------------*/
/**
   Retrieve disk identification .

   \param       value - output parameter where the Signature of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetSignature(unsigned int & value) const
    {
#if defined(linux)
        if (m_isMBR)
        {
            value = m_Properties.signature;
            return true;
        }

        return false;
#else
        // sun is not supported; other platforms are not yet implemented
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve number of tracks in each cylinder on the physical disk drive.
   this value equals the number of head.
   \param       value - output parameter where the TracksPerCylinder of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTracksPerCylinder(scxulong& value) const
    {
#if defined(linux)|| defined(sun)
        value = m_tracksPerCylinder;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Retrieve total number of tracks on the physical disk drive..
   \param       value - output parameter where the TotalTracks of the disk is stored.
   \returns     true if value is supported on platform, false otherwise.
*/
    bool StaticPhysicalDiskInstance::GetTotalTracks(scxulong& value) const
    {
#if defined(linux)|| defined(sun)
        value = m_totalTracks;

        return true;
#else
        (void) value;

        return false;
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   Dump the object as a string for logging purposes.

   \returns     String representation of object, suitable for logging.
*/
    const wstring StaticPhysicalDiskInstance::DumpString() const
    {
        return SCXDumpStringBuilder("StaticPhysicalDiskInstance")
            .Text("Name", GetId())
            .Text("Device", m_device)
            .Text("RawDevice", m_rawDevice)
            .Scalar("Online", m_online)

            .Scalar("Availability", m_Properties.availability)
            .Text("FirmwareRevision", m_Properties.firmwareRevision)
            .Scalar("InterfaceType", m_intType)
            .Scalar("IsMBR", m_isMBR)
            .Text("Manufacturer", m_manufacturer)
            .Scalar("MediaLoaded", m_Properties.mediaLoaded)
            .Text("MediaType", m_Properties.mediaType)
            .Text("Model", m_model)
            .Scalar("Partitions", m_Properties.partitions)
            .Scalar("PowerManagementSupported", m_Properties.powermanagementSupported)
            .Text("SerialNo", m_Properties.serialNumber)
            .Scalar("Signature", m_Properties.signature)

            // SCSI address.
            .Scalar("SCSIBus", m_Properties.SCSIBus)
            .Scalar("SCSIPort", m_Properties.SCSIPort)
            .Scalar("SCSITargetID", m_Properties.SCSITargetId)
            .Scalar("SCSILogicalUnit", m_Properties.SCSILogicalUnit)

            // Disk geometry.
            .Scalar("SizeInBytes", m_sizeInBytes)
            // Cylinders/Heads/Tracks.
            .Scalar("TracksPerCylinder", m_tracksPerCylinder)
            .Scalar("TotalHeads", m_totalHeads)
            .Scalar("SectorsPerTrack", m_Properties.sectorsPerTrack)
            // Rest of the goemetry.
            .Scalar("SectorSize", m_sectorSize)
            .Scalar("TotalCylinders", m_totalCylinders)
            .Scalar("TotalSectors", m_totalSectors)
            .Scalar("TotalTracks", m_totalTracks)
            .Scalar("TrackSize", m_trackSize);
    }

#if defined(aix)
/*----------------------------------------------------------------------------*/
/**
   Decode a VPD ("Vital Product Data") structure on the AIX platform.
   Object is updated as appropriate for the AIX platform.

   \param       vpdItem - CuVPD structure from AIX platform
   \returns     Updated object (for manufacturer and model)
*/

    void StaticPhysicalDiskInstance::DecodeVPD(const struct CuVPD *vpdItem)
    {
        // Decodes a VPD (Vital Product Data) on the AIX platform.
        //
        // A VPD structure can be in two forms:
        // 1. Section 12.5 of document "Standard for Power Architecture Platform
        //    Requirements (Workstation, Server)", Version 2.2, dated 10/9/1007.
        //    This document is available on power.org, and has also been posted
        //    to the Wiki (Documents/Networking and Protocols), as well as a
        //    link in WI 3344.
        // 2. From IBM's old microchannel bus machines which IBM still uses
        //    for devices that are still around from those days.
        //
        // We only decode the old microchannel bus machines.  All disks
        // (except perhaps SAS disks - we can't test) return their VPD data
        // in the old microchannel bus format.  If we can't decode the VPD
        // data, then the manufacturer & model simply isn't provided.
        //
        // If, at a future date, AIX returns data as per section 12.5, then
        // this code can be updated to support that.
        //
        // This information comes via a personal contact at IBM, and is not
        // formally documented by IBM.
        //
        // The old microchannel bus format will always start with an asterick
        // (*) character.  In general, it can be described as consisting of one
        // or more VPD keywords with each one having the following structure:
        //
        //    *KKLdd...d
        //
        // where:
        //    *       = The asterick character ('*')
        //    KK      = A two-character keyword
        //    L       = A one byte binary value of half the length of the entire
        //              "*KKLdd...d" string of bytes.  In other words, 2*L is
        //              the length of the string of bytes.
        //    dd....d = The string of actual VPD data for the keyword.  This may
        //              be binary or ascii string (not NULL terminated) depending
        //              on the keyword.
        //
        // Note: 2*L includes the 4 bytes for the *KKL bytes as well as the
        // number of bytes in dd...d.  Also note that because L has to be
        // doubled, the length is always an even number of bytes.  If there is
        // an odd number of bytes in the dd...d value, then it must be padded
        // with an 0x00 byte.
        //
        // There should be an 0x00 byte following the last *KKdd...d.  So you
        // process the data in a loop looking for * characters.  The L value can
        // be used to determine where the next '*' character should be.  However,
        // if it is 0x00, then you are done.

        const char *p = vpdItem->vpd;
        if (*p != '*')
        {
            // If we're not in old microchannel bus format, we're done
            return;
        }

        std::string tag;
        std::vector<char> value;
        value.resize(sizeof(vpdItem->vpd) + 1, 0);

        while (*p == '*' && p < (vpdItem->vpd + sizeof(vpdItem->vpd)) ) {
            // Length includes "*xxl", where XX=2 char ID and l==total len
            int totalLen = ( p[3] * 2 );
            int itemLen = totalLen - 4;

            tag.clear();
            tag.assign( p+1, p+1+2 );

            SCXASSERT( itemLen < sizeof(vpdItem->vpd) );
            memcpy(&value[0], &p[4], itemLen);
            value[itemLen] = '\0';

            if ( debugDumpVPD )
            {
                std::cout << "  Tag: " << tag.c_str() << ", Value: " << &value[0] << std::endl;
            }

            if (0 == tag.compare("MF"))
            {
                m_manufacturer = StrTrim(StrFromUTF8(&value[0]));
            }
            else if (0 == tag.compare("TM"))
            {
                m_model = StrTrim(StrFromUTF8(&value[0]));
            }

            p += totalLen;
        }

        return;
    }

/*----------------------------------------------------------------------------*/
/**
   Look up some data via the ODM (Object Data Model) interface on AIX.

   \param       class - Class symbol to be looked up
   \param       criteria - Search criteria to use
   \param       pData - Class-specific data structure to return data into

    \returns     -1 if internal error occurred, 0 if search criteria failed, > 1 if lookup succeeded
*/

    int StaticPhysicalDiskInstance::LookupODM(CLASS_SYMBOL c, const wstring &wCriteria, void *pData)
    {
        void *pResult;

        try {
            SCXodm odm;

            pResult = odm.Get(c, wCriteria, pData);
            if (NULL == pResult)
        {
                return 0;
            }
        }
        catch (SCXodmException)
        {
            return -1;
        }

        SCXASSERT( pResult == pData );
        return 1;
    }
#endif

#if defined(linux) || defined(sun)

/*----------------------------------------------------------------------------*/
/**
    This method validates the disk size and geometry data and if data is not valid falls back to 255
    heads - 63 cylinders per track geometry, just like command line utilities like fdisk do.
    Then it sets disk size and geometry data for this physical disk instance.

    \param totalSizeFromK       - Total disk size returned by the kernel or 0 if kernel could not provide the data.
    \param sectorSizeFromK      - Sector size returned by the kernel or 0 if kernel could not provide the data.
    \param cylinderCntFromK     - Cylinder count returned by the kernel or 0 if kernel could not provide the data.
    \param headCntFromK         - Head count returned by the kernel or 0 if kernel could not provide the data.
    \param sectorsPerTrackFromK - Sectors/track returned by the kernel or 0 if kernel could not provide the data.
*/
    void StaticPhysicalDiskInstance::GetDiskGeometry(scxulong totalSizeFromK, scxulong sectorSizeFromK,
            scxulong cylinderCntFromK, scxulong headCntFromK, scxulong sectorsPerTrackFromK)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        // Clear all the data, if we do not succeed we return 0.
        m_sizeInBytes = 0;
        m_totalCylinders = 0;
        m_totalHeads = 0;
        m_totalSectors = 0;
        m_totalTracks = 0;
        m_trackSize = 0;
        m_tracksPerCylinder = 0;
        m_sectorSize = 0;
        m_Properties.sectorsPerTrack = 0;

        // We have all the data we can get from the kernel. Now evaluate the data and calculate the geometry.
        // We will try to see if kernel cylinder-head-sector data mekes sense by comparing it to the actual
        // disk size. If it doesnt look goot we use default geometry of 255 heads and 63 sectors per track
        // and from that calculate how many cylinders we have. Similar approach is used by utilities like
        // fdisk that also use geometry of 255 heads and 63 sectors per track as a last resort, except they may
        // also try to get partition data from disk as well as user data.
        //
        // Additional info:
        // according to http://support.microsoft.com/kb/258281, (c,h,s) remapping, used on old IDE drives > 512 Mb,
        // assumes that there are 255 heads and 63 sectors per track; INT 13h func. 48h remapping, used on SCSI and
        // SATA drives, usually assumes 15 or 16 heads.


        // First we must have total size of the disk. Without that we can not evaluate if the rest of the data is
        // valid or not.
        if (totalSizeFromK == 0)
        {
            // If we don't have size we can not continue.
            std::wstringstream out;
            out << L"Total disk size not detected for physical disk \"" << m_device <<
                    L"\". Disk geometry will not be provided.";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            return;
        }

        // Now determine sector size.
        if (sectorSizeFromK == 0)
        {
            // We do not have a sector size, use most common size of 512 for the disk geometry.
            sectorSizeFromK = 512;
            std::wstringstream out;
            out << L"Sector size not detected for physical disk \"" << m_device <<
                    L"\". Using size of 512.";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        // By default we use 255 heads - 63 sectors/track geometry, unless kernel data contains valid data.
        scxulong headCntTmp = 255;
        scxulong sectorsPerTrackTmp = 63;
        scxulong cylinderCntTmp = totalSizeFromK / (sectorSizeFromK * sectorsPerTrackTmp * headCntTmp);
        // Now try to find better solution from the kernel data.
        if (cylinderCntFromK != 0 && headCntFromK != 0 && sectorsPerTrackFromK != 0)
        {
            // There is kernel CHS data, check if it is valid.
            
            scxulong cylinderSizeFromK = sectorSizeFromK * sectorsPerTrackFromK * headCntFromK;
            // Get difference between total size and size according to geometry from kernel data.
            scxlong delta = static_cast<scxlong>(totalSizeFromK) -
                    static_cast<scxlong>(cylinderSizeFromK * cylinderCntFromK);
            // Difference must be less than one cylinder size or we decide we have invalid geometry from the kernel data
            // and we use default 255 heads - 63 sectors/track geometry.
            scxulong absDelta = ::llabs(delta);
            if (absDelta < cylinderSizeFromK)
            {
                // Kernel geometry is valid, use it.
                headCntTmp = headCntFromK;
                cylinderCntTmp = cylinderCntFromK;
                sectorsPerTrackTmp = sectorsPerTrackFromK;
            }
#if defined(sun)
            // Check if we are missing one cylinder (common on Solaris x86) and if we do correct it. If cylinder
            // is missing delta must be positive. We also need to do check for positive delta so division by
            // cylinderSizeFromK would work properly.
            else if((delta > 0) && ((delta / cylinderSizeFromK) == 1))
            {
                // We are missing exactly one cylinder. Kernel geometry is valid, use it but increase the cylinder
                // count by 1.
                headCntTmp = headCntFromK;
                cylinderCntTmp = cylinderCntFromK + 1;
                sectorsPerTrackTmp = sectorsPerTrackFromK;
            }
#endif            
        }

        // Success, load the data into the object.
        m_sizeInBytes = totalSizeFromK;
        m_totalCylinders = cylinderCntTmp;
        m_totalHeads = headCntTmp;
        m_totalSectors = totalSizeFromK / sectorSizeFromK;
        m_totalTracks = totalSizeFromK / (sectorsPerTrackTmp * sectorSizeFromK);
        m_trackSize = sectorsPerTrackTmp * sectorSizeFromK;
        m_tracksPerCylinder = headCntTmp;
        m_sectorSize = static_cast<unsigned int>(sectorSizeFromK);
        m_Properties.sectorsPerTrack = static_cast<unsigned int>(sectorsPerTrackTmp);
    }
#endif// defined(linux) || defined(sun)

#if defined(linux)

/*----------------------------------------------------------------------------*/
/**
    This method gets from the kernel disk size and disk geometry for both SCSI and ATA drives.
    Then it validates the data and sets disk size and geometry for this disk instance.
*/

    void StaticPhysicalDiskInstance::DiskSizeAndGeometryFromKernel()
    {
        scxulong totalSize = 0;
        scxulong sectorSize = 0;
        scxulong cylinderCnt = 0;
        scxulong headCnt = 0;
        scxulong sectorsPerTrack = 0;

        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

        struct hd_geometry kernelGeometry;
        memset(&kernelGeometry, 0, sizeof(kernelGeometry));
        // Value returned by BLKSSZGET is of unknown type. BLKSSZGET is defined with _IO that has data size
        // hardcoded to 0, and there's no explanation in the LINUX header for the data size. Some examples
        // use int some use long some use size_t. To be on the safe side we'll use 64 bit so there's no possibility
        // that ioctl may overwrite memory around the variable.
        u_int64_t kernelSectorSize = 0;
        // Value returned by BLKGETSIZE is long. There is a bug in the LINUX header where BLKGETSIZE is
        // defined with _IO which has data size hardcoded to 0 since it is not read/write ioctl code but
        // just a control code. However there is a comment in the header explaining that ioctl returns long.
        unsigned long kernelTotalSize = 0;
        // Value returned by BLKGETSIZE64 is unsigned 64 bit. There is a bug in the LINUX header where BLKGETSIZE64 is
        // declared as returning size_t which may be 32 bit. But ioctl actualy returns 64 bits, as explained in
        // the header.
        u_int64_t kernelTotalSize64 = 0;

        // First get all data we can from the kernel. Also check if any of the data is 0, just in case.
        // Log errors on failures.
        int ret = m_deps->ioctl(HDIO_GETGEO, &kernelGeometry);
        if (ret == 0 && kernelGeometry.sectors != 0 && kernelGeometry.heads != 0 && kernelGeometry.cylinders != 0)
        {
            cylinderCnt = kernelGeometry.cylinders;
            headCnt = kernelGeometry.heads;
            sectorsPerTrack = kernelGeometry.sectors;
        }
        else if (ret == -1)
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(HDIO_GETGEO) failed with errno = " << StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(HDIO_GETGEO) returned non-zero value = " << StrFrom(ret) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        ret = m_deps->ioctl(BLKSSZGET, &kernelSectorSize);
        if (ret == 0)
        {
            sectorSize = kernelSectorSize;
        }
        else if (ret == -1)
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(BLKSSZGET) failed with errno = " << StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(BLKSSZGET) returned non-zero value = " << StrFrom(ret) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        ret = m_deps->ioctl(BLKGETSIZE64, &kernelTotalSize64);
        if (ret == 0)
        {
            totalSize = kernelTotalSize64;
        }
        else if (ret == -1)
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(BLKGETSIZE64) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());

            // Failure, but there's another option to get total disk size.
            ret = m_deps->ioctl(BLKGETSIZE, &kernelTotalSize);
            if (ret == 0)
            {
                totalSize = kernelTotalSize * 512;
            }
            else if (ret == -1)
            {
                out.str(L"");
                out << L"On device \"" << m_device << L"\" ioctl(BLKGETSIZE) failed with errno = " << StrFrom(errno) << L".";
                SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            }
            else
            {
                out.str(L"");
                out << L"On device \"" << m_device << L"\" ioctl(BLKGETSIZE) returned non-zero value = " << StrFrom(ret) << L".";
                SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            }
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(BLKGETSIZE64) returned non-zero value = " << StrFrom(ret) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
        GetDiskGeometry(totalSize, sectorSize, cylinderCnt, headCnt, sectorsPerTrack);
    }

    /*----------------------------------------------------------------------------*/
    /**
       update disk signature by file descriptor
    */
    void StaticPhysicalDiskInstance::UpdateDiskSignature()
    {
        const int     MBR_LEN = 512;   // boot sector size
        unsigned char mbrbuf[MBR_LEN]; // buffer to store boot sector size

        /* try to get disk signature */
        if(MBR_LEN == m_deps->read((void *)mbrbuf, MBR_LEN) )
        {
            /* byte 510 is 0x55 and byte 511 is 0xaa means it is a MBR partition */
            if(mbrbuf[510] == 0x55 && mbrbuf[511] == 0xaa)
            {
                m_isMBR = true;
                // signature store in disk as little endian
                m_Properties.signature = (unsigned int)mbrbuf[0x1B8] |
                                         ((unsigned int)mbrbuf[0x1B9] << 8) |
                                         ((unsigned int)mbrbuf[0x1BA] << 16) |
                                         ((unsigned int)mbrbuf[0x1BB] << 24);
            }
            else
            {
                m_isMBR = false;
                SCX_LOGERROR(m_log, L"get signature error: disk does not use MBR");
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error reading mbr sector, errno=" + StrFrom(errno));
        }

    }

/*----------------------------------------------------------------------------*/
    /**
       check support writing status
    */
    void StaticPhysicalDiskInstance::CheckSupportWriting()
    {
        int ro;                 // indicate if the disk readonly
        if(m_deps->ioctl(int(BLKROGET), &ro) == 0)
        {
            if(!ro)
            {
                m_Properties.capabilities[eDiskCapSupportsWriting]           = eDiskCapSupportsWriting;
            }
        }
    }

/*----------------------------------------------------------------------------*/
    /**
       Issues power mode drive command. This function uses WIN_CHECKPOWERMODE1 or WIN_CHECKPOWERMODE2 ioctl command to
       get the power state of the device. This ioctl issues ATA command "CHECK POWER MODE" that is described in
       "INCITS 452-2008 (D1699): AT Attachment 8 - ATA/ATAPI Command Set" document that can be obtained by contacting
       www.t13.org.
    */
    __u8 StaticPhysicalDiskInstance::DriveCmdATAPowerMode(__u8 modeCmd)
    {
        __u8 args[4]   = {0, 0, 0, 0};
        __u8 powerMode = POWERMODE_UNSET;

        SCXASSERT(modeCmd == WIN_CHECKPOWERMODE1 || modeCmd == WIN_CHECKPOWERMODE2);
        args[0] = modeCmd;

        // Get power state of ata driver.
        if(m_deps->ioctl(int(HDIO_DRIVE_CMD), args) == 0)
        {
           powerMode = args[2];
        }
        else
        {
            if (errno == EIO && args[0] == 0 && args[1] == 0)
            {
                powerMode = POWERMODE_STANDBY;
            }
        }

        return powerMode;

    }

/*----------------------------------------------------------------------------*/
    /**
       check disk power mode
    */
    void StaticPhysicalDiskInstance::CheckATAPowerMode()
    {
        __u8 powerMode = POWERMODE_UNSET;

        // get power state of ata driver.
        if (POWERMODE_UNSET == (powerMode = DriveCmdATAPowerMode(WIN_CHECKPOWERMODE1)))
        {
            SCX_LOGWARNING(m_log, L"ioctl WIN_CHECKPOWERMODE1 failed");
            if (POWERMODE_UNSET == (powerMode = DriveCmdATAPowerMode(WIN_CHECKPOWERMODE2)))
            {
                SCX_LOGWARNING(m_log, L"ioctl WIN_CHECKPOWERMODE2 failed");
            }
        }

        switch(powerMode)
        {
            case POWERMODE_STANDBY:  // Device is in Standby mode.
                m_Properties.availability = eDiskAvaPowerSave_Standby;
                break;
            case POWERMODE_SPINDOWN: // Device is in NV Cache Power Mode and the spindle is spun own or spinning down.
            case POWERMODE_SPINUP:   // Device is in NV Cache Power Mode and the spindle is spun up or spinning up.
                m_Properties.availability = eDiskAvaPowerSave_LowPowerMode;
                break;
            case POWERMODE_IDLE:     // Device is in Idle mode.
            case POWERMODE_ACTIVE:   // Device is in Active mode or Idle mode.
                m_Properties.availability = eDiskAvaRunningOrFullPower;
                break;
            default: 
                m_Properties.availability = eDiskAvaUnknown;
        }

    }

    /**
       (WI-479079) check the power mode for SCSI disks.
    */
    /*******************************************
     * Notes: The reason I didn't use the ioctl command (implemented in StaticPhysicalDiskInstance::CheckATAPowerMode() above)
     * was that I wasn't really sure if the older kernels would have that implemented in the scsi driver (scsi_ioctl.c).
     * It seems though the newer kernels have it included ( www.mjmwired.net/kernel/Documentation/ioctl/hdio.txt )
     * Moreover, using SCSI command provides more information and flexibility.
     */
    void StaticPhysicalDiskInstance::CheckSCSIPowerMode()
    {
        /* Build an SCSI REQUEST SENSE command (SPC-4, Rev. 36e, 24 AUGUST 2012, Section 6.39)
         *                                        www.t10.org/drafts.htm */
        unsigned char inqCmdBlk[6] = {0x03, 0, 0, 0, 0, 0 }; // Operatoin Code 03h.
                                                             // DESC bit(bit 0) is set to zero for fixed format sense data.
        unsigned char *sense_b = NULL; // No need for a buffer for the sense data.
                                       // The sense data are reported in the response.

        unsigned char rsp_buff[252]; // 252 is the length of the fixed format 
                                     // sense data (8 bytes + Additional Sense Data(244 bytes)).  

        unsigned short dxfer_len = sizeof(rsp_buff); // length of the dxferp in bytes
        sg_io_hdr_t   io_hdr;

        inqCmdBlk[4] = (unsigned char)dxfer_len; /* allocation length */

        memset(&io_hdr, 0, sizeof(io_hdr));

        io_hdr.interface_id    = 'S'; // Must be set to S.
        io_hdr.cmd_len         = sizeof(inqCmdBlk);
        io_hdr.mx_sb_len       = 0;
        io_hdr.dxfer_direction = SG_DXFER_FROM_DEV; // e.g. SCSI READ command.
        io_hdr.dxfer_len       = dxfer_len;
        io_hdr.dxferp          = rsp_buff;
        io_hdr.cmdp            = inqCmdBlk;
        io_hdr.sbp             = sense_b;
        io_hdr.timeout         = 30000; /* 30 seconds; Avoiding timeout.*/

        if (m_deps->ioctl(int(SG_IO), &io_hdr) < 0)
        {
            m_Properties.availability = eDiskAvaUnknown;
            return;
        }
        if( !(io_hdr.host_status == 0 && io_hdr.driver_status == 0 && io_hdr.masked_status == GOOD) )
        {// The query didn't succeed. 
            m_Properties.availability = eDiskAvaUnknown;
            return;
        }

        // Find out the power state of the disk.
        unsigned char sense_key = (unsigned char)(rsp_buff[2] & 0x0F); // Get the sense key.

        if ( sense_key == 0 )
        {// No sense data to be reported i.e. completed without errors. 
            m_Properties.availability =  eDiskAvaRunningOrFullPower;
            return;
        }

        unsigned char ASC = rsp_buff[12]; // Additional Sense Data.
        unsigned char ASCQ = rsp_buff[13]; // Additional Sense Data Qualifier.

        // SPC-4(Rev. 36e) section E.2 for ASC and ASCQ codes.
        switch (ASC)
        {
            case 0x04: // NOT READY 
                if ( ASCQ == 0x09 ) // SELF-TEST IN PROGRESS
                    m_Properties.availability = eDiskAvaInTest;
                else if ( ASCQ == 0x12 ) // OFFLINE
                    m_Properties.availability = eDiskAvaOffLine;
                break;

            case 0x0B: // WARNING
                m_Properties.availability = eDiskAvaWarning;
                break;

            case 0x5E:  
                if ( ASCQ == 0x00 ) // LOW POWER CONDITION ON 
                    m_Properties.availability = eDiskAvaPowerSave_LowPowerMode;

                else if ( ASCQ == 0x41 ||
                          ASCQ == 0x42 ) // POWER STATE CHANGE TO ACTIVE OR IDLE
                    m_Properties.availability =  eDiskAvaRunningOrFullPower;

                else if ( ASCQ == 0x43 ) // POWER STATE CHANGE TO STANDBY 
                    m_Properties.availability =  eDiskAvaPowerSave_Standby;
                break;

            default:
                m_Properties.availability = eDiskAvaUnknown;
                break;
        }

    } // End of CheckSCSIPowerMode()

    /*----------------------------------------------------------------------------*/
    /**
       Update the SCSI attributes include SCSIBus, SCSIPort, SCSILogicalUnit, SCSITargetId. Function also sets
       disk type to SCSI.
    */
    void StaticPhysicalDiskInstance::UpdateSCSIAttributes()
    {
        // Apparently linux community does not know the data type returned by SCSI_IOCTL_GET_BUS_NUMBER.
        // We'll use 64 bits just in case. There may be a problem if we run into a machine with different byte ordering
        // but it's still better than overwritting the stack or heap. By the time we run into such a machine the linux
        // community may figure out the data type.
        __u64 busNo = 0;
        if (m_deps->ioctl(SCSI_IOCTL_GET_BUS_NUMBER, &busNo) == 0)
        {
            m_intType = eDiskIfcSCSI;
            m_Properties.SCSIBus = static_cast<unsigned int>(busNo);
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(SCSI_IOCTL_GET_BUS_NUMBER) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOGTRACE(m_log, out.str());
        }

        // This struct exists only in the kernel space. It is usually in the include/scsi/scsi_ioctl.h file inside
        // #ifdef __KERNEL__ block. But that is not guaranteed so it must be replicated in the user space for ioctl to
        // be used.
        typedef struct my_scsi_idlun
        {
            __u32 dev_id;         // One byte for bus, port, lun and target.
            __u32 host_unique_id; // distinguishes adapter cards from same supplier.
        } My_scsi_idlun;
        My_scsi_idlun idLun;
        memset(&idLun, 0, sizeof(idLun));
        if (m_deps->ioctl(SCSI_IOCTL_GET_IDLUN, &idLun) == 0)
        {
            m_intType = eDiskIfcSCSI;
            m_Properties.SCSIPort        = static_cast<unsigned short>((idLun.dev_id >> 16) & 0x00ff);
            m_Properties.SCSILogicalUnit = static_cast<unsigned short>((idLun.dev_id >> 8) & 0x00ff);
            m_Properties.SCSITargetId    = static_cast<unsigned short>(idLun.dev_id & 0x00ff);
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(SCSI_IOCTL_GET_IDLUN) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOGTRACE(m_log, out.str());
        }
    }

/*----------------------------------------------------------------------------*/
/**
   get partitions information from the file /proc/partitions
*/
    void StaticPhysicalDiskInstance::ParsePartitions()
    {
        /* read /proc/partitions to determine the number of the partitions */
        const wchar_t * const proc_part = L"/proc/partitions"; // partitions file path
        int                major     = 0;                  // major number of a partition
        int                minor     = 0;                  // minor number of a partition
        int                blocks    = 0;                  // blocks of a partition
        vector<wstring>  allLines;                       // all lines read from partitions table
        wstring          name;                           // name of the partition

        size_t pos = m_rawDevice.rfind('/') + 1;
        wstring dev_name = m_rawDevice.substr(pos);
        wstring dev_dir = m_rawDevice.substr(0, pos);
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(SCXFilePath(proc_part), allLines, nlfs);
        m_Properties.partitions = 0;
        vector<wstring> partitions;
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); ++it)
        {
            wistringstream is(*it);
            is>>major>>minor>>blocks>>name;
            if(0 == name.find(dev_name) && name != dev_name && isdigit(name[dev_name.size()]) )
            {
                ++m_Properties.partitions;
                partitions.push_back(dev_dir + name);
            }
        }
        allLines.clear();
        SCXFile::ReadAllLines(SCXFilePath(L"/proc/mounts"), allLines, nlfs);
        m_Properties.mediaLoaded = false;
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); ++it)
        {
            wistringstream is(*it);
            is>>name;
            if(find(partitions.begin(), partitions.end(), name) != partitions.end())
            {
                m_Properties.mediaLoaded = true;
                break;
            }
        }
    }
    /**
       SCSI Generic(sg) inquery.
       Get the data retrieved from the SCSI INQUIRY command.

       \param[in] page vital product data page code
       \param[in] evpd if set the evpd bit of the command
       \param[in|out] dxferp INQUIRY data
       \param[in] dxfer_len length of the dxferp in bytes

       \return true if inquery succeeded, false if ioctl fail or error happened.
    */
    bool StaticPhysicalDiskInstance::SqInq(int page, int evpd, void * dxferp, unsigned short dxfer_len)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        // SCSI INQUIRY command has 6 bytes,  OPERATION CODE(12h).
        unsigned char inqCmdBlk[6] = {0x12, 0, 0, 0, 0, 0 };
        unsigned char sense_b[32]; // 32 bytes is enough for test result.
        sg_io_hdr_t   io_hdr;

        if (evpd)
        {
            inqCmdBlk[1] |= 1;      // Enable evpd, at bit 0 byte 1.
        }
        inqCmdBlk[2] = (unsigned char) page;    // Page code in byte 2.
        inqCmdBlk[3] = (unsigned char)((dxfer_len >> 8) & 0xff); // Allocation length, MSB.
        inqCmdBlk[4] = (unsigned char)(dxfer_len & 0xff);        // Allocation, LSB.

        memset(&io_hdr, 0, sizeof(io_hdr));
        memset(sense_b, 0, sizeof(sense_b));

        io_hdr.interface_id    = 'S';
        io_hdr.cmd_len         = sizeof(inqCmdBlk);
        io_hdr.mx_sb_len       = sizeof(sense_b);
        io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
        io_hdr.dxfer_len       = dxfer_len;
        io_hdr.dxferp          = dxferp;
        io_hdr.cmdp            = inqCmdBlk;
        io_hdr.sbp             = sense_b;
        io_hdr.timeout         = 30000; // 30 seconds.

        if (m_deps->ioctl(int(SG_IO), &io_hdr) < 0)
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(SG_IO) trying to access page " << page <<
                L" with evpd " << evpd << "L failed with errno = " << StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            return false;
        }
        if (io_hdr.status == 0 &&io_hdr.host_status == 0 && io_hdr.driver_status == 0)
        {
            return true;
        }
        else
        {
            // Refer to "SCSI Primary Commands - 4" (SPC-4), chapter 4.5.1, available at www.t10.org/members/w_spc4.htm.
            unsigned char sense_key;
            if (sense_b[0] & 0x2) // For response code 72h and 73h, sense key in byte 1.
            {
                sense_key = (unsigned char)(sense_b[1] & 0xf);
            }
            else                 // For response code 72h and 73h, sense key in byte 2.
            {
                sense_key = (unsigned char)(sense_b[2] & 0xf);
            }
            // 1h RECOVERED ERROR: Indicates that the command completed
            // successfully, with some recovery action performed by the
            // device server.
            if (sense_key == 0x01)
            {
                return true;
            }
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(SG_IO) trying to access page " << page <<
                L" with evpd " << evpd << L" succeeded but return status indicated failure: "
                L"status = " << io_hdr.status << L", host_status = " << io_hdr.host_status << L", driver_status = " <<
                io_hdr.driver_status << L", sense_b[0] = " << sense_b[0] << L", sense_b[1] = " << sense_b[1] <<
                L", sense_b[2] = " << sense_b[2] << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            return false;
        }
    }

#endif

/*----------------------------------------------------------------------------*/
/**
   Update the instance.

   \throws      SCXErrnoOpenException when system calls fail.
*/
    void StaticPhysicalDiskInstance::Update()
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        /*
         * In some cases, we must modify the device ID (m_device) that was
         * passed to us by provider (for example, the provider never passes
         * the raw device name, but we need that). Rather than changing the
         * real device, and then potentially running into problems if the
         * device ID doesn't match with the device ID from other sources,
         * we make a copy of the device in m_rawDevice.  Then we can change
         * this, as needed, for various platforms.
         *
         * The m_rawDevice is for internal use only, and never shared via a
         * Get() method.
         */

        if (m_rawDevice.length() == 0)
        {
            m_rawDevice = m_device;
        }

#if defined(linux)

        // Open the device (Note: We must have privileges for this to work).
        if (!m_deps->open(StrToUTF8(m_rawDevice).c_str(), O_RDONLY | O_NONBLOCK))
        {
            throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
        }

        // Clear properties. It is important to clear all the properties here because when we collect data from the
        // hardware we may do so only if property is not set already. So if all of the properties are not cleared here
        // that may prevent the code from resetting the particular property with fresh data from the hardware.
        Clear();

        // For many properties we have two interfaces available: SCSI with SG_ ioctls and ATA with HDIO_ ioctls.
        // Often ATA drives are also available through SCSI interface by using SCSI translation. The approach we use
        // is to try any interface available for particular property and keep the best data we get. Determining the
        // actual device type is not 100% reliable. Often ATA drives are named sdX instead of hdX. Also ATA drives
        // may be exposed through SCSI interface and it seems that emulation flag may be inaccurate regardless of the
        // translation. Considering all that the approach we take is to first the SCSI ioctls and decide it is SCSI drive
        // unless ATA ioctls work as well. In that case we assume it is ATA drive and SCSI was just translation layer.
        // If neither works we check the drive name to determine if it is a virtual drive.

        // Be nice, see if we have SG_IO before using it. We can be nice because it appears that if SG_GET_VERSION_NUM
        // fails, then SG_IO also always fails. If that changes then this if should be removed and functions using
        // SG_IO should be called regardless of SG_GET_VERSION_NUM call.
        int ver = 0;
        if (m_deps->ioctl(SG_GET_VERSION_NUM, &ver) == 0 )
        {
            // We have SCSI ioctl working. However it might also be just ATA translation. We also have to check
            // HDIO_ ioctls. For now we will assume it is SCSI drive.
            m_intType = eDiskIfcSCSI;
            // SG_IO is available only on versions 3.0 and above.
            if(ver >= 30000)
            {
                // We have SG_IO.
                unsigned char rsp_buff[255];
                    
                // Refer to "SCSI Primary Commands - 4" (SPC-4), chapter 6.4.1, available at www.t10.org/members/w_spc4.htm.
                // Do standard INQUIRY, which is page code 0, EVPD 0. SqInq() function uses SG_IO.
                if (SqInq(0, 0, rsp_buff, sizeof (rsp_buff)) == true)
                {
                    // Byte 15:8 of Standard INQUIRY data is "T10 VENDOR IDENTIFICATION", we use it as manufacturer.
                    string manufacturer(reinterpret_cast<const char *>(rsp_buff) + 8, 8);
                    // Trim string to first 0.
                    manufacturer = manufacturer.c_str();
                    // Trim the whitespaces. It is necessary because if property is not available it may not be just.
                    // empty string but a string of 'space' characters.
                    m_manufacturer = StrTrim(StrFromUTF8(manufacturer));

                    // Treat PRODUCT REVISION LEVEL as firmware revision.
                    // PRODUCT REVISION LEVEL has 4 bytes, starting at byte 32.
                    string fwRev(reinterpret_cast<const char *>(rsp_buff) + 32, 4);
                    fwRev = fwRev.c_str();
                    m_Properties.firmwareRevision = StrTrim(StrFromUTF8(fwRev));

                    // Treat PRODUCT IDENTIFICATION as model.
                    // PRODUCT IDENTIFICATION has 16 bytes starting at byte 16 */
                    string model(reinterpret_cast<const char *>(rsp_buff) + 16, 16);
                    model = model.c_str();
                    m_model = StrTrim(StrFromUTF8(model));

                    // Find out if media is removable.
                    // RMB bit 7 byte 1 indicate removable or not.
                    if (filterbit<unsigned char>(rsp_buff[1], 7))
                    {
                        // Removable media other than floppy.
                        m_Properties.mediaType = mediaTypeNames[1];
                        m_Properties.capabilities[eDiskCapSupportsRemovableMedia] = eDiskCapSupportsRemovableMedia;
                    }
                    else
                    {
                        // Fixed hard disk media.
                        m_Properties.mediaType = mediaTypeNames[2];
                    }
                }
                // Refer to "SCSI Primary Commands - 4" (SPC-4), chapter 7.7.1, available at www.t10.org/members/w_spc4.htm.
                // Page code 80h is to get Unit Serial Number, optional. SqInq() function uses SG_IO.
                if (SqInq(0x80, 1, rsp_buff, sizeof(rsp_buff)))
                {
                    string serialNumber(reinterpret_cast<const char *>(rsp_buff) + 4, rsp_buff[3]);
                    // Trim string to first 0.
                    serialNumber = serialNumber.c_str();
                    // Trim the whitespaces. It is necessary because if property is not available it may not be just
                    // empty string but a string of 'space' characters.
                    m_Properties.serialNumber = StrTrim(StrFromUTF8(serialNumber));
                }
                // Set the m_Properties.availability. Since this is our first attempt we do not have to check if this
                // property is also set. Also, this is more advanced way of getting power state so we give it a priority.
                // CheckSCSIPowerMode() function also uses SG_IO.
            }
            CheckSCSIPowerMode();
        }
        // Rest of the SCSI. Also update disk type to SCSI unless it is already set to SCSI in the previous code.
        // We do not put this function inside SG_GET_VERSION_NUM block since SG_GET_VERSION_NUM is the upper SG layer
        // of the SCSI functionality while UpdateSCSIAttributes() uses SCSI_IOCTL_ that is middle layer SCSI. It is
        // unclear if SG_GET_VERSION_NUM can be used to check if SCSI_IOCTL_ is available. To be sure we decouple the two.
        UpdateSCSIAttributes();

        // Now be nice with HDIO_, first check if we have HDIO_ interface. We can be nice because it appears that if
        // HDIO_GET_32BIT fails then other HDIO_ ioctl-s also fail. However if that changes then other HDIO_
        // ioctl-s should be called regardless of HDIO_GET_32BIT. Note, even if HDIO_GET_32BIT succeeds other
        // HDIO_ ioctl-s like HDIO_GET_IDENTITY may still sometimes fail.
        int io32bit = 0;
        if (m_deps->ioctl(HDIO_GET_32BIT, &io32bit) == 0)
        {
            // We have HDIO_ ioctl. It is most likely ATA drive regardless of SCSI test that happened before since SCSI
            // could be only translation layer. It seems that SCSI is not available through HDIO_ ioctls (usually and
            // for now).
            m_intType = eDiskIfcIDE;

            struct hd_driveid hdid;
            memset(&hdid, 0, sizeof(hdid));
            if (m_deps->ioctl(HDIO_GET_IDENTITY, &hdid) == 0)
            {
                string serialNumberHDIO(reinterpret_cast<char*>(hdid.serial_no), sizeof(hdid.serial_no));
                // Trim string to first 0.
                serialNumberHDIO = serialNumberHDIO.c_str();
                // Trim the whitespaces. It is necessary because if property is not available it may not be just
                // empty string but a string of 'space' characters.
                wstring serialNumber = StrTrim(StrFromUTF8(serialNumberHDIO));
                // Check if we have a better serial number. Sometimes one set of ioctl-s gives less info than the other
                // so we will usually go by size when determining what data is better.
                if (serialNumber.size() > m_Properties.serialNumber.size())
                {
                    m_Properties.serialNumber = serialNumber;
                }

                // Try to get firmware revision.
                string fwRevHDIO(reinterpret_cast<char*>(hdid.fw_rev), sizeof(hdid.fw_rev));
                fwRevHDIO = fwRevHDIO.c_str();
                wstring fwRev = StrTrim(StrFromUTF8(fwRevHDIO));
                if (fwRev.size() > m_Properties.firmwareRevision.size())
                {
                    m_Properties.firmwareRevision = fwRev;
                }

                // Try to get model name.
                string modelHDIO(reinterpret_cast<char*>(hdid.model), sizeof(hdid.model));
                modelHDIO = modelHDIO.c_str();
                wstring model = StrTrim(StrFromUTF8(modelHDIO));
                if (model.size() > m_model.size())
                {
                    m_model = model;
                }
                
                // Get the power management capabilities. NOTE: right now we do not get power management caps by
                // using SCSI interface, however if at some point latter we do implement that we will have to
                // decide what data to use, SCSI or IDE. Right now we simply set power management using IDE if data is
                // available from HDIO_GET_IDENTITY.
                // First check if power management is supported.
                // If bit 3 of word 82 is set to one, the Power Management feature set is supported.
                m_Properties.powermanagementSupported = filterbit<unsigned short>(hdid.command_set_1, 3);
                // Collect check power management capabilities.
                if (!m_Properties.powermanagementSupported)
                {
                    m_Properties.powerManagementCapabilities.push_back(eNotSupported);
                }
                else
                {
                    // Word 85 bit 3, 1 = Power Management feature set enabled.
                    if (filterbit<unsigned short>(hdid.cfs_enable_1, 3))
                    {
                        m_Properties.powerManagementCapabilities.push_back(eEnabled);
                    }
                    else
                    {
                        m_Properties.powerManagementCapabilities.push_back(eDisabled);
                    }
                    // If bit 5 of word 86 is set to one, the Power-Up In Standby feature set has been enabled via the
                    // SET FEATURES command.
                    if(filterbit<unsigned short>(hdid.cfs_enable_2, 5))
                    {
                        m_Properties.powerManagementCapabilities.push_back(ePowerSavingModesEnteredAutomatically);
                    }
                }

                // Find out if media is removable. We give priority to SCSI INQUIRE command, so we will update this
                // property only if it was not done already.
                if(m_Properties.mediaType == mediaTypeNames[3])
                {
                    // Media type is "Format is unknown" which means it was not already set by SCSI.
                    if (filterbit<unsigned short>(hdid.config, 7))
                    {
                        m_Properties.mediaType = mediaTypeNames[1];
                        m_Properties.capabilities[eDiskCapSupportsRemovableMedia] = eDiskCapSupportsRemovableMedia;
                    }
                    else
                    {
                        m_Properties.mediaType = mediaTypeNames[2];
                    }
                }
            }
            else
            {
                std::wstringstream out;
                out << L"On device \"" << m_device << L"\" ioctl(HDIO_GET_32BIT) succeeded but "
                        L"ioctl(HDIO_GET_IDENTITY) failed with errno = " <<
                        StrFrom(errno) << L".";
                SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
            }
            // See if availability property is already set. If it is then don't do anything because CheckSCSIPowerMode()
            // has priority so we want to keep it if it succeeded. CheckSCSIPowerMode() has priority because it is done 
            // by using SCSI INQUIRE command which seems to be more advanced. Also, if CheckATAPowerMode() fails it
            // resets the availability to eDiskAvaUnknown so we lose data if it is already set by CheckSCSIPowerMode().
            if (m_Properties.availability == eDiskAvaUnknown)
            {
                // OK, availability is not already set, set it now.
                CheckATAPowerMode();
            }
        }
        // We tried ioctl's. If we still don't have the disk type then try to determine from the disk name.
        // Determining ATA or SCSI from name is unreliable and there is a trend to have all disks named sdX.
        // We try to determine only if drive is virtual.
        if (m_intType == eDiskIfcUnknown)
        {
            if (GetId().substr(0, 3) == L"vxd")
            {
                m_intType = eDiskIfcVirtual;
            }
        }

        // Determine remaining properties.
        if (!m_cdDrive)
        {
            // Not CD or DVD, get geometry.
            DiskSizeAndGeometryFromKernel();
        }
        CheckSupportWriting();
        UpdateDiskSignature();
        ParsePartitions();

        // Not all the code paths detect power management capabilities. For example SCSI or virtual drives are not checked
        // for this property. In the case there are no power management capabilities, powermanagementSupported must contain
        // eNotSupported.
        if (m_Properties.powerManagementCapabilities.empty())
        {
            m_Properties.powerManagementCapabilities.push_back(eNotSupported);
        }

        // Close the handle to the drive.
        if (0 != m_deps->close())
        {
            // Can't imagine that we can't close the fd, but if so, we don't want to miss that.
            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#elif defined(aix)

        int res;

        /*
         * On AIX, we use the ODM interface (Object Data Model).  We query the
         * ODM for various information that we need.  The ODM is populated at
         * boot time with the hardware that the system contains (plus a bunch
         * of other stuff).
         */

        std::wstring id = GetId();
        size_t slashpos;
        if ( (slashpos = id.rfind('/')) != std::wstring::npos )
        {
            id = id.substr(slashpos+1);
        }

        std::wstring criteria = L"name=" + id;
        bool fIsVirtualDisk = false;

        // Get the CuDv object to give us the interface type
        // This also tells us if this is a virtual disk ...
        //
        // If it's a virtual disk, minimal information is avalable, so we do
        // the best we can do and provide reasonable defaults for the rest.

        struct CuDv dvData;
        memset(&dvData, '\0', sizeof(dvData));
        switch (LookupODM(CuDv_CLASS, criteria, &dvData))
        {
        case 0: /* Huh?  Criteria failed???  We're broken ... */
            throw SCXInternalErrorException(StrAppend(L"Invalid ODM (CuDv) criteria: ", criteria), SCXSRCLOCATION);
            break;

        case 1: /* Success case */
        {
            // The interface, found in PdDvLn_Lvalue, is of the form:
            //    <class>/<subclass>/<type>
            // The <subclass> will be a string like "scsi", or "iscsi",
            // or "ide" (perhaps on AIX for x86).

            std::vector<wstring> parts;
            StrTokenize(StrFromUTF8(dvData.PdDvLn_Lvalue), parts, L"/");

            if (0 == StrCompare(parts[1], L"scsi", true)
                || 0 == StrCompare(parts[1], L"iscsi", true))
            {
                m_intType = eDiskIfcSCSI;
            }
            else if (0 == StrCompare(parts[1], L"ide", true))
            {
                m_intType = eDiskIfcIDE;
            }
            else if (0 == StrCompare(parts[1], L"vscsi", true))
            {
                m_intType = eDiskIfcSCSI;
                fIsVirtualDisk = true;
            }
            else if (0 == StrCompare(parts[1], L"vide", true))
            {
                m_intType = eDiskIfcIDE;
                fIsVirtualDisk = true;
            }
            else if (0 == StrCompare(parts[1], L"advm", true))
            {
                m_intType = eDiskIfcVirtual;
                fIsVirtualDisk = true;
            }

            break;
        }

        default:
            /* Ignore all other values (just case -1) */
            break;
        }

        if ( ! fIsVirtualDisk )
        {
            // Get the VPD data, which gives us manufacturer and model

            struct CuVPD vpdData;
            memset(&vpdData, '\0', sizeof(vpdData));
            switch (LookupODM(CuVPD_CLASS, criteria, &vpdData))
            {
            case 0: /* Huh?  Criteria failed???  We're broken ... */
                throw SCXInternalErrorException(L"Invalid ODM (CuVPD) criteria: " + criteria, SCXSRCLOCATION);
                break;

            case 1: /* Success case */
                DecodeVPD(&vpdData);
                break;

            default:
                /* Ignore all other values (just case -1) */
                break;
            }

            // Next get the CuAt object, which gives us the size
            // Note: On some devices, this isn't available ...
            wstring attrCriteria = criteria + L" and attribute=size_in_mb";
            struct CuAt atData;
            memset(&atData, '\0', sizeof(atData));
            if (1 == LookupODM(CuAt_CLASS, attrCriteria, &atData))
            {
                m_sizeInBytes = atol(atData.value);
                m_sizeInBytes *= 1024 * 1024; /* Get from MB to bytes */
            }
        }
        else
        {
            // Note: CuAt is present for virtual disks but doesn't have the size_in_mb attribute.
            //       CuVPD isn't available at all for firtual disks (thus our defaults here)

            m_manufacturer = L"IBM";        // Obviously
            m_model = L"Virtual";           // Reasonabe value (sort of)
            m_sizeInBytes = 0;              // Not able to determinate
        }

#elif defined(hpux)

        /*
         * Open the device (Note: we must have privileges for this to work)
         *
         * Note: We usually get a device like "/dev/disk/disk3".  While we can open
         * this device, the ioctl()'s we call won't work unless we have the raw device
         * open.
         *
         * As a result, we bag the device we received and construct our own given the
         * name of the device.
         */

        if (m_rawDevice == m_device)
        { // Happens first time called
            if (wstring::npos != m_rawDevice.find(L"/dsk/"))
            {
                m_rawDevice = SCXCoreLib::StrAppend(L"/dev/rdsk/", GetId());
            }
            else
            {
                m_rawDevice = SCXCoreLib::StrAppend(L"/dev/rdisk/", GetId());
            }
        }
        if (!m_deps->open(StrToUTF8(m_rawDevice).c_str(), O_RDONLY))
        {
            throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
        }

        /* Look up the disk manufacturer and model */

#if defined(hpux) && (PF_MAJOR==11) && (PF_MINOR<31)
        //
        // Need to use old-style structures for HP-UX 11i v2
        //

        struct inquiry_2 scsiData;
        memset(&scsiData, 0, sizeof(scsiData));
        if (0 == m_deps->ioctl(int(SIOC_INQUIRY), &scsiData))
        {
            /* We got space-filled strings in our structure - null terminate 'em */

            char vendorID[sizeof(scsiData.vendor_id)+1], productID[sizeof(scsiData.product_id)+1];

            memset(vendorID, '\0', sizeof(vendorID));
            memset(productID, '\0', sizeof(productID));

            memcpy(vendorID, scsiData.vendor_id, sizeof(vendorID)-1);
            memcpy(productID, scsiData.product_id, sizeof(productID)-1);

            m_manufacturer = StrTrim(StrFromUTF8(vendorID));
            m_model = StrTrim(StrFromUTF8(productID));
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive inquiry data, errno=" + StrFrom(errno));
        }

        /* Look up capacity, interface type, etc */

        disk_describe_type diskDescr;
        memset(&diskDescr, 0, sizeof(diskDescr));
        if (0 == m_deps->ioctl(int(DIOC_DESCRIBE), &diskDescr))
        {
            /* Set SCSI if we know that's what we've got (20=SCSI) */

            if (20 == diskDescr.intf_type)
            {
                m_intType = eDiskIfcSCSI;
            }

            /* Capacity and sector size */

            m_sizeInBytes = static_cast<scxulong> (diskDescr.maxsva);

            m_sectorSize = diskDescr.lgblksz;
            if (0 == m_sectorSize)
            {
                /* Hmm, didn't get a sector size - let's make a reasonable guess */
                m_sizeInBytes *= 512;
            }
            else
            {
                m_sizeInBytes *= m_sectorSize;
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive description, errno=" + StrFrom(errno));
        }
#else
        inquiry_3_t scsiData;
        memset(&scsiData, 0, sizeof(scsiData));
        if (0 == m_deps->ioctl(int(SIOC_INQUIRY), &scsiData))
        {
            /* We got space-filled strings in our structure - null terminate 'em */

            char vendorID[sizeof(scsiData.vendor_id)+1], productID[sizeof(scsiData.product_id)+1];

            memset(vendorID, '\0', sizeof(vendorID));
            memset(productID, '\0', sizeof(productID));

            memcpy(vendorID, scsiData.vendor_id, sizeof(vendorID)-1);
            memcpy(productID, scsiData.product_id, sizeof(productID)-1);

            m_manufacturer = StrTrim(StrFromUTF8(vendorID));
            m_model = StrTrim(StrFromUTF8(productID));
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive inquiry data, errno=" + StrFrom(errno));
        }

        /* Look up capacity, interface type, etc */

        disk_describe_type_ext_t diskDescr;
        memset(&diskDescr, 0, sizeof(diskDescr));
        if (0 == m_deps->ioctl(int(DIOC_DESCRIBE_EXT), &diskDescr))
        {
            /* Set SCSI if we know that's what we've got (20=SCSI) */

            if (20 == diskDescr.intf_type)
            {
                m_intType = eDiskIfcSCSI;
            }

            /* Capacity and sector size */

            m_sizeInBytes = static_cast<scxulong> (diskDescr.maxsva_high);
            m_sizeInBytes = (m_sizeInBytes << 32) + diskDescr.maxsva_low;

            m_sectorSize = diskDescr.lgblksz;
            if (0 == m_sectorSize)
            {
                /* Hmm, didn't get a sector size - let's make a reasonable guess */
                m_sizeInBytes *= 512;
            }
            else
            {
                m_sizeInBytes *= m_sectorSize;
            }
        }
        else
        {
            SCX_LOGERROR(m_log, L"System error getting drive description, errno=" + StrFrom(errno));
        }
#endif

        /* Close the handle to the drive */

        if (0 != m_deps->close())
        {
            /* Can't imagine that we can't close the device, but if so, we don't want to miss that */

            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#elif defined(sun)

        /*
         * Open the device (Note: We must have privileges for this to work)
         *
         * Note: We usually get a device like "/dev/dsk/c0d0", but this won't
         * work.  We try once (just in case), but if that fails, we build our
         * own path to look like: "/dev/rdsk/c0d0s0".  If that fails too, then
         * we just bag it.
         */

        SCX_LOGHYSTERICAL(m_log, L"Update(): trying disk device " + m_rawDevice );
        if (!m_deps->open((const char *)StrToUTF8(m_rawDevice).c_str(), (int)O_RDONLY))
        {
            /* Reconstruct the path from the name and try again */
            /* Note that we need to check several slices if the disk does not use all of them. */
            for (int i = 0; i<=15; ++i)
            {
                m_rawDevice = L"/dev/rdsk/" + GetId() + StrAppend(L"s", i);
                SCX_LOGHYSTERICAL(m_log, L"Update(): re-trying disk device " + m_rawDevice );
                if (!m_deps->open((const char *)StrToUTF8(m_rawDevice).c_str(), (int)O_RDONLY))
                {
                    if ((EIO != errno && ENXIO != errno) || 15 <= i) // EIO _or_ ENXIO is received if the slice is not used.
                    {
                        throw SCXErrnoOpenException(m_rawDevice, errno, SCXSRCLOCATION);
                    }
                }
                else //the raw file has been opened
                {
                    /*for raw device /dev/rdsk/cntndnsn
                    Where:
                    cn    controller n
                    tn    SCSI target id n (0-6)
                    dn    SCSI LUN n (0-7 normally; some HBAs support LUNs to 15
                               or 32. See the specific manpage for details)
                    referece: http://www.s-gms.ms.edus.si/cgi-bin/man-cgi?sd+7D
                    */
                    size_t pos = GetId().find_first_of('c');
                    if( pos != wstring::npos )
                    {
                        m_Properties.SCSIBus=StrToUInt(GetId().substr(pos+1,1));
                    }
                    pos = GetId().find_first_of('t');
                    if( pos != wstring::npos )
                    {
                        m_Properties.SCSITargetId=StrToUInt(GetId().substr(pos+1,1));
                    }
                    pos = GetId().find_first_of('d');
                    if( pos != wstring::npos )
                    {
                        m_Properties.SCSILogicalUnit=StrToUInt(GetId().substr(pos+1,1));
                    }

                    break;

                }
            }
        }

        SCX_LOGHYSTERICAL(m_log, L"Update(): opened disk device " + m_rawDevice );

        /*
          Look up the size of the disk.

          We also use this call to determine if this is a disk at all.  On Sun,
          we get called for all sorts of devices that may not be hard drives at
          all ...
        */

        struct dk_minfo dkmedia;
        memset(&dkmedia, '\0', sizeof(dkmedia));
        if (0 == m_deps->ioctl(int(DKIOCGMEDIAINFO), &dkmedia))
        {
            m_sizeInBytes = static_cast<scxulong> (dkmedia.dki_capacity) * dkmedia.dki_lbsize;

            // verify that media is supported
            if ( dkmedia.dki_media_type != DK_FIXED_DISK ){
                // close the descriptor - strange, but we don't use helper class
                // with 'close-in-dtor' here
                m_deps->close();
                // this exception means that disk is not of interest (CD, tape, etc)
                throw SCXNotSupportedException(m_rawDevice + L" has unsupported type " + StrFrom(dkmedia.dki_media_type)
                                               , SCXSRCLOCATION);
            }
            else //the device is fixed disk
            {
                //For fixed disk drives, MediaLoaded will always be TRUE
                m_Properties.mediaLoaded=true;

                //"Fixed hard disk media"
                m_Properties.mediaType=mediaTypeNames[2];

            }


        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(DKIOCGMEDIAINFO) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        /* Look up the drive interface type */

        struct dk_cinfo dkinfo;
        memset(&dkinfo, '\0', sizeof(dkinfo));
        if (0 == m_deps->ioctl(int(DKIOCINFO), &dkinfo))
        {
            switch (dkinfo.dki_ctype)
            {
            case DKC_DIRECT:
                m_intType = eDiskIfcIDE;
                break;

            case DKC_SCSI_CCS:
                m_intType = eDiskIfcSCSI;
                break;
            }
        }
        else
        {
            SCX_LOGTRACE(m_log, L"System error getting disk interface information, errno=" + StrFrom(errno));
        }

        /* Look up the drive's sector size */

        struct vtoc dkvtoc;
        memset(&dkvtoc, '\0', sizeof(dkvtoc));
        if (0 == m_deps->ioctl(int(DKIOCGVTOC), &dkvtoc))
        {
            m_sectorSize = dkvtoc.v_sectorsz;

            //m_Properties.partitions=dkvtoc.v_nparts;
            int validPartitionNumber=0;
            for(int i=0;i<dkvtoc.v_nparts;i++)
            {
                if(dkvtoc.v_part[i].p_tag>0)
                {
                    validPartitionNumber++;
                }
            }
            m_Properties.partitions = validPartitionNumber;
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(DKIOCGVTOC) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        /* Look up drive geometry information */

        struct dk_geom dkgeom;
        memset(&dkgeom, '\0', sizeof(dkgeom));
        if (0 == m_deps->ioctl(int(DKIOCGGEOM), &dkgeom))
        {
            m_totalCylinders = dkgeom.dkg_pcyl;
            m_totalHeads = dkgeom.dkg_nhead;
            //on http://wwwcgi.rdg.ac.uk:8081/cgi-bin/cgiwrap/wsi14/poplog/man/7I/dkio,
            //dkg_nsect means /* # of sectors per track*/
            m_Properties.sectorsPerTrack = dkgeom.dkg_nsect;
        }
        else
        {
            std::wstringstream out;
            out << L"On device \"" << m_device << L"\" ioctl(DKIOCGGEOM) failed with errno = " <<
                    StrFrom(errno) << L".";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }

        GetDiskGeometry(m_sizeInBytes, m_sectorSize, m_totalCylinders, m_totalHeads, m_Properties.sectorsPerTrack);

        /* Close the handle to the drive */

        if (0 != m_deps->close())
        {
            /* Can't imagine that we can't close the device, but if so, we don't want to miss that */

            throw SCXErrnoException(L"close", errno, SCXSRCLOCATION);
        }

#else
#error Implementation for Update() method not provided for platform
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       Called each time when 'Update' throws

       input parametr (exception from Update) is ignored
    */
    void StaticPhysicalDiskInstance::SetUnexpectedException( const SCXCoreLib::SCXException& )
    {
        // this function is invoked if Update throws;
        // mark disk as 'off-line'; keep the rest from previous
        m_online = false;
    }

} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
