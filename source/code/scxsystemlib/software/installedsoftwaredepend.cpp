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


#if defined(linux) && defined(PF_DISTRO_ULINUX)
    /**
       This is a helper function to make the symbol lookup process simpler.
       \param handle : the LibHandle for the library to do the lookup on
       \param symbol : the symbol to look up
       \param errorMessage : the output error message if there is one
    */
    static void * GetSymbolSimple(LibHandle& handle, string symbol, string& errorMessage)
    {
        void * retval = handle.GetSymbol(symbol.c_str());
        if (retval == NULL)
        {
            // unable to find the symbol
            errorMessage = " unable to find symbol: " + symbol + " :: " + errorMessage + string(handle.GetError());
            return NULL;
        }
        
        return retval;
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
    Init running context. 
    */
    void InstalledSoftwareDependencies::Init()
    {
#if defined(linux) && defined(PF_DISTRO_ULINUX)
        // The librpm functions and the container for the dynamic library symbol handles are not thread safe.
        SCXThreadLock rpmLock(ThreadLockHandleGet(L"RPMLock"));
        
        // dlopen libraries
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        
        if (!gs_popt.m_handle.IsOpen())
        {
            SCXLibGlob poptDL(L"libpopt[^a-zA-Z]*so*");
            vector<SCXFilePath> poptPaths = poptDL.Get();
            
            for (vector<SCXFilePath>::iterator it = poptPaths.begin(); it != poptPaths.end(); it++)
            {
                wstring wlibname = it->Get();
                string libname = StrToUTF8(wlibname);
                gs_popt.m_handle.Open(libname.c_str());
                if (!gs_popt.m_handle.IsOpen())
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(wlibname));
                    SCX_LOG(m_log, severity,  "Unable to open popt library: " + libname + " :: " + string(gs_popt.m_handle.GetError()));
                    continue;
                }
                
                // dynamically load the symbols that we need from this library
                string errorMessage;
                if ( !(gs_popt.m_poptAliasOptions = (struct poptOption *)GetSymbolSimple(gs_popt.m_handle, "poptAliasOptions", errorMessage)) ||
                     !(gs_popt.m_poptHelpOptions = (struct poptOption *)GetSymbolSimple(gs_popt.m_handle, "poptHelpOptions", errorMessage)) ||
                     !(gs_popt.mf_poptGetArgs = __extension__(const char ** (*)(poptContext con))GetSymbolSimple(gs_popt.m_handle, "poptGetArgs", errorMessage))
                    )
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(wlibname));
                    SCX_LOG(m_log, severity,  "In library: " + libname + errorMessage);
                    if(gs_popt.m_handle.Close())
                    {
                        SCX_LOGERROR(m_log, "Unable to close detected popt library: " + libname);
                    }
                    continue;
                }
                
                break;
            }
        }

        if (gs_popt.m_handle.IsOpen() && !gs_rpm.m_handle.IsOpen())
        {
            SCXLibGlob rpmDL(L"librpm[^a-zA-Z]*so*");
            vector<SCXFilePath> rpmPaths = rpmDL.Get();
            
            for (vector<SCXFilePath>::iterator it = rpmPaths.begin(); it != rpmPaths.end(); it++)
            {
                wstring wlibname = it->Get();
                string libname = StrToUTF8(wlibname);
                gs_rpm.m_handle.Open(libname.c_str());
                if (!gs_rpm.m_handle.IsOpen())
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(wlibname));
                    SCX_LOG(m_log, severity,  "Unable to open rpm library: " + libname + " :: " + string(gs_rpm.m_handle.GetError()));
                    continue;
                }
                
                // dynamically load the symbols we need from the rpm library and check if any errors            
                string errorMessage;
                if ( !(gs_rpm.m_rpmQueryPoptTable = (struct poptOption *)GetSymbolSimple(gs_rpm.m_handle, "rpmQueryPoptTable", errorMessage)) ||
                     !(gs_rpm.m_rpmQVSourcePoptTable = (struct poptOption *)GetSymbolSimple(gs_rpm.m_handle, "rpmQVSourcePoptTable", errorMessage)) ||
                     !(gs_rpm.m_rpmQVKArgs = (struct rpmQVKArguments_s *)GetSymbolSimple(gs_rpm.m_handle, "rpmQVKArgs", errorMessage)) ||
                     !(gs_rpm.mf_rpmcliInit = __extension__(poptContext (*)(int argc, char *const argv[], struct poptOption * optionsTable))GetSymbolSimple(gs_rpm.m_handle, "rpmcliInit", errorMessage)) ||
                     !(gs_rpm.mf_rpmtsCreate = __extension__(rpmts (*)(void))GetSymbolSimple(gs_rpm.m_handle, "rpmtsCreate", errorMessage)) ||
                     !(gs_rpm.mf_rpmcliFini = __extension__(poptContext (*)(poptContext optCon))GetSymbolSimple(gs_rpm.m_handle, "rpmcliFini", errorMessage)) ||
                     !(gs_rpm.mf_rpmcliQuery = __extension__(int (*)(rpmts ts, QVA_t qva, const char ** argv))GetSymbolSimple(gs_rpm.m_handle, "rpmcliQuery", errorMessage)) ||
                     !(gs_rpm.mf_rpmtsFree = __extension__(rpmts (*)(rpmts ts))GetSymbolSimple(gs_rpm.m_handle, "rpmtsFree", errorMessage)) 
                    )
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(wlibname));
                    SCX_LOG(m_log, severity,  "In library: " + libname + errorMessage);
                    if (gs_rpm.m_handle.Close())
                    {
                        SCX_LOGERROR(m_log, "Unable to close detected rpm library: " + libname);
                    }
                    continue;
                }
                
                break;
            }
        }

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
    on linux, id will be display name since it's unique and we can get it from RPM API
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

        GetRPMQueryResult(argc, argv, L"rpmoutput", ids);
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
    Call RPM Query API
    \param argc : count of arguments of argv.
    \param argv : points to all arguments
    \returns     the result if the rpm command runs sucessfully,when invoking RPM API fails, it is errno. of failures ,otherwise 0.
    \throws SCXInternalErrorException if querying RPM fails
    */
    int InstalledSoftwareDependencies::InvokeRPMQuery(int argc, char * argv[])
    {
#if defined(PF_DISTRO_ULINUX)
        // The librpm functions and the container for the dynamic library symbol handles are not thread safe.
        SCXThreadLock rpmLock(ThreadLockHandleGet(L"RPMLock"));
        if (!gs_rpm.m_handle.IsOpen())
        {
            return -1;
        }
        if (!gs_popt.m_handle.IsOpen())
        {
            return -1;
        }

        struct poptOption optionsTable[] =
        {
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, gs_rpm.m_rpmQueryPoptTable, 0, "Query options (with -q or --query):", NULL },
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, gs_rpm.m_rpmQVSourcePoptTable, 0, "Query/Verify options:", NULL },
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, gs_popt.m_poptAliasOptions, 0, "Options implemented via popt alias/exec:", NULL },
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, gs_popt.m_poptHelpOptions, 0, "Help options:", NULL },
            POPT_TABLEEND
        };

        int errorCode = 0;
        poptContext context = NULL;
        rpmts ts = NULL;

        //m_rpmQVKArgs is a global variable, it will be changed when invoking rpmcliQuery,
        //and the change will invalidate next time's invoking, so we need to keep a copy of it before invoking.
        struct rpmQVKArguments_s tempRpmQVKArgs;
        memcpy(&tempRpmQVKArgs, gs_rpm.m_rpmQVKArgs, sizeof(struct rpmQVKArguments_s));
        QVA_t qva = gs_rpm.m_rpmQVKArgs;
        context = gs_rpm.mf_rpmcliInit(argc, argv, optionsTable);
        if (context == NULL)
        {
            throw SCXInternalErrorException(L"rpmcliInit failed", SCXSRCLOCATION);
        }

        ts = gs_rpm.mf_rpmtsCreate();
        if (ts == NULL)
        {
            (void)gs_rpm.mf_rpmcliFini(context);
            throw SCXInternalErrorException(L"rpmtsCreate failed", SCXSRCLOCATION);
        }

        // 0 on success, else number of failures
        const char ** poptArgs = gs_popt.mf_poptGetArgs(context);
        errorCode = gs_rpm.mf_rpmcliQuery(ts, qva, poptArgs);

        if (errorCode != 0)
        {
            (void)gs_rpm.mf_rpmtsFree(ts);
            (void)gs_rpm.mf_rpmcliFini(context);
            memcpy(gs_rpm.m_rpmQVKArgs, &tempRpmQVKArgs, sizeof (struct rpmQVKArguments_s));    // restore global QVK arguments
            
            // If the query failed, this can occur when a package is not found (which can happen if both dpkg and rpm are installed on
            // the same machine), so do not throw an exception in this case.
            //throw SCXInternalErrorException(UnexpectedErrno(L"rpmcliQuery failed", errorCode), SCXSRCLOCATION);
            return errorCode;
        }

        ts = gs_rpm.mf_rpmtsFree(ts);

        context = gs_rpm.mf_rpmcliFini(context);

        //recover rpmQVKArgs data after invoking rpmcliQuery;
        memcpy(gs_rpm.m_rpmQVKArgs, &tempRpmQVKArgs, sizeof (struct rpmQVKArguments_s));
        return errorCode;

#else // REDHAT or SUSE
        // The librpm functions and the container for the dynamic library symbol handles are not thread safe.
        SCXThreadLock rpmLock(ThreadLockHandleGet(L"RPMLock"));
        struct poptOption optionsTable[] = 
        { 
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQueryPoptTable, 0, "Query options (with -q or --query):", NULL },
            { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmQVSourcePoptTable, 0, "Query/Verify options:", NULL }, 
            POPT_AUTOALIAS 
            POPT_AUTOHELP 
            POPT_TABLEEND 
        };

        int errorCode = 0;
        poptContext context = NULL;
        rpmts ts = NULL;

        //rpmQVKArgs is a global variable, it will be changed when invoking rpmcliQuery, 
        //and the change will invalidate next time's invoking, so we need to keep a copy of it before invoking. 
        struct rpmQVKArguments_s tempRpmQVKArgs;
        memcpy(&tempRpmQVKArgs, &rpmQVKArgs, sizeof(struct rpmQVKArguments_s));
        QVA_t qva = &rpmQVKArgs;

        context = rpmcliInit(argc, argv, optionsTable);
        if (context == NULL)
        {
            throw SCXInternalErrorException(L"rpmcliInit failed", SCXSRCLOCATION);
        }

        ts = rpmtsCreate();
        if (ts == NULL)
        {
            (void)rpmcliFini(context);
            throw SCXInternalErrorException(L"rpmtsCreate failed", SCXSRCLOCATION);
        }

        //0 on success, else number of failures 
        //in redhat 4 or 5 and SuSE 9, 10, and 11 x86, the RPM version is 4.4.2,
        //  prototype: int rpmcliQuery (rpmts ts, QVA_t qva,const char** argv)
        //in redhat 6 and SuSE 11 ia64, the RPM version is 4.8,
        //  prototype: int rpmcliQuery(rpmts ts, QVA_t qva, ARGV_const_t argv)
        const char ** poptArgs = poptGetArgs(context);
#if (defined(PF_DISTRO_REDHAT) && PF_MAJOR < 6) || defined(PF_DISTRO_SUSE)
        errorCode = rpmcliQuery(ts, qva, poptArgs);
#else
        errorCode = rpmcliQuery(ts, qva, const_cast<ARGV_const_t>(poptArgs));
#endif
        if (errorCode != 0)
        {
            (void)rpmtsFree(ts);
            (void)rpmcliFini(context);
            memcpy(&rpmQVKArgs, &tempRpmQVKArgs, sizeof (struct rpmQVKArguments_s));    // restore global QVK arguments
            throw SCXInternalErrorException(UnexpectedErrno(L"rpmcliQuery failed", errorCode), SCXSRCLOCATION);
        }

        ts = rpmtsFree(ts);

        context = rpmcliFini(context);

        //recover rpmQVKArgs data after invoking rpmcliQuery;
        memcpy(&rpmQVKArgs, &tempRpmQVKArgs, sizeof (struct rpmQVKArguments_s));

        return errorCode;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
    Call RPM Query API, and get the result
    \param argc : count of arguments of argv.
    \param argv : points to all arguments
    \param tempFileName : temporay file that the query result is saved to. 
    \param result : RPM API query result
    \throws SCXInternalErrorException if freopen stdout fails
    */
    void InstalledSoftwareDependencies::GetRPMQueryResult(int argc, char * argv[], const wstring& tempFileName, std::vector<wstring>& result)
    {
        // We need to redirect stdout to a file so we can capture the result of InvokeRPMQuery() call.
        
        // Flush stdout.
        if (fflush(stdout) != 0)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"fflush(stdout) failed", errno), SCXSRCLOCATION);
        }
        // Make a copy of original stdout destination.
        int oldStdOutFD = dup(1);
        if (oldStdOutFD == -1)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"dup(1) failed", errno), SCXSRCLOCATION);
        }
        // Open new stdout destination.
        int fileFD = open(SCXCoreLib::StrToUTF8(tempFileName).c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fileFD == -1)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"open(" + tempFileName + L") failed", errno), SCXSRCLOCATION);
        }
        // Redirect stdout to the new destination.
        if (dup2(fileFD, 1) == -1)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"dup2(fileFD, 1) failed", errno), SCXSRCLOCATION);
        }

        InvokeRPMQuery(argc, argv);

        // Flush new stdout.
        if (fflush(stdout) != 0)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"fflush(stdout) failed", errno), SCXSRCLOCATION);
        }
        // Restore stdout to the original destination.
        if (dup2(oldStdOutFD, 1) == -1)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"dup2(oldStdOutFD, 1) failed", errno), SCXSRCLOCATION);
        }
        // Close new stdout destination, not needed anymore.
        if (close(fileFD) != 0)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"close(fileFD) failed", errno), SCXSRCLOCATION);
        }
        // Close copy of the original stdout destination.
        if (close(oldStdOutFD) != 0)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"close(oldStdOutFD) failed", errno), SCXSRCLOCATION);
        }

        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(tempFileName, std::ios::in));
        fs.SetOwner();
        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLines(*fs, result, nlfs);
        fs->close();
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
    pass "-qi softwareName" params to RPM API and retrun the raw data about the software.
    \param softwareName : the productName or displayName of softwareName
    \param contents : Software information date retured by RPM API
    */
    void InstalledSoftwareDependencies::GetSoftwareInfoRawData(const wstring& softwareName, std::vector<wstring>& contents)
    {
        const std::string rpmCommandName = "";
        const std::string rpmCommandType = "-q";
        const std::string rpmQueryFormat = "--qf=Name:%{Name}\n\
Version:%{Version}\n\
Vendor:%{Vendor}\n\
Release:%{Release}\n\
BuildTime:%{BuildTime}\n\
InstallTime:%{InstallTime}\n\
BuildHost:%{BuildHost}\n\
Group:%{Group}\n\
SourceRPM:%{SourceRPM}\n\
License:%{License}\n\
Packager:%{Packager}\n\
URL:%{URL}\n\
Summary:%{Summary}\n\
";

        int argc = 4;
        std::string nSoftwareName = SCXCoreLib::StrToUTF8(softwareName);
        char * argv[] = {const_cast<char*>(rpmCommandName.c_str()),
            const_cast<char*>(rpmCommandType.c_str()),
            const_cast<char*>(rpmQueryFormat.c_str()),
            const_cast<char*>(nSoftwareName.c_str())};

        GetRPMQueryResult(argc, argv, L"rpmpackageoutput", contents);

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
