/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file       biosdepend.cpp

\brief      functions to access Sun SPARC BIOS information or Solaris x86 boot information

\date       2011-03-28 14:56:20

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/biosdepend.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxoserror.h>

#if defined(sun) && defined(sparc)
 #include <errno.h>
 #include <stdio.h>
 #include <memory.h>
 #include <fcntl.h>
 #include <unistd.h>
 //#include <stdlib.h>
 #include <libdevinfo.h>
 #include <sys/openpromio.h>
 #define MAXVALSZ (1024 - sizeof (int))

 static const char* DI_ROOT_PATH = "/";
 static const char* DI_PROM_PATH = "/openprom";
 static const wchar_t* PROM_PROP_VERSION = L"version";
 static const wchar_t* PROM_PROP_MODEL = L"model";
 static const char* PROM_DEV = "/dev/openprom";
#endif

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
      Constructor
    */
    BiosDependencies::BiosDependencies()
    {
#if defined(sun) && defined(sparc)
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.bios.biosdepend"));
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Virtual destructor
    */
    BiosDependencies::~BiosDependencies()
    {
    }

#if defined(sun) && defined(sparc)

    /*----------------------------------------------------------------------------*/
    /**
      Get the prom version

      \param        version - returned prom version value
      \throws       SCXInternalErrorException if querying prom property value failed
    */
    void BiosDependencies::GetPromVersion(wstring& version)
    {
        static LogSuppressor suppressor(eInfo, eTrace);

        version.clear();

        struct openpromio* opp;
        int fd;
        int proplen;
        size_t buflen;

        fd = open(PROM_DEV, O_RDONLY);
        if (fd < 0)
        {
            std::wstring errMsg(L"open of /dev/openprom failed");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }

         /* Allocate an openpromio structure big enough to get the PROM version string */
        opp = (struct openpromio*)malloc(sizeof (struct openpromio) + MAXVALSZ);
        if (opp == NULL)
        {
            (void)close(fd);
            throw SCXInternalErrorException(L"could not allocate memory", SCXSRCLOCATION);
        }
        (void)memset(opp, 0, sizeof (struct openpromio) + MAXVALSZ);
        opp->oprom_size = MAXVALSZ;
        if (ioctl(fd, OPROMGETVERSION, opp) < 0)
        {
            std::wstring errMsg(L"ioctl on /dev/openprom failed");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            free(opp);
            (void)close(fd);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }
        (void)close(fd);
        version = StrFromUTF8((char*)opp->oprom_array);
        free(opp);

        return;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get the Sparc prom manufacturer

      \param[in]    manufacturer - returned prom manufacturer value.
      \throws       SCXInternalErrorException if querying prom property value failed
    */
    void BiosDependencies::GetPromManufacturer(wstring& manufacturer)
    {
        GetPromPropertyValue(PROM_PROP_MODEL, manufacturer);
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get the Sparc prom property value

      \param[in]    propName - property name.
      \param[out]   retValue - returned property value.
      \throws       SCXInternalErrorException if querying prom property value failed
    */
    void BiosDependencies::GetPromPropertyValue(const wstring& propName, wstring& retValue)
    {
        static LogSuppressor suppressor(eInfo, eTrace);

        retValue.clear();

        di_node_t root_node = NULL;
        di_node_t promNode = NULL;
        di_prom_handle_t ph = NULL;
        char *strp = NULL;
        int rval;
        bool fFound = false;

        // Snapshot with all information
        if ((root_node = di_init(DI_ROOT_PATH, DINFOCPYALL)) == DI_NODE_NIL)
        {
            std::wstring errMsg(L"di_init() failed");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }

        // Create handle to PROM
        if ((ph = di_prom_init()) == DI_PROM_HANDLE_NIL)
        {
            std::wstring errMsg(L"di_prom_init() failed");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            di_fini(root_node);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }

        // Verify that the node is present.
#if (PF_MINOR < 10)
        di_prom_prop_t prop = di_prom_prop_next(ph, root_node, DI_PROM_PROP_NIL);
        while (prop != DI_PROM_PROP_NIL)
        {
             char *promPropName = NULL;

             promPropName = di_prom_prop_name(prop);
             if (promPropName != NULL)
             {
                 fFound = (bool)(0 == propName.compare(SCXCoreLib::StrFromUTF8(promPropName)));
                 if (fFound)
                 {
                     break;
                 }
             }
             prop = di_prom_prop_next(ph, root_node, prop);
        }

        if (fFound)
        {
            promNode = root_node;
        }
        else
        {
            di_prom_fini(ph);
            di_fini(root_node);
            wstring errMsg(L"Search for propery \"");
            errMsg += propName + L"\" failed.";
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), errMsg);
            return;
        }
#else
        // open openprom node
        const std::string nodePath = DI_PROM_PATH;
        if ((promNode = di_lookup_node(root_node, const_cast<char*>(nodePath.c_str()))) == DI_NODE_NIL)
        {
            std::wstring errMsg(L"di_lookup_node for /openprom failed");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            di_prom_fini(ph);
            di_fini(root_node);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }

#endif
        // get prom property value
        rval = di_prom_prop_lookup_strings(ph, promNode, const_cast<char*>(StrToUTF8(propName).c_str()), &strp);
        if (rval == -1)
        {
            std::wstring errMsg(L"di_prom_prop_lookup_strings() failed for property name :version");
            SCXErrnoException e(errMsg, errno, SCXSRCLOCATION);
            di_prom_fini(ph);
            di_fini(root_node);
            SCX_LOG(m_log, suppressor.GetSeverity(errMsg), e.What());
            return;
        }

        retValue = StrFromUTF8(strp);

        // Clean up handles
        di_prom_fini(ph);

        di_fini(root_node);
    }
#endif
}
