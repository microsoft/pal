/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Test the logic in the LVM utility methods.

    \date        2010-12-16 02:46:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <map>

#if defined(linux) &&                                   \
    ! ( defined(PF_DISTRO_SUSE)   && (PF_MAJOR<=9) ) && \
    ! ( defined(PF_DISTRO_REDHAT) && (PF_MAJOR<=4) )

#include <scxsystemlib/scxlvmutils.h>

#include <testutils/scxunit.h>


class SCXLVMUtilsTest : public CPPUNIT_NS::TestFixture
{

    CPPUNIT_TEST_SUITE( SCXLVMUtilsTest );

    CPPUNIT_TEST( CanDetectDeviceMapperDevices );

    // The majority of the LVM unit test is ignored on RHEL4 and SLES9
    CPPUNIT_TEST( GetDMDevice_Returns_Empty_String_When_Input_Not_LVM );
    CPPUNIT_TEST( GetDMDevice_Throws_When_Input_FileNotFound );
    CPPUNIT_TEST( GetDMDevice_Throws_When_Stat_NOT_isMatch );
    CPPUNIT_TEST( GetDMDevice_Returns_dev_dm_When_Stat_isMatch );
    CPPUNIT_TEST( GetDMDevice_Throws_When_SysBlockDmDev_FileNotFound );
    CPPUNIT_TEST( GetDMDevice_Throws_When_SysBlockDmDev_Empty );
    CPPUNIT_TEST( GetDMDevice_Throws_When_SysBlockDmDev_NotMajorColonMinor );
    CPPUNIT_TEST( GetDMDevice_Throws_When_SysBlockDmDev_WrongMajorColonMinor );
    CPPUNIT_TEST( GetDMDevice_Returns_dm_When_SysBlockDmDev_isMatch );
    CPPUNIT_TEST( GetDMDevice_Returns_dm_When_SysBlockDmDev_isMatch_With_Extra_lines_Ignored );
    CPPUNIT_TEST( GetDMSlaves_Throws_FileNotFound_When_Input_Device_Has_No_SlavesPath );
    CPPUNIT_TEST( GetDMSlaves_Throws_When_Input_Device_Has_No_SlaveEntries );
    CPPUNIT_TEST( GetDMSlaves_Ignores_Invalid_SlaveEntries );
    CPPUNIT_TEST( GetDMSlaves_Throws_Up );
    CPPUNIT_TEST( GetDMSlaves_Works );
    CPPUNIT_TEST( GetDMSlaves_SlavesWithDMEntries_TraversesToDevice );
    CPPUNIT_TEST( GetDMSlaves_SlavesWithCircularLinks_Throws );

    CPPUNIT_TEST_SUITE_END();

private:
    /**
       This class implements SCXLVMUtilsDepends by providing none of the required
       functionality/
     */
    class SCXLVMUtilsDependsNotImplemented
        : public SCXSystemLib::SCXLVMUtilsDepends
    {
    public:
        virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
            const SCXCoreLib::SCXFilePath               & path,
            const SCXCoreLib::SCXDirectorySearchOptions  options)
        {
            // warnings as errors, so deal with the unused params
            (void) path;
            (void) options;

            std::vector< SCXCoreLib::SCXFilePath > empty;

            CPPUNIT_FAIL("The SCXLVMUtils external dependency GetFileSystemEntries is not implemented for this test.");

            return (empty);
        }


        virtual void Stat(
            const SCXCoreLib::SCXFilePath            & path,
            SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
        {
            // warnings as errors, so deal with the unused params
            (void) path;
            (void) pStat;

            CPPUNIT_FAIL("The SCXLVMUtils external dependency Stat is not implemented for this test.");
        }


        virtual void ReadAllLinesAsUTF8(
            const SCXCoreLib::SCXFilePath & source,
            std::vector< std::wstring >   & lines,
            SCXCoreLib::SCXStream::NLFs   & nlfs)
        {
            // warnings as errors, so deal with the unused params
            (void) source;
            (void) lines;
            (void) nlfs;

            CPPUNIT_FAIL("The SCXLVMUtils external dependency ReadAllLinesAsUTF8 is not implemented for this test.");
        }


        virtual ~SCXLVMUtilsDependsNotImplemented() { }


    protected:
        /**
           This method throws SCXFilePathNotFoundException for the given path,
           all other parameters are ignored without generating a warning.
         */
        static void ThrowFileNotFound(const SCXCoreLib::SCXFilePath & path)
        {
            throw SCXCoreLib::SCXFilePathNotFoundException(path.Get(), SCXSRCLOCATION);
        }


        /**
           Fill in the parts of the given stat structure in whatever way is
           necessary to imitate a device with the given major/minor ID.
         */
        static void SetupDevice(SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat, int major, int minor)
        {
            pStat->st_rdev = makedev(major, minor);
        }


        /**
           This class is useful when testing SCXLVMUtils::GetDMDevice.  The
           class is used to force the test to follow the code path for systems
           that do not provide the /dev/dm-<minor> device paths.

           This is done by manipulating the result of calls to Stat.  In the
           first call to Stat the stat structure for a device-mapper device
           with a given minor ID is setup.  The second call is assumed to be a
           Stat of /dev/dm-<minor>, which on systems without /dev/dm-<minor>
           device paths the result is a FileNotFoundException.

           \param[in]      path   path to file/directory to perform stat on.
           \param[out]     pStat  Pointer to a stat structure that should be filled.
           \param[in/out]  state  used to determine if this is an even or odd numbered call.
         */
        class StatHelper_DeviceMapperNoDevDM
        {
        public:
            int m_count;
            int m_major;
            int m_minor;


            /**
               Initialize the class with the given device values.

               The default values are for a device-mapper device (253) with the
               minor device ID of 2.

               \param[in]  minor  The minor device ID to use when setting up a device.
               \param[in]  major  The major device ID to use when setting up a device.
             */
            StatHelper_DeviceMapperNoDevDM(int minor = 2, int major = 253)
                : m_count(0),
                  m_major(major),
                  m_minor(minor)
            {
            }


            /**
               This implementation of Stat will return a good device on the first
               call and throw on the second.

               \param[in]      path   path to file/directory to perform stat on.
               \param[out]     pStat  Pointer to a stat structure that should be filled.
             */
            void Stat(const SCXCoreLib::SCXFilePath & path, SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                if ((m_count++ % 2) == 0)
                {
                    // even calls (0, 2, 4, etc...) get a device setup
                    SetupDevice(pStat, m_major, m_minor);
                }
                else
                {
                    // for odd calls, the behavior is for an invalid path
                    ThrowFileNotFound(path.Get());
                }
            }
        };
    };


public:
    void setUp(void)
    {
    }


    void tearDown(void)
    {
    }

    void CanDetectDeviceMapperDevices()
    {
        // This test validates that the system can differentiate between
        // device-mapper (dm) device (i.e. LVM) and other devices.  Note
        // that the device /dev/mapper/control is not a dm device, but is
        // in fact the device that implements device-mapper, and should
        // not be counted as a dm-device.
        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsNotImplemented());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);


        // First a few that should return false always.
        CPPUNIT_ASSERT(! lvmUtils.IsDMDevice(L"/dev/hdb2"));
        CPPUNIT_ASSERT(! lvmUtils.IsDMDevice(L"/dev/dm-0"));
        CPPUNIT_ASSERT(! lvmUtils.IsDMDevice(L"/dev/mapper/control"));

        // The following are valid dm device paths (i.e. LVM) and should always
        // return true.
        CPPUNIT_ASSERT(lvmUtils.IsDMDevice(L"/dev/mapper/with-dash"));
        CPPUNIT_ASSERT(lvmUtils.IsDMDevice(L"/dev/mapper/without"));
    }


    void GetDMDevice_Returns_Empty_String_When_Input_Not_LVM()
    {
        // This tests the first condition in GetDMDevice.  Since none of these
        // paths are to an LVM device, each call should result in an immediate
        // return with an empty string.

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsNotImplemented());
        SCXSystemLib::SCXLVMUtils lvmUtils(extDepends);

        CPPUNIT_ASSERT(lvmUtils.GetDMDevice(L"/dev/hda").empty());
        CPPUNIT_ASSERT(lvmUtils.GetDMDevice(L"/dev/hdb2").empty());
        // GetDMDevice() always immediately returns dm-* devices
        CPPUNIT_ASSERT(!lvmUtils.GetDMDevice(L"/dev/dm-0").empty());
        CPPUNIT_ASSERT(lvmUtils.GetDMDevice(L"/dev/mapper").empty());
        CPPUNIT_ASSERT(lvmUtils.GetDMDevice(L"/proc").empty());
    }


    void GetDMDevice_Throws_When_Input_FileNotFound()
    {
        // This test validates that an exception is thrown when the input path
        // is a valid LVM device path string, but there is no device with that
        // path on the system.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception.
         */
        class SCXLVMUtilsDependsStatThrowsFileNotFound
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                // warnings as errors, so deal with the unused params
                (void) pStat;

                ThrowFileNotFound(path);
            }


            virtual ~SCXLVMUtilsDependsStatThrowsFileNotFound() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsStatThrowsFileNotFound());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar"), SCXCoreLib::SCXFilePathNotFoundException);
    }


    void GetDMDevice_Throws_When_Stat_NOT_isMatch()
    {
        // This test validates that an exception is thrown when the stat info
        // of the dm device doesn't match the stat info of the LVM device.

        /**
           This class implements SCXLVMUtilsDepends::Stat that sets pStat to
           represent a misc device on even calls and a device-mapper device on
           odd calls.
         */
        class SCXLVMUtilsDependsStatMisMatch
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            int m_count;


            SCXLVMUtilsDependsStatMisMatch() : m_count(0) { }


            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                // warnings as errors, so deal with the unused params
                (void) path;

                if ((m_count++ % 2) == 0)
                {
                    SetupDevice(pStat, 10, 2);
                }
                else
                {
                    SetupDevice(pStat, 253, 2);
                }
            }


            virtual ~SCXLVMUtilsDependsStatMisMatch() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsStatMisMatch());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/not-a-dm-device"), SCXSystemLib::SCXBadLVMDeviceException);
    }


    void GetDMDevice_Returns_dev_dm_When_Stat_isMatch()
    {
        // For all input device paths in /dev/mapper, if the stat of the input
        // matches the stat of /dev/dm-<minor>, where <minor> is the minor device
        // id of the input device, then the result is /dev/dm-<minor>.  In this
        // test, <minor> is always 2.

        /**
           This class implements SCXLVMUtilsDepends::Stat that sets pStat to
           always represent device-mapper minor device 2.
         */
        class SCXLVMUtilsDependsStatDevMapper2
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                // warnings as errors, so deal with the unused params
                (void) path;

                SetupDevice(pStat, 253, 2);
            }


            virtual ~SCXLVMUtilsDependsStatDevMapper2() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsStatDevMapper2());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_EQUAL(0, lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar").compare(L"/dev/dm-2"));
    }


    void GetDMDevice_Throws_When_SysBlockDmDev_FileNotFound()
    {
        // This test validates that an exception is thrown when the system
        // does not provide /dev/dm-<minor> helper devices, and the file
        // at /sys/block/dm-<minor>/dev can't be found.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will throw on all calls.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ThrowsFileNotFound
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) lines;
                (void) nlfs;

                ThrowFileNotFound(source);
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ThrowsFileNotFound() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ThrowsFileNotFound());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar"), SCXCoreLib::SCXFilePathNotFoundException);
    }


    void GetDMDevice_Throws_When_SysBlockDmDev_Empty()
    {
        // This test validates that an exception is thrown when the system
        // does not provide /dev/dm-<minor> helper devices, and the file
        // at /sys/block/dm-<minor>/dev has 0 lines.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will return an empty vector
           of lines.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsEmpty
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev is empty
                lines.clear();
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsEmpty() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsEmpty());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar"), SCXSystemLib::SCXBadLVMDeviceException);
    }


    void GetDMDevice_Throws_When_SysBlockDmDev_NotMajorColonMinor()
    {
        // This test validates that an exception is thrown when the system
        // does not provide /dev/dm-<minor> helper devices, and the file
        // at /sys/block/dm-<minor>/dev is not in the <major> ':' <minor>
        // format.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will return a line vector that
           is not in the expected format.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongFormat
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has information that is not
                // in the format <major> ':' <minor>
                lines.clear();
                lines.push_back(std::wstring(L"information that is not in the format <major> ':' <minor>"));
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongFormat() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongFormat());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar"), SCXSystemLib::SCXBadLVMDeviceException);
    }


    void GetDMDevice_Throws_When_SysBlockDmDev_WrongMajorColonMinor()
    {
        // This test validates that an exception is thrown when the system
        // does not provide /dev/dm-<minor> helper devices, and the file
        // at /sys/block/dm-<minor>/dev has info in the <major> ':' <minor>
        // format, but with the wrong values for <major> and/or <minor>.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will return a properly formated
           line vector, but the content is not a match.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongMajorMinor
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has information that is not "253:2"
                lines.clear();
                lines.push_back(std::wstring(L"253:0"));
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongMajorMinor() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsWrongMajorMinor());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar"), SCXSystemLib::SCXBadLVMDeviceException);
    }


    void GetDMDevice_Returns_dm_When_SysBlockDmDev_isMatch()
    {
        // This test validates that a dm-<minor> name is returned when the
        // system does not provide /dev/dm-<minor> helper devices, and the
        // file at /sys/block/dm-<minor>/dev has <major>/<minor> values that
        // match those of the LVM device (in this case 253:2).

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will return good data.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsCorrectMajorMinor
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has the correct information, i.e. "253:2"
                lines.clear();
                lines.push_back(std::wstring(L"253:2"));
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsCorrectMajorMinor() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsCorrectMajorMinor());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_EQUAL(0, lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar").compare(L"dm-2"));
    }


    void GetDMDevice_Returns_dm_When_SysBlockDmDev_isMatch_With_Extra_lines_Ignored()
    {
        // This test validates that a dm-<minor> name is returned when the
        // system does not provide /dev/dm-<minor> helper devices, and the
        // file at /sys/block/dm-<minor>/dev has <major>/<minor> values that
        // match those of the LVM device (in this case 253:2), and all other
        // lines are ignored.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception
           on the odd numbered calls (which, since this is C++, means the 2nd,
           4th, 6th call).  The ReadAllLinesAsUTF8 will return good data with
           some extra lines.
         */
        class SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsExtraLongCorrectMajorMinor
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                static StatHelper_DeviceMapperNoDevDM statHelper;

                statHelper.Stat(path, pStat);
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has the correct information, i.e. "253:2"
                lines.clear();
                lines.push_back(std::wstring(L"253:2"));
                lines.push_back(std::wstring(L"this line will be ignored"));
                lines.push_back(std::wstring(L"this one too"));
                lines.push_back(std::wstring(L""));
                lines.push_back(std::wstring(L""));
                lines.push_back(std::wstring(L"actually, all but the first are ignored"));
            }


            virtual ~SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsExtraLongCorrectMajorMinor() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsReadAllLinesAsUTF8ReturnsExtraLongCorrectMajorMinor());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_EQUAL(0, lvmUtils.GetDMDevice(L"/dev/mapper/lvgSystem-lvVar").compare(L"dm-2"));
    }


    void GetDMSlaves_Throws_FileNotFound_When_Input_Device_Has_No_SlavesPath()
    {
        // This test validates that an exception is thrown when the input produces
        // an invalid slave device path.

        /**
           This class implements SCXLVMUtilsDepends::GetFileSystemEntries that throws an exception.
         */
        class SCXLVMUtilsDependsGetFileSystemEntriesThrowsFileNotFound
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
                const SCXCoreLib::SCXFilePath               & path,
                const SCXCoreLib::SCXDirectorySearchOptions  options)
            {
                // warnings as errors, so deal with the unused params
                (void) options;

                ThrowFileNotFound(path.Get());

                std::vector< SCXCoreLib::SCXFilePath > result;
                return (result);
            }


            virtual ~SCXLVMUtilsDependsGetFileSystemEntriesThrowsFileNotFound() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsGetFileSystemEntriesThrowsFileNotFound());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMSlaves(L"/dev/dm-2"), SCXCoreLib::SCXFilePathNotFoundException);
    }


    void GetDMSlaves_Throws_When_Input_Device_Has_No_SlaveEntries()
    {
        // This test validates that an exception is thrown when the input produces
        // an valid slave device path with no slave device entries.

        /**
           This class implements SCXLVMUtilsDepends::GetFileSystemEntries that
           returns an empty vector.
         */
        class SCXLVMUtilsDependsNoSlaveEntriesThrows
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
                const SCXCoreLib::SCXFilePath               & path,
                const SCXCoreLib::SCXDirectorySearchOptions  options)
            {
                // warnings as errors, so deal with the unused params
                (void) path;
                (void) options;

                std::vector< SCXCoreLib::SCXFilePath > result;

                return (result);
            }


            virtual ~SCXLVMUtilsDependsNoSlaveEntriesThrows() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsNoSlaveEntriesThrows());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMSlaves(L"/dev/dm-2"), SCXSystemLib::SCXBadLVMDeviceException);
    }


    void GetDMSlaves_Ignores_Invalid_SlaveEntries()
    {
        // This test validates that an exception is thrown when the input produces
        // an valid slave device path with no slave device entries.

        /**
           This class implements SCXLVMUtilsDepends::GetFileSystemEntries that
           returns an empty vector.
         */
        class SCXLVMUtilsDependsInvalidSlaveEntries
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
                const SCXCoreLib::SCXFilePath               & path,
                const SCXCoreLib::SCXDirectorySearchOptions  options)
            {
                // warnings as errors, so deal with the unused params
                (void) path;
                (void) options;

                std::vector< SCXCoreLib::SCXFilePath > result;

                result.push_back(L"/.");
                result.push_back(L"/..");
                result.push_back(L"////////");

                return (result);
            }


            virtual ~SCXLVMUtilsDependsInvalidSlaveEntries() { }
        };

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsInvalidSlaveEntries());
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_EQUAL(((size_t) 0), lvmUtils.GetDMSlaves(L"/dev/dm-2").size());
    }


    void GetDMSlaves_Throws_Up()
    {
        // This test validates that an exception thrown in the helper methods
        // is passed up.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception.
         */
        class SCXLVMUtilsDependsStatThrowsFileNotFoundOn3rdCall
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            const std::vector< std::wstring > & m_slaves;
            int                                 m_slaveIndex;


            SCXLVMUtilsDependsStatThrowsFileNotFoundOn3rdCall(const std::vector< std::wstring > & slaves) : m_slaves(slaves), m_slaveIndex(0) { }


            virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
                const SCXCoreLib::SCXFilePath               & path,
                const SCXCoreLib::SCXDirectorySearchOptions  options)
            {
                // warnings as errors, so deal with the unused params
                (void) options;

                std::vector< SCXCoreLib::SCXFilePath > result;

                for (std::vector< std::wstring >::const_iterator iter = m_slaves.begin(); iter != m_slaves.end(); iter++)
                {
                    SCXCoreLib::SCXFilePath slavePath(path);

                    slavePath.AppendDirectory(*iter);
                    result.push_back(slavePath);
                }

                return (result);
            }


            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                // warnings as errors, so deal with the unused params
                (void) path;

                switch (m_slaveIndex)
                {
                    case 0:
                        // plausible /hda5 devid
                        SetupDevice(pStat, 3, 5);
                        break;

                    case 1:
                        // plausible /hdb2 devid
                        SetupDevice(pStat, 4, 2);
                        break;

                    default:
                        // there is no longer a system path for /dev/hdd2
                        ThrowFileNotFound(path.Get());
                        break;
                }
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has information that is not "253:2"
                lines.clear();

                switch (m_slaveIndex++)
                {
                    case 0:
                        // plausible /hda5 devid
                        lines.push_back(std::wstring(L"3:5"));
                        break;

                    case 1:
                        // plausible /hdb2 devid
                        lines.push_back(std::wstring(L"4:2"));
                        break;

                    default:
                        // shouldn't reach this point
                        CPPUNIT_FAIL("The SCXLVMUtils external dependency ReadAllLinesAsUTF8 is not implemented for this device.");
                        break;
                }
            }


            virtual ~SCXLVMUtilsDependsStatThrowsFileNotFoundOn3rdCall() { }
        };

        std::vector< std::wstring > slaves;

        slaves.push_back(L"hda5");
        slaves.push_back(L"hdb2");
        slaves.push_back(L"hdd2");

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsStatThrowsFileNotFoundOn3rdCall(slaves));
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMSlaves(L"/dev/dm-2"), SCXCoreLib::SCXFilePathNotFoundException);
    }


    void GetDMSlaves_Works()
    {
        // This test validates that an exception thrown in the helper methods
        // is passed up.

        /**
           This class implements SCXLVMUtilsDepends::Stat that throws an exception.
         */
        class SCXLVMUtilsDependsProcess3
            : public SCXLVMUtilsDependsNotImplemented
        {
        public:
            const std::vector< std::wstring > & m_slaves;
            int                                 m_slaveIndex;


            SCXLVMUtilsDependsProcess3(const std::vector< std::wstring > & slaves) : m_slaves(slaves), m_slaveIndex(0) { }


            virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
                const SCXCoreLib::SCXFilePath               & path,
                const SCXCoreLib::SCXDirectorySearchOptions  options)
            {
                // warnings as errors, so deal with the unused params
                (void) options;

                std::vector< SCXCoreLib::SCXFilePath > result;

                for (std::vector< std::wstring >::const_iterator iter = m_slaves.begin(); iter != m_slaves.end(); iter++)
                {
                    SCXCoreLib::SCXFilePath slavePath(path);

                    slavePath.AppendDirectory(*iter);
                    result.push_back(slavePath);
                }

                return (result);
            }


            virtual void Stat(
                const SCXCoreLib::SCXFilePath            & path,
                SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
            {
                // warnings as errors, so deal with the unused params
                (void) path;

                switch (m_slaveIndex)
                {
                    case 0:
                        // plausible /hda5 devid
                        SetupDevice(pStat, 3, 5);
                        break;

                    case 1:
                        // plausible /hdb2 devid
                        SetupDevice(pStat, 4, 2);
                        break;

                    case 2:
                        // plausible /hdd2 devid
                        SetupDevice(pStat, 5, 2);
                        break;

                    default:
                        // shouldn't reach this point
                        CPPUNIT_FAIL("The SCXLVMUtils external dependency ReadAllLinesAsUTF8 is not implemented for this device.");
                        break;
                }
            }


            virtual void ReadAllLinesAsUTF8(
                const SCXCoreLib::SCXFilePath & source,
                std::vector< std::wstring >   & lines,
                SCXCoreLib::SCXStream::NLFs   & nlfs)
            {
                // warnings as errors, so deal with the unused params
                (void) source;
                (void) nlfs;

                // the file at /sys/block/dm-2/dev has information that is not "253:2"
                lines.clear();

                switch (m_slaveIndex++)
                {
                    case 0:
                        // plausible /hda5 devid
                        lines.push_back(std::wstring(L"3:5"));
                        break;

                    case 1:
                        // plausible /hdb2 devid
                        lines.push_back(std::wstring(L"4:2"));
                        break;

                    case 2:
                        // plausible /hdb2 devid
                        lines.push_back(std::wstring(L"5:2"));
                        break;

                    default:
                        // shouldn't reach this point
                        CPPUNIT_FAIL("The SCXLVMUtils external dependency ReadAllLinesAsUTF8 is not implemented for this device.");
                        break;
                }
            }


            virtual ~SCXLVMUtilsDependsProcess3() { }
        };

        std::vector< std::wstring > slaves;

        slaves.push_back(L"hda5");
        slaves.push_back(L"hdb2");
        slaves.push_back(L"hdd2");

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new SCXLVMUtilsDependsProcess3(slaves));
        SCXSystemLib::SCXLVMUtils                                 lvmUtils(extDepends);

        std::vector< std::wstring > result = lvmUtils.GetDMSlaves(L"/dev/dm-2");

        size_t index = 0;
        for (std::vector< std::wstring >::const_iterator iter = result.begin(); iter != result.end(); iter++)
        {
            std::wstringstream devPath;

            devPath << L"/dev/" << slaves[index++];

            CPPUNIT_ASSERT_EQUAL(0, iter->compare(devPath.str()));
        }

        CPPUNIT_ASSERT_EQUAL(((size_t) 3), index);
    }

    /**
     * This class mocks SCXLVMUtilsDepends for testing GetDMSlaves.
     * Given a map M of strings to vectors of strings, this class simulates the following directory structure
     * for each entry (X, [Y1,Y2,..Yn]) of M:
     * /sys/block/X
     * /sys/block/X/dev  <-- contains one line "253:2"
     * /sys/block/X/slaves
     * /sys/block/X/slaves/Y1
     * /sys/block/X/slaves/Y2
     * ....
     * /sys/block/X/slaves/Yn
     */
    class TestGetDMSlavesSCXLVMUtilDepends
        : public SCXSystemLib::SCXLVMUtilsDepends
    {
    public:
        std::map< std::wstring, std::vector<std::wstring> > & m_slaves;

        TestGetDMSlavesSCXLVMUtilDepends(std::map< std::wstring, std::vector< std::wstring > > & slaves) : m_slaves(slaves) { }

        virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
            const SCXCoreLib::SCXFilePath               & path,
            const SCXCoreLib::SCXDirectorySearchOptions  options)
        {
            // to avoid getting unused variable warning
            (void) options;
            std::wstring slavesExt = L"/slaves/";
            std::wstring pathStr = path.Get().substr(0, path.Get().length() - slavesExt.length());
            SCXCoreLib::SCXFilePath blockFile = pathStr;

            std::vector< SCXCoreLib::SCXFilePath > result;
            std::vector<std::wstring> slaveEntries = m_slaves[blockFile.GetFilename()];

            for (std::vector< std::wstring >::const_iterator iter = slaveEntries.begin(); iter != slaveEntries.end(); iter++)
            {
                SCXCoreLib::SCXFilePath slavePath(path);

                slavePath.AppendDirectory(*iter);
                result.push_back(slavePath);
            }

            return (result);
        }


        virtual void Stat(
            const SCXCoreLib::SCXFilePath            & path,
            SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
        {
            // warnings as errors, so deal with the unused params
            (void) path;
            pStat->st_rdev = makedev(253, 2);
        }


        virtual void ReadAllLinesAsUTF8(
            const SCXCoreLib::SCXFilePath & source,
            std::vector< std::wstring >   & lines,
            SCXCoreLib::SCXStream::NLFs   & nlfs)
        {
            // warnings as errors, so deal with the unused params
            (void) source;
            (void) nlfs;

            lines.clear();
            lines.push_back(L"253:2");
        }

        virtual ~TestGetDMSlavesSCXLVMUtilDepends() { }
    };

    void GetDMSlaves_SlavesWithDMEntries_TraversesToDevice()
    {
        /*
         * Following directory structure is setup here:
         * /sys/block/dm-1
         * /sys/block/dm-1/slaves/dm-2
         * /sys/block/dm-1/slaves/hda1
         * /sys/block/dm-2/slaves/dm-3
         * /sys/block/dm-2/slaves/hda2
         * /sys/block/dm-3/slaves/hda3
         */
        std::map< std::wstring, std::vector<std::wstring> > deviceSlaveMap;
        deviceSlaveMap[L"dm-1"].push_back(L"dm-2");
        deviceSlaveMap[L"dm-1"].push_back(L"hda1");
        deviceSlaveMap[L"dm-2"].push_back(L"dm-3");
        deviceSlaveMap[L"dm-2"].push_back(L"hda2");
        deviceSlaveMap[L"dm-3"].push_back(L"hda3");

        std::map< std::wstring, std::vector<std::wstring> > expectedSlaveMap;
        expectedSlaveMap[L"/dev/dm-1"].push_back(L"/dev/hda1");
        expectedSlaveMap[L"/dev/dm-1"].push_back(L"/dev/hda2");
        expectedSlaveMap[L"/dev/dm-1"].push_back(L"/dev/hda3");

        expectedSlaveMap[L"/dev/dm-2"].push_back(L"/dev/hda2");
        expectedSlaveMap[L"/dev/dm-2"].push_back(L"/dev/hda3");

        expectedSlaveMap[L"/dev/dm-3"].push_back(L"/dev/hda3");

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new TestGetDMSlavesSCXLVMUtilDepends(deviceSlaveMap));
        SCXSystemLib::SCXLVMUtils lvmUtils(extDepends);

        for (std::map< std::wstring, std::vector<std::wstring> >::iterator it = expectedSlaveMap.begin(); it != expectedSlaveMap.end(); ++it)
        {
            std::vector< std::wstring > result = lvmUtils.GetDMSlaves(it->first);
            CPPUNIT_ASSERT_EQUAL(result.size(), it->second.size());
            for(size_t i=0; i < result.size(); i++)
            {
               CPPUNIT_ASSERT(it->second[i] == result[i]);
            }
        }
    }

    void GetDMSlaves_SlavesWithCircularLinks_Throws()
    {
        /*
         * The following negative scenario is being setup here:
         * /sys/block/dm-1
         * /sys/block/dm-1/slaves/dm-2
         * /sys/block/dm-2
         * /sys/block/dm-2/slaves/dm-3
         * /sys/block/dm-3
         * /sys/block/dm-3/slaves/dm-1
         */
        std::map< std::wstring, std::vector<std::wstring> > deviceSlaveMap;

        deviceSlaveMap[L"dm-1"].push_back(L"dm-2");
        deviceSlaveMap[L"dm-2"].push_back(L"dm-3");
        deviceSlaveMap[L"dm-3"].push_back(L"dm-1");

        SCXCoreLib::SCXHandle< SCXSystemLib::SCXLVMUtilsDepends > extDepends(new TestGetDMSlavesSCXLVMUtilDepends(deviceSlaveMap));
        SCXSystemLib::SCXLVMUtils lvmUtils(extDepends);
        CPPUNIT_ASSERT_THROW(lvmUtils.GetDMSlaves(L"/dev/dm-1"), SCXSystemLib::SCXBadLVMDeviceException);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLVMUtilsTest );

#endif // #if defined(linux) && ! SUSE_9 && ! REDHAT_4
