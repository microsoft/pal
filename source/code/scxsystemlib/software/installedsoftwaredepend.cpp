/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file        

\brief      installedSoftware dependancy.

\date       2011-01-18 14:56:20

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/installedsoftwaredepend.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/scxprocess.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <vector>
#include <algorithm>
#if defined(hpux)
#include <list>
#endif

#if defined(linux)
#include <fcntl.h>
#include <scxcorelib/scxlibglob.h>
#include <scxcorelib/logsuppressor.h>
#endif


#if defined(linux)
#define MAGIC_RPM_SEP "_/=/_"
#endif


using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
#if defined(hpux)
    const wstring InstalledSoftwareDependencies::keyPublisher     = L"publisher";
    const wstring InstalledSoftwareDependencies::keyTag           = L"tag";
    const wstring InstalledSoftwareDependencies::keyRevision      = L"revision";
    const wstring InstalledSoftwareDependencies::keyTitle         = L"title";
    const wstring InstalledSoftwareDependencies::keyInstallDate   = L"install_date";
    const wstring InstalledSoftwareDependencies::keyInstallSource = L"install_source";
    const wstring InstalledSoftwareDependencies::keyDirectory     = L"directory";
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    */
    InstalledSoftwareDependencies::InstalledSoftwareDependencies(SCXCoreLib::SCXHandle<SoftwareDependencies> deps)
        : m_deps(deps)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.software.installedsoftwaredepencies");
        Init();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor
    */
    InstalledSoftwareDependencies::~InstalledSoftwareDependencies()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
    Init running context. 
    */
    void InstalledSoftwareDependencies::Init()
    {
#if defined(linux) && defined(PF_DISTRO_ULINUX)
        // The librpm functions and the container for the dynamic library symbol handles are not thread safe.
        SCXThreadLock rpmLock(ThreadLockHandleGet(L"RPMLock"));
        
        /**
           A dpkg status file will have entries similar to this:

Package: testDPKGpackage
Status: install ok installed
Priority: important
Section: net
Installed-Size: 212
Maintainer: Marco d'Itri <md@linux.it>
Architecture: i386
Source: tcp-wrappers
Version: 8.6.q-16
Replaces: libwrap0 (<< 7.6-8)
Depends: libc6 (>= 2.7-1), libwrap0 (>= 7.6-4~), debconf (>= 0.5) | debconf-2.0
Description: Wietse Venema's TCP wrapper utilities
 Wietse Venema's network logger, also known as TCPD or LOG_TCP.
 .
 These programs log the client host name of incoming telnet,
 ftp, rsh, rlogin, finger etc. requests.
 .
 Security options are:
  - access control per host, domain and/or service;
  - detection of host name spoofing or host address spoofing;
  - security traps to implement an early-warning system.

Package: nfs-common
Status: install ok installed
Priority: standard
Section: net
Installed-Size: 504
Maintainer: Debian kernel team <debian-kernel@lists.debian.org>
Architecture: i386
Source: nfs-utils
Version: 1:1.1.2-6lenny2
Replaces: mount (<< 2.13~), nfs-client, nfs-kernel-server (<< 1:1.0.7-5)
Provides: nfs-client
Depends: portmap | rpcbind, adduser, ucf, lsb-base (>= 1.3-9ubuntu3), netbase (>= 4.24), initscripts (>= 2.86.ds1-38.1), libc6 (>= 2.7-1), libcomerr2 (>= 1.01), libevent1 (>= 1.3e), libgssglue1, libkrb53 (>= 1.6.dfsg.2), libnfsidmap2, librpcsecgss3, libwrap0 (>= 7.6-4~)
Conflicts: nfs-client
Conffiles:
 /etc/init.d/nfs-common 2a8ec2053cd94daa29f2c8644c268dd4
Description: NFS support files common to client and server
 Use this package on any machine that uses NFS, either as client or
 server.  Programs included: lockd, statd, showmount, nfsstat, gssd
 and idmapd.
 .
 Upstream: SourceForge project "nfs", CVS module nfs-utils.
Homepage: http://nfs.sourceforge.net/

        */
        /*
            Notes about my assumptions:
            1. New packages begin after a newline with nothing on that line
            2. Lines that begin with a space are part of the description: they can be safely ignored
            3. Values can be gathered be taking everything following the first colon on a line. (Where the key is everything before the first colon)
        */

        // parse dpkg "status", if exists, and persist data into DPKGMap
        // test if the file exists, if not, then return
        const wstring dpkg_location = m_deps->GetDPKGStatusLocation();
        if (!SCXFile::Exists(dpkg_location))
        {
            return;
        }

        // read in file /var/lib/dpkg/status
        vector<wstring> localLines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLinesAsUTF8(dpkg_location, localLines, nlfs);

        map<wstring, wstring> keyvalues;

        for (vector<wstring>::iterator line = localLines.begin(); line != localLines.end(); line++)
        {
            wstring curStr = *line;
            if (curStr == L"")
            {
                // end this package block and insert all keyvalues into vectorKeyValues
                if (keyvalues.size() > 0)
                {
                    // if "installed" is one of the space separated tokens in the Status: line, insert this into the DPKG map
                    wstring status = keyvalues[L"Status"];
                    vector<wstring> statusTokens;
                    StrTokenize(status, statusTokens, L" ");
                    
                    if (find(statusTokens.begin(), statusTokens.end(), L"installed") != statusTokens.end())
                    {
                        PackageInfo pkginfo;                    
                        wstring package = keyvalues[L"Package"];
                        wstring version = keyvalues[L"Version"];
                        wstring maintainer = keyvalues[L"Maintainer"];
                        wstring section = keyvalues[L"Section"];
                        wstring homepage = keyvalues[L"Homepage"];
                        wstring description = keyvalues[L"Description"];

                        pkginfo.Name = StrTrim(package);
                        pkginfo.Version = StrTrim(version);
                        pkginfo.Vendor = StrTrim(maintainer);
                        pkginfo.Release = L"";
                        pkginfo.BuildTime = L"";
                        pkginfo.InstallTime = L"";
                        pkginfo.BuildHost = L"";
                        pkginfo.Group = StrTrim(section);
                        pkginfo.SourceRPM = L"";
                        pkginfo.License = L"";
                        pkginfo.Packager = L"";
                        pkginfo.URL = StrTrim(homepage);
                        pkginfo.Summary = StrTrim(description);
                        DPKGMap[pkginfo.Name] = pkginfo;   
                    }
                    
                    keyvalues.clear();
                }
                continue;
            }
            
            if (curStr.substr(0,1) == L" ")
            {
                // ignore this line, go to next line
                continue;
            }
            
            wstring::size_type colonPos;
            if ( (colonPos = curStr.find_first_of(L":")) == wstring::npos )
            {
                // not a valid line, ignore this and include a warning log message
                
                
                
                continue;
            }
            
            // now this is a valid line, add it to the keyvalues map
            wstring key = curStr.substr(0, colonPos);
            wstring value = curStr.substr(colonPos+1);
            
            keyvalues[key] = value;
        }
        
        if (keyvalues.size() > 0)
        {
            // if "installed" is one of the space separated tokens in the Status: line, insert this into the DPKG map
            wstring status = keyvalues[L"Status"];
            vector<wstring> statusTokens;
            StrTokenize(status, statusTokens, L" ");
            
            if (find(statusTokens.begin(), statusTokens.end(), L"installed") != statusTokens.end())
            {
                PackageInfo pkginfo;                    
                wstring package = keyvalues[L"Package"];
                wstring version = keyvalues[L"Version"];
                wstring maintainer = keyvalues[L"Maintainer"];
                wstring section = keyvalues[L"Section"];
                wstring homepage = keyvalues[L"Homepage"];
                wstring description = keyvalues[L"Description"];
                
                pkginfo.Name = StrTrim(package);
                pkginfo.Version = StrTrim(version);
                pkginfo.Vendor = StrTrim(maintainer);
                pkginfo.Release = L"";
                pkginfo.BuildTime = L"";
                pkginfo.InstallTime = L"";
                pkginfo.BuildHost = L"";
                pkginfo.Group = StrTrim(section);
                pkginfo.SourceRPM = L"";
                pkginfo.License = L"";
                pkginfo.Packager = L"";
                pkginfo.URL = StrTrim(homepage);
                pkginfo.Summary = StrTrim(description);
                DPKGMap[pkginfo.Name] = pkginfo;   
            }

            keyvalues.clear();
        }
        
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
    Clean up running context.
    */
    void InstalledSoftwareDependencies::CleanUp()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get All installed software Ids,
    on linux, id will be display name since it's unique and we can get it from RPM cli
    on solaris, id will be the name of folder where pkginfo stored.
    /param ids : contains all ids which identiry each software package.
    */
    void InstalledSoftwareDependencies::GetInstalledSoftwareIds(vector<wstring>& ids)
    {
#if defined(linux)
        const std::string rpmCommandName = "";
        const std::string rpmCommandType = "-qa";
        int argc = 2;
        char * argv[] = {const_cast<char*>(rpmCommandName.c_str()),
            const_cast<char*>(rpmCommandType.c_str())};

        GetRPMQueryResult(argc, argv, ids);
#if defined(PF_DISTRO_ULINUX)
        GetDPKGList(ids);
#endif

#elif defined(sun)
        vector<SCXFilePath> allDirs = SCXDirectory::GetDirectories(L"/var/sadm/pkg/");
        for(vector<SCXFilePath>::iterator it = allDirs.begin(); it != allDirs.end(); it++) {
            it->Append(L"pkginfo");
            if(SCXFile::Exists(*it)) {
                ids.push_back(it->Get());
            }
        }
#elif defined(hpux)
        vector<SCXFilePath> allDirs = SCXDirectory::GetDirectories(L"/var/adm/sw/products/");
        for(vector<SCXFilePath>::iterator it = allDirs.begin(); it != allDirs.end(); it++)
        {
            wstring sProductDirectory(it->Get());
            SCXFilePath sIndexFilepath(it->Get() + L"pfiles/INDEX");

            if(SCXFile::Exists(sIndexFilepath))
            {
                ids.push_back(sProductDirectory);
            }
        }
#elif defined(aix)
        // If the Filesets and history have not yet been collected, do so now.
        if(!m_Ids.empty() || GetAllFilesetLines())
        {
            //ids.assign(m_Filesets.begin(), m_Filesets.end());
            ids.assign(m_Ids.begin(), m_Ids.end());
        }
#else
        ids.clear();
#endif
    }

#if defined(linux)
    /*----------------------------------------------------------------------------*/
    /**
    Call RPM cli using popen, and get the result
    \param argc : count of arguments of argv.
    \param argv : points to all arguments
    \param result : RPM query result
    \throws SCXErrnoException if popen or pclose fails
    */
    void InstalledSoftwareDependencies::GetRPMQueryResult(int argc, char * argv[], std::vector<wstring>& result)
    {
        // We need to redirect stdout to a file so we can capture the result of InvokeRPMQuery() call.
	std::wstring rpmPath = L"/bin/rpm";
	std::wstringstream commandToRun;

	static LogSuppressor warningSuppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
	static LogSuppressor infoSuppressor(SCXCoreLib::eInfo, SCXCoreLib::eTrace);

	if (!SCXFile::Exists(SCXFilePath(rpmPath)))
	{
	    SCX_LOG(m_log, infoSuppressor.GetSeverity(L"/bin/rpm lookup"), L"No rpm executable at /bin/rpm, therefore skipping rpm package enumeration.");
	    return;
	}

	commandToRun << rpmPath;
	for (int i = 0; i < argc; ++i)
	{
	    commandToRun << L" ";
	    commandToRun << StrFrom(argv[i]);
	}
	
	std::istringstream input;
	std::ostringstream output;
	std::ostringstream error;
	int returnCode = SCXCoreLib::SCXProcess::Run(commandToRun.str(), input, output, error);

	if (returnCode != 0)
	{
	    SCX_LOG(m_log, warningSuppressor.GetSeverity(commandToRun.str()), std::wstring(L"RPM command returned nonzero value.  Return value: ") + StrFrom(returnCode) + std::wstring(L", exact command ran: ") + commandToRun.str());
	    return;
	}

	std::wstring wcontent = StrFrom(output.str().c_str());
	StrReplaceAll(wcontent, StrFrom(MAGIC_RPM_SEP), L"\n");
	StrTokenize(wcontent, result, L"\n");
	
    }
    
#if defined(PF_DISTRO_ULINUX)
    void InstalledSoftwareDependencies::GetDPKGInfo(const wstring & searchedPackage, vector<wstring>& result)
    {
        std::map<wstring, PackageInfo>::iterator it = DPKGMap.find(searchedPackage);
        if (it == DPKGMap.end())
        {
            // unable to find package in DPKGMap
            return;
        }

        PackageInfo temppi = it->second;
        result.push_back(L"Name:" + temppi.Name);
        result.push_back(L"Version:" + temppi.Version);
        result.push_back(L"Group:" + temppi.Group);
        result.push_back(L"URL:" + temppi.URL);
        result.push_back(L"Summary:" + temppi.Summary);
    }

    void InstalledSoftwareDependencies::GetDPKGList(vector<wstring>& result)
    {
        for (std::map<wstring, PackageInfo>::iterator it = DPKGMap.begin(); it != DPKGMap.end(); it++)
        {
            result.push_back(it->first);
        }
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
    pass "-qi softwareName" params to RPM cli and retrun the raw data about the software.
    \param softwareName : the productName or displayName of softwareName
    \param contents : Software information date retured by RPM cli
    */
    void InstalledSoftwareDependencies::GetSoftwareInfoRawData(const wstring& softwareName, std::vector<wstring>& contents)
    {
        const std::string rpmCommandName = "";
        const std::string rpmCommandType = "-q";
        const std::string rpmQueryFormat = "--qf=Name:%{Name}" MAGIC_RPM_SEP "Version:%{Version}" MAGIC_RPM_SEP "Vendor:%{Vendor}" MAGIC_RPM_SEP "Release:%{Release}" MAGIC_RPM_SEP "BuildTime:%{BuildTime}" MAGIC_RPM_SEP "InstallTime:%{InstallTime}" MAGIC_RPM_SEP "BuildHost:%{BuildHost}" MAGIC_RPM_SEP "Group:%{Group}" MAGIC_RPM_SEP "SourceRPM:%{SourceRPM}" MAGIC_RPM_SEP "License:%{License}" MAGIC_RPM_SEP "Packager:%{Packager}" MAGIC_RPM_SEP "URL:%{URL}" MAGIC_RPM_SEP "Summary:%{Summary}" MAGIC_RPM_SEP;

        int argc = 4;
        std::string nSoftwareName = SCXCoreLib::StrToUTF8(StrTrim(softwareName));
        char * argv[] = {const_cast<char*>(rpmCommandName.c_str()),
            const_cast<char*>(rpmCommandType.c_str()),
            const_cast<char*>(rpmQueryFormat.c_str()),
            const_cast<char*>(nSoftwareName.c_str())};

        GetRPMQueryResult(argc, argv, contents);

#if defined(PF_DISTRO_ULINUX)
        // is it ok to make an assumption that RPM and DPKGs are mutually exclusive on the same machine? 
        // (i.e. no 'apache' installed with both RPM and DPKG)
        GetDPKGInfo(softwareName, contents);

#endif
    }
#endif

#if defined(aix)
    /*----------------------------------------------------------------------------*/
    /**
    Split the fileset listing csv and read select properties.
    */
    bool InstalledSoftwareDependencies::GetFilesetProperties(const wstring& productID, wstring& description, wstring& version)
    {
        bool fRet = false;
        std::vector<wstring> vListingTokens;
        MAPLPPLISTING::const_iterator cit = m_lppListing.find(productID);
        if (cit != m_lppListing.end())
        {
            wstring fileset(cit->second);

            // Split string.  Include empty fields to preserve ordinality.
            StrTokenize(fileset, vListingTokens, wstring(L":"), /*trim*/ true, /*emptytokens*/ true);

            if (vListingTokens.size() > LPPLISTDescription && vListingTokens.size() > LPPLISTLevel)
            {
                description = vListingTokens[LPPLISTDescription];
                version     = vListingTokens[LPPLISTLevel];
                fRet = true;
            }
            else
            {
                SCX_LOGERROR(m_log, L"Bad listing: " + fileset + L" field count " + StrFrom(vListingTokens.size()));
            }
        }

        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Split the fileset history csv and read select properties.
    */
    bool InstalledSoftwareDependencies::GetFilesetProperties(const wstring& productID, SCXCoreLib::SCXCalendarTime& installDate)
    {
        bool fRet = false;
        std::vector<wstring> vHistoryTokens;
        MAPLPPHISTORY::const_iterator cit = m_lppHistory.find(productID);
        if (cit != m_lppHistory.end())
        {
            wstring fileset(cit->second);

            // Split string.  Include empty fields to preserve ordinality.
            StrTokenize(fileset, vHistoryTokens, wstring(L":"), /*trim*/ true, /*emptytokens*/ true);

            if (vHistoryTokens.size() >= LPPHIST_MAX)
            {
                struct tm installtm;
                wstring sInstallDate = vHistoryTokens[LPPHISTDate] + L" " + vHistoryTokens[LPPHISTTime];

                std::transform(sInstallDate.begin(), sInstallDate.end(), sInstallDate.begin(), InstalledSoftwareDependencies::SemiToColon);

                string dateTime = StrToUTF8(sInstallDate);
                const char * strpResult = strptime(dateTime.c_str(), "%x %X", &installtm);
                if (strpResult == (dateTime.c_str() + dateTime.size()))
                {
                    try
                    {
                        SCXCoreLib::SCXCalendarTime installDateBuild( (scxyear)installtm.tm_year + 1900, (scxmonth)installtm.tm_mon + 1, (scxday)installtm.tm_mday);
                        installDateBuild.SetHour((scxhour)installtm.tm_hour);
                        installDateBuild.SetMinute((scxminute)installtm.tm_min);
                        installDateBuild.SetSecond((scxsecond)installtm.tm_sec);
                        installDate = installDateBuild;
                        fRet = true;
                    }
                    catch(const SCXException& e)
                    {
                        SCX_LOGERROR(m_log, wstring(L"Bad lpp history date: ") + e.What());
                        fRet = false;
                    }
                }
                else
                {
                    SCX_LOGERROR(m_log, wstring(L"Bad install date ") + sInstallDate);
                }
            }
            else
            {
                SCX_LOGERROR(m_log, L"Bad history: " + fileset + L" field count " + StrFrom(vHistoryTokens.size()) + L". Expected >= LPHIST_MAX " + StrFrom(LPPHIST_MAX));
            }
        }

        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get properties from fileset.
    */
    bool InstalledSoftwareDependencies::GetProperties(const wstring& productID, wstring& version, wstring& description, SCXCoreLib::SCXCalendarTime& installDate)
    {
        bool fRet = false;

        if (m_lppHistory.size() == 0)
        {
            GetAllFilesetLines();
        }
        if (GetFilesetProperties(productID, description, version) && GetFilesetProperties(productID, installDate))
        {
            fRet = true;
        }
        else
        {
            SCX_LOGERROR(m_log, wstring(L"Failed to collect properties for ") + productID);
        }

        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get second field of colon separated string.  History and listing store product id in second field.
    */
    bool InstalledSoftwareDependencies::CSVSecondField(const wstring& csvLine, wstring & field)
    {
        bool fRet = false;
        wstring productID;
    
        size_t firstColon = csvLine.find(L':', 0);
        if (firstColon != wstring::npos)
        {
             size_t fieldStart = firstColon + 1;
             size_t secondColon = csvLine.find(L':', fieldStart);
             if (firstColon != wstring::npos)
             {
                 size_t ccField = secondColon - fieldStart;
                 field = csvLine.substr(fieldStart, ccField);
                 fRet = true;
             }
         }
    
         return fRet;
     }

    /*----------------------------------------------------------------------------*/
    /**
    Read the pkginfo file and stores all lines in m_Filesets and m_History.
    */
    bool InstalledSoftwareDependencies::GetAllFilesetLines()
    {
        bool fRet = true;

        try
        {
            int rc = 0;
            std::istringstream processInput;
            std::ostringstream processOutput;
            std::ostringstream processErr;
            const wstring  cmdListing(L"/usr/bin/lslpp -Lcq all");
            const wstring  cmdHistory(L"/usr/bin/lslpp -hcq all");

            if (0 == (rc = SCXCoreLib::SCXProcess::Run(cmdListing, processInput, processOutput, processErr, 15000)))
            {
                if (!!processOutput)
                {
                    wistringstream lines(StrFromUTF8(processOutput.str()));
                    wstring line;

                    if (!lines) 
                    {
                        fRet = false;
                    }
                    else
                    {
                        m_lppListing.clear();
                        m_lppHistory.clear();
                    }

                    while(!!lines)
                    {
                        wstring productID;
                        std::getline(lines, line);

                        // Peel off id and push in into m_Ids.
                        if (CSVSecondField(line, productID))
                        {
                            m_Ids.push_back(productID);
                            m_lppListing.insert(MAPLPPLISTING::value_type(productID, line));
                        }
                    }
                }

                // Re-use the output streams.
                processOutput.clear();
                processOutput.str("");
                processErr.clear();
                processErr.str("");
           
                SCX_LOGINFO(m_log, wstring(L"lpp fileset count: ") + StrFrom(m_lppListing.size()));
            }
            else
            {
                SCX_LOGERROR(m_log, wstring(L"Listing command returned error: ") + StrFrom(rc));
                fRet = false;
            }

            if (0 == (rc = SCXCoreLib::SCXProcess::Run(cmdHistory, processInput, processOutput, processErr, 15000)))
            {
                if (!!processOutput)
                {
                    wistringstream lines(StrFromUTF8(processOutput.str()));
                    wstring line;

                    if (!lines) 
                    {
                        fRet = false;
                    }

                    while(!!lines)
                    {
                        wstring productID;

                        std::getline(lines, line);

                        if (CSVSecondField(line, productID))
                        {
                            m_lppHistory.insert(MAPLPPHISTORY::value_type(productID, line));
                        }
                    }
                }

                SCX_LOGINFO(m_log, wstring(L"lpp history count: ") + StrFrom(m_lppHistory.size()));
            }
            else
            {
                SCX_LOGERROR(m_log, wstring(L"History command returned error: ") + StrFrom(rc));
                fRet = false;
            }

        }
        catch (SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, wstring(L"attempt to execute lslpp command.") + e.What());
            fRet = false;
        }

        return fRet;

    }
#endif

#if defined(sun)
    /*----------------------------------------------------------------------------*/
    /**
    Read the pkginfo file and returns all lines.
    \param pkgFile : the path of pkginfo file.
    */
    void InstalledSoftwareDependencies::GetAllLinesOfPkgInfo(const wstring& pkgFile, vector<wstring>& allLines)
    {
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(SCXFilePath(pkgFile), allLines, nlfs);
    }
#endif

#if defined(hpux)

    /*----------------------------------------------------------------------------*/
    /**
    Read the INDEX file and returns all lines.
    \param indexFile : the path of installation properties file.

    Example input:-------------------
    vendor
    tag HP
    title "Hewlett-Packard Company"
    description "Hewlett-Packard Company"
    end
    product //or bundle in other directories
    tag AVIO-GVSD
    data_model_revision 2.40
    instance_id 1
    control_directory AVIO-GVSD
    revision B.11.31.0909
    title "HPVM Guest AVIO Storage Software"
    description "Vendor Name                     Hewlett-Packard Company
    
    Product Name                    AVIO-GVSD
    
    The GVSD-KRN fileset contains the GVSD kernel module.
    
    The GVSD-RUN fileset contains the GVSD user space components.
    "
    mod_time 1294005058
    create_time 1294003675
    install_date 201101021650.58
    architecture HP-UX_B.11.31_IA
    machine_type ia64*
    os_name HP-UX
    os_release B.11.31
    os_version *
    install_source scxrrhpi05.scx.com:/var/opt/ignite/depots/Rel_B.11.31/core_media
    install_type physical
    vendor_tag HP
    directory /
    all_filesets GVSD-KRN GVSD-RUN 
    is_locatable false
    location /
    ---------------------------------
    */
    bool InstalledSoftwareDependencies::GetAllPropertiesOfIndexFile(const wstring& indexFile, PROPMAP & allProperties)
    {
        typedef std::list<wstring> LISTPROPS;
        typedef LISTPROPS::iterator LISTPROPS_IT;

        bool fRet = false;
    
        std::wifstream fsIndex(StrToUTF8(indexFile).c_str());
        wstring propertyKey;
        wstring propertyValue;
        int  size = 0;
        // Key entries found in "product" or "fileset" sections of INDEX file.
        wstring arrProductKeys[] = {
            keyTag          ,
            keyRevision     ,
            keyTitle        ,
            keyInstallDate  ,
            keyInstallSource,
            keyDirectory 
        };
        LISTPROPS listProductKeys(arrProductKeys, arrProductKeys + (sizeof(arrProductKeys)/sizeof(arrProductKeys[0])));
        typedef enum { isEnd, isVendor, isProduct } INDEX_SECTION;
        INDEX_SECTION indexsection = isEnd;
        bool fVendorSectionDone = false;
    
        // Get the size of the file (cheaply).
        fsIndex.seekg(0, std::ios_base::end);
        size = fsIndex.tellg();
        fsIndex.seekg(0, std::ios_base::beg);
    
        // Read all key/value pairs.
        // Keys follow '\n' and have no whitespace.
        // Values follow first whitespace after key.
        // Values may be quoted strings containing '\n' characters.
    
        while (!!fsIndex)
        {
            LISTPROPS_IT propit = listProductKeys.end();
            wchar_t sep = ' ';

            if (listProductKeys.empty() && fVendorSectionDone)
            {
                break;
            }
    
            // Read property key
            fsIndex >> propertyKey;
    
            // Check for INDEX section change
            if (propertyKey.empty())
            {
                continue;
            } else if (0 == propertyKey.compare(L"vendor") && indexsection == isEnd)
            {
                indexsection = isVendor;
                continue;
            }else if ((0 == propertyKey.compare(L"product") || 0 == propertyKey.compare(L"bundle")) && indexsection == isEnd) 
            {
                indexsection = isProduct;
                continue;
            } else if (0 == propertyKey.compare(L"end"))
            {
                if (indexsection == isVendor)
                {
                    fVendorSectionDone = true;
                }
                indexsection = isEnd;
                continue;
            }
    
            // Move stream pointer to property value.
            if (!SkipSeparator(fsIndex, sep))
            {
                break;
            }
    
            // Check for quoted (possibly multi-line) value.
            if (L'"' == sep)
            {
                std::getline(fsIndex, propertyValue, L'"');
                fsIndex.ignore(1);
            } else if(L'\n' == sep)
            {
                // No value after key.
                fsIndex.ignore(1);
                continue;
            } else
            {
                // Read property value.
                fsIndex >> propertyValue;
                if (propertyValue.empty())
                {
                    // No value after key.
    
                    continue;
                }
            }
    
            // Store key/value pair according to the INDEX section in which it is found.
            if (indexsection == isVendor)
            {
                if (0 == propertyKey.compare(InstalledSoftwareDependencies::keyTitle))
                {
                    // "vendor/title" maps to "publisher" key.
                    allProperties[keyPublisher] = propertyValue;
                    fRet = true;
                }
            } else if (indexsection == isProduct)
            {
                // Collect value.  Handle possible quoted string.
                propit = std::find(listProductKeys.begin(), listProductKeys.end(), propertyKey);
                if (propit != listProductKeys.end())
                {
                    // Remove from list and put key/value into map.
                    listProductKeys.erase(propit);
                    allProperties[propertyKey] = propertyValue;
    
                    fRet = true;
                }
            }
        }
    
        return fRet;
    
    }
    
    /** ----------------------------------------------------------------------------
     *  Skip separator charactars.  Leave pointer on first character of value.
     */
    bool InstalledSoftwareDependencies::SkipSeparator(std::wifstream& fsIndex, wchar_t& sep)
    {
        bool fSuccess = fsIndex.good();
        sep = L' ';
    
        // Skip white space until value.  Stop at quoted value.
        while(sep == L' ' && fSuccess)
        {
            sep = fsIndex.peek();
            if (sep == L'"')
            {
                fsIndex.get(sep);
                // current pointer is at value quote char + 1.
                break;
            }
    
            if (sep == L' ')
            {
                fsIndex.get(sep);
            }
    
            fSuccess = fsIndex.good();
        }
    
        return fSuccess;
    
    }

#endif
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
