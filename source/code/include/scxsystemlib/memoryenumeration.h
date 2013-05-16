/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    
*/
/**
    \file        

    \brief      Enumeration of Memory. Only contains the total instance.
    
    \date       2007-07-03 09:56:20

*/
/*----------------------------------------------------------------------------*/
#ifndef MEMORYENUMERATION_H
#define MEMORYENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/memoryinstance.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents a colletion of Memory instances. Acutally only
        contains the total instance.
        
        PAL Holding the memory instance.

    */
    class MemoryEnumeration : public EntityEnumeration<MemoryInstance>
    {
    public:
        MemoryEnumeration();
        virtual void Init();

        virtual const std::wstring DumpString() const;
        
    private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle.
    };

}

#endif /* MEMORYENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
