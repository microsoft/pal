/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        getlinuxos_test.cpp

    \brief       Test cases for the GetLinuxOS.sh script

    \date        2012-10-01 14:24:00

*/
/*----------------------------------------------------------------------------*/

// We only run on Linux platforms.  We could test for PF_DISTRO_REDHAT,
// PF_DISTRO_SUSE, and PF_DISTRO_ULINUX.  But, just to be super safe:

#if !defined(aix) && !defined(hpux) && !defined(sun)

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <testutils/scxtestutils.h>

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

using namespace std;
using namespace SCXCoreLib;

static const wstring s_wsScriptFile  = L"./testfiles/GetLinuxOS.sh";
static const wstring s_wsDisableFile = L"./disablereleasefileupdates";
static const wstring s_wsReleaseFile = L"./scx-release";

class SCXGetLinuxOS_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXGetLinuxOS_Test );
    CPPUNIT_TEST( TestReleaseFileGenerated );
    CPPUNIT_TEST( TestDisableFileDisables );
    CPPUNIT_TEST( TestNoReleaseFile );
    CPPUNIT_TEST( TestPlatform_RHEL_61 );
    CPPUNIT_TEST( TestPlatform_RHEL_70 );
    CPPUNIT_TEST( TestPlatform_SLES_9_0 );
    CPPUNIT_TEST( TestPlatform_SLES_10 );
    CPPUNIT_TEST( TestPlatform_Oracle_5 );
    CPPUNIT_TEST( TestPlatform_Oracle_6 );
    CPPUNIT_TEST( TestPlatform_NeoKylin );
    CPPUNIT_TEST( TestPlatform_Debian_5_0_10 );
    CPPUNIT_TEST( TestPlatform_Ubuntu_11 );
    CPPUNIT_TEST( TestPlatform_CentOS_5 );
    CPPUNIT_TEST( TestPlatform_CentOS_7 );
    CPPUNIT_TEST( TestPlatform_Debian_7_0 );
    CPPUNIT_TEST( TestPlatform_openSUSE_11_4 );
    CPPUNIT_TEST( TestPlatform_openSUSE_12_3 );
    CPPUNIT_TEST( TestPlatform_ALT_Linux_6_0_0 );
    CPPUNIT_TEST( TestPlatform_Fedora_8 );
    CPPUNIT_TEST_SUITE_END();

private:


public:
    void setUp(void)
    {
        // Be sure that the release and disable files don't exist
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        SelfDeletingFilePath delDisableFile( s_wsDisableFile );
    }

    void tearDown(void)
    {

    }

    // Helper routine - load the release file into a map (hash)
    void LoadReleaseFile(map<string,string>& relFile)
    {
        // Just use SCX native <wstring> routines to load the file
        // Then convert to multibyte characters to populate the map

        vector<wstring> lines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(s_wsReleaseFile, lines, nlfs);

        for (size_t i = 0; i < lines.size(); i++ )
        {
            vector<wstring> tokens;
            StrTokenize( lines[i], tokens, L"=" );
            CPPUNIT_ASSERT_EQUAL_MESSAGE( StrToUTF8(StrAppend(L"On line `" + lines[i] + L"\", only received token count of ", tokens.size())), static_cast<size_t>(2), tokens.size() );

            relFile[StrToUTF8(tokens[0])] = StrToUTF8(tokens[1]);
        }

        // We have lines like this (assuming an unrecognized system):
        //
        //      OSName=Linux
        //      OSVersion=2.6.32-131.0.15.el6.x86_64
        //      OSShortName=Linux_2.6.32-131.0.15.el6.x86_64 (x86_64)
        //      OSFullName=Linux 2.6.32-131.0.15.el6.x86_64 (x86_64)
        //      OSAlias=Universal
        //      OSManufacturer=Red Hat, Inc.
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(6), relFile.size() );
    }

    // Helper routine - dump release file map (debug purposes only)
    void DumpReleaseFile(const map<string,string>& relFile)
    {
        cout << endl << "Release File map:" << endl;

        for ( map<string,string>::const_iterator it=relFile.begin() ; it != relFile.end() ; ++it )
            cout << "\t" << it->first << " = " << it->second << endl;
    }

    // Helper routine - run the script (passing any parameters to the script)
    void ExecuteScript(const wstring &param)
    {
        // Run the script.
        istringstream input;
        ostringstream output, error;

        wstring command = s_wsScriptFile;
        command += L" " + param;

        CPPUNIT_ASSERT_EQUAL( static_cast<int>(0), SCXCoreLib::SCXProcess::Run(command, input, output, error) );
        CPPUNIT_ASSERT_EQUAL( string(""), error.str() );
        CPPUNIT_ASSERT_EQUAL( string(""), output.str() );
    }

    // Helper routine - run the script and load the results into a map
    void ExecuteScript(const wstring &param, map<string,string>& releaseFile )
    {
        ExecuteScript( param );
        LoadReleaseFile( releaseFile );
    }

    //
    // Unit tests follow
    //

    // Test - on the current system - that a release file is generated
    // (No injection - just figure out the current system)
    void TestReleaseFileGenerated(void)
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        ExecuteScript( L"" );

        // Be sure that the release file exists
        CPPUNIT_ASSERT( SCXFile::Exists(s_wsReleaseFile) );

        // Be sure that the release file is not empty
        SCXFileSystem::SCXStatStruct stat;
        SCXFileSystem::Stat( SCXFilePath(s_wsReleaseFile), &stat );
        CPPUNIT_ASSERT( 0 != stat.st_size );

        // Load the file to insure that it has our usual number of lines
        map<string,string> releaseFile;
        LoadReleaseFile(releaseFile);
    }

    // Test that the disable file will really disable overwrite of the release file.
    // Create empty release and disable files, then run the script.  When the script
    // completes, the release file should still be empty.
    void TestDisableFileDisables()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        SelfDeletingFilePath delDisableFile( s_wsDisableFile );

        // Create empty release & disable files - and close them by limiting scope
        {
            vector<wstring> emptyVector;
            SCXFile::WriteAllLines( s_wsReleaseFile, emptyVector, ios_base::out );
            SCXFile::WriteAllLines( s_wsDisableFile, emptyVector, ios_base::out );
        }

        CPPUNIT_ASSERT( SCXFile::Exists(s_wsReleaseFile) );
        CPPUNIT_ASSERT( SCXFile::Exists(s_wsDisableFile) );

        // Run the script.  Script should run, but nothing should be generated.
        ExecuteScript( L"" );

        // Verify that the release file is still empty
        SCXFileSystem::SCXStatStruct stat;
        SCXFileSystem::Stat( SCXFilePath(s_wsReleaseFile), &stat );
        CPPUNIT_ASSERT( 0 == stat.st_size );
    }

    // Run the script with no release file at all (no /etc/*-release file); be sure that
    // we get proper behavior (defaults for an unrecognized system).
    //
    // Also insure that STDERR and STDOUT both are empty after running the script.
    void TestNoReleaseFile()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"/bogus/path", releaseFile );

        // Get uname information
        struct utsname unameInfo;
        CPPUNIT_ASSERT_EQUAL( static_cast<int>(0), uname(&unameInfo) );

        // Verify our data:
        //      OSName=Linux
        //      OSVersion=2.6.32-131.0.15.el6.x86_64
        //      OSShortName=Linux_<version>
        //      OSFullName=Linux 2.6.32-131.0.15.el6.x86_64 (x86_64)
        //      OSAlias=Universal
        //      OSManufacturer="Universal"

        CPPUNIT_ASSERT_EQUAL( string("Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string(unameInfo.release), releaseFile["OSVersion"] );
        string osShortName = string("Linux_") + unameInfo.release;
        CPPUNIT_ASSERT_EQUAL( osShortName, releaseFile["OSShortName"] );
        string osFullName = string("Linux ") + unameInfo.release;
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find(osFullName) );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSAlias"].find("Universal") );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSManufacturer"].find("Universal") );
    }

    // Platform RHEL version 6.1:
    //   /etc/redhat-release:
    //          Red Hat Enterprise Linux Server release 6.1 (Santiago)
    void TestPlatform_RHEL_61()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/rhel_6.1", releaseFile );

        // Verify our data:
        //      OSName=Red Hat Enterprise Linux
        //      OSVersion=6.1
        //      OSShortName=RHEL_6.1
        //      OSFullName=Red Hat Enterprise Linux Server release 6.1 (Santiago)
        //      OSAlias=RHEL
        //      OSManufacturer=Red Hat, Inc.

        CPPUNIT_ASSERT_EQUAL( string("Red Hat Enterprise Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("6.1"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("RHEL_6.1"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Red Hat Enterprise Linux Server release 6.1 (Santiago)") );
        CPPUNIT_ASSERT_EQUAL( string("RHEL"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Red Hat, Inc."), releaseFile["OSManufacturer"] );
    }

    // Platform RHEL version 7.0:
    void TestPlatform_RHEL_70()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/rhel_7.0", releaseFile );

        // Verify our data:
        //      OSName=Red Hat Enterprise Linux
        //      OSVersion=7.0
        //      OSShortName=RHEL_7.0
        //      OSFullName=Red Hat Enterprise Linux Server release 7.0 (Maipo)
        //      OSAlias=RHEL
        //      OSManufacturer=Red Hat, Inc.

        CPPUNIT_ASSERT_EQUAL( string("Red Hat Enterprise Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("7.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("RHEL_7.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Red Hat Enterprise Linux Server release 7.0 (Maipo)") );
        CPPUNIT_ASSERT_EQUAL( string("RHEL"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Red Hat, Inc."), releaseFile["OSManufacturer"] );
    }

    // Platform SLES version 9 (Patch Level 0):
    void TestPlatform_SLES_9_0()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/sles_9.0", releaseFile );

        // Verify our data:
        //      OSName = SUSE LINUX Enterprise Server
        //      OSVersion = 9.0
        //      OSShortName=SUSE_9.0
        //      OSFullName = SUSE LINUX Enterprise Server 9 (x86_64)
        //      OSAlias = SLES
        //      OSManufacturer = SUSE GmbH

        CPPUNIT_ASSERT_EQUAL( string("SUSE LINUX Enterprise Server"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("9.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE_9.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("SUSE LINUX Enterprise Server 9.0") );
        CPPUNIT_ASSERT_EQUAL( string("SLES"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE GmbH"), releaseFile["OSManufacturer"] );
    }

    // Platform SLES version 10 (Patch Level 1):
    void TestPlatform_SLES_10()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/sles_10", releaseFile );

        // Verify our data:
        //      OSName=SUSE Linux Enterprise Server
        //      OSVersion=10.1
        //      OSShortName=SUSE_10.1
        //      OSFullName=SUSE Linux Enterprise Server 10.1 (x86_64)
        //      OSAlias=SLES
        //      OSManufacturer = SUSE GmbH

        CPPUNIT_ASSERT_EQUAL( string("SUSE Linux Enterprise Server"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("10.1"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE_10.1"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("SUSE Linux Enterprise Server 10.1") );
        CPPUNIT_ASSERT_EQUAL( string("SLES"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE GmbH"), releaseFile["OSManufacturer"] );
    }

    // Platform Oracle Enterprise Linux 5 (presents itself as Enterprise Linux):
    void TestPlatform_Oracle_5()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/oracle_5", releaseFile );

        // Verify our data:
        //      OSName = Enterprise Linux Server
        //      OSVersion = 5.0
        //      OSShortName=Oracle_5.0
        //      OSFullName = Enterprise Linux Server 5 (x86_64)
        //      OSAlias = UniversalR
        //      OSManufacturer = Oracle Corporation

        CPPUNIT_ASSERT_EQUAL( string("Enterprise Linux Server"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("5.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Oracle_5.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Enterprise Linux Server 5.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Oracle Corporation"), releaseFile["OSManufacturer"] );
    }

    // Platform Oracle Enterprise Linux 6:
    void TestPlatform_Oracle_6()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/oracle_6", releaseFile );

        // Verify our data:
        //      OSName = Oracle Linux Server
        //      OSVersion = 6.0
        //      OSShortName=Oracle_6.0
        //      OSFullName = Oracle Linux Server 6.0 (x86_64)
        //      OSAlias = UniversalR
        //      OSManufacturer = Oracle Corporation

        CPPUNIT_ASSERT_EQUAL( string("Oracle Linux Server"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("6.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Oracle_6.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Oracle Linux Server 6.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Oracle Corporation"), releaseFile["OSManufacturer"] );
    }

    // Platform NeoKylin version 5.6:
    void TestPlatform_NeoKylin()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/neokylin_5.6", releaseFile );

        // Verify our data:
        //      OSName=NeoKylin Linux Server
        //      OSVersion=5.6
        //      OSShortName=NeoKylin_5.6
        //      OSFullName=NeoKylin Linux Server 5.6 (x86_64)
        //      OSAlias=UniversalR
        //      OSManufacturer = China Standard Software Co., Ltd.

        CPPUNIT_ASSERT_EQUAL( string("NeoKylin Linux Server"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("5.6"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("NeoKylin_5.6"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("NeoKylin Linux Server 5.6") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("China Standard Software Co., Ltd."), releaseFile["OSManufacturer"] );
    }

    // Platform Debian 5.0.10:
    void TestPlatform_Debian_5_0_10()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/debian_5.0.10", releaseFile );

        // Verify our data:
        //      OSName = Debian
        //      OSVersion = 5.0.10
        //      OSShortName=Debian_5.0.10
        //      OSFullName = Debian 5.0.10 (x86_64)
        //      OSAlias = UniversalD
        //      OSManufacturer = Software in the Public Interest, Inc.

        CPPUNIT_ASSERT_EQUAL( string("Debian"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("5.0.10"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Debian_5.0.10"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Debian 5.0.10") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalD"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Software in the Public Interest, Inc."), releaseFile["OSManufacturer"] );
    }

    // Platform Ubuntu 11:
    void TestPlatform_Ubuntu_11()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/ubuntu_11", releaseFile );

        // Verify our data:
        //      OSName = Ubuntu
        //      OSVersion = 11.04
        //      OSShortName=Ubuntu_11.04
        //      OSFullName = Ubuntu 11.04 (x86_64)
        //      OSAlias = UniversalD
        //      OSManufacturer = Canonical Group Limited

        CPPUNIT_ASSERT_EQUAL( string("Ubuntu"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("11.04"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Ubuntu_11.04"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Ubuntu 11.04") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalD"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Canonical Group Limited"), releaseFile["OSManufacturer"] );
    }

    // Platform CentOS 5:
    void TestPlatform_CentOS_5()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/centos_5", releaseFile );

        // Verify our data:
        //      OSName = CentOS
        //      OSVersion = 5.0
        //      OSShortName=CentOS_5.0
        //      OSFullName = CentOS 5 (x86_64)
        //      OSAlias = UniversalR
        //      OSManufacturer = Central Logistics GmbH

        CPPUNIT_ASSERT_EQUAL( string("CentOS"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("5.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("CentOS_5.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("CentOS 5.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Central Logistics GmbH"), releaseFile["OSManufacturer"] );
    }

    // Platform CentOS 7:
    void TestPlatform_CentOS_7()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/centos_7", releaseFile );

        // Verify our data:
        //      OSName = CentOS Linux
        //      OSVersion = 7.0
        //      OSShortName=CentOS_7.0
        //      OSFullName = CentOS Linux 7.0
        //      OSAlias = UniversalR
        //      OSManufacturer = Central Logistics GmbH

        CPPUNIT_ASSERT_EQUAL( string("CentOS Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("7.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("CentOS_7.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("CentOS Linux 7.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Central Logistics GmbH"), releaseFile["OSManufacturer"] );
    }

    // Platform Debian 7.0:
    void TestPlatform_Debian_7_0()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/debian_7.0", releaseFile );

        // Verify our data:
        //      OSName = Debian GNU/Linux
        //      OSVersion = 7.0
        //      OSShortName=Debian_7.0
        //      OSFullName = Debian GNU/Linux 7.0 (wheezy)
        //      OSAlias = UniversalD
        //      OSManufacturer = Software in the Public Interest, Inc.

        CPPUNIT_ASSERT_EQUAL( string("Debian GNU/Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("7.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Debian_7.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Debian") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalD"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Software in the Public Interest, Inc."), releaseFile["OSManufacturer"] );
    }

    // Platform openSUSE 11.4:
    void TestPlatform_openSUSE_11_4()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/openSUSE_11.4", releaseFile );

        // Verify our data:
        //      OSName = openSUSE
        //      OSVersion = 11.4
        //      OSShortName=OpenSUSE_11.4
        //      OSFullName = openSUSE 11.4 (x86_64)
        //      OSAlias = UniversalR
        //      OSManufacturer = SUSE GmbH

        CPPUNIT_ASSERT_EQUAL( string("openSUSE"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("11.4"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("OpenSUSE_11.4"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("openSUSE 11.4") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE GmbH"), releaseFile["OSManufacturer"] );
    }

    // Platform openSUSE 12.3:
    void TestPlatform_openSUSE_12_3()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/openSUSE_12.3", releaseFile );

        // Verify our data:
        //      OSName = openSUSE
        //      OSVersion = 12.3
        //      OSShortName=OpenSUSE_12.3
        //      OSFullName = openSUSE 12.3 (x86_64)
        //      OSAlias = UniversalR
        //      OSManufacturer = SUSE GmbH

        CPPUNIT_ASSERT_EQUAL( string("openSUSE"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("12.3"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("OpenSUSE_12.3"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("openSUSE 12.3") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("SUSE GmbH"), releaseFile["OSManufacturer"] );
    }

    void TestPlatform_ALT_Linux_6_0_0()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/ALT_Linux_6.0.0", releaseFile );

        // Verify our data:
        //      OSName = ALT Linux
        //      OSVersion = 6.0.0
        //      OSShortName=ALTLinux_6.0.0
        //      OSFullName = ALT Linux 6.0.0
        //      OSAlias = UniversalR
        //      OSManufacturer = ALT Linux Ltd

        CPPUNIT_ASSERT_EQUAL( string("ALT Linux"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("6.0.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("ALTLinux_6.0.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("ALT Linux 6.0.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("ALT Linux Ltd"), releaseFile["OSManufacturer"] );
    }

    void TestPlatform_Fedora_8()
    {
        SelfDeletingFilePath delReleaseFile( s_wsReleaseFile );
        map<string,string> releaseFile;
        ExecuteScript( L"./testfiles/platforms/fedora_8", releaseFile );

        // Verify our data:
        //      OSName = Fedora
        //      OSVersion = 8.0
        //      OSShortName=Fedora_8.0
        //      OSFullName = Fedora 8.0
        //      OSAlias = UniversalR
        //      OSManufacturer = Red Hat, Inc.

        CPPUNIT_ASSERT_EQUAL( string("Fedora"), releaseFile["OSName"] );
        CPPUNIT_ASSERT_EQUAL( string("8.0"), releaseFile["OSVersion"] );
        CPPUNIT_ASSERT_EQUAL( string("Fedora_8.0"), releaseFile["OSShortName"] );
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), releaseFile["OSFullName"].find("Fedora 8.0") );
        CPPUNIT_ASSERT_EQUAL( string("UniversalR"), releaseFile["OSAlias"] );
        CPPUNIT_ASSERT_EQUAL( string("Red Hat, Inc."), releaseFile["OSManufacturer"] );
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXGetLinuxOS_Test );

#endif // !defined(aix) && !defined(hpux) && !defined(sun)
