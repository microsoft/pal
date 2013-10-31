/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines the full static disk information enumeration PAL for logical disks.
    
    \date        2011-05-06 13:40:00

*/
/*----------------------------------------------------------------------------*/
#ifndef STATICLOGICALDISFULLKENUMERATION_H
#define STATICLOGICALDISKFULLENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/staticlogicaldiskinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxhandle.h>
#include <vector>
#include <scxsystemlib/staticlogicaldiskenumeration.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a set of discovered logical disks and their static data.
    Override the function Update() to support all logical disk,including all file system types.
*/
    class StaticLogicalDiskFullEnumeration : public StaticLogicalDiskEnumeration 
    {
    public:
        StaticLogicalDiskFullEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticLogicalDiskFullEnumeration();

        virtual void Update(bool updateInstances=true);
    private:
        std::vector<std::wstring> m_mntTaboptions;
    };

} /* namespace SCXSystemLib */
#endif /* STATICLOGICALDISKFULLENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
