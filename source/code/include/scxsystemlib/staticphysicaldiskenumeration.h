/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Defines the static disk information enumeration PAL for physical disks.

    \date        2008-03-19 11:42:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATICPHYSICALDISKENUMERATION_H
#define STATICPHYSICALDISKENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/staticphysicaldiskinstance.h>
#include <scxsystemlib/diskdepend.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a set of discovered physical disks and their static data.
*/
    class StaticPhysicalDiskEnumeration : public EntityEnumeration<StaticPhysicalDiskInst\
ance>
    {
    public:
        StaticPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StaticPhysicalDiskEnumeration();

        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

#if defined (sun)
    protected:
        virtual void UpdateSolarisHelper();
#endif

    private:
        //! Private constructor. Should not be used.
        StaticPhysicalDiskEnumeration();        //!< Private constructor (intentionally not implemented)
        SCXCoreLib::SCXHandle<StaticPhysicalDiskInstance> AddDiskInstance(const std::wstring& name, const std::wstring& device);

        SCXCoreLib::SCXHandle<DiskDepend> m_deps; //!< Disk dependency object used for dependency injection.
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle
    };

} /* namespace SCXSystemLib */

#endif /* STATICPHYSICALDISKENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
