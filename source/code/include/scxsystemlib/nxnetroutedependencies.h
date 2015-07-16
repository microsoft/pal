/*----------------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved license.txt for license information
*/

/**
   
     \file

     \brief   Define the product-specific dependencies for SCXSystemLib

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
       Class representing all external dependecies for NetRoute

    */
    class NxNetRouteDependencies
    {
        friend class NxNetRouteEnumeration;

    public:
        /*--------------------------------------------------------------------------------------------*/
        /**
           Constructor

        */
        NxNetRouteDependencies();
        NxNetRouteDependencies(std::wstring pathToFile);
        /*--------------------------------------------------------------------------------------------*/
        /**
           Virtual destructor

        */
        virtual ~NxNetRouteDependencies();
        /*--------------------------------------------------------------------------------------------*/
        /**
          Init  running context

        */
        virtual void Init();
        /*--------------------------------------------------------------------------------------------*/
        /**
        Clean up running context.

        */
        void CleanUp();

       

        /*--------------------------------------------------------------------------------------------*/
        /**
        Return string to test parsing.

        */
        virtual std::wstring GetRouteFileLocation()
        {
            // NOTE: let's set the path in the constructor.  this will let us dep inject a new
            // fake path for writing so we don't clobber the real file
            return L"/proc/net/route";
        }
   
       std::wstring GetPathToFile()const { return m_pathToFile; }
       void SetPathToFile(const std::wstring pathToFile) { m_pathToFile = pathToFile; }
       

    protected:
        SCXCoreLib::SCXLogHandle m_log;
        std::vector<std::wstring> vDependencyInjection;
        std::wstring m_pathToFile;
    };
}

#endif //NXNETROUTEDEPENDENCIES_H
