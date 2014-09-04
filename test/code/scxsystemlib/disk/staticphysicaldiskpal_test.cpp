/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Disk PAL tests for static information on physical disks.

    \date        2008-03-19 12:06:07

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>

#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxtestutils.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/statisticalphysicaldiskenumeration.h>
#include <scxsystemlib/staticphysicaldiskenumeration.h>

// On scxcm-sles11-01 with disk /dev/hda, ioctl(SG_*), ioctl(SCSI_IOCTL_*) and ioctl(HDIO_*) all fail so it's
// impossible to determine the disk type. Machine is hosted on Xen and will be moved shortly to Hyper-V so the
// problem will be fixed. Once move to Hyper-V happens, just remove s_brokenTest.
#if defined(linux)
const static bool s_brokenTest = true;
#endif

using namespace SCXSystemLib;

#include "diskdepend_mock.h"

class SCXStaticPhysicalDiskPalTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStaticPhysicalDiskPalTest );
    CPPUNIT_TEST( TestDumpString );
    CPPUNIT_TEST( TestGetMethods );
    CPPUNIT_TEST( TestSamePhysicalDisksAsStatisticalDisks );
#if defined(hpux)
    CPPUNIT_TEST( TestBug6883_PartialHPUXDiscovery );
#endif
#if defined(linux)
    CPPUNIT_TEST( Test_Bug_12123_Sun_Device_Names_On_Linux );
    CPPUNIT_TEST( Test_WI_479079_SCSI_Availability );
#endif
#if defined (sun)
    CPPUNIT_TEST( Test_Bug_15583_IgnoreUfsCdromForSolaris );
#endif
    CPPUNIT_TEST(TestPhysicalDiskGeometry);
    CPPUNIT_TEST(TestPhysicalDiskVendorSNumber);
    CPPUNIT_TEST(TestPhysicalDiskOpticalDrive);
    SCXUNIT_TEST_ATTRIBUTE(TestDumpString, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestGetMethods, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestSamePhysicalDisksAsStatisticalDisks, SLOW);
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskEnumeration> m_diskEnum;
    std::wstring fauxMntTab;

public:
    SCXStaticPhysicalDiskPalTest()
    {
        fauxMntTab = L"test_mnttab";
    }

    void setUp(void)
    {
        m_diskEnum = 0;
    }

    void tearDown(void)
    {
        //unlink(SCXCoreLib::StrToUTF8(fauxMntTab).c_str());

        if (m_diskEnum != 0)
        {
            m_diskEnum->CleanUp();
            m_diskEnum = 0;
        }
    }

    bool MeetsPrerequisites(std::wstring testName)
    {
#if defined(aix)
        /* No privileges needed on AIX */
        return true;
#elif defined(linux) | defined(hpux) | defined(sun)
        /* Most platforms need privileges to execute Update() method */
        if (0 == geteuid())
        {
            return true;
        }

        std::wstring warnText;

        warnText = L"Platform needs privileges to run " + testName + L" test";

        SCXUNIT_WARNING(warnText);
        return false;
#else
#error Must implement method MeetsPrerequisites for this platform
#endif
    }

    /* This serves to test creation, straight iteration, and DumpString */
    void TestDumpString(void)
    {
        /* This serves to test creation, straight iteration, and DumpString */

        if (!MeetsPrerequisites(L"TestDumpString"))
        {
            return;
        }
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new SCXSystemLib::DiskDependDefault() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticPhysicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = *iter;

            std::wcout << std::endl << di->DumpString();
        }
    }

    void TestGetMethods(void)
    {
        if (!MeetsPrerequisites(L"TestGetMethods"))
        {
            return;
        } 

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new SCXSystemLib::DiskDependDefault() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticPhysicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = *iter;

            bool resDiskName, resDiskDevice, resIntType, resManuf, resModel, resSizeBytes,
                resCylCount, resHeadCount, resSectorCount, resSectorSize;
            std::wstring strDiskName, strDiskDevice, strManuf, strModel;
            scxulong valSizeInBytes, valCylCount, valHeadCount, valSectorCount;
            unsigned int valSectorSize;
            DiskInterfaceType diType;

            bool resMediaLoaded, resPowerManagementSupported, resAvailability, resSCSILogicalUnit;
            bool resSCSITargetId, resPowerManagementCapabilities, resCapabilities, resCapabilityDescriptions;
            bool resFirmwareRevision = false;
            bool resMediaType, resSerialNumber, resPartitions, resSCSIBus, resSectorsPerTrack;
            bool resSignature, resTracksPerCylinder, resTotalTracks;

            bool valMediaLoaded, valPowermanagementSupported;
            unsigned short valAvailability, valSCSILogicalUnit, valSCSITargetId;
            vector<unsigned short> valPowerManagementCapabilities, valCapabilities;
            vector<std::wstring> valCapabilityDescriptions;
            std::wstring valFirmwareRevision, valMediaType, valSerialNumber;
            unsigned int valPartitions, valSCSIBus, valSectorsPerTrack, valSignature;
            scxulong valTracksPerCylinder, valTotalTracks;

            // Do some initialization to pacify Purify
            resDiskName = resDiskDevice = resIntType = resManuf = resModel = resSizeBytes
                = resCylCount = resHeadCount = resSectorCount = resSectorSize = false;
            valSizeInBytes = valCylCount = valHeadCount = valSectorCount = 0;
            valSectorSize = 0;

            resDiskName = di->GetDiskName(strDiskName);
            resDiskDevice = di->GetDiskDevice(strDiskDevice);
            resIntType = di->GetInterfaceType(diType);
            resManuf = di->GetManufacturer(strManuf);
            resModel = di->GetModel(strModel);
            resSizeBytes = di->GetSizeInBytes(valSizeInBytes);
            resCylCount = di->GetTotalCylinders(valCylCount);
            resHeadCount = di->GetTotalHeads(valHeadCount);
            resSectorCount = di->GetTotalSectors(valSectorCount);
            resSectorSize = di->GetSectorSize(valSectorSize);

            resAvailability                = di->GetAvailability(valAvailability);
            resCapabilities                = di->GetCapabilities(valCapabilities);
            resCapabilityDescriptions      = di->GetCapabilityDescriptions(valCapabilityDescriptions);
            resFirmwareRevision            = di->GetFirmwareRevision(valFirmwareRevision);
            resMediaLoaded                 = di->GetMediaLoaded(valMediaLoaded);
            resMediaType                   = di->GetMediaType(valMediaType);
            resPartitions                  = di->GetPartitions(valPartitions);
            resPowerManagementCapabilities = di->GetPowerManagementCapabilities(valPowerManagementCapabilities);
            resPowerManagementSupported    = di->GetPowerManagementSupported(valPowermanagementSupported);
            resSCSIBus                     = di->GetSCSIBus(valSCSIBus);
            resSCSILogicalUnit             = di->GetSCSILogicalUnit(valSCSILogicalUnit);
            resSCSITargetId                = di->GetSCSITargetId(valSCSITargetId);
            resSectorsPerTrack             = di->GetSectorsPerTrack(valSectorsPerTrack);
            resSerialNumber                = di->GetSerialNumber(valSerialNumber);
            resSignature                   = di->GetSignature(valSignature);
            resTracksPerCylinder           = di->GetTracksPerCylinder(valTracksPerCylinder);
            resTotalTracks                 = di->GetTotalTracks(valTotalTracks);

#if defined(linux)

            // WI 12792: Disk unit test failure on new dev host
            //
            // On SLES11, SCSI disks start returning data, at least for Hyper-V.
            // Only verify that it's non-blank for IDE disks (/dev/h*)
            //
            //
            // WI 23115: New infrastructure system for SLES11 (64-bit) not
            // passing unit tests.
            // For this, turns out that IDE disks behave differently for SLES-11
            // 64-bit vs. all other platforms.  Easiest solution is to have the
            // Infra team deploy vxd (virtual Xen disk devices); these behave
            // consistently.
            //
            // The OpsMgr team dictates this to Infra; ConfigMgr does not.  Dont' relay on this for CM.

            bool hdDisk = false;
            // CM team does not require vxd disks; allow the unit tests to pass without them
#if 0
            if (L'h' == strDiskName[0]) {
                hdDisk = true;
            }
#endif

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskName() failed", resDiskName);
            CPPUNIT_ASSERT_MESSAGE("GetDiskName() returned empty", strDiskName.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskDevice() failed", resDiskDevice);
            CPPUNIT_ASSERT_MESSAGE("GetDiskDevice() returned empty", strDiskDevice.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetInterfaceType() failed", resIntType);
            if (!s_brokenTest)
            {
                CPPUNIT_ASSERT_MESSAGE("GetInterfaceType() value wrong", diType != eDiskIfcUnknown);
            }

            CPPUNIT_ASSERT_MESSAGE("Method GetManufacturer() failed", resManuf);
            // Even if supported, the manufacturer still may be returned as empty

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskName() failed", resDiskName);
            CPPUNIT_ASSERT_MESSAGE("GetDiskName() returned empty", strDiskName.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskDevice() failed", resDiskDevice);
            CPPUNIT_ASSERT_MESSAGE("GetDiskDevice() returned empty", strDiskDevice.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetModel() failed", resModel);
            CPPUNIT_ASSERT_MESSAGE("GetModel() value returned empty", strModel.size() != 0 || !hdDisk);

            CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", resSizeBytes);
            CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() returned zero", valSizeInBytes);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalCylinders() failed", resCylCount);
            // Can't always determine cylinder count based on VM, LVM, etc
            //CPPUNIT_ASSERT_MESSAGE("GetTotalCylinders() returned zero", valCylCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalHeads() failed", resHeadCount);
            // Can't always determine head count based on VM, LVM, etc
            //CPPUNIT_ASSERT_MESSAGE("GetTotalHeads() returned zero", valHeadCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalSectors() failed", resSectorCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalSectors() returned zero", valSectorCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetSectorSize() failed", resSectorSize || !hdDisk);
            CPPUNIT_ASSERT_MESSAGE("GetSectorSize() returned zero",
                                   (resSectorSize ? valSectorSize : 0));

            // can't predict it is loaded or not
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaLoaded() failed", resMediaLoaded);

            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementSupported() failed", resPowerManagementSupported);

            if(diType == eDiskIfcSCSI)
            {
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSILogicalUnit() failed", resSCSILogicalUnit);
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSITargetId() failed", resSCSITargetId);
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSIBus() failed", resSCSIBus);
            }

            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementCapabilities() failed", resPowerManagementCapabilities);
            CPPUNIT_ASSERT_MESSAGE("GetPowerManagementCapabilities() returned 0", valPowerManagementCapabilities.size() > 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() returned unexpected value",
                                   valAvailability == eDiskAvaPowerSave_Standby
                                   || valAvailability == eDiskAvaPowerSave_LowPowerMode
                                   || valAvailability == eDiskAvaRunningOrFullPower
                                   || valAvailability == eDiskAvaUnknown );

            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilities() failed", resCapabilities);
            CPPUNIT_ASSERT_MESSAGE("GetCapabilities() returned 0", valCapabilities.size() > 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilityDescriptions() failed", resCapabilityDescriptions);
            CPPUNIT_ASSERT_MESSAGE("GetCapabilityDescriptions() returned 0", valCapabilityDescriptions.size() > 0);
            
            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilities() and GetCapabilityDescriptions() returned different members count", valCapabilities.size() == valCapabilityDescriptions.size());
            for(unsigned i = 0; i < valCapabilities.size(); ++i)
            {
                CPPUNIT_ASSERT_MESSAGE("entry in result of Method GetCapabilities() and GetCapabilityDescriptions() do not same index",
                                       valCapabilityDescriptions[i] == capabilityDescriptions[valCapabilities[i]]);
            }
            // NOTE: We do not test the actual returned value since it is not guaranteed hardware will return it.
            // We only check that GetFirmwareRevision() returned true.
            CPPUNIT_ASSERT_MESSAGE("Method GetFirmwareRevision() failed", resFirmwareRevision);

            CPPUNIT_ASSERT_MESSAGE("Method GetPartitions() failed", resPartitions);

            CPPUNIT_ASSERT_MESSAGE("Method GetSectorsPerTrack() failed", resSectorsPerTrack);
            CPPUNIT_ASSERT_MESSAGE("GetSectorsPerTrack() returned 0", valSectorsPerTrack != 0);

            /* valSignature may be 0 */
            CPPUNIT_ASSERT_MESSAGE("Method GetSignature() failed", resSignature);

            CPPUNIT_ASSERT_MESSAGE("Method GetTracksPerCylinder() vailed", resTracksPerCylinder);
            CPPUNIT_ASSERT_MESSAGE("Method GetTracksPerCylinder() vailed", resTracksPerCylinder);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalTracks() failed", resTotalTracks);
            
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaType() failed", resMediaType);
            CPPUNIT_ASSERT_MESSAGE("GetMediaType() returned 0", !valMediaType.empty());

            CPPUNIT_ASSERT_MESSAGE("Method GetSerialNumber() failed", resSerialNumber);
            // valSerialNumber may be empty (particularly in virtual environments)

#elif defined(aix)

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskName() failed", resDiskName);
            CPPUNIT_ASSERT_MESSAGE("GetDiskName() returned empty", strDiskName.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskDevice() failed", resDiskDevice);
            CPPUNIT_ASSERT_MESSAGE("GetDiskDevice() returned empty", strDiskDevice.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetInterfaceType() failed", resIntType);
            CPPUNIT_ASSERT_MESSAGE("GetInterfactType() value wrong", diType != eDiskIfcUnknown);

            CPPUNIT_ASSERT_MESSAGE("Method GetManufacturer() failed", resManuf);
            CPPUNIT_ASSERT_MESSAGE("GetManufacturer() returned empty", strManuf.size() != 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetModel() failed", resModel);
            CPPUNIT_ASSERT_MESSAGE("GetModel() value wrong", strModel.size() != 0 );

            // On AIX, we sometimes can't figure out the size of a specific disk ... allow value of zero
            CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", resSizeBytes);
            CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() value too small",
                                   valSizeInBytes == 0 || valSizeInBytes >= 20 * 1024 * 1024 /* 20GB */);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalCylinders() succeeded", !resCylCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalCylinders() returned non-zero", valCylCount == 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalHeads() succeeded", !resHeadCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalHeads() returned non-zero", valHeadCount == 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalSectors() succeeded", !resSectorCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalSectors() returned non-zero", valSectorCount == 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetSectorSize() succeeded", !resSectorSize);
            CPPUNIT_ASSERT_MESSAGE("GetSectorSize() returned non-zero", valSectorSize == 0);

            // The following are not yet implemented on the AIX platform
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() is implemented", !resAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilities() is implemented", !resCapabilities);
            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilityDescriptions() is implemented", !resCapabilityDescriptions);
            CPPUNIT_ASSERT_MESSAGE("Method GetFirmwareRevision() is implemented", !resFirmwareRevision);
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaLoaded() is implemented", !resMediaLoaded);
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaType() is implemented", !resMediaType);
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitions() is implemented", !resPartitions);
            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementCapabilities() is implemented", !resPowerManagementCapabilities);
            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementSupported() is implemented", !resPowerManagementSupported);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSIBus() is implemented", !resSCSIBus);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSILogicalUnit() is implemented", !resSCSILogicalUnit);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSITargetId() is implemented", !resSCSITargetId);
            CPPUNIT_ASSERT_MESSAGE("Method GetSectorsPerTrack() is implemented", !resSectorsPerTrack);
            CPPUNIT_ASSERT_MESSAGE("Method GetSerialNumber() is implemented", !resSerialNumber);
            CPPUNIT_ASSERT_MESSAGE("Method GetSignature() is implemented", !resSignature);
            CPPUNIT_ASSERT_MESSAGE("Method GetTracksPerCylinder() is implemented", !resTracksPerCylinder);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalTracks() is implemented", !resTotalTracks);

#elif defined(hpux)

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskName() failed", resDiskName);
            CPPUNIT_ASSERT_MESSAGE("GetDiskName() returned empty", strDiskName.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskDevice() failed", resDiskDevice);
            CPPUNIT_ASSERT_MESSAGE("GetDiskDevice() returned empty", strDiskDevice.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetInterfaceType() failed", resIntType);
            CPPUNIT_ASSERT_MESSAGE("GetInterfactType() value wrong", diType != eDiskIfcUnknown);

            CPPUNIT_ASSERT_MESSAGE("Method GetManufacturer() failed", resManuf);
            CPPUNIT_ASSERT_MESSAGE("GetManufacturer() returned empty", strManuf.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetModel() failed", resModel);
            CPPUNIT_ASSERT_MESSAGE("GetModel() returned empty", strModel.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", resSizeBytes);
            CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() returned zero", valSizeInBytes);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalCylinders() succeeded", !resCylCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalCylinders() returned non-zero", !valCylCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalHeads() succeeded", !resHeadCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalHeads() returned non-zero", !valHeadCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetTotalSectors() succeeded", !resSectorCount);
            CPPUNIT_ASSERT_MESSAGE("GetTotalSectors() returned non-zero", !valSectorCount);

            CPPUNIT_ASSERT_MESSAGE("Method GetSectorSize() failed", resSectorSize);
            CPPUNIT_ASSERT_MESSAGE("GetSectorSize() returned zero", valSectorSize);

            // The following are not yet implemented on the HPUX platform
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() is implemented", !resAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilities() is implemented", !resCapabilities);
            CPPUNIT_ASSERT_MESSAGE("Method GetCapabilityDescriptions() is implemented", !resCapabilityDescriptions);
            CPPUNIT_ASSERT_MESSAGE("Method GetFirmwareRevision() is implemented", !resFirmwareRevision);
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaLoaded() is implemented", !resMediaLoaded);
            CPPUNIT_ASSERT_MESSAGE("Method GetMediaType() is implemented", !resMediaType);
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitions() is implemented", !resPartitions);
            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementCapabilities() is implemented", !resPowerManagementCapabilities);
            CPPUNIT_ASSERT_MESSAGE("Method GetPowerManagementSupported() is implemented", !resPowerManagementSupported);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSIBus() is implemented", !resSCSIBus);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSILogicalUnit() is implemented", !resSCSILogicalUnit);
            CPPUNIT_ASSERT_MESSAGE("Method GetSCSITargetId() is implemented", !resSCSITargetId);
            CPPUNIT_ASSERT_MESSAGE("Method GetSectorsPerTrack() is implemented", !resSectorsPerTrack);
            CPPUNIT_ASSERT_MESSAGE("Method GetSerialNumber() is implemented", !resSerialNumber);
            CPPUNIT_ASSERT_MESSAGE("Method GetSignature() is implemented", !resSignature);
            CPPUNIT_ASSERT_MESSAGE("Method GetTracksPerCylinder() is implemented", !resTracksPerCylinder);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalTracks() is implemented", !resTotalTracks);

#elif defined(sun)

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskName() failed", resDiskName);
            CPPUNIT_ASSERT_MESSAGE("GetDiskName() returned empty", strDiskName.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetDiskDevice() failed", resDiskDevice);
            CPPUNIT_ASSERT_MESSAGE("GetDiskDevice() returned empty", strDiskDevice.size());

            CPPUNIT_ASSERT_MESSAGE("Method GetManufacturer() succeeded", !resManuf);
            CPPUNIT_ASSERT_MESSAGE("GetManufacturer() returned non-empty", strManuf.size() == 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetModel() succeeded", !resModel);
            CPPUNIT_ASSERT_MESSAGE("GetModel() returned non-empty", strModel.size() == 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetInterfaceType() failed", resIntType);
            CPPUNIT_ASSERT_MESSAGE("Method GetSizeInBytes() failed", resSizeBytes);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalCylinders() failed", resCylCount);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalHeads() failed", resHeadCount);
            CPPUNIT_ASSERT_MESSAGE("Method GetTotalSectors() failed", resSectorCount);
            CPPUNIT_ASSERT_MESSAGE("Method GetSectorSize() failed", resSectorSize);

            CPPUNIT_ASSERT_MESSAGE("Method GetMediaLoaded() failed", resMediaLoaded);
            
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitions() failed", resPartitions);

            CPPUNIT_ASSERT_MESSAGE("Method GetSectorsPerTrack() failed", resSectorsPerTrack);
            // Note: Newer T5-2 virtualized systems may not support GetSectorsPerTrack (zero returned)
            //CPPUNIT_ASSERT_MESSAGE("GetSectorsPerTrack() returned 0", valSectorsPerTrack != 0);

            CPPUNIT_ASSERT_MESSAGE("Method GetMediaType() failed", resMediaType);
            CPPUNIT_ASSERT_MESSAGE("GetMediaType() returned 0", !valMediaType.empty());


            // We sometimes fail to get values on Solaris. 
            // Check that if we cannot determine disk type, we don't get any ohther values either.
            if (diType == eDiskIfcUnknown)
            {
                CPPUNIT_ASSERT_MESSAGE("GetSectorSize() returned non-zero for disk of unknown type", valSectorSize == 0);
                CPPUNIT_ASSERT_MESSAGE("GetTotalHeads() returned non-zero for disk of unknown type", valHeadCount == 0);
                CPPUNIT_ASSERT_MESSAGE("GetTotalCylinders() returned non-zero for disk of unknown type", valCylCount == 0);
                CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() returned non-zero for disk of unknown type", valSizeInBytes == 0);
                CPPUNIT_ASSERT_MESSAGE("GetTotalSectors() returned non-zero for disk of unknown type", valSectorCount == 0);
            }
            else if(diType == eDiskIfcSCSI)
            {
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSILogicalUnit() failed", resSCSILogicalUnit);
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSITargetId() failed", resSCSITargetId);
                CPPUNIT_ASSERT_MESSAGE("Method GetSCSIBus() failed", resSCSIBus);
            }
            else
            {
                CPPUNIT_ASSERT_MESSAGE("GetSectorSize() returned zero", valSectorSize);
                CPPUNIT_ASSERT_MESSAGE("GetTotalHeads() returned zero", valHeadCount);
                CPPUNIT_ASSERT_MESSAGE("GetTotalCylinders() returned zero", valCylCount);
                CPPUNIT_ASSERT_MESSAGE("GetSizeInBytes() returned zero", valSizeInBytes);
                CPPUNIT_ASSERT_MESSAGE("GetTotalSectors() returned zero", valSectorCount);
            }
            
#else
#error Must implement tests for values/return types of Get* methods
#endif
        }
    }

    void TestSamePhysicalDisksAsStatisticalDisks()
    {
        if (!MeetsPrerequisites(L"TestSamePhysicalDisksAsStatisticalDisks"))
        {
            return;
        }

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new SCXSystemLib::DiskDependDefault() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        SCXSystemLib::StatisticalPhysicalDiskEnumeration statisticalDisks(deps);
        statisticalDisks.Init();
        statisticalDisks.Update(true);

        CPPUNIT_ASSERT_EQUAL(statisticalDisks.Size(), m_diskEnum->Size());

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StatisticalPhysicalDiskInstance>::EntityIterator iter = statisticalDisks.Begin(); iter != statisticalDisks.End(); iter++)
        {
            std::wstring name;
            SCXCoreLib::SCXHandle<SCXSystemLib::StatisticalPhysicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(di->GetDiskName(name));
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> inst = m_diskEnum->GetInstance(name);
            CPPUNIT_ASSERT(0 != inst);
        }

        statisticalDisks.CleanUp();
    }

#if defined(hpux)
    void TestBug6883_PartialHPUXDiscovery()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        deps->SetOpenErrno("/dev/rdisk/disk3",0); // Set to zero to fake file and ioctl operations.
        deps->SetOpenErrno("/dev/rdisk/disk5",ENXIO);
        deps->SetOpenErrno("/dev/rdisk/disk7",ENXIO);
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        fprintf(fp, 
                "/dev/vg00/lvol3 / vxfs ioerror=nodisable,log,dev=40000003 0 1 1213709666\n"
                "DevFS /dev/deviceFileSystem DevFS defaults,dev=4000000 0 0 1213709709\n"
                "-hosts /net autofs ignore,indirect,nosuid,soft,nobrowse,dev=4000003 0 0 1213709740\n");
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);
        
        SCXCoreLib::SCXHandle<LvmTabTest> lvmtab( new LvmTabTest() );
        lvmtab->AddVG(L"/dev/vg00", L"/dev/disk/disk3", L"/dev/disk/disk5");
        lvmtab->AddVG(L"/dev/vg01", L"/dev/disk/disk7");
        deps->SetLvmTab(lvmtab);
        
        struct pst_diskinfo diskInfo;
        memset(&diskInfo, 0, sizeof(diskInfo));
        diskInfo.psd_dev.psd_minor = 3;
        deps->SetPstDiskInfo(&diskInfo, 1);

        struct stat statData;
        memset(&statData, 0, sizeof(statData));

        deps->SetStat("/dev/disk/disk5", statData);
        deps->SetStat("/dev/disk/disk7", statData);
        statData.st_rdev = 3;        
        deps->SetStat("/dev/disk/disk3", statData);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
    }
#endif

#if defined(linux)
    void Test_Bug_12123_Sun_Device_Names_On_Linux()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        fprintf(fp,
                "/dev/cciss/c0d0p2 / reiserfs rw,acl,user_xattr 0 0\n"
                "/dev/cciss/c0d1p1 /home reiserfs rw,acl,user_xattr 0 0\n");
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);

        deps->SetOpenErrno("/dev/cciss/c0d0", 0);
        deps->SetOpenErrno("/dev/cciss/c0d1", 0);
        deps->SetOpenErrno("/dev/cciss/c", EACCES);

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), m_diskEnum->Size());
    }
#endif

#if defined(linux)
    void Test_WI_479079_SCSI_Availability()
    { 
        bool resAvailability;
        unsigned short valAvailability; 

        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticPhysicalDiskInstance>::EntityIterator iter = m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = *iter;

            deps->m_WI_479079_TestNumber = 0;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaUnknown);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaUnknown);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaUnknown);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaUnknown);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaRunningOrFullPower);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaInTest);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaOffLine);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaWarning);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaPowerSave_LowPowerMode);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaRunningOrFullPower);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaRunningOrFullPower);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaPowerSave_Standby);

            deps->m_WI_479079_TestNumber++;
            di->Update();
            resAvailability = di->GetAvailability(valAvailability);
            CPPUNIT_ASSERT_MESSAGE("Method GetAvailability() failed", resAvailability);
            CPPUNIT_ASSERT_MESSAGE("GetAvailability() returned unvalid", valAvailability == eDiskAvaUnknown);
        }

        deps->m_WI_479079_TestNumber = -1; // To by pass ioctl for anything that calls it after this test.

    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
       Created for Bug #15583 (QFE: CSS: Customer get 'disk full' alerts when
       mounting CD-roms). The problem occurs in the Statistical Logical Disk
       Enumeration (which reads /etc/mnttab).  From the Solaris documentation,
       we know that "the file /etc/mnttab is really a file system that provides
       read-only access to the table of mounted file systems for the current host."
       Thus for Solaris it is not sufficient to decide on the file system format, the
       device path must also be examined.

       \date            12/02/2009
    */
    void Test_Bug_15583_IgnoreUfsCdromForSolaris()
    {
        SCXCoreLib::SCXHandle<DiskDependTest> deps( new DiskDependTest() );

        deps->SetOpenErrno("/dev/dsk/c0t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s0",0); // Set to zero to fake file and ioctl operations.
        deps->SetOpenErrno("/dev/dsk/c0t0d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c0t0d0s7",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s0",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s3",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s7",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s0",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s1",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s2",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s3",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s4",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s5",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s6",0);
        deps->SetOpenErrno("/dev/dsk/c9t1d0s7",0);

        struct stat statData;
        memset(&statData, 0, sizeof(statData));

        deps->SetStat("/dev/dsk/c0t0d0", statData);
        deps->SetStat("/dev/dsk/c0t0d0s0", statData);
        deps->SetStat("/dev/dsk/c0t0d0s0",statData);
        deps->SetStat("/dev/dsk/c0t0d0s1",statData);
        deps->SetStat("/dev/dsk/c0t0d0s2",statData);
        deps->SetStat("/dev/dsk/c0t0d0s4",statData);
        deps->SetStat("/dev/dsk/c0t0d0s5",statData);
        deps->SetStat("/dev/dsk/c0t0d0s6",statData);
        deps->SetStat("/dev/dsk/c0t0d0s7",statData);
        deps->SetStat("/dev/dsk/c9t0d0", statData);
        deps->SetStat("/dev/dsk/c9t0d0s0",statData);
        deps->SetStat("/dev/dsk/c9t0d0s1",statData);
        deps->SetStat("/dev/dsk/c9t0d0s2",statData);
        deps->SetStat("/dev/dsk/c9t0d0s3",statData);
        deps->SetStat("/dev/dsk/c9t0d0s4",statData);
        deps->SetStat("/dev/dsk/c9t0d0s5",statData);
        deps->SetStat("/dev/dsk/c9t0d0s6",statData);
        deps->SetStat("/dev/dsk/c9t0d0s7",statData);
        deps->SetStat("/dev/dsk/c9t1d0s0",statData);
        deps->SetStat("/dev/dsk/c9t1d0s1",statData);
        deps->SetStat("/dev/dsk/c9t1d0s2",statData);
        deps->SetStat("/dev/dsk/c9t1d0s3",statData);
        deps->SetStat("/dev/dsk/c9t1d0s4",statData);
        deps->SetStat("/dev/dsk/c9t1d0s5",statData);
        deps->SetStat("/dev/dsk/c9t1d0s6",statData);
        deps->SetStat("/dev/dsk/c9t1d0s7",statData);

        SCXCoreLib::SelfDeletingFilePath mntTab(fauxMntTab);
        FILE* fp = fopen(SCXCoreLib::StrToUTF8(fauxMntTab).c_str(), "wb");
        CPPUNIT_ASSERT(0 != fp);
        // Note: sample data comes from a Solaris 9 development box with a UFS CD
        // in the drive. The SPARC (not the x86) installation media is an example
        // of a CD-ROM with a UFS file system.
        fprintf(fp,
                "/dev/dsk/c9t0d0s0       /       ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800010  1258671407\n"
                "/proc   /proc   proc    dev=4600000     1258671406\n"
                "mnttab  /etc/mnttab     mntfs   dev=46c0000     1258671406\n"
                "fd      /dev/fd fd      rw,suid,dev=4700000     1258671407\n"
                "swap    /var/run        tmpfs   xattr,dev=1     1258671408\n"
                "swap    /tmp    tmpfs   xattr,dev=2     1258671409\n"
                "/dev/dsk/c9t0d0s7       /export/home    ufs     rw,intr,largefiles,logging,xattr,onerror=panic,suid,dev=800017  1258671409\n"
                "-hosts  /net    autofs  indirect,nosuid,ignore,nobrowse,dev=4880001     1258671410\n"
                "auto_home       /home   autofs  indirect,ignore,nobrowse,dev=4880002    1258671410\n"
                "-xfn    /xfn    autofs  indirect,ignore,dev=4880003     1258671410\n"
                "scxsun14:vold(pid345)   /vol    nfs     ignore,noquota,dev=4840001      1258671413\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s6 /cdrom/sol_10_606_sparc/s6      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0007       1259791871\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s5 /cdrom/sol_10_606_sparc/s5      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0006       1259791871\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s4 /cdrom/sol_10_606_sparc/s4      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0005       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s3 /cdrom/sol_10_606_sparc/s3      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0004       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s2 /cdrom/sol_10_606_sparc/s2      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0003       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s1 /cdrom/sol_10_606_sparc/s1      ufs     ro,intr,largefiles,xattr,onerror=panic,nosuid,dev=16c0002       1259791872\n"
                "/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s0 /cdrom/sol_10_606_sparc/s0      hsfs    maplcase,noglobal,nosuid,ro,rr,traildot,dev=16c0001     1259791873\n");
        fclose(fp);
        deps->SetMountTabPath(fauxMntTab);

        deps->SetOpenErrno("/dev/dsk/c9t0d0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s0", 0);
        deps->SetOpenErrno("/dev/dsk/c9t0d0s7", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s6", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s5", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s4", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s3", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s2", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s1", 0);
        deps->SetOpenErrno("/vol/dev/dsk/c0t0d0/sol_10_606_sparc/s0", 0);

        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::MockSolarisStaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));

        // The DVD/CD-ROMs are not to be reported by the disk provider/pal.
        // To verify this will see that the size returned in the number of
        // entries in the (faux) /etc/mnttab file. In fact, the only two lines
        // that should be reported are for root (i.e. /) and /export/home
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), m_diskEnum->Size());

        // Verify that none of the returned entries start with the string
        // '/vol/dev/dsk/c0t0d0/sol_10_606_sparc' for this case

        SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = *(m_diskEnum->Begin());
        std::wstring expected(L"/dev/dsk/c9t0d0");
        std::wstring actual;
        CPPUNIT_ASSERT(di->GetDiskDevice(actual));

        // Double-check that any device found is NOT a CD-ROM
        std::wstring cdrom(L"/vol/dev/dsk/c0t0d0/sol_10_606_sparc");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Found CD-ROM path in the DiskDevice when it should be absent",
                std::wstring::npos, actual.find(cdrom));

        // Verify that the name of the only device returned
        CPPUNIT_ASSERT_EQUAL(0, expected.compare(actual));
    }

    /*----------------------------------------------------------------------------*/
    /**
        Function verifies disk geometry by using mock operating system PhysicalDiskSimulationDepend
        that provides several mock physical disks, some with correct info and some with incorrect disk
        geometry info.
    */
    void TestPhysicalDiskGeometry()
    {
#if defined(linux) || defined(sun)    
        std::vector<PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults> Tests;
        // Note, our OneTestCase.totalSize is always multiple of 1024 because on Solaris it is passed as two
        // numbers to be multiplied, so we simply divide the size by 1024.
    
        PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults OneTestCase;

        // Test case when total size is not available, the provider should return all 0.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
#if defined(linux)
        OneTestCase.strDiskName = L"/dev/hde1";
        OneTestCase.strDiskDevice = L"/dev/hde";
#elif defined(sun)
        OneTestCase.strDiskName = L"/dev/dsk/c0d100s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d100";
#endif
        OneTestCase.valSizeInBytes = 0;
        OneTestCase.valCylCount = 0;
        OneTestCase.valHeadCount = 0;
        OneTestCase.valSectorCount = 0;
        OneTestCase.valTracksPerCylinder = 0;
        OneTestCase.valTotalTracks = 0;
        OneTestCase.valSectorSize = 0;
        OneTestCase.valSectorsPerTrack = 0;
        // Mock OS internal variables.
        OneTestCase.totalSize = 0;
        OneTestCase.sectorSize = 1024;
        OneTestCase.headCnt = 8;
        OneTestCase.sectPerTrackCnt = 32;
        OneTestCase.cylCnt = 1024;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when sector size is not available, the provider should use default size of 512.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
#if defined(linux)
        OneTestCase.strDiskName = L"/dev/hdf1";
        OneTestCase.strDiskDevice = L"/dev/hdf";
#elif defined(sun)
        OneTestCase.strDiskName = L"/dev/dsk/c0d101s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d101";
#endif
        OneTestCase.valSizeInBytes = 1024*1024*1024;
        OneTestCase.valCylCount = (1024*1024*1024)/(255*63*512);
        OneTestCase.valHeadCount = 255;
        OneTestCase.valSectorCount = (1024*1024*1024)/512;
        OneTestCase.valTracksPerCylinder = 255;
        OneTestCase.valTotalTracks = (1024*1024*1024)/(63*512);
        OneTestCase.valSectorSize = 512;
        OneTestCase.valSectorsPerTrack = 63;
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 0;
        OneTestCase.headCnt = 8;
        OneTestCase.sectPerTrackCnt = 32;
        OneTestCase.cylCnt = 1024;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when geometry is not available, the provider should use default 255 heads - 63 sect/track geometry.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
#if defined(linux)
        OneTestCase.strDiskName = L"/dev/hdg1";
        OneTestCase.strDiskDevice = L"/dev/hdg";
#elif defined(sun)
        OneTestCase.strDiskName = L"/dev/dsk/c0d102s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d102";
#endif
        OneTestCase.valSizeInBytes = 1024*1024*1024;
        OneTestCase.valCylCount = (1024*1024*1024)/(255*63*1024);
        OneTestCase.valHeadCount = 255;
        OneTestCase.valSectorCount = (1024*1024*1024)/1024;
        OneTestCase.valTracksPerCylinder = 255;
        OneTestCase.valTotalTracks = (1024*1024*1024)/(63*1024);
        OneTestCase.valSectorSize = 1024;
        OneTestCase.valSectorsPerTrack = 63;
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 1024;
        OneTestCase.headCnt = 0;
        OneTestCase.sectPerTrackCnt = 0;
        OneTestCase.cylCnt = 0;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when geometry exists but is invalid, the provider should use default
        // 255 heads - 63 sect/track geometry.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
#if defined(linux)
        OneTestCase.strDiskName = L"/dev/hdh1";
        OneTestCase.strDiskDevice = L"/dev/hdh";
#elif defined(sun)
        OneTestCase.strDiskName = L"/dev/dsk/c0d103s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d103";
#endif
        OneTestCase.valSizeInBytes = 1024*1024*1024;
        OneTestCase.valCylCount = (1024*1024*1024)/(255*63*1024);
        OneTestCase.valHeadCount = 255;
        OneTestCase.valSectorCount = (1024*1024*1024)/1024;
        OneTestCase.valTracksPerCylinder = 255;
        OneTestCase.valTotalTracks = (1024*1024*1024)/(63*1024);
        OneTestCase.valSectorSize = 1024;
        OneTestCase.valSectorsPerTrack = 63;
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 1024;
        OneTestCase.headCnt = 7;
        OneTestCase.sectPerTrackCnt = 55;
        OneTestCase.cylCnt = 33;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when geometry is valid, the provider should return exactly the same data.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
#if defined(linux)
        OneTestCase.strDiskName = L"/dev/hdi1";
        OneTestCase.strDiskDevice = L"/dev/hdi";
#elif defined(sun)
        OneTestCase.strDiskName = L"/dev/dsk/c0d104s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d104";
#endif
        // Note we increased total size by 1024*1024, still less than one cyl size so geometry should still be ok.
        OneTestCase.valSizeInBytes = 1024*1024*1025;
        OneTestCase.valCylCount = 256;
        OneTestCase.valHeadCount = 128;
        OneTestCase.valSectorCount = (1024*1024*1025)/1024;
        OneTestCase.valTracksPerCylinder = 128;
        OneTestCase.valTotalTracks = (1024*1024*1025)/(32*1024);
        OneTestCase.valSectorSize = 1024;
        OneTestCase.valSectorsPerTrack = 32;
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1025;
        OneTestCase.sectorSize = 1024;
        OneTestCase.headCnt = 128;
        OneTestCase.sectPerTrackCnt = 32;
        OneTestCase.cylCnt = 256;
        // Add to the test list.
        Tests.push_back(OneTestCase);

#if defined(sun)
        // On Solaris x86 it is very common that kernel is reporting one cylinder less, we expect the provider
        // to correct that and return correct geometry.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/dsk/c0d105s4";
        OneTestCase.strDiskDevice = L"/dev/rdsk/c0d105";
        // Note we increased total size by 1024*1024, still less than one cyl size so geometry should still be ok.
        OneTestCase.valSizeInBytes = 1024*1024*1025;
        OneTestCase.valCylCount = (1024*1024*1025)/(128*32*512);
        OneTestCase.valHeadCount = 128;
        OneTestCase.valSectorCount = (1024*1024*1025)/512;
        OneTestCase.valTracksPerCylinder = 128;
        OneTestCase.valTotalTracks = (1024*1024*1025)/(32*512);
        OneTestCase.valSectorSize = 512;
        OneTestCase.valSectorsPerTrack = 32;
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1025;
        OneTestCase.sectorSize = 512;
        OneTestCase.headCnt = 128;
        OneTestCase.sectPerTrackCnt = 32;
        OneTestCase.cylCnt = 512 - 1; // Missing one cylinder.
        // Add to the test list.
        Tests.push_back(OneTestCase);
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // End of test definitions, now do the tests.

        SCXCoreLib::SCXHandle<PhysicalDiskSimulationDepend> deps( new PhysicalDiskSimulationDepend() );
        deps->SetupMockOS(Tests);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        CPPUNIT_ASSERT_EQUAL(Tests.size(), m_diskEnum->Size());

        size_t i;
        for (i = 0; i < m_diskEnum->Size(); i++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = m_diskEnum->GetInstance(i);
            //std::wcout << std::endl << di->DumpString();// Keep this around for debugging.

            bool resDiskName, resDiskDevice, resSizeBytes, resSectorSize, resCylCount, resHeadCount, resSectorsPerTrack,
                resSectorCount, resTracksPerCylinder, resTotalTracks;
            std::wstring strDiskName, strDiskDevice;
            scxulong valSizeInBytes, valCylCount, valHeadCount, valSectorCount, valTracksPerCylinder, valTotalTracks;
            unsigned int valSectorSize, valSectorsPerTrack;
            
            stringstream str;
            str << "For iteration " << i << " , disk " << SCXCoreLib::StrToUTF8(Tests[i].strDiskName);
            string msg = str.str();

            resDiskName = di->GetDiskName(strDiskName);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskName);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskName),
                SCXCoreLib::StrToUTF8(strDiskName));

            resDiskDevice = di->GetDiskDevice(strDiskDevice);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskDevice);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskDevice),
                SCXCoreLib::StrToUTF8(strDiskDevice));

            resSizeBytes = di->GetSizeInBytes(valSizeInBytes);
            CPPUNIT_ASSERT_MESSAGE(msg, resSizeBytes);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSizeInBytes, valSizeInBytes);

            resSectorSize = di->GetSectorSize(valSectorSize);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorSize);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorSize, valSectorSize);

            resCylCount = di->GetTotalCylinders(valCylCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resCylCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valCylCount, valCylCount);

            resHeadCount = di->GetTotalHeads(valHeadCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resHeadCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valHeadCount, valHeadCount);

            resSectorsPerTrack = di->GetSectorsPerTrack(valSectorsPerTrack);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorsPerTrack);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorsPerTrack, valSectorsPerTrack);

            resSectorCount = di->GetTotalSectors(valSectorCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorCount, valSectorCount);

            resTracksPerCylinder = di->GetTracksPerCylinder(valTracksPerCylinder);
            CPPUNIT_ASSERT_MESSAGE(msg, resTracksPerCylinder);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valTracksPerCylinder, valTracksPerCylinder);

            resTotalTracks = di->GetTotalTracks(valTotalTracks);
            CPPUNIT_ASSERT_MESSAGE(msg, resTotalTracks);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valTotalTracks, valTotalTracks);
        }
#endif// defined(linux) || defined(sun)    
    }
    void TestPhysicalDiskVendorSNumber()
    {
#if defined(linux)
        std::vector<PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults> Tests;
    
        PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults OneTestCase;

        // Test case when no serial number and no manufacturer is available.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/hde1";
        OneTestCase.strDiskDevice = L"/dev/hde";
        // Mock OS internal variables.
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when serial number through HDIO_GET_IDENTITY and no manufacturer is available.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/hdf1";
        OneTestCase.strDiskDevice = L"/dev/hdf";
        OneTestCase.strSerialNumber = L"A1B2C3D4";
        // Mock OS internal variables.
        OneTestCase.ioctl_HDIO_GET_IDENTITY_OK = true;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Test case when serial and manufacturer through SG_IO.
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/hdg1";
        OneTestCase.strDiskDevice = L"/dev/hdg";
        OneTestCase.strSerialNumber = L"E5F6G7H8";
        OneTestCase.strManufacturer = L"MSFT_SCX";
        // Mock OS internal variables.
        OneTestCase.ioctl_SG_IO_OK = true;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // End of test definitions, now do the tests.

        SCXCoreLib::SCXHandle<PhysicalDiskSimulationDepend> deps( new PhysicalDiskSimulationDepend() );
        deps->SetupMockOS(Tests);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        CPPUNIT_ASSERT_EQUAL(Tests.size(), m_diskEnum->Size());

        size_t i;
        for (i = 0; i < m_diskEnum->Size(); i++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = m_diskEnum->GetInstance(i);
            //std::wcout << std::endl << di->DumpString();// Keep this around for debugging.

            bool resDiskName, resDiskDevice, resManufacturer, resSerialNumber;
            std::wstring strDiskName, strDiskDevice, strManufacturer, strSerialNumber;
            
            stringstream str;
            str << "For iteration " << i << " , disk " << SCXCoreLib::StrToUTF8(Tests[i].strDiskName);
            string msg = str.str();

            resDiskName = di->GetDiskName(strDiskName);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskName);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskName),
                SCXCoreLib::StrToUTF8(strDiskName));

            resDiskDevice = di->GetDiskDevice(strDiskDevice);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskDevice);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskDevice),
                SCXCoreLib::StrToUTF8(strDiskDevice));

            resManufacturer = di->GetManufacturer(strManufacturer);
            CPPUNIT_ASSERT_MESSAGE(msg, resManufacturer);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strManufacturer),
                SCXCoreLib::StrToUTF8(strManufacturer));

            resSerialNumber = di->GetSerialNumber(strSerialNumber);
            CPPUNIT_ASSERT_MESSAGE(msg, resSerialNumber);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strSerialNumber),
                SCXCoreLib::StrToUTF8(strSerialNumber));
        }
#endif    
    }
    void TestPhysicalDiskOpticalDrive()
    {
#if defined(linux)
        // Build test case with three drives, one HD, one unmounted CD/DVD drive and one mounted CD/DVD drive.
        // CD/DVDs should return 0 for disk geometry.

        std::vector<PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults> Tests;
        PhysicalDiskSimulationDepend::PhysicalDiskSimulationExpectedResults OneTestCase;

        // Mounted CD/DVD
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/cda";
        OneTestCase.strDiskDevice = L"/dev/cda";
        OneTestCase.valSizeInBytes = 0;
        OneTestCase.valCylCount = 0;
        OneTestCase.valHeadCount = 0;
        OneTestCase.valSectorCount = 0;
        OneTestCase.valTracksPerCylinder = 0;
        OneTestCase.valTotalTracks = 0;
        OneTestCase.valSectorSize = 0;
        OneTestCase.valSectorsPerTrack = 0;
        OneTestCase.strManufacturer = L"CD Co.";
        OneTestCase.strSerialNumber = L"CDF6G7H8";
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 1024;
        OneTestCase.ioctl_SG_IO_OK = true;
        OneTestCase.cdDrive = true;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Unmounted CD/DVD
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/cdb";
        OneTestCase.strDiskDevice = L"/dev/cdb";
        OneTestCase.valSizeInBytes = 0;
        OneTestCase.valCylCount = 0;
        OneTestCase.valHeadCount = 0;
        OneTestCase.valSectorCount = 0;
        OneTestCase.valTracksPerCylinder = 0;
        OneTestCase.valTotalTracks = 0;
        OneTestCase.valSectorSize = 0;
        OneTestCase.valSectorsPerTrack = 0;
        OneTestCase.strManufacturer = L"CD1 Co.";
        OneTestCase.strSerialNumber = L"CD7777H8";
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 1024;
        OneTestCase.ioctl_SG_IO_OK = true;
        OneTestCase.mounted = false;
        OneTestCase.cdDrive = true;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        // Hard Disk
        OneTestCase.Clear();
        // Expected results first and always explicit, never calculated.            
        OneTestCase.strDiskName = L"/dev/hdg1";
        OneTestCase.strDiskDevice = L"/dev/hdg";
        OneTestCase.valSizeInBytes = 1024*1024*1024;
        OneTestCase.valCylCount = (1024*1024*1024)/(255*63*1024);
        OneTestCase.valHeadCount = 255;
        OneTestCase.valSectorCount = (1024*1024*1024)/1024;
        OneTestCase.valTracksPerCylinder = 255;
        OneTestCase.valTotalTracks = (1024*1024*1024)/(63*1024);
        OneTestCase.valSectorSize = 1024;
        OneTestCase.valSectorsPerTrack = 63;
        OneTestCase.strManufacturer = L"Disk Co.";
        OneTestCase.strSerialNumber = L"DSF6G7H8";
        // Mock OS internal variables.
        OneTestCase.totalSize = 1024*1024*1024;
        OneTestCase.sectorSize = 1024;
        OneTestCase.ioctl_SG_IO_OK = true;
        // Add to the test list.
        Tests.push_back(OneTestCase);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // End of test definitions, now do the tests.

        SCXCoreLib::SCXHandle<PhysicalDiskSimulationDependCD> deps( new PhysicalDiskSimulationDependCD() );
        deps->SetupMockOS(Tests);
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticPhysicalDiskEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        CPPUNIT_ASSERT_EQUAL(Tests.size(), m_diskEnum->Size());

        size_t i;
        for (i = 0; i < m_diskEnum->Size(); i++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticPhysicalDiskInstance> di = m_diskEnum->GetInstance(i);
            //std::wcout << std::endl << di->DumpString();// Keep this around for debugging.

            bool resDiskName, resDiskDevice, resSizeBytes, resSectorSize, resCylCount, resHeadCount, resSectorsPerTrack,
                resSectorCount, resTracksPerCylinder, resTotalTracks, resManufacturer, resSerialNumber;
            std::wstring strDiskName, strDiskDevice, strManufacturer, strSerialNumber;
            scxulong valSizeInBytes, valCylCount, valHeadCount, valSectorCount, valTracksPerCylinder, valTotalTracks;
            unsigned int valSectorSize, valSectorsPerTrack;
            
            stringstream str;
            str << "For iteration " << i << " , disk " << SCXCoreLib::StrToUTF8(Tests[i].strDiskName);
            string msg = str.str();

            resDiskName = di->GetDiskName(strDiskName);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskName);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskName),
                SCXCoreLib::StrToUTF8(strDiskName));

            resDiskDevice = di->GetDiskDevice(strDiskDevice);
            CPPUNIT_ASSERT_MESSAGE(msg, resDiskDevice);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strDiskDevice),
                SCXCoreLib::StrToUTF8(strDiskDevice));

            resSizeBytes = di->GetSizeInBytes(valSizeInBytes);
            CPPUNIT_ASSERT_MESSAGE(msg, resSizeBytes);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSizeInBytes, valSizeInBytes);

            resSectorSize = di->GetSectorSize(valSectorSize);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorSize);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorSize, valSectorSize);

            resCylCount = di->GetTotalCylinders(valCylCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resCylCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valCylCount, valCylCount);

            resHeadCount = di->GetTotalHeads(valHeadCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resHeadCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valHeadCount, valHeadCount);

            resSectorsPerTrack = di->GetSectorsPerTrack(valSectorsPerTrack);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorsPerTrack);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorsPerTrack, valSectorsPerTrack);

            resSectorCount = di->GetTotalSectors(valSectorCount);
            CPPUNIT_ASSERT_MESSAGE(msg, resSectorCount);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valSectorCount, valSectorCount);

            resTracksPerCylinder = di->GetTracksPerCylinder(valTracksPerCylinder);
            CPPUNIT_ASSERT_MESSAGE(msg, resTracksPerCylinder);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valTracksPerCylinder, valTracksPerCylinder);

            resTotalTracks = di->GetTotalTracks(valTotalTracks);
            CPPUNIT_ASSERT_MESSAGE(msg, resTotalTracks);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, Tests[i].valTotalTracks, valTotalTracks);

            resManufacturer = di->GetManufacturer(strManufacturer);
            CPPUNIT_ASSERT_MESSAGE(msg, resManufacturer);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strManufacturer),
                SCXCoreLib::StrToUTF8(strManufacturer));

            resSerialNumber = di->GetSerialNumber(strSerialNumber);
            CPPUNIT_ASSERT_MESSAGE(msg, resSerialNumber);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, SCXCoreLib::StrToUTF8(Tests[i].strSerialNumber),
                SCXCoreLib::StrToUTF8(strSerialNumber));
        }
#endif    
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStaticPhysicalDiskPalTest );
