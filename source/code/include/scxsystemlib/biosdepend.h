/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        

    \brief      Implements the default implementation for BIOS dependencies.

    \date       2011-03-28 14:56:20

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <stdio.h>
#include <vector>

namespace SCXSystemLib
{
    class BiosDependencies
    {
    public:
        BiosDependencies();
        virtual ~BiosDependencies();

#if defined(sun) && defined(sparc)
        /*----------------------------------------------------------------------------*/
        /**
        Get sparc prom version
        \param version : returned prom version value.
        */
        virtual void GetPromVersion(std::wstring& version);

        /*----------------------------------------------------------------------------*/
        /**
        Get sparc prom manufacturer
        \param manufacturer : returned prom manufacturer value.
        */
        virtual void GetPromManufacturer(std::wstring& manufacturer);

    private:
        /*----------------------------------------------------------------------------*/
        /**
        Get sparc prom property value
        \param propName : property name.
        \param retValue : returned property value.
        \throws SCXInternalErrorException if querying prom proptery value failed
        */
        void GetPromPropertyValue(const std::wstring& propName,std::wstring& retValue);

        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
#endif
    };
}
