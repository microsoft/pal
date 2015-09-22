/*----------------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved. See license.txt for license information.
*/

/**
    \file    nxnetroutedependencies.h

    \brief   Provides the file path for nxNetRoute provider.

    \date    2015-06-08  11:30:00
*/
/*--------------------------------------------------------------------------------------------*/
#ifndef NXNETROUTEDEPENDENCIES_H
#define NXNETROUTEDEPENDENCIES_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlogitem.h>

#include <string>
#include <vector>

namespace SCXSystemLib
{
    /*--------------------------------------------------------------------------------------------*/
    /**
       Class representing all external dependecies for NetRoute provider.
    */
    class NxNetRouteDependencies
    {

    public:

        /*--------------------------------------------------------------------------------------------*/
        /**
           Constructor

           @param[in] pathToFile The full path to the route file.
           @note The default path should be the same for all Linux distros
        */
        NxNetRouteDependencies(std::wstring pathToRouteFile = L"/proc/net/route");

        /*--------------------------------------------------------------------------------------------*/
        /**
           Virtual Destructor
        */
        virtual ~NxNetRouteDependencies();

        /*--------------------------------------------------------------------------------------------*/
        /**
          Init
        */
        virtual void Init();

        /*--------------------------------------------------------------------------------------------*/
        /**
         @return the path to the route file as a wide string
        */
        std::wstring GetPathToFile()const;

        /*--------------------------------------------------------------------------------------------*/
        /**
          @param[in] pathToProcNetRouteFile  Set the path to the route file as a wide string.
        */
        void SetPathToFile(const std::wstring pathToProcNetRouteFile);

        /*--------------------------------------------------------------------------------------------*/
        /**
          @return std::vector& A vector containing the lines of the route file
        */
        std::vector<std::wstring>& GetLines();

    protected:
        SCXCoreLib::SCXLogHandle m_log;/*!< logging object */
        std::vector<std::wstring> m_lines;/*!< holds each line of the route file */
        std::wstring m_pathToProcNetRouteFile; /*!< the fully qualified path to the route file (ie, proc/net/route) */

    };
}

#endif //NXNETROUTEDEPENDENCIES_H

