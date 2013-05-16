/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Disk Partition PAL tests for static information on disk partitions.

    \date        2011-09-28 12:06:07

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/diskdepend.h>
#include <scxsystemlib/staticdiskpartitionenumeration.h> 
#include <scxsystemlib/staticlogicaldiskenumeration.h> 
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxtestutils.h>
#include "diskdepend_mock.h"

class TestStaticDiskParDeps : public SCXSystemLib::StaticDiskPartitionInstanceDeps
{
public:
        TestStaticDiskParDeps() { }
        using SCXSystemLib::StaticDiskPartitionInstanceDeps::Run;
        int Run(const std::wstring &command, std::istream &mystdin,
                std::ostream &mystdout, std::ostream &mystderr, unsigned timeout)
        {
            (void)command;
            (void)mystdin;
            (void)mystderr;
            (void)timeout;
            mystdout << "Not a match, yet not an error" << endl;
            return 0;
        }
        //! Destructor
        ~TestStaticDiskParDeps() { }
};

class SCXStaticDiskPartitionPalTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStaticDiskPartitionPalTest );
    CPPUNIT_TEST( TestCreation );
    CPPUNIT_TEST( TestDumpString );
    CPPUNIT_TEST( TestGetMethods );
    CPPUNIT_TEST( TestSimulatedDiskPartitions );
#if defined(sun)
    CPPUNIT_TEST( TestReturnMatch_wi501457 );
#endif
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::StaticDiskPartitionEnumeration> m_diskPartEnum;

public:
    SCXStaticDiskPartitionPalTest()
    {
    }

    void setUp(void)
    {
        m_diskPartEnum = NULL;
    }

    void tearDown(void)
    {
        if (m_diskPartEnum != 0)
        {
            m_diskPartEnum->CleanUp();
            m_diskPartEnum = NULL;
        }
    }

    void TestCreation(void)
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum = new SCXSystemLib::StaticDiskPartitionEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum->Init());
    }


    /* This serves to test straight iteration and dump string */
    void TestDumpString(void)
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum = new SCXSystemLib::StaticDiskPartitionEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum->Init());

        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticDiskPartitionInstance>::EntityIterator iter =
            m_diskPartEnum->Begin(); iter != m_diskPartEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticDiskPartitionInstance> dp = *iter;

            CPPUNIT_ASSERT(0 != dp);
            std::wcout << std::endl << dp->DumpString();
        }

    }

    /* Tests each of the Get() methods and verifies that the results are reasonable */
    /* When this runs on RedHat it will test the RH version of the code. When on Solaris, the Sun version */
    void TestGetMethods()
    {
#if defined(aix) || defined(hpux)
        // Some properties like block size or block count are available only if file system is installed on a
        // partition and partition is mounted. On HPUX and AIX list of logical disks is equivalent to list of
        // mounted partitions. Here we generate the list of mounted partitions.
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> depsLogDisk( new DiskDependTest() );
        SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskEnumeration> m_diskEnum;
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum = new SCXSystemLib::StaticLogicalDiskEnumeration(depsLogDisk));
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Init());
        CPPUNIT_ASSERT_NO_THROW(m_diskEnum->Update(true));
        vector<std::wstring> logicalDisks;
        for (SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticLogicalDiskInstance>::EntityIterator iter =
                m_diskEnum->Begin(); iter != m_diskEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticLogicalDiskInstance> di = *iter;
            CPPUNIT_ASSERT(0 != di);

            bool res;
            std::wstring strVal;
            res = di->GetDeviceID(strVal);
            CPPUNIT_ASSERT_MESSAGE("Method StaticLogicalDiskInstance::GetDeviceID() failed", res);
            CPPUNIT_ASSERT_MESSAGE("StaticLogicalDiskInstance::GetDeviceID() returned empty", strVal.size());
            logicalDisks.push_back(strVal);
        }
        // Counter of mounted partitions. At the end we will confirm that this number will match logical disks count.
        size_t mountedPartCnt = 0;
#endif

        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum = new SCXSystemLib::StaticDiskPartitionEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum->Init());

        SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticDiskPartitionInstance>::EntityIterator iter;
        bool resBool = false;

        for (iter = m_diskPartEnum->Begin(); iter != m_diskPartEnum->End(); iter++)
        {
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticDiskPartitionInstance> dp = *iter;
            CPPUNIT_ASSERT(0 != dp);

            CPPUNIT_ASSERT_NO_THROW(dp->Update());

            scxulong blockSz, partIdx, partitionSz, startOffs, numBlks;
            bool bootPflag;
            std::wstring diskDrive; 

            resBool = dp->GetDeviceId(diskDrive);
            CPPUNIT_ASSERT_MESSAGE("Method GetDeviceId() failed", resBool);
            CPPUNIT_ASSERT_MESSAGE("GetDeviceId() returned empty", diskDrive.size());

#if defined(aix) || defined(hpux)
            bool mounted = false;
            std::wstring logicalDiskDrive = diskDrive;
#if defined(aix)
            logicalDiskDrive = L"/dev/" + logicalDiskDrive;
#endif
            if (std::find(logicalDisks.begin(), logicalDisks.end(), logicalDiskDrive) != logicalDisks.end())
            {
                // Partition device name is found in the list of logical disks. That means that this partition is
                // mounted.
                mounted = true;
                mountedPartCnt++;
            }
#else
            bool mounted = true;
#endif
            resBool = dp->GetPartitionBlockSize(blockSz);
            CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() failed, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.', resBool);
            if (mounted)
            {
                CPPUNIT_ASSERT_MESSAGE("GetBlockSize() returned zero, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                    0 != blockSz);

                /* Block size is generally power of 2 >= 512 and <= 8192 */
                CPPUNIT_ASSERT_MESSAGE("Method GetBlockSize() invalid value, " + SCXCoreLib::StrToMultibyte(diskDrive) +
                    '.',
                    blockSz == 512 || blockSz == 1024 || blockSz == 2048 ||
                    blockSz == 4096 || blockSz == 8192 ||
                    blockSz == 65536 /* Oddball case for HP-UX pa-risc /stand file system */|| 
                    blockSz == 131072 /* this one was found on SUN/zfs */ );
            }
            else
            {
                // Partition is not mounted. Value must be 0.
                CPPUNIT_ASSERT_MESSAGE("GetBlockSize() did not return 0, " + SCXCoreLib::StrToMultibyte(diskDrive) +
                    '.', 0 == blockSz);
            }

            resBool = dp->GetNumberOfBlocks(numBlks);
            CPPUNIT_ASSERT_MESSAGE("Method GetNumberOfBlocks() failed, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                resBool);
            if (mounted)
            {
                CPPUNIT_ASSERT_MESSAGE("GetNumberOfBlocks() returned invalid value, " +
                    SCXCoreLib::StrToMultibyte(diskDrive) + '.', 0 != numBlks);
            }
            else
            {
                // Partition is not mounted. Value must be 0.
                CPPUNIT_ASSERT_MESSAGE("GetNumberOfBlocks() did not return 0, " +
                    SCXCoreLib::StrToMultibyte(diskDrive) + '.', 0 == numBlks);
            }
            
            resBool = dp->GetBootPartition(bootPflag);
            CPPUNIT_ASSERT_MESSAGE("Method GetBootPartition() failed, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                resBool);

            resBool = dp->GetIndex(partIdx);
            CPPUNIT_ASSERT_MESSAGE("Method GetIndex() failed", resBool);
            CPPUNIT_ASSERT_MESSAGE("GetIndex() returned invalid value, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                32 > partIdx);

            resBool = dp->GetPartitionSizeInBytes(partitionSz);
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitionSizeInBytes() failed, " + SCXCoreLib::StrToMultibyte(diskDrive) +
                '.', resBool);
            CPPUNIT_ASSERT_MESSAGE("GetPartitionSizeInBytes() returned zero, " + SCXCoreLib::StrToMultibyte(diskDrive) +
                '.', 0 != partitionSz);

            // TODO: Check this is a 0 offset valid??
            resBool = dp->GetStartingOffset(startOffs);
            CPPUNIT_ASSERT_MESSAGE("Method GetStartingOffset() failed, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                resBool);
// Notice that for solaris versions 9 and 10 we don't do the offset value test since it can be either zero or non zero.
#if defined(aix) || defined(hpux) || (defined(sun) && PF_MAJOR == 5 && PF_MINOR >= 11)
            // There's no offset in AIX, HPUX or Solaris 11 ZFS world. Partition ("logical volume") is spread over
            // multiple disks. In this case GetStartingOffset() succeeds and returns 0.
            CPPUNIT_ASSERT_MESSAGE("GetStartingOffset() did not return zero on AIX, HPUX or Solaris 11 ZFS, " +
                SCXCoreLib::StrToMultibyte(diskDrive) + '.', 0 == startOffs);
#elif defined(linux)
            CPPUNIT_ASSERT_MESSAGE("GetStartingOffset() returned zero, " + SCXCoreLib::StrToMultibyte(diskDrive) + '.',
                0 != startOffs);
#endif
            dp->CleanUp();

        } // EndFor
#if defined(aix) || defined(hpux)
        // Verify that number of mounted partitions and number of logical disks match.
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
                "For AIX and HPUX number of logical disks and number of mounted partitions should be the same.",
                m_diskEnum->Size(), mountedPartCnt);
#endif
    }

#if defined(aix) || defined(hpux) || defined(sun)
    void OneDiskPartitionTest(SCXSystemLib::EntityEnumeration<SCXSystemLib::
            StaticDiskPartitionInstance>::EntityIterator &iter,
            const char* diskDriveExpected, scxulong blockSzExpected, scxulong numBlksExpected,
            bool bootPflagExpected, scxulong partIdxExpected, scxulong partitionSzExpected)
    {
            CPPUNIT_ASSERT(iter != m_diskPartEnum->End());

            bool resBool = false;
            SCXCoreLib::SCXHandle<SCXSystemLib::StaticDiskPartitionInstance> dp = *iter;
            CPPUNIT_ASSERT(0 != dp);
            CPPUNIT_ASSERT_NO_THROW(dp->Update());

            scxulong blockSz, partIdx, partitionSz, startOffs, numBlks;
            bool bootPflag;
            std::wstring diskDrive;

            resBool = dp->GetDeviceId(diskDrive);
            CPPUNIT_ASSERT_MESSAGE("Method GetDeviceId() failed", resBool);
            CPPUNIT_ASSERT(SCXCoreLib::StrFromMultibyte(string(diskDriveExpected)) == diskDrive);

            resBool = dp->GetPartitionBlockSize(blockSz);
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitionBlockSize() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(blockSzExpected, blockSz);

            resBool = dp->GetNumberOfBlocks(numBlks);
            CPPUNIT_ASSERT_MESSAGE("Method GetNumberOfBlocks() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(numBlksExpected, numBlks);

            resBool = dp->GetBootPartition(bootPflag);
            CPPUNIT_ASSERT_MESSAGE("Method GetBootPartition() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(bootPflagExpected, bootPflag);

            resBool = dp->GetIndex(partIdx);
            CPPUNIT_ASSERT_MESSAGE("Method GetIndex() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(partIdxExpected, partIdx);

            resBool = dp->GetPartitionSizeInBytes(partitionSz);
            CPPUNIT_ASSERT_MESSAGE("Method GetPartitionSizeInBytes() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(partitionSzExpected, partitionSz);

            resBool = dp->GetStartingOffset(startOffs);
            CPPUNIT_ASSERT_MESSAGE("Method GetStartingOffset() failed", resBool);
            CPPUNIT_ASSERT_EQUAL(scxulong(0), startOffs);

            dp->CleanUp();

            iter++;
    }

    void OneDiskPartitionSystemTest()
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::DiskDepend> deps( new DiskPartLogVolDiskDependTest() );
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum = new SCXSystemLib::StaticDiskPartitionEnumeration(deps));
        CPPUNIT_ASSERT_NO_THROW(m_diskPartEnum->Init());
        CPPUNIT_ASSERT_EQUAL(m_diskPartEnum->Size(), static_cast<size_t>(partition_cnt));
        SCXSystemLib::EntityEnumeration<SCXSystemLib::StaticDiskPartitionInstance>::EntityIterator iter;

        // First partition.
        iter = m_diskPartEnum->Begin();
        OneDiskPartitionTest(iter, partition0_name, mountPoint0_frsize, mountPoint0_blocks, partition0_boot, 0,
            mountPoint0_frsize*mountPoint0_blocks);
        // Second partition.
        OneDiskPartitionTest(iter, partition1_name, mountPoint1_frsize, mountPoint1_blocks, partition1_boot, 1,
            mountPoint1_frsize*mountPoint1_blocks);
#if defined(hpux) || defined(aix)
        // Third partition. Unmounted boot partition.
        OneDiskPartitionTest(iter, partition2_name, 0, 0, partition2_boot, 2, partition2_size);
#endif
#if defined(hpux)
        // Fourth partition. Trying to confuse providers boot logic.
        OneDiskPartitionTest(iter, partition3_name, 0, 0, partition3_boot, 3, partition3_size);
#endif
        // End.
        CPPUNIT_ASSERT(iter == m_diskPartEnum->End());
    }
#endif// defined(aix) || defined(hpux) || defined(sun)

    // Simulates mock hardware with several devices and several mount points and checks if, as a result of
    // a call to enumeration method, exactly same hardware is returned.
    void TestSimulatedDiskPartitions()
    {
#if defined(aix) || defined(sun)
        OneDiskPartitionSystemTest();
#elif defined(hpux)
        // For hpux we have 2 separate cases. In one case boot and root are separate logical volumes and
        // in the other case they are the same logical volume.
        bootRootShareLV = false;
        OneDiskPartitionSystemTest();
        bootRootShareLV = true;
        OneDiskPartitionSystemTest();
#endif
    }

#if defined(sun)
    void TestReturnMatch_wi501457 ()
    {
        bool rtn(true);
        SCXCoreLib::SCXHandle<TestStaticDiskParDeps> deps = SCXCoreLib::SCXHandle<TestStaticDiskParDeps>(new TestStaticDiskParDeps());
        SCXSystemLib::StaticDiskPartitionInstance sdpInst(deps);
        wstring bootpathStr(L"");
        rtn = sdpInst.GetBootDrivePath(bootpathStr);
        CPPUNIT_ASSERT(!rtn);
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStaticDiskPartitionPalTest );
