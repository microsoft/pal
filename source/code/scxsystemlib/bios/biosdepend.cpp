/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
\file       biosdepend.cpp

\brief      functions to access Sun SPARC BIOS information or Solaris x86 boot information

\date       2011-03-28 14:56:20

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/biosdepend.h>
#include <scxcorelib/scxlog.h>
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
        struct openpromio* opp;
        int fd;
        int proplen;
        size_t buflen;

        fd = open(PROM_DEV, O_RDONLY);
        if (fd < 0)
        {
            throw SCXInternalErrorException(L"open of /dev/openprom failed", SCXSRCLOCATION);
        }

         /* Allocate an openpromio structure big enough to get the PROM version string */
        opp = (struct openpromio*)malloc(sizeof (struct openpromio) + MAXVALSZ);
        if (opp == NULL)
        {
            throw SCXInternalErrorException(L"could not allocate memory", SCXSRCLOCATION);
        }
        (void)memset(opp, 0, sizeof (struct openpromio) + MAXVALSZ);
        opp->oprom_size = MAXVALSZ;
        if (ioctl(fd, OPROMGETVERSION, opp) < 0)
        {
            free(opp);
            throw SCXInternalErrorException(L"ioctl on /dev/openprom failed", SCXSRCLOCATION);
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
        di_node_t root_node = NULL;
        di_node_t promNode = NULL;
        di_prom_handle_t ph = NULL;
        char *strp = NULL;
        int rval;
        bool fFound = false;

        // Snapshot with all information
        if ((root_node = di_init(DI_ROOT_PATH, DINFOCPYALL)) == DI_NODE_NIL)
        {
            throw SCXInternalErrorException(L"di_init() failed", SCXSRCLOCATION);
        }

        // Create handle to PROM
        if ((ph = di_prom_init()) == DI_PROM_HANDLE_NIL)
        {
            di_fini(root_node);
            throw SCXInternalErrorException(L"di_prom_init() failed", SCXSRCLOCATION);
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
            di_fini(root_node);
            wstring exmsg(L"Search for propery \"");
            exmsg += propName + L"\" failed.";
            throw SCXInternalErrorException(exmsg.c_str(), SCXSRCLOCATION);
        }
#else
        // open openprom node
        const std::string nodePath = DI_PROM_PATH;
        if ((promNode = di_lookup_node(root_node, const_cast<char*>(nodePath.c_str()))) == DI_NODE_NIL)
        {
            di_fini(root_node);
            throw SCXInternalErrorException(L"di_lookup_node for /openprom failed", SCXSRCLOCATION);
        }

#endif
        // get prom property value
        rval = di_prom_prop_lookup_strings(ph, promNode, const_cast<char*>(StrToUTF8(propName).c_str()), &strp);
        if (rval == -1)
        {
            di_prom_fini(ph);
            di_fini(root_node);
            throw SCXInternalErrorException(L"di_prom_prop_lookup_strings() failed for property name :version", SCXSRCLOCATION);
        }

        retValue = StrFromUTF8(strp);

        // Clean up handles
        di_prom_fini(ph);

        di_fini(root_node);
    }
#endif
}
