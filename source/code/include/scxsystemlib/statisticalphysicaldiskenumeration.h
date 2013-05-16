/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Defines the statistical disk information enumeration PAL for physical disks.
    
    \date        2008-04-28 15:20:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATISTICALPHYSICALDISKENUMERATION_H
#define STATISTICALPHYSICALDISKENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/statisticalphysicaldiskinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxhandle.h>
#include <scxsystemlib/diskdepend.h>
#include <map>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /** 
         Represents a set of discovered physical disks and their statistical data 
         on a system.
    
         Will start a sample thread to sample disk statistics when initiated.
    */
    class StatisticalPhysicalDiskEnumeration : public EntityEnumeration<StatisticalPhysicalDiskInstance>
    {
    public:
        StatisticalPhysicalDiskEnumeration(SCXCoreLib::SCXHandle<DiskDepend> deps);
        virtual ~StatisticalPhysicalDiskEnumeration();

        SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> FindDiskByDevice(const std::wstring& device, bool includeSamplerDevice = false);

        virtual void Init();
        virtual void CleanUp();
        virtual void Update(bool updateInstances);
        virtual void UpdateInstances();
        void InitInstances();
        void SampleDisks();

        // provide class-specific implementation to add locking
        bool RemoveInstanceById(const EntityInstanceId& id);

        virtual const std::wstring DumpString() const;

        static void DiskSampler(SCXCoreLib::SCXThreadParamHandle& param);

#if defined (sun)
    protected:
        virtual void UpdateSolarisHelper();
#endif

    private:
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle
        SCXCoreLib::SCXHandle<DiskDepend> m_deps; //!< Dependencies object
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> m_sampler;       //!< Data sampler.
        SCXCoreLib::SCXThreadLockHandle m_lock; //!< Handles locking in the disk enumeration.
        std::map<std::wstring,scxulong> m_pathToRdev; //!< Cache for path to rdev values.

        void FindPhysicalDisks();
        
        void UpdatePathToRdev(const std::wstring& dir);
        SCXCoreLib::SCXHandle<StatisticalPhysicalDiskInstance> AddDiskInstance(const std::wstring& name, const std::wstring& device);
    };

    /*----------------------------------------------------------------------------*/
    /**
        Parameters for the disk sampler thread keeping all DiskInstances up to date.
    */
    class StatisticalPhysicalDiskSamplerParam : public SCXCoreLib::SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor
        */
        StatisticalPhysicalDiskSamplerParam()
            : m_diskEnum(NULL)
        {}

        StatisticalPhysicalDiskEnumeration* m_diskEnum;  //!< Pointer to the disk enumeration associated with the thread.
    };

}
#endif /* STATISTICALPHYSICALDISKENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
