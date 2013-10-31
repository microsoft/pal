/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines the statistical disk information instance PAL for physical disks.
    
    \date        2008-04-28 15:20:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATISTICALPHYSICALDISKINSTANCE_H
#define STATISTICALPHYSICALDISKINSTANCE_H

#include <scxsystemlib/statisticaldiskinstance.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Represents a single statistical physical disk instance.
    */
    class StatisticalPhysicalDiskInstance : public StatisticalDiskInstance
    {
        friend class StatisticalPhysicalDiskEnumeration;
    public:
        StatisticalPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal = false);
        virtual ~StatisticalPhysicalDiskInstance()
        {
            m_currentInstancesCount--;
        }

        virtual bool GetReadsPerSecond(scxulong& value) const;
        virtual bool GetWritesPerSecond(scxulong& value) const;
        virtual bool GetBytesPerSecond(scxulong& read, scxulong& write) const;
        virtual bool GetDiskSize(scxulong& mbUsed, scxulong& mbFree) const;
        virtual bool GetBlockSize(scxulong& blockSize) const;

        virtual void Sample();

        virtual bool GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const;

        /*----------------------------------------------------------------------------*/
        /**
           Test aid. Gets the number of instances that currently exist.
        
           \returns     number of instances currently in existance.
        */
        static size_t GetCurrentInstancesCount()
        {
            return m_currentInstancesCount;
        }
        /*----------------------------------------------------------------------------*/
        /**
           Test aid. Gets the number of instances created since the module was started and static varables
           were initialized.
        
           \returns     number of instances created since the start of the module.
        */
        static size_t GetInstancesCountSinceModuleStart()
        {
            return m_instancesCountSinceModuleStart;
        }
    private:
        // For testing purposes we count the number of instances currently in existance.
        static size_t m_currentInstancesCount;
        // For testing purposes we count the number of instances created since the module was started and static varables
        // were initialized.
        static size_t m_instancesCountSinceModuleStart;
        // Don't allow any compiler-generated constructors since those wouldn't update static counters.
        // Default private constructor may not be necessary since we already have a constructor for this class, but we
        // have to deal with a dozen platforms with old compilers that may not be fully compliant.
        // So just in case we'll add one.
        StatisticalPhysicalDiskInstance();
        StatisticalPhysicalDiskInstance(const StatisticalPhysicalDiskInstance&);
    };
}
#endif /* STATISTICALPHYSICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
