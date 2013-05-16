/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 

*/
/**
    \file

    \brief          Enumeration of Operating System
    \date           08-03-04 14:14:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef OSENUMERATION_H
#define OSENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/osinstance.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a collection of OperatingSystem data.
       There is only one instance.
    */
    class OSEnumeration : public EntityEnumeration<OSInstance>
    {
    public:
        static const wchar_t *moduleIdentifier;         //!< Module identifier

        OSEnumeration();
        ~OSEnumeration();
        virtual void Init();

        virtual const std::wstring DumpString() const;

    private:
        SCXCoreLib::SCXLogHandle m_log;                 //!< Handle to log file 
    };

}

#endif /* OSENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
