/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of BIOS 

   \date        11-01-14 15:00:00
*/
/*----------------------------------------------------------------------------*/
#ifndef BIOSENUMERATION_H
#define BIOSENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/biosinstance.h>
#include <scxcorelib/scxlog.h>


namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a colletion of BIOS

       PAL Holding collection of BIOS

    */
    class BIOSEnumeration : public EntityEnumeration<BIOSInstance>
    {
    public:
        BIOSEnumeration();
        ~BIOSEnumeration();
        virtual void Init();
        virtual void CleanUp();
    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.

    };

}

#endif /* BIOSENUMERATION_H*/
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
