/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test we have the appropiate rights to perform disk tests.

    \date        2008-01-21 15:39:42

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxfile.h>
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>

class DiskRightsTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( DiskRightsTest );
#if defined(hpux)
    CPPUNIT_TEST( IsLvmtabReadable );
#endif
#if defined(linux)
    CPPUNIT_TEST( AreDevicesReadable );
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    void IsLvmtabReadable()
    {
        std::ifstream stream("/etc/lvmtab", std::ios_base::in);
        if ( ! stream.good())
            SCXUNIT_WARNING(L"/etc/lvmtab is not readable (may not exist or you do not have reading privileges)");
    }

    void AreDevicesReadable()
    {
        // Only do this check if /etc/mtab contains references to logical volumes.
        bool vg_found = false;
        std::ifstream mtab("/etc/mtab", std::ios_base::in);
        while ( ! vg_found && SCXCoreLib::SCXStream::IsGood(mtab))
        {
            SCXCoreLib::SCXStream::NLF nlf;
            std::wstring line;
            SCXCoreLib::SCXStream::ReadLineAsUTF8(mtab, line, nlf);
            if (line.find(L" ") != std::wstring::npos)
            {
                line = line.substr(0,line.find(L" "));
            }
            if (line.rfind(L"/") != std::wstring::npos && 
                std::wstring::npos != line.substr(line.rfind(L"/")).find(L"-"))
                vg_found = true;
        }
        mtab.close();

        if ( ! vg_found)
            return;

        // Assume at least one ide or scsi device is available on the system.
        const char* PATHS[] = { "/dev/hda1", "/dev/hda2", "/dev/hda3", "/dev/hda4",
                                "/dev/hdb1", "/dev/hdb2", "/dev/hdb3", "/dev/hdb4",
                                "/dev/sda1", "/dev/sda2", "/dev/sda3", "/dev/sda4",
                                "/dev/sdb1", "/dev/sdb2", "/dev/sdb3", "/dev/sdb4",
                                0 };

        const char** path = PATHS;
        bool ok = false;
        while (( ! ok) && (0 != *path))
        {
            std::ifstream stream(*path++, std::ios_base::in);
            ok = SCXCoreLib::SCXStream::IsGood(stream);
        }
        if ( ! ok)
            SCXUNIT_WARNING(L"None of the expected devices are readble (probably you do not have read privileges or your system has non default names for IDE & SCSI devices)");
        SCXUNIT_RESET_ASSERTION();
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( DiskRightsTest );
